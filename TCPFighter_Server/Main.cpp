

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
#include "Network.h"
#include "SessionManager.h"
//===================================================

//===================================================
// ������Ʈ
//===================================================
#include "Player.h"

#include "PacketProc.h"
#include "Session.h"


//#define SERVERIP "192.168.30.16"
#define SERVERIP "127.0.0.1"
#define SERVERPORT 5000

bool g_bShutdown = false;   // ���� ������ �������� ���θ� �����ϱ� ���� ����. true�� ������ ������ �����Ѵ�.

void Update(void);

int main()
{
    //=====================================================================================================================================
    // listen ���� �غ�
    //=====================================================================================================================================
    CWinSockManager& winSockManager = CWinSockManager::getInstance();

    UINT8 options = 0;
    options |= OPTION_NONBLOCKING;

    winSockManager.StartServer(PROTOCOL_TYPE::TCP_IP, SERVERPORT, options);

    //=====================================================================================================================================
    // ���� �ð� ����
    //=====================================================================================================================================
    CTimerManager& timerManager = CTimerManager::getInstance();
    timerManager.InitServerFrame(50);   // ���� ������ 50���� �ʱ�ȭ

    //=====================================================================================================================================
    // I/O ó���� ����ϴ� �Ŵ��� ȣ��
    //=====================================================================================================================================
    CNetIOManager& NetIOManager = CNetIOManager::getInstance();
    NetIOManager.RegisterCallbackFunction(PacketProc);

    //=====================================================================================================================================
    // ���� ����
    //=====================================================================================================================================
    while (!g_bShutdown)
    {
        // ��Ʈ��ũ I/O ó��
        NetIOManager.netIOProcess();

        // ���� ������ Ȯ��
        if (timerManager.isStartFrame())
            // ���� ���� ������Ʈ
            Update();
    }
}

// ���� ����
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

        // 1. ����� ���ǿ� PACKET_SC_CREATE_MY_CHARACTER �� ����
        // 2. PACKET_SC_CREATE_OTHER_CHARACTER �� ����� ������ ������ ��� ��ε�ĳ��Ʈ
        // 3. PACKET_SC_CREATE_OTHER_CHARACTER �� g_clientList�� �ִ� ��� ĳ���� ������ ��� ����� ���ǿ��� ����

        //=====================================================================================================================================
        // 1. ����� ���ǿ� PACKET_SC_CREATE_MY_CHARACTER �� ����
        //=====================================================================================================================================

        PACKET_HEADER header;
        PACKET_SC_CREATE_MY_CHARACTER packetCreateMyCharacter;

        UINT16 posX, posY;
        newbiePlayer->getPosition(posX, posY);
        mpCreateMyCharacter(&header, &packetCreateMyCharacter, newbiePlayer->m_pSession->uid, newbiePlayer->GetDirection(), posX, posY, newbiePlayer->GetHp());

        CSessionManager::UnicastPacket(newbiePlayer->m_pSession, &header, &packetCreateMyCharacter);

        //=====================================================================================================================================
        // 2. PACKET_SC_CREATE_OTHER_CHARACTER �� ����� ������ ������ ��� ��ε�ĳ��Ʈ
        //=====================================================================================================================================

        PACKET_SC_CREATE_OTHER_CHARACTER packetCreateOtherCharacter;
        mpCreateOtherCharacter(&header, &packetCreateOtherCharacter, newbiePlayer->m_pSession->uid, newbiePlayer->GetDirection(), posX, posY, newbiePlayer->GetHp());

        // ���� ������ client�� ������ ���ӵǾ� �ִ� ��� client�� ���� ������ client�� ���� broadcast �õ�
        CSessionManager::BroadcastPacket(newbiePlayer->m_pSession, &header, &packetCreateOtherCharacter);

        //=====================================================================================================================================
        // 3. PACKET_SC_CREATE_OTHER_CHARACTER �� g_clientList�� �ִ� ��� ĳ���� ������ ��� ����� ���ǿ��� ����
        //=====================================================================================================================================

        // ���ο� ������ �õ��ϴ� Ŭ���̾�Ʈ�� ���� Ŭ���̾�Ʈ �������� ����

        auto& ObjectList = ObjectManager.GetObjectList();
        for (const auto& Object : ObjectList)
        {
            if (Object == newbiePlayer)
                continue;

            CPlayer* pPlayer = dynamic_cast<CPlayer*>(Object);

            pPlayer->getPosition(posX, posY);
            mpCreateOtherCharacter(&header, &packetCreateOtherCharacter, pPlayer->m_pSession->uid, pPlayer->GetDirection(), posX, posY, pPlayer->GetHp());

            CSessionManager::UnicastPacket(newbiePlayer->m_pSession, &header, &packetCreateOtherCharacter);

            // �����̰� �ִ� ��Ȳ�̶��
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


    //// ��Ȱ��ȭ�� Ŭ���̾�Ʈ�� ����Ʈ���� ����
    //// ���⼭ �����ϴ� ������ ���� �����ӿ� ���� �󿡼� ���ŵ� ���ǵ��� sendQ�� ������� ���� ���ŵǱ� ���ؼ� �̷��� �ۼ�.
    //auto it = g_clientList.begin();
    //while (it != g_clientList.end())
    //{
    //    // ��Ȱ��ȭ �Ǿ��ٸ�
    //    if (!(*it)->isAlive)
    //    {
    //        // sendQ�� ��������� Ȯ��.
    //        if ((*it)->sendQ.GetUseSize() != 0)
    //        {
    //            // ���� ������� �ʾҴٸ� �ǵ����� ���� ���.
    //            DebugBreak();
    //        }

    //        // ����
    //        closesocket((*it)->sock);

    //        delete (*it)->pPlayer;  // �÷��̾� ����
    //        delete (*it);           // ���� ����

    //        it = g_clientList.erase(it);
    //    }
    //    // Ȱ�� ���̶��
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
//    // accept�� �Ϸ�Ǿ��ٸ� ���ǿ� ��� ��, �ش� ���ǿ� ��Ŷ ����
//    SESSION* Session = new SESSION;
//    Session->isAlive = true;
//
//    // ���� ���� �߰�
//    Session->sock = ClientSocket;
//
//    // IP / PORT ���� �߰�
//    memcpy(Session->IP, winSockManager.GetIP(ClientAddr).c_str(), sizeof(Session->IP));
//    Session->port = winSockManager.GetPort(ClientAddr);
//
//    // UID �ο�
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
