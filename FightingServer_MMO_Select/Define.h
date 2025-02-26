#pragma once

/*************************

       Define List

**************************/

//-----------------------------------------------------------------
// 30�� �̻��� �ǵ��� �ƹ��� �޽��� ���ŵ� ���°�� ���� ����.
//-----------------------------------------------------------------
#define dfNETWORK_PACKET_RECV_TIMEOUT	30000

//-----------------------------------------------------------------
// ���� �ɼ�
//-----------------------------------------------------------------
#define dfINVALID_ID    -1
#define dfMAX_SESSION   16000

#define dfFPS           25
#define dfX_MOVE        6
#define dfY_MOVE        4
#define dfMAX_HP        100

//-----------------------------------------------------------------
// ȭ�� �̵�����
//-----------------------------------------------------------------
#define dfRANGE_MOVE_TOP	0
#define dfRANGE_MOVE_LEFT	0
#define dfRANGE_MOVE_RIGHT	6400
#define dfRANGE_MOVE_BOTTOM	6400

#define dfSECTOR_SIZE       100
#define dfSECTOR_WIDTH_CNT  dfRANGE_MOVE_RIGHT  / dfSECTOR_SIZE
#define dfSECTOR_HEIGHT_CNT dfRANGE_MOVE_BOTTOM / dfSECTOR_SIZE

//-----------------------------------------------------------------
// �̵� ����üũ ����
//-----------------------------------------------------------------
#define dfERROR_RANGE		50


//---------------------------------------------------------------
// ���ݹ���.
//---------------------------------------------------------------
#define dfATTACK1_RANGE_X		80
#define dfATTACK2_RANGE_X		90
#define dfATTACK3_RANGE_X		100
#define dfATTACK1_RANGE_Y		10
#define dfATTACK2_RANGE_Y		10
#define dfATTACK3_RANGE_Y		20

//---------------------------------------------------------------
// ���ݷ�.
//---------------------------------------------------------------
#define dfATTACK1_DAMAGE        3
#define dfATTACK2_DAMAGE        5
#define dfATTACK3_DAMAGE        7

//---------------------------------------------------------------
// ���� �ɼ�
//---------------------------------------------------------------
#define	RINGBUFFER_SIZE	1024 * 5
#define	RINGBUFFER_SIZEMAX	1024 * 10
#define RINGBUFFER_SIZE	1024 * 5
#define RINGBUFFER_SIZEMAX	1024 * 10

#define PACKET_CODE 0x89

#define UNICAST     0
#define BROADCAST   1