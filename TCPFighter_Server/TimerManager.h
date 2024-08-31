#pragma once

#include "Singleton.h"

class CTimerManager : public SingletonBase<CTimerManager>
{
public:
	explicit CTimerManager() noexcept {}
	~CTimerManager() noexcept {}

	// ���� �����ڿ� ���� �����ڸ� �����Ͽ� ���� ����
	CTimerManager(const CTimerManager&) = delete;
	CTimerManager& operator=(const CTimerManager&) = delete;

public:
	// ��ǥ ������. �ظ��ϸ� 1000�� ����� ������ ��.
	void InitServerFrame(DWORD _targetFPS)
	{
		timeBeginPeriod(1);							// Ÿ�̸� ���е�(�ػ�) 1ms ����

		m_targetFPS = _targetFPS;					// ��ǥ �ʴ� ������
		m_targetFrameTime = 1000 / m_targetFPS;     // 1 �����ӿ� �־����� �ð�
		m_currentServerTime = timeGetTime();		// ���� ���� �ð� ����
	}

	// ������ üũ�ϴ� �Լ�
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