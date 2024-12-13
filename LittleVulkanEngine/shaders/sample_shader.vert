#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstantData{//ÿ����ɫ����ڵ�ֻ��ʹ��һ���������Ϳ�
	mat4 transform;
	vec3 color;
}pushConstantData;

void main(){
	gl_Position = pushConstantData.transform * vec4(position, 1.0);
	fragColor = color;//Ҳ���԰����ͳ�������Ƭ����ɫ��
}