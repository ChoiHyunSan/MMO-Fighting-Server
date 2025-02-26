#pragma once
#include "pch.h"

#define NET_SKIP 8

class Server
{
public:
	Server();
	~Server();

private:
	inline bool InitNetwork();
	inline bool InitContents();

public:
	bool UpdateNetwork();
	bool UpdateContents();

private:
	
	/*     Session	    */
	class Session
	{
	public:
		Session(SOCKET socket, DWORD ID, SOCKADDR_IN* addr);
		~Session();
	
		SOCKET			_socket;
		SOCKADDR_IN		_addr;

		DWORD			_ID;
			
		CRingBuffer		_recvRingBuffer;
		CRingBuffer		_sendRingBuffer;

		CPacket			_packetBuffer;

		DWORD			_lastRecvTime;

		unsigned long long      cnt = 0;
	};

	/*      Sector	    */
	struct stSECTOR_POS
	{
		int _x;
		int _y;
	};
	struct stSECTOR_AROUND
	{
		int				_cnt;
		stSECTOR_POS	_around[9];
	};

	/*     Character    */
	class Character
	{	
	public:
		Character(Session* session);

		Session*		_session = nullptr;

		SHORT			_x;
		SHORT			_y;
		BYTE			_hp;
		BOOL			_moveFlag;

		BYTE			_dir;
		BYTE			_moveDir;

		// 섹터정보
		stSECTOR_POS      _sectorPos;
		stSECTOR_AROUND   _sectorAround;

		// 테스트 
		bool syncFlag = false;
	};

public:
	unordered_map<SOCKET, Session*>		_sessionMap;		// KEY : SOCKET , VALUE : Session*
	unordered_map<DWORD, Character*>    _characterMap;		// KEY : ID		, VALUE : Character*

private:
	list<Character*>					_sectorList[dfSECTOR_HEIGHT_CNT][dfSECTOR_WIDTH_CNT];

	// Select함수에 넣을 set모음
	SOCKET	 _readSockets[dfMAX_SESSION];
	SOCKET	 _writeSockets[dfMAX_SESSION];
	
private:
	/*    네트워크 처리 함수     */
	inline bool SelectProc(const int readSetIndex, const int readSetSize, const int writeSetIndex, const int writeSetSize);
	inline bool ReadProc(Session* session);
	inline bool WriteProc(Session* session);
	inline bool AcceptProc();
	inline int	GetSessionID();
	inline void ReserveDeleteSession(Session* session);
	inline void DeleteReserveSession();

	/*     컨텐츠 처리 함수      */
	inline bool UpdateMove(Character* character);
	inline bool CheckBorder(const int xPos, const int yPos);
	inline bool CheckMarginCoordinate(Character* character, const int xPos, const int yPos);
	inline bool AttackSector(const int sX, const int sY, Character* character, const CHAR attackType);
	inline void Attack(Character* character, const SHORT rangeX, const SHORT rangeY, const CHAR type);

	/*       섹터  처리함수      */
	inline void PushSectorList(const int x, const int y, Character* character);
	inline void EraseSectorList(const int x, const int y, Character* character);
	inline void SetAroundSector(const int x, const int y, Character* character);
	inline void NotifyCharacterToSector(const int x, const int y, Character* character, const bool createFlag /*true : 생성, false : 제거*/);
	inline void CreateSectorToCharacter(const int x, const int y, Character* character);
	inline void DeleteCharacterToSector(const int x, const int y, Character* character);
	inline void DeleteSectorToCharacter(const int x, const int y, Character* character);
	inline void InvaidSectorLL(Character* character);
	inline void InvaidSectorLU(Character* character);
	inline void InvaidSectorLD(Character* character);
	inline void InvaidSectorRR(Character* character);
	inline void InvaidSectorRU(Character* character);
	inline void InvaidSectorRD(Character* character);
	inline void InvaidSectorDD(Character* character);
	inline void InvaidSectorUU(Character* character);

	/*     프레임 처리 함수      */
	inline bool SkipFrame();

	/* 네트워크 메시지 생성 함수 */
	inline void CreatePacket_CreateOtherCharacter(CPacket& packet, const DWORD ID, const BYTE hp, const  SHORT x, const SHORT y, const BYTE dir);
	inline void CreatePacket_CreateMyCharacter(CPacket& packet, const DWORD ID, const BYTE hp, const SHORT x, const SHORT y, const BYTE dir);
	inline void CreatePacket_MoveStartCharacter(CPacket& packet, const DWORD ID, const BYTE dir, const SHORT x, const SHORT y);
	inline void CreatePacket_MoveStopCharacter(CPacket& packet, const DWORD ID, const BYTE dir, const SHORT x, const SHORT y);
	inline void CreatePacket_DeleteCharacter(CPacket& packet, const DWORD ID);
	inline void CreatePacket_ECHO(CPacket& packet, const DWORD time);
	inline void CreatePacket_SYNC(CPacket& packet, const DWORD ID, const SHORT x, const SHORT y);
	inline void CreatePacket_Attack1(CPacket& packet, const DWORD ID, const BYTE dir, const SHORT x, const SHORT y);
	inline void CreatePacket_Attack2(CPacket& packet, const DWORD ID, const BYTE dir, const SHORT x, const SHORT y);
	inline void CreatePacket_Attack3(CPacket& packet, const DWORD ID, const BYTE dir, const SHORT x, const SHORT y);
	inline void CreatePacket_Damage(CPacket& packet, const DWORD attackID, const DWORD damageID, const BYTE damageHP);

	/* 네트워크 메시지 처리 함수 */
	inline bool HandleCSPacket(Session* session);
	inline void CS_MoveStartCharacter(Session* session, CPacket& packet);
	inline void CS_MoveStopCharacter(Session* session, CPacket& packet);
	inline void CS_Attack1(Session* session, CPacket& packet);
	inline void CS_Attack2(Session* session, CPacket& packet);
	inline void CS_Attack3(Session* session, CPacket& packet);
	inline void CS_ECHO(Session* session, CPacket& packet);

	inline void SC_CreateMyCharacter(Session* session, CPacket& packet);
	inline void SC_CreateOtherCharacter(Session* session, CPacket& packet);
	inline void SC_MoveStartCharacter(Session* session, CPacket& packet);
	inline void SC_MoveStopCharacter(Session* session, CPacket& packet);
	inline void SC_DeleteCharacter(Session* session, CPacket& packet);
	inline void SC_Attack1(Session* session, CPacket& packet);
	inline void SC_Attack2(Session* session, CPacket& packet);
	inline void SC_Attack3(Session* session, CPacket& packet);
	inline void SC_Damage(Session* session, CPacket& packet);
	inline void SC_ECHO(Session* session, CPacket& packet);
	inline void SC_SYNC(Session* session, CPacket& packet);

	/* 네트워크 메시지 송신 함수 */
	inline void SC_SendUnicast(char* msg, const int size, Session* session);
	inline void SC_SendSectorOne(const int sX, const int sY, char* msg, const int size, Session* except = nullptr);
	inline void SC_SendAroundSector(Session* session, char* msg, const int size, const bool sendMe = false);

	/*      서버 관리 함수       */
	inline void Shutdown();

private:
	SOCKET			_listenSocket;
	SHORT			_connectCnt = 0;


	set<SOCKET>		_deleteSet;
	bool			_IDArr[dfMAX_SESSION];
	queue<int>		_IDQueue;
	
public:
	/*		시스템 변수들		*/
	static  int		_updateCnt;
	static	bool	_shutdown;

	int	 _netCnt = 0;
	int	 _syncCount = 0;
};

