#pragma once

#include "Common.h"

typedef struct _tagSession SESSION;

class Stub
{
public:
    virtual bool CS_MOVE_START(SESSION* pSession, UINT8 Direction, UINT16 X, UINT16 Y);
    virtual bool CS_MOVE_STOP(SESSION* pSession, UINT8 Direction, UINT16 X, UINT16 Y);
    virtual bool CS_ATTACK1(SESSION* pSession, UINT8 Direction, UINT16 X, UINT16 Y);
    virtual bool CS_ATTACK2(SESSION* pSession, UINT8 Direction, UINT16 X, UINT16 Y);
    virtual bool CS_ATTACK3(SESSION* pSession, UINT8 Direction, UINT16 X, UINT16 Y);

public:
    bool PacketProc(SESSION* pSession, PACKET_TYPE packetType, CPacket* pPacket);
};
