#version 450

const vec2 OFFSETS[6] = vec2[](
  vec2(-1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, -1.0),
  vec2(1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, 1.0)
);

layout (location = 0) out vec2 fragOffset;

struct PointLight {
  vec4 position; // ignore w
  vec4 color; // w ��ǿ��
};

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  vec4 ambientLightColor; // w is intensity
  PointLight pointLights[10];
  int numLights;
} ubo;

layout(push_constant) uniform Push {
  vec4 position;
  vec4 color;
  float radius;
} push;


void main() {
    //����ǰ�����ƫ������ OFFSETS ��������ȡ������gl_VertexIndex �ǵ�ǰ���ڴ���Ķ����������
    fragOffset = OFFSETS[gl_VertexIndex];

    //����ͼ��������ȡ��������ҷ�����Ϸ����������������ڼ����Դ������ռ��еľ�ȷλ�á�
    vec3 cameraRightWorld = {ubo.view[0][0], ubo.view[1][0], ubo.view[2][0]};
    vec3 cameraUpWorld = {ubo.view[0][1], ubo.view[1][1], ubo.view[2][1]};

    //�����Դ������ռ��е�λ�ã���ʽ�а�����Դ�Ļ���λ�� ubo.lightPosition �ͻ���ƫ�����Լ� LIGHT_RADIUS �����ƫ������
    //��ͨ���� fragOffset ����������ķ����������������������������γ����յ�λ�á�
    //vec3 positionWorld = ubo.lightPosition.xyz
    //  + LIGHT_RADIUS * fragOffset.x * cameraRightWorld
    //  + LIGHT_RADIUS * fragOffset.y * cameraUpWorld;
    vec3 positionWorld = push.position.xyz
        + push.radius * fragOffset.x * cameraRightWorld
        + push.radius * fragOffset.y * cameraUpWorld;

    //ת��Ϊ�ü��ռ�����
    gl_Position = ubo.projection * ubo.view * vec4(positionWorld, 1.0);
}

/*
�� Vulkan �У�һ���ܵ�����Ⱦ�������ֱ����Ϊ��һ���ܵ������룬��Ϊ Vulkan ��һ��������Ϊ������ API�������ǻ���ͼ�μ����״̬����ÿ���ܵ�����Ⱦ�����Ƕ����ģ�ͨ��ͨ��֡�������Framebuffer������Ⱦͨ����Render Pass��������������ݡ�������ϸ������������Լ�����ڶ���ܵ�֮�䴫�����ݡ�

### 1. ��Ⱦ�ܵ��Ķ�����

- **��Ⱦ�ܵ��Ķ�����**��ÿ����Ⱦ�ܵ����������������Ļ����ʾ�����أ��Ƕ����ģ������������ܵ��������޹ء��ܵ������ͨ����д�뵽��Ⱦͨ����Render Pass���ж����֡��������Framebuffer����
  
- **���������ʹ��**�����ڵ��� `simpleRenderSystem.render(frameInfo)` �� `pointLightSystem.render(frameInfo)` ʱ��ÿ��ϵͳ����������Ե���������м�¼����Ⱦ��������Щ������ʵ��ִ��ʱ������ύ�� GPU��

### 2. ����������ܵ�֮�䴫������

��Ȼһ���ܵ�����Ⱦ�������ֱ�Ӵ��ݸ���һ���ܵ���������ͨ�����·�ʽ���ʵ�����ݹ���

- **֡��������Framebuffer��**��
  - ��һ���ܵ���������糡������ɫ�������Ϣ�ȣ�������Ⱦ��һ��֡�������е���ɫ���ӣ�color attachment������ȸ��ӣ�depth attachment���ϡ�
  - �ڶ����ܵ������������Ⱦ������ʹ�����֡�����������ݣ�ͨ��ͨ��һ��ȫ���ı��Σ�Fullscreen quad��������������������ȡ�ʹ�����Ⱦ�����

- **����Texture��**��
  - ��һ���ܵ����Խ���Ⱦ����洢��һ�������С�Ȼ���ڵڶ����ܵ��У�����԰����������Ϊ�����������������ɫ�����ܹ�������Ⱦ��ͼ�����ݡ�
  - ���ַ��������ɺ���Ч��������ռ��㡢��Ӱӳ�䡢ģ���ȣ�ʱ�ر����á�

### 3. ʾ������

��������ʵ��һ������Ч�������̿������£�

1. **ʹ�õ�һ���ܵ���Ⱦ����**��
   ```cpp
   // ��Ⱦ��������ɫ������
   lveRenderer.beginSwapChainRenderPass(commandBuffer);
   simpleRenderSystem.renderGameObjects(frameInfo); // ��Ⱦ����
   lveRenderer.endSwapChainRenderPass(commandBuffer);
   ```

2. **����ɫ��������������ȡΪ����**���ں�̨������
   - ����Ҫȷ��û�����������ɫ�����������߽������ݿ�����һ�����õ������ϡ�

3. **�ڵڶ����ܵ���ʹ�����������й��ռ���**��
   ```cpp
   pointLightSystem.render(frameInfo); // ʹ��֮ǰ��Ⱦ�Ľ�������й��ռ���
   ```

### 4. �ܽ�

�� Vulkan �У���Ȼһ���ܵ��������ֱ��������һ���ܵ���������ͨ��֡������������ȷ�ʽ���м�Ӵ��ݡ������ķ�ʽ������ͼ����Ⱦ�ĸ����׶�֮�乲�����ݣ�ʵ�ָ����ӵ��Ӿ�Ч��������ա���Ӱ�ͺ��ڴ���ȡ���������ṩ�˸߶ȵ�����ԣ�ͬʱҲ�����˹ܵ��Ķ����ԡ�

*/