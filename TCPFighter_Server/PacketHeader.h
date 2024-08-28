#pragma once

#include "PacketDefine.h"

#define MAX_PACKET_SIZE 10

enum class PACKET_TYPE : BYTE
{
   SC_CREATE_MY_CHARACTER = 0,
   SC_CREATE_OTHER_CHARACTER,
   SC_DELETE_CHARACTER,

   CS_MOVE_START = 10,
   SC_MOVE_START,
   CS_MOVE_STOP,
   SC_MOVE_STOP,

   CS_ATTACK1 = 20,
   SC_ATTACK1,
   CS_ATTACK2,
   SC_ATTACK2,
   CS_ATTACK3,
   SC_ATTACK3,

   SC_DAMAGE = 30,

   CS_SYNC = 250,
   SC_SYNC,

   END
};


#pragma pack(push, 1) // 1����Ʈ ����

//===================================================
// ��Ŷ ���
//===================================================
typedef struct _tagPACKET_HEADER {
    BYTE byCode;    // ��Ŷ �ڵ� 0x89
    BYTE bySize;    // ��Ŷ ������
    BYTE byType;    // ��Ŷ Ÿ��
}PACKET_HEADER;


//===================================================
// Server To Client
//===================================================
// �� ĳ���͸� �����ϴ� ��Ŷ
typedef struct _tagPACKET_SC_CREATE_MY_CHARACTER {
    UINT32 playerID;
    UINT8 direction;
    UINT16 x;
    UINT16 y;
    UINT8 hp;
}PACKET_SC_CREATE_MY_CHARACTER;

// �ٸ� ĳ���͸� �����ϴ� ��Ŷ
typedef struct _tagPACKET_SC_CREATE_OTHER_CHARACTER {
    UINT32 playerID;
    UINT8 direction;
    UINT16 x;
    UINT16 y;
    UINT8 hp;
}PACKET_SC_CREATE_OTHER_CHARACTER;

// ĳ���� ������ �˸��� ��Ŷ
typedef struct _tagPACKET_SC_DELETE_CHARACTER {
    UINT32 playerID;
}PACKET_SC_DELETE_CHARACTER;

// ĳ���� ������ ������ �˸��� ��Ŷ
typedef struct _tagPACKET_SC_MOVE_START {
    UINT32 playerID;
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_SC_MOVE_START;

// ĳ���� �̵� ���� ��Ŷ
typedef struct _tagPACKET_SC_MOVE_STOP {
    UINT32 playerID;
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_SC_MOVE_STOP;

// ���� 1��. ���� �ָ�
typedef struct _tagPACKET_SC_ATTACK1 {
    UINT32 playerID;
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_SC_ATTACK1;

// ���� 2��. ū �ָ�
typedef struct _tagPACKET_SC_ATTACK2 {
    UINT32 playerID;
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_SC_ATTACK2;

// ���� 3��. ������
typedef struct _tagPACKET_SC_ATTACK3 {
    UINT32 playerID;
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_SC_ATTACK3;

// ĳ���� ������ ��Ŷ
typedef struct _tagPACKET_SC_DAMAGE {
    UINT32 attackPlayerID;  // ������ ID
    UINT32 damagedPlayerID; // �ǰ��� ID
    UINT8 damagedHP;    // ������ hp
}PACKET_SC_DAMAGE;



//===================================================
// Client To Server
//===================================================
// �� ĳ���� ������ ������ �˸��� ��Ŷ
typedef struct _tagPACKET_CS_MOVE_START {
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_CS_MOVE_START;

// �� ĳ���� ������ ������ �˸��� ��Ŷ
typedef struct _tagPACKET_CS_MOVE_STOP {
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_CS_MOVE_STOP;

// ���� 1��. ���� �ָ�
typedef struct _tagPACKET_CS_ATTACK1 {
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_CS_ATTACK1;

// ���� 2��. ū �ָ�
typedef struct _tagPACKET_CS_ATTACK2 {
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_CS_ATTACK2;

// ���� 3��. ������
typedef struct _tagPACKET_CS_ATTACK3 {
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_CS_ATTACK3;

#pragma pack(pop) // ����� ���� ���·� ����