#pragma once

class CPlayer;
class CPacket;

typedef struct _tagSession
{
    // ���� ���θ� �Ǻ��ϴ� ����
    bool isAlive;

    // ���� info - ����, ip, port
    USHORT port;
    char IP[16];
    SOCKET sock;
    CRingBuffer recvQ;  // ���ſ� ������
    CRingBuffer sendQ;  // �۽ſ� ������

    // ���� INFO
    UINT16 uid; // ID
    UINT8 flagField;
    CPlayer* pPlayer;
}SESSION;

extern std::list<SESSION*> g_clientList;    // ������ ������ ���ǵ鿡 ���� ����
extern SOCKET g_listenSocket;              // listen ����

extern int g_id;   // ���ǵ鿡 id�� �ο��ϱ� ���� ����. �� ���� 1�� ������Ű�鼭 id�� �ο��Ѵ�. 
extern bool g_bShutdown;   // ���� ������ �������� ���θ� �����ϱ� ���� ����. true�� ������ ������ �����Ѵ�.

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
