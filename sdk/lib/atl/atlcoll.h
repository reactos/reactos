#ifndef __ATLCOLL_H__
#define __ATLCOLL_H__

#pragma once
#include "atlbase.h"
#include "atlexcept.h"

// FIXME: We need to include <new> for placement new, but that would mean everyone using atl
// would also need to set the option 'WITH_STL'..
// For now we just copy the definition here, under a guard..
#ifndef _NEW
inline void* operator new (size_t size, void* ptr) noexcept { return ptr; }
inline void operator delete (void* ptr, void* voidptr2) noexcept { }
#endif


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
        // A simple memmove works for most of the types.
        // You'll have to override this for types that have pointers to their
        // own members.

#if defined(__GNUC__) && __GNUC__ >= 8
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif
        memmove(Dest, Source, NumElements * sizeof(T));
#if defined(__GNUC__) && __GNUC__ >= 8
    #pragma GCC diagnostic pop
#endif
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


template<typename T, class Allocator = CCRTAllocator>
class CHeapPtrElementTraits :
    public CDefaultElementTraits< CHeapPtr<T, Allocator> >
{
public:
    typedef CHeapPtr<T, Allocator>& INARGTYPE;
    typedef T*& OUTARGTYPE;
};



template<typename E, class ETraits = CElementTraits<E> >
class CAtlArray
{
public:
    typedef typename ETraits::INARGTYPE INARGTYPE;
    typedef typename ETraits::OUTARGTYPE OUTARGTYPE;

private:
    E* m_pData;
    size_t m_Size;
    size_t m_AllocatedSize;
    size_t m_GrowBy;


#pragma push_macro("new")
#undef new

    void CreateItems(E* pData, size_t Size)
    {
        for (size_t n = 0; n < Size; ++n)
        {
            ::new (pData + n) E;
        }
    }

#pragma pop_macro("new")

    void DestructItems(E* pData, size_t Size)
    {
        for (size_t n = 0; n < Size; ++n)
        {
            pData[n].~E();
        }
    }

    bool GrowAllocatedData(size_t nNewSize)
    {
        if (m_pData)
        {
            size_t addSize = m_GrowBy > 0 ? m_GrowBy : m_AllocatedSize / 2;
            size_t allocSize = m_AllocatedSize + addSize;
            if (allocSize < nNewSize)
                allocSize = nNewSize;

            E* pData = (E*)malloc(nNewSize * sizeof(E));

            if (pData == NULL)
            {
                return false;
            }

            // Copy the objects over (default implementation will just move them without calling anything
            ETraits::RelocateElements(pData, m_pData, m_Size);

            free(m_pData);
            m_pData = pData;
            m_AllocatedSize = nNewSize;
        }
        else
        {
            // We need to allocate a new buffer
            size_t allocSize = m_GrowBy > nNewSize ? m_GrowBy : nNewSize;
            m_pData = (E*)malloc(allocSize * sizeof(E));
            if (m_pData == NULL)
            {
                return false;
            }
            m_AllocatedSize = allocSize;
        }
        return true;
    }

    /* The CAtlArray class does not support construction by copy */
private:
    CAtlArray(_In_ const CAtlArray&);
    CAtlArray& operator=(_In_ const CAtlArray&);

public:
    CAtlArray();
    ~CAtlArray();

    size_t Add(INARGTYPE element);
    size_t Add();

    bool SetCount(size_t nNewSize, int nGrowBy = - 1);
    size_t GetCount() const;

    E& operator[](size_t ielement);
    const E& operator[](size_t ielement) const;

    E& GetAt(size_t iElement);
    const E& GetAt(size_t iElement) const;

    E* GetData();
    const E* GetData() const;


    //FIXME: Most of this class is missing!
};

//
// CAtlArray public methods
//

template<typename E, class ETraits>
CAtlArray< E, ETraits >::CAtlArray()
    : m_pData(NULL)
    , m_Size(0)
    , m_AllocatedSize(0)
    , m_GrowBy(0)
{
}

template<typename E, class ETraits>
CAtlArray< E, ETraits >::~CAtlArray()
{
    // Destroy all items
    SetCount(0, -1);
}

#pragma push_macro("new")
#undef new

template<typename E, class ETraits>
size_t CAtlArray<E, ETraits>::Add(INARGTYPE element)
{
    if (m_Size >= m_AllocatedSize)
    {
        if (!GrowAllocatedData(m_Size + 1))
        {
            AtlThrow(E_OUTOFMEMORY);
        }
    }

    ::new (m_pData + m_Size) E(element);
    m_Size++;

    return m_Size - 1;
}

#pragma pop_macro("new")

template<typename E, class ETraits>
size_t CAtlArray<E, ETraits>::Add()
{
    if (!SetCount(m_Size + 1))
    {
        AtlThrow(E_OUTOFMEMORY);
    }

    return m_Size - 1;
}

template<typename E, class ETraits>
bool CAtlArray<E, ETraits>::SetCount(size_t nNewSize, int nGrowBy /*= -1*/)
{

    if (nGrowBy > -1)
    {
        m_GrowBy = (size_t)nGrowBy;
    }

    if (nNewSize == m_Size)
    {
        // Do nothing
    }
    else if (nNewSize == 0)
    {
        if (m_pData)
        {
            DestructItems(m_pData, m_Size);
            m_pData = NULL;
        }
        m_Size = m_AllocatedSize = NULL;
    }
    else if (nNewSize < m_AllocatedSize)
    {
        if (nNewSize > m_Size)
        {
            CreateItems(m_pData + m_Size, nNewSize - m_Size);
        }
        else
        {
            DestructItems(m_pData + nNewSize, m_Size - nNewSize);
        }
        m_Size = nNewSize;
    }
    else
    {
        if (!GrowAllocatedData(nNewSize))
        {
            return false;
        }

        CreateItems(m_pData + m_Size, nNewSize - m_Size);
        m_Size = nNewSize;
    }

    return true;
}

template<typename E, class ETraits>
size_t CAtlArray<E, ETraits>::GetCount() const
{
    return m_Size;
}

template<typename E, class ETraits>
E& CAtlArray<E, ETraits>::operator[](size_t iElement)
{
    ATLASSERT(iElement < m_Size);

    return m_pData[iElement];
}

template<typename E, class ETraits>
const E& CAtlArray<E, ETraits>::operator[](size_t iElement) const
{
    ATLASSERT(iElement < m_Size);

    return m_pData[iElement];
}

template<typename E, class ETraits>
E& CAtlArray<E, ETraits>::GetAt(size_t iElement)
{
    ATLASSERT(iElement < m_Size);

    return m_pData[iElement];
}

template<typename E, class ETraits>
const E& CAtlArray<E, ETraits>::GetAt(size_t iElement) const
{
    ATLASSERT(iElement < m_Size);

    return m_pData[iElement];
}

template<typename E, class ETraits>
E* CAtlArray<E, ETraits>::GetData()
{
    return m_pData;
}

template<typename E, class ETraits>
const E* CAtlArray<E, ETraits>::GetData() const
{
    return m_pData;
}


template<typename E, class ETraits = CElementTraits<E> >
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

    /* The CNode class does not support construction by copy */
    private:
        CNode(_In_ const CNode&);
        CNode& operator=(_In_ const CNode&);
    };

private:
    CAtlPlex* m_Blocks;
    UINT m_BlockSize;
    CNode* m_HeadNode;
    CNode* m_TailNode;
    CNode* m_FreeNode;
    size_t m_NumElements;

/* The CAtlList class does not support construction by copy */
private:
    CAtlList(_In_ const CAtlList&);
    CAtlList& operator=(_In_ const CAtlList&);

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
    const E& GetPrev(_Inout_ POSITION& pos) const;

    E& GetAt(_In_ POSITION pos);
    const E& GetAt(_In_ POSITION pos) const;

    POSITION AddHead(INARGTYPE element);
    POSITION AddTail(INARGTYPE element);

    void AddHeadList(_In_ const CAtlList<E, ETraits>* plNew);
    void AddTailList(_In_ const CAtlList<E, ETraits>* plNew);

    E RemoveHead();
    E RemoveTail();

    POSITION InsertBefore(_In_ POSITION pos, INARGTYPE element);
    POSITION InsertAfter(_In_ POSITION pos, INARGTYPE element);

    void RemoveAll();
    void RemoveAt(_In_ POSITION pos);

    POSITION Find(
        INARGTYPE element,
        _In_opt_ POSITION posStartAfter = NULL) const;
    POSITION FindIndex(_In_ size_t iElement) const;

    void SwapElements(POSITION pos1, POSITION pos2);

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

template <typename E, class ETraits>
void CAtlList<E, ETraits>::AddHeadList(_In_ const CAtlList<E, ETraits>* plNew)
{
    ATLASSERT(plNew != NULL && plNew != this);
    POSITION pos = plNew->GetTailPosition();
    while (pos)
        AddHead(plNew->GetPrev(pos));
}

template <typename E, class ETraits>
void CAtlList<E, ETraits>::AddTailList(_In_ const CAtlList<E, ETraits>* plNew)
{
    ATLASSERT(plNew != NULL && plNew != this);
    POSITION pos = plNew->GetHeadPosition();
    while (pos)
        AddTail(plNew->GetNext(pos));
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
POSITION CAtlList<E, ETraits >::InsertBefore(_In_ POSITION pos, _In_ INARGTYPE element)
{
    if (pos == NULL)
        return AddHead(element);

    CNode* OldNode = (CNode*)pos;
    CNode* Node = CreateNode(element, OldNode->m_Prev, OldNode);

    if (OldNode->m_Prev != NULL)
    {
        OldNode->m_Prev->m_Next = Node;
    }
    else
    {
        m_HeadNode = Node;
    }
    OldNode->m_Prev = Node;

    return (POSITION)Node;
}

template<typename E, class ETraits>
POSITION CAtlList<E, ETraits >::InsertAfter(_In_ POSITION pos, _In_ INARGTYPE element)
{
    if (pos == NULL)
        return AddTail(element);

    CNode* OldNode = (CNode*)pos;
    CNode* Node = CreateNode(element, OldNode, OldNode->m_Next);

    if (OldNode->m_Next != NULL)
    {
        OldNode->m_Next->m_Prev = Node;
    }
    else
    {
        m_TailNode = Node;
    }
    OldNode->m_Next = Node;

    return (POSITION)Node;
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

template<typename E, class ETraits>
void CAtlList< E, ETraits >::SwapElements(POSITION pos1, POSITION pos2)
{
    if (pos1 == pos2)
        return;


    CNode *node1 = (CNode *)pos1;
    CNode *node2 = (CNode *)pos2;

    CNode *tmp = node1->m_Prev;
    node1->m_Prev = node2->m_Prev;
    node2->m_Prev = tmp;

    if (node1->m_Prev)
        node1->m_Prev->m_Next = node1;
    else
        m_HeadNode = node1;

    if (node2->m_Prev)
        node2->m_Prev->m_Next = node2;
    else
        m_HeadNode = node2;

    tmp = node1->m_Next;
    node1->m_Next = node2->m_Next;
    node2->m_Next = tmp;

    if (node1->m_Next)
        node1->m_Next->m_Prev = node1;
    else
        m_TailNode = node1;

    if (node2->m_Next)
        node2->m_Next->m_Prev = node2;
    else
        m_TailNode = node2;
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

    CNode* NewNode = m_FreeNode;
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
        AtlThrowImp(E_OUTOFMEMORY);
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


template<typename E, class Allocator = CCRTAllocator >
class CHeapPtrList :
    public CAtlList<CHeapPtr<E, Allocator>, CHeapPtrElementTraits<E, Allocator> >
{
public:
    CHeapPtrList(_In_ UINT nBlockSize = 10) :
        CAtlList<CHeapPtr<E, Allocator>, CHeapPtrElementTraits<E, Allocator> >(nBlockSize)
    {
    }

private:
    CHeapPtrList(const CHeapPtrList&);
    CHeapPtrList& operator=(const CHeapPtrList*);
};


}

#endif
