#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

void LoadSound()
{
	//--- BGM
	mciSendString(TEXT("open \"Music/bgm1.mp3\" alias b1"), NULL, 0, NULL);
	//--- 효과음
	mciSendString(TEXT("open \"Music/effect1.wav\" alias e1"), NULL, 0, NULL);
	mciSendString(TEXT("open \"Music/effect2.wav\" alias e2"), NULL, 0, NULL);
}

void PlayBGM()
{
	mciSendString(TEXT("play b1 repeat"), NULL, 0, NULL);
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
		mciSendString(TEXT("seek se1 to start"), NULL, 0, NULL);
		mciSendString(TEXT("play se1"), NULL, 0, NULL);
		break;
	default:
		break;
	}
}