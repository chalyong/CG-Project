#include <vector>
#include <array>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <gl/glm/glm.hpp>

#define MAX_COUNT 200
#define STRING_SIZE 2000

//--- 도형 데이터 저장소
struct SHP_POINT { float x, y, z; };
struct SHP_TEX { float x, y, z; };
struct SHP_FACE { int v[3]; int vt[3]; int vn[3]; bool hasVt; bool hasVn; };

static std::vector<SHP_POINT> g_p;
static std::vector<SHP_TEX> g_t;
static std::vector<SHP_POINT> g_n;
static std::vector<SHP_FACE> g_f;
static std::vector<float> g_triPos;
static std::vector<float> g_triCol;
static std::vector<float> g_triNrm;
static int g_pcnt = 0, g_tcnt = 0, g_ncnt = 0, g_fcnt = 0;

//--- 한 줄에서 x, y, z 세 값을 읽는다.
static bool parseVec3(const std::string& cmd, size_t start, SHP_POINT& out)
{
	std::istringstream iss(cmd.substr(start));
	return static_cast<bool>(iss >> out.x >> out.y >> out.z);
}

//--- OBJ 인덱스 하나를 0-based로 바꾼다. (음수 인덱스도 지원)
static bool parseIndex(const std::string& s, int count, bool required, int& out)
{
	out = -1;
	if (s.empty()) return !required;

	int idx = 0;
	try {
		idx = std::stoi(s);
	}
	catch (...) {
		return false;
	}

	if (idx > 0) idx -= 1;
	else if (idx < 0) idx = count + idx;
	else return false;

	if (idx < 0 || idx >= count) return false;
	out = idx;
	return true;
}

//--- f 토큰(v, v/vt, v//vn, v/vt/vn)을 읽는다.
static bool parseFaceToken(const std::string& tok, int& vi, int& ti, int& ni)
{
	size_t s1 = tok.find('/');
	std::string sv = tok.substr(0, s1);
	if (!parseIndex(sv, g_pcnt, true, vi)) return false;

	ti = -1;
	ni = -1;

	if (s1 == std::string::npos) return true;

	size_t s2 = tok.find('/', s1 + 1);
	std::string svt = (s2 == std::string::npos)
		? tok.substr(s1 + 1)
		: tok.substr(s1 + 1, s2 - s1 - 1);

	if (!svt.empty() && !parseIndex(svt, g_tcnt, true, ti)) return false;

	if (s2 != std::string::npos) {
		std::string svn = tok.substr(s2 + 1);
		if (!svn.empty() && !parseIndex(svn, g_ncnt, true, ni)) return false;
	}

	return true;
}

//--- 파일 파싱 + 검사 + 데이터 확인 출력
bool loadShape(const char* path)
{
	g_p.clear();
	g_t.clear();
	g_n.clear();
	g_f.clear();
	g_triPos.clear();
	g_triCol.clear();
	g_triNrm.clear();
	g_pcnt = g_tcnt = g_ncnt = g_fcnt = 0;

	std::ifstream fl(path);
	if (!fl.is_open()) {
		std::cerr << "파일을 열 수 없습니다.\n";
		return false;
	}

	std::string cmd;
	while (std::getline(fl, cmd)) {
		if (!cmd.empty() && cmd.back() == '\r') cmd.pop_back();
		cmd += '\n';

		if (cmd[0] == '#' || cmd[0] == 'g' || cmd[0] == '\n') continue;

		//std::cout << cmd;
		int mck = 0;
		int zck = 0;
		int len = 0;
		int spck = 0;
		int err = 0;

		if (cmd[0] == 'v' && cmd[1] == ' ') {
			SHP_POINT pt{ 0.0f, 0.0f, 0.0f };
			for (size_t i = 2; i < cmd.length(); i++) {
				if (cmd[i] == '-') mck = 1;
				else if (cmd[i] >= '0' && cmd[i] <= '9') {
					if (spck <= 2) {
						float* v = (spck == 0) ? &pt.x : (spck == 1) ? &pt.y : &pt.z;
						if (zck == 0) {
							*v = *v * 10 + (float)(cmd[i] - '0');
						}
						else {
							len++;
							float temp = (float)(cmd[i] - '0');
							for (int j = 0; j < len; j++) {
								temp /= 10;
							}
							*v += temp;
						}
					}
				}
				else if (cmd[i] == '.') {
					zck = 1;
				}
				else if (cmd[i] == ' ') {
					if (mck) {
						if (spck == 0) pt.x *= -1;
						else if (spck == 1) pt.y *= -1;
						else if (spck == 2) pt.z *= -1;
					}

					spck++;
					mck = 0;
					zck = 0;
					len = 0;

					if (spck > 2) break;
				}
				else if (cmd[i] == '\n') {
					if (mck && spck == 2) pt.z *= -1;
					break;
				}
				else if (cmd[i] == '#') {
					break;
				}
				else {
					std::cout << "파일에 오류가 있음! v 입력을 하나 건너뜁니다.\n";
					err = 1;
					break;
				}
			}
			//std::cout << g_pcnt + 1 << '.' << '(' << pt.x << ' ' << pt.y << ' ' << pt.z << ')' << std::endl;
			if (pt.x < -1 || pt.x > 1 || pt.y < -1 || pt.y > 1 || pt.z < -1 || pt.z > 1) {
				std::cout << "파일에 오류가 있음! v 입력을 하나 건너뜁니다.\n";
				err = 1;
			}
			if (err == 0) {
				g_p.push_back(pt);
				g_pcnt++;
			}
		}
		else if (cmd[0] == 'v' && cmd[1] == 't' && cmd[2] == ' ') {
			SHP_TEX tc{ 0.0f, 0.0f, 0.0f };
			for (size_t i = 3; i < cmd.length(); i++) {
				if (cmd[i] == '-') mck = 1;
				else if (cmd[i] >= '0' && cmd[i] <= '9') {
					if (spck <= 2) {
						float* v = (spck == 0) ? &tc.x : (spck == 1) ? &tc.y : &tc.z;
						if (zck == 0) {
							*v = *v * 10 + (float)(cmd[i] - '0');
						}
						else {
							len++;
							float temp = (float)(cmd[i] - '0');
							for (int j = 0; j < len; j++) {
								temp /= 10;
							}
							*v += temp;
						}
					}
				}
				else if (cmd[i] == '.') {
					zck = 1;
				}
				else if (cmd[i] == ' ') {
					if (mck) {
						if (spck == 0) tc.x *= -1;
						else if (spck == 1) tc.y *= -1;
						else if (spck == 2) tc.z *= -1;
					}

					spck++;
					mck = 0;
					zck = 0;
					len = 0;

					if (spck > 2) break;
				}
				else if (cmd[i] == '\n') {
					if (mck && spck == 2) tc.z *= -1;
					break;
				}
				else if (cmd[i] == '#') {
					break;
				}
				else {
					std::cout << "파일에 오류가 있음! vt 입력을 하나 건너뜁니다.\n";
					err = 1;
					break;
				}
			}
			//std::cout << g_tcnt + 1 << '.' << '(' << tc.x << ' ' << tc.y << ' ' << tc.z << ')' << std::endl;
			if (tc.x < -1 || tc.x > 1 || tc.y < -1 || tc.y > 1 || tc.z < -1 || tc.z > 1) {
				std::cout << "파일에 오류가 있음! vt 입력을 하나 건너뜁니다.\n";
				err = 1;
			}
			if (err == 0) {
				g_t.push_back(tc);
				g_tcnt++;
			}
		}
		else if (cmd[0] == 'v' && cmd[1] == 'n' && cmd[2] == ' ') {
			SHP_POINT vn{ 0.0f, 0.0f, 0.0f };
			if (!parseVec3(cmd, 3, vn)) {
				std::cout << "파일에 오류가 있음! vn 입력을 하나 건너뜁니다.\n";
			}
			else {
				g_n.push_back(vn);
				g_ncnt++;
			}
		}
		else if (cmd[0] == 'f' && cmd[1] == ' ') {
			int vi[3] = { 0, 0, 0 };
			int ti[3] = { -1, -1, -1 };
			int ni[3] = { -1, -1, -1 };

			std::istringstream iss(cmd.substr(2));
			std::string tok;
			bool ok = true;
			int k = 0;

			while (iss >> tok) {
				if (k >= 3) {
					ok = false;
					break;
				}
				if (!parseFaceToken(tok, vi[k], ti[k], ni[k])) {
					ok = false;
					break;
				}
				k++;
			}

			if (!ok || k != 3) {
				std::cout << "없는 점을 선택하였습니다.";
				ok = false;
			}
			else if (vi[0] == vi[1] || vi[0] == vi[2] || vi[1] == vi[2]) {
				std::cout << "동일한 점을 2개 이상 선택하여, 삼각형을 만들 수 없습니다.";
				ok = false;
			}
			else if ((g_p[vi[0]].x == g_p[vi[1]].x && g_p[vi[0]].y == g_p[vi[1]].y && g_p[vi[0]].z == g_p[vi[1]].z) ||
				(g_p[vi[0]].x == g_p[vi[2]].x && g_p[vi[0]].y == g_p[vi[2]].y && g_p[vi[0]].z == g_p[vi[2]].z) ||
				(g_p[vi[2]].x == g_p[vi[1]].x && g_p[vi[2]].y == g_p[vi[1]].y && g_p[vi[2]].z == g_p[vi[1]].z)) {
				std::cout << "동일한 위치에 있는 점을 2개 이상 선택하여, 삼각형을 만들 수 없습니다.";
				ok = false;
			}

			g_fcnt++;

			if (ok) {
				SHP_FACE fc = {
					{ vi[0], vi[1], vi[2] },
					{ ti[0], ti[1], ti[2] },
					{ ni[0], ni[1], ni[2] },
					(ti[0] >= 0 && ti[1] >= 0 && ti[2] >= 0),
					(ni[0] >= 0 && ni[1] >= 0 && ni[2] >= 0)
				};
				g_f.push_back(fc);
			}
		}
		else {
			std::cout << "없는 명령어 입력!\n";
		}
	}

	fl.close();
	return true;
}

//--- 인덱스 버퍼: 삼각형당 3정점 (GL_TRIANGLES)
bool buildTriangleData()
{
	g_triPos.clear();
	g_triCol.clear();
	g_triNrm.clear();

	static const float col[6][3] = {
		{ 1.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f },
		{ 1.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 1.0f },
		{ 1.0f, 0.0f, 1.0f }
	};

	for (size_t i = 0; i < g_f.size(); i++) {
		const SHP_FACE& fc = g_f[i];

		const SHP_POINT& a = g_p[fc.v[0]];
		const SHP_POINT& b = g_p[fc.v[1]];
		const SHP_POINT& c = g_p[fc.v[2]];

		glm::vec3 pa(a.x, a.y, a.z);
		glm::vec3 pb(b.x, b.y, b.z);
		glm::vec3 pc(c.x, c.y, c.z);

		glm::vec3 faceN = glm::cross(pb - pa, pc - pa);
		if (glm::length(faceN) > 0.000001f)
			faceN = glm::normalize(faceN);
		else
			faceN = glm::vec3(0.0f, 1.0f, 0.0f);

		for (int k = 0; k < 3; k++) {
			const SHP_POINT& pt = g_p[fc.v[k]];
			g_triPos.push_back(pt.x);
			g_triPos.push_back(pt.y);
			g_triPos.push_back(pt.z);

			float cr, cg, cb;
			int ti = fc.hasVt ? fc.vt[k] : -1;
			if (ti >= 0 && ti < g_tcnt) {
				cr = g_t[ti].x;
				cg = g_t[ti].y;
				cb = g_t[ti].z;
			}
			else {
				cr = col[i % 6][0];
				cg = col[i % 6][1];
				cb = col[i % 6][2];
			}
			g_triCol.push_back(cr);
			g_triCol.push_back(cg);
			g_triCol.push_back(cb);

			glm::vec3 n = faceN;
			if (fc.hasVn) {
				const SHP_POINT& vn = g_n[fc.vn[k]];
				n = glm::vec3(vn.x, vn.y, vn.z);
				if (glm::length(n) > 0.000001f)
					n = glm::normalize(n);
				else
					n = faceN;
			}

			g_triNrm.push_back(n.x);
			g_triNrm.push_back(n.y);
			g_triNrm.push_back(n.z);
		}
	}
	return !g_triPos.empty();
}

//--- VAO / VBO 업로드
void uploadBuffer(GLuint& vao, GLuint& vboPos, GLuint& vboCol,
	const float* pos, size_t posN,
	const float* col, size_t colN,
	GLint posLoc, GLint colLoc,
	GLuint* vboNrm = nullptr,
	const float* nrm = nullptr,
	size_t nrmN = 0,
	GLint nrmLoc = -1)
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vboPos);
	glBindBuffer(GL_ARRAY_BUFFER, vboPos);
	glBufferData(GL_ARRAY_BUFFER, posN * sizeof(float), pos, GL_STATIC_DRAW);
	glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(posLoc);

	glGenBuffers(1, &vboCol);
	glBindBuffer(GL_ARRAY_BUFFER, vboCol);
	glBufferData(GL_ARRAY_BUFFER, colN * sizeof(float), col, GL_STATIC_DRAW);
	glVertexAttribPointer(colLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(colLoc);

	if (vboNrm != nullptr && nrm != nullptr && nrmN > 0 && nrmLoc >= 0) {
		glGenBuffers(1, vboNrm);
		glBindBuffer(GL_ARRAY_BUFFER, *vboNrm);
		glBufferData(GL_ARRAY_BUFFER, nrmN * sizeof(float), nrm, GL_STATIC_DRAW);
		glVertexAttribPointer(nrmLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
		glEnableVertexAttribArray(nrmLoc);
	}
}
