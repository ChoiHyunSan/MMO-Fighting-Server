#include "CProfiler.h"
#include <iostream>

CProfiler g_profiler;

//////////////////////////////////////////////////////////////////////////////////
//
//  Begin(const char* name)
//	- ���� Ÿ�̸� üũ�� �����ϴ� �Լ�
//	- ���� ������ �ִ� ������ �ʰ��ϰų� ¦�� ���� �ʰ� ȣ��Ǵ� ��� ������ �α��Ѵ�.
// 
//////////////////////////////////////////////////////////////////////////////////
void CProfiler::Begin(const char* name)
{
	// �̹� �ִ� ���
	for (int index = 0; index < DATA_COUNT; index++)
	{
		if (_profileDatas[index].flag == true 
			&& strcmp(_profileDatas[index].name , name) == 0)
		{
			// �ʱ�ȭ �Ǿ����� ���� ��� (End�� ȣ����� �ʰ� �ٽ� Begin�� ȣ��� ���)
			if (_profileDatas[index].startTime.QuadPart != 0)
			{
				// �ٽ� Begin�� ȣ���� �������� �ð��� ���.
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

	// ���� ���
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
	
	// �� �̻� �±׸� �� �� ���� ��� (out of data_count)

	return;
}

void CProfiler::End(const char* name)
{
	for (int index = 0; index < DATA_COUNT; index++)
	{
		if (_profileDatas[index].flag == true
			&& strcmp(_profileDatas[index].name, name) == 0)
		{
			// �ð��� �����Ǿ� ���� ���� ���
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

	// �߸� End�� ȣ��� ���

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

		// ���, Min, Max ���ϱ�
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

	fprintf(pFile, "|%18s |%17.4f�� |%10.4f�� |%9.4f�� |%11d | \n", _profileDatas[index].name, average, min, max, _profileDatas[index].callCount);
	}
	fprintf(pFile, "---------------------------------------------------------------------------------- \n");

	fclose(pFile);
}
