#include <yocto/yocto_cli.h>
#include <yocto/yocto_math.h>
#include <yocto/yocto_mesh.h>
#include <yocto/yocto_modelio.h>
#include <yocto/yocto_sampling.h>
#include <yocto/yocto_shape.h>

#include "scene_export.h"
#include "splineio.h"
using namespace yocto;

int main(int argc, const char* argv[]) {
  // command line parameters

  auto  max_edge_length = 0.0f;
  auto  timings_name    = ""s;
  auto  append_timings  = false;
  auto  scene_name      = ""s;
  auto  mesh_name       = "mesh.ply"s;
  auto  test_name       = "mesh.ply"s;
  auto  algorithm       = "dc-uniform"s;
  auto  params          = spline_params{};
  auto  save            = false;
  float line_thickness  = 0.004f;

  // parse command line
  auto cli = make_cli("splinetime", "Applies operations on a triangle mesh");

  // Input mesh.
  add_argument(cli, "mesh", mesh_name, "input mesh");

  // Filenames for outputs. No output if empty.
  add_option(cli, "test", test_name, "test path");
  add_option(cli, "scene", scene_name, "scene name");

  add_option(cli, "line-thickness", line_thickness, "line thickness");
  add_option(cli, "timings", timings_name, "output timings");
  add_option(cli, "append-timings", append_timings, "append timings");
  add_option(cli, "max-edge-legth", max_edge_length, "max edge length");

  // Set parameter of algorithm.
  add_option(cli, "algorithm", algorithm,
      "bezier algorithm {dc-uniform,lr-uniform,dc-adaptive,lr-adaptive,flipout}");
  // add_option(cli, "precision", params.precision, "for adaptive algorithms");
  // add_option(cli, "subdivisions", params.subdivisions, "for uniform
  // algorithms", {1, 10});
  parse_cli(cli, argc, argv);

  // if (algorithm == "flipout") {
  //   params.flipout = true;
  // } else {
  //   for (auto i = 0; i < spline_algorithm_names.size(); i++) {
  //     if (algorithm == spline_algorithm_names[i])
  //       params.algorithm = (spline_algorithm)i;
  //   }
  // }

  auto model_basename = path_basename(mesh_name);
  auto path_name      = ""s;
  auto stats_name     = ""s;

  //  if (scene_name.size()) {
  //    string ioerror;
  //    if (!make_directory(scene_name, ioerror)) {
  //      print_fatal(ioerror);
  //    }
  //    stats_name = path_join(
  //        path_join(scene_name, "stats"), model_basename + ".json");
  //    path_name = path_join(
  //        path_join(scene_name, "paths"), model_basename + ".ply");
  //    // scene_name   = path_join(path_join(scene_name, "scenes"),
  //    // model_basename) +;
  //    timings_name = path_join(scene_name, "timings.csv");
  //  }

  // mesh data

  auto mesh     = spline_mesh{};
  auto progress = vec2i{0, 5};

  // load mesh
  print_progress("load mesh", progress.x++, progress.y);
  auto ioerror = ""s;
  if (!load_shape(mesh_name, mesh, ioerror)) print_fatal(ioerror);

  init_mesh(mesh);

  auto error       = string{};
  auto spline_test = load_test(test_name, error);
  if (error.size()) {
    printf("error: %s\n", error.c_str());
  }

  auto xxx   = test_params{};
  auto stats = test_bezier_curve(mesh, spline_test.control_points, xxx);
  printf("seconds: %lf, max_angle: %f\n", stats.seconds, stats.max_angle);

  // done
  print_progress("trials done", progress.x++, progress.y);

  // output timings
  if (timings_name.size() && !append_timings) {
    auto timings_file = fopen(timings_name.c_str(), "w");
    fprintf(timings_file,
        "model,triangles,num_points,bezier_tot(s),bezier_avg(s),num_control_points,max_angle(rad)\n");
    fclose(timings_file);
  }

  if (timings_name.size()) {
    auto timings_file = fopen(timings_name.c_str(), "a");
    fprintf(timings_file, "%s, %d, %d, %.15f, %.15f, %d, %f\n",
        mesh_name.c_str(), (int)mesh.triangles.size(),
        (int)stats.positions.size(), stats.seconds, stats.seconds_avg,
        stats.num_control_points, stats.max_angle);
    fclose(timings_file);
  }

  if (scene_name.size()) {
    auto curve           = stats.positions;
    auto control_polygon = polyline_positions(mesh, spline_test.control_points);
    auto control_points  = spline_test.control_points;
    save_scene(scene_name, mesh_name, mesh, curve, control_polygon,
        control_points, spline_test.camera, line_thickness);
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
}
