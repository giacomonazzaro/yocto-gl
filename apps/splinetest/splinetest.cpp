//
// LICENSE:
//
// Copyright (c) 2016 -- 2020 Fabio Pellacini
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//

#include "splinetest.h"

#include <float.h>
#include <yocto/yocto_cli.h>
#include <yocto/yocto_math.h>
#include <yocto/yocto_mesh.h>
#include <yocto/yocto_modelio.h>
#include <yocto/yocto_sampling.h>
#include <yocto/yocto_shape.h>

#include "scene_export.h"

using namespace yocto;

// default camera
struct camera_settings {
  vec3f from   = {0, 0, 3};
  vec3f to     = {0, 0, 0};
  float lens   = 0.100f;
  float aspect = 0;
};

void save_scene(const string& scene_name, const string& mesh_name,
    const spline_mesh& mesh, const vector<vec3f>& bezier,
    const vector<vec3f>& control_polygon, const vector<mesh_point>& points,
    const camera_settings& camera) {
  string ioerror;
  if (!make_directory(path_dirname(scene_name), ioerror)) print_fatal(ioerror);
  if (!make_directory(path_join(path_dirname(scene_name), "shapes"), ioerror))
    print_fatal(ioerror);
  if (!make_directory(path_join(path_dirname(scene_name), "textures"), ioerror))
    print_fatal(ioerror);

  //  print_progress("save scene", progress.x++, progress.y++);
  auto scene_timer = simple_timer{};
  auto scene       = scene_model{};

  make_scene_floating(mesh, scene, path_basename(mesh_name), camera.from,
      camera.to, camera.lens, camera.aspect, mesh.triangles, mesh.positions,
      points, control_polygon, bezier);

  if (!save_scene(scene_name, scene, ioerror)) print_fatal(ioerror);
  // stats["scene"]             =
  //        json_value::object(); stats["scene"]["time"]     =
  //        elapsed_nanoseconds(scene_timer); stats["scene"]["filename"] =
  //        scene_name;
}

// -----------------------------------------------------------------------------
// MAIN FUNCTION
// -----------------------------------------------------------------------------
int main(int argc, const char* argv[]) {
  // command line parameters
  auto validate        = false;
  auto trials          = 100;
  auto max_edge_length = 0.0f;
  auto selected_trial  = -1;
  auto timings_name    = ""s;
  auto append_timings  = false;
  auto output_name     = ""s;
  auto mesh_name       = "mesh.ply"s;
  auto scene_name      = ""s;
  auto algorithm       = "dc"s;
  auto params          = test_params{};
  auto save            = false;

  auto convert_bin_to_ply = false;

  // parse command line
  auto cli = make_cli("ymshproc", "Applies operations on a triangle mesh");

  // Input mesh.
  add_argument(cli, "mesh", mesh_name, "input mesh");

  // Filenames for outputs. No output if empty.
  add_option(cli, "output", output_name, "output name");
  add_option(cli, "scene", scene_name, "scene path");
  add_option(cli, "convert", convert_bin_to_ply, "convert bin to ply");

  add_option(cli, "timings", timings_name, "output timings");
  add_option(cli, "append-timings", append_timings, "append timings");
  add_option(cli, "max-edge-legth", max_edge_length, "max edge length");

  // Options for test.
  add_option(cli, "validate", validate, "validate mesh");
  add_option(cli, "trials", trials, "number of trials", {1, 100});
  add_option(cli, "selected-trial", selected_trial, "run just selected trial",
      {0, 99});

  // Set parameter of algorithm.
  add_option(cli, "algorithm", algorithm,
      "bezier algorithm {dc-uniform,lr-uniform,dc-adaptive,lr-adaptiveflipout}");
  add_option(cli, "precision", params.precision, "for adaptive algorithms");
  add_option(cli, "subdivisions", params.subdivisions, "for uniform algorithms",
      {1, 10});
  parse_cli(cli, argc, argv);

  auto model_basename = path_basename(mesh_name);
  auto path_name      = ""s;
  auto stats_name     = ""s;

  if (output_name.size()) {
    string ioerror;
    if (!make_directory(output_name, ioerror)) {
      print_fatal(ioerror);
    }
    stats_name = path_join(
        path_join(output_name, "stats"), model_basename + ".json");
    path_name = path_join(
        path_join(output_name, "paths"), model_basename + ".ply");
    // scene_name   = path_join(path_join(output_name, "scenes"),
    // model_basename) +;
    timings_name = path_join(output_name, "timings.csv");
  }

  // mesh data
  auto mesh = spline_mesh{};

  // stats, progress
  //  auto stats    = json_value::object();
  auto progress = vec2i{0, 5};

  // load mesh
  print_progress("load mesh", progress.x++, progress.y);
  auto ioerror = ""s;
  if (!load_shape(mesh_name, mesh, ioerror)) print_fatal(ioerror);

  if (algorithm == "flipout") {
    mesh.flipout_mesh = flipout::make_flipout_mesh(
        mesh.triangles, mesh.positions);
  }
  mesh.adjacencies = face_adjacencies(mesh.triangles);
  //  stats["load_time"] = elapsed_nanoseconds(load_timer);
  //  stats["filename"]  = mesh_name;
  //  stats["triangles"] = mesh.triangles.size();
  //  stats["vertices"]  = mesh.positions.size();

  // check if valid
  if (validate) {
    print_progress("validate mesh", progress.x++, progress.y);
    auto [ok, error] = validate_mesh(mesh);
    //    stats["valid"]   = ok;
    if (!ok) {
      //      stats["validation"] = error;
      //      save_stats(stats_name, stats);
      print_fatal("validation error: " + error);
    }
  } else {
    //    stats["valid"] = true;
  }

  // transform
  print_progress("rescale mesh", progress.x++, progress.y);
  auto rescale_timer = simple_timer{};
  auto bbox          = invalidb3f;
  for (auto& position : mesh.positions) bbox = merge(bbox, position);
  for (auto& position : mesh.positions)
    position = (position - center(bbox)) / max(size(bbox));
  //  stats["rescale_time"] = elapsed_nanoseconds(rescale_timer);

  // build bvh
  print_progress("build bvh", progress.x++, progress.y);
  auto bvh_timer = simple_timer{};
  mesh.bvh       = make_triangles_bvh(mesh.triangles, mesh.positions, {});
  //  stats["bvh_time"] = elapsed_nanoseconds(bvh_timer);

  // build graph
  print_progress("build graph", progress.x++, progress.y);
  auto graph_timer = simple_timer{};
  mesh.dual_solver = make_dual_geodesic_solver(
      mesh.triangles, mesh.positions, 0.05);
  //  stats["solver_time"] = elapsed_nanoseconds(graph_timer);

  progress = {0, trials};

  struct spline_stat {
    int    curve_length;
    int    trial;
    double seconds;
    double initial_guess_seconds;
    double shortening_seconds;
    string error;
  };
  int  num_passed   = 0;
  auto spline_stats = vector<spline_stat>{};

  auto camera   = camera_settings{};
  camera.aspect = size(bbox).x / size(bbox).y;

  auto success = false;
  for (auto trial = 0; trial < trials; trial++) {
    if (selected_trial != -1 && trial != selected_trial) continue;

    print_progress("running trials", progress.x++, progress.y);
    size_t dummy;
    auto   hash = 0;
    try {
      hash = stoi(path_basename(mesh_name), &dummy);
    } catch (std::exception e) {
    }
    auto points = sample_points(mesh.triangles, mesh.positions, bbox, mesh.bvh,
        camera.from, camera.to, camera.lens, camera.aspect, trial + hash);

    auto& stat = spline_stats.emplace_back();

    // Try computing bezier curve.
    try {
      auto bezier_timer = simple_timer{};
      auto bezier       = bezier_curve(mesh, points, params);
      stat.seconds      = double(elapsed_nanoseconds(bezier_timer)) / 1e9;

      stat.curve_length += (int)bezier.size();
      stat.trial = trial;
      num_passed += 1;

      auto control_polygon = polyline_positions(mesh, points);

      // save scene
      if (scene_name.size() && !success) {
        save_scene(scene_name, mesh_name, mesh, bezier, control_polygon, points,
            camera);
        success = true;
      }
    } catch (std::exception& e) {
      stat.error = e.what();
    }
  }

  // TODO(giacomo): save path when we need to draw
  // save path
  // if (path_name.size()) {
  //   if (!make_directory(path_dirname(path_name), ioerror))
  //   print_fatal(ioerror print_fatal(ioerror);); print_progress("save path",
  //   progress.x++, progress.y++); if (!save_mesh_points(path_name, path,
  //   ioerror)) print_fatal(ioerror);
  // }

  // save stats
  if (stats_name.size()) {
    print_progress("save stats", progress.x++, progress.y++);
    //    save_stats(stats_name, stats);
    // if (!make_directory(path_dirname(stats_name), ioerror))
    //   print_fatal(ioerror);
    // auto stats_timer = simple_timer{};
    // if (!save_json(stats_name, stats, ioerror)) print_fatal(ioerror);
    // stats["stats"]         = json_value::object();
    // stats["stats"]["time"] = elapsed_nanoseconds(stats_timer);
  }

  // done
  print_progress("trials done", progress.x++, progress.y);

  // output timings
  if (timings_name.size() && !append_timings) {
    auto timings_file = fopen(timings_name.c_str(), "w");
    fprintf(timings_file, "model,triangles,trial,num_points,seconds,error\n");
    fclose(timings_file);
  }

  if (timings_name.size()) {
    auto timings_file = fopen(timings_name.c_str(), "a");
    for (auto& stat : spline_stats) {
      fprintf(timings_file, "%s, %d, %d, %d, %.15f %s\n", mesh_name.c_str(),
          (int)mesh.triangles.size(), stat.trial, stat.curve_length,
          stat.seconds, stat.error.c_str());
    }
    fclose(timings_file);
  }

  // else {
  //   float avg_seconds = 0;
  //   float min_seconds = spline_stats[0].seconds;
  //   float max_seconds = spline_stats[0].seconds;
  //   for (auto& stat : spline_stats) {
  //     avg_seconds += stat.seconds;
  //     max_seconds = yocto::max(max_seconds, stat.seconds);
  //     min_seconds = yocto::min(min_seconds, stat.seconds);
  //   }
  //   avg_seconds /= trials;
  //   printf("min_seconds: %f\n", min_seconds);
  //   printf("max_seconds: %f\n", max_seconds);
  //   printf("avg_seconds: %f\n", avg_seconds);
  // }

  return 0;
}
