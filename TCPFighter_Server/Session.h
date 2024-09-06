#pragma once

class CPlayer;
class CPacket;

typedef struct _tagSession
{
    // 삭제 여부를 판별하는 변수
    bool isAlive;

    // 세션 info - 소켓, ip, port
    USHORT port;
    char IP[16];
    SOCKET sock;
    CRingBuffer recvQ;  // 수신용 링버퍼
    CRingBuffer sendQ;  // 송신용 링버퍼

    // 유저 INFO
    UINT16 uid; // ID
    UINT8 flagField;
    CPlayer* pPlayer;
}SESSION;

extern std::list<SESSION*> g_clientList;    // 서버에 접속한 세션들에 대한 정보
extern SOCKET g_listenSocket;              // listen 소켓

extern int g_id;   // 세션들에 id를 부여하기 위한 기준. 이 값을 1씩 증가시키면서 id를 부여한다. 
extern bool g_bShutdown;   // 메인 루프가 끝났는지 여부를 결정하기 위한 변수. true면 서버의 루프를 종료한다.

//==========================================================================================================================================
// Broadcast
//==========================================================================================================================================
void BroadcastData(SESSION* excludeSession, PACKET_HEADER* pPacket, UINT8 dataSize);
void BroadcastData(SESSION* excludeSession, CPacket* pPacket, UINT8 dataSize);
void BroadcastPacket(SESSION* excludeSession, PACKET_HEADER* pHeader, CPacket* pPacket);

//==========================================================================================================================================
// Unicast
//==========================================================================================================================================
void UnicastData(SESSION* includeSession, PACKET_HEADER* pPacket, UINT8 dataSize);
void UnicastData(SESSION* includeSession, CPacket* pPacket, UINT8 dataSize);
void UnicastPacket(SESSION* includeSession, PACKET_HEADER* pHeader, CPacket* pPacket);

SESSION* createSession(SOCKET ClientSocket, SOCKADDR_IN ClientAddr, UINT32 id, UINT16 posX, UINT16 posY, UINT8 hp, UINT8 direction);
void NotifyClientDisconnected(SESSION* disconnectedSession);
