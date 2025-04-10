

#include "../geometries/mesh.h"
#include "../light.h"
#include "nodes/core/def/node_def.hpp"
#include "pxr/base/gf/frustum.h"
#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/hd/tokens.h"
#include "render_node_base.h"
#include "rich_type_buffer.hpp"
#include "utils/draw_fullscreen.h"
NODE_DEF_OPEN_SCOPE
NODE_DECLARATION_FUNCTION(shadow_mapping)
{
    b.add_input<int>("resolution").default_val(1024).min(256).max(4096);
    b.add_input<float>("ortho_size").default_val(20.0f).min(1.0f).max(1000.0f);
    b.add_input<float>("near_plane").default_val(1.0f).min(0.1f).max(2.0f);
    b.add_input<float>("far_plane").default_val(25.0f).min(1.0f).max(100.0f);
    b.add_input<float>("perspective_fov").default_val(120.0f).min(10.0f).max(160.0f);

    b.add_input<std::string>("Shader").default_val("shaders/shadow_mapping.fs");

    b.add_output<TextureHandle>("Shadow Maps");
}

NODE_EXECUTION_FUNCTION(shadow_mapping)
{
    auto resolution = params.get_input<int>("resolution");
    auto ortho_size = params.get_input<float>("ortho_size");
    auto near_plane = params.get_input<float>("near_plane");
    auto far_plane = params.get_input<float>("far_plane");
    auto perspective_fov = params.get_input<float>("perspective_fov");

    TextureDesc shadow_map_texture_desc;
    shadow_map_texture_desc.array_size = lights.size();
    // shadow_map_texture_desc.array_size = 1;
    shadow_map_texture_desc.size = GfVec2i(resolution);
    // shadow_map_texture_desc.format = HdFormatUNorm8Vec4;
    shadow_map_texture_desc.format = HdFormatFloat32UInt8;
    auto shadow_map_texture = resource_allocator.create(shadow_map_texture_desc);

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

    // std::vector<TextureHandle> depth_textures;
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

            GfMatrix4d light_world_transform_d = lights[light_id]->Get(HdTokens->transform).GetWithDefault<GfMatrix4d>();
            GfMatrix4f light_world_transform = GfMatrix4f(light_world_transform_d);

            GfMatrix4f light_view_mat;
            GfMatrix4f light_projection_mat;

            bool has_light = false;

            // // Output current light type for debugging
            // std::cout << "Light ID: " << light_id << ", Type: " 
            //           << lights[light_id]->GetLightType().GetText() << std::endl;
                      
            if (lights[light_id]->GetLightType() ==
                HdPrimTypeTokens->sphereLight)
            {

                GfFrustum frustum;
                GfVec3f light_position = {light_params.GetPosition()[0],
                                          light_params.GetPosition()[1],
                                          light_params.GetPosition()[2]};

                light_view_mat = GfMatrix4f().SetLookAt(
                    light_position, GfVec3f(0, 0, 0), GfVec3f(0, 0, 1));
                frustum.SetPerspective(perspective_fov, 1.0, near_plane, far_plane);
                light_projection_mat =
                    GfMatrix4f(frustum.ComputeProjectionMatrix());

                has_light = true;
            }
            else if (lights[light_id]->GetLightType() == HdPrimTypeTokens->distantLight)
            {
                // GfVec3f light_direction = GfVec3f(light_world_transform.GetRow3(2)).GetNormalized(); // 变换矩阵的z轴

                // 视图矩阵：我们需要一个“虚拟”位置来设置lookat
                // 位置应该沿着光照的反方向，距离场景足够远
                // LooAt 的目标点可以是场景中心
                // GfVec3f look_at_target(0, 0, 0); // 看向场景中心
                // GfVec3f virtual_pos = look_at_target - light_direction * (far_plane * 0.5f);
                // GfVec3f up_vector(0, 1, 0);
                // // if (fabs(light_direction.Dot(up_vector)) > 0.999f)
                // // {
                // //     up_vector = GfVec3f(0, 0, 1); // 如果平行，尝试 Z 轴向上
                // // }
                // light_view_mat = GfMatrix4f().SetLookAt(virtual_pos, look_at_target, up_vector);

                // // 投影矩阵
                // float half_size = ortho_size * 0.5f;
                // GfFrustum frustum;
                // frustum.SetOrthographic(
                //     -half_size, half_size, // left, right
                //     -half_size, half_size, // bottom, top
                //     near_plane, far_plane);
                // light_projection_mat = GfMatrix4f(frustum.ComputeProjectionMatrix());
                // has_light = true;
                GfFrustum frustum;
                GfVec3f light_position = {light_params.GetPosition()[0],
                                          light_params.GetPosition()[1],
                                          light_params.GetPosition()[2]};

                light_view_mat = GfMatrix4f().SetLookAt(
                    light_position, GfVec3f(0, 0, 0), GfVec3f(0, 0, 1));
                frustum.SetPerspective(perspective_fov, 1.0, near_plane, far_plane);
                light_projection_mat =
                    GfMatrix4f(frustum.ComputeProjectionMatrix());

                has_light = true;
            }
            else if (lights[light_id]->GetLightType() == HdPrimTypeTokens->rectLight || lights[light_id]->GetLightType() == HdPrimTypeTokens->diskLight)
            {
                // 区域光 -> 近似为聚光灯 -> 透视投影
                GfVec3f light_pos = light_world_transform.ExtractTranslation();
                GfVec3f light_direction = -GfVec3f(light_world_transform.GetRow3(2)).GetNormalized();
                GfVec3f look_at_target = light_pos + light_direction; // 看向光源前方

                GfVec3f up_vector = GfVec3f(light_world_transform.GetRow3(1)).GetNormalized();

                // // 检查 view_dir 和 up_vector 是否平行
                // if (fabs(light_direction.Dot(up_vector)) > 0.999f)
                // {
                //     // 如果平行（例如光照直上/下），选择另一个轴，如 X 轴
                //     up_vector = GfVec3f(light_world_transform.GetRow3(0)).GetNormalized();
                //     if (fabs(light_direction.Dot(up_vector)) > 0.999f)
                //     {
                //         // 如果还是平行（不太可能），用世界 Y 或 Z
                //         up_vector = GfVec3f(0, 1, 0); // 或者 (0, 0, 1)
                //     }
                // }
                light_view_mat = GfMatrix4f().SetLookAt(light_pos, look_at_target, up_vector);

                // 投影矩阵
                GfFrustum frustum;
                frustum.SetPerspective(
                    perspective_fov, // 使用输入的 FOV
                    1.0,             // Aspect ratio
                    near_plane,      // Near
                    far_plane);      // Far
                light_projection_mat = GfMatrix4f(frustum.ComputeProjectionMatrix());
                has_light = true;
            }

            if (!has_light)
            {
                continue;
            }

            shader_handle->shader.setMat4("light_view", light_view_mat);
            shader_handle->shader.setMat4(
                "light_projection", GfMatrix4f(light_projection_mat));

            // glFramebufferTextureLayer(
            //     GL_FRAMEBUFFER,
            //     GL_COLOR_ATTACHMENT0,
            //     shadow_map_texture->texture_id,
            //     0,
            //     light_id);
            glFramebufferTextureLayer(
                GL_FRAMEBUFFER,
                GL_DEPTH_ATTACHMENT, // *** 附加到深度附件 ***
                shadow_map_texture->texture_id, // *** 使用正确的纹理数组 ***
                0,         // Mipmap level
                light_id); // 数组层索引

            // shadow_map_texture_desc.format = HdFormatFloat32UInt8;
            // shadow_map_texture_desc.array_size = 1;
            // auto depth_texture_for_opengl =
            //     resource_allocator.create(shadow_map_texture_desc);
            // depth_textures.push_back(depth_texture_for_opengl);

            // glFramebufferTexture2D(
            //     GL_FRAMEBUFFER,
            //     GL_DEPTH_STENCIL_ATTACHMENT,
            //     GL_TEXTURE_2D,
            //     depth_texture_for_opengl->texture_id,
            //     0);
            glDrawBuffer(GL_NONE); // *** 添加: 不写入颜色缓冲 ***
            glReadBuffer(GL_NONE); // *** 添加: 不读取颜色缓冲 ***

            glClear(GL_DEPTH_BUFFER_BIT); // 只清除深度缓冲

            // glClearColor(0.f, 0.f, 0.f, 1.0f);
            // glClear(
            //     GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
            //     GL_STENCIL_BUFFER_BIT);

            // 检查帧缓冲状态 - 放在这里！
            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                glDeleteFramebuffers(1, &framebuffer);
                throw std::runtime_error("Framebuffer incomplete for light " + 
                                        std::to_string(light_id) + ": " + 
                                        std::to_string(status));
            }

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
        }
    }

    // for (auto &&depth_texture : depth_textures)
    // {
    //     resource_allocator.destroy(depth_texture);
    // }

    resource_allocator.destroy(shader_handle);
    glDeleteFramebuffers(1, &framebuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    auto shader_error = shader_handle->shader.get_error();

    params.set_output("Shadow Maps", shadow_map_texture);
    if (!shader_error.empty())
    {
        throw std::runtime_error(shader_error);
    }
}

NODE_DECLARATION_UI(shadow_mapping);
NODE_DEF_CLOSE_SCOPE
