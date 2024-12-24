#version 450

layout (location = 0) in vec2 fragOffset;//�����������Ӷ�����ɫ�����ݹ�������ʾ��ǰƬ������ڹ�Դλ�õ�ƫ����������Ϊ vec2��
layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  vec4 ambientLightColor;
  vec3 lightPosition;
  vec4 lightColor;
} ubo;

void main() {
    //����ӵ�ǰƬ�ε���Դ�ľ��롣ʹ�� dot(fragOffset, fragOffset) �õ� fragOffset ��ƽ������ͨ�� sqrt ȡƽ�������õ�ʵ�ʾ��롣
    float dis = sqrt(dot(fragOffset, fragOffset));

    //�����Դ����Ч��Χ�������Դ���� 1.0 �Ĳ��ֲ�����Ⱦ
    if (dis >= 1.0) {
        discard;
    }
    outColor = vec4(ubo.lightColor.xyz, 1.0);
}
