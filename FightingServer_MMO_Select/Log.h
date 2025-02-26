#pragma once
/*************************

	 Logging Function

**************************/
#include "pch.h"

#define dfLOG_LEVEL_NONE	-1
#define dfLOG_LEVEL_ERROR	0
#define dfLOG_LEVEL_DEBUG	1
#define dfLOG_LEVEL_SYSTEM	2

extern int		g_iLogLevel;

#define __LOG(msg, logLevel)		\
do{									\
	if(g_iLogLevel >=  logLevel)	\
		wcout << msg << endl;		\
}while(0)							\

void Log(const WCHAR* msg, int errCode = 0, int logLevel = dfLOG_LEVEL_NONE, const WCHAR* file = L"SocketError.txt");
