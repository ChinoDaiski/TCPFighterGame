#pragma once

// Make Packet ÇÔ¼öµé
void mpCreateMyCharacter(PACKET_HEADER* pHeader, PACKET_SC_CREATE_MY_CHARACTER* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp);
void mpCreateOtherCharacter(PACKET_HEADER* pHeader, PACKET_SC_CREATE_OTHER_CHARACTER* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp);
void mpDeleteCharacter(PACKET_HEADER* pHeader, PACKET_SC_DELETE_CHARACTER* pPacket, UINT32 playerID);

void mpMoveStart(PACKET_HEADER* pHeader, PACKET_SC_MOVE_START* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
void mpMoveStop(PACKET_HEADER* pHeader, PACKET_SC_MOVE_STOP* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);

void mpAttack1(PACKET_HEADER* pHeader, PACKET_SC_ATTACK1* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
void mpAttack2(PACKET_HEADER* pHeader, PACKET_SC_ATTACK2* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
void mpAttack3(PACKET_HEADER* pHeader, PACKET_SC_ATTACK3* pPacket, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);

void mpDamage(PACKET_HEADER* pHeader, PACKET_SC_DAMAGE* pPacket, UINT32 attackPlayerID, UINT32 damagedPlayerID, UINT8 damagedHP);