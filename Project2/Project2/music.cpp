#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

void LoadSound()
{
	//--- BGM
	mciSendString(TEXT("open \"Music/bgm1.wav\" alias b1"), NULL, 0, NULL);
	//--- 효과음
	//mciSendString(TEXT("open \"Music/effect1.wav\" alias e1"), NULL, 0, NULL);
	//mciSendString(TEXT("open \"Music/effect2.wav\" alias e2"), NULL, 0, NULL);
}

void PlayBGM()
{
	mciSendString(TEXT("play b1"), NULL, 0, NULL);
}

void StopBGM()
{
	mciSendString(TEXT("stop b1"), NULL, 0, NULL);
}

void PlayEffect(int effectType)
{
	switch (effectType)
	{
	case 1:
		//mciSendString(TEXT("seek e1 to start"), NULL, 0, NULL);
		//mciSendString(TEXT("play e1"), NULL, 0, NULL);
		break;
	default:
		break;
	}
}

void UpdateBGM()
{
	//--- waveaudio 장치는 repeat 미지원 -> 끝나면 다시 재생
	static ULONGLONG lastCheck = 0;
	ULONGLONG now = GetTickCount64();
	if (now - lastCheck < 500) return;   // 0.5초마다 확인
	lastCheck = now;

	TCHAR mode[64] = { 0 };
	if (mciSendString(TEXT("status b1 mode"), mode, 64, NULL) != 0) return;
	if (lstrcmpi(mode, TEXT("stopped")) == 0)
		mciSendString(TEXT("play b1"), NULL, 0, NULL);
}