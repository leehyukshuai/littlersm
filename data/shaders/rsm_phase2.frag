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

vec3 randomBiasVec(vec3 vec, float theta, int seed) {
    vec3 ret = vec3(0.0), axis = vec3(0.0);
    
    vec = normalize(vec);

    if (vec.x < 0.001) {
        axis = vec3(0, vec.z, -vec.y);
    }
    else if (vec.y < 0.001) {
        axis = vec3(vec.x, 0, -vec.z);
    }
    else {
        axis = vec3(vec.y, -vec.x, 0);
    }

    axis = normalize(axis);

    vec3 randomVec = texelFetch(randomMap, ivec2(seed, 0), 0).xyz;

    // we need 2 random numbers
    float r1 = randomVec.x, r2 = randomVec.y;

    // get random axis
    float theta1 = r1 * 6.283 * 16;

    vec4 quad1 = vec4(cos(theta1 / 2), sin(theta1 / 2) * vec);
    mat3 rotate_mat1 = quad2mat3(quad1);

    axis = normalize(rotate_mat1 * axis);

    // get random vector
    theta1 = r2 * theta;

    quad1 = vec4(cos(theta1 / 2), sin(theta1 / 2) * axis);
    rotate_mat1 = quad2mat3(quad1);

    ret = normalize(rotate_mat1 * vec);

    return ret;
}

void main()
{
    // // For Debug:
    // vec3 coord = normalize(fs_in.FragPos - lightPos);
    // float result = 0;
    // int sample_num = 50;
    // for (int i = 0; i < sample_num; ++i) {
    //     vec3 sampleCoord = randomBiasVec(coord, 0.0, i);
    //     float depth = texture(depthMap, sampleCoord).x * 255;
    //     result += depth;
    // }
    // result /= sample_num;
    // FragColor = vec4(result, result, result, 1.0);

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
