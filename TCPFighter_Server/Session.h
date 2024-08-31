#pragma once

typedef struct _tagSession
{
    // ���� ���θ� �Ǻ��ϴ� ����
    bool isAlive;

    // ���� info - ����, ip, port
    USHORT port;
    char IP[16];
    SOCKET sock;
    CRingBuffer recvQ;  // ���ſ� ������
    CRingBuffer sendQ;  // �۽ſ� ������

    // ���� INFO
    UINT16 uid;
}SESSION;