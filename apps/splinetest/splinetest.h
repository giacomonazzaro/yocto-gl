#pragma once
#include <yocto/yocto_cli.h>
#include <yocto/yocto_mesh.h>
#include <yocto/yocto_sampling.h>
#include <yocto/yocto_scene.h>
#include <yocto/yocto_shape.h>

#include "ext/flipout/flipout.h"
using namespace yocto;
using namespace flipout;

struct spline_mesh : scene_shape {
  vector<vec3i>        adjacencies  = {};
  dual_geodesic_solver dual_solver  = {};
  shape_bvh            bvh          = {};
  flipout_mesh         flipout_mesh = {};
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

//
// inline shortest_path_stats test_shortest_path(
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

inline vector<vec3f> bezier_curve(const spline_mesh& mesh,
    const vector<mesh_point>& control_points, const test_params& params) {
  if (params.flipout) {
    auto vertices = vector<int>(4);
    for (int i = 0; i < 4; i++) {
      vertices[i] = mesh.triangles[control_points[i].face].x;
    }
    auto bezier = make_polyline(mesh.flipout_mesh.topology.get(),
        mesh.flipout_mesh.geometry.get(), vertices);
    subdivide_bezier(bezier.get(), params.subdivisions);
    return path_positions(bezier.get());
  }

  auto points = compute_bezier_path(mesh.dual_solver, mesh.triangles,
      mesh.positions, mesh.adjacencies, control_points, params);
  return polyline_positions(mesh, points);
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
    const vec3f& camera_from, const vec3f& camera_to, float camera_lens,
    float camera_aspect, uint64_t trial, int num_points = 4,
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

    auto ray  = camera_ray(lookat_frame(camera_from, camera_to, {0, 1, 0}),
        camera_lens, camera_aspect, 0.036f, uv);
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
