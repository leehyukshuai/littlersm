#pragma once
#include <glm/glm.hpp>
#include <vector>
namespace mesh {
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 coord;
        glm::vec3 tangent;
    };

    struct Mesh {
        std::vector<Vertex>   vertices;
        std::vector<unsigned> indices;
    };

    Mesh makeSquare(float size, int tiles = 1) {
        Mesh  ret;
        float l      = size / 2.0f;
        ret.vertices = std::vector<Vertex> {
            {  { -l, 0.0f, l }, { 0, 1, 0 },         { 0, 0 }, { 1, 0, 0 } },
            { { -l, 0.0f, -l }, { 0, 1, 0 },     { 0, tiles }, { 1, 0, 0 } },
            {  { l, 0.0f, -l }, { 0, 1, 0 }, { tiles, tiles }, { 1, 0, 0 } },
            {   { l, 0.0f, l }, { 0, 1, 0 },     { tiles, 0 }, { 1, 0, 0 } },
        };
        ret.indices = std::vector<unsigned> {
            0, 1, 2, 0, 2, 3
        };
        return ret;
    }

    Mesh makeCube(float size) {
        Mesh  ret;
        float l      = size / 2.0f;
        ret.vertices = std::vector<Vertex> {
            { { -l, -l, -l }, { 0.0f, 0.0f, -1.0f }, {}, {} },
            {  { l, -l, -l }, { 0.0f, 0.0f, -1.0f }, {}, {} },
            {   { l, l, -l }, { 0.0f, 0.0f, -1.0f }, {}, {} },
            {   { l, l, -l }, { 0.0f, 0.0f, -1.0f }, {}, {} },
            {  { -l, l, -l }, { 0.0f, 0.0f, -1.0f }, {}, {} },
            { { -l, -l, -l }, { 0.0f, 0.0f, -1.0f }, {}, {} },
            {  { -l, -l, l },  { 0.0f, 0.0f, 1.0f }, {}, {} },
            {   { l, -l, l },  { 0.0f, 0.0f, 1.0f }, {}, {} },
            {    { l, l, l },  { 0.0f, 0.0f, 1.0f }, {}, {} },
            {    { l, l, l },  { 0.0f, 0.0f, 1.0f }, {}, {} },
            {   { -l, l, l },  { 0.0f, 0.0f, 1.0f }, {}, {} },
            {  { -l, -l, l },  { 0.0f, 0.0f, 1.0f }, {}, {} },
            {   { -l, l, l }, { -1.0f, 0.0f, 0.0f }, {}, {} },
            {  { -l, l, -l }, { -1.0f, 0.0f, 0.0f }, {}, {} },
            { { -l, -l, -l }, { -1.0f, 0.0f, 0.0f }, {}, {} },
            { { -l, -l, -l }, { -1.0f, 0.0f, 0.0f }, {}, {} },
            {  { -l, -l, l }, { -1.0f, 0.0f, 0.0f }, {}, {} },
            {   { -l, l, l }, { -1.0f, 0.0f, 0.0f }, {}, {} },
            {    { l, l, l },  { 1.0f, 0.0f, 0.0f }, {}, {} },
            {   { l, l, -l },  { 1.0f, 0.0f, 0.0f }, {}, {} },
            {  { l, -l, -l },  { 1.0f, 0.0f, 0.0f }, {}, {} },
            {  { l, -l, -l },  { 1.0f, 0.0f, 0.0f }, {}, {} },
            {   { l, -l, l },  { 1.0f, 0.0f, 0.0f }, {}, {} },
            {    { l, l, l },  { 1.0f, 0.0f, 0.0f }, {}, {} },
            { { -l, -l, -l }, { 0.0f, -1.0f, 0.0f }, {}, {} },
            {  { l, -l, -l }, { 0.0f, -1.0f, 0.0f }, {}, {} },
            {   { l, -l, l }, { 0.0f, -1.0f, 0.0f }, {}, {} },
            {   { l, -l, l }, { 0.0f, -1.0f, 0.0f }, {}, {} },
            {  { -l, -l, l }, { 0.0f, -1.0f, 0.0f }, {}, {} },
            { { -l, -l, -l }, { 0.0f, -1.0f, 0.0f }, {}, {} },
            {  { -l, l, -l },  { 0.0f, 1.0f, 0.0f }, {}, {} },
            {   { l, l, -l },  { 0.0f, 1.0f, 0.0f }, {}, {} },
            {    { l, l, l },  { 0.0f, 1.0f, 0.0f }, {}, {} },
            {    { l, l, l },  { 0.0f, 1.0f, 0.0f }, {}, {} },
            {   { -l, l, l },  { 0.0f, 1.0f, 0.0f }, {}, {} },
            {  { -l, l, -l },  { 0.0f, 1.0f, 0.0f }, {}, {} },
        };
        ret.indices = std::vector<unsigned>(36);
        for (int i = 0; i < 36; ++i) ret.indices[i] = i;
        return ret;
    }

    Mesh makeSphere(unsigned precision, float radius) {
        Mesh ret;

        int mNumVertices = (precision + 1) * (precision + 1);
        int mNumIndices  = precision * precision * 6;
        ret.vertices.resize(mNumVertices);
        ret.indices.resize(mNumIndices);

        for (int i = 0; i <= precision; i++) {
            for (int j = 0; j <= precision; j++) {
                float y   = static_cast<float>(cos(glm::radians(180.0f - i * 180.0f / (float) precision)));
                float tmp = glm::radians(j * 360 / (float) precision);
                float x   = static_cast<float>(-cos(tmp) * abs(cos(asin(y))));
                float z   = static_cast<float>(sin(tmp) * abs(cos(asin(y))));
                int   idx = i * (precision + 1) + j;

                ret.vertices[idx].position = radius * glm::vec3(x, y, z);
                ret.vertices[idx].normal   = glm::vec3(x, y, z);
                ret.vertices[idx].coord    = glm::vec2();
                ret.vertices[idx].tangent  = glm::vec3();
            }
        }

        for (int i = 0; i < precision; i++) {
            for (int j = 0; j < precision; j++) {
                int base              = 6 * (i * precision + j);
                ret.indices[base]     = i * (precision + 1) + j;
                ret.indices[base + 1] = i * (precision + 1) + j + 1;
                ret.indices[base + 2] = (i + 1) * (precision + 1) + j;
                ret.indices[base + 3] = i * (precision + 1) + j + 1;
                ret.indices[base + 4] = (i + 1) * (precision + 1) + j + 1;
                ret.indices[base + 5] = (i + 1) * (precision + 1) + j;
            }
        }
        return ret;
    }
} // namespace mesh