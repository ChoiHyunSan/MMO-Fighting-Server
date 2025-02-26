#include "CPacket.h"
#include <iostream>
using namespace std;

CPacket::CPacket(int iBufferSize)
	: _buffer(nullptr), _dataSize(0), _readIndex(0), _bufferSize(iBufferSize)
{
	_buffer = new char[_bufferSize];
}

CPacket::~CPacket()
{
	delete[] _buffer;
}

int CPacket::MoveWritePos(int iSize)
{
	if (_dataSize + iSize < 0 || _dataSize + iSize > _bufferSize - 1)
		return 0;

	_dataSize += iSize;

	return iSize;
}

int CPacket::MoveReadPos(int iSize)
{
	if (_readIndex + iSize < 0 || _readIndex + iSize > _bufferSize - 1)
		return 0;

	_readIndex += iSize;
	return iSize;
}

 CPacket& CPacket::operator=(CPacket& clSrcPacket)
{
	if (_buffer != nullptr)
	{
		free(_buffer);
	}

	_buffer = (char*)malloc(clSrcPacket.GetBufferSize());
	if (_buffer != nullptr);
	{
		memcpy(_buffer, clSrcPacket.GetBufferPtr(), clSrcPacket.GetBufferSize());
	}

	return *this;
}

 CPacket& CPacket::operator<<(unsigned char byValue)
{
	if (_bufferSize < _dataSize + sizeof(unsigned char)) return *this;

	memcpy((_buffer + _dataSize), &byValue, sizeof(unsigned char));
	_dataSize += sizeof(unsigned char);

	return *this;
}

inline CPacket& CPacket::operator<<(char chValue)
{
	if (_bufferSize < _dataSize + sizeof(char)) return *this;

	memcpy((_buffer + _dataSize), &chValue, sizeof(char));
	_dataSize += sizeof(char);

	return *this;
}

CPacket& CPacket::operator<<(short shValue)
{
	if (_bufferSize < _dataSize + sizeof(short)) return *this;

	memcpy((_buffer + _dataSize), &shValue, sizeof(short));
	_dataSize += sizeof(short);

	return *this;
}

 CPacket& CPacket::operator<<(unsigned short wValue)
{
	if (_bufferSize < _dataSize + sizeof(unsigned short)) return *this;

	memcpy((_buffer + _dataSize), &wValue, sizeof(unsigned short));
	_dataSize += sizeof(unsigned short);

	return *this;
}

 CPacket& CPacket::operator<<(int iValue)
{
	if (_bufferSize < _dataSize + sizeof(int)) return *this;

	memcpy((_buffer + _dataSize), &iValue, sizeof(int));
	_dataSize += sizeof(int);

	return *this;
}

 CPacket& CPacket::operator<<(DWORD iValue)
{
	if (_bufferSize < _dataSize + sizeof(DWORD)) return *this;

	memcpy((_buffer + _dataSize), &iValue, sizeof(DWORD));
	_dataSize += sizeof(DWORD);

	return *this;
}

 CPacket& CPacket::operator<<(long lValue)
{
	if (_bufferSize < _dataSize + sizeof(long)) return *this;

	memcpy((_buffer + _dataSize), &lValue, sizeof(long));
	_dataSize += sizeof(long);

	return *this;
}

 CPacket& CPacket::operator<<(float fValue)
{
	if (_bufferSize < _dataSize + sizeof(float)) return *this;

	memcpy((_buffer + _dataSize), &fValue, sizeof(float));
	_dataSize += sizeof(float);

	return *this;
}

CPacket& CPacket::operator<<(__int64 iValue)
{
	if (_bufferSize < _dataSize + sizeof(__int64)) return *this;

	memcpy((_buffer + _dataSize), &iValue, sizeof(__int64));
	_dataSize += sizeof(__int64);

	return *this;
}

CPacket& CPacket::operator<<(double dValue)
{
	if (_bufferSize < _dataSize + sizeof(double)) return *this;

	memcpy((_buffer + _dataSize), &dValue, sizeof(double));
	_dataSize += sizeof(double);

	return *this;
}

CPacket& CPacket::operator>>(BYTE& byValue)
{
	if ((_dataSize - _readIndex) < sizeof(BYTE)) return *this; 

	byValue = (BYTE)*(BYTE*)(_buffer + _readIndex);
	_readIndex += sizeof(BYTE);

	return *this;
}

 CPacket& CPacket::operator>>(char& chValue)
{
	if ((_dataSize - _readIndex) < sizeof(char)) return *this;

	chValue = (char) * (char*)(_buffer + _readIndex);
	_readIndex += sizeof(char);

	return *this;
}

 CPacket& CPacket::operator>>(short& shValue)
{
	if ((_dataSize - _readIndex) < sizeof(short)) return *this;

	shValue = (short) * (short*)(_buffer + _readIndex);
	_readIndex += sizeof(short);

	return *this;
}

CPacket& CPacket::operator>>(WORD& wValue)
{
	if ((_dataSize - _readIndex) < sizeof(WORD)) return *this;

	wValue = (WORD) * (WORD*)(_buffer + _readIndex);
	_readIndex += sizeof(WORD);

	return *this;
}

CPacket& CPacket::operator>>(int& iValue)
{
	if ((_dataSize - _readIndex) < sizeof(int)) return *this;

	iValue = (int) * (int*)(_buffer + _readIndex);
	_readIndex += sizeof(int);

	return *this;
}

 CPacket& CPacket::operator>>(DWORD& dwValue)
{
	if ((_dataSize - _readIndex) < sizeof(DWORD)) return *this;

	dwValue = (DWORD)* (DWORD*)(_buffer + _readIndex);
	_readIndex += sizeof(DWORD);

	return *this;
}

 CPacket& CPacket::operator>>(float& fValue)
{
	if ((_dataSize - _readIndex) < sizeof(float)) return *this;

	fValue = (float) * (float*)(_buffer + _readIndex);
	_readIndex += sizeof(float);

	return *this;
}

CPacket& CPacket::operator>>(__int64& iValue)
{
	if ((_dataSize - _readIndex) < sizeof(__int64)) return *this;

	iValue = (__int64) * (__int64*)(_buffer + _readIndex);
	_readIndex += sizeof(__int64);

	return *this;
}

 CPacket& CPacket::operator>>(double& dValue)
{
	if ((_dataSize - _readIndex) < sizeof(double)) return *this;

	dValue = (double) * (double*)(_buffer + _readIndex);
	_readIndex += sizeof(double);

	return *this;
}

inline int CPacket::GetData(char* chpDest, int iSize)
{
	iSize = min(iSize, (_dataSize - _readIndex));

	memcpy(chpDest, (_buffer + _readIndex), iSize);
	_readIndex += iSize;

	return iSize;
}

inline int CPacket::PutData(char* chpSrc, int iSrcSize)
{
	iSrcSize = min(iSrcSize, (_bufferSize - _dataSize));

	memcpy((_buffer + _dataSize), chpSrc, iSrcSize);
	_dataSize += iSrcSize;

	return iSrcSize;
}
