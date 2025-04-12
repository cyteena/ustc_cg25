#version 450 core

// Output occlusion factor (0.0 = occluded, 1.0 = not occluded)
layout (location = 0) out float FragColor; // Outputting a single float

// Input texture coordinates from fullscreen quad
in vec2 TexCoords;

// G-Buffer Textures (View Space)
uniform sampler2D gPosition; // View-space position (X, Y, linear_Z or depth)
uniform sampler2D gNormal;   // View-space normal

// Noise Texture for rotation
uniform sampler2D texNoise;

// SSAO Kernel Samples (View Space)
// Make sure KERNEL_SIZE in C++ matches this array size
const int KERNEL_SIZE = 64;
uniform vec3 samples[KERNEL_SIZE];

// SSAO Parameters
uniform float radius = 0.5;
uniform float bias = 0.025;

// Projection Matrix (to reconstruct view-space position from depth if needed,
// and to transform samples to screen space)
uniform mat4 projection;

// Screen dimensions needed for noise texture tiling
uniform vec2 noiseScale; // = vec2(screenWidth/noiseTexWidth, screenHeight/noiseTexWidth)

// --- SSAO Calculation ---
void main()
{
    // Get G-Buffer data for current fragment
    // IMPORTANT: Ensure gPosition stores view-space position, not depth or world pos.
    // If it stores depth, you need to reconstruct view-space position first.
    vec3 fragPos   = texture(gPosition, TexCoords).xyz;
    vec3 normal    = normalize(texture(gNormal, TexCoords).rgb);

    // Get random vector from noise texture to rotate kernel samples
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);

    // Create TBN matrix (Tangent, Bitangent, Normal) using the normal and random vector
    // Gram-Schmidt process to ensure orthogonality
    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    // Calculate occlusion
    float occlusion = 0.0;
    for(int i = 0; i < KERNEL_SIZE; ++i)
    {
        // Get sample position in view space
        vec3 samplePosView = TBN * samples[i]; // Transform sample from tangent to view space
        samplePosView = fragPos + samplePosView * radius; // Offset by radius

        // Project sample position to screen space [0, 1]
        vec4 offset = vec4(samplePosView, 1.0);
        offset      = projection * offset;    // To clip space
        offset.xyz /= offset.w;               // Perspective divide
        offset.xyz  = offset.xyz * 0.5 + 0.5; // Transform to [0,1] range (texture coords)

        // Check if sample is within screen bounds
        if (offset.x >= 0.0 && offset.x <= 1.0 && offset.y >= 0.0 && offset.y <= 1.0)
        {
            // Get depth/position of sample point from G-Buffer
            // IMPORTANT: Ensure gPosition stores comparable view-space Z.
            // Assuming gPosition.z is linear view-space Z (often negative).
            // If gPosition stores non-linear depth [0,1], comparison needs adjustment.
            float sampleStoredZ = texture(gPosition, offset.xy).z;

            // Calculate occlusion contribution
            // Check if sample point is "behind" the stored surface point in view space
            // (samplePosView.z is further away from camera than sampleStoredZ)
            // Add bias to avoid self-occlusion artifacts.
            if (samplePosView.z <= sampleStoredZ + bias) // Check if sample is in front or very close
            {
                 // Consider sample depth relative to fragment depth
                 // Use smoothstep for softer occlusion near the radius edge
                 float depthDifference = abs(sampleStoredZ - fragPos.z);
                 if (depthDifference < radius) // Only consider samples within radius in depth
                 {
                    // Check if the sample point is actually occluding
                    // samplePosView.z is the view-space Z of the sample point
                    // sampleStoredZ is the view-space Z of the geometry at the sample's screen position
                    // If samplePosView.z is further away (more negative or larger positive depending on convention)
                    // than sampleStoredZ, it means the sample point is behind something.
                    // We want to count how many samples are *in front* of the geometry.
                    // Let's redefine: occlusion means how many samples *fail* the depth test.
                    // If samplePosView is closer to the camera than the geometry at its projected position,
                    // it means the space is empty. We want to count samples that are *behind* geometry.

                    // Revised check: Is the sample point further away (occluded by) the geometry at its projected location?
                    // We add bias to samplePosView.z to push it slightly away from the actual surface.
                    if (samplePosView.z >= sampleStoredZ + bias) // Check if sample is behind the stored surface + bias
                    {
                         occlusion += 1.0;
                    }
                    // Alternative: Check if sample point is in front of current fragment
                    // float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - samplePosView.z));
                    // occlusion += (sampleStoredZ >= samplePosView.z + bias ? 1.0 : 0.0) * rangeCheck;
                 }
            }
        }
    }
    // Average occlusion factor and invert
    // Clamp to prevent potential over-occlusion if bias/radius are large
    occlusion = clamp(1.0 - (occlusion / float(KERNEL_SIZE)), 0.0, 1.0);

    FragColor = occlusion;
}
