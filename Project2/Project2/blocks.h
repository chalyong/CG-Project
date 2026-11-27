#pragma once

#include <vector>

//--- 맵에 배치되는 랜덤 블럭
struct Block
{
	float x, y, z;   // 중심 좌표
	float w, h, d;   // 크기 (가로 / 높이 / 세로)
};

//--- 블럭이 놓이는 바닥 표면 높이 (main.cpp lData와 동일)
constexpr float BLOCK_BASE_Y = -1.0f;

void InitBlocks(int count);
const std::vector<Block>& GetBlocks();
