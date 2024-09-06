#include "pch.h"

#include "ServerProxy.h"
#include "ServerStub.h"

#include "Session.h"
#include "Player.h"
#include "Packet.h"

void ServerProxy::SC_CreateMyCharacter_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << playerID;
    Packet << direction;
    Packet << posX;
    Packet << posY;
    Packet << hp;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_CREATE_MY_CHARACTER;

    BroadcastPacket(pSession, &header, &Packet);
}

void ServerProxy::SC_CreateMyCharacter_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << playerID;
    Packet << direction;
    Packet << posX;
    Packet << posY;
    Packet << hp;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_CREATE_MY_CHARACTER;

    UnicastPacket(pSession, &header, &Packet);
}

void ServerProxy::SC_CreateOtherCharacter_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << playerID;
    Packet << direction;
    Packet << posX;
    Packet << posY;
    Packet << hp;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;

    BroadcastPacket(pSession, &header, &Packet);
}

void ServerProxy::SC_CreateOtherCharacter_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << playerID;
    Packet << direction;
    Packet << posX;
    Packet << posY;
    Packet << hp;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;

    UnicastPacket(pSession, &header, &Packet);
}

void ServerProxy::SC_DeleteCharacter_ForAll(SESSION* pSession, UINT32 playerID)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << playerID;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_DELETE_CHARACTER;

    BroadcastPacket(pSession, &header, &Packet);
}

void ServerProxy::SC_DeleteCharacter_ForSingle(SESSION* pSession, UINT32 playerID)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << playerID;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_DELETE_CHARACTER;

    UnicastPacket(pSession, &header, &Packet);
}

void ServerProxy::SC_MoveStart_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << playerID;
    Packet << direction;
    Packet << posX;
    Packet << posY;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_MOVE_START;

    BroadcastPacket(pSession, &header, &Packet);
}

void ServerProxy::SC_MoveStart_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << playerID;
    Packet << direction;
    Packet << posX;
    Packet << posY;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_MOVE_START;

    UnicastPacket(pSession, &header, &Packet);
}

void ServerProxy::SC_MoveStop_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << playerID;
    Packet << direction;
    Packet << posX;
    Packet << posY;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_MOVE_STOP;

    BroadcastPacket(pSession, &header, &Packet);
}

void ServerProxy::SC_MoveStop_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << playerID;
    Packet << direction;
    Packet << posX;
    Packet << posY;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_MOVE_STOP;

    UnicastPacket(pSession, &header, &Packet);
}

void ServerProxy::SC_Attack1_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << playerID;
    Packet << direction;
    Packet << posX;
    Packet << posY;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_ATTACK1;

    BroadcastPacket(pSession, &header, &Packet);
}

void ServerProxy::SC_Attack1_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << playerID;
    Packet << direction;
    Packet << posX;
    Packet << posY;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_ATTACK1;

    UnicastPacket(pSession, &header, &Packet);
}

void ServerProxy::SC_Attack2_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << playerID;
    Packet << direction;
    Packet << posX;
    Packet << posY;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_ATTACK2;

    BroadcastPacket(pSession, &header, &Packet);
}

void ServerProxy::SC_Attack2_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << playerID;
    Packet << direction;
    Packet << posX;
    Packet << posY;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_ATTACK2;

    UnicastPacket(pSession, &header, &Packet);
}

void ServerProxy::SC_Attack3_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << playerID;
    Packet << direction;
    Packet << posX;
    Packet << posY;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_ATTACK3;

    BroadcastPacket(pSession, &header, &Packet);
}

void ServerProxy::SC_Attack3_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << playerID;
    Packet << direction;
    Packet << posX;
    Packet << posY;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_ATTACK3;

    UnicastPacket(pSession, &header, &Packet);
}

void ServerProxy::SC_Damage_ForAll(SESSION* pSession, UINT32 attackPlayerID, UINT32 damagedPlayerID, UINT8 damagedHP)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << attackPlayerID;
    Packet << damagedPlayerID;
    Packet << damagedHP;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_DAMAGE;

    BroadcastPacket(nullptr, &header, &Packet);
}

void ServerProxy::SC_Damage_ForSingle(SESSION* pSession, UINT32 attackPlayerID, UINT32 damagedPlayerID, UINT8 damagedHP)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << attackPlayerID;
    Packet << damagedPlayerID;
    Packet << damagedHP;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_DAMAGE;

    UnicastPacket(nullptr, &header, &Packet);
}
