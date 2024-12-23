#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projectionViewMatrix;
  vec4 ambientLightColor;	// 环境光，w 分量表示光强
  vec3 lightPosition;
  vec4 lightColor;			//点光源的颜色，包括一个用于表示光强的 w 分量。
} ubo;

layout(push_constant) uniform PushConstantData{//每个着色器入口点只能使用一个常量推送块
	mat4 modelMatrix;
	mat4 normalMatrix;
}pushConstantData;

void main(){
	vec4 positionWorld = pushConstantData.modelMatrix * vec4(position, 1.0);
	gl_Position = ubo.projectionViewMatrix * positionWorld;

	vec3 normalWorldSpace = normalize(mat3(pushConstantData.normalMatrix) * normal);

	//计算到光源的方向、光照衰减值（基于光源位置和顶点位置的距离）以及光源颜色。
	vec3 directionToLight = ubo.lightPosition - positionWorld.xyz;
	float attenuation = 1.0 / dot(directionToLight, directionToLight); // 衰减因子：距离平方的倒数（1.0 /）。随着距离的增加，attenuation 的值会减少，从而模拟光源的衰减（光源越远，光强越弱）。

	//计算光源的颜色及其强度，并考虑到光源与顶点之间的衰减（即距离衰减）
	vec3 lightColor = ubo.lightColor.xyz * ubo.lightColor.w * attenuation;//颜色 * 强度 * 衰减银子

	//计算漫反射光，使用法线与光源方向之间的点积，确保不为负值。
	vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
	vec3 diffuseLight = lightColor * max(dot(normalWorldSpace, normalize(directionToLight)), 0);

	//将漫反射光、环境光与顶点颜色进行结合，得到最终的片段颜色输出。
	fragColor = (diffuseLight + ambientLight) * color;
	}