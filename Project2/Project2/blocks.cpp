#include "blocks.h"
#include "player.h"

#include <cmath>
#include <cstdlib>

namespace {
	std::vector<Block> g_blocks;

	const float FLOOR_Y = -1.0f;            // 바닥 표면 높이 (main.cpp lData와 동일)
	const float MAP_HALF = 60.0f;           // 소환 범위: 중심에서 +-60
	const float SPAWN_SAFE_RADIUS = 8.0f;   // 시작 지점 주변은 비워둠
	const int   OVERLAP_RETRY = 20;         // 블럭 겹침 회피 시도 횟수

	//--- [lo, hi] 범위의 랜덤 값
	float RandomRange(float lo, float hi)
	{
		return lo + (rand() % 10000) / 9999.0f * (hi - lo);
	}

	//--- XZ 평면에서 두 블럭이 겹치는지
	bool OverlapsXZ(const Block& a, const Block& b)
	{
		const float MARGIN = 0.5f;   // 블럭 사이 최소 간격
		float dx = fabsf(a.x - b.x);
		float dz = fabsf(a.z - b.z);
		return dx < (a.w + b.w) * 0.5f + MARGIN
			&& dz < (a.d + b.d) * 0.5f + MARGIN;
	}
}

void InitBlocks(int count)
{
	g_blocks.clear();

	//--- 블럭 높이는 플레이어 점프 높이의 2배까지
	const float MAX_HEIGHT = Player::JUMP_HEIGHT * 2.0f;

	for (int i = 0; i < count; ++i)
	{
		Block b;
		b.w = RandomRange(1.0f, 5.0f);
		b.d = RandomRange(1.0f, 5.0f);
		b.h = RandomRange(0.5f, MAX_HEIGHT);

		//--- 시작 지점을 피하며 랜덤 위치 + 기존 블럭과 안 겹치는 자리 찾기
		bool placed = false;
		for (int t = 0; t < OVERLAP_RETRY; ++t)
		{
			b.x = RandomRange(-MAP_HALF, MAP_HALF);
			b.z = RandomRange(-MAP_HALF, MAP_HALF);

			if (b.x * b.x + b.z * b.z < SPAWN_SAFE_RADIUS * SPAWN_SAFE_RADIUS)
				continue;   // 시작 지점 근처는 제외

			bool clash = false;
			for (const Block& e : g_blocks) {
				if (OverlapsXZ(b, e)) { clash = true; break; }
			}
			if (clash) continue;

			placed = true;
			break;
		}
		if (!placed) continue;   // 자리를 못 찾으면 이번 블럭은 건너뜀

		//--- 바닥(y = -1) 위에 설치 (중심 좌표라 높이의 절반만큼 올림)
		b.y = FLOOR_Y + b.h * 0.5f;

		g_blocks.push_back(b);
	}
}

const std::vector<Block>& GetBlocks()
{
	return g_blocks;
}
