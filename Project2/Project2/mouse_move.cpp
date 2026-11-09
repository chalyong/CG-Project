#include "mouse_move.h"

#include <cmath>
#include <gl/glm/gtc/matrix_transform.hpp>

namespace {

float yaw = 0.0f;
float pitch = 0.0f;
double mouseDx = 0.0, mouseDy = 0.0;

const float SENS = 0.0015f;
const float PITCH_LIMIT = glm::radians(90.0f);
const float TWO_PI = 6.28318530718f;

bool wasFocused = false;

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (!glfwGetWindowAttrib(window, GLFW_FOCUSED)) return;

	int ww, wh;
	glfwGetWindowSize(window, &ww, &wh);

	// 포인터는 항상 중앙에 있다고 가정하고 중앙 기준 이동량만 누적
	mouseDx += xpos - ww / 2.0;
	mouseDy += wh / 2.0 - ypos;
}

void InitMouseMove(GLFWwindow* window)
{
	glfwSetCursorPosCallback(window, CursorPosCallback);

	yaw = 0.0f;
	pitch = 0.0f;
	mouseDx = mouseDy = 0.0;
	wasFocused = false;
}

void UpdateMouseMove(GLFWwindow* window)
{
	bool focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;

	if (focused && !wasFocused) {
		mouseDx = mouseDy = 0.0;
	}
	wasFocused = focused;

	if (focused) {
		yaw += (float)mouseDx * SENS;
		pitch += (float)mouseDy * SENS;

		// 좌우는 무제한, 상하는 수직 위/바닥까지
		yaw = fmodf(yaw, TWO_PI);
		pitch = glm::clamp(pitch, -PITCH_LIMIT, PITCH_LIMIT);
	}
	mouseDx = mouseDy = 0.0;

	if (focused) {
		int ww, wh;
		glfwGetWindowSize(window, &ww, &wh);
		glfwSetCursorPos(window, ww / 2.0, wh / 2.0);
	}
}

glm::mat4 GetMouseMoveView()
{
	float cp = cosf(pitch), sp = sinf(pitch);
	float cy = cosf(yaw), sy = sinf(yaw);

	glm::vec3 eye(0.0f);
	glm::vec3 dir(-cp * sy, sp, cp * cy);
	glm::vec3 up(sp * sy, cp, -sp * cy);
	glm::vec3 at = eye + dir;

	return glm::lookAt(eye, at, up);
}
