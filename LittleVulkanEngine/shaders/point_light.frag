#version 450

// 点光圆盘：按 fragOffset 距离 discard 成圆，并用 alpha 做柔边（需管线 alpha blend）

layout (location = 0) in vec2 fragOffset;
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
    float dis = sqrt(dot(fragOffset, fragOffset));

    // 单位圆外丢弃，把正方形 billboard 裁成圆
    if (dis >= 1.0) {
        discard;
    }

    // cos 衰减：中心 1 → 边缘 0，用作亮度与 alpha
    float cosDis = 0.5 * (cos(dis * M_PI) + 1.0);
    outColor = vec4(push.color.xyz + 0.5 * cosDis, cosDis);
}
