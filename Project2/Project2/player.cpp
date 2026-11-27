#include "player.h"
#include "blocks.h"
#include <cmath>
#include <gl/glm/glm.hpp>

namespace {
	const float FOOT_OFFSET = 1.0f;   // player.y 에서 발까지의 거리 (캐릭터 모델 하단)
	const float PLAYER_HALF = 0.4f;   // 수평 반경 (충돌/착지 판정)

	//--- XZ 평면에서 블럭과 겹치는지
	bool OverlapXZ(float px, float pz, const Block& b)
	{
		return fabsf(px - b.x) < b.w * 0.5f + PLAYER_HALF
			&& fabsf(pz - b.z) < b.d * 0.5f + PLAYER_HALF;
	}

	//--- 해당 위치로 이동했을 때 블럭에 부딪히는지 (발이 상판보다 낮을 때만)
	bool CollidesAt(float px, float pz, float feetY)
	{
		for (const Block& b : GetBlocks()) {
			float top = BLOCK_BASE_Y + b.h;
			if (feetY >= top - 0.05f) continue;   // 상판보다 높으면 통과
			if (OverlapXZ(px, pz, b)) return true;
		}
		return false;
	}
}
Player::Player()
	: maxHealth(100.0f), health(100.0f)
	, maxMana(60.0f), mana(0.0f), manaRegen(2.0f)
	, x(0.0f), y(0.0f), z(0.0f)
	, moveSpeed(3.0f)
	, vy(0.0f), onGround(true)
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
	mana = 0;                // MP 0
	vy = 0.0f;               // 수직 속도 초기화
	onGround = true;         // 지면 위로 시작
}

void Player::Jump()
{
	if (!onGround) return;   // 지면/발판 위에서만 점프

	vy = JUMP_POWER;         // 상승 속도 부여
}

void Player::Update(int dirF, int dirS, float yaw, float dt)
{
	//--- 수평 이동 (블럭 충돌은 축별로 처리해 미끄러짐 허용)
	float oldX = x, oldZ = z;
	float feet = y - FOOT_OFFSET;   // 이동 시점의 발 높이

	Move(dirF, dirS, yaw, dt);
	float dx = x - oldX, dz = z - oldZ;
	x = oldX; z = oldZ;
	if (!CollidesAt(oldX + dx, oldZ, feet)) x = oldX + dx;
	if (!CollidesAt(x, oldZ + dz, feet)) z = oldZ + dz;

	//--- 중력 적용
	float prevY = y;
	vy -= GRAVITY * dt;
	y += vy * dt;

	//--- 지면/발판 착지 (가장 높은 받침대를 찾아 착지)
	float support = GROUND_Y;
	for (const Block& b : GetBlocks()) {
		if (!OverlapXZ(x, z, b)) continue;
		float topY = BLOCK_BASE_Y + b.h + FOOT_OFFSET;
		if (topY <= prevY + 0.1f && topY > support) support = topY;
	}

	if (y <= support) {
		y = support;
		vy = 0.0f;
		onGround = true;
	}
	else {
		onGround = false;
	}

	//--- 마나 회복
	mana += manaRegen * dt;
	if (mana > maxMana) mana = maxMana;
}

void Player::UseSkill()
{
	if (mana < maxMana) return;   // 마나가 가득 찬 경우에만 사용 가능

	// TODO: 스킬 내용 작성 예정
	mana = 0.0f;
}