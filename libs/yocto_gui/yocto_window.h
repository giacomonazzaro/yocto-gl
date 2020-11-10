//
// Yocto/ImGui: Utilities for writing native GUIs with Dear ImGui and GLFW.
//

//
// LICENSE:
//
// Copyright (c) 2016 -- 2020 Fabio Pellacini
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//

#ifndef _YOCTO_WINDOW_
#define _YOCTO_WINDOW_

#include <yocto/yocto_math.h>

#include <array>
#include <functional>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// USING DIRECTIVES
// -----------------------------------------------------------------------------
namespace yocto {

// using directives
using std::array;
using std::function;
using std::string;
using std::vector;

}  // namespace yocto

// forward declaration
struct GLFWwindow;

// -----------------------------------------------------------------------------
// UI APPLICATION
// -----------------------------------------------------------------------------
namespace yocto {

// Forward declaration of OpenGL window
struct gui_window;

struct gui_button {
  enum struct state : unsigned char {
    up = 0,
    down,
    releasing,
    pressing,
  };
  state state = state::up;

  // TODO(giacomo): needed to keep same API for now
  operator bool() const { return state == state::down; }
};

// Input state
struct gui_input {
  // Ever-changing data
  vec2f    mouse_pos  = {0, 0};  // position excluding gui
  vec2f    mouse_last = {0, 0};  // last mouse position excluding gui
  uint64_t clock_now  = 0;       // clock now
  uint64_t clock_last = 0;       // clock last
  double   time_now   = 0;       // time now
  double   time_delta = 0;       // time delta
  int      frame      = 0;

  gui_button mouse_left   = {};
  gui_button mouse_middle = {};
  gui_button mouse_right  = {};
  vec2f      scroll       = {0, 0};  // scroll input

  bool modifier_alt   = false;  // alt modifier
  bool modifier_ctrl  = false;  // ctrl modifier
  bool modifier_shift = false;  // shift modifier

  vec2i window_size          = {0, 0};
  vec4i framebuffer_viewport = {0, 0, 0, 0};
  vec2i framebuffer_size     = {0, 0};
  bool  is_window_focused    = false;  // window is focused
  bool  widgets_active       = false;

  std::array<gui_button, 512> key_buttons = {};
};

// Init callback called after the window has opened
using init_callback = function<void(gui_window*, const gui_input& input)>;
// Clear callback called after the window is cloased
using clear_callback = function<void(gui_window*, const gui_input& input)>;
// Draw callback called every frame and when resizing
using draw_callback = function<void(gui_window*, const gui_input& input)>;
// Draw callback for drawing widgets
using widgets_callback = function<void(gui_window*, const gui_input& input)>;
// Drop callback that returns that list of dropped strings.
using drop_callback =
    function<void(gui_window*, const vector<string>&, const gui_input& input)>;
// Key callback that returns key codes, pressed/released flag and modifier keys
using key_callback =
    function<void(gui_window*, int key, bool pressed, const gui_input& input)>;
// Char callback that returns ASCII key
using char_callback =
    function<void(gui_window*, unsigned int key, const gui_input& input)>;
// Mouse click callback that returns left/right button, pressed/released flag,
// modifier keys
using click_callback = function<void(
    gui_window*, bool left, bool pressed, const gui_input& input)>;
// Scroll callback that returns scroll amount
using scroll_callback =
    function<void(gui_window*, float amount, const gui_input& input)>;
// Update functions called every frame
using uiupdate_callback = function<void(gui_window*, const gui_input& input)>;
// Update functions called every frame
using update_callback = function<void(gui_window*, const gui_input& input)>;

// User interface callcaks
struct gui_callbacks {
  init_callback     init_cb     = {};
  clear_callback    clear_cb    = {};
  draw_callback     draw_cb     = {};
  widgets_callback  widgets_cb  = {};
  drop_callback     drop_cb     = {};
  key_callback      key_cb      = {};
  char_callback     char_cb     = {};
  click_callback    click_cb    = {};
  scroll_callback   scroll_cb   = {};
  update_callback   update_cb   = {};
  uiupdate_callback uiupdate_cb = {};
};

// run the user interface with the give callbacks
void run_ui(const vec2i& size, const string& title,
    const gui_callbacks& callbaks, int widgets_width = 320,
    bool widgets_left = true);

}  // namespace yocto

// -----------------------------------------------------------------------------
// UI WINDOW
// -----------------------------------------------------------------------------
namespace yocto {

// OpenGL window wrapper
struct gui_window {
  GLFWwindow* win           = nullptr;
  string      title         = "";
  int         widgets_width = 0;
  bool        widgets_left  = true;
  gui_input   input         = {};
  vec4f       background    = {0.15f, 0.15f, 0.15f, 1.0f};

  // callbacks
  init_callback     init_cb     = {};
  clear_callback    clear_cb    = {};
  draw_callback     draw_cb     = {};
  widgets_callback  widgets_cb  = {};
  drop_callback     drop_cb     = {};
  key_callback      key_cb      = {};
  char_callback     char_cb     = {};
  click_callback    click_cb    = {};
  scroll_callback   scroll_cb   = {};
  update_callback   update_cb   = {};
  uiupdate_callback uiupdate_cb = {};
};

// Windows initialization
void init_window(gui_window* win, const vec2i& size, const string& title,
    bool widgets, int widgets_width = 320, bool widgets_left = true);

// Window cleanup
void clear_window(gui_window* win);

// Set callbacks
void set_init_callback(gui_window* win, init_callback init_cb);
void set_clear_callback(gui_window* win, clear_callback clear_cb);
void set_draw_callback(gui_window* win, draw_callback draw_cb);
void set_widgets_callback(gui_window* win, widgets_callback widgets_cb);
void set_drop_callback(gui_window* win, drop_callback drop_cb);
void set_key_callback(gui_window* win, key_callback cb);
void set_char_callback(gui_window* win, char_callback cb);
void set_click_callback(gui_window* win, click_callback cb);
void set_scroll_callback(gui_window* win, scroll_callback cb);
void set_uiupdate_callback(gui_window* win, uiupdate_callback cb);
void set_update_callback(gui_window* win, update_callback cb);

// Run loop
void run_ui(gui_window* win);
void set_close(gui_window* win, bool close);

}  // namespace yocto

#endif
