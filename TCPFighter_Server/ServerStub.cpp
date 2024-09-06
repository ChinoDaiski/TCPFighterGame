#include "pch.h"

#include "Proxy.h"
#include "ServerStub.h"

#include "Session.h"
#include "Player.h"
#include "Packet.h"

extern Proxy serverProxy;

bool ServerStub::CS_MoveStart(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y)
{
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
    serverProxy.SC_MoveStart_ForAll(pSession, pSession->uid, pSession->pPlayer->GetDirection(), x, y);


    //=====================================================================================================================================
    // 이동 연산 시작을 알림
    //=====================================================================================================================================
    pSession->pPlayer->SetFlag(FLAG_MOVING, true);

    return true;
}

bool ServerStub::CS_MoveStop(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y)
{
    // 현재 선택된 클라이언트가 서버에게 움직임을 멈출 것이라 요청
    // 1. 받은 데이터 처리
    // 2. PACKET_SC_MOVE_STOP 을 브로드캐스팅
    // 3. 서버 내에서 이동 연산 멈춤을 알림

    //=====================================================================================================================================
    // 1. 받은 데이터 처리
    //=====================================================================================================================================
    pSession->pPlayer->SetDirection(direction);
    pSession->pPlayer->SetPosition(x, y);

    //=====================================================================================================================================
    // 2. PACKET_SC_MOVE_STOP 를 브로드캐스팅
    //=====================================================================================================================================
    serverProxy.SC_MoveStop_ForAll(pSession, pSession->uid, pSession->pPlayer->GetDirection(), x, y);

    //=====================================================================================================================================
    // 3. 서버 내에서 이동 연산 멈춤을 알림
    //=====================================================================================================================================
    pSession->pPlayer->SetFlag(FLAG_MOVING, false);

    return true;
}

bool ServerStub::CS_ATTACK1(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y)
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
    pSession->pPlayer->SetPosition(x, y);

    serverProxy.SC_Attack1_ForAll(pSession, pSession->uid, pSession->pPlayer->GetDirection(), x, y);

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

            serverProxy.SC_Damage_ForAll(pSession, pSession->uid, client->uid, client->pPlayer->GetHp());

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

bool ServerStub::CS_ATTACK2(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y)
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
    pSession->pPlayer->SetPosition(x, y);

    serverProxy.SC_Attack2_ForAll(pSession, pSession->uid, pSession->pPlayer->GetDirection(), x, y);

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

            serverProxy.SC_Damage_ForAll(pSession, pSession->uid, client->uid, client->pPlayer->GetHp());

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

bool ServerStub::CS_ATTACK3(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y)
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
    pSession->pPlayer->SetPosition(x, y);

    serverProxy.SC_Attack3_ForAll(pSession, pSession->uid, pSession->pPlayer->GetDirection(), x, y);

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

            serverProxy.SC_Damage_ForAll(pSession, pSession->uid, client->uid, client->pPlayer->GetHp());

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
