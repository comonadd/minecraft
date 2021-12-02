#include <stdio.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fmt/core.h>

#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <streambuf>
#include <thread>
#include <vector>

#include "PerlinNoise/PerlinNoise.hpp"
#include "shaders.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "util.hpp"

static const GLfloat vertices[] = {
    -1.0f, -1.0f, -1.0f,                       // triangle 1 : begin
    -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,   // triangle 1 : end
    1.0f,  1.0f,  -1.0f,                       // triangle 2 : begin
    -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,  // triangle 2 : end
    1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,
    1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f,
    -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f,
    -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  1.0f,
    1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,
    1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,
    1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  -1.0f, 1.0f};

static const GLfloat g_color_buffer_data[] = {
    1.0f, 1.f,  1.0f, 1.0f, 1.0f, 1.0f, 1.f,  1.f,  1.0f, 1.0f, 1.f,  1.0f,
    1.0f, 1.0f, 1.0f, 1.f,  1.f,  1.0f, 1.0f, 1.f,  1.0f, 1.0f, 1.0f, 1.0f,
    1.f,  1.f,  1.0f, 1.0f, 1.f,  1.0f, 1.0f, 1.0f, 1.0f, 1.f,  1.f,  1.0f,
    1.0f, 1.f,  1.0f, 1.0f, 1.0f, 1.0f, 1.f,  1.f,  1.0f, 1.0f, 1.f,  1.0f,
    1.0f, 1.0f, 1.0f, 1.f,  1.f,  1.0f, 1.0f, 1.f,  1.0f, 1.0f, 1.0f, 1.0f,
    1.f,  1.f,  1.0f, 1.0f, 1.f,  1.0f, 1.0f, 1.0f, 1.0f, 1.f,  1.f,  1.0f,
    1.0f, 1.f,  1.0f, 1.0f, 1.0f, 1.0f, 1.f,  1.f,  1.0f, 1.0f, 1.f,  1.0f,
    1.0f, 1.0f, 1.0f, 1.f,  1.f,  1.0f, 1.0f, 1.f,  1.0f, 1.0f, 1.0f, 1.0f,
    1.f,  1.f,  1.0f, 1.0f, 1.f,  1.0f, 1.0f, 1.0f, 1.0f, 1.f,  1.f,  1.0f};

static const GLfloat g_uv_buffer_data[] = {  //
    0.000059f, 1.0f - 0.000004f,             //
    0.000103f, 1.0f - 0.336048f,             //
    0.335973f, 1.0f - 0.335903f,             //
    1.000023f, 1.0f - 0.000013f,             //
    0.667979f, 1.0f - 0.335851f,             //
    0.999958f, 1.0f - 0.336064f,             //
    0.667979f, 1.0f - 0.335851f,             //
    0.336024f, 1.0f - 0.671877f,             //
    0.667969f, 1.0f - 0.671889f,             //
    1.000023f, 1.0f - 0.000013f,             //
    0.668104f, 1.0f - 0.000013f,             //
    0.667979f, 1.0f - 0.335851f,             //
    0.000059f, 1.0f - 0.000004f,             //
    0.335973f, 1.0f - 0.335903f,             //
    0.336098f, 1.0f - 0.000071f,             //
    0.667979f, 1.0f - 0.335851f,             //
    0.335973f, 1.0f - 0.335903f,             //
    0.336024f, 1.0f - 0.671877f,             //
    1.000004f, 1.0f - 0.671847f,             //
    0.999958f, 1.0f - 0.336064f,             //
    0.667979f, 1.0f - 0.335851f,             //
    0.668104f, 1.0f - 0.000013f,             //
    0.335973f, 1.0f - 0.335903f,             //
    0.667979f, 1.0f - 0.335851f,             //
    0.335973f, 1.0f - 0.335903f,             //
    0.668104f, 1.0f - 0.000013f,             //
    0.336098f, 1.0f - 0.000071f,             //
    0.000103f, 1.0f - 0.336048f,             //
    0.000004f, 1.0f - 0.671870f,             //
    0.336024f, 1.0f - 0.671877f,             //
    0.000103f, 1.0f - 0.336048f,             //
    0.336024f, 1.0f - 0.671877f,             //
    0.335973f, 1.0f - 0.335903f,             //
    0.667969f, 1.0f - 0.671889f,             //
    1.000004f, 1.0f - 0.671847f,             //
    0.667979f, 1.0f - 0.335851f};

const int WIDTH = 800;
const int HEIGHT = 600;

struct Entity {
  GLuint vbo;
  GLuint colorbuffer;
  GLuint texture;
};

enum class BlockType { Dirt, Grass, Stone, Water, Sand, Air, Unknown };

struct Block {
  BlockType type = BlockType::Air;
};

const int CHUNK_LENGTH = 16;
const int CHUNK_WIDTH = CHUNK_LENGTH;
const int CHUNK_HEIGHT = 16;

float BLOCK_WIDTH = 2.0f;
float BLOCK_LENGTH = BLOCK_WIDTH;
float BLOCK_HEIGHT = BLOCK_WIDTH;

struct Chunk {
  Block blocks[CHUNK_LENGTH][CHUNK_WIDTH][CHUNK_HEIGHT];
  int x;
  int y;
};

const int radius = 2;

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
  float speed = 8.0f;
  glm::vec3 camera_up = glm::vec3(0.0, 0.1, 0.0);

  GLuint shader;

  std::vector<Entity> entities;

  float delta_time = 0.0f;  // Time between current frame and last frame
  float last_frame = 0.0f;  // Time of last frame

  Chunk chunks[4 * (radius * radius)];
} state;

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
  auto basic_shader = state.shader;
  glUseProgram(basic_shader);

  // Camera matrix
  glm::mat4 View = glm::lookAt(
      state.camera_pos, state.camera_pos + state.camera_front, state.camera_up);

  // Generates a really hard-to-read matrix, but a normal, standard 4x4 matrix
  // nonetheless
  glm::mat4 Projection =
      glm::perspective(glm::radians(45.0f),
                       (float)state.width / (float)state.height, 0.1f, 100.0f);

  // render world
  auto block_entity = state.entities[0];
  for (auto &chunk : state.chunks) {
    for (int block_x = 0; block_x < CHUNK_WIDTH; ++block_x) {
      for (int block_y = 0; block_y < CHUNK_LENGTH; ++block_y) {
        for (int block_z = CHUNK_HEIGHT - 1; block_z >= 0; --block_z) {
          Block block = chunk.blocks[block_x][block_y][block_z];
          if (block.type == BlockType::Air) {
            continue;  // skip
          }

          auto entity = block_entity;

          // Model matrix : an identity matrix (model will be at the origin)
          glm::mat4 Model = glm::mat4(1.0f);
          float real_x = chunk.x + block_x * BLOCK_WIDTH;
          float real_y = chunk.y + block_y * BLOCK_LENGTH;
          float real_z = block_z * BLOCK_HEIGHT;
          glm::vec3 pos(real_x, real_z, real_y);  // swap y and z
          Model = glm::translate(Model, pos);

          // Our ModelViewProjection : multiplication of our 3 matrices
          glm::mat4 mvp =
              Projection * View *
              Model;  // Remember, matrix multiplication is the other way around

          // Get a handle for our "MVP" uniform
          // Only during the initialisation
          GLuint MatrixID = glGetUniformLocation(basic_shader, "MVP");

          // Send our transformation to the currently bound shader, in the "MVP"
          // uniform This is done in the main loop since each model will have a
          // different MVP matrix (At least for the M part)
          glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);

          // position
          GLint posAttrib = glGetAttribLocation(basic_shader, "position");
          glEnableVertexAttribArray(posAttrib);
          glBindBuffer(GL_ARRAY_BUFFER, entity.vbo);
          glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);

          GLint texture_attrib = glGetAttribLocation(basic_shader, "texUV");
          glEnableVertexAttribArray(texture_attrib);
          glVertexAttribPointer(texture_attrib, 2, GL_FLOAT, GL_FALSE,
                                8 * sizeof(float), (void *)(6 * sizeof(float)));
          glEnableVertexAttribArray(texture_attrib);

          glBindTexture(GL_TEXTURE_2D, entity.texture);

          // color
          // GLint colAttrib = glGetAttribLocation(basic_shader, "color");
          // glEnableVertexAttribArray(colAttrib);
          // glBindBuffer(GL_ARRAY_BUFFER, entity.colorbuffer);
          // glVertexAttribPointer(
          //     colAttrib,  // attribute. No particular reason for 1, but must
          //                 // match the layout in the shader.
          //     3,          // size
          //     GL_FLOAT,   // type
          //     GL_FALSE,   // normalized?
          //     0,          // stride
          //     (void *)0   // array buffer offset
          // );

          // draw
          glDrawArrays(GL_TRIANGLES, 0, 12 * 3);
        }
      }
    }
  }

  // for (auto &entity : state.entities) {
  //   GLint posAttrib = glGetAttribLocation(basic_shader, "position");
  //   glEnableVertexAttribArray(posAttrib);
  //   glBindBuffer(GL_ARRAY_BUFFER, entity.vbo);
  //   glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
  //
  //   // 2nd attribute buffer : colors
  //   GLint colAttrib = glGetAttribLocation(basic_shader, "color");
  //   glEnableVertexAttribArray(colAttrib);
  //   glBindBuffer(GL_ARRAY_BUFFER, entity.colorbuffer);
  //   glVertexAttribPointer(
  //       colAttrib,  // attribute. No particular reason for 1, but must
  //                   // match the layout in the shader.
  //       3,          // size
  //       GL_FLOAT,   // type
  //       GL_FALSE,   // normalized?
  //       0,          // stride
  //       (void *)0   // array buffer offset
  //   );
  //
  //   glDrawArrays(GL_TRIANGLES, 0, 12 * 3);
  // }
}

void update() {
  float current_frame = glfwGetTime();
  state.delta_time = current_frame - state.last_frame;
  state.last_frame = current_frame;

  process_keys();
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
}

int main() {
  init_graphics();
  glfwSetInputMode(state.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPosCallback(state.window, mouse_callback);

  int center_x = 0;
  int center_y = 0;

  int first_chunk_x =
      center_x - (center_x % CHUNK_WIDTH) - (CHUNK_WIDTH * radius);
  int first_chunk_y =
      center_y - (center_y % CHUNK_LENGTH) - (CHUNK_LENGTH * radius);

  int chunk_cols = radius * 2;
  int chunk_rows = radius * 2;
  int chunk_idx = 0;

  int seed = 3849534;
  const siv::PerlinNoise perlin(seed);
  double frequency = 16.0;
  const double fx = 100.0 / frequency;
  const double fy = 100.0 / frequency;

  for (int chunk_row = 0; chunk_row < chunk_rows; ++chunk_row) {
    int chunk_y = first_chunk_y + chunk_row * CHUNK_LENGTH;
    for (int chunk_col = 0; chunk_col < chunk_cols; ++chunk_col) {
      int chunk_x = first_chunk_x + chunk_col * CHUNK_WIDTH;

      Chunk chunk;
      chunk.x = chunk_x;
      chunk.y = chunk_y;

      // blocks
      for (int local_chunk_x = 0; local_chunk_x < CHUNK_WIDTH;
           ++local_chunk_x) {
        int global_x = chunk.x + local_chunk_x;

        for (int local_chunk_y = 0; local_chunk_y < CHUNK_LENGTH;
             ++local_chunk_y) {
          int global_y = chunk.y + local_chunk_y;

          int octaves = 8;
          auto noise = perlin.accumulatedOctaveNoise2D_0_1(
              (float)global_x / fx, (float)global_y / fy, octaves);

          int maxHeight = CHUNK_HEIGHT;
          int columnHeight = noise * maxHeight;

          fmt::print("Perlin noise at x={}, y={}: {}, determinedHeight={}\n",
                     global_x, global_y, noise, columnHeight);

          for (int height = columnHeight; height >= 0; --height) {
            Block block;
            chunk.blocks[local_chunk_y][local_chunk_x][height] = block;

            if (height > 20) {
              block.type = BlockType::Grass;
            } else if (height > 15) {
              block.type = BlockType::Dirt;
            } else if (height >= 0) {
              block.type = BlockType::Stone;
            }
          }
        }
      }

      state.chunks[chunk_idx++] = chunk;
    }
  }

  // Load the only shader we have
  auto basic_shader =
      load_shader("./shaders/basic_vs.glsl", "./shaders/basic_fs.glsl");
  state.shader = basic_shader;

  int width, height, nrChannels;
  unsigned char *data =
      stbi_load("images/dirt.jpg", &width, &height, &nrChannels, 0);

  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  // set the texture wrapping/filtering options (on the currently bound texture
  // object)
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  if (data) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    std::cout << "Failed to load texture" << std::endl;
  }
  stbi_image_free(data);

  Entity cube_entity;
  cube_entity.texture = texture;

  GLuint vbo;
  glGenBuffers(1, &vbo);  // Generate 1 buffer
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  cube_entity.vbo = vbo;

  GLuint colorbuffer;
  glGenBuffers(1, &colorbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data),
               g_color_buffer_data, GL_STATIC_DRAW);
  cube_entity.colorbuffer = colorbuffer;

  state.entities.push_back(cube_entity);

  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

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
