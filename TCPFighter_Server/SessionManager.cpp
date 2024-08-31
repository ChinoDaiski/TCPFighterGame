#include "pch.h"
#include "SessionManager.h"
#include "WinSockManager.h"

std::map<UINT16, SESSION*> CSessionManager::m_UserSessionMap;
UINT16 CSessionManager::m_gID = 0;  // ���� ���� id �ʱ� �� ����

CSessionManager::CSessionManager() noexcept
{
}

CSessionManager::~CSessionManager() noexcept
{
}

void CSessionManager::BroadcastData(SESSION* excludeSession, void* pData, UINT8 dataSize)
{
    for (auto& Session : m_UserSessionMap)
    {
        SESSION* pSession = Session.second;

        // �ش� ������ alive�� �ƴϰų� ������ �����̶�� �Ѿ��
        if (!pSession->isAlive || pSession == excludeSession)
            continue;

        // �޽��� ����, ������ sendQ�� �����͸� ����
        int retVal = pSession->sendQ.Enqueue((const char*)pData, dataSize);

        if (retVal != dataSize)
        {
            NotifyClientDisconnected(pSession);

            // �̷� ���� �־ �ȵ����� Ȥ�� �𸣴� �˻�, enqueue���� ������ �� ���� �������� ũ�Ⱑ ����á�ٴ� �ǹ��̹Ƿ�, resize���� ������ ���� ����� ���� �׽�Ʈ�ϸ鼭 ����
            int error = WSAGetLastError();
            std::cout << "Error : BroadcastData(), It might be full of sendQ" << error << "\n";
            DebugBreak();
        }
    }
}

void CSessionManager::BroadcastPacket(SESSION* excludeSession, PACKET_HEADER* pHeader, void* pPacket)
{
    BroadcastData(excludeSession, pHeader, sizeof(PACKET_HEADER));
    BroadcastData(excludeSession, pPacket, pHeader->bySize);
}

void CSessionManager::UnicastData(SESSION* includeSession, void* pData, UINT8 dataSize)
{
    if (!includeSession->isAlive)
        return;

    // ������ sendQ�� �����͸� ����
    int retVal = includeSession->sendQ.Enqueue((const char*)pData, dataSize);

    if (retVal != dataSize)
    {
        NotifyClientDisconnected(includeSession);

        // �̷� ���� �־ �ȵ����� Ȥ�� �𸣴� �˻�, enqueue���� ������ �� ���� �������� ũ�Ⱑ ����á�ٴ� �ǹ��̹Ƿ�, resize���� ������ ���� ����� ���� �׽�Ʈ�ϸ鼭 ����
        int error = WSAGetLastError();
        std::cout << "Error : UnicastData(), It might be full of sendQ" << error << "\n";
        DebugBreak();
    }
}

void CSessionManager::UnicastPacket(SESSION* includeSession, PACKET_HEADER* pHeader, void* pPacket)
{
    UnicastData(includeSession, pHeader, sizeof(PACKET_HEADER));
    UnicastData(includeSession, pPacket, pHeader->bySize);
}

void CSessionManager::NotifyClientDisconnected(SESSION* disconnectedSession)
{
    // ���� �̹� �׾��ٸ� NotifyClientDisconnected�� ȣ��Ǿ��� �����̹Ƿ� �ߺ��� ����. üũ�� ��.
    if (disconnectedSession->isAlive == false)
    {
        DebugBreak();
    }

    disconnectedSession->isAlive = false;

    // PACKET_SC_DELETE_CHARACTER ��Ŷ ����
    PACKET_HEADER header;
    PACKET_SC_DELETE_CHARACTER packetDelete;
    mpDeleteCharacter(&header, &packetDelete, disconnectedSession->uid);

    // ������ ��Ŷ ������ ���� ������ �����ϰ� ��ε�ĳ��Ʈ
    BroadcastPacket(disconnectedSession, &header, &packetDelete);

    CSessionManager::DeleteSession(disconnectedSession->uid);
}

void CSessionManager::DeleteSession(UINT16 uid)
{
    closesocket(m_UserSessionMap[uid]->sock);
    m_UserSessionMap.erase(uid);
}

SESSION* CSessionManager::CreateSession(SOCKET ClientSocket, SOCKADDR_IN ClientAddr)
{
    SESSION* Session = new SESSION;
    Session->isAlive = true;

    // ���� ���� �߰�
    Session->sock = ClientSocket;

    // IP / PORT ���� �߰�
    memcpy(Session->IP, CWinSockManager::GetIP(ClientAddr).c_str(), sizeof(Session->IP));
    Session->port = CWinSockManager::GetPort(ClientAddr);

    // UID �ο�
    Session->uid = m_gID;
    m_gID++;

    m_UserSessionMap[Session->uid] = Session;

    return Session;
}
