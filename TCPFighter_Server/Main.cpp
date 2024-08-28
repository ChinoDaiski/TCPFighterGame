

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





//#define SERVERIP "192.168.30.16"
#define SERVERIP "127.0.0.1"
#define SERVERPORT 5000


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

std::list<SESSION*> g_clientList;    // 서버에 접속한 세션들에 대한 정보
SOCKET g_listenSocket;              // listen 소켓

int g_id;   // 세션들에 id를 부여하기 위한 기준. 이 값을 1씩 증가시키면서 id를 부여한다. 
bool g_bShutdown = false;   // 메인 루프가 끝났는지 여부를 결정하기 위한 변수. true면 서버의 루프를 종료한다.



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

//==========================================================================================================================================
// processPacket
//==========================================================================================================================================
void processReceivedPacket(SESSION* client, PACKET_TYPE pt, void* pData);
void processSendPacket(SESSION* client, PACKET_TYPE pt);

SESSION* createSession(SOCKET ClientSocket, SOCKADDR_IN ClientAddr, int id, int posX, int posY, int hp, int direction);
void NotifyClientDisconnected(SESSION* disconnectedSession);




void netIOProcess(void);

void netProc_Recv(SESSION* pSession);
void netProc_Accept(SESSION* pSession);
void netProc_Send(SESSION* pSession);

bool PacketProc(SESSION* pSession, PACKET_TYPE packetType, void* pPacket);

bool netPacketProc_MoveStart(SESSION* pSession, void* pPacket);
bool netPacketProc_MoveStop(SESSION* pSession, void* pPacket);
bool netPacketProc_ATTACK1(SESSION* pSession, void* pPacket);
bool netPacketProc_ATTACK2(SESSION* pSession, void* pPacket);
bool netPacketProc_ATTACK3(SESSION* pSession, void* pPacket);







void Update(void);





DWORD g_targetFPS;			    // 1초당 목표 프레임
DWORD g_targetFrame;		    // 1초당 주어지는 시간 -> 1000 / targetFPS
DWORD g_currentServerTime;		// 서버 로직이 시작될 때 초기화되고, 이후에 프레임이 지날 때 마다 targetFrameTime 만큼 더함.




int main()
{

    CTimerManager& pTimerInstance = CTimerManager::getInstance();
    pTimerInstance.InitTimer(50);
    pTimerInstance.SetStartServerTime();

    // 리슨소켓 준비
    CWinSockManager<SESSION>& winSockManager = CWinSockManager<SESSION>::getInstance();

    UINT8 options = 0;
    options |= OPTION_NONBLOCKING;

    winSockManager.StartServer(PROTOCOL_TYPE::TCP_IP, SERVERPORT, options);
    g_listenSocket = winSockManager.GetListenSocket();

    // 전역 유저 id 초기 값 설정
    g_id = 0;

    // 패킷 중 최대 크기의 패킷 사이즈 크기의 버퍼
    char packetBuffer[MAX_PACKET_SIZE + sizeof(PACKET_HEADER) + 1];

    TIMEVAL timeVal;
    timeVal.tv_sec = 0;
    timeVal.tv_usec = 0;

    // 타이머 정밀도(해상도) 1ms 설정
    timeBeginPeriod(1);

    g_targetFPS = 50;   // 목표 초당 프레임
    g_targetFrame = 1000 / 50;  // 1 프레임에 주어지는 시간

    // 전역 서버 시간 설정
    g_currentServerTime = timeGetTime();

    while (!g_bShutdown)
    {
        // 네트워크 I/O 처리
        netIOProcess();

        // 게임 로직 업데이트
        Update();
    }
}

void Update(void)
{
    //---------------------------------------------------------------
    // 프레임 로직
    //---------------------------------------------------------------

    if (timeGetTime() < (g_currentServerTime + g_targetFrame))
        return;
    else
    {
        g_currentServerTime += g_targetFrame;
    }

    for (auto& client : g_clientList)
    {
        if (0 >= client->pPlayer->GetHp())
        {
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
                }
            }
        }
    }

    //비활성화된 클라이언트를 리스트에서 제거
    auto it = g_clientList.begin();
    while (it != g_clientList.end())
    {
        // 비활성화 되었다면
        if (!(*it)->isAlive)
        {
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
}

//        //---------------------------------------------------------------
//        // recv
//        //---------------------------------------------------------------
//
//        // 셋 초기화
//        FD_ZERO(&readSet);
//
//        // readSet에 listenSocket 등록
//        FD_SET(g_listenSocket, &readSet);
//
//        // ClientList에 있는 소켓 전부 listenSocket 등록
//        for (const auto& client : g_clientList)
//        {
//            FD_SET(client->sock, &readSet);
//        }
//
//        // select 호출
//        int selectRetVal = select(0, &readSet, NULL, NULL, &timeVal);
//        // 첫 연결이나 read할 것이 없다면 무제한 대기. 
//        // 백로그 큐나 recv할 패킷이 오면 넘어간다.
//
//        if (selectRetVal == SOCKET_ERROR)
//        {
//            std::cout << "Error : readSet select(), " << WSAGetLastError() << "\n";
//            DebugBreak();
//        }
//
//        // 세션들 중 들어오는 정보가 있다면 다른 클라이언트에 broadcast 하기.
//        for (const auto& client : g_clientList)
//        {
//            // 해당 세션이 readSet에 있으면
//            BOOL isInReadSet = FD_ISSET(client->sock, &readSet);
//            while (isInReadSet)
//            {
//                // 데이터 받아서 링버퍼에 채우기
//                int directDeqSize = client->recvQ.DirectEnqueueSize();
//                int receiveRetVal = recv(client->sock, client->recvQ.GetRearBufferPtr(), directDeqSize, 0);
//
//                if (receiveRetVal == SOCKET_ERROR)
//                {
//                    int error = WSAGetLastError();
//
//                    // 수신 버퍼에 데이터가 없으므로 나중에 다시 recv 시도
//                    if (error == WSAEWOULDBLOCK)
//                        continue;
//
//                    // 수신 버퍼와의 연결이 끊어진다는, rst가 왔다는 의미.
//                    if (error == WSAECONNRESET)
//                    {
//                        // 다른 클라이언트들에 연결이 끊어졌다는 것을 알림.
//                        NotifyClientDisconnected(client);
//                        break;
//                    }
//
//                    // 상대방 쪽에서 closesocket을 하면 recv 실패
//                    if (error == WSAECONNABORTED)
//                    {
//                        NotifyClientDisconnected(client);
//                        break;
//                    }
//
//                    // 만약 WSAEWOULDBLOCK이 아닌 다른 에러라면 진짜 문제
//                    std::cout << "Error : recv(), " << WSAGetLastError() << "\n";
//                    break;
//                }
//                // recv 반환값이 0이라는 것은 상대방쪽에서 rst를 보냈다는 의미이므로 disconnect 처리를 해줘야한다.
//                else if (receiveRetVal == 0)
//                {
//                    // 더 이상 받을 값이 없으므로 break!
//                    NotifyClientDisconnected(client);
//                    //std::cout << "recv() return 0," << WSAGetLastError() << "\n";
//                    break;
//                }
//
//                client->recvQ.MoveRear(receiveRetVal);
//                break;
//            }
//
//
//
//            // 받은 데이터로부터 로직 처리에 필요한 정보 업데이트 
//
//            // 링버퍼 해석
//            // 우선 peek로 헤더를 확인, 이후에 payload값이 존재하는지 확인하여 payload 처리. 이를 링버퍼에 남아 있는 값이 헤더의 크기보다 작을 때 까지 반복한다.
//            // 물론 여기선 16byte를 기준으로 처리한다. 왜? 모든 패킷 크기가 16byte로 설정되어 있으니깐
//            while (client->recvQ.GetUseSize() >= sizeof(PACKET_HEADER))
//            {
//                PACKET_HEADER header;
//                int headerSize = sizeof(header);
//                int retVal = client->recvQ.Peek(reinterpret_cast<char*>(&header), headerSize);
//
//                // 링버퍼에서 peek으로 읽어올 수 있는 값과 헤더의 길이가 일치하지 않는다면 처리를 못한다는 의미이므로 넘긴다.
//                if (retVal != headerSize)
//                {
//                    break;
//                }
//
//                // 링버퍼에서 패킷 전체 길이만큼의 데이터가 있는지 확인한다.
//                int packetSize = header.bySize + headerSize;
//                if (client->recvQ.GetUseSize() >= packetSize)
//                {
//                    // 데이터가 있다면 dequeue, 이때 예상되는 최대 패킷 크기를 가지는 버퍼를 사용한다.
//                    int retVal = client->recvQ.Dequeue(packetBuffer, packetSize);
//                    packetBuffer[retVal] = '\0';
//
//                    // 추가적인 검사는 이미 if문에서 했으니 안한다.
//                    // casting 후 사용
//
//                    // 클라이언트로 부터 받아올 데이터는 move 관련 패킷뿐이므로 여기서는 이 패킷만 처리한다.
//                    processReceivedPacket(client, static_cast<PACKET_TYPE>(header.byType), packetBuffer + sizeof(PACKET_HEADER));
//                }
//                else
//                    break;
//            }
//        }
//
//        // listenSocket이 readSet에 남아 있다면
//        BOOL isInReadSet = FD_ISSET(g_listenSocket, &readSet);
//        if (isInReadSet)
//        {
//            // accept 시도
//            ClientSocket = winSockManager.Accept(ClientAddr);
//
//            // accept가 완료되었다면 세션에 등록 후, 해당 세션에 패킷 전송
//            SESSION* Session = createSession(ClientSocket, ClientAddr,
//                g_id,
//                rand() % (dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT) + dfRANGE_MOVE_LEFT,
//                rand() % (dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP) + dfRANGE_MOVE_TOP,
//                100,
//                dfPACKET_MOVE_DIR_LL
//            );
//
//            // 연결된 세션에 [ 자기 캐릭터를 생성 하는 프로토콜 ] 전송
//            processSendPacket(Session, PACKET_TYPE::SC_CREATE_MY_CHARACTER);
//
//            // 데이터를 보내는 중에 삭제될 수 도 있으니 살아있는 여부 검사
//            if (Session->isAlive)
//            {
//                // 모든 과정 이후 g_clientList에 세션 등록
//                g_clientList.push_back(Session);
//
//                // 전역 id값 증가
//                ++g_id;
//            }
//            else
//                delete Session;
//        }
//
//        // 비활성화된 클라이언트를 리스트에서 제거
//        auto it = g_clientList.begin();
//        while (it != g_clientList.end())
//        {
//            // 비활성화 되었다면
//            if (!(*it)->isAlive)
//            {
//                // 제거
//                closesocket((*it)->sock);
//
//                delete (*it)->pPlayer;  // 플레이어 삭제
//                delete (*it);           // 세션 삭제
//
//                it = g_clientList.erase(it);
//            }
//            // 활성 중이라면
//            else
//            {
//                ++it;
//            }
//        }
//
//
//
//        //---------------------------------------------------------------
//        // 프레임 로직
//        //---------------------------------------------------------------
//        for (auto& client : g_clientList)
//        {
//            if (client->flagField != 0)
//            {
//                // 플레이어가 움직이고 있다면
//                if (client->flagField & FLAG_MOVING)
//                {
//                    // 방향에 따라 다른 위치 움직임
//                    client->pPlayer->Move();
//
//                    UINT16 x, y;
//                    client->pPlayer->getPosition(x, y);
//                    std::cout << client->uid << " / " << x << ", " << y << "\n";
//
//                    
//                }
//
//                // 플레이어가 죽었다면
//                if (client->flagField & FLAG_DEAD)
//                {
//                    client->isAlive = false;
//                }
//            }
//        }
//
//
//
//
//
//
//
//        //---------------------------------------------------------------
//        // send
//        //---------------------------------------------------------------
//
//        // write set 적용
//        // 셋 초기화
//        FD_ZERO(&writeSet);
//
//        // ClientList에 있는 소켓 전부 writeSet 등록. 이미 위에서 한번 걸렸으니 isAlive 여부 검사하지 말고 링버퍼 크기만 검사
//        for (const auto& client : g_clientList)
//        {
//            // 만약 sendQ가 비어 있지 않다면 
//            if (client->sendQ.GetUseSize() != 0)
//                // writeSet에 등록
//                FD_SET(client->sock, &writeSet);
//        }
//
//        // select 호출
//        selectRetVal = select(0, NULL, &writeSet, NULL, &timeVal);
//        // 백로그 큐나 recv할 패킷이 오면 넘어간다.
//
//        if (selectRetVal == SOCKET_ERROR)
//        {
//            int error = WSAGetLastError();
//
//            // writeSet에 있는 하나 이상의 소켓 정보가 유효하지 않다면 10022 에러가 발생한다.
//            // 어짜피 다음에 유효 검사를 하니깐 넘어간다.
//            if (error == WSAEINVAL)
//            {
//                ;
//            }
//            else
//            {
//                std::cout << "Error : writeSet select(), " << error << "\n";
//                DebugBreak();
//            }
//        }
//
//        for (const auto& client : g_clientList)
//        {
//            // 해당 세션이 writeSet에 있으면
//            BOOL isInWriteSet = FD_ISSET(client->sock, &writeSet);
//            while (isInWriteSet)
//            {
//                // 링버퍼에 있는 데이터 한번에 send. 
//                int directDeqSize = client->sendQ.DirectDequeueSize();
//                int useSize = client->sendQ.GetUseSize();
//
//                // 사용하는 용량이 directDeqSize보다 작거나 같을 경우
//                if (useSize <= directDeqSize)
//                {
//                    int retval = send(client->sock, client->sendQ.GetFrontBufferPtr(), useSize, 0);
//
//                    // 여기서 소켓 에러 처리
//                    if (retval == SOCKET_ERROR)
//                    {
//                        int error = WSAGetLastError();
//                        
//                        // 중간에 강제로 연결 끊김.
//                        if (error == WSAECONNRESET)
//                            continue;
//
//                        // 여기서 예외 처리 작업
//                        DebugBreak();
//                    }
//
//                    // 만약 send가 실패한다면
//                    if (retval != useSize)
//                    {
//                        int error = WSAGetLastError();
//
//                        // 여기서 예외 처리 작업
//                        DebugBreak();
//                    }
//
//                    client->sendQ.MoveFront(retval);
//                }
//                // 사용하는 용량이 directDeqSize보다 클 경우
//                else
//                {
//                    int retval = send(client->sock, client->sendQ.GetFrontBufferPtr(), directDeqSize, 0);
//
//                    // 여기서 소켓 에러 처리
//                    if (retval == SOCKET_ERROR)
//                    {
//                        int error = WSAGetLastError();
//
//                        // 여기서 예외 처리 작업
//                        DebugBreak();
//                    }
//
//                    // 만약 send가 실패한다면
//                    if (retval != directDeqSize)
//                    {
//                        int error = WSAGetLastError();
//
//                        // 여기서 예외 처리 작업
//                        DebugBreak();
//                    }
//
//                    client->sendQ.MoveFront(retval);
//
//                    // 남은 부분 전송
//                    retval = send(client->sock, client->sendQ.GetFrontBufferPtr(), useSize - directDeqSize, 0);
//
//                    // 만약 send가 실패한다면
//                    if (retval != (useSize - directDeqSize))
//                    {
//                        int error = WSAGetLastError();
//
//                        // 여기서 예외 처리 작업
//                        DebugBreak();
//                    }
//
//                    client->sendQ.MoveFront(retval);
//                }
//                break;
//            }
//        }
//
//    }
//}

// 클라이언트에게 데이터를 브로드캐스트하는 함수
void BroadcastData(SESSION* excludeSession, void* pData, UINT8 dataSize)
{
    for (auto& client : g_clientList)
    {
        // 해당 세션이 alive가 아니거나 제외할 세션이라면 넘어가기
        if (!client->isAlive || excludeSession == client)
            continue;

        // 메시지 전파, 세션의 sendQ에 데이터를 삽입
        int retVal = client->sendQ.Enqueue((const char*)pData, dataSize);

        if (retVal != dataSize)
        {
            // 이런 일은 있어선 안되지만 혹시 모르니 검사, enqueue에서 문제가 난 것은 링버퍼의 크기가 가득찼다는 의미이므로, resize할지 말지는 오류 생길시 가서 테스트하면서 진행
            DebugBreak();
        }
    }
}

void BroadcastPacket(SESSION* excludeSession, PACKET_HEADER* pHeader, void* pPacket)
{
    BroadcastData(excludeSession, pHeader, sizeof(PACKET_HEADER));
    BroadcastData(excludeSession, pPacket, pHeader->bySize);
}

// 클라이언트 연결이 끊어진 경우에 호출되는 함수
void NotifyClientDisconnected(SESSION* disconnectedSession)
{
    disconnectedSession->isAlive = false;

    // PACKET_SC_DELETE_CHARACTER 패킷 생성
    PACKET_HEADER header;
    PACKET_SC_DELETE_CHARACTER packetDelete;
    mpDeleteCharacter(&header, &packetDelete, disconnectedSession->uid);

    // 생성된 패킷 연결이 끊긴 세션을 제외하고 브로드캐스트
    BroadcastPacket(disconnectedSession, &header, &packetDelete);
}

// 인자로 받은 세션들에게 데이터 전송을 시도하는 함수
void UnicastData(SESSION* includeSession, void* pData, UINT8 dataSize)
{
    if (!includeSession->isAlive)
        return;

    // 세션의 sendQ에 데이터를 삽입
    int retVal = includeSession->sendQ.Enqueue((const char*)pData, dataSize);

    if (retVal != dataSize)
    {
        // 이런 일은 있어선 안되지만 혹시 모르니 검사
        DebugBreak();
    }
}

void UnicastPacket(SESSION* includeSession, PACKET_HEADER* pHeader, void* pPacket)
{
    UnicastData(includeSession, pHeader, sizeof(PACKET_HEADER));
    UnicastData(includeSession, pPacket, pHeader->bySize);
}

//
//void processReceivedPacket(SESSION* session, PACKET_TYPE pt, void* pData)
//{
//    switch (pt)
//    {
//    case PACKET_TYPE::SC_CREATE_MY_CHARACTER:
//    {
//        // 이 패킷은 processSendPacket 전용 패킷
//    }
//    break;
//
//    case PACKET_TYPE::SC_CREATE_OTHER_CHARACTER:
//    {
//       // 이 패킷은 넘어오진 않음. 이 패킷을 작성할 타이밍에 가서 작업 예정.
//    }
//    break;
//    case PACKET_TYPE::SC_DELETE_CHARACTER:
//    {
//        // 
//    }
//    break;
//    case PACKET_TYPE::CS_MOVE_START:
//    {
//        // 현재 선택된 클라이언트가 서버에게 움직임을 시작할 것이라 요청함.
//        // 1. 받은 데이터 처리
//        // 2. PACKET_SC_MOVE_START 를 브로드캐스팅
//        // 3. 서버 내에서 이동 연산 시작을 알림
//
//        //=====================================================================================================================================
//        // 1. 받은 데이터 처리
//        //=====================================================================================================================================
//        PACKET_CS_MOVE_START* packetCSMoveStart = static_cast<PACKET_CS_MOVE_START*>(pData);
//        session->pPlayer->SetDirection(packetCSMoveStart->direction);
//        session->pPlayer->SetPosition(packetCSMoveStart->x, packetCSMoveStart->y);
//
//        //=====================================================================================================================================
//        // 2. PACKET_SC_MOVE_START 를 브로드캐스팅
//        //=====================================================================================================================================
//        PACKET_SC_MOVE_START packetMoveStart;
//        packetMoveStart.direction = packetCSMoveStart->direction;
//        packetMoveStart.playerID = session->uid;
//        packetMoveStart.x = packetCSMoveStart->x;
//        packetMoveStart.y = packetCSMoveStart->y;
//
//        BroadcastPacket(session, &packetMoveStart, sizeof(PACKET_SC_MOVE_START), static_cast<UINT8>(PACKET_TYPE::SC_MOVE_START));
//
//        //=====================================================================================================================================
//        // 3. 이동 연산 시작을 알림
//        //=====================================================================================================================================
//        session->pPlayer->SetFlag(FLAG_MOVING, true);
//    }
//    break;
//    case PACKET_TYPE::SC_MOVE_START:
//    {
//        // 이건 여기서 안옴. 올일 있으면 그때 처리.
//    }
//    break;
//    case PACKET_TYPE::CS_MOVE_STOP:
//    {
//        // 현재 선택된 클라이언트가 서버에게 움직임을 멈출 것이라 요청
//        // 1. 받은 데이터 처리
//        // 2. PACKET_SC_MOVE_STOP 을 브로드캐스팅
//        // 3. 서버 내에서 이동 연산 멈춤을 알림
//
//        //=====================================================================================================================================
//        // 1. 받은 데이터 처리
//        //=====================================================================================================================================
//        PACKET_CS_MOVE_STOP* packetCSMoveStop = static_cast<PACKET_CS_MOVE_STOP*>(pData);
//        session->pPlayer->SetDirection(packetCSMoveStop->direction);
//        session->pPlayer->SetPosition(packetCSMoveStop->x, packetCSMoveStop->y);
//         
//
//        //=====================================================================================================================================
//        // 2. PACKET_SC_MOVE_STOP 를 브로드캐스팅
//        //=====================================================================================================================================
//        PACKET_SC_MOVE_STOP packetSCMoveStop;
//        packetSCMoveStop.playerID = session->uid;
//        packetSCMoveStop.direction = packetCSMoveStop->direction;
//        packetSCMoveStop.x = packetCSMoveStop->x;
//        packetSCMoveStop.y = packetCSMoveStop->y;
//
//        BroadcastPacket(session, &packetSCMoveStop, sizeof(PACKET_SC_MOVE_STOP), static_cast<UINT8>(PACKET_TYPE::SC_MOVE_STOP));
//
//        //=====================================================================================================================================
//        // 3. 서버 내에서 이동 연산 멈춤을 알림
//        //=====================================================================================================================================
//        session->pPlayer->SetFlag(FLAG_MOVING, false);
//    }
//    break;
//    case PACKET_TYPE::SC_MOVE_STOP:
//    {
//        // 이건 여기서 안옴. 올일 있으면 그때 처리.
//    }
//    break;
//    case PACKET_TYPE::CS_ATTACK1:
//    {
//        // 클라이언트로 부터 공격 메시지가 들어옴.
//        // g_clientList를 순회하며 공격 1의 범위를 연산해서 데미지를 넣어줌.
//        // 1. dfPACKET_SC_ATTACK1 을 브로드캐스팅
//        // 2. 공격받을 캐릭터를 검색. 검색에 성공하면 3, 4번 절차 진행
//        // 3. dfPACKET_SC_DAMAGE 를 브로드캐스팅
//        // 4. 만약 체력이 0 이하로 떨어졌다면 dfPACKET_SC_DELETE_CHARACTER 를 브로드캐스팅하고, 서버에서 삭제할 수 있도록 함
//
//        //=====================================================================================================================================
//        // 1. dfPACKET_SC_ATTACK1 을 브로드캐스팅
//        //=====================================================================================================================================
//        PACKET_SC_ATTACK1 packetAttack1;
//        packetAttack1.direction = session->pPlayer->GetFacingDirection();
//        packetAttack1.playerID = session->uid;
//        session->pPlayer->getPosition(packetAttack1.x, packetAttack1.y);
//        BroadcastPacket(nullptr, &packetAttack1, sizeof(packetAttack1), static_cast<UINT8>(PACKET_TYPE::SC_ATTACK1));
//
//        //=====================================================================================================================================
//        // 2. 공격받을 캐릭터를 검색. 검색에 성공하면 3, 4번 절차 진행
//        //=====================================================================================================================================
//
//        // 내가 바라보는 방향에 따라 공격 범위가 달라짐.
//        UINT16 left, right, top, bottom;
//        UINT16 posX, posY;
//        session->pPlayer->getPosition(posX, posY);
//
//        // 왼쪽을 바라보고 있었다면
//        if (session->pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
//        {
//            left = posX - dfATTACK1_RANGE_X;
//            right = posX;
//        }
//        // 오른쪽을 바라보고 있었다면
//        else
//        {
//            left = posX;
//            right = posX + dfATTACK1_RANGE_X;
//        }
//
//        top = posY - dfATTACK1_RANGE_Y;
//        bottom = posY + dfATTACK1_RANGE_Y;
//
//        for (auto& client : g_clientList)
//        {
//            if (client->uid == session->uid)
//                continue;
//
//            client->pPlayer->getPosition(posX, posY);
//
//            // 다른 플레이어의 좌표가 공격 범위에 있을 경우
//            if (posX >= left && posX <= right &&
//                posY >= top && posY <= bottom)
//            {
//                //=====================================================================================================================================
//                // 3. dfPACKET_SC_DAMAGE 를 브로드캐스팅
//                //=====================================================================================================================================
//                // 1명만 데미지를 입도록 함
//                client->pPlayer->Damaged(dfATTACK1_DAMAGE);
//
//                PACKET_SC_DAMAGE packetDamage;
//                packetDamage.attackPlayerID = session->uid;
//                packetDamage.damagedHP = client->pPlayer->GetHp();
//                packetDamage.damagedPlayerID = client->uid;
//
//                BroadcastPacket(nullptr, &packetDamage, sizeof(packetDamage), static_cast<UINT8>(PACKET_TYPE::SC_DAMAGE));
//
//                //=====================================================================================================================================
//                // 4. 만약 체력이 0 이하로 떨어졌다면 dfPACKET_SC_DELETE_CHARACTER 를 브로드캐스팅하고, 서버에서 삭제할 수 있도록 함
//                //=====================================================================================================================================
//                if (packetDamage.damagedHP <= 0)
//                {
//                    PACKET_SC_DELETE_CHARACTER packetDeleteCharacter;
//                    packetDeleteCharacter.playerID = client->uid;
//
//                    BroadcastPacket(nullptr, &packetDeleteCharacter, sizeof(packetDeleteCharacter), static_cast<UINT8>(PACKET_TYPE::SC_DELETE_CHARACTER));
//
//                    // 서버에서 삭제할 수 있도록 isAlive를 false로 설정
//                    client->isAlive = false;
//                }
//
//                // 여기서 break를 없애면 광역 데미지
//                break;
//            }
//        }
//    }
//    break;
//    case PACKET_TYPE::SC_ATTACK1:
//    {
//
//    }
//    break;
//    case PACKET_TYPE::CS_ATTACK2:
//    {
//        // 클라이언트로 부터 공격 메시지가 들어옴.
//        // g_clientList를 순회하며 공격 1의 범위를 연산해서 데미지를 넣어줌.
//        // 1. dfPACKET_SC_ATTACK2 를 브로드캐스팅
//        // 2. 공격받을 캐릭터를 검색. 검색에 성공하면 3, 4번 절차 진행
//        // 3. dfPACKET_SC_DAMAGE 를 브로드캐스팅
//        // 4. 만약 체력이 0 이하로 떨어졌다면 dfPACKET_SC_DELETE_CHARACTER 를 브로드캐스팅하고, 서버에서 삭제할 수 있도록 함
//
//        //=====================================================================================================================================
//        // 1. dfPACKET_SC_ATTACK2 을 브로드캐스팅
//        //=====================================================================================================================================
//        PACKET_SC_ATTACK2 packetAttack2;
//        packetAttack2.direction = session->pPlayer->GetFacingDirection();
//        packetAttack2.playerID = session->uid;
//        session->pPlayer->getPosition(packetAttack2.x, packetAttack2.y);
//        BroadcastPacket(nullptr, &packetAttack2, sizeof(packetAttack2), static_cast<UINT8>(PACKET_TYPE::SC_ATTACK2));
//
//        //=====================================================================================================================================
//        // 2. 공격받을 캐릭터를 검색. 검색에 성공하면 3, 4번 절차 진행
//        //=====================================================================================================================================
//
//        // 내가 바라보는 방향에 따라 공격 범위가 달라짐.
//        UINT16 left, right, top, bottom;
//        UINT16 posX, posY;
//        session->pPlayer->getPosition(posX, posY);
//
//        // 왼쪽을 바라보고 있었다면
//        if (session->pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
//        {
//            left = posX - dfATTACK2_RANGE_X;
//            right = posX;
//        }
//        // 오른쪽을 바라보고 있었다면
//        else
//        {
//            left = posX;
//            right = posX + dfATTACK2_RANGE_X;
//        }
//
//        top = posY - dfATTACK2_RANGE_Y;
//        bottom = posY + dfATTACK2_RANGE_Y;
//
//        for (auto& client : g_clientList)
//        {
//            if (client->uid == session->uid)
//                continue;
//
//            client->pPlayer->getPosition(posX, posY);
//
//            // 다른 플레이어의 좌표가 공격 범위에 있을 경우
//            if (posX >= left && posX <= right &&
//                posY >= top && posY <= bottom)
//            {
//                //=====================================================================================================================================
//                // 3. dfPACKET_SC_DAMAGE 를 브로드캐스팅
//                //=====================================================================================================================================
//                // 1명만 데미지를 입도록 함
//                client->pPlayer->Damaged(dfATTACK2_DAMAGE);
//
//                PACKET_SC_DAMAGE packetDamage;
//                packetDamage.attackPlayerID = session->uid;
//                packetDamage.damagedHP = client->pPlayer->GetHp();
//                packetDamage.damagedPlayerID = client->uid;
//
//                BroadcastPacket(nullptr, &packetDamage, sizeof(packetDamage), static_cast<UINT8>(PACKET_TYPE::SC_DAMAGE));
//
//                //=====================================================================================================================================
//                // 4. 만약 체력이 0 이하로 떨어졌다면 dfPACKET_SC_DELETE_CHARACTER 를 브로드캐스팅하고, 서버에서 삭제할 수 있도록 함
//                //=====================================================================================================================================
//                if (packetDamage.damagedHP <= 0)
//                {
//                    PACKET_SC_DELETE_CHARACTER packetDeleteCharacter;
//                    packetDeleteCharacter.playerID = client->uid;
//
//                    BroadcastPacket(nullptr, &packetDeleteCharacter, sizeof(packetDeleteCharacter), static_cast<UINT8>(PACKET_TYPE::SC_DELETE_CHARACTER));
//
//                    // 서버에서 삭제할 수 있도록 isAlive를 false로 설정
//                    client->isAlive = false;
//                }
//
//                // 여기서 break를 없애면 광역 데미지
//                break;
//            }
//        }
//    }
//    break;
//    case PACKET_TYPE::SC_ATTACK2:
//    {
//
//    }
//    break;
//    case PACKET_TYPE::CS_ATTACK3:
//    {
//        // 클라이언트로 부터 공격 메시지가 들어옴.
//        // g_clientList를 순회하며 공격 1의 범위를 연산해서 데미지를 넣어줌.
//        // 1. dfPACKET_SC_ATTACK3 을 브로드캐스팅
//        // 2. 공격받을 캐릭터를 검색. 검색에 성공하면 3, 4번 절차 진행
//        // 3. dfPACKET_SC_DAMAGE 를 브로드캐스팅
//        // 4. 만약 체력이 0 이하로 떨어졌다면 dfPACKET_SC_DELETE_CHARACTER 를 브로드캐스팅하고, 서버에서 삭제할 수 있도록 함
//
//        //=====================================================================================================================================
//        // 1. dfPACKET_SC_ATTACK3 을 브로드캐스팅
//        //=====================================================================================================================================
//        PACKET_SC_ATTACK3 packetAttack3;
//        packetAttack3.direction = session->pPlayer->GetFacingDirection();
//        packetAttack3.playerID = session->uid;
//        session->pPlayer->getPosition(packetAttack3.x, packetAttack3.y);
//        BroadcastPacket(nullptr, &packetAttack3, sizeof(packetAttack3), static_cast<UINT8>(PACKET_TYPE::SC_ATTACK3));
//
//        //=====================================================================================================================================
//        // 2. 공격받을 캐릭터를 검색. 검색에 성공하면 3, 4번 절차 진행
//        //=====================================================================================================================================
//
//        // 내가 바라보는 방향에 따라 공격 범위가 달라짐.
//        UINT16 left, right, top, bottom;
//        UINT16 posX, posY;
//        session->pPlayer->getPosition(posX, posY);
//
//        // 왼쪽을 바라보고 있었다면
//        if (session->pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
//        {
//            left = posX - dfATTACK3_RANGE_X;
//            right = posX;
//        }
//        // 오른쪽을 바라보고 있었다면
//        else
//        {
//            left = posX;
//            right = posX + dfATTACK3_RANGE_X;
//        }
//
//        top = posY - dfATTACK3_RANGE_Y;
//        bottom = posY + dfATTACK3_RANGE_Y;
//
//        for (auto& client : g_clientList)
//        {
//            if (client->uid == session->uid)
//                continue;
//
//            client->pPlayer->getPosition(posX, posY);
//
//            // 다른 플레이어의 좌표가 공격 범위에 있을 경우
//            if (posX >= left && posX <= right &&
//                posY >= top && posY <= bottom)
//            {
//                //=====================================================================================================================================
//                // 3. dfPACKET_SC_DAMAGE 를 브로드캐스팅
//                //=====================================================================================================================================
//                // 1명만 데미지를 입도록 함
//                client->pPlayer->Damaged(dfATTACK3_DAMAGE);
//
//                PACKET_SC_DAMAGE packetDamage;
//                packetDamage.attackPlayerID = session->uid;
//                packetDamage.damagedHP = client->pPlayer->GetHp();
//                packetDamage.damagedPlayerID = client->uid;
//
//                BroadcastPacket(nullptr, &packetDamage, sizeof(packetDamage), static_cast<UINT8>(PACKET_TYPE::SC_DAMAGE));
//
//                //=====================================================================================================================================
//                // 4. 만약 체력이 0 이하로 떨어졌다면 dfPACKET_SC_DELETE_CHARACTER 를 브로드캐스팅하고, 서버에서 삭제할 수 있도록 함
//                //=====================================================================================================================================
//                if (packetDamage.damagedHP <= 0)
//                {
//                    PACKET_SC_DELETE_CHARACTER packetDeleteCharacter;
//                    packetDeleteCharacter.playerID = client->uid;
//
//                    BroadcastPacket(nullptr, &packetDeleteCharacter, sizeof(packetDeleteCharacter), static_cast<UINT8>(PACKET_TYPE::SC_DELETE_CHARACTER));
//
//                    // 서버에서 삭제할 수 있도록 isAlive를 false로 설정
//                    client->isAlive = false;
//                }
//
//                // 여기서 break를 없애면 광역 데미지
//                break;
//            }
//        }
//    }
//    break;
//    case PACKET_TYPE::SC_ATTACK3:
//    {
//
//    }
//    break;
//    case PACKET_TYPE::SC_DAMAGE:
//    {
//
//    }
//    break;
//    case PACKET_TYPE::CS_SYNC:
//    {
//
//    }
//    break;
//    case PACKET_TYPE::SC_SYNC:
//    {
//
//    }
//    break;
//    default:
//        NotifyClientDisconnected(session);
//        break;
//    }
//}
//
//void processSendPacket(SESSION* session, PACKET_TYPE pt)
//{
//    switch (pt)
//    {
//    case PACKET_TYPE::SC_CREATE_MY_CHARACTER:
//    {
//        // 클라이언트가 접속했을 때 전송되는 패킷. 접속한 클라이언트에게 날려주는 패킷이다.
//
//        session->flagField = 0;
//
//        CPlayer* pPlayer = new CPlayer(
//            rand() % (dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT) + dfRANGE_MOVE_LEFT,
//            rand() % (dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP) + dfRANGE_MOVE_TOP,
//            dfPACKET_MOVE_DIR_LL,
//            100
//        );
//        pPlayer->SetFlagField(&session->flagField);
//
//        session->pPlayer = pPlayer;
//        session->uid = g_id;
//
//        // 다음의 정보를 전송
//        // 1. PACKET_SC_CREATE_MY_CHARACTER 를 client에게 전송
//        // 2. PACKET_SC_CREATE_OTHER_CHARACTER 에 client의 정보를 담아 브로드캐스트.
//        // 3. PACKET_SC_CREATE_OTHER_CHARACTER 에 g_clientList에 있는 모든 캐릭터 정보를 담아 client에게 전송
//
//        //=====================================================================================================================================
//        // 1. PACKET_SC_CREATE_MY_CHARACTER 를 client에게 전송
//        //=====================================================================================================================================
//
//        PACKET_SC_CREATE_MY_CHARACTER packetCreateMyCharacter;
//
//        // 내용 작성
//        packetCreateMyCharacter.direction = session->pPlayer->GetDirection();
//        packetCreateMyCharacter.hp = session->pPlayer->GetHp();
//        packetCreateMyCharacter.playerID = session->uid;
//        session->pPlayer->getPosition(packetCreateMyCharacter.x, packetCreateMyCharacter.y);
//
//        UnicastPacket(session, &packetCreateMyCharacter, sizeof(packetCreateMyCharacter), static_cast<UINT8>(PACKET_TYPE::SC_CREATE_MY_CHARACTER));
//
//
//        //=====================================================================================================================================
//        // 2. PACKET_SC_CREATE_OTHER_CHARACTER 에 client의 정보를 담아 브로드캐스트.
//        //=====================================================================================================================================
//
//        PACKET_SC_CREATE_OTHER_CHARACTER packetCreateOtherCharacter;
//
//        // 이렇게 하는 이유는 일단 패킷에 들어가는 정보가 동일하니깐 이렇게 처리. 만약 달라지거나 내부 변수 위치가 바뀌면 재작성요망.
//        memcpy(&packetCreateOtherCharacter, &packetCreateMyCharacter, sizeof(PACKET_SC_CREATE_OTHER_CHARACTER));
//
//        // 접속되어 있는 모든 client에 새로 접속한 client의 정보 broadcast 시도
//        BroadcastPacket(NULL, &packetCreateOtherCharacter, sizeof(packetCreateOtherCharacter), static_cast<UINT8>(PACKET_TYPE::SC_CREATE_OTHER_CHARACTER));
//
//
//        //=====================================================================================================================================
//        // 3. PACKET_SC_CREATE_OTHER_CHARACTER 에 g_clientList에 있는 모든 캐릭터 정보를 담아 client에게 전송
//        //=====================================================================================================================================
//
//        // 새로운 연결을 시도하는 클라이언트에 기존 클라이언트 정보들을 전달
//        for (const auto& client : g_clientList)
//        {
//            packetCreateOtherCharacter.direction = client->pPlayer->GetDirection();
//            packetCreateOtherCharacter.hp = client->pPlayer->GetHp();
//            packetCreateOtherCharacter.playerID = client->uid;
//            client->pPlayer->getPosition(packetCreateOtherCharacter.x, packetCreateOtherCharacter.y);
//
//            UnicastPacket(session, &packetCreateOtherCharacter, sizeof(packetCreateOtherCharacter), static_cast<UINT8>(PACKET_TYPE::SC_CREATE_OTHER_CHARACTER));
//        }
//
//
//        /*UINT16 x, y;
//        session->pPlayer->getPosition(x, y);
//        std::cout << session->uid << " / " << x << ", " << y << "\n";*/
//    }
//    break;
//
//    case PACKET_TYPE::SC_CREATE_OTHER_CHARACTER:
//    {
//
//    }
//    break;
//    case PACKET_TYPE::SC_DELETE_CHARACTER:
//    {
//
//    }
//    break;
//    case PACKET_TYPE::CS_MOVE_START:
//    {
//       
//    }
//    break;
//    case PACKET_TYPE::SC_MOVE_START:
//    {
//        
//    }
//    break;
//    case PACKET_TYPE::CS_MOVE_STOP:
//    {
//
//    }
//    break;
//    case PACKET_TYPE::SC_MOVE_STOP:
//    {
//        
//    }
//    break;
//    case PACKET_TYPE::CS_ATTACK1:
//    {
//       
//    }
//    break;
//    case PACKET_TYPE::SC_ATTACK1:
//    {
//
//    }
//    break;
//    case PACKET_TYPE::CS_ATTACK2:
//    {
//
//    }
//    break;
//    case PACKET_TYPE::SC_ATTACK2:
//    {
//
//    }
//    break;
//    case PACKET_TYPE::CS_ATTACK3:
//    {
//
//    }
//    break;
//    case PACKET_TYPE::SC_ATTACK3:
//    {
//
//    }
//    break;
//    case PACKET_TYPE::SC_DAMAGE:
//    {
//
//    }
//    break;
//    case PACKET_TYPE::CS_SYNC:
//    {
//
//    }
//    break;
//    case PACKET_TYPE::SC_SYNC:
//    {
//
//    }
//    break;
//    default:
//        NotifyClientDisconnected(session);
//        break;
//    }
//}


SESSION* createSession(SOCKET ClientSocket, SOCKADDR_IN ClientAddr, int id, int posX, int posY, int hp, int direction)
{
    CWinSockManager<SESSION>& winSockManager = CWinSockManager<SESSION>::getInstance();

    // accept가 완료되었다면 세션에 등록 후, 해당 세션에 패킷 전송
    SESSION* Session = new SESSION;
    Session->isAlive = true;

    // 소켓 정보 추가
    Session->sock = ClientSocket;

    // IP / PORT 정보 추가
    memcpy(Session->IP, winSockManager.GetIP(ClientAddr).c_str(), sizeof(Session->IP));
    Session->port = winSockManager.GetPort(ClientAddr);

    // UID 부여
    Session->uid = id;

    Session->flagField = 0;

    CPlayer* pPlayer = new CPlayer(posX, posY, direction, hp);
    pPlayer->SetFlagField(&Session->flagField);

    Session->pPlayer = pPlayer;

    return Session;
}

bool netPacketProc_MoveStart(SESSION* pSession, void* pPacket)
{
    PACKET_CS_MOVE_START* pMovePacket = static_cast<PACKET_CS_MOVE_START*>(pPacket);

    // 메시지 수신 로그 확인
    // ==========================================================================================================
    // 서버의 위치와 받은 패킷의 위치값이 너무 큰 차이가 난다면 끊어버림                           .
    // 본 게임의 좌표 동기화 구조는 단순한 키보드 조작 (클라어안트의 션처리, 서버의 후 반영) 방식으로
    // 클라이언트의 좌표를 그대로 믿는 방식을 택하고 있음.
    // 실제 온라인 게임이라면 클라이언트에서 목적지를 공유하는 방식을 택해야함.
    // 지금은 간단한 구현을 목적으로 하고 있으므로 오차범위 내에서 클라이언트 좌표를 믿도록 한다.
    // ==========================================================================================================

    UINT16 posX, posY;
    pSession->pPlayer->getPosition(posX, posY);
    if (
        std::abs(posX - pMovePacket->x) > dfERROR_RANGE ||
        std::abs(posY - pMovePacket->y) > dfERROR_RANGE
        )
    {
        NotifyClientDisconnected(pSession);

        // 로그 찍을거면 여기서 찍을 것

        return false;
    }

    // ==========================================================================================================
    // 동작을 변경. 지금 구현에선 동작번호가 방향값. 내부에서 바라보는 방향도 변경
    // ==========================================================================================================
    pSession->pPlayer->SetDirection(pMovePacket->direction);
    

    // ==========================================================================================================
    // 당사자를 제외한, 현재 접속중인 모든 사용자에게 패킷을 뿌림.
    // ==========================================================================================================
    PACKET_HEADER header;
    PACKET_SC_MOVE_START packetSCMoveStart;
    mpMoveStart(&header, &packetSCMoveStart, pSession->uid, pSession->pPlayer->GetFacingDirection(), posX, posY);
    BroadcastPacket(pSession, &header, &packetSCMoveStart);

    return true;
}

bool netPacketProc_MoveStop(SESSION* pSession, void* pPacket)
{
    // 현재 선택된 클라이언트가 서버에게 움직임을 멈출 것이라 요청
    // 1. 받은 데이터 처리
    // 2. PACKET_SC_MOVE_STOP 을 브로드캐스팅
    // 3. 서버 내에서 이동 연산 멈춤을 알림

    //=====================================================================================================================================
    // 1. 받은 데이터 처리
    //=====================================================================================================================================
    PACKET_CS_MOVE_STOP* packetCSMoveStop = static_cast<PACKET_CS_MOVE_STOP*>(pPacket);
    pSession->pPlayer->SetDirection(packetCSMoveStop->direction);
    pSession->pPlayer->SetPosition(packetCSMoveStop->x, packetCSMoveStop->y);


    //=====================================================================================================================================
    // 2. PACKET_SC_MOVE_STOP 를 브로드캐스팅
    //=====================================================================================================================================
    PACKET_HEADER header;
    PACKET_SC_MOVE_STOP packetSCMoveStop;
    mpMoveStop(&header, &packetSCMoveStop, pSession->uid, packetCSMoveStop->direction, packetCSMoveStop->x, packetCSMoveStop->y);

    BroadcastPacket(pSession, &header, &packetSCMoveStop);

    //=====================================================================================================================================
    // 3. 서버 내에서 이동 연산 멈춤을 알림
    //=====================================================================================================================================
    pSession->pPlayer->SetFlag(FLAG_MOVING, false);

    return true;
}

bool netPacketProc_ATTACK1(SESSION* pSession, void* pPacket)
{
    // 클라이언트로 부터 공격 메시지가 들어옴.
    // g_clientList를 순회하며 공격 1의 범위를 연산해서 데미지를 넣어줌.
    // 1. dfPACKET_SC_ATTACK1 을 브로드캐스팅
    // 2. 공격받을 캐릭터를 검색. 검색에 성공하면 3, 4번 절차 진행
    // 3. dfPACKET_SC_DAMAGE 를 브로드캐스팅
    // 4. 만약 체력이 0 이하로 떨어졌다면 dfPACKET_SC_DELETE_CHARACTER 를 브로드캐스팅하고, 서버에서 삭제할 수 있도록 함

    //=====================================================================================================================================
    // 1. dfPACKET_SC_ATTACK1 을 브로드캐스팅
    //=====================================================================================================================================
    PACKET_CS_ATTACK1* packetCSAttack1 = static_cast<PACKET_CS_ATTACK1*>(pPacket);
    pSession->pPlayer->SetPosition(packetCSAttack1->x, packetCSAttack1->y);

    PACKET_HEADER header;
    PACKET_SC_ATTACK1 packetSCAttack1;
    mpAttack1(&header, &packetSCAttack1, pSession->uid, pSession->pPlayer->GetDirection(), packetCSAttack1->x, packetCSAttack1->y);
    BroadcastPacket(pSession, &header, &packetSCAttack1);

    //=====================================================================================================================================
    // 2. 공격받을 캐릭터를 검색. 검색에 성공하면 3, 4번 절차 진행
    //=====================================================================================================================================

    // 내가 바라보는 방향에 따라 공격 범위가 달라짐.
    UINT16 left, right, top, bottom;
    UINT16 posX, posY;
    pSession->pPlayer->getPosition(posX, posY);

    // 왼쪽을 바라보고 있었다면
    if (pSession->pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
    {
        left = posX - dfATTACK1_RANGE_X;
        right = posX;
    }
    // 오른쪽을 바라보고 있었다면
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

        // 다른 플레이어의 좌표가 공격 범위에 있을 경우
        if (posX >= left && posX <= right &&
            posY >= top && posY <= bottom)
        {
            //=====================================================================================================================================
            // 3. dfPACKET_SC_DAMAGE 를 브로드캐스팅
            //=====================================================================================================================================
            // 1명만 데미지를 입도록 함
            client->pPlayer->Damaged(dfATTACK1_DAMAGE);

            PACKET_SC_DAMAGE packetDamage;
            mpDamage(&header, &packetDamage, pSession->uid, client->uid, client->pPlayer->GetHp());

            BroadcastPacket(pSession, &header, &packetDamage);

            //=====================================================================================================================================
            // 4. 만약 체력이 0 이하로 떨어졌다면 dfPACKET_SC_DELETE_CHARACTER 를 브로드캐스팅하고, 서버에서 삭제할 수 있도록 함
            //=====================================================================================================================================
            if (packetDamage.damagedHP <= 0)
            {
                PACKET_SC_DELETE_CHARACTER packetDeleteCharacter;
                mpDeleteCharacter(&header, &packetDeleteCharacter, client->uid);

                BroadcastPacket(pSession, &header, &packetDeleteCharacter);

                // 서버에서 삭제할 수 있도록 isAlive를 false로 설정
                client->isAlive = false;
            }

            // 여기서 break를 없애면 광역 데미지
            break;
        }
    }

    return true;
}

bool netPacketProc_ATTACK2(SESSION* pSession, void* pPacket)
{
    // 클라이언트로 부터 공격 메시지가 들어옴.
    // g_clientList를 순회하며 공격 2의 범위를 연산해서 데미지를 넣어줌.
    // 1. dfPACKET_SC_ATTACK2 을 브로드캐스팅
    // 2. 공격받을 캐릭터를 검색. 검색에 성공하면 3, 4번 절차 진행
    // 3. dfPACKET_SC_DAMAGE 를 브로드캐스팅
    // 4. 만약 체력이 0 이하로 떨어졌다면 dfPACKET_SC_DELETE_CHARACTER 를 브로드캐스팅하고, 서버에서 삭제할 수 있도록 함

    //=====================================================================================================================================
    // 1. dfPACKET_SC_ATTACK2 을 브로드캐스팅
    //=====================================================================================================================================
    PACKET_CS_ATTACK2* packetCSAttack2 = static_cast<PACKET_CS_ATTACK2*>(pPacket);
    pSession->pPlayer->SetPosition(packetCSAttack2->x, packetCSAttack2->y);

    PACKET_HEADER header;
    PACKET_SC_ATTACK2 packetSCAttack2;
    mpAttack2(&header, &packetSCAttack2, pSession->uid, pSession->pPlayer->GetDirection(), packetCSAttack2->x, packetCSAttack2->y);
    BroadcastPacket(pSession, &header, &packetSCAttack2);

    //=====================================================================================================================================
    // 2. 공격받을 캐릭터를 검색. 검색에 성공하면 3, 4번 절차 진행
    //=====================================================================================================================================

    // 내가 바라보는 방향에 따라 공격 범위가 달라짐.
    UINT16 left, right, top, bottom;
    UINT16 posX, posY;
    pSession->pPlayer->getPosition(posX, posY);

    // 왼쪽을 바라보고 있었다면
    if (pSession->pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
    {
        left = posX - dfATTACK2_RANGE_X;
        right = posX;
    }
    // 오른쪽을 바라보고 있었다면
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

        // 다른 플레이어의 좌표가 공격 범위에 있을 경우
        if (posX >= left && posX <= right &&
            posY >= top && posY <= bottom)
        {
            //=====================================================================================================================================
            // 3. dfPACKET_SC_DAMAGE 를 브로드캐스팅
            //=====================================================================================================================================
            // 1명만 데미지를 입도록 함
            client->pPlayer->Damaged(dfATTACK2_DAMAGE);

            PACKET_SC_DAMAGE packetDamage;
            mpDamage(&header, &packetDamage, pSession->uid, client->uid, client->pPlayer->GetHp());

            BroadcastPacket(pSession, &header, &packetDamage);

            //=====================================================================================================================================
            // 4. 만약 체력이 0 이하로 떨어졌다면 dfPACKET_SC_DELETE_CHARACTER 를 브로드캐스팅하고, 서버에서 삭제할 수 있도록 함
            //=====================================================================================================================================
            if (packetDamage.damagedHP <= 0)
            {
                PACKET_SC_DELETE_CHARACTER packetDeleteCharacter;
                mpDeleteCharacter(&header, &packetDeleteCharacter, client->uid);

                BroadcastPacket(pSession, &header, &packetDeleteCharacter);

                // 서버에서 삭제할 수 있도록 isAlive를 false로 설정
                client->isAlive = false;
            }

            // 여기서 break를 없애면 광역 데미지
            break;
        }
    }

    return true;
}

bool netPacketProc_ATTACK3(SESSION* pSession, void* pPacket)
{
    // 클라이언트로 부터 공격 메시지가 들어옴.
     // g_clientList를 순회하며 공격 3의 범위를 연산해서 데미지를 넣어줌.
     // 1. dfPACKET_SC_ATTACK3 을 브로드캐스팅
     // 2. 공격받을 캐릭터를 검색. 검색에 성공하면 3, 4번 절차 진행
     // 3. dfPACKET_SC_DAMAGE 를 브로드캐스팅
     // 4. 만약 체력이 0 이하로 떨어졌다면 dfPACKET_SC_DELETE_CHARACTER 를 브로드캐스팅하고, 서버에서 삭제할 수 있도록 함

     //=====================================================================================================================================
     // 1. dfPACKET_SC_ATTACK3 을 브로드캐스팅
     //=====================================================================================================================================
    PACKET_CS_ATTACK3* packetCSAttack3 = static_cast<PACKET_CS_ATTACK3*>(pPacket);
    pSession->pPlayer->SetPosition(packetCSAttack3->x, packetCSAttack3->y);

    PACKET_HEADER header;
    PACKET_SC_ATTACK3 packetSCAttack3;
    mpAttack3(&header, &packetSCAttack3, pSession->uid, pSession->pPlayer->GetDirection(), packetCSAttack3->x, packetCSAttack3->y);
    BroadcastPacket(pSession, &header, &packetSCAttack3);

    //=====================================================================================================================================
    // 2. 공격받을 캐릭터를 검색. 검색에 성공하면 3, 4번 절차 진행
    //=====================================================================================================================================

    // 내가 바라보는 방향에 따라 공격 범위가 달라짐.
    UINT16 left, right, top, bottom;
    UINT16 posX, posY;
    pSession->pPlayer->getPosition(posX, posY);

    // 왼쪽을 바라보고 있었다면
    if (pSession->pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
    {
        left = posX - dfATTACK3_RANGE_X;
        right = posX;
    }
    // 오른쪽을 바라보고 있었다면
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

        // 다른 플레이어의 좌표가 공격 범위에 있을 경우
        if (posX >= left && posX <= right &&
            posY >= top && posY <= bottom)
        {
            //=====================================================================================================================================
            // 3. dfPACKET_SC_DAMAGE 를 브로드캐스팅
            //=====================================================================================================================================
            // 1명만 데미지를 입도록 함
            client->pPlayer->Damaged(dfATTACK3_DAMAGE);

            PACKET_SC_DAMAGE packetDamage;
            mpDamage(&header, &packetDamage, pSession->uid, client->uid, client->pPlayer->GetHp());

            BroadcastPacket(pSession, &header, &packetDamage);

            //=====================================================================================================================================
            // 4. 만약 체력이 0 이하로 떨어졌다면 dfPACKET_SC_DELETE_CHARACTER 를 브로드캐스팅하고, 서버에서 삭제할 수 있도록 함
            //=====================================================================================================================================
            if (packetDamage.damagedHP <= 0)
            {
                PACKET_SC_DELETE_CHARACTER packetDeleteCharacter;
                mpDeleteCharacter(&header, &packetDeleteCharacter, client->uid);

                BroadcastPacket(pSession, &header, &packetDeleteCharacter);

                // 서버에서 삭제할 수 있도록 isAlive를 false로 설정
                client->isAlive = false;
            }

            // 여기서 break를 없애면 광역 데미지
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
        if (!PacketProc(pSession, static_cast<PACKET_TYPE>(header.byType), tempPacketBuffer))
        {
            NotifyClientDisconnected(pSession);
            break;
        }
    }
}

void netProc_Accept()
{
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

    // 연결된 세션에 [ 자기 캐릭터를 생성 하는 프로토콜 ] 전송


    // 클라이언트가 접속했을 때 전송되는 패킷. 접속한 클라이언트에게 날려주는 패킷이다.

    // 여기 수정중!!!!!!!! 여기부터 수정할 것!!!!!
    
    // 다음의 정보를 전송
    // 1. PACKET_SC_CREATE_MY_CHARACTER 를 client에게 전송
    // 2. PACKET_SC_CREATE_OTHER_CHARACTER 에 client의 정보를 담아 브로드캐스트.
    // 3. PACKET_SC_CREATE_OTHER_CHARACTER 에 g_clientList에 있는 모든 캐릭터 정보를 담아 client에게 전송
    
    //=====================================================================================================================================
    // 1. PACKET_SC_CREATE_MY_CHARACTER 를 client에게 전송
    //=====================================================================================================================================

    PACKET_SC_CREATE_MY_CHARACTER packetCreateMyCharacter;

    // 내용 작성
    packetCreateMyCharacter.direction = session->pPlayer->GetDirection();
    packetCreateMyCharacter.hp = session->pPlayer->GetHp();
    packetCreateMyCharacter.playerID = session->uid;
    session->pPlayer->getPosition(packetCreateMyCharacter.x, packetCreateMyCharacter.y);

    UnicastPacket(session, &packetCreateMyCharacter, sizeof(packetCreateMyCharacter), static_cast<UINT8>(PACKET_TYPE::SC_CREATE_MY_CHARACTER));


    //=====================================================================================================================================
    // 2. PACKET_SC_CREATE_OTHER_CHARACTER 에 client의 정보를 담아 브로드캐스트.
    //=====================================================================================================================================

    PACKET_SC_CREATE_OTHER_CHARACTER packetCreateOtherCharacter;

    // 이렇게 하는 이유는 일단 패킷에 들어가는 정보가 동일하니깐 이렇게 처리. 만약 달라지거나 내부 변수 위치가 바뀌면 재작성요망.
    memcpy(&packetCreateOtherCharacter, &packetCreateMyCharacter, sizeof(PACKET_SC_CREATE_OTHER_CHARACTER));

    // 접속되어 있는 모든 client에 새로 접속한 client의 정보 broadcast 시도
    BroadcastPacket(NULL, &packetCreateOtherCharacter, sizeof(packetCreateOtherCharacter), static_cast<UINT8>(PACKET_TYPE::SC_CREATE_OTHER_CHARACTER));


    //=====================================================================================================================================
    // 3. PACKET_SC_CREATE_OTHER_CHARACTER 에 g_clientList에 있는 모든 캐릭터 정보를 담아 client에게 전송
    //=====================================================================================================================================

    // 새로운 연결을 시도하는 클라이언트에 기존 클라이언트 정보들을 전달
    for (const auto& client : g_clientList)
    {
        packetCreateOtherCharacter.direction = client->pPlayer->GetDirection();
        packetCreateOtherCharacter.hp = client->pPlayer->GetHp();
        packetCreateOtherCharacter.playerID = client->uid;
        client->pPlayer->getPosition(packetCreateOtherCharacter.x, packetCreateOtherCharacter.y);

        UnicastPacket(session, &packetCreateOtherCharacter, sizeof(packetCreateOtherCharacter), static_cast<UINT8>(PACKET_TYPE::SC_CREATE_OTHER_CHARACTER));
    }

    // 데이터를 보내는 중에 삭제될 수 도 있으니 살아있는 여부 검사
    if (Session->isAlive)
    {
        // 모든 과정 이후 g_clientList에 세션 등록
        g_clientList.push_back(Session);

        // 전역 id값 증가
        ++g_id;
    }
    else
        delete Session;
}

void netProc_Send(SESSION* pSession)
{
}


void netIOProcess(void)
{
    SESSION* pSession;

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

    int iResult, iCnt;
    bool bProcFlag;

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
                netProc_Recv(client);
            }

            if (FD_ISSET(client->sock, &WriteSet))
            {
                --iResult;

                // send 이벤트 처리. 메시지 송신
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
