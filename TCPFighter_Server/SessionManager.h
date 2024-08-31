#pragma once

#include "Singleton.h"
#include "Session.h"

class CSessionManager : public SingletonBase<CSessionManager> {
private:
    friend class SingletonBase<CSessionManager>;

public:
    explicit CSessionManager() noexcept;
    ~CSessionManager() noexcept;

    // 복사 생성자와 대입 연산자를 삭제하여 복사 방지
    CSessionManager(const CSessionManager&) = delete;
    CSessionManager& operator=(const CSessionManager&) = delete;

public:
    //==========================================================================================================================================
    // Broadcast
    //==========================================================================================================================================
    static void BroadcastData(SESSION* excludeSession, void* pMsg, UINT8 msgSize);
    static void BroadcastPacket(SESSION* excludeSession, PACKET_HEADER* pHeader, void* pPacket);

    //==========================================================================================================================================
    // Unicast
    //==========================================================================================================================================
    static void UnicastData(SESSION* includeSession, void* pMsg, UINT8 msgSize);
    static void UnicastPacket(SESSION* includeSession, PACKET_HEADER* pHeader, void* pPacket);

public:
    // 생성 / 소멸
    static SESSION* CreateSession(SOCKET ClientSocket, SOCKADDR_IN ClientAddr);
    static void NotifyClientDisconnected(SESSION* disconnectedSession);

public:
    static std::map<UINT16, SESSION*>& GetUserSessionMap(void) { return m_UserSessionMap; }

private:
    static void DeleteSession(UINT16 uid);

private:
    static UINT16 m_gID;
    static std::map<UINT16, SESSION*> m_UserSessionMap;
};

