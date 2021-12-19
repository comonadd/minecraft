#include <stdio.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fmt/core.h>

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
#include "shaders.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "util.hpp"
#include "world.hpp"

const int WIDTH = 1600;
const int HEIGHT = 960;

struct Entity {
  GLuint vbo;
  GLuint colorbuffer;
  GLuint texture;
};

struct {
  GLFWwindow *window;
  int width = WIDTH;
  int height = HEIGHT;

  bool firstMouse = true;

  float yaw = -90.0f;
  float pitch = 0.0f;
  float sensitivity = 0.1f;
  float lastX = (float)WIDTH / 2.0f;
  float lastY = (float)HEIGHT / 2.0f;

  glm::vec3 camera_pos = glm::vec3(4.0, 50.0, 50.0);
  glm::vec3 camera_front = glm::vec3(0.0, 0.0, -1.0f);
  float speed = 32.0f;
  glm::vec3 camera_up = glm::vec3(0.0, 0.1, 0.0);

  GLuint shader;

  std::vector<Entity> entities;

  float delta_time = 0.0f;  // Time between current frame and last frame
  float last_frame = 0.0f;  // Time of last frame

  World world;

  // TODO: Update on resize
  // TODO: Also need to change the clipping distance based on the chunk radius
  glm::mat4 Projection = glm::perspective(
      glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 800.0f);
} state;

struct Texture {
  GLuint texture;
};

Texture load_texture(std::string const &path) {
  int width, height, nrChannels;
  unsigned char *data =
      stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

  unsigned int texture;
  glGenTextures(1, &texture);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  if (data) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    std::cout << "Failed to load texture" << std::endl;
  }
  stbi_image_free(data);
  Texture res;
  res.texture = texture;
  return res;
}

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

void init_graphics() {
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

  glfwSetInputMode(state.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPosCallback(state.window, mouse_callback);
}

void quit() { glfwSetWindowShouldClose(state.window, GL_TRUE); }

void process_keys() {
  auto *window = state.window;

  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
      glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
    quit();
  }

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

void render() {
  auto block_attrib = state.world.block_attrib;
  GLsizei stride = sizeof(GLfloat) * 8;
  glUseProgram(block_attrib.shader);
  for (auto &chunk : state.world.chunks) {
    glBindVertexArray(chunk->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, chunk->buffer);

    // MV
    glm::mat4 View =
        glm::lookAt(state.camera_pos, state.camera_pos + state.camera_front,
                    state.camera_up);
    glm::mat4 mvp = state.Projection * View;
    glUniformMatrix4fv(block_attrib.MVP, 1, GL_FALSE, &mvp[0][0]);

    glEnableVertexAttribArray(block_attrib.position);
    //    glEnableVertexAttribArray(block_attrib.normal);
    glEnableVertexAttribArray(block_attrib.uv);

    glVertexAttribPointer(block_attrib.position, 3, GL_FLOAT, GL_FALSE, stride,
                          (void *)offsetof(VertexData, pos));
    //    glVertexAttribPointer(block_attrib.normal, 3, GL_FLOAT, GL_FALSE,
    //    stride,
    //                          (void *)offsetof(VertexData, normal));
    glVertexAttribPointer(block_attrib.uv, 2, GL_FLOAT, GL_FALSE, stride,
                          (void *)offsetof(VertexData, uv));

    // render the chunk mesh
    fmt::print("Rendering {} vertices! \n", chunk->mesh.size());
    glDrawArrays(GL_TRIANGLES, 0, chunk->mesh.size());

    glDisableVertexAttribArray(block_attrib.position);
    //    glDisableVertexAttribArray(block_attrib.normal);
    glDisableVertexAttribArray(block_attrib.uv);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }
}

void update() {
  float current_frame = glfwGetTime();
  state.delta_time = current_frame - state.last_frame;
  state.last_frame = current_frame;

  process_keys();

  load_chunks_around_player(state.world, state.camera_pos);
}

void setup_noise() {
  const siv::PerlinNoise perlin(state.world.seed);
  state.world.perlin = perlin;
  // state.world.simplex = SimplexNoise(1.0f, 0.4f, 2.0f, 0.5f);
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

int main() {
  init_graphics();
  setup_noise();
  auto basic_shader =
      load_shader("./shaders/basic_vs.glsl", "./shaders/basic_fs.glsl");
  state.shader = basic_shader;
  Texture texture = load_texture("images/texture.png");

  // During init, enable debug output
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, 0);

  Attrib block_attrib;
  block_attrib.shader = basic_shader;
  block_attrib.position = glGetAttribLocation(block_attrib.shader, "position");
  block_attrib.normal = glGetAttribLocation(block_attrib.shader, "normal");
  block_attrib.uv = glGetAttribLocation(block_attrib.shader, "texUV");
  block_attrib.MVP = glGetUniformLocation(basic_shader, "MVP");
  state.world.block_attrib = block_attrib;

  auto *window = state.window;

  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    update();
    render();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
