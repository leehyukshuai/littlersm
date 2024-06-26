#version 330 core

in vec2 uv_fs;

uniform bool use_base_color;
uniform sampler2D base_color;
uniform vec4 base_color_factor;

layout(location = 0) out vec4 frag_color;

void main() {
  frag_color = use_base_color ? vec4(texture(base_color, uv_fs).rgb, 1.0) : base_color_factor;
}