#pragma once

#include "Session.h"

class CObject;
class CPlayer;

bool PacketProc(SESSION* pSession, PACKET_TYPE packetType, void* pPacket);

bool netPacketProc_MoveStart(CPlayer* pPlayer, void* pPacket);
bool netPacketProc_MoveStop(CPlayer* pPlayer, void* pPacket);

bool netPacketProc_ATTACK1(CPlayer* pPlayer, void* pPacket);
bool netPacketProc_ATTACK2(CPlayer* pPlayer, void* pPacket);
bool netPacketProc_ATTACK3(CPlayer* pPlayer, void* pPacket);