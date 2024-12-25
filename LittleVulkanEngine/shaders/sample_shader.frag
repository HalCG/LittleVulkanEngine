#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;

layout (location = 0) out vec4 outColor;

struct PointLight {
  vec4 position; // ignore w
  vec4 color; // w ��ǿ��
};

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  mat4 invView;
  vec4 ambientLightColor; // w ��ǿ��
  PointLight pointLights[10];
  int numLights;
} ubo;

layout(push_constant) uniform PushConstantData{//ÿ����ɫ����ڵ�ֻ��ʹ��һ���������Ϳ�
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
  vec3 specularLight = vec3(0.0);                                       //��ʼ��һ�������ı����������ۼ����й�Դ�ľ��淴���Ĺ��ס�
  vec3 surfaceNormal = normalize(fragNormalWorld);

  vec3 cameraPosWorld = ubo.invView[3].xyz;                             //ͨ��������棬ȡ�����������������ƽ�ƣ��õ������������
  vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

  for (int i = 0; i < ubo.numLights; i++) {
    PointLight light = ubo.pointLights[i];
    vec3 directionToLight = light.position.xyz - fragPosWorld;
    float attenuation = 1.0 / dot(directionToLight, directionToLight);  // distance squared ����˥��ϵ��
    directionToLight = normalize(directionToLight);

    float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
    vec3 intensity = light.color.xyz * light.color.w * attenuation;     //���㵱ǰ��Դ��ǿ�ȣ�������ɫ��ǿ����ˣ�������˥��ֵ���õ����յĹ�Դǿ�ȡ�

    //������
    diffuseLight += intensity * cosAngIncidence;

    // �����
    vec3 halfAngle = normalize(directionToLight + viewDirection);
    float blinnTerm = dot(surfaceNormal, halfAngle);
    blinnTerm = clamp(blinnTerm, 0, 1);
    blinnTerm = pow(blinnTerm, 512.0); // higher values -> sharper highlight
    specularLight += intensity * blinnTerm;
  }

  outColor = vec4(diffuseLight * fragColor + specularLight * fragColor, 1.0);
}

/*
�������Ч�������������⡢�������;��淴�����һ�������е�Ӱ�졣�������н�����δ��룺

```glsl
vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
```
���д������ȼ��㻷���⣨ambient light����`ubo.ambientLightColor` ��һ��������������ɫ��ǿ�ȵ�������`.xyz` ��ȡ��ɫ���֣��� `.w` ͨ����ʾǿ�ȣ���˽����������ɫ��ǿ����˵õ�������Ĺ��ס�

```glsl
vec3 specularLight = vec3(0.0);
```
���д����ʼ��һ�������ı����������ۼ����й�Դ�ľ��淴���Ĺ��ס�

```glsl
vec3 surfaceNormal = normalize(fragNormalWorld);
```
���д��뽫Ƭ�εķ������� `fragNormalWorld` ���й�һ������ȷ�����ĳ���Ϊ 1���������ڼ�����������֮��Ĺ�ϵ��

```glsl
vec3 cameraPosWorld = ubo.invView[3].xyz;
vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);
```
�������Ȼ�ȡ����ռ��е������λ�ã�`cameraPosWorld`����Ȼ�����ӵ�ǰƬ��λ�õ�����������߷���`viewDirection`���������й�һ����

```glsl
for (int i = 0; i < ubo.numLights; i++) {
    PointLight light = ubo.pointLights[i];
    vec3 directionToLight = light.position.xyz - fragPosWorld;
    float attenuation = 1.0 / dot(directionToLight, directionToLight); // distance squared
    directionToLight = normalize(directionToLight);
```
��δ�����һ��ѭ�����������еĵ��Դ `PointLight`��ͨ�� `light.position` ��ȡ��Դ��λ�ã��������Դ����ǰƬ��λ�õķ��� `directionToLight`��Ȼ��ͨ������`directionToLight`������ƽ������������˥��ϵ��������ʹ�õ��Ǽ򵥵�ƽ������˥�� `1 / (distance ^ 2)`����󣬶Ը÷����������й�һ����

```glsl
    float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
```
��һ�д�������������淨��֮��ļнǵ�����ֵ��������ǣ���ʹ�� `dot` �������м��㣬��ʹ�� `max` ��ȷ����ֵ��С�� 0��

```glsl
    vec3 intensity = light.color.xyz * light.color.w * attenuation;
```
���д�����㵱ǰ��Դ��ǿ�ȣ�������ɫ��ǿ����ˣ�������˥��ֵ���õ����յĹ�Դǿ�ȡ�

```glsl
    //������
    diffuseLight += intensity * cosAngIncidence;
```
��������������Ĺ��ף���ǿ�ȳ�������ǵ�����ֵ��Ȼ��ӵ� `diffuseLight` �С�

```glsl
    // �����
    vec3 halfAngle = normalize(directionToLight + viewDirection);
```
���м�����ʸ����half-angle vector�������ǹ��߷�������߷���ĺ͵Ĺ�һ��ֵ�����ʸ���ڼ��㾵�淴���ʱ����Ҫ��

```glsl
    float blinnTerm = dot(surfaceNormal, halfAngle);
    blinnTerm = clamp(blinnTerm, 0, 1);
    blinnTerm = pow(blinnTerm, 512.0); // higher values -> sharper highlight
```
�������ȼ��㷨�ߺͰ������֮��ĵ�����õ�һ��ֵ `blinnTerm`�����ֵ�����������淴�䡣ͨ�� `clamp` ������ 0 �� 1 ֮�䣬���Ž��������� 512 �η�����ʹ�߹������

```glsl
    specularLight += intensity * blinnTerm;
}
```
��󣬽����淴��ļ��������Ե�ǰ��Դ��ǿ�Ȳ��ۼӵ� `specularLight` �С�

������������δ����ۺϿ����˻����⡢������;��淴��Ĺ���Ч�����ܹ���Ч����Ⱦ3D�����еĹ��ա���������ʹ�ü�ʹ���ж����Դ������£�Ҳ�ܱ�֤����Ч����׼ȷ�Ժ���ʵ�С� ����Դ����ĳ��������Ҫ��һ���Ľ��ͻ���������������ʣ�������ң�

*/