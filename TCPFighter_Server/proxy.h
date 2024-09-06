#pragma once

typedef struct _tagSession SESSION;

class Proxy
{
public:
	// Server To Client
	virtual void SC_CreateMyCharacter_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp);
	virtual void SC_CreateMyCharacter_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp);

	virtual void SC_CreateOtherCharacter_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp);
	virtual void SC_CreateOtherCharacter_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp);

	virtual void SC_DeleteCharacter_ForAll(SESSION* pSession, UINT32 playerID);
	virtual void SC_DeleteCharacter_ForSingle(SESSION* pSession, UINT32 playerID);

	virtual void SC_MoveStart_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
	virtual void SC_MoveStart_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);

	virtual void SC_MoveStop_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
	virtual void SC_MoveStop_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);

	virtual void SC_Attack1_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
	virtual void SC_Attack1_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);

	virtual void SC_Attack2_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
	virtual void SC_Attack2_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);

	virtual void SC_Attack3_ForAll(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
	virtual void SC_Attack3_ForSingle(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);

	virtual void SC_Damage_ForAll(SESSION* pSession, UINT32 attackPlayerID, UINT32 damagedPlayerID, UINT8 damagedHP);
	virtual void SC_Damage_ForSingle(SESSION* pSession, UINT32 attackPlayerID, UINT32 damagedPlayerID, UINT8 damagedHP);
};


/*
virtual void SC_CreateMyCharacter(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp);
virtual void SC_CreateOtherCharacter(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp);
virtual void SC_DeleteCharacter(SESSION* pSession, UINT32 playerID);
virtual void SC_MoveStart(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
virtual void SC_MoveStop(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
virtual void SC_Attack1(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
virtual void SC_Attack2(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
virtual void SC_Attack3(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
virtual void SC_Damage(SESSION* pSession, UINT32 attackPlayerID, UINT32 damagedPlayerID, UINT8 damagedHP);
*/

/*
{
	[Client] [Server]
	MoveStart(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y);
	[Client] [Server]
	MoveStop(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y);
	[Client] [Server]
	ATTACK1(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y);
	[Client] [Server]
	ATTACK2(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y);
	[Client] [Server]
	ATTACK3(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y);


	[Server] [Client]
	CreateMyCharacter(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp);
	[Server] [Client]
	CreateOtherCharacter(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY, UINT8 hp);
	[Server] [Client]
	DeleteCharacter(SESSION* pSession, UINT32 playerID);
	[Server] [Client]
	MoveStart(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
	[Server] [Client]
	MoveStop(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
	[Server] [Client]
	Attack1(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
	[Server] [Client]
	Attack2(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
	[Server] [Client]
	Attack3(SESSION* pSession, UINT32 playerID, UINT8 direction, UINT16 posX, UINT16 posY);
	[Server] [Client]
	Damage(SESSION* pSession, UINT32 attackPlayerID, UINT32 damagedPlayerID, UINT8 damagedHP);
}
*/