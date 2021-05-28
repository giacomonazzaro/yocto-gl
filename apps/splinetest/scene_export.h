#pragma once
#include <yocto/yocto_sceneio.h>

#include "splinetest.h"

using namespace yocto;

scene_camera& add_camera(scene_model& scene, const string& name,
    const frame3f& frame, float lens, float aspect, float aperture = 0,
    float focus = 10, bool orthographic = false, float film = 0.036) {
  auto& camera        = scene.cameras.emplace_back();
  camera.frame        = frame;
  camera.lens         = lens;
  camera.aspect       = aspect;
  camera.film         = film;
  camera.orthographic = orthographic;
  camera.aperture     = aperture;
  camera.focus        = focus;
  return camera;
}

scene_instance& add_instance(scene_model& scene, const string& name,
    const frame3f& frame, int shape, int material) {
  auto& instance    = scene.instances.emplace_back();
  instance.frame    = frame;
  instance.shape    = shape;
  instance.material = material;
  return instance;
}

int add_shape(
    scene_model& scene, const string& name, const scene_shape& shape_data) {
  auto  id    = (int)scene.shapes.size();
  auto& shape = scene.shapes.emplace_back();
  shape       = shape_data;
  return id;
}

int add_specular_material(scene_model& scene, const string& name,
    const vec3f& color, int color_tex, float roughness, int roughness_tex = -1,
    int normal_tex = -1, float ior = 1.5, float specular = 1,
    int specular_tex = -1, const vec3f& spectint = {1, 1, 1},
    int spectint_tex = -1) {
  auto  id               = (int)scene.materials.size();
  auto& material         = scene.materials.emplace_back();
  material.type          = scene_material_type::glossy;
  material.color         = color;
  material.color_tex     = color_tex;
  material.roughness     = roughness;
  material.roughness_tex = roughness_tex;
  material.ior           = ior;
  material.normal_tex    = normal_tex;
  return id;
}

//
// scene_environment& add_environment(scene_model& scene, const string&
// name,
//    const frame3f& frame, const vec3f& emission,
//    scene_texture& emission_tex = nullptr) {
//  auto environment          = add_environment(scene, name);
//  environment->frame        = frame;
//  environment->emission     = emission;
//  environment->emission_tex = emission_tex;
//  return environment;
//}
//
//}  // namespace yocto

// vector<vec3f> path_positions(const vector<vec3i>& triangles,
//     const vector<vec3f>& positions, const vector<mesh_point>& path) {
//   auto ppositions = vector<vec3f>{};
//   ppositions.reserve(path.size());
//   for (auto& point : path) {
//     auto& triangle = triangles[point.face];
//     ppositions.push_back(interpolate_triangle(positions[triangle.x],
//         positions[triangle.y], positions[triangle.z], point.uv));
//   }
//   return ppositions;
// }

scene_shape path_to_lines(const vector<vec3i>& triangles,
    const vector<vec3f>& positions, const vector<vec3f>& path,
    float radius) {
  auto shape      = scene_shape{};
    shape.positions = path; //eval_positions(triangles, positions, path);
  // shape.normals   = ...;  // TODO(fabio): tangents
  shape.radius = vector<float>(shape.positions.size(), radius);
  shape.lines  = vector<vec2i>(shape.positions.size() - 1);
  for (auto k = 0; k < shape.lines.size(); k++) shape.lines[k] = {k, k + 1};
  return shape;
}

scene_shape path_to_points(const vector<vec3i>& triangles,
    const vector<vec3f>& positions, const vector<vec3f>& path,
    float radius) {
  auto shape      = scene_shape{};
    shape.positions = path;//eval_positions(triangles, positions, path);
  // shape.normals   = ...;  // TODO(fabio): tangents
  shape.radius = vector<float>(shape.positions.size(), radius);
  shape.points = vector<int>(shape.positions.size());
  for (auto k = 0; k < shape.points.size(); k++) shape.points[k] = k;
  return shape;
}

scene_shape path_to_quads(const vector<vec3i>& triangles,
    const vector<vec3f>& positions, const vector<vec3f>& ppositions,
    float point_thickness, float line_thickness) {
//  auto ppositions = eval_positions(triangles, positions, path);
  auto shape      = scene_shape{};
  for (auto idx = 0; idx < ppositions.size(); idx++) {
    if (point_thickness > 0) {
      auto sphere = make_sphere(4, point_thickness);
      for (auto& p : sphere.positions) p += ppositions[idx];
      merge_quads(shape.quads, shape.positions, shape.normals, shape.texcoords,
          sphere.quads, sphere.positions, sphere.normals, sphere.texcoords);
    }
    // if (line_thickness > 0 && idx < (int)ppositions.size() - 1 &&
    //     length(ppositions[idx] - ppositions[idx + 1]) > point_thickness) {
    if (line_thickness > 0 && idx < (int)ppositions.size() - 1) {
      auto cylinder = make_uvcylinder({32, 1, 1},
          {line_thickness, length(ppositions[idx] - ppositions[idx + 1]) / 2});
      auto frame    = frame_fromz((ppositions[idx] + ppositions[idx + 1]) / 2,
          normalize(ppositions[idx + 1] - ppositions[idx]));
      for (auto& p : cylinder.positions) p = transform_point(frame, p);
      for (auto& n : cylinder.normals) n = transform_direction(frame, n);
      merge_quads(shape.quads, shape.positions, shape.normals, shape.texcoords,
          cylinder.quads, cylinder.positions, cylinder.normals,
          cylinder.texcoords);
    }
  }
  return shape;
}

void make_scene_floating(const spline_mesh& mesh, scene_model& scene,
    const string& mesh_name, vec3f camera_from, vec3f camera_to,
    float camera_lens, float camera_aspect, const vector<vec3i>& triangles,
    const vector<vec3f>& positions, const vector<mesh_point>& points,
    const vector<vec3f>& path, const vector<vec3f>& curve,
    bool use_environment = false, bool points_as_meshes = true,
    float point_thickness = 0.006f, float line_thickness = 0.004f) {
  auto path_name  = mesh_name + "-path";
  auto pointsname = mesh_name + "-points";

  // camera
  camera_from.x += 0.5;
  camera_from.y += 0.5;
  add_camera(scene, mesh_name + "-cam",
      lookat_frame(camera_from, camera_to, {0, 1, 0}), camera_lens,
      camera_aspect, 0, length(camera_from - camera_to));

  // mesh
  auto mesh_color = vec3f{.113, .309, .164};
  add_instance(scene, mesh_name, identity3x4f,
      add_shape(scene, mesh_name, mesh),
      add_specular_material(scene, mesh_name, mesh_color, -1, 0.5));

  auto edges = scene_shape{};
  for (auto& tr : mesh.triangles) {
    for (int k = 0; k < 3; k++) {
      auto a = tr[k];
      auto b = tr[(k + 1) % 3];
      if (a > b) continue;
      auto index = (int)edges.positions.size();
      edges.radius.push_back(0.001);
      edges.radius.push_back(0.001);
      edges.lines.push_back({index, index + 1});
      edges.positions.push_back(mesh.positions[a]);
      edges.positions.push_back(mesh.positions[b]);
    }
  }

  add_instance(scene, "edges", identity3x4f, add_shape(scene, "edges", edges),
      add_specular_material(scene, "edges", mesh_color * 0.75, -1, 0.0));
  // scene.materials.back().opacity = 0.5;

  auto control_polygon = polyline_positions(mesh, points);
  // vector<mesh_point>{};
  // for (int i = 0; i < 3; ++i) {
  //   auto a  = points[i];
  //   auto b  = points[i + 1];
  //   auto pp = compute_geodesic_path(mesh, a, b);
  //   // auto pp = convert_mesh_path(
  //   //     mesh.triangles, mesh.adjacencies, p.strip, p.lerps, p.start,
  //   p.end)
  //   //               .points;
  //   control_polygon.insert(control_polygon.end(), pp.begin(), pp.end());
  // }

  if (points_as_meshes) {
    add_instance(scene, "control_polygon", identity3x4f,
        add_shape(scene, "control_polygon",
            path_to_quads(triangles, positions, control_polygon,
                line_thickness / 3, line_thickness / 3)),
        add_specular_material(scene, "control_polygon", {0, 0, 1}, -1, 0.2));
  } else {
    add_instance(scene, "control_polygon", identity3x4f,
        add_shape(scene, "control_polygon",
            path_to_lines(
                triangles, positions, control_polygon, line_thickness / 3)),
        add_specular_material(scene, "control_polygon", {0, 0, 1}, -1, 0.2));
  }

  // curve
  // printf("%d: %f\n", trial, stat.seconds);
  auto& bezier_points = curve;
  if (points_as_meshes) {
    add_instance(scene, path_name, identity3x4f,
        add_shape(scene, path_name,
            path_to_quads(triangles, positions, bezier_points, line_thickness,
                line_thickness)),
        add_specular_material(scene, path_name, {1, 0, 0}, -1, 0.2));
  } else {
    add_instance(scene, path_name, identity3x4f,
        add_shape(scene, path_name,
            path_to_lines(triangles, positions, bezier_points, line_thickness)),
        add_specular_material(scene, path_name, {1, 0, 0}, -1, 0.2));
  }

  // points
    // TODO(giacomo): incomplete
//  if (points_as_meshes) {
//    add_instance(scene, pointsname, identity3x4f,
//        add_shape(scene, pointsname,
//            path_to_quads(triangles, positions, points, point_thickness, 0)),
//        add_specular_material(scene, pointsname, {0, 0, 1}, -1, 0.2));
//  } else {
//    add_instance(scene, pointsname, identity3x4f,
//        add_shape(scene, pointsname,
//            path_to_points(triangles, positions, points, point_thickness)),
//        add_specular_material(scene, pointsname, {0, 0, 1}, -1, 0.2));
//  }

  // environment
  // TODO(fabio): environment
  // if (use_environment) {
  //   add_environment(scene, "environment", identity3x4f, {1, 1, 1}, nullptr);
  // }
}

// Save a path
// bool save_mesh_points(
//    const string& filename, const vector<mesh_point>& path, string& error) {
//  auto format_error = [filename, &error]() {
//    error = filename + ": unknown format";
//    return false;
//  };
//
//  auto ext = path_extension(filename);
//  if (ext == ".json" || ext == ".JSON") {
//    auto js    = json_value{};
//    js         = json_value::object();
//    js["path"] = to_json(path);
//    return save_json(filename, js, error);
//  } else if (ext == ".ply" || ext == ".PLY") {
//    auto ply_guard = std::make_unique<ply_model>();
//    auto ply       = ply_guard.get();
//    auto ids       = vector<int>(path.size());
//    auto uvs       = vector<vec2f>(path.size());
//    for (auto idx = 0; idx < path.size(); idx++) {
//      ids[idx] = path[idx].face;
//      uvs[idx] = path[idx].uv;
//    }
//    add_value(ply, "mesh_points", "face", ids);
//    add_values(ply, "mesh_points", {"u", "v"}, uvs);
//    return save_ply(filename, ply, error);
//  } else {
//    return format_error();
//  }
//}
