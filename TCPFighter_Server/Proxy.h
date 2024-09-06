#include "Session.h"
#include "Common.h"

typedef struct _tagSession SESSION;

class Proxy
{
public:
    virtual void SC_CREATE_MY_CHARACTER_FOR_All(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y, UINT8 HP);
    virtual void SC_CREATE_MY_CHARACTER_FOR_SINGLE(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y, UINT8 HP);

    virtual void SC_CREATE_OTHER_CHARACTER_FOR_All(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y, UINT8 HP);
    virtual void SC_CREATE_OTHER_CHARACTER_FOR_SINGLE(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y, UINT8 HP);

    virtual void SC_DELETE_CHARACTER_FOR_All(SESSION* pSession, UINT32 ID);
    virtual void SC_DELETE_CHARACTER_FOR_SINGLE(SESSION* pSession, UINT32 ID);

    virtual void SC_MOVE_START_FOR_All(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y);
    virtual void SC_MOVE_START_FOR_SINGLE(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y);

    virtual void SC_MOVE_STOP_FOR_All(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y);
    virtual void SC_MOVE_STOP_FOR_SINGLE(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y);

    virtual void SC_ATTACK1_FOR_All(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y);
    virtual void SC_ATTACK1_FOR_SINGLE(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y);

    virtual void SC_ATTACK2_FOR_All(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y);
    virtual void SC_ATTACK2_FOR_SINGLE(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y);

    virtual void SC_ATTACK3_FOR_All(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y);
    virtual void SC_ATTACK3_FOR_SINGLE(SESSION* pSession, UINT32 ID, UINT8 Direction, UINT16 X, UINT16 Y);

    virtual void SC_DAMAGE_FOR_All(SESSION* pSession, UINT32 AttackID, UINT32 DamageID, UINT8 DamageHP);
    virtual void SC_DAMAGE_FOR_SINGLE(SESSION* pSession, UINT32 AttackID, UINT32 DamageID, UINT8 DamageHP);

};
