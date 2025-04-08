#version 430 core

// Define a uniform struct for lights
struct Light {
    // The matrices are used for shadow mapping. You need to fill it according to how we are filling it when building the normal maps (node_render_shadow_mapping.cpp). 
    // Now, they are filled with identity matrix. You need to modify C++ code innode_render_deferred_lighting.cpp.
    // Position and color are filled.
    mat4 light_projection;
    mat4 light_view;
    vec3 position;
    float radius;
    vec3 color; // Just use the same diffuse and specular color.
    int shadow_map_id;
};

layout(binding = 0) buffer lightsBuffer {
Light lights[4];
};

uniform vec2 iResolution;

uniform sampler2D diffuseColorSampler;
uniform sampler2D normalMapSampler; // You should apply normal mapping in rasterize_impl.fs
uniform sampler2D metallicRoughnessSampler;
uniform sampler2DArray shadow_maps;
uniform sampler2D position;

// uniform float alpha;
uniform vec3 camPos;

uniform int light_count;

layout(location = 0) out vec4 Color;

void main() {
vec2 uv = gl_FragCoord.xy / iResolution;

vec3 pos = texture2D(position,uv).xyz;
vec3 normal = texture2D(normalMapSampler,uv).xyz;

vec4 metalnessRoughness = texture2D(metallicRoughnessSampler,uv);
float metal = metalnessRoughness.x;
float roughness = metalnessRoughness.y;
Color = vec4(0,0,0,1);
for(int i = 0; i < light_count; i ++) {

float shadow_map_value = texture(shadow_maps, vec3(uv, lights[i].shadow_map_id)).x;

// // Visualization of shadow map
// Color += vec4(shadow_map_value, 0, 0, 1);

// HW6_TODO: first comment the line above ("Color +=..."). That's for quick Visualization.
// You should first do the Blinn Phong shading here. You can use roughness to modify alpha. Or you can pass in an alpha value through the uniform above.

// 1. 获取材质属性
vec3 albedo = texture(diffuseColorSampler, uv).rgb; // 获取表面基色

// 可以根据 roughness (0=光滑, 1=粗糙) 来映射，例如:
float shininess = mix(256.0, 2.0, roughness * roughness); // 从 2 到 256，粗糙度影响平方关系

// 2. 计算光照所需向量
vec3 lightDir = normalize(lights[i].position - pos); // 指向光源的方向
vec3 viewDir = normalize(camPos - pos); // 指向相机的方向
vec3 halfwayDir = normalize(lightDir + viewDir); // Blinn-Phong 使用的半程向量
vec3 N = normalize(normal); // 确保法线是归一化的


// 3. 计算光照衰减 (可选，但推荐)
// 可以使用光源半径或距离平方反比等方式
float distanceToLight = length(lights[i].position - pos);
// 基于半径的简单二次衰减:
float attenuation = pow(max(0.0, 1.0 - distanceToLight / lights[i].radius), 2.0);
// 或者简单的距离平方反比 (调整常数来控制衰减快慢):
// float attenuation = 1.0 / (1.0 + 0.1 * distanceToLight + 0.05 * distanceToLight * distanceToLight);
// 如果光源半径为0或负数，则不进行衰减
if (lights[i].radius <= 0.0) {
    attenuation = 1.0;
}

// 5. 计算镜面反射 (Specular)
// 注意：Blinn-Phong的镜面反射通常用白色或光源颜色，而不是albedo
// 这里我们用光源颜色作为镜面反射的基础强度
float spec = pow(max(dot(N, halfwayDir), 0.0), shininess); // (N dot H)^shininess
vec3 specular = lights[i].color * spec; // * vec3(1.0); // 可以乘以一个镜面反射系数，但简单起见先省略

// --- 阴影映射部分 ---

// 6. 变换片段位置到光源的裁剪空间
vec4 posLightSpace = lights[i].light_projection * lights[i].light_view * vec4(pos, 1.0);

// 7. 执行透视除法，得到归一化设备坐标 (NDC)，范围[-1, 1]
vec3 projCoords = posLightSpace.xyz / posLightSpace.w;

// 8. 将NDC坐标变换到纹理坐标范围 [0, 1]
//  - xy 用于采样阴影贴图
//  - z  是当前片段在光源视角下的深度值
vec2 shadowMapUV = projCoords.xy * 0.5 + 0.5;
float currentDepth = projCoords.z * 0.5 + 0.5; // 将 [-1, 1] 的深度映射到 [0, 1]

// 9. (重要) 阴影偏移/Bias，防止自阴影 (Shadow Acne)
// 可以是一个小常数，或者基于法线和光线方向计算 slope scale bias
float bias = max(0.005 * (1.0 - dot(N, lightDir)), 0.0005); // 基于坡度的偏置

// 10. 采样阴影贴图，获取该位置存储的最小深度值 (来自遮挡物)
// 使用 shadow_map_id 作为纹理数组的层索引
float shadowMapDepth = texture(shadow_maps, vec3(shadowMapUV, lights[i].shadow_map_id)).r; // 深度图通常存r通道

// 11. 比较深度值，判断是否在阴影中
// 注意：如果 currentDepth 比 shadowMapDepth 大 (加上 bias)，说明当前片段比遮挡物更远，处于阴影中
float shadow = 0.0; // 0.0 = 完全光照, 1.0 = 完全阴影

// 检查是否在光源视锥内 (shadowMapUV 在 [0,1] 范围内) 且 比阴影图深度深
if (shadowMapUV.x > 0.0 && shadowMapUV.x < 1.0 && shadowMapUV.y > 0.0 && shadowMapUV.y < 1.0 && currentDepth > shadowMapDepth + bias) {
    shadow = 1.0; // 在阴影中
}

// 12. (可选) PCSS 或 PCF 平滑阴影边缘 (这里先做硬阴影)
// PCSS/PCF 会在这里用 shadowMapUV 和 bias 进行更复杂的采样和计算

// 13. 结合光照和阴影
// 只有不在阴影中的部分 (shadow = 0.0) 才接收光照
vec3 lighting = attenuation * (diffuse + specular) * (1.0 - shadow);
Color.rgb += lighting;


// --- 阴影映射部分结束 ---
// --- HW6_TODO 结束 ---

// After finishing Blinn Phong shading, you can do shadow mapping with the help of the provided shadow_map_value. You will need to refer to the node, node_render_shadow_mapping.cpp, for the light matrices definition. Then you need to fill the mat4 light_projection; mat4 light_view; with similar approach that we fill position and color.
// For shadow mapping, as is discussed in the course, you should compare the value "position depth from the light's view" against the "blocking object's depth.", then you can decide whether it's shadowed.

// PCSS is also applied here.
}

}