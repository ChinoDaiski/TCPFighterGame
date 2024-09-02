#pragma once

#include <Windows.h>
#include <cstring>
#include <type_traits>

class CPacket
{
public:
	enum class en_PACKET : UINT8
	{
		eBUFFER_DEFAULT = 1400		// ��Ŷ�� �⺻ ���� ������.
	};

	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Return:
	//////////////////////////////////////////////////////////////////////////
	explicit CPacket(int iBufferSize = static_cast<UINT8>(en_PACKET::eBUFFER_DEFAULT));
	~CPacket();

	//////////////////////////////////////////////////////////////////////////
	// ��Ŷ û��.
	//
	// Parameters: ����.
	// Return: ����.
	//////////////////////////////////////////////////////////////////////////
	void	Clear(void);

	//////////////////////////////////////////////////////////////////////////
	// ���� ������ ���.
	//
	// Parameters: ����.
	// Return: (int)��Ŷ ���� ������ ���.
	//////////////////////////////////////////////////////////////////////////
	int	GetBufferSize(void) { return m_iBufferSize; }

	//////////////////////////////////////////////////////////////////////////
	// ���� ������� ������ ���.
	//
	// Parameters: ����.
	// Return: (int)������� ����Ÿ ������.
	//////////////////////////////////////////////////////////////////////////
	int		GetDataSize(void) { return m_iRear - m_iFront; }

	//////////////////////////////////////////////////////////////////////////
	// ���� ������ ���.
	//
	// Parameters: ����.
	// Return: (char *)���� ������.
	//////////////////////////////////////////////////////////////////////////
	char* GetBufferPtr(void) { return m_chpBuffer; }

	//////////////////////////////////////////////////////////////////////////
	// ���� Pos �̵�. (�����̵��� �ȵ�)
	// GetBufferPtr �Լ��� �̿��Ͽ� �ܺο��� ������ ���� ������ ������ ��� ���. 
	//
	// Parameters: (int) �̵� ������.
	// Return: (int) �̵��� ������.
	//////////////////////////////////////////////////////////////////////////
	int		MoveWritePos(int iSize);
	int		MoveReadPos(int iSize);






	/* ============================================================================= */
	// ������ �����ε�
	/* ============================================================================= */
	CPacket& operator = (CPacket& clSrcPacket);

	//////////////////////////////////////////////////////////////////////////
	// �ֱ�.	�� ���� Ÿ�Ը��� ��� ����.
	//////////////////////////////////////////////////////////////////////////
	template<typename T>
	CPacket& operator<<(const T& value) {
		if (m_iRear + sizeof(T) > m_iBufferSize)
		{
			// ��Ŷ�� �ִ� ������ ���� �� ���� ���� �� ����.
			DebugBreak();
		}

		if (!std::is_arithmetic<T>::value)
			return *this;

		// ������ ���� �� ������ �̵�
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
	// ����.	�� ���� Ÿ�Ը��� ��� ����.
	//////////////////////////////////////////////////////////////////////////

	template<typename T>
	CPacket& operator>>(const T& value) {
		if (!std::is_arithmetic<T>::value)
			return *this;

		// ������ ���� �� ������ �̵�
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
	// ����Ÿ ���.
	//
	// Parameters: (char *)Dest ������. (int)Size.
	// Return: (int)������ ������.
	//////////////////////////////////////////////////////////////////////////
	int		GetData(char* chpDest, int iSize);

	//////////////////////////////////////////////////////////////////////////
	// ����Ÿ ����.
	//
	// Parameters: (char *)Src ������. (int)SrcSize.
	// Return: (int)������ ������.
	//////////////////////////////////////////////////////////////////////////
	int		PutData(char* chpSrc, int iSrcSize);


private:
	char* m_chpBuffer;      // �������� ������ ���� �迭
	int m_iBufferSize;      // ������ ��ü �뷮
	int m_iFront;           // ���� �б� ��ġ (front)
	int m_iRear;            // ���� ���� ��ġ (rear)
};