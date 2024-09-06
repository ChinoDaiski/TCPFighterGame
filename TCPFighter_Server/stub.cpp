#include "pch.h"
#include "Stub.h"
#include "Packet.h"

bool Stub::CS_MOVE_START(SESSION* pSession, UINT8 Direction, UINT16 X, UINT16 Y)
{
    return true;
}

bool Stub::CS_MOVE_STOP(SESSION* pSession, UINT8 Direction, UINT16 X, UINT16 Y)
{
    return true;
}

bool Stub::CS_ATTACK1(SESSION* pSession, UINT8 Direction, UINT16 X, UINT16 Y)
{
    return true;
}

bool Stub::CS_ATTACK2(SESSION* pSession, UINT8 Direction, UINT16 X, UINT16 Y)
{
    return true;
}

bool Stub::CS_ATTACK3(SESSION* pSession, UINT8 Direction, UINT16 X, UINT16 Y)
{
    return true;
}

bool Stub::PacketProc(SESSION* pSession, PACKET_TYPE packetType, CPacket* pPacket)
{
    switch (packetType)
    {
    case PACKET_TYPE::CS_MOVE_START:
    {
        UINT8 Direction;
        UINT16 X;
        UINT16 Y;

        *pPacket >> Direction;
        *pPacket >> X;
        *pPacket >> Y;

        CS_MOVE_START(pSession, Direction, X, Y);
    }
    break;
    case PACKET_TYPE::CS_MOVE_STOP:
    {
        UINT8 Direction;
        UINT16 X;
        UINT16 Y;

        *pPacket >> Direction;
        *pPacket >> X;
        *pPacket >> Y;

        CS_MOVE_STOP(pSession, Direction, X, Y);
    }
    break;
    case PACKET_TYPE::CS_ATTACK1:
    {
        UINT8 Direction;
        UINT16 X;
        UINT16 Y;

        *pPacket >> Direction;
        *pPacket >> X;
        *pPacket >> Y;

        CS_ATTACK1(pSession, Direction, X, Y);
    }
    break;
    case PACKET_TYPE::CS_ATTACK2:
    {
        UINT8 Direction;
        UINT16 X;
        UINT16 Y;

        *pPacket >> Direction;
        *pPacket >> X;
        *pPacket >> Y;

        CS_ATTACK2(pSession, Direction, X, Y);
    }
    break;
    case PACKET_TYPE::CS_ATTACK3:
    {
        UINT8 Direction;
        UINT16 X;
        UINT16 Y;

        *pPacket >> Direction;
        *pPacket >> X;
        *pPacket >> Y;

        CS_ATTACK3(pSession, Direction, X, Y);
    }
    break;
    default:
        break;
    }
    return true;
}
