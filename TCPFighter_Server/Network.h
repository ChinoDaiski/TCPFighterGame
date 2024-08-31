#pragma once

#include "Singleton.h"
#include "Session.h"

typedef bool(*PacketCallback)(SESSION* pSession, PACKET_TYPE packetType, void* pPacket);

class CNetIOManager : public SingletonBase<CNetIOManager> {
private:
    friend class SingletonBase<CNetIOManager>;

public:
    explicit CNetIOManager() noexcept;
    ~CNetIOManager() noexcept;

    // 복사 생성자와 대입 연산자를 삭제하여 복사 방지
    CNetIOManager(const CNetIOManager&) = delete;
    CNetIOManager& operator=(const CNetIOManager&) = delete;

public:
    void netIOProcess(void);

    void netProc_Accept(void);
    void netProc_Send(SESSION* pSession);
    void netProc_Recv(SESSION* pSession);

public:
    std::queue<SESSION*>& GetAcceptSessionQueue(void) { return m_AcceptSessionQueue; }

public:
    void RegisterCallbackFunction(PacketCallback callback) { m_callback = callback; }

private:
    std::queue<SESSION*> m_AcceptSessionQueue;
    PacketCallback m_callback;
};
