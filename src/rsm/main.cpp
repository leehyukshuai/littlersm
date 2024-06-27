#include "../common/data.hpp"
#include "../common/gltf.hpp"
#include "../common/mesh.hpp"
#include "../common/shader.hpp"
#include "../common/texture.hpp"
#include "app.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
// FOR DEBUGGING
#include <iostream>
#include <stb_image_write.h>

namespace rsm {
    class RSMApp final : public App {
    public:
        RSMApp():
            App("Texture and Lighting", 1600, 1200) {
            camera::Sphere viewSphere;
            viewSphere.radius = 5.0f;
            viewSphere.theta = PI/2.0f;
            _camera.jump(glm::vec3(0, 1, 0), viewSphere);
            _camera.moveSpeed = 1.0f;
        }
        ~RSMApp() {
            glDeleteFramebuffers(1, &_shadowDepthFbo);
        }

    private:
        const unsigned SCR_WIDTH = 1600, SCR_HEIGHT = 1200;
        const unsigned SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

        std::unique_ptr<Gltf> _scene;

        glm::vec3 _pointLightColor { 1, 1, 1 };
        glm::vec3 _pointLightPosition { 0, 1.8, 0 };

        std::unique_ptr<Program> _program, _depthProgram;

        GLuint _shadowDepthFbo, _shadowDepthMap;

        bool _disableControl { false };

    private:
        void init() override {
            glGenFramebuffers(1, &_shadowDepthFbo);
            glGenTextures(1, &_shadowDepthMap);
            glBindTexture(GL_TEXTURE_CUBE_MAP, _shadowDepthMap);
            for (GLuint i = 0; i < 6; ++i)
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glBindFramebuffer(GL_FRAMEBUFFER, _shadowDepthFbo);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _shadowDepthMap, 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "Error: Framebuffer is not complete!" << std::endl;
                exit(1);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            _scene = std::make_unique<Gltf>("cornell_box/scene.gltf");
            // _scene   = std::make_unique<Gltf>("FlightHelmet/FlightHelmet.gltf");
            _program      = Program::create_from_files("shaders/shadow.vert", "shaders/shadow.frag");
            _depthProgram = Program::create_from_files("shaders/depth.vert", "shaders/depth.geom", "shaders/depth.frag");
        }

        void update() override {
            App::update();
            drawui();
            render();
        }

        void drawui() {
            if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
            }
            if (ImGui::Checkbox("Disable Camera Control", &_disableControl))
                _camera.enabled = ! _disableControl;
            if (ImGui::CollapsingHeader("Hint")) {
                ImGui::TextWrapped(
                    "1. Use WASD+QE to move your camera.\n"
                    "2. Drag screen to rotate your front.\n"
                    "3. Disable `Camera Control` to set uniforms without moving your camera.");
            }
        }

        void render() {
            glEnable(GL_DEPTH_TEST);
            // glEnable(GL_CULL_FACE);

            // 1. first render to depth cubemap
            // ConfigureShaderAndMatrices
            GLfloat                aspect     = (GLfloat) SHADOW_WIDTH / (GLfloat) SHADOW_HEIGHT;
            GLfloat                near       = 0.1f;
            GLfloat                far        = 500.0f;
            glm::mat4              shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far);
            std::vector<glm::mat4> shadowTransforms;
            shadowTransforms.push_back(shadowProj * glm::lookAt(_pointLightPosition, _pointLightPosition + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(_pointLightPosition, _pointLightPosition + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(_pointLightPosition, _pointLightPosition + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(_pointLightPosition, _pointLightPosition + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(_pointLightPosition, _pointLightPosition + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(_pointLightPosition, _pointLightPosition + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));
            // RenderScene
            glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, _shadowDepthFbo);
            glClear(GL_DEPTH_BUFFER_BIT);
            glUseProgram(_depthProgram->get());
            for (GLuint i = 0; i < 6; ++i)
                glUniformMatrix4fv(glGetUniformLocation(_depthProgram->get(), ("shadowMatrices[" + std::to_string(i) + "]").c_str()), 1, GL_FALSE, glm::value_ptr(shadowTransforms[i]));
            glUniform1f(glGetUniformLocation(_depthProgram->get(), "far_plane"), far);
            glUniform3fv(glGetUniformLocation(_depthProgram->get(), "lightPos"), 1, glm::value_ptr(_pointLightPosition));
            for (auto & draw : _scene->draws) {
                glUniformMatrix4fv(glGetUniformLocation(_depthProgram->get(), "model"), 1, GL_FALSE, glm::value_ptr(draw.transform));
                for (auto & prim : _scene->meshes[draw.index]) {
                    prim.mesh->draw();
                }
            }

            // 2. then render scene as normal with shadow mapping (using depth cubemap)
            glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClearColor(0.0, 0.2, 0.2, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            // ConfigureShaderAndMatrices
            auto viewTransform       = _camera.getViewMatrix();
            auto projectionTransform = _camera.getProjectionMatrix(getAspect());
            // RenderScene
            glUseProgram(_program->get());
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, _shadowDepthMap);
            glUniform1i(glGetUniformLocation(_program->get(), "depthMap"), 0);
            glUniformMatrix4fv(glGetUniformLocation(_program->get(), "projection"), 1, false, glm::value_ptr(projectionTransform));
            glUniformMatrix4fv(glGetUniformLocation(_program->get(), "view"), 1, false, glm::value_ptr(viewTransform));
            glUniform3fv(glGetUniformLocation(_program->get(), "lightPos"), 1, glm::value_ptr(_pointLightPosition));
            glUniform3fv(glGetUniformLocation(_program->get(), "viewPos"), 1, glm::value_ptr(_camera.getPosition()));
            glUniform1f(glGetUniformLocation(_program->get(), "far_plane"), far);
            for (auto & draw : _scene->draws) {
                glUniformMatrix4fv(glGetUniformLocation(_program->get(), "model"), 1, GL_FALSE, glm::value_ptr(draw.transform));
                for (auto & prim : _scene->meshes[draw.index]) {
                    auto mat      = _scene->materials[prim.material].get();
                    auto base_tex = _scene->textures[mat->base_color].get();
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, base_tex->get());
                    glUniform1i(glGetUniformLocation(_program->get(), "base_color"), 1);
                    glUniform1i(glGetUniformLocation(_program->get(), "use_base_color"), mat->base_color != 0);
                    glUniform4fv(glGetUniformLocation(_program->get(), "base_color_factor"), 1, glm::value_ptr(mat->base_color_factor));
                    prim.mesh->draw();
                }
            }
        }
    };
}; // namespace rsm

int main() {
    try {
        rsm::RSMApp app {};
        app.run();
    } catch (std::exception & e) {
        std::cerr << e.what() << std::endl;
    }
}