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

// for ImGui
uniform bool disableDirLight;
uniform float indirectLightPower;
uniform float sampleRange;
uniform int sampleNum;

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
    vec3  specular = (shininess == 0 ? 1.0 : pow(max(dot(normal, normalize(lightDir + viewDir)), 0.0), 64)) * specularColor * lightIntensity;
    return diffuse + specular;
}

mat3 quad2mat3(vec4 quad) {
    float xx = quad.x * quad.x, yy = quad.y * quad.y, zz = quad.z * quad.z, ww = quad.w * quad.w,
        xy = quad.x * quad.y, yz = quad.y * quad.z, zw = quad.z * quad.w, 
        xz = quad.x * quad.z, xw = quad.x * quad.w, yw = quad.y * quad.w;
    return mat3 (
        xx + yy - zz - ww, 2*(yz - xw), 2*(xz + yw),
        2*(xw + yz), xx - yy + zz - ww, 2*(zw - xy),
        2*(yw - xz), 2*(xy + zw), xx - yy - zz + ww
    );
}

vec3 randomBiasVec(vec3 vec, float sinTheta, vec2 randomVec) {

    vec = normalize(vec);

    vec3 vert1 = vec3(0), vert2 = vec3(0);
    if (vec.x < 0.001) {
        vert1 = vec3(0, vec.z, -vec.y);
    }
    else if (vec.y < 0.001) {
        vert1 = vec3(vec.z, 0, -vec.x);
    }
    else {
        vert1 = vec3(vec.y, -vec.x, 0);
    }
    vert1 = normalize(vert1);
    vert2 = normalize(cross(vec, vert1));

    randomVec = randomVec * 2 - vec2(1.0);
    return normalize(vec + sinTheta * (randomVec.x * vert1 + randomVec.y * vert2));

    // vec3 randomVec = fract(vec3(sin(seed * 128.233), sin(seed * seed * 1241.431), sin(seed * seed * seed * 97.5354312)));
    // randomVec = randomVec * 2 - vec3(1.0);

    // return normalize(vec + sinTheta * randomVec);
}

void main()
{
    // // For Flux Debug
    // vec3 coord = normalize(fs_in.FragPos - lightPos);
    // vec3 flux = vec3(texture(positionMap, coord));
    // FragColor = vec4(flux, 1.0);

    // // For Debug:
    // vec3 coord = normalize(fs_in.FragPos - lightPos);
    // float result = 0;
    // vec3 flux_result = vec3(0);
    // int sample_num = 600;
    // for (int i = 0; i < sample_num; ++i) {
    //     vec3 r = texelFetch(randomMap, ivec2(i, 0), 0).xyz;
    //     vec3 sampleCoord = randomBiasVec(coord, 0.3, r.xy);
    //     float depth = texture(depthMap, sampleCoord).x * 255;
    //     result += depth;

    //     vec3 flux = vec3(texture(fluxMap, sampleCoord));
    //     flux_result += flux;
    // }
    // result /= sample_num;
    // flux_result /= sample_num;
    // FragColor = vec4(flux_result, 1.0);

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

    if (disableDirLight) {
        directLighting = vec3(0,0,0);
    }

    // 2. indirect lighting
    vec3 indirectLighting = vec3(0, 0, 0);
    int sample_num = sampleNum;
    vec3 coord = normalize(fs_in.FragPos - lightPos);
    for (int i = 0; i < sample_num; ++i) {
        // sample point nearing coord
        vec3 r = texelFetch(randomMap, ivec2(i, 0), 0).xyz;
        // vec3 sampleCoord = randomBiasVec(coord, sampleRange, r.xy);
        vec3 sampleCoord = normalize(coord + (r * 2 - vec3(1)) * sampleRange);
        vec3 patchPosition = texture(positionMap, sampleCoord).xyz;
        patchPosition = vec3(patchPosition.x * 2 - 1, patchPosition.y * 2, patchPosition.z * 2 - 1);
        vec3 patchFlux = texture(fluxMap, sampleCoord).xyz;
        vec3 patchNormal = normalize(texture(normalMap, sampleCoord).xyz * 2.0 - vec3(1.0));
        vec3 deltaPos = fs_in.FragPos - patchPosition;
        vec3 indirectLightDir = -normalize(deltaPos);
        // vec3 indirectLightIntensity = clamp(patchFlux * max(0, dot(patchNormal, deltaPos)) * max(0, dot(normal, -deltaPos)) / pow(length(deltaPos) , 4.0), vec3(0), patchFlux * 300);
        vec3 indirectLightIntensity = patchFlux * max(0, dot(patchNormal, deltaPos)) * max(0, dot(normal, -deltaPos)) / pow(deltaPos.x * deltaPos.x + deltaPos.y * deltaPos.y + deltaPos.z * deltaPos.z , 2.0);
        // blin-phong model
        indirectLighting += indirectLightIntensity;
        // indirectLighting += shade(indirectLightIntensity, indirectLightDir, normal, viewDir, color, color, 64.0);
    }
    indirectLighting = clamp (indirectLighting/sample_num, 0.0, 1.0);

    // // 3. sum up
    FragColor = vec4(indirectLighting * indirectLightPower + directLighting, 1.0);
    // FragColor = vec4(indirectLighting, 1.0);
}
