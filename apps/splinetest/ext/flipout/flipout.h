#pragma once
#include <geometrycentral/surface/edge_length_geometry.h>
#include <geometrycentral/surface/flip_geodesics.h>
#include <geometrycentral/surface/manifold_surface_mesh.h>
#include <geometrycentral/surface/mesh_graph_algorithms.h>
#include <geometrycentral/surface/meshio.h>
#include <geometrycentral/surface/polygon_soup_mesh.h>
#include <geometrycentral/surface/vertex_position_geometry.h>
#include <geometrycentral/utilities/timing.h>
#include <yocto/yocto_math.h>

using namespace yocto;
using namespace geometrycentral;
using namespace geometrycentral::surface;

namespace flipout {

struct Flipout_Mesh {
  std::unique_ptr<ManifoldSurfaceMesh>    topology;
  std::unique_ptr<VertexPositionGeometry> geometry;
};

struct flipout_path_stats {
  double initial_guess = 0;
  double shortening    = 0;
};

Flipout_Mesh make_flipout_mesh(
    const std::vector<vec3i>& triangles, const std::vector<vec3f>& positions);

Flipout_Mesh load_flipout_mesh(const std::string& filename);

std::pair<std::unique_ptr<FlipEdgeNetwork>, flipout_path_stats>
create_path_from_points(ManifoldSurfaceMesh* mesh,
    VertexPositionGeometry* geometry, int vertex_start, int vertex_end,
    float angleEPS = 1e-5, bool straightenAtMarked = true);

void shorten_path(FlipEdgeNetwork* edge_network, float angleEPS = 1e-5,
    bool straightenAtMarked = true);

// std::unique_ptr<FlipEdgeNetwork>
float make_polyline(
    ManifoldSurfaceMesh* mesh, VertexPositionGeometry* geometry);
//, const vector<int>& vertices, bool closed,
// bool marked);

void subdivide_bezier(FlipEdgeNetwork* control_polygon, int num_subdivisions);

// Convert to world space positions.
std::vector<vec3f> path_positions(FlipEdgeNetwork* edge_network);

std::unique_ptr<FlipEdgeNetwork> make_polyline(ManifoldSurfaceMesh* mesh,
    VertexPositionGeometry* geometry, const std::vector<int>& vertices,
    bool closed = false, bool markInterior = false);

}  // namespace flipout
