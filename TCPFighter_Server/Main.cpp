

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
#include "Network.h"
#include "SessionManager.h"
//===================================================

//===================================================
// 오브젝트
//===================================================
#include "Player.h"

#include "PacketProc.h"
#include "Session.h"


//#define SERVERIP "192.168.30.16"
#define SERVERIP "127.0.0.1"
#define SERVERPORT 5000

bool g_bShutdown = false;   // 메인 루프가 끝났는지 여부를 결정하기 위한 변수. true면 서버의 루프를 종료한다.

void Update(void);

int main()
{
    //=====================================================================================================================================
    // listen 소켓 준비
    //=====================================================================================================================================
    CWinSockManager& winSockManager = CWinSockManager::getInstance();

    UINT8 options = 0;
    options |= OPTION_NONBLOCKING;

    winSockManager.StartServer(PROTOCOL_TYPE::TCP_IP, SERVERPORT, options);

    //=====================================================================================================================================
    // 서버 시간 설정
    //=====================================================================================================================================
    CTimerManager& timerManager = CTimerManager::getInstance();
    timerManager.InitServerFrame(50);   // 서버 프레임 50으로 초기화

    //=====================================================================================================================================
    // I/O 처리를 담당하는 매니저 호출
    //=====================================================================================================================================
    CNetIOManager& NetIOManager = CNetIOManager::getInstance();
    NetIOManager.RegisterCallbackFunction(PacketProc);

    //=====================================================================================================================================
    // 메인 로직
    //=====================================================================================================================================
    while (!g_bShutdown)
    {
        // 네트워크 I/O 처리
        NetIOManager.netIOProcess();

        // 서버 프레임 확인
        if (timerManager.isStartFrame())
            // 게임 로직 업데이트
            Update();
    }
}

// 메인 로직
void Update(void)
{
    CNetIOManager& NetIOManager = CNetIOManager::getInstance();
    CObjectManager& ObjectManager = CObjectManager::getInstance();

    auto& acceptSessionQueue = NetIOManager.GetAcceptSessionQueue();
    while (!acceptSessionQueue.empty())
    {
        SESSION* pSession = acceptSessionQueue.front();
        acceptSessionQueue.pop();

        CPlayer* newbiePlayer = new CPlayer(
            rand() % (dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT) + dfRANGE_MOVE_LEFT,
            rand() % (dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP) + dfRANGE_MOVE_TOP,
            dfPACKET_MOVE_DIR_LL,
            100
        );

        newbiePlayer->SetFlagField(0);
        newbiePlayer->SetSpeed(MOVE_X_PER_FRAME, MOVE_Y_PER_FRAME);

        ObjectManager.RegisterObject(newbiePlayer, pSession);

        // 1. 연결된 세션에 PACKET_SC_CREATE_MY_CHARACTER 를 전송
        // 2. PACKET_SC_CREATE_OTHER_CHARACTER 에 연결된 세션의 정보를 담아 브로드캐스트
        // 3. PACKET_SC_CREATE_OTHER_CHARACTER 에 g_clientList에 있는 모든 캐릭터 정보를 담아 연결된 세션에게 전송

        //=====================================================================================================================================
        // 1. 연결된 세션에 PACKET_SC_CREATE_MY_CHARACTER 를 전송
        //=====================================================================================================================================

        PACKET_HEADER header;
        PACKET_SC_CREATE_MY_CHARACTER packetCreateMyCharacter;

        UINT16 posX, posY;
        newbiePlayer->getPosition(posX, posY);
        mpCreateMyCharacter(&header, &packetCreateMyCharacter, newbiePlayer->m_pSession->uid, newbiePlayer->GetDirection(), posX, posY, newbiePlayer->GetHp());

        CSessionManager::UnicastPacket(newbiePlayer->m_pSession, &header, &packetCreateMyCharacter);

        //=====================================================================================================================================
        // 2. PACKET_SC_CREATE_OTHER_CHARACTER 에 연결된 세션의 정보를 담아 브로드캐스트
        //=====================================================================================================================================

        PACKET_SC_CREATE_OTHER_CHARACTER packetCreateOtherCharacter;
        mpCreateOtherCharacter(&header, &packetCreateOtherCharacter, newbiePlayer->m_pSession->uid, newbiePlayer->GetDirection(), posX, posY, newbiePlayer->GetHp());

        // 새로 접속한 client를 제외한 접속되어 있는 모든 client에 새로 접속한 client의 정보 broadcast 시도
        CSessionManager::BroadcastPacket(newbiePlayer->m_pSession, &header, &packetCreateOtherCharacter);

        //=====================================================================================================================================
        // 3. PACKET_SC_CREATE_OTHER_CHARACTER 에 g_clientList에 있는 모든 캐릭터 정보를 담아 연결된 세션에게 전송
        //=====================================================================================================================================

        // 새로운 연결을 시도하는 클라이언트에 기존 클라이언트 정보들을 전달

        auto& ObjectList = ObjectManager.GetObjectList();
        for (const auto& Object : ObjectList)
        {
            if (Object == newbiePlayer)
                continue;

            CPlayer* pPlayer = dynamic_cast<CPlayer*>(Object);

            pPlayer->getPosition(posX, posY);
            mpCreateOtherCharacter(&header, &packetCreateOtherCharacter, pPlayer->m_pSession->uid, pPlayer->GetDirection(), posX, posY, pPlayer->GetHp());

            CSessionManager::UnicastPacket(newbiePlayer->m_pSession, &header, &packetCreateOtherCharacter);

            // 움직이고 있는 상황이라면
            if (pPlayer->isBitSet(FLAG_MOVING))
            {
                PACKET_SC_MOVE_START packetSCMoveStart;
                mpMoveStart(&header, &packetSCMoveStart, pPlayer->m_pSession->uid, pPlayer->GetDirection(), posX, posY);
                CSessionManager::UnicastPacket(newbiePlayer->m_pSession, &header, &packetSCMoveStart);
            }
        }
    }

    ObjectManager.Update(0.f);
    ObjectManager.LateUpdate(0.f);


    //// 비활성화된 클라이언트를 리스트에서 제거
    //// 여기서 제거하는 이유는 이전 프레임에 로직 상에서 제거될 세션들의 sendQ가 비워지고 나서 제거되길 원해서 이렇게 작성.
    //auto it = g_clientList.begin();
    //while (it != g_clientList.end())
    //{
    //    // 비활성화 되었다면
    //    if (!(*it)->isAlive)
    //    {
    //        // sendQ가 비워졌는지 확인.
    //        if ((*it)->sendQ.GetUseSize() != 0)
    //        {
    //            // 만약 비워지지 않았다면 의도하지 않은 결과.
    //            DebugBreak();
    //        }

    //        // 제거
    //        closesocket((*it)->sock);

    //        delete (*it)->pPlayer;  // 플레이어 삭제
    //        delete (*it);           // 세션 삭제

    //        it = g_clientList.erase(it);
    //    }
    //    // 활성 중이라면
    //    else
    //    {
    //        ++it;
    //    }
    //}

}

//SESSION* createSession(SOCKET ClientSocket, SOCKADDR_IN ClientAddr, int id, int posX, int posY, int hp, int direction)
//{
//    CWinSockManager<SESSION>& winSockManager = CWinSockManager<SESSION>::getInstance();
//
//    // accept가 완료되었다면 세션에 등록 후, 해당 세션에 패킷 전송
//    SESSION* Session = new SESSION;
//    Session->isAlive = true;
//
//    // 소켓 정보 추가
//    Session->sock = ClientSocket;
//
//    // IP / PORT 정보 추가
//    memcpy(Session->IP, winSockManager.GetIP(ClientAddr).c_str(), sizeof(Session->IP));
//    Session->port = winSockManager.GetPort(ClientAddr);
//
//    // UID 부여
//    Session->uid = id;
//
//    Session->flagField = 0;
//
//    CPlayer* pPlayer = new CPlayer(posX, posY, direction, hp);
//    pPlayer->SetFlagField(&Session->flagField);
//    pPlayer->SetSpeed(MOVE_X_PER_FRAME, MOVE_Y_PER_FRAME);
//
//    Session->pPlayer = pPlayer;
//
//    return Session;
//}
