#include "pch.h"
#include "makePacket.h"

void mpCreateMyCharacter(PACKET_HEADER* pHeader, PACKET_SC_CREATE_MY_CHARACTER* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp)
{
    pHeader->byCode = dfNETWORK_PACKET_CODE;
    pHeader->bySize = sizeof(PACKET_SC_CREATE_MY_CHARACTER);
    pHeader->byType = dfPACKET_SC_CREATE_MY_CHARACTER;

    pPacket->playerID = playerID;
    pPacket->direction = direction;
    pPacket->x = posX;
    pPacket->y = posY;
    pPacket->hp = hp;
}

void mpCreateOtherCharacter(PACKET_HEADER* pHeader, PACKET_SC_CREATE_OTHER_CHARACTER* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp)
{
    pHeader->byCode = dfNETWORK_PACKET_CODE;
    pHeader->bySize = sizeof(PACKET_SC_CREATE_OTHER_CHARACTER);
    pHeader->byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;

    pPacket->playerID = playerID;
    pPacket->direction = direction;
    pPacket->x = posX;
    pPacket->y = posY;
    pPacket->hp = hp;
}

void mpDeleteCharacter(PACKET_HEADER* pHeader, PACKET_SC_DELETE_CHARACTER* pPacket, UINT32 playerID)
{
    pHeader->byCode = dfNETWORK_PACKET_CODE;
    pHeader->bySize = sizeof(PACKET_SC_DELETE_CHARACTER);
    pHeader->byType = dfPACKET_SC_DELETE_CHARACTER;

    pPacket->playerID = playerID;
}

void mpMoveStart(PACKET_HEADER* pHeader, PACKET_SC_MOVE_START* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    pHeader->byCode = dfNETWORK_PACKET_CODE;
    pHeader->bySize = sizeof(PACKET_SC_MOVE_START);
    pHeader->byType = dfPACKET_SC_MOVE_START;

    pPacket->playerID = playerID;
    pPacket->direction = direction;
    pPacket->x = posX;
    pPacket->y = posY;
}

void mpMoveStop(PACKET_HEADER* pHeader, PACKET_SC_MOVE_STOP* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    pHeader->byCode = dfNETWORK_PACKET_CODE;
    pHeader->bySize = sizeof(PACKET_SC_MOVE_STOP);
    pHeader->byType = dfPACKET_SC_MOVE_STOP;

    pPacket->playerID = playerID;
    pPacket->direction = direction;
    pPacket->x = posX;
    pPacket->y = posY;
}

void mpAttack1(PACKET_HEADER* pHeader, PACKET_SC_ATTACK1* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    pHeader->byCode = dfNETWORK_PACKET_CODE;
    pHeader->bySize = sizeof(PACKET_SC_ATTACK1);
    pHeader->byType = dfPACKET_SC_ATTACK1;

    pPacket->playerID = playerID;
    pPacket->direction = direction;
    pPacket->x = posX;
    pPacket->y = posY;
}

void mpAttack2(PACKET_HEADER* pHeader, PACKET_SC_ATTACK2* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    pHeader->byCode = dfNETWORK_PACKET_CODE;
    pHeader->bySize = sizeof(PACKET_SC_ATTACK2);
    pHeader->byType = dfPACKET_SC_ATTACK2;

    pPacket->playerID = playerID;
    pPacket->direction = direction;
    pPacket->x = posX;
    pPacket->y = posY;
}

void mpAttack3(PACKET_HEADER* pHeader, PACKET_SC_ATTACK3* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    pHeader->byCode = dfNETWORK_PACKET_CODE;
    pHeader->bySize = sizeof(PACKET_SC_ATTACK3);
    pHeader->byType = dfPACKET_SC_ATTACK3;

    pPacket->playerID = playerID;
    pPacket->direction = direction;
    pPacket->x = posX;
    pPacket->y = posY;
}

void mpDamage(PACKET_HEADER* pHeader, PACKET_SC_DAMAGE* pPacket, UINT32 attackPlayerID, UINT32 damagedPlayerID, UINT8 damagedHP)
{
    pHeader->byCode = dfNETWORK_PACKET_CODE;
    pHeader->bySize = sizeof(PACKET_SC_DAMAGE);
    pHeader->byType = dfPACKET_SC_DAMAGE;

    pPacket->attackPlayerID = attackPlayerID;
    pPacket->damagedPlayerID = damagedPlayerID;
    pPacket->damagedHP = damagedHP;
}
