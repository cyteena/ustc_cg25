#version 430 core // 4.30 core version GLSL

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
}; // define a shader Storage Buffer Object - SSBO, named lightsBuffer
// most store 4 Light structs, send to GPU at the same time

uniform vec2 iResolution;

// Uniform texture sampler, used to read data from G-buffer texture
uniform sampler2D diffuseColorSampler;
uniform sampler2D normalMapSampler; // You should apply normal mapping in rasterize_impl.fs
uniform sampler2D metallicRoughnessSampler;
uniform sampler2DArray shadow_maps; // multiple lights generate the shallow map. each layer corresponds to one shallow map of one light
uniform sampler2D position;

// uniform float alpha;
uniform vec3 camPos;

uniform int light_count;

layout(location = 0) out vec4 Color; // Define the output of the shader, location = 0 shows that this is the first color output

void main() { // the main function of the shader
vec2 uv = gl_FragCoord.xy / iResolution; // calculation the normalized texture location (uv) of the current fragment in screen space

vec3 pos = texture2D(position,uv).xyz; // read the world location of the current fragment from position texture
vec3 normal = texture2D(normalMapSampler,uv).xyz; // read the normal vector of the current fragment from the normalMapSampler texture, make sure that is normalized

vec4 metalnessRoughness = texture2D(metallicRoughnessSampler,uv); // read metal and roughness
float metal = metalnessRoughness.x;
float roughness = metalnessRoughness.y;
Color = vec4(0,0,0,1); // the initial output color is black
for(int i = 0; i < light_count; i ++) { // traverse all the moving lights

float shadow_map_value = texture(shadow_maps, vec3(uv, lights[i].shadow_map_id)).x;

// Visualization of shadow map
Color += vec4(shadow_map_value, 0, 0, 1);

// HW6_TODO: first comment the line above ("Color +=..."). That's for quick Visualization.
// You should first do the Blinn Phong shading here. You can use roughness to modify alpha. Or you can pass in an alpha value through the uniform above.

vec3 albedo = texture(diffuseColorSampler, uv).rgb; // 获取表面基色

//高光指数 shininess 越大， 高光点越小越亮
//根据roughness (0 = smooth, 1 = rough)来映射, 例如
float shininess = mix(256.0, 2.0 , roughness * roughness);

//2. 计算光照所需向量
vec3 lightDir = normalize(lights[i].position - pos); // 指定光源方向
vec3 viewDir = normalize(camPos - pos); // 指定相机方向
vec3 halfwayDir = normalize(lightDir + viewDir); // Blinn-Phong 使用的半程向量
vec3 N = normalize(normal); // 法线归一化

//3. 计算光照衰减
// 光源半径平方反比
float distanceToLight = length(lights[i].position  - pos);
// 基于半径的简单二次衰减:
float attenuation = pow(max(0.0, 1.0 - distanceToLight / lights[i].radius), 2.0);
// 如果光源半径为0或者负数，则不衰减
if (lights[i].radius <= 0.0){
    attenuation = 1.0;
}

//4. diffuse
float diff = max(dot(N, lightDir), 0.0);
vec3 diffuse = lights[i].color * diff * albedo;

//5. Specular (镜面反射)
float spec = pow(max(dot(N, halfwayDir), 0.0), shininess);
vec3 specular = lights[i].color * spec;

// After finishing Blinn Phong shading, you can do shadow mapping with the help of the provided shadow_map_value. You will need to refer to the node, node_render_shadow_mapping.cpp, for the light matrices definition. Then you need to fill the mat4 light_projection; mat4 light_view; with similar approach that we fill position and color.
// For shadow mapping, as is discussed in the course, you should compare the value "position depth from the light's view" against the "blocking object's depth.", then you can decide whether it's shadowed.

//6. 变换片段位置到光源的裁剪空间
vec4 posLightSpace = lights[i].light_projection * lights[i].light_view * vec4(pos, 1.0);

//7. 执行透视除法，得到归一化的设备坐标(NDC), 范围[-1, 1]
vec3 projCoords = posLightSpace.xyz / posLightSpace.w;

//8. 将NDC坐标变换到纹理坐标范围[0, 1]
// - xy 用于采样阴影贴图
// - z 是当前片段在光源视角下的深度值
vec2 shadowMapUV = projCoords.xy * 0.5 + 0.5;
float currentDepth = projCoords.z * 0.5  + 0.5;

// 9. 阴影偏移/Bias, 防止自阴影(shadow Acne)
float bias = max(0.005 * (1.0 - dot(N, lightDir)), 0.0005);


// 10. 采样阴影贴图，获取改为支持存储的最小深度值（来自遮挡物）
float shadowMapDepth = texture(shadow_maps, vec3(shadowMapUV, lights[i].shadow_map_id)).r;

//11. 比较深度值，判断是否在阴影中
//note: 如果currentDepth 比 shadowMapDepth 大 (加上bias), 说明当前片段比遮挡物更远，处于阴影中

float shadow = 0.0;
if(shadowMapUV.x > 0.0 && shadowMapUV.x < 1.0 && shadowMapUV.y > 0.0 && shadowMapUV.y < 1.0 && currentDepth > )


// PCSS is also applied here.
}

}