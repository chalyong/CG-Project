#include <vector>
#include <array>

#define MAX_COUNT 200
#define STRING_SIZE 2000

//--- 도형 데이터 저장소
struct SHP_POINT { float x, y, z; };
struct SHP_TEX { float x, y, z; };
struct SHP_FACE { int v[3]; int vt[3]; bool hasVt; };

static std::vector<SHP_POINT> g_p;
static std::vector<SHP_TEX> g_t;
static std::vector<SHP_FACE> g_f;
static std::vector<float> g_triPos;
static std::vector<float> g_triCol;
static int g_pcnt = 0, g_tcnt = 0, g_fcnt = 0;

//--- 파일 파싱 + 검사 + 데이터 확인 출력
bool loadShape(const char* path)
{
	g_p.clear();
	g_t.clear();
	g_f.clear();
	g_triPos.clear();
	g_triCol.clear();
	g_pcnt = g_tcnt = g_fcnt = 0;

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
		else if (cmd[0] == 'f' && cmd[1] == ' ') {
			int ck[2][3] = {};
			int mod = 0;
			bool hasSlash = false;
			for (size_t i = 2; i < cmd.length(); i++) {
				if (cmd[i] >= '0' && cmd[i] <= '9') {
					ck[mod][spck] *= 10;
					ck[mod][spck] += cmd[i] - '0';
				}
				else if (cmd[i] == '/') {
					mod++;
					hasSlash = true;
				}
				else if (cmd[i] == ' ') {
					spck++;
					mod = 0;
				}
			}
			//std::cout << "( ";
			for (int i = 0; i < 2; i++) {
				for (int j = 0; j < 3; j++) {
			//		std::cout << ck[i][j] << ' ';
					ck[i][j] -= 1;
				}
			//	if (i == 0)
			//		std::cout << "/ ";
			//	else
			//		std::cout << ')';
			}

			bool ok = true;
			if (ck[0][0] < 0 || ck[0][1] < 0 || ck[0][2] < 0 || ck[0][0] >= g_pcnt || ck[0][1] >= g_pcnt || ck[0][2] >= g_pcnt ||
				(g_tcnt > 0 && (ck[1][0] < -1 || ck[1][1] < -1 || ck[1][2] < -1 || ck[1][0] >= g_tcnt || ck[1][1] >= g_tcnt || ck[1][2] >= g_tcnt))) {
				std::cout << "없는 점을 선택하였습니다.";
				ok = false;
			}
			else if (ck[0][0] == ck[0][1] || ck[0][0] == ck[0][2] || ck[0][1] == ck[0][2]) {
				std::cout << "동일한 점을 2개 이상 선택하여, 삼각형을 만들 수 없습니다.";
				ok = false;
			}
			else if ((g_p[ck[0][0]].x == g_p[ck[0][1]].x && g_p[ck[0][0]].y == g_p[ck[0][1]].y && g_p[ck[0][0]].z == g_p[ck[0][1]].z) ||
				(g_p[ck[0][0]].x == g_p[ck[0][2]].x && g_p[ck[0][0]].y == g_p[ck[0][2]].y && g_p[ck[0][0]].z == g_p[ck[0][2]].z) ||
				(g_p[ck[0][2]].x == g_p[ck[0][1]].x && g_p[ck[0][2]].y == g_p[ck[0][1]].y && g_p[ck[0][2]].z == g_p[ck[0][1]].z)) {
				std::cout << "동일한 위치에 있는 점을 2개 이상 선택하여, 삼각형을 만들 수 없습니다.";
				ok = false;
			}
			else {
			//	std::cout << ' ' << ck[0][0] + 1 << ". " << '(' << g_p[ck[0][0]].x << ' ' << g_p[ck[0][0]].y << ' ' << g_p[ck[0][0]].z << ')';
			//	std::cout << ' ' << ck[0][1] + 1 << ". " << '(' << g_p[ck[0][1]].x << ' ' << g_p[ck[0][1]].y << ' ' << g_p[ck[0][1]].z << ')';
			//	std::cout << ' ' << ck[0][2] + 1 << ". " << '(' << g_p[ck[0][2]].x << ' ' << g_p[ck[0][2]].y << ' ' << g_p[ck[0][2]].z << ')';
			//	std::cout << " /";
			//	if (g_tcnt > 0 && ck[1][0] >= 0)
			//		std::cout << ' ' << ck[1][0] + 1 << ". " << '(' << g_t[ck[1][0]].x << ' ' << g_t[ck[1][0]].y << ' ' << g_t[ck[1][0]].z << ')';
			//	if (g_tcnt > 0 && ck[1][1] >= 0)
			//		std::cout << ' ' << ck[1][1] + 1 << ". " << '(' << g_t[ck[1][1]].x << ' ' << g_t[ck[1][1]].y << ' ' << g_t[ck[1][1]].z << ')';
			//	if (g_tcnt > 0 && ck[1][2] >= 0)
			//		std::cout << ' ' << ck[1][2] + 1 << ". " << '(' << g_t[ck[1][2]].x << ' ' << g_t[ck[1][2]].y << ' ' << g_t[ck[1][2]].z << ')';
			}
			//std::cout << std::endl;
			g_fcnt++;

			if (ok) {
				SHP_FACE fc = {
					{ ck[0][0], ck[0][1], ck[0][2] },
					{ ck[1][0], ck[1][1], ck[1][2] },
					hasSlash
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
		}
	}
	return !g_triPos.empty();
}

//--- VAO / VBO 업로드
void uploadBuffer(GLuint& vao, GLuint& vboPos, GLuint& vboCol,
	const float* pos, size_t posN,
	const float* col, size_t colN,
	GLint posLoc, GLint colLoc)
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
}
