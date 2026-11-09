#include "player.h"
#include <cmath>
#include <gl/glm/glm.hpp>

Player::Player()
	: maxHealth(100.0f), health(100.0f)
	, maxMana(60.0f), mana(0.0f), manaRegen(2.0f)
	, x(0.0f), y(0.0f), z(0.0f)
	, moveSpeed(3.0f)
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