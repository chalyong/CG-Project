#pragma once

#include <vector>

//--- 맵에 배치되는 랜덤 블럭
struct Block
{
	float x, y, z;   // 중심 좌표
	float w, h, d;   // 크기 (가로 / 높이 / 세로)
};

void InitBlocks(int count);
const std::vector<Block>& GetBlocks();
