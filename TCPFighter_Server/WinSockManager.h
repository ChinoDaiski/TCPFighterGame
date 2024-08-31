#pragma once

#include "Singleton.h"

#define OPTION_NONBLOCKING 0x01

enum class PROTOCOL_TYPE
{
    TCP_IP,
    UDP,
    END
};

class CWinSockManager : public SingletonBase<CWinSockManager> {
private:
    friend class SingletonBase<CWinSockManager>;

public:
    explicit CWinSockManager() noexcept;
    ~CWinSockManager() noexcept;
    
    // 복사 생성자와 대입 연산자를 삭제하여 복사 방지
    CWinSockManager(const CWinSockManager&) = delete;
    CWinSockManager& operator=(const CWinSockManager&) = delete;

private:
    void InitWinSock() noexcept;
    void CreateListenSocket(PROTOCOL_TYPE type);
    void Bind(UINT16 serverPort, UINT32 serverIP);
    void Listen(UINT32 somaxconn);
    void SetOptions(UINT8 options);
    void Cleanup() noexcept;

public:
    void StartServer(PROTOCOL_TYPE type, UINT16 serverPort, UINT8 options = 0, UINT32 serverIP = INADDR_ANY, UINT32 somaxconn = SOMAXCONN_HINT(65535)) noexcept;
    SOCKET Accept(SOCKADDR_IN& ClientAddr) noexcept;

public:
    static std::string GetIP(const SOCKADDR_IN& ClientAddr) noexcept;
    static std::string GetIP(const IN_ADDR& ClientInAddr) noexcept;
    static UINT16 GetPort(const SOCKADDR_IN& ClientAddr) noexcept;
    static UINT16 GetPort(const UINT16& ClientInAddr) noexcept;

public:
    constexpr SOCKET GetListenSocket(void) { return m_listenSocket; }

private:
    SOCKET m_listenSocket;                // listen 소켓
};