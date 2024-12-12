#version 450

layout(location = 0) in vec2 position;
//layout(location = 1) in vec3 color;

//layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstantData{//每个着色器入口点只能使用一个常量推送块
	mat2 transform;
	vec2 offset;
	vec3 color;
}pushConstantData;

void main(){
	gl_Position = vec4(pushConstantData.transform * position + pushConstantData.offset, 0.0, 1);
	//fragColor = pushConstantData.color;//也可以把推送常量放入片段着色器
}