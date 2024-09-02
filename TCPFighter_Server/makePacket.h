#pragma once

class CPacket;

// Make Packet ÇÔ¼öµé
void mpCreateMyCharacter(PACKET_HEADER* pHeader, CPacket* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp);
void mpCreateOtherCharacter(PACKET_HEADER* pHeader, CPacket* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp);
void mpDeleteCharacter(PACKET_HEADER* pHeader, CPacket* pPacket, UINT32 playerID);
void mpMoveStart(PACKET_HEADER* pHeader, CPacket* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
void mpMoveStop(PACKET_HEADER* pHeader, CPacket* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
void mpAttack1(PACKET_HEADER* pHeader, CPacket* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
void mpAttack2(PACKET_HEADER* pHeader, CPacket* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
void mpAttack3(PACKET_HEADER* pHeader, CPacket* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
void mpDamage(PACKET_HEADER* pHeader, CPacket* pPacket, UINT32 attackPlayerID, UINT32 damagedPlayerID, UINT8 damagedHP);