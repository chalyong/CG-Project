#include "player.h"
#include "blocks.h"
#include "boss.h"
#include <cmath>
#include <gl/glm/glm.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include <gl/glm/gtc/type_ptr.hpp>

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
	, maxMana(60.0f), mana(60.0f), manaRegen(2.0f)
	, x(0.0f), y(0.0f), z(0.0f)
	, moveSpeed(3.0f)
	, vy(0.0f), onGround(true), yaw(0.0f), walkPhase(0.0f), swing(0.0f), spin(0.0f)
	, skillState(SkillState::NONE), skillTimer(0.0f)
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
	onGround = true;
	yaw = 0.0f;             // 시점 초기화
	walkPhase = 0.0f;        // 걷기 스윙 초기화
	swing = 0.0f;         // 지면 위로 시작
	spin = 0.0f;          // 스킬 회전 초기화
	skillState = SkillState::NONE;   // 스킬 상태 초기화
	skillTimer = 0.0f;
}

void Player::Jump()
{
	if (!onGround) return;   // 지면/발판 위에서만 점프

	vy = JUMP_POWER;         // 상승 속도 부여
}

void Player::Update(int dirF, int dirS, float yaw, float dt)
{
	this->yaw = yaw;   // Draw용 방향 저장

	//--- 걷기 스윙 갱신 (이동 중에만 진폭 유지)
	float targetSwing = (dirF != 0 || dirS != 0) ? 1.0f : 0.0f;
	float swingStep = dt * 10.0f;
	if (swingStep > 1.0f) swingStep = 1.0f;
	swing += (targetSwing - swing) * swingStep;
	if (targetSwing > 0.0f) walkPhase += dt * 10.0f;
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

bool Player::UseSkill()
{
	if (mana < maxMana) return false;                  // 마나가 가득 찬 경우에만 발동
	if (skillState != SkillState::NONE) return false;  // 이미 시전 중

	mana = 0.0f;
	skillState = SkillState::FREEZE;
	skillTimer = 0.0f;
	spin = 0.0f;
	return true;
}

void Player::TakeDamage(float amount)
{
	health -= amount;
	if (health < 0.0f) health = 0.0f;
}

void Player::Draw(GLuint mainProgram, const glm::mat4& proj, const glm::mat4& view)
{
	glUseProgram(mainProgram);

	//--- 포즈 각도(도): 다리는 엉덩이, 팔은 어깨 관절 기준
	float legL, legR, armL, armR;
	if (!onGround) {
		legL = 25.0f;  legR = -25.0f;    // 점프: 다리 벌림
		armL = -25.0f; armR = -25.0f;    // 점프: 팔 앞으로
	}
	else {
		float step = sinf(walkPhase) * swing;
		legL = 25.0f * step;   legR = -25.0f * step;
		armL = -20.0f * step;  armR = 20.0f * step;
	}

	const glm::mat4 I(1.0f);
	const glm::vec3 AX(1.0f, 0.0f, 0.0f);
	auto R = [&](float deg, const glm::vec3& axis) { return glm::rotate(I, glm::radians(deg), axis); };
	auto T = [&](const glm::vec3& v) { return glm::translate(I, v); };
	auto S = [&](const glm::vec3& v) { return glm::scale(I, v); };

	//--- 룰트: 발 위치 + 방향
	glm::mat4 root = glm::translate(I, glm::vec3(x, y, z)) * glm::rotate(I, glm::radians(spin) - yaw, glm::vec3(0.0f, 1.0f, 0.0f));

	GLint modelLoc     = glGetUniformLocation(mainProgram, "model");
	GLint normalLoc    = glGetUniformLocation(mainProgram, "normalMatrix");
	GLint transformLoc = glGetUniformLocation(mainProgram, "transform");
	GLint colorLoc     = glGetUniformLocation(mainProgram, "overrideColor");
	glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);   // 기본 흰색 (틴트 없음)

	//--- 파트 하나 그리기
	auto DrawPart = [&](const glm::mat4& local) {
		glm::mat4 model = root * local;
		glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(model)));
		glm::mat4 mvp = proj * view * model;
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix3fv(normalLoc, 1, GL_FALSE, glm::value_ptr(normalMat));
		glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(mvp));
		glDrawArrays(GL_TRIANGLES, 0, 36);
	};

	// 머리 / 몸통 (전체 높이 -1.0 ~ +0.5 유지)
	DrawPart(T(glm::vec3(0.0f, 0.32f, 0.0f)) * S(glm::vec3(0.36f, 0.36f, 0.24f)));
	DrawPart(T(glm::vec3(0.0f, -0.21f, 0.0f)) * S(glm::vec3(0.52f, 0.68f, 0.24f)));

	// 다리 (엉덩이 pivot y = -0.55, 관절 아래 -0.225 -> 발바닥 -1.0)
	DrawPart(T(glm::vec3(0.13f, -0.55f, 0.0f)) * R(legL, AX)
		* T(glm::vec3(0.0f, -0.225f, 0.0f)) * S(glm::vec3(0.20f, 0.45f, 0.20f)));
	DrawPart(T(glm::vec3(-0.13f, -0.55f, 0.0f)) * R(legR, AX)
		* T(glm::vec3(0.0f, -0.225f, 0.0f)) * S(glm::vec3(0.20f, 0.45f, 0.20f)));

	// 팔 (어깨 pivot y = 0.10)
	glm::mat4 shoulderL = T(glm::vec3(0.32f, 0.10f, 0.0f)) * R(armL, AX);
	glm::mat4 shoulderR = T(glm::vec3(-0.32f, 0.10f, 0.0f)) * R(armR, AX);
	DrawPart(shoulderL * T(glm::vec3(0.0f, -0.275f, 0.0f)) * S(glm::vec3(0.16f, 0.55f, 0.16f)));
	DrawPart(shoulderR * T(glm::vec3(0.0f, -0.275f, 0.0f)) * S(glm::vec3(0.16f, 0.55f, 0.16f)));

	// 검 (오른손에 종속, 앞으로 뻗음)
	DrawPart(shoulderR * T(glm::vec3(0.0f, -0.50f, 0.45f)) * S(glm::vec3(0.08f, 0.14f, 0.85f)));
}

void Player::UpdateSkill(float dt, SwordBoss& boss)
{
	if (skillState == SkillState::NONE) return;

	skillTimer += dt;

	//--- FREEZE: 1초간 모든 객체 정지 (갱신은 main에서 생략)
	if (skillState == SkillState::FREEZE) {
		if (skillTimer >= SKILL_FREEZE_TIME) {
			skillState = SkillState::SPIN;
			skillTimer = 0.0f;
		}
		return;
	}

	//--- SPIN: 캐릭터가 0.5초에 한 바퀴
	spin = (skillTimer / SKILL_SPIN_TIME) * 360.0f;
	if (skillTimer >= SKILL_SPIN_TIME) {
		spin = 0.0f;
		skillState = SkillState::NONE;

		//--- 범위 공격 판정 (반경 안의 적)
		float dx = boss.x - x;
		float dz = boss.z - z;
		if (dx * dx + dz * dz <= SKILL_RADIUS * SKILL_RADIUS && !boss.IsDead()) {
			boss.CancelPattern();        // 적의 패턴 취소
			boss.TakeDamage(SKILL_DAMAGE);   // 60 피해
		}
	}
}

bool Player::IsSkillFrozen() const
{
	return skillState == SkillState::FREEZE;
}

float Player::GetSkillProgress() const
{
	if (skillState != SkillState::FREEZE) return 0.0f;
	float p = skillTimer / SKILL_FREEZE_TIME;
	return (p > 1.0f) ? 1.0f : p;   // 프레임 오버런 방지 -> 정확히 1바퀴
}