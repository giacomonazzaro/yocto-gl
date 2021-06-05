#pragma once

#include <yocto/yocto_mesh.h>
#include <yocto/yocto_sceneio.h>

#include <string>
#include <vector>

#include "splinetest.h"

struct spline_test {
  std::vector<yocto::mesh_point> control_points = {};
  yocto::scene_camera            camera         = {};
  yocto::spline_params           params         = {};
};

spline_test load_test(const std::string& filename, std::string& error);

// bool save_bezier_params(const std::string& filename,
//     const std::vector<yocto::mesh_point>&  points,
//     const yocto::spline_params& params, std::string& error);

// bool parse_spline_algorithm(
//    spline_algorithm& algorithm, const std::string& text);
// std::string to_std::string(spline_algorithm algorithm);
