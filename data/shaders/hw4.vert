#version 330 core

layout(location = 0) in  vec3 a_Position;
layout(location = 1) in  vec3 a_Normal;
layout(location = 2) in  vec2 a_Coord;
layout(location = 3) in  vec3 a_Tangent;

out vec3 v_Position;
out vec3 v_Normal;
out vec2 v_Coord;
out mat3 v_TBN;

uniform mat4 u_transform;

void main() {
  vec3 T = normalize(a_Tangent);
  vec3 N = normalize(a_Normal);
  vec3 B = cross(N, T);
  
  gl_Position = u_transform * vec4(a_Position, 1.0f);

  v_Position = a_Position;
  v_Coord = a_Coord;
  v_Normal = N;
  v_TBN = mat3(T, B, N);
}
