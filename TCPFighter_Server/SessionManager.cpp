#include "pch.h"
#include "SessionManager.h"
#include "WinSockManager.h"

std::map<UINT16, SESSION*> CSessionManager::m_UserSessionMap;
UINT16 CSessionManager::m_gID = 0;  // 전역 유저 id 초기 값 설정

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

        // 해당 세션이 alive가 아니거나 제외할 세션이라면 넘어가기
        if (!pSession->isAlive || pSession == excludeSession)
            continue;

        // 메시지 전파, 세션의 sendQ에 데이터를 삽입
        int retVal = pSession->sendQ.Enqueue((const char*)pData, dataSize);

        if (retVal != dataSize)
        {
            NotifyClientDisconnected(pSession);

            // 이런 일은 있어선 안되지만 혹시 모르니 검사, enqueue에서 문제가 난 것은 링버퍼의 크기가 가득찼다는 의미이므로, resize할지 말지는 오류 생길시 가서 테스트하면서 진행
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

    // 세션의 sendQ에 데이터를 삽입
    int retVal = includeSession->sendQ.Enqueue((const char*)pData, dataSize);

    if (retVal != dataSize)
    {
        NotifyClientDisconnected(includeSession);

        // 이런 일은 있어선 안되지만 혹시 모르니 검사, enqueue에서 문제가 난 것은 링버퍼의 크기가 가득찼다는 의미이므로, resize할지 말지는 오류 생길시 가서 테스트하면서 진행
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
    // 만약 이미 죽었다면 NotifyClientDisconnected가 호출되었던 상태이므로 중복인 상태. 체크할 것.
    if (disconnectedSession->isAlive == false)
    {
        DebugBreak();
    }

    disconnectedSession->isAlive = false;

    // PACKET_SC_DELETE_CHARACTER 패킷 생성
    PACKET_HEADER header;
    PACKET_SC_DELETE_CHARACTER packetDelete;
    mpDeleteCharacter(&header, &packetDelete, disconnectedSession->uid);

    // 생성된 패킷 연결이 끊긴 세션을 제외하고 브로드캐스트
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

    // 소켓 정보 추가
    Session->sock = ClientSocket;

    // IP / PORT 정보 추가
    memcpy(Session->IP, CWinSockManager::GetIP(ClientAddr).c_str(), sizeof(Session->IP));
    Session->port = CWinSockManager::GetPort(ClientAddr);

    // UID 부여
    Session->uid = m_gID;
    m_gID++;

    m_UserSessionMap[Session->uid] = Session;

    return Session;
}
