#pragma once

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
	bool onGround;   // 지면/발판 위에 있는지 // 수직 속도

	// 함수
	void Move(int dirF, int dirS, float yaw, float deltaTime);
	void Reset();
	void Jump();
	void Update(int dirF, int dirS, float yaw, float dt);
	void UseSkill();
	void TakeDamage(float amount);
};
