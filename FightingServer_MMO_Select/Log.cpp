#pragma once
#include "Log.h"

// 해당 변수를 변경하여 런타임으로 로그 정보를 남긴다.
int	g_iLogLevel = dfLOG_LEVEL_ERROR;

void Log(const WCHAR* msg, int errCode, int logLevel, const WCHAR* file)
{
	if (g_iLogLevel > logLevel) return;

	FILE* pFile;
	bool result = _wfopen_s(&pFile, file, L"a");
	if (result == 0)
	{
		fwprintf(pFile, L"%s", msg);
		if (errCode != 0)
		{
			fwprintf(pFile, L" , errCode : %d", errCode);
		}
		fwprintf(pFile, L"\n");

		fclose(pFile);
	}
}