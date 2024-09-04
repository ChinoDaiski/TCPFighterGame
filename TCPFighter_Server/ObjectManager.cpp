#include "pch.h"
#include "ObjectManager.h"

#include "Session.h"
#include "SessionManager.h"

CObjectManager::CObjectManager() noexcept
{
}

CObjectManager::~CObjectManager() noexcept
{
    for (auto& Object : m_ObjectList)
    {
        delete Object;
    }

    m_ObjectList.clear();
}

void CObjectManager::Update(float deltatime)
{
	for (auto& Object : m_ObjectList)
		Object->Update(deltatime);
}

void CObjectManager::LateUpdate(float deltatime)
{
    auto it = m_ObjectList.begin();
    while (it != m_ObjectList.end())
    {
        // ������Ʈ�� ��Ȱ��ȭ �Ǿ��ٸ�
        if ((*it)->isDead() || (*it)->m_pSession->isAlive == false)
        {
            if ((*it)->m_pSession->isAlive != false)
                CSessionManager::NotifyClientDisconnected((*it)->m_pSession); // ������ �׾����� �˸�
            
            delete (*it);                   // �÷��̾� ����
            it = m_ObjectList.erase(it);    // ����Ʈ���� iter ����
        }
        // Ȱ�� ���̶��
        else
        {
            (*it)->LateUpdate(deltatime);
            ++it;
        }
    }
}

void CObjectManager::RegisterObject(CObject* pObject, SESSION* pSession)
{
    pObject->m_pSession = pSession;
    m_ObjectList.push_back(pObject);
}
