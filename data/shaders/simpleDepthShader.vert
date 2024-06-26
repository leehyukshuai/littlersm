#version 330 core
layout (location = 0) in vec3 position;

uniform mat4 u_lightTransform;

void main()
{
    gl_Position = u_lightTransform * vec4(position, 1.0f);
}
