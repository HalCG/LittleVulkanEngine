#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstantData{//每个着色器入口点只能使用一个常量推送块
	mat4 transform;
	vec3 color;
}pushConstantData;

void main(){
	gl_Position = pushConstantData.transform * vec4(position, 1.0);
	fragColor = color;//也可以把推送常量放入片段着色器
}