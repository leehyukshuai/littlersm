#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform bool use_base_color;
uniform sampler2D base_color;
uniform vec4 base_color_factor;

uniform sampler2D randomMap;
uniform samplerCube depthMap;
uniform samplerCube fluxMap;
uniform samplerCube normalMap;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

uniform bool disableDirectLight;
uniform bool disableIndirectLight;
uniform float indirectLightPower;
uniform float directLightPower;
uniform float sampleRange;
uniform int sampleNum;

uniform float far_plane;

float calcShadow(vec3 fragPos)
{
    vec3 fragToLight = fragPos - lightPos;
    float closestDepth = texture(depthMap, fragToLight).r;
    // Re-transform back to original depth value
    closestDepth *= far_plane;
    float currentDepth = length(fragToLight);
    float bias = 0.05;
    float shadow = currentDepth - bias > closestDepth ? 0.9 : 0.0;
    return shadow;
}

vec3 shade(vec3 lightIntensity, vec3 lightDir, vec3 normal, vec3 viewDir, vec3 diffuseColor, vec3 specularColor, float shininess) {
    vec3  diffuse = max(dot(lightDir, normal), 0.0) * diffuseColor * lightIntensity;
    vec3  specular = (shininess == 0 ? 1.0 : pow(max(dot(normal, normalize(lightDir + viewDir)), 0.0), 64)) * specularColor * lightIntensity;
    return diffuse + specular;
}

vec3 randomBiasVec(vec3 vec, float sinTheta, vec2 randomVec) {
    vec3 vert1 = vec3(0), vert2 = vec3(0);
    vert1 = vec3(vec.y, -vec.x, 0) + vec3(vec.z, 0, -vec.x) + vec3(0, vec.z, -vec.y);
    vert1 = normalize(vert1);
    vert2 = cross(vec, vert1);

    randomVec = randomVec * 2 - vec2(1.0);
    return normalize(vec + sinTheta * (randomVec.x * vert1 + randomVec.y * vert2));
}

void main()
{
    // 1. direct lighting
    vec3 directLighting = vec3(0, 0, 0);
    vec3 color = use_base_color ? texture(base_color, fs_in.TexCoords).rgb : base_color_factor.rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    float lightDist = length(lightPos - fs_in.FragPos);
    float attenuation = 0.6 / (lightDist * lightDist);
    vec3 directLightIntensity = lightColor * attenuation;
    directLighting = shade(directLightIntensity, lightDir, normal, viewDir, color, color, 64.0);
    float shadow = calcShadow(fs_in.FragPos);
    directLighting *= 1.0 - shadow;
    directLighting *= vec3(!disableDirectLight);

    // 2. indirect lighting
    vec3 indirectLighting = vec3(0, 0, 0);
    vec3 coord = normalize(fs_in.FragPos - lightPos);
    for (int i = 0; i < sampleNum; ++i) {
        // TODO: 修改随机采样函数，增加对于权重的处理
        vec3 r = texelFetch(randomMap, ivec2(i, 0), 0).xyz;
        vec3 sampleCoord = randomBiasVec(coord, sampleRange, r.xy);
        // vec3 sampleCoord = normalize(coord + (r * 2 - vec3(1)) * sampleRange);
        float patchDepth = texture(depthMap, sampleCoord).x * far_plane;
        vec3 patchPosition = lightPos + patchDepth * sampleCoord;
        vec3 patchFlux = texture(fluxMap, sampleCoord).xyz;
        vec3 patchNormal = normalize(texture(normalMap, sampleCoord).xyz * 2.0 - vec3(1.0));
        vec3 deltaPos = fs_in.FragPos - patchPosition;
        vec3 indirectLightDir = -normalize(deltaPos);
        vec3 indirectLightIntensity = clamp(patchFlux * max(0, dot(patchNormal, deltaPos)) * max(0, dot(normal, -deltaPos)) / pow(dot(deltaPos, deltaPos) , 2.0), vec3(0), patchFlux);
        indirectLighting += shade(indirectLightIntensity, indirectLightDir, normal, viewDir, color, color, 64.0);
    }
    indirectLighting = clamp(indirectLighting / sampleNum, 0.0, 1.0);
    indirectLighting *= vec3(!disableIndirectLight);

    // 3. sum up
    FragColor = vec4(directLighting * directLightPower + indirectLighting * indirectLightPower, 1.0);
}
