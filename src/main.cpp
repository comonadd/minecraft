#include <stdio.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <streambuf>
#include <thread>

#include "shaders.hpp"
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
    0.583f, 0.771f, 0.014f, 0.609f, 0.115f, 0.436f, 0.327f, 0.483f, 0.844f,
    0.822f, 0.569f, 0.201f, 0.435f, 0.602f, 0.223f, 0.310f, 0.747f, 0.185f,
    0.597f, 0.770f, 0.761f, 0.559f, 0.436f, 0.730f, 0.359f, 0.583f, 0.152f,
    0.483f, 0.596f, 0.789f, 0.559f, 0.861f, 0.639f, 0.195f, 0.548f, 0.859f,
    0.014f, 0.184f, 0.576f, 0.771f, 0.328f, 0.970f, 0.406f, 0.615f, 0.116f,
    0.676f, 0.977f, 0.133f, 0.971f, 0.572f, 0.833f, 0.140f, 0.616f, 0.489f,
    0.997f, 0.513f, 0.064f, 0.945f, 0.719f, 0.592f, 0.543f, 0.021f, 0.978f,
    0.279f, 0.317f, 0.505f, 0.167f, 0.620f, 0.077f, 0.347f, 0.857f, 0.137f,
    0.055f, 0.953f, 0.042f, 0.714f, 0.505f, 0.345f, 0.783f, 0.290f, 0.734f,
    0.722f, 0.645f, 0.174f, 0.302f, 0.455f, 0.848f, 0.225f, 0.587f, 0.040f,
    0.517f, 0.713f, 0.338f, 0.053f, 0.959f, 0.120f, 0.393f, 0.621f, 0.362f,
    0.673f, 0.211f, 0.457f, 0.820f, 0.883f, 0.371f, 0.982f, 0.099f, 0.879f};

const int WIDTH = 800;
const int HEIGHT = 600;

struct {
  GLFWwindow *window;
  int width = WIDTH;
  int height = HEIGHT;

  glm::vec3 camera_pos = glm::vec3(4, 3, 3);
  glm::vec3 camera_dir = glm::vec3(0, 0, 0);
  glm::vec3 speed_vec = glm::vec3(0.5, 0.5, 0.5);

  GLuint shader;
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
}

void quit() { glfwSetWindowShouldClose(state.window, GL_TRUE); }

void process_keys(float delta_time) {
  auto *window = state.window;

  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
      glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
    quit();
  }

  glm::vec3 mov_vec(0.0, 0.0, 0.0);

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    mov_vec.x = 1.0;
  } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    mov_vec.x = -1.0;
  }

  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    mov_vec.y = -1.0;
  } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    mov_vec.y = 1.0;
  }

  state.camera_pos += mov_vec * state.speed_vec * delta_time;
}

void render() {
  auto basic_shader = state.shader;
  glUseProgram(basic_shader);

  // Camera matrix
  glm::mat4 View = glm::lookAt(
      state.camera_pos, state.camera_dir,
      glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
  );

  // Generates a really hard-to-read matrix, but a normal, standard 4x4 matrix
  // nonetheless
  glm::mat4 Projection =
      glm::perspective(glm::radians(45.0f),
                       (float)state.width / (float)state.height, 0.1f, 100.0f);

  // Model matrix : an identity matrix (model will be at the origin)
  glm::mat4 Model = glm::mat4(1.0f);

  // Our ModelViewProjection : multiplication of our 3 matrices
  glm::mat4 mvp =
      Projection * View *
      Model;  // Remember, matrix multiplication is the other way around

  // Get a handle for our "MVP" uniform
  // Only during the initialisation
  GLuint MatrixID = glGetUniformLocation(basic_shader, "MVP");

  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // This is done in the main loop since each model will have a different MVP
  // matrix (At least for the M part)
  glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);

  glDrawArrays(GL_TRIANGLES, 0, 12 * 3);
}

void update(float delta_time) { process_keys(delta_time); }

int main() {
  init_graphics();

  // Load the only shader we have
  auto basic_shader =
      load_shader("./shaders/basic_vs.glsl", "./shaders/basic_fs.glsl");
  state.shader = basic_shader;

  GLuint vbo;
  glGenBuffers(1, &vbo);  // Generate 1 buffer
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  GLuint colorbuffer;
  glGenBuffers(1, &colorbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data),
               g_color_buffer_data, GL_STATIC_DRAW);

  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  GLint posAttrib = glGetAttribLocation(basic_shader, "position");
  glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glBindBuffer(GL_ARRAY_BUFFER, posAttrib);
  glEnableVertexAttribArray(posAttrib);

  // 2nd attribute buffer : colors
  GLint colorAttrib = glGetAttribLocation(basic_shader, "color");
  glEnableVertexAttribArray(colorAttrib);
  glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
  glVertexAttribPointer(
      colorAttrib,  // attribute. No particular reason for 1, but must
                    // match the layout in the shader.
      3,            // size
      GL_FLOAT,     // type
      GL_FALSE,     // normalized?
      0,            // stride
      (void *)0     // array buffer offset
  );

  auto *window = state.window;

  float delta_time = 1.0;

  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);

    update(delta_time);
    render();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
