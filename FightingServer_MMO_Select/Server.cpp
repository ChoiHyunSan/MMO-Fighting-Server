#pragma once
#include "Server.h"

int Server::_updateCnt = 0;
bool Server::_shutdown = true;

Server::Server()
{
	// TODO : 서버 초기화를 담당한다.
	__LOG(L"Server Start", dfLOG_LEVEL_SYSTEM);

	// 시스템 초기화 설정
	{
		srand(7777);
		timeBeginPeriod(1);
	}

	// 네트워크 & 컨텐츠 초기화
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
	// TODO : 사용한 리소스를 모두 해제한다.
	{
		timeEndPeriod(1);

		closesocket(_listenSocket);
		WSACleanup();
	}
}

bool Server::InitNetwork()
{
	// TODO : 네트워크 초기화할 데이터를 처리한다.
	
	// 세션 리소스 세팅
	{
		if (_readSockets == nullptr || _writeSockets == nullptr)
		{
			__LOG(L"Allocate Error", dfLOG_LEVEL_ERROR);
			Log(L"Allocate Error", 0, dfLOG_LEVEL_ERROR);
			return false;
		}
	}

	// 네트워크 세팅
	{
		// WSA 초기화
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

		// Listen socket 생성
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

		// Linger 옵션 설정
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

		// Non-Blocking 설정
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
	// TODO : 컨텐츠에서 초기화할 데이터를 처리한다.

	memset(_IDArr, 0, sizeof(_IDArr));
	for (int i = 0; i < dfMAX_SESSION; i++)
		_IDQueue.push(i);

	return true;
}

bool Server::UpdateNetwork()
{
	// TODO : READ/WRITE SET에 대한 SELECT를 처리한다.
	
	// NetWork Loop를 계산한다.
	_netCnt++;
	if (_netCnt % NET_SKIP != 0) return true;

	// READ & WRITE set check
	int stReadSet = 0;
	int endReadSet = 0;
	int stWriteSet = 0;
	int endWriteSet = 0;
	
	// Socket 순회탐색
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

	// Select처리
	{
		// 모두 최대로 처리가능한 개수인 경우
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
		// Read만 최대로 처리가능한 개수인 경우
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
		// Write만 최대로 처리가능한 개수인 경우
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
	// TODO : 프레임 단위로 컨텐츠 로직을 처리한다.
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

		// 움직이는 상태인지 확인 
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

	// Select 탐색
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

	// SelectProc시마다 accpet를 가능할 때까지 처리하는 함수 진행
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

	// Direct Input처리
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
			// 링 버퍼를 넘기는 크기의 메시지를 보냄 -> 연결 종료
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

			// WSAEWOULDBLOCK으로 작업을 수행하지 못한 경우에는 다음 프레임 상태를 기다린다.
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

	// Step 1. 세션 ID 탐색
	DWORD sessionID = GetSessionID();
	if (sessionID == dfINVALID_ID)
	{
		Log(L"Accept Failed : Full Session", 0, dfLOG_LEVEL_ERROR);
		return true;
	}

	PRO_BEGIN("Accept SOCKET");

	// Step 2. 클라이언트 주소 세팅
	SOCKADDR_IN clientAddr;
	memset(&clientAddr, 0, sizeof(clientAddr));
	int addrLen = sizeof(clientAddr);

	// Step 3. Accept함수 호출 및 예외처리
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

	// Step 4. 세션 ID 부여
	// _IDArr[sessionID] = true;
	_IDQueue.pop();

	PRO_END("Accept SOCKET");

	PRO_BEGIN("Accept Alloc");
	// Step 5. Session & Character 생성
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

	// Step 6. 주위섹터 탐색
	SetAroundSector(newCharacter->_sectorPos._x, newCharacter->_sectorPos._y, newCharacter);

	// Step 7. 생성 작업
	{
		// 자기 자신에 대한 캐릭터 생성 요청
		{
			newSession->_packetBuffer.Clear();
			CreatePacket_CreateMyCharacter(newSession->_packetBuffer, newSession->_ID, newCharacter->_hp, newCharacter->_x, newCharacter->_y, newCharacter->_dir);
			SC_CreateMyCharacter(newSession, newSession->_packetBuffer);
		}

		// 주위 섹터탐색
		int sectorCnt = newCharacter->_sectorAround._cnt;
		for (int cnt = 0; cnt < sectorCnt; cnt++)
		{
			stSECTOR_POS* sectorAround = &newCharacter->_sectorAround._around[cnt];
			list<Character*>* sectorList = &_sectorList[sectorAround->_y][sectorAround->_x];
			for (list<Character*>::iterator iter = sectorList->begin(); iter != sectorList->end(); iter++)
			{
				Character* otherCharacter = (*iter);
				Session* otherSession = otherCharacter->_session;

				// 새로운 세션 생성에 대한 메시지 보내기
				{
					otherSession->_packetBuffer.Clear();
					CreatePacket_CreateOtherCharacter(otherSession->_packetBuffer, newSession->_ID, newCharacter->_hp, newCharacter->_x, newCharacter->_y, newCharacter->_dir);
					SC_CreateOtherCharacter(otherSession, otherSession->_packetBuffer);
				}

				// 새로운 세션에 기존 캐릭터를 생성하는 메시지를 보낸다.
				{
					newSession->_packetBuffer.Clear();
					CreatePacket_CreateOtherCharacter(newSession->_packetBuffer, otherSession->_ID, otherCharacter->_hp, otherCharacter->_x, otherCharacter->_y, otherCharacter->_dir);
					// CreatePacket_CreateOtherCharacter(newSession->_packetBuffer, otherSession->_ID, otherCharacter->_hp, otherCharacter->_x, otherCharacter->_y, otherCharacter->_dir);
					SC_CreateOtherCharacter(newSession, newSession->_packetBuffer);
				}

				// 상태정보를 확인하여 정보 보내기
				if (otherCharacter->_moveFlag)
				{
					newSession->_packetBuffer.Clear();
					CreatePacket_MoveStartCharacter(newSession->_packetBuffer, otherSession->_ID, otherCharacter->_moveDir, otherCharacter->_x, otherCharacter->_y);

					SC_SendUnicast((char*)newSession->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), newSession);
				}
			}
		}

	}

	// Step 8. 섹터에 추가
	PushSectorList(newCharacter->_sectorPos._x, newCharacter->_sectorPos._y, newCharacter);

	// Step 9. 세션 추가
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

		// 제거정보 알림 (주위 섹터)
		Session* deleteSession = _sessionMap[deleteSocket];

		deleteSession->_packetBuffer.Clear();
		CreatePacket_DeleteCharacter(deleteSession->_packetBuffer, ID);
		SC_DeleteCharacter(deleteSession, deleteSession->_packetBuffer);
		
		// _sectorList에서 제거
		EraseSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

		// _characterMap에서 제거
		delete character;
		
		_characterMap.erase(ID);
		character = nullptr;

		// ID할당 제거
		_IDQueue.push(ID);

		// _sessionMap에서 제거
		delete deleteSession;

		_sessionMap.erase(deleteSocket);

		// 소켓 리소스 반환
		closesocket(deleteSocket);
	}

	_deleteSet.clear();
}

inline bool Server::UpdateMove(Character* character)
{
	int newX, newY;

	// 방향에 따라 swich문 처리
	switch (character->_moveDir)
	{
		case dfPACKET_MOVE_DIR_LL:
		{
			newX = character->_x - dfX_MOVE;
			newY = character->_y;

			// case 1 : 왼쪽 섹터로 넘어가는 경우
			if (character->_sectorPos._x != 0 /* 가장 왼쪽 섹터가 아닌 경우 */
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

			if (character->_sectorPos._x != 0 /* 가장 왼쪽 섹터가 아닌 경우 */
				&& ((character->_sectorPos._x) * dfSECTOR_SIZE) > newX)
			{
				// case 1 : 왼쪽위쪽 섹터로 넘어가는 경우
				if (character->_sectorPos._y != 0 /* 가장 위쪽 섹터가 아닌 경우 */
					&& ((character->_sectorPos._y) * dfSECTOR_SIZE) > newY)
				{
					InvaidSectorLU(character);
				}
				// case 2 : 왼쪽 섹터로 넘어가는 경우
				else
				{
					InvaidSectorLL(character);
				}
			}
			else
			{
				// case 3 : 위족 섹터로 넘어가는 경우
				if (character->_sectorPos._y != 0 /* 가장 위쪽 섹터가 아닌 경우 */
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

			// case 1 : 위쪽 섹터로 넘어가는 경우
			if (character->_sectorPos._y != 0 /* 가장 위쪽 섹터가 아닌 경우 */
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

			if (character->_sectorPos._x != dfSECTOR_WIDTH_CNT - 1 /* 가장 오른쪽 섹터가 아닌 경우 */
				&& ((character->_sectorPos._x + 1) * dfSECTOR_SIZE) <= newX)
			{
				// 오른쪽위쪽 섹터를 침범하는 경우
				if (character->_sectorPos._y != 0 /* 가장 위쪽 섹터가 아닌 경우 */
					&& ((character->_sectorPos._y) * dfSECTOR_SIZE) > newY)
				{
					InvaidSectorRU(character);
				}
				// 오른쪽 섹터를 침범하는 경우
				else
				{
					InvaidSectorRR(character);
				}
			}
			else
			{
				// 위쪽 섹터를 침범하는 경우
				if (character->_sectorPos._y != 0 /* 가장 위쪽 섹터가 아닌 경우 */
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
		
			// case 1 : 오른쪽 섹터로 넘어가는 경우
			if (character->_sectorPos._x != dfSECTOR_WIDTH_CNT - 1 /* 가장 오른쪽 섹터가 아닌 경우 */
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

			if (character->_sectorPos._x != dfSECTOR_WIDTH_CNT - 1 /* 가장 오른쪽 섹터가 아닌 경우 */
				&& ((character->_sectorPos._x + 1) * dfSECTOR_SIZE) <= newX)
			{
				if (character->_sectorPos._y != dfSECTOR_HEIGHT_CNT - 1 /* 가장 아래쪽 섹터가 아닌 경우 */
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
				if (character->_sectorPos._y != dfSECTOR_HEIGHT_CNT - 1 /* 가장 아래쪽 섹터가 아닌 경우 */
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

			// case 1 : 아래쪽 섹터로 넘어간 경우
			if (character->_sectorPos._y != dfSECTOR_HEIGHT_CNT - 1 /* 가장 아래쪽 섹터가 아닌 경우 */
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

			if (character->_sectorPos._x != 0 /* 가장 왼쪽 섹터가 아닌 경우 */
				&& ((character->_sectorPos._x - 1) * dfSECTOR_SIZE) > newX)
			{
				if (character->_sectorPos._y != dfSECTOR_HEIGHT_CNT - 1 /* 가장 아래쪽 섹터가 아닌 경우 */
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
				if (character->_sectorPos._y != dfSECTOR_HEIGHT_CNT - 1 /* 가장 아래쪽 섹터가 아닌 경우 */
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
	// 1) 클라이언트가 보낸 위치값이 내가 계산한 값과의 오차를 확인
	if (abs(character->_x - xPos) > 50 || abs(character->_y - yPos) > 50)
	{
		return false;
	}

	return true;
}

// 공격 성공 시, true 반환
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

				// 피격알림 메시지 보내기
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

				// 피격알림 메시지 보내기
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

				// 피격알림 메시지 보내기
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
	// 현재 위치한 섹터 확인
	{
		if (AttackSector(character->_sectorPos._x, character->_sectorPos._y, character, type)) return;
	}

	if (character->_dir == dfPACKET_MOVE_DIR_LL)
	{
		// 위쪽 섹터 검색해야 하는지 여부확인
		if (character->_sectorPos._y != 0 && ((character->_sectorPos._y) * dfSECTOR_SIZE) > character->_y - rangeY / 2)
		{
			// 왼쪽 섹터 검색해야하는지 여부 확인
			if (character->_sectorPos._x != 0 && ((character->_sectorPos._x - 1) * dfSECTOR_SIZE) > character->_x - rangeX)
			{
				// 왼쪽, 위쪽, 왼쪽위쪽 확인
				if (AttackSector(character->_sectorPos._x - 1, character->_sectorPos._y, character, type)) return;
				if (AttackSector(character->_sectorPos._x, character->_sectorPos._y - 1, character, type)) return;
				if (AttackSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character, type)) return;
			}
			else
			{
				// 위쪽 확인
				if (AttackSector(character->_sectorPos._x, character->_sectorPos._y - 1, character, type)) return;
			}
		}
		else
		{
			// 아래쪽 섹터 검색해야 하는지 여부 확인
			if (character->_sectorPos._y != dfSECTOR_HEIGHT_CNT - 1 && ((character->_sectorPos._y + 1) * dfSECTOR_SIZE) <= character->_y + rangeY / 2)
			{
				// 왼쪽 섹터 검색해야하는지 여부 확인
				if (character->_sectorPos._x != 0 && ((character->_sectorPos._x - 1) * dfSECTOR_SIZE) > character->_x - rangeX)
				{
					// 왼쪽 아래쪽 왼쪽아래쪽 확인
					if (AttackSector(character->_sectorPos._x - 1, character->_sectorPos._y, character, type)) return;
					if (AttackSector(character->_sectorPos._x, character->_sectorPos._y + 1, character, type)) return;
					if (AttackSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character, type)) return;
				}
				else
				{
					// 아래쪽 확인
					if (AttackSector(character->_sectorPos._x, character->_sectorPos._y + 1, character, type)) return;
				}
			}
			else
			{
				// 왼쪽 섹터 검색해야하는지 여부 확인
				if (character->_sectorPos._x != 0 && ((character->_sectorPos._x - 1) * dfSECTOR_SIZE) > character->_x - rangeX)
				{
					// 왼쪽 섹터 확인
					if (AttackSector(character->_sectorPos._x - 1, character->_sectorPos._y, character, type)) return;
				}
			}
		}
	}
	else
	{
		// 위쪽 섹터 검색해야 하는지 여부확인
		if (character->_sectorPos._y != 0 && ((character->_sectorPos._y) * dfSECTOR_SIZE) > character->_y - rangeY / 2)
		{
			// 오른쪽 섹터 검색해야 하는지 확인
			if (character->_sectorPos._x != dfSECTOR_WIDTH_CNT - 1 && ((character->_sectorPos._x + 1) * dfSECTOR_SIZE) <= character->_x + rangeX)
			{
				// 위쪽 오른쪽 위쪽오른쪽 확인
				if (AttackSector(character->_sectorPos._x, character->_sectorPos._y + 1, character, type)) return;
				if (AttackSector(character->_sectorPos._x + 1, character->_sectorPos._y, character, type)) return;
				if (AttackSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character, type)) return;
			}
			else
			{
				// 위쪽 확인
				if (AttackSector(character->_sectorPos._x, character->_sectorPos._y + 1, character, type)) return;
			}
		}
		else
		{
			// 아래쪽 섹터 검색해야 하는지 여부 확인
			if (character->_sectorPos._y != dfSECTOR_HEIGHT_CNT - 1 && ((character->_sectorPos._y + 1) * dfSECTOR_SIZE) <= character->_y + rangeY / 2)
			{
				// 오른쪽 섹터 검색해야 하는지 확인
				if (character->_sectorPos._x != dfSECTOR_WIDTH_CNT - 1 && ((character->_sectorPos._x + 1) * dfSECTOR_SIZE) <= character->_x + rangeX)
				{
					// 아래쪽 오른쪽 아래쪽오른쪽 확인
					if (AttackSector(character->_sectorPos._x, character->_sectorPos._y - 1, character, type)) return;
					if (AttackSector(character->_sectorPos._x + 1, character->_sectorPos._y, character, type)) return;
					if (AttackSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character, type)) return;
				}
				else
				{
					// 아래쪽 확인
					if (AttackSector(character->_sectorPos._x, character->_sectorPos._y - 1, character, type)) return;
				}
			}
			else
			{
				// 오른쪽 섹터 검색해야 하는지 확인
				if (character->_sectorPos._x != dfSECTOR_WIDTH_CNT - 1 && ((character->_sectorPos._x + 1) * dfSECTOR_SIZE) <= character->_x + rangeX)
				{
					// 오른쪽 확인
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
	// 가장자리 섹터에 위치했는지 판단
	
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
	else                                                    // KeyPad상 위치
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
	// Sector Border 체크
	if (sX < 0 || sX >= dfSECTOR_WIDTH_CNT || sY < 0 || sY >= dfSECTOR_HEIGHT_CNT) return;

	if (createFlag)
	{
		list<Character*>::iterator iter = _sectorList[sY][sX].begin();
		for (; iter != _sectorList[sY][sX].end(); iter++)
		{
			Character* notiChar = (*iter);
			if (notiChar == character) continue;

			// 생성 메시지
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
		
		// 생성 메시지
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
	// 필요 없는 것 같음. 잠금
	return;

	if (sX < 0 || sX >= dfSECTOR_WIDTH_CNT || sY < 0 || sY >= dfSECTOR_HEIGHT_CNT) return;

	// 생성 메시지
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
	// 기존 섹터에서 제거 - 해당 플레이어가 보이지 않을 주변 섹터에 알리기
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

	// 새로운 섹터에 추가 - 해당 플레이어가 보여야 할 섹터에 알리기
	character->_sectorPos._x -= 1;
	PushSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y, character, true);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character, true);

	// AroundSector 세팅 + 상태 갱신 메시지
	{
		SetAroundSector(character->_sectorPos._x, character->_sectorPos._y, character);

		character->_session->_packetBuffer.Clear();
		CreatePacket_MoveStartCharacter(character->_session->_packetBuffer, character->_session->_ID, character->_moveDir, character->_x, character->_y);

		SC_SendSectorOne(character->_sectorPos._x - 1, character->_sectorPos._y - 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x - 1, character->_sectorPos._y, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x - 1, character->_sectorPos._y + 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
	}

	// 새로 생긴 섹터에 있던 캐릭터들에 대한 처리
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character);
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y, character);
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character);
}

inline void Server::InvaidSectorLU(Character* character)
{
	// 기존 섹터에서 제거 - 해당 플레이어가 보이지 않을 주변 섹터에 알리기
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

	// 새로운 섹터에 추가 - 해당 플레이어가 보여야 할 섹터에 알리기
	character->_sectorPos._y -= 1;
	character->_sectorPos._x -= 1;
	PushSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y, character, true);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character, true);


	// AroundSector 세팅 + 상태 갱신 메시지
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

	// 새로 생긴 섹터에 있던 캐릭터들에 대한 처리
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character);
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y, character);
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character);
	CreateSectorToCharacter(character->_sectorPos._x, character->_sectorPos._y - 1, character);
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character);
}

inline void Server::InvaidSectorLD(Character* character)
{
	// 기존 섹터에서 제거 - 해당 플레이어가 보이지 않을 주변 섹터에 알리기
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

	// 새로운 섹터에 추가 - 해당 플레이어가 보여야 할 섹터에 알리기
	character->_sectorPos._y -= 1;
	character->_sectorPos._x -= 1;
	PushSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y, character, true);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x, character->_sectorPos._y + 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character, true);


	// AroundSector 세팅 + 상태 갱신 메시지
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

	// 새로 생긴 섹터에 있던 캐릭터들에 대한 처리
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character);
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y, character);
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character);
	CreateSectorToCharacter(character->_sectorPos._x, character->_sectorPos._y + 1, character);
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);
}

inline void Server::InvaidSectorRR(Character* character)
{
	// 기존 섹터에서 제거 - 해당 플레이어가 보이지 않을 주변 섹터에 알리기
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

	// 새로운 섹터에 추가 - 해당 플레이어가 보여야 할 섹터에 알리기
	character->_sectorPos._x += 1;
	PushSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character, true);

	// AroundSector 세팅 + 상태 갱신 메시지
	{
		SetAroundSector(character->_sectorPos._x, character->_sectorPos._y, character);

		character->_session->_packetBuffer.Clear();
		CreatePacket_MoveStartCharacter(character->_session->_packetBuffer, character->_session->_ID, character->_moveDir, character->_x, character->_y);

		SC_SendSectorOne(character->_sectorPos._x + 1, character->_sectorPos._y - 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x + 1, character->_sectorPos._y, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x + 1, character->_sectorPos._y + 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
	}

	// 새로 생긴 섹터에 있던 캐릭터들에 대한 처리
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character);
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y, character);
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);
}

inline void Server::InvaidSectorRU(Character* character)
{
	// 기존 섹터에서 제거 - 해당 플레이어가 보이지 않을 주변 섹터에 알리기
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

	// 새로운 섹터에 추가 - 해당 플레이어가 보여야 할 섹터에 알리기
	character->_sectorPos._y -= 1;
	character->_sectorPos._x += 1;
	PushSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character, true);


	// AroundSector 세팅 + 상태 갱신 메시지
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

	// 새로 생긴 섹터에 있던 캐릭터들에 대한 처리
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y, character);
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character);
	CreateSectorToCharacter(character->_sectorPos._x, character->_sectorPos._y - 1, character);
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character);
}

inline void Server::InvaidSectorRD(Character* character)
{
	// 기존 섹터에서 제거 - 해당 플레이어가 보이지 않을 주변 섹터에 알리기
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

	// 새로운 섹터에 추가 - 해당 플레이어가 보여야 할 섹터에 알리기
	character->_sectorPos._y += 1;
	character->_sectorPos._x += 1;
	PushSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x, character->_sectorPos._y + 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character, true);


	// AroundSector 세팅 + 상태 갱신 메시지
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

	// 새로 생긴 섹터에 있던 캐릭터들에 대한 처리
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y, character);
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character);
	CreateSectorToCharacter(character->_sectorPos._x, character->_sectorPos._y + 1, character);
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character);
}

inline void Server::InvaidSectorDD(Character* character)
{
	// 기존 섹터에서 제거 - 해당 플레이어가 보이지 않을 주변 섹터에 알리기
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

	// 새로운 섹터에 추가 - 해당 플레이어가 보여야 할 섹터에 알리기
	character->_sectorPos._y += 1;
	PushSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x, character->_sectorPos._y + 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character, true);

	// AroundSector 세팅 + 상태 갱신 메시지
	{
		SetAroundSector(character->_sectorPos._x, character->_sectorPos._y, character);

		character->_session->_packetBuffer.Clear();
		CreatePacket_MoveStartCharacter(character->_session->_packetBuffer, character->_session->_ID, character->_moveDir, character->_x, character->_y);

		SC_SendSectorOne(character->_sectorPos._x - 1, character->_sectorPos._y + 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x, character->_sectorPos._y + 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x + 1, character->_sectorPos._y + 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
	}

	// 새로 생긴 섹터에 있던 캐릭터들에 대한 처리
	CreateSectorToCharacter(character->_sectorPos._x - 1, character->_sectorPos._y + 1, character);
	CreateSectorToCharacter(character->_sectorPos._x, character->_sectorPos._y + 1, character);
	CreateSectorToCharacter(character->_sectorPos._x + 1, character->_sectorPos._y + 1, character);
}

inline void Server::InvaidSectorUU(Character* character)
{
	// 기존 섹터에서 제거 - 해당 플레이어가 보이지 않을 주변 섹터에 알리기
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

	// 새로운 섹터에 추가 - 해당 플레이어가 보여야 할 섹터에 알리기
	character->_sectorPos._y -= 1;
	PushSectorList(character->_sectorPos._x, character->_sectorPos._y, character);

	NotifyCharacterToSector(character->_sectorPos._x - 1, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x, character->_sectorPos._y - 1, character, true);
	NotifyCharacterToSector(character->_sectorPos._x + 1, character->_sectorPos._y - 1, character, true);

	// AroundSector 세팅 + 상태 갱신 메시지
	{
		SetAroundSector(character->_sectorPos._x, character->_sectorPos._y, character);

		character->_session->_packetBuffer.Clear();
		CreatePacket_MoveStartCharacter(character->_session->_packetBuffer, character->_session->_ID, character->_moveDir, character->_x, character->_y);

		SC_SendSectorOne(character->_sectorPos._x - 1, character->_sectorPos._y - 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x, character->_sectorPos._y - 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
		SC_SendSectorOne(character->_sectorPos._x + 1, character->_sectorPos._y - 1, (char*)character->_session->_packetBuffer.GetBufferPtr(), sizeof(stPacketHeader) + sizeof(stPACKET_SC_MOVE_START), nullptr);
	}

	// 새로 생긴 섹터에 있던 캐릭터들에 대한 처리
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

		// 패킷 헤더 꺼내기
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
				// 잘못된 패킷을 보내는 경우 연결 해제
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
	// 캐릭터 moveFlag 변경
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

	// 메세지 생성
	{
		packet.Clear();
		CreatePacket_MoveStartCharacter(packet, session->_ID, character->_moveDir, character->_x, character->_y);

		// 메세지 처리
		SC_MoveStartCharacter(session, packet);
	}
}

inline void Server::CS_MoveStopCharacter(Session* session, CPacket& packet)
{
	BYTE dir;
	SHORT x;
	SHORT y;

	packet >> dir >> x >> y;

	// 캐릭터 moveFlag 변경
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

	// 메세지 생성
	{
		packet.Clear();
		CreatePacket_MoveStopCharacter(packet, session->_ID, character->_dir, character->_x, character->_y);

		// 메세지 처리
		SC_MoveStopCharacter(session, packet);
	}
}

inline void Server::CS_Attack1(Session* session, CPacket& packet)
{
	Character* character = _characterMap[session->_ID];

	// 공격처리 주변 섹터에 알림
	{
		packet.Clear();
		CreatePacket_Attack1(packet, session->_ID, character->_dir, character->_x, character->_y);

		SC_Attack1(session, packet);
	}

	// 공격적중 탐색 (먼저 한명 때리면 나머지는 무시)
	{
		Attack(character, dfATTACK1_RANGE_X, dfATTACK1_RANGE_Y, dfPACKET_SC_ATTACK1);
	}
}

inline void Server::CS_Attack2(Session* session, CPacket& packet)
{
	Character* character = _characterMap[session->_ID];

	// 공격처리 주변 섹터에 알림
	{
		packet.Clear();
		CreatePacket_Attack2(packet, session->_ID, character->_dir, character->_x, character->_y);

		SC_Attack2(session, packet);
	}

	// 공격적중 탐색 (먼저 한명 때리면 나머지는 무시)
	{
		Attack(character, dfATTACK2_RANGE_X, dfATTACK2_RANGE_Y, dfPACKET_SC_ATTACK2);
	}
}

inline void Server::CS_Attack3(Session* session, CPacket& packet)
{
	Character* character = _characterMap[session->_ID];

	// 공격처리 주변 섹터에 알림
	{
		packet.Clear();
		CreatePacket_Attack3(packet, session->_ID, character->_dir, character->_x, character->_y);

		SC_Attack3(session, packet);
	}

	// 공격적중 탐색 (먼저 한명 때리면 나머지는 무시)
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
	// 싱크 발생 시 증가
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

		// 해당 연결 종료
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

	// 마지막 수신시간 초기화
	_lastRecvTime = timeGetTime();
}

Server::Session::~Session()
{

}

Server::Character::Character(Session* session)
	: _session(session), _hp(dfMAX_HP), _moveFlag(false)
{	

	// 초기 위치, 방향 지정
	{
		_x = (rand() % dfRANGE_MOVE_RIGHT);
		if (_x < dfRANGE_MOVE_LEFT) _x = dfRANGE_MOVE_LEFT;

		_y = (rand() % dfRANGE_MOVE_BOTTOM);
		if (_y < dfRANGE_MOVE_TOP) _y = dfRANGE_MOVE_TOP;

		// 스폰하는 위치에 따라서 바라보는 방향 지정
		if (_x > (dfRANGE_MOVE_LEFT + dfRANGE_MOVE_RIGHT) / 2)
		{
			_dir = dfPACKET_MOVE_DIR_LL;
		}
		else
		{
			_dir = dfPACKET_MOVE_DIR_RR;
		}
	}

	// 섹터 지정
	{
		// SECTOR_POS
		_sectorPos._x = (_x / dfSECTOR_SIZE);
		_sectorPos._y = (_y / dfSECTOR_SIZE);
	}
}
