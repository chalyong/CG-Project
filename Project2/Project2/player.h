#pragma once

#include <GL/glew.h>
#include <gl/glm/glm.hpp>

class SwordBoss;   // 전방 선언 (UpdateSkill의 인자)

class Player
{
public:
	Player();

	// 최대 체력 / 현재 체력
	float maxHealth;
	float health;

	// 최대 마나 / 현재 마나 / 마나 회복량
	float maxMana;
	float mana;
	float manaRegen;

	// 위치
	float x, y, z;
	float moveSpeed;
	//--- 물리 상수
	static constexpr float GRAVITY = 18.0f;
	static constexpr float JUMP_POWER = 7.0f;
	static constexpr float GROUND_Y = 0.0f;
	static constexpr float JUMP_HEIGHT = JUMP_POWER * JUMP_POWER / (2.0f * GRAVITY);   // 점프 최고 높이 (약 1.36)

	float vy;
	bool onGround;   // 지면/발판 위에 있는지
	float yaw;         // 현재 바라보는 방향 (Update에서 갱신)
	float walkPhase;   // 걷기 스윙 위상
	float swing;       // 걷기 스윙 진폭 (0 = 정지)
	float spin;         // 스킬 회전 각도(도, 0~360)

	//--- 스킬 (시간 정지 + 범위 공격)
	enum class SkillState { NONE, FREEZE, SPIN };
	static constexpr float SKILL_FREEZE_TIME = 2.0f;   // 정지 지속 시간
	static constexpr float SKILL_SPIN_TIME   = 0.5f;   // 캐릭터 한 바퀴 회전 시간
	static constexpr float SKILL_RADIUS      = 3.2f;   // 범위 공격 반경
	static constexpr float SKILL_DAMAGE      = 60.0f;  // 범위 공격 피해
	SkillState skillState;
	float skillTimer;
	// 함수
	void Move(int dirF, int dirS, float yaw, float deltaTime);
	void Reset();
	void Jump();
	void Update(int dirF, int dirS, float yaw, float dt);
	bool UseSkill();
	void UpdateSkill(float dt, SwordBoss& boss);   // 스킬 진행 + 범위 공격 판정
	bool IsSkillFrozen() const;                    // 정지(공전) 중인지
	float GetSkillProgress() const;                // 정지 진행률 0~1 (카메라 공전용)
	void TakeDamage(float amount);
	void Draw(GLuint mainProgram, const glm::mat4& proj, const glm::mat4& view);
};
