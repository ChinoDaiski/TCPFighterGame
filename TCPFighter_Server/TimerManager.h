#pragma once

#include "Singleton.h"

class CTimerManager : public SingletonBase<CTimerManager>
{
public:
	explicit CTimerManager() noexcept {}
	~CTimerManager() noexcept {}

	// 복사 생성자와 대입 연산자를 삭제하여 복사 방지
	CTimerManager(const CTimerManager&) = delete;
	CTimerManager& operator=(const CTimerManager&) = delete;

public:
	// 목표 프레임. 왠만하면 1000의 약수로 설정할 것.
	void InitServerFrame(DWORD _targetFPS)
	{
		timeBeginPeriod(1);							// 타이머 정밀도(해상도) 1ms 설정

		m_targetFPS = _targetFPS;					// 목표 초당 프레임
		m_targetFrameTime = 1000 / m_targetFPS;     // 1 프레임에 주어지는 시간
		m_currentServerTime = timeGetTime();		// 전역 서버 시간 설정
	}

	// 프레임 체크하는 함수
	bool isStartFrame(void)
	{
		if (timeGetTime() < (m_currentServerTime + m_targetFrameTime))
			return false;
		
		m_currentServerTime += m_targetFrameTime;

		return true;
	}

private:
	DWORD m_targetFPS;
	DWORD m_targetFrameTime;
	DWORD m_currentServerTime;
};