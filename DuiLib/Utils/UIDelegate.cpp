#include "StdAfx.h"

namespace DuiLib {

CDelegateBase::CDelegateBase(void* pObject, void* pFn) 
{
    m_pObject = pObject;
    m_pFn = pFn; 
}

CDelegateBase::CDelegateBase(const CDelegateBase& rhs) 
{
    m_pObject = rhs.m_pObject;
    m_pFn = rhs.m_pFn; 
}

CDelegateBase::~CDelegateBase()
{

}

bool CDelegateBase::Equals(const CDelegateBase& rhs) const 
{    
    return m_pObject == rhs.m_pObject && GetFn() == rhs.GetFn();
}

bool CDelegateBase::operator() (void* param) 
{
    return Invoke(param); 
}

void* CDelegateBase::GetFn() const
{
    return m_pFn; 
}

void* CDelegateBase::GetObject() 
{
    return m_pObject; 
}

CEventSource::~CEventSource()
{
    for( int i = 0; i < m_aDelegates.GetSize(); i++ ) {
        CDelegateBase* pObject = static_cast<CDelegateBase*>(m_aDelegates[i]);
        if( pObject) delete pObject;
    }
}

CEventSource::operator bool()
{
    return m_aDelegates.GetSize() > 0;
}

void CEventSource::operator+= (const CDelegateBase& d)
{ 
    for( int i = 0; i < m_aDelegates.GetSize(); i++ ) {
        CDelegateBase* pObject = static_cast<CDelegateBase*>(m_aDelegates[i]);
        if( pObject && pObject->Equals(d) ) return;
    }

    m_aDelegates.Add(d.Copy());
}

void CEventSource::operator+= (FnType pFn)
{ 
    (*this) += MakeDelegate(pFn);
}

void CEventSource::operator-= (const CDelegateBase& d) 
{
    for( int i = 0; i < m_aDelegates.GetSize(); i++ ) {
        CDelegateBase* pObject = static_cast<CDelegateBase*>(m_aDelegates[i]);
        if( pObject && pObject->Equals(d) ) {
            delete pObject;
            m_aDelegates.Remove(i);
            return;
        }
    }
}
void CEventSource::operator-= (FnType pFn)
{ 
    (*this) -= MakeDelegate(pFn);
}

bool CEventSource::operator() (void* param)
{
    // 注意：若某 delegate 在回调内 operator-= 注销自身，Remove 会使后续元素左移、
    // ++i 跳过一个回调（duilib 广泛使用的已知设计取舍，暂不改变行为）。
    // 此处改为安全版本：删除发生时不递增，保证所有剩余 delegate 均被调用。
    int i = 0;
    while( i < m_aDelegates.GetSize() ) {
        CDelegateBase* pObject = static_cast<CDelegateBase*>(m_aDelegates[i]);
        if( pObject && !(*pObject)(param) ) return false;
        // 若回调内修改了容器（元素被移除或数量变化），当前下标不递增；
        // 否则正常推进。删除发生在 i 处时，新元素已滑到 i，同样不递增。
        if( i < m_aDelegates.GetSize() && static_cast<CDelegateBase*>(m_aDelegates[i]) == pObject )
            ++i;
    }
    return true;
}
void CEventSource::Clear()
{
	for( int i = 0; i < m_aDelegates.GetSize(); i++ ) {
		CDelegateBase* pObject = static_cast<CDelegateBase*>(m_aDelegates[i]);
		if( pObject) delete pObject;
	}
	m_aDelegates.Empty();
}
} // namespace DuiLib
