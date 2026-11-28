#pragma once

#include <GL/glew.h>
#include <gl/glm/glm.hpp>
#include <string>



class SwordBoss
{
public:
	enum class Pattern { NONE, FAN_SLASH, DASH, WHIRLWIND, SLAM };
	enum class State { CHASE, TELEGRAPH, ACTIVE, RECOVER, DEAD };

	SwordBoss();

	// 셰이더 프로그램(main의 shaderProgramID)의 attribute 위치를 사용해 OBJ 메쉬 업로드
	void Init(GLuint mainProgram, const std::string& objDir = "Boss/");
	void Reset();

	// 반환값: 이번 프레임에 플레이어가 받은 피해량 (player.health 에서 빼서 사용)
	float Update(float dt, float px, float py, float pz);

	// 로봇 본체 (main 셰이더 사용, 조명 적용)
	void Draw(GLuint mainProgram, const glm::mat4& proj, const glm::mat4& view);
	// 공격 예고 / 피격 범위 장판 (전용 반투명 셰이더). 깊이 테스트가 켜진 상태에서 호출
	void DrawAttackArea(const glm::mat4& proj, const glm::mat4& view);

	// 플레이어 필살기 등에서 사용
	void TakeDamage(float amount);
	bool IsDead() const { return state == State::DEAD; }

	float maxHealth;
	float health;
	float x, z;        // 발 위치 (지면 y = -1)
	float yaw;         // 바라보는 방향 (앞 = (sin yaw, 0, cos yaw))

	State   GetState()   const { return state; }
	Pattern GetPattern() const { return pattern; }

private:
	//--- 메쉬 (OBJ 1개 = 파트 1개)
	struct Mesh {
		GLuint vao = 0, vboPos = 0, vboCol = 0, vboNrm = 0;
		int count = 0;
		bool hasMaterial = false;   // mtl 색이 있으면 파트 색 대신 사용
		glm::mat4 fit{ 1.0f };      // 바운딩 박스를 [-0.5, 0.5]^3 로 맞추는 행렬
	};
	enum Part { BODY, HEAD, VISOR, ARM_L, ARM_R, LEG_L, LEG_R, BLADE, PART_COUNT };

	struct Pose {
		float torsoYaw = 0.0f;                    // 허리 회전
		float armLPitch = 0.0f, armLRoll = 0.0f;  // 어깨 앞뒤 / 옆으로
		float armRPitch = 0.0f, armRRoll = 0.0f;
		float swordPitch = -80.0f;                // 손목 (검 각도)
		float legLPitch = 0.0f, legRPitch = 0.0f;
		float lean = 0.0f;                        // 몸 전체 앞(+)/뒤(-) 기울기
		float lift = 0.0f;                        // 점프 높이
	};

	Mesh meshes[PART_COUNT];
	bool initialized;

	State   state;
	Pattern pattern;
	Pattern lastPattern;
	float stateTime;      // 현재 상태 경과 시간
	float cooldown;       // 다음 공격까지 남은 시간
	float chaseTime;      // 추격만 한 시간 (너무 길면 돌진)
	bool  hitApplied;     // 1회 판정 공격이 이미 맞췄는지
	float tickTimer;      // 회오리 다단 히트 타이머
	float walkPhase;
	float spinAngle;      // 회오리 회전 (스무딩 없이 누적)
	float hitFlash;       // 피격 시 흰색 번쩍임

	// 공격 시작 시 고정되는 정보 (예고와 판정이 같은 범위를 사용)
	float atkX, atkZ;     // 공격 기준 위치
	float atkYaw;         // 공격 방향
	float dashTravel;     // 돌진으로 이동한 거리

	Pose pose;

	//--- 범위 표시용 리소스
	GLuint areaProgram, areaVAO, areaVBO;

	//--- 내부 함수
	void ChoosePattern(float dist);
	void EnterState(State s);
	void MoveWithCollision(float dx, float dz);
	Pose TargetPose() const;
	float PatternTelegraph(Pattern p) const;
	float PatternActive(Pattern p) const;
	float PatternRecover(Pattern p) const;
	float CheckHit(float px, float py, float pz);
	void DrawPart(GLuint program, Part part, const glm::mat4& model, const glm::mat4& vp,
		const glm::vec3& color);
};
