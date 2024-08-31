#include "pch.h"
#include "PacketProc.h"

#include "Player.h"
#include "SessionManager.h"
#include "ObjectManager.h"

bool PacketProc(SESSION* pSession, PACKET_TYPE packetType, void* pPacket)
{
    CObject* pObject = nullptr;

    CObjectManager& ObjectManager = CObjectManager::getInstance();
    auto& ObjectList = ObjectManager.GetObjectList();
    for (auto& Object : ObjectList)
    {
        if (Object->m_pSession == pSession)
        {
            pObject = Object;
            break;
        }
    }

    switch (packetType)
    {
    case PACKET_TYPE::CS_MOVE_START:
        return netPacketProc_MoveStart(dynamic_cast<CPlayer*>(pObject), pPacket);
        break;
    case PACKET_TYPE::CS_MOVE_STOP:
        return netPacketProc_MoveStop(dynamic_cast<CPlayer*>(pObject), pPacket);
        break;
    case PACKET_TYPE::CS_ATTACK1:
        return netPacketProc_ATTACK1(dynamic_cast<CPlayer*>(pObject), pPacket);
        break;
    case PACKET_TYPE::CS_ATTACK2:
        return netPacketProc_ATTACK2(dynamic_cast<CPlayer*>(pObject), pPacket);
        break;
    case PACKET_TYPE::CS_ATTACK3:
        return netPacketProc_ATTACK3(dynamic_cast<CPlayer*>(pObject), pPacket);
        break;
        //case PACKET_TYPE::CS_SYNC:
        //    break;
    default:
        break;
    }

    return true;
}

bool netPacketProc_MoveStart(CPlayer* pPlayer, void* pPacket)
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
    pPlayer->getPosition(posX, posY);
    if (
        std::abs(posX - pMovePacket->x) > dfERROR_RANGE ||
        std::abs(posY - pMovePacket->y) > dfERROR_RANGE
        )
    {
        CSessionManager::NotifyClientDisconnected(pPlayer->m_pSession);

        // 로그 찍을거면 여기서 찍을 것
        int gapX = std::abs(posX - pMovePacket->x);
        int gapY = std::abs(posY - pMovePacket->y);
        DebugBreak();

        return false;
    }

    // ==========================================================================================================
    // 동작을 변경. 지금 구현에선 동작번호가 방향값. 내부에서 바라보는 방향도 변경
    // ==========================================================================================================
    pPlayer->SetDirection(pMovePacket->direction);


    // ==========================================================================================================
    // 당사자를 제외한, 현재 접속중인 모든 사용자에게 패킷을 뿌림.
    // ==========================================================================================================
    PACKET_HEADER header;
    PACKET_SC_MOVE_START packetSCMoveStart;
    mpMoveStart(&header, &packetSCMoveStart, pPlayer->m_pSession->uid, pPlayer->GetDirection(), posX, posY);
    CSessionManager::BroadcastPacket(pPlayer->m_pSession, &header, &packetSCMoveStart);


    //=====================================================================================================================================
    // 이동 연산 시작을 알림
    //=====================================================================================================================================
    pPlayer->SetFlag(FLAG_MOVING, true);

    return true;
}

bool netPacketProc_MoveStop(CPlayer* pPlayer, void* pPacket)
{
    // 현재 선택된 클라이언트가 서버에게 움직임을 멈출 것이라 요청
    // 1. 받은 데이터 처리
    // 2. PACKET_SC_MOVE_STOP 을 브로드캐스팅
    // 3. 서버 내에서 이동 연산 멈춤을 알림

    //=====================================================================================================================================
    // 1. 받은 데이터 처리
    //=====================================================================================================================================
    PACKET_CS_MOVE_STOP* packetCSMoveStop = static_cast<PACKET_CS_MOVE_STOP*>(pPacket);
    pPlayer->SetDirection(packetCSMoveStop->direction);
    pPlayer->SetPosition(packetCSMoveStop->x, packetCSMoveStop->y);


    //=====================================================================================================================================
    // 2. PACKET_SC_MOVE_STOP 를 브로드캐스팅
    //=====================================================================================================================================
    PACKET_HEADER header;
    PACKET_SC_MOVE_STOP packetSCMoveStop;
    mpMoveStop(&header, &packetSCMoveStop, pPlayer->m_pSession->uid, packetCSMoveStop->direction, packetCSMoveStop->x, packetCSMoveStop->y);

    CSessionManager::BroadcastPacket(pPlayer->m_pSession, &header, &packetSCMoveStop);

    //=====================================================================================================================================
    // 3. 서버 내에서 이동 연산 멈춤을 알림
    //=====================================================================================================================================
    pPlayer->SetFlag(FLAG_MOVING, false);

    return true;
}

bool netPacketProc_ATTACK1(CPlayer* pPlayer, void* pPacket)
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
    PACKET_CS_ATTACK1* packetCSAttack1 = static_cast<PACKET_CS_ATTACK1*>(pPacket);
    pPlayer->SetPosition(packetCSAttack1->x, packetCSAttack1->y);

    PACKET_HEADER header;
    PACKET_SC_ATTACK1 packetSCAttack1;
    mpAttack1(&header, &packetSCAttack1, pPlayer->m_pSession->uid, pPlayer->GetDirection(), packetCSAttack1->x, packetCSAttack1->y);
    CSessionManager::BroadcastPacket(pPlayer->m_pSession, &header, &packetSCAttack1);

    //=====================================================================================================================================
    // 2. 공격받을 캐릭터를 검색. 검색에 성공하면 3, 4번 절차 진행
    //=====================================================================================================================================

    // 내가 바라보는 방향에 따라 공격 범위가 달라짐.
    UINT16 left, right, top, bottom;
    UINT16 posX, posY;
    pPlayer->getPosition(posX, posY);

    // 왼쪽을 바라보고 있었다면
    if (pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
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


    CObjectManager& ObjectManager = CObjectManager::getInstance();
    auto& ObjectList = ObjectManager.GetObjectList();
    for (auto& Object : ObjectList)
    {
        if (Object == pPlayer)
            continue;

        Object->getPosition(posX, posY);

        // 다른 플레이어의 좌표가 공격 범위에 있을 경우
        if (posX >= left && posX <= right &&
            posY >= top && posY <= bottom)
        {
            //=====================================================================================================================================
            // 3. dfPACKET_SC_DAMAGE 를 브로드캐스팅
            //=====================================================================================================================================
            // 1명만 데미지를 입도록 함
            dynamic_cast<CPlayer*>(Object)->Damaged(dfATTACK1_DAMAGE);

            PACKET_SC_DAMAGE packetDamage;
            mpDamage(&header, &packetDamage, pPlayer->m_pSession->uid, Object->m_pSession->uid, dynamic_cast<CPlayer*>(Object)->GetHp());

            CSessionManager::BroadcastPacket(nullptr, &header, &packetDamage);

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

bool netPacketProc_ATTACK2(CPlayer* pPlayer, void* pPacket)
{
    // 클라이언트로 부터 공격 메시지가 들어옴.
      // g_clientList를 순회하며 공격 2의 범위를 연산해서 데미지를 넣어줌.
      // 1. dfPACKET_SC_ATTACK2 를 브로드캐스팅
      // 2. 공격받을 캐릭터를 검색. 검색에 성공하면 3, 4번 절차 진행
      // 3. dfPACKET_SC_DAMAGE 를 브로드캐스팅
      // 4. 만약 체력이 0 이하로 떨어졌다면 dfPACKET_SC_DELETE_CHARACTER 를 브로드캐스팅하고, 서버에서 삭제할 수 있도록 함 -> 이 부분은 로직에서 처리하도록 바꿈.

      //=====================================================================================================================================
      // 1. dfPACKET_SC_ATTACK2 을 브로드캐스팅
      //=====================================================================================================================================
    PACKET_CS_ATTACK2* packetCSAttack2 = static_cast<PACKET_CS_ATTACK2*>(pPacket);
    pPlayer->SetPosition(packetCSAttack2->x, packetCSAttack2->y);

    PACKET_HEADER header;
    PACKET_SC_ATTACK2 packetSCAttack2;
    mpAttack2(&header, &packetSCAttack2, pPlayer->m_pSession->uid, pPlayer->GetDirection(), packetCSAttack2->x, packetCSAttack2->y);
    CSessionManager::BroadcastPacket(pPlayer->m_pSession, &header, &packetSCAttack2);

    //=====================================================================================================================================
    // 2. 공격받을 캐릭터를 검색. 검색에 성공하면 3, 4번 절차 진행
    //=====================================================================================================================================

    // 내가 바라보는 방향에 따라 공격 범위가 달라짐.
    UINT16 left, right, top, bottom;
    UINT16 posX, posY;
    pPlayer->getPosition(posX, posY);

    // 왼쪽을 바라보고 있었다면
    if (pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
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


    CObjectManager& ObjectManager = CObjectManager::getInstance();
    auto& ObjectList = ObjectManager.GetObjectList();
    for (auto& Object : ObjectList)
    {
        if (Object == pPlayer)
            continue;

        Object->getPosition(posX, posY);

        // 다른 플레이어의 좌표가 공격 범위에 있을 경우
        if (posX >= left && posX <= right &&
            posY >= top && posY <= bottom)
        {
            //=====================================================================================================================================
            // 3. dfPACKET_SC_DAMAGE 를 브로드캐스팅
            //=====================================================================================================================================
            // 1명만 데미지를 입도록 함
            dynamic_cast<CPlayer*>(Object)->Damaged(dfATTACK2_DAMAGE);

            PACKET_SC_DAMAGE packetDamage;
            mpDamage(&header, &packetDamage, pPlayer->m_pSession->uid, Object->m_pSession->uid, dynamic_cast<CPlayer*>(Object)->GetHp());

            CSessionManager::BroadcastPacket(nullptr, &header, &packetDamage);

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

bool netPacketProc_ATTACK3(CPlayer* pPlayer, void* pPacket)
{
    // 클라이언트로 부터 공격 메시지가 들어옴.
     // g_clientList를 순회하며 공격 3의 범위를 연산해서 데미지를 넣어줌.
     // 1. dfPACKET_SC_ATTACK3 을 브로드캐스팅
     // 2. 공격받을 캐릭터를 검색. 검색에 성공하면 3, 4번 절차 진행
     // 3. dfPACKET_SC_DAMAGE 를 브로드캐스팅
     // 4. 만약 체력이 0 이하로 떨어졌다면 dfPACKET_SC_DELETE_CHARACTER 를 브로드캐스팅하고, 서버에서 삭제할 수 있도록 함 -> 이 부분은 로직에서 처리하도록 바꿈.

     //=====================================================================================================================================
     // 1. dfPACKET_SC_ATTACK3 을 브로드캐스팅
     //=====================================================================================================================================
    PACKET_CS_ATTACK3* packetCSAttack3 = static_cast<PACKET_CS_ATTACK3*>(pPacket);
    pPlayer->SetPosition(packetCSAttack3->x, packetCSAttack3->y);

    PACKET_HEADER header;
    PACKET_SC_ATTACK3 packetSCAttack3;
    mpAttack3(&header, &packetSCAttack3, pPlayer->m_pSession->uid, pPlayer->GetDirection(), packetCSAttack3->x, packetCSAttack3->y);
    CSessionManager::BroadcastPacket(pPlayer->m_pSession, &header, &packetSCAttack3);

    //=====================================================================================================================================
    // 2. 공격받을 캐릭터를 검색. 검색에 성공하면 3, 4번 절차 진행
    //=====================================================================================================================================

    // 내가 바라보는 방향에 따라 공격 범위가 달라짐.
    UINT16 left, right, top, bottom;
    UINT16 posX, posY;
    pPlayer->getPosition(posX, posY);

    // 왼쪽을 바라보고 있었다면
    if (pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
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


    CObjectManager& ObjectManager = CObjectManager::getInstance();
    auto& ObjectList = ObjectManager.GetObjectList();
    for (auto& Object : ObjectList)
    {
        if (Object == pPlayer)
            continue;

        Object->getPosition(posX, posY);

        // 다른 플레이어의 좌표가 공격 범위에 있을 경우
        if (posX >= left && posX <= right &&
            posY >= top && posY <= bottom)
        {
            //=====================================================================================================================================
            // 3. dfPACKET_SC_DAMAGE 를 브로드캐스팅
            //=====================================================================================================================================
            // 1명만 데미지를 입도록 함
            dynamic_cast<CPlayer*>(Object)->Damaged(dfATTACK3_DAMAGE);

            PACKET_SC_DAMAGE packetDamage;
            mpDamage(&header, &packetDamage, pPlayer->m_pSession->uid, Object->m_pSession->uid, dynamic_cast<CPlayer*>(Object)->GetHp());

            CSessionManager::BroadcastPacket(nullptr, &header, &packetDamage);

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
