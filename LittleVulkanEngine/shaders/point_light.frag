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

const float M_PI = 3.1415926538;

void main() {
    //����ӵ�ǰƬ�ε���Դ�ľ��롣ʹ�� dot(fragOffset, fragOffset) �õ� fragOffset ��ƽ������ͨ�� sqrt ȡƽ�������õ�ʵ�ʾ��롣
    float dis = sqrt(dot(fragOffset, fragOffset));

    //�����Դ����Ч��Χ�������Դ���� 1.0 �Ĳ��ֲ�����Ⱦ���������ι�Դ��ΪԲ��
    if (dis >= 1.0) {
        discard;
    }
    
    float cosDis = 0.5 * (cos(dis * M_PI) + 1.0); // ranges from 1 -> 0
    outColor = vec4(push.color.xyz + 0.5 * cosDis, cosDis);
}


/*
### 1. ��ɫ��ֵ
```glsl
float cosDis = 0.5 * (cos(dis * M_PI) + 1.0); // ranges from 1 -> 0
```
- **`cos(dis * M_PI)`**�����㵱ǰ���� `dis` ������ֵ������ \( \pi \) ʹ�õ� `dis` Ϊ 0 ʱ�����ҵ�ֵΪ 1������ `dis` Ϊ 1 ʱ�����ҵ�ֵΪ -1��
- **������ʽ**��������ֵƽ�ƺ����ţ�ʹ�䷶Χ�� 1���� `dis=0`���� 0���� `dis=1`������ʵ����һ������ǿ�ȵ���ǿ�Ƚ����Ч����

### 2. �����ɫ
```glsl
outColor = vec4(push.color.xyz + 0.5 * cosDis, cosDis);
```
- **`push.color.xyz`**����ʾ�������ɫ��ֻ�� RGB ������
- **`0.5 * cosDis`**��������õ��Ľ���ֵ `cosDis` ���� 0.5���������Խ���ɫ��ǿ�ȵ���Ϊһ���µķ�Χ��
- **`outColor`**������һ����ά������RGBA�������У�
  - RGB ������ԭʼ��ɫ�ͽ���ǿ�ȵ���ϡ�
  - A ������alpha���� `cosDis` ���������ڿ��Ƹ�Ƭ�ε�͸���ȡ�`cosDis` Խ�ӽ� 0����ʾԽ͸����Խ�ӽ� 1����ʾԽ��͸����
*/