#version 450

const vec2 OFFSETS[6] = vec2[](
  vec2(-1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, -1.0),
  vec2(1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, 1.0)
);

layout (location = 0) out vec2 fragOffset;

struct PointLight {
  vec4 position; // ignore w
  vec4 color; // w 是强度
};

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  vec4 ambientLightColor; // w is intensity
  PointLight pointLights[10];
  int numLights;
} ubo;

layout(push_constant) uniform Push {
  vec4 position;
  vec4 color;
  float radius;
} push;


void main() {
    //将当前顶点的偏移量从 OFFSETS 数组中提取出来。gl_VertexIndex 是当前正在处理的顶点的索引。
    fragOffset = OFFSETS[gl_VertexIndex];

    //从视图矩阵中提取摄像机的右方向和上方向，这两个向量用于计算光源在世界空间中的精确位置。
    vec3 cameraRightWorld = {ubo.view[0][0], ubo.view[1][0], ubo.view[2][0]};
    vec3 cameraUpWorld = {ubo.view[0][1], ubo.view[1][1], ubo.view[2][1]};

    //计算光源在世界空间中的位置，公式中包含光源的基本位置 ubo.lightPosition 和基于偏移量以及 LIGHT_RADIUS 计算的偏移量。
    //它通过将 fragOffset 乘以摄像机的方向向量（右向量和上向量）来形成最终的位置。
    //vec3 positionWorld = ubo.lightPosition.xyz
    //  + LIGHT_RADIUS * fragOffset.x * cameraRightWorld
    //  + LIGHT_RADIUS * fragOffset.y * cameraUpWorld;
    vec3 positionWorld = push.position.xyz
        + push.radius * fragOffset.x * cameraRightWorld
        + push.radius * fragOffset.y * cameraUpWorld;

    //转换为裁剪空间坐标
    gl_Position = ubo.projection * ubo.view * vec4(positionWorld, 1.0);
}

/*
在 Vulkan 中，一个管道的渲染结果不会直接作为下一个管道的输入，因为 Vulkan 是一种以命令为基础的 API，而不是基于图形级别的状态机。每个管道的渲染过程是独立的，通常通过帧缓冲对象（Framebuffer）和渲染通道（Render Pass）来管理输出数据。下面详细解释这个过程以及如何在多个管道之间传递数据。

### 1. 渲染管道的独立性

- **渲染管道的独立性**：每个渲染管道的输出（最终在屏幕上显示的像素）是独立的，并且与其他管道的输入无关。管道的输出通常被写入到渲染通道（Render Pass）中定义的帧缓冲区（Framebuffer）。
  
- **命令缓冲区的使用**：你在调用 `simpleRenderSystem.render(frameInfo)` 和 `pointLightSystem.render(frameInfo)` 时，每个系统都会在其各自的命令缓冲区中记录其渲染操作。这些操作在实际执行时会逐个提交给 GPU。

### 2. 如何在两个管道之间传递数据

虽然一个管道的渲染结果不会直接传递给另一个管道，但可以通过以下方式间接实现数据共享：

- **帧缓冲区（Framebuffer）**：
  - 第一个管道的输出（如场景的颜色、深度信息等）可以渲染到一个帧缓冲区中的颜色附加（color attachment）或深度附加（depth attachment）上。
  - 第二个管道则可以在其渲染过程中使用这个帧缓冲区的内容，通常通过一个全屏四边形（Fullscreen quad）或者其他几何体来读取和处理渲染结果。

- **纹理（Texture）**：
  - 第一个管道可以将渲染结果存储到一个纹理中。然后在第二个管道中，你可以绑定这个纹理作为输入的描述符集，着色器将能够访问渲染的图像数据。
  - 这种方法在生成后处理效果（如光照计算、阴影映射、模糊等）时特别有用。

### 3. 示例流程

假设你想实现一个光照效果，流程可能如下：

1. **使用第一个管道渲染场景**：
   ```cpp
   // 渲染场景到颜色缓冲区
   lveRenderer.beginSwapChainRenderPass(commandBuffer);
   simpleRenderSystem.renderGameObjects(frameInfo); // 渲染场景
   lveRenderer.endSwapChainRenderPass(commandBuffer);
   ```

2. **将颜色缓冲区的内容提取为纹理**（在后台处理）：
   - 你需要确保没有销毁这个颜色缓冲区，或者将其内容拷贝到一个可用的纹理上。

3. **在第二个管道中使用这个纹理进行光照计算**：
   ```cpp
   pointLightSystem.render(frameInfo); // 使用之前渲染的结果，进行光照计算
   ```

### 4. 总结

在 Vulkan 中，虽然一个管道的输出不直接用于下一个管道，但可以通过帧缓冲区和纹理等方式进行间接传递。这样的方式允许在图形渲染的各个阶段之间共享数据，实现更复杂的视觉效果，如光照、阴影和后期处理等。这种设计提供了高度的灵活性，同时也保持了管道的独立性。

*/