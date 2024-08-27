#pragma once

#include "Object.h"

#define FLAG_MOVING         0x01
#define FLAG_DEAD           0x02

class CPlayer : public CObject {
public:
    explicit CPlayer(UINT16 _x, UINT16 _y, UINT8 _direction, UINT8 _hp) noexcept;
    virtual ~CPlayer();

    // 기타 멤버 함수 선언
    virtual void Update(float deltaTime) override;
    virtual void Move(void) override;

public:
    // Getter 함수
    constexpr UINT8 GetHp() const { return m_hp; }
    constexpr UINT8 GetDirection() const { return m_direction; }
    constexpr UINT8 GetFacingDirection() const { return m_facingDirection; }

    // Setter 함수
    void SetHp(int _hp) { m_hp = _hp; }
    void SetDirection(int _direction);
    void Damaged(int _hp);

public:
    void SetFlag(UINT8 flag, bool bOnOff);
    void ToggleFlag(UINT8 flag);
    bool isBitSet(UINT8 flag);

public:
    void SetFlagField(UINT8* pField);

private:
    UINT8 m_hp;          // 체력
    UINT8 m_direction;   // 방향
    UINT8 m_facingDirection;    // 캐릭터가 바라보고 있는 방향
    UINT8 m_speedX = 3;
    UINT8 m_speedY = 2;
    UINT8* m_pFlagField;  // 행동 관련 flagField;
};