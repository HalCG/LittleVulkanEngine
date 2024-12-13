#version 450

layout (location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstantData{//每个着色器入口点只能使用一个常量推送块
	mat4 transform;
	vec3 color;
}pushConstantData;


void main(){
	outColor = vec4(fragColor, 1.0);
}