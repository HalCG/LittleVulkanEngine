#version 450

// 网格物体顶点着色器：GlobalUbo(set0,binding0) + push model/normal matrix
// location 必须与 LVEModel::Vertex 属性描述一致

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

// 输出到片段着色器，光栅化阶段自动插值
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;

struct PointLight {
  vec4 position; // ignore w
  vec4 color; // w 是强度
};

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  mat4 invView;
  vec4 ambientLightColor;	// 环境光，w 为强度
  PointLight pointLights[10];
  int numLights;
} ubo;

layout(push_constant) uniform PushConstantData {
	mat4 modelMatrix;
	mat4 normalMatrix;
} pushConstantData;

void main(){
	vec4 positionWorld = pushConstantData.modelMatrix * vec4(position, 1.0);
	gl_Position = ubo.projection * ubo.view * positionWorld;

	fragNormalWorld = normalize(mat3(pushConstantData.normalMatrix) * normal);
	fragPosWorld = positionWorld.xyz;
	fragColor = color;
}
