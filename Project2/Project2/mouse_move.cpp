#include "mouse_move.h"

#include <cmath>
#include <gl/glm/gtc/matrix_transform.hpp>

namespace {
	float yaw = 0.0f;
	float pitch = 0.0f;
	double mouseDx = 0.0, mouseDy = 0.0;

	const float SENS = 0.0015f;
	const float PITCH_LIMIT = glm::radians(60.0f);
	const float TWO_PI = 6.28318530718f;

	bool wasFocused = false;
}
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

float GetMouseMoveYaw()
{
	return yaw;
}

glm::mat4 GetMouseMoveView(const glm::vec3& playerPosition)
{
	//--- 3인칭 카메라 오프셋 (플레이어 기준)
	const float EYE_RIGHT = 0.0f;   // 화면 오른쪽 x
	const float EYE_UP = 3.5f;   // 눈 높이 y
	const float EYE_BACK = 8.0f;   // 뒤쪽 거리 -z
	const float TARGET_UP = 0.2f;   // 캐릭터를 보는 높이

	float cp = cosf(pitch), sp = sinf(pitch);
	float cy = cosf(yaw), sy = sinf(yaw);

	// yaw 기준 이동 축 (player.cpp와 동일 규약)
	glm::vec3 forward(-sy, 0.0f, cy);
	glm::vec3 right(-cy, 0.0f, -sy);

	// yaw로 회전하는 eye 위치
	glm::vec3 pivot = playerPosition + glm::vec3(0.0f, EYE_UP, 0.0f);
	glm::vec3 eye = pivot
		+ right * EYE_RIGHT
		- forward * (EYE_BACK * cp)
		- glm::vec3(0.0f, EYE_BACK * sp, 0.0f);

	// 캐릭터를 바라보는 목표점
	glm::vec3 target = playerPosition + glm::vec3(0.0f, TARGET_UP, 0.0f);

	// 땅을 뚫지 않게 (pitch 위쪽 한계 보조)
	if (eye.y < 0.3f) eye.y = 0.3f;

	return glm::lookAt(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
}

void ResetMouseMove()
{
	yaw = 0.0f;
	pitch = 0.0f;
	mouseDx = mouseDy = 0.0;
}

//--- 커서 이동량만 버린다 (타이틀/엔드에서 잔여 드리프트 방지)
void ClearMouseMoveDelta()
{
	mouseDx = mouseDy = 0.0;
}
