#pragma once


//-----------------------------------------------------------------
// 화면 이동영역
//-----------------------------------------------------------------
#define dfRANGE_MOVE_TOP	50
#define dfRANGE_MOVE_LEFT	10
#define dfRANGE_MOVE_RIGHT	630
#define dfRANGE_MOVE_BOTTOM	470

//-----------------------------------------------------------------
// 프레임당 이동 단위
//-----------------------------------------------------------------
#define MOVE_X_PER_FRAME 3
#define MOVE_Y_PER_FRAME 2

//-----------------------------------------------------------------
// 이동 오류체크 범위
//-----------------------------------------------------------------
#define dfERROR_RANGE		50

//-----------------------------------------------------------------
// 공격범위
//-----------------------------------------------------------------
#define dfATTACK1_RANGE_X		80
#define dfATTACK2_RANGE_X		90
#define dfATTACK3_RANGE_X		100
#define dfATTACK1_RANGE_Y		10
#define dfATTACK2_RANGE_Y		10
#define dfATTACK3_RANGE_Y		20


//-----------------------------------------------------------------
// 데미지
//-----------------------------------------------------------------
#define dfATTACK1_DAMAGE	10
#define dfATTACK2_DAMAGE	15
#define dfATTACK3_DAMAGE	20


//-----------------------------------------------------------------
// 네트워크 패킷 코드
//-----------------------------------------------------------------
#define dfNETWORK_PACKET_CODE 0x89


//-----------------------------------------------------------------
// 이동 정보
//-----------------------------------------------------------------
#define dfPACKET_MOVE_DIR_LL					0
#define dfPACKET_MOVE_DIR_LU					1
#define dfPACKET_MOVE_DIR_UU					2
#define dfPACKET_MOVE_DIR_RU					3
#define dfPACKET_MOVE_DIR_RR					4
#define dfPACKET_MOVE_DIR_RD					5
#define dfPACKET_MOVE_DIR_DD					6
#define dfPACKET_MOVE_DIR_LD					7