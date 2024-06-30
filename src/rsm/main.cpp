#include "../common/data.hpp"
#include "../common/gltf.hpp"
#include "../common/mesh.hpp"
#include "../common/shader.hpp"
#include "../common/texture.hpp"
#include "app.h"
#include "classes.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <vector>
// FOR DEBUGGING
#include <iostream>

namespace rsm {
    enum Scene { DEBUG_SCENE,
                 CORNELL_BOX,
                 FLIGHT_HELMET };
    const char * sceneNames[] = { "DEBUG_SCENE", "CORNELL_BOX", "FLIGHT_HELMET" };

    class RSMApp final : public App {
    public:
        RSMApp():
            App("RSM DEMO", 1600, 1200) {}

    private:
        const unsigned SCR_WIDTH = 1600, SCR_HEIGHT = 1200;
        const unsigned SHADOW_SIZE = 1024;

        glm::vec3 _pointLightIntensity { 1, 1, 1 };
        glm::vec3 _pointLightPosition;

        Scene _currentScene { Scene::DEBUG_SCENE };

        std::unique_ptr<Gltf>        _scene;
        std::unique_ptr<Program>     _program, _shadowProgram;
        std::unique_ptr<FrameBuffer> _shadowFbo;
        std::unique_ptr<Texture2D>   _randomMap;
        std::unique_ptr<TextureCube> _depthMap, _normalMap, _fluxMap;

        bool  _disableDirectLight { false };
        bool  _disableIndirectLight { false };
        float _directLightPower { 1.0 };
        float _indirectLightPower { 1.3 };

        float _sampleRange { 0.6 };
        int   _sampleNum { 20 };

    private:
        void init() override {
            loadScene(_currentScene);

            _program       = Program::create_from_files("shaders/rsm_phase2.vert", "shaders/rsm_phase2.frag");
            _shadowProgram = Program::create_from_files("shaders/rsm_phase1.vert", "shaders/rsm_phase1.geom", "shaders/rsm_phase1.frag");

            _shadowFbo = std::make_unique<FrameBuffer>();

            _randomMap = std::make_unique<Texture2D>("images/random_map.png");

            _depthMap  = std::make_unique<TextureCube>(SHADOW_SIZE, GL_FLOAT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT);
            _fluxMap   = std::make_unique<TextureCube>(SHADOW_SIZE, GL_FLOAT, GL_RGB, GL_RGB);
            _normalMap = std::make_unique<TextureCube>(SHADOW_SIZE, GL_FLOAT, GL_RGB, GL_RGB);

            glBindFramebuffer(GL_FRAMEBUFFER, _shadowFbo->get());
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthMap->get(), 0);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _fluxMap->get(), 0);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, _normalMap->get(), 0);
            GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
            glDrawBuffers(3, attachments);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "Error: Framebuffer is not complete!" << std::endl;
                exit(1);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void loadScene(Scene scene) {
            camera::Sphere viewSphere;
            viewSphere.radius = 5.0f;
            viewSphere.theta  = PI / 2.0f;
            glm::vec3 target;

            switch (scene) {
            case Scene::DEBUG_SCENE:
                _scene              = std::make_unique<Gltf>("models/debug_scene/scene.gltf");
                target              = glm::vec3(1, 1, -1);
                _pointLightPosition = glm::vec3(1, 1.6, -1);
                break;
            case Scene::CORNELL_BOX:
                _scene              = std::make_unique<Gltf>("models/cornell_box/scene.gltf");
                target              = glm::vec3(0, 1, 0);
                _pointLightPosition = glm::vec3(0, 1.6, 0);
                break;
            case Scene::FLIGHT_HELMET:
                _scene              = std::make_unique<Gltf>("models/flight_helmet/scene.gltf");
                target              = glm::vec3(0, 0.2, 0);
                viewSphere.radius   = 2.0f;
                _pointLightPosition = glm::vec3(0, 1.6, 1.6);
                break;
            }

            _camera.jump(target, viewSphere);
        };

        void update() override {
            App::update();
            drawui();
            render();
        }

        void drawui() {
            ImGui::Checkbox("Fix camera", &_disableControl);
            ImGui::Text("FPS: %.1f", 1.0f / App::getDelta());
            int currentScene = static_cast<int>(_currentScene);
            if (ImGui::Combo("Select Scene", &currentScene, sceneNames, IM_ARRAYSIZE(sceneNames))) {
                _currentScene = static_cast<Scene>(currentScene);
                loadScene(_currentScene);
            }

            if (ImGui::CollapsingHeader("RSM Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::SliderFloat("Sample Range", &_sampleRange, 0.0f, 1.6f, "%.2f");
                ImGui::SliderInt("Sample Number", &_sampleNum, 0, 600);
                ImGui::SliderFloat("Direct Factor", &_directLightPower, 0.0f, 4.0f, "%.2f");
                ImGui::SliderFloat("Indirect Factor", &_indirectLightPower, 0.0f, 10.0f, "%.2f");
                ImGui::Checkbox("Mask Direct Light", &_disableDirectLight);
                ImGui::Checkbox("Mask Indirect Light", &_disableIndirectLight);
            }
            if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::SliderFloat3("Light Position", glm::value_ptr(_pointLightPosition), -2, 2, "%.2f");
                ImGui::SliderFloat3("Light Intensity", glm::value_ptr(_pointLightIntensity), 0, 10, "%.2f");
            }
            if (ImGui::CollapsingHeader("Hint")) {
                ImGui::TextWrapped(
                    "1. `Press QWEASD` and `Drag screen` to adjust the camera view.\n"
                    "3. Reduce the `Sample Number` to improve the frame rate.");
            }
        }

        void render() {
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);

            // 1. first render to depth cubemap
            // ConfigureShaderAndMatrices
            GLfloat                near       = 0.1f;
            GLfloat                far        = 500.0f;
            glm::mat4              shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, near, far);
            std::vector<glm::mat4> shadowTransforms;
            shadowTransforms.push_back(shadowProj * glm::lookAt(_pointLightPosition, _pointLightPosition + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(_pointLightPosition, _pointLightPosition + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(_pointLightPosition, _pointLightPosition + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(_pointLightPosition, _pointLightPosition + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(_pointLightPosition, _pointLightPosition + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(_pointLightPosition, _pointLightPosition + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));
            // RenderScene
            glViewport(0, 0, SHADOW_SIZE, SHADOW_SIZE);
            glBindFramebuffer(GL_FRAMEBUFFER, _shadowFbo->get());
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUseProgram(_shadowProgram->get());
            for (GLuint i = 0; i < 6; ++i)
                glUniformMatrix4fv(glGetUniformLocation(_shadowProgram->get(), ("shadowMatrices[" + std::to_string(i) + "]").c_str()), 1, GL_FALSE, glm::value_ptr(shadowTransforms[i]));
            glUniform3fv(glGetUniformLocation(_shadowProgram->get(), "lightPos"), 1, glm::value_ptr(_pointLightPosition));
            glUniform3fv(glGetUniformLocation(_shadowProgram->get(), "lightColor"), 1, glm::value_ptr(_pointLightIntensity));
            glUniform1f(glGetUniformLocation(_shadowProgram->get(), "far_plane"), far);
            for (auto & draw : _scene->draws) {
                glUniformMatrix4fv(glGetUniformLocation(_shadowProgram->get(), "model"), 1, GL_FALSE, glm::value_ptr(draw.transform));
                for (auto & prim : _scene->meshes[draw.index]) {
                    auto mat      = _scene->materials[prim.material].get();
                    auto base_tex = _scene->textures[mat->base_color].get();
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, base_tex->get());
                    glUniform1i(glGetUniformLocation(_shadowProgram->get(), "base_color"), 0);
                    glUniform1i(glGetUniformLocation(_shadowProgram->get(), "use_base_color"), mat->base_color != 0);
                    glUniform4fv(glGetUniformLocation(_shadowProgram->get(), "base_color_factor"), 1, glm::value_ptr(mat->base_color_factor));
                    prim.mesh->draw();
                }
            }

            // 2. then render scene as normal with shadow mapping (using depth cubemap)
            glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            // ConfigureShaderAndMatrices
            auto viewTransform       = _camera.getViewMatrix();
            auto projectionTransform = _camera.getProjectionMatrix(getAspect());
            // RenderScene
            glUseProgram(_program->get());

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, _depthMap->get());
            glUniform1i(glGetUniformLocation(_program->get(), "depthMap"), 0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, _fluxMap->get());
            glUniform1i(glGetUniformLocation(_program->get(), "fluxMap"), 1);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_CUBE_MAP, _normalMap->get());
            glUniform1i(glGetUniformLocation(_program->get(), "normalMap"), 2);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, _randomMap->get());
            glUniform1i(glGetUniformLocation(_program->get(), "randomMap"), 3);

            glUniformMatrix4fv(glGetUniformLocation(_program->get(), "projection"), 1, false, glm::value_ptr(projectionTransform));
            glUniformMatrix4fv(glGetUniformLocation(_program->get(), "view"), 1, false, glm::value_ptr(viewTransform));
            glUniform3fv(glGetUniformLocation(_program->get(), "lightPos"), 1, glm::value_ptr(_pointLightPosition));
            glUniform3fv(glGetUniformLocation(_program->get(), "lightColor"), 1, glm::value_ptr(_pointLightIntensity));
            glUniform3fv(glGetUniformLocation(_program->get(), "viewPos"), 1, glm::value_ptr(_camera.getPosition()));
            glUniform1f(glGetUniformLocation(_program->get(), "far_plane"), far);

            glUniform1f(glGetUniformLocation(_program->get(), "sampleRange"), _sampleRange);
            glUniform1i(glGetUniformLocation(_program->get(), "sampleNum"), _sampleNum);
            glUniform1i(glGetUniformLocation(_program->get(), "disableDirectLight"), _disableDirectLight);
            glUniform1i(glGetUniformLocation(_program->get(), "disableIndirectLight"), _disableIndirectLight);
            glUniform1f(glGetUniformLocation(_program->get(), "indirectLightPower"), _indirectLightPower);
            glUniform1f(glGetUniformLocation(_program->get(), "directLightPower"), _directLightPower);

            for (auto & draw : _scene->draws) {
                glUniformMatrix4fv(glGetUniformLocation(_program->get(), "model"), 1, GL_FALSE, glm::value_ptr(draw.transform));
                for (auto & prim : _scene->meshes[draw.index]) {
                    auto mat      = _scene->materials[prim.material].get();
                    auto base_tex = _scene->textures[mat->base_color].get();
                    glActiveTexture(GL_TEXTURE4);
                    glBindTexture(GL_TEXTURE_2D, base_tex->get());
                    glUniform1i(glGetUniformLocation(_program->get(), "base_color"), 4);
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