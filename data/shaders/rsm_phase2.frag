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
uniform samplerCube positionMap;
uniform samplerCube normalMap;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

uniform float far_plane;

float calcShadow(vec3 fragPos)
{
    // Get vector between fragment position and light position
    vec3 fragToLight = fragPos - lightPos;
    // Use the light to fragment vector to sample from the depth map
    float closestDepth = texture(depthMap, fragToLight).r;
    // It is currently in linear range between [0,1]. Re-transform back to original value
    closestDepth *= far_plane;
    // Now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    // Now calculate shadows
    float bias = 0.05;
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    return shadow;
}

vec3 shade(vec3 lightIntensity, vec3 lightDir, vec3 normal, vec3 viewDir, vec3 diffuseColor, vec3 specularColor, float shininess) {
    vec3  diffuse = max(dot(lightDir, normal), 0.0) * diffuseColor * lightIntensity;
    vec3  specular = (shininess == 0 ? 1.: pow(max(dot(normal, normalize(lightDir + viewDir)), 0.0), 64)) * specularColor * lightIntensity;
    return diffuse + specular;
}

void main()
{
    // 1. direct lighting
    vec3 directLighting = vec3(0, 0, 0);
    vec3 color = use_base_color ? texture(base_color, fs_in.TexCoords).rgb : base_color_factor.rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    // calculate attenuation
    float lightDist = length(lightPos - fs_in.FragPos);
    float attenuation = 0.6 / (lightDist * lightDist);
    vec3 directLightIntensity = lightColor * attenuation;
    directLighting = shade(directLightIntensity, lightDir, normal, viewDir, color, color, 64.0);
    float shadow = calcShadow(fs_in.FragPos);
    directLighting *= (1.0 - shadow);

    // 2. indirect lighting
    vec3 indirectLighting = vec3(0, 0, 0);
    // int sample_num = 100;
    // for (int i = 0; i < sample_num; ++i) {
    //     vec3 coord = normalize(fs_in.FragPos - lightPos);
    //     // sample point nearing coord
    //     vec3 sampleCoord = coord;       // TODO: randomize sampleCoord
    //     vec3 patchPosition = texture(positionMap, sampleCoord).xyz;
    //     vec3 patchFlux = texture(fluxMap, sampleCoord).xyz;
    //     vec3 patchNormal = texture(normalMap, sampleCoord).xyz;
    //     vec3 deltaPos = fs_in.FragPos - patchPosition;
    //     vec3 indirectLightDir = -normalize(deltaPos);
    //     vec3 indirectLightIntensity = patchFlux * max(0, dot(patchNormal, deltaPos)) * max(0, dot(fs_in.Normal, -deltaPos)) / pow(length(deltaPos), 4.0);
    //     // blin-phong model
    //     indirectLighting += shade(indirectLightIntensity, indirectLightDir, normal, viewDir, color, color, 64.0);
    // }

    // 3. sum up
    FragColor = vec4(directLighting + indirectLighting, 1.0);
}
