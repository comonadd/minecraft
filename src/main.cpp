#include <stdio.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <gperftools/heap-profiler.h>
#include <gperftools/profiler.h>
#include <gperftools/tcmalloc.h>
#include <sys/resource.h>
#include <sys/time.h>

#include <array>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <set>
#include <streambuf>
#include <thread>
#include <utility>
#include <vector>

#include "PerlinNoise/PerlinNoise.hpp"
#include "SimplexNoise/src/SimplexNoise.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/imgui.h"
#include "shaders.hpp"
#include "texture.hpp"
#include "util.hpp"
#include "world.hpp"

const int WIDTH = 1600;
const int HEIGHT = 960;

struct Entity {
  GLuint vbo;
  GLuint colorbuffer;
  GLuint texture;
};

// Determines what to show
enum class Mode {
  Playing = 0,
  Menu = 1,
};

struct SkyVertexData {
  glm::vec3 pos;
  glm::vec2 uv;
};

vector<SkyVertexData> make_skybox_mesh() {
  static const float positions[6][4][3] = {
      {{-1, -1, -1}, {-1, -1, +1}, {-1, +1, -1}, {-1, +1, +1}},
      {{+1, -1, -1}, {+1, -1, +1}, {+1, +1, -1}, {+1, +1, +1}},
      {{-1, +1, -1}, {-1, +1, +1}, {+1, +1, -1}, {+1, +1, +1}},
      {{-1, -1, -1}, {-1, -1, +1}, {+1, -1, -1}, {+1, -1, +1}},
      {{-1, -1, -1}, {-1, +1, -1}, {+1, -1, -1}, {+1, +1, -1}},
      {{-1, -1, +1}, {-1, +1, +1}, {+1, -1, +1}, {+1, +1, +1}}};
  static const float uvs[6][4][2] = {
      {{0, 0}, {1, 0}, {0, 1}, {1, 1}}, {{1, 0}, {0, 0}, {1, 1}, {0, 1}},
      {{0, 1}, {0, 0}, {1, 1}, {1, 0}}, {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
      {{0, 0}, {0, 1}, {1, 0}, {1, 1}}, {{1, 0}, {1, 1}, {0, 0}, {0, 1}}};
  static const float indices[6][6] = {{0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3},
                                      {0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3},
                                      {0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3}};
  vector<SkyVertexData> res;
  for (int i = 0; i < 6; ++i) {
    for (int v = 0; v < 6; ++v) {
      SkyVertexData p;
      int j = indices[i][v];
      p.pos.x = positions[i][j][0];
      p.pos.y = positions[i][j][1];
      p.pos.z = positions[i][j][2];
      p.uv.x = uvs[i][j][0];
      p.uv.y = uvs[i][j][1];
      res.push_back(p);
    }
  }
  return res;
}

struct {
  // Main window state
  GLFWwindow *window = nullptr;
  int width = WIDTH;
  int height = HEIGHT;

  // Camera mouse state
  bool firstMouse = true;
  float yaw = -90.0f;
  float pitch = 0.0f;
  float sensitivity = 0.1f;
  float lastX = (float)WIDTH / 2.0f;
  float lastY = (float)HEIGHT / 2.0f;
  glm::vec3 camera_pos = glm::vec3(4.0, 50.0, 50.0);
  glm::vec3 camera_front = glm::vec3(0.0, 0.0, -1.0f);
  glm::vec3 camera_up = glm::vec3(0.0, 0.1, 0.0);
  // TODO: Update on resize
  // TODO: Also need to change the clipping distance based on the chunk radius
  glm::mat4 Projection = glm::perspective(
      glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 800.0f);

  // Player state
  float speed = 16.0f;
  WorldPos player_pos;

  // Other
  float delta_time = 0.0f;  // Time between current frame and last frame
  float last_frame = 0.0f;  // Time of last frame
  World world;
  int rendering_distance = 8;
  Mode mode = Mode::Playing;

  Texture minimap_tex;

  GLuint sky_vao;
  GLuint sky_buffer;
  vector<SkyVertexData> sky_mesh = make_skybox_mesh();
} state;

inline glm::vec3 get_block_pos_looking_at() { return state.camera_front; }

void process_left_click() {
  auto new_block_place_pos = get_block_pos_looking_at();
  place_block_at(state.world, BlockType::Air, new_block_place_pos);
}

void process_right_click() {
  // auto [new_block_place_pos, chunk] = get_block_pos_looking_at();
  // auto block_at_target = chunk_get_block_at_global(chunk,
  // new_block_place_pos); bool can_place =
  // can_place_at_block(block_at_target.type); if (!can_place) return;
  // TODO: Currently, this code overrides the block we're looking at.
  // But we need to place a new block right next to the block side we're facing.
  // place_block_at(BlockType::Dirt, new_block_place_pos);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
  if (state.firstMouse) {
    state.lastX = xpos;
    state.lastY = ypos;
    state.firstMouse = false;
  }

  float xoffset = xpos - state.lastX;
  float yoffset =
      state.lastY -
      ypos;  // reversed since y-coordinates range from bottom to top
  state.lastX = xpos;
  state.lastY = ypos;
  xoffset *= state.sensitivity;
  yoffset *= state.sensitivity;

  state.yaw += xoffset;
  state.pitch += yoffset;

  if (state.pitch > 89.0f) state.pitch = 89.0f;
  if (state.pitch < -89.0f) state.pitch = -89.0f;

  // calculate camera direction
  glm::vec3 direction;
  direction.x = cos(glm::radians(state.yaw)) * cos(glm::radians(state.pitch));
  direction.y = sin(glm::radians(state.pitch));
  direction.z = sin(glm::radians(state.yaw)) * cos(glm::radians(state.pitch));
  state.camera_front = glm::normalize(direction);

  // process keys pressed
  return;
  bool left_click = false;
  bool right_click = true;
  if (left_click) {
    process_left_click();
  } else if (right_click) {
    process_right_click();
  }
}

void quit() { glfwSetWindowShouldClose(state.window, GL_TRUE); }

void toggle_menu() {
  state.mode = state.mode == Mode::Menu ? Mode::Playing : Mode::Menu;
}

void process_keys() {
  auto *window = state.window;

  // movement
  float mov_vel = state.speed * state.delta_time;
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    state.camera_pos += mov_vel * state.camera_front;
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    state.camera_pos -= mov_vel * state.camera_front;
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    state.camera_pos -=
        glm::normalize(glm::cross(state.camera_front, state.camera_up)) *
        mov_vel;
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    state.camera_pos +=
        glm::normalize(glm::cross(state.camera_front, state.camera_up)) *
        mov_vel;
}

void render_info_bar() {
  rusage u;
  int res = getrusage(RUSAGE_SELF, &u);
  if (res != 0) {
    // do something
  }
  ImGui::Begin("Hello, world!");
  ImGui::Text("Information");
  ImGui::SameLine();
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
              1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
  ImGui::Text("Chunks loaded: %lu", state.world.loaded_chunks.size());
  ImGui::Text("X=%i, Y=%i, Z=%i", (int)round(state.camera_pos.x),
              (int)round(state.camera_pos.y), (int)round(state.camera_pos.z));
  ImGui::Text("Biome: %s", get_biome_name_at(state.world, state.player_pos));
  float mem_usage_kb = (float)u.ru_maxrss;
  ImGui::Text("Total memory usage: %f MB", round(mem_usage_kb / 1024.0F));
  ImGui::Text("Render distance: %i", state.rendering_distance);
  int total_vertices = 0;
  for (auto &chunk : state.world.chunks) {
    total_vertices += chunk->mesh_size;
  }
  ImGui::Text("Vertices to render: %i", total_vertices);
  ImGui::Text("World time: %i", state.world.time);
  ImGui::Text("Time of day: %i", state.world.time_of_day);
  ImGui::Text("Sun position: %f, %f, %f", state.world.sun_pos.x,
              state.world.sun_pos.y, state.world.sun_pos.z);
  ImGui::End();
}

void render_menu() {
  ImGui::Begin("Game menu");
  ImGui::Text("Game menu");
  // Rendering distance slider
  float rdf = (float)state.rendering_distance;
  ImGui::SliderFloat("float", &rdf, 0.0f, 32.0f);
  state.rendering_distance = round(rdf);
  ImGui::End();
}

void render_world() {
  if (auto block_shader = shader_storage::get_shader("block")) {
    glUseProgram(block_shader->id);
    if (auto tex = texture_storage::get_texture("block")) {
      auto t = tex->get();
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, t->texture);
      auto block_attrib = block_shader->attr;
      // MV
      glm::mat4 View =
          glm::lookAt(state.camera_pos, state.camera_pos + state.camera_front,
                      state.camera_up);
      glm::mat4 mvp = state.Projection * View;
      glUniformMatrix4fv(block_attrib.MVP, 1, GL_FALSE, &mvp[0][0]);
      glUniform3fv(block_attrib.light_pos, 1, &state.world.sun_pos[0]);
      for (auto &chunk : state.world.chunks) {
        glBindVertexArray(chunk->vao);
        // render the chunk mesh
        glDrawArrays(GL_TRIANGLES, 0, chunk->mesh_size);
        glBindVertexArray(0);
      }
      glUseProgram(0);
    }
  }
}

void render_minimap() {
  // if (auto shader = shader_storage::get_shader("minimap")) {
  //   glBindTexture(GL_TEXTURE_2D, state.minimap_tex.texture);
  //   glUseProgram(shader->id);
  //   glDrawArrays(GL_TRIANGLES, 0, 6);
  //   glUseProgram(0);
  // }
}

void render_sky() {
  if (auto shader = shader_storage::get_shader("sky")) {
    if (auto sky_tex = texture_storage::get_texture("sky")) {
      glDepthMask(GL_FALSE);
      glUseProgram(shader->id);
      auto attr = shader->attr;
      // MV
      glm::mat4 View =
          glm::lookAt(state.camera_pos, state.camera_pos + state.camera_front,
                      state.camera_up);
      glm::mat4 model = glm::mat4(1);
      model = glm::translate(model, state.camera_pos);
      model = glm::scale(model, glm::vec3(10.0f, 10.0f, 10.0f));
      glm::mat4 mvp = state.Projection * View * model;
      glUniformMatrix4fv(attr.MVP, 1, GL_FALSE, &mvp[0][0]);
      glBindVertexArray(state.sky_vao);
      glBindBuffer(GL_ARRAY_BUFFER, state.sky_buffer);
      const auto tex = sky_tex->get();
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, tex->texture);
      // render the chunk mesh
      glDrawArrays(GL_TRIANGLES, 0, state.sky_mesh.size());
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindVertexArray(0);
      glUseProgram(0);
      glDepthMask(GL_TRUE);
    }
  }
}

void render() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  switch (state.mode) {
    case Mode::Playing: {
      // render_info_bar();
      render_sky();
      render_world();
      // render_minimap();
    } break;
    case Mode::Menu: {
      render_menu();
    } break;
  }

  ImGui::Render();
  int display_w, display_h;
  glfwGetFramebufferSize(state.window, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void update() {
  float current_frame = glfwGetTime();
  state.delta_time = current_frame - state.last_frame;
  state.last_frame = current_frame;

  // update time
  int ticks_passed = state.delta_time * TICKS_PER_SECOND;
  state.world.time += ticks_passed;

  if (state.world.time_of_day >= DAY_DURATION) {
    state.world.time_of_day = 0;
  } else {
    state.world.time_of_day += ticks_passed;
  }

  static float __prev_d = 0;

  // calculate sun position
  float sun_degrees =
      360.0f * ((float)state.world.time_of_day / (float)DAY_DURATION);
  // fmt::print("Sun degrees: {}\n", sun_degrees);
  // fmt::print("Sun degrees delta: {}\n", sun_degrees - __prev_d);
  __prev_d = sun_degrees;
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::rotate(model, glm::radians(sun_degrees),
                      glm::vec3(0.0f, 0.0f, 1.0f));
  state.world.sun_pos = model * glm::vec4(0.0, 100.0f, 0.0f, 0.0f);

  process_keys();

  state.player_pos =
      glm::ivec3{state.camera_pos.x, state.camera_pos.y, state.camera_pos.z};

  // Calculate the minimap image
  // calculate_minimap_tex(state.minimap_tex, state.world, state.player_pos,
  //                       state.rendering_distance);

  if (state.mode == Mode::Playing) {
#ifdef MEMORY_DEBUG
    HeapProfilerStart("output_inside.prof");
#endif
    load_chunks_around_player(state.world, state.camera_pos,
                              state.rendering_distance);
    unload_distant_chunks(state.world, state.player_pos,
                          state.rendering_distance);
#ifdef MEMORY_DEBUG
    HeapProfilerStop();
#endif
  }
}

void setup_noise() {
  const siv::PerlinNoise perlin(state.world.seed);
  state.world.perlin = perlin;
}

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
                                GLenum severity, GLsizei length,
                                const GLchar *message, const void *userParam) {
  // buffer memory stuff warning
  if (type == 0x8251) return;
  fprintf(stderr,
          "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
          (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity,
          message);
}

void reset_chunks() {
  for (auto &p : state.world.loaded_chunks) {
    // deallocate buffers
    auto &chunk = p.second;
    unload_chunk(chunk);
    delete chunk;
  }
  state.world.chunks.clear();
  state.world.loaded_chunks.clear();
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
    quit();
  } else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    toggle_menu();
  } else if (key == GLFW_KEY_H && action == GLFW_PRESS) {
    // Generate world map picture
    world_dump_heights(state.world);
  } else if (key == GLFW_KEY_R && action == GLFW_PRESS) {
    reset_chunks();
  }
}

void init_graphics() {
  const char *glsl_version = "#version 150";
  glfwInit();

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  state.window = glfwCreateWindow(state.width, state.height, "OpenGL", nullptr,
                                  nullptr);  // Windowed
  glfwMakeContextCurrent(state.window);

  glewExperimental = GL_TRUE;
  glewInit();

  // Enable depth test
  glEnable(GL_DEPTH_TEST);
  // Accept fragment if it closer to the camera than the former one
  glDepthFunc(GL_LESS);

#ifdef DISABLE_CURSOR
  glfwSetInputMode(state.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
#endif
  glfwSetCursorPosCallback(state.window, mouse_callback);
  glfwSetKeyCallback(state.window, key_callback);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsClassic();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(state.window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);
}

int main() {
  init_graphics();
  setup_noise();
  shader_storage::load_shader(
      "block", "./shaders/basic_vs.glsl", "./shaders/basic_fs.glsl",
      [&](Shader &shader) -> void {
        Attrib block_attrib;
        block_attrib.position = glGetAttribLocation(shader.id, "position");
        block_attrib.normal = glGetAttribLocation(shader.id, "normal");
        block_attrib.uv = glGetAttribLocation(shader.id, "texUV");
        block_attrib.ao = glGetAttribLocation(shader.id, "ao");
        block_attrib.light = glGetAttribLocation(shader.id, "light");
        block_attrib.MVP = glGetUniformLocation(shader.id, "MVP");
        block_attrib.light_pos = glGetUniformLocation(shader.id, "light_pos");
        shader.attr = block_attrib;
      });
  texture_storage::load_texture("block", "images/texture.png");

  texture_storage::load_texture("sky", "images/sky.png");
  shader_storage::load_shader(
      "sky", "./shaders/sky_vs.glsl", "./shaders/sky_fs.glsl",
      [&](Shader &shader) -> void {
        Attrib attr;
        attr.position = glGetAttribLocation(shader.id, "position");
        attr.MVP = glGetUniformLocation(shader.id, "mvp");
        attr.uv = glGetAttribLocation(shader.id, "texUV");
        shader.attr = attr;
        auto psize = sizeof(state.sky_mesh[0]);
        auto stride = psize;
        glGenVertexArrays(1, &state.sky_vao);
        glGenBuffers(1, &state.sky_buffer);
        glBindVertexArray(state.sky_vao);
        glBindBuffer(GL_ARRAY_BUFFER, state.sky_buffer);
        auto s = psize * state.sky_mesh.size();
        glBufferData(GL_ARRAY_BUFFER, s, state.sky_mesh.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(attr.position, 3, GL_FLOAT, GL_FALSE, stride,
                              (void *)offsetof(SkyVertexData, pos));
        glVertexAttribPointer(attr.uv, 2, GL_FLOAT, GL_FALSE, stride,
                              (void *)offsetof(SkyVertexData, uv));
        glEnableVertexAttribArray(attr.position);
        glEnableVertexAttribArray(attr.uv);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glDeleteBuffers(1, &state.sky_buffer);
      });

  // During init, enable debug output
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, 0);

  auto *window = state.window;

  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    update();
    render();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(state.window);
  glfwTerminate();

  return 0;
}
