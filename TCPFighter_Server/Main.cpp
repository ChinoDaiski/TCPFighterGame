

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

#include "Packet.h"

#include "Proxy.h"
#include "ServerStub.h"



//#define SERVERIP "192.168.30.16"
#define SERVERIP "127.0.0.1"
#define SERVERPORT 5000


bool g_bShutdown = false;
std::list<SESSION*> g_clientList;
int g_id;
SOCKET g_listenSocket;              // listen ����

void netIOProcess(void);

void netProc_Recv(SESSION* pSession);
void netProc_Accept();
void netProc_Send(SESSION* pSession);

void Update(void);

DWORD g_targetFPS;			    // 1�ʴ� ��ǥ ������
DWORD g_targetFrame;		    // 1�ʴ� �־����� �ð� -> 1000 / targetFPS
DWORD g_currentServerTime;		// ���� ������ ���۵� �� �ʱ�ȭ�ǰ�, ���Ŀ� �������� ���� �� ���� targetFrameTime ��ŭ ����.

// ���� ��ü�� ���Ͻ� ����ϱ�
ServerStub serverStub;
Proxy serverProxy;

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
    timeBeginPeriod(1);                     // Ÿ�̸� ���е�(�ػ�) 1ms ��

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
        try
        {
            // ��Ʈ��ũ I/O ó��
            netIOProcess();

            // ���� ���� ������Ʈ
            Update();
        }
        catch (const std::exception& e)
        {
            // ���⼭ stackWalk ���� ����
            g_bShutdown = true;
        }
    }

    // ���� ������ �ִ� ���� �����ϰ� DB� ����
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
            /*
            // sendQ�� ��������� Ȯ��.
            if ((*it)->sendQ.GetUseSize() != 0)
            {
                // ���� ������� �ʾҴٸ� �ǵ����� ���� ���.
                DebugBreak();
            }
            */

            // ������ ĳ���� ���� ��ε�ĳ��Ʈ
            serverProxy.SC_DeleteCharacter_ForAll((*it), (*it)->uid);

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

                    /*
                    UINT16 x, y;
                    client->pPlayer->getPosition(x, y);
                    std::cout << client->uid << " / " << x << ", " << y << "\n";
                    */
                }
            }
        }
    }
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
        CPacket Packet;
        Packet.PutData(tempPacketBuffer, recvQDeqRetVal);
        if (!serverStub.PacketProc(pSession, static_cast<PACKET_TYPE>(header.byType), &Packet))
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
    UINT16 posX, posY;
    Session->pPlayer->getPosition(posX, posY);
    serverProxy.SC_CreateMyCharacter_ForSingle(Session, Session->uid, Session->pPlayer->GetDirection(), posX, posY, Session->pPlayer->GetHp());

    //=====================================================================================================================================
    // 2. PACKET_SC_CREATE_OTHER_CHARACTER �� ����� ������ ������ ��� ��ε�ĳ��Ʈ
    //=====================================================================================================================================
    serverProxy.SC_CreateOtherCharacter_ForAll(Session, Session->uid, Session->pPlayer->GetDirection(), posX, posY, Session->pPlayer->GetHp());

    //=====================================================================================================================================
    // 3. PACKET_SC_CREATE_OTHER_CHARACTER �� g_clientList�� �ִ� ��� ĳ���� ������ ��� ����� ���ǿ��� ����
    //=====================================================================================================================================

    // ���ο� ������ �õ��ϴ� Ŭ���̾�Ʈ�� ���� Ŭ���̾�Ʈ �������� ����
    for (const auto& client : g_clientList)
    {
        client->pPlayer->getPosition(posX, posY);
        serverProxy.SC_CreateOtherCharacter_ForSingle(Session, client->uid, client->pPlayer->GetDirection(), posX, posY, client->pPlayer->GetHp());

        // �����̰� �ִ� ��Ȳ�̶��
        if (client->pPlayer->isBitSet(FLAG_MOVING))
        {
            serverProxy.SC_MoveStart_ForSingle(Session, client->uid, client->pPlayer->GetDirection(), posX, posY);
        }
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
