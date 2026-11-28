#include "ui.h"

#include <GL/glew.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace {
	GLuint uiProgramID = 0;
	GLuint uiVAO = 0;
	GLuint uiVBO = 0;

	GLuint texTitle = 0;
	GLuint texClear = 0;
	GLuint texOver = 0;
	GLuint texRetry = 0;

	GLint uProjLoc = -1;
	GLint uRectLoc = -1;
	GLint uColorLoc = -1;
	GLint uTexLoc = -1;
	GLint uUseTexLoc = -1;

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

	//--- PNG 로드 -> OpenGL 텍스처 (실패 시 0)
	GLuint LoadTexture(const char* path)
	{
		int iw = 0, ih = 0, channels = 0;
		unsigned char* data = stbi_load(path, &iw, &ih, &channels, 4);
		if (!data) {
			std::cerr << "ERROR: image load failed: " << path << " (" << stbi_failure_reason() << ")" << std::endl;
			return 0;
		}

		GLuint tex = 0;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, iw, ih, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glBindTexture(GL_TEXTURE_2D, 0);

		stbi_image_free(data);
		return tex;
	}

	//--- 사각형 하나 그리기
	void DrawRect(float x, float y, float w, float h, const glm::vec4& color)
	{
		if (w <= 0.0f || h <= 0.0f) return;   // 게이지가 0이면 그리지 않는다
		glUniform1i(uUseTexLoc, 0);
		glUniform4f(uRectLoc, x, y, w, h);
		glUniform4fv(uColorLoc, 1, &color.x);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	//--- 7세그먼트 숫자 (a,b,c,d,e,f,g)
	const bool SEG[10][7] = {
		{ true,  true,  true,  true,  true,  true,  false }, // 0
		{ false, true,  true,  false, false, false, false }, // 1
		{ true,  true,  false, true,  true,  false, true  }, // 2
		{ true,  true,  true,  true,  false, false, true  }, // 3
		{ false, true,  true,  false, false, true,  true  }, // 4
		{ true,  false, true,  true,  false, true,  true  }, // 5
		{ true,  false, true,  true,  true,  true,  true  }, // 6
		{ true,  true,  true,  false, false, false, false }, // 7
		{ true,  true,  true,  true,  true,  true,  true  }, // 8
		{ true,  true,  true,  true,  false, true,  true  }, // 9
	};

	void DrawDigit(float x, float y, float H, int d, const glm::vec4& color)
	{
		if (d < 0 || d > 9) return;
		const float t = H * 0.16f;          // 세그먼트 두께
		const float W = H * 0.62f;          // 숫자 폭
		const float topLen = (H - t) * 0.5f;
		const float botY = y + (H + t) * 0.5f;
		const bool* s = SEG[d];

		if (s[0]) DrawRect(x, y, W, t, color);                            // a (위)
		if (s[6]) DrawRect(x, y + (H - t) * 0.5f, W, t, color);           // g (가운데)
		if (s[3]) DrawRect(x, y + H - t, W, t, color);                    // d (아래)
		if (s[5]) DrawRect(x, y, t, topLen, color);                       // f (왼위)
		if (s[1]) DrawRect(x + W - t, y, t, topLen, color);               // b (오른위)
		if (s[4]) DrawRect(x, botY, t, topLen, color);                    // e (왼아래)
		if (s[2]) DrawRect(x + W - t, botY, t, topLen, color);            // c (오른아래)
	}

	void DrawNumber(float centerX, float y, float H, int value, const glm::vec4& color)
	{
		if (value < 0) value = 0;
		std::string str = std::to_string(value);
		const float W = H * 0.62f;
		const float gap = H * 0.30f;
		float total = (float)str.size() * W + (float)(str.size() - 1) * gap;
		float sx = centerX - total * 0.5f;
		for (size_t i = 0; i < str.size(); ++i)
			DrawDigit(sx + (float)i * (W + gap), y, H, str[i] - '0', color);
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

//--- 이미지(텍스처) 사각형
void DrawImage(float x, float y, float w, float h, unsigned int texture)
{
	if (texture == 0 || w <= 0.0f || h <= 0.0f) return;   // 로드 실패/크기 0이면 그리지 않는다
	glUniform1i(uUseTexLoc, 1);
	glUniform4f(uRectLoc, x, y, w, h);
	glUniform4f(uColorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
	glBindTexture(GL_TEXTURE_2D, (GLuint)texture);
	glDrawArrays(GL_TRIANGLES, 0, 6);
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
	uTexLoc = glGetUniformLocation(uiProgramID, "uTex");
	uUseTexLoc = glGetUniformLocation(uiProgramID, "uUseTex");
	glUniform1i(uTexLoc, 0);   // 텍스처 유닛 0

	//--- UI 이미지 로드
	texTitle = LoadTexture("Image/title_screen.png");
	texClear = LoadTexture("Image/clear_screen.png");
	texOver = LoadTexture("Image/over_screen.png");
	texRetry = LoadTexture("Image/retry_button.png");

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
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);   // vTexCoord
	glEnableVertexAttribArray(1);
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
	//--- 타이틀 이미지 전면 배치 (1280x960 원본 크기)
	DrawImage(0.0f, 0.0f, w, h, texTitle);
	EndUI();
}

void DrawEndScreen(float w, float h, bool clear)
{
	if (uiProgramID == 0) return;
	BeginUI(w, h);
	//--- 클리어 / 게임오버 배너 (가로 전폭, 원본 1024x512 비율 유지)
	float bannerH = w * 0.5f;
	if (bannerH > 640.0f) bannerH = 640.0f;   // retry 버튼과 겹치지 않게 세로 상한
	DrawImage(0.0f, 40.0f, w, bannerH, clear ? texClear : texOver);

	//--- retry 버튼 (512x128, 하단 중앙 - 조건 무관)
	DrawImage((w - 512.0f) * 0.5f, 700.0f, 512.0f, 128.0f, texRetry);
	EndUI();
}

//--- 모드 전환 페이드 (검은 전면)
void DrawFade(float screenW, float screenH, float alpha)
{
	if (uiProgramID == 0 || alpha <= 0.0f) return;
	if (alpha > 1.0f) alpha = 1.0f;

	BeginUI(screenW, screenH);
	DrawRect(0.0f, 0.0f, screenW, screenH, glm::vec4(0.0f, 0.0f, 0.0f, alpha));
	EndUI();
}

//--- 보스 HP (화면 12시, 가로 80% + 위에 수치)
void DrawBossHP(float screenW, float screenH, float bossHealth, float bossMaxHealth)
{
	if (uiProgramID == 0 || bossMaxHealth <= 0.0f) return;

	float ratio = bossHealth / bossMaxHealth;
	if (ratio < 0.0f) ratio = 0.0f;
	if (ratio > 1.0f) ratio = 1.0f;

	const float barW = screenW * 0.8f;          // 가로 80%
	const float barH = 26.0f;
	const float x = (screenW - barW) * 0.5f;    // 12시(가운데) 정렬
	const float yBar = 54.0f;
	const float numH = 30.0f;
	const float yNum = 14.0f;

	BeginUI(screenW, screenH);

	// 수치 배경판 (하늘이 밝아도 읽히도록)
	DrawRect(x, yNum - 4.0f, barW, numH + 8.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.55f));
	// 수치 (가운데 정렬)
	DrawNumber(x + barW * 0.5f, yNum, numH, (int)(bossHealth + 0.5f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	// HP 바: 테두리 / 배경 / 채움
	DrawRect(x - 3.0f, yBar - 3.0f, barW + 6.0f, barH + 6.0f, glm::vec4(0.90f, 0.90f, 0.90f, 1.0f));
	DrawRect(x, yBar, barW, barH, glm::vec4(0.10f, 0.10f, 0.10f, 1.0f));
	DrawRect(x, yBar, barW * ratio, barH, glm::vec4(0.85f, 0.12f, 0.12f, 1.0f));

	EndUI();
}