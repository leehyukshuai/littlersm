#pragma once
#include "../common/application.hpp"
#include "camera.h"

class App : public Application {
protected:
    camera::Camera _camera;
    bool           _left_button_pressed {false};
    double         _last_xpos, _last_ypos;
    bool           _disableControl { false };

public:
    App(const char * name, int width, int height):
        Application(name, width, height) {}

public:
    void update() override {
        if (!_disableControl) _camera.update(getDelta(), _window);
    }

private:
    void cursor_position_callback(double xpos, double ypos) override {
        if (_left_button_pressed) {
            float dx   = xpos - _last_xpos;
            float dy   = ypos - _last_ypos;
            _last_xpos = xpos;
            _last_ypos = ypos;
            _camera.rotate(dx, dy);
        }
    }

    void scroll_callback(double xoffset, double yoffset) override {
        _camera.scroll(yoffset);
    }
    void mouse_button_callback(int button, int action, int mods) override {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                _left_button_pressed = true;
                glfwGetCursorPos(_window, &_last_xpos, &_last_ypos);
            } else if (action == GLFW_RELEASE) {
                _left_button_pressed = false;
            }
        }
    }
};