// Left empty. This is optional. For implemetation, you can find many references from nearby shaders. You might need random number generators (RNG) to distribute points in a (Hemi)sphere. You can ask AI for both of them (RNG and sampling in a sphere) or try to find some resources online. Later I will add some links to the document about this.
#include <vector>
#include <random> // For noise/kernel generation
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../light.h" // Assuming CameraData might be here or in another common header
#include "nodes/core/def/node_def.hpp"
#include "pxr/imaging/hd/tokens.h"
#include "render_node_base.h"
#include "rich_type_buffer.hpp"
#include "utils/draw_fullscreen.h"
#include "nodes/core/logic/node_exec_context.hpp" // For accessing shared data like camera

NODE_DEF_OPEN_SCOPE

// --- SSAO Helper Data & Functions ---

// Helper function to generate SSAO kernel samples (hemisphere)
// Should be called only once ideally.
static std::vector<glm::vec3> ssaoKernel;
static bool kernelGenerated = false;
const int KERNEL_SIZE = 64; // Or make this an input parameter

void generateSSAOKernel() {
    if (kernelGenerated) return;

    std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between 0.0 - 1.0
    std::default_random_engine generator;
    ssaoKernel.reserve(KERNEL_SIZE);
    for (unsigned int i = 0; i < KERNEL_SIZE; ++i) {
        glm::vec3 sample(
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) // Only sample in positive Z hemisphere
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(generator); // Distribute within hemisphere

        // Scale samples so they're more aligned to the center of the kernel
        float scale = float(i) / float(KERNEL_SIZE);
        scale = glm::lerp(0.1f, 1.0f, scale * scale); // Use squared lerp for non-linear distribution
        sample *= scale;
        ssaoKernel.push_back(sample);
    }
    kernelGenerated = true;
}

// Helper function to generate noise texture
// Should be called only once ideally.
static GLuint noiseTextureID = 0;
static bool noiseTextureGenerated = false;
const int NOISE_TEX_SIZE = 4; // 4x4 noise texture

void generateNoiseTexture() {
    if (noiseTextureGenerated) return;

    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoNoise;
    ssaoNoise.reserve(NOISE_TEX_SIZE * NOISE_TEX_SIZE);
    for (unsigned int i = 0; i < NOISE_TEX_SIZE * NOISE_TEX_SIZE; i++) {
        glm::vec3 noise(
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            0.0f // Rotate around Z axis in tangent space (or view space if normals are view space)
        );
        ssaoNoise.push_back(glm::normalize(noise)); // Use normalized vectors
    }

    glGenTextures(1, &noiseTextureID);
    glBindTexture(GL_TEXTURE_2D, noiseTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, NOISE_TEX_SIZE, NOISE_TEX_SIZE, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // Tiling
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind

    noiseTextureGenerated = true;
}

// --- Node Definition ---

NODE_DECLARATION_FUNCTION(ssao)
{
    // Inputs from G-Buffer / previous passes
    // b.add_input<TextureHandle>("Color"); // Original color often not needed directly by SSAO pass
    b.add_input<TextureHandle>("Position"); // View-space Position (X, Y, linear_Z) is preferred
    b.add_input<TextureHandle>("Normal");   // View-space Normal is needed

    // SSAO Parameters (can be uniforms or node inputs)
    b.add_input<float>("Radius").default_val(0.5f);
    b.add_input<float>("Bias").default_val(0.025f);
    // b.add_input<int>("KernelSize").default_val(64); // Use const for now

    b.add_input<std::string>("Shader").default_val("shaders/ssao.fs");
    b.add_output<TextureHandle>("Occlusion"); // Outputting the AO factor (usually 0.0-1.0)
}

NODE_EXECUTION_FUNCTION(ssao)
{
    // --- 1. Get Inputs & Context ---
    // Texture Inputs
    auto position_texture = params.get_input<TextureHandle>("Position");
    auto normal_texture = params.get_input<TextureHandle>("Normal");

    // Check if inputs are valid
    if (!position_texture || !normal_texture) {
        spdlog::error("SSAO Node: Position or Normal texture input is missing.");
        return false;
    }

    // SSAO Parameters
    auto ssao_radius = params.get_input<float>("Radius");
    auto ssao_bias = params.get_input<float>("Bias");
    // int kernel_size = params.get_input<int>("KernelSize"); // Using const KERNEL_SIZE

    // Get Camera Data (Projection matrix is needed)
    // Assuming camera data is shared via context
    auto camera_data_ptr = node_context.get_shared_data<CameraData>("camera_data");
    if (!camera_data_ptr) {
        spdlog::error("SSAO Node: CameraData not found in context.");
        return false;
    }
    const CameraData& camera_data = *camera_data_ptr;
    glm::mat4 projection_matrix = camera_data.projection; // Get projection matrix

    // Determine Size
    auto size = position_texture->desc.size; // Get size from one of the input textures

    // --- 2. Generate Kernel & Noise Texture (Once) ---
    generateSSAOKernel();
    generateNoiseTexture();

    // --- 3. Create Output Texture ---
    TextureDesc ao_texture_desc;
    ao_texture_desc.size = size;
    // Output a single channel float texture for the occlusion factor
    ao_texture_desc.format = HdFormatFloat32;
    auto ao_texture = resource_allocator.create(ao_texture_desc);

    // --- 4. Setup Framebuffer ---
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        ao_texture->texture_id,
        0);

    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        spdlog::error("SSAO Node: Framebuffer is not complete!");
        glDeleteFramebuffers(1, &framebuffer);
        resource_allocator.destroy(ao_texture); // Clean up allocated texture
        return false;
    }

    // --- 5. Setup Shader ---
    auto shaderPath = params.get_input<std::string>("Shader");
    ShaderDesc shader_desc;
    shader_desc.set_vertex_path(
        std::filesystem::path(RENDER_NODES_FILES_DIR) /
        std::filesystem::path("shaders/fullscreen.vs"));
    shader_desc.set_fragment_path(
        std::filesystem::path(RENDER_NODES_FILES_DIR) /
        std::filesystem::path(shaderPath));

    auto shader_ptr = resource_allocator.create(shader_desc);
    if (!shader_ptr || !shader_ptr->shader.ID) {
        spdlog::error("SSAO Node: Failed to load shader: {}", shaderPath);
        glDeleteFramebuffers(1, &framebuffer);
        resource_allocator.destroy(ao_texture);
        // Don't destroy shader_ptr if creation failed, it might be invalid
        return false;
    }
    auto& shader = shader_ptr->shader; // Use reference for convenience
    shader.use();

    // --- 6. Set Uniforms & Bind Textures ---
    shader.setVec2("iResolution", size);

    // Bind G-Buffer textures
    shader.setTexture("gPosition", position_texture->texture_id, 0); // Texture unit 0
    shader.setTexture("gNormal", normal_texture->texture_id, 1);    // Texture unit 1

    // Bind Noise Texture
    shader.setTexture("texNoise", noiseTextureID, 2);                // Texture unit 2

    // Upload Kernel Samples (assuming shader expects vec3 array named "samples")
    shader.setVec3Array("samples", ssaoKernel);
    // shader.setInt("kernelSize", KERNEL_SIZE); // Pass kernel size if needed by shader loop

    // Pass Projection Matrix & Parameters
    shader.setMat4("projection", projection_matrix);
    shader.setFloat("radius", ssao_radius);
    shader.setFloat("bias", ssao_bias);
    // Pass noise texture scale if needed (depends on shader implementation)
    shader.setVec2("noiseScale", glm::vec2((float)size.x / NOISE_TEX_SIZE, (float)size.y / NOISE_TEX_SIZE));


    // --- 7. Draw Fullscreen Quad ---
    unsigned int VBO, VAO;
    CreateFullScreenVAO(VAO, VBO);

    // Clear the AO texture to 1.0 (fully unoccluded) before drawing
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // No depth test needed for fullscreen pass
    glDisable(GL_DEPTH_TEST);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // Re-enable depth test if it was enabled before
    // glEnable(GL_DEPTH_TEST); // Or manage state externally

    // --- 8. Cleanup ---
    DestroyFullScreenVAO(VAO, VBO);
    resource_allocator.destroy(shader_ptr); // Destroy shader resource

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind FBO
    glDeleteFramebuffers(1, &framebuffer); // Delete FBO

    // --- 9. Set Output ---
    params.set_output("Occlusion", ao_texture); // Set the correct output name
    return true;
}

// Make sure UI declaration matches the function name
NODE_DECLARATION_UI(ssao);
NODE_DEF_CLOSE_SCOPE