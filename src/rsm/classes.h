#include <GL/glew.h>

class TextureCube {
public:
    TextureCube(unsigned size, GLenum data_type, GLenum format, GLenum internal_format) {
        glGenTextures(1, &_tex_id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _tex_id);
        for (GLuint i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internal_format, size, size, 0, format, data_type, nullptr);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    ~TextureCube() {
        glDeleteTextures(1, &_tex_id);
    }

    GLuint get() const {
        return _tex_id;
    }

private:
    GLuint _tex_id;
};

class FrameBuffer {
public:
    FrameBuffer() {
        glGenFramebuffers(1, &_id);
    }
    ~FrameBuffer() {
        glDeleteBuffers(1, &_id);
    }

    GLuint get() const {
        return _id;
    }

private:
    GLuint _id;
};
