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

void main() {
    //计算从当前片段到光源的距离。使用 dot(fragOffset, fragOffset) 得到 fragOffset 的平方，再通过 sqrt 取平方根，得到实际距离。
    float dis = sqrt(dot(fragOffset, fragOffset));

    //定义光源的有效范围，距离光源超过 1.0 的部分不被渲染，把正方形光源变为圆形
    if (dis >= 1.0) {
        discard;
    }
    outColor = vec4(push.color.xyz, 1.0);
}
