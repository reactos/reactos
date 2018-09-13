#ifndef __CLIST_HPP__
#define __CLIST_HPP__

#include <limits.h>
#include <string.h>

// "Borrowed" from MFC
#ifdef _DEBUG
#define ASSERT_VALID(p) AssertValid()
#else
#define ASSERT_VALID(p) ((void)0)
#endif

#ifndef ASSERT
#define ASSERT(x)
#endif

#ifndef AFXAPI
    #define AFXAPI __stdcall
#endif

typedef void* LISTPOSITION;  // abstract iteration position

#ifdef __cplusplus

BOOL AFXAPI AfxIsValidAddress(const void* lp,
            UINT nBytes, BOOL bReadWrite = TRUE);


struct CPlex    // warning variable length structure
{
    CPlex* pNext;
    UINT nMax;
    UINT nCur;
    /* BYTE data[maxNum*elementSize]; */

    void* data() { return this+1; }

    static CPlex* PASCAL Create(CPlex*& head, UINT nMax, UINT cbElement);
            // like 'calloc' but no zero fill
            // may throw memory exceptions

    void FreeDataChain();       // free this one and links
};

#ifdef _global_helper_
/////////////////////////////////////////////////////////////////////////////
// global helpers (can be overridden)

template<class TYPE>
inline void AFXAPI ConstructElements(TYPE* pElements, int nCount)
{
    ASSERT(nCount == 0 ||
        AfxIsValidAddress(pElements, nCount * sizeof(TYPE)));

    // default is bit-wise zero initialization
    memset((void*)pElements, 0, nCount * sizeof(TYPE));
}

template<class TYPE>
inline void AFXAPI DestructElements(TYPE* pElements, int nCount)
{
    ASSERT(nCount == 0 ||
        AfxIsValidAddress(pElements, nCount * sizeof(TYPE)));

        pElements;
        nCount;
}

/* No need to serialize the list
template<class TYPE>
void AFXAPI SerializeElements(CArchive& ar, TYPE* pElements, int nCount)
{
    ASSERT(nCount == 0 ||
        AfxIsValidAddress(pElements, nCount * sizeof(TYPE)));

    // default is bit-wise read/write
    if (ar.IsStoring())
        ar.Write((void*)pElements, nCount * sizeof(TYPE));
    else
        ar.Read((void*)pElements, nCount * sizeof(TYPE));
}
*/

/* No need for debug dumping
#ifdef _DEBUG
template<class TYPE>
void AFXAPI DumpElements(CDumpContext& dc, const TYPE* pElements, int nCount)
{
    ASSERT(nCount == 0 ||
        AfxIsValidAddress(pElements, nCount * sizeof(TYPE)));
    dc; // not used
    pElements;  // not used
    nCount; // not used

    // default does nothing
}
#endif
*/

template<class TYPE, class ARG_TYPE>
BOOL AFXAPI CompareElements(const TYPE* pElement1, const ARG_TYPE* pElement2)
{
    ASSERT(AfxIsValidAddress(pElement1, sizeof(TYPE)));
    ASSERT(AfxIsValidAddress(pElement2, sizeof(ARG_TYPE)));

    return *pElement1 == *pElement2;
}

template<class ARG_KEY>
inline UINT AFXAPI HashKey(ARG_KEY key)
{
    // default identity hash - works for most primitive values
    return ((UINT)(void*)(DWORD)key) >> 4;
}
#endif // _global_helper_

/////////////////////////////////////////////////////////////////////////////
// CList<TYPE, ARG_TYPE>

template<class TYPE, class ARG_TYPE>
class CList
{
#ifndef unix
protected:
#else
    // If this was not made public we get complier warnings
    // that CList<T,A>::CNode is not accessible from file scope
    // which means functions cant return CNode pointers
public:
#endif /* unix */
    struct CNode
    {
        CNode* pNext;
        CNode* pPrev;
        TYPE data;
    };
public:
// Construction
    CList(int nBlockSize = 10);

// Attributes (head and tail)
    // count of elements
    int GetCount() const;
    BOOL IsEmpty() const;

    // peek at head or tail
    TYPE& GetHead();
    TYPE GetHead() const;
    TYPE& GetTail();
    TYPE GetTail() const;

// Operations
    // get head or tail (and remove it) - don't call on empty list !
    TYPE RemoveHead();
    TYPE RemoveTail();

    // add before head or after tail
    LISTPOSITION AddHead(ARG_TYPE newElement);
    LISTPOSITION AddTail(ARG_TYPE newElement);

    // add another list of elements before head or after tail
    void AddHead(CList* pNewList);
    void AddTail(CList* pNewList);

    // remove all elements
    void RemoveAll();

    // iteration
    LISTPOSITION GetHeadPosition() const;
    LISTPOSITION GetTailPosition() const;
    TYPE& GetNext(LISTPOSITION& rPosition); // return *Position++
    TYPE GetNext(LISTPOSITION& rPosition) const; // return *Position++
    TYPE& GetPrev(LISTPOSITION& rPosition); // return *Position--
    TYPE GetPrev(LISTPOSITION& rPosition) const; // return *Position--

    // getting/modifying an element at a given position
    TYPE& GetAt(LISTPOSITION position);
    TYPE GetAt(LISTPOSITION position) const;
    void SetAt(LISTPOSITION pos, ARG_TYPE newElement);
    void RemoveAt(LISTPOSITION position);

    // inserting before or after a given position
    LISTPOSITION InsertBefore(LISTPOSITION position, ARG_TYPE newElement);
    LISTPOSITION InsertAfter(LISTPOSITION position, ARG_TYPE newElement);

    // helper functions (note: O(n) speed)
    LISTPOSITION Find(ARG_TYPE searchValue, LISTPOSITION startAfter = NULL) const;
        // defaults to starting at the HEAD, return NULL if not found
    LISTPOSITION FindIndex(int nIndex) const;
        // get the 'nIndex'th element (may return NULL)

// Implementation
protected:
    CNode* m_pNodeHead;
    CNode* m_pNodeTail;
    int m_nCount;
    CNode* m_pNodeFree;
    struct CPlex* m_pBlocks;
    int m_nBlockSize;

    CNode* NewNode(CNode*, CNode*);
    void FreeNode(CNode*);

public:
    ~CList();
//  void Serialize(CArchive&);
#ifdef _DEBUG
//  void Dump(CDumpContext&) const;
    void AssertValid() const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// CList<TYPE, ARG_TYPE> inline functions

template<class TYPE, class ARG_TYPE>
inline int CList<TYPE, ARG_TYPE>::GetCount() const
    { return m_nCount; }
template<class TYPE, class ARG_TYPE>
inline BOOL CList<TYPE, ARG_TYPE>::IsEmpty() const
    { return m_nCount == 0; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CList<TYPE, ARG_TYPE>::GetHead()
    { ASSERT(m_pNodeHead != NULL);
        return m_pNodeHead->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE CList<TYPE, ARG_TYPE>::GetHead() const
    { ASSERT(m_pNodeHead != NULL);
        return m_pNodeHead->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CList<TYPE, ARG_TYPE>::GetTail()
    { ASSERT(m_pNodeTail != NULL);
        return m_pNodeTail->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE CList<TYPE, ARG_TYPE>::GetTail() const
    { ASSERT(m_pNodeTail != NULL);
        return m_pNodeTail->data; }
template<class TYPE, class ARG_TYPE>
inline LISTPOSITION CList<TYPE, ARG_TYPE>::GetHeadPosition() const
    { return (LISTPOSITION) m_pNodeHead; }
template<class TYPE, class ARG_TYPE>
inline LISTPOSITION CList<TYPE, ARG_TYPE>::GetTailPosition() const
    { return (LISTPOSITION) m_pNodeTail; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CList<TYPE, ARG_TYPE>::GetNext(LISTPOSITION& rPosition) // return *Position++
    { CNode* pNode = (CNode*) rPosition;
        ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
        rPosition = (LISTPOSITION) pNode->pNext;
        return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE CList<TYPE, ARG_TYPE>::GetNext(LISTPOSITION& rPosition) const // return *Position++
    { CNode* pNode = (CNode*) rPosition;
        ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
        rPosition = (LISTPOSITION) pNode->pNext;
        return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CList<TYPE, ARG_TYPE>::GetPrev(LISTPOSITION& rPosition) // return *Position--
    { CNode* pNode = (CNode*) rPosition;
        ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
        rPosition = (LISTPOSITION) pNode->pPrev;
        return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE CList<TYPE, ARG_TYPE>::GetPrev(LISTPOSITION& rPosition) const // return *Position--
    { CNode* pNode = (CNode*) rPosition;
        ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
        rPosition = (LISTPOSITION) pNode->pPrev;
        return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CList<TYPE, ARG_TYPE>::GetAt(LISTPOSITION position)
    { CNode* pNode = (CNode*) position;
        ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
        return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE CList<TYPE, ARG_TYPE>::GetAt(LISTPOSITION position) const
    { CNode* pNode = (CNode*) position;
        ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
        return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline void CList<TYPE, ARG_TYPE>::SetAt(LISTPOSITION pos, ARG_TYPE newElement)
    { CNode* pNode = (CNode*) pos;
        ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
        pNode->data = newElement; }

template<class TYPE, class ARG_TYPE>
CList<TYPE, ARG_TYPE>::CList(int nBlockSize)
{
    ASSERT(nBlockSize > 0);

    m_nCount = 0;
    m_pNodeHead = m_pNodeTail = m_pNodeFree = NULL;
    m_pBlocks = NULL;
    m_nBlockSize = nBlockSize;
}

template<class TYPE, class ARG_TYPE>
void CList<TYPE, ARG_TYPE>::RemoveAll()
{
    ASSERT_VALID(this);

    // destroy elements
    CNode* pNode;
    for (pNode = m_pNodeHead; pNode != NULL; pNode = pNode->pNext)
        DestructElements(&pNode->data, 1);

    m_nCount = 0;
    m_pNodeHead = m_pNodeTail = m_pNodeFree = NULL;
    m_pBlocks->FreeDataChain();
    m_pBlocks = NULL;
}

template<class TYPE, class ARG_TYPE>
CList<TYPE, ARG_TYPE>::~CList()
{
    RemoveAll();
    ASSERT(m_nCount == 0);
}

/////////////////////////////////////////////////////////////////////////////
// Node helpers
//
// Implementation note: CNode's are stored in CPlex blocks and
//  chained together. Free blocks are maintained in a singly linked list
//  using the 'pNext' member of CNode with 'm_pNodeFree' as the head.
//  Used blocks are maintained in a doubly linked list using both 'pNext'
//  and 'pPrev' as links and 'm_pNodeHead' and 'm_pNodeTail'
//   as the head/tail.
//
// We never free a CPlex block unless the List is destroyed or RemoveAll()
//  is used - so the total number of CPlex blocks may grow large depending
//  on the maximum past size of the list.
//

template<class TYPE, class ARG_TYPE>
CList<TYPE, ARG_TYPE>::CNode*
CList<TYPE, ARG_TYPE>::NewNode(CList::CNode* pPrev, CList::CNode* pNext)
{
    if (m_pNodeFree == NULL)
    {
        // add another block
        CPlex* pNewBlock = CPlex::Create(m_pBlocks, m_nBlockSize,
                 sizeof(CNode));

        // chain them into free list
        CNode* pNode = (CNode*) pNewBlock->data();
        // free in reverse order to make it easier to debug
        pNode += m_nBlockSize - 1;
        for (int i = m_nBlockSize-1; i >= 0; i--, pNode--)
        {
            pNode->pNext = m_pNodeFree;
            m_pNodeFree = pNode;
        }
    }
    ASSERT(m_pNodeFree != NULL);  // we must have something

    CList::CNode* pNode = m_pNodeFree;
    m_pNodeFree = m_pNodeFree->pNext;
    pNode->pPrev = pPrev;
    pNode->pNext = pNext;
    m_nCount++;
    ASSERT(m_nCount > 0);  // make sure we don't overflow

    ConstructElements(&pNode->data, 1);
    return pNode;
}

template<class TYPE, class ARG_TYPE>
void CList<TYPE, ARG_TYPE>::FreeNode(CList::CNode* pNode)
{
    DestructElements(&pNode->data, 1);
    pNode->pNext = m_pNodeFree;
    m_pNodeFree = pNode;
    m_nCount--;
    ASSERT(m_nCount >= 0);  // make sure we don't underflow
}

template<class TYPE, class ARG_TYPE>
LISTPOSITION CList<TYPE, ARG_TYPE>::AddHead(ARG_TYPE newElement)
{
    ASSERT_VALID(this);

    CNode* pNewNode = NewNode(NULL, m_pNodeHead);
    pNewNode->data = newElement;
    if (m_pNodeHead != NULL)
        m_pNodeHead->pPrev = pNewNode;
    else
        m_pNodeTail = pNewNode;
    m_pNodeHead = pNewNode;
    return (LISTPOSITION) pNewNode;
}

template<class TYPE, class ARG_TYPE>
LISTPOSITION CList<TYPE, ARG_TYPE>::AddTail(ARG_TYPE newElement)
{
    ASSERT_VALID(this);

    CNode* pNewNode = NewNode(m_pNodeTail, NULL);
    pNewNode->data = newElement;
    if (m_pNodeTail != NULL)
        m_pNodeTail->pNext = pNewNode;
    else
        m_pNodeHead = pNewNode;
    m_pNodeTail = pNewNode;
    return (LISTPOSITION) pNewNode;
}

template<class TYPE, class ARG_TYPE>
void CList<TYPE, ARG_TYPE>::AddHead(CList* pNewList)
{
    ASSERT_VALID(this);

    ASSERT(pNewList != NULL);
    ASSERT_VALID(pNewList);

    // add a list of same elements to head (maintain order)
    LISTPOSITION pos = pNewList->GetTailPosition();
    while (pos != NULL)
        AddHead(pNewList->GetPrev(pos));
}

template<class TYPE, class ARG_TYPE>
void CList<TYPE, ARG_TYPE>::AddTail(CList* pNewList)
{
    ASSERT_VALID(this);
    ASSERT(pNewList != NULL);
    ASSERT_VALID(pNewList);

    // add a list of same elements
    LISTPOSITION pos = pNewList->GetHeadPosition();
    while (pos != NULL)
        AddTail(pNewList->GetNext(pos));
}

template<class TYPE, class ARG_TYPE>
TYPE CList<TYPE, ARG_TYPE>::RemoveHead()
{
    ASSERT_VALID(this);
    ASSERT(m_pNodeHead != NULL);  // don't call on empty list !!!
    ASSERT(AfxIsValidAddress(m_pNodeHead, sizeof(CNode)));

    CNode* pOldNode = m_pNodeHead;
    TYPE returnValue = pOldNode->data;

    m_pNodeHead = pOldNode->pNext;
    if (m_pNodeHead != NULL)
        m_pNodeHead->pPrev = NULL;
    else
        m_pNodeTail = NULL;
    FreeNode(pOldNode);
    return returnValue;
}

template<class TYPE, class ARG_TYPE>
TYPE CList<TYPE, ARG_TYPE>::RemoveTail()
{
    ASSERT_VALID(this);
    ASSERT(m_pNodeTail != NULL);  // don't call on empty list !!!
    ASSERT(AfxIsValidAddress(m_pNodeTail, sizeof(CNode)));

    CNode* pOldNode = m_pNodeTail;
    TYPE returnValue = pOldNode->data;

    m_pNodeTail = pOldNode->pPrev;
    if (m_pNodeTail != NULL)
        m_pNodeTail->pNext = NULL;
    else
        m_pNodeHead = NULL;
    FreeNode(pOldNode);
    return returnValue;
}

template<class TYPE, class ARG_TYPE>
LISTPOSITION CList<TYPE, ARG_TYPE>::InsertBefore(LISTPOSITION position, ARG_TYPE newElement)
{
    ASSERT_VALID(this);

    if (position == NULL)
        return AddHead(newElement); // insert before nothing -> head of the list

    // Insert it before position
    CNode* pOldNode = (CNode*) position;
    CNode* pNewNode = NewNode(pOldNode->pPrev, pOldNode);
    pNewNode->data = newElement;

    if (pOldNode->pPrev != NULL)
    {
        ASSERT(AfxIsValidAddress(pOldNode->pPrev, sizeof(CNode)));
        pOldNode->pPrev->pNext = pNewNode;
    }
    else
    {
        ASSERT(pOldNode == m_pNodeHead);
        m_pNodeHead = pNewNode;
    }
    pOldNode->pPrev = pNewNode;
    return (LISTPOSITION) pNewNode;
}

template<class TYPE, class ARG_TYPE>
LISTPOSITION CList<TYPE, ARG_TYPE>::InsertAfter(LISTPOSITION position, ARG_TYPE newElement)
{
    ASSERT_VALID(this);

    if (position == NULL)
        return AddTail(newElement); // insert after nothing -> tail of the list

    // Insert it before position
    CNode* pOldNode = (CNode*) position;
    ASSERT(AfxIsValidAddress(pOldNode, sizeof(CNode)));
    CNode* pNewNode = NewNode(pOldNode, pOldNode->pNext);
    pNewNode->data = newElement;

    if (pOldNode->pNext != NULL)
    {
        ASSERT(AfxIsValidAddress(pOldNode->pNext, sizeof(CNode)));
        pOldNode->pNext->pPrev = pNewNode;
    }
    else
    {
        ASSERT(pOldNode == m_pNodeTail);
        m_pNodeTail = pNewNode;
    }
    pOldNode->pNext = pNewNode;
    return (LISTPOSITION) pNewNode;
}

template<class TYPE, class ARG_TYPE>
void CList<TYPE, ARG_TYPE>::RemoveAt(LISTPOSITION position)
{
    ASSERT_VALID(this);

    CNode* pOldNode = (CNode*) position;
    ASSERT(AfxIsValidAddress(pOldNode, sizeof(CNode)));

    // remove pOldNode from list
    if (pOldNode == m_pNodeHead)
    {
        m_pNodeHead = pOldNode->pNext;
    }
    else
    {
        ASSERT(AfxIsValidAddress(pOldNode->pPrev, sizeof(CNode)));
        pOldNode->pPrev->pNext = pOldNode->pNext;
    }
    if (pOldNode == m_pNodeTail)
    {
        m_pNodeTail = pOldNode->pPrev;
    }
    else
    {
        ASSERT(AfxIsValidAddress(pOldNode->pNext, sizeof(CNode)));
        pOldNode->pNext->pPrev = pOldNode->pPrev;
    }
    FreeNode(pOldNode);
}

template<class TYPE, class ARG_TYPE>
LISTPOSITION CList<TYPE, ARG_TYPE>::FindIndex(int nIndex) const
{
    ASSERT_VALID(this);
    ASSERT(nIndex >= 0);

    if (nIndex >= m_nCount)
        return NULL;  // went too far

    CNode* pNode = m_pNodeHead;
    while (nIndex--)
    {
        ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
        pNode = pNode->pNext;
    }
    return (LISTPOSITION) pNode;
}

template<class TYPE, class ARG_TYPE>
LISTPOSITION CList<TYPE, ARG_TYPE>::Find(ARG_TYPE searchValue, LISTPOSITION startAfter) const
{
    ASSERT_VALID(this);

    CNode* pNode = (CNode*) startAfter;
    if (pNode == NULL)
    {
        pNode = m_pNodeHead;  // start at head
    }
    else
    {
        ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
        pNode = pNode->pNext;  // start after the one specified
    }

    for (; pNode != NULL; pNode = pNode->pNext)
        if (CompareElements(&pNode->data, &searchValue))
            return (LISTPOSITION)pNode;
    return NULL;
}

#if 0       // Member was comment'd out in the class definition
template<class TYPE, class ARG_TYPE>
void CList<TYPE, ARG_TYPE>::Serialize(CArchive& ar)
{
    ASSERT_VALID(this);

    CObject::Serialize(ar);

    if (ar.IsStoring())
    {
        ar << (WORD) m_nCount;
        for (CNode* pNode = m_pNodeHead; pNode != NULL; pNode = pNode->pNext)
        {
            ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
            SerializeElements(ar, &pNode->data, 1);
        }
    }
    else
    {
        WORD nNewCount;
        ar >> nNewCount;

        TYPE newData;
        while (nNewCount--)
        {
            SerializeElements(ar, &newData, 1);
            AddTail(newData);
        }
    }
}
#endif

#ifdef _DEBUG
template<class TYPE, class ARG_TYPE>
void CList<TYPE, ARG_TYPE>::Dump(CDumpContext& dc) const
{
    CObject::Dump(dc);

    dc << "with " << m_nCount << " elements";
    if (dc.GetDepth() > 0)
    {
        LISTPOSITION pos = GetHeadPosition();
        while (pos != NULL)
        {
            dc << "\n";
            DumpElements(dc, &((CList*)this)->GetNext(pos), 1);
        }
    }

    dc << "\n";
}

template<class TYPE, class ARG_TYPE>
void CList<TYPE, ARG_TYPE>::AssertValid() const
{
//  CObject::AssertValid();

    if (m_nCount == 0)
    {
        // empty list
        ASSERT(m_pNodeHead == NULL);
        ASSERT(m_pNodeTail == NULL);
    }
    else
    {
        // non-empty list
        ASSERT(AfxIsValidAddress(m_pNodeHead, sizeof(CNode)));
        ASSERT(AfxIsValidAddress(m_pNodeTail, sizeof(CNode)));
    }
}
#endif //_DEBUG

#endif // __cplusplus

#endif // __CLIST_HPP__
 

