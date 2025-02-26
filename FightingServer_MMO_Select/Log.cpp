#pragma once
#include "Log.h"

// �ش� ������ �����Ͽ� ��Ÿ������ �α� ������ �����.
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