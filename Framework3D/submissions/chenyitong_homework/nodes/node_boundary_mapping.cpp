#include "GCore/Components/MeshOperand.h"
#include "geom_node_base.h"
#include "GCore/util_openmesh_bind.h"
#include <Eigen/Sparse>
#include <cmath>
#include <Logger/Logger.h>

/*
** @brief HW4_TutteParameterization
**
** This file contains two nodes whose primary function is to map the boundary of
*a mesh to a plain convex closed curve (circle of square), setting the stage for subsequent
*   Laplacian equation
** solution and mesh parameterization tasks.
**
** Key to this node's implementation is the adapt manipulation of half-edge data
*structures
** to identify and modify the boundary of the mesh.
**
** Task Overview:
** - The two execution functions (node_map_boundary_to_square_exec,
** node_map_boundary_to_circle_exec) require an update to accurately map the
*mesh boundary to a and
** circles. This entails identifying the boundary edges, evenly distributing
*boundary vertices along
** the square's perimeter, and ensuring the internal vertices' positions remain
*unchanged.
** - A focus on half-edge data structures to efficiently traverse and modify
*mesh boundaries.
*/

NODE_DEF_OPEN_SCOPE

/*
** HW4_TODO: Node to map the mesh boundary to a circle.
*/

NODE_DECLARATION_FUNCTION(circle_boundary_mapping)
{
    // Input-1: Original 3D mesh with boundary
    b.add_input<Geometry>("Input");
    // Output-1: Processed 3D mesh whose boundary is mapped to a square and the
    // interior vertices remains the same
    b.add_output<Geometry>("Output");
}

NODE_EXECUTION_FUNCTION(circle_boundary_mapping)
{
    // Get the input from params
    auto input = params.get_input<Geometry>("Input");

    // (TO BE UPDATED) Avoid processing the node when there is no input
    if (!input.get_component<MeshComponent>())
    {
        log::error("Boundary Mapping: Need Geometry Input.");
        throw std::runtime_error("Boundary Mapping: Need Geometry Input.");
    }

    /* ----------------------------- Preprocess -------------------------------
    ** Create a halfedge structure (using OpenMesh) for the input mesh. The
    ** half-edge data structure is a widely used data structure in geometric
    ** processing, offering convenient operations for traversing and modifying
    ** mesh elements.
    */
    auto halfedge_mesh = operand_to_openmesh(&input);
    log::info("Halfedge mesh created.");

    /* ----------- [HW4_TODO] TASK 2.1: Boundary Mapping (to circle)
     *------------
     ** In this task, you are required to map the boundary of the mesh to a
     *circle
     ** shape while ensuring the internal vertices remain unaffected. This step
     *is
     ** crucial for setting up the mesh for subsequent parameterization tasks.
     **
     ** Algorithm Pseudocode for Boundary Mapping to Circle
     ** ------------------------------------------------------------------------
     ** 1. Identify the boundary loop(s) of the mesh using the half-edge
     *structure.
     **
     ** 2. Calculate the total length of the boundary loop to determine the
     *spacing
     **    between vertices when mapped to a square.
     **
     ** 3. Sequentially assign each boundary vertex a new position along the
     *square's
     **    perimeter, maintaining the calculated spacing to ensure proper
     *distribution.
     **
     ** 4. Keep the interior vertices' positions unchanged during this process.
     **
     ** Note: How to distribute the points on the circle?
     **
     ** Note: It would be better to normalize the boundary to a unit circle in
     *[0,1]x[0,1] for
     ** texture mapping.
     */
    std::vector<OpenMesh::VertexHandle> boundary_vertices;
    OpenMesh::HalfedgeHandle start_he;

    // 1. 找到一个在边界的half-edge作为循环起点
    for (auto he_it = halfedge_mesh->halfedges_begin(); he_it != halfedge_mesh->halfedges_end(); ++he_it)
    {
        if (halfedge_mesh->is_boundary(*he_it))
        {
            start_he = *he_it;
            break;
        }
    }

    if (!halfedge_mesh->is_boundary(start_he))
    {
        log::error("No boundary found in the mesh.");
        throw std::runtime_error("No boundary found in the mesh.");
    }

    OpenMesh::HalfedgeHandle current_he = start_he;
    do
    {
        OpenMesh::VertexHandle vh = halfedge_mesh->from_vertex_handle(current_he);
        boundary_vertices.emplace_back(vh);
        current_he = halfedge_mesh->next_halfedge_handle(current_he);
    } while (current_he != start_he);

    int num_boundary_vertices = boundary_vertices.size();
    log::info("Number of boundary vertices: %d", num_boundary_vertices);

    for (int i = 0; i < num_boundary_vertices; ++i)
    {
        double angle = 2.0 * M_PI * static_cast<double>(i) / num_boundary_vertices;
        double x = 0.5 * cos(angle);
        double y = 0.5 * sin(angle);
        halfedge_mesh->point(boundary_vertices[i]) = OpenMesh::Vec3d(x, y, 0.0);
    }
    log::info("Boundary vertices mapped to circle.");

    /* ----------------------------- Postprocess ------------------------------
    ** Convert the result mesh from the halfedge structure back to Geometry
    *format as the node's
    ** output.
    */
    auto geometry = openmesh_to_operand(halfedge_mesh.get());
    log::info("Mesh converted back to Geometry format.");

    // Set the output of the nodes
    params.set_output("Output", std::move(*geometry));
    log::info("Output set.");
    return true;
}

/*
** HW4_TODO: Node to map the mesh boundary to a square.
*/

NODE_DECLARATION_FUNCTION(square_boundary_mapping)
{
    // Input-1: Original 3D mesh with boundary
    b.add_input<Geometry>("Input");

    // Output-1: Processed 3D mesh whose boundary is mapped to a square and the
    // interior vertices remains the same
    b.add_output<Geometry>("Output");
}

NODE_EXECUTION_FUNCTION(square_boundary_mapping)
{
    // Get the input from params
    auto input = params.get_input<Geometry>("Input");

    // Avoid processing the node when there is no input
    if (!input.get_component<MeshComponent>())
    {
        log::error("Input does not contain a mesh");
        throw std::runtime_error("Input does not contain a mesh");
    }

    /* ----------------------------- Preprocess -------------------------------
    ** Create a halfedge structure (using OpenMesh) for the input mesh.
    */
    auto halfedge_mesh = operand_to_openmesh(&input);
    log::info("Halfedge mesh created.");

    /* ----------- TASK 2.2: Boundary Mapping (to square) ------------
     ** In this task, you are required to map the boundary of the mesh to a
     ** square shape while ensuring the internal vertices remain unaffected.
     */
    
    // 1. Find a boundary half-edge as starting point
    std::vector<OpenMesh::VertexHandle> boundary_vertices;
    OpenMesh::HalfedgeHandle start_he;

    for (auto he_it = halfedge_mesh->halfedges_begin(); he_it != halfedge_mesh->halfedges_end(); ++he_it)
    {
        if (halfedge_mesh->is_boundary(*he_it))
        {
            start_he = *he_it;
            break;
        }
    }

    if (!halfedge_mesh->is_boundary(start_he))
    {
        log::error("No boundary found in the mesh.");
        throw std::runtime_error("No boundary found in the mesh.");
    }

    // 2. Collect all boundary vertices in order
    OpenMesh::HalfedgeHandle current_he = start_he;
    do
    {
        OpenMesh::VertexHandle vh = halfedge_mesh->from_vertex_handle(current_he);
        boundary_vertices.emplace_back(vh);
        current_he = halfedge_mesh->next_halfedge_handle(current_he);
    } while (current_he != start_he);

    // 3. Map boundary vertices to a square
    int num_boundary_vertices = boundary_vertices.size();
    log::info("Number of boundary vertices: %d", num_boundary_vertices);
    
    // 计算周长并归一化到单位正方形
    double total_length = 0.0;
    std::vector<double> cumulative_lengths;
    int vertices_per_side = num_boundary_vertices / 4;
    int remainder = num_boundary_vertices % 4;
    
    // Adjust vertices per side to account for remainder
    std::vector<int> side_counts(4, vertices_per_side);
    for (int i = 0; i < remainder; ++i) {
        side_counts[i]++;
    }
    
    // Map vertices to square sides
    int vertex_index = 0;
    
    // 将顶点按周长比例分配到四个边
    constexpr double side_length = 1.0; // 单位正方形边长
    double accumulated = 0.0;
    
    // 修正顶点分配逻辑（删除所有跳过顶点的条件判断）
    // Bottom edge: (x, y) = (t, 0)
    for (int i = 0; i < side_counts[0]; ++i) {
        double t = static_cast<double>(i) / (side_counts[0] - 1);
        halfedge_mesh->point(boundary_vertices[vertex_index++]) = OpenMesh::Vec3d(t, 0.0, 0.0);
    }

    // Right edge: (x, y) = (1, t)
    for (int i = 0; i < side_counts[1]; ++i) {
        double t = static_cast<double>(i) / (side_counts[1] - 1);
        halfedge_mesh->point(boundary_vertices[vertex_index++]) = OpenMesh::Vec3d(1.0, t, 0.0);
    }

    // Top edge: (x, y) = (1-t, 1)
    for (int i = 0; i < side_counts[2]; ++i) {
        double t = static_cast<double>(i) / (side_counts[2] - 1);
        halfedge_mesh->point(boundary_vertices[vertex_index++]) = OpenMesh::Vec3d(1.0 - t, 1.0, 0.0);
    }

    // Left edge: (x, y) = (0, 1-t)
    for (int i = 0; i < side_counts[3]; ++i) {
        double t = static_cast<double>(i) / (side_counts[3] - 1);
        halfedge_mesh->point(boundary_vertices[vertex_index++]) = OpenMesh::Vec3d(0.0, 1.0 - t, 0.0);
    }
    log::info("Left edge mapped.");

    /* ----------------------------- Postprocess ------------------------------
    ** Convert the result mesh from the halfedge structure back to Geometry
    *format as the node's
    ** output.
    */
    auto geometry = openmesh_to_operand(halfedge_mesh.get());
    log::info("Mesh converted back to Geometry format.");

    // Set the output of the nodes
    params.set_output("Output", std::move(*geometry));
    log::info("Output set.");
    return true;
}

NODE_DECLARATION_UI(boundary_mapping);
NODE_DEF_CLOSE_SCOPE
