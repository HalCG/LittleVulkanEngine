#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projectionViewMatrix;
  vec4 ambientLightColor;	// �����⣬w ������ʾ��ǿ
  vec3 lightPosition;
  vec4 lightColor;			//���Դ����ɫ������һ�����ڱ�ʾ��ǿ�� w ������
} ubo;

layout(push_constant) uniform PushConstantData{//ÿ����ɫ����ڵ�ֻ��ʹ��һ���������Ϳ�
	mat4 modelMatrix;
	mat4 normalMatrix;
}pushConstantData;

void main(){
	vec4 positionWorld = pushConstantData.modelMatrix * vec4(position, 1.0);
	gl_Position = ubo.projectionViewMatrix * positionWorld;

	vec3 normalWorldSpace = normalize(mat3(pushConstantData.normalMatrix) * normal);

	//���㵽��Դ�ķ��򡢹���˥��ֵ�����ڹ�Դλ�úͶ���λ�õľ��룩�Լ���Դ��ɫ��
	vec3 directionToLight = ubo.lightPosition - positionWorld.xyz;
	float attenuation = 1.0 / dot(directionToLight, directionToLight); // ˥�����ӣ�����ƽ���ĵ�����1.0 /�������ž�������ӣ�attenuation ��ֵ����٣��Ӷ�ģ���Դ��˥������ԴԽԶ����ǿԽ������

	//�����Դ����ɫ����ǿ�ȣ������ǵ���Դ�붥��֮���˥����������˥����
	vec3 lightColor = ubo.lightColor.xyz * ubo.lightColor.w * attenuation;//��ɫ * ǿ�� * ˥������

	//����������⣬ʹ�÷������Դ����֮��ĵ����ȷ����Ϊ��ֵ��
	vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
	vec3 diffuseLight = lightColor * max(dot(normalWorldSpace, normalize(directionToLight)), 0);

	//��������⡢�������붥����ɫ���н�ϣ��õ����յ�Ƭ����ɫ�����
	fragColor = (diffuseLight + ambientLight) * color;
	}