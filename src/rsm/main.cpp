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
    class RSMApp final : public App {
    public:
        RSMApp():
            App("Texture and Lighting", 1600, 1200) {
            camera::Sphere viewSphere;
            viewSphere.radius = 5.0f;
            viewSphere.theta  = PI / 2.0f;
            _camera.jump(glm::vec3(0, 1, 0), viewSphere);
            _camera.moveSpeed = 1.0f;
        }

    private:
        const unsigned SCR_WIDTH = 1600, SCR_HEIGHT = 1200;
        const unsigned SHADOW_SIZE = 1024;

        glm::vec3 _pointLightColor { 1, 1, 1 };
        glm::vec3 _pointLightPosition { 0, 1.8, 0 };

        std::unique_ptr<Gltf> _scene;
        std::unique_ptr<Program> _program, _shadowProgram;
        std::unique_ptr<FrameBuffer> _shadowFbo;
        std::unique_ptr<Texture2D> _randomMap;
        std::unique_ptr<TextureCube> _depthMap, _positionMap, _normalMap, _fluxMap;

        bool _disableControl { false };

    private:
        void init() override {
            _scene = std::make_unique<Gltf>("cornell_box/scene.gltf");
            // _scene   = std::make_unique<Gltf>("FlightHelmet/FlightHelmet.gltf");

            _program      = Program::create_from_files("shaders/rsm_phase2.vert", "shaders/rsm_phase2.frag");
            _shadowProgram = Program::create_from_files("shaders/rsm_phase1.vert", "shaders/rsm_phase1.geom", "shaders/rsm_phase1.frag");

            _shadowFbo = std::make_unique<FrameBuffer>();

            _randomMap = std::make_unique<Texture2D>("images/random_map.png");

            _depthMap = std::make_unique<TextureCube>(SHADOW_SIZE, GL_FLOAT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT);
            _positionMap = std::make_unique<TextureCube>(SHADOW_SIZE, GL_UNSIGNED_BYTE, GL_RGB, GL_RGB);
            _fluxMap = std::make_unique<TextureCube>(SHADOW_SIZE, GL_UNSIGNED_BYTE, GL_RGB, GL_RGB);
            _normalMap = std::make_unique<TextureCube>(SHADOW_SIZE, GL_UNSIGNED_BYTE, GL_RGB, GL_RGB);

            glBindFramebuffer(GL_FRAMEBUFFER, _shadowFbo->get());
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthMap->get(), 0);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _positionMap->get(), 0);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, _normalMap->get(), 0);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, _fluxMap->get(), 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "Error: Framebuffer is not complete!" << std::endl;
                exit(1);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
            glUniform1f(glGetUniformLocation(_shadowProgram->get(), "far_plane"), far);
            glUniform3fv(glGetUniformLocation(_shadowProgram->get(), "lightPos"), 1, glm::value_ptr(_pointLightPosition));
            for (auto & draw : _scene->draws) {
                glUniformMatrix4fv(glGetUniformLocation(_shadowProgram->get(), "model"), 1, GL_FALSE, glm::value_ptr(draw.transform));
                for (auto & prim : _scene->meshes[draw.index]) {
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
            glBindTexture(GL_TEXTURE_CUBE_MAP, _positionMap->get());
            glUniform1i(glGetUniformLocation(_program->get(), "positionMap"), 1);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_CUBE_MAP, _normalMap->get());
            glUniform1i(glGetUniformLocation(_program->get(), "normalMap"), 2);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_CUBE_MAP, _fluxMap->get());
            glUniform1i(glGetUniformLocation(_program->get(), "fluxMap"), 3);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, _randomMap->get());
            glUniform1i(glGetUniformLocation(_program->get(), "randomMap"), 4);

            glUniformMatrix4fv(glGetUniformLocation(_program->get(), "projection"), 1, false, glm::value_ptr(projectionTransform));
            glUniformMatrix4fv(glGetUniformLocation(_program->get(), "view"), 1, false, glm::value_ptr(viewTransform));
            glUniform3fv(glGetUniformLocation(_program->get(), "lightPos"), 1, glm::value_ptr(_pointLightPosition));
            glUniform3fv(glGetUniformLocation(_program->get(), "lightColor"), 1, glm::value_ptr(_pointLightColor));
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