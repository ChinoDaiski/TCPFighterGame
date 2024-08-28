#pragma once

#include "Singleton.h"
#include "Object.h"

class CObjectManager : public SingletonBase<CObjectManager> {
private:
    friend class SingletonBase<CObjectManager>;

public:
    explicit CObjectManager() noexcept{}
    ~CObjectManager() noexcept {}
    
    // 복사 생성자와 대입 연산자를 삭제하여 복사 방지
    CObjectManager(const CObjectManager&) = delete;
    CObjectManager& operator=(const CObjectManager&) = delete;

private:
    std::list<CObject*> m_objList;
};