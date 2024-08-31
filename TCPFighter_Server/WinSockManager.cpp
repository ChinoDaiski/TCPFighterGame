
#include "pch.h"
#include "WinSockManager.h"

CWinSockManager::CWinSockManager() noexcept
{
    m_listenSocket = INVALID_SOCKET;
}

CWinSockManager::~CWinSockManager() noexcept
{
    Cleanup();
}

void CWinSockManager::InitWinSock() noexcept
{
    // 윈도우 소켓 초기화
    WSADATA wsaData;

    int retVal = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (retVal != 0) {
        std::cout << "Error : WSAStartup failed " << retVal << ", " << WSAGetLastError() << "\n";
        DebugBreak();
    }
}

void CWinSockManager::CreateListenSocket(PROTOCOL_TYPE type)
{
    switch (type)
    {
    case PROTOCOL_TYPE::TCP_IP:
        // 이미 2인자에서 TCP냐 UDP냐 결정이 났기 때문에 3인자에 넣지 않아도 무방하다.
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

void CWinSockManager::Bind(UINT16 serverPort, UINT32 serverIP)
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

void CWinSockManager::Listen(UINT32 somaxconn)
{
    int retVal = listen(m_listenSocket, somaxconn);
    if (retVal == SOCKET_ERROR)
    {
        std::cout << "Error : Listen(), " << WSAGetLastError() << "\n";
        DebugBreak();
    }
}

inline void CWinSockManager::SetOptions(UINT8 options)
{
    if (options == 0)
        return;

    if (options | OPTION_NONBLOCKING)
    {
        ULONG on = 1;   // 0이 아닐 경우 non-blocking 모델로 전환
        int NonBlockingRetVal = ioctlsocket(m_listenSocket, FIONBIO, &on);
        if (NonBlockingRetVal == SOCKET_ERROR)
        {
            std::cout << "Error : SetOptions(), OPTION_NONBLOCKING. 논 블로킹 전환 실패. " << WSAGetLastError() << "\n";
        }
    }
}

void CWinSockManager::StartServer(PROTOCOL_TYPE type, UINT16 serverPort, UINT8 options, UINT32 serverIP, UINT32 somaxconn) noexcept {

    // 윈도우 초기화
    InitWinSock();

    // listen 소켓 생성
    CreateListenSocket(type);

    // bind
    Bind(serverPort, serverIP);

    // option 설정
    SetOptions(options);

    // listen
    Listen(somaxconn);
}

SOCKET CWinSockManager::Accept(SOCKADDR_IN& ClientAddr) noexcept
{
    // accept 실행
    int addrlen = sizeof(ClientAddr);
    SOCKET ClientSocket = accept(m_listenSocket, (SOCKADDR*)&ClientAddr, &addrlen);

    if (ClientSocket == INVALID_SOCKET)
    {
        std::cout << "Error : Accept(), " << WSAGetLastError() << "\n";
    }
    // accept를 여러번 할꺼면 WOULDBLOCK이 나올 때 까지 반복해서 accept를 시도해야한다. 하지만 여기선 1회만 할 것이므로 INVALID 처리한 하고 넘어감.

    // 접속한 클라이언트 정보를 보고 싶다면
    // std::cout<<"[TCP 서버] 클라이언트 접속 : IP 주소 = % s, 포트 번호 = % d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

    return ClientSocket;
}

std::string CWinSockManager::GetIP(const SOCKADDR_IN& ClientAddr) noexcept
{
    return GetIP(ClientAddr.sin_addr);
}

std::string CWinSockManager::GetIP(const IN_ADDR& ClientInAddr) noexcept
{
    char pAddrBuf[INET_ADDRSTRLEN];

    // 바이너리 주소를 사람이 읽을 수 있는 형태로 변환
    if (inet_ntop(AF_INET, &ClientInAddr, pAddrBuf, sizeof(pAddrBuf)) == NULL) {
        std::cout << "Error : inet_ntop()\n";
        DebugBreak();
    }

    char strIP[16];
    memcpy(strIP, pAddrBuf, sizeof(char) * 16);

    return std::string{ strIP };
}

UINT16 CWinSockManager::GetPort(const SOCKADDR_IN& ClientAddr) noexcept
{
    return GetPort(ClientAddr.sin_port);
}

UINT16 CWinSockManager::GetPort(const UINT16& port) noexcept
{
    return ntohs(port);
}

void CWinSockManager::Cleanup() noexcept
{
    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }

    WSACleanup();
}
