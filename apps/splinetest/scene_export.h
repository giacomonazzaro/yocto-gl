#pragma once
#include <yocto/yocto_sceneio.h>

#include "splinetest.h"

using namespace yocto;

inline scene_camera& add_camera(scene_model& scene, const frame3f& frame,
    float lens, float aspect, float aperture = 0, float focus = 10,
    bool orthographic = false, float film = 0.036) {
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

inline scene_instance& add_instance(
    scene_model& scene, int shape, int material) {
  auto& instance    = scene.instances.emplace_back();
  instance.shape    = shape;
  instance.material = material;
  return instance;
}

inline int add_shape(scene_model& scene, const scene_shape& shape_data) {
  auto  id    = (int)scene.shapes.size();
  auto& shape = scene.shapes.emplace_back();
  shape       = shape_data;
  return id;
}

inline int add_material(scene_model& scene, const vec3f& color, float roughness,
    bool unlit = false) {
  auto  id           = (int)scene.materials.size();
  auto& material     = scene.materials.emplace_back();
  material.type      = unlit ? scene_material_type::unlit
                             : scene_material_type::glossy;
  material.color     = color;
  material.roughness = roughness;
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

inline scene_shape path_to_lines(const vector<vec3i>& triangles,
    const vector<vec3f>& positions, const vector<vec3f>& path, float radius) {
  auto shape      = scene_shape{};
  shape.positions = path;  // eval_positions(triangles, positions, path);
  // shape.normals   = ...;  // TODO(fabio): tangents
  shape.radius = vector<float>(shape.positions.size(), radius);
  shape.lines  = vector<vec2i>(shape.positions.size() - 1);
  for (auto k = 0; k < shape.lines.size(); k++) shape.lines[k] = {k, k + 1};
  return shape;
}

inline scene_shape path_to_points(const vector<vec3i>& triangles,
    const vector<vec3f>& positions, const vector<vec3f>& path, float radius) {
  auto shape      = scene_shape{};
  shape.positions = path;  // eval_positions(triangles, positions, path);
  // shape.normals   = ...;  // TODO(fabio): tangents
  shape.radius = vector<float>(shape.positions.size(), radius);
  shape.points = vector<int>(shape.positions.size());
  for (auto k = 0; k < shape.points.size(); k++) shape.points[k] = k;
  return shape;
}

inline scene_shape polyline_shape(const vector<vec3f>& positions,
    float point_thickness, float line_thickness) {
  //  auto positions = eval_positions(triangles, positions, path);
  auto shape = scene_shape{};
  for (auto idx = 0; idx < positions.size(); idx++) {
    if (point_thickness > 0) {
      auto sphere = make_sphere(4, point_thickness);
      for (auto& p : sphere.positions) p += positions[idx];
      merge_quads(shape.quads, shape.positions, shape.normals, shape.texcoords,
          sphere.quads, sphere.positions, sphere.normals, sphere.texcoords);
    }
    // if (line_thickness > 0 && idx < (int)positions.size() - 1 &&
    //     length(positions[idx] - positions[idx + 1]) > point_thickness) {
    if (line_thickness > 0 && idx < (int)positions.size() - 1) {
      auto cylinder = make_uvcylinder({32, 1, 1},
          {line_thickness, length(positions[idx] - positions[idx + 1]) / 2});
      auto frame    = frame_fromz((positions[idx] + positions[idx + 1]) / 2,
          normalize(positions[idx + 1] - positions[idx]));
      for (auto& p : cylinder.positions) p = transform_point(frame, p);
      for (auto& n : cylinder.normals) n = transform_direction(frame, n);
      merge_quads(shape.quads, shape.positions, shape.normals, shape.texcoords,
          cylinder.quads, cylinder.positions, cylinder.normals,
          cylinder.texcoords);
    }
  }
  return shape;
}

inline void make_scene_floating(const spline_mesh& mesh, scene_model& scene,
    const scene_camera& camera, const vector<mesh_point>& points,
    const vector<vec3f>& path, const vector<vec3f>& curve,
    bool use_environment = false, float point_thickness = 0.006f,
    float line_thickness = 0.004f) {
  // camera
  // camera_from.x += 0.5;
  // camera_from.y += 0.5;
  scene.cameras.push_back(camera);

  // mesh
  auto mesh_color = vec3f{0.8, 0.8, 0.8};
  add_instance(
      scene, add_shape(scene, mesh), add_material(scene, mesh_color, 0.5));

  if (0) {
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
    add_instance(scene, add_shape(scene, edges),
        add_material(scene, mesh_color * 0.75, 1.0, false));
  }

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

  add_instance(scene,
      add_shape(scene, polyline_shape(control_polygon, line_thickness / 3,
                           line_thickness / 3)),
      add_material(scene, {0, 0, 1}, 0.2));

  // curve
  // printf("%d: %f\n", trial, stat.seconds);
  auto& bezier_points = curve;

  add_instance(scene,
      add_shape(
          scene, polyline_shape(bezier_points, line_thickness, line_thickness)),
      add_material(scene, {1, 0, 0}, 0.2));

  // points
  auto points_positions = vector<vec3f>(points.size());
  for (int i = 0; i < points_positions.size(); i++) {
    points_positions[i] = eval_position(
        mesh.triangles, mesh.positions, points[i]);
  }
  add_instance(scene,
      add_shape(scene, polyline_shape(points_positions, point_thickness, 0)),
      add_material(scene, {0, 0, 1}, 0.2));
}

// default camera
struct camera_settings {
  vec3f from   = {0, 0, 3};
  vec3f to     = {0, 0, 0};
  float lens   = 0.100f;
  float aspect = 0;
};

inline void save_scene(const string& scene_name, const string& mesh_name,
    const spline_mesh& mesh, const vector<vec3f>& bezier,
    const vector<vec3f>& control_polygon, const vector<mesh_point>& points,
    const scene_camera& camera) {
  string ioerror;
  if (!make_directory(path_dirname(scene_name), ioerror)) print_fatal(ioerror);
  if (!make_directory(path_join(path_dirname(scene_name), "shapes"), ioerror))
    print_fatal(ioerror);
  if (!make_directory(path_join(path_dirname(scene_name), "textures"), ioerror))
    print_fatal(ioerror);

  //  print_progress("save scene", progress.x++, progress.y++);
  auto scene_timer = simple_timer{};
  auto scene       = scene_model{};

  make_scene_floating(mesh, scene, camera, points, control_polygon, bezier);

  if (!save_scene(scene_name, scene, ioerror)) {
    printf("%s: %s\n", __FUNCTION__, ioerror.c_str());
    print_fatal(ioerror);
  }
  // stats["scene"]             =
  //        json_value::object(); stats["scene"]["time"]     =
  //        elapsed_nanoseconds(scene_timer); stats["scene"]["filename"] =
  //        scene_name;
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
