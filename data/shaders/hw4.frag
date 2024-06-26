#version 330 core

in  vec3 v_Position;
in  vec3 v_Normal;
in  vec2 v_Coord;
in  mat3 v_TBN;

uniform float u_directLightIntensity;
uniform vec3 u_directLightColor;
uniform vec3 u_directLightDirection;

uniform float u_pointLightIntensity;
uniform vec3 u_pointLightColor;
uniform vec3 u_pointLightPosition;
uniform int u_attenuationOrder;

uniform vec3 u_eyePos;

uniform float u_ambientScale;
uniform float u_specularScale;
uniform float u_shininess;

uniform bool u_useNormalMap;

uniform sampler2D u_diffuseMap;
uniform sampler2D u_normalMap;

vec3 Shade(vec3 lightIntensity, vec3 lightDir, vec3 normal, vec3 viewDir, vec3 diffuseColor, vec3 specularColor, float shininess) {
    vec3  diffuse = max(dot(lightDir, normal), 0.0) * diffuseColor * lightIntensity;
    vec3  specular = (shininess == 0 ? 1.: pow(max(dot(normal, normalize(lightDir + viewDir)), 0.0), shininess)) * specularColor * lightIntensity;
    return diffuse + specular;
}

void main() {
    float gamma = 2.2;

    vec3 normal = v_Normal;
    if (u_useNormalMap) {
        normal = texture(u_normalMap, v_Coord).rgb;
        normal = normalize(normal * 2.0 - 1.0);
        normal = normalize(v_TBN * normal);
    }

    vec3 viewDir = normalize(u_eyePos - v_Position);
    vec3 directLightDir = normalize(u_directLightDirection);
    vec3 pointLightDir = normalize(u_pointLightPosition - v_Position);
    float pointLightDist = length(u_pointLightPosition - v_Position);
    float pointLightAttenuation  = 1. / (u_attenuationOrder == 2 ? pointLightDist * pointLightDist : (u_attenuationOrder == 1  ? pointLightDist : 1.));

    vec3 texColor = pow(texture(u_diffuseMap, v_Coord).rgb, vec3(gamma));

    vec3 total = u_ambientScale * texColor;
    total += Shade(u_directLightIntensity * u_directLightColor, directLightDir, normal, viewDir, texColor, vec3(u_specularScale), u_shininess);
    total += Shade(u_pointLightIntensity * u_pointLightColor, pointLightDir, normal, viewDir, texColor, vec3(u_specularScale), u_shininess) * pointLightAttenuation;
    
    gl_FragColor = vec4(pow(total, vec3(1. / gamma)), 1.0);
}
