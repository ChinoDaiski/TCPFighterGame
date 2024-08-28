#pragma once

#include "Singleton.h"
#include "Object.h"

class CObjectManager : public SingletonBase<CObjectManager> {
private:
    friend class SingletonBase<CObjectManager>;

public:
    explicit CObjectManager() noexcept{}
    ~CObjectManager() noexcept {}
    
    // ���� �����ڿ� ���� �����ڸ� �����Ͽ� ���� ����
    CObjectManager(const CObjectManager&) = delete;
    CObjectManager& operator=(const CObjectManager&) = delete;

private:
    std::list<CObject*> m_objList;
};