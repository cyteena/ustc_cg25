#version 430 core

// Define a uniform struct for lights
struct Light {
    mat4 light_projection; // Projection matrix from light's perspective
    mat4 light_view;       // View matrix from light's perspective
    vec3 position;         // World space position of the light
    float radius;          // Radius for attenuation calculation
    vec3 color;            // Light color (used for both diffuse and specular)
    int shadow_map_id;     // Index for the shadow map texture array layer
    float lightWorldSize;
};

// Buffer containing light data
layout(binding = 0) buffer lightsBuffer {
    Light lights[]; // Use unsized array for flexibility if needed, but sized is safer if max is known
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
uniform int light_count; // Ensure this uniform is correctly set in C++

// Output color
layout(location = 0) out vec4 FragColor; // Renamed from 'Color' to 'FragColor' (common practice)

void main() {
    // Calculate UV coordinates for sampling G-Buffer textures
    vec2 uv = gl_FragCoord.xy / iResolution;

    // --- Sample G-Buffer ---
    vec3 fragPos = texture(position, uv).xyz;          // Fragment's world position
    vec3 N = normalize(texture(normalMapSampler, uv).xyz); // Fragment's world normal (ensure it's normalized)
    vec3 albedo = texture(diffuseColorSampler, uv).rgb; // Fragment's base color
    vec4 metalnessRoughness = texture(metallicRoughnessSampler, uv);
    // float metal = metalnessRoughness.r; // Metalness (Not directly used in basic Blinn-Phong)
    float roughness = metalnessRoughness.g; // Roughness (Used to calculate shininess)

    // Initialize final color to black (ambient light could be added here or separately)
    vec3 totalLighting = vec3(0.0);
    int num_lights_to_process = light_count;
    for(int i = 0; i < num_lights_to_process; i++) {

        // --- Light Properties ---
        vec3 lightPos = lights[i].position;
        vec3 lightColor = lights[i].color;
        float lightRadius = lights[i].radius;

        // --- Blinn-Phong Calculations ---

        // 1. Calculate required vectors
        vec3 lightDir = normalize(lightPos - fragPos);   // Direction from fragment to light
        vec3 viewDir = normalize(camPos - fragPos);      // Direction from fragment to camera
        vec3 halfwayDir = normalize(lightDir + viewDir); // Halfway vector

        // 2. Calculate Attenuation based on distance and light radius
        float distanceToLight = length(lightPos - fragPos);
        float attenuation = 1.0; // Default: no attenuation
        if (lightRadius > 0.0) {
             // Quadratic falloff based on radius (clamped to [0, 1])
             // Alternative: Inverse square falloff (adjust constants as needed)
             const float constant = 1.0;
             const float linear = 0.05;
             const float quadratic = 0.05;
             attenuation = 1.0 / (constant + linear * distanceToLight + quadratic * (distanceToLight * distanceToLight));
            //  attenuation = pow(max(0.0, 1.0 - distanceToLight / lightRadius), 2.0);
        }


        // 3. Calculate Diffuse Term (Lambertian)
        float diffFactor = max(dot(N, lightDir), 0.0);
        vec3 diffuse = lightColor * albedo * diffFactor;

        // 4. Calculate Specular Term (Blinn-Phong)
        // Map roughness to shininess: low roughness -> high shininess (sharp), high roughness -> low shininess (blurry)
        // Adjust the range (e.g., 2 to 256) and mapping (e.g., linear, squared) based on desired look
        float shininess = mix(256.0, 32.0, roughness * roughness); // Example: squared roughness mapping
        float specFactor = pow(max(dot(N, halfwayDir), 0.0), shininess);
        // Specular color is typically the light color, possibly modulated by a surface property (Fs, Fresnel - not in basic Blinn-Phong)
        vec3 specular = lightColor * specFactor; // * vec3(1.0); // Optional specular intensity factor


        // --- Shadow Mapping ---

        float shadow = 0.0; // 0.0 = lit, 1.0 = shadowed

        // 1. Transform fragment position to light's clip space
        vec4 fragPosLightSpace = lights[i].light_projection * lights[i].light_view * vec4(fragPos, 1.0);

        // 2. Perform perspective divide to get Normalized Device Coordinates (NDC) [-1, 1]
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

        // 3. Check if fragment is within the light's frustum [-1, 1] NDC range
        // If outside, it cannot be lit or shadowed by this light's map, skip shadow check (treat as lit or handle differently)
        // We add a small epsilon to avoid precision issues at the exact borders
        if (abs(projCoords.x) <= 1.0 && abs(projCoords.y) <= 1.0 && projCoords.z <= 1.0)
        {
             // 4. Convert NDC to shadow map texture coordinates [0, 1]
            vec2 shadowMapUV = projCoords.xy * 0.5 + 0.5;
            // Convert NDC depth [-1, 1] to shadow map depth [0, 1]
            float currentDepth = projCoords.z * 0.5 + 0.5;

            // 5. Calculate shadow bias to prevent shadow acne
            // Bias increases as the surface is perpendicular to the light (dot(N, lightDir) approaches 0)
            // Clamp bias to a minimum value. Adjust factors (0.005, 0.0005) as needed.
            float bias = max(0.05 * (1.0 - dot(N, lightDir)), 0.005); // Increased bias values often needed

             // Ensure we don't sample outside the texture border (though check above helps)
             // Clamp UVs just in case, or rely on texture border settings (e.g., CLAMP_TO_BORDER with value 1.0)
            shadowMapUV = clamp(shadowMapUV, 0.0, 1.0);


             // Handle potential perspective aliasing near the far plane. If current depth is very close to 1.0,
             // it might be behind the far plane representation in the shadow map. Treat as lit maybe?
             // Or ensure shadow map clear depth is exactly 1.0.
             if (currentDepth > 1.0) { // If fragment is beyond the light's far plane (after mapping)
                 shadow = 0.0; // Treat as lit (cannot be shadowed by map content)
             } else {
                 // 6. Sample the shadow map
                 // Use the light's shadow_map_id as the layer index for the texture array
                 float shadowMapDepth = texture(shadow_maps, vec3(shadowMapUV, lights[i].shadow_map_id)).r;

                 // 7. Compare depths: if fragment is further than the depth in the map (plus bias), it's in shadow
                 if (shadowMapDepth < 1.0 && currentDepth > shadowMapDepth + bias) { // shadowMapDepth < 1.0 avoids comparing against cleared background
                    shadow = 0.5; // Fragment is in shadow
                 }
                 // else shadow remains 0.0 (lit)
             }
        }

        // --- Combine Lighting and Shadow ---
        // Apply attenuation and shadow multiplier
        // Only add light contribution if not fully shadowed (shadow = 0.0)
        totalLighting += attenuation * (diffuse + specular) * 1.0;
    }

    // --- Final Color Output ---
    // Add ambient term here if needed: totalLighting += ambientColor * albedo;
    vec3 ambientColor = vec3(0.5);
    totalLighting += ambientColor * albedo;
    FragColor = vec4(totalLighting, 1.0); // Set alpha to 1.0 for opaque
}