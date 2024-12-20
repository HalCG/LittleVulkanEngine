#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projectionViewMatrix;
  vec3 directionToLight;
} ubo;

layout(push_constant) uniform PushConstantData{//每个着色器入口点只能使用一个常量推送块
	mat4 modelMatrix;
	mat4 normalMatrix;
}pushConstantData;

const float AMBIENT = 0.02;

void main(){
	gl_Position = ubo.projectionViewMatrix * pushConstantData.modelMatrix * vec4(position, 1.0);
	vec3 normalWorldSpace = normalize(mat3(pushConstantData.normalMatrix) * normal);
	float lightIntensity = AMBIENT + max(dot(normalWorldSpace, ubo.directionToLight), 0);
	fragColor = lightIntensity * color;
}