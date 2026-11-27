#include "player.h"
#include <cmath>
#include <gl/glm/glm.hpp>

Player::Player()
	: maxHealth(100.0f), health(100.0f)
	, maxMana(60.0f), mana(0.0f), manaRegen(2.0f)
	, x(0.0f), y(0.0f), z(0.0f)
	, moveSpeed(3.0f)
	, vy(0.0f)
{
}

void Player::Move(int dirF, int dirS, float yaw, float deltaTime)
{
	glm::vec3 forward(-sinf(yaw), 0.0f, cosf(yaw));
	glm::vec3 right(-cosf(yaw), 0.0f, -sinf(yaw));

	glm::vec3 move = forward * (float)dirF + right * (float)dirS;

	if (glm::length(move) > 0.0f) {
		move = glm::normalize(move);
	}

	x += move.x * moveSpeed * deltaTime;
	z += move.z * moveSpeed * deltaTime;
}

void Player::Reset()
{
	x = y = z = 0.0f;        // 시작 위치
	health = maxHealth;      // HP 가득
	mana = 0;				 // MP 0
	vy = 0.0f;				 // 수직 속도 초기화
}

void Player::Jump()
{
	if (y <= GROUND_Y) {
		vy = JUMP_POWER; // 점프 속도 설정
	}
}

void Player::Update(int dirF, int dirS, float yaw, float dt)
{
	Move(dirF, dirS, yaw, dt);
	// 중력 적용
	vy -= GRAVITY * dt;
	y += vy * dt;
	// 땅에 닿으면 중력 초기화
	if (y < GROUND_Y) {
		y = GROUND_Y;
		vy = 0.0f;
	}
	// 마나 회복
	mana += manaRegen * dt;
	if (mana > maxMana) mana = maxMana;

}

void Player::UseSkill()
{
	if (mana < maxMana) return;   // 마나가 가득 찬 경우에만 사용 가능

	// TODO: 스킬 내용 작성 예정
	mana = 0.0f;
}