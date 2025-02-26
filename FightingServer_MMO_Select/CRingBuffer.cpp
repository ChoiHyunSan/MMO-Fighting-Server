#include "CRingBuffer.h"
#include <Windows.h>
#include <iostream>
using namespace std;

CRingBuffer::CRingBuffer()
	: _bufferSize(RINGBUFFER_SIZEMAX), _buf(nullptr)
{
	Init();
}

CRingBuffer::CRingBuffer(int size)
	: _bufferSize(size + 1), _buf(nullptr)
{
	Init();
}

CRingBuffer::~CRingBuffer()
{
	delete[] _buf;
}

void CRingBuffer::Init()
{
	_buf = new char[_bufferSize];
	// _buf = (char*)malloc(sizeof(char) * _bufferSize);
	if (_buf == nullptr)
	{
		// 에러처리
		return;
	}

	_front = _rear = 0;
}

void CRingBuffer::Resize(const int size)
{
	if (GetUseSize() > size) return;

	char* newBuf = new char[size + 1];
	if (newBuf == nullptr)
	{
		// 에러 처리

		return;
	}

	// 비어있는 경우
	if (_front == _rear)
	{
		delete[] _buf;
		_buf = newBuf;
		_bufferSize = size;
		return;
	};

	if (_front < _rear)
	{
		memcpy(newBuf, &_buf[(_front + 1) % _bufferSize], _rear - _front);
	}
	else
	{
		// front + 1 ~ last Index
		memcpy(newBuf, &_buf[(_front + 1) % _bufferSize], _bufferSize - _front);

		// first Index ~ rear
		memcpy(newBuf + (_bufferSize - _front), _buf, _rear + 1);
	}

	delete[] _buf;
	_buf = newBuf;
	_bufferSize = size;
	return;
}

/////////////////////////////////////////////////////////////////////////
// WritePos 에 데이타 넣음.
//
// Parameters: (char *)데이타 포인터. (int)크기. 
// Return: (int)넣은 크기.
/////////////////////////////////////////////////////////////////////////
int CRingBuffer::Enqueue(char* chpData, int iSize)
{
	int canEnqueueSize = GetFreeSize();

	// Enqueue가능한 사이즈보다 iSize보다 크다면, 어짜피 다 못담기 때문에 크기를 맞춘다.
	if (iSize > canEnqueueSize)
	{
		iSize = canEnqueueSize;
	}

	if (_front <= _rear)
	{
		// rear 다음부터 마지막 인덱스의 남은 공간
		int rearBackSize = _bufferSize - _rear - 1;

		// rear뒤의 공간이 iSize만큼 채울 공간이 있는 경우
		if (rearBackSize >= iSize)
		{
			memcpy(&_buf[(_rear + 1) % _bufferSize], chpData, iSize);
			_rear = (_rear + iSize) % _bufferSize;

			//if (*(int*)(_buf + RINGBUFFER_SIZE) != 0xfdfdfdfd)
			//{
			//	cout << L"error" << endl;
			//}
			return iSize;
		}
		else
		{
			// rear 다음부터 맨 끝까지 채우기
			memcpy(&_buf[(_rear + 1) % _bufferSize], chpData, rearBackSize);

			// 0부터 front이전까지 채우기
			memcpy(_buf, chpData + rearBackSize, iSize - rearBackSize);

			_rear = (_rear + iSize) % _bufferSize;
			return iSize;
		}
	}
	else
	{
		// rear 다음부터 iSize만큼 채운다.
		memcpy(&_buf[(_rear + 1) % _bufferSize], chpData, iSize);
		_rear = (_rear + iSize) % _bufferSize;

		return iSize;
	}
}

/////////////////////////////////////////////////////////////////////////
// ReadPos 에서 데이타 가져옴. ReadPos 이동.
//
// Parameters: (char *)데이타 포인터. (int)크기.
// Return: (int)가져온 크기.
/////////////////////////////////////////////////////////////////////////
int CRingBuffer::Dequeue(char* chpDest, int iSize)
{
	// 꺼내려는 크기가 들어있는 크기보다 크다면 iSize를 줄인다.
	int useSize = GetUseSize();
	if (useSize == 0) return 0;

	if (iSize > useSize)
	{
		iSize = useSize;
	}

	if (_front < _rear)
	{
		memcpy(chpDest, &_buf[_front + 1], iSize);
		_front = (_front + iSize) % _bufferSize;
		return iSize;
	}
	else
	{
		// front 다음부터 마지막 인덱스까지의 공간
		int frontBackSize = _bufferSize - _front - 1;

		if (frontBackSize > iSize)
		{
			memcpy(chpDest, &_buf[(_front + 1) % _bufferSize], iSize);
			_front = (_front + iSize) % _bufferSize;
			return iSize;
		}
		else
		{
			memcpy(chpDest, &_buf[(_front + 1) % _bufferSize], frontBackSize);
			memcpy(chpDest + frontBackSize, _buf, iSize - frontBackSize);
			_front = (_front + iSize) % _bufferSize;
			return iSize;
		}
	}
}

/////////////////////////////////////////////////////////////////////////
// ReadPos 에서 데이타 읽어옴. ReadPos 고정.
//
// Parameters: (char *)데이타 포인터. (int)크기.
// Return: (int)가져온 크기.
/////////////////////////////////////////////////////////////////////////
int CRingBuffer::Peek(char* chpDest, int iSize)
{
	// 꺼내려는 크기가 들어있는 크기보다 크다면 iSize를 줄인다.
	int useSize = GetUseSize();
	if (useSize == 0) return 0;

	if (iSize > useSize)
	{
		iSize = useSize;
	}

	if (_front < _rear)
	{
		memcpy(chpDest, &_buf[_front + 1], iSize);
		return iSize;
	}
	else
	{
		// front 다음부터 마지막 인덱스까지의 공간
		int frontBackSize = _bufferSize - _front - 1;

		if (frontBackSize > iSize)
		{
			memcpy(chpDest, &_buf[(_front + 1) % _bufferSize], iSize);
			return iSize;
		}
		else
		{
			memcpy(chpDest, &_buf[(_front + 1) % _bufferSize], frontBackSize);
			memcpy(chpDest + frontBackSize, _buf, iSize - frontBackSize);
			return iSize;
		}
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////
// 원하는 길이만큼 읽기위치 에서 삭제 / 쓰기 위치 이동
//
// Parameters: 없음.
// Return: (int)이동크기
/////////////////////////////////////////////////////////////////////////
int CRingBuffer::MoveRear(int iSize)
{
	if (iSize >= 0)
	{
		if (iSize > GetFreeSize())
		{
			iSize = GetFreeSize();
		}
	}
	else
	{
		if (iSize * -1 > GetUseSize())
		{
			iSize = GetUseSize() * -1;
		}
	}

	_rear = (_rear + iSize + _bufferSize) % _bufferSize;
	return iSize;
}
int CRingBuffer::MoveFront(int iSize)
{
	if (iSize >= 0)
	{
		if (iSize > GetUseSize())
		{
			iSize = GetUseSize();
		}
	}
	else
	{
		if (iSize * - 1> GetFreeSize())
		{
			iSize = GetFreeSize() * -1;
		}
	}

	_front = (_front + iSize + _bufferSize) % _bufferSize;
	return iSize;
}

/////////////////////////////////////////////////////////////////////////
// 버퍼의 Front 포인터 얻음.
//
// Parameters: 없음.
// Return: (char *) 버퍼 포인터.
/////////////////////////////////////////////////////////////////////////
char* CRingBuffer::GetFrontBufferPtr(void)
{
	return &_buf[_front];
}

/////////////////////////////////////////////////////////////////////////
// 버퍼의 RearPos 포인터 얻음.
//
// Parameters: 없음.
// Return: (char *) 버퍼 포인터.
/////////////////////////////////////////////////////////////////////////
char* CRingBuffer::GetRearBufferPtr(void)
{
	return &_buf[_rear];
}

/////////////////////////////////////////////////////////////////////////
// 버퍼의 WritePos 포인터 얻음.
//
// Parameters: 없음.
// Return: (char *) 버퍼 포인터.
/////////////////////////////////////////////////////////////////////////
char* CRingBuffer::GetWritePtr(void)
{
	return &_buf[(_rear + 1) % _bufferSize];
}
