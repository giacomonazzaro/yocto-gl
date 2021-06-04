#pragma once

#include "splinetest.h"
#include <yocto/yocto_mesh.h>
#include <yocto/yocto_sceneio.h>

#include <string>
#include <vector>

bool load_bezier_params(const std::string& filename,
    std::vector<yocto::mesh_point>& points, yocto::spline_params& params,
    std::string& error);
bool save_bezier_params(const std::string& filename,
    const std::vector<yocto::mesh_point>& points, const yocto::spline_params& params,
    std::string& error);

//bool parse_spline_algorithm(
//    spline_algorithm& algorithm, const std::string& text);
//std::string to_std::string(spline_algorithm algorithm);
