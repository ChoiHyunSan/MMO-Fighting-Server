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
		// ����ó��
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
		// ���� ó��

		return;
	}

	// ����ִ� ���
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
// WritePos �� ����Ÿ ����.
//
// Parameters: (char *)����Ÿ ������. (int)ũ��. 
// Return: (int)���� ũ��.
/////////////////////////////////////////////////////////////////////////
int CRingBuffer::Enqueue(char* chpData, int iSize)
{
	int canEnqueueSize = GetFreeSize();

	// Enqueue������ ������� iSize���� ũ�ٸ�, ��¥�� �� ����� ������ ũ�⸦ �����.
	if (iSize > canEnqueueSize)
	{
		iSize = canEnqueueSize;
	}

	if (_front <= _rear)
	{
		// rear �������� ������ �ε����� ���� ����
		int rearBackSize = _bufferSize - _rear - 1;

		// rear���� ������ iSize��ŭ ä�� ������ �ִ� ���
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
			// rear �������� �� ������ ä���
			memcpy(&_buf[(_rear + 1) % _bufferSize], chpData, rearBackSize);

			// 0���� front�������� ä���
			memcpy(_buf, chpData + rearBackSize, iSize - rearBackSize);

			_rear = (_rear + iSize) % _bufferSize;
			return iSize;
		}
	}
	else
	{
		// rear �������� iSize��ŭ ä���.
		memcpy(&_buf[(_rear + 1) % _bufferSize], chpData, iSize);
		_rear = (_rear + iSize) % _bufferSize;

		return iSize;
	}
}

/////////////////////////////////////////////////////////////////////////
// ReadPos ���� ����Ÿ ������. ReadPos �̵�.
//
// Parameters: (char *)����Ÿ ������. (int)ũ��.
// Return: (int)������ ũ��.
/////////////////////////////////////////////////////////////////////////
int CRingBuffer::Dequeue(char* chpDest, int iSize)
{
	// �������� ũ�Ⱑ ����ִ� ũ�⺸�� ũ�ٸ� iSize�� ���δ�.
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
		// front �������� ������ �ε��������� ����
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
// ReadPos ���� ����Ÿ �о��. ReadPos ����.
//
// Parameters: (char *)����Ÿ ������. (int)ũ��.
// Return: (int)������ ũ��.
/////////////////////////////////////////////////////////////////////////
int CRingBuffer::Peek(char* chpDest, int iSize)
{
	// �������� ũ�Ⱑ ����ִ� ũ�⺸�� ũ�ٸ� iSize�� ���δ�.
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
		// front �������� ������ �ε��������� ����
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
// ���ϴ� ���̸�ŭ �б���ġ ���� ���� / ���� ��ġ �̵�
//
// Parameters: ����.
// Return: (int)�̵�ũ��
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
// ������ Front ������ ����.
//
// Parameters: ����.
// Return: (char *) ���� ������.
/////////////////////////////////////////////////////////////////////////
char* CRingBuffer::GetFrontBufferPtr(void)
{
	return &_buf[_front];
}

/////////////////////////////////////////////////////////////////////////
// ������ RearPos ������ ����.
//
// Parameters: ����.
// Return: (char *) ���� ������.
/////////////////////////////////////////////////////////////////////////
char* CRingBuffer::GetRearBufferPtr(void)
{
	return &_buf[_rear];
}

/////////////////////////////////////////////////////////////////////////
// ������ WritePos ������ ����.
//
// Parameters: ����.
// Return: (char *) ���� ������.
/////////////////////////////////////////////////////////////////////////
char* CRingBuffer::GetWritePtr(void)
{
	return &_buf[(_rear + 1) % _bufferSize];
}
