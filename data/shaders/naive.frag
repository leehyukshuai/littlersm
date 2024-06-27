#version 330 core

in vec2 uv_fs;

uniform bool use_base_color;
uniform sampler2D base_color;
uniform vec4 base_color_factor;

layout(location = 0) out vec4 frag_color;

void main() {
  vec3 flat_color = use_base_color ? texture(base_color, uv_fs).rgb : base_color_factor.rgb;
  frag_color = flat_color;
}