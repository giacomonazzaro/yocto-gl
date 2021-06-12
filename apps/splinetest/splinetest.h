#pragma once
#include <yocto/yocto_cli.h>
#include <yocto/yocto_mesh.h>
#include <yocto/yocto_sampling.h>
#include <yocto/yocto_scene.h>
#include <yocto/yocto_sceneio.h>
#include <yocto/yocto_shape.h>

#include "ext/flipout/flipout.h"
using namespace yocto;
using namespace flipout;

struct spline_mesh : scene_shape {
  vector<vec3i>        adjacencies     = {};
  dual_geodesic_solver dual_solver     = {};
  shape_bvh            bvh             = {};
  Flipout_Mesh         flipout_mesh    = {};
  bbox3f               bbox            = invalidb3f;
  float                max_edge_length = 0;
};

struct test_params : spline_params {
  bool flipout = false;
};

inline vector<mesh_point> shortest_path(
    const spline_mesh& mesh, const mesh_point& start, const mesh_point& end) {
  return compute_shortest_path(mesh.dual_solver, mesh.triangles, mesh.positions,
      mesh.adjacencies, start, end);
}

inline vector<vec3f> polyline_positions(
    const spline_mesh& mesh, const vector<mesh_point>& control_points) {
  if (control_points.empty()) return {};
  auto points = vector<mesh_point>{};
  for (int i = 0; i < control_points.size() - 1; i++) {
    auto a  = control_points[i];
    auto b  = control_points[i + 1];
    auto pp = shortest_path(mesh, a, b);
    points.insert(points.end(), pp.begin(), pp.end());
  }
  auto positions = vector<vec3f>(points.size());
  for (int i = 0; i < points.size(); i++) {
    positions[i] = eval_position(mesh.triangles, mesh.positions, points[i]);
  }
  return positions;
}

struct shortest_path_stats {
  double initial_guess = 0;
  double shortening    = 0;
};

struct bezier_test {
  vector<vec3f> positions          = {};
  double        seconds            = 0;
  double        seconds_avg        = 0;
  float         max_angle          = 0;
  float         max_segment_length = 0;
  int           num_control_points = 0;
};

inline shortest_path_stats test_shortest_path(const spline_mesh& mesh,
    const mesh_point& start, const mesh_point& end, bool flipout) {
  if (flipout) {
    //    auto vertices = vector<int>(4);
    //    for (int i = 0; i < 4; i++) {
    //      vertices[i] = mesh.triangles[control_points[i].face][i % 3];
    //    }
    int  va       = mesh.triangles[start.face][0];
    int  vb       = mesh.triangles[end.face][0];
    auto polyline = create_path_from_points(
        (ManifoldSurfaceMesh*)mesh.flipout_mesh.topology.get(),
        (VertexPositionGeometry*)mesh.flipout_mesh.geometry.get(), va, vb);
    return {polyline.second.initial_guess, polyline.second.shortening};
  }

  auto result = shortest_path_stats{};
  auto path   = geodesic_path{};
  auto t0     = simple_timer{};
  auto strip  = compute_strip(
      mesh.dual_solver, mesh.triangles, mesh.positions, end, start);
  strip                = reduce_strip(mesh.dual_solver, strip);
  result.initial_guess = elapsed_seconds(t0);

  auto t1 = simple_timer{};
  path    = shortest_path(
      mesh.triangles, mesh.positions, mesh.adjacencies, start, end, strip);
  result.shortening = elapsed_seconds(t1);

  return result;
}
//
//  shortest_path_stats test_shortest_path(
//    const spline_mesh& mesh, const mesh_point& start, const mesh_point& end) {
//  auto result = shortest_path_stats{};
//  auto path   = geodesic_path{};
//  auto t0     = simple_timer{};
//  auto strip  = compute_strip(
//      mesh.dual_solver, mesh.triangles, mesh.positions, end, start);
//  strip                = reduce_strip(mesh.dual_solver, strip);
//  result.initial_guess = elapsed_seconds(t0);
//
//  auto t1 = simple_timer{};
//  path    = shortest_path(mesh.triangles, mesh.positions, mesh.adjacencies,
//  start, end, strip); result.shortening = elapsed_seconds(t1);
//
//  // get mesh points
//  // return convert_mesh_path(
//  // triangles, adjacencies, path.strip, path.lerps, path.start, path.end)
//  // .points;
//}

inline vector<vec3f> eval_positions(const vector<vec3i>& triangles,
    const vector<vec3f>& positions, const vector<mesh_point>& points) {
  auto result = vector<vec3f>(points.size());
  for (int i = 0; i < points.size(); i++) {
    result[i] = eval_position(triangles, positions, points[i]);
  }
  return result;
}

// TODO(giacomo): put in yocto_mesh (explode params)
inline vec2f tangent_path_direction(const dual_geodesic_solver& solver,
    const vector<vec3i>& triangles, const vector<vec3f>& positions,
    const vector<vec3i>& adjacencies, const geodesic_path& path,
    bool start = true) {
  auto find = [](const vec3i& vec, int x) {
    for (int i = 0; i < size(vec); i++)
      if (vec[i] == x) return i;
    return -1;
  };

  auto direction = vec2f{};

  if (start) {
    auto start_tr = triangle_coordinates(triangles, positions, path.start);

    if (path.lerps.empty()) {
      direction = interpolate_triangle(
          start_tr[0], start_tr[1], start_tr[2], path.end.uv);
    } else {
      auto x    = path.lerps[0];
      auto k    = find(adjacencies[path.strip[0]], path.strip[1]);
      direction = lerp(start_tr[k], start_tr[(k + 1) % 3], x);
    }
  } else {
    auto end_tr = triangle_coordinates(triangles, positions, path.end);
    if (path.lerps.empty()) {
      direction = interpolate_triangle(
          end_tr[0], end_tr[1], end_tr[2], path.start.uv);
    } else {
      auto x = path.lerps.back();
      auto k = find(
          adjacencies[path.strip.rbegin()[0]], path.strip.rbegin()[1]);
      direction = lerp(end_tr[k], end_tr[(k + 1) % 3], 1 - x);
    }
  }
  return normalize(direction);
}

inline bool is_bezier_straight_enough(const geodesic_path& a,
    const geodesic_path& b, const geodesic_path& c,
    const dual_geodesic_solver& solver, const vector<vec3i>& triangles,
    const vector<vec3f>& positions, const vector<vec3i>& adjacencies,
    const spline_params& params) {
  // TODO(giacomo): we don't need all positions!
  // auto a_positions = path_positions(solver, triangles, positions,
  // adjacencies,  a); auto b_positions = path_positions(solver, triangles,
  // positions, adjacencies,  b); auto c_positions = path_positions(solver,
  // triangles, positions, adjacencies,  c);

  {
    // On curve apex we may never reach straightess, so we check curve
    // length.
    auto pos  = array<vec3f, 4>{};
    pos[0]    = eval_position(triangles, positions, a.start);
    pos[1]    = eval_position(triangles, positions, b.start);
    pos[2]    = eval_position(triangles, positions, c.start);
    pos[3]    = eval_position(triangles, positions, c.end);
    float len = 0;
    for (int i = 0; i < 3; i++) {
      len += length(pos[i] - pos[i + 1]);
    }
    if (len < params.min_curve_size) return true;
  }

  {
    auto dir0 = tangent_path_direction(
        solver, triangles, positions, adjacencies, a, false);  // end
    auto dir1 = tangent_path_direction(
        solver, triangles, positions, adjacencies, b, true);  // start
    auto angle1 = cross(dir0, dir1);
    if (fabs(angle1) > params.precision) {
      // printf("a1: %f > %f\n", angle1, params.precision);
      return false;
    }
  }

  {
    auto dir0 = tangent_path_direction(
        solver, triangles, positions, adjacencies, b, false);  // end
    auto dir1 = tangent_path_direction(
        solver, triangles, positions, adjacencies, c, true);  // start
    auto angle1 = cross(dir0, dir1);
    if (fabs(angle1) > params.precision) {
      // printf("a2: %f > %f\n", angle1, params.precision);
      return false;
    }
  }

  return true;
}

inline float tangent_space_angle(
    const vec3f& a, const vec3f& b, const vec3f& c) {
  return yocto::abs(1 - (dot(-normalize(a - b), normalize(c - b))));
}

inline float tangent_space_angle(const spline_mesh& mesh, const mesh_point& a,
    const mesh_point& b, const mesh_point& c) {
  auto base_triangle = triangle_coordinates(mesh.triangles, mesh.positions, b);
  auto get_coords    = [&](mesh_point p) -> vec2f {
    if (p.face == b.face) {
      return interpolate_triangle(
          base_triangle[0], base_triangle[1], base_triangle[2], p.uv);
    } else {
      auto neigh_triangle = unfold_face(
          mesh.triangles, mesh.positions, base_triangle, b.face, p.face);
      return interpolate_triangle(
          neigh_triangle[0], neigh_triangle[1], neigh_triangle[2], p.uv);
    }
  };

  auto dir0 = get_coords(a);
  auto dir1 = get_coords(c);
  return yocto::abs(cross(dir0, dir1));
}

inline float max_tangent_space_angle(
    const spline_mesh& mesh, const vector<mesh_point>& points) {
  auto max_angle = 0.0f;
  for (int i = 1; i < points.size() - 1; i++) {
    auto angle = tangent_space_angle(
        mesh, points[i - 1], points[i], points[i + 1]);
    max_angle = yocto::max(max_angle, angle);
  }
  return max_angle;
}

inline float max_tangent_space_angle(const vector<vec3f>& positions) {
  auto max_angle = 0.0f;
  for (int i = 1; i < positions.size() - 1; i++) {
    auto angle = tangent_space_angle(
        positions[i - 1], positions[i], positions[i + 1]);
    max_angle = yocto::max(max_angle, angle);
  }
  return max_angle;
}

inline shortest_path_stats test_control_polygon(
    const spline_mesh& mesh, const vector<mesh_point>& points, bool flipout) {
  auto result = shortest_path_stats{};
  if (flipout) {
    for (int i = 0; i < 3; i++) {
      int  va       = mesh.triangles[points[i].face][0];
      int  vb       = mesh.triangles[points[i + 1].face][0];
      auto polyline = create_path_from_points(
          (ManifoldSurfaceMesh*)mesh.flipout_mesh.topology.get(),
          (VertexPositionGeometry*)mesh.flipout_mesh.geometry.get(), va, vb);
      result.initial_guess += polyline.second.initial_guess;
      result.shortening += polyline.second.shortening;
    }
    return result;
  }

  for (int i = 0; i < 3; i++) {
    auto start = points[i];
    auto end   = points[i + 1];
    auto path  = geodesic_path{};
    auto t0    = simple_timer{};
    auto strip = compute_strip(
        mesh.dual_solver, mesh.triangles, mesh.positions, end, start);
    strip = reduce_strip(mesh.dual_solver, strip);
    result.initial_guess += elapsed_seconds(t0);

    auto t1 = simple_timer{};
    path    = shortest_path(
        mesh.triangles, mesh.positions, mesh.adjacencies, start, end, strip);
    result.shortening += elapsed_seconds(t1);
  }

  return result;
}

inline std::unique_ptr<FlipEdgeNetwork> flipout_bezier_curve(
    const spline_mesh& mesh, const vector<mesh_point>& control_points,
    const test_params& params) {
  auto vertices = vector<int>(4);
  for (int i = 0; i < 4; i++) {
    vertices[i] = mesh.triangles[control_points[i].face].x;
  }
  auto bezier = make_polyline(mesh.flipout_mesh.topology.get(),
      mesh.flipout_mesh.geometry.get(), vertices);
  subdivide_bezier(bezier.get(), params.subdivisions);
  return bezier;
}

inline bezier_test test_bezier_curve(const spline_mesh& mesh,
    const vector<mesh_point>& control_points, const test_params& params) {
  auto result = bezier_test{};
  if (!params.flipout) {
    auto t0        = simple_timer{};
    auto points    = compute_bezier_path(mesh.dual_solver, mesh.triangles,
        mesh.positions, mesh.adjacencies, control_points, params);
    result.seconds = elapsed_seconds(t0);

    //    result.max_angle = max_tangent_space_angle(mesh, points);
    result.positions = polyline_positions(mesh, points);
  } else {
    auto t0        = simple_timer{};
    auto bezier    = flipout_bezier_curve(mesh, control_points, params);
    result.seconds = elapsed_seconds(t0);

    result.positions = path_positions(bezier.get());
  }

  for (int i = 1; i < result.positions.size(); i++) {
    auto len = length(result.positions[i] - result.positions[i - 1]);
    result.max_segment_length = yocto::max(len, result.max_segment_length);
  }
  result.num_control_points = (int)control_points.size();
  result.seconds_avg = result.seconds / ((result.num_control_points - 1) / 3);
  return result;
}

inline vector<mesh_point> shortest_path(
    const spline_mesh& mesh, const vector<mesh_point>& points) {
  // geodesic path
  auto path = vector<mesh_point>{};
  for (auto idx = 0; idx < (int)points.size() - 1; idx++) {
    auto segment = compute_shortest_path(mesh.dual_solver, mesh.triangles,
        mesh.positions, mesh.adjacencies, points[idx], points[idx + 1]);
    path.insert(path.end(), segment.begin(), segment.end());
  }
  return path;
}

inline pair<bool, string> validate_mesh(
    const spline_mesh& mesh, float connection_threshold = 1) {
  // check for connected components
  auto visited     = vector<bool>(mesh.adjacencies.size(), false);
  auto num_visited = 0;
  auto stack       = vector<int>{0};
  while (!stack.empty()) {
    auto face = stack.back();
    stack.pop_back();
    if (visited[face]) continue;
    visited[face] = true;
    num_visited += 1;
    for (auto neighbor : mesh.adjacencies[face]) {
      assert(neighbor != -1);
      if (neighbor < 0 || visited[neighbor]) continue;
      stack.push_back(neighbor);
    }
  }

  if (num_visited < mesh.adjacencies.size() * connection_threshold) {
    auto error = string(256, '\0');
    sprintf(error.data(), "mesh is not connectd enough, connection = %f < %f\n",
        float(num_visited) / mesh.adjacencies.size(), connection_threshold);
    error.resize(strlen(error.data()));
    return {false, error};
  }

  // Check for degenerate and almost-degenerate triangles.
  // for (auto& tr : mesh.triangles) {
  //   if (tr.x == tr.y || tr.y == tr.z || tr.z == tr.x) {
  //     auto error = string(256, '\0');
  //     sprintf(error.data(),
  //         "mesh contains the degenerate triangle [%d, %d, %d]\n", tr.x,
  //         tr.y, tr.z);
  //     return {false, error};
  //   }
  // }
  // auto min_len = flt_max;
  // for (auto& tr : mesh.triangles) {
  //   auto x  = mesh.positions[tr.x];
  //   auto y  = mesh.positions[tr.y];
  //   auto z  = mesh.positions[tr.z];
  //   min_len = yocto::min(min_len, length(x - y));
  //   min_len = yocto::min(min_len, length(y - z));
  //   min_len = yocto::min(min_len, length(z - x));
  // }
  //
  // if (min_len == 0) {
  //   auto error = "mesh contains zero-length edges\n"s;
  //   return {false, error};
  // }
  // printf("min edge length: %f\n", min_len);

  return {true, ""};
}

inline vector<mesh_point> sample_points(const vector<vec3i>& triangles,
    const vector<vec3f>& positions, const bbox3f& bbox, const shape_bvh& bvh,
    const scene_camera& camera, uint64_t trial, int num_points = 4,
    int ray_trials = 10000) {
  // init data
  auto points  = vector<mesh_point>{};
  auto rng_ray = make_rng(9867198237913, trial * 2 + 1);
  // try to pick in the camera
  auto  ray_trial = 0;
  float aspect    = size(bbox).x / size(bbox).y;
  auto  uvs       = vector<vec2f>{};
  while (points.size() < num_points) {
    if (ray_trial++ >= ray_trials) break;
    auto uv = rand2f(rng_ray);
    if (points.size()) {
      uv = 2 * uv - vec2f{1, 1};
      uv *= min(aspect, 1 / aspect);
      uv += 2 * uvs[0] - vec2f{1, 1};
      uv = uv * 0.5 + vec2f{0.5, 0.5};
    }

    auto ray = camera_ray(camera.frame, camera.lens, camera.aspect, 0.036f, uv);
    auto isec = intersect_triangles_bvh(bvh, triangles, positions, ray);
    if (!isec.hit) continue;
    if (isec.element < 0 || isec.element > triangles.size()) continue;
    if (!(isec.uv.x > 0 && isec.uv.x < 1)) continue;
    if (!(isec.uv.y > 0 && isec.uv.y < 1)) continue;
    points.push_back({isec.element, isec.uv});
    uvs.push_back(uv);
  }
  // pick based on area
  auto rng_area = make_rng(9867198237913);
  auto cdf      = sample_triangles_cdf(triangles, positions);
  while (points.size() < num_points) {
    auto [triangle, uv] = sample_triangles(
        cdf, rand1f(rng_area), rand2f(rng_area));
    points.push_back({mesh_point{triangle, uv}});
  }
  return points;
}

inline void init_mesh(spline_mesh& mesh) {
  mesh.adjacencies = face_adjacencies(mesh.triangles);

  for (auto& p : mesh.positions) mesh.bbox = merge(mesh.bbox, p);
  auto s = max(size(mesh.bbox));
  auto c = center(mesh.bbox);
  for (auto& p : mesh.positions) p = (p - c) / s;

  mesh.bbox.max = (mesh.bbox.max - c) / s;
  mesh.bbox.min = (mesh.bbox.min - c) / s;

  for (auto& [a, b, c] : mesh.triangles) {
    auto l0              = length(mesh.positions[a] - mesh.positions[b]);
    auto l1              = length(mesh.positions[a] - mesh.positions[c]);
    auto l2              = length(mesh.positions[b] - mesh.positions[c]);
    mesh.max_edge_length = yocto::max(l0, mesh.max_edge_length);
    mesh.max_edge_length = yocto::max(l1, mesh.max_edge_length);
    mesh.max_edge_length = yocto::max(l2, mesh.max_edge_length);
  }

  mesh.bvh = make_triangles_bvh(mesh.triangles, mesh.positions, {});

  mesh.dual_solver = make_dual_geodesic_solver(
      mesh.triangles, mesh.positions, 0.05);
}
