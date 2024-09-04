

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
void BroadcastData(SESSION* excludeSession, PACKET_HEADER* pPacket, UINT8 dataSize);
void BroadcastData(SESSION* excludeSession, CPacket* pPacket, UINT8 dataSize);
void BroadcastPacket(SESSION* excludeSession, PACKET_HEADER* pHeader, CPacket* pPacket);

//==========================================================================================================================================
// Unicast
//==========================================================================================================================================
void UnicastData(SESSION* includeSession, PACKET_HEADER* pPacket, UINT8 dataSize);
void UnicastData(SESSION* includeSession, CPacket* pPacket, UINT8 dataSize);
void UnicastPacket(SESSION* excludeSession, PACKET_HEADER* pHeader, CPacket* pPacket);

SESSION* createSession(SOCKET ClientSocket, SOCKADDR_IN ClientAddr, UINT32 id, UINT16 posX, UINT16 posY, UINT8 hp, UINT8 direction);
void NotifyClientDisconnected(SESSION* disconnectedSession);




void netIOProcess(void);

void netProc_Recv(SESSION* pSession);
void netProc_Accept();
void netProc_Send(SESSION* pSession);

bool PacketProc(SESSION* pSession, PACKET_TYPE packetType, CPacket* pPacket);

bool netPacketProc_MoveStart(SESSION* pSession, CPacket* pPacket);
bool netPacketProc_MoveStop(SESSION* pSession, CPacket* pPacket);
bool netPacketProc_ATTACK1(SESSION* pSession, CPacket* pPacket);
bool netPacketProc_ATTACK2(SESSION* pSession, CPacket* pPacket);
bool netPacketProc_ATTACK3(SESSION* pSession, CPacket* pPacket);

void Update(void);

DWORD g_targetFPS;			    // 1초당 목표 프레임
DWORD g_targetFrame;		    // 1초당 주어지는 시간 -> 1000 / targetFPS
DWORD g_currentServerTime;		// 서버 로직이 시작될 때 초기화되고, 이후에 프레임이 지날 때 마다 targetFrameTime 만큼 더함.

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
    timeBeginPeriod(1);                     // 타이머 정밀도(해상도) 1ms 설정

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
        catch (const std::exception&)
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

void BroadcastData(SESSION* excludeSession, PACKET_HEADER* pPacket, UINT8 dataSize)
{
    for (auto& client : g_clientList)
    {
        // 해당 세션이 alive가 아니거나 제외할 세션이라면 넘어가기
        if (!client->isAlive || excludeSession == client)
            continue;

        // 메시지 전파, 세션의 sendQ에 데이터를 삽입
        int retVal = client->sendQ.Enqueue((const char*)pPacket, dataSize);

        if (retVal != dataSize)
        {
            NotifyClientDisconnected(client);

            // 이런 일은 있어선 안되지만 혹시 모르니 검사, enqueue에서 문제가 난 것은 링버퍼의 크기가 가득찼다는 의미이므로, resize할지 말지는 오류 생길시 가서 테스트하면서 진행
            int error = WSAGetLastError();
            std::cout << "Error : BroadcastData(), It might be full of sendQ" << error << "\n";
            DebugBreak();
        }
    }
}

// 클라이언트에게 데이터를 브로드캐스트하는 함수
void BroadcastData(SESSION* excludeSession, CPacket* pPacket, UINT8 dataSize)
{
    for (auto& client : g_clientList)
    {
        // 해당 세션이 alive가 아니거나 제외할 세션이라면 넘어가기
        if (!client->isAlive || excludeSession == client)
            continue;

        // 메시지 전파, 세션의 sendQ에 데이터를 삽입
        int retVal = client->sendQ.Enqueue((const char*)pPacket->GetBufferPtr(), dataSize);

        if (retVal != dataSize)
        {
            NotifyClientDisconnected(client);

            // 이런 일은 있어선 안되지만 혹시 모르니 검사, enqueue에서 문제가 난 것은 링버퍼의 크기가 가득찼다는 의미이므로, resize할지 말지는 오류 생길시 가서 테스트하면서 진행
            int error = WSAGetLastError();
            std::cout << "Error : BroadcastData(), It might be full of sendQ" << error << "\n";
            DebugBreak();
        }
    }
}

void BroadcastPacket(SESSION* excludeSession, PACKET_HEADER* pHeader, CPacket* pPacket)
{
    BroadcastData(excludeSession, pHeader, sizeof(PACKET_HEADER));
    BroadcastData(excludeSession, pPacket, pHeader->bySize);

    pPacket->MoveReadPos(pHeader->bySize);
    pPacket->Clear();
}

// 클라이언트 연결이 끊어진 경우에 호출되는 함수
void NotifyClientDisconnected(SESSION* disconnectedSession)
{
    // 만약 이미 죽었다면 NotifyClientDisconnected가 호출되었던 상태이므로 중복인 상태. 체크할 것.
    if (disconnectedSession->isAlive == false)
    {
        DebugBreak();
    }

    disconnectedSession->isAlive = false;

    // PACKET_SC_DELETE_CHARACTER 패킷 생성
    PACKET_HEADER header;
    CPacket Packet;
    mpDeleteCharacter(&header, &Packet, disconnectedSession->uid);

    // 생성된 패킷 연결이 끊긴 세션을 제외하고 브로드캐스트
    BroadcastPacket(disconnectedSession, &header, &Packet);
}

void UnicastData(SESSION* includeSession, PACKET_HEADER* pPacket, UINT8 dataSize)
{
    if (!includeSession->isAlive)
        return;

    // 세션의 sendQ에 데이터를 삽입
    int retVal = includeSession->sendQ.Enqueue((const char*)pPacket, dataSize);

    if (retVal != dataSize)
    {
        NotifyClientDisconnected(includeSession);

        // 이런 일은 있어선 안되지만 혹시 모르니 검사, enqueue에서 문제가 난 것은 링버퍼의 크기가 가득찼다는 의미이므로, resize할지 말지는 오류 생길시 가서 테스트하면서 진행
        int error = WSAGetLastError();
        std::cout << "Error : UnicastData(), It might be full of sendQ" << error << "\n";
        DebugBreak();
    }
}

// 인자로 받은 세션들에게 데이터 전송을 시도하는 함수
void UnicastData(SESSION* includeSession, CPacket* pPacket, UINT8 dataSize)
{
    if (!includeSession->isAlive)
        return;

    // 세션의 sendQ에 데이터를 삽입
    int retVal = includeSession->sendQ.Enqueue((const char*)pPacket->GetBufferPtr(), dataSize);

    if (retVal != dataSize)
    {
        NotifyClientDisconnected(includeSession);

        // 이런 일은 있어선 안되지만 혹시 모르니 검사, enqueue에서 문제가 난 것은 링버퍼의 크기가 가득찼다는 의미이므로, resize할지 말지는 오류 생길시 가서 테스트하면서 진행
        int error = WSAGetLastError();
        std::cout << "Error : UnicastData(), It might be full of sendQ" << error << "\n";
        DebugBreak();
    }
}

void UnicastPacket(SESSION* includeSession, PACKET_HEADER* pHeader, CPacket* pPacket)
{
    UnicastData(includeSession, pHeader, sizeof(PACKET_HEADER));
    UnicastData(includeSession, pPacket, pHeader->bySize);

    pPacket->MoveReadPos(pHeader->bySize);
    pPacket->Clear();
}

SESSION* createSession(SOCKET ClientSocket, SOCKADDR_IN ClientAddr, UINT32 id, UINT16 posX, UINT16 posY, UINT8 hp, UINT8 direction)
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
    pPlayer->SetSpeed(MOVE_X_PER_FRAME, MOVE_Y_PER_FRAME);

    Session->pPlayer = pPlayer;

    return Session;
}

bool netPacketProc_MoveStart(SESSION* pSession, CPacket* _pPacket)
{
    UINT8 direction;
    UINT16 x;
    UINT16 y;

    *_pPacket >> direction;
    *_pPacket >> x;
    *_pPacket >> y;

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
        std::abs(posX - x) > dfERROR_RANGE ||
        std::abs(posY - y) > dfERROR_RANGE
        )
    {
        NotifyClientDisconnected(pSession);

        // 로그 찍을거면 여기서 찍을 것
        int gapX = std::abs(posX - x);
        int gapY = std::abs(posY - y);
        DebugBreak();

        return false;
    }

    // ==========================================================================================================
    // 동작을 변경. 지금 구현에선 동작번호가 방향값. 내부에서 바라보는 방향도 변경
    // ==========================================================================================================
    pSession->pPlayer->SetDirection(direction);
    

    // ==========================================================================================================
    // 당사자를 제외한, 현재 접속중인 모든 사용자에게 패킷을 뿌림.
    // ==========================================================================================================
    PACKET_HEADER header;
    CPacket Packet;
    mpMoveStart(&header, &Packet, pSession->uid, pSession->pPlayer->GetDirection(), posX, posY);
    BroadcastPacket(pSession, &header, &Packet);

    
    //=====================================================================================================================================
    // 이동 연산 시작을 알림
    //=====================================================================================================================================
    pSession->pPlayer->SetFlag(FLAG_MOVING, true);

    return true;
}

bool netPacketProc_MoveStop(SESSION* pSession, CPacket* _pPacket)
{
    // 현재 선택된 클라이언트가 서버에게 움직임을 멈출 것이라 요청
    // 1. 받은 데이터 처리
    // 2. PACKET_SC_MOVE_STOP 을 브로드캐스팅
    // 3. 서버 내에서 이동 연산 멈춤을 알림

    //=====================================================================================================================================
    // 1. 받은 데이터 처리
    //=====================================================================================================================================
    UINT8 direction;
    UINT16 x;
    UINT16 y;
    *_pPacket >> direction;
    *_pPacket >> x;
    *_pPacket >> y;

    pSession->pPlayer->SetDirection(direction);
    pSession->pPlayer->SetPosition(x, y);


    //=====================================================================================================================================
    // 2. PACKET_SC_MOVE_STOP 를 브로드캐스팅
    //=====================================================================================================================================
    PACKET_HEADER header;
    CPacket Packet;
    mpMoveStop(&header, &Packet, pSession->uid, direction, x, y);

    BroadcastPacket(pSession, &header, &Packet);

    //=====================================================================================================================================
    // 3. 서버 내에서 이동 연산 멈춤을 알림
    //=====================================================================================================================================
    pSession->pPlayer->SetFlag(FLAG_MOVING, false);

    return true;
}

bool netPacketProc_ATTACK1(SESSION* pSession, CPacket* pPacket)
{
    // 클라이언트로 부터 공격 메시지가 들어옴.
    // g_clientList를 순회하며 공격 1의 범위를 연산해서 데미지를 넣어줌.
    // 1. dfPACKET_SC_ATTACK1 을 브로드캐스팅
    // 2. 공격받을 캐릭터를 검색. 검색에 성공하면 3, 4번 절차 진행
    // 3. dfPACKET_SC_DAMAGE 를 브로드캐스팅
    // 4. 만약 체력이 0 이하로 떨어졌다면 dfPACKET_SC_DELETE_CHARACTER 를 브로드캐스팅하고, 서버에서 삭제할 수 있도록 함 -> 이 부분은 로직에서 처리하도록 바꿈.

    //=====================================================================================================================================
    // 1. dfPACKET_SC_ATTACK1 을 브로드캐스팅
    //=====================================================================================================================================
    UINT8 direction;
    UINT16 x;
    UINT16 y;

    *pPacket >> direction;
    *pPacket >> x;
    *pPacket >> y;

    pSession->pPlayer->SetPosition(x, y);

    PACKET_HEADER header;
    CPacket Packet;
    mpAttack1(&header, &Packet, pSession->uid, pSession->pPlayer->GetDirection(), x, y);
    BroadcastPacket(pSession, &header, &Packet);

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

            CPacket Packet;
            mpDamage(&header, &Packet, pSession->uid, client->uid, client->pPlayer->GetHp());
            BroadcastPacket(nullptr, &header, &Packet);
            
            /*
            //=====================================================================================================================================
            // 4. 만약 체력이 0 이하로 떨어졌다면 dfPACKET_SC_DELETE_CHARACTER 를 브로드캐스팅하고, 서버에서 삭제할 수 있도록 함
            //=====================================================================================================================================
            if (packetDamage.damagedHP <= 0)
            {
                PACKET_SC_DELETE_CHARACTER packetDeleteCharacter;
                mpDeleteCharacter(&header, &packetDeleteCharacter, client->uid);

                BroadcastPacket(nullptr, &header, &packetDeleteCharacter);

                // 서버에서 삭제할 수 있도록 isAlive를 false로 설정
                client->isAlive = false;
            }
            */

            // 여기서 break를 없애면 광역 데미지
            break;
        }
    }

    return true;
}

bool netPacketProc_ATTACK2(SESSION* pSession, CPacket* pPacket)
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
    UINT8 direction;
    UINT16 x;
    UINT16 y;

    *pPacket >> direction;
    *pPacket >> x;
    *pPacket >> y;

    pSession->pPlayer->SetPosition(x, y);

    PACKET_HEADER header;
    CPacket Packet;
    mpAttack2(&header, &Packet, pSession->uid, pSession->pPlayer->GetDirection(), x, y);
    BroadcastPacket(pSession, &header, &Packet);

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
            CPacket Packet;
            mpDamage(&header, &Packet, pSession->uid, client->uid, client->pPlayer->GetHp());

            BroadcastPacket(nullptr, &header, &Packet);

            /*
            //=====================================================================================================================================
            // 4. 만약 체력이 0 이하로 떨어졌다면 dfPACKET_SC_DELETE_CHARACTER 를 브로드캐스팅하고, 서버에서 삭제할 수 있도록 함
            //=====================================================================================================================================
            if (packetDamage.damagedHP <= 0)
            {
                PACKET_SC_DELETE_CHARACTER packetDeleteCharacter;
                mpDeleteCharacter(&header, &packetDeleteCharacter, client->uid);

                BroadcastPacket(nullptr, &header, &packetDeleteCharacter);

                // 서버에서 삭제할 수 있도록 isAlive를 false로 설정
                client->isAlive = false;
            }
            */

            // 여기서 break를 없애면 광역 데미지
            break;
        }
    }

    return true;
}

bool netPacketProc_ATTACK3(SESSION* pSession, CPacket* pPacket)
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
    UINT8 direction;
    UINT16 x;
    UINT16 y;

    *pPacket >> direction;
    *pPacket >> x;
    *pPacket >> y;

    pSession->pPlayer->SetPosition(x, y);

    PACKET_HEADER header;
    CPacket Packet;
    mpAttack3(&header, &Packet, pSession->uid, pSession->pPlayer->GetDirection(), x, y);
    BroadcastPacket(pSession, &header, &Packet);

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

            CPacket Packet;
            mpDamage(&header, &Packet, pSession->uid, client->uid, client->pPlayer->GetHp());

            BroadcastPacket(nullptr, &header, &Packet);

            /*
            //=====================================================================================================================================
            // 4. 만약 체력이 0 이하로 떨어졌다면 dfPACKET_SC_DELETE_CHARACTER 를 브로드캐스팅하고, 서버에서 삭제할 수 있도록 함
            //=====================================================================================================================================
            if (packetDamage.damagedHP <= 0)
            {
                PACKET_SC_DELETE_CHARACTER packetDeleteCharacter;
                mpDeleteCharacter(&header, &packetDeleteCharacter, client->uid);

                BroadcastPacket(nullptr, &header, &packetDeleteCharacter);

                // 서버에서 삭제할 수 있도록 isAlive를 false로 설정
                client->isAlive = false;
            }
            */

            // 여기서 break를 없애면 광역 데미지
            break;
        }
    }

    return true;
}

bool PacketProc(SESSION* pSession, PACKET_TYPE packetType, CPacket* pPacket)
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
        CPacket Packet;
        Packet.PutData(tempPacketBuffer, recvQDeqRetVal);
        if (!PacketProc(pSession, static_cast<PACKET_TYPE>(header.byType), &Packet))
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

    PACKET_HEADER header;
    CPacket Packet;

    //=====================================================================================================================================
    // 1. 연결된 세션에 PACKET_SC_CREATE_MY_CHARACTER 를 전송
    //=====================================================================================================================================
    UINT16 posX, posY;
    Session->pPlayer->getPosition(posX, posY);
    mpCreateMyCharacter(&header, &Packet, Session->uid, Session->pPlayer->GetDirection(), posX, posY, Session->pPlayer->GetHp());

    UnicastPacket(Session, &header, &Packet);

    //=====================================================================================================================================
    // 2. PACKET_SC_CREATE_OTHER_CHARACTER 에 연결된 세션의 정보를 담아 브로드캐스트
    //=====================================================================================================================================

    mpCreateOtherCharacter(&header, &Packet, Session->uid, Session->pPlayer->GetDirection(), posX, posY, Session->pPlayer->GetHp());

    // 접속되어 있는 모든 client에 새로 접속한 client의 정보 broadcast 시도
    BroadcastPacket(NULL, &header, &Packet);

    //=====================================================================================================================================
    // 3. PACKET_SC_CREATE_OTHER_CHARACTER 에 g_clientList에 있는 모든 캐릭터 정보를 담아 연결된 세션에게 전송
    //=====================================================================================================================================

    // 새로운 연결을 시도하는 클라이언트에 기존 클라이언트 정보들을 전달
    for (const auto& client : g_clientList)
    {
        Packet.Clear();
        client->pPlayer->getPosition(posX, posY);
        mpCreateOtherCharacter(&header, &Packet, client->uid, client->pPlayer->GetDirection(), posX, posY, client->pPlayer->GetHp());

        UnicastPacket(Session, &header, &Packet);

        // 움직이고 있는 상황이라면
        if (client->pPlayer->isBitSet(FLAG_MOVING))
        {
            Packet.Clear();
            mpMoveStart(&header, &Packet, client->uid, client->pPlayer->GetDirection(), posX, posY);
            UnicastPacket(Session, &header, &Packet);
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
