#pragma once
#include "lve_model.h"

//// libs
//#include <glm/gtc/matrix_transform.hpp>

// std
#include <memory>
namespace lve {
    struct TransformComponent {
        glm::vec3 translation{};
        glm::vec3 scale{ 1.f, 1.f, 1.f };
        glm::vec3 rotation{};

        // 矩阵对应 Translate * Ry * Rx * Rz * Scale
        // 旋转对应于 Y(1)、X(2)、Z(3) 的 Tait-bryan 角
        // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
        glm::mat4 mat4();

        glm::mat3 normalMatrix();
    };

    class LVEGameObject {
    public:
        using id_t = unsigned int;
        static LVEGameObject createGameObject() {
            static id_t currentId = 0;
            return LVEGameObject{ currentId++ };
        }

        LVEGameObject(const LVEGameObject&) = delete;
        LVEGameObject& operator=(const LVEGameObject&) = delete;
        LVEGameObject(LVEGameObject&&) = default;
        LVEGameObject& operator=(LVEGameObject&&) = default;

        id_t getId() { return id; }
        std::shared_ptr<LVEModel> model{};
        glm::vec3 color{};
        TransformComponent transform{};

    private:
        LVEGameObject(id_t objId) : id{ objId } {}
        id_t id;
    };
}  // namespace lve