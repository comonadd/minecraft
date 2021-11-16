#include <stdio.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <thread>

std::string read_whole_file(char const *fp) {
  std::ifstream vs_f(fp);
  if (!vs_f.is_open()) {
    std::cout << "Failed to open the file at " << fp << '\n';
    exit(-1);
    return std::string();
  }
  std::string str((std::istreambuf_iterator<char>(vs_f)),
                  std::istreambuf_iterator<char>());
  return std::move(str);
}

GLuint load_shader(char const *vs_path, char const *fs_path) {
  GLint res;
  auto vs_source = read_whole_file(vs_path);
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  auto *vss = (vs_source.data());
  glShaderSource(vertexShader, 1, &vss, NULL);
  glCompileShader(vertexShader);
  GLint status;
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    char buffer[512];
    glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
    std::cout << "Error during vertex shader compilation:\n" << buffer;
    exit(-1);
  }
  auto fs_source = read_whole_file(fs_path);
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  auto *fss = fs_source.data();
  glShaderSource(fragmentShader, 1, &fss, NULL);
  glCompileShader(fragmentShader);
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    char buffer[512];
    glGetShaderInfoLog(fragmentShader, 512, NULL, buffer);
    std::cout << "Error during fragment shader compilation:\n" << buffer;
    exit(-1);
  }
  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  return shaderProgram;
}

struct {
  GLFWwindow *window;
} state;

int main() {
  glfwInit();

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  state.window =
      glfwCreateWindow(800, 600, "OpenGL", nullptr, nullptr); // Windowed
  glfwMakeContextCurrent(state.window);

  glewExperimental = GL_TRUE;
  glewInit();
  GLuint vertexBuffer;
  glGenBuffers(1, &vertexBuffer);
  printf("%u\n", vertexBuffer);

  float vertices[] = {
      -1.0f, 1.0f,  // Vertex 1 (X, Y)
      1.0f,  1.0f,  // Vertex 2 (X, Y)
      1.0f,  -1.0f, // Vertex 3 (X, Y)
      1.0f,  -1.0f, // Vertex 1 (X, Y)
      -1.0f, -1.0f, // Vertex 2 (X, Y)
      -1.0f, -1.0f, // Vertex 3 (X, Y)
  };
  GLuint vbo;
  glGenBuffers(1, &vbo); // Generate 1 buffer
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  auto basic_shader =
      load_shader("./shaders/basic_vs.glsl", "./shaders/basic_fs.glsl");
  glUseProgram(basic_shader);

  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  GLint posAttrib = glGetAttribLocation(basic_shader, "position");
  glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(posAttrib);

  auto *window = state.window;
  while (!glfwWindowShouldClose(window)) {
    glfwSwapBuffers(window);

    glDrawArrays(GL_TRIANGLES, 0, 3 * 2);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, GL_TRUE);
    }

    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
