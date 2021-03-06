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
#include <cxxopts.hpp>
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
#include "camera.hpp"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/imgui.h"
#include "shaders.hpp"
#include "skybox.hpp"
#include "texture.hpp"
#include "util.hpp"
#include "world.hpp"

const int WIDTH = 1920;
const int HEIGHT = 1080;
constexpr auto DEFAULT_OUT_DIR = "./temp";

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

struct ChunkBorderVertex {
  vec3 pos;
};

using std::thread;

struct {
  // Main window state
  GLFWwindow *window = nullptr;
  int width = WIDTH;
  int height = HEIGHT;

  // Camera mouse state
  Camera camera{WIDTH, HEIGHT, 32.0f};

  // TODO: Update on resize
  // TODO: Also need to change the clipping distance based on the chunk radius
  glm::mat4 Projection = glm::perspective(
      glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 800.0f);

  // Player state
  WorldPos player_pos;

  World world;

  // Other
  float delta_time = 0.0f;  // Time between current frame and last frame
  float last_frame = 0.0f;  // Time of last frame
  int rendering_distance = 8;
  Mode mode = Mode::Playing;

  Texture minimap_tex;

  // skybox stuff
  GLuint sky_vao;
  GLuint sky_buffer;
  vector<SkyVertexData> sky_mesh = make_skybox_mesh();
  GLuint celestial_buffer;
  GLuint celestial_vao;
  vector<SkyVertexData> celestial_mesh = make_celestial_body_mesh();
  GLuint cloud_vao;
  GLuint cloud_buffer;
  vector<SkyVertexData> cloud_mesh;

  // sun/moon size
  float celestial_size = 2.5f;

  bool render_chunk_borders = false;
  GLuint chunk_borders_buffer;
  vector<ChunkBorderVertex> chunk_borders_mesh;
  GLuint chunk_borders_vao;

  // thread responsible for world gen
  thread *gen_thread = nullptr;
} state;

inline glm::vec3 get_block_pos_looking_at() {  //
  auto player_reach = 3.0f;
  return state.camera.camera_pos + state.camera.camera_front * player_reach;
}

void process_left_click() {
  auto new_block_place_pos = get_block_pos_looking_at();
  place_block_at(state.world, BlockType::Air, new_block_place_pos);
}

void disable_cursor() {
  glfwSetInputMode(state.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void enable_cursor() {
  glfwSetInputMode(state.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void process_right_click() {
  auto new_block_place_pos = get_block_pos_looking_at();
  place_block_at(state.world, BlockType::Dirt, new_block_place_pos);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
  // Disable in-game camera control with mouse if not playing
  if (state.mode != Mode::Playing) {
    return;
  }

  camera_process_movement(state.camera, xpos, ypos);
}

void quit() { glfwSetWindowShouldClose(state.window, GL_TRUE); }

void toggle_menu() {
  state.mode = state.mode == Mode::Menu ? Mode::Playing : Mode::Menu;
  if (state.mode == Mode::Menu) {
    enable_cursor();
  } else {
    disable_cursor();
  }
}

void process_keys() {
  if (state.mode != Mode::Playing) {
    // these keys are only for playing the game
    // do not process them if in menu mode
    return;
  }
  auto *window = state.window;

  // movement
  float dt = state.delta_time;
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    camera_move_forward(state.camera, dt);
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    camera_move_backward(state.camera, dt);
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    camera_move_left(state.camera, dt);
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    camera_move_right(state.camera, dt);
  }
}

void render_info_bar() {
  rusage u;
  int res = getrusage(RUSAGE_SELF, &u);
  if (res != 0) {
    // do something
  }
  ImGui::Begin("Info");
  ImGui::SetWindowSize({400.0f, 600.0f});
  ImGui::SameLine();
  ImGui::Text("Avg %.3f ms/frame (%.1f FPS)",
              1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
  ImGui::Text("Chunks loaded: %lu", state.world.loaded_chunks.size());
  ImGui::Text("X=%i, Y=%i, Z=%i", (int)round(state.camera.camera_pos.x),
              (int)round(state.camera.camera_pos.y),
              (int)round(state.camera.camera_pos.z));
  ImGui::Text("Biome: %s", get_biome_name_at(state.world, state.player_pos));
  float mem_usage_kb = (float)u.ru_maxrss;
  ImGui::Text("Total memory usage: %f MB", round(mem_usage_kb / 1024.0F));
  ImGui::Text("Render distance: %i", state.rendering_distance);
  int total_vertices = 0;
  for_all_chunks_in_rd(state.world, [&](Chunk &chunk) {
    total_vertices += chunk.mesh_size;  //
  });
  ImGui::Text("Vertices to render: %i", total_vertices);
  ImGui::Text("World time: %lu", state.world.time);
  ImGui::Text("Time of day (ticks): %i", state.world.time_of_day);
  int hours = floor((float)state.world.time_of_day / (float)ONE_HOUR);
  int minutes =
      round((float)state.world.time_of_day - (float)hours * (float)ONE_HOUR) /
      ONE_MINUTE;
  ImGui::Text("Time of day: %i:%i", hours, minutes);

  if (auto target = state.world.target_block) {
    auto block = *target;
    auto target_block_name = get_block_name(block.type);
    auto target_block_pos = *state.world.target_block_pos;
    ImGui::Text("Target: %s", target_block_name.c_str());
    ImGui::Text("Target pos: %i,%i,%i", target_block_pos.x, target_block_pos.y,
                target_block_pos.z);
  } else {
    ImGui::Text("Target: None");
  }

  ImGui::Text("Sun position: %f, %f, %f", state.world.sun_pos.x,
              state.world.sun_pos.y, state.world.sun_pos.z);
  ImGui::End();
}

inline u32 chunks_for_rdf(u32 rdf) {
  return 4 * rdf * rdf;  //
}

void change_rendering_distance(u32 new_rdf) {
  state.rendering_distance = new_rdf;
  auto n = chunks_for_rdf(new_rdf);
  state.world.chunks.resize(n);
  for (u32 i = 0; i < state.world.chunks.size(); ++i) {
    state.world.chunks[i] = nullptr;
  }
}

void render_menu() {
  ImGui::Begin("Game menu");

  // Rendering distance slider
  ImGui::Text("Rendering distance");
  float rdf = (float)state.rendering_distance;
  ImGui::SliderFloat("rendering_distance", &rdf, 0.0f, 32.0f);
  auto new_rdf = round(rdf);
  if (new_rdf != state.rendering_distance) {
    change_rendering_distance(new_rdf);
  }

  // fog density
  ImGui::Text("Fog density");
  ImGui::SliderFloat("fog_density", &state.world.fog_density, 0.0f, 0.02f);

  // fog gradient
  ImGui::Text("Fog gradient");
  ImGui::SliderFloat("fog_gradient", &state.world.fog_gradient, 0.0f, 16.0f);

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
          glm::lookAt(state.camera.camera_pos,
                      state.camera.camera_pos + state.camera.camera_front,
                      state.camera.camera_up);
      glUniformMatrix4fv(block_attrib.view, 1, GL_FALSE, &View[0][0]);
      glUniformMatrix4fv(block_attrib.projection, 1, GL_FALSE,
                         &state.Projection[0][0]);

      glUniform3fv(block_attrib.light_pos, 1, &state.world.sun_pos[0]);
      glUniform3fv(block_attrib.sky_color, 1, &state.world.sky_color[0]);
      if (state.world.fog_enabled) {
        glUniform1f(block_attrib.fog_density, state.world.fog_density);
        glUniform1f(block_attrib.fog_gradient, state.world.fog_gradient);
      } else {
        glUniform1f(block_attrib.fog_density, 0.0f);
        glUniform1f(block_attrib.fog_gradient, state.world.fog_gradient);
      }

      for_all_chunks_in_rd(state.world, [&](Chunk &chunk) {
        glBindVertexArray(chunk.vao);
        // render the chunk mesh
        glDrawArrays(GL_TRIANGLES, 0, chunk.mesh_size);
        glBindVertexArray(0);
      });
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
    auto sky_tex = texture_storage::get_texture("sky");
    glDepthMask(GL_FALSE);
    glUseProgram(shader->id);
    auto attr = shader->attr;
    // MV
    glm::mat4 View =
        glm::lookAt(state.camera.camera_pos,
                    state.camera.camera_pos + state.camera.camera_front,
                    state.camera.camera_up);
    glm::mat4 model = glm::mat4(1);
    model = glm::translate(model, state.camera.camera_pos);
    model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4 mvp = state.Projection * View * model;
    glUniformMatrix4fv(attr.MVP, 1, GL_FALSE, &mvp[0][0]);

    glUniform3fv(attr.sky_color, 1, &state.world.sky_color[0]);

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

void render_celestial() {
  if (auto shader = shader_storage::get_shader("celestial")) {
    auto sky_tex = state.world.is_day ? texture_storage::get_texture("sun")
                                      : texture_storage::get_texture("moon");
    glDepthMask(GL_FALSE);
    glUseProgram(shader->id);
    auto attr = shader->attr;
    // MV
    glm::mat4 View = glm::lookAt(state.world.origin,
                                 state.world.origin + state.camera.camera_front,
                                 state.camera.camera_up);
    glm::mat4 model = glm::mat4(1);
    model = glm::translate(model, glm::vec3(state.world.sun_pos));
    auto s = state.world.celestial_size;
    model = glm::scale(model, glm::vec3(s, s, s));
    auto VM = View * model;

    // always face the player
    auto size = state.celestial_size;
    VM[0][0] = VM[1][1] = VM[2][2] = size;
    VM[0][1] = VM[0][2] = VM[1][2] = 0.0f;
    VM[1][0] = VM[2][0] = VM[2][1] = 0.0f;
    VM = glm::rotate(VM, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 mvp = state.Projection * VM;
    glUniformMatrix4fv(attr.MVP, 1, GL_FALSE, &mvp[0][0]);
    glBindVertexArray(state.celestial_vao);
    glBindBuffer(GL_ARRAY_BUFFER, state.celestial_buffer);
    const auto tex = sky_tex->get();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex->texture);
    // render the chunk mesh
    glDrawArrays(GL_TRIANGLES, 0, state.celestial_mesh.size());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
    glDepthMask(GL_TRUE);
  }
}

float CLOUD_MOVEMENT_SPEED = 0.02f;

void render_clouds() {
  if (state.cloud_mesh.size() == 0) return;
  if (auto shader = shader_storage::get_shader("clouds")) {
    // glDepthMask(GL_FALSE);
    glUseProgram(shader->id);
    auto attr = shader->attr;
    // MV
    glm::mat4 View =
        glm::lookAt(state.camera.camera_pos,
                    state.camera.camera_pos + state.camera.camera_front,
                    state.camera.camera_up);
    glm::mat4 model = glm::mat4(1);
    model = glm::translate(
        model,
        glm::vec3((float)state.world.time * CLOUD_MOVEMENT_SPEED, 0.0f, 0.0f));
    // model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4 mvp = state.Projection * View * model;
    glUniformMatrix4fv(attr.MVP, 1, GL_FALSE, &mvp[0][0]);
    glBindVertexArray(state.cloud_vao);
    glBindBuffer(GL_ARRAY_BUFFER, state.cloud_buffer);
    // render the chunk mesh
    glDrawArrays(GL_TRIANGLES, 0, state.cloud_mesh.size());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
    // glDepthMask(GL_TRUE);
  }
}

void render_chunk_borders() {
  if (auto shader = shader_storage::get_shader("line")) {
    glUseProgram(shader->id);
    auto attr = shader->attr;
    // MV
    glm::mat4 View =
        glm::lookAt(state.camera.camera_pos,
                    state.camera.camera_pos + state.camera.camera_front,
                    state.camera.camera_up);
    glm::mat4 mvp = state.Projection * View;
    glBindVertexArray(state.chunk_borders_vao);
    glUniformMatrix4fv(attr.MVP, 1, GL_FALSE, &mvp[0][0]);
    glBindBuffer(GL_ARRAY_BUFFER, state.chunk_borders_buffer);
    glDrawArrays(GL_LINES, 0, state.chunk_borders_mesh.size());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
  }
}

void render() {
  render_sky();
  render_clouds();
  render_celestial();
  render_world();
  if (state.render_chunk_borders) {
    render_chunk_borders();
  }

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  switch (state.mode) {
    case Mode::Playing: {
      // render_minimap();
      render_info_bar();
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

void regenerate_clouds() {
  if (auto shader = shader_storage::get_shader("clouds")) {
    state.cloud_mesh = make_clouds_mesh(state.player_pos);
    auto attr = shader->attr;
    auto psize = sizeof(state.cloud_mesh[0]);
    auto stride = psize;
    glBindVertexArray(state.cloud_vao);
    glBindBuffer(GL_ARRAY_BUFFER, state.cloud_buffer);
    auto s = psize * state.cloud_mesh.size();
    glBufferData(GL_ARRAY_BUFFER, s, state.cloud_mesh.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(attr.position, 3, GL_FLOAT, GL_FALSE, stride,
                          (void *)offsetof(SkyVertexData, pos));
    glVertexAttribPointer(attr.color, 3, GL_FLOAT, GL_FALSE, stride,
                          (void *)offsetof(SkyVertexData, col));
    //    glVertexAttribPointer(attr.uv, 2, GL_FLOAT, GL_FALSE, stride,
    //                          (void *)offsetof(SkyVertexData, uv));
    glEnableVertexAttribArray(attr.position);
    glEnableVertexAttribArray(attr.color);
    //    glEnableVertexAttribArray(attr.uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }
}

void update() {
  // delta time
  float current_frame = glfwGetTime();
  state.delta_time = current_frame - state.last_frame;
  state.last_frame = current_frame;

  // integer player position (block coord)
  state.player_pos =
      glm::ivec3{state.camera.camera_pos.x, state.camera.camera_pos.y,
                 state.camera.camera_pos.z};

  // Only allocate a new buffer if none was allocated before
  auto shader = shader_storage::get_shader("block");
  for_all_chunks_in_rd(state.world, [&](Chunk &chunk) {
    auto &mesh = chunk.mesh;

    if (!chunk.is_dirty) return;

    auto block_attrib = shader->attr;
    //  if (chunk.vao == 0) {
    // Configure the VAO
    glGenVertexArrays(1, &chunk.vao);
    glGenBuffers(1, &chunk.buffer);
#ifdef VAO_ALLOCATION
    fmt::print("Allocating VAO={}\n", chunk.vao);
#endif
    //  }
    glBindVertexArray(chunk.vao);

    // Update chunk buffer mesh data
    glBindBuffer(GL_ARRAY_BUFFER, chunk.buffer);
    // so update the chunk buffer
    auto mesh_size = sizeof((*mesh)[0]) * mesh->size();
    auto *meshp = mesh->data();
    chunk.mesh_size = mesh->size();
    glBufferData(GL_ARRAY_BUFFER, mesh_size, meshp, GL_STATIC_DRAW);

    // Update VAO settings
    GLsizei stride = sizeof(VertexData);
    glVertexAttribPointer(block_attrib.position, 3, GL_FLOAT, GL_FALSE, stride,
                          (void *)offsetof(VertexData, pos));
    glVertexAttribPointer(block_attrib.normal, 3, GL_FLOAT, GL_FALSE, stride,
                          (void *)offsetof(VertexData, normal));
    glVertexAttribPointer(block_attrib.uv, 2, GL_FLOAT, GL_FALSE, stride,
                          (void *)offsetof(VertexData, uv));
    // glVertexAttribPointer(block_attrib.ao, 1, GL_FLOAT, GL_FALSE, stride,
    //                       (void *)offsetof(VertexData, ao));
    // glVertexAttribPointer(block_attrib.light, 1, GL_FLOAT, GL_FALSE, stride,
    //                       (void *)offsetof(VertexData, light));
    glEnableVertexAttribArray(block_attrib.position);
    glEnableVertexAttribArray(block_attrib.normal);
    glEnableVertexAttribArray(block_attrib.uv);
    // glEnableVertexAttribArray(block_attrib.ao);
    // glEnableVertexAttribArray(block_attrib.light);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
    glDeleteBuffers(1, &chunk.buffer);

    // we've regenerated the chunk mesh
    chunk.is_dirty = false;

    delete mesh;
  });

  // Unload unused chunks
  {
    std::lock_guard<std::mutex> guard(state.world.chunk_unload_mutex);
    while (state.world.chunks_to_unload.size() > 0) {
      auto [key, chunkp] = state.world.chunks_to_unload.back();
      unload_chunk(chunkp);
      state.world.loaded_chunks.erase(key);
      state.world.chunks_to_unload.pop_back();
    }
  }

  if (state.render_chunk_borders) {
    auto &mesh = state.chunk_borders_mesh;
    mesh.clear();
    for_all_chunks_in_rd(state.world, [&](Chunk &chunk) {
      mesh.push_back(ChunkBorderVertex{vec3(chunk.x, 0, chunk.y)});
      mesh.push_back(ChunkBorderVertex{vec3(chunk.x, CHUNK_HEIGHT, chunk.y)});
    });
    if (mesh.size() != 0) {
      if (auto shader = shader_storage::get_shader("line")) {
        auto attr = shader->attr;
        glBindVertexArray(state.chunk_borders_vao);
        auto psize = sizeof(state.chunk_borders_mesh[0]);
        auto s = psize * state.chunk_borders_mesh.size();
        auto stride = psize;
        glBindBuffer(GL_ARRAY_BUFFER, state.chunk_borders_buffer);
        glBufferData(GL_ARRAY_BUFFER, s, state.chunk_borders_mesh.data(),
                     GL_STATIC_DRAW);
        glVertexAttribPointer(attr.position, 3, GL_FLOAT, GL_FALSE, stride,
                              (void *)offsetof(SkyVertexData, pos));
        glEnableVertexAttribArray(attr.position);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
      }
    }
  }

  // clouds
  auto ct = state.world.time;
  static float last_time_gen_clouds = 0;
  i32 CLOUD_GEN_INTERVAL = TICKS_PER_SECOND * 2;
  i32 cdt = (ct - last_time_gen_clouds);
  if (cdt > CLOUD_GEN_INTERVAL) {
    regenerate_clouds();
    last_time_gen_clouds = ct;
  }

  // determine the target block
  state.world.target_block_pos = get_block_pos_looking_at();
  if (state.world.target_block_pos) {
    state.world.target_block =
        get_block_at_global_pos(state.world, *state.world.target_block_pos);
  }

  // Calculate the minimap image
  // calculate_minimap_tex(state.minimap_tex, state.world, state.player_pos,
  //                       state.rendering_distance);

  process_keys();
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

void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods) {
  // process keys pressed
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    process_left_click();
  } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
    process_right_click();
  }
}

Seed random_seed() {
  static std::random_device rd;
  static std::default_random_engine seed_eng;
  static std::mt19937::result_type seed =
      rd() ^
      ((std::mt19937::result_type)
           std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
               .count() +
       (std::mt19937::result_type)
           std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::high_resolution_clock::now().time_since_epoch())
               .count());
  static std::mt19937_64 seed_gen{seed};
  return seed_gen();
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
    quit();
  } else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    toggle_menu();
  } else if (key == GLFW_KEY_H && action == GLFW_PRESS) {
    // Generate world map picture
    world_dump_heights(state.world, DEFAULT_OUT_DIR);
  } else if (key == GLFW_KEY_R && action == GLFW_PRESS) {
    reset_chunks();
  } else if (key == GLFW_KEY_V && action == GLFW_PRESS) {
    Seed seed = random_seed();
    init_world(state.world, seed);
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
  glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
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
  disable_cursor();
#endif
  glfwSetCursorPosCallback(state.window, mouse_callback);
  glfwSetKeyCallback(state.window, key_callback);
  glfwSetMouseButtonCallback(state.window, mouse_button_callback);

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

void load_shaders() {
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
        block_attrib.model = glGetUniformLocation(shader.id, "model");
        block_attrib.view = glGetUniformLocation(shader.id, "view");
        block_attrib.projection = glGetUniformLocation(shader.id, "projection");
        block_attrib.sky_color = glGetUniformLocation(shader.id, "sky_color");
        block_attrib.light_pos = glGetUniformLocation(shader.id, "light_pos");
        block_attrib.fog_density =
            glGetUniformLocation(shader.id, "fog_density");
        block_attrib.fog_gradient =
            glGetUniformLocation(shader.id, "fog_gradient");

        shader.attr = block_attrib;
      });

  shader_storage::load_shader(
      "sky", "./shaders/sky_vs.glsl", "./shaders/sky_fs.glsl",
      [&](Shader &shader) -> void {
        Attrib attr;
        attr.position = glGetAttribLocation(shader.id, "position");
        attr.color = glGetAttribLocation(shader.id, "color");
        attr.uv = glGetAttribLocation(shader.id, "texUV");
        attr.MVP = glGetUniformLocation(shader.id, "mvp");
        attr.sky_color = glGetUniformLocation(shader.id, "sky_color");
        attr.blend_factor = glGetUniformLocation(shader.id, "blendFactor");
        shader.attr = attr;
        auto psize = sizeof(state.sky_mesh[0]);
        auto stride = psize;

        // sky
        {
          glGenVertexArrays(1, &state.sky_vao);
          glGenBuffers(1, &state.sky_buffer);
          glBindVertexArray(state.sky_vao);
          auto s = psize * state.sky_mesh.size();
          glBindBuffer(GL_ARRAY_BUFFER, state.sky_buffer);
          glBufferData(GL_ARRAY_BUFFER, s, state.sky_mesh.data(),
                       GL_STATIC_DRAW);
          glVertexAttribPointer(attr.position, 3, GL_FLOAT, GL_FALSE, stride,
                                (void *)offsetof(SkyVertexData, pos));
          glVertexAttribPointer(attr.color, 3, GL_FLOAT, GL_FALSE, stride,
                                (void *)offsetof(SkyVertexData, col));
          glVertexAttribPointer(attr.uv, 2, GL_FLOAT, GL_FALSE, stride,
                                (void *)offsetof(SkyVertexData, uv));
          glEnableVertexAttribArray(attr.position);
          glEnableVertexAttribArray(attr.color);
          glEnableVertexAttribArray(attr.uv);
          glBindBuffer(GL_ARRAY_BUFFER, 0);
          glBindVertexArray(0);
        }
      });

  shader_storage::load_shader(
      "celestial", "./shaders/celestial_vs.glsl", "./shaders/celestial_fs.glsl",
      [&](Shader &shader) -> void {
        Attrib attr;
        attr.position = glGetAttribLocation(shader.id, "position");
        attr.color = glGetAttribLocation(shader.id, "color");
        attr.uv = glGetAttribLocation(shader.id, "texUV");
        attr.MVP = glGetUniformLocation(shader.id, "mvp");
        shader.attr = attr;

        auto psize = sizeof(state.sky_mesh[0]);
        auto stride = psize;

        // celestial body
        {
          glGenVertexArrays(1, &state.celestial_vao);
          glGenBuffers(1, &state.celestial_buffer);
          glBindVertexArray(state.celestial_vao);
          glBindBuffer(GL_ARRAY_BUFFER, state.celestial_buffer);
          auto s = psize * state.sky_mesh.size();
          glBufferData(GL_ARRAY_BUFFER, s, state.celestial_mesh.data(),
                       GL_STATIC_DRAW);
          glVertexAttribPointer(attr.position, 3, GL_FLOAT, GL_FALSE, stride,
                                (void *)offsetof(SkyVertexData, pos));
          glVertexAttribPointer(attr.color, 3, GL_FLOAT, GL_FALSE, stride,
                                (void *)offsetof(SkyVertexData, col));
          glVertexAttribPointer(attr.uv, 2, GL_FLOAT, GL_FALSE, stride,
                                (void *)offsetof(SkyVertexData, uv));
          glEnableVertexAttribArray(attr.position);
          glEnableVertexAttribArray(attr.color);
          glEnableVertexAttribArray(attr.uv);
          glBindBuffer(GL_ARRAY_BUFFER, 0);
          glBindVertexArray(0);
        }
      });

  shader_storage::load_shader(
      "clouds", "./shaders/clouds_vs.glsl", "./shaders/clouds_fs.glsl",
      [&](Shader &shader) -> void {
        Attrib attr;
        attr.position = glGetAttribLocation(shader.id, "position");
        attr.color = glGetAttribLocation(shader.id, "color");
        attr.uv = glGetAttribLocation(shader.id, "texUV");
        attr.MVP = glGetUniformLocation(shader.id, "mvp");
        shader.attr = attr;
        {
          glGenVertexArrays(1, &state.cloud_vao);
          glGenBuffers(1, &state.cloud_buffer);
          glBindVertexArray(state.cloud_vao);
          glBindBuffer(GL_ARRAY_BUFFER, state.cloud_buffer);
        }
      });

  shader_storage::load_shader(
      "line", "./shaders/line_vs.glsl", "./shaders/line_fs.glsl",
      [&](Shader &shader) -> void {
        Attrib attr;
        attr.position = glGetAttribLocation(shader.id, "position");
        attr.MVP = glGetUniformLocation(shader.id, "mvp");
        shader.attr = attr;
        {
          glGenVertexArrays(1, &state.chunk_borders_vao);
          glGenBuffers(1, &state.chunk_borders_buffer);
          glBindVertexArray(state.chunk_borders_vao);
          glBindBuffer(GL_ARRAY_BUFFER, state.chunk_borders_buffer);
        }
      });
}

void load_textures() {
  texture_storage::init();
  texture_storage::load_texture("block", "images/texture.png");
  texture_storage::load_texture("sky", "images/sky.png");
  texture_storage::load_texture("sun", "images/sun.png");
  texture_storage::load_texture("moon", "images/moon.png");
}

Seed DEFAULT_SEED = 2873947234821;

int main(int argc, char **argv) {
  cxxopts::Options options("Minecraft",
                           "Procedurally generated voxel world simulation");
  options.add_options()  //
      ("g,gen", "Generate worldgen images and exit",
       cxxopts::value<bool>())  //
      ("o,gen-out", "Worldgen output directory",
       cxxopts::value<string>()->default_value(DEFAULT_OUT_DIR))  //
      ("s,seed", "Worldgen seed",
       cxxopts::value<u64>()->default_value(std::to_string(DEFAULT_SEED)))  //
      ;

  init_graphics();

  // load resources
  load_textures();

  load_shaders();

  // During init, enable debug output
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, 0);

  auto *window = state.window;

  auto parsed_opts = options.parse(argc, argv);
  auto seed = parsed_opts["seed"].as<u64>();

  fmt::print("Using seed {}\n", seed);
  init_world(state.world, seed);

  if (parsed_opts["gen"].as<bool>()) {
    fmt::print("Generating the worldgen maps with seed={}...\n", seed);
    auto out_dir_given = parsed_opts["gen-out"].as<string>();
    auto out_dir = out_dir_given != "" ? out_dir_given : DEFAULT_OUT_DIR;
    auto start = std::chrono::high_resolution_clock::now();
    world_dump_heights(state.world, out_dir);
    auto now = std::chrono::high_resolution_clock::now();
    auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
    auto ms = milliseconds.count();
    fmt::print("Took {} ms!\n", ms);
    return 0;
  }

  glfwShowWindow(state.window);

  // initial resize
  change_rendering_distance(state.rendering_distance);

  if (state.mode == Mode::Playing) {
    state.gen_thread = new thread{[&]() -> void {
      while (!glfwWindowShouldClose(window)) {
        world_update(state.world, state.delta_time, state.player_pos,
                     state.rendering_distance);
        sleep(1);
      }
    }};
  }

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
