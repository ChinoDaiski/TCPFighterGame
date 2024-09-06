#pragma once

#include "proxy.h"

class ServerProxy : public Proxy
{
public:
	// Server To Client
	virtual void SC_CreateMyCharacter_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp) override;
	virtual void SC_CreateMyCharacter_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp) override;

	virtual void SC_CreateOtherCharacter_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp) override;
	virtual void SC_CreateOtherCharacter_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp) override;

	virtual void SC_DeleteCharacter_ForAll(SESSION* pSession, UINT32 playerID) override;
	virtual void SC_DeleteCharacter_ForSingle(SESSION* pSession, UINT32 playerID) override;

	virtual void SC_MoveStart_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY) override;
	virtual void SC_MoveStart_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY) override;

	virtual void SC_MoveStop_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY) override;
	virtual void SC_MoveStop_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY) override;

	virtual void SC_Attack1_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY) override;
	virtual void SC_Attack1_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY) override;

	virtual void SC_Attack2_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY) override;
	virtual void SC_Attack2_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY) override;

	virtual void SC_Attack3_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY) override;
	virtual void SC_Attack3_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY) override;

	virtual void SC_Damage_ForAll(SESSION* pSession, UINT32 attackPlayerID, UINT32 damagedPlayerID, UINT8 damagedHP) override;
	virtual void SC_Damage_ForSingle(SESSION* pSession, UINT32 attackPlayerID, UINT32 damagedPlayerID, UINT8 damagedHP) override;
};