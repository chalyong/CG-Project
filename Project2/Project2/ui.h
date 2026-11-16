#pragma once

//--- HP / MP 등 2D 인터페이스(화면 고정 오버레이)
void InitUI();
void DrawUI(float screenW, float screenH,
	float health, float maxHealth,
	float mana, float maxMana);
