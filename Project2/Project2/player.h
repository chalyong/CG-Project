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
	float vy; // 수직 속도

	// 함수
	void Move(int dirF, int dirS, float yaw, float deltaTime);
	void Reset();
	void Jump();
	void Update(int dirF, int dirS, float yaw, float dt);
};
