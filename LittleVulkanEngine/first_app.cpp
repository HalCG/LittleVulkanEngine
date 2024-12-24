#include "first_app.h"

#include "keyboard_movement_controller.h"
#include "lve_buffer.h"
#include "lve_camera.h"
#include "simple_render_system.h"
#include "point_light_system.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include <glm/gtc/constants.hpp>

//std
#include <stdexcept>
#include <array>
#include <chrono>
#include <memory>

namespace lve {

	struct GlobalUbo {
		glm::mat4 projection{ 1.f };
		glm::mat4 view{ 1.f };
		glm::vec4 ambientLightColor{ 1.f, 1.f, 1.f, .02f };		// w��ʾ��ǿ
		glm::vec3 lightPosition{ -1.f };
		alignas(16) glm::vec4 lightColor{ 1.f };				// w��ʾ��ǿ
	};

	FirstApp::FirstApp() {
		globalPool =
			LVEDescriptorPool::Builder(lveDevice)
			.setMaxSets(LVESwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LVESwapChain::MAX_FRAMES_IN_FLIGHT)
			.build();
		loadGameObjects();
	}

	FirstApp::~FirstApp() {
	}

	void FirstApp::run() {
		//LVEBuffer globalUboBuffer{
		//	  lveDevice,
		//	  sizeof(GlobalUbo),
		//	  LVESwapChain::MAX_FRAMES_IN_FLIGHT,//MAX_FRAMES_IN_FLIGHT  !!!ȫ��ʹ�ã�Լ��������frame��uboBuffer��render commandBuffers��imageAvailableSemaphores��renderFinishedSemaphores��inFlightFences
		//	  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		//	  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,//δѡ��һ���ԣ��������������Ⱦ�Ļ���
		//	  lveDevice.properties.limits.minUniformBufferOffsetAlignment,
		//};
		//globalUboBuffer.map();

		std::vector<std::unique_ptr<LVEBuffer>> uboBuffers(LVESwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < uboBuffers.size(); ++i) {
			uboBuffers[i] = std::make_unique<LVEBuffer>(
				lveDevice,
				sizeof(GlobalUbo),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
				);
			uboBuffers[i]->map();
		}

		auto globalSetLayout =
			LVEDescriptorSetLayout::Builder(lveDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.build();

		//��ʾ�ڶ�֡��Ⱦʱ��������ҪΪÿһ֡׼��һ��������������趨ͨ������֧��˫�����໺����Ⱦ���Լ��� GPU �� CPU ֮��ĵȴ�ʱ�䡣
		std::vector<VkDescriptorSet> globalDescriptorSets(LVESwapChain::MAX_FRAMES_IN_FLIGHT);

		//�������������Ҫ�����������֡��������ء�����������ָ��Ķ���buffer��image...��
		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			LVEDescriptorWriter(*globalSetLayout, *globalPool)		//����һ��������д������������֮ǰ���������������ֺ������ء�
				.writeBuffer(0, &bufferInfo)						//���� 0 �Ű�λ�úͶ�Ӧ�Ļ�����������Ϣд���������С������������Ϣ�Ὣ���ݴ��ݸ���ɫ��
				.build(globalDescriptorSets[i]);					//������������������洢����Ӧ��
		}


		SimpleRenderSystem simpleRenderSystem{
			lveDevice,
			lveRenderer.getSwapChainRenderPass(),			//��ȡ����������Ⱦͨ����ͨ���������л�ͼʱ�������ϸ��Ϣ��
			globalSetLayout->getDescriptorSetLayout() };	//ȡ֮ǰ������ȫ�����������֣��Ա�����Ⱦ������ʹ�á�

		PointLightSystem pointLightSystem{
							lveDevice,
							lveRenderer.getSwapChainRenderPass(),
							globalSetLayout->getDescriptorSetLayout() };

		LVECamera camera{};
		////camera.setViewDirection(glm::vec3(0.f), glm::vec3(0.5f, 0.f, 1.f));
		//camera.setViewTarget(glm::vec3(-1.f, -2.f, -2.f), glm::vec3(0.f, 0.f, 2.5f));

		auto viewerObject = LVEGameObject::createGameObject();
		viewerObject.transform.translation.z = -2.5f;
		KeyboardMovementController cameraController{};

		auto currentTime = std::chrono::high_resolution_clock::now();
		while (!lveWindow.shouldClose()) {
			glfwPollEvents();

			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime =
				std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;
			cameraController.moveInPlaneXZ(lveWindow.getGLFWwindow(), frameTime, viewerObject);
			camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

			float aspect = lveRenderer.getAspectRatio();
			//camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
			//camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10.f);
			camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);

			if (auto commandBuffer = lveRenderer.beginFrame()) {
				int frameIndex = lveRenderer.getFrameIndex();
				FrameInfo frameInfo{
					frameIndex,
					frameTime,
					commandBuffer,
					camera,
					globalDescriptorSets[frameIndex],
					gameObjects };

				// update
				GlobalUbo ubo{};
				ubo.projection = camera.getProjection();
				ubo.view = camera.getView();
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();//!!! ע�� ��ǰ��Ⱦ���߶�Ӧ��ubo������ˢ�£�����֪�洢λ�ü�����

				/*
				* �����ܵ���ʹ�ã��� Vulkan �У�ʹ�ö���ܵ��ǳ������������������ڴ���ͬ���͵���Ⱦ����ʱ��ÿ���ܵ������в�ͬ��״̬����Դ�󶨣���Ӧ��ͬ����Ⱦ����
				* ÿ���ܵ��ڴ���ʱ����Ҫָ��һ���ܵ����֣�������ֶ����˸ùܵ�����ʹ�õ��������������ͳ�����ͨ�����ַ�ʽ��Vulkan ֪������Ⱦ��������η��ʺ�ʹ����Щ��Դ��
				* �� pointLightSystem �У��ܵ����ְ����˹�Դ��ص��������������� simpleRenderSystem �У��ܵ���������ܰ�����ģ����Ⱦ��ص�����������
				*/
				// render�� һ���������Ⱦ���̣��Լ��ɲ�ͬ����Ⱦ��������Ⱦ����͹�ԴЧ����
				lveRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(frameInfo);		//����������Ϸ������ģ�ͺ���������Ԫ�أ����Ƶ���ǰ֡��
				pointLightSystem.render(frameInfo);						//���������еĹ�Դ������ÿ����Դ�Ĺ���Ч����������Ӧ�õ��Ѿ���Ⱦ����Ϸ��������������ӳ������պ���Ӱ��Ч����
				lveRenderer.endSwapChainRenderPass(commandBuffer);
				lveRenderer.endFrame();									//��������Ⱦ��vkQueueSubmit ��������ύ��ͼ�ζ��С�
			}
		} 

		vkDeviceWaitIdle(lveDevice.device());
	}

	void FirstApp::loadGameObjects() {
		std::shared_ptr<LVEModel> lveModel =
			LVEModel::createModelFromFile(lveDevice, "C:/Users/tolcf/Desktop/models/flat_vase.obj");
		auto flatVase = LVEGameObject::createGameObject();
		flatVase.model = lveModel;
		flatVase.transform.translation = { -.5f, .5f, 0.f };
		flatVase.transform.scale = { 3.f, 1.5f, 3.f };
		gameObjects.emplace(flatVase.getId(), std::move(flatVase));

		lveModel = LVEModel::createModelFromFile(lveDevice, "C:/Users/tolcf/Desktop/models/smooth_vase.obj");
		auto smoothVase = LVEGameObject::createGameObject();
		smoothVase.model = lveModel;
		smoothVase.transform.translation = { .5f, .5f, 0.f };
		smoothVase.transform.scale = { 3.f, 1.5f, 3.f };
		gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

		lveModel = LVEModel::createModelFromFile(lveDevice, "C:/Users/tolcf/Desktop/models/quad.obj");
		auto floor = LVEGameObject::createGameObject();
		floor.model = lveModel;
		floor.transform.translation = { 0.f, .5f, 0.f };
		floor.transform.scale = { 3.f, 1.f, 3.f };
		gameObjects.emplace(floor.getId(), std::move(floor));
	}
};

/*
�����ṩ�� `GlobalUbo` �ṹ���У�`alignas(16)` ������ָ�� `lightColor` �ֶε��ڴ���뷽ʽ�����ֶ�����ͼ�α���зǳ���Ҫ����������ʹ���� Vulkan �� Direct3D �����ִ�ͼ�� API ʱ����������ϸ������

### �ڴ����ı���

1. **�����Ŀ��**:
	- �ڴ������ָ�������ڴ��д洢�ķ�ʽ��ʹ����ʼ��ַ�����ض��Ķ���Ҫ��һ����˵���������͵Ĵ�С�Ͷ���Ҫ���Ӱ�������ڴ��еĲ��֡�
	- �������ҪĿ��������߷���Ч�ʣ��������� SIMD ָ��� AVX��SSE���� GPU ����ʱ��������Լ����ڴ�����ӳٲ�������ܡ�

2. **GPU �� UBO**:
   - ��ͼ�α���У�Uniform Buffer Objects (UBOs) ���ڽ�ͳһ���ݴ��ݸ���ɫ����Ϊ��֤���ݰ����ض���ʽ��ȷ���ݣ����ݵĶ�����Ե���Ϊ��Ҫ��
   - ������� GPU �ܹ���ͨ��Ҫ���ض����������ͱ��밴��һ�����ֽڶ��롣���磬`glm::vec4` ͨ����Ҫ���ڴ����� 16 �ֽڶ��루��Ϊ���Ĵ�С�� 16 �ֽڣ���

### �� `lightColor` ʹ�� `alignas(16)`

1. **�Ϲ���**:
   - ʹ�� `alignas(16)` ��֤ `lightColor` ����ʼ��ַ�� 16 �ֽڶ���ġ���ȷ���������� GPU �׶α���ȷ��ȡ������������벻�����µ��������⡣

2. **�ṹ����**:
   - �ڽṹ���У��Ƕ������ݿ��ܵ��½ṹ������Ķ������⣬�Ӷ�Ӱ������ֶεĲ��֡���ʹ�� `alignas(16)` ��`lightColor` ȷ����λ���ܹ���������С�ֶεĶ���Ҫ�󣬱�������ṹ�ĺ����ԡ�

3. **�����Ż�**:
   - ͨ��ȷ�� `lightColor` 16 �ֽڶ��룬������� GPU �����������ܣ������˷���ʱ���ڴ��ȡ�ɱ���

### �ܽ�

��֮������� `GlobalUbo` �ṹ��ʱʹ�� `alignas(16)` ��Ϊ��ȷ�� `lightColor` ���� 16 �ֽڶ���洢���Է����ִ� GPU �ķ��������������ܲ�����Ǳ�ڵĶ������⡣�����ʵʱ��Ⱦ�е����ݴ����������Ż�����Ӱ�졣����и������Ӧ�ó��������ϸ�������ۣ���ӭ���ʣ�

*/


/*
��ͼ����Ⱦ�����У�**���Ʋ�����ʵ��ִ��**������������ύ�� GPU ����еġ�����Ĵ����У�`draw` ����ȷʵֻ�ǽ����������¼�� **`commandBuffer`** �У�������ֱ�ӽ��л��ơ��������������̵Ļ������裺

1. **�����¼**���������ṩ�Ĵ����У�ͨ������ `vkCmdBindDescriptorSets`��`vkCmdPushConstants` �� `obj.model->draw(frameInfo.commandBuffer);` ��¼������Ҫ��������������������������ͳ�������������ȣ��������������һ�׶γ�Ϊ **�����¼��Command Recording��**��

2. **�����ύ**����������¼�������� **`vkQueueSubmit`** �����Ƶĺ���������������ύ��ͼ�ζ��С����磺
   ```cpp
   vkQueueSubmit(queue, 1, &submitInfo, nullptr);
   ```
   ����������У���¼����������е�����ᱻ�ύ�� GPU���Ա㿪ʼִ�С���һ�׶γ�Ϊ **�����ύ��Command Submission��**��

3. **GPUִ��**��һ������������ύ��GPU ���Ὺʼִ����Щ�����ʱ�����Ʋ�������������GPU �ᰴ����������е��������ִ�У����紦�������ݡ����б任�����й�դ�����������յ�Ƭ�εȡ�

4. **�����ʾ**����󣬵���������ִ����ϣ�������ᱻд��֡������������ͨ�����������ֵ���Ļ�ϡ���ͨ���漰���� `vkQueuePresentKHR`��

### �ܽ�

- **���Ʋ�����ʵ��ִ��** ���ύ����������� GPU ����ʱ�������������ڵ��� `draw` ����ʱ��
- `draw` ����ֻ�����¼��������е�ʵ�ʻ��Ʋ������������� GPU �����������ύ�������еġ�

����������ϸ�˽��ύ��ִ�й����е��κξ��岽�裬���߶� Vulkan �е�ͼ�ιܵ��и�������ʣ�����ʱ���ң�

*/