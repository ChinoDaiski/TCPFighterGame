#include "pch.h"
#include "stub.h"
#include "Packet.h"

bool Stub::CS_MoveStart(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y)
{
    return true;
}

bool Stub::CS_MoveStop(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y)
{
    return true;
}

bool Stub::CS_ATTACK1(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y)
{
    return true;
}

bool Stub::CS_ATTACK2(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y)
{
    return true;
}

bool Stub::CS_ATTACK3(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y)
{
    return true;
}

bool Stub::PacketProc(SESSION* pSession, PACKET_TYPE packetType, CPacket* pPacket)
{
    switch (packetType)
    {
    case PACKET_TYPE::CS_MOVE_START:
    {
        UINT8 direction;
        UINT16 x;
        UINT16 y;

        *pPacket >> direction;
        *pPacket >> x;
        *pPacket >> y;

        CS_MoveStart(pSession, direction, x, y);
    }
    break;
    case PACKET_TYPE::CS_MOVE_STOP:
    {
        UINT8 direction;
        UINT16 x;
        UINT16 y;

        *pPacket >> direction;
        *pPacket >> x;
        *pPacket >> y;

        CS_MoveStop(pSession, direction, x, y);
    }
    break;
    case PACKET_TYPE::CS_ATTACK1:
    {
        UINT8 direction;
        UINT16 x;
        UINT16 y;

        *pPacket >> direction;
        *pPacket >> x;
        *pPacket >> y;

        CS_ATTACK1(pSession, direction, x, y);
    }
    break;
    case PACKET_TYPE::CS_ATTACK2:
    {
        UINT8 direction;
        UINT16 x;
        UINT16 y;

        *pPacket >> direction;
        *pPacket >> x;
        *pPacket >> y;

        CS_ATTACK2(pSession, direction, x, y);
    }
    break;
    case PACKET_TYPE::CS_ATTACK3:
    {
        UINT8 direction;
        UINT16 x;
        UINT16 y;

        *pPacket >> direction;
        *pPacket >> x;
        *pPacket >> y;

        CS_ATTACK3(pSession, direction, x, y);
    }
    break;
    default:
        break;
    }

    return true;
}
