#pragma once

#include "stub.h"

// 서버에서 실행되는 컨텐츠 코드들. 이는 클라이언트에서 온 패킷들을 처리하는 부분이다.
class ServerStub : public Stub
{
public:
	// Client To Server
	virtual bool CS_MOVE_START(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y) override;
	virtual bool CS_MOVE_STOP(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y) override;
	virtual bool CS_ATTACK1(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y) override;
	virtual bool CS_ATTACK2(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y) override;
	virtual bool CS_ATTACK3(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y) override;
};