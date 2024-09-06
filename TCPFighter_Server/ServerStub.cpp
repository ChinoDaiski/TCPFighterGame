#include "pch.h"

#include "Proxy.h"
#include "ServerStub.h"

#include "Session.h"
#include "Player.h"
#include "Packet.h"

extern Proxy serverProxy;

bool ServerStub::CS_MoveStart(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y)
{
    // �޽��� ���� �α� Ȯ��
    // ==========================================================================================================
    // ������ ��ġ�� ���� ��Ŷ�� ��ġ���� �ʹ� ū ���̰� ���ٸ� �������                           .
    // �� ������ ��ǥ ����ȭ ������ �ܼ��� Ű���� ���� (Ŭ����Ʈ�� ��ó��, ������ �� �ݿ�) �������
    // Ŭ���̾�Ʈ�� ��ǥ�� �״�� �ϴ� ����� ���ϰ� ����.
    // ���� �¶��� �����̶�� Ŭ���̾�Ʈ���� �������� �����ϴ� ����� ���ؾ���.
    // ������ ������ ������ �������� �ϰ� �����Ƿ� �������� ������ Ŭ���̾�Ʈ ��ǥ�� �ϵ��� �Ѵ�.
    // ==========================================================================================================

    UINT16 posX, posY;
    pSession->pPlayer->getPosition(posX, posY);
    if (
        std::abs(posX - x) > dfERROR_RANGE ||
        std::abs(posY - y) > dfERROR_RANGE
        )
    {
        NotifyClientDisconnected(pSession);

        // �α� �����Ÿ� ���⼭ ���� ��
        int gapX = std::abs(posX - x);
        int gapY = std::abs(posY - y);
        DebugBreak();

        return false;
    }

    // ==========================================================================================================
    // ������ ����. ���� �������� ���۹�ȣ�� ���Ⱚ. ���ο��� �ٶ󺸴� ���⵵ ����
    // ==========================================================================================================
    pSession->pPlayer->SetDirection(direction);


    // ==========================================================================================================
    // ����ڸ� ������, ���� �������� ��� ����ڿ��� ��Ŷ�� �Ѹ�.
    // ==========================================================================================================
    serverProxy.SC_MoveStart_ForAll(pSession, pSession->uid, pSession->pPlayer->GetDirection(), x, y);


    //=====================================================================================================================================
    // �̵� ���� ������ �˸�
    //=====================================================================================================================================
    pSession->pPlayer->SetFlag(FLAG_MOVING, true);

    return true;
}

bool ServerStub::CS_MoveStop(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y)
{
    // ���� ���õ� Ŭ���̾�Ʈ�� �������� �������� ���� ���̶� ��û
    // 1. ���� ������ ó��
    // 2. PACKET_SC_MOVE_STOP �� ��ε�ĳ����
    // 3. ���� ������ �̵� ���� ������ �˸�

    //=====================================================================================================================================
    // 1. ���� ������ ó��
    //=====================================================================================================================================
    pSession->pPlayer->SetDirection(direction);
    pSession->pPlayer->SetPosition(x, y);

    //=====================================================================================================================================
    // 2. PACKET_SC_MOVE_STOP �� ��ε�ĳ����
    //=====================================================================================================================================
    serverProxy.SC_MoveStop_ForAll(pSession, pSession->uid, pSession->pPlayer->GetDirection(), x, y);

    //=====================================================================================================================================
    // 3. ���� ������ �̵� ���� ������ �˸�
    //=====================================================================================================================================
    pSession->pPlayer->SetFlag(FLAG_MOVING, false);

    return true;
}

bool ServerStub::CS_ATTACK1(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y)
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
    pSession->pPlayer->SetPosition(x, y);

    serverProxy.SC_Attack1_ForAll(pSession, pSession->uid, pSession->pPlayer->GetDirection(), x, y);

    //=====================================================================================================================================
    // 2. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 3, 4�� ���� ����
    //=====================================================================================================================================

    // ���� �ٶ󺸴� ���⿡ ���� ���� ������ �޶���.
    UINT16 left, right, top, bottom;
    UINT16 posX, posY;
    pSession->pPlayer->getPosition(posX, posY);

    // ������ �ٶ󺸰� �־��ٸ�
    if (pSession->pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
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

    for (auto& client : g_clientList)
    {
        if (client->uid == pSession->uid)
            continue;

        client->pPlayer->getPosition(posX, posY);

        // �ٸ� �÷��̾��� ��ǥ�� ���� ������ ���� ���
        if (posX >= left && posX <= right &&
            posY >= top && posY <= bottom)
        {
            //=====================================================================================================================================
            // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
            //=====================================================================================================================================
            // 1�� �������� �Ե��� ��
            client->pPlayer->Damaged(dfATTACK1_DAMAGE);

            serverProxy.SC_Damage_ForAll(pSession, pSession->uid, client->uid, client->pPlayer->GetHp());

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

bool ServerStub::CS_ATTACK2(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y)
{
    // Ŭ���̾�Ʈ�� ���� ���� �޽����� ����.
    // g_clientList�� ��ȸ�ϸ� ���� 2�� ������ �����ؼ� �������� �־���.
    // 1. dfPACKET_SC_ATTACK2 �� ��ε�ĳ����
    // 2. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 3, 4�� ���� ����
    // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
    // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� ��

    //=====================================================================================================================================
    // 1. dfPACKET_SC_ATTACK2 �� ��ε�ĳ����
    //=====================================================================================================================================
    pSession->pPlayer->SetPosition(x, y);

    serverProxy.SC_Attack2_ForAll(pSession, pSession->uid, pSession->pPlayer->GetDirection(), x, y);

    //=====================================================================================================================================
    // 2. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 3, 4�� ���� ����
    //=====================================================================================================================================

    // ���� �ٶ󺸴� ���⿡ ���� ���� ������ �޶���.
    UINT16 left, right, top, bottom;
    UINT16 posX, posY;
    pSession->pPlayer->getPosition(posX, posY);

    // ������ �ٶ󺸰� �־��ٸ�
    if (pSession->pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
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

    for (auto& client : g_clientList)
    {
        if (client->uid == pSession->uid)
            continue;

        client->pPlayer->getPosition(posX, posY);

        // �ٸ� �÷��̾��� ��ǥ�� ���� ������ ���� ���
        if (posX >= left && posX <= right &&
            posY >= top && posY <= bottom)
        {
            //=====================================================================================================================================
            // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
            //=====================================================================================================================================
            // 1�� �������� �Ե��� ��
            client->pPlayer->Damaged(dfATTACK2_DAMAGE);

            serverProxy.SC_Damage_ForAll(pSession, pSession->uid, client->uid, client->pPlayer->GetHp());

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

bool ServerStub::CS_ATTACK3(SESSION* pSession, UINT8 direction, UINT16 x, UINT16 y)
{
    // Ŭ���̾�Ʈ�� ���� ���� �޽����� ����.
    // g_clientList�� ��ȸ�ϸ� ���� 3�� ������ �����ؼ� �������� �־���.
    // 1. dfPACKET_SC_ATTACK3 �� ��ε�ĳ����
    // 2. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 3, 4�� ���� ����
    // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
    // 4. ���� ü���� 0 ���Ϸ� �������ٸ� dfPACKET_SC_DELETE_CHARACTER �� ��ε�ĳ�����ϰ�, �������� ������ �� �ֵ��� ��

    //=====================================================================================================================================
    // 1. dfPACKET_SC_ATTACK3 �� ��ε�ĳ����
    //=====================================================================================================================================
    pSession->pPlayer->SetPosition(x, y);

    serverProxy.SC_Attack3_ForAll(pSession, pSession->uid, pSession->pPlayer->GetDirection(), x, y);

    //=====================================================================================================================================
    // 2. ���ݹ��� ĳ���͸� �˻�. �˻��� �����ϸ� 3, 4�� ���� ����
    //=====================================================================================================================================

    // ���� �ٶ󺸴� ���⿡ ���� ���� ������ �޶���.
    UINT16 left, right, top, bottom;
    UINT16 posX, posY;
    pSession->pPlayer->getPosition(posX, posY);

    // ������ �ٶ󺸰� �־��ٸ�
    if (pSession->pPlayer->GetFacingDirection() == dfPACKET_MOVE_DIR_LL)
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

    for (auto& client : g_clientList)
    {
        if (client->uid == pSession->uid)
            continue;

        client->pPlayer->getPosition(posX, posY);

        // �ٸ� �÷��̾��� ��ǥ�� ���� ������ ���� ���
        if (posX >= left && posX <= right &&
            posY >= top && posY <= bottom)
        {
            //=====================================================================================================================================
            // 3. dfPACKET_SC_DAMAGE �� ��ε�ĳ����
            //=====================================================================================================================================
            // 1�� �������� �Ե��� ��
            client->pPlayer->Damaged(dfATTACK3_DAMAGE);

            serverProxy.SC_Damage_ForAll(pSession, pSession->uid, client->uid, client->pPlayer->GetHp());

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
