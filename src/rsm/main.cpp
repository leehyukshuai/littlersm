#include "../common/data.hpp"
#include "../common/gltf.hpp"
#include "../common/mesh.hpp"
#include "../common/shader.hpp"
#include "../common/texture.hpp"
#include "app.h"
#include "mesh.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <iostream>

namespace rsm {
    // class FrameBuffer {
    // public:
    //     FrameBuffer() {
    //         glGenFramebuffers(1, &_id);
    //     }
    //     ~FrameBuffer() {
    //         glDeleteFramebuffers(1, &_id);
    //     }

    //     GLuint get() const {
    //         return _id;
    //     }

    // private:
    //     GLuint _id {};
    // };

    class RSMApp final : public App {
    public:
        RSMApp():
            App("Texture and Lighting", 1600, 1200) {
            camera::Sphere viewSphere;
            viewSphere.radius = 4.0f;
            viewSphere.phi    = PI / 3.6f;
            viewSphere.theta  = 0.3f;
            _camera.jump(glm::vec3(0, 0, 0), viewSphere);
            _camera.moveSpeed = 4.0f;
        }

    private:
        // const unsigned               SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
        // std::unique_ptr<Texture2D>   _shadowDepthMap;
        // std::unique_ptr<FrameBuffer> _shadowDepthFbo;
        // std::unique_ptr<Program>     _simpleDepthShader;

        std::unique_ptr<Gltf>    _scene;
        std::unique_ptr<Program> _program;
        bool                     _disableControl { false };

    private:
        void init() override {
            // _shadowDepthFbo = std::make_unique<FrameBuffer>();
            // _shadowDepthMap = std::make_unique<Texture2D>(nullptr, GL_FLOAT, SHADOW_WIDTH, SHADOW_HEIGHT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT);

            // glBindFramebuffer(GL_FRAMEBUFFER, _shadowDepthFbo->get());
            // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _shadowDepthMap->get(), 0);
            // glDrawBuffer(GL_NONE);
            // glReadBuffer(GL_NONE);
            // glBindFramebuffer(GL_FRAMEBUFFER, 0);
            _scene   = std::make_unique<Gltf>("cornell_box/scene.gltf");
            // _scene   = std::make_unique<Gltf>("FlightHelmet/FlightHelmet.gltf");
            _program = Program::create_from_files("shaders/naive.vert", "shaders/naive.frag");
        }

        void update() override {
            _camera.update(getDelta(), _window);
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
            draw();
        }

        void draw() {
            glEnable(GL_DEPTH_TEST);
            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            auto viewTransform       = _camera.getViewMatrix();
            auto projectionTransform = _camera.getProjectionMatrix(getAspect());
            auto cameraTransform     = projectionTransform * viewTransform;

            // /// Update shadow depth map
            // glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            // glBindFramebuffer(GL_FRAMEBUFFER, _shadowDepthFbo->get());
            // glClear(GL_DEPTH_BUFFER_BIT);
            // // Configure shader and matrices
            // GLfloat   near_plane = 0.1f, far_plane = 100.0f;
            // glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
            // glm::mat4 lightView       = glm::lookAt(_directLightDirection, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            // glm::mat4 lightTransform  = lightProjection * lightView;
            // glUseProgram(_simpleDepthShader->get());
            // glUniformMatrix4fv(glGetUniformLocation(_program->get(), "u_lightTransform"), 1, GL_FALSE, glm::value_ptr(lightTransform));
            // glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            // glBindFramebuffer(GL_FRAMEBUFFER, _shadowDepthFbo->get());
            // glClear(GL_DEPTH_BUFFER_BIT);
            // // Render scene
            // glBindVertexArray(_vao->get());
            // glDrawElements(GL_TRIANGLES, _square.indices.size(), GL_UNSIGNED_INT, 0);

            // glBindFramebuffer(GL_FRAMEBUFFER, 0);
            // // Bind shadow map
            // //Render as default

            glUseProgram(_program->get());
            for (auto & draw : _scene->draws) {
                auto transform = projectionTransform * viewTransform * draw.transform;
                glUniformMatrix4fv(glGetUniformLocation(_program->get(), "transform"), 1, false, (GLfloat *) &transform);
                for (auto & prim : _scene->meshes[draw.index]) {
                    auto mat      = _scene->materials[prim.material].get();
                    auto base_tex = _scene->textures[mat->base_color].get();
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, base_tex->get());
                    glUniform1i(glGetUniformLocation(_program->get(), "base_color"), 0);
                    glUniform1i(glGetUniformLocation(_program->get(), "use_base_color"), mat->base_color != 0);
                    glUniform4fv(glGetUniformLocation(_program->get(), "base_color_factor"), 1, glm::value_ptr(mat->base_color_factor));
                    prim.mesh->draw();
                }
            }

            // glUniformMatrix4fv(glGetUniformLocation(_program->get(), "u_transform"), 1, false, glm::value_ptr(cameraTransform));
            // glUniform3fv(glGetUniformLocation(_program->get(), "u_directLightColor"), 1, glm::value_ptr(_directLightColor));
            // glUniform3fv(glGetUniformLocation(_program->get(), "u_directLightDirection"), 1, glm::value_ptr(_directLightDirection));
            // glUniform1f(glGetUniformLocation(_program->get(), "u_directLightIntensity"), _directLightIntensity);
            // glUniform3fv(glGetUniformLocation(_program->get(), "u_pointLightColor"), 1, glm::value_ptr(_pointLightColor));
            // glUniform3fv(glGetUniformLocation(_program->get(), "u_pointLightPosition"), 1, glm::value_ptr(_pointLightPosition));
            // glUniform1f(glGetUniformLocation(_program->get(), "u_pointLightIntensity"), _pointLightIntensity);
            // glUniform1i(glGetUniformLocation(_program->get(), "u_attenuationOrder"), _attenuationOrder);
            // glUniform1f(glGetUniformLocation(_program->get(), "u_ambientScale"), _ambientScale);
            // glUniform1f(glGetUniformLocation(_program->get(), "u_specularScale"), _specularScale);
            // glUniform1i(glGetUniformLocation(_program->get(), "u_useNormalMap"), int(_useNormalMap));
            // glUniform1f(glGetUniformLocation(_program->get(), "u_shininess"), _shininess);
            // glUniform3fv(glGetUniformLocation(_program->get(), "u_eyePos"), 1, glm::value_ptr(_camera.getPosition()));
            // glUniform1i(glGetUniformLocation(_program->get(), "u_diffuseMap"), 0);
            // glUniform1i(glGetUniformLocation(_program->get(), "u_normalMap"), 1);
            // glBindVertexArray(_vao->get());
            // glDrawElements(GL_TRIANGLES, _square.indices.size(), GL_UNSIGNED_INT, 0);

            // glUseProgram(_flatProgram->get());
            // glUniformMatrix4fv(glGetUniformLocation(_flatProgram->get(), "u_transform"), 1, false, glm::value_ptr(cameraTransform));
            // glUniformMatrix4fv(glGetUniformLocation(_flatProgram->get(), "u_translate"), 1, false, glm::value_ptr(bulbTransform));
            // glUniform3fv(glGetUniformLocation(_flatProgram->get(), "u_color"), 1, glm::value_ptr(_pointLightColor));
            // glBindVertexArray(_flatVao->get());
            // glDrawElements(GL_TRIANGLES, _sphere.indices.size(), GL_UNSIGNED_INT, 0);
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