#include "pch.h"
#include "Proxy.h"

#include "Session.h"
#include "Packet.h"

void Proxy::SC_CREATE_MY_CHARACTER_FOR_All(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y, UINT8 HP)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << ID;
    Packet << Direction;
    Packet << X;
    Packet << Y;
    Packet << HP;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_CREATE_MY_CHARACTER;

    BroadcastPacket(pSession, &header, &Packet);
}

void Proxy::SC_CREATE_MY_CHARACTER_FOR_SINGLE(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y, UINT8 HP)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << ID;
    Packet << Direction;
    Packet << X;
    Packet << Y;
    Packet << HP;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_CREATE_MY_CHARACTER;

    UnicastPacket(pSession, &header, &Packet);
}

void Proxy::SC_CREATE_OTHER_CHARACTER_FOR_All(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y, UINT8 HP)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << ID;
    Packet << Direction;
    Packet << X;
    Packet << Y;
    Packet << HP;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;

    BroadcastPacket(pSession, &header, &Packet);
}

void Proxy::SC_CREATE_OTHER_CHARACTER_FOR_SINGLE(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y, UINT8 HP)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << ID;
    Packet << Direction;
    Packet << X;
    Packet << Y;
    Packet << HP;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;

    UnicastPacket(pSession, &header, &Packet);
}

void Proxy::SC_DELETE_CHARACTER_FOR_All(SESSION* pSession, UINT32 ID)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << ID;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_DELETE_CHARACTER;

    BroadcastPacket(pSession, &header, &Packet);
}

void Proxy::SC_DELETE_CHARACTER_FOR_SINGLE(SESSION* pSession, UINT32 ID)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << ID;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_DELETE_CHARACTER;

    UnicastPacket(pSession, &header, &Packet);
}

void Proxy::SC_MOVE_START_FOR_All(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << ID;
    Packet << Direction;
    Packet << X;
    Packet << Y;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_MOVE_START;

    BroadcastPacket(pSession, &header, &Packet);
}

void Proxy::SC_MOVE_START_FOR_SINGLE(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << ID;
    Packet << Direction;
    Packet << X;
    Packet << Y;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_MOVE_START;

    UnicastPacket(pSession, &header, &Packet);
}

void Proxy::SC_MOVE_STOP_FOR_All(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << ID;
    Packet << Direction;
    Packet << X;
    Packet << Y;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_MOVE_STOP;

    BroadcastPacket(pSession, &header, &Packet);
}

void Proxy::SC_MOVE_STOP_FOR_SINGLE(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << ID;
    Packet << Direction;
    Packet << X;
    Packet << Y;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_MOVE_STOP;

    UnicastPacket(pSession, &header, &Packet);
}

void Proxy::SC_ATTACK1_FOR_All(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << ID;
    Packet << Direction;
    Packet << X;
    Packet << Y;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_ATTACK1;

    BroadcastPacket(pSession, &header, &Packet);
}

void Proxy::SC_ATTACK1_FOR_SINGLE(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << ID;
    Packet << Direction;
    Packet << X;
    Packet << Y;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_ATTACK1;

    UnicastPacket(pSession, &header, &Packet);
}

void Proxy::SC_ATTACK2_FOR_All(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << ID;
    Packet << Direction;
    Packet << X;
    Packet << Y;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_ATTACK2;

    BroadcastPacket(pSession, &header, &Packet);
}

void Proxy::SC_ATTACK2_FOR_SINGLE(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << ID;
    Packet << Direction;
    Packet << X;
    Packet << Y;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_ATTACK2;

    UnicastPacket(pSession, &header, &Packet);
}

void Proxy::SC_ATTACK3_FOR_All(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << ID;
    Packet << Direction;
    Packet << X;
    Packet << Y;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_ATTACK3;

    BroadcastPacket(pSession, &header, &Packet);
}

void Proxy::SC_ATTACK3_FOR_SINGLE(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << ID;
    Packet << Direction;
    Packet << X;
    Packet << Y;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_ATTACK3;

    UnicastPacket(pSession, &header, &Packet);
}

void Proxy::SC_DAMAGE_FOR_All(SESSION* pSession, UINT32 AttackID, UINT32 DamageID, UINT8 DamageHP)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << AttackID;
    Packet << DamageID;
    Packet << DamageHP;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_DAMAGE;

    BroadcastPacket(pSession, &header, &Packet);
}

void Proxy::SC_DAMAGE_FOR_SINGLE(SESSION* pSession, UINT32 AttackID, UINT32 DamageID, UINT8 DamageHP)
{
    PACKET_HEADER header;
    CPacket Packet;

    Packet << AttackID;
    Packet << DamageID;
    Packet << DamageHP;

    header.byCode = dfNETWORK_PACKET_CODE;
    header.bySize = Packet.GetDataSize();
    header.byType = dfPACKET_SC_DAMAGE;

    UnicastPacket(pSession, &header, &Packet);
}

