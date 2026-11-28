#include "boss.h"
#include "blocks.h"

#include <gl/glm/gtc/matrix_transform.hpp>
#include <gl/glm/gtc/type_ptr.hpp>

#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <cfloat>

namespace {
	const float PI = 3.14159265f;

	// 수치
	const float GROUND_Y      = BLOCK_BASE_Y;  // 바닥 높이 (-1)
	const float AREA_Y        = GROUND_Y + 0.02f; // 장판 높이 (바닥과 z-fighting 방지)
	const float BOSS_RADIUS   = 0.9f;   // 보스 몸 충돌 반경
	const float PLAYER_RADIUS = 0.4f;   // 플레이어 판정 반경 (player.cpp PLAYER_HALF와 동일)
	const float PLAYER_FOOT   = 1.0f;   // player.y - 발 높이 (player.cpp FOOT_OFFSET과 동일)
	const float MAP_LIMIT     = 95.0f;  // 바닥 밖으로 나가지 않도록

	const float CHASE_SPEED   = 2.4f;   // 추격 속도 (플레이어 걷기 3, 달리기 6)
	const float KEEP_DIST     = 2.2f;   // 이 거리 안이면 더 다가가지 않음
	const float FIRST_DELAY   = 2.0f;   // 시작 후 첫 공격까지 대기
	const float ATTACK_GAP    = 1.2f;   // 공격 사이 기본 쿨다운

	// 부채꼴 베기
	const float FAN_RADIUS    = 6.5f;
	const float FAN_HALF_DEG  = 50.0f;
	const float FAN_DAMAGE    = 20.0f;
	// 돌진
	const float DASH_LENGTH   = 12.0f;
	const float DASH_HALF_W   = 1.0f;
	const float DASH_DAMAGE   = 25.0f;
	// 회오리
	const float WHIRL_RADIUS  = 3.5f;
	const float WHIRL_SPEED   = 2.2f;   // 회전하며 플레이어 쪽으로 이동
	const float WHIRL_TICK    = 0.4f;   // 다단 히트 간격
	const float WHIRL_DAMAGE  = 8.0f;   // 틱당 피해
	const float WHIRL_SPIN    = 900.0f; // 회전 속도 (도/초)
	// 근접 범위 (점프 후 내려찍기)
	const float SLAM_RADIUS   = 5.0f;
	const float SLAM_DAMAGE   = 30.0f;
	const float SLAM_JUMP_H   = 1.4f;   // 보스가 뛰어오르는 높이
	const float SLAM_DODGE_H  = 0.6f;   // 플레이어 발이 이 높이보다 높으면 회피

	// 파트별 크기
	const glm::vec3 PART_SIZE[] = {
		{ 1.3f, 1.3f, 0.7f },   // BODY
		{ 0.8f, 0.8f, 0.8f },   // HEAD
		{ 0.55f, 0.15f, 0.1f }, // VISOR (눈)
		{ 0.35f, 1.2f, 0.35f }, // ARM_L
		{ 0.35f, 1.2f, 0.35f }, // ARM_R
		{ 0.45f, 1.2f, 0.45f }, // LEG_L
		{ 0.45f, 1.2f, 0.45f }, // LEG_R
		{ 0.1f, 1.9f, 0.28f },  // BLADE (손에서 팔 방향으로 뻗음)
	};
	const glm::vec3 PART_COLOR[] = {
		{ 0.70f, 0.15f, 0.15f }, // BODY  - 진홍
		{ 0.80f, 0.80f, 0.85f }, // HEAD  - 은색
		{ 1.00f, 0.85f, 0.10f }, // VISOR - 노랑
		{ 0.30f, 0.30f, 0.35f }, // ARM_L
		{ 0.30f, 0.30f, 0.35f }, // ARM_R
		{ 0.20f, 0.20f, 0.25f }, // LEG_L
		{ 0.20f, 0.20f, 0.25f }, // LEG_R
		{ 0.75f, 0.85f, 1.00f }, // BLADE - 푸른 강철
	};
	const char* PART_FILE[] = {
		"sword_body.obj", "sword_head.obj", "sword_visor.obj",
		"sword_arm_l.obj", "sword_arm_r.obj",
		"sword_leg_l.obj", "sword_leg_r.obj", "sword_blade.obj",
	};

	//--- 관절 위치 (보스 발 기준, 앞 = +Z, 오른쪽 = -X)
	const float HIP_Y = 1.2f, HIP_X = 0.3f;
	const float SHOULDER_Y = 2.4f, SHOULDER_X = 0.85f;
	const float ARM_LEN = 1.2f, LEG_LEN = 1.2f;

	float RandomRange(float lo, float hi) { return lo + (rand() % 10000) / 9999.0f * (hi - lo); }
	float Clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
	float Lerp(float a, float b, float t) { return a + (b - a) * t; }
	float SmoothStep(float t) { t = Clamp01(t); return t * t * (3.0f - 2.0f * t); }


	struct ObjData {
		std::vector<float> pos, col, nrm;
		bool hasMaterial = false;
		glm::vec3 bmin{ FLT_MAX }, bmax{ -FLT_MAX };
	};

	int ResolveIndex(int idx, int count) { return idx > 0 ? idx - 1 : count + idx; }

	void ParseFaceToken(const std::string& tok, int& v, int& n, int vCount, int nCount)
	{
		v = n = -1;
		size_t s1 = tok.find('/');
		v = ResolveIndex(std::atoi(tok.substr(0, s1).c_str()), vCount);
		if (s1 == std::string::npos) return;
		size_t s2 = tok.find('/', s1 + 1);
		if (s2 == std::string::npos) return;
		std::string sn = tok.substr(s2 + 1);
		if (!sn.empty()) n = ResolveIndex(std::atoi(sn.c_str()), nCount);
	}

	std::map<std::string, glm::vec3> LoadMtl(const std::string& path)
	{
		std::map<std::string, glm::vec3> mats;
		std::ifstream f(path);
		if (!f.is_open()) return mats;
		std::string line, cur;
		while (std::getline(f, line)) {
			std::istringstream iss(line);
			std::string cmd;
			iss >> cmd;
			if (cmd == "newmtl") iss >> cur;
			else if (cmd == "Kd" && !cur.empty()) {
				glm::vec3 c(1.0f);
				iss >> c.r >> c.g >> c.b;
				mats[cur] = c;
			}
		}
		return mats;
	}

	bool LoadObj(const std::string& path, ObjData& out)
	{
		std::ifstream f(path);
		if (!f.is_open()) return false;

		std::string dir = path.substr(0, path.find_last_of("/\\") + 1);
		std::vector<glm::vec3> vs, ns;
		std::map<std::string, glm::vec3> mats;
		glm::vec3 curColor(1.0f);

		std::string line;
		while (std::getline(f, line)) {
			std::istringstream iss(line);
			std::string cmd;
			iss >> cmd;
			if (cmd == "v") {
				glm::vec3 p; iss >> p.x >> p.y >> p.z; vs.push_back(p);
			}
			else if (cmd == "vn") {
				glm::vec3 n; iss >> n.x >> n.y >> n.z; ns.push_back(n);
			}
			else if (cmd == "mtllib") {
				std::string name; iss >> name; mats = LoadMtl(dir + name);
			}
			else if (cmd == "usemtl") {
				std::string name; iss >> name;
				auto it = mats.find(name);
				if (it != mats.end()) { curColor = it->second; out.hasMaterial = true; }
				else curColor = glm::vec3(1.0f);
			}
			else if (cmd == "f") {
				std::vector<int> fv, fn;
				std::string tok;
				while (iss >> tok) {
					int v, n;
					ParseFaceToken(tok, v, n, (int)vs.size(), (int)ns.size());
					if (v < 0 || v >= (int)vs.size()) { fv.clear(); break; }
					fv.push_back(v);
					fn.push_back((n >= 0 && n < (int)ns.size()) ? n : -1);
				}
				// 다각형은 부채꼴(fan)로 삼각형 분할
				for (size_t k = 1; k + 1 < fv.size(); ++k) {
					size_t idx[3] = { 0, k, k + 1 };
					glm::vec3 a = vs[fv[0]], b = vs[fv[k]], c = vs[fv[k + 1]];
					glm::vec3 faceN = glm::cross(b - a, c - a);
					faceN = glm::length(faceN) > 1e-6f ? glm::normalize(faceN) : glm::vec3(0, 1, 0);
					for (size_t i : idx) {
						glm::vec3 p = vs[fv[i]];
						glm::vec3 n = fn[i] >= 0 ? ns[fn[i]] : faceN;
						out.pos.insert(out.pos.end(), { p.x, p.y, p.z });
						out.col.insert(out.col.end(), { curColor.r, curColor.g, curColor.b });
						out.nrm.insert(out.nrm.end(), { n.x, n.y, n.z });
						out.bmin = glm::min(out.bmin, p);
						out.bmax = glm::max(out.bmax, p);
					}
				}
			}
		}
		return !out.pos.empty();
	}

	//--- OBJ 가 없을 때 사용하는 단위 큐브 (중심 원점, 한 변 1)
	void MakeCube(ObjData& out)
	{
		const glm::vec3 N[6] = { {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1} };
		for (const glm::vec3& n : N) {
			glm::vec3 u = (fabsf(n.y) > 0.5f) ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
			glm::vec3 v = glm::cross(n, u);
			glm::vec3 c = n * 0.5f;
			glm::vec3 q[4] = { c - u * 0.5f - v * 0.5f, c + u * 0.5f - v * 0.5f,
			                   c + u * 0.5f + v * 0.5f, c - u * 0.5f + v * 0.5f };
			// 바깥쪽이 앞면(CCW)이 되도록 순서 맞춤
			bool flip = glm::dot(glm::cross(q[1] - q[0], q[2] - q[0]), n) < 0.0f;
			int order[6] = { 0, 1, 2, 0, 2, 3 };
			if (flip) { order[1] = 2; order[2] = 1; order[4] = 3; order[5] = 2; }
			for (int i : order) {
				out.pos.insert(out.pos.end(), { q[i].x, q[i].y, q[i].z });
				out.col.insert(out.col.end(), { 1.0f, 1.0f, 1.0f });
				out.nrm.insert(out.nrm.end(), { n.x, n.y, n.z });
			}
		}
		out.bmin = glm::vec3(-0.5f);
		out.bmax = glm::vec3(0.5f);
	}

	GLuint MakeVbo(const std::vector<float>& data, GLint loc)
	{
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
		if (loc >= 0) {
			glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
			glEnableVertexAttribArray(loc);
		}
		return vbo;
	}

	// 범위 표시 셰이더 (반투명 단색 + 가장자리 강조)

	const char* AREA_VS = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in float aEdge;
uniform mat4 uVP;
out float vEdge;
void main() {
	gl_Position = uVP * vec4(aPos, 1.0);
	vEdge = aEdge;
})";
	const char* AREA_FS = R"(#version 330 core
in float vEdge;
uniform vec4 uColor;
out vec4 FragColor;
void main() {
	float a = uColor.a * mix(0.55, 1.0, vEdge * vEdge);
	FragColor = vec4(uColor.rgb, a);
})";

	GLuint CompileProgram(const char* vsSrc, const char* fsSrc)
	{
		auto compile = [](GLenum type, const char* src) {
			GLuint s = glCreateShader(type);
			glShaderSource(s, 1, &src, nullptr);
			glCompileShader(s);
			GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
			if (!ok) {
				char log[512]; glGetShaderInfoLog(s, 512, nullptr, log);
				std::cerr << "ERROR: boss area shader 컴파일 실패\n" << log << std::endl;
			}
			return s;
		};
		GLuint vs = compile(GL_VERTEX_SHADER, vsSrc);
		GLuint fs = compile(GL_FRAGMENT_SHADER, fsSrc);
		GLuint p = glCreateProgram();
		glAttachShader(p, vs);
		glAttachShader(p, fs);
		glLinkProgram(p);
		glDeleteShader(vs);
		glDeleteShader(fs);
		return p;
	}

	//--- 장판 정점: (x, y, z, edge) / edge 0 = 중심, 1 = 가장자리
	void PushV(std::vector<float>& v, float x, float z, float e)
	{
		v.insert(v.end(), { x, AREA_Y, z, e });
	}

	// 부채꼴 (halfAngle = PI 이면 원)
	void AddSector(std::vector<float>& v, float cx, float cz, float dirYaw,
		float halfAngle, float radius, int seg = 48)
	{
		for (int i = 0; i < seg; ++i) {
			float a0 = dirYaw - halfAngle + 2.0f * halfAngle * i / seg;
			float a1 = dirYaw - halfAngle + 2.0f * halfAngle * (i + 1) / seg;
			PushV(v, cx, cz, 0.0f);
			PushV(v, cx + sinf(a0) * radius, cz + cosf(a0) * radius, 1.0f);
			PushV(v, cx + sinf(a1) * radius, cz + cosf(a1) * radius, 1.0f);
		}
	}

	// 직사각형 (시작점에서 dirYaw 방향으로 length 만큼)
	void AddRect(std::vector<float>& v, float cx, float cz, float dirYaw,
		float length, float halfW)
	{
		float fx = sinf(dirYaw), fz = cosf(dirYaw);   // 앞
		float rx = -fz, rz = fx;                        // 옆
		float x0 = cx + rx * halfW, z0 = cz + rz * halfW;
		float x1 = cx - rx * halfW, z1 = cz - rz * halfW;
		float x2 = x1 + fx * length, z2 = z1 + fz * length;
		float x3 = x0 + fx * length, z3 = z0 + fz * length;
		PushV(v, x0, z0, 0.3f); PushV(v, x1, z1, 0.3f); PushV(v, x2, z2, 1.0f);
		PushV(v, x0, z0, 0.3f); PushV(v, x2, z2, 1.0f); PushV(v, x3, z3, 1.0f);
	}

	//--- 판정 함수 (XZ 평면)
	bool HitCircle(float cx, float cz, float r, float px, float pz)
	{
		float dx = px - cx, dz = pz - cz;
		float rr = r + PLAYER_RADIUS;
		return dx * dx + dz * dz < rr * rr;
	}

	bool HitSector(float cx, float cz, float dirYaw, float halfAngle, float r, float px, float pz)
	{
		float dx = px - cx, dz = pz - cz;
		float d = sqrtf(dx * dx + dz * dz);
		if (d > r + PLAYER_RADIUS) return false;
		if (d < BOSS_RADIUS) return true;             // 바로 붙어 있으면 맞음
		float cosA = (dx * sinf(dirYaw) + dz * cosf(dirYaw)) / d;
		return cosA >= cosf(halfAngle);
	}

	bool HitRect(float cx, float cz, float dirYaw, float length, float halfW, float px, float pz)
	{
		float fx = sinf(dirYaw), fz = cosf(dirYaw);
		float dx = px - cx, dz = pz - cz;
		float along = dx * fx + dz * fz;
		float side = dx * -fz + dz * fx;
		return along >= -PLAYER_RADIUS && along <= length + PLAYER_RADIUS
			&& fabsf(side) <= halfW + PLAYER_RADIUS;
	}

	//--- 보스가 해당 위치에서 블럭과 겹치는지
	bool BossCollides(float bx, float bz)
	{
		for (const Block& b : GetBlocks()) {
			if (fabsf(bx - b.x) < b.w * 0.5f + BOSS_RADIUS
				&& fabsf(bz - b.z) < b.d * 0.5f + BOSS_RADIUS) return true;
		}
		return false;
	}
}

// 생성 / 초기화
SwordBoss::SwordBoss()
	: maxHealth(300.0f), health(300.0f), x(0.0f), z(7.0f), yaw(PI)
	, initialized(false)
	, state(State::CHASE), pattern(Pattern::NONE), lastPattern(Pattern::NONE)
	, stateTime(0.0f), cooldown(FIRST_DELAY), chaseTime(0.0f)
	, hitApplied(false), tickTimer(0.0f), walkPhase(0.0f), spinAngle(0.0f), hitFlash(0.0f)
	, atkX(0.0f), atkZ(0.0f), atkYaw(0.0f), dashTravel(0.0f)
	, areaProgram(0), areaVAO(0), areaVBO(0)
{
}

void SwordBoss::Init(GLuint mainProgram, const std::string& objDir)
{
	GLint posLoc = glGetAttribLocation(mainProgram, "vPos");
	GLint colLoc = glGetAttribLocation(mainProgram, "vColor");
	GLint nrmLoc = glGetAttribLocation(mainProgram, "vNormal");

	for (int i = 0; i < PART_COUNT; ++i) {
		ObjData data;
		std::string path = objDir + PART_FILE[i];
		if (!LoadObj(path, data)) {
			std::cout << "[Boss] " << path << " 없음 -> 큐브로 대체\n";
			data = ObjData();
			MakeCube(data);
		}

		Mesh& m = meshes[i];
		m.count = (int)(data.pos.size() / 3);
		m.hasMaterial = data.hasMaterial;

		// 바운딩 박스 중심을 원점으로, 크기를 1로 맞춤 (축별)
		glm::vec3 ext = data.bmax - data.bmin;
		glm::vec3 center = (data.bmax + data.bmin) * 0.5f;
		for (int k = 0; k < 3; ++k) if (ext[k] < 1e-5f) ext[k] = 1.0f;   // 평면 파트 대비
		m.fit = glm::scale(glm::mat4(1.0f), 1.0f / ext) * glm::translate(glm::mat4(1.0f), -center);

		glGenVertexArrays(1, &m.vao);
		glBindVertexArray(m.vao);
		m.vboPos = MakeVbo(data.pos, posLoc);
		m.vboCol = MakeVbo(data.col, colLoc);
		m.vboNrm = MakeVbo(data.nrm, nrmLoc);
	}

	//범위 표시용 셰이더 / 동적 버퍼
	areaProgram = CompileProgram(AREA_VS, AREA_FS);
	glGenVertexArrays(1, &areaVAO);
	glBindVertexArray(areaVAO);
	glGenBuffers(1, &areaVBO);
	glBindBuffer(GL_ARRAY_BUFFER, areaVBO);
	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	initialized = true;
	Reset();
}

void SwordBoss::Reset()
{
	health = maxHealth;
	x = 0.0f; z = 7.0f;       // 블럭이 없는 시작 안전 구역(반경 8) 안
	yaw = PI;                 // 원점(플레이어) 쪽을 바라봄
	state = State::CHASE;
	pattern = lastPattern = Pattern::NONE;
	stateTime = 0.0f;
	cooldown = FIRST_DELAY;
	chaseTime = 0.0f;
	hitApplied = false;
	tickTimer = 0.0f;
	walkPhase = spinAngle = hitFlash = 0.0f;
	dashTravel = 0.0f;
	pose = Pose();
}

void SwordBoss::TakeDamage(float amount)
{
	if (state == State::DEAD) return;
	health -= amount;
	hitFlash = 1.0f;
	if (health <= 0.0f) {
		health = 0.0f;
		EnterState(State::DEAD);
	}
}

// 패턴 데이터
float SwordBoss::PatternTelegraph(Pattern p) const
{
	switch (p) {
	case Pattern::FAN_SLASH: return 0.9f;
	case Pattern::DASH:      return 1.0f;
	case Pattern::WHIRLWIND: return 0.8f;
	case Pattern::SLAM:      return 1.2f;
	default:                 return 0.0f;
	}
}
float SwordBoss::PatternActive(Pattern p) const
{
	switch (p) {
	case Pattern::FAN_SLASH: return 0.2f;
	case Pattern::DASH:      return 0.4f;
	case Pattern::WHIRLWIND: return 2.4f;
	case Pattern::SLAM:      return 0.25f;
	default:                 return 0.0f;
	}
}
float SwordBoss::PatternRecover(Pattern p) const
{
	switch (p) {
	case Pattern::FAN_SLASH: return 0.6f;
	case Pattern::DASH:      return 0.7f;
	case Pattern::WHIRLWIND: return 0.9f;   // 회오리 후 어지러움 -> 반격 타이밍
	case Pattern::SLAM:      return 0.9f;
	default:                 return 0.0f;
	}
}

// 거리에 따라 패턴 선택 (같은 패턴 연속은 한 번 다시 뽑음)
void SwordBoss::ChoosePattern(float dist)
{
	auto pick = [dist]() {
		int r = rand() % 3;
		if (dist <= 4.5f) {                                   // 근거리
			const Pattern close[3] = { Pattern::SLAM, Pattern::FAN_SLASH, Pattern::WHIRLWIND };
			return close[r];
		}
		if (dist <= 9.0f) {                                   // 중거리
			const Pattern mid[3] = { Pattern::FAN_SLASH, Pattern::WHIRLWIND, Pattern::DASH };
			return mid[r];
		}
		return Pattern::DASH;                                 // 원거리
	};
	pattern = pick();
	if (pattern == lastPattern && pattern != Pattern::DASH) pattern = pick();
	lastPattern = pattern;
}

void SwordBoss::EnterState(State s)
{
	state = s;
	stateTime = 0.0f;
	if (s == State::TELEGRAPH) {
		atkX = x; atkZ = z; atkYaw = yaw;   // 예고 시작 시 범위 고정
		hitApplied = false;
		dashTravel = 0.0f;
	}
	if (s == State::ACTIVE) {
		tickTimer = 0.0f;                     // 회오리 첫 틱은 바로
	}
	if (s == State::CHASE) {
		pattern = Pattern::NONE;
		chaseTime = 0.0f;
	}
}

//축별 이동 + 블럭 충돌 (이미 겹친 상태면 빠져나갈 수 있게 허용)
void SwordBoss::MoveWithCollision(float dx, float dz)
{
	bool stuck = BossCollides(x, z);
	if (stuck || !BossCollides(x + dx, z)) x += dx;
	if (stuck || !BossCollides(x, z + dz)) z += dz;
	x = fmaxf(-MAP_LIMIT, fminf(MAP_LIMIT, x));
	z = fmaxf(-MAP_LIMIT, fminf(MAP_LIMIT, z));
}

// 업데이트 (상태 머신)
float SwordBoss::Update(float dt, float px, float py, float pz)
{
	stateTime += dt;
	if (hitFlash > 0.0f) hitFlash = fmaxf(0.0f, hitFlash - dt * 4.0f);

	float toX = px - x, toZ = pz - z;
	float dist = sqrtf(toX * toX + toZ * toZ);
	float yawToPlayer = atan2f(toX, toZ);
	float damage = 0.0f;

	switch (state) {
	case State::CHASE: {
		yaw = yawToPlayer;
		if (dist > KEEP_DIST) {
			MoveWithCollision(sinf(yaw) * CHASE_SPEED * dt, cosf(yaw) * CHASE_SPEED * dt);
			walkPhase += dt * 7.0f;
		}
		cooldown -= dt;
		chaseTime += dt;
		if (cooldown <= 0.0f && (dist <= 16.0f || chaseTime > 4.0f)) {
			ChoosePattern(dist);
			EnterState(State::TELEGRAPH);
		}
		break;
	}
	case State::TELEGRAPH:
		// 회오리는 예고 중에도 플레이어를 바라봄 (원형 범위라 방향 고정 불필요)
		if (pattern == Pattern::WHIRLWIND) yaw = yawToPlayer;
		if (stateTime >= PatternTelegraph(pattern)) EnterState(State::ACTIVE);
		break;

	case State::ACTIVE: {
		float activeT = PatternActive(pattern);
		if (pattern == Pattern::DASH) {
			// 고정된 방향으로 직선 돌진. 블럭에 부딪히면 그 자리에서 멈춤
			float step = DASH_LENGTH / activeT * dt;
			if (dashTravel + step > DASH_LENGTH) step = DASH_LENGTH - dashTravel;
			float ox = x, oz = z;
			MoveWithCollision(sinf(atkYaw) * step, cosf(atkYaw) * step);
			float moved = sqrtf((x - ox) * (x - ox) + (z - oz) * (z - oz));
			dashTravel += moved;
			if (moved < step * 0.5f) stateTime = activeT;     // 막힘 -> 돌진 종료
		}
		else if (pattern == Pattern::WHIRLWIND) {
			spinAngle += WHIRL_SPIN * dt;
			tickTimer -= dt;
			if (dist > 0.5f)
				MoveWithCollision(sinf(yawToPlayer) * WHIRL_SPEED * dt, cosf(yawToPlayer) * WHIRL_SPEED * dt);
			atkX = x; atkZ = z;
		}
		damage = CheckHit(px, py, pz);
		if (stateTime >= activeT) EnterState(State::RECOVER);
		break;
	}
	case State::RECOVER:
		if (stateTime >= PatternRecover(pattern)) {
			EnterState(State::CHASE);
			cooldown = ATTACK_GAP + RandomRange(0.0f, 0.8f);
		}
		break;

	case State::DEAD:
		break;
	}

	// 포즈 보간 (공격 순간은 빠르게, 나머지는 부드럽게)
	Pose target = TargetPose();
	float rate = (state == State::ACTIVE) ? 30.0f : (state == State::TELEGRAPH ? 10.0f : 7.0f);
	float k = 1.0f - expf(-rate * dt);
	pose.torsoYaw   = Lerp(pose.torsoYaw,   target.torsoYaw,   k);
	pose.armLPitch  = Lerp(pose.armLPitch,  target.armLPitch,  k);
	pose.armLRoll   = Lerp(pose.armLRoll,   target.armLRoll,   k);
	pose.armRPitch  = Lerp(pose.armRPitch,  target.armRPitch,  k);
	pose.armRRoll   = Lerp(pose.armRRoll,   target.armRRoll,   k);
	pose.swordPitch = Lerp(pose.swordPitch, target.swordPitch, k);
	pose.legLPitch  = Lerp(pose.legLPitch,  target.legLPitch,  k);
	pose.legRPitch  = Lerp(pose.legRPitch,  target.legRPitch,  k);
	pose.lean       = Lerp(pose.lean,       target.lean,       state == State::DEAD ? 1.0f - expf(-3.0f * dt) : k);
	pose.lift       = (pattern == Pattern::SLAM) ? target.lift : Lerp(pose.lift, 0.0f, k);   // 점프는 직접 계산

	if (state != State::ACTIVE || pattern != Pattern::WHIRLWIND) {
		// 회오리 끝나면 가장 가까운 정면 각도로 되돌림
		spinAngle = fmodf(spinAngle, 360.0f);
		spinAngle = Lerp(spinAngle, spinAngle > 180.0f ? 360.0f : 0.0f, k);
	}

	return damage;
}

// ACTIVE 상태의 피격 판정. 반환: 이번 프레임 피해량
float SwordBoss::CheckHit(float px, float py, float pz)
{
	float feet = py - PLAYER_FOOT - GROUND_Y;   // 바닥 기준 플레이어 발 높이

	switch (pattern) {
	case Pattern::FAN_SLASH:
		if (!hitApplied && HitSector(atkX, atkZ, atkYaw, FAN_HALF_DEG * PI / 180.0f, FAN_RADIUS, px, pz)) {
			hitApplied = true;
			return FAN_DAMAGE;
		}
		break;
	case Pattern::DASH:
		// 시작점 ~ 현재 보스 위치까지 지나간 경로 안에 있으면 맞음
		if (!hitApplied && HitRect(atkX, atkZ, atkYaw, dashTravel + BOSS_RADIUS, DASH_HALF_W, px, pz)) {
			hitApplied = true;
			return DASH_DAMAGE;
		}
		break;
	case Pattern::WHIRLWIND:
		if (HitCircle(atkX, atkZ, WHIRL_RADIUS, px, pz)) {
			if (tickTimer <= 0.0f) {
				tickTimer = WHIRL_TICK;
				return WHIRL_DAMAGE;
			}
		}
		break;
	case Pattern::SLAM:
		// 충격파: 점프 중이면 회피
		if (!hitApplied && feet < SLAM_DODGE_H && HitCircle(atkX, atkZ, SLAM_RADIUS, px, pz)) {
			hitApplied = true;
			return SLAM_DAMAGE;
		}
		break;
	default:
		break;
	}
	return 0.0f;
}


// 포즈
SwordBoss::Pose SwordBoss::TargetPose() const
{
	Pose p;
	float swing = sinf(walkPhase);

	switch (state) {
	case State::CHASE:
		p.legLPitch = 30.0f * swing;
		p.legRPitch = -30.0f * swing;
		p.armLPitch = -25.0f * swing;
		p.armRPitch = -15.0f + 15.0f * swing;
		p.swordPitch = -80.0f;
		break;

	case State::TELEGRAPH: {
		float t = Clamp01(stateTime / PatternTelegraph(pattern));
		switch (pattern) {
		case Pattern::FAN_SLASH:   // 허리를 비틀어 검을 뒤로 당김
			p.torsoYaw = 70.0f;
			p.armRPitch = -90.0f; p.armRRoll = -30.0f;
			p.swordPitch = 0.0f;
			p.armLPitch = -20.0f;
			p.legLPitch = -15.0f; p.legRPitch = 15.0f;
			break;
		case Pattern::DASH:        // 몸을 숙이고 검을 뒤로
			p.lean = 20.0f;
			p.armRPitch = 40.0f;
			p.swordPitch = -90.0f;
			p.armLPitch = -40.0f;
			p.legLPitch = -30.0f; p.legRPitch = 25.0f;
			break;
		case Pattern::WHIRLWIND:   // 양팔을 벌리고 반대로 감음
			p.torsoYaw = 90.0f;
			p.armRRoll = -85.0f; p.armLRoll = 85.0f;
			p.swordPitch = 0.0f;
			break;
		case Pattern::SLAM: {      // 검을 머리 위로 든 뒤 뛰어오름
			p.armRPitch = -170.0f; p.armLPitch = -170.0f;
			p.swordPitch = 0.0f;
			p.lean = -10.0f;
			float jt = Clamp01((t - 0.6f) / 0.4f);   // 마지막 40% 동안 점프
			p.lift = SLAM_JUMP_H * sinf(jt * PI * 0.5f);
			break;
		}
		default: break;
		}
		break;
	}

	case State::ACTIVE: {
		float t = Clamp01(stateTime / PatternActive(pattern));
		switch (pattern) {
		case Pattern::FAN_SLASH:   // 70도 -> -70도로 휘두름
			p.torsoYaw = Lerp(70.0f, -70.0f, SmoothStep(t));
			p.armRPitch = -90.0f; p.armRRoll = -30.0f;
			p.swordPitch = 0.0f;
			p.armLPitch = -20.0f;
			p.legLPitch = -15.0f; p.legRPitch = 15.0f;
			break;
		case Pattern::DASH:        // 찌르기 자세로 돌진
			p.lean = 25.0f;
			p.armRPitch = -90.0f;
			p.swordPitch = 0.0f;
			p.armLPitch = 40.0f;
			p.legLPitch = -40.0f; p.legRPitch = 40.0f;
			break;
		case Pattern::WHIRLWIND:   // 실제 회전은 spinAngle
			p.armRRoll = -90.0f; p.armLRoll = 90.0f;
			p.swordPitch = 0.0f;
			p.legLPitch = 10.0f * sinf(stateTime * 20.0f);
			p.legRPitch = -10.0f * sinf(stateTime * 20.0f);
			break;
		case Pattern::SLAM:        // 내려찍기
			p.armRPitch = -40.0f; p.armLPitch = -40.0f;
			p.swordPitch = 0.0f;
			p.lean = 25.0f;
			p.lift = 0.0f;
			p.legLPitch = -30.0f; p.legRPitch = 30.0f;
			break;
		default: break;
		}
		break;
	}

	case State::RECOVER:   // 숨 고르기 (살짝 숙임)
		p.lean = 10.0f;
		p.armRPitch = -10.0f;
		p.swordPitch = -100.0f;
		break;

	case State::DEAD:      // 뒤로 쓰러짐
		p.lean = -90.0f;
		p.armRRoll = -40.0f; p.armLRoll = 40.0f;
		break;
	}
	return p;
}

// 렌더링 - 계층 변환 (발 -> 허리 -> 몸통/머리/어깨 -> 팔 -> 검)
void SwordBoss::DrawPart(GLuint program, Part part, const glm::mat4& model, const glm::mat4& vp,
	const glm::vec3& color)
{
	const Mesh& m = meshes[part];
	glm::mat4 full = model * glm::scale(glm::mat4(1.0f), PART_SIZE[part]) * m.fit;
	glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(full)));
	glm::mat4 mvp = vp * full;

	glm::vec3 base = m.hasMaterial ? glm::vec3(1.0f) : color;   // mtl 색이 있으면 그대로
	glm::vec3 c = glm::mix(base, glm::vec3(1.0f), hitFlash * 0.8f);

	glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(full));
	glUniformMatrix3fv(glGetUniformLocation(program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMat));
	glUniformMatrix4fv(glGetUniformLocation(program, "transform"), 1, GL_FALSE, glm::value_ptr(mvp));
	glUniform3f(glGetUniformLocation(program, "overrideColor"), c.r, c.g, c.b);

	glBindVertexArray(m.vao);
	glDrawArrays(GL_TRIANGLES, 0, m.count);
}

void SwordBoss::Draw(GLuint program, const glm::mat4& proj, const glm::mat4& view)
{
	if (!initialized) return;
	glUseProgram(program);

	const glm::mat4 I(1.0f);
	const glm::vec3 X(1, 0, 0), Y(0, 1, 0), Z(0, 0, 1);
	auto R = [&](float deg, const glm::vec3& axis) { return glm::rotate(I, glm::radians(deg), axis); };
	auto T = [&](float tx, float ty, float tz) { return glm::translate(I, glm::vec3(tx, ty, tz)); };

	glm::mat4 vp = proj * view;

	// 루트: 발 위치, 방향, 몸 전체 기울기 (기울기 축 = 발)
	glm::mat4 root = T(x, GROUND_Y + pose.lift, z) * glm::rotate(I, yaw, Y) * R(pose.lean, X);

	// 다리 (엉덩이 관절 기준 회전, 메쉬는 관절 아래로)
	glm::mat4 legL = root * T(HIP_X, HIP_Y, 0) * R(pose.legLPitch, X) * T(0, -LEG_LEN * 0.5f, 0);
	glm::mat4 legR = root * T(-HIP_X, HIP_Y, 0) * R(pose.legRPitch, X) * T(0, -LEG_LEN * 0.5f, 0);
	DrawPart(program, LEG_L, legL, vp, PART_COLOR[LEG_L]);
	DrawPart(program, LEG_R, legR, vp, PART_COLOR[LEG_R]);

	// 상체: 허리에서 회전 (부채꼴 휘두르기 + 회오리)
	glm::mat4 upper = root * T(0, HIP_Y, 0) * R(pose.torsoYaw + spinAngle, Y) * T(0, -HIP_Y, 0);

	DrawPart(program, BODY,  upper * T(0, 1.85f, 0), vp, PART_COLOR[BODY]);
	DrawPart(program, HEAD,  upper * T(0, 2.9f, 0), vp, PART_COLOR[HEAD]);
	DrawPart(program, VISOR, upper * T(0, 2.95f, 0.42f), vp, PART_COLOR[VISOR]);

	// 팔 (어깨 관절: roll -> pitch)
	glm::mat4 shoulderL = upper * T(SHOULDER_X, SHOULDER_Y, 0) * R(pose.armLRoll, Z) * R(pose.armLPitch, X);
	glm::mat4 shoulderR = upper * T(-SHOULDER_X, SHOULDER_Y, 0) * R(pose.armRRoll, Z) * R(pose.armRPitch, X);
	DrawPart(program, ARM_L, shoulderL * T(0, -ARM_LEN * 0.5f, 0), vp, PART_COLOR[ARM_L]);
	DrawPart(program, ARM_R, shoulderR * T(0, -ARM_LEN * 0.5f, 0), vp, PART_COLOR[ARM_R]);

	// 검: 오른손 끝(손목)에서 swordPitch 만큼 꺾여 팔 방향(-Y)으로 뻗음
	glm::mat4 blade = shoulderR * T(0, -ARM_LEN, 0) * R(pose.swordPitch, X) * T(0, -PART_SIZE[BLADE].y * 0.5f, 0);
	DrawPart(program, BLADE, blade, vp, PART_COLOR[BLADE]);

	// main 의 다른 오브젝트에 영향이 없도록 색 원복
	glUniform3f(glGetUniformLocation(program, "overrideColor"), 1.0f, 1.0f, 1.0f);
	glBindVertexArray(0);
}

// 공격 범위 표시
void SwordBoss::DrawAttackArea(const glm::mat4& proj, const glm::mat4& view)
{
	if (!initialized) return;
	if (state != State::TELEGRAPH && state != State::ACTIVE) return;

	const float fanHalf = FAN_HALF_DEG * PI / 180.0f;
	std::vector<float> outline, fill;   // outline = 전체 범위, fill = 차오르는 부분 / 장판

	if (state == State::TELEGRAPH) {
		float t = Clamp01(stateTime / PatternTelegraph(pattern));
		switch (pattern) {
		case Pattern::FAN_SLASH:
			AddSector(outline, atkX, atkZ, atkYaw, fanHalf, FAN_RADIUS);
			AddSector(fill, atkX, atkZ, atkYaw, fanHalf, FAN_RADIUS * t);
			break;
		case Pattern::DASH:
			AddRect(outline, atkX, atkZ, atkYaw, DASH_LENGTH + BOSS_RADIUS, DASH_HALF_W);
			AddRect(fill, atkX, atkZ, atkYaw, (DASH_LENGTH + BOSS_RADIUS) * t, DASH_HALF_W);
			break;
		case Pattern::WHIRLWIND:
			AddSector(outline, atkX, atkZ, 0.0f, PI, WHIRL_RADIUS);
			AddSector(fill, atkX, atkZ, 0.0f, PI, WHIRL_RADIUS * t);
			break;
		case Pattern::SLAM:
			AddSector(outline, atkX, atkZ, 0.0f, PI, SLAM_RADIUS);
			AddSector(fill, atkX, atkZ, 0.0f, PI, SLAM_RADIUS * t);
			break;
		default: break;
		}
	}
	else {   // ACTIVE: 피격 범위 장판
		switch (pattern) {
		case Pattern::FAN_SLASH: AddSector(fill, atkX, atkZ, atkYaw, fanHalf, FAN_RADIUS); break;
		case Pattern::DASH:      AddRect(fill, atkX, atkZ, atkYaw, dashTravel + BOSS_RADIUS, DASH_HALF_W); break;
		case Pattern::WHIRLWIND: AddSector(fill, atkX, atkZ, 0.0f, PI, WHIRL_RADIUS); break;
		case Pattern::SLAM:      AddSector(fill, atkX, atkZ, 0.0f, PI, SLAM_RADIUS); break;
		default: break;
		}
	}

	//--- GL 상태 저장 후 반투명 설정
	GLint prevProgram = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
	GLboolean prevBlend = glIsEnabled(GL_BLEND);
	GLboolean prevCull = glIsEnabled(GL_CULL_FACE);
	GLboolean prevDepthMask = GL_TRUE;
	glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);   // 위/아래 어느 쪽에서 봐도 보이게
	glDepthMask(GL_FALSE);     // 반투명은 깊이 기록 X (블럭에는 가려짐)

	glUseProgram(areaProgram);
	glm::mat4 vp = proj * view;
	glUniformMatrix4fv(glGetUniformLocation(areaProgram, "uVP"), 1, GL_FALSE, glm::value_ptr(vp));
	GLint colorLoc = glGetUniformLocation(areaProgram, "uColor");
	glBindVertexArray(areaVAO);
	glBindBuffer(GL_ARRAY_BUFFER, areaVBO);

	auto drawList = [&](const std::vector<float>& v, float r, float g, float b, float a) {
		if (v.empty()) return;
		glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_DYNAMIC_DRAW);
		glUniform4f(colorLoc, r, g, b, a);
		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(v.size() / 4));
	};

	if (state == State::TELEGRAPH) {
		drawList(outline, 1.0f, 0.55f, 0.1f, 0.25f);   // 예고 범위 (주황)
		drawList(fill, 1.0f, 0.15f, 0.1f, 0.40f);      // 차오르는 게이지 (빨강)
	}
	else {
		float pulse = 0.65f + 0.15f * sinf(stateTime * 40.0f);
		drawList(fill, 0.9f, 0.0f, 0.0f, pulse);       // 피격 범위 장판
	}

	//--- GL 상태 복원
	glBindVertexArray(0);
	glDepthMask(prevDepthMask);
	if (prevCull) glEnable(GL_CULL_FACE);
	if (!prevBlend) glDisable(GL_BLEND);
	glUseProgram((GLuint)prevProgram);
}
