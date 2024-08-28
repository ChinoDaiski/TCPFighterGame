#pragma once

#include "Singleton.h"

class CTimerManager : public SingletonBase<CTimerManager>
{
public:
	// ��ǥ ������. �ظ��ϸ� 1000�� ����� ������ ��.
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
	}

	// ������ ������ �� ȣ��
	void StartFrame(void)
	{
		//currentTick = timeGetTime();
	}

	// ������ ���� �� ȣ��
	void EndFrame(void)
	{
		// �� ������ ���� Ÿ�� ������ ��ŭ ������ �ð� ����.
		// �� �ð��� ��¥ ������ �帥 �ð��� �����ؾ� ��.
		startServerTime += targetFrameTime;

		// targetFrameTime ��ŭ Sleep()
		int SleepTime = (startServerTime - timeGetTime());
		if (SleepTime <= 0)
		{
			// ��ŵ
			//std::cout << "��ŵ!\n";
		}
		else
		{
			Sleep(SleepTime);
		}

		fps++;

		// 1�ʰ� �����ٸ�
		DWORD passingTime = startServerTime - startFrameTime;
		if (passingTime >= 1000)
		{
			// �ʿ��ϸ� ���⼭ fps ����
			//std::cout << fps << " / " << passingTime << "\n";

			startFrameTime += passingTime;

			fps = 0;
		}
	}

private:
	DWORD targetFPS;			// 1�ʴ� ��ǥ ������
	DWORD targetFrameTime;		// 1�ʴ� �־����� �ð� -> 1000 / targetFPS
	DWORD startFrameTime;		// ����
	DWORD fps;					// ���� ���� fps
	DWORD startServerTime;		// ���� ������ ���۵� �� �ʱ�ȭ�ǰ�, ���Ŀ� �������� ���� �� ���� targetFrameTime ��ŭ ����.
};