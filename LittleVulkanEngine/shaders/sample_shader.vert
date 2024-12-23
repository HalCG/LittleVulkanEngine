#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

//��������ɫ��ִ�����OpenGL���Զ��ڶ���֮���ֵ��Щ���������
//����OpenGLͨ��ʹ�����Բ�ֵ��fragNormalWorld���������ݶ����Ȩ�أ����������ڵ�ǰƬ���еı��أ����в�ֵ��
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projectionViewMatrix;
  vec4 ambientLightColor;	// �����⣬w ������ʾ��ǿ
  vec3 lightPosition;
  vec4 lightColor;			//���Դ��w ������ʾ��ǿ
} ubo;

layout(push_constant) uniform PushConstantData{//ÿ����ɫ����ڵ�ֻ��ʹ��һ���������Ϳ�
	mat4 modelMatrix;
	mat4 normalMatrix;
}pushConstantData;

void main(){
		vec4 positionWorld = pushConstantData.modelMatrix * vec4(position, 1.0);
		gl_Position = ubo.projectionViewMatrix * positionWorld;


		fragNormalWorld = normalize(mat3(pushConstantData.normalMatrix) * normal);
		fragPosWorld = positionWorld.xyz;
		fragColor = color;
	}

/*
��OpenGL��Ⱦ�����У�������ɫ����Ƭ����ɫ��֮������ݴ���ͨ����ͨ����ֵʵ�ֵġ������ṩ�Ĵ���Ƭ���У�����������δӶ�����ɫ�����ݵ�Ƭ����ɫ���ģ���Ҫ��ͨ��**varying**�����������ִ�OpenGL�г�Ϊ**out**����������ɵġ�

### ��ֵ����

1. **��������**��
   �ڶ�����ɫ���У�������Ҫ����һ���������������`fragNormalWorld`��������������Ƭ����ɫ��֮������ӡ���ͨ����ʹ�� `out` �ؼ��֣�

   ```glsl
   // ������ɫ��
   out vec3 fragNormalWorld;
   ```

2. **����������**��
   �ڶ�����ɫ���У�����㷨�������������꣬�����丳ֵ�����������������磬��Ĵ������������У�

   ```glsl
   fragNormalWorld = normalize(mat3(pushConstantData.normalMatrix) * normal);
   ```

   ȷ�����Ѿ��� `normal` �����ݴ��ݸ�������ɫ���������ģ�͵ķ������任����ȡ����ռ��еķ�������

3. **Ĭ�ϲ�ֵ**��
   ��������ɫ��ִ�����OpenGL���Զ��ڶ���֮���ֵ��Щ�����������Ƭ����ɫ���У�����Ҫ�����붥����ɫ�����Ӧ���������������`fragNormalWorld`����ʹ�� `in` �ؼ��֣�

   ```glsl
   // Ƭ����ɫ��
   in vec3 fragNormalWorld;
   ```

4. **ʹ�ò�ֵ��ķ�����**��
   ��Ƭ����ɫ���У����յ��� `fragNormalWorld` ����ͨ��ͼ�ι����в�ֵ�����õ��ġ�����OpenGLͨ��ʹ�����Բ�ֵ��`fragNormalWorld`���������ݶ����Ȩ�أ����������ڵ�ǰƬ���еı��أ����в�ֵ��

### ����ʾ��

������һ���򵥵�ʾ����չʾ����ڶ�����ɫ����Ƭ����ɫ����ʵ�����ֲ�ֵ��

```glsl
// ������ɫ��
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;

out vec3 fragNormalWorld;
out vec3 fragPosWorld;
out vec4 fragColor;

uniform mat4 modelMatrix;
uniform mat4 projectionViewMatrix;
uniform mat4 normalMatrix;

void main() {
    vec4 positionWorld = modelMatrix * vec4(position, 1.0);
    gl_Position = projectionViewMatrix * positionWorld;

    fragNormalWorld = normalize(mat3(normalMatrix) * normal);
    fragPosWorld = positionWorld.xyz;
    fragColor = color;
}

// Ƭ����ɫ��
#version 330 core

in vec3 fragNormalWorld;
in vec3 fragPosWorld;
in vec4 fragColor;

out vec4 color;

void main() {
    // ʹ�ò�ֵ��ķ��߽��й��ռ���
    // ...
    color = fragColor; // ���߽�����������
}
```

### �ܽ�

����������ͨ���ڶ�����ɫ���ж��������������Ƭ����ɫ���ж����Ӧ�����������OpenGL���Զ����в�ֵ����ʹ�÷����������������ԣ��ܹ��ڶ�����ɫ����Ƭ����ɫ��֮��˳�����ݡ�ϣ��������Ͷ�������ֵ������������������㻹�������������Ҫ�������ϸ�ڣ���ӭ�������ۡ�

*/