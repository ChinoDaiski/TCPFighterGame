
#include "pch.h"
#include "Session.h"
#include "Player.h"
#include "Packet.h"
#include "WinSockManager.h"

void BroadcastData(SESSION* excludeSession, PACKET_HEADER* pPacket, UINT8 dataSize)
{
    for (auto& client : g_clientList)
    {
        // �ش� ������ alive�� �ƴϰų� ������ �����̶�� �Ѿ��
        if (!client->isAlive || excludeSession == client)
            continue;

        // �޽��� ����, ������ sendQ�� �����͸� ����
        int retVal = client->sendQ.Enqueue((const char*)pPacket, dataSize);

        if (retVal != dataSize)
        {
            NotifyClientDisconnected(client);

            // �̷� ���� �־ �ȵ����� Ȥ�� �𸣴� �˻�, enqueue���� ������ �� ���� �������� ũ�Ⱑ ����á�ٴ� �ǹ��̹Ƿ�, resize���� ������ ���� ����� ���� �׽�Ʈ�ϸ鼭 ����
            int error = WSAGetLastError();
            std::cout << "Error : BroadcastData(), It might be full of sendQ" << error << "\n";
            DebugBreak();
        }
    }
}

// Ŭ���̾�Ʈ���� �����͸� ��ε�ĳ��Ʈ�ϴ� �Լ�
void BroadcastData(SESSION* excludeSession, CPacket* pPacket, UINT8 dataSize)
{
    for (auto& client : g_clientList)
    {
        // �ش� ������ alive�� �ƴϰų� ������ �����̶�� �Ѿ��
        if (!client->isAlive || excludeSession == client)
            continue;

        // �޽��� ����, ������ sendQ�� �����͸� ����
        int retVal = client->sendQ.Enqueue((const char*)pPacket->GetBufferPtr(), dataSize);

        if (retVal != dataSize)
        {
            NotifyClientDisconnected(client);

            // �̷� ���� �־ �ȵ����� Ȥ�� �𸣴� �˻�, enqueue���� ������ �� ���� �������� ũ�Ⱑ ����á�ٴ� �ǹ��̹Ƿ�, resize���� ������ ���� ����� ���� �׽�Ʈ�ϸ鼭 ����
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

// Ŭ���̾�Ʈ ������ ������ ��쿡 ȣ��Ǵ� �Լ�
void NotifyClientDisconnected(SESSION* disconnectedSession)
{
    // ���� �̹� �׾��ٸ� NotifyClientDisconnected�� ȣ��Ǿ��� �����̹Ƿ� �ߺ��� ����. üũ�� ��.
    if (disconnectedSession->isAlive == false)
    {
        DebugBreak();
    }

    disconnectedSession->isAlive = false;
}

void UnicastData(SESSION* includeSession, PACKET_HEADER* pPacket, UINT8 dataSize)
{
    if (!includeSession->isAlive)
        return;

    // ������ sendQ�� �����͸� ����
    int retVal = includeSession->sendQ.Enqueue((const char*)pPacket, dataSize);

    if (retVal != dataSize)
    {
        NotifyClientDisconnected(includeSession);

        // �̷� ���� �־ �ȵ����� Ȥ�� �𸣴� �˻�, enqueue���� ������ �� ���� �������� ũ�Ⱑ ����á�ٴ� �ǹ��̹Ƿ�, resize���� ������ ���� ����� ���� �׽�Ʈ�ϸ鼭 ����
        int error = WSAGetLastError();
        std::cout << "Error : UnicastData(), It might be full of sendQ" << error << "\n";
        DebugBreak();
    }
}

// ���ڷ� ���� ���ǵ鿡�� ������ ������ �õ��ϴ� �Լ�
void UnicastData(SESSION* includeSession, CPacket* pPacket, UINT8 dataSize)
{
    if (!includeSession->isAlive)
        return;

    // ������ sendQ�� �����͸� ����
    int retVal = includeSession->sendQ.Enqueue((const char*)pPacket->GetBufferPtr(), dataSize);

    if (retVal != dataSize)
    {
        NotifyClientDisconnected(includeSession);

        // �̷� ���� �־ �ȵ����� Ȥ�� �𸣴� �˻�, enqueue���� ������ �� ���� �������� ũ�Ⱑ ����á�ٴ� �ǹ��̹Ƿ�, resize���� ������ ���� ����� ���� �׽�Ʈ�ϸ鼭 ����
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

    // accept�� �Ϸ�Ǿ��ٸ� ���ǿ� ��� ��, �ش� ���ǿ� ��Ŷ ����
    SESSION* Session = new SESSION;
    Session->isAlive = true;

    // ���� ���� �߰�
    Session->sock = ClientSocket;

    // IP / PORT ���� �߰�
    memcpy(Session->IP, winSockManager.GetIP(ClientAddr).c_str(), sizeof(Session->IP));
    Session->port = winSockManager.GetPort(ClientAddr);

    // UID �ο�
    Session->uid = id;

    Session->flagField = 0;

    CPlayer* pPlayer = new CPlayer(posX, posY, direction, hp);
    pPlayer->SetFlagField(&Session->flagField);
    pPlayer->SetSpeed(MOVE_X_PER_FRAME, MOVE_Y_PER_FRAME);

    Session->pPlayer = pPlayer;

    return Session;
}