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

    // �޽��� ���� �α� Ȯ��
    // ==========================================================================================================
    // ������ ��ġ�� ���� ��Ŷ�� ��ġ���� �ʹ� ū ���̰� ���ٸ� �������                           .
    // �� ������ ��ǥ ����ȭ ������ �ܼ��� Ű���� ���� (Ŭ����Ʈ�� ��ó��, ������ �� �ݿ�) �������
    // Ŭ���̾�Ʈ�� ��ǥ�� �״�� �ϴ� ����� ���ϰ� ����.
    // ���� �¶��� �����̶�� Ŭ���̾�Ʈ���� �������� �����ϴ� ����� ���ؾ���.
    // ������ ������ ������ �������� �ϰ� �����Ƿ� �������� ������ Ŭ���̾�Ʈ ��ǥ�� �ϵ��� �Ѵ�.
    // ==========================================================================================================

    UINT16 posX, posY;
    pPlayer->getPosition(posX, posY);
    if (
        std::abs(posX - pMovePacket->x) > dfERROR_RANGE ||
        std::abs(posY - pMovePacket->y) > dfERROR_RANGE
        )
    {
        CSessionManager::NotifyClientDisconnected(pPlayer->m_pSession);

        // �α� �����Ÿ� ���⼭ ���� ��
        int gapX = std::abs(posX - pMovePacket->x);
        int gapY = std::abs(posY - pMovePacket->y);
        DebugBreak();

        return false;
    }

    // ==========================================================================================================
    // ������ ����. ���� �������� ���۹�ȣ�� ���Ⱚ. ���ο��� �ٶ󺸴� ���⵵ ����
    // ==========================================================================================================
    pPlayer->SetDirection(pMovePacket->direction);


    // ==========================================================================================================
    // ����ڸ� ������, ���� �������� ��� ����ڿ��� ��Ŷ�� �Ѹ�.
    // ==========================================================================================================
    PACKET_HEADER header;
    PACKET_SC_MOVE_START packetSCMoveStart;
    mpMoveStart(&header, &packetSCMoveStart, pPlayer->m_pSession->uid, pPlayer->GetDirection(), posX, posY);
    CSessionManager::BroadcastPacket(pPlayer->m_pSession, &header, &packetSCMoveStart);


    //=====================================================================================================================================
    // �̵� ���� ������ �˸�
    //=====================================================================================================================================
    pPlayer->SetFlag(FLAG_MOVING, true);

    return true;
}

bool netPacketProc_MoveStop(CPlayer* pPlayer, void* pPacket)
{
    // ���� ���õ� Ŭ���̾�Ʈ�� �������� �������� ���� ���̶� ��û
    // 1. ���� ������ ó��
    // 2. PACKET_SC_MOVE_STOP �� ��ε�ĳ����
    // 3. ���� ������ �̵� ���� ������ �˸�

    //=====================================================================================================================================
    // 1. ���� ������ ó��
    //=====================================================================================================================================
    PACKET_CS_MOVE_STOP* packetCSMoveStop = static_cast<PACKET_CS_MOVE_STOP*>(pPacket);
    pPlayer->SetDirection(packetCSMoveStop->direction);
    pPlayer->SetPosition(packetCSMoveStop->x, packetCSMoveStop->y);


    //=====================================================================================================================================
    // 2. PACKET_SC_MOVE_STOP �� ��ε�ĳ����
    //=====================================================================================================================================
    PACKET_HEADER header;
    PACKET_SC_MOVE_STOP packetSCMoveStop;
    mpMoveStop(&header, &packetSCMoveStop, pPlayer->m_pSession->uid, packetCSMoveStop->direction, packetCSMoveStop->x, packetCSMoveStop->y);

    CSessionManager::BroadcastPacket(pPlayer->m_pSession, &header, &packetSCMoveStop);

    //=====================================================================================================================================
    // 3. ���� ������ �̵� ���� ������ �˸�
    //=====================================================================================================================================
    pPlayer->SetFlag(FLAG_MOVING, false);

    return true;
}

bool netPacketProc_ATTACK1(CPlayer* pPlayer, void* pPacket)
{
    // Ŭ���̾�Ʈ�� ���� ���� �޽����� ����.
    // g_clientList�� ��ȸ�ϸ� ���� 1�� ������ �����ؼ� �������� �־���.
    // 1. dfPACKET_SC_ATTACK1 �� ��ε�ĳ����
    // 2. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 3, 4�� ���� ����
    // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
    // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� �� -> �� �κ��� �������� ó���ϵ��� �ٲ�.

    //=====================================================================================================================================
    // 1. dfPACKET_SC_ATTACK1 �� ��ε�ĳ����
    //=====================================================================================================================================
    PACKET_CS_ATTACK1* packetCSAttack1 = static_cast<PACKET_CS_ATTACK1*>(pPacket);
    pPlayer->SetPosition(packetCSAttack1->x, packetCSAttack1->y);

    PACKET_HEADER header;
    PACKET_SC_ATTACK1 packetSCAttack1;
    mpAttack1(&header, &packetSCAttack1, pPlayer->m_pSession->uid, pPlayer->GetDirection(), packetCSAttack1->x, packetCSAttack1->y);
    CSessionManager::BroadcastPacket(pPlayer->m_pSession, &header, &packetSCAttack1);

    //=====================================================================================================================================
    // 2. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 3, 4�� ���� ����
    //=====================================================================================================================================

    // ���� �ٶ󺸴� ���⿡ ���� ���� ������ �޶���.
    UINT16 left, right, top, bottom;
    UINT16 posX, posY;
    pPlayer->getPosition(posX, posY);

    // ������ �ٶ󺸰� �־��ٸ�
    if (pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
    {
        left = posX - dfATTACK1_RANGE_X;
        right = posX;
    }
    // �������� �ٶ󺸰� �־��ٸ�
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

        // �ٸ� �÷��̾��� ��ǥ�� ���� ������ ���� ���
        if (posX >= left && posX <= right &&
            posY >= top && posY <= bottom)
        {
            //=====================================================================================================================================
            // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
            //=====================================================================================================================================
            // 1�� �������� �Ե��� ��
            dynamic_cast<CPlayer*>(Object)->Damaged(dfATTACK1_DAMAGE);

            PACKET_SC_DAMAGE packetDamage;
            mpDamage(&header, &packetDamage, pPlayer->m_pSession->uid, Object->m_pSession->uid, dynamic_cast<CPlayer*>(Object)->GetHp());

            CSessionManager::BroadcastPacket(nullptr, &header, &packetDamage);

            /*
            //=====================================================================================================================================
            // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� ��
            //=====================================================================================================================================
            if (packetDamage.damagedHP <= 0)
            {
                PACKET_SC_DELETE_CHARACTER packetDeleteCharacter;
                mpDeleteCharacter(&header, &packetDeleteCharacter, client->uid);

                BroadcastPacket(nullptr, &header, &packetDeleteCharacter);

                // �������� ������ �� �ֵ��� isAlive�� false�� ����
                client->isAlive = false;
            }
            */

            // ���⼭ break�� ���ָ� ���� ������
            break;
        }
    }

    return true;
}

bool netPacketProc_ATTACK2(CPlayer* pPlayer, void* pPacket)
{
    // Ŭ���̾�Ʈ�� ���� ���� �޽����� ����.
      // g_clientList�� ��ȸ�ϸ� ���� 2�� ������ �����ؼ� �������� �־���.
      // 1. dfPACKET_SC_ATTACK2 �� ��ε�ĳ����
      // 2. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 3, 4�� ���� ����
      // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
      // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� �� -> �� �κ��� �������� ó���ϵ��� �ٲ�.

      //=====================================================================================================================================
      // 1. dfPACKET_SC_ATTACK2 �� ��ε�ĳ����
      //=====================================================================================================================================
    PACKET_CS_ATTACK2* packetCSAttack2 = static_cast<PACKET_CS_ATTACK2*>(pPacket);
    pPlayer->SetPosition(packetCSAttack2->x, packetCSAttack2->y);

    PACKET_HEADER header;
    PACKET_SC_ATTACK2 packetSCAttack2;
    mpAttack2(&header, &packetSCAttack2, pPlayer->m_pSession->uid, pPlayer->GetDirection(), packetCSAttack2->x, packetCSAttack2->y);
    CSessionManager::BroadcastPacket(pPlayer->m_pSession, &header, &packetSCAttack2);

    //=====================================================================================================================================
    // 2. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 3, 4�� ���� ����
    //=====================================================================================================================================

    // ���� �ٶ󺸴� ���⿡ ���� ���� ������ �޶���.
    UINT16 left, right, top, bottom;
    UINT16 posX, posY;
    pPlayer->getPosition(posX, posY);

    // ������ �ٶ󺸰� �־��ٸ�
    if (pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
    {
        left = posX - dfATTACK2_RANGE_X;
        right = posX;
    }
    // �������� �ٶ󺸰� �־��ٸ�
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

        // �ٸ� �÷��̾��� ��ǥ�� ���� ������ ���� ���
        if (posX >= left && posX <= right &&
            posY >= top && posY <= bottom)
        {
            //=====================================================================================================================================
            // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
            //=====================================================================================================================================
            // 1�� �������� �Ե��� ��
            dynamic_cast<CPlayer*>(Object)->Damaged(dfATTACK2_DAMAGE);

            PACKET_SC_DAMAGE packetDamage;
            mpDamage(&header, &packetDamage, pPlayer->m_pSession->uid, Object->m_pSession->uid, dynamic_cast<CPlayer*>(Object)->GetHp());

            CSessionManager::BroadcastPacket(nullptr, &header, &packetDamage);

            /*
            //=====================================================================================================================================
            // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� ��
            //=====================================================================================================================================
            if (packetDamage.damagedHP <= 0)
            {
                PACKET_SC_DELETE_CHARACTER packetDeleteCharacter;
                mpDeleteCharacter(&header, &packetDeleteCharacter, client->uid);

                BroadcastPacket(nullptr, &header, &packetDeleteCharacter);

                // �������� ������ �� �ֵ��� isAlive�� false�� ����
                client->isAlive = false;
            }
            */

            // ���⼭ break�� ���ָ� ���� ������
            break;
        }
    }

    return true;
}

bool netPacketProc_ATTACK3(CPlayer* pPlayer, void* pPacket)
{
    // Ŭ���̾�Ʈ�� ���� ���� �޽����� ����.
     // g_clientList�� ��ȸ�ϸ� ���� 3�� ������ �����ؼ� �������� �־���.
     // 1. dfPACKET_SC_ATTACK3 �� ��ε�ĳ����
     // 2. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 3, 4�� ���� ����
     // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
     // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� �� -> �� �κ��� �������� ó���ϵ��� �ٲ�.

     //=====================================================================================================================================
     // 1. dfPACKET_SC_ATTACK3 �� ��ε�ĳ����
     //=====================================================================================================================================
    PACKET_CS_ATTACK3* packetCSAttack3 = static_cast<PACKET_CS_ATTACK3*>(pPacket);
    pPlayer->SetPosition(packetCSAttack3->x, packetCSAttack3->y);

    PACKET_HEADER header;
    PACKET_SC_ATTACK3 packetSCAttack3;
    mpAttack3(&header, &packetSCAttack3, pPlayer->m_pSession->uid, pPlayer->GetDirection(), packetCSAttack3->x, packetCSAttack3->y);
    CSessionManager::BroadcastPacket(pPlayer->m_pSession, &header, &packetSCAttack3);

    //=====================================================================================================================================
    // 2. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 3, 4�� ���� ����
    //=====================================================================================================================================

    // ���� �ٶ󺸴� ���⿡ ���� ���� ������ �޶���.
    UINT16 left, right, top, bottom;
    UINT16 posX, posY;
    pPlayer->getPosition(posX, posY);

    // ������ �ٶ󺸰� �־��ٸ�
    if (pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
    {
        left = posX - dfATTACK3_RANGE_X;
        right = posX;
    }
    // �������� �ٶ󺸰� �־��ٸ�
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

        // �ٸ� �÷��̾��� ��ǥ�� ���� ������ ���� ���
        if (posX >= left && posX <= right &&
            posY >= top && posY <= bottom)
        {
            //=====================================================================================================================================
            // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
            //=====================================================================================================================================
            // 1�� �������� �Ե��� ��
            dynamic_cast<CPlayer*>(Object)->Damaged(dfATTACK3_DAMAGE);

            PACKET_SC_DAMAGE packetDamage;
            mpDamage(&header, &packetDamage, pPlayer->m_pSession->uid, Object->m_pSession->uid, dynamic_cast<CPlayer*>(Object)->GetHp());

            CSessionManager::BroadcastPacket(nullptr, &header, &packetDamage);

            /*
            //=====================================================================================================================================
            // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� ��
            //=====================================================================================================================================
            if (packetDamage.damagedHP <= 0)
            {
                PACKET_SC_DELETE_CHARACTER packetDeleteCharacter;
                mpDeleteCharacter(&header, &packetDeleteCharacter, client->uid);

                BroadcastPacket(nullptr, &header, &packetDeleteCharacter);

                // �������� ������ �� �ֵ��� isAlive�� false�� ����
                client->isAlive = false;
            }
            */

            // ���⼭ break�� ���ָ� ���� ������
            break;
        }
    }

    return true;
}
