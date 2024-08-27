#pragma once

#include "Singleton.h"

class CTimerManager : public SingletonBase<CTimerManager>
{
public:
	// 목표 프레임. 왠만하면 1000의 약수로 설정할 것.
	void InitTimer(DWORD _targetFPS)
	{
		timeBeginPeriod(1);

		targetFPS = _targetFPS;
		targetFrameTime = 1000 / targetFPS;
	}

	void SetStartServerTime(void)
	{
		startServerTime = timeGetTime();
		fps = 0;
		startFrameTime = startServerTime;
		skipTime = 0;
	}

	// 프레임 시작할 때 호출
	void StartFrame(void)
	{
		//currentTick = timeGetTime();
	}

	// 프레임 끝날 때 호출
	void EndFrame(void)
	{
		// 매 프레임 마다 타겟 프레임 만큼 서버의 시간 증가.
		// 이 시간이 진짜 서버가 흐른 시간과 동일해야 함.
		startServerTime += targetFrameTime;

		// targetFrameTime 만큼 Sleep()
		int SleepTime = (startServerTime - timeGetTime());
		if (SleepTime <= 0)
		{
			// 스킵
			//std::cout << "스킵!\n";
		}
		else
		{
			Sleep(SleepTime);
		}

		fps++;

		// 1초가 지났다면
		DWORD passingTime = startServerTime - startFrameTime;
		if (passingTime >= 1000)
		{
			// 필요하면 여기서 fps 측정
			//std::cout << fps << " / " << passingTime << "\n";

			startFrameTime += passingTime;

			fps = 0;
		}
	}

private:
	DWORD currentTick;			// 현재 틱. 윈도우가 시작된 이후로부터 흐른 시간을 기록해둔 값. TimeGetTime 함수로 값을 가져옴.
								// 이를 Sleep도 오버헤드가 있기에 멈추길 원하는 시간 이상으로 멈춘다. 이를 보정하기 위해 사용되는 값.
	DWORD targetFPS;			// 1초당 목표 프레임
	DWORD targetFrameTime;		// 1초당 주어지는 시간 -> 1000 / targetFPS
	DWORD startFrameTime;		// 시작
	DWORD fps;					// 현재 서버 fps
	DWORD startServerTime;		// 서버 로직이 시작될 때 초기화되고, 이후에 프레임이 지날 때 마다 targetFrameTime 만큼 더함.
	DWORD cmpStartServerTime;	// startServerTime과 비교를 위한 값으로, fps의 정확한 측정을 위해 사용하는 값이다. 
	DWORD skipTime;
};