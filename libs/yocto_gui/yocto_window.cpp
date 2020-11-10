//
// Utilities to use OpenGL 3, GLFW and ImGui.
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

// TODO(giacomo): review inlcudes
#include "yocto_window.h"

#include <yocto/yocto_commonio.h>

#include <algorithm>
#include <array>
#include <cstdarg>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include "ext/glad/glad.h"

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GLFW/glfw3.h>

#ifdef _WIN32
#undef near
#undef far
#endif

// -----------------------------------------------------------------------------
// USING DIRECTIVES
// -----------------------------------------------------------------------------
namespace yocto {

// using directives
using std::array;
using std::mutex;
using std::pair;
using std::unordered_map;
using namespace std::string_literals;

}  // namespace yocto

// -----------------------------------------------------------------------------
// UI APPLICATION
// -----------------------------------------------------------------------------
namespace yocto {

// run the user interface with the give callbacks
void run_ui(const vec2i& size, const string& title,
    const gui_callbacks& callbacks, int widgets_width, bool widgets_left) {
  auto win_guard = std::make_unique<gui_window>();
  auto win       = win_guard.get();
  init_window(win, size, title, (bool)callbacks.widgets_cb, widgets_width,
      widgets_left);

  set_init_callback(win, callbacks.init_cb);
  set_clear_callback(win, callbacks.clear_cb);
  set_draw_callback(win, callbacks.draw_cb);
  set_widgets_callback(win, callbacks.widgets_cb);
  set_drop_callback(win, callbacks.drop_cb);
  set_key_callback(win, callbacks.key_cb);
  set_char_callback(win, callbacks.char_cb);
  set_click_callback(win, callbacks.click_cb);
  set_scroll_callback(win, callbacks.scroll_cb);
  set_update_callback(win, callbacks.update_cb);
  set_uiupdate_callback(win, callbacks.uiupdate_cb);

  // run ui
  run_ui(win);

  // clear
  clear_window(win);
}

}  // namespace yocto

// -----------------------------------------------------------------------------
// UI WINDOW
// -----------------------------------------------------------------------------
namespace yocto {

inline void update_button_from_input(gui_button& button, bool pressing) {
  if (pressing) {
    assert(button.state != gui_button::state::down);
    button.state = gui_button::state::pressing;
  } else {
    button.state = gui_button::state::releasing;
  }
}

inline void update_button_for_next_frame(gui_button& button) {
  if (button.state == gui_button::state::pressing) {
    button.state = gui_button::state::down;
  } else if (button.state == gui_button::state::releasing) {
    button.state = gui_button::state::up;
  }
}

static void draw_window(gui_window* win) {
  glClearColor(win->background.x, win->background.y, win->background.z,
      win->background.w);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  if (win->draw_cb) win->draw_cb(win, win->input);
  // if (win->widgets_cb) {
  //   ImGui_ImplOpenGL3_NewFrame();
  //   ImGui_ImplGlfw_NewFrame();
  //   ImGui::NewFrame();
  //   auto window = zero2i;
  //   glfwGetWindowSize(win->win, &window.x, &window.y);
  //   if (win->widgets_left) {
  //     ImGui::SetNextWindowPos({0, 0});
  //     ImGui::SetNextWindowSize({(float)win->widgets_width, (float)window.y});
  //   } else {
  //     ImGui::SetNextWindowPos({(float)(window.x - win->widgets_width), 0});
  //     ImGui::SetNextWindowSize({(float)win->widgets_width, (float)window.y});
  //   }
  //   ImGui::SetNextWindowCollapsed(false);
  //   ImGui::SetNextWindowBgAlpha(1);
  //   if (ImGui::Begin(win->title.c_str(), nullptr,
  //           ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
  //               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
  //               ImGuiWindowFlags_NoSavedSettings)) {
  //     win->widgets_cb(win, win->input);
  //   }
  //   ImGui::End();
  //   ImGui::Render();
  //   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  // }
  glfwSwapBuffers(win->win);
}

void init_window(gui_window* win, const vec2i& size, const string& title,
    bool widgets, int widgets_width, bool widgets_left) {
  // init glfw
  if (!glfwInit())
    throw std::runtime_error("cannot initialize windowing system");
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // create window
  win->title = title;
  win->win = glfwCreateWindow(size.x, size.y, title.c_str(), nullptr, nullptr);
  if (win->win == nullptr)
    throw std::runtime_error{"cannot initialize windowing system"};
  glfwMakeContextCurrent(win->win);
  glfwSwapInterval(1);  // Enable vsync

  // set user data
  glfwSetWindowUserPointer(win->win, win);

  // set callbacks
  glfwSetWindowRefreshCallback(win->win, [](GLFWwindow* glfw) {
    auto win = (gui_window*)glfwGetWindowUserPointer(glfw);
    draw_window(win);
  });
  glfwSetDropCallback(
      win->win, [](GLFWwindow* glfw, int num, const char** paths) {
        auto win = (gui_window*)glfwGetWindowUserPointer(glfw);
        if (win->drop_cb) {
          auto pathv = vector<string>();
          for (auto i = 0; i < num; i++) pathv.push_back(paths[i]);
          win->drop_cb(win, pathv, win->input);
        }
      });
  glfwSetKeyCallback(win->win,
      [](GLFWwindow* glfw, int key, int scancode, int action, int mods) {
        auto win   = (gui_window*)glfwGetWindowUserPointer(glfw);
        auto press = (action == GLFW_PRESS);
        update_button_from_input(win->input.key_buttons[key], press);
        if (win->key_cb) win->key_cb(win, key, (bool)action, win->input);
      });
  glfwSetCharCallback(win->win, [](GLFWwindow* glfw, unsigned int key) {
    auto win = (gui_window*)glfwGetWindowUserPointer(glfw);
    if (win->char_cb) win->char_cb(win, key, win->input);
  });
  glfwSetMouseButtonCallback(
      win->win, [](GLFWwindow* glfw, int button, int action, int mods) {
        auto win   = (gui_window*)glfwGetWindowUserPointer(glfw);
        auto press = (action == GLFW_PRESS);
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
          update_button_from_input(win->input.mouse_left, press);
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
          update_button_from_input(win->input.mouse_right, press);
        } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
          update_button_from_input(win->input.mouse_middle, press);
        }
        if (win->click_cb)
          win->click_cb(
              win, button == GLFW_MOUSE_BUTTON_LEFT, (bool)action, win->input);
      });
  glfwSetScrollCallback(
      win->win, [](GLFWwindow* glfw, double xoffset, double yoffset) {
        auto win          = (gui_window*)glfwGetWindowUserPointer(glfw);
        win->input.scroll = {(float)xoffset, (float)yoffset};
        if (win->scroll_cb) win->scroll_cb(win, (float)yoffset, win->input);
      });
  glfwSetWindowSizeCallback(
      win->win, [](GLFWwindow* glfw, int width, int height) {
        auto win = (gui_window*)glfwGetWindowUserPointer(glfw);
        glfwGetWindowSize(
            win->win, &win->input.window_size.x, &win->input.window_size.y);
        if (win->widgets_width) win->input.window_size.x -= win->widgets_width;
        glfwGetFramebufferSize(win->win, &win->input.framebuffer_viewport.z,
            &win->input.framebuffer_viewport.w);
        win->input.framebuffer_viewport.x = 0;
        win->input.framebuffer_viewport.y = 0;
        if (win->widgets_width) {
          auto win_size = zero2i;
          glfwGetWindowSize(win->win, &win_size.x, &win_size.y);
          auto offset = (int)(win->widgets_width *
                              (float)win->input.framebuffer_viewport.z /
                              win_size.x);
          win->input.framebuffer_viewport.z -= offset;
          if (win->widgets_left) win->input.framebuffer_viewport.x += offset;
        }
      });

  // init gl extensions
  if (!gladLoadGL())
    throw std::runtime_error{"cannot initialize OpenGL extensions"};

  // widgets
  //   if (widgets) {
  //     ImGui::CreateContext();
  //     ImGui::GetIO().IniFilename       = nullptr;
  //     ImGui::GetStyle().WindowRounding = 0;
  //     ImGui_ImplGlfw_InitForOpenGL(win->win, true);
  // #ifndef __APPLE__
  //     ImGui_ImplOpenGL3_Init();
  // #else
  //     ImGui_ImplOpenGL3_Init("#version 330");
  // #endif
  //     ImGui::StyleColorsDark();
  //     win->widgets_width = widgets_width;
  //     win->widgets_left  = widgets_left;
  //   }
}

void clear_window(gui_window* win) {
  glfwDestroyWindow(win->win);
  glfwTerminate();
  win->win = nullptr;
}

// Run loop
void run_ui(gui_window* win) {
  // init
  if (win->init_cb) win->init_cb(win, win->input);

  // loop
  while (!glfwWindowShouldClose(win->win)) {
    // update input
    win->input.mouse_last = win->input.mouse_pos;
    auto mouse_posx = 0.0, mouse_posy = 0.0;
    glfwGetCursorPos(win->win, &mouse_posx, &mouse_posy);
    win->input.mouse_pos = vec2f{(float)mouse_posx, (float)mouse_posy};
    if (win->widgets_width && win->widgets_left)
      win->input.mouse_pos.x -= win->widgets_width;
    //    win->input.mouse_left = glfwGetMouseButton(
    //                                win->win, GLFW_MOUSE_BUTTON_LEFT) ==
    //                                GLFW_PRESS;
    //    win->input.mouse_right =
    //        glfwGetMouseButton(win->win, GLFW_MOUSE_BUTTON_RIGHT) ==
    //        GLFW_PRESS;
    win->input.modifier_alt =
        glfwGetKey(win->win, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
        glfwGetKey(win->win, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
    win->input.modifier_shift =
        glfwGetKey(win->win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(win->win, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    win->input.modifier_ctrl =
        glfwGetKey(win->win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(win->win, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    glfwGetWindowSize(
        win->win, &win->input.window_size.x, &win->input.window_size.y);
    if (win->widgets_width) win->input.window_size.x -= win->widgets_width;
    glfwGetFramebufferSize(win->win, &win->input.framebuffer_viewport.z,
        &win->input.framebuffer_viewport.w);
    win->input.framebuffer_viewport.x = 0;
    win->input.framebuffer_viewport.y = 0;
    if (win->widgets_width) {
      auto win_size = zero2i;
      glfwGetWindowSize(win->win, &win_size.x, &win_size.y);
      auto offset = (int)(win->widgets_width *
                          (float)win->input.framebuffer_viewport.z /
                          win_size.x);
      win->input.framebuffer_viewport.z -= offset;
      if (win->widgets_left) win->input.framebuffer_viewport.x += offset;
    }
    if (win->widgets_width) {
      // TODO(giacomo): restore this
      // auto io                   = &ImGui::GetIO();
      // win->input.widgets_active = io->WantTextInput || io->WantCaptureMouse
      // ||
      //                             io->WantCaptureKeyboard;
    }

    // time
    win->input.clock_last = win->input.clock_now;
    win->input.clock_now =
        std::chrono::high_resolution_clock::now().time_since_epoch().count();
    win->input.time_now = (double)win->input.clock_now / 1000000000.0;
    win->input.time_delta =
        (double)(win->input.clock_now - win->input.clock_last) / 1000000000.0;

    // update ui
    if (win->uiupdate_cb && !win->input.widgets_active)
      win->uiupdate_cb(win, win->input);

    // update
    if (win->update_cb) win->update_cb(win, win->input);

    // draw
    draw_window(win);

    // event hadling
    glfwPollEvents();
  }

  // clear
  if (win->clear_cb) win->clear_cb(win, win->input);
}

void set_init_callback(gui_window* win, init_callback cb) { win->init_cb = cb; }
void set_clear_callback(gui_window* win, clear_callback cb) {
  win->clear_cb = cb;
}
void set_draw_callback(gui_window* win, draw_callback cb) { win->draw_cb = cb; }
void set_widgets_callback(gui_window* win, widgets_callback cb) {
  win->widgets_cb = cb;
}
void set_drop_callback(gui_window* win, drop_callback drop_cb) {
  win->drop_cb = drop_cb;
}
void set_key_callback(gui_window* win, key_callback cb) { win->key_cb = cb; }
void set_char_callback(gui_window* win, char_callback cb) { win->char_cb = cb; }
void set_click_callback(gui_window* win, click_callback cb) {
  win->click_cb = cb;
}
void set_scroll_callback(gui_window* win, scroll_callback cb) {
  win->scroll_cb = cb;
}
void set_uiupdate_callback(gui_window* win, uiupdate_callback cb) {
  win->uiupdate_cb = cb;
}
void set_update_callback(gui_window* win, update_callback cb) {
  win->update_cb = cb;
}

void set_close(gui_window* win, bool close) {
  glfwSetWindowShouldClose(win->win, close ? GLFW_TRUE : GLFW_FALSE);
}

}  // namespace yocto
