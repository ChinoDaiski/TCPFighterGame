#pragma once

typedef struct _tagSession SESSION;

class Stub
{
public:
	// Server To Client
	virtual bool CS_MoveStart(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y);
	virtual bool CS_MoveStop(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y);
	virtual bool CS_ATTACK1(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y);
	virtual bool CS_ATTACK2(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y);
	virtual bool CS_ATTACK3(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y);

public:
	// Packet-Procedure
	bool PacketProc(SESSION* pSession, PACKET_TYPE packetType, CPacket* pPacket);
};