#pragma once

#include <GL/glfw3.h>
#include <gl/glm/glm.hpp>

void InitMouseMove(GLFWwindow* window);
void UpdateMouseMove(GLFWwindow* window);
float GetMouseMoveYaw();
glm::mat4 GetMouseMoveView(const glm::vec3& eyePosition);