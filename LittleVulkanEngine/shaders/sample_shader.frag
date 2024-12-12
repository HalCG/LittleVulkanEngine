#version 450

//layout(location = 0) in vec3 fragColor;
layout(push_constant) uniform PushConstantData{//每个着色器入口点只能使用一个常量推送块
	mat2 transform;
	vec2 offset;
	vec3 color;
}pushConstantData;

layout(location = 0) out vec4 outColor;

void main(){
	outColor = vec4(pushConstantData.color, 1.0);
}