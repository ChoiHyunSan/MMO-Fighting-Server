#pragma once
#include <Windows.h>

//////////////////////////////////////////////////////////////////////////////////
//
//	Profiler
//	- 원하는 기능에 대한 성능 분석을 위해 시간을 측정하여 진행한 결과를 보여준다.
//	- PROFLIE 을 define하여 원할 때마다 사용할 수 있다.
// 
//////////////////////////////////////////////////////////////////////////////////

#define DATA_COUNT 10
#define PROFILE

#define TICK_VER

//////////////////////////////////////////////////////////////////////////////////
//
//	Profile_Data
//	- Profile에 필요한 데이터를 모아놓은 구조체
//	
//////////////////////////////////////////////////////////////////////////////////
struct Profile_Data
{
	bool			flag;				// 사용중인 샘플인지 체크
	char			name[64];			// 데이터 이름

	LARGE_INTEGER	startTime;			// 측정 시작 시간

	__int64			totalTime;			// 총합 시간
	__int64			min[2] = { 0,0 };	// 최소 값 
	__int64			max[2] = { 0,0 };	// 최대 값

	int				callCount;			// 호출 횟수

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

