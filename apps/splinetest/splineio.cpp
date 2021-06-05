#include "splineio.h"

// #include <yocto/yocto_commonio.h>

#include "ext/json.hpp"

using json = nlohmann::json;

// -----------------------------------------------------------------------------
// JSON SUPPORT
// -----------------------------------------------------------------------------
namespace yocto {

using json = nlohmann::json;
using std::array;

// support for json conversions
inline void to_json(json& j, const vec2f& value) {
  nlohmann::to_json(j, (const array<float, 2>&)value);
}

inline void from_json(const json& j, vec2f& value) {
  nlohmann::from_json(j, (array<float, 2>&)value);
}

// support for json conversions
inline void to_json(json& js, const mesh_point& value) {
  js["face"] = value.face;
  js["uv"]   = value.uv;
}

inline void from_json(const json& js, mesh_point& value) {
  js.at("face").get_to(value.face);
  js.at("uv").get_to(value.uv);
}

inline void to_json(json& j, const frame3f& value) {
  nlohmann::to_json(j, (const array<float, 12>&)value);
}
inline void from_json(const json& j, frame3f& value) {
  nlohmann::from_json(j, (array<float, 12>&)value);
}
inline void to_json(json& js, const scene_camera& camera) {
  js["frame"]    = camera.frame;
  js["lens"]     = camera.lens;
  js["aspect"]   = camera.aspect;
  js["film"]     = camera.film;
  js["aperture"] = camera.aperture;
  js["focus"]    = camera.focus;
}

inline void from_json(const json& js, scene_camera& camera) {
  js.at("frame").get_to(camera.frame);
  js.at("lens").get_to(camera.lens);
  js.at("aspect").get_to(camera.aspect);
  js.at("film").get_to(camera.film);
  js.at("aperture").get_to(camera.aperture);
  js.at("focus").get_to(camera.focus);
}
}  // namespace yocto

bool parse_spline_algorithm(spline_algorithm& algorithm, const string& text) {
  for (auto i = 0; i < spline_algorithm_names.size(); i++) {
    if (text == spline_algorithm_names[i]) {
      algorithm = (spline_algorithm)i;
      return true;
    }
  }
  return false;
}

string to_string(spline_algorithm algorithm) {
  return spline_algorithm_names[(int)algorithm];
}

inline bool save_json(const string& filename, const json& js, string& error) {
  return save_text(filename, js.dump(2), error);
}

inline bool load_json(const string& filename, json& js, string& error) {
  // error helpers
  auto parse_error = [filename, &error]() -> json {
    error = filename + ": parse error in json";
    printf("error loading json %s\n:  %s\n", filename.c_str(), error.c_str());
    return false;
  };
  auto text = ""s;
  if (!load_text(filename, text, error)) return false;
  try {
    js = json::parse(text);
    return true;
  } catch (std::exception& e) {
    return parse_error();
  }

  return true;
}

// support for json conversions
inline void to_json(json& js, const spline_algorithm& value) {
  js = spline_algorithm_names[(int)value];
}

inline void from_json(const json& js, spline_algorithm& value) {
  auto vs = js.get<string>();
  for (auto i = 0; i < spline_algorithm_names.size(); i++) {
    if (vs == spline_algorithm_names[i]) {
      value = (spline_algorithm)i;
      return;
    }
  }
  throw std::invalid_argument{"unknown spline_algorithm"};
}

spline_test load_test(const string& filename, string& error) {
  auto js     = json{};
  auto result = spline_test{};
  if (!load_json(filename, js, error)) return {};
  try {
    result.control_points =
        js["splines"][0]["control_points"].get<vector<mesh_point>>();
    result.camera.frame = js["camera"]["frame"];
    result.camera.focus = js["camera"]["focus"];
    // params.algorithm    = js["params"]["algorithm"];
    // params.subdivisions = js["params"]["subdivisions"];
    // params.precision    = js["params"]["precision"];
  } catch (std::exception& e) {
    error = "error parsing json: ("s + e.what() + ")"s;
    return {};
  }
  return result;
}

bool save_bezier_params(const string& filename,
    const vector<mesh_point>& control_points, const spline_params& params,
    string& error) {
  auto js                      = json{};
  js["points"]                 = control_points;
  js["params"]["algorithm"]    = params.algorithm;
  js["params"]["subdivisions"] = params.subdivisions;
  js["params"]["precision"]    = params.precision;
  if (!save_json(filename, js, error)) return false;
  return true;
}
