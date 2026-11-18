#pragma once

//--- HP / MP 등 2D 인터페이스(화면 고정 오버레이)
void InitUI();
void DrawUI(float screenW, float screenH,
	float health, float maxHealth,
	float mana, float maxMana);

//--- 오버레이 공용 도형
void DrawBox(float x, float y, float w, float h,
	float r, float g, float b, float a);

//--- 모드별 화면
void DrawTitleScreen(float screenW, float screenH);
void DrawEndScreen(float screenW, float screenH, bool clear);