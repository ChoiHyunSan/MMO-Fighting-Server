#pragma once
#include "Server.h"

int Server::_updateCnt = 0;
bool Server::_shutdown = true;

Server::Server()
{
	// TODO : ���� �ʱ�ȭ�� ����Ѵ�.
	__LOG(L"Server Start", dfLOG_LEVEL_SYSTEM);

	// �ý��� �ʱ�ȭ ����
	{
		srand(7777);
		timeBeginPeriod(1);
	}

	// ��Ʈ��ũ & ������ �ʱ�ȭ
	{
		if (InitNetwork() == false)
		{
			__LOG(L"Server Network Init Fail", dfLOG_LEVEL_ERROR);
			return;
		}
		__LOG(L"Server Network Init Success", dfLOG_LEVEL_SYSTEM);

		if (InitContents() == false)
		{
			__LOG(L"Server Contents Init Fail", dfLOG_LEVEL_ERROR);
			return;
		}
		__LOG(L"Server Contents Init Success", dfLOG_LEVEL_SYSTEM);
	}
}

Server::~Server()
{
	// TODO : ����� ���ҽ��� ��� �����Ѵ�.
	{
		timeEndPeriod(1);

		closesocket(_listenSocket);
		WSACleanup();
	}
}

bool Server::InitNetwork()
{
	// TODO : ��Ʈ��ũ �ʱ�ȭ�� �����͸� ó���Ѵ�.
	
	// ���� ���ҽ� ����
	{
		if (_readSockets == nullptr || _writeSockets == nullptr)
		{
			__LOG(L"Allocate Error", dfLOG_LEVEL_ERROR);
			Log(L"Allocate Error", 0, dfLOG_LEVEL_ERROR);
			return false;
		}
	}

	// ��Ʈ��ũ ����
	{
		// WSA �ʱ�ȭ
		{
			WSAData wsaData;
			if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
			{
				int errCode = WSAGetLastError();
				__LOG(L"WSAStartup function error", dfLOG_LEVEL_ERROR);
				Log(L"WSAStart function error", errCode, dfLOG_LEVEL_ERROR);
				return false;
			}
		}

		// Listen socket ����
		{
			_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
			if (_listenSocket == SOCKET_ERROR)
			{
				int errCode = WSAGetLastError();
				__LOG(L"Socket function error", dfLOG_LEVEL_ERROR);
				Log(L"Socket function error", errCode, dfLOG_LEVEL_ERROR);
				return false;
			}
		}

		// Linger �ɼ� ����
		{
			LINGER linger;
			linger.l_onoff = 1;
			linger.l_linger = 0;
			if (SOCKET_ERROR == setsockopt(_listenSocket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger)))
			{
				int errCode = WSAGetLastError();
				__LOG(L"Linger option error", dfLOG_LEVEL_ERROR);
				Log(L"Linger option error", errCode, dfLOG_LEVEL_ERROR);
				return false;
			}
		}

		// Bind
		{
			SOCKADDR_IN serverAddr;
			memset(&serverAddr, 0, sizeof(serverAddr));
			serverAddr.sin_family = AF_INET;
			serverAddr.sin_port = htons(dfNETWORK_PORT);
			InetPtonW(AF_INET, L"0.0.0.0", &serverAddr.sin_addr);
			if (SOCKET_ERROR == bind(_listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)))
			{
				int errCode = WSAGetLastError();
				__LOG(L"Bind function error", dfLOG_LEVEL_ERROR);
				Log(L"Bind function error", errCode, dfLOG_LEVEL_ERROR);
				return false;
			}
		}

		// Listen
		{
			if (SOCKET_ERROR == listen(_listenSocket, SOMAXCONN_HINT(1000)))
			{
				int errCode = WSAGetLastError();
				__LOG(L"Listen function error", dfLOG_LEVEL_ERROR);
				Log(L"Listen function error", errCode, dfLOG_LEVEL_ERROR);
				return false;
			}
		}

		// Non-Blocking ����
		{
			u_long on = 1;
			if (SOCKET_ERROR == ioctlsocket(_listenSocket, FIONBIO, &on))
			{
				int errCode = WSAGetLastError();
				__LOG(L"Set non-blocking socket error", dfLOG_LEVEL_ERROR);
				Log(L"Set non-blocking socket error", errCode, dfLOG_LEVEL_ERROR);
				return false;
			}
		}
	}

	return true;
}

bool Server::InitContents()
{
	// TODO : ���������� �ʱ�ȭ�� �����͸� ó���Ѵ�.

	memset(_IDArr, 0, sizeof(_IDArr));
	for (int i = 0; i < dfMAX_SESSION; i++)
		_IDQueue.push(i);

	return true;
}

bool Server::UpdateNetwork()
{
	// TODO : READ/WRITE SET�� ���� SELECT�� ó���Ѵ�.
	
	// NetWork Loop�� ����Ѵ�.
	_netCnt++;
	if (_netCnt % NET_SKIP != 0) return true;

	// READ & WRITE set check
	int stReadSet = 0;
	int endReadSet = 0;
	int stWriteSet = 0;
	int endWriteSet = 0;
	
	// Socket ��ȸŽ��
	{
		for (unordered_map<SOCKET, Session*>::iterator iter = _sessionMap.begin(); iter != _sessionMap.end(); iter++)
		{
			_readSockets[endReadSet++] = (*iter).first;	// Session

			if ((*iter).second->_sendRingBuffer.GetUseSize() > 0)
			{
				_writeSockets[endWriteSet++] = (*iter).first;
			}
		}
	}

	// Selectó��
	{
		// ��� �ִ�� ó�������� ������ ���
		while ((endReadSet - stReadSet) >= (FD_SETSIZE - 1)
			&& (endWriteSet - stWriteSet) >= FD_SETSIZE)
		{
			if (SelectProc(stReadSet, FD_SETSIZE - 1, stWriteSet, FD_SETSIZE) == false)
			{
				__LOG(L"SelectProc Fail", dfLOG_LEVEL_ERROR);
				Log(L"SelectProc Fail", 0, dfLOG_LEVEL_ERROR);
				return false;
			}
			stReadSet	+= (FD_SETSIZE - 1);
			stWriteSet	+= FD_SETSIZE;
		}
		// Read�� �ִ�� ó�������� ������ ���
		while ((endReadSet - stReadSet) >= (FD_SETSIZE - 1)
			&& (endWriteSet - stWriteSet) < FD_SETSIZE)
		{
			if (SelectProc(stReadSet, FD_SETSIZE - 1, 0, 0) == false)
			{
				__LOG(L"SelectProc Fail", dfLOG_LEVEL_ERROR);
				Log(L"SelectProc Fail", 0, dfLOG_LEVEL_ERROR);
				return false;
			}

			stReadSet	+= (FD_SETSIZE - 1);
		}
		// Write�� �ִ�� ó�������� ������ ���
		while ((endReadSet - stReadSet) < (FD_SETSIZE - 1)
			&& (endWriteSet - stWriteSet) >= FD_SETSIZE)
		{
			if (SelectProc(0, 0, stWriteSet, FD_SETSIZE) == false)
			{
				__LOG(L"SelectProc Fail", dfLOG_LEVEL_ERROR);
				Log(L"SelectProc Fail", 0, dfLOG_LEVEL_ERROR);
				return false;
			}
			stWriteSet	+= FD_SETSIZE;
		}

		if (SelectProc(stReadSet, (endReadSet - stReadSet), stWriteSet, (endWriteSet - stWriteSet)) == false)
		{
			__LOG(L"SelectProc Fail", dfLOG_LEVEL_ERROR);
			Log(L"SelectProc Fail", 0, dfLOG_LEVEL_ERROR);
			return false;
		}
	}



	PRO_BEGIN("DeleteSession");
	DeleteReserveSession();
	PRO_END("DeleteSession");

	return true;
}

bool Server::UpdateContents()
{
	// TODO : ������ ������ ������ ������ ó���Ѵ�.
	if (SkipFrame()) return true;

	_updateCnt++;
	
	PRO_BEGIN("Update Contents");
	// Logic Update
	for (unordered_map<DWORD, Character*>::iterator iter = _characterMap.begin(); iter != _characterMap.end(); iter++)
	{
		Character* character = (*iter).second;
		if ((timeGetTime() - character->_session->_lastRecvTime) >= dfNETWORK_PACKET_RECV_TIMEOUT)
		{
			ReserveDeleteSession(character->_session);
			continue;
		}

		if (character->_hp <= 0)
		{
			ReserveDeleteSession(character->_session);
			continue;
		}

		// �����̴� �������� Ȯ�� 
		if (character->_moveFlag == false)
			continue;

		PRO_BEGIN("UpdateMove");
		UpdateMove(character);
		PRO_END("UpdateMove");
	}

	PRO_BEGIN("DeleteSession");
	DeleteReserveSession();
	PRO_END("DeleteSession");

	PRO_END("Update Contents");
	return true;
}

inline bool Server::SelectProc(const int readSetIndex, const int readSetSize, const int writeSetIndex, const int writeSetSize)
{
	FD_SET readSet, writeSet;
	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);

	FD_SET(_listenSocket, &readSet);
	for (int index = 0; index < readSetSize; index++)
	{
		FD_SET(_readSockets[readSetIndex + index], &readSet);
	}

	for (int index = 0; index < writeSetSize; index++)
	{
		FD_SET(_writeSockets[writeSetIndex + index], &writeSet);
	}

	timeval timeVal;
	timeVal.tv_sec = 0;
	timeVal.tv_usec = 0;
	select(0, &readSet, &writeSet, nullptr, &timeVal);

	// Select Ž��
	{
		// AcceptProc
		{
			if (FD_ISSET(_listenSocket, &readSet));
			{
				if (AcceptProc() == false)
				{
					__LOG(L"AcceptProc Fail", dfLOG_LEVEL_ERROR);
					Log(L"AcceptProc Fail", 0, dfLOG_LEVEL_ERROR);
					return false;
				}
			}
		}

		// ReadProc
		{
			for (int index = 0; index < readSetSize; index++)
			{
				if (FD_ISSET(_readSockets[readSetIndex + index], &readSet))
				{
					if (ReadProc(_sessionMap[_readSockets[readSetIndex + index]]) == false)
					{
						__LOG(L"ReadProc Fail", dfLOG_LEVEL_ERROR);
						Log(L"ReadProc Fail", 0, dfLOG_LEVEL_ERROR);
						return false;
					}
				}
			}
		}

		// WriteProc
		{
			for (int index = 0; index < writeSetSize; index++)
			{
				if (FD_ISSET(_writeSockets[writeSetIndex + index], &writeSet))
				{
					if (WriteProc(_sessionMap[_writeSockets[writeSetIndex + index]]) == false)
					{
						__LOG(L"WriteProc Fail", dfLOG_LEVEL_ERROR);
						Log(L"WriteProc Fail", 0, dfLOG_LEVEL_ERROR);
						return false;
					}
				}
			}
		}
	}

	// SelectProc�ø��� accpet�� ������ ������ ó���ϴ� �Լ� ����
	{
		while (true)
		{
			FD_ZERO(&readSet);
			FD_SET(_listenSocket, &readSet);

			timeval timeVal;
			timeVal.tv_sec = 0;
			timeVal.tv_usec = 0;
			select(0, &readSet, nullptr, nullptr, &timeVal);

			if (FD_ISSET(_listenSocket, &readSet))
			{
				if (AcceptProc() == false)
				{
					__LOG(L"AcceptProc Fail", dfLOG_LEVEL_ERROR);
					Log(L"AcceptProc Fail", 0, dfLOG_LEVEL_ERROR);
					return false;
				}
			}
			else
			{
				break;
			}
		}
	}

	return true;
}

inline bool Server::ReadProc(Session* session)
{
	PRO_BEGIN("ReadProc");

	session->_lastRecvTime = timeGetTime();

	if (session == nullptr)
	{
		Log(L"Null pointer error in ReadProc()", 0 , dfLOG_LEVEL_ERROR);
		__LOG(L"WriteProc Fail" << __FUNCTIONW__ , dfLOG_LEVEL_ERROR);

		PRO_END("ReadEnd");
		return false;
	}

	// Direct Inputó��
	PRO_BEGIN("ReadProc Len");
	int directSize = session->_recvRingBuffer.DirectEnqueueSize();
	int retVal;
	if (directSize != 0)
	{
		retVal = recv(session->_socket, session->_recvRingBuffer.GetWritePtr(), directSize, 0);
	}
	else if (directSize == 0 && session->_recvRingBuffer.GetFreeSize() != 0)
	{
		retVal = recv(session->_socket, session->_recvRingBuffer.GetWritePtr(), session->_recvRingBuffer.GetFreeSize(), 0);
	}
	else
	{
		if (session->_recvRingBuffer.GetBufferSize() >= RINGBUFFER_SIZEMAX)
		{
			// �� ���۸� �ѱ�� ũ���� �޽����� ���� -> ���� ����
			Log(L"Recv ringbuffer overflow", 0, dfLOG_LEVEL_ERROR);
			__LOG(L"Recv ringbuffer overflow, sessionID : " << session->_ID, dfLOG_LEVEL_ERROR);
			ReserveDeleteSession(session);
			return true;
		}
		else
		{
			session->_recvRingBuffer.Resize((int)(session->_recvRingBuffer.GetBufferSize() * 1.5));
			retVal = recv(session->_socket, session->_recvRingBuffer.GetWritePtr(), session->_recvRingBuffer.GetFreeSize(), 0);
		}
	}
	session->_recvRingBuffer.MoveRear(retVal);
	PRO_END("ReadProc Len");

	if (retVal == 0)
	{
		__LOG(L"Session close, sessionID : " << session->_ID, dfLOG_LEVEL_ERROR);
		ReserveDeleteSession(session);
		return true;
	}
	else if (retVal == SOCKET_ERROR)
	{
		int errCode = WSAGetLastError();
		if (errCode != WSAEWOULDBLOCK)
		{
			if (errCode != WSAECONNRESET)
			{
				Log(L"Recv WSA function error", errCode, dfLOG_LEVEL_ERROR);
				__LOG(L"Recv WSA funcion error, sessionID : " << session->_ID << "errCode : " << errCode, dfLOG_LEVEL_ERROR);
			}
			ReserveDeleteSession(session);
		}
		PRO_END("ReadProc");
		return true;
	}

	if (HandleCSPacket(session) == false)
	{
		Log(L"HandleCSPacket Error", 0, dfLOG_LEVEL_ERROR);
		__LOG(L"HandleCSPacket Error", dfLOG_LEVEL_ERROR);

		PRO_END("ReadProc");
		return false;
	}

	PRO_END("ReadProc");
	return true;
}

inline bool Server::WriteProc(Session* session)
{
	PRO_BEGIN("WriteProc");
	if (session->_sendRingBuffer.GetUseSize() == 0)
		return true;

	while (true)
	{
		char sendBuf[RINGBUFFER_SIZEMAX];
		int ret = session->_sendRingBuffer.Peek(sendBuf, RINGBUFFER_SIZEMAX);
		if (ret == 0)
		{
			break;
		}

		ret = send(session->_socket, sendBuf, ret, 0);
		if (ret == SOCKET_ERROR)
		{
			int errCode = WSAGetLastError();
			if (errCode != WSAEWOULDBLOCK)
			{
				if (errCode != WSAECONNRESET)
				{
					Log(L"Send WSA function Error", errCode, dfLOG_LEVEL_ERROR);
					__LOG(L"Send WSA function Error" << " errCode : " << errCode, dfLOG_LEVEL_ERROR);

					ReserveDeleteSession(session);
				}
			}

			// WSAEWOULDBLOCK���� �۾��� �������� ���� ��쿡�� ���� ������ ���¸� ��ٸ���.
			PRO_END("WriteProc");
			return true;
		}

		session->_sendRingBuffer.MoveFront(ret);
	}
	PRO_END("WriteProc");
	return true;
}

inline bool Server::AcceptProc()
{
	PRO_BEGIN("AcceptProc");

	// Step 1. ���� ID Ž��
	DWORD sessionID = GetSessionID();
	if (sessionID == dfINVALID_ID)
	{
		Log(L"Accept Failed : Full Session", 0, dfLOG_LEVEL_ERROR);
		return true;
	}

	PRO_BEGIN("Accept SOCKET");

	// Step 2. Ŭ���̾�Ʈ �ּ� ����
	SOCKADDR_IN clientAddr;
	memset(&clientAddr, 0, sizeof(clientAddr));
	int addrLen = sizeof(clientAddr);

	// Step 3. Accept�Լ� ȣ�� �� ����ó��
	SOCKET clientSocket = accept(_listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
	if (clientSocket == SOCKET_ERROR)
	{
		int errCode = WSAGetLastError();
		if (errCode != WSAEWOULDBLOCK)
		{
			Log(L"Accept function error", errCode, dfLOG_LEVEL_ERROR);
			__LOG(L"Accept function error in " << __FUNCTIONW__ << ", errCode : " << errCode, dfLOG_LEVEL_ERROR);

			return false;
		}
		return true;
	}

	// Step 4. ���� ID �ο�
	// _IDArr[sessionID] = true;
	_IDQueue.pop();

	PRO_END("Accept SOCKET");

	PRO_BEGIN("Accept Alloc");
	// Step 5. Session & Character ����
	Session* newSession = new Session(clientSocket, sessionID, &clientAddr);
	if (newSession == nullptr)
	{
		Log(L"Session allocate error", 0, dfLOG_LEVEL_ERROR);
		__LOG(L"Session allocate error", dfLOG_LEVEL_ERROR);
		return false;
	}
	Character* newCharacter = new Character(newSession);
	if (newCharacter == nullptr)
	{
		Log(L"Character allocate error", 0, dfLOG_LEVEL_ERROR);
		__LOG(L"Character allocate error", dfLOG_LEVEL_ERROR);
		return false;
	}
	PRO_END("Accept Alloc");

	PRO_BEGIN("Accept Sector");

	// Step 6. �������� Ž��
	SetAroundSector(newCharacter->_sectorPos._x, newCharacter->_sectorPos._y, newCharacter);

	// Step 7. ���� �۾�
	{
		// �ڱ� �ڽſ� ���� ĳ���� ���� ��û
		{
			newSession->_packetBuffer.Clear();
			CreatePacket_CreateMyCharacter(newSession->_packetBuffer, newSession->_ID, newCharacter->_hp, newCharacter->_x, newCharacter->_y, newCharacter->_dir);
			SC_CreateMyCharacter(newSession, newSession->_packetBuffer);
		}

		// ���� ����Ž��
		int sectorCnt = newCharacter->_sectorAround._cnt;
		for (int cnt = 0; cnt < sectorCnt; cnt++)
		{
			stSECTOR_POS* sectorAround = &newCharacter->_sectorAround._around[cnt];
			list<Character*>* sectorList = &_sectorList[sectorAround->_y][sectorAround->_x];
			for (list<Character*>::iterator iter = sectorList->begin(); iter != sectorList->end(); iter++)
			{
				Character* otherCharacter = (*iter);
				Session* otherSession = otherCharacter->_session;

				// ���ο� ���� ������ ���� �޽��� ������
				{
					otherSession->_packetBuffer.Clear();
					CreatePacket_CreateOtherCharacter(otherSession->_packetBuffer, newSession->_ID, newCharacter->_hp, newCharacter->_x, newCharacter->_y, newCharacter->_dir);
					SC_CreateOtherCharacter(otherSession, otherSession->_packetBuffer);
				}

				// ���ο� ���ǿ� ���� ĳ���͸� �����ϴ� �޽����� ������.
				{
					newSession->_packetBuffer.Clear();
					CreatePacket_CreateOtherCharacter(newSession->_packetBuffer, otherSession->_ID, otherCharacter->_hp, otherCharacter->_x, otherCharacter->_y, otherCharacter->_dir);
					// CreatePacket_CreateOtherCharacter(newSession->_packetBuffer, otherSession->_ID, otherCharacter->_hp, otherCharacter->_x, otherCharacter->_y, otherCharacter->_dir);
					SC_CreateOtherCharacter(newSession, newSession->_packetBuffer);
				}

				// ���������� Ȯ���Ͽ� ���� ������
				if (otherCharacter->_moveFlag)
				{
					newSession->_packetBuffer.Clear();
					CreatePacket_MoveStartCharacter(newSession->_packetBuffer, otherSession->_ID, otherCharacter->_moveDir, otherCharacter->_x, otherCharacter->_y);

					SC_SendUnicast((char*)newSession->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), newSession);
				}
			}
		}

	}

	// Step 8. ���Ϳ� �߰�
	PushSectorList(newCharacter->_sectorPos._x, newCharacter->_sectorPos._y, newCharacter);

	// Step 9. ���� �߰�
	{
		if (_sessionMap.find(clientSocket) != _sessionMap.end())
		{
			Log(L"Session already Exist in map", 0, dfLOG_LEVEL_ERROR);

			const char* timeStr = __TIME__;
			wchar_t wTimeStr[9];
			mbstowcs(wTimeStr, timeStr, 9);
			Log(wTimeStr , 0, dfLOG_LEVEL_ERROR);
		}
		_sessionMap[clientSocket]		= newSession;

		if (_characterMap.find(newSession->_ID) != _characterMap.end())
		{
			Log(L"Character already Exist in map", 0, dfLOG_LEVEL_ERROR);

			const char* timeStr = __TIME__;
			wchar_t wTimeStr[9];
			mbstowcs(wTimeStr, timeStr, 9);
			Log(wTimeStr, 0, dfLOG_LEVEL_ERROR);
		}
		_characterMap[newSession->_ID]	= newCharacter;
	}

	PRO_END("Accept Sector");

	PRO_END("AcceptProc");

	return true;
}

inline int Server::GetSessionID()
{
	if(_IDQueue.empty())
		return dfINVALID_ID;

	int ID = _IDQueue.front();
	return ID;	
}

inline void Server::ReserveDeleteSession(Session* session)
{
	_deleteSet.insert(session->_socket);
}

inline void Server::DeleteReserveSession()
{
	for (set<SOCKET>::iterator deleteIter = _deleteSet.begin(); deleteIter != _deleteSet.end(); deleteIter++)
	{
		SOCKET deleteSocket = (*deleteIter);
		DWORD ID = _sessionMap[deleteSocket]->_ID;
		Character* character = _characterMap[ID];

		// �������� �˸� (���� ����)
		Session* deleteSession = _sessionMap[deleteSocket];

		deleteSession->_packetBuffer.Clear();
		CreatePacket_DeleteCharacter(deleteSession->_packetBuffer, ID);
		SC_DeleteCharacter(deleteSession, deleteSession->_packetBuffer);
		
		// _sectorList���� ����
		EraseSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

		// _characterMap���� ����
		delete character;
		
		_characterMap.erase(ID);
		character = nullptr;

		// ID�Ҵ� ����
		_IDQueue.push(ID);

		// _sessionMap���� ����
		delete deleteSession;

		_sessionMap.erase(deleteSocket);

		// ���� ���ҽ� ��ȯ
		closesocket(deleteSocket);
	}

	_deleteSet.clear();
}

inline bool Server::UpdateMove(Character* character)
{
	int newX, newY;

	// ���⿡ ���� swich�� ó��
	switch (character->_moveDir)
	{
		case dfPACKET_MOVE_DIR_LL:
		{
			newX = character->_x - dfX_MOVE;
			newY = character->_y;

			// case 1 : ���� ���ͷ� �Ѿ�� ���
			if (character->_sectorPos._x != 0 /* ���� ���� ���Ͱ� �ƴ� ��� */
				&& ((character->_sectorPos._x - 1) * dfSECTOR_SIZE) > newX)
			{
				InvaidSectorLL(character);			
			}
			break;
		}
		case dfPACKET_MOVE_DIR_LU:
		{
			newX = character->_x - dfX_MOVE;
			newY = character->_y - dfY_MOVE;

			if (character->_sectorPos._x != 0 /* ���� ���� ���Ͱ� �ƴ� ��� */
				&& ((character->_sectorPos._x) * dfSECTOR_SIZE) > newX)
			{
				// case 1 : �������� ���ͷ� �Ѿ�� ���
				if (character->_sectorPos._y != 0 /* ���� ���� ���Ͱ� �ƴ� ��� */
					&& ((character->_sectorPos._y) * dfSECTOR_SIZE) > newY)
				{
					InvaidSectorLU(character);
				}
				// case 2 : ���� ���ͷ� �Ѿ�� ���
				else
				{
					InvaidSectorLL(character);
				}
			}
			else
			{
				// case 3 : ���� ���ͷ� �Ѿ�� ���
				if (character->_sectorPos._y != 0 /* ���� ���� ���Ͱ� �ƴ� ��� */
					&& ((character->_sectorPos._y) * dfSECTOR_SIZE) > newY)
				{
					InvaidSectorUU(character);
				}
			}
			break;
		}
		case dfPACKET_MOVE_DIR_UU:
		{
			newX = character->_x;
			newY = character->_y - dfY_MOVE;

			// case 1 : ���� ���ͷ� �Ѿ�� ���
			if (character->_sectorPos._y != 0 /* ���� ���� ���Ͱ� �ƴ� ��� */
				&& ((character->_sectorPos._y) * dfSECTOR_SIZE) > newY)
			{
				InvaidSectorUU(character);
			}

			break;
		}
		case dfPACKET_MOVE_DIR_RU:
		{
			newX = character->_x + dfX_MOVE;
			newY = character->_y - dfY_MOVE;

			if (character->_sectorPos._x != dfSECTOR_WIDTH_CNT - 1 /* ���� ������ ���Ͱ� �ƴ� ��� */
				&& ((character->_sectorPos._x + 1) * dfSECTOR_SIZE) <= newX)
			{
				// ���������� ���͸� ħ���ϴ� ���
				if (character->_sectorPos._y != 0 /* ���� ���� ���Ͱ� �ƴ� ��� */
					&& ((character->_sectorPos._y) * dfSECTOR_SIZE) > newY)
				{
					InvaidSectorRU(character);
				}
				// ������ ���͸� ħ���ϴ� ���
				else
				{
					InvaidSectorRR(character);
				}
			}
			else
			{
				// ���� ���͸� ħ���ϴ� ���
				if (character->_sectorPos._y != 0 /* ���� ���� ���Ͱ� �ƴ� ��� */
					&& ((character->_sectorPos._y) * dfSECTOR_SIZE) > newY)
				{
					InvaidSectorUU(character);
				}
			}

			break;
		}
		case dfPACKET_MOVE_DIR_RR:
		{
			newX = character->_x + dfX_MOVE;
			newY = character->_y;
		
			// case 1 : ������ ���ͷ� �Ѿ�� ���
			if (character->_sectorPos._x != dfSECTOR_WIDTH_CNT - 1 /* ���� ������ ���Ͱ� �ƴ� ��� */
				&& ((character->_sectorPos._x + 1) * dfSECTOR_SIZE) <= newX)
			{
				InvaidSectorRR(character);
			}
			break;
		}
		case dfPACKET_MOVE_DIR_RD:
		{
			newX = character->_x + dfX_MOVE;
			newY = character->_y + dfY_MOVE;

			if (character->_sectorPos._x != dfSECTOR_WIDTH_CNT - 1 /* ���� ������ ���Ͱ� �ƴ� ��� */
				&& ((character->_sectorPos._x + 1) * dfSECTOR_SIZE) <= newX)
			{
				if (character->_sectorPos._y != dfSECTOR_HEIGHT_CNT - 1 /* ���� �Ʒ��� ���Ͱ� �ƴ� ��� */
					&& ((character->_sectorPos._y + 1) * dfSECTOR_SIZE) <= newY)
				{
					InvaidSectorRD(character);
				}
				else
				{
					InvaidSectorRR(character);
				}
			}
			else
			{
				if (character->_sectorPos._y != dfSECTOR_HEIGHT_CNT - 1 /* ���� �Ʒ��� ���Ͱ� �ƴ� ��� */
					&& ((character->_sectorPos._y + 1) * dfSECTOR_SIZE) <= newY)
				{
					InvaidSectorDD(character);
				}
			}

			break;
		}
		case dfPACKET_MOVE_DIR_DD:
		{
			newX = character->_x;
			newY = character->_y + dfY_MOVE;

			// case 1 : �Ʒ��� ���ͷ� �Ѿ ���
			if (character->_sectorPos._y != dfSECTOR_HEIGHT_CNT - 1 /* ���� �Ʒ��� ���Ͱ� �ƴ� ��� */
				&& ((character->_sectorPos._y + 1) * dfSECTOR_SIZE) <= newY)
			{
				InvaidSectorDD(character);
			}
			break;
		}
		case dfPACKET_MOVE_DIR_LD:
		{
			newX = character->_x - dfX_MOVE;
			newY = character->_y + dfY_MOVE;

			if (character->_sectorPos._x != 0 /* ���� ���� ���Ͱ� �ƴ� ��� */
				&& ((character->_sectorPos._x - 1) * dfSECTOR_SIZE) > newX)
			{
				if (character->_sectorPos._y != dfSECTOR_HEIGHT_CNT - 1 /* ���� �Ʒ��� ���Ͱ� �ƴ� ��� */
					&& ((character->_sectorPos._y + 1) * dfSECTOR_SIZE) <= newY)
				{
					InvaidSectorLD(character);
				}
				else
				{
					InvaidSectorLL(character);
				}
			}
			else
			{
				if (character->_sectorPos._y != dfSECTOR_HEIGHT_CNT - 1 /* ���� �Ʒ��� ���Ͱ� �ƴ� ��� */
					&& ((character->_sectorPos._y + 1) * dfSECTOR_SIZE) <= newY)
				{
					InvaidSectorDD(character);
				}
			}
			break;
		}
		default:
		{
			Log(L"Invalid Move dir Error",0 , dfLOG_LEVEL_ERROR);
			__LOG(L"Invalid move dir from sessionID : " << character->_session->_ID << "  dir : " << character->_moveDir, dfLOG_LEVEL_NONE);
			ReserveDeleteSession(character->_session);
			return true;
		}
	}

	if (CheckBorder(newX, newY) == false)
	{
		return true;
	}
	// __LOG(L"sessionID : " << character->_session->_ID << "dir : " << character->_moveDir << "   X : " << character->_x << "   Y : " << character->_y, dfLOG_LEVEL_DEBUG);
	
	character->_x = newX;
	character->_y = newY;

	return true;
}

inline bool Server::CheckBorder(const int xPos, const int yPos)
{
	if (xPos < dfRANGE_MOVE_LEFT || xPos > dfRANGE_MOVE_RIGHT || yPos < dfRANGE_MOVE_TOP || yPos > dfRANGE_MOVE_BOTTOM)
	{
		return false;
	}
	return true;
}

inline bool Server::CheckMarginCoordinate(Character* character, const int xPos, const int yPos)
{
	// 1) Ŭ���̾�Ʈ�� ���� ��ġ���� ���� ����� ������ ������ Ȯ��
	if (abs(character->_x - xPos) > 50 || abs(character->_y - yPos) > 50)
	{
		return false;
	}

	return true;
}

// ���� ���� ��, true ��ȯ
inline bool Server::AttackSector(const int sX, const int sY, Character* character, const CHAR attackType)
{
	if (sX < 0 || sX >= dfSECTOR_WIDTH_CNT || sY < 0 || sY >= dfSECTOR_HEIGHT_CNT) return false;

	switch (attackType)
	{
		case dfPACKET_SC_ATTACK1:
		{
			for (list<Character*>::iterator iter = _sectorList[sY][sX].begin(); iter != _sectorList[sY][sX].end(); iter++)
			{
				Character* attackChar = (*iter);
				if (attackChar == character) continue;

				if (character->_dir == dfPACKET_MOVE_DIR_LL)
				{
					if (character->_x < attackChar->_x) continue;
					if ((character->_x - attackChar->_x) > dfATTACK1_RANGE_X) continue;
					if (abs(character->_y - attackChar->_y) > dfATTACK1_RANGE_Y / 2) continue;
				}
				else if (character->_dir == dfPACKET_MOVE_DIR_RR)
				{
					if (character->_x > attackChar->_x) continue;
					if ((attackChar->_x - character->_x) > dfATTACK1_RANGE_X) continue;
					if (abs(character->_y - attackChar->_y) > dfATTACK1_RANGE_Y / 2) continue;
				}

				char newHp = attackChar->_hp - dfATTACK1_DAMAGE;
				if (newHp <= 0)
				{
					newHp = 0;
				}

				// �ǰݾ˸� �޽��� ������
				{
					character->_session->_packetBuffer.Clear();
					CreatePacket_Damage(character->_session->_packetBuffer, character->_session->_ID, attackChar->_session->_ID, attackChar->_hp);

					SC_Damage(character->_session, character->_session->_packetBuffer);
				}
				attackChar->_hp = newHp;

				return true;
			}
			break;
		}
		case dfPACKET_SC_ATTACK2:
		{
			for (list<Character*>::iterator iter = _sectorList[sY][sX].begin(); iter != _sectorList[sY][sX].end(); iter++)
			{
				Character* attackChar = (*iter);
				if (attackChar == character) continue;

				if (character->_dir == dfPACKET_MOVE_DIR_LL)
				{
					if (character->_x < attackChar->_x) continue;
					if ((character->_x - attackChar->_x) > dfATTACK2_RANGE_X) continue;
					if (abs(character->_y - attackChar->_y) > dfATTACK2_RANGE_Y / 2) continue;
				}
				else if (character->_dir == dfPACKET_MOVE_DIR_RR)
				{
					if (character->_x > attackChar->_x) continue;
					if ((attackChar->_x - character->_x) > dfATTACK2_RANGE_X) continue;
					if (abs(character->_y - attackChar->_y) > dfATTACK2_RANGE_Y / 2) continue;
				}

				char newHp = attackChar->_hp - dfATTACK2_DAMAGE;
				if (newHp <= 0)
				{
					newHp = 0;
				}

				// �ǰݾ˸� �޽��� ������
				{
					character->_session->_packetBuffer.Clear();
					CreatePacket_Damage(character->_session->_packetBuffer, character->_session->_ID, attackChar->_session->_ID, attackChar->_hp);

					SC_Damage(character->_session, character->_session->_packetBuffer);
				}
				attackChar->_hp = newHp;

				return true;
			}
			break;
		}
		case dfPACKET_SC_ATTACK3:
		{
			for (list<Character*>::iterator iter = _sectorList[sY][sX].begin(); iter != _sectorList[sY][sX].end(); iter++)
			{
				Character* attackChar = (*iter);
				if (attackChar == character) continue;

				if (character->_dir == dfPACKET_MOVE_DIR_LL)
				{
					if (character->_x < attackChar->_x) continue;
					if ((character->_x - attackChar->_x) > dfATTACK3_RANGE_X) continue;
					if (abs(character->_y - attackChar->_y) > dfATTACK3_RANGE_Y / 2) continue;
				}
				else if (character->_dir == dfPACKET_MOVE_DIR_RR)
				{
					if (character->_x > attackChar->_x) continue;
					if ((attackChar->_x - character->_x) > dfATTACK3_RANGE_X) continue;
					if (abs(character->_y - attackChar->_y) > dfATTACK3_RANGE_Y / 2) continue;
				}
			
				char newHp = attackChar->_hp - dfATTACK3_DAMAGE;
				if (newHp <= 0)
				{
					newHp = 0;
				}

				// �ǰݾ˸� �޽��� ������
				{
					character->_session->_packetBuffer.Clear();
					CreatePacket_Damage(character->_session->_packetBuffer, character->_session->_ID, attackChar->_session->_ID, attackChar->_hp);

					SC_Damage(character->_session, character->_session->_packetBuffer);
				}
				attackChar->_hp = newHp;

				return true;
			}
			break;
		}
	}

	return false;
}

inline void Server::PushSectorList(const int x, const int y, Character* character)
{
	if (character == nullptr) return;
	if (x < 0 || x >= dfSECTOR_WIDTH_CNT || y < 0 || y >= dfSECTOR_HEIGHT_CNT) return;

	_sectorList[y][x].push_front(character);

	return;
}

inline void Server::Attack(Character* character, const SHORT rangeX, const SHORT rangeY, const CHAR type)
{
	// ���� ��ġ�� ���� Ȯ��
	{
		if (AttackSector(character->_sectorPos._x, character->_sectorPos._y, character, type)) return;
	}

	if (character->_dir == dfPACKET_MOVE_DIR_LL)
	{
		// ���� ���� �˻��ؾ� �ϴ��� ����Ȯ��
		if (character->_sectorPos._y != 0 && ((character->_sectorPos._y) * dfSECTOR_SIZE) > character->_y - rangeY / 2)
		{
			// ���� ���� �˻��ؾ��ϴ��� ���� Ȯ��
			if (character->_sectorPos._x != 0 && ((character->_sectorPos._x - 1) * dfSECTOR_SIZE) > character->_x - rangeX)
			{
				// ����, ����, �������� Ȯ��
				if (AttackSector(character->_sectorPos._x - 1, character->_sectorPos._y, character, type)) return;
				if (AttackSector(character->_sectorPos._x, character->_sectorPos._y - 1, character, type)) return;
				if (AttackSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character, type)) return;
			}
			else
			{
				// ���� Ȯ��
				if (AttackSector(character->_sectorPos._x, character->_sectorPos._y - 1, character, type)) return;
			}
		}
		else
		{
			// �Ʒ��� ���� �˻��ؾ� �ϴ��� ���� Ȯ��
			if (character->_sectorPos._y != dfSECTOR_HEIGHT_CNT - 1 && ((character->_sectorPos._y + 1) * dfSECTOR_SIZE) <= character->_y + rangeY / 2)
			{
				// ���� ���� �˻��ؾ��ϴ��� ���� Ȯ��
				if (character->_sectorPos._x != 0 && ((character->_sectorPos._x - 1) * dfSECTOR_SIZE) > character->_x - rangeX)
				{
					// ���� �Ʒ��� ���ʾƷ��� Ȯ��
					if (AttackSector(character->_sectorPos._x - 1, character->_sectorPos._y, character, type)) return;
					if (AttackSector(character->_sectorPos._x, character->_sectorPos._y + 1, character, type)) return;
					if (AttackSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character, type)) return;
				}
				else
				{
					// �Ʒ��� Ȯ��
					if (AttackSector(character->_sectorPos._x, character->_sectorPos._y + 1, character, type)) return;
				}
			}
			else
			{
				// ���� ���� �˻��ؾ��ϴ��� ���� Ȯ��
				if (character->_sectorPos._x != 0 && ((character->_sectorPos._x - 1) * dfSECTOR_SIZE) > character->_x - rangeX)
				{
					// ���� ���� Ȯ��
					if (AttackSector(character->_sectorPos._x - 1, character->_sectorPos._y, character, type)) return;
				}
			}
		}
	}
	else
	{
		// ���� ���� �˻��ؾ� �ϴ��� ����Ȯ��
		if (character->_sectorPos._y != 0 && ((character->_sectorPos._y) * dfSECTOR_SIZE) > character->_y - rangeY / 2)
		{
			// ������ ���� �˻��ؾ� �ϴ��� Ȯ��
			if (character->_sectorPos._x != dfSECTOR_WIDTH_CNT - 1 && ((character->_sectorPos._x + 1) * dfSECTOR_SIZE) <= character->_x + rangeX)
			{
				// ���� ������ ���ʿ����� Ȯ��
				if (AttackSector(character->_sectorPos._x, character->_sectorPos._y + 1, character, type)) return;
				if (AttackSector(character->_sectorPos._x + 1, character->_sectorPos._y, character, type)) return;
				if (AttackSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character, type)) return;
			}
			else
			{
				// ���� Ȯ��
				if (AttackSector(character->_sectorPos._x, character->_sectorPos._y + 1, character, type)) return;
			}
		}
		else
		{
			// �Ʒ��� ���� �˻��ؾ� �ϴ��� ���� Ȯ��
			if (character->_sectorPos._y != dfSECTOR_HEIGHT_CNT - 1 && ((character->_sectorPos._y + 1) * dfSECTOR_SIZE) <= character->_y + rangeY / 2)
			{
				// ������ ���� �˻��ؾ� �ϴ��� Ȯ��
				if (character->_sectorPos._x != dfSECTOR_WIDTH_CNT - 1 && ((character->_sectorPos._x + 1) * dfSECTOR_SIZE) <= character->_x + rangeX)
				{
					// �Ʒ��� ������ �Ʒ��ʿ����� Ȯ��
					if (AttackSector(character->_sectorPos._x, character->_sectorPos._y - 1, character, type)) return;
					if (AttackSector(character->_sectorPos._x + 1, character->_sectorPos._y, character, type)) return;
					if (AttackSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character, type)) return;
				}
				else
				{
					// �Ʒ��� Ȯ��
					if (AttackSector(character->_sectorPos._x, character->_sectorPos._y - 1, character, type)) return;
				}
			}
			else
			{
				// ������ ���� �˻��ؾ� �ϴ��� Ȯ��
				if (character->_sectorPos._x != dfSECTOR_WIDTH_CNT - 1 && ((character->_sectorPos._x + 1) * dfSECTOR_SIZE) <= character->_x + rangeX)
				{
					// ������ Ȯ��
					if (AttackSector(character->_sectorPos._x + 1, character->_sectorPos._y, character, type)) return;
				}
			}
		}
	}
}

inline void Server::EraseSectorList(const int x, const int y, Character* character)
{
	if (character == nullptr) return;
	if (x < 0 || x >= dfSECTOR_WIDTH_CNT || y < 0 || y >= dfSECTOR_HEIGHT_CNT) return;

	for (list<Character*>::iterator iter = _sectorList[y][x].begin(); iter != _sectorList[y][x].end(); iter++)
	{
		if ((*iter) == character)
		{
			_sectorList[y][x].erase(iter);
			return;
		}
	}

	return;
}

inline void Server::SetAroundSector(const int x, const int y, Character* character)
{	
	// �����ڸ� ���Ϳ� ��ġ�ߴ��� �Ǵ�
	
	if (x == 0 && y == 0)
	{
		character->_sectorAround._cnt = 4;
		character->_sectorAround._around[0]._x = 0;
		character->_sectorAround._around[0]._y = 0;
		character->_sectorAround._around[1]._x = 0;
		character->_sectorAround._around[1]._y = 1;
		character->_sectorAround._around[2]._x = 1;
		character->_sectorAround._around[2]._y = 0;
		character->_sectorAround._around[3]._x = 1;
		character->_sectorAround._around[3]._y = 1;
	}
	else if (x == 0 && y == dfSECTOR_HEIGHT_CNT - 1)
	{
		character->_sectorAround._cnt = 4;
		character->_sectorAround._around[0]._x = 0;
		character->_sectorAround._around[0]._y = dfSECTOR_HEIGHT_CNT - 1;
		character->_sectorAround._around[1]._x = 0;
		character->_sectorAround._around[1]._y = dfSECTOR_HEIGHT_CNT - 2;
		character->_sectorAround._around[2]._x = 1;
		character->_sectorAround._around[2]._y = dfSECTOR_HEIGHT_CNT - 1;
		character->_sectorAround._around[3]._x = 1;
		character->_sectorAround._around[3]._y = dfSECTOR_HEIGHT_CNT - 2;
	}
	else if (x == dfSECTOR_WIDTH_CNT - 1 && y == 0)
	{
		character->_sectorAround._cnt = 4;
		character->_sectorAround._around[0]._x = dfSECTOR_WIDTH_CNT - 1;
		character->_sectorAround._around[0]._y = 0;
		character->_sectorAround._around[1]._x = dfSECTOR_WIDTH_CNT - 1;
		character->_sectorAround._around[1]._y = 1;
		character->_sectorAround._around[2]._x = dfSECTOR_WIDTH_CNT - 2;
		character->_sectorAround._around[2]._y = 0;
		character->_sectorAround._around[3]._x = dfSECTOR_WIDTH_CNT - 2;
		character->_sectorAround._around[3]._y = 1;
	}
	else if (x == dfSECTOR_WIDTH_CNT - 1 && dfSECTOR_HEIGHT_CNT - 1)
	{
		character->_sectorAround._cnt = 4;
		character->_sectorAround._around[0]._x = dfSECTOR_WIDTH_CNT - 1;
		character->_sectorAround._around[0]._y = dfSECTOR_HEIGHT_CNT - 1;
		character->_sectorAround._around[1]._x = dfSECTOR_WIDTH_CNT - 1;
		character->_sectorAround._around[1]._y = dfSECTOR_HEIGHT_CNT - 2;
		character->_sectorAround._around[2]._x = dfSECTOR_WIDTH_CNT - 2;
		character->_sectorAround._around[2]._y = dfSECTOR_HEIGHT_CNT - 1;
		character->_sectorAround._around[3]._x = dfSECTOR_WIDTH_CNT - 2;
		character->_sectorAround._around[3]._y = dfSECTOR_HEIGHT_CNT - 2;
	}
	else if (x == 0)
	{
		character->_sectorAround._cnt = 6;
		character->_sectorAround._around[0]._x = 0;
		character->_sectorAround._around[0]._y = y;
		character->_sectorAround._around[1]._x = 0;
		character->_sectorAround._around[1]._y = y + 1;
		character->_sectorAround._around[2]._x = 0;
		character->_sectorAround._around[2]._y = y - 1;
		character->_sectorAround._around[3]._x = 1;
		character->_sectorAround._around[3]._y = y;
		character->_sectorAround._around[4]._x = 1;
		character->_sectorAround._around[4]._y = y + 1;
		character->_sectorAround._around[5]._x = 1;
		character->_sectorAround._around[5]._y = y - 1;
	}
	else if (y == 0)
	{
		character->_sectorAround._cnt = 6;
		character->_sectorAround._around[0]._x = x;
		character->_sectorAround._around[0]._y = 0;
		character->_sectorAround._around[1]._x = x + 1;
		character->_sectorAround._around[1]._y = 0;
		character->_sectorAround._around[2]._x = x - 1;
		character->_sectorAround._around[2]._y = 0;
		character->_sectorAround._around[3]._x = x;
		character->_sectorAround._around[3]._y = 1;
		character->_sectorAround._around[4]._x = x + 1;
		character->_sectorAround._around[4]._y = 1;
		character->_sectorAround._around[5]._x = x - 1;
		character->_sectorAround._around[5]._y = 1; 
	}
	else if (x == dfSECTOR_WIDTH_CNT - 1)
	{
		character->_sectorAround._cnt = 6;
		character->_sectorAround._around[0]._x = dfSECTOR_WIDTH_CNT - 1;
		character->_sectorAround._around[0]._y = y;
		character->_sectorAround._around[1]._x = dfSECTOR_WIDTH_CNT - 1;
		character->_sectorAround._around[1]._y = y + 1;
		character->_sectorAround._around[2]._x = dfSECTOR_WIDTH_CNT - 1;
		character->_sectorAround._around[2]._y = y - 1;
		character->_sectorAround._around[3]._x = dfSECTOR_WIDTH_CNT - 2;
		character->_sectorAround._around[3]._y = y;
		character->_sectorAround._around[4]._x = dfSECTOR_WIDTH_CNT - 2;
		character->_sectorAround._around[4]._y = y + 1;
		character->_sectorAround._around[5]._x = dfSECTOR_WIDTH_CNT - 2;
		character->_sectorAround._around[5]._y = y - 1;
	}
	else if (y == dfSECTOR_HEIGHT_CNT - 1)
	{
		character->_sectorAround._cnt = 6;
		character->_sectorAround._around[0]._x = x;
		character->_sectorAround._around[0]._y = dfSECTOR_HEIGHT_CNT - 1;
		character->_sectorAround._around[1]._x = x + 1;
		character->_sectorAround._around[1]._y = dfSECTOR_HEIGHT_CNT - 1;
		character->_sectorAround._around[2]._x = x - 1;
		character->_sectorAround._around[2]._y = dfSECTOR_HEIGHT_CNT - 1;
		character->_sectorAround._around[3]._x = x;
		character->_sectorAround._around[3]._y = dfSECTOR_HEIGHT_CNT - 2;
		character->_sectorAround._around[4]._x = x + 1;
		character->_sectorAround._around[4]._y = dfSECTOR_HEIGHT_CNT - 2;
		character->_sectorAround._around[5]._x = x - 1;
		character->_sectorAround._around[5]._y = dfSECTOR_HEIGHT_CNT - 2;
	}
	else                                                    // KeyPad�� ��ġ
	{
		character->_sectorAround._cnt = 9;
		character->_sectorAround._around[0]._x = x;			// 5
		character->_sectorAround._around[0]._y = y;
		character->_sectorAround._around[1]._x = x;			// 8
		character->_sectorAround._around[1]._y = y - 1;		
		character->_sectorAround._around[2]._x = x + 1;		// 3
		character->_sectorAround._around[2]._y = y + 1;
		character->_sectorAround._around[3]._x = x + 1;		// 6
		character->_sectorAround._around[3]._y = y;
		character->_sectorAround._around[4]._x = x + 1;		// 9
		character->_sectorAround._around[4]._y = y - 1;
		character->_sectorAround._around[5]._x = x - 1;		// 1
		character->_sectorAround._around[5]._y = y + 1;
		character->_sectorAround._around[6]._x = x - 1;		// 4
		character->_sectorAround._around[6]._y = y;
		character->_sectorAround._around[7]._x = x - 1;		// 7
		character->_sectorAround._around[7]._y = y - 1;
		character->_sectorAround._around[8]._x = x;			// 2
		character->_sectorAround._around[8]._y = y + 1;
	}
}

inline void Server::NotifyCharacterToSector(const int sX, const int sY, Character* character, const bool createFlag)
{
	// Sector Border üũ
	if (sX < 0 || sX >= dfSECTOR_WIDTH_CNT || sY < 0 || sY >= dfSECTOR_HEIGHT_CNT) return;

	if (createFlag)
	{
		list<Character*>::iterator iter = _sectorList[sY][sX].begin();
		for (; iter != _sectorList[sY][sX].end(); iter++)
		{
			Character* notiChar = (*iter);
			if (notiChar == character) continue;

			// ���� �޽���
			notiChar->_session->_packetBuffer.Clear();
			CreatePacket_CreateOtherCharacter(notiChar->_session->_packetBuffer, character->_session->_ID, character->_hp, character->_x, character->_y, character->_y);

			SC_CreateOtherCharacter(notiChar->_session, notiChar->_session->_packetBuffer);
		}
	}
	else
	{
		list<Character*>::iterator iter = _sectorList[sY][sX].begin();
		for (; iter != _sectorList[sY][sX].end(); iter++)
		{
			Character* notiChar = (*iter);
			if (notiChar == character) continue;

			notiChar->_session->_packetBuffer.Clear();
			CreatePacket_DeleteCharacter(notiChar->_session->_packetBuffer, character->_session->_ID);

			SC_SendUnicast(notiChar->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_DELETE_CHARACTER), notiChar->_session);
		}
	}
}

inline void Server::CreateSectorToCharacter(const int sX, const int sY, Character* character)
{
	if (sX < 0 || sX >= dfSECTOR_WIDTH_CNT || sY < 0 || sY >= dfSECTOR_HEIGHT_CNT) return;

	list<Character*>::iterator iter = _sectorList[sY][sX].begin();
	for (; iter != _sectorList[sY][sX].end(); iter++)
	{
		Character* newChar = (*iter);
		if (newChar == character) continue;
		
		// ���� �޽���
		character->_session->_packetBuffer.Clear();
		CreatePacket_CreateOtherCharacter(character->_session->_packetBuffer, newChar->_session->_ID, newChar->_hp, newChar->_x, newChar->_y, newChar->_dir);

		SC_CreateOtherCharacter(character->_session, character->_session->_packetBuffer);

		if (newChar->_moveFlag == true)
		{
			character->_session->_packetBuffer.Clear();
			CreatePacket_MoveStartCharacter(character->_session->_packetBuffer, newChar->_session->_ID, newChar->_moveDir, newChar->_x, newChar->_y);

			SC_SendUnicast((char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), character->_session);
		}
	}
}

inline void Server::DeleteCharacterToSector(const int sX, const int sY, Character* character)
{
	// �ʿ� ���� �� ����. ���
	return;

	if (sX < 0 || sX >= dfSECTOR_WIDTH_CNT || sY < 0 || sY >= dfSECTOR_HEIGHT_CNT) return;

	// ���� �޽���
	character->_session->_packetBuffer.Clear();
	CreatePacket_DeleteCharacter(character->_session->_packetBuffer, character->_session->_ID);

	SC_SendSectorOne(sX, sY, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_DELETE_CHARACTER), nullptr);
}

inline void Server::DeleteSectorToCharacter(const int sX, const int sY, Character* character)
{
	if (sX < 0 || sX >= dfSECTOR_WIDTH_CNT || sY < 0 || sY >= dfSECTOR_HEIGHT_CNT) return;

	list<Character*>::iterator iter = _sectorList[sY][sX].begin();
	for (; iter != _sectorList[sY][sX].end(); iter++)
	{
		if ((*iter) == character) continue;

		character->_session->_packetBuffer.Clear();
		CreatePacket_DeleteCharacter(character->_session->_packetBuffer, (*iter)->_session->_ID);

		SC_SendUnicast((char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_DELETE_CHARACTER), character->_session);
	}
}

inline void Server::InvaidSectorLL(Character* character)
{
	// ���� ���Ϳ��� ���� - �ش� �÷��̾ ������ ���� �ֺ� ���Ϳ� �˸���
	EraseSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character, false);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y, character, false);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character, false);

	DeleteCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character);
	DeleteCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y, character);
	DeleteCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);

	DeleteSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);
	DeleteSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y, character);
	DeleteSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character);

	// ���ο� ���Ϳ� �߰� - �ش� �÷��̾ ������ �� ���Ϳ� �˸���
	character->_sectorPos._x -= 1;
	PushSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y, character, true);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character, true);

	// AroundSector ���� + ���� ���� �޽���
	{
		SetAroundSector(character->_sectorPos._x, character->_sectorPos._y, character);

		character->_session->_packetBuffer.Clear();
		CreatePacket_MoveStartCharacter(character->_session->_packetBuffer, character->_session->_ID, character->_moveDir, character->_x, character->_y);

		SC_SendSectorOne(character->_sectorPos._x - 1, character->_sectorPos._y - 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x - 1, character->_sectorPos._y, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x - 1, character->_sectorPos._y + 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
	}

	// ���� ���� ���Ϳ� �ִ� ĳ���͵鿡 ���� ó��
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character);
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y, character);
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character);
}

inline void Server::InvaidSectorLU(Character* character)
{
	// ���� ���Ϳ��� ���� - �ش� �÷��̾ ������ ���� �ֺ� ���Ϳ� �˸���
	EraseSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character, false);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y, character, false);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character, false);
	NotifyCharacterToSector(character->_sectorPos._x, character->_sectorPos._y + 1, character, false);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character, false);

	DeleteCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character);
	DeleteCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y, character);
	DeleteCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);
	DeleteCharacterToSector(character->_sectorPos._x, character->_sectorPos._y + 1, character);
	DeleteCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character);

	DeleteSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);
	DeleteSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y, character);
	DeleteSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character);
	DeleteSectorToCharacter(character->_sectorPos._x, character->_sectorPos._y + 1, character);
	DeleteSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character);

	// ���ο� ���Ϳ� �߰� - �ش� �÷��̾ ������ �� ���Ϳ� �˸���
	character->_sectorPos._y -= 1;
	character->_sectorPos._x -= 1;
	PushSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y, character, true);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character, true);


	// AroundSector ���� + ���� ���� �޽���
	{
		SetAroundSector(character->_sectorPos._x, character->_sectorPos._y, character);

		character->_session->_packetBuffer.Clear();
		CreatePacket_MoveStartCharacter(character->_session->_packetBuffer, character->_session->_ID, character->_moveDir, character->_x, character->_y);

		SC_SendSectorOne(character->_sectorPos._x - 1, character->_sectorPos._y + 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x - 1, character->_sectorPos._y, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x - 1, character->_sectorPos._y - 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x, character->_sectorPos._y - 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x + 1, character->_sectorPos._y - 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
	}

	// ���� ���� ���Ϳ� �ִ� ĳ���͵鿡 ���� ó��
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character);
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y, character);
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character);
	CreateSectorToCharacter(character->_sectorPos._x, character->_sectorPos._y - 1, character);
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character);
}

inline void Server::InvaidSectorLD(Character* character)
{
	// ���� ���Ϳ��� ���� - �ش� �÷��̾ ������ ���� �ֺ� ���Ϳ� �˸���
	EraseSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character, false);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y, character, false);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character, false);
	NotifyCharacterToSector(character->_sectorPos._x, character->_sectorPos._y - 1, character, false);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character, false);

	DeleteCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character);
	DeleteCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y, character);
	DeleteCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);
	DeleteCharacterToSector(character->_sectorPos._x, character->_sectorPos._y - 1, character);
	DeleteCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character);

	DeleteSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);
	DeleteSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y, character);
	DeleteSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character);
	DeleteSectorToCharacter(character->_sectorPos._x, character->_sectorPos._y - 1, character);
	DeleteSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character);

	// ���ο� ���Ϳ� �߰� - �ش� �÷��̾ ������ �� ���Ϳ� �˸���
	character->_sectorPos._y -= 1;
	character->_sectorPos._x -= 1;
	PushSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y, character, true);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x, character->_sectorPos._y + 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character, true);


	// AroundSector ���� + ���� ���� �޽���
	{
		SetAroundSector(character->_sectorPos._x, character->_sectorPos._y, character);

		character->_session->_packetBuffer.Clear();
		CreatePacket_MoveStartCharacter(character->_session->_packetBuffer, character->_session->_ID, character->_moveDir, character->_x, character->_y);

		SC_SendSectorOne(character->_sectorPos._x - 1, character->_sectorPos._y + 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x - 1, character->_sectorPos._y, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x - 1, character->_sectorPos._y - 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x, character->_sectorPos._y + 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x + 1, character->_sectorPos._y + 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
	}

	// ���� ���� ���Ϳ� �ִ� ĳ���͵鿡 ���� ó��
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character);
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y, character);
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character);
	CreateSectorToCharacter(character->_sectorPos._x, character->_sectorPos._y + 1, character);
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);
}

inline void Server::InvaidSectorRR(Character* character)
{
	// ���� ���Ϳ��� ���� - �ش� �÷��̾ ������ ���� �ֺ� ���Ϳ� �˸���
	EraseSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character, false);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y, character, false);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character, false);

	DeleteCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character);
	DeleteCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y, character);
	DeleteCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character);

	DeleteSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character);
	DeleteSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y, character);
	DeleteSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character);

	// ���ο� ���Ϳ� �߰� - �ش� �÷��̾ ������ �� ���Ϳ� �˸���
	character->_sectorPos._x += 1;
	PushSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character, true);

	// AroundSector ���� + ���� ���� �޽���
	{
		SetAroundSector(character->_sectorPos._x, character->_sectorPos._y, character);

		character->_session->_packetBuffer.Clear();
		CreatePacket_MoveStartCharacter(character->_session->_packetBuffer, character->_session->_ID, character->_moveDir, character->_x, character->_y);

		SC_SendSectorOne(character->_sectorPos._x + 1, character->_sectorPos._y - 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x + 1, character->_sectorPos._y, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x + 1, character->_sectorPos._y + 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
	}

	// ���� ���� ���Ϳ� �ִ� ĳ���͵鿡 ���� ó��
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character);
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y, character);
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);
}

inline void Server::InvaidSectorRU(Character* character)
{
	// ���� ���Ϳ��� ���� - �ش� �÷��̾ ������ ���� �ֺ� ���Ϳ� �˸���
	EraseSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character, false);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y, character, false);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character, false);
	NotifyCharacterToSector(character->_sectorPos._x, character->_sectorPos._y + 1, character, false);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character, false);

	DeleteCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character);
	DeleteCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y, character);
	DeleteCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character);
	DeleteCharacterToSector(character->_sectorPos._x, character->_sectorPos._y + 1, character);
	DeleteCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);

	DeleteSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character);
	DeleteSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y, character);
	DeleteSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character);
	DeleteSectorToCharacter(character->_sectorPos._x, character->_sectorPos._y + 1, character);
	DeleteSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);

	// ���ο� ���Ϳ� �߰� - �ش� �÷��̾ ������ �� ���Ϳ� �˸���
	character->_sectorPos._y -= 1;
	character->_sectorPos._x += 1;
	PushSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character, true);


	// AroundSector ���� + ���� ���� �޽���
	{
		SetAroundSector(character->_sectorPos._x, character->_sectorPos._y, character);

		character->_session->_packetBuffer.Clear();
		CreatePacket_MoveStartCharacter(character->_session->_packetBuffer, character->_session->_ID, character->_moveDir, character->_x, character->_y);

		SC_SendSectorOne(character->_sectorPos._x + 1, character->_sectorPos._y + 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x + 1, character->_sectorPos._y, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x + 1, character->_sectorPos._y - 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x, character->_sectorPos._y - 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x - 1, character->_sectorPos._y - 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
	}

	// ���� ���� ���Ϳ� �ִ� ĳ���͵鿡 ���� ó��
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y, character);
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character);
	CreateSectorToCharacter(character->_sectorPos._x, character->_sectorPos._y - 1, character);
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character);
}

inline void Server::InvaidSectorRD(Character* character)
{
	// ���� ���Ϳ��� ���� - �ش� �÷��̾ ������ ���� �ֺ� ���Ϳ� �˸���
	EraseSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character, false);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y, character, false);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character, false);
	NotifyCharacterToSector(character->_sectorPos._x, character->_sectorPos._y - 1, character, false);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character, false);

	DeleteCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character);
	DeleteCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y, character);
	DeleteCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character);
	DeleteCharacterToSector(character->_sectorPos._x, character->_sectorPos._y - 1, character);
	DeleteCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character);

	DeleteSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character);
	DeleteSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y, character);
	DeleteSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character);
	DeleteSectorToCharacter(character->_sectorPos._x, character->_sectorPos._y - 1, character);
	DeleteSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character);

	// ���ο� ���Ϳ� �߰� - �ش� �÷��̾ ������ �� ���Ϳ� �˸���
	character->_sectorPos._y += 1;
	character->_sectorPos._x += 1;
	PushSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x, character->_sectorPos._y + 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character, true);


	// AroundSector ���� + ���� ���� �޽���
	{
		SetAroundSector(character->_sectorPos._x, character->_sectorPos._y, character);

		character->_session->_packetBuffer.Clear();
		CreatePacket_MoveStartCharacter(character->_session->_packetBuffer, character->_session->_ID, character->_moveDir, character->_x, character->_y);

		SC_SendSectorOne(character->_sectorPos._x + 1, character->_sectorPos._y + 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x + 1, character->_sectorPos._y, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x + 1, character->_sectorPos._y - 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x, character->_sectorPos._y + 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x - 1, character->_sectorPos._y + 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
	}

	// ���� ���� ���Ϳ� �ִ� ĳ���͵鿡 ���� ó��
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y, character);
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character);
	CreateSectorToCharacter(character->_sectorPos._x, character->_sectorPos._y + 1, character);
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character);
}

inline void Server::InvaidSectorDD(Character* character)
{
	// ���� ���Ϳ��� ���� - �ش� �÷��̾ ������ ���� �ֺ� ���Ϳ� �˸���
	EraseSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character, false);
	NotifyCharacterToSector(character->_sectorPos._x, character->_sectorPos._y - 1, character, false);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character, false);

	DeleteCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character);
	DeleteCharacterToSector(character->_sectorPos._x, character->_sectorPos._y - 1, character);
	DeleteCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y -1, character);

	DeleteSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character);
	DeleteSectorToCharacter(character->_sectorPos._x, character->_sectorPos._y - 1, character);
	DeleteSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character);

	// ���ο� ���Ϳ� �߰� - �ش� �÷��̾ ������ �� ���Ϳ� �˸���
	character->_sectorPos._y += 1;
	PushSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x, character->_sectorPos._y + 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character, true);

	// AroundSector ���� + ���� ���� �޽���
	{
		SetAroundSector(character->_sectorPos._x, character->_sectorPos._y, character);

		character->_session->_packetBuffer.Clear();
		CreatePacket_MoveStartCharacter(character->_session->_packetBuffer, character->_session->_ID, character->_moveDir, character->_x, character->_y);

		SC_SendSectorOne(character->_sectorPos._x - 1, character->_sectorPos._y + 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x, character->_sectorPos._y + 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x + 1, character->_sectorPos._y + 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
	}

	// ���� ���� ���Ϳ� �ִ� ĳ���͵鿡 ���� ó��
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character);
	CreateSectorToCharacter(character->_sectorPos._x, character->_sectorPos._y + 1, character);
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);
}

inline void Server::InvaidSectorUU(Character* character)
{
	// ���� ���Ϳ��� ���� - �ش� �÷��̾ ������ ���� �ֺ� ���Ϳ� �˸���
	EraseSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character, false);
	NotifyCharacterToSector(character->_sectorPos._x, character->_sectorPos._y + 1, character, false);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character, false);

	DeleteCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);
	DeleteCharacterToSector(character->_sectorPos._x, character->_sectorPos._y + 1, character);
	DeleteCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character);

	DeleteSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);
	DeleteSectorToCharacter(character->_sectorPos._x, character->_sectorPos._y + 1, character);
	DeleteSectorToCharacter(character->_sectorPos._x -1 , character->_sectorPos._y + 1, character);

	// ���ο� ���Ϳ� �߰� - �ش� �÷��̾ ������ �� ���Ϳ� �˸���
	character->_sectorPos._y -= 1;
	PushSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character, true);

	// AroundSector ���� + ���� ���� �޽���
	{
		SetAroundSector(character->_sectorPos._x, character->_sectorPos._y, character);

		character->_session->_packetBuffer.Clear();
		CreatePacket_MoveStartCharacter(character->_session->_packetBuffer, character->_session->_ID, character->_moveDir, character->_x, character->_y);

		SC_SendSectorOne(character->_sectorPos._x - 1, character->_sectorPos._y - 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x, character->_sectorPos._y - 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x + 1, character->_sectorPos._y - 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
	}

	// ���� ���� ���Ϳ� �ִ� ĳ���͵鿡 ���� ó��
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character);
	CreateSectorToCharacter(character->_sectorPos._x, character->_sectorPos._y - 1, character);
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character);
}

inline bool Server::SkipFrame()
{
	static unsigned int oldTick = timeGetTime();
	if (timeGetTime() - oldTick < (1000 / dfFPS))
	{
		return true;
	}

	oldTick += (1000 / dfFPS);
	return false;
}

inline void Server::CreatePacket_CreateOtherCharacter(CPacket& packet, const DWORD ID, const BYTE hp, const SHORT x, const SHORT y, const BYTE dir)
{
	packet << (BYTE)0x89;
	packet << (BYTE)sizeof(stPACKET_SC_CREATE_OTHER_CHARACTER);
	packet << (BYTE)dfPACKET_SC_CREATE_OTHER_CHARACTER;

	packet << ID;
	packet << dir;
	packet << x;
	packet << y;
	packet << hp;
}

inline void Server::CreatePacket_CreateMyCharacter(CPacket& packet, const DWORD ID, const BYTE hp, const SHORT x, const SHORT y, const BYTE dir)
{
	packet << (BYTE)0x89;
	packet << (BYTE)sizeof(stPACKET_SC_CREATE_MY_CHARACTER);
	packet << (BYTE)dfPACKET_SC_CREATE_MY_CHARACTER;

	packet << ID;
	packet << dir;
	packet << x;
	packet << y;
	packet << hp;
}

inline void Server::CreatePacket_MoveStartCharacter(CPacket& packet, const DWORD ID, const BYTE dir, const SHORT x, const SHORT y)
{
	packet << (BYTE)0x89;
	packet << (BYTE)sizeof(stPACKET_SC_MOVE_START);
	packet << (BYTE)dfPACKET_SC_MOVE_START;

	packet << ID;
	packet << dir;
	packet << x;
	packet << y;
}

inline void Server::CreatePacket_MoveStopCharacter(CPacket& packet, const DWORD ID, const BYTE dir, const SHORT x, const SHORT y)
{
	packet << (BYTE)0x89;
	packet << (BYTE)sizeof(stPACKET_SC_MOVE_STOP);
	packet << (BYTE)dfPACKET_SC_MOVE_STOP;

	packet << ID;
	packet << dir;
	packet << x;
	packet << y;
}

inline void Server::CreatePacket_DeleteCharacter(CPacket& packet, const  DWORD ID)
{
	packet << (BYTE)0x89;
	packet << (BYTE)sizeof(stPACKET_SC_DELETE_CHARACTER);
	packet << (BYTE)dfPACKET_SC_DELETE_CHARACTER;

	packet << ID;
}

inline void Server::CreatePacket_ECHO(CPacket& packet, const DWORD time)
{
	packet << (BYTE)0x89;
	packet << (BYTE)sizeof(stPACKET_SC_ECHO);
	packet << (BYTE)dfPACKET_SC_ECHO;

	packet << time;
}

inline void Server::CreatePacket_SYNC(CPacket& packet, const DWORD ID, const SHORT x, const SHORT y)
{
	packet << (BYTE)0x89;
	packet << (BYTE)sizeof(stPACKET_SC_SYNC);
	packet << (BYTE)dfPACKET_SC_SYNC;

	packet << ID;
	packet << x;
	packet << y;
}

inline void Server::CreatePacket_Attack1(CPacket& packet, const DWORD ID, const BYTE dir, const SHORT x, const SHORT y)
{
	packet << (BYTE)0x89;
	packet << (BYTE)sizeof(stPACKET_SC_ATTACK1);
	packet << (BYTE)dfPACKET_SC_ATTACK1;

	packet << ID;
	packet << dir;
	packet << x;
	packet << y;
}

inline void Server::CreatePacket_Attack2(CPacket& packet, const DWORD ID, const BYTE dir, const SHORT x, const SHORT y)
{
	packet << (BYTE)0x89;
	packet << (BYTE)sizeof(stPACKET_SC_ATTACK2);
	packet << (BYTE)dfPACKET_SC_ATTACK2;

	packet << ID;
	packet << dir;
	packet << x;
	packet << y;
}

inline void Server::CreatePacket_Attack3(CPacket& packet, const DWORD ID, const BYTE dir, const SHORT x, const SHORT y)
{
	packet << (BYTE)0x89;
	packet << (BYTE)sizeof(stPACKET_SC_ATTACK3);
	packet << (BYTE)dfPACKET_SC_ATTACK3;

	packet << ID;
	packet << dir;
	packet << x;
	packet << y;
}

inline void Server::CreatePacket_Damage(CPacket& packet, const DWORD attackID, const DWORD damageID, const BYTE damageHP)
{
	packet << (BYTE)0x89;
	packet << (BYTE)sizeof(stPACKET_SC_DAMAGE);
	packet << (BYTE)dfPACKET_SC_DAMAGE;

	packet << attackID;
	packet << damageID;
	packet << damageHP;
}

inline bool Server::HandleCSPacket(Session* session)
{
	PRO_BEGIN("HandleCSPacket");

	for (;;)
	{
		int useSize = session->_recvRingBuffer.GetUseSize();
		if (useSize <= sizeof(stPacketHeader))
		{
			break;
		}

		stPacketHeader header;
		session->_recvRingBuffer.Peek((char*)&header, sizeof(header));

		if (header.byCode != dfPACKET_CODE)
		{
			Log(L"Invalid packet code error.", 0, dfLOG_LEVEL_DEBUG);
			__LOG(L"Invalid packet code error. code : " << header.byCode , dfLOG_LEVEL_DEBUG);

			// TEST
			{
				wcout << L"code : " << header.byCode << " size : " << header.bySize << " type : " << header.byType << endl;
				
				int len = min(session->_recvRingBuffer.GetUseSize(), header.bySize);
				char arr[100];
				memcpy(arr, session->_recvRingBuffer.GetFrontBufferPtr(), len);
				for (int i = 0; i < len; i++)
				{
					wcout << arr[i] << L" : " <<  static_cast<int>(arr[i]) << endl;
				}
			}
			Log(L"Packet Code Error", 0, dfLOG_LEVEL_ERROR);
			ReserveDeleteSession(session);

			// return false;
			return true;
		}

		// ��Ŷ ��� ������
		if (session->_recvRingBuffer.GetUseSize() < sizeof(header) + header.bySize) break;
		session->_recvRingBuffer.MoveFront(sizeof(header));
		
		session->_packetBuffer.Clear();
		session->_recvRingBuffer.Dequeue((char*)session->_packetBuffer.GetBufferPtr(), header.bySize);
		session->_packetBuffer.MoveWritePos(header.bySize);

		switch (header.byType)
		{
			case dfPACKET_CS_MOVE_START:
			{
				CS_MoveStartCharacter(session, session->_packetBuffer);
				break;
			}
			case dfPACKET_CS_MOVE_STOP:
			{
				CS_MoveStopCharacter(session, session->_packetBuffer);
				break;
			}
			case dfPACKET_CS_ATTACK1:
			{
				CS_Attack1(session, session->_packetBuffer);
				break;
			}
			case dfPACKET_CS_ATTACK2:
			{
				CS_Attack2(session, session->_packetBuffer);
				break;
			}
			case dfPACKET_CS_ATTACK3:
			{
				CS_Attack3(session, session->_packetBuffer);
				break;
			}
			case dfPACKET_CS_ECHO:
			{
				CS_ECHO(session, session->_packetBuffer);
				break;
			}
			default:
			{
				// �߸��� ��Ŷ�� ������ ��� ���� ����
				Log(L"Invalid packet error.", 0, dfLOG_LEVEL_ERROR);
				__LOG(L"Invalid packet error. sessionID : " << session->_ID << " packetType : " << header.byType , dfLOG_LEVEL_ERROR);
				ReserveDeleteSession(session);
				break;
			}
		}
	}

	PRO_END("HandleCSPacket");

	return true;
}

inline void Server::CS_MoveStartCharacter(Session* session, CPacket& packet)
{
	// ĳ���� moveFlag ����
	BYTE moveDir;
	SHORT x;
	SHORT y;

	packet >> moveDir >> x >> y;

	if (moveDir == 0x89)
	{
		return;
	}

	Character* character = _characterMap[session->_ID];
	character->_moveFlag = true;
	character->_moveDir = moveDir;

	if (CheckMarginCoordinate(character, x, y) == false)
	{

		// TODO : Sync
		packet.Clear();
		CreatePacket_SYNC(packet, session->_ID, character->_x, character->_y);

		SC_SYNC(session, packet);
		return;
	}
	else
	{
		character->_x = x;
		character->_y = y;
	}

	// �޼��� ����
	{
		packet.Clear();
		CreatePacket_MoveStartCharacter(packet, session->_ID, character->_moveDir, character->_x, character->_y);

		// �޼��� ó��
		SC_MoveStartCharacter(session, packet);
	}
}

inline void Server::CS_MoveStopCharacter(Session* session, CPacket& packet)
{
	BYTE dir;
	SHORT x;
	SHORT y;

	packet >> dir >> x >> y;

	// ĳ���� moveFlag ����
	Character* character = _characterMap[session->_ID];
	character->_moveFlag = false;
	character->_dir = dir;

	if (CheckMarginCoordinate(character, x, y) == false)
	{
		packet.Clear();
		CreatePacket_SYNC(packet, session->_ID, character->_x, character->_y);

		SC_SYNC(session, packet);
		return;
	}
	else
	{
		character->_x = x;
		character->_y = y;
	}

	// �޼��� ����
	{
		packet.Clear();
		CreatePacket_MoveStopCharacter(packet, session->_ID, character->_dir, character->_x, character->_y);

		// �޼��� ó��
		SC_MoveStopCharacter(session, packet);
	}
}

inline void Server::CS_Attack1(Session* session, CPacket& packet)
{
	Character* character = _characterMap[session->_ID];

	// ����ó�� �ֺ� ���Ϳ� �˸�
	{
		packet.Clear();
		CreatePacket_Attack1(packet, session->_ID, character->_dir, character->_x, character->_y);

		SC_Attack1(session, packet);
	}

	// �������� Ž�� (���� �Ѹ� ������ �������� ����)
	{
		Attack(character, dfATTACK1_RANGE_X, dfATTACK1_RANGE_Y, dfPACKET_SC_ATTACK1);
	}
}

inline void Server::CS_Attack2(Session* session, CPacket& packet)
{
	Character* character = _characterMap[session->_ID];

	// ����ó�� �ֺ� ���Ϳ� �˸�
	{
		packet.Clear();
		CreatePacket_Attack2(packet, session->_ID, character->_dir, character->_x, character->_y);

		SC_Attack2(session, packet);
	}

	// �������� Ž�� (���� �Ѹ� ������ �������� ����)
	{
		Attack(character, dfATTACK2_RANGE_X, dfATTACK2_RANGE_Y, dfPACKET_SC_ATTACK2);
	}
}

inline void Server::CS_Attack3(Session* session, CPacket& packet)
{
	Character* character = _characterMap[session->_ID];

	// ����ó�� �ֺ� ���Ϳ� �˸�
	{
		packet.Clear();
		CreatePacket_Attack3(packet, session->_ID, character->_dir, character->_x, character->_y);

		SC_Attack3(session, packet);
	}

	// �������� Ž�� (���� �Ѹ� ������ �������� ����)
	{
		Attack(character, dfATTACK3_RANGE_X, dfATTACK3_RANGE_Y, dfPACKET_SC_ATTACK3);
	}
}

inline void Server::CS_ECHO(Session* session, CPacket& packet)
{
	DWORD time;
	packet >> time;

	packet.Clear();
	CreatePacket_ECHO(packet, time);

	session->cnt += 1;
	SC_ECHO(session, packet);
}

inline void Server::SC_CreateMyCharacter(Session* session, CPacket& packet)
{
	//__LOG(L"Unicast create my character", dfLOG_LEVEL_SYSTEM);
	
	SC_SendUnicast((char*)packet.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_CREATE_MY_CHARACTER), session);
}

inline void Server::SC_CreateOtherCharacter(Session* session, CPacket& packet)
{
	//__LOG(L"Unicast create other character", dfLOG_LEVEL_SYSTEM);

	SC_SendUnicast((char*)packet.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_CREATE_OTHER_CHARACTER), session);
}

inline void Server::SC_MoveStartCharacter(Session* session, CPacket& packet)
{
	//__LOG(L"Send AroundSector MoveStart character", dfLOG_LEVEL_SYSTEM);

	SC_SendAroundSector(session, (char*)packet.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), false);
}

inline void Server::SC_MoveStopCharacter(Session* session, CPacket& packet)
{
	//__LOG(L"Send AroundSector MoveStop character", dfLOG_LEVEL_SYSTEM);

	SC_SendAroundSector(session, (char*)packet.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_STOP), false);
}

inline void Server::SC_DeleteCharacter(Session* session, CPacket& packet)
{
	//__LOG(L"Send AroundSector delete character", dfLOG_LEVEL_SYSTEM);

	SC_SendAroundSector(session, (char*)packet.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_DELETE_CHARACTER), false);
}

inline void Server::SC_Attack1(Session* session, CPacket& packet)
{
	//__LOG(L"Send Sector Around Attack1 character ", dfLOG_LEVEL_SYSTEM);

	SC_SendAroundSector(session, (char*)packet.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_ATTACK1), false);
}

inline void Server::SC_Attack2(Session* session, CPacket& packet)
{
	//__LOG(L"Send Sector Around Attack2 character ", dfLOG_LEVEL_SYSTEM);

	SC_SendAroundSector(session, (char*)packet.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_ATTACK2), false);
}

inline void Server::SC_Attack3(Session* session, CPacket& packet)
{
	//__LOG(L"Send Sector Around Attack3 character ", dfLOG_LEVEL_SYSTEM);

	SC_SendAroundSector(session, (char*)packet.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_ATTACK3), false);
}

inline void Server::SC_Damage(Session* session, CPacket& packet)
{
	//__LOG(L"Send Sector Around Damage character ", dfLOG_LEVEL_SYSTEM);

	SC_SendAroundSector(session, (char*)packet.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_DAMAGE), true);
}

inline void Server::SC_ECHO(Session* session, CPacket& packet)
{
	//__LOG(L"Send Unicast ECHO character ", dfLOG_LEVEL_SYSTEM);
	SC_SendUnicast((char*)packet.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_ECHO), session);
}

inline void Server::SC_SYNC(Session* session, CPacket& packet)
{
	// ��ũ �߻� �� ����
	_syncCount++;

	SC_SendUnicast((char*)packet.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_SYNC), session);
}

inline void Server::SC_SendUnicast(char* msg, const int size, Session* session)
{
	if (session == nullptr)
	{
		Log(L"Null session access in Unicast", 0, dfLOG_LEVEL_DEBUG);
		__LOG(L"Null session access in Unicast", dfLOG_LEVEL_DEBUG);
		return;
	}

	if (session->_sendRingBuffer.GetFreeSize() < size)
	{
		__LOG(L"Send RingBuffer Full Error! SessionID :  " << session->_ID, dfLOG_LEVEL_ERROR);

		// �ش� ���� ����
		ReserveDeleteSession(session);
		return;
	}

	session->_sendRingBuffer.Enqueue(msg, size);	
}

inline void Server::SC_SendSectorOne(const int sX, const int sY, char* msg, const int size, Session* except)
{
	if (sX < 0 || sX >= dfSECTOR_WIDTH_CNT || sY < 0 || sY >= dfSECTOR_HEIGHT_CNT) return;

	for (list<Character*>::iterator iter = _sectorList[sY][sX].begin(); iter != _sectorList[sY][sX].end(); iter++)
	{
		if ((*iter)->_session == except) continue;

		SC_SendUnicast(msg, size, (*iter)->_session);
	}
}

inline void Server::SC_SendAroundSector(Session* session, char* msg, const int size, const bool sendMe)
{
	Character* character = _characterMap[session->_ID];

	for (int index = 0; index < character->_sectorAround._cnt; index++)
	{
		int sectorX = character->_sectorAround._around[index]._x;
		int sectorY = character->_sectorAround._around[index]._y;

		if (character->_sectorPos._x == sectorX && character->_sectorPos._y && sendMe == false)
		{
			SC_SendSectorOne(sectorX, sectorY, msg, size, session);	
		}
		else
		{
			SC_SendSectorOne(sectorX, sectorY, msg, size, nullptr);
		}
	}
}

Server::Session::Session(SOCKET socket, DWORD ID, SOCKADDR_IN* addr)
	: _socket(socket), _ID(ID)
{
	// SCOKADDR_IN
	memcpy(&_addr, addr, sizeof(SOCKADDR_IN));

	// ������ ���Žð� �ʱ�ȭ
	_lastRecvTime = timeGetTime();
}

Server::Session::~Session()
{

}

Server::Character::Character(Session* session)
	: _session(session), _hp(dfMAX_HP), _moveFlag(false)
{	

	// �ʱ� ��ġ, ���� ����
	{
		_x = (rand() % dfRANGE_MOVE_RIGHT);
		if (_x < dfRANGE_MOVE_LEFT) _x = dfRANGE_MOVE_LEFT;

		_y = (rand() % dfRANGE_MOVE_BOTTOM);
		if (_y < dfRANGE_MOVE_TOP) _y = dfRANGE_MOVE_TOP;

		// �����ϴ� ��ġ�� ���� �ٶ󺸴� ���� ����
		if (_x > (dfRANGE_MOVE_LEFT + dfRANGE_MOVE_RIGHT) / 2)
		{
			_dir = dfPACKET_MOVE_DIR_LL;
		}
		else
		{
			_dir = dfPACKET_MOVE_DIR_RR;
		}
	}

	// ���� ����
	{
		// SECTOR_POS
		_sectorPos._x = (_x / dfSECTOR_SIZE);
		_sectorPos._y = (_y / dfSECTOR_SIZE);
	}
}
