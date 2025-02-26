#pragma once
#include <Windows.h>

//////////////////////////////////////////////////////////////////////////////////
//
//	Profiler
//	- ���ϴ� ��ɿ� ���� ���� �м��� ���� �ð��� �����Ͽ� ������ ����� �����ش�.
//	- PROFLIE �� define�Ͽ� ���� ������ ����� �� �ִ�.
// 
//////////////////////////////////////////////////////////////////////////////////

#define DATA_COUNT 10
#define PROFILE

#define TICK_VER

//////////////////////////////////////////////////////////////////////////////////
//
//	Profile_Data
//	- Profile�� �ʿ��� �����͸� ��Ƴ��� ����ü
//	
//////////////////////////////////////////////////////////////////////////////////
struct Profile_Data
{
	bool			flag;				// ������� �������� üũ
	char			name[64];			// ������ �̸�

	LARGE_INTEGER	startTime;			// ���� ���� �ð�

	__int64			totalTime;			// ���� �ð�
	__int64			min[2] = { 0,0 };	// �ּ� �� 
	__int64			max[2] = { 0,0 };	// �ִ� ��

	int				callCount;			// ȣ�� Ƚ��

	Profile_Data() : flag(false), totalTime(0), callCount(0)
	{
		startTime.QuadPart = 0;

	}
};

class CProfiler
{
public:
	CProfiler()
	{

	}

	~CProfiler()
	{

	}

public:
	void Begin(const char* name);
	void End(const char* name);

	void Reset();
	void DataOut(const char* fileName);

private:
	Profile_Data _profileDatas[DATA_COUNT];
	HANDLE	     _errorFIle;
};

extern CProfiler g_profiler;

#ifdef PROFILE
#define PRO_BEGIN(name)			g_profiler.Begin(name)
#define PRO_END(name)			g_profiler.End(name)
#define PRO_PRINT(fileName)		g_profiler.DataOut(fileName)
#define PRO_RESET				g_profiler.Reset()
#else
#define PRO_BEGIN(name)	
#define PRO_END(name)	
#define PRO_PRINT(fileName)
#define PRO_RESET
#endif

