# 排查笔记：Swapchain Semaphore 复用错误导致整机卡顿

本文记录一次真实运行问题：画面能画、程序也「在跑」，但校验层刷屏，整台电脑几乎卡死。  
对应修复在 `LittleVulkanEngine/lve_swap_chain.h` / `.cpp`。

官方参考：[Swapchain Semaphore Reuse](https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html)

---

## 1. 现象（用户侧）

启动日志大致如下：

```text
Device count: 1
physical device: NVIDIA GeForce RTX 4060 Ti
Present mode: Mailbox
validation layer: vkQueueSubmit(): pSubmits[0].pSignalSemaphores[0] (VkSemaphore ...)
  is being signaled by VkQueue ..., but it may still be in use by VkSwapchainKHR ...
Most recently acquired image indices: [0], 1, 2.
Swapchain image 0 was presented but was not re-acquired,
  so VkSemaphore ... may still be in use and cannot be safely reused with image index 2.
...
libpng warning: iCCP: known incorrect sRGB profile
```

主观感受：

| 观察 | 含义 |
|------|------|
| 窗口里场景正常渲染 | 主路径（录制 / submit / present）基本通了 |
| validation 反复刷同类错误 | 同步对象生命周期有问题，不是偶发 |
| 电脑整体卡顿 / 几乎冻住 | 不只是「控制台吵」，可能拖垮 GPU/驱动调度 |
| Present mode = Mailbox | 常见为 **3 张** swap 图；比 FIFO 更容易踩「帧槽数 < 图像数」的坑 |
| `libpng iCCP` | 纹理/图标色彩配置警告，**与卡顿无关**，可忽略 |

结论先记住一句：**「能画 ≠ 同步正确」**。校验层报的是真 bug。

---

## 2. 怎么发现问题的（排查路径）

### 2.1 不先猜「驱动坏了」，先读 validation 原文

关键句拆开：

1. **`pSignalSemaphores[0] is being signaled ... but it may still be in use by VkSwapchainKHR`**  
   → 你要对某个 binary semaphore 再 signal，但 present 路径可能还握着它。

2. **`Most recently acquired image indices: [0], 1, 2`**  
   → 交换链至少在轮转 0→1→2；括号标出「这个 semaphore 上次是跟着哪张图 present 的」。

3. **`Swapchain image 0 was presented but was not re-acquired`**  
   → 图 0 已经 present 出去了，还没再次 acquire；与图 0 绑定的那个 present-wait semaphore **还不能复用**。

4. **Hint 直接点名两种修法**  
   - (a) **每张 swap 图一个 semaphore，用 `imageIndex` 索引**  
   - (b) 用 `VK_KHR_swapchain_maintenance1`，present 配 Fence  

规范条目：`VUID-vkQueueSubmit-pSignalSemaphores-00067`（signal 时 binary semaphore 必须未处于 signaled / 仍被占用状态）。

→ 问题域立刻收窄到：**present 用的那个 signal semaphore 的复用策略**，而不是 shader、模型路径、descriptor。

### 2.2 对照本工程同步对象数量

打开 `LVESwapChain`，旧写法（Brendan Galea / 多数早期教程）是：

```text
MAX_FRAMES_IN_FLIGHT = 2

imageAvailableSemaphores[2]   // 按 currentFrame
renderFinishedSemaphores[2]   // 按 currentFrame  ← 问题点
inFlightFences[2]             // 按 currentFrame
imagesInFlight[imageCount]    // 按 imageIndex
```

同时本机选择了 **Mailbox**，`imageCount` 通常是 **3**。

于是出现经典不匹配：

```text
CPU 帧槽：  0, 1, 0, 1, 0, 1, ...
swap 图像： 0, 1, 2, 0, 1, 2, ...
```

`renderFinished[frame]` 只有 2 个，却要服务 3 张图的 present。

### 2.3 用时间线推「为什么第 3 次 submit 必炸」

简化一轮（数字是示意）：

| 时刻 | currentFrame | 取得的 imageIndex | signal / present 用的 semaphore | 问题？ |
|------|--------------|-------------------|----------------------------------|--------|
| T0 | 0 | 0 | `renderFinished[0]` | 正常 |
| T1 | 1 | 1 | `renderFinished[1]` | 正常 |
| T2 | 0 | 2 | **又用 `renderFinished[0]`** | 图 0 往往仍未 re-acquire，`renderFinished[0]` 仍可能被 present 占用 → **违规 signal** |

校验层日志里的「acquired … `[0], 1, 2`」和「reuse with image index 2」正是这条时间线。

### 2.4 为什么会「把电脑卡住」

- 校验层在 Debug 下对每次违规做检查与日志，成本已经不低。  
- 更关键：对 binary semaphore **错误复用** 属于未定义行为；驱动侧队列可能长时间等待 / 重试，Mailbox 又尽量塞满显示管道，表现为 **整机卡顿**，而不只是窗口掉帧。  
- 所以：卡顿是症状放大，根因仍是同步错误。

### 2.5 排除项（避免走弯路）

| 嫌疑 | 为何先排除 |
|------|------------|
| `libpng iCCP` | 仅色彩配置文件警告，与 Vulkan 队列无关 |
| shader / 模型路径错误 | 那类问题通常黑屏或启动直接失败，不会稳定刷「semaphore still in use」 |
| device layer `enabledLayerCount` | 设备层早已废弃；修的是另一类 validation，不是本条 |
| 「Mailbox 本身有 bug」 | Mailbox 合法；它只是更容易暴露「2 帧槽 × 3 图像」的教程写法缺陷 |

---

## 3. 根因（一句话）

> **Present 会占用 `queueSubmit` 里 signal、并在 `queuePresent` 里 wait 的那个 binary semaphore，直到对应的 swapchain image 再次被 acquire。**  
> 旧教程按 `currentFrame`（飞行帧数）复用 `renderFinished`；当 `imageCount > MAX_FRAMES_IN_FLIGHT` 时，会在「图尚未 re-acquire」时再次 signal 同一 semaphore → 违反规范。

两个索引必须分清：

| 索引 | 本工程用途 |
|------|------------|
| `currentFrame`（0..MAX_FRAMES_IN_FLIGHT-1） | `imageAvailable`、fence、command buffer、每帧 UBO |
| `imageIndex`（0..imageCount-1） | Framebuffer、**以及 `renderFinished`（给 present 用）** |

---

## 4. 解决思路

### 4.1 采用官方建议 (a)：每图一个 present semaphore

不引入新扩展，改动面最小、与现有 fence 逻辑兼容：

1. `renderFinishedSemaphores.resize(imageCount())`  
2. `submit` 的 `pSignalSemaphores` → `renderFinishedSemaphores[*imageIndex]`  
3. `present` 的 `pWaitSemaphores` → 同一个（本工程里仍是上面那个数组元素）  
4. 析构时按 `renderFinishedSemaphores.size()` 销毁，而不是写死 `MAX_FRAMES_IN_FLIGHT`

`imageAvailable` / `inFlightFences` **仍可按帧槽**：因为在再次使用同一帧槽前，会先 `vkWaitForFences(inFlightFences[currentFrame])`，保证上一轮「wait 了该 imageAvailable 的 submit」已完成。

### 4.2 为何这样安全

当再次 `acquire` 到某个 `imageIndex = i` 时，驱动保证：此前对图 `i` 的 present 已结束。  
因此绑定给图 `i` 的 `renderFinished[i]` 此时可以再次被 signal。  
这与「按帧槽轮转」脱钩，天然适配 2、3、甚至更多 swap 图像。

### 4.3 备选方案 (b)（本工程未采用）

`VK_KHR_swapchain_maintenance1` 允许给 present 挂 Fence，用另一种方式跟踪「present 是否结束」。  
对教程工程来说，(a) 更直观，也更好对照 validation hint。

### 4.4 代码落点（修后）

`createSyncObjects`：

```cpp
imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
renderFinishedSemaphores.resize(imageCount());  // 按图，不是按帧
// ...
for (frame)  create imageAvailable + fence;
for (image)  create renderFinished;
```

`submitCommandBuffers`：

```cpp
VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[*imageIndex] };
// submit signal → present wait 共用
```

---

## 5. 验证方式

1. Debug 构建，开 validation layer。  
2. 确认日志仍为 `Present mode: Mailbox`（或任意 `imageCount >= 3`）。  
3. 跑几十秒：  
   - **不应再出现** `pSignalSemaphores ... may still be in use by VkSwapchainKHR`  
   - 系统交互应恢复正常，不再整机卡死  
4. 可选：resize 窗口触发 swapchain 重建，确认新链按新的 `imageCount` 重新创建 per-image semaphore。

---

## 6. 可复用的排查清单（以后同类问题）

1. **先完整读 validation 第一段 + Hint**，往往已经给出修法。  
2. **数一数**：飞行帧数 vs `imageCount` vs 每种 semaphore 的数组长度。  
3. **分清索引**：谁按 `frame`，谁必须按 `imageIndex`（凡进 `queuePresent` wait 的 binary semaphore，优先怀疑要按图）。  
4. **能画不能证明同步对**：卡顿 + semaphore VUID → 优先查同步，而不是美术资源。  
5. 改完用同一 Present mode 复现路径验证，避免「改成 FIFO 碰巧不炸」当成修好。

---

## 7. 相关文件

| 文件 | 角色 |
|------|------|
| `LittleVulkanEngine/lve_swap_chain.h` | 同步对象声明与注释 |
| `LittleVulkanEngine/lve_swap_chain.cpp` | `createSyncObjects` / submit / present / 析构 |
| `docs/CODE_GUIDE.md` §4.2–4.3 | 帧槽 vs imageIndex 概念导读 |
