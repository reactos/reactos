///////////////////////////////////////////////////////////////////////
//
//  TVSTACK.H
//
//
//  Copyright 1986-1996 Microsoft Corporation. All Rights Reserved.
///////////////////////////////////////////////////////////////////////


#define StackSize 64

class CTIStack
{

public:
    CTIStack()  { m_ptos = m_ahti; m_pMax = m_ahti + StackSize - 1;}
    ~CTIStack() {};

    BOOL Push(HTREEITEM hti);
    HTREEITEM Pop(void);
    BOOL IsEmpty(void);

private:
    HTREEITEM m_ahti[StackSize];
    HTREEITEM *m_ptos;
    HTREEITEM *m_pMax;
};

inline BOOL CTIStack::Push(HTREEITEM hti)
{
    Assert(m_ptos >= m_ahti);
    Assert(m_ptos <= m_pMax);

    if(m_ptos == m_pMax)
        return FALSE;

    *m_ptos++ = hti;
    return TRUE;
}

inline BOOL CTIStack::IsEmpty(void)
{
    Assert(m_ptos >= m_ahti);
    Assert(m_ptos <= m_pMax);
    
    return (m_ptos == m_ahti);
}

inline HTREEITEM CTIStack::Pop(void)
{
    Assert(m_ptos > m_ahti);
    Assert(m_ptos <= m_pMax);

    if(m_ptos == m_ahti)
        return NULL;
        
    return *--m_ptos;
}

