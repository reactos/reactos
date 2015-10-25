#ifndef __ATLCOLL_H__
#define __ATLCOLL_H__

#pragma once
#include "atlbase.h"


struct __POSITION
{
};
typedef __POSITION* POSITION;


namespace ATL
{

class CAtlPlex
{
public:
    CAtlPlex* m_Next;

#if (_AFX_PACKING >= 8)
    DWORD dwReserved[1];
#endif

    static inline CAtlPlex* Create(
        _Inout_ CAtlPlex*& Entry,
        _In_ size_t MaxElements,
        _In_ size_t ElementSize
        )
    {
        CAtlPlex* Block;

        ATLASSERT(MaxElements > 0);
        ATLASSERT(ElementSize > 0);

        size_t BufferSize = sizeof(CAtlPlex) + (MaxElements * ElementSize);

        void *Buffer = HeapAlloc(GetProcessHeap(), 0, BufferSize);
        if (Buffer == NULL) return NULL;

        Block = static_cast< CAtlPlex* >(Buffer);
        Block->m_Next = Entry;
        Entry = Block;

        return Block;
    }

    void* GetData()
    {
        return (this + 1);
    }

    inline void Destroy()
    {
        CAtlPlex* Block;
        CAtlPlex* Next;

        Block = this;
        while (Block != NULL)
        {
            Next = Block->m_Next;
            HeapFree(GetProcessHeap(), 0, Block);
            Block = Next;
        }
    }
};


template<typename T>
class CElementTraitsBase
{
public:
    typedef const T& INARGTYPE;
    typedef T& OUTARGTYPE;

    static void CopyElements(
        _Out_writes_all_(NumElements) T* Dest,
        _In_reads_(NumElements) const T* Source,
        _In_ size_t NumElements)
    {
        for (size_t i = 0; i < NumElements; i++)
        {
            Dest[i] = Source[i];
        }
    }

    static void RelocateElements(
        _Out_writes_all_(NumElements) T* Dest,
        _In_reads_(NumElements) T* Source,
        _In_ size_t NumElements)
    {
        memmove_s(Dest, NumElements * sizeof(T), Source, NumElements * sizeof(T));
    }
};

template<typename T>
class CDefaultCompareTraits
{
public:
    static bool CompareElements(
        _In_ const T& Val1,
        _In_ const T& Val2)
    {
        return (Val1 == Val2);
    }

    static int CompareElementsOrdered(
        _In_ const T& Val1,
        _In_ const T& Val2)
    {
        if (Val1 < Val2)
        {
            return -1;
        }
        else if (Val1 > Val2)
        {
            return 1;
        }

        return 0; // equal
    }
};

template<typename T>
class CDefaultElementTraits :
    public CElementTraitsBase<T>,
    public CDefaultCompareTraits<T>
{
};


template<typename T>
class CElementTraits :
    public CDefaultElementTraits<T>
{
};

template<typename E, class ETraits = CElementTraits<E>>
class CAtlList
{
private:
    typedef typename ETraits::INARGTYPE INARGTYPE;

private:
    class CNode :  public __POSITION
    {
    public:
        CNode* m_Next;
        CNode* m_Prev;
        E m_Element;

    public:
        CNode(INARGTYPE Element) :
            m_Element(Element)
        {
        }
    };

private:
    CAtlPlex* m_Blocks;
    UINT m_BlockSize;
    CNode* m_HeadNode;
    CNode* m_TailNode;
    CNode* m_FreeNode;
    size_t m_NumElements;

public:
    CAtlList(_In_ UINT nBlockSize = 10);
    ~CAtlList();

    size_t GetCount() const;
    bool IsEmpty() const;

    POSITION GetHeadPosition() const;
    POSITION GetTailPosition() const;

    E& GetNext(_Inout_ POSITION& pos);
    const E& GetNext(_Inout_ POSITION& pos) const;
    E& GetPrev(_Inout_ POSITION& pos);
    const E& GetPrev(_Inout_ POSITION& pos) const throw();

    E& GetAt(_In_ POSITION pos);
    const E& GetAt(_In_ POSITION pos) const;

    POSITION AddHead(INARGTYPE element);
    POSITION AddTail(INARGTYPE element);

    E RemoveHead();
    E RemoveTail();
    void RemoveAll();
    void RemoveAt(_In_ POSITION pos) throw();

    POSITION Find(
        INARGTYPE element,
        _In_opt_ POSITION posStartAfter = NULL) const;
    POSITION FindIndex(_In_ size_t iElement) const;

private:
    CNode* CreateNode(
        INARGTYPE element,
        _In_opt_ CNode* pPrev,
        _In_opt_ CNode* pNext
        );

    void FreeNode(
        _Inout_ CNode* pNode
        );

    CNode* GetFreeNode(
        );

};


//
// CAtlist public methods
//

template<typename E, class ETraits>
CAtlList< E, ETraits >::CAtlList(_In_ UINT nBlockSize) :
    m_Blocks(NULL),
    m_BlockSize(nBlockSize),
    m_HeadNode(NULL),
    m_TailNode(NULL),
    m_FreeNode(NULL),
    m_NumElements(0)
{
    ATLASSERT(nBlockSize > 0);
}

template<typename E, class ETraits>
CAtlList<E, ETraits >::~CAtlList(void)
{
    RemoveAll();
}

template<typename E, class ETraits>
inline size_t CAtlList< E, ETraits >::GetCount() const
{
    return m_NumElements;
}

template<typename E, class ETraits>
inline bool CAtlList< E, ETraits >::IsEmpty() const
{
    return (m_NumElements == 0);
}

template<typename E, class ETraits>
inline POSITION CAtlList<E, ETraits>::GetHeadPosition() const
{
    return (POSITION)m_HeadNode;
}

template<typename E, class ETraits>
inline POSITION CAtlList<E, ETraits>::GetTailPosition() const
{
    return (POSITION)m_TailNode;
}

template<typename E, class ETraits>
inline E& CAtlList< E, ETraits >::GetNext(_Inout_ POSITION& pos)
{
    CNode* Node = (CNode*)pos;
    pos = (POSITION)Node->m_Next;
    return Node->m_Element;
}

template<typename E, class ETraits>
inline const E& CAtlList< E, ETraits >::GetNext(_Inout_ POSITION& pos) const
{
    CNode* Node = (CNode*)pos;
    pos = (POSITION)Node->m_Next;
    return Node->m_Element;
}

template<typename E, class ETraits>
inline E& CAtlList< E, ETraits >::GetPrev(_Inout_ POSITION& pos)
{
    CNode* Node = (CNode*)pos;
    pos = (POSITION)Node->m_Prev;
    return Node->m_Element;
}

template<typename E, class ETraits>
inline const E& CAtlList< E, ETraits >::GetPrev(_Inout_ POSITION& pos) const
{
    CNode* Node = (CNode*)pos;
    pos = (POSITION)Node->m_Prev;
    return Node->m_Element;
}

template<typename E, class ETraits>
inline E& CAtlList< E, ETraits >::GetAt(_In_ POSITION pos)
{
    CNode* Node = (CNode*)pos;
    return Node->m_Element;
}

template<typename E, class ETraits>
inline const E& CAtlList< E, ETraits >::GetAt(_In_ POSITION pos) const
{
    CNode* Node = (CNode*)pos;
    return Node->m_Element;
}

template<typename E, class ETraits>
POSITION CAtlList<E, ETraits>::AddHead(INARGTYPE element)
{
    CNode* Node = CreateNode(element, NULL, m_HeadNode);
    if (m_HeadNode)
    {
        m_HeadNode->m_Prev = Node;
    }
    else
    {
        m_TailNode = Node;
    }
    m_HeadNode = Node;

    return (POSITION)Node;
}

template<typename E, class ETraits>
POSITION CAtlList<E, ETraits>::AddTail(INARGTYPE element)
{
    CNode* Node = CreateNode(element, m_TailNode, NULL);
    if (m_TailNode)
    {
        m_TailNode->m_Next = Node;
    }
    else
    {
        m_HeadNode = Node;
    }
    m_TailNode = Node;

    return (POSITION)Node;
}

template<typename E, class ETraits>
E CAtlList<E, ETraits>::RemoveHead()
{
    CNode* Node = m_HeadNode;
    E Element(Node->m_Element);

    m_HeadNode = Node->m_Next;
    if (m_HeadNode)
    {
        m_HeadNode->m_Prev = NULL;
    }
    else
    {
        m_TailNode = NULL;
    }
    FreeNode(Node);

    return Element;
}

template<typename E, class ETraits>
E CAtlList<E, ETraits>::RemoveTail()
{
    CNode* Node = m_TailNode;
    E Element(Node->m_Element);

    m_TailNode = Node->m_Prev;
    if (m_TailNode)
    {
        m_TailNode->m_Next = NULL;
    }
    else
    {
        m_HeadNode = NULL;
    }
    FreeNode(Node);

    return Element;
}

template<typename E, class ETraits>
void CAtlList<E, ETraits >::RemoveAll()
{
    while (m_NumElements > 0)
    {
        CNode* Node = m_HeadNode;
        m_HeadNode = m_HeadNode->m_Next;
        FreeNode(Node);
    }

    m_HeadNode = NULL;
    m_TailNode = NULL;
    m_FreeNode = NULL;

    if (m_Blocks)
    {
        m_Blocks->Destroy();
        m_Blocks = NULL;
    }
}

template<typename E, class ETraits>
void CAtlList<E, ETraits >::RemoveAt(_In_ POSITION pos)
{
    ATLASSERT(pos != NULL);

    CNode* OldNode = (CNode*)pos;
    if (OldNode == m_HeadNode)
    {
        m_HeadNode = OldNode->m_Next;
    }
    else
    {
        OldNode->m_Prev->m_Next = OldNode->m_Next;
    }
    if (OldNode == m_TailNode)
    {
        m_TailNode = OldNode->m_Prev;
    }
    else
    {
        OldNode->m_Next->m_Prev = OldNode->m_Prev;
    }
    FreeNode(OldNode);
}

template<typename E, class ETraits>
POSITION CAtlList< E, ETraits >::Find(
    INARGTYPE element,
    _In_opt_ POSITION posStartAfter) const
{
    CNode* Node = (CNode*)posStartAfter;
    if (Node == NULL)
    {
        Node = m_HeadNode;
    }
    else
    {
        Node = Node->m_Next;
    }

    for (; Node != NULL; Node = Node->m_Next)
    {
        if (ETraits::CompareElements(Node->m_Element, element))
            return (POSITION)Node;
    }

    return NULL;
}

template<typename E, class ETraits>
POSITION CAtlList< E, ETraits >::FindIndex(_In_ size_t iElement) const
{
    if (iElement >= m_NumElements)
        return NULL;

    if (m_HeadNode == NULL)
        return NULL;

    CNode* Node = m_HeadNode;
    for (size_t i = 0; i < iElement; i++)
    {
        Node = Node->m_Next;
    }

    return (POSITION)Node;
}


//
// CAtlist private methods
//

template<typename E, class ETraits>
typename CAtlList<E, ETraits>::CNode* CAtlList<E, ETraits>::CreateNode(
    INARGTYPE element,
    _In_opt_ CNode* Prev,
    _In_opt_ CNode* Next
    )
{
    GetFreeNode();

    CNode* NewNode = GetFreeNode();
    CNode* NextFree = m_FreeNode->m_Next;

    NewNode = new CNode(element);

    m_FreeNode = NextFree;
    NewNode->m_Prev = Prev;
    NewNode->m_Next = Next;
    m_NumElements++;

    return NewNode;
}

template<typename E, class ETraits>
void CAtlList<E, ETraits>::FreeNode(
    _Inout_ CNode* pNode
    )
{
    pNode->~CNode();
    pNode->m_Next = m_FreeNode;
    m_FreeNode = pNode;

    m_NumElements--;
    if (m_NumElements == 0)
    {
        RemoveAll();
    }
}

template<typename E, class ETraits>
typename CAtlList<E, ETraits>::CNode* CAtlList< E, ETraits>::GetFreeNode()
{
    if (m_FreeNode)
    {
        return m_FreeNode;
    }

    CAtlPlex* Block = CAtlPlex::Create(m_Blocks, m_BlockSize, sizeof(CNode));
    if (Block == NULL)
    {
        throw(E_OUTOFMEMORY);
    }
    
    CNode* Node = (CNode*)Block->GetData();
    Node += (m_BlockSize - 1);
    for (int i = m_BlockSize - 1; i >= 0; i--)
    {
        Node->m_Next = m_FreeNode;
        m_FreeNode = Node;
        Node--;
    }

    return m_FreeNode;
}

}

#endif