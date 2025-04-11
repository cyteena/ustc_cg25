#version 430 core

// Define a uniform struct for lights
struct Light {
    mat4 light_projection; // Projection matrix from light's perspective
    mat4 light_view;       // View matrix from light's perspective
    vec3 position;         // World space position of the light
    float radius;          // Radius for attenuation calculation
    vec3 color;            // Light color (used for both diffuse and specular)
    int shadow_map_id;     // Index for the shadow map texture array layer
    float lightWorldSize;  // Approximate size of the light source in world units (for PCSS penumbra)
                           // Needs to be set from C++
};

// Buffer containing light data
layout(binding = 0) buffer lightsBuffer {
    Light lights[];
};

// Screen resolution
uniform vec2 iResolution;

// Input textures from G-Buffer
uniform sampler2D diffuseColorSampler;      // Albedo / Base Color
uniform sampler2D normalMapSampler;         // World-space Normal
uniform sampler2D metallicRoughnessSampler; // Metallic (r) and Roughness (g)
uniform sampler2D position;                 // World-space Position

// Shadow map texture array
uniform sampler2DArray shadow_maps;

// Camera position in world space
uniform vec3 camPos;

// Number of active lights
uniform int light_count;

// Output color
layout(location = 0) out vec4 FragColor;

// --- PCSS Constants ---
const int BLOCKER_SEARCH_NUM_SAMPLES = 16;
const int PCF_NUM_SAMPLES = 32; // Note: poissonDisk array below only has 16 samples

const vec2 poissonDisk[16] = vec2[](
   vec2( -0.94201624, -0.39906216 ), vec2( 0.94558609, -0.76890725 ),
   vec2( -0.094184101, -0.92938870 ), vec2( 0.34495938, 0.29387760 ),
   vec2( -0.91588581, 0.45771432 ), vec2( -0.81544232, -0.87912464 ),
   vec2( -0.38277543, 0.27676845 ), vec2( 0.97484398, 0.75648379 ),
   vec2( 0.44323325, -0.97511554 ), vec2( 0.53742981, -0.47373420 ),
   vec2( -0.26496911, -0.41893023 ), vec2( 0.79197514, 0.19090188 ),
   vec2( -0.24188840, 0.99706507 ), vec2( -0.81409955, 0.91437590 ),
   vec2( 0.19984126, 0.78641367 ), vec2( 0.14383161, -0.14100790 )
);

// --- PCSS Helper Functions ---

// Step 1: Find average blocker depth
float findBlockerDepth(sampler2DArray shadowMapSampler, int layer, vec2 uv, float currentDepth, vec2 searchRegionRadiusUV) {
    float avgBlockerDepth = 0.0;
    int blockerCount = 0;

    for (int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; ++i) {
        vec2 offset = poissonDisk[i % 16];
        vec2 sampleUV = uv + offset * searchRegionRadiusUV;

        // Ensure sampling within bounds [0,1] might be needed depending on texture wrap mode
        // sampleUV = clamp(sampleUV, 0.0, 1.0); // Optional: Add if necessary

        float shadowMapDepth = texture(shadowMapSampler, vec3(sampleUV, layer)).r;

        if (shadowMapDepth < 1.0 && shadowMapDepth < currentDepth) { // Check it's not background and is a potential blocker
            avgBlockerDepth += shadowMapDepth;
            blockerCount++;
        }
    }

    if (blockerCount == 0) {
        return 1.0; // No blockers found
    }

    return avgBlockerDepth / float(blockerCount);
}

// Step 2: Estimate penumbra size based on blocker depth
float calculatePenumbraSize(float zReceiver, float zBlocker, float lightWorldSize, vec2 shadowMapTexelSize) {
    // Add small epsilon to prevent division by zero / instability near zero
    const float epsilon = 0.00001;

    // Blocker is not occluding or no blocker found
    if (zBlocker <= 0.0 || zBlocker >= zReceiver) {
        return 0.0; // No penumbra
    }

    // Formula: Penumbra = (ReceiverDepth - BlockerDepth) * LightSize / BlockerDepth
    float penumbraWorld = (zReceiver - zBlocker) * lightWorldSize / max(zBlocker, epsilon);

    // Convert world penumbra size roughly to UV space radius
    // This part remains a heuristic and needs tuning!
    // Dividing by zReceiver attempts to account for perspective, factor 2.0 is arbitrary.
    float penumbraRadiusPixels = penumbraWorld / (max(zReceiver, epsilon) * 2.0);

    // Convert pixel radius to UV radius
    return penumbraRadiusPixels * shadowMapTexelSize.x;
}


// Step 3: Perform PCF filtering over the estimated penumbra region
float pcfFilter(sampler2DArray shadowMapSampler, int layer, vec2 uv, float currentDepth, float bias, vec2 filterRadiusUV) {
    float shadow = 0.0;
    vec2 shadowMapTexelSize = 1.0 / textureSize(shadowMapSampler, 0).xy;

    // **FIXED**: Compare filterRadiusUV.x (float) with the threshold
    // If filter radius is very small (e.g., less than half a texel), just do a single sample
    if (filterRadiusUV.x < shadowMapTexelSize.x * 0.5) {
         float shadowMapDepth = texture(shadowMapSampler, vec3(uv, layer)).r;
         // Use bias in comparison
         if (shadowMapDepth < 1.0 && currentDepth > shadowMapDepth + bias) {
            return 1.0; // Fully shadowed
         } else {
            return 0.0; // Fully lit
         }
    }

    for (int i = 0; i < PCF_NUM_SAMPLES; ++i) {
        vec2 offset = poissonDisk[i % 16]; // Reuse samples
        vec2 sampleUV = uv + offset * filterRadiusUV;

        // Ensure sampling within bounds [0,1] might be needed depending on texture wrap mode
        sampleUV = clamp(sampleUV, 0.0, 1.0); // Optional: Add if necessary

        float shadowMapDepth = texture(shadowMapSampler, vec3(sampleUV, layer)).r;

        // Compare depth with bias
        if (shadowMapDepth < 1.0 && currentDepth > shadowMapDepth + bias) {
            shadow += 1.0; // This sample is occluded
        }
    }

    return shadow / float(PCF_NUM_SAMPLES);
}


void main() {
    vec2 uv = gl_FragCoord.xy / iResolution;

    vec3 fragPos = texture(position, uv).xyz;
    vec3 N = normalize(texture(normalMapSampler, uv).xyz);
    vec3 albedo = texture(diffuseColorSampler, uv).rgb;
    vec4 metalnessRoughness = texture(metallicRoughnessSampler, uv);
    float roughness = metalnessRoughness.g;

    vec3 totalLighting = vec3(0.0);

    int num_lights_to_process = min(light_count, lights.length());
    // int num_lights_to_process = light.length();
    for(int i = 0; i < num_lights_to_process; i++) {

        vec3 lightPos = lights[i].position;
        vec3 lightColor = lights[i].color;
        float lightRadius = lights[i].radius;

        vec3 lightDir = normalize(lightPos - fragPos);
        vec3 viewDir = normalize(camPos - fragPos);
        vec3 halfwayDir = normalize(lightDir + viewDir);

        float distanceToLight = length(lightPos - fragPos);
        float attenuation = 1.0;
        if (lightRadius > 0.0) {
             attenuation = pow(max(0.0, 1.0 - distanceToLight / lightRadius), 2.0);
        }
        // Optimization: If attenuation is zero, skip lighting/shadow calc for this light
        if (attenuation <= 0.0 || (distanceToLight > lightRadius && lightRadius > 0.0)) {
             continue; // Skip to next light
        }

        float diffFactor = max(dot(N, lightDir), 0.0);
        vec3 diffuse = lightColor * albedo * diffFactor;

        float shininess = mix(256.0, 2.0, roughness * roughness);
        float specFactor = pow(max(dot(N, halfwayDir), 0.0), shininess);
        vec3 specular = lightColor * specFactor;

        // --- PCSS Shadow Mapping ---
        float shadow = 0.0; // Default to lit

        vec4 fragPosLightSpace = lights[i].light_projection * lights[i].light_view * vec4(fragPos, 1.0);

        // **FIXED**: Check W before perspective divide
        if (fragPosLightSpace.w > 0.0)
        {
            // 2. Perform perspective divide to get NDC [-1, 1]
            vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

            // 3. Check if fragment is within the light's frustum NDC cube [-1, 1]
            //    Also check if it's in front of the near plane (projCoords.z <= 1.0 maps to depth <= 1.0)
            if (abs(projCoords.x) <= 1.0 && abs(projCoords.y) <= 1.0 && projCoords.z <= 1.0)
            {
                // 4. Convert NDC to shadow map UV [0, 1] and depth [0, 1]
                vec2 shadowMapUV = projCoords.xy * 0.5 + 0.5;
                float currentDepth = projCoords.z * 0.5 + 0.5; // Receiver depth [0, 1]

                // currentDepth should technically be <= 1.0 because projCoords.z <= 1.0
                // However, floating point inaccuracy might push it slightly > 1.0
                if (currentDepth <= 1.0001) { // Allow for slight FP error

                    // 5. Calculate shadow bias (normal-based)
                    // Might need different bias strategy for PCSS, but start with this.
                    // Reduce bias maybe, as PCF smooths edges? Test this.
                    float bias = max(0.001 * (1.0 - dot(N, lightDir)), 0.0001); // Potentially smaller bias needed?

                    vec2 shadowMapTexelSize = 1.0 / textureSize(shadow_maps, 0).xy;
                    int shadowLayer = lights[i].shadow_map_id;

                    // Define search region radius (heuristic, needs tuning)
                    float baseLightSize = 0.5; // Example base size for scaling
                    float blockerSearchRadiusTexels = 4.0 * (lightWorldSize / max(baseLightSize, 0.01)); // Scale radius, avoid division by zero
                    blockerSearchRadiusTexels = clamp(blockerSearchRadiusTexels, 2.0, 16.0); // Clamp radius to reasonable min/max texels
                    vec2 blockerSearchRadiusUV = blockerSearchRadiusTexels * shadowMapTexelSize;

                    // --- PCSS Steps ---
                    // Step 1: Find Average Blocker Depth
                    float avgBlockerDepth = findBlockerDepth(shadow_maps, shadowLayer, shadowMapUV, currentDepth, blockerSearchRadiusUV);

                    // Step 2: Calculate Penumbra Size (returns UV radius for PCF)
                    float penumbraRadiusUV = calculatePenumbraSize(currentDepth, avgBlockerDepth, lightWorldSize, shadowMapTexelSize);

                    // Optional: Clamp max penumbra radius to avoid excessive blur/cost
                    // float maxPenumbraTexels = 32.0;
                    // penumbraRadiusUV = min(penumbraRadiusUV, maxPenumbraTexels * shadowMapTexelSize.x);

                    // Step 3: Perform PCF Filtering
                    shadow = pcfFilter(shadow_maps, shadowLayer, shadowMapUV, currentDepth, bias, vec2(penumbraRadiusUV)); // Pass vec2 for radius

                }
                // else: fragment is beyond far plane (currentDepth > 1.0), treat as lit (shadow = 0.0)
            }
            // else: fragment outside X/Y bounds of frustum, treat as lit (shadow = 0.0)
        }
        // else: fragment behind light's view origin, treat as lit (shadow = 0.0)


        // --- Combine Lighting and Shadow ---
        totalLighting += attenuation * (diffuse + specular) * (1.0 - shadow); // Apply soft shadow
    }

    // --- Final Color Output ---
    vec3 ambientColor = vec3(0.5); // Add a small ambient term
    totalLighting += ambientColor * albedo;
    FragColor = vec4(totalLighting, 1.0);
}