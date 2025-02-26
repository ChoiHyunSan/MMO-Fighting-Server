#include "CProfiler.h"
#include <iostream>

CProfiler g_profiler;

//////////////////////////////////////////////////////////////////////////////////
//
//  Begin(const char* name)
//	- 성능 타이머 체크를 시작하는 함수
//	- 저장 가능한 최대 개수를 초과하거나 짝이 맞지 않게 호출되는 경우 에러를 로깅한다.
// 
//////////////////////////////////////////////////////////////////////////////////
void CProfiler::Begin(const char* name)
{
	// 이미 있는 경우
	for (int index = 0; index < DATA_COUNT; index++)
	{
		if (_profileDatas[index].flag == true 
			&& strcmp(_profileDatas[index].name , name) == 0)
		{
			// 초기화 되어있지 않은 경우 (End가 호출되지 않고 다시 Begin이 호출된 경우)
			if (_profileDatas[index].startTime.QuadPart != 0)
			{
				// 다시 Begin을 호출한 시점부터 시간을 잰다.
				QueryPerformanceCounter(&_profileDatas[index].startTime);
				return;
			}

			strcpy_s(_profileDatas[index].name, name);
			_profileDatas[index].flag		= true;
			_profileDatas[index].callCount += 1;
			QueryPerformanceCounter(&_profileDatas[index].startTime);
			return;
		}
	}

	// 없는 경우
	for (int index = 0; index < DATA_COUNT; index++)
	{
		if (_profileDatas[index].flag == false)
		{
			if (_profileDatas[index].startTime.QuadPart != 0)
			{
				return;
			}

			strcpy_s(_profileDatas[index].name, name);
			_profileDatas[index].flag = true;
			_profileDatas[index].callCount += 1;
			QueryPerformanceCounter(&_profileDatas[index].startTime);
			return;
		}
	}
	
	// 더 이상 태그를 걸 수 없는 경우 (out of data_count)

	return;
}

void CProfiler::End(const char* name)
{
	for (int index = 0; index < DATA_COUNT; index++)
	{
		if (_profileDatas[index].flag == true
			&& strcmp(_profileDatas[index].name, name) == 0)
		{
			// 시간이 측정되어 있지 않은 경우
			if (_profileDatas[index].startTime.QuadPart == 0)
			{
				return;
			}
			LARGE_INTEGER endTime;
			QueryPerformanceCounter(&endTime);
			
			_int64 time = endTime.QuadPart - _profileDatas[index].startTime.QuadPart;

			if (_profileDatas[index].max[0] < time)
			{
				if (_profileDatas[index].max[1] < time)
				{
					_profileDatas[index].max[0] = _profileDatas[index].max[1];
					_profileDatas[index].max[1] = time;
				}
				else
				{
					_profileDatas[index].max[0] = time;
				}
			}
			else if (_profileDatas[index].min[0] > time
				|| _profileDatas[index].min[0] == 0)
			{
				if (_profileDatas[index].min[1] > time
					|| _profileDatas[index].min[1] == 0)
				{
					_profileDatas[index].min[0] = _profileDatas[index].min[1];
					_profileDatas[index].min[1] = time;
				}
				else
				{
					_profileDatas[index].min[0] = time;
				}
			}

			_profileDatas[index].totalTime += time;
			_profileDatas[index].startTime.QuadPart = 0;

			return;
		}
	}

	// 잘못 End가 호출된 경우

}

void CProfiler::Reset()
{
	for (int index = 0; index < DATA_COUNT; index++)
	{
		if (_profileDatas[index].flag == true)
		{
			_profileDatas[index].callCount = 0;
			_profileDatas[index].max[0] = 0;
			_profileDatas[index].max[1] = 0;
			_profileDatas[index].min[0] = 0;
			_profileDatas[index].min[1] = 0;
			_profileDatas[index].startTime.QuadPart = 0;
			_profileDatas[index].totalTime = 0;
		}
	}
}

void CProfiler::DataOut(const char* fileName)
{
	FILE* pFile;
	fopen_s(&pFile, fileName, "w");
	if (pFile == nullptr)
	{
		return;
	}

	fprintf(pFile, "---------------------------------------------------------------------------------- \n");
	fprintf(pFile, "|              Name |            Average |         Min |        Max |       Call | \n");

	for (int index = 0; index < DATA_COUNT; index++)
	{
		if (_profileDatas[index].flag == false) continue;

		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);

		// 평균, Min, Max 구하기
		int minMaxCount = 0;
		if (_profileDatas[index].max[1] != 0) minMaxCount++;
		if (_profileDatas[index].max[0] != 0) minMaxCount++;
		if (_profileDatas[index].min[0] != 0) minMaxCount++;
		if (_profileDatas[index].min[1] != 0) minMaxCount++;

		__int64 totalCount = _profileDatas[index].totalTime - (_profileDatas[index].max[0] + _profileDatas[index].max[1] + _profileDatas[index].min[0] + _profileDatas[index].min[1]);

#ifdef  TICK_VER
		float average = (float)totalCount / (_profileDatas[index].callCount - minMaxCount);
		float min = (float)_profileDatas[index].min[1];
		float max = (float)_profileDatas[index].max[1];
#endif

#ifndef  TICK_VER
		float average = (float)totalCount / (_profileDatas[index].callCount - minMaxCount) / freq.QuadPart;
		float min = (float)_profileDatas[index].min[1] / freq.QuadPart;
		float max = (float)_profileDatas[index].max[1] / freq.QuadPart;
#endif 

	fprintf(pFile, "|%18s |%17.4f㎲ |%10.4f㎲ |%9.4f㎲ |%11d | \n", _profileDatas[index].name, average, min, max, _profileDatas[index].callCount);
	}
	fprintf(pFile, "---------------------------------------------------------------------------------- \n");

	fclose(pFile);
}
