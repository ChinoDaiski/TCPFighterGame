#include "pch.h"
#include "Network.h"

#include "WinSockManager.h"

#include "Session.h"
#include "SessionManager.h"

CNetIOManager::CNetIOManager() noexcept
{
}

CNetIOManager::~CNetIOManager() noexcept
{
}

void CNetIOManager::netIOProcess(void)
{
    FD_SET ReadSet;
    FD_SET WriteSet;

    FD_ZERO(&ReadSet);
    FD_ZERO(&WriteSet);

    // listen ���� �ֱ�
    CWinSockManager& winSockManager = CWinSockManager::getInstance();
    SOCKET listenSocket = winSockManager.GetListenSocket();
    FD_SET(listenSocket, &ReadSet);

    // listen ���� �� �������� ��� Ŭ���̾�Ʈ�� ���� SOCKET üũ
    CSessionManager& sessionManger = CSessionManager::getInstance();
    auto& UserSessionMap = sessionManger.GetUserSessionMap();
    for (auto& Session : UserSessionMap)
    {
        SESSION* pSession = Session.second;

        // �ش� client�� ReadSet�� ���
        FD_SET(pSession->sock, &ReadSet);

        // ���� ���� �����Ͱ� �ִٸ� WriteSet�� ���
        if (pSession->sendQ.GetUseSize() > 0)
            FD_SET(pSession->sock, &WriteSet);
    }

    //=====================================================================================================================================
    // select���� ����� timeVal ����
    //=====================================================================================================================================
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
        for (auto& Session : UserSessionMap)
        {
            SESSION* pSession = Session.second;

            if (FD_ISSET(pSession->sock, &ReadSet))
            {
                --iResult;

                // recv �̺�Ʈ ó��. �޽��� ���� �� �޽��� �б� ���� ó��
                if (pSession->isAlive)
                    netProc_Recv(pSession);
            }

            if (FD_ISSET(pSession->sock, &WriteSet))
            {
                --iResult;

                // send �̺�Ʈ ó��. �޽��� �۽�
                if (pSession->isAlive)
                    netProc_Send(pSession);
            }
        }
    }
    else if (iResult == SOCKET_ERROR)
    {
        std::cout << "Error : select(), " << WSAGetLastError() << "\n";
        DebugBreak();
    }
}

void CNetIOManager::netProc_Accept(void)
{
    // Ŭ���̾�Ʈ�� �������� �� ����Ǵ� ����
    // ��α� ť�� ������ �Ǿ����� �����ϰ� Accept �õ�
    SOCKET ClientSocket;
    SOCKADDR_IN ClientAddr;

    CWinSockManager& winSockManager = CWinSockManager::getInstance();
    SOCKET listenSocket = winSockManager.GetListenSocket();

    // accept �õ�
    ClientSocket = winSockManager.Accept(ClientAddr);

    // ������ ���� m_AcceptSessionList�� ����
    CSessionManager& sessionManger = CSessionManager::getInstance();
    SESSION* pSession = sessionManger.CreateSession(ClientSocket, ClientAddr);
    m_AcceptSessionQueue.push(pSession);
}

/*
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

        // �����̰� �ִ� ��Ȳ�̶��
        if (client->pPlayer->isBitSet(FLAG_MOVING))
        {
            PACKET_SC_MOVE_START packetSCMoveStart;
            mpMoveStart(&header, &packetSCMoveStart, client->uid, client->pPlayer->GetDirection(), posX, posY);
            UnicastPacket(Session, &header, &packetSCMoveStart);
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
*/

void CNetIOManager::netProc_Send(SESSION* pSession)
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
                CSessionManager::NotifyClientDisconnected(pSession);
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

void CNetIOManager::netProc_Recv(SESSION* pSession)
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
        CSessionManager::NotifyClientDisconnected(pSession);
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
            CSessionManager::NotifyClientDisconnected(pSession);
            return;
        }

        // ���� �ʿ��� closesocket�� �ϸ� recv ����
        if (error == WSAECONNABORTED)
        {
            CSessionManager::NotifyClientDisconnected(pSession);
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
            CSessionManager::NotifyClientDisconnected(pSession);
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
        if (!m_callback(pSession, static_cast<PACKET_TYPE>(header.byType), tempPacketBuffer))
        {
            CSessionManager::NotifyClientDisconnected(pSession);
            break;
        }
    }
}
