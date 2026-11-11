#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include <time.h>
#include <cstring>
#include <cmath>

#include "buffer.cpp"
#include "mouse_move.h"
#include "player.h"

using namespace std;

#define FILE_NAME "cube.obj.txt"
#define TIME 0.02

//--- 사용자 정의 함수
void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
GLvoid drawScene();
void InitBuffer(const char* shapePath);

//--- 필요한 변수 선언
GLint width = 1000, height = 1000;
GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;

Player player;
int dir_f, dir_s;
bool keyW = false, keyS = false, keyA = false, keyD = false;

GLuint VAO_l, VBO_lD, VBO_lC;
float lData[] = {
6.0f, -1.0f, 6.0f,
6.0f, -1.0f, -6.0f,
-6.0f, -1.0f, 6.0f,

6.0f, -1.0f, -6.0f,
-6.0f, -1.0f, -6.0f,
-6.0f, -1.0f, 6.0f
};
float lCr[] = {
0.0f, 0.8f, 0.0f,
0.0f, 0.8f, 0.0f,
0.0f, 0.8f, 0.0f,

0.0f, 0.8f, 0.0f,
0.0f, 0.8f, 0.0f,
0.0f, 0.8f, 0.0f
};
int lCnt = 18;

GLuint VAO_o, VBO_oD, VBO_oC, VBO_oN;

//변환 행렬 제작
glm::mat4 centerMat(1.0f);
glm::mat4 modelPlayer = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.25f, 0.0f));
glm::mat4 sizePlayer = glm::scale(modelPlayer, glm::vec3(0.8f, 1.5f, 0.2f));

float crR[3] = { 0.9f, 0.0f, 0.0f }, crG[3] = { 0.0f,0.9f,0.0f }, crB[3] = { 0.0f, 0.0f, 0.9f },
crC[3] = { 0.0f, 0.9f, 0.9f }, crY[3] = { 0.9f, 0.9f, 0.0f }, crM[3] = { 0.9f, 0.0f, 0.9f },
crLg[3] = { 0.7f, 0.7f, 0.7f }, crDg[3] = { 0.3f, 0.3f, 0.3f }, crDDg[3] = { 0.2f, 0.2f, 0.2f },
crBl[3] = { 0.0f, 0.0f, 0.0f };

int mx, my;
int clicked;

// 투영 행렬 제작
glm::mat4 projection = glm::perspective(
	glm::radians(45.0f),                        // fovy
	(float)width / (float)height,               // aspect
	0.1f, 100.0f);                              // n, f

glm::mat4 orthoProj = glm::ortho(
	-8.0f, 8.0f,
	-8.0f, 8.0f,
	0.1f, 100.0f);

glm::mat4 view = glm::mat4(1.0f);

//--- 파일에서 셰이더 코드 읽어오기
string filetobuf(const char* file)
{
	ifstream shaderFile(file);
	if (!shaderFile.is_open())
	{
		return "";
	}
	string source((istreambuf_iterator<char>(shaderFile)), istreambuf_iterator<char>());
	shaderFile.close();
	return source;
}


// 입력 처리 / 콜백
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	if (key == GLFW_KEY_W) keyW = (action != GLFW_RELEASE);
	if (key == GLFW_KEY_S) keyS = (action != GLFW_RELEASE);
	if (key == GLFW_KEY_A) keyA = (action != GLFW_RELEASE);
	if (key == GLFW_KEY_D) keyD = (action != GLFW_RELEASE);
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		x -= width / 2;
		y = -y + height / 2;
		x /= width / 2;
		y /= height / 2;

		mx = x;
		my = y;
		if (action == GLFW_PRESS) {
			clicked = 1;
		}
		if (action == GLFW_RELEASE) {
			clicked = 0;
		}
	}

}

//--- 메인 함수
int main(int argc, char** argv)
{
	srand((unsigned int)time(NULL));

	//--- 도형 데이터 파일 경로 (인자로 받거나 콘솔 입력)
	string shapeFile;
	if (argc > 1) shapeFile = argv[1];
	else {
		shapeFile = FILE_NAME;
	}
	if (shapeFile.empty()) {
		cerr << "ERROR: 도형 데이터 파일이 지정되지 않음\n";
		return -1;
	}

	//--- GLFW 초기화
	if (!glfwInit())
		return -1;
	//--- OpenGL 버전 및 프로파일 설정
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//--- 윈도우 생성
	GLFWwindow* window =
		glfwCreateWindow(width, height, "OpenGL Window", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}
	//--- 생성한 윈도우의 OpenGL Context를 현재 Context로 설정
	glfwMakeContextCurrent(window);
	//--- GLEW 초기화
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}
	//--- 셰이더 읽어와서 셰이더 프로그램 만들기
	make_vertexShaders();
	make_fragmentShaders();
	shaderProgramID = make_shaderProgram();

	//--- VAO와 VBO 객체 생성
	InitBuffer(shapeFile.c_str());

	//--- 콜백 함수 등록
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	InitMouseMove(window);

	//--- 렌더링 루프
	double prevTime = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		UpdateMouseMove(window);

		double now = glfwGetTime();
		float dt = (float)(now - prevTime);
		prevTime = now;
		if (dt > 0.1f) dt = 0.1f;

		bool focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;

		int dir_f = 0, dir_s = 0;
		if (focused) {
			dir_f = (keyW ? 1 : 0) - (keyS ? 1 : 0);
			dir_s = (keyD ? 1 : 0) - (keyA ? 1 : 0);
		}

		player.Move(dir_f, dir_s, GetMouseMoveYaw(), dt);

		drawScene();
		glfwSwapBuffers(window);
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

//--- 버텍스 셰이더 객체 만들기
void make_vertexShaders()
{
	string vertexSource = filetobuf("vertex3.glsl");
	const char* source = vertexSource.c_str();
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &source, NULL);
	glCompileShader(vertexShader);
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		cerr << "ERROR: vertex shader 컴파일 실패\n" << errorLog << endl;
		return;
	}
}

//--- 프래그먼트 셰이더 객체 만들기
void make_fragmentShaders()
{
	string fragmentSource = filetobuf("fragment3.glsl");
	const char* source = fragmentSource.c_str();
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &source, NULL);
	glCompileShader(fragmentShader);
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		cerr << "ERROR: fragment shader 컴파일 실패\n" << errorLog << endl;
		return;
	}
}

//--- 셰이더 프로그램 만들고 셰이더 객체 링크하기
GLuint make_shaderProgram()
{
	GLint result;
	GLchar errorLog[512];
	GLuint shaderID = glCreateProgram();
	glAttachShader(shaderID, vertexShader);
	glAttachShader(shaderID, fragmentShader);
	glLinkProgram(shaderID);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	glGetProgramiv(shaderID, GL_LINK_STATUS, &result);
	if (!result)
	{
		glGetProgramInfoLog(shaderID, 512, NULL, errorLog);
		cerr << "ERROR: shader program 연결 실패\n" << errorLog << endl;
		return 0;
	}
	glUseProgram(shaderID);

	return shaderID;
}

void InitBuffer(const char* shapePath)
{
	GLint pAttribute = glGetAttribLocation(shaderProgramID, "vPos");
	if (pAttribute == -1) { cerr << "ERROR: vPos attribute를 찾지 못함\n"; return; }
	GLint cAttribute = glGetAttribLocation(shaderProgramID, "vColor");
	if (cAttribute == -1) { cerr << "ERROR: vColor attribute를 찾지 못함\n"; return; }
	GLint nAttribute = glGetAttribLocation(shaderProgramID, "vNormal");
	if (nAttribute == -1) { cerr << "ERROR: vNormal attribute를 찾지 못함\n"; return; }

	if (!loadShape(shapePath)) return;
	buildTriangleData();

	uploadBuffer(VAO_o, VBO_oD, VBO_oC,
		g_triPos.data(), g_triPos.size(),
		g_triCol.data(), g_triCol.size(),
		pAttribute, cAttribute,
		&VBO_oN, g_triNrm.data(), g_triNrm.size(), nAttribute);

	uploadBuffer(VAO_l, VBO_lD, VBO_lC,
		lData, sizeof(lData) / sizeof(float),
		lCr, sizeof(lCr) / sizeof(float),
		pAttribute, cAttribute);
}

//--- 출력 함수
GLvoid drawScene()
{	
	view = GetMouseMoveView(glm::vec3(player.x, player.y, player.z));
	float yaw = GetMouseMoveYaw();

	//--- 배경색 설정
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//--- 렌더링 파이프라인에 세이더 불러오기
	glUseProgram(shaderProgramID);

	//--- 변환 행렬 값을 버텍스 세이더로 보내기
	unsigned int transformLocation = glGetUniformLocation(shaderProgramID, "transform");
	GLint colorLocation = glGetUniformLocation(shaderProgramID, "overrideColor");

	// 조명 키기/끄기
	GLint isLightOn = glGetUniformLocation(shaderProgramID, "LightOn");
	glUniform1i(isLightOn, 1); // 0 - 끔 / 1 - 켬
	// 주변 조명
	GLint lightColorLocation = glGetUniformLocation(shaderProgramID, "lightColor");
	GLint ambientStrengthLocation = glGetUniformLocation(shaderProgramID, "ambientStrength");
	glUniform3f(lightColorLocation, 1.0f, 1.0f, 1.0f); // 빛 색상
	glUniform1f(ambientStrengthLocation, 0.3f); // 주변 강도
	// 산란 조명
	GLint lightPosLocation = glGetUniformLocation(shaderProgramID, "lightPos");
	GLint diffuseStrengthLocation = glGetUniformLocation(shaderProgramID, "diffuseStrength");
	GLint modelLocation = glGetUniformLocation(shaderProgramID, "model");
	GLint normalMatrixLocation = glGetUniformLocation(shaderProgramID, "normalMatrix");
	glUniform3f(lightPosLocation, 4.0f, 5.0f, -4.0f); // 빛 위치 / 현재 우상단
	glUniform1f(diffuseStrengthLocation, 0.5f); // 산란 강도
	// 거울 조명
	GLint viewPosLocation = glGetUniformLocation(shaderProgramID, "viewPos");
	GLint specularStrengthLocation = glGetUniformLocation(shaderProgramID, "specularStrength");
	GLint shininessLocation = glGetUniformLocation(shaderProgramID, "shininess");
	glm::vec3 viewPos = glm::vec3(glm::inverse(view)[3]);
	glUniform3fv(viewPosLocation, 1, glm::value_ptr(viewPos)); // 카메라 위치
	glUniform1f(specularStrengthLocation, 0.8f); // 거울 강도
	glUniform1f(shininessLocation, 32.0f); // 반사 세기 / 32, 64, 128

	glm::mat4 proj = projection;

	glm::mat4 mvp;     // 도형
	glm::mat4 mvpAxis = proj * view * centerMat; // 축
	glm::mat4 model; // 빛 계산을 위한 모델 / 산란
	glm::mat3 normalMat; // 빛 계산을 위한 법선 행렬 / 산란

	glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	//glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);

	glUniform3f(colorLocation, 1.0f, 1.0f, 1.0f);

	// txt 단위 큐브
	{
		glBindVertexArray(VAO_o);

		modelPlayer = glm::translate(glm::mat4(1.0f), glm::vec3(player.x, player.y, player.z))
			* glm::rotate(glm::mat4(1.0f), -yaw, glm::vec3(0.0f, 1.0f, 0.0f));

		model = modelPlayer * sizePlayer;
		normalMat = glm::transpose(glm::inverse(glm::mat3(model)));
		mvp = proj * view * model;
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix3fv(normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(normalMat));
		glUniformMatrix4fv(transformLocation, 1, GL_FALSE, glm::value_ptr(mvp));
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

	// 바닥 넓은 평면
	{
		glBindVertexArray(VAO_l);
		glUniformMatrix4fv(transformLocation, 1, GL_FALSE, glm::value_ptr(mvpAxis));
		glDrawArrays(GL_TRIANGLES, 0, lCnt);
	}

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}
