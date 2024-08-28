#pragma once

#include "pch.h"
#include "Singleton.h"


#define OPTION_NONBLOCKING 0x01

enum class PROTOCOL_TYPE
{
    TCP_IP,
    UDP,
    END
};

// ������ ���� ���ø� Ŭ������ �����մϴ�.
template <typename SessionType>
class CWinSockManager : public SingletonBase<CWinSockManager<SessionType>> {
private:
    friend class SingletonBase<CWinSockManager<SessionType>>;

public:
    explicit CWinSockManager() noexcept;
    ~CWinSockManager() noexcept;
    
    // ���� �����ڿ� ���� �����ڸ� �����Ͽ� ���� ����
    CWinSockManager(const CWinSockManager&) = delete;
    CWinSockManager& operator=(const CWinSockManager&) = delete;

private:
    void InitWinSock() noexcept;
    constexpr void CreateListenSocket(PROTOCOL_TYPE type);
    constexpr void Bind(UINT16 serverPort, UINT32 serverIP);
    constexpr void Listen(UINT32 somaxconn);
    constexpr void SetOptions(UINT8 options);
    void Cleanup() noexcept;

public:
    constexpr void StartServer(PROTOCOL_TYPE type, UINT16 serverPort, UINT8 options = 0, UINT32 serverIP = INADDR_ANY, UINT32 somaxconn = SOMAXCONN_HINT(65535)) noexcept;
    constexpr SOCKET Accept(SOCKADDR_IN& ClientAddr) noexcept;

public:
    std::string GetIP(const SOCKADDR_IN& ClientAddr) noexcept;
    std::string GetIP(const IN_ADDR& ClientInAddr) noexcept;
    UINT16 GetPort(const SOCKADDR_IN& ClientAddr) noexcept;
    UINT16 GetPort(const UINT16& ClientInAddr) noexcept;
    constexpr SOCKET GetListenSocket(void) { return m_listenSocket; }

private:
    SOCKET m_listenSocket;                // listen ����
};

template<typename SessionType>
CWinSockManager<SessionType>::CWinSockManager() noexcept
{
    m_listenSocket = INVALID_SOCKET;
}

template<typename SessionType>
CWinSockManager<SessionType>::~CWinSockManager() noexcept
{
    Cleanup();
}

template<typename SessionType>
void CWinSockManager<SessionType>::InitWinSock() noexcept
{
    // ������ ���� �ʱ�ȭ
    WSADATA wsaData;

    int retVal = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (retVal != 0) {
        std::cout << "Error : WSAStartup failed " << retVal << ", " << WSAGetLastError() << "\n";
        DebugBreak();
    }
}

template<typename SessionType>
constexpr void CWinSockManager<SessionType>::CreateListenSocket(PROTOCOL_TYPE type)
{
    switch (type)
    {
    case PROTOCOL_TYPE::TCP_IP:
        // �̹� 2���ڿ��� TCP�� UDP�� ������ ���� ������ 3���ڿ� ���� �ʾƵ� �����ϴ�.
        m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_listenSocket == INVALID_SOCKET)
        {
            std::cout << "Error : TCP CreateListenSocket()" << WSAGetLastError() << "\n";
            DebugBreak();
        }
        break;
    case PROTOCOL_TYPE::UDP:
        m_listenSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_listenSocket == INVALID_SOCKET)
        {
            std::cout << "Error : UDP CreateListenSocket()" << WSAGetLastError() << "\n";
            DebugBreak();
        }
        break;
    default:
        std::cout << "Error : default socket()" << WSAGetLastError() << "\n";
        DebugBreak();
        break;
    }
}

template<typename SessionType>
constexpr void CWinSockManager<SessionType>::Bind(UINT16 serverPort, UINT32 serverIP)
{
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(serverIP);
    serveraddr.sin_port = htons(serverPort);
    int retVal = bind(m_listenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr));

    if (retVal == SOCKET_ERROR)
    {
        std::cout << "Error : Bind(), " << WSAGetLastError() << "\n";
        DebugBreak();
    }
}

template<typename SessionType>
constexpr void CWinSockManager<SessionType>::Listen(UINT32 somaxconn)
{
    int retVal = listen(m_listenSocket, somaxconn);
    if (retVal == SOCKET_ERROR)
    {
        std::cout << "Error : Listen(), " << WSAGetLastError() << "\n";
        DebugBreak();
    }
}

template<typename SessionType>
inline constexpr void CWinSockManager<SessionType>::SetOptions(UINT8 options)
{
    if (options == 0)
        return;

    if (options | OPTION_NONBLOCKING)
    {
        ULONG on = 1;   // 0�� �ƴ� ��� non-blocking �𵨷� ��ȯ
        int NonBlockingRetVal = ioctlsocket(m_listenSocket, FIONBIO, &on);
        if (NonBlockingRetVal == SOCKET_ERROR)
        {
            std::cout << "Error : SetOptions(), OPTION_NONBLOCKING. �� ���ŷ ��ȯ ����. " << WSAGetLastError() << "\n";
        }
    }
}

template<typename SessionType>
constexpr void CWinSockManager<SessionType>::StartServer(PROTOCOL_TYPE type, UINT16 serverPort, UINT8 options, UINT32 serverIP, UINT32 somaxconn) noexcept {

    // ������ �ʱ�ȭ
    InitWinSock();

    // listen ���� ����
    CreateListenSocket(type);

    // bind
    Bind(serverPort, serverIP);

    // option ����
    SetOptions(options);

    // listen
    Listen(somaxconn);
}

template<typename SessionType>
constexpr SOCKET CWinSockManager<SessionType>::Accept(SOCKADDR_IN& ClientAddr) noexcept
{
    // accept ����
    int addrlen = sizeof(ClientAddr);
    SOCKET ClientSocket = accept(m_listenSocket, (SOCKADDR*)&ClientAddr, &addrlen);

    if (ClientSocket == INVALID_SOCKET)
    {
        std::cout << "Error : Accept(), " << WSAGetLastError() << "\n";
    }

    // ������ Ŭ���̾�Ʈ ������ ���� �ʹٸ�
    // std::cout<<"[TCP ����] Ŭ���̾�Ʈ ���� : IP �ּ� = % s, ��Ʈ ��ȣ = % d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

    return ClientSocket;
}

template<typename SessionType>
std::string CWinSockManager<SessionType>::GetIP(const SOCKADDR_IN& ClientAddr) noexcept
{
    return GetIP(ClientAddr.sin_addr);
}

template<typename SessionType>
std::string CWinSockManager<SessionType>::GetIP(const IN_ADDR& ClientInAddr) noexcept
{
    char pAddrBuf[INET_ADDRSTRLEN];

    // ���̳ʸ� �ּҸ� ����� ���� �� �ִ� ���·� ��ȯ
    if (inet_ntop(AF_INET, &ClientInAddr, pAddrBuf, sizeof(pAddrBuf)) == NULL) {
        std::cout << "Error : inet_ntop()\n";
        DebugBreak();
    }

    char strIP[16];
    memcpy(strIP, pAddrBuf, sizeof(char) * 16);

    return std::string{ strIP };
}

template<typename SessionType>
UINT16 CWinSockManager<SessionType>::GetPort(const SOCKADDR_IN& ClientAddr) noexcept
{
    return GetPort(ClientAddr.sin_port);
}

template<typename SessionType>
UINT16 CWinSockManager<SessionType>::GetPort(const UINT16& port) noexcept
{
    return ntohs(port);
}

template<typename SessionType>
void CWinSockManager<SessionType>::Cleanup() noexcept
{
    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }

    WSACleanup();
}
