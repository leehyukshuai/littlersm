#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <GLFW/glfw3.h>
#define PI (glm::pi<float>())

namespace camera {

    struct Sphere {
        float theta  = 0;
        float phi    = 0;
        float radius = 1;
        void  makeSafe() {
            if (theta < 0)
                theta += 2 * PI;
            if (theta > 2 * PI)
                theta -= 2 * PI;
            if (phi < -PI / 2)
                phi = -PI / 2;
            if (phi > PI / 2)
                phi = PI / 2;
        }
        glm::vec3 getDelta() const {
            glm::vec3 delta;
            delta.x = glm::cos(phi) * glm::cos(theta);
            delta.y = glm::sin(phi);
            delta.z = glm::cos(phi) * glm::sin(theta);
            return delta;
        }
        glm::vec3 getRight() const {
            glm::vec3 h = glm::vec3(cos(theta), 0, sin(theta));
            return glm::cross(glm::vec3(0, 1, 0), h);
        }
        glm::vec3 getUp() const {
            return glm::cross(getDelta(), getRight());
        }
        void mix(const Sphere & other, float v) {
            radius       = glm::mix(radius, other.radius, v);
            phi          = glm::mix(phi, other.phi, v);
            float dtheta = theta - other.theta;
            if (dtheta > PI) theta -= 2 * PI;
            if (dtheta < -PI) theta += 2 * PI;
            theta = glm::mix(theta, other.theta, v);
            makeSafe();
        }
    };

    class Camera {
    public:
        bool enabled{ true };
        float fovy { glm::radians(45.0f) };
        float zNear { 0.01f };
        float zFar { 1000.0f };

        Sphere    sphere { .radius = 10 };
        glm::vec3 target { 0, 0, 0 };
        Sphere    sphere0 { .radius = 10 };
        glm::vec3 target0 { 0, 0, 0 };

        float moveSpeed { 1.0 };
        float rotateSpeed { 0.01 };
        float scrollSpeed { 1 };
        float jumpSpeed { 0.01 };
        void  move(glm::vec3 delta) {
            if (!enabled) return;
            auto up      = glm::vec3(0, 1, 0);
            auto right   = sphere.getRight();
            auto forward = glm::cross(up, right);
            target0 += (-forward * delta.z + right * delta.x + up * delta.y) * moveSpeed;
        }
        void rotate(float dx, float dy) {
            if (!enabled) return;
            sphere0.theta += dx * rotateSpeed;
            sphere0.phi += dy * rotateSpeed;
            sphere0.makeSafe();
        }
        void scroll(float dy) {
            if (!enabled) return;
            sphere0.radius *= scrollSpeed * (1 - dy * 0.1);
        }

        glm::vec3 getPosition() const {
            return target + sphere.getDelta() * sphere.radius;
        }

        glm::mat4 getTransformMatrix(float aspect) {
            return getProjectionMatrix(aspect) * getViewMatrix();
        }

        glm::mat4 getProjectionMatrix(float aspect) {
            return glm::perspective(fovy, aspect, zNear, zFar);
        }

        glm::mat4 getViewMatrix() {
            return glm::lookAt(getPosition(), target, sphere.getUp());
        }

        void update(float dt, GLFWwindow * window) {
            processInput(dt, window);
            target = glm::mix(target, target0, jumpSpeed);
            sphere.mix(sphere0, jumpSpeed);
        }

        void jump(glm::vec3 newTarget, Sphere newSphere) {
            target0 = newTarget;
            sphere0 = newSphere;
        }

        void processInput(float dt, GLFWwindow * window) {
            glm::vec3 moveDelta(0);
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                moveDelta.z -= 1;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                moveDelta.z += 1;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                moveDelta.x += 1;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                moveDelta.x -= 1;
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
                moveDelta.y += 1;
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
                moveDelta.y -= 1;
            move(moveDelta * dt);
        }
    };
} // namespace camera