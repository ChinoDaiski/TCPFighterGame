

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

//std::list<CObject>

int g_id;   // ���ǵ鿡 id�� �ο��ϱ� ���� ����. �� ���� 1�� ������Ű�鼭 id�� �ο��Ѵ�. 

void NotifyClientDisconnected(SESSION* disconnectedSession);

//==========================================================================================================================================
// Broadcast
//==========================================================================================================================================
void BroadcastData(SESSION* excludeSession, void* pMsg, UINT8 msgSize);
void BroadcastPacket(SESSION* excludeSession, void* pMsg, UINT8 packetType, UINT8 packetSize);

//==========================================================================================================================================
// Unicast
//==========================================================================================================================================
void UnicastData(SESSION* includeSession, void* pMsg, UINT8 msgSize);
void UnicastPacket(SESSION* includeSession, void* pMsg, UINT8 packetType, UINT8 packetSize);


void processReceivedPacket(SESSION* client, PACKET_TYPE pt, void* pData);
void processSendPacket(SESSION* client, PACKET_TYPE pt);

SESSION* createSession(SOCKET ClientSocket, SOCKADDR_IN ClientAddr, int id, int posX, int posY, int hp, int direction);

int main()
{
    CWinSockManager<SESSION>& winSockManager = CWinSockManager<SESSION>::getInstance();

    UINT8 options;
    options |= OPTION_NONBLOCKING;

    winSockManager.StartServer(PROTOCOL_TYPE::TCP_IP, SERVERPORT, options);
    g_listenSocket = winSockManager.GetListenSocket();

    // ������ ��ſ� ����� ����
    SOCKET ClientSocket;
    SOCKADDR_IN ClientAddr;
    fd_set readSet;
    fd_set writeSet;

    // id �ʱ� �� ����
    g_id = 0;

    // ��Ŷ �� �ִ� ũ���� ��Ŷ ������ ũ���� ����
    char packetBuffer[MAX_PACKET_SIZE + sizeof(PACKET_HEADER) + 1];

    TIMEVAL timeVal;
    timeVal.tv_sec = 0;
    timeVal.tv_usec = 0;

    CTimerManager& pTimerInstance = CTimerManager::getInstance();
    pTimerInstance.InitTimer(50); 
    pTimerInstance.SetStartServerTime();

    while (true)
    {
        pTimerInstance.StartFrame();

        //---------------------------------------------------------------
        // recv
        //---------------------------------------------------------------

        // �� �ʱ�ȭ
        FD_ZERO(&readSet);

        // readSet�� listenSocket ���
        FD_SET(g_listenSocket, &readSet);

        // ClientList�� �ִ� ���� ���� listenSocket ���
        for (const auto& client : g_clientList)
        {
            FD_SET(client->sock, &readSet);
        }

        // select ȣ��
        int selectRetVal = select(0, &readSet, NULL, NULL, &timeVal);
        // ù �����̳� read�� ���� ���ٸ� ������ ���. 
        // ��α� ť�� recv�� ��Ŷ�� ���� �Ѿ��.

        if (selectRetVal == SOCKET_ERROR)
        {
            std::cout << "Error : readSet select(), " << WSAGetLastError() << "\n";
            DebugBreak();
        }

        // ���ǵ� �� ������ ������ �ִٸ� �ٸ� Ŭ���̾�Ʈ�� broadcast �ϱ�.
        for (const auto& client : g_clientList)
        {
            // �ش� ������ readSet�� ������
            BOOL isInReadSet = FD_ISSET(client->sock, &readSet);
            while (isInReadSet)
            {
                // ������ �޾Ƽ� �����ۿ� ä���
                int directDeqSize = client->recvQ.DirectEnqueueSize();
                int receiveRetVal = recv(client->sock, client->recvQ.GetRearBufferPtr(), directDeqSize, 0);

                if (receiveRetVal == SOCKET_ERROR)
                {
                    int error = WSAGetLastError();

                    // ���� ���ۿ� �����Ͱ� �����Ƿ� ���߿� �ٽ� recv �õ�
                    if (error == WSAEWOULDBLOCK)
                        continue;

                    // ���� ���ۿ��� ������ �������ٴ�, rst�� �Դٴ� �ǹ�.
                    if (error == WSAECONNRESET)
                    {
                        // �ٸ� Ŭ���̾�Ʈ�鿡 ������ �������ٴ� ���� �˸�.
                        NotifyClientDisconnected(client);
                        break;
                    }

                    // ���� �ʿ��� closesocket�� �ϸ� recv ����
                    if (error == WSAECONNABORTED)
                    {
                        NotifyClientDisconnected(client);
                        break;
                    }

                    // ���� WSAEWOULDBLOCK�� �ƴ� �ٸ� ������� ��¥ ����
                    std::cout << "Error : recv(), " << WSAGetLastError() << "\n";
                    break;
                }
                // recv ��ȯ���� 0�̶�� ���� �����ʿ��� rst�� ���´ٴ� �ǹ��̹Ƿ� disconnect ó���� ������Ѵ�.
                else if (receiveRetVal == 0)
                {
                    // �� �̻� ���� ���� �����Ƿ� break!
                    NotifyClientDisconnected(client);
                    //std::cout << "recv() return 0," << WSAGetLastError() << "\n";
                    break;
                }

                client->recvQ.MoveRear(receiveRetVal);
                break;
            }



            // ���� �����ͷκ��� ���� ó���� �ʿ��� ���� ������Ʈ 

            // ������ �ؼ�
            // �켱 peek�� ����� Ȯ��, ���Ŀ� payload���� �����ϴ��� Ȯ���Ͽ� payload ó��. �̸� �����ۿ� ���� �ִ� ���� ����� ũ�⺸�� ���� �� ���� �ݺ��Ѵ�.
            // ���� ���⼱ 16byte�� �������� ó���Ѵ�. ��? ��� ��Ŷ ũ�Ⱑ 16byte�� �����Ǿ� �����ϱ�
            while (client->recvQ.GetUseSize() >= sizeof(PACKET_HEADER))
            {
                PACKET_HEADER header;
                int headerSize = sizeof(header);
                int retVal = client->recvQ.Peek(reinterpret_cast<char*>(&header), headerSize);

                // �����ۿ��� peek���� �о�� �� �ִ� ���� ����� ���̰� ��ġ���� �ʴ´ٸ� ó���� ���Ѵٴ� �ǹ��̹Ƿ� �ѱ��.
                if (retVal != headerSize)
                {
                    break;
                }

                // �����ۿ��� ��Ŷ ��ü ���̸�ŭ�� �����Ͱ� �ִ��� Ȯ���Ѵ�.
                int packetSize = header.bySize + headerSize;
                if (client->recvQ.GetUseSize() >= packetSize)
                {
                    // �����Ͱ� �ִٸ� dequeue, �̶� ����Ǵ� �ִ� ��Ŷ ũ�⸦ ������ ���۸� ����Ѵ�.
                    int retVal = client->recvQ.Dequeue(packetBuffer, packetSize);
                    packetBuffer[retVal] = '\0';

                    // �߰����� �˻�� �̹� if������ ������ ���Ѵ�.
                    // casting �� ���

                    // Ŭ���̾�Ʈ�� ���� �޾ƿ� �����ʹ� move ���� ��Ŷ���̹Ƿ� ���⼭�� �� ��Ŷ�� ó���Ѵ�.
                    processReceivedPacket(client, static_cast<PACKET_TYPE>(header.byType), packetBuffer + sizeof(PACKET_HEADER));
                }
                else
                    break;
            }
        }

        // listenSocket�� readSet�� ���� �ִٸ�
        BOOL isInReadSet = FD_ISSET(g_listenSocket, &readSet);
        if (isInReadSet)
        {
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

            // ����� ���ǿ� [ �ڱ� ĳ���͸� ���� �ϴ� �������� ] ����
            processSendPacket(Session, PACKET_TYPE::SC_CREATE_MY_CHARACTER);

            // �����͸� ������ �߿� ������ �� �� ������ ����ִ� ���� �˻�
            if (Session->isAlive)
            {
                // ��� ���� ���� g_clientList�� ���� ���
                g_clientList.push_back(Session);

                // ���� id�� ����
                ++g_id;
            }
            else
                delete Session;
        }

        // ��Ȱ��ȭ�� Ŭ���̾�Ʈ�� ����Ʈ���� ����
        auto it = g_clientList.begin();
        while (it != g_clientList.end())
        {
            // ��Ȱ��ȭ �Ǿ��ٸ�
            if (!(*it)->isAlive)
            {
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



        //---------------------------------------------------------------
        // ������ ����
        //---------------------------------------------------------------
        for (auto& client : g_clientList)
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

                // �÷��̾ �׾��ٸ�
                if (client->flagField & FLAG_DEAD)
                {
                    client->isAlive = false;
                }
            }
        }







        //---------------------------------------------------------------
        // send
        //---------------------------------------------------------------

        // write set ����
        // �� �ʱ�ȭ
        FD_ZERO(&writeSet);

        // ClientList�� �ִ� ���� ���� writeSet ���. �̹� ������ �ѹ� �ɷ����� isAlive ���� �˻����� ���� ������ ũ�⸸ �˻�
        for (const auto& client : g_clientList)
        {
            // ���� sendQ�� ��� ���� �ʴٸ� 
            if (client->sendQ.GetUseSize() != 0)
                // writeSet�� ���
                FD_SET(client->sock, &writeSet);
        }

        // select ȣ��
        selectRetVal = select(0, NULL, &writeSet, NULL, &timeVal);
        // ��α� ť�� recv�� ��Ŷ�� ���� �Ѿ��.

        if (selectRetVal == SOCKET_ERROR)
        {
            int error = WSAGetLastError();

            // writeSet�� �ִ� �ϳ� �̻��� ���� ������ ��ȿ���� �ʴٸ� 10022 ������ �߻��Ѵ�.
            // ��¥�� ������ ��ȿ �˻縦 �ϴϱ� �Ѿ��.
            if (error == WSAEINVAL)
            {
                ;
            }
            else
            {
                std::cout << "Error : writeSet select(), " << error << "\n";
                DebugBreak();
            }
        }

        for (const auto& client : g_clientList)
        {
            // �ش� ������ writeSet�� ������
            BOOL isInWriteSet = FD_ISSET(client->sock, &writeSet);
            while (isInWriteSet)
            {
                // �����ۿ� �ִ� ������ �ѹ��� send. 
                int directDeqSize = client->sendQ.DirectDequeueSize();
                int useSize = client->sendQ.GetUseSize();

                // ����ϴ� �뷮�� directDeqSize���� �۰ų� ���� ���
                if (useSize <= directDeqSize)
                {
                    int retval = send(client->sock, client->sendQ.GetFrontBufferPtr(), useSize, 0);

                    // ���⼭ ���� ���� ó��
                    if (retval == SOCKET_ERROR)
                    {
                        int error = WSAGetLastError();
                        
                        // �߰��� ������ ���� ����.
                        if (error == WSAECONNRESET)
                            continue;

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

                    client->sendQ.MoveFront(retval);
                }
                // ����ϴ� �뷮�� directDeqSize���� Ŭ ���
                else
                {
                    int retval = send(client->sock, client->sendQ.GetFrontBufferPtr(), directDeqSize, 0);

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

                    client->sendQ.MoveFront(retval);

                    // ���� �κ� ����
                    retval = send(client->sock, client->sendQ.GetFrontBufferPtr(), useSize - directDeqSize, 0);

                    // ���� send�� �����Ѵٸ�
                    if (retval != (useSize - directDeqSize))
                    {
                        int error = WSAGetLastError();

                        // ���⼭ ���� ó�� �۾�
                        DebugBreak();
                    }

                    client->sendQ.MoveFront(retval);
                }
                break;
            }
        }



        pTimerInstance.EndFrame();
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
            // �̷� ���� �־ �ȵ����� Ȥ�� �𸣴� �˻�, enqueue���� ������ �� ���� �������� ũ�Ⱑ ����á�ٴ� �ǹ��̹Ƿ�, resize���� ������ ���� ����� ���� �׽�Ʈ�ϸ鼭 ����
            DebugBreak();
        }
    }
}

void BroadcastPacket(SESSION* excludeSession, void* pData, UINT8 packetSize, UINT8 packetType)
{
    PACKET_HEADER header;
    header.byCode = 0x89;
    header.bySize = packetSize;
    header.byType = packetType;

    BroadcastData(excludeSession, (void*)&header, sizeof(header));
    BroadcastData(excludeSession, pData, packetSize);
}

// Ŭ���̾�Ʈ ������ ������ ��쿡 ȣ��Ǵ� �Լ�
void NotifyClientDisconnected(SESSION* disconnectedSession)
{
    disconnectedSession->isAlive = false;

    // ���⼭ ���� ���� �������� �ۼ�
    PACKET_SC_DELETE_CHARACTER packetDelete;
    packetDelete.playerID = disconnectedSession->uid;

    // ������ ������ Ŭ���̾�Ʈ ������ ��� Ŭ���̾�Ʈ���� ��ε�ĳ��Ʈ
    BroadcastPacket(disconnectedSession, &packetDelete, sizeof(packetDelete), static_cast<UINT8>(PACKET_TYPE::SC_DELETE_CHARACTER));
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
        // �̷� ���� �־ �ȵ����� Ȥ�� �𸣴� �˻�
        DebugBreak();
    }
}

void UnicastPacket(SESSION* includeSession, void* pMsg, UINT8 packetSize, UINT8 packetType)
{
    // ��Ŷ ��� ������
    PACKET_HEADER header;
    header.byCode = 0x89;
    header.bySize = packetSize;
    header.byType = packetType;
    UnicastData(includeSession, &header, sizeof(header));

    // ��Ŷ ������
    UnicastData(includeSession, pMsg, packetSize);
}

void processReceivedPacket(SESSION* session, PACKET_TYPE pt, void* pData)
{
    switch (pt)
    {
    case PACKET_TYPE::SC_CREATE_MY_CHARACTER:
    {
        // �� ��Ŷ�� processSendPacket ���� ��Ŷ
    }
    break;

    case PACKET_TYPE::SC_CREATE_OTHER_CHARACTER:
    {
       // �� ��Ŷ�� �Ѿ���� ����. �� ��Ŷ�� �ۼ��� Ÿ�ֿ̹� ���� �۾� ����.
    }
    break;
    case PACKET_TYPE::SC_DELETE_CHARACTER:
    {
        // 
    }
    break;
    case PACKET_TYPE::CS_MOVE_START:
    {
        // ���� ���õ� Ŭ���̾�Ʈ�� �������� �������� ������ ���̶� ��û��.
        // 1. ���� ������ ó��
        // 2. PACKET_SC_MOVE_START �� ��ε�ĳ����
        // 3. ���� ������ �̵� ���� ������ �˸�

        //=====================================================================================================================================
        // 1. ���� ������ ó��
        //=====================================================================================================================================
        PACKET_CS_MOVE_START* packetCSMoveStart = static_cast<PACKET_CS_MOVE_START*>(pData);
        session->pPlayer->SetDirection(packetCSMoveStart->direction);
        session->pPlayer->SetPosition(packetCSMoveStart->x, packetCSMoveStart->y);

        //=====================================================================================================================================
        // 2. PACKET_SC_MOVE_START �� ��ε�ĳ����
        //=====================================================================================================================================
        PACKET_SC_MOVE_START packetMoveStart;
        packetMoveStart.direction = packetCSMoveStart->direction;
        packetMoveStart.playerID = session->uid;
        packetMoveStart.x = packetCSMoveStart->x;
        packetMoveStart.y = packetCSMoveStart->y;

        BroadcastPacket(session, &packetMoveStart, sizeof(PACKET_SC_MOVE_START), static_cast<UINT8>(PACKET_TYPE::SC_MOVE_START));

        //=====================================================================================================================================
        // 3. �̵� ���� ������ �˸�
        //=====================================================================================================================================
        session->pPlayer->SetFlag(FLAG_MOVING, true);
    }
    break;
    case PACKET_TYPE::SC_MOVE_START:
    {
        // �̰� ���⼭ �ȿ�. ���� ������ �׶� ó��.
    }
    break;
    case PACKET_TYPE::CS_MOVE_STOP:
    {
        // ���� ���õ� Ŭ���̾�Ʈ�� �������� �������� ���� ���̶� ��û
        // 1. ���� ������ ó��
        // 2. PACKET_SC_MOVE_STOP �� ��ε�ĳ����
        // 3. ���� ������ �̵� ���� ������ �˸�

        //=====================================================================================================================================
        // 1. ���� ������ ó��
        //=====================================================================================================================================
        PACKET_CS_MOVE_STOP* packetCSMoveStop = static_cast<PACKET_CS_MOVE_STOP*>(pData);
        session->pPlayer->SetDirection(packetCSMoveStop->direction);
        session->pPlayer->SetPosition(packetCSMoveStop->x, packetCSMoveStop->y);
         

        //=====================================================================================================================================
        // 2. PACKET_SC_MOVE_STOP �� ��ε�ĳ����
        //=====================================================================================================================================
        PACKET_SC_MOVE_STOP packetSCMoveStop;
        packetSCMoveStop.playerID = session->uid;
        packetSCMoveStop.direction = packetCSMoveStop->direction;
        packetSCMoveStop.x = packetCSMoveStop->x;
        packetSCMoveStop.y = packetCSMoveStop->y;

        BroadcastPacket(session, &packetSCMoveStop, sizeof(PACKET_SC_MOVE_STOP), static_cast<UINT8>(PACKET_TYPE::SC_MOVE_STOP));

        //=====================================================================================================================================
        // 3. ���� ������ �̵� ���� ������ �˸�
        //=====================================================================================================================================
        session->pPlayer->SetFlag(FLAG_MOVING, false);
    }
    break;
    case PACKET_TYPE::SC_MOVE_STOP:
    {
        // �̰� ���⼭ �ȿ�. ���� ������ �׶� ó��.
    }
    break;
    case PACKET_TYPE::CS_ATTACK1:
    {
        // Ŭ���̾�Ʈ�� ���� ���� �޽����� ����.
        // g_clientList�� ��ȸ�ϸ� ���� 1�� ������ �����ؼ� �������� �־���.
        // 1. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 2, 3�� ���� ����
        // 2. dfPACKET_SC_ATTACK1 �� ��ε�ĳ����
        // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
        // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� ��


        //=====================================================================================================================================
        // 1. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 2, 3�� ���� ����
        //=====================================================================================================================================

        // ���� �ٶ󺸴� ���⿡ ���� ���� ������ �޶���.
        UINT16 left, right, top, bottom;
        UINT16 posX, posY;
        session->pPlayer->getPosition(posX, posY);

        // ������ �ٶ󺸰� �־��ٸ�
        if (session->pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
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
            if (client->uid == session->uid)
                continue;

            client->pPlayer->getPosition(posX, posY);

            // �ٸ� �÷��̾��� ��ǥ�� ���� ������ ���� ���
            if (posX >= left && posX <= right &&
                posY >= top && posY <= bottom)
            {
                //=====================================================================================================================================
                // 2. dfPACKET_SC_ATTACK1 �� ��ε�ĳ����
                //=====================================================================================================================================
                PACKET_SC_ATTACK1 packetAttack1;
                packetAttack1.direction = session->pPlayer->GetFacingDirection();
                packetAttack1.playerID = session->uid;
                session->pPlayer->getPosition(packetAttack1.x, packetAttack1.y);
                BroadcastPacket(nullptr, &packetAttack1, sizeof(packetAttack1), static_cast<UINT8>(PACKET_TYPE::SC_ATTACK1));

                //=====================================================================================================================================
                // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
                //=====================================================================================================================================
                // 1�� �������� �Ե��� ��
                client->pPlayer->Damaged(dfATTACK1_DAMAGE);

                PACKET_SC_DAMAGE packetDamage;
                packetDamage.attackPlayerID = session->uid;
                packetDamage.damagedHP = client->pPlayer->GetHp();
                packetDamage.damagedPlayerID = client->uid;

                BroadcastPacket(nullptr, &packetDamage, sizeof(packetDamage), static_cast<UINT8>(PACKET_TYPE::SC_DAMAGE));

                //=====================================================================================================================================
                // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� ��
                //=====================================================================================================================================
                if (packetDamage.damagedHP <= 0)
                {
                    PACKET_SC_DELETE_CHARACTER packetDeleteCharacter;
                    packetDeleteCharacter.playerID = client->uid;

                    BroadcastPacket(nullptr, &packetDeleteCharacter, sizeof(packetDeleteCharacter), static_cast<UINT8>(PACKET_TYPE::SC_DELETE_CHARACTER));

                    // �������� ������ �� �ֵ��� isAlive�� false�� ����
                    client->isAlive = false;
                }

                // ���⼭ break�� ���ָ� ���� ������
                break;
            }
        }
    }
    break;
    case PACKET_TYPE::SC_ATTACK1:
    {

    }
    break;
    case PACKET_TYPE::CS_ATTACK2:
    {
        // Ŭ���̾�Ʈ�� ���� ���� �޽����� ����.
        // g_clientList�� ��ȸ�ϸ� ���� 2�� ������ �����ؼ� �������� �־���.
        // 1. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 2, 3�� ���� ����
        // 2. dfPACKET_SC_ATTACK2 �� ��ε�ĳ����
        // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
        // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� ��


        //=====================================================================================================================================
        // 1. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 2, 3�� ���� ����
        //=====================================================================================================================================

        // ���� �ٶ󺸴� ���⿡ ���� ���� ������ �޶���.
        UINT16 left, right, top, bottom;
        UINT16 posX, posY;
        session->pPlayer->getPosition(posX, posY);

        // ������ �ٶ󺸰� �־��ٸ�
        if (session->pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
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
            if (client->uid == session->uid)
                continue;

            client->pPlayer->getPosition(posX, posY);

            // �ٸ� �÷��̾��� ��ǥ�� ���� ������ ���� ���
            if (posX >= left && posX <= right &&
                posY >= top && posY <= bottom)
            {
                //=====================================================================================================================================
                // 2. dfPACKET_SC_ATTACK1 �� ��ε�ĳ����
                //=====================================================================================================================================
                PACKET_SC_ATTACK2 packetAttack2;
                packetAttack2.direction = session->pPlayer->GetFacingDirection();
                packetAttack2.playerID = session->uid;
                session->pPlayer->getPosition(packetAttack2.x, packetAttack2.y);
                BroadcastPacket(nullptr, &packetAttack2, sizeof(packetAttack2), static_cast<UINT8>(PACKET_TYPE::SC_ATTACK2));

                //=====================================================================================================================================
                // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
                //=====================================================================================================================================
                // 1�� �������� �Ե��� ��
                client->pPlayer->Damaged(dfATTACK2_DAMAGE);

                PACKET_SC_DAMAGE packetDamage;
                packetDamage.attackPlayerID = session->uid;
                packetDamage.damagedHP = client->pPlayer->GetHp();
                packetDamage.damagedPlayerID = client->uid;

                BroadcastPacket(nullptr, &packetDamage, sizeof(packetDamage), static_cast<UINT8>(PACKET_TYPE::SC_DAMAGE));

                //=====================================================================================================================================
                // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� ��
                //=====================================================================================================================================
                if (packetDamage.damagedHP <= 0)
                {
                    PACKET_SC_DELETE_CHARACTER packetDeleteCharacter;
                    packetDeleteCharacter.playerID = client->uid;

                    BroadcastPacket(nullptr, &packetDeleteCharacter, sizeof(packetDeleteCharacter), static_cast<UINT8>(PACKET_TYPE::SC_DELETE_CHARACTER));

                    // �������� ������ �� �ֵ��� isAlive�� false�� ����
                    client->isAlive = false;
                }

                // ���⼭ break�� ���ָ� ���� ������
                break;
            }
        }
    }
    break;
    case PACKET_TYPE::SC_ATTACK2:
    {

    }
    break;
    case PACKET_TYPE::CS_ATTACK3:
    {
        // Ŭ���̾�Ʈ�� ���� ���� �޽����� ����.
       // g_clientList�� ��ȸ�ϸ� ���� 3�� ������ �����ؼ� �������� �־���.
       // 1. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 2, 3�� ���� ����
       // 2. dfPACKET_SC_ATTACK3 �� ��ε�ĳ����
       // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
       // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� ��


       //=====================================================================================================================================
       // 1. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 2, 3�� ���� ����
       //=====================================================================================================================================

       // ���� �ٶ󺸴� ���⿡ ���� ���� ������ �޶���.
        UINT16 left, right, top, bottom;
        UINT16 posX, posY;
        session->pPlayer->getPosition(posX, posY);

        // ������ �ٶ󺸰� �־��ٸ�
        if (session->pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
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
            if (client->uid == session->uid)
                continue;

            client->pPlayer->getPosition(posX, posY);

            // �ٸ� �÷��̾��� ��ǥ�� ���� ������ ���� ���
            if (posX >= left && posX <= right &&
                posY >= top && posY <= bottom)
            {
                //=====================================================================================================================================
                // 2. dfPACKET_SC_ATTACK1 �� ��ε�ĳ����
                //=====================================================================================================================================
                PACKET_SC_ATTACK3 packetAttack3;
                packetAttack3.direction = session->pPlayer->GetFacingDirection();
                packetAttack3.playerID = session->uid;
                session->pPlayer->getPosition(packetAttack3.x, packetAttack3.y);
                BroadcastPacket(nullptr, &packetAttack3, sizeof(packetAttack3), static_cast<UINT8>(PACKET_TYPE::SC_ATTACK3));

                //=====================================================================================================================================
                // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
                //=====================================================================================================================================
                // 1�� �������� �Ե��� ��
                client->pPlayer->Damaged(dfATTACK3_DAMAGE);

                PACKET_SC_DAMAGE packetDamage;
                packetDamage.attackPlayerID = session->uid;
                packetDamage.damagedHP = client->pPlayer->GetHp();
                packetDamage.damagedPlayerID = client->uid;

                BroadcastPacket(nullptr, &packetDamage, sizeof(packetDamage), static_cast<UINT8>(PACKET_TYPE::SC_DAMAGE));

                //=====================================================================================================================================
                // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� ��
                //=====================================================================================================================================
                if (packetDamage.damagedHP <= 0)
                {
                    PACKET_SC_DELETE_CHARACTER packetDeleteCharacter;
                    packetDeleteCharacter.playerID = client->uid;

                    BroadcastPacket(nullptr, &packetDeleteCharacter, sizeof(packetDeleteCharacter), static_cast<UINT8>(PACKET_TYPE::SC_DELETE_CHARACTER));

                    // �������� ������ �� �ֵ��� isAlive�� false�� ����
                    client->isAlive = false;
                }

                // ���⼭ break�� ���ָ� ���� ������
                break;
            }
        }
    }
    break;
    case PACKET_TYPE::SC_ATTACK3:
    {

    }
    break;
    case PACKET_TYPE::SC_DAMAGE:
    {

    }
    break;
    case PACKET_TYPE::CS_SYNC:
    {

    }
    break;
    case PACKET_TYPE::SC_SYNC:
    {

    }
    break;
    default:
        NotifyClientDisconnected(session);
        break;
    }
}

void processSendPacket(SESSION* session, PACKET_TYPE pt)
{
    switch (pt)
    {
    case PACKET_TYPE::SC_CREATE_MY_CHARACTER:
    {
        // Ŭ���̾�Ʈ�� �������� �� ���۵Ǵ� ��Ŷ. ������ Ŭ���̾�Ʈ���� �����ִ� ��Ŷ�̴�.

        session->flagField = 0;

        CPlayer* pPlayer = new CPlayer(
            rand() % (dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT) + dfRANGE_MOVE_LEFT,
            rand() % (dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP) + dfRANGE_MOVE_TOP,
            dfPACKET_MOVE_DIR_LL,
            100
        );
        pPlayer->SetFlagField(&session->flagField);

        session->pPlayer = pPlayer;
        session->uid = g_id;

        // ������ ������ ����
        // 1. PACKET_SC_CREATE_MY_CHARACTER �� client���� ����
        // 2. PACKET_SC_CREATE_OTHER_CHARACTER �� client�� ������ ��� ��ε�ĳ��Ʈ.
        // 3. PACKET_SC_CREATE_OTHER_CHARACTER �� g_clientList�� �ִ� ��� ĳ���� ������ ��� client���� ����

        //=====================================================================================================================================
        // 1. PACKET_SC_CREATE_MY_CHARACTER �� client���� ����
        //=====================================================================================================================================

        PACKET_SC_CREATE_MY_CHARACTER packetCreateMyCharacter;

        // ���� �ۼ�
        packetCreateMyCharacter.direction = session->pPlayer->GetDirection();
        packetCreateMyCharacter.hp = session->pPlayer->GetHp();
        packetCreateMyCharacter.playerID = session->uid;
        session->pPlayer->getPosition(packetCreateMyCharacter.x, packetCreateMyCharacter.y);

        UnicastPacket(session, &packetCreateMyCharacter, sizeof(packetCreateMyCharacter), static_cast<UINT8>(PACKET_TYPE::SC_CREATE_MY_CHARACTER));


        //=====================================================================================================================================
        // 2. PACKET_SC_CREATE_OTHER_CHARACTER �� client�� ������ ��� ��ε�ĳ��Ʈ.
        //=====================================================================================================================================

        PACKET_SC_CREATE_OTHER_CHARACTER packetCreateOtherCharacter;

        // �̷��� �ϴ� ������ �ϴ� ��Ŷ�� ���� ������ �����ϴϱ� �̷��� ó��. ���� �޶����ų� ���� ���� ��ġ�� �ٲ�� ���ۼ����.
        memcpy(&packetCreateOtherCharacter, &packetCreateMyCharacter, sizeof(PACKET_SC_CREATE_OTHER_CHARACTER));

        // ���ӵǾ� �ִ� ��� client�� ���� ������ client�� ���� broadcast �õ�
        BroadcastPacket(NULL, &packetCreateOtherCharacter, sizeof(packetCreateOtherCharacter), static_cast<UINT8>(PACKET_TYPE::SC_CREATE_OTHER_CHARACTER));


        //=====================================================================================================================================
        // 3. PACKET_SC_CREATE_OTHER_CHARACTER �� g_clientList�� �ִ� ��� ĳ���� ������ ��� client���� ����
        //=====================================================================================================================================

        // ���ο� ������ �õ��ϴ� Ŭ���̾�Ʈ�� ���� Ŭ���̾�Ʈ �������� ����
        for (const auto& client : g_clientList)
        {
            packetCreateOtherCharacter.direction = client->pPlayer->GetDirection();
            packetCreateOtherCharacter.hp = client->pPlayer->GetHp();
            packetCreateOtherCharacter.playerID = client->uid;
            client->pPlayer->getPosition(packetCreateOtherCharacter.x, packetCreateOtherCharacter.y);

            UnicastPacket(session, &packetCreateOtherCharacter, sizeof(packetCreateOtherCharacter), static_cast<UINT8>(PACKET_TYPE::SC_CREATE_OTHER_CHARACTER));
        }


        /*UINT16 x, y;
        session->pPlayer->getPosition(x, y);
        std::cout << session->uid << " / " << x << ", " << y << "\n";*/
    }
    break;

    case PACKET_TYPE::SC_CREATE_OTHER_CHARACTER:
    {

    }
    break;
    case PACKET_TYPE::SC_DELETE_CHARACTER:
    {

    }
    break;
    case PACKET_TYPE::CS_MOVE_START:
    {
       
    }
    break;
    case PACKET_TYPE::SC_MOVE_START:
    {
        
    }
    break;
    case PACKET_TYPE::CS_MOVE_STOP:
    {

    }
    break;
    case PACKET_TYPE::SC_MOVE_STOP:
    {
        
    }
    break;
    case PACKET_TYPE::CS_ATTACK1:
    {
       
    }
    break;
    case PACKET_TYPE::SC_ATTACK1:
    {

    }
    break;
    case PACKET_TYPE::CS_ATTACK2:
    {

    }
    break;
    case PACKET_TYPE::SC_ATTACK2:
    {

    }
    break;
    case PACKET_TYPE::CS_ATTACK3:
    {

    }
    break;
    case PACKET_TYPE::SC_ATTACK3:
    {

    }
    break;
    case PACKET_TYPE::SC_DAMAGE:
    {

    }
    break;
    case PACKET_TYPE::CS_SYNC:
    {

    }
    break;
    case PACKET_TYPE::SC_SYNC:
    {

    }
    break;
    default:
        NotifyClientDisconnected(session);
        break;
    }
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

    CPlayer* pPlayer = new CPlayer(posX, posY, direction, hp);
    Session->pPlayer = pPlayer;

    return Session;
}
