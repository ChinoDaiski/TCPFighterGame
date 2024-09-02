#pragma once

#include <Windows.h>
#include <cstring>
#include <type_traits>

class CPacket
{
public:
	enum class en_PACKET : UINT8
	{
		eBUFFER_DEFAULT = 1400		// 패킷의 기본 버퍼 사이즈.
	};

	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Return:
	//////////////////////////////////////////////////////////////////////////
	explicit CPacket(int iBufferSize = static_cast<UINT8>(en_PACKET::eBUFFER_DEFAULT));
	~CPacket();

	//////////////////////////////////////////////////////////////////////////
	// 패킷 청소.
	//
	// Parameters: 없음.
	// Return: 없음.
	//////////////////////////////////////////////////////////////////////////
	void	Clear(void);

	//////////////////////////////////////////////////////////////////////////
	// 버퍼 사이즈 얻기.
	//
	// Parameters: 없음.
	// Return: (int)패킷 버퍼 사이즈 얻기.
	//////////////////////////////////////////////////////////////////////////
	int	GetBufferSize(void) { return m_iBufferSize; }

	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 사이즈 얻기.
	//
	// Parameters: 없음.
	// Return: (int)사용중인 데이타 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int		GetDataSize(void) { return m_iRear - m_iFront; }

	//////////////////////////////////////////////////////////////////////////
	// 버퍼 포인터 얻기.
	//
	// Parameters: 없음.
	// Return: (char *)버퍼 포인터.
	//////////////////////////////////////////////////////////////////////////
	char* GetBufferPtr(void) { return m_chpBuffer; }

	//////////////////////////////////////////////////////////////////////////
	// 버퍼 Pos 이동. (음수이동은 안됨)
	// GetBufferPtr 함수를 이용하여 외부에서 강제로 버퍼 내용을 수정할 경우 사용. 
	//
	// Parameters: (int) 이동 사이즈.
	// Return: (int) 이동된 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int		MoveWritePos(int iSize);
	int		MoveReadPos(int iSize);






	/* ============================================================================= */
	// 연산자 오버로딩
	/* ============================================================================= */
	CPacket& operator = (CPacket& clSrcPacket);

	//////////////////////////////////////////////////////////////////////////
	// 넣기.	각 변수 타입마다 모두 만듬.
	//////////////////////////////////////////////////////////////////////////
	template<typename T>
	CPacket& operator<<(const T& value) {
		if (m_iRear + sizeof(T) > m_iBufferSize)
		{
			// 패킷의 최대 사이즈 보다 더 많은 값이 들어갈 예정.
			DebugBreak();
		}

		if (!std::is_arithmetic<T>::value)
			return *this;

		// 데이터 복사 및 포인터 이동
		std::memcpy(m_chpBuffer + m_iRear, (void*)&value, sizeof(T));
		MoveWritePos(sizeof(T));

		return *this;
	}

	//CPacket& operator << (unsigned char byValue);
	//CPacket& operator << (char chValue);

	//CPacket& operator << (short shValue);
	//CPacket& operator << (unsigned short wValue);

	//CPacket& operator << (int iValue);
	//CPacket& operator << (long lValue);
	//CPacket& operator << (float fValue);

	//CPacket& operator << (__int64 iValue);
	//CPacket& operator << (double dValue);


	//////////////////////////////////////////////////////////////////////////
	// 빼기.	각 변수 타입마다 모두 만듬.
	//////////////////////////////////////////////////////////////////////////

	template<typename T>
	CPacket& operator>>(const T& value) {
		if (!std::is_arithmetic<T>::value)
			return *this;

		// 데이터 복사 및 포인터 이동
		std::memcpy((void*)&value, m_chpBuffer + m_iFront, sizeof(T));
		MoveReadPos(sizeof(T));

		return *this;
	}

	//CPacket& operator >> (BYTE& byValue);
	//CPacket& operator >> (char& chValue);

	//CPacket& operator >> (short& shValue);
	//CPacket& operator >> (WORD& wValue);

	//CPacket& operator >> (int& iValue);
	//CPacket& operator >> (DWORD& dwValue);
	//CPacket& operator >> (float& fValue);

	//CPacket& operator >> (__int64& iValue);
	//CPacket& operator >> (double& dValue);




	//////////////////////////////////////////////////////////////////////////
	// 데이타 얻기.
	//
	// Parameters: (char *)Dest 포인터. (int)Size.
	// Return: (int)복사한 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int		GetData(char* chpDest, int iSize);

	//////////////////////////////////////////////////////////////////////////
	// 데이타 삽입.
	//
	// Parameters: (char *)Src 포인터. (int)SrcSize.
	// Return: (int)복사한 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int		PutData(char* chpSrc, int iSrcSize);


private:
	char* m_chpBuffer;      // 링버퍼의 데이터 저장 배열
	int m_iBufferSize;      // 버퍼의 전체 용량
	int m_iFront;           // 현재 읽기 위치 (front)
	int m_iRear;            // 현재 쓰기 위치 (rear)
};