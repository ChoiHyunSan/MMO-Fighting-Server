#pragma once

#pragma warning(disable : 4996)

#include <iostream>
#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#include <vector>
#include <queue>
#include <map>
#include <unordered_map>
#include <conio.h>
#include "Log.h"
#include <Windows.h>
#include "CRingBuffer.h"
#include <set>
#include <iomanip>
#pragma comment(lib, "ws2_32.lib")

#include <mmsystem.h>
#pragma comment (lib, "winmm")

#include "Define.h"
#include "CProfiler.h"
#include "CPacket.h"
using namespace std;

#include "Protocol.h"

#define DEBUG