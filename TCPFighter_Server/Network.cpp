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

    // listen 소켓 넣기
    CWinSockManager& winSockManager = CWinSockManager::getInstance();
    SOCKET listenSocket = winSockManager.GetListenSocket();
    FD_SET(listenSocket, &ReadSet);

    // listen 소켓 및 접속중인 모든 클라이언트에 대해 SOCKET 체크
    CSessionManager& sessionManger = CSessionManager::getInstance();
    auto& UserSessionMap = sessionManger.GetUserSessionMap();
    for (auto& Session : UserSessionMap)
    {
        SESSION* pSession = Session.second;

        // 해당 client를 ReadSet에 등록
        FD_SET(pSession->sock, &ReadSet);

        // 만약 보낼 데이터가 있다면 WriteSet에 등록
        if (pSession->sendQ.GetUseSize() > 0)
            FD_SET(pSession->sock, &WriteSet);
    }

    //=====================================================================================================================================
    // select에서 사용할 timeVal 설정
    //=====================================================================================================================================
    TIMEVAL timeVal;
    timeVal.tv_sec = 0;
    timeVal.tv_usec = 0;

    int iResult;

    // select 호출. 반환되는 값은 반응이 있는 소켓 갯수들의 합.
    iResult = select(0, &ReadSet, &WriteSet, NULL, &timeVal);
    // 첫 연결이나 read할 것이 없다면 무제한 대기. 
    // 백로그 큐나 recv할 패킷이 오면 넘어간다.

    // iResult 값이 0 이상이라면 읽을 데이터 / 쓸 데이터가 있다는 뜻이다.
    if (0 < iResult)
    {
        // 백로그 큐에 소켓이 있다면, accept 진행
        if (FD_ISSET(listenSocket, &ReadSet))
        {
            // accept 이벤트 처리. 접속 및 Session 생성
            netProc_Accept();
        }

        // 세션을 돌며 어떤 세션이 반응을 보였는지 확인
        for (auto& Session : UserSessionMap)
        {
            SESSION* pSession = Session.second;

            if (FD_ISSET(pSession->sock, &ReadSet))
            {
                --iResult;

                // recv 이벤트 처리. 메시지 수신 및 메시지 분기 로직 처리
                if (pSession->isAlive)
                    netProc_Recv(pSession);
            }

            if (FD_ISSET(pSession->sock, &WriteSet))
            {
                --iResult;

                // send 이벤트 처리. 메시지 송신
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
    // 클라이언트가 접속했을 때 진행되는 과정
    // 백로그 큐에 접속이 되었음을 감지하고 Accept 시도
    SOCKET ClientSocket;
    SOCKADDR_IN ClientAddr;

    CWinSockManager& winSockManager = CWinSockManager::getInstance();
    SOCKET listenSocket = winSockManager.GetListenSocket();

    // accept 시도
    ClientSocket = winSockManager.Accept(ClientAddr);

    // 세션을 만들어서 m_AcceptSessionList에 넣음
    CSessionManager& sessionManger = CSessionManager::getInstance();
    SESSION* pSession = sessionManger.CreateSession(ClientSocket, ClientAddr);
    m_AcceptSessionQueue.push(pSession);
}

/*
    // accept가 완료되었다면 세션에 등록 후, 해당 세션에 패킷 전송
    SESSION* Session = createSession(ClientSocket, ClientAddr,
        g_id,
        rand() % (dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT) + dfRANGE_MOVE_LEFT,
        rand() % (dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP) + dfRANGE_MOVE_TOP,
        100,
        dfPACKET_MOVE_DIR_LL
    );

    // 1. 연결된 세션에 PACKET_SC_CREATE_MY_CHARACTER 를 전송
    // 2. PACKET_SC_CREATE_OTHER_CHARACTER 에 연결된 세션의 정보를 담아 브로드캐스트
    // 3. PACKET_SC_CREATE_OTHER_CHARACTER 에 g_clientList에 있는 모든 캐릭터 정보를 담아 연결된 세션에게 전송

    //=====================================================================================================================================
    // 1. 연결된 세션에 PACKET_SC_CREATE_MY_CHARACTER 를 전송
    //=====================================================================================================================================

    PACKET_HEADER header;
    PACKET_SC_CREATE_MY_CHARACTER packetCreateMyCharacter;

    UINT16 posX, posY;
    Session->pPlayer->getPosition(posX, posY);
    mpCreateMyCharacter(&header, &packetCreateMyCharacter, Session->uid, Session->pPlayer->GetDirection(), posX, posY, Session->pPlayer->GetHp());

    UnicastPacket(Session, &header, &packetCreateMyCharacter);

    //=====================================================================================================================================
    // 2. PACKET_SC_CREATE_OTHER_CHARACTER 에 연결된 세션의 정보를 담아 브로드캐스트
    //=====================================================================================================================================

    PACKET_SC_CREATE_OTHER_CHARACTER packetCreateOtherCharacter;
    mpCreateOtherCharacter(&header, &packetCreateOtherCharacter, Session->uid, Session->pPlayer->GetDirection(), posX, posY, Session->pPlayer->GetHp());

    // 접속되어 있는 모든 client에 새로 접속한 client의 정보 broadcast 시도
    BroadcastPacket(NULL, &header, &packetCreateOtherCharacter);

    //=====================================================================================================================================
    // 3. PACKET_SC_CREATE_OTHER_CHARACTER 에 g_clientList에 있는 모든 캐릭터 정보를 담아 연결된 세션에게 전송
    //=====================================================================================================================================

    // 새로운 연결을 시도하는 클라이언트에 기존 클라이언트 정보들을 전달
    for (const auto& client : g_clientList)
    {
        client->pPlayer->getPosition(posX, posY);
        mpCreateOtherCharacter(&header, &packetCreateOtherCharacter, client->uid, client->pPlayer->GetDirection(), posX, posY, client->pPlayer->GetHp());

        UnicastPacket(Session, &header, &packetCreateOtherCharacter);

        // 움직이고 있는 상황이라면
        if (client->pPlayer->isBitSet(FLAG_MOVING))
        {
            PACKET_SC_MOVE_START packetSCMoveStart;
            mpMoveStart(&header, &packetSCMoveStart, client->uid, client->pPlayer->GetDirection(), posX, posY);
            UnicastPacket(Session, &header, &packetSCMoveStart);
        }
    }

    // 데이터를 보내는 중에 삭제될 수 도 있으니 살아있는 여부 검사. 원래 있어서는 안되지만 sendQ가 가득찼을 경우 에러가 발생할 수 있음. 혹시 모르니 검사
    if (Session->isAlive)
    {
        // 모든 과정 이후 g_clientList에 세션 등록
        g_clientList.push_back(Session);

        // 전역 id값 증가
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
    // 링버퍼에 있는 데이터 한번에 send. 
    int directDeqSize = pSession->sendQ.DirectDequeueSize();
    int useSize = pSession->sendQ.GetUseSize();

    // 사용하는 용량이 directDeqSize보다 작거나 같을 경우
    if (useSize <= directDeqSize)
    {
        int retval = send(pSession->sock, pSession->sendQ.GetFrontBufferPtr(), useSize, 0);

        // 여기서 소켓 에러 처리
        if (retval == SOCKET_ERROR)
        {
            int error = WSAGetLastError();

            // 중간에 강제로 연결 끊김.
            if (error == WSAECONNRESET)
            {
                CSessionManager::NotifyClientDisconnected(pSession);
                return;
            }

            // 여기서 예외 처리 작업
            DebugBreak();
        }

        // 만약 send가 실패한다면
        if (retval != useSize)
        {
            int error = WSAGetLastError();

            // 여기서 예외 처리 작업
            DebugBreak();
        }

        pSession->sendQ.MoveFront(retval);
    }
    // 사용하는 용량이 directDeqSize보다 클 경우
    else
    {
        // directDeqSize 만큼만 send
        int retval = send(pSession->sock, pSession->sendQ.GetFrontBufferPtr(), directDeqSize, 0);

        // 여기서 소켓 에러 처리
        if (retval == SOCKET_ERROR)
        {
            int error = WSAGetLastError();

            // 여기서 예외 처리 작업
            DebugBreak();
        }

        // 만약 send가 실패한다면
        if (retval != directDeqSize)
        {
            int error = WSAGetLastError();

            // 여기서 예외 처리 작업
            DebugBreak();
        }

        pSession->sendQ.MoveFront(retval);

        // 남은 부분이 있으면 어짜피 다시 들어온다. 그때 추가적으로 보낼 것이 있다면 보내는게 오버헤드가 적다. 
        // 만약 응답성이 떨어지는 것 같으면 여기를 살려서 다시 시도해볼 것.
        /*
        // 남은 부분 전송
        retval = send(pSession->sock, pSession->sendQ.GetFrontBufferPtr(), useSize - directDeqSize, 0);

        // 만약 send가 실패한다면
        if (retval != (useSize - directDeqSize))
        {
            int error = WSAGetLastError();

            // 여기서 예외 처리 작업
            DebugBreak();
        }

        pSession->sendQ.MoveFront(retval);
        */
    }
}

void CNetIOManager::netProc_Recv(SESSION* pSession)
{
    // 임시 수신 버퍼
    char tempRecvBuffer[RINGBUFFER_SIZE];
    char tempPacketBuffer[MAX_PACKET_SIZE + 1];

    // recv 임시 수신 버퍼로 호출
    int recvRetVal = recv(pSession->sock, tempRecvBuffer, RINGBUFFER_SIZE, 0);

    // recv 반환값이 0이라는 것은 상대방쪽에서 rst를 보냈다는 의미이므로 disconnect 처리를 해줘야한다.
    if (recvRetVal == 0)
    {
        // 더 이상 받을 값이 없으므로 break!
        CSessionManager::NotifyClientDisconnected(pSession);
        //std::cout << "recv() return 0," << WSAGetLastError() << "\n";
        return;
    }
    // recv 에러일 경우
    else if (recvRetVal == SOCKET_ERROR)
    {
        int error = WSAGetLastError();

        // 수신 버퍼에 데이터가 없으므로 나중에 다시 recv 시도
        if (error == WSAEWOULDBLOCK)
            return;

        // 수신 버퍼와의 연결이 끊어진다는, rst가 왔다는 의미.
        if (error == WSAECONNRESET)
        {
            // 다른 클라이언트들에 연결이 끊어졌다는 것을 알림.
            CSessionManager::NotifyClientDisconnected(pSession);
            return;
        }

        // 상대방 쪽에서 closesocket을 하면 recv 실패
        if (error == WSAECONNABORTED)
        {
            CSessionManager::NotifyClientDisconnected(pSession);
            return;
        }

        // 만약 WSAEWOULDBLOCK이 아닌 다른 에러라면 진짜 문제
        std::cout << "Error : recv(), " << WSAGetLastError() << "\n";
        DebugBreak();
    }

    // 우선 데이터를 모두 넣음.
    int recvQEnqRetVal = pSession->recvQ.Enqueue(tempRecvBuffer, recvRetVal);
    if (recvQEnqRetVal != recvRetVal)
    {
        std::cout << "Error : recvQ.Enqueue(), " << WSAGetLastError() << "\n";
        DebugBreak();
    }

    // 완료패킷 처리 부
    // RecvQ에 있는 모든 완성패킷을 해석/처리
    while (true)
    {
        // 1. RecvQ에 최소한의 사이즈가 있는지 확인. 조건은 [ 헤더 사이즈 이상의 데이터가 있는지 확인 ]하는 것.
        if (pSession->recvQ.GetUseSize() < sizeof(PACKET_HEADER))
            break;

        // 2. RecvQ에서 PACKET_HEADER 정보 Peek
        PACKET_HEADER header;
        int headerSize = sizeof(header);
        int retVal = pSession->recvQ.Peek(reinterpret_cast<char*>(&header), headerSize);

        // 원래 사이즈 검사를 해줘야 하지만 위에서 검사했으므로 넘김.
        /*
        if (retVal != headerSize)
        {
            break;
        }
        */

        // 3. header의 Code 부분 확인. CRC 확인으로 이상한 값이 있으면 disconnect 처리
        if (header.byCode != dfNETWORK_PACKET_CODE)
        {
            CSessionManager::NotifyClientDisconnected(pSession);
            break;
        }

        // 4. 헤더의 len값과 RecvQ의 데이터 사이즈 비교
        if ((header.bySize + sizeof(PACKET_HEADER)) > pSession->recvQ.GetUseSize())
            break;

        // 5. Peek 했던 헤더를 RecvQ에서 지운다.
        pSession->recvQ.MoveFront(sizeof(PACKET_HEADER));

        // 6. RecvQ에서 header의 len 크기만큼 임시 패킷 버퍼를 뽑는다.
        int recvQDeqRetVal = pSession->recvQ.Dequeue(tempPacketBuffer, header.bySize);
        tempPacketBuffer[recvQDeqRetVal] = '\0';

        // 7. 헤더의 타입에 따른 분기를 위해 패킷 프로시저 호출
        if (!m_callback(pSession, static_cast<PACKET_TYPE>(header.byType), tempPacketBuffer))
        {
            CSessionManager::NotifyClientDisconnected(pSession);
            break;
        }
    }
}
