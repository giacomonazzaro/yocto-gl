#pragma once
#include <yocto/yocto_shape.h>

#include "ext/flipout/flipout.h"
using namespace yocto;

struct spline_mesh : scene_shape {
  vector<vec3i>        adjacencies = {};
  dual_geodesic_solver dual_solver = {};
  shape_bvh            bvh         = {};
};

inline vector<mesh_point> compute_geodesic_path(
    const spline_mesh& mesh, const mesh_point& start, const mesh_point& end) {
  return compute_shortest_path(mesh.dual_solver, mesh.triangles, mesh.positions,
      mesh.adjacencies, start, end);
}

inline vector<vec3f> eval_positions(const vector<vec3i>& triangles,
    const vector<vec3f>& positions, const vector<mesh_point>& points) {
  auto result = vector<vec3f>(points.size());
  for (int i = 0; i < points.size(); i++) {
    result[i] = eval_position(triangles, positions, points[i]);
  }
  return result;
}

inline vector<mesh_point> trace_path(
    const spline_mesh& mesh, const vector<mesh_point>& points) {
  // geodesic path
  auto path = vector<mesh_point>{};
  for (auto idx = 0; idx < (int)points.size() - 1; idx++) {
    auto segment = compute_geodesic_path(mesh, points[idx], points[idx + 1]);
    //    auto segment = convert_mesh_path(
    //        mesh.triangles, mesh.adjacencies, p.strip, p.lerps, p.start,
    //        p.end)
    //                       .points;
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
  //         "mesh contains the degenerate triangle [%d, %d, %d]\n", tr.x, tr.y,
  //         tr.z);
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