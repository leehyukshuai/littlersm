#version 330 core

layout (location = 0) out vec3 Position;
layout (location = 1) out vec3 Normal;
layout (location = 2) out vec3 Flux;

uniform bool use_base_color;
uniform sampler2D base_color;
uniform vec4 base_color_factor;
uniform float far_plane;
uniform vec3 lightPos;
uniform vec3 lightColor;

in GS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

vec3 shade(vec3 lightIntensity, vec3 lightDir, vec3 normal, vec3 viewDir, vec3 diffuseColor, vec3 specularColor, float shininess) {
    vec3  diffuse = max(dot(lightDir, normal), 0.0) * diffuseColor * lightIntensity;
    return diffuse;
}

void main()
{
    // get distance between fragment and light source
    float lightDistance = length(fs_in.FragPos.xyz - lightPos);
    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / far_plane;
    // write this as modified depth
    gl_FragDepth = lightDistance;

    // Position = fs_in.FragPos;
    Position = vec3((fs_in.FragPos.x + 1.0) / 2.0, fs_in.FragPos.y / 2.0, (fs_in.FragPos.z + 1.0) / 2.0 );

    // Normal = fs_in.Normal;
    Normal = (normalize(fs_in.Normal) + vec3(1.0)) / 2.0;

    vec3 directLighting = vec3(0, 0, 0);
    vec3 color = use_base_color ? texture(base_color, fs_in.TexCoords).rgb : base_color_factor.rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    // calculate attenuation
    float lightDist = length(lightPos - fs_in.FragPos);
    float attenuation = 0.6 / (lightDist * lightDist);
    Flux = max(dot(lightDir, normal), 0.0) * color * lightColor * attenuation;
}