#include "pch.h"
#include "makePacket.h"
#include "Packet.h"

void mpCreateMyCharacter(PACKET_HEADER* pHeader, CPacket *pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp)
{
    *pPacket << playerID;
    *pPacket << direction;
    *pPacket << posX;
    *pPacket << posY;
    *pPacket << hp;

    pHeader->byCode = dfNETWORK_PACKET_CODE;
    pHeader->bySize = pPacket->GetDataSize();
    pHeader->byType = dfPACKET_SC_CREATE_MY_CHARACTER;
}

void mpCreateOtherCharacter(PACKET_HEADER* pHeader, CPacket* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp)
{
    *pPacket << playerID;
    *pPacket << direction;
    *pPacket << posX;
    *pPacket << posY;
    *pPacket << hp;

    pHeader->byCode = dfNETWORK_PACKET_CODE;
    pHeader->bySize = pPacket->GetDataSize();
    pHeader->byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;
}

void mpDeleteCharacter(PACKET_HEADER* pHeader, CPacket* pPacket, UINT32 playerID)
{
    *pPacket << playerID;

    pHeader->byCode = dfNETWORK_PACKET_CODE;
    pHeader->bySize = pPacket->GetDataSize();
    pHeader->byType = dfPACKET_SC_DELETE_CHARACTER;
}

void mpMoveStart(PACKET_HEADER* pHeader, CPacket* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    *pPacket << playerID;
    *pPacket << direction;
    *pPacket << posX;
    *pPacket << posY;

    pHeader->byCode = dfNETWORK_PACKET_CODE;
    pHeader->bySize = pPacket->GetDataSize();
    pHeader->byType = dfPACKET_SC_MOVE_START;
}

void mpMoveStop(PACKET_HEADER* pHeader, CPacket* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    *pPacket << playerID;
    *pPacket << direction;
    *pPacket << posX;
    *pPacket << posY;

    pHeader->byCode = dfNETWORK_PACKET_CODE;
    pHeader->bySize = pPacket->GetDataSize();
    pHeader->byType = dfPACKET_SC_MOVE_STOP;
}

void mpAttack1(PACKET_HEADER* pHeader, CPacket* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    *pPacket << playerID;
    *pPacket << direction;
    *pPacket << posX;
    *pPacket << posY;

    pHeader->byCode = dfNETWORK_PACKET_CODE;
    pHeader->bySize = pPacket->GetDataSize();
    pHeader->byType = dfPACKET_SC_ATTACK1;
}

void mpAttack2(PACKET_HEADER* pHeader, CPacket* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    *pPacket << playerID;
    *pPacket << direction;
    *pPacket << posX;
    *pPacket << posY;

    pHeader->byCode = dfNETWORK_PACKET_CODE;
    pHeader->bySize = pPacket->GetDataSize();
    pHeader->byType = dfPACKET_SC_ATTACK2;
}

void mpAttack3(PACKET_HEADER* pHeader, CPacket* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    *pPacket << playerID;
    *pPacket << direction;
    *pPacket << posX;
    *pPacket << posY;

    pHeader->byCode = dfNETWORK_PACKET_CODE;
    pHeader->bySize = pPacket->GetDataSize();
    pHeader->byType = dfPACKET_SC_ATTACK3;
}

void mpDamage(PACKET_HEADER* pHeader, CPacket* pPacket, UINT32 attackPlayerID, UINT32 damagedPlayerID, UINT8 damagedHP)
{
    *pPacket << attackPlayerID;
    *pPacket << damagedPlayerID;
    *pPacket << damagedHP;

    pHeader->byCode = dfNETWORK_PACKET_CODE;
    pHeader->bySize = pPacket->GetDataSize();
    pHeader->byType = dfPACKET_SC_DAMAGE;
}
