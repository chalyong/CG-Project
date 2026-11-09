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

#include "buffer.cpp"

using namespace std;

#define FILE_NAME "cube.obj_19.txt"
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

GLuint VAO_l, VBO_lD, VBO_lC;
float lData[] = {
6.0f, -1.0f, 6.0f,
6.0f, -1.0f, -6.0f,
-6.0f, -1.0f, 6.0f,

6.0f, -1.0f, -6.0f,
-6.0f, -1.0f, 6.0f,
-6.0f, -1.0f, -6.0f
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

GLuint VAO_o, VBO_oD, VBO_oC;

//변환 행렬 제작
glm::mat4 centerMat(1.0f);

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

glm::vec3 eye(0.0f, 0.0f, 0.0f), at(0, 0, 1), upv(0, 1, 0);
glm::mat4 view = glm::lookAt(eye, at, upv);

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

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	double x, y;
	glfwGetCursorPos(window, &x, &y);
	x -= width / 2;
	y = -y + height / 2;
	x /= width / 2;
	y /= height / 2;

	if (clicked) {
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
	glfwSetCursorPosCallback(window, CursorPosCallback);


	//--- 렌더링 루프
	while (!glfwWindowShouldClose(window))
	{
		drawScene();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

//--- 버텍스 셰이더 객체 만들기
void make_vertexShaders()
{
	string vertexSource = filetobuf("vertex2.glsl");
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
	string fragmentSource = filetobuf("fragment2.glsl");
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

	if (!loadShape(shapePath)) return;
	buildTriangleData();

	uploadBuffer(VAO_o, VBO_oD, VBO_oC,
		g_triPos.data(), g_triPos.size(),
		g_triCol.data(), g_triCol.size(),
		pAttribute, cAttribute);

	uploadBuffer(VAO_l, VBO_lD, VBO_lC,
		lData, sizeof(lData) / sizeof(float),
		lCr, sizeof(lCr) / sizeof(float),
		pAttribute, cAttribute);
}

//--- 출력 함수
GLvoid drawScene()
{
	// 회전 타이머
	if (glfwGetTime() > TIME) {


		glfwSetTime(0.0);
	}


	//--- 배경색 설정
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//--- 렌더링 파이프라인에 세이더 불러오기
	glUseProgram(shaderProgramID);

	//--- 변환 행렬 값을 버텍스 세이더로 보내기
	unsigned int transformLocation = glGetUniformLocation(shaderProgramID, "transform");
	GLint colorLocation = glGetUniformLocation(shaderProgramID, "overrideColor");

	glm::mat4 proj = projection;

	glm::mat4 mvp;     // 도형
	glm::mat4 mvpAxis = proj * view * centerMat; // 축

	glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	//glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);

	// txt 단위 큐브
	{
		glBindVertexArray(VAO_o);
		mvp = proj * view * centerMat;
		glUniformMatrix4fv(transformLocation, 1, GL_FALSE, glm::value_ptr(mvp));
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

	glUniform3f(colorLocation, 1.0f, 1.0f, 1.0f);

	// 바닥 넓은 평면
	{
		glBindVertexArray(VAO_l);
		glUniformMatrix4fv(transformLocation, 1, GL_FALSE, glm::value_ptr(mvpAxis));
		glDrawArrays(GL_TRIANGLES, 0, lCnt);
	}

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}
