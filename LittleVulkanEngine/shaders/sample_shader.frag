#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;

layout (location = 0) out vec4 outColor;

struct PointLight {
  vec4 position; // ignore w
  vec4 color; // w 是强度
};

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  mat4 invView;
  vec4 ambientLightColor; // w 是强度
  PointLight pointLights[10];
  int numLights;
} ubo;

layout(push_constant) uniform PushConstantData{//每个着色器入口点只能使用一个常量推送块
	mat4 modelMatrix;
	mat4 normalMatrix;
}pushConstantData;


void main(){
/*
  vec3 directionToLight = ubo.lightPosition - fragPosWorld;
  float attenuation = 1.0 / dot(directionToLight, directionToLight); // distance squared
  vec3 lightColor = ubo.lightColor.xyz * ubo.lightColor.w * attenuation;
  vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
  vec3 diffuseLight = lightColor * max(dot(normalize(fragNormalWorld), normalize(directionToLight)), 0);
  outColor = vec4((diffuseLight + ambientLight) * fragColor, 1.0);
*/
  vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
  vec3 specularLight = vec3(0.0);                                       //初始化一个镜面光的变量，用于累加所有光源的镜面反射光的贡献。
  vec3 surfaceNormal = normalize(fragNormalWorld);

  vec3 cameraPosWorld = ubo.invView[3].xyz;                             //通过相机的逆，取到相机相对世界桌标的平移，得到相机世界坐标
  vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

  for (int i = 0; i < ubo.numLights; i++) {
    PointLight light = ubo.pointLights[i];
    vec3 directionToLight = light.position.xyz - fragPosWorld;
    float attenuation = 1.0 / dot(directionToLight, directionToLight);  // distance squared 计算衰减系数
    directionToLight = normalize(directionToLight);

    float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
    vec3 intensity = light.color.xyz * light.color.w * attenuation;     //计算当前光源的强度，将其颜色和强度相乘，并乘上衰减值，得到最终的光源强度。

    //漫反射
    diffuseLight += intensity * cosAngIncidence;

    // 镜面光
    vec3 halfAngle = normalize(directionToLight + viewDirection);
    float blinnTerm = dot(surfaceNormal, halfAngle);
    blinnTerm = clamp(blinnTerm, 0, 1);
    blinnTerm = pow(blinnTerm, 512.0); // higher values -> sharper highlight
    specularLight += intensity * blinnTerm;
  }

  outColor = vec4(diffuseLight * fragColor + specularLight * fragColor, 1.0);
}

/*
计算光照效果，包括环境光、漫反射光和镜面反射光在一个场景中的影响。我们逐行解析这段代码：

```glsl
vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
```
这行代码首先计算环境光（ambient light），`ubo.ambientLightColor` 是一个包含环境光颜色和强度的向量。`.xyz` 提取颜色部分，而 `.w` 通常表示强度，因此将环境光的颜色与强度相乘得到环境光的贡献。

```glsl
vec3 specularLight = vec3(0.0);
```
这行代码初始化一个镜面光的变量，用于累加所有光源的镜面反射光的贡献。

```glsl
vec3 surfaceNormal = normalize(fragNormalWorld);
```
这行代码将片段的法线向量 `fragNormalWorld` 进行归一化，以确保它的长度为 1。法线用于计算光照与表面之间的关系。

```glsl
vec3 cameraPosWorld = ubo.invView[3].xyz;
vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);
```
这里首先获取世界空间中的摄像机位置（`cameraPosWorld`），然后计算从当前片段位置到摄像机的视线方向（`viewDirection`），并进行归一化。

```glsl
for (int i = 0; i < ubo.numLights; i++) {
    PointLight light = ubo.pointLights[i];
    vec3 directionToLight = light.position.xyz - fragPosWorld;
    float attenuation = 1.0 / dot(directionToLight, directionToLight); // distance squared
    directionToLight = normalize(directionToLight);
```
这段代码是一个循环，遍历所有的点光源 `PointLight`。通过 `light.position` 获取光源的位置，并计算光源到当前片段位置的方向 `directionToLight`。然后通过计算`directionToLight`向量的平方长度来计算衰减系数，这里使用的是简单的平方反比衰减 `1 / (distance ^ 2)`。最后，对该方向向量进行归一化。

```glsl
    float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
```
这一行代码计算光线与表面法线之间的夹角的余弦值（即入射角），使用 `dot` 函数进行计算，并使用 `max` 来确保该值不小于 0。

```glsl
    vec3 intensity = light.color.xyz * light.color.w * attenuation;
```
这行代码计算当前光源的强度，将其颜色和强度相乘，并乘上衰减值，得到最终的光源强度。

```glsl
    //漫反射
    diffuseLight += intensity * cosAngIncidence;
```
这里计算漫反射光的贡献，将强度乘以入射角的余弦值，然后加到 `diffuseLight` 中。

```glsl
    // 镜面光
    vec3 halfAngle = normalize(directionToLight + viewDirection);
```
这行计算半角矢量（half-angle vector），它是光线方向和视线方向的和的归一化值。半角矢量在计算镜面反射光时很重要。

```glsl
    float blinnTerm = dot(surfaceNormal, halfAngle);
    blinnTerm = clamp(blinnTerm, 0, 1);
    blinnTerm = pow(blinnTerm, 512.0); // higher values -> sharper highlight
```
这里首先计算法线和半角向量之间的点积，得到一个值 `blinnTerm`，这个值用于描述镜面反射。通过 `clamp` 限制在 0 到 1 之间，接着将它提升到 512 次方，以使高光更尖锐。

```glsl
    specularLight += intensity * blinnTerm;
}
```
最后，将镜面反射的计算结果乘以当前光源的强度并累加到 `specularLight` 中。

综上所述，这段代码综合考虑了环境光、漫反射和镜面反射的光照效果，能够有效地渲染3D场景中的光照。这种设置使得即使在有多个光源的情况下，也能保证光照效果的准确性和真实感。 如果对代码的某个部分需要进一步的解释或者其他方面的疑问，请告诉我！

*/