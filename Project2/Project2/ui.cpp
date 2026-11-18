#include "ui.h"

#include <GL/glew.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
	GLuint uiProgramID = 0;
	GLuint uiVAO = 0;
	GLuint uiVBO = 0;

	GLint uProjLoc = -1;
	GLint uRectLoc = -1;
	GLint uColorLoc = -1;

	//--- 셰이더 소스 읽기
	std::string ReadShader(const char* path)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open()) {
			std::cerr << "ERROR: UI 셰이더 파일을 열 수 없습니다: " << path << "\n";
			return "";
		}
		return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	}

	//--- 셰이더 컴파일 (실패하면 로그 출력)
	GLuint CompileShader(GLenum type, const char* src)
	{
		GLuint shader = glCreateShader(type);
		glShaderSource(shader, 1, &src, NULL);
		glCompileShader(shader);

		GLint result = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
		if (!result) {
			GLchar log[512];
			glGetShaderInfoLog(shader, 512, NULL, log);
			std::cerr << "ERROR: UI 셰이더 컴파일 실패\n" << log << "\n";
			glDeleteShader(shader);
			return 0;
		}
		return shader;
	}

	//--- 사각형 하나 그리기
	void DrawRect(float x, float y, float w, float h, const glm::vec4& color)
	{
		if (w <= 0.0f || h <= 0.0f) return;   // 게이지가 0이면 그리지 않는다
		glUniform4f(uRectLoc, x, y, w, h);
		glUniform4fv(uColorLoc, 1, &color.x);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	void BeginUI(float screenW, float screenH)
	{
		if (uiProgramID == 0) return;
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glUseProgram(uiProgramID);
		glBindVertexArray(uiVAO);

		//--- 화면 좌표계: 좌상단이 (0,0)
		glm::mat4 proj = glm::ortho(0.0f, screenW, screenH, 0.0f);
		glUniformMatrix4fv(uProjLoc, 1, GL_FALSE, glm::value_ptr(proj));
	}

	void EndUI()
	{
		glBindVertexArray(0);
		glDisable(GL_BLEND);
	}
}

void DrawBox(float x, float y, float w, float h, float r, float g, float b, float a)
{
	DrawRect(x, y, w, h, glm::vec4(r, g, b, a));
}

void InitUI()
{
	std::string vsSrc = ReadShader("vertexUI.glsl");
	std::string fsSrc = ReadShader("fragmentUI.glsl");
	if (vsSrc.empty() || fsSrc.empty()) return;

	GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSrc.c_str());
	GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSrc.c_str());
	if (vs == 0 || fs == 0) return;

	uiProgramID = glCreateProgram();
	glAttachShader(uiProgramID, vs);
	glAttachShader(uiProgramID, fs);
	glLinkProgram(uiProgramID);

	GLint linked = 0;
	glGetProgramiv(uiProgramID, GL_LINK_STATUS, &linked);
	glDeleteShader(vs);
	glDeleteShader(fs);
	if (!linked) {
		GLchar log[512];
		glGetProgramInfoLog(uiProgramID, 512, NULL, log);
		std::cerr << "ERROR: UI 셰이더 링크 실패\n" << log << "\n";
		glDeleteProgram(uiProgramID);
		uiProgramID = 0;
		return;
	}

	uProjLoc = glGetUniformLocation(uiProgramID, "uProj");
	uRectLoc = glGetUniformLocation(uiProgramID, "uRect");
	uColorLoc = glGetUniformLocation(uiProgramID, "uColor");

	//--- 단위 사각형 (0,0) ~ (1,1) 두 개의 삼각형
	float quad[] = {
		0.0f, 0.0f,  1.0f, 0.0f,  0.0f, 1.0f,
		0.0f, 1.0f,  1.0f, 0.0f,  1.0f, 1.0f
	};

	glGenVertexArrays(1, &uiVAO);
	glGenBuffers(1, &uiVBO);
	glBindVertexArray(uiVAO);
	glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
}

void DrawUI(float screenW, float screenH,
	float health, float maxHealth,
	float mana, float maxMana)
{
	if (uiProgramID == 0) return;

	//--- 게이지 비율 (0 ~ 1)
	float hpRatio = maxHealth > 0.0f ? health / maxHealth : 0.0f;
	float mpRatio = maxMana > 0.0f ? mana / maxMana : 0.0f;
	if (hpRatio < 0.0f) hpRatio = 0.0f;
	if (hpRatio > 1.0f) hpRatio = 1.0f;
	if (mpRatio < 0.0f) mpRatio = 0.0f;
	if (mpRatio > 1.0f) mpRatio = 1.0f;

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(uiProgramID);
	glBindVertexArray(uiVAO);

	BeginUI(screenW, screenH);

	// HP: 테두리 / 배경 / 채움
	DrawRect(16.0f, screenH - 78.0f, 308.0f, 32.0f, glm::vec4(0.90f, 0.90f, 0.90f, 1.0f));
	DrawRect(20.0f, screenH - 74.0f, 300.0f, 24.0f, glm::vec4(0.10f, 0.10f, 0.10f, 1.0f));
	DrawRect(20.0f, screenH - 74.0f, 300.0f * hpRatio, 24.0f, glm::vec4(0.85f, 0.12f, 0.12f, 1.0f));

	// MP: 테두리 / 배경 / 채움
	DrawRect(16.0f, screenH - 40.0f, 308.0f, 24.0f, glm::vec4(0.90f, 0.90f, 0.90f, 1.0f));
	DrawRect(20.0f, screenH - 36.0f, 300.0f, 16.0f, glm::vec4(0.10f, 0.10f, 0.10f, 1.0f));
	DrawRect(20.0f, screenH - 36.0f, 300.0f * mpRatio, 16.0f, glm::vec4(0.25f, 0.45f, 0.95f, 1.0f));

	EndUI();
}

void DrawTitleScreen(float w, float h)
{
	if (uiProgramID == 0) return;
	BeginUI(w, h);

	DrawBox(0.0f, 0.0f, w, h, 0.05f, 0.06f, 0.10f, 0.75f);        // 반투명 배경(3D가 어둡게 비침)

	DrawBox(w * 0.5f - 320.0f, h * 0.24f, 640.0f, 140.0f,         // 제목 배너
		0.12f, 0.12f, 0.16f, 0.95f);
	DrawBox(w * 0.5f - 316.0f, h * 0.24f + 4.0f, 632.0f, 8.0f,    // 배너 상단 액센트
		0.85f, 0.65f, 0.15f, 1.0f);

	DrawBox(w * 0.5f - 150.0f, h * 0.55f, 300.0f, 80.0f,          // START 버튼(테두리)
		0.18f, 0.60f, 0.30f, 1.0f);
	DrawBox(w * 0.5f - 146.0f, h * 0.55f + 4.0f, 292.0f, 72.0f,   // START 버튼(속)
		0.10f, 0.42f, 0.20f, 1.0f);

	DrawBox(w * 0.5f - 220.0f, h * 0.74f, 440.0f, 44.0f,          // 안내 영역
		0.0f, 0.0f, 0.0f, 0.55f);

	EndUI();
}

void DrawEndScreen(float w, float h, bool clear)
{
	if (uiProgramID == 0) return;
	BeginUI(w, h);

	DrawBox(0.0f, 0.0f, w, h, 0.10f, 0.02f, 0.02f, 0.75f);        // 패배: 붉은 빛

	DrawBox(w * 0.5f - 320.0f, h * 0.30f, 640.0f, 160.0f,         // 결과 배너
		clear ? 0.10f : 0.35f, clear ? 0.35f : 0.06f, 0.10f, 0.95f);

	DrawBox(w * 0.5f - 150.0f, h * 0.62f, 300.0f, 70.0f,          // 재시작 버튼
		0.20f, 0.20f, 0.25f, 1.0f);
	DrawBox(w * 0.5f - 146.0f, h * 0.62f + 4.0f, 292.0f, 62.0f,
		0.12f, 0.12f, 0.16f, 1.0f);

	EndUI();
}
