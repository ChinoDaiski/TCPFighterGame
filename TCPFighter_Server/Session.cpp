
#include "pch.h"
#include "Session.h"
#include "Player.h"
#include "Packet.h"
#include "WinSockManager.h"

void BroadcastData(SESSION* excludeSession, PACKET_HEADER* pPacket, UINT8 dataSize)
{
    for (auto& client : g_clientList)
    {
        // 해당 세션이 alive가 아니거나 제외할 세션이라면 넘어가기
        if (!client->isAlive || excludeSession == client)
            continue;

        // 메시지 전파, 세션의 sendQ에 데이터를 삽입
        int retVal = client->sendQ.Enqueue((const char*)pPacket, dataSize);

        if (retVal != dataSize)
        {
            NotifyClientDisconnected(client);

            // 이런 일은 있어선 안되지만 혹시 모르니 검사, enqueue에서 문제가 난 것은 링버퍼의 크기가 가득찼다는 의미이므로, resize할지 말지는 오류 생길시 가서 테스트하면서 진행
            int error = WSAGetLastError();
            std::cout << "Error : BroadcastData(), It might be full of sendQ" << error << "\n";
            DebugBreak();
        }
    }
}

// 클라이언트에게 데이터를 브로드캐스트하는 함수
void BroadcastData(SESSION* excludeSession, CPacket* pPacket, UINT8 dataSize)
{
    for (auto& client : g_clientList)
    {
        // 해당 세션이 alive가 아니거나 제외할 세션이라면 넘어가기
        if (!client->isAlive || excludeSession == client)
            continue;

        // 메시지 전파, 세션의 sendQ에 데이터를 삽입
        int retVal = client->sendQ.Enqueue((const char*)pPacket->GetBufferPtr(), dataSize);

        if (retVal != dataSize)
        {
            NotifyClientDisconnected(client);

            // 이런 일은 있어선 안되지만 혹시 모르니 검사, enqueue에서 문제가 난 것은 링버퍼의 크기가 가득찼다는 의미이므로, resize할지 말지는 오류 생길시 가서 테스트하면서 진행
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

// 클라이언트 연결이 끊어진 경우에 호출되는 함수
void NotifyClientDisconnected(SESSION* disconnectedSession)
{
    // 만약 이미 죽었다면 NotifyClientDisconnected가 호출되었던 상태이므로 중복인 상태. 체크할 것.
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

    // 세션의 sendQ에 데이터를 삽입
    int retVal = includeSession->sendQ.Enqueue((const char*)pPacket, dataSize);

    if (retVal != dataSize)
    {
        NotifyClientDisconnected(includeSession);

        // 이런 일은 있어선 안되지만 혹시 모르니 검사, enqueue에서 문제가 난 것은 링버퍼의 크기가 가득찼다는 의미이므로, resize할지 말지는 오류 생길시 가서 테스트하면서 진행
        int error = WSAGetLastError();
        std::cout << "Error : UnicastData(), It might be full of sendQ" << error << "\n";
        DebugBreak();
    }
}

// 인자로 받은 세션들에게 데이터 전송을 시도하는 함수
void UnicastData(SESSION* includeSession, CPacket* pPacket, UINT8 dataSize)
{
    if (!includeSession->isAlive)
        return;

    // 세션의 sendQ에 데이터를 삽입
    int retVal = includeSession->sendQ.Enqueue((const char*)pPacket->GetBufferPtr(), dataSize);

    if (retVal != dataSize)
    {
        NotifyClientDisconnected(includeSession);

        // 이런 일은 있어선 안되지만 혹시 모르니 검사, enqueue에서 문제가 난 것은 링버퍼의 크기가 가득찼다는 의미이므로, resize할지 말지는 오류 생길시 가서 테스트하면서 진행
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

    // accept가 완료되었다면 세션에 등록 후, 해당 세션에 패킷 전송
    SESSION* Session = new SESSION;
    Session->isAlive = true;

    // 소켓 정보 추가
    Session->sock = ClientSocket;

    // IP / PORT 정보 추가
    memcpy(Session->IP, winSockManager.GetIP(ClientAddr).c_str(), sizeof(Session->IP));
    Session->port = winSockManager.GetPort(ClientAddr);

    // UID 부여
    Session->uid = id;

    Session->flagField = 0;

    CPlayer* pPlayer = new CPlayer(posX, posY, direction, hp);
    pPlayer->SetFlagField(&Session->flagField);
    pPlayer->SetSpeed(MOVE_X_PER_FRAME, MOVE_Y_PER_FRAME);

    Session->pPlayer = pPlayer;

    return Session;
}