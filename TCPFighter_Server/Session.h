#pragma once

typedef struct _tagSession
{
    // 삭제 여부를 판별하는 변수
    bool isAlive;

    // 세션 info - 소켓, ip, port
    USHORT port;
    char IP[16];
    SOCKET sock;
    CRingBuffer recvQ;  // 수신용 링버퍼
    CRingBuffer sendQ;  // 송신용 링버퍼

    // 유저 INFO
    UINT16 uid;
}SESSION;