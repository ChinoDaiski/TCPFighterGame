#pragma once

typedef struct _tagSession SESSION;

// �⺻ CObject Ŭ���� ����
class CObject {
public:
    CObject(UINT16 _x = 0, UINT16 _y = 0, bool bDead = false) noexcept : m_x(_x), m_y(_y), m_bDead(bDead) {}
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

public:
    // ���� ����
    virtual void Move() = 0;

public:
    // ������Ʈ ���� (���� �������� ������Ʈ ���¸� �����ϴ� �� ���)
    virtual void Update(float deltaTime) = 0;
    virtual void LateUpdate(float deltaTime) = 0;

public:
    bool isDead(void) { return m_bDead; }

public:
    SESSION* m_pSession;

protected:
    UINT16 m_x, m_y;
    bool m_bDead;
};
