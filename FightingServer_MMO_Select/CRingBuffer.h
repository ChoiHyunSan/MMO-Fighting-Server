#pragma once
/*************************

        CRingBuffer

**************************/
// 1바이트 단위로 데이터를 저장하는 원형 큐

#define	RINGBUFFER_SIZE	1024 * 5
#define	RINGBUFFER_SIZEMAX	1024 * 20

class CRingBuffer
{
public:
    CRingBuffer();
    CRingBuffer(int size);
    ~CRingBuffer();

public:
    void Resize(const int size);      // 크기 재조정
    int  GetBufferSize()              // 버퍼크기 반환
    {
        return _bufferSize;
    }

    /////////////////////////////////////////////////////////////////////////
    // 현재 사용중인 용량 얻기.
    //
    // Parameters: 없음.
    // Return: (int)사용중인 용량.
    /////////////////////////////////////////////////////////////////////////
    int	GetUseSize(void)
    {
        int front = _front;
        int rear = _rear;

        if (front == rear) return 0;
        else if ((rear + 1) % _bufferSize == front)  return _bufferSize - 1;
        else if (front < rear)  return rear - front;
        else return (_bufferSize - front + rear - 1);
    }

    /////////////////////////////////////////////////////////////////////////
    // 현재 버퍼에 남은 용량 얻기. 
    //
    // Parameters: 없음.
    // Return: (int)남은용량.
    /////////////////////////////////////////////////////////////////////////
    int	GetFreeSize(void)
    {
        return (_bufferSize - 1) - GetUseSize();
    }

    /////////////////////////////////////////////////////////////////////////
    // WritePos 에 데이타 넣음.
    //
    // Parameters: (char *)데이타 포인터. (int)크기. 
    // Return: (int)넣은 크기.
    /////////////////////////////////////////////////////////////////////////
    int	Enqueue(char* chpData, int iSize);

    /////////////////////////////////////////////////////////////////////////
    // ReadPos 에서 데이타 가져옴. ReadPos 이동.
    //
    // Parameters: (char *)데이타 포인터. (int)크기.
    // Return: (int)가져온 크기.
    /////////////////////////////////////////////////////////////////////////
    int	Dequeue(char* chpDest, int iSize);

    /////////////////////////////////////////////////////////////////////////
    // ReadPos 에서 데이타 읽어옴. ReadPos 고정.
    //
    // Parameters: (char *)데이타 포인터. (int)크기.
    // Return: (int)가져온 크기.
    /////////////////////////////////////////////////////////////////////////
    int	Peek(char* chpDest, int iSize);

    /////////////////////////////////////////////////////////////////////////
    // 버퍼의 모든 데이타 삭제.
    //
    // Parameters: 없음.
    // Return: 없음.
    /////////////////////////////////////////////////////////////////////////
    void ClearBuffer(void)
    {
        _rear = _front;
    }

    /////////////////////////////////////////////////////////////////////////
    // 버퍼 포인터로 외부에서 한방에 읽고, 쓸 수 있는 길이.
    // (끊기지 않은 길이)
    //
    // 원형 큐의 구조상 버퍼의 끝단에 있는 데이터는 끝 -> 처음으로 돌아가서
    // 2번에 데이터를 얻거나 넣을 수 있음. 이 부분에서 끊어지지 않은 길이를 의미
    //
    // Parameters: 없음.
    // Return: (int)사용가능 용량.
    ////////////////////////////////////////////////////////////////////////
    int	DirectEnqueueSize(void)
    {
        if (_front <= _rear) return _bufferSize - _rear - 1;
        else return _front - _rear - 1;
    }
    int	DirectDequeueSize(void)
    {
        if (_front == _rear) return 0;
        else if (_front < _rear) return _rear - _front;
        else return _bufferSize - _front - 1;
    }

    /////////////////////////////////////////////////////////////////////////
    // 원하는 길이만큼 읽기위치 에서 삭제 / 쓰기 위치 이동
    //
    // Parameters: 없음.
    // Return: (int)이동크기
    /////////////////////////////////////////////////////////////////////////
    int	MoveRear(int iSize);
    int	MoveFront(int iSize);

    /////////////////////////////////////////////////////////////////////////
    // 버퍼의 Front 포인터 얻음.
    //
    // Parameters: 없음.
    // Return: (char *) 버퍼 포인터.
    /////////////////////////////////////////////////////////////////////////
    char* GetFrontBufferPtr(void);

    /////////////////////////////////////////////////////////////////////////
    // 버퍼의 RearPos 포인터 얻음.
    //
    // Parameters: 없음.
    // Return: (char *) 버퍼 포인터.
    /////////////////////////////////////////////////////////////////////////
    char* GetRearBufferPtr(void);

    char* GetWritePtr(void);

private:
    void Init();

private:
    int _bufferSize;

    char* _buf;
    int _front;       // 맨 앞 원소의 바로 앞 인덱스
    int _rear;        // 맨 뒤 원소의 인덱스
};

