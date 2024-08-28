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


#pragma pack(push, 1) // 1바이트 정렬

//===================================================
// 패킷 헤더
//===================================================
typedef struct _tagPACKET_HEADER {
    BYTE byCode;    // 패킷 코드 0x89
    BYTE bySize;    // 패킷 사이즈
    BYTE byType;    // 패킷 타입
}PACKET_HEADER;


//===================================================
// Server To Client
//===================================================
// 내 캐릭터를 생성하는 패킷
typedef struct _tagPACKET_SC_CREATE_MY_CHARACTER {
    UINT32 playerID;
    UINT8 direction;
    UINT16 x;
    UINT16 y;
    UINT8 hp;
}PACKET_SC_CREATE_MY_CHARACTER;

// 다른 캐릭터를 생성하는 패킷
typedef struct _tagPACKET_SC_CREATE_OTHER_CHARACTER {
    UINT32 playerID;
    UINT8 direction;
    UINT16 x;
    UINT16 y;
    UINT8 hp;
}PACKET_SC_CREATE_OTHER_CHARACTER;

// 캐릭터 삭제를 알리는 패킷
typedef struct _tagPACKET_SC_DELETE_CHARACTER {
    UINT32 playerID;
}PACKET_SC_DELETE_CHARACTER;

// 캐릭터 움직임 시작을 알리는 패킷
typedef struct _tagPACKET_SC_MOVE_START {
    UINT32 playerID;
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_SC_MOVE_START;

// 캐릭터 이동 중지 패킷
typedef struct _tagPACKET_SC_MOVE_STOP {
    UINT32 playerID;
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_SC_MOVE_STOP;

// 공격 1번. 작은 주먹
typedef struct _tagPACKET_SC_ATTACK1 {
    UINT32 playerID;
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_SC_ATTACK1;

// 공격 2번. 큰 주먹
typedef struct _tagPACKET_SC_ATTACK2 {
    UINT32 playerID;
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_SC_ATTACK2;

// 공격 3번. 발차기
typedef struct _tagPACKET_SC_ATTACK3 {
    UINT32 playerID;
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_SC_ATTACK3;

// 캐릭터 데미지 패킷
typedef struct _tagPACKET_SC_DAMAGE {
    UINT32 attackPlayerID;  // 공격자 ID
    UINT32 damagedPlayerID; // 피격자 ID
    UINT8 damagedHP;    // 피해자 hp
}PACKET_SC_DAMAGE;



//===================================================
// Client To Server
//===================================================
// 내 캐릭터 움직임 시작을 알리는 패킷
typedef struct _tagPACKET_CS_MOVE_START {
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_CS_MOVE_START;

// 내 캐릭터 움직임 멈춤을 알리는 패킷
typedef struct _tagPACKET_CS_MOVE_STOP {
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_CS_MOVE_STOP;

// 공격 1번. 작은 주먹
typedef struct _tagPACKET_CS_ATTACK1 {
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_CS_ATTACK1;

// 공격 2번. 큰 주먹
typedef struct _tagPACKET_CS_ATTACK2 {
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_CS_ATTACK2;

// 공격 3번. 발차기
typedef struct _tagPACKET_CS_ATTACK3 {
    UINT8 direction;
    UINT16 x;
    UINT16 y;
}PACKET_CS_ATTACK3;

#pragma pack(pop) // 저장된 정렬 상태로 복원