#version 450

layout (location = 0) in vec2 fragOffset;//�����������Ӷ�����ɫ�����ݹ�������ʾ��ǰƬ������ڹ�Դλ�õ�ƫ����������Ϊ vec2��
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
    //����ӵ�ǰƬ�ε���Դ�ľ��롣ʹ�� dot(fragOffset, fragOffset) �õ� fragOffset ��ƽ������ͨ�� sqrt ȡƽ�������õ�ʵ�ʾ��롣
    float dis = sqrt(dot(fragOffset, fragOffset));

    //�����Դ����Ч��Χ�������Դ���� 1.0 �Ĳ��ֲ�����Ⱦ���������ι�Դ��ΪԲ��
    if (dis >= 1.0) {
        discard;
    }
    outColor = vec4(push.color.xyz, 1.0);
}
