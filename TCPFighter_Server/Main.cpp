

//===================================================
// pch
//===================================================
#include "pch.h"

//===================================================
// �Ŵ��� class
//===================================================
#include "WinSockManager.h" // ������ ����
#include "ObjectManager.h"
#include "TimerManager.h"
//===================================================

//===================================================
// ������Ʈ
//===================================================
#include "Player.h"





//#define SERVERIP "192.168.30.16"
#define SERVERIP "127.0.0.1"
#define SERVERPORT 5000


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

std::list<SESSION*> g_clientList;    // ������ ������ ���ǵ鿡 ���� ����
SOCKET g_listenSocket;              // listen ����

int g_id;   // ���ǵ鿡 id�� �ο��ϱ� ���� ����. �� ���� 1�� ������Ű�鼭 id�� �ο��Ѵ�. 
bool g_bShutdown = false;   // ���� ������ �������� ���θ� �����ϱ� ���� ����. true�� ������ ������ �����Ѵ�.



//==========================================================================================================================================
// Broadcast
//==========================================================================================================================================
void BroadcastData(SESSION* excludeSession, void* pMsg, UINT8 msgSize);
void BroadcastPacket(SESSION* excludeSession, PACKET_HEADER* pHeader, void* pPacket);

//==========================================================================================================================================
// Unicast
//==========================================================================================================================================
void UnicastData(SESSION* includeSession, void* pMsg, UINT8 msgSize);
void UnicastPacket(SESSION* excludeSession, PACKET_HEADER* pHeader, void* pPacket);

SESSION* createSession(SOCKET ClientSocket, SOCKADDR_IN ClientAddr, int id, int posX, int posY, int hp, int direction);
void NotifyClientDisconnected(SESSION* disconnectedSession);




void netIOProcess(void);

void netProc_Recv(SESSION* pSession);
void netProc_Accept();
void netProc_Send(SESSION* pSession);

bool PacketProc(SESSION* pSession, PACKET_TYPE packetType, void* pPacket);

bool netPacketProc_MoveStart(SESSION* pSession, void* pPacket);
bool netPacketProc_MoveStop(SESSION* pSession, void* pPacket);
bool netPacketProc_ATTACK1(SESSION* pSession, void* pPacket);
bool netPacketProc_ATTACK2(SESSION* pSession, void* pPacket);
bool netPacketProc_ATTACK3(SESSION* pSession, void* pPacket);

void Update(void);

DWORD g_targetFPS;			    // 1�ʴ� ��ǥ ������
DWORD g_targetFrame;		    // 1�ʴ� �־����� �ð� -> 1000 / targetFPS
DWORD g_currentServerTime;		// ���� ������ ���۵� �� �ʱ�ȭ�ǰ�, ���Ŀ� �������� ���� �� ���� targetFrameTime ��ŭ ����.

int main()
{
    //=====================================================================================================================================
    // listen ���� �غ�
    //=====================================================================================================================================
    CWinSockManager<SESSION>& winSockManager = CWinSockManager<SESSION>::getInstance();

    UINT8 options = 0;
    options |= OPTION_NONBLOCKING;

    winSockManager.StartServer(PROTOCOL_TYPE::TCP_IP, SERVERPORT, options);
    g_listenSocket = winSockManager.GetListenSocket();

    //=====================================================================================================================================
    // ���� �ð� ����
    //=====================================================================================================================================
    timeBeginPeriod(1);                     // Ÿ�̸� ���е�(�ػ�) 1ms ����

    g_targetFPS = 50;                       // ��ǥ �ʴ� ������
    g_targetFrame = 1000 / g_targetFPS;     // 1 �����ӿ� �־����� �ð�
    g_currentServerTime = timeGetTime();    // ���� ���� �ð� ����

    //=====================================================================================================================================
    // ���� ���� id �ʱ� �� ����
    //=====================================================================================================================================
    g_id = 0;

    //=====================================================================================================================================
    // select���� ����� timeVal ����
    //=====================================================================================================================================
    TIMEVAL timeVal;
    timeVal.tv_sec = 0;
    timeVal.tv_usec = 0;

    //=====================================================================================================================================
    // ���� ����
    //=====================================================================================================================================
    while (!g_bShutdown)
    {
        // ��Ʈ��ũ I/O ó��
        netIOProcess();

        // ���� ���� ������Ʈ
        Update();
    }
}

// ���� ����
void Update(void)
{
    // ������ ���߱�
    if (timeGetTime() < (g_currentServerTime + g_targetFrame))
        return;
    else
        g_currentServerTime += g_targetFrame;

    // ��Ȱ��ȭ�� Ŭ���̾�Ʈ�� ����Ʈ���� ����
    // ���⼭ �����ϴ� ������ ���� �����ӿ� ���� �󿡼� ���ŵ� ���ǵ��� sendQ�� ������� ���� ���ŵǱ� ���ؼ� �̷��� �ۼ�.
    auto it = g_clientList.begin();
    while (it != g_clientList.end())
    {
        // ��Ȱ��ȭ �Ǿ��ٸ�
        if (!(*it)->isAlive)
        {
            // sendQ�� ��������� Ȯ��.
            if ((*it)->sendQ.GetUseSize() != 0)
            {
                // ���� ������� �ʾҴٸ� �ǵ����� ���� ���.
                DebugBreak();
            }

            // ����
            closesocket((*it)->sock);

            delete (*it)->pPlayer;  // �÷��̾� ����
            delete (*it);           // ���� ����

            it = g_clientList.erase(it);
        }
        // Ȱ�� ���̶��
        else
        {
            ++it;
        }
    }

    for (auto& client : g_clientList)
    {
        if (0 >= client->pPlayer->GetHp())
        {
            //=====================================================================================================================================
            // ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� ��
            //=====================================================================================================================================
            NotifyClientDisconnected(client);
        }
        else
        {
            if (client->flagField != 0)
            {
                // �÷��̾ �����̰� �ִٸ�
                if (client->flagField & FLAG_MOVING)
                {
                    // ���⿡ ���� �ٸ� ��ġ ������
                    client->pPlayer->Move();

                    UINT16 x, y;
                    client->pPlayer->getPosition(x, y);
                    std::cout << client->uid << " / " << x << ", " << y << "\n";
                }
            }
        }
    }
}

// Ŭ���̾�Ʈ���� �����͸� ��ε�ĳ��Ʈ�ϴ� �Լ�
void BroadcastData(SESSION* excludeSession, void* pData, UINT8 dataSize)
{
    for (auto& client : g_clientList)
    {
        // �ش� ������ alive�� �ƴϰų� ������ �����̶�� �Ѿ��
        if (!client->isAlive || excludeSession == client)
            continue;

        // �޽��� ����, ������ sendQ�� �����͸� ����
        int retVal = client->sendQ.Enqueue((const char*)pData, dataSize);

        if (retVal != dataSize)
        {
            NotifyClientDisconnected(client);

            // �̷� ���� �־ �ȵ����� Ȥ�� �𸣴� �˻�, enqueue���� ������ �� ���� �������� ũ�Ⱑ ����á�ٴ� �ǹ��̹Ƿ�, resize���� ������ ���� ����� ���� �׽�Ʈ�ϸ鼭 ����
            int error = WSAGetLastError();
            std::cout << "Error : BroadcastData(), It might be full of sendQ" << error << "\n";
            DebugBreak();
        }
    }
}

void BroadcastPacket(SESSION* excludeSession, PACKET_HEADER* pHeader, void* pPacket)
{
    BroadcastData(excludeSession, pHeader, sizeof(PACKET_HEADER));
    BroadcastData(excludeSession, pPacket, pHeader->bySize);
}

// Ŭ���̾�Ʈ ������ ������ ��쿡 ȣ��Ǵ� �Լ�
void NotifyClientDisconnected(SESSION* disconnectedSession)
{
    // ���� �̹� �׾��ٸ� NotifyClientDisconnected�� ȣ��Ǿ��� �����̹Ƿ� �ߺ��� ����. üũ�� ��.
    if (disconnectedSession->isAlive == false)
    {
        DebugBreak();
    }

    disconnectedSession->isAlive = false;

    // PACKET_SC_DELETE_CHARACTER ��Ŷ ����
    PACKET_HEADER header;
    PACKET_SC_DELETE_CHARACTER packetDelete;
    mpDeleteCharacter(&header, &packetDelete, disconnectedSession->uid);

    // ������ ��Ŷ ������ ���� ������ �����ϰ� ��ε�ĳ��Ʈ
    BroadcastPacket(disconnectedSession, &header, &packetDelete);
}

// ���ڷ� ���� ���ǵ鿡�� ������ ������ �õ��ϴ� �Լ�
void UnicastData(SESSION* includeSession, void* pData, UINT8 dataSize)
{
    if (!includeSession->isAlive)
        return;

    // ������ sendQ�� �����͸� ����
    int retVal = includeSession->sendQ.Enqueue((const char*)pData, dataSize);

    if (retVal != dataSize)
    {
        NotifyClientDisconnected(includeSession);

        // �̷� ���� �־ �ȵ����� Ȥ�� �𸣴� �˻�, enqueue���� ������ �� ���� �������� ũ�Ⱑ ����á�ٴ� �ǹ��̹Ƿ�, resize���� ������ ���� ����� ���� �׽�Ʈ�ϸ鼭 ����
        int error = WSAGetLastError();
        std::cout << "Error : UnicastData(), It might be full of sendQ" << error << "\n";
        DebugBreak();
    }
}

void UnicastPacket(SESSION* includeSession, PACKET_HEADER* pHeader, void* pPacket)
{
    UnicastData(includeSession, pHeader, sizeof(PACKET_HEADER));
    UnicastData(includeSession, pPacket, pHeader->bySize);
}

SESSION* createSession(SOCKET ClientSocket, SOCKADDR_IN ClientAddr, int id, int posX, int posY, int hp, int direction)
{
    CWinSockManager<SESSION>& winSockManager = CWinSockManager<SESSION>::getInstance();

    // accept�� �Ϸ�Ǿ��ٸ� ���ǿ� ��� ��, �ش� ���ǿ� ��Ŷ ����
    SESSION* Session = new SESSION;
    Session->isAlive = true;

    // ���� ���� �߰�
    Session->sock = ClientSocket;

    // IP / PORT ���� �߰�
    memcpy(Session->IP, winSockManager.GetIP(ClientAddr).c_str(), sizeof(Session->IP));
    Session->port = winSockManager.GetPort(ClientAddr);

    // UID �ο�
    Session->uid = id;

    Session->flagField = 0;

    CPlayer* pPlayer = new CPlayer(posX, posY, direction, hp);
    pPlayer->SetFlagField(&Session->flagField);
    pPlayer->SetSpeed(MOVE_X_PER_FRAME, MOVE_Y_PER_FRAME);

    Session->pPlayer = pPlayer;

    return Session;
}

bool netPacketProc_MoveStart(SESSION* pSession, void* pPacket)
{
    PACKET_CS_MOVE_START* pMovePacket = static_cast<PACKET_CS_MOVE_START*>(pPacket);

    // �޽��� ���� �α� Ȯ��
    // ==========================================================================================================
    // ������ ��ġ�� ���� ��Ŷ�� ��ġ���� �ʹ� ū ���̰� ���ٸ� �������                           .
    // �� ������ ��ǥ ����ȭ ������ �ܼ��� Ű���� ���� (Ŭ����Ʈ�� ��ó��, ������ �� �ݿ�) �������
    // Ŭ���̾�Ʈ�� ��ǥ�� �״�� �ϴ� ����� ���ϰ� ����.
    // ���� �¶��� �����̶�� Ŭ���̾�Ʈ���� �������� �����ϴ� ����� ���ؾ���.
    // ������ ������ ������ �������� �ϰ� �����Ƿ� �������� ������ Ŭ���̾�Ʈ ��ǥ�� �ϵ��� �Ѵ�.
    // ==========================================================================================================

    UINT16 posX, posY;
    pSession->pPlayer->getPosition(posX, posY);
    if (
        std::abs(posX - pMovePacket->x) > dfERROR_RANGE ||
        std::abs(posY - pMovePacket->y) > dfERROR_RANGE
        )
    {
        NotifyClientDisconnected(pSession);

        // �α� �����Ÿ� ���⼭ ���� ��
        int gapX = std::abs(posX - pMovePacket->x);
        int gapY = std::abs(posY - pMovePacket->y);
        DebugBreak();

        return false;
    }

    // ==========================================================================================================
    // ������ ����. ���� �������� ���۹�ȣ�� ���Ⱚ. ���ο��� �ٶ󺸴� ���⵵ ����
    // ==========================================================================================================
    pSession->pPlayer->SetDirection(pMovePacket->direction);
    

    // ==========================================================================================================
    // ����ڸ� ������, ���� �������� ��� ����ڿ��� ��Ŷ�� �Ѹ�.
    // ==========================================================================================================
    PACKET_HEADER header;
    PACKET_SC_MOVE_START packetSCMoveStart;
    mpMoveStart(&header, &packetSCMoveStart, pSession->uid, pSession->pPlayer->GetDirection(), posX, posY);
    BroadcastPacket(pSession, &header, &packetSCMoveStart);

    
    //=====================================================================================================================================
    // �̵� ���� ������ �˸�
    //=====================================================================================================================================
    pSession->pPlayer->SetFlag(FLAG_MOVING, true);

    return true;
}

bool netPacketProc_MoveStop(SESSION* pSession, void* pPacket)
{
    // ���� ���õ� Ŭ���̾�Ʈ�� �������� �������� ���� ���̶� ��û
    // 1. ���� ������ ó��
    // 2. PACKET_SC_MOVE_STOP �� ��ε�ĳ����
    // 3. ���� ������ �̵� ���� ������ �˸�

    //=====================================================================================================================================
    // 1. ���� ������ ó��
    //=====================================================================================================================================
    PACKET_CS_MOVE_STOP* packetCSMoveStop = static_cast<PACKET_CS_MOVE_STOP*>(pPacket);
    pSession->pPlayer->SetDirection(packetCSMoveStop->direction);
    pSession->pPlayer->SetPosition(packetCSMoveStop->x, packetCSMoveStop->y);


    //=====================================================================================================================================
    // 2. PACKET_SC_MOVE_STOP �� ��ε�ĳ����
    //=====================================================================================================================================
    PACKET_HEADER header;
    PACKET_SC_MOVE_STOP packetSCMoveStop;
    mpMoveStop(&header, &packetSCMoveStop, pSession->uid, packetCSMoveStop->direction, packetCSMoveStop->x, packetCSMoveStop->y);

    BroadcastPacket(pSession, &header, &packetSCMoveStop);

    //=====================================================================================================================================
    // 3. ���� ������ �̵� ���� ������ �˸�
    //=====================================================================================================================================
    pSession->pPlayer->SetFlag(FLAG_MOVING, false);

    return true;
}

bool netPacketProc_ATTACK1(SESSION* pSession, void* pPacket)
{
    // Ŭ���̾�Ʈ�� ���� ���� �޽����� ����.
    // g_clientList�� ��ȸ�ϸ� ���� 1�� ������ �����ؼ� �������� �־���.
    // 1. dfPACKET_SC_ATTACK1 �� ��ε�ĳ����
    // 2. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 3, 4�� ���� ����
    // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
    // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� �� -> �� �κ��� �������� ó���ϵ��� �ٲ�.

    //=====================================================================================================================================
    // 1. dfPACKET_SC_ATTACK1 �� ��ε�ĳ����
    //=====================================================================================================================================
    PACKET_CS_ATTACK1* packetCSAttack1 = static_cast<PACKET_CS_ATTACK1*>(pPacket);
    pSession->pPlayer->SetPosition(packetCSAttack1->x, packetCSAttack1->y);

    PACKET_HEADER header;
    PACKET_SC_ATTACK1 packetSCAttack1;
    mpAttack1(&header, &packetSCAttack1, pSession->uid, pSession->pPlayer->GetDirection(), packetCSAttack1->x, packetCSAttack1->y);
    BroadcastPacket(pSession, &header, &packetSCAttack1);

    //=====================================================================================================================================
    // 2. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 3, 4�� ���� ����
    //=====================================================================================================================================

    // ���� �ٶ󺸴� ���⿡ ���� ���� ������ �޶���.
    UINT16 left, right, top, bottom;
    UINT16 posX, posY;
    pSession->pPlayer->getPosition(posX, posY);

    // ������ �ٶ󺸰� �־��ٸ�
    if (pSession->pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
    {
        left = posX - dfATTACK1_RANGE_X;
        right = posX;
    }
    // �������� �ٶ󺸰� �־��ٸ�
    else
    {
        left = posX;
        right = posX + dfATTACK1_RANGE_X;
    }

    top = posY - dfATTACK1_RANGE_Y;
    bottom = posY + dfATTACK1_RANGE_Y;

    for (auto& client : g_clientList)
    {
        if (client->uid == pSession->uid)
            continue;

        client->pPlayer->getPosition(posX, posY);

        // �ٸ� �÷��̾��� ��ǥ�� ���� ������ ���� ���
        if (posX >= left && posX <= right &&
            posY >= top && posY <= bottom)
        {
            //=====================================================================================================================================
            // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
            //=====================================================================================================================================
            // 1�� �������� �Ե��� ��
            client->pPlayer->Damaged(dfATTACK1_DAMAGE);

            PACKET_SC_DAMAGE packetDamage;
            mpDamage(&header, &packetDamage, pSession->uid, client->uid, client->pPlayer->GetHp());

            BroadcastPacket(nullptr, &header, &packetDamage);
            
            /*
            //=====================================================================================================================================
            // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� ��
            //=====================================================================================================================================
            if (packetDamage.damagedHP <= 0)
            {
                PACKET_SC_DELETE_CHARACTER packetDeleteCharacter;
                mpDeleteCharacter(&header, &packetDeleteCharacter, client->uid);

                BroadcastPacket(nullptr, &header, &packetDeleteCharacter);

                // �������� ������ �� �ֵ��� isAlive�� false�� ����
                client->isAlive = false;
            }
            */

            // ���⼭ break�� ���ָ� ���� ������
            break;
        }
    }

    return true;
}

bool netPacketProc_ATTACK2(SESSION* pSession, void* pPacket)
{
    // Ŭ���̾�Ʈ�� ���� ���� �޽����� ����.
    // g_clientList�� ��ȸ�ϸ� ���� 2�� ������ �����ؼ� �������� �־���.
    // 1. dfPACKET_SC_ATTACK2 �� ��ε�ĳ����
    // 2. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 3, 4�� ���� ����
    // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
    // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� ��

    //=====================================================================================================================================
    // 1. dfPACKET_SC_ATTACK2 �� ��ε�ĳ����
    //=====================================================================================================================================
    PACKET_CS_ATTACK2* packetCSAttack2 = static_cast<PACKET_CS_ATTACK2*>(pPacket);
    pSession->pPlayer->SetPosition(packetCSAttack2->x, packetCSAttack2->y);

    PACKET_HEADER header;
    PACKET_SC_ATTACK2 packetSCAttack2;
    mpAttack2(&header, &packetSCAttack2, pSession->uid, pSession->pPlayer->GetDirection(), packetCSAttack2->x, packetCSAttack2->y);
    BroadcastPacket(pSession, &header, &packetSCAttack2);

    //=====================================================================================================================================
    // 2. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 3, 4�� ���� ����
    //=====================================================================================================================================

    // ���� �ٶ󺸴� ���⿡ ���� ���� ������ �޶���.
    UINT16 left, right, top, bottom;
    UINT16 posX, posY;
    pSession->pPlayer->getPosition(posX, posY);

    // ������ �ٶ󺸰� �־��ٸ�
    if (pSession->pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
    {
        left = posX - dfATTACK2_RANGE_X;
        right = posX;
    }
    // �������� �ٶ󺸰� �־��ٸ�
    else
    {
        left = posX;
        right = posX + dfATTACK2_RANGE_X;
    }

    top = posY - dfATTACK2_RANGE_Y;
    bottom = posY + dfATTACK2_RANGE_Y;

    for (auto& client : g_clientList)
    {
        if (client->uid == pSession->uid)
            continue;

        client->pPlayer->getPosition(posX, posY);

        // �ٸ� �÷��̾��� ��ǥ�� ���� ������ ���� ���
        if (posX >= left && posX <= right &&
            posY >= top && posY <= bottom)
        {
            //=====================================================================================================================================
            // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
            //=====================================================================================================================================
            // 1�� �������� �Ե��� ��
            client->pPlayer->Damaged(dfATTACK2_DAMAGE);

            PACKET_SC_DAMAGE packetDamage;
            mpDamage(&header, &packetDamage, pSession->uid, client->uid, client->pPlayer->GetHp());

            BroadcastPacket(nullptr, &header, &packetDamage);

            /*
            //=====================================================================================================================================
            // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� ��
            //=====================================================================================================================================
            if (packetDamage.damagedHP <= 0)
            {
                PACKET_SC_DELETE_CHARACTER packetDeleteCharacter;
                mpDeleteCharacter(&header, &packetDeleteCharacter, client->uid);

                BroadcastPacket(nullptr, &header, &packetDeleteCharacter);

                // �������� ������ �� �ֵ��� isAlive�� false�� ����
                client->isAlive = false;
            }
            */

            // ���⼭ break�� ���ָ� ���� ������
            break;
        }
    }

    return true;
}

bool netPacketProc_ATTACK3(SESSION* pSession, void* pPacket)
{
    // Ŭ���̾�Ʈ�� ���� ���� �޽����� ����.
     // g_clientList�� ��ȸ�ϸ� ���� 3�� ������ �����ؼ� �������� �־���.
     // 1. dfPACKET_SC_ATTACK3 �� ��ε�ĳ����
     // 2. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 3, 4�� ���� ����
     // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
     // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� ��

     //=====================================================================================================================================
     // 1. dfPACKET_SC_ATTACK3 �� ��ε�ĳ����
     //=====================================================================================================================================
    PACKET_CS_ATTACK3* packetCSAttack3 = static_cast<PACKET_CS_ATTACK3*>(pPacket);
    pSession->pPlayer->SetPosition(packetCSAttack3->x, packetCSAttack3->y);

    PACKET_HEADER header;
    PACKET_SC_ATTACK3 packetSCAttack3;
    mpAttack3(&header, &packetSCAttack3, pSession->uid, pSession->pPlayer->GetDirection(), packetCSAttack3->x, packetCSAttack3->y);
    BroadcastPacket(pSession, &header, &packetSCAttack3);

    //=====================================================================================================================================
    // 2. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 3, 4�� ���� ����
    //=====================================================================================================================================

    // ���� �ٶ󺸴� ���⿡ ���� ���� ������ �޶���.
    UINT16 left, right, top, bottom;
    UINT16 posX, posY;
    pSession->pPlayer->getPosition(posX, posY);

    // ������ �ٶ󺸰� �־��ٸ�
    if (pSession->pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
    {
        left = posX - dfATTACK3_RANGE_X;
        right = posX;
    }
    // �������� �ٶ󺸰� �־��ٸ�
    else
    {
        left = posX;
        right = posX + dfATTACK3_RANGE_X;
    }

    top = posY - dfATTACK3_RANGE_Y;
    bottom = posY + dfATTACK3_RANGE_Y;

    for (auto& client : g_clientList)
    {
        if (client->uid == pSession->uid)
            continue;

        client->pPlayer->getPosition(posX, posY);

        // �ٸ� �÷��̾��� ��ǥ�� ���� ������ ���� ���
        if (posX >= left && posX <= right &&
            posY >= top && posY <= bottom)
        {
            //=====================================================================================================================================
            // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
            //=====================================================================================================================================
            // 1�� �������� �Ե��� ��
            client->pPlayer->Damaged(dfATTACK3_DAMAGE);

            PACKET_SC_DAMAGE packetDamage;
            mpDamage(&header, &packetDamage, pSession->uid, client->uid, client->pPlayer->GetHp());

            BroadcastPacket(nullptr, &header, &packetDamage);

            /*
            //=====================================================================================================================================
            // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� ��
            //=====================================================================================================================================
            if (packetDamage.damagedHP <= 0)
            {
                PACKET_SC_DELETE_CHARACTER packetDeleteCharacter;
                mpDeleteCharacter(&header, &packetDeleteCharacter, client->uid);

                BroadcastPacket(nullptr, &header, &packetDeleteCharacter);

                // �������� ������ �� �ֵ��� isAlive�� false�� ����
                client->isAlive = false;
            }
            */

            // ���⼭ break�� ���ָ� ���� ������
            break;
        }
    }

    return true;
}

bool PacketProc(SESSION* pSession, PACKET_TYPE packetType, void* pPacket)
{
    switch (packetType)
    {
    case PACKET_TYPE::CS_MOVE_START:
        return netPacketProc_MoveStart(pSession, pPacket);
        break;
    case PACKET_TYPE::CS_MOVE_STOP:
        return netPacketProc_MoveStop(pSession, pPacket);
        break;
    case PACKET_TYPE::CS_ATTACK1:
        return netPacketProc_ATTACK1(pSession, pPacket);
        break;
    case PACKET_TYPE::CS_ATTACK2:
        return netPacketProc_ATTACK2(pSession, pPacket);
        break;
    case PACKET_TYPE::CS_ATTACK3:
        return netPacketProc_ATTACK3(pSession, pPacket);
        break;
    //case PACKET_TYPE::CS_SYNC:
    //    break;
    default:
        break;
    }
    
    return true;
}

void netProc_Recv(SESSION* pSession)
{
    // �ӽ� ���� ����
    char tempRecvBuffer[RINGBUFFER_SIZE];
    char tempPacketBuffer[MAX_PACKET_SIZE + 1];

    // recv �ӽ� ���� ���۷� ȣ��
    int recvRetVal = recv(pSession->sock, tempRecvBuffer, RINGBUFFER_SIZE, 0);

    // recv ��ȯ���� 0�̶�� ���� �����ʿ��� rst�� ���´ٴ� �ǹ��̹Ƿ� disconnect ó���� ������Ѵ�.
    if (recvRetVal == 0)
    {
        // �� �̻� ���� ���� �����Ƿ� break!
        NotifyClientDisconnected(pSession);
        //std::cout << "recv() return 0," << WSAGetLastError() << "\n";
        return;
    }
    // recv ������ ���
    else if (recvRetVal == SOCKET_ERROR)
    {
        int error = WSAGetLastError();

        // ���� ���ۿ� �����Ͱ� �����Ƿ� ���߿� �ٽ� recv �õ�
        if (error == WSAEWOULDBLOCK)
            return;

        // ���� ���ۿ��� ������ �������ٴ�, rst�� �Դٴ� �ǹ�.
        if (error == WSAECONNRESET)
        {
            // �ٸ� Ŭ���̾�Ʈ�鿡 ������ �������ٴ� ���� �˸�.
            NotifyClientDisconnected(pSession);
            return;
        }

        // ���� �ʿ��� closesocket�� �ϸ� recv ����
        if (error == WSAECONNABORTED)
        {
            NotifyClientDisconnected(pSession);
            return;
        }

        // ���� WSAEWOULDBLOCK�� �ƴ� �ٸ� ������� ��¥ ����
        std::cout << "Error : recv(), " << WSAGetLastError() << "\n";
        DebugBreak();
    }

    // �켱 �����͸� ��� ����.
    int recvQEnqRetVal = pSession->recvQ.Enqueue(tempRecvBuffer, recvRetVal);
    if (recvQEnqRetVal != recvRetVal)
    {
        std::cout << "Error : recvQ.Enqueue(), " << WSAGetLastError() << "\n";
        DebugBreak();
    }

    // �Ϸ���Ŷ ó�� ��
    // RecvQ�� �ִ� ��� �ϼ���Ŷ�� �ؼ�/ó��
    while (true)
    {
        // 1. RecvQ�� �ּ����� ����� �ִ��� Ȯ��. ������ [ ��� ������ �̻��� �����Ͱ� �ִ��� Ȯ�� ]�ϴ� ��.
        if (pSession->recvQ.GetUseSize() < sizeof(PACKET_HEADER))
            break;

        // 2. RecvQ���� PACKET_HEADER ���� Peek
        PACKET_HEADER header;
        int headerSize = sizeof(header);
        int retVal = pSession->recvQ.Peek(reinterpret_cast<char*>(&header), headerSize);

        // ���� ������ �˻縦 ����� ������ ������ �˻������Ƿ� �ѱ�.
        /*
        if (retVal != headerSize)
        {
            break;
        }
        */

        // 3. header�� Code �κ� Ȯ��. CRC Ȯ������ �̻��� ���� ������ disconnect ó��
        if (header.byCode != dfNETWORK_PACKET_CODE)
        {
            NotifyClientDisconnected(pSession);
            break;
        }

        // 4. ����� len���� RecvQ�� ������ ������ ��
        if ((header.bySize + sizeof(PACKET_HEADER)) > pSession->recvQ.GetUseSize())
            break;

        // 5. Peek �ߴ� ����� RecvQ���� �����.
        pSession->recvQ.MoveFront(sizeof(PACKET_HEADER));

        // 6. RecvQ���� header�� len ũ�⸸ŭ �ӽ� ��Ŷ ���۸� �̴´�.
        int recvQDeqRetVal = pSession->recvQ.Dequeue(tempPacketBuffer, header.bySize);
        tempPacketBuffer[recvQDeqRetVal] = '\0';

        // 7. ����� Ÿ�Կ� ���� �б⸦ ���� ��Ŷ ���ν��� ȣ��
        if (!PacketProc(pSession, static_cast<PACKET_TYPE>(header.byType), tempPacketBuffer))
        {
            NotifyClientDisconnected(pSession);
            break;
        }
    }
}

void netProc_Accept()
{
    // Ŭ���̾�Ʈ�� �������� �� ����Ǵ� ����
    // ��α� ť�� ������ �Ǿ����� �����ϰ� Accept �õ�
    SOCKET ClientSocket;
    SOCKADDR_IN ClientAddr;

    CWinSockManager<SESSION>& winSockManager = CWinSockManager<SESSION>::getInstance();
    SOCKET listenSocket = winSockManager.GetListenSocket();

    // accept �õ�
    ClientSocket = winSockManager.Accept(ClientAddr);

    // accept�� �Ϸ�Ǿ��ٸ� ���ǿ� ��� ��, �ش� ���ǿ� ��Ŷ ����
    SESSION* Session = createSession(ClientSocket, ClientAddr,
        g_id,
        rand() % (dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT) + dfRANGE_MOVE_LEFT,
        rand() % (dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP) + dfRANGE_MOVE_TOP,
        100,
        dfPACKET_MOVE_DIR_LL
    );

    // 1. ����� ���ǿ� PACKET_SC_CREATE_MY_CHARACTER �� ����
    // 2. PACKET_SC_CREATE_OTHER_CHARACTER �� ����� ������ ������ ��� ��ε�ĳ��Ʈ
    // 3. PACKET_SC_CREATE_OTHER_CHARACTER �� g_clientList�� �ִ� ��� ĳ���� ������ ��� ����� ���ǿ��� ����
    
    //=====================================================================================================================================
    // 1. ����� ���ǿ� PACKET_SC_CREATE_MY_CHARACTER �� ����
    //=====================================================================================================================================

    PACKET_HEADER header;
    PACKET_SC_CREATE_MY_CHARACTER packetCreateMyCharacter;

    UINT16 posX, posY;
    Session->pPlayer->getPosition(posX, posY);
    mpCreateMyCharacter(&header, &packetCreateMyCharacter, Session->uid, Session->pPlayer->GetDirection(), posX, posY, Session->pPlayer->GetHp());

    UnicastPacket(Session, &header, &packetCreateMyCharacter);

    //=====================================================================================================================================
    // 2. PACKET_SC_CREATE_OTHER_CHARACTER �� ����� ������ ������ ��� ��ε�ĳ��Ʈ
    //=====================================================================================================================================

    PACKET_SC_CREATE_OTHER_CHARACTER packetCreateOtherCharacter;
    mpCreateOtherCharacter(&header, &packetCreateOtherCharacter, Session->uid, Session->pPlayer->GetDirection(), posX, posY, Session->pPlayer->GetHp());

    // ���ӵǾ� �ִ� ��� client�� ���� ������ client�� ���� broadcast �õ�
    BroadcastPacket(NULL, &header, &packetCreateOtherCharacter);

    //=====================================================================================================================================
    // 3. PACKET_SC_CREATE_OTHER_CHARACTER �� g_clientList�� �ִ� ��� ĳ���� ������ ��� ����� ���ǿ��� ����
    //=====================================================================================================================================

    // ���ο� ������ �õ��ϴ� Ŭ���̾�Ʈ�� ���� Ŭ���̾�Ʈ �������� ����
    for (const auto& client : g_clientList)
    {
        client->pPlayer->getPosition(posX, posY);
        mpCreateOtherCharacter(&header, &packetCreateOtherCharacter, client->uid, client->pPlayer->GetDirection(), posX, posY, client->pPlayer->GetHp());

        UnicastPacket(Session, &header, &packetCreateOtherCharacter);
    }

    // �����͸� ������ �߿� ������ �� �� ������ ����ִ� ���� �˻�. ���� �־�� �ȵ����� sendQ�� ����á�� ��� ������ �߻��� �� ����. Ȥ�� �𸣴� �˻�
    if (Session->isAlive)
    {
        // ��� ���� ���� g_clientList�� ���� ���
        g_clientList.push_back(Session);

        // ���� id�� ����
        ++g_id;
    }
    else
    {
        int error = WSAGetLastError();
        DebugBreak();

        delete Session;
    }
}

void netProc_Send(SESSION* pSession)
{
    // �����ۿ� �ִ� ������ �ѹ��� send. 
    int directDeqSize = pSession->sendQ.DirectDequeueSize();
    int useSize = pSession->sendQ.GetUseSize();

    // ����ϴ� �뷮�� directDeqSize���� �۰ų� ���� ���
    if (useSize <= directDeqSize)
    {
        int retval = send(pSession->sock, pSession->sendQ.GetFrontBufferPtr(), useSize, 0);

        // ���⼭ ���� ���� ó��
        if (retval == SOCKET_ERROR)
        {
            int error = WSAGetLastError();

            // �߰��� ������ ���� ����.
            if (error == WSAECONNRESET)
            {
                NotifyClientDisconnected(pSession);
                return;
            }

            // ���⼭ ���� ó�� �۾�
            DebugBreak();
        }

        // ���� send�� �����Ѵٸ�
        if (retval != useSize)
        {
            int error = WSAGetLastError();

            // ���⼭ ���� ó�� �۾�
            DebugBreak();
        }

        pSession->sendQ.MoveFront(retval);
    }
    // ����ϴ� �뷮�� directDeqSize���� Ŭ ���
    else
    {
        // directDeqSize ��ŭ�� send
        int retval = send(pSession->sock, pSession->sendQ.GetFrontBufferPtr(), directDeqSize, 0);

        // ���⼭ ���� ���� ó��
        if (retval == SOCKET_ERROR)
        {
            int error = WSAGetLastError();

            // ���⼭ ���� ó�� �۾�
            DebugBreak();
        }

        // ���� send�� �����Ѵٸ�
        if (retval != directDeqSize)
        {
            int error = WSAGetLastError();

            // ���⼭ ���� ó�� �۾�
            DebugBreak();
        }

        pSession->sendQ.MoveFront(retval);

        // ���� �κ��� ������ ��¥�� �ٽ� ���´�. �׶� �߰������� ���� ���� �ִٸ� �����°� ������尡 ����. 
        // ���� ���伺�� �������� �� ������ ���⸦ ����� �ٽ� �õ��غ� ��.
        /*
        // ���� �κ� ����
        retval = send(pSession->sock, pSession->sendQ.GetFrontBufferPtr(), useSize - directDeqSize, 0);

        // ���� send�� �����Ѵٸ�
        if (retval != (useSize - directDeqSize))
        {
            int error = WSAGetLastError();

            // ���⼭ ���� ó�� �۾�
            DebugBreak();
        }

        pSession->sendQ.MoveFront(retval);
        */
    }
}

void netIOProcess(void)
{
    FD_SET ReadSet;
    FD_SET WriteSet;

    FD_ZERO(&ReadSet);
    FD_ZERO(&WriteSet);

    // listen ���� �ֱ�
    CWinSockManager<SESSION>& winSockManager = CWinSockManager<SESSION>::getInstance();
    SOCKET listenSocket = winSockManager.GetListenSocket();
    FD_SET(listenSocket, &ReadSet);

    // listen ���� �� �������� ��� Ŭ���̾�Ʈ�� ���� SOCKET üũ
    for (auto& client : g_clientList)
    {
        // �ش� client�� ReadSet�� ���
        FD_SET(client->sock, &ReadSet);

        // ���� ���� �����Ͱ� �ִٸ� WriteSet�� ���
        if (client->sendQ.GetUseSize() > 0)
            FD_SET(client->sock, &WriteSet);
    }

    TIMEVAL timeVal;
    timeVal.tv_sec = 0;
    timeVal.tv_usec = 0;

    int iResult;

    // select ȣ��. ��ȯ�Ǵ� ���� ������ �ִ� ���� �������� ��.
    iResult = select(0, &ReadSet, &WriteSet, NULL, &timeVal);
    // ù �����̳� read�� ���� ���ٸ� ������ ���. 
    // ��α� ť�� recv�� ��Ŷ�� ���� �Ѿ��.

    // iResult ���� 0 �̻��̶�� ���� ������ / �� �����Ͱ� �ִٴ� ���̴�.
    if (0 < iResult)
    {
        // ��α� ť�� ������ �ִٸ�, accept ����
        if (FD_ISSET(listenSocket, &ReadSet))
        {
            // accept �̺�Ʈ ó��. ���� �� Session ����
            netProc_Accept();
        }

        // ������ ���� � ������ ������ �������� Ȯ��
        for (auto& client : g_clientList)
        {
            if (FD_ISSET(client->sock, &ReadSet))
            {
                --iResult;

                // recv �̺�Ʈ ó��. �޽��� ���� �� �޽��� �б� ���� ó��
                if (client->isAlive)
                    netProc_Recv(client);
            }

            if (FD_ISSET(client->sock, &WriteSet))
            {
                --iResult;

                // send �̺�Ʈ ó��. �޽��� �۽�
                if (client->isAlive)
                    netProc_Send(client);
            }
        }
    }
    else if (iResult == SOCKET_ERROR)
    {
        std::cout << "Error : select(), " << WSAGetLastError() << "\n";
        DebugBreak();
    }
}
