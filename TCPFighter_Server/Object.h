#pragma once

// 기본 CObject 클래스 정의
class CObject {
public:
    constexpr CObject(UINT16 _x = 0, UINT16 _y = 0) noexcept : m_x(_x), m_y(_y) {}
    virtual ~CObject() = default;

    // 위치 설정 및 가져오기
    constexpr void SetPosition(UINT16 _x, UINT16 _y) {
        m_x = _x;
        m_y = _y;
    }

    constexpr void getPosition(UINT16& _x, UINT16& _y) const {
        _x = m_x;
        _y = m_y;
    }

    virtual void Move() = 0;

    // 업데이트 메서드 (서버 로직에서 오브젝트 상태를 갱신하는 데 사용)
    virtual void Update(float deltaTime) = 0;

protected:
    UINT16 m_x, m_y;
};
