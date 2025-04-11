

#include "../geometries/mesh.h"
#include "../light.h"
#include "nodes/core/def/node_def.hpp"
#include "pxr/base/gf/frustum.h"
#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/hd/tokens.h"
#include "render_node_base.h"
#include "rich_type_buffer.hpp"
#include "utils/draw_fullscreen.h"
#include <vector> // Include vector for storing matrices

NODE_DEF_OPEN_SCOPE

// Structure to hold matrices for one light
struct LightMatrixInfo {
    GfMatrix4f viewMatrix;
    GfMatrix4f projectionMatrix;
    // We can add light index/ID if needed for matching, but assuming order is consistent for now.
};


NODE_DECLARATION_FUNCTION(shadow_mapping)
{
    b.add_input<int>("resolution").default_val(1024).min(256).max(4096);
    b.add_input<std::string>("Shader").default_val("shaders/shadow_mapping.fs");

    b.add_output<TextureHandle>("Shadow Maps");
    // Add output for the calculated light matrices
    b.add_output<std::vector<LightMatrixInfo>>("Light Matrices");
}

NODE_EXECUTION_FUNCTION(shadow_mapping)
{
    auto resolution = params.get_input<int>("resolution");

    // Vector to store the calculated matrices for output
    std::vector<LightMatrixInfo> light_matrices_output;
    light_matrices_output.reserve(lights.size()); // Pre-allocate space

    TextureDesc texture_desc;
    texture_desc.array_size = lights.size();
    // texture_desc.array_size = 1;
    texture_desc.size = GfVec2i(resolution);
    texture_desc.format = HdFormatUNorm8Vec4;
    auto shadow_map_texture = resource_allocator.create(texture_desc);

    auto shaderPath = params.get_input<std::string>("Shader");

    ShaderDesc shader_desc;
    shader_desc.set_vertex_path(
        std::filesystem::path(RENDER_NODES_FILES_DIR) /
        std::filesystem::path("shaders/shadow_mapping.vs"));

    shader_desc.set_fragment_path(
        std::filesystem::path(RENDER_NODES_FILES_DIR) /
        std::filesystem::path(shaderPath));
    auto shader_handle = resource_allocator.create(shader_desc);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    std::vector<TextureHandle> depth_textures;
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glViewport(0, 0, resolution, resolution);
    int NN = lights.size();
    auto tmp = global_payload;
    for (int light_id = 0; light_id < NN; ++light_id)
    {
        shader_handle->shader.use();
        if (!lights[light_id]->GetId().IsEmpty())
        {
            GlfSimpleLight light_params =
                lights[light_id]->Get(HdTokens->params).Get<GlfSimpleLight>();

            // HW6: The matrices for lights information is here! Current value
            // is set that "it just works". However, you should try to modify
            // the values to see how it affects the performance of the shadow
            // maps.

            GfMatrix4f light_view_mat;
            GfMatrix4f light_projection_mat;

            bool has_light = false;
            if (lights[light_id]->GetLightType() ==
                HdPrimTypeTokens->sphereLight)
            {

                GfFrustum frustum;
                GfVec3f light_position = {light_params.GetPosition()[0],
                                          light_params.GetPosition()[1],
                                          light_params.GetPosition()[2]};

                light_view_mat = GfMatrix4f().SetLookAt(
                    light_position, GfVec3f(0, 0, 0), GfVec3f(0, 0, 1));
                frustum.SetPerspective(120.f, 1.0, 1, 25.f);
                light_projection_mat =
                    GfMatrix4f(frustum.ComputeProjectionMatrix());

                has_light = true;
            }
            else if (lights[light_id]->GetLightType() ==
            HdPrimTypeTokens->rectLight) {
                // --- HW6: Rect Light Shadow Mapping Setup ---
                // Treat rect light as a spotlight originating from its center
                // for the purpose of generating the shadow map.
                GfVec3f position = { light_params.GetPosition()[0],
                                    light_params.GetPosition()[1],
                                    light_params.GetPosition()[2] };

                // Direction the light is pointing
                GfVec3f direction = { light_params.GetSpotDirection()[0],
                                    light_params.GetSpotDirection()[1],
                                    light_params.GetSpotDirection()[2] };
                // It's crucial that SpotDirection is correctly set for the rect light in Hydra/USD.
                // If it's zero or invalid, the LookAt matrix will be ill-defined.

                GfVec3f lookAtTarget = position + direction;

                // Define the 'up' vector for the LookAt matrix.
                // Common choice is world up (e.g., Z-up or Y-up).
                // Need a robust way to handle cases where 'direction' is parallel to 'up'.
                GfVec3f worldUp(0.0f, 0.0f, 1.0f); // Assuming Z-up for USD default
                if (GfAbs(GfDot(direction, worldUp)) > 0.999f) {
                    // If direction is too close to world Z-up/down, use world Y-up instead
                    worldUp = GfVec3f(0.0f, 1.0f, 0.0f);
                }
                // If direction is also parallel to Y-up (unlikely unless pointing straight Y),
                // you might need yet another fallback (e.g., X-up).

                light_view_mat.SetLookAt(position, lookAtTarget, worldUp);

                // --- Projection Matrix Setup ---
                // Use a perspective projection resembling a spotlight.
                GfFrustum frustum;

                // FOV: Define how wide the light "sees". 90 degrees is a common starting point.
                // You could potentially derive this from width/height/distance for a tighter fit,
                // but a fixed reasonable FOV is often sufficient for the shadow map pass.
                float fov = 90.0f; // Degrees. **Tune this value!**

                // Near/Far Planes: Define the depth range for the shadow map.
                // **These are critical for shadow map precision and MUST be tuned!**
                // Start with values similar to the sphere light, but adjust based on your scene.
                float near_plane = 0.5f;  // Closer near plane might be needed for local lights
                float far_plane = 50.0f; // Might need to be larger depending on light range

                frustum.SetPerspective(fov, 1.0, near_plane, far_plane);
                light_projection_mat = GfMatrix4f(frustum.ComputeProjectionMatrix());

                has_light = true;
                // --- End HW6 ---
            }
            if (!has_light)
            {
                // Add placeholder matrices if light is skipped to maintain order
                light_matrices_output.push_back({GfMatrix4f(1.0), GfMatrix4f(1.0)});
                continue;
            }

            // Store the calculated matrices for this light
            light_matrices_output.push_back({light_view_mat, light_projection_mat});

            // Set matrices for the shadow rendering shader
            shader_handle->shader.setMat4("light_view", light_view_mat);
            shader_handle->shader.setMat4("light_projection", light_projection_mat); // No need to cast GfMatrix4f again

            glFramebufferTextureLayer(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                shadow_map_texture->texture_id,
                0,
                light_id);

            texture_desc.format = HdFormatFloat32UInt8;
            texture_desc.array_size = 1;
            auto depth_texture_for_opengl =
                resource_allocator.create(texture_desc);
            depth_textures.push_back(depth_texture_for_opengl);

            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_DEPTH_STENCIL_ATTACHMENT,
                GL_TEXTURE_2D,
                depth_texture_for_opengl->texture_id,
                0);

            glClearColor(0.f, 0.f, 0.f, 1.0f);
            glClear(
                GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT);

            for (int mesh_id = 0; mesh_id < meshes.size(); ++mesh_id)
            {
                auto mesh = meshes[mesh_id];

                shader_handle->shader.setMat4("model", mesh->transform);

                mesh->RefreshGLBuffer();

                glBindVertexArray(mesh->VAO);
                glDrawElements(
                    GL_TRIANGLES,
                    static_cast<unsigned int>(
                        mesh->triangulatedIndices.size() * 3),
                    GL_UNSIGNED_INT,
                    0);
                glBindVertexArray(0);
            }
        } else {
             // Add placeholder matrices for lights with empty IDs
             light_matrices_output.push_back({GfMatrix4f(1.0), GfMatrix4f(1.0)});
        }
    }

    for (auto &&depth_texture : depth_textures)
    {
        resource_allocator.destroy(depth_texture);
    }

    resource_allocator.destroy(shader_handle);
    glDeleteFramebuffers(1, &framebuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Set the output containing the collected matrices
    params.set_output("Light Matrices", light_matrices_output);

    auto shader_error = shader_handle->shader.get_error();

    params.set_output("Shadow Maps", shadow_map_texture);
    if (!shader_error.empty())
    {
        throw std::runtime_error(shader_error);
    }
}

NODE_DECLARATION_UI(shadow_mapping);
NODE_DEF_CLOSE_SCOPE
