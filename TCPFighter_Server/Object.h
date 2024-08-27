#pragma once

// �⺻ CObject Ŭ���� ����
class CObject {
public:
    constexpr CObject(UINT16 _x = 0, UINT16 _y = 0) noexcept : m_x(_x), m_y(_y) {}
    virtual ~CObject() = default;

    // ��ġ ���� �� ��������
    constexpr void SetPosition(UINT16 _x, UINT16 _y) {
        m_x = _x;
        m_y = _y;
    }

    constexpr void getPosition(UINT16& _x, UINT16& _y) const {
        _x = m_x;
        _y = m_y;
    }

    virtual void Move() = 0;

    // ������Ʈ �޼��� (���� �������� ������Ʈ ���¸� �����ϴ� �� ���)
    virtual void Update(float deltaTime) = 0;

protected:
    UINT16 m_x, m_y;
};
