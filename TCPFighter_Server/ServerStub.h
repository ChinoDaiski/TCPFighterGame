#pragma once

#include "stub.h"

// �������� ����Ǵ� ������ �ڵ��. �̴� Ŭ���̾�Ʈ���� �� ��Ŷ���� ó���ϴ� �κ��̴�.
class ServerStub : public Stub
{
public:
	// Server To Client
	virtual bool CS_MoveStart(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y) override;
	virtual bool CS_MoveStop(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y) override;
	virtual bool CS_ATTACK1(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y) override;
	virtual bool CS_ATTACK2(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y) override;
	virtual bool CS_ATTACK3(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y) override;
};