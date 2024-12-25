#version 450

layout (location = 0) in vec2 fragOffset;//这个输入变量从顶点着色器传递过来，表示当前片段相对于光源位置的偏移量，类型为 vec2。
layout (location = 0) out vec4 outColor;

struct PointLight {
  vec4 position; // ignore w
  vec4 color; // w is intensity
};

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  mat4 invView;
  vec4 ambientLightColor;
  PointLight pointLights[10];
  int numLights;
} ubo;

layout(push_constant) uniform Push {
  vec4 position;
  vec4 color;
  float radius;
} push;

const float M_PI = 3.1415926538;

void main() {
    //计算从当前片段到光源的距离。使用 dot(fragOffset, fragOffset) 得到 fragOffset 的平方，再通过 sqrt 取平方根，得到实际距离。
    float dis = sqrt(dot(fragOffset, fragOffset));

    //定义光源的有效范围，距离光源超过 1.0 的部分不被渲染，把正方形光源变为圆形
    if (dis >= 1.0) {
        discard;
    }
    
    float cosDis = 0.5 * (cos(dis * M_PI) + 1.0); // ranges from 1 -> 0
    outColor = vec4(push.color.xyz + 0.5 * cosDis, cosDis);
}


/*
### 1. 颜色插值
```glsl
float cosDis = 0.5 * (cos(dis * M_PI) + 1.0); // ranges from 1 -> 0
```
- **`cos(dis * M_PI)`**：计算当前距离 `dis` 的余弦值，乘以 \( \pi \) 使得当 `dis` 为 0 时，余弦的值为 1，而当 `dis` 为 1 时，余弦的值为 -1。
- **整体表达式**：将余弦值平移和缩放，使其范围从 1（当 `dis=0`）到 0（当 `dis=1`），即实现了一个从满强度到无强度渐变的效果。

### 2. 输出颜色
```glsl
outColor = vec4(push.color.xyz + 0.5 * cosDis, cosDis);
```
- **`push.color.xyz`**：表示传入的颜色，只有 RGB 分量。
- **`0.5 * cosDis`**：将计算得到的渐变值 `cosDis` 乘以 0.5，这样可以将颜色的强度调整为一个新的范围。
- **`outColor`**：生成一个四维向量（RGBA），其中：
  - RGB 分量是原始颜色和渐变强度的组合。
  - A 分量（alpha）由 `cosDis` 决定，用于控制该片段的透明度。`cosDis` 越接近 0，表示越透明；越接近 1，表示越不透明。
*/