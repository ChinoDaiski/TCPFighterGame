

//===================================================
// pch
//===================================================
#include "pch.h"

//===================================================
// 매니저 class
//===================================================
#include "WinSockManager.h" // 윈도우 소켓
#include "ObjectManager.h"
#include "TimerManager.h"
//===================================================

//===================================================
// 오브젝트
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
SOCKET g_listenSocket;              // listen 소켓

void netIOProcess(void);

void netProc_Recv(SESSION* pSession);
void netProc_Accept();
void netProc_Send(SESSION* pSession);

void Update(void);

DWORD g_targetFPS;			    // 1초당 목표 프레임
DWORD g_targetFrame;		    // 1초당 주어지는 시간 -> 1000 / targetFPS
DWORD g_currentServerTime;		// 서버 로직이 시작될 때 초기화되고, 이후에 프레임이 지날 때 마다 targetFrameTime 만큼 더함.

// 서버 객체에 프록시 등록하기
ServerStub serverStub;
Proxy serverProxy;

int main()
{
    //=====================================================================================================================================
    // listen 소켓 준비
    //=====================================================================================================================================
    CWinSockManager<SESSION>& winSockManager = CWinSockManager<SESSION>::getInstance();

    UINT8 options = 0;
    options |= OPTION_NONBLOCKING;

    winSockManager.StartServer(PROTOCOL_TYPE::TCP_IP, SERVERPORT, options);
    g_listenSocket = winSockManager.GetListenSocket();

    //=====================================================================================================================================
    // 서버 시간 설정
    //=====================================================================================================================================
    timeBeginPeriod(1);                     // 타이머 정밀도(해상도) 1ms 설

    g_targetFPS = 50;                       // 목표 초당 프레임
    g_targetFrame = 1000 / g_targetFPS;     // 1 프레임에 주어지는 시간
    g_currentServerTime = timeGetTime();    // 전역 서버 시간 설정

    //=====================================================================================================================================
    // 전역 유저 id 초기 값 설정
    //=====================================================================================================================================
    g_id = 0;

    //=====================================================================================================================================
    // select에서 사용할 timeVal 설정
    //=====================================================================================================================================
    TIMEVAL timeVal;
    timeVal.tv_sec = 0;
    timeVal.tv_usec = 0;

    //=====================================================================================================================================
    // 메인 로직
    //=====================================================================================================================================

    while (!g_bShutdown)
    {
        try
        {
            // 네트워크 I/O 처리
            netIOProcess();

            // 게임 로직 업데이트
            Update();
        }
        catch (const std::exception& e)
        {
            // 여기서 stackWalk 적용 예정
            g_bShutdown = true;
        }
    }

    // 현재 서버에 있는 정보 안전하게 DB등에 저장
}

// 메인 로직
void Update(void)
{
    // 프레임 맞추기
    if (timeGetTime() < (g_currentServerTime + g_targetFrame))
        return;
    else
        g_currentServerTime += g_targetFrame;

    // 비활성화된 클라이언트를 리스트에서 제거
    // 여기서 제거하는 이유는 이전 프레임에 로직 상에서 제거될 세션들의 sendQ가 비워지고 나서 제거되길 원해서 이렇게 작성.
    auto it = g_clientList.begin();
    while (it != g_clientList.end())
    {
        // 비활성화 되었다면
        if (!(*it)->isAlive)
        {
            /*
            // sendQ가 비워졌는지 확인.
            if ((*it)->sendQ.GetUseSize() != 0)
            {
                // 만약 비워지지 않았다면 의도하지 않은 결과.
                DebugBreak();
            }
            */

            // 삭제될 캐릭터 정보 브로드캐스트
            serverProxy.SC_DeleteCharacter_ForAll((*it), (*it)->uid);

            // 제거
            closesocket((*it)->sock);

            delete (*it)->pPlayer;  // 플레이어 삭제
            delete (*it);           // 세션 삭제

            it = g_clientList.erase(it);
        }
        // 활성 중이라면
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
            // 만약 체력이 0 이하로 떨어졌다면 dfPACKET_SC_DELETE_CHARACTER 를 브로드캐스팅하고, 서버에서 삭제할 수 있도록 함
            //=====================================================================================================================================
            NotifyClientDisconnected(client);
        }
        else
        {
            if (client->flagField != 0)
            {
                // 플레이어가 움직이고 있다면
                if (client->flagField & FLAG_MOVING)
                {
                    // 방향에 따라 다른 위치 움직임
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
    // 임시 수신 버퍼
    char tempRecvBuffer[RINGBUFFER_SIZE];
    char tempPacketBuffer[MAX_PACKET_SIZE + 1];

    // recv 임시 수신 버퍼로 호출
    int recvRetVal = recv(pSession->sock, tempRecvBuffer, RINGBUFFER_SIZE, 0);

    // recv 반환값이 0이라는 것은 상대방쪽에서 rst를 보냈다는 의미이므로 disconnect 처리를 해줘야한다.
    if (recvRetVal == 0)
    {
        // 더 이상 받을 값이 없으므로 break!
        NotifyClientDisconnected(pSession);
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
            NotifyClientDisconnected(pSession);
            return;
        }

        // 상대방 쪽에서 closesocket을 하면 recv 실패
        if (error == WSAECONNABORTED)
        {
            NotifyClientDisconnected(pSession);
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
            NotifyClientDisconnected(pSession);
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
    // 클라이언트가 접속했을 때 진행되는 과정
    // 백로그 큐에 접속이 되었음을 감지하고 Accept 시도
    SOCKET ClientSocket;
    SOCKADDR_IN ClientAddr;

    CWinSockManager<SESSION>& winSockManager = CWinSockManager<SESSION>::getInstance();
    SOCKET listenSocket = winSockManager.GetListenSocket();

    // accept 시도
    ClientSocket = winSockManager.Accept(ClientAddr);

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
    UINT16 posX, posY;
    Session->pPlayer->getPosition(posX, posY);
    serverProxy.SC_CreateMyCharacter_ForSingle(Session, Session->uid, Session->pPlayer->GetDirection(), posX, posY, Session->pPlayer->GetHp());

    //=====================================================================================================================================
    // 2. PACKET_SC_CREATE_OTHER_CHARACTER 에 연결된 세션의 정보를 담아 브로드캐스트
    //=====================================================================================================================================
    serverProxy.SC_CreateOtherCharacter_ForAll(Session, Session->uid, Session->pPlayer->GetDirection(), posX, posY, Session->pPlayer->GetHp());

    //=====================================================================================================================================
    // 3. PACKET_SC_CREATE_OTHER_CHARACTER 에 g_clientList에 있는 모든 캐릭터 정보를 담아 연결된 세션에게 전송
    //=====================================================================================================================================

    // 새로운 연결을 시도하는 클라이언트에 기존 클라이언트 정보들을 전달
    for (const auto& client : g_clientList)
    {
        client->pPlayer->getPosition(posX, posY);
        serverProxy.SC_CreateOtherCharacter_ForSingle(Session, client->uid, client->pPlayer->GetDirection(), posX, posY, client->pPlayer->GetHp());

        // 움직이고 있는 상황이라면
        if (client->pPlayer->isBitSet(FLAG_MOVING))
        {
            serverProxy.SC_MoveStart_ForSingle(Session, client->uid, client->pPlayer->GetDirection(), posX, posY);
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
}

void netProc_Send(SESSION* pSession)
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
                NotifyClientDisconnected(pSession);
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

void netIOProcess(void)
{
    FD_SET ReadSet;
    FD_SET WriteSet;

    FD_ZERO(&ReadSet);
    FD_ZERO(&WriteSet);

    // listen 소켓 넣기
    CWinSockManager<SESSION>& winSockManager = CWinSockManager<SESSION>::getInstance();
    SOCKET listenSocket = winSockManager.GetListenSocket();
    FD_SET(listenSocket, &ReadSet);

    // listen 소켓 및 접속중인 모든 클라이언트에 대해 SOCKET 체크
    for (auto& client : g_clientList)
    {
        // 해당 client를 ReadSet에 등록
        FD_SET(client->sock, &ReadSet);

        // 만약 보낼 데이터가 있다면 WriteSet에 등록
        if (client->sendQ.GetUseSize() > 0)
            FD_SET(client->sock, &WriteSet);
    }

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
        for (auto& client : g_clientList)
        {
            if (FD_ISSET(client->sock, &ReadSet))
            {
                --iResult;

                // recv 이벤트 처리. 메시지 수신 및 메시지 분기 로직 처리
                if (client->isAlive)
                    netProc_Recv(client);
            }

            if (FD_ISSET(client->sock, &WriteSet))
            {
                --iResult;

                // send 이벤트 처리. 메시지 송신
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
