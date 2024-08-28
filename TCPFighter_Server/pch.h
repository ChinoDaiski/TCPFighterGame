#pragma once

#define NOMINMAX
#include <iostream>

#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include <Windows.h>
#pragma comment(lib, "Winmm.lib")




//===================================================
// 자료구조
//===================================================
#include <list>
#include <string>
#include <vector>
//===================================================

//===================================================
// 링버퍼 사용
//===================================================
#include "RingBuffer.h"
//===================================================

//===================================================
// 패킷 정보
//===================================================
#include "PacketHeader.h"
#include "makePacket.h"
//===================================================

//===================================================
// 프로젝트에서 사용되는 define 된 정보
//===================================================
#include "Defines.h"
//===================================================