#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "common.hpp"

struct Camera {
  bool firstMouse = true;
  float yaw = -90.0f;
  float pitch = 0.0f;
  float sensitivity = 0.1f;
  glm::vec3 camera_pos = glm::vec3(4.0, 50.0, 50.0);
  glm::vec3 camera_front = glm::vec3(0.0, 0.0, -1.0f);
  glm::vec3 camera_up = glm::vec3(0.0, 0.1, 0.0);
  float lastX;
  float lastY;
  float speed = 32.0f;

  Camera(u32 window_width, u32 window_height, float initial_speed)
      : lastX((float)window_width / 2.0f),
        lastY((float)window_height / 2.0f),
        speed(initial_speed) {}
};

void camera_process_movement(Camera &camera, double xpos, double ypos) {
  auto &state = camera;
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

inline void camera_move_forward(Camera &camera, float dt) {
  camera.camera_pos += camera.speed * dt * camera.camera_front;
}

inline void camera_move_backward(Camera &camera, float dt) {
  camera.camera_pos -= camera.speed * dt * camera.camera_front;
}

inline void camera_move_left(Camera &camera, float dt) {
  camera.camera_pos -=
      glm::normalize(glm::cross(camera.camera_front, camera.camera_up)) *
      camera.speed * dt;
}

inline void camera_move_right(Camera &camera, float dt) {
  camera.camera_pos +=
      glm::normalize(glm::cross(camera.camera_front, camera.camera_up)) *
      camera.speed * dt;
}

#endif
