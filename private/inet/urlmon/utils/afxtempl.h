// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXTEMPL_H__
#define __AFXTEMPL_H__

#ifndef __AFXPLEX_H__
//#include <afxplex_.h>
//#include "..\utils\afxplex_.h"
#endif

#ifdef unix
#include <mainwin.h>
#endif /* unix */

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, off)
#endif
#ifndef _AFX_FULLTYPEINFO
#pragma component(mintypeinfo, on)
#endif

#ifdef _AFX_PACKING
#pragma pack(push, _AFX_PACKING)
#endif

#ifdef _DEBUG
static char _szAfxTempl[] = "afxtempl.h";
#undef THIS_FILE
#define THIS_FILE _szAfxTempl
#endif

#ifndef ALL_WARNINGS
#pragma warning(disable: 4114)
#endif

/////////////////////////////////////////////////////////////////////////////
// global helpers (can be overridden)
/*
use our new for now
#ifdef new
#undef new
#define _REDEF_NEW
#endif

#ifndef _INC_NEW
        #include <new.h>
#endif
*/
template<class TYPE>
inline void AFXAPI ConstructElements(TYPE* pElements, int nCount)
{
        ASSERT(nCount == 0 ||
                AfxIsValidAddress(pElements, nCount * sizeof(TYPE)));

        // first do bit-wise zero initialization
        memset((void*)pElements, 0, nCount * sizeof(TYPE));

        // then call the constructor(s)
        for (; nCount--; pElements++)
        {
        }
}

template<class TYPE>
inline void AFXAPI DestructElements(TYPE* pElements, int nCount)
{
        ASSERT(nCount == 0 ||
                AfxIsValidAddress(pElements, nCount * sizeof(TYPE)));

        // call the destructor(s)
        for (; nCount--; pElements++)
                pElements->~TYPE();
}

template<class TYPE>
inline void AFXAPI CopyElements(TYPE* pDest, const TYPE* pSrc, int nCount)
{
        ASSERT(nCount == 0 ||
                AfxIsValidAddress(pDest, nCount * sizeof(TYPE)));
        ASSERT(nCount == 0 ||
                AfxIsValidAddress(pSrc, nCount * sizeof(TYPE)));

        // default is element-copy using assignment
        while (nCount--)
                *pDest++ = *pSrc;
}

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

#ifdef _DEBUG
#ifndef unix
template<class TYPE>
void AFXAPI DumpElements(CDumpContext& dc, const TYPE* pElements, int nCount)
{
        ASSERT(nCount == 0 ||
                AfxIsValidAddress(pElements, nCount * sizeof(TYPE)));
        &dc; // not used
        pElements;  // not used
        nCount; // not used

        // default does nothing
}
#endif /* unix */
#endif

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
        return PtrToUlong((void*)(DWORD_PTR)key) >> 4;
}

// special versions for CString
void AFXAPI ConstructElements(CString* pElements, int nCount);
void AFXAPI DestructElements(CString* pElements, int nCount);
void AFXAPI CopyElements(CString* pDest, const CString* pSrc, int nCount);
void AFXAPI SerializeElements(CArchive& ar, CString* pElements, int nCount);
UINT AFXAPI HashKey(LPCTSTR key);

// forward declarations
class COleVariant;
struct tagVARIANT;

// special versions for COleVariant
/*
void AFXAPI ConstructElements(COleVariant* pElements, int nCount);
void AFXAPI DestructElements(COleVariant* pElements, int nCount);
void AFXAPI CopyElements(COleVariant* pDest, const COleVariant* pSrc, int nCount);
void AFXAPI SerializeElements(CArchive& ar, COleVariant* pElements, int nCount);
void AFXAPI DumpElements(CDumpContext& dc, COleVariant* pElements, int nCount);
UINT AFXAPI HashKey(const struct tagVARIANT& var);
*/

// special versions for guids
//void AFXAPI ConstructElements(GUID* pElements, int nCount);
//void AFXAPI DestructElements(GUID* pElements, int nCount);
//void AFXAPI CopyElements(GUID* pDest, const GUID* pSrc, int nCount);
//void AFXAPI SerializeElements(CArchive& ar, GUID* pElements, int nCount);
//UINT AFXAPI HashKey(GUID key);

inline UINT AFXAPI HashKey(GUID Key)
{
   UINT hash = 0;
   BYTE FAR* lpb = (BYTE FAR*)&Key;
   UINT cbKey = sizeof(GUID);

   while (cbKey-- != 0)
        hash = 257 * hash + *lpb++;

   return hash;
}

inline UINT AFXAPI HashKey(SYSTEMTIME Key)
{
   UINT hash = 0;
   BYTE FAR* lpb = (BYTE FAR*)&Key;
   UINT cbKey = sizeof(SYSTEMTIME);

   while (cbKey-- != 0)
        hash = 257 * hash + *lpb++;

   return hash;
}
/*
inline UINT AFXAPI HashKey(IXY Key)
{
   UINT hash = 0;
   BYTE FAR* lpb = (BYTE FAR*)&Key;
   UINT cbKey = sizeof(IXY);

   while (cbKey-- != 0)
        hash = 257 * hash + *lpb++;

   return hash;
}
*/



//#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////
// CArray<TYPE, ARG_TYPE>

template<class TYPE, class ARG_TYPE>
class CArray : public CObject
{
public:
// Construction
        CArray();

// Attributes
        int GetSize() const;
        int GetUpperBound() const;
        void SetSize(int nNewSize, int nGrowBy = -1);

// Operations
        // Clean up
        void FreeExtra();
        void RemoveAll();

        // Accessing elements
        TYPE GetAt(int nIndex) const;
        void SetAt(int nIndex, ARG_TYPE newElement);
        TYPE& ElementAt(int nIndex);

        // Direct Access to the element data (may return NULL)
        const TYPE* GetData() const;
        TYPE* GetData();

        // Potentially growing the array
        void SetAtGrow(int nIndex, ARG_TYPE newElement);
        int Add(ARG_TYPE newElement);
        int Append(const CArray& src);
        void Copy(const CArray& src);

        // overloaded operator helpers
        TYPE operator[](int nIndex) const;
        TYPE& operator[](int nIndex);

        // Operations that move elements around
        void InsertAt(int nIndex, ARG_TYPE newElement, int nCount = 1);
        void RemoveAt(int nIndex, int nCount = 1);
        void InsertAt(int nStartIndex, CArray* pNewArray);

// Implementation
protected:
        TYPE* m_pData;   // the actual array of data
        int m_nSize;     // # of elements (upperBound - 1)
        int m_nMaxSize;  // max allocated
        int m_nGrowBy;   // grow amount

public:
        ~CArray();
        void Serialize(CArchive&);
#ifdef _DEBUG
        void Dump(CDumpContext&) const;
        void AssertValid() const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// CArray<TYPE, ARG_TYPE> inline functions

template<class TYPE, class ARG_TYPE>
inline int CArray<TYPE, ARG_TYPE>::GetSize() const
        { return m_nSize; }
template<class TYPE, class ARG_TYPE>
inline int CArray<TYPE, ARG_TYPE>::GetUpperBound() const
        { return m_nSize-1; }
template<class TYPE, class ARG_TYPE>
inline void CArray<TYPE, ARG_TYPE>::RemoveAll()
        { SetSize(0, -1); }
template<class TYPE, class ARG_TYPE>
inline TYPE CArray<TYPE, ARG_TYPE>::GetAt(int nIndex) const
        { ASSERT(nIndex >= 0 && nIndex < m_nSize);
                return m_pData[nIndex]; }
template<class TYPE, class ARG_TYPE>
inline void CArray<TYPE, ARG_TYPE>::SetAt(int nIndex, ARG_TYPE newElement)
        { ASSERT(nIndex >= 0 && nIndex < m_nSize);
                m_pData[nIndex] = newElement; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CArray<TYPE, ARG_TYPE>::ElementAt(int nIndex)
        { ASSERT(nIndex >= 0 && nIndex < m_nSize);
                return m_pData[nIndex]; }
template<class TYPE, class ARG_TYPE>
inline const TYPE* CArray<TYPE, ARG_TYPE>::GetData() const
        { return (const TYPE*)m_pData; }
template<class TYPE, class ARG_TYPE>
inline TYPE* CArray<TYPE, ARG_TYPE>::GetData()
        { return (TYPE*)m_pData; }
template<class TYPE, class ARG_TYPE>
inline int CArray<TYPE, ARG_TYPE>::Add(ARG_TYPE newElement)
        { int nIndex = m_nSize;
                SetAtGrow(nIndex, newElement);
                return nIndex; }
template<class TYPE, class ARG_TYPE>
inline TYPE CArray<TYPE, ARG_TYPE>::operator[](int nIndex) const
        { return GetAt(nIndex); }
template<class TYPE, class ARG_TYPE>
inline TYPE& CArray<TYPE, ARG_TYPE>::operator[](int nIndex)
        { return ElementAt(nIndex); }

/////////////////////////////////////////////////////////////////////////////
// CArray<TYPE, ARG_TYPE> out-of-line functions

template<class TYPE, class ARG_TYPE>
CArray<TYPE, ARG_TYPE>::CArray()
{
        m_pData = NULL;
        m_nSize = m_nMaxSize = m_nGrowBy = 0;
}

template<class TYPE, class ARG_TYPE>
CArray<TYPE, ARG_TYPE>::~CArray()
{
        ASSERT_VALID(this);

        if (m_pData != NULL)
        {
                DestructElements(m_pData, m_nSize);
                delete[] (BYTE*)m_pData;
        }
}

template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::SetSize(int nNewSize, int nGrowBy)
{
        ASSERT_VALID(this);
        ASSERT(nNewSize >= 0);

        if (nGrowBy != -1)
                m_nGrowBy = nGrowBy;  // set new size

        if (nNewSize == 0)
        {
                // shrink to nothing
                if (m_pData != NULL)
                {
                        DestructElements(m_pData, m_nSize);
                        delete[] (BYTE*)m_pData;
                        m_pData = NULL;
                }
                m_nSize = m_nMaxSize = 0;
        }
        else if (m_pData == NULL)
        {
                // create one with exact size
#ifdef SIZE_T_MAX
                ASSERT(nNewSize <= SIZE_T_MAX/sizeof(TYPE));    // no overflow
#endif
                m_pData = (TYPE*) new BYTE[nNewSize * sizeof(TYPE)];
                ConstructElements(m_pData, nNewSize);
                m_nSize = m_nMaxSize = nNewSize;
        }
        else if (nNewSize <= m_nMaxSize)
        {
                // it fits
                if (nNewSize > m_nSize)
                {
                        // initialize the new elements
                        ConstructElements(&m_pData[m_nSize], nNewSize-m_nSize);
                }
                else if (m_nSize > nNewSize)
                {
                        // destroy the old elements
                        DestructElements(&m_pData[nNewSize], m_nSize-nNewSize);
                }
                m_nSize = nNewSize;
        }
        else
        {
                // otherwise, grow array
                int nGrowBy = m_nGrowBy;
                if (nGrowBy == 0)
                {
                        // heuristically determine growth when nGrowBy == 0
                        //  (this avoids heap fragmentation in many situations)
                        nGrowBy = m_nSize / 8;
                        nGrowBy = (nGrowBy < 4) ? 4 : ((nGrowBy > 1024) ? 1024 : nGrowBy);
                }
                int nNewMax;
                if (nNewSize < m_nMaxSize + nGrowBy)
                        nNewMax = m_nMaxSize + nGrowBy;  // granularity
                else
                        nNewMax = nNewSize;  // no slush

                ASSERT(nNewMax >= m_nMaxSize);  // no wrap around
#ifdef SIZE_T_MAX
                ASSERT(nNewMax <= SIZE_T_MAX/sizeof(TYPE)); // no overflow
#endif
                TYPE* pNewData = (TYPE*) new BYTE[nNewMax * sizeof(TYPE)];

                // copy new data from old
                memcpy(pNewData, m_pData, m_nSize * sizeof(TYPE));

                // construct remaining elements
                ASSERT(nNewSize > m_nSize);
                ConstructElements(&pNewData[m_nSize], nNewSize-m_nSize);

                // get rid of old stuff (note: no destructors called)
                delete[] (BYTE*)m_pData;
                m_pData = pNewData;
                m_nSize = nNewSize;
                m_nMaxSize = nNewMax;
        }
}

template<class TYPE, class ARG_TYPE>
int CArray<TYPE, ARG_TYPE>::Append(const CArray& src)
{
        ASSERT_VALID(this);
        ASSERT(this != &src);   // cannot append to itself

        int nOldSize = m_nSize;
        SetSize(m_nSize + src.m_nSize);
        CopyElements(m_pData + nOldSize, src.m_pData, src.m_nSize);
        return nOldSize;
}

template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::Copy(const CArray& src)
{
        ASSERT_VALID(this);
        ASSERT(this != &src);   // cannot append to itself

        SetSize(src.m_nSize);
        CopyElements(m_pData, src.m_pData, src.m_nSize);
}

template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::FreeExtra()
{
        ASSERT_VALID(this);

        if (m_nSize != m_nMaxSize)
        {
                // shrink to desired size
#ifdef SIZE_T_MAX
                ASSERT(m_nSize <= SIZE_T_MAX/sizeof(TYPE)); // no overflow
#endif
                TYPE* pNewData = NULL;
                if (m_nSize != 0)
                {
                        pNewData = (TYPE*) new BYTE[m_nSize * sizeof(TYPE)];
                        // copy new data from old
                        memcpy(pNewData, m_pData, m_nSize * sizeof(TYPE));
                }

                // get rid of old stuff (note: no destructors called)
                delete[] (BYTE*)m_pData;
                m_pData = pNewData;
                m_nMaxSize = m_nSize;
        }
}

template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::SetAtGrow(int nIndex, ARG_TYPE newElement)
{
        ASSERT_VALID(this);
        ASSERT(nIndex >= 0);

        if (nIndex >= m_nSize)
                SetSize(nIndex+1, -1);
        m_pData[nIndex] = newElement;
}

template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::InsertAt(int nIndex, ARG_TYPE newElement, int nCount /*=1*/)
{
        ASSERT_VALID(this);
        ASSERT(nIndex >= 0);    // will expand to meet need
        ASSERT(nCount > 0);     // zero or negative size not allowed

        if (nIndex >= m_nSize)
        {
                // adding after the end of the array
                SetSize(nIndex + nCount, -1);   // grow so nIndex is valid
        }
        else
        {
                // inserting in the middle of the array
                int nOldSize = m_nSize;
                SetSize(m_nSize + nCount, -1);  // grow it to new size
                // destroy intial data before copying over it
                DestructElements(&m_pData[nOldSize], nCount);
                // shift old data up to fill gap
                memmove(&m_pData[nIndex+nCount], &m_pData[nIndex],
                        (nOldSize-nIndex) * sizeof(TYPE));

                // re-init slots we copied from
                ConstructElements(&m_pData[nIndex], nCount);
        }

        // insert new value in the gap
        ASSERT(nIndex + nCount <= m_nSize);
        while (nCount--)
                m_pData[nIndex++] = newElement;
}

template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::RemoveAt(int nIndex, int nCount)
{
        ASSERT_VALID(this);
        ASSERT(nIndex >= 0);
        ASSERT(nCount >= 0);
        ASSERT(nIndex + nCount <= m_nSize);

        // just remove a range
        int nMoveCount = m_nSize - (nIndex + nCount);
        DestructElements(&m_pData[nIndex], nCount);
        if (nMoveCount)
                memcpy(&m_pData[nIndex], &m_pData[nIndex + nCount],
                        nMoveCount * sizeof(TYPE));
        m_nSize -= nCount;
}

template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::InsertAt(int nStartIndex, CArray* pNewArray)
{
        ASSERT_VALID(this);
        ASSERT(pNewArray != NULL);
        ASSERT_VALID(pNewArray);
        ASSERT(nStartIndex >= 0);

        if (pNewArray->GetSize() > 0)
        {
                InsertAt(nStartIndex, pNewArray->GetAt(0), pNewArray->GetSize());
                for (int i = 0; i < pNewArray->GetSize(); i++)
                        SetAt(nStartIndex + i, pNewArray->GetAt(i));
        }
}

template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::Serialize(CArchive& ar)
{
        ASSERT_VALID(this);
#ifndef unix
        // UNIX found decl commented out in coll.hxx
        CObject::Serialize(ar);
#else
	MwBugCheck();
#endif /* unix */
#ifndef unix
        if (ar.IsStoring())
        {
                ar.WriteCount(m_nSize);
        }
        else
        {
                DWORD nOldSize = ar.ReadCount();
                SetSize(nOldSize, -1);
        }
        SerializeElements(ar, m_pData, m_nSize);
#endif /* unix */
}

#ifdef _DEBUG
#ifndef unix
template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::Dump(CDumpContext& dc) const
{
        CObject::Dump(dc);

        dc << "with " << m_nSize << " elements";
        if (dc.GetDepth() > 0)
        {
                dc << "\n";
                DumpElements(dc, m_pData, m_nSize);
        }

        dc << "\n";
}
#endif /* unix */

template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::AssertValid() const
{
        CObject::AssertValid();

        if (m_pData == NULL)
        {
                ASSERT(m_nSize == 0);
                ASSERT(m_nMaxSize == 0);
        }
        else
        {
                ASSERT(m_nSize >= 0);
                ASSERT(m_nMaxSize >= 0);
                ASSERT(m_nSize <= m_nMaxSize);
                ASSERT(AfxIsValidAddress(m_pData, m_nMaxSize * sizeof(TYPE)));
        }
}
#endif //_DEBUG

#define _LIST_DEFINED_

#ifndef _LIST_DEFINED_

/////////////////////////////////////////////////////////////////////////////
// CList<TYPE, ARG_TYPE>

template<class TYPE, class ARG_TYPE>
class CList : public CObject
{
protected:
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
        POSITION AddHead(ARG_TYPE newElement);
        POSITION AddTail(ARG_TYPE newElement);

        // add another list of elements before head or after tail
        void AddHead(CList* pNewList);
        void AddTail(CList* pNewList);

        // remove all elements
        void RemoveAll();

        // iteration
        POSITION GetHeadPosition() const;
        POSITION GetTailPosition() const;
        TYPE& GetNext(POSITION& rPosition); // return *Position++
        TYPE GetNext(POSITION& rPosition) const; // return *Position++
        TYPE& GetPrev(POSITION& rPosition); // return *Position--
        TYPE GetPrev(POSITION& rPosition) const; // return *Position--

        // getting/modifying an element at a given position
        TYPE& GetAt(POSITION position);
        TYPE GetAt(POSITION position) const;
        void SetAt(POSITION pos, ARG_TYPE newElement);
        void RemoveAt(POSITION position);

        // inserting before or after a given position
        POSITION InsertBefore(POSITION position, ARG_TYPE newElement);
        POSITION InsertAfter(POSITION position, ARG_TYPE newElement);

        // helper functions (note: O(n) speed)
        POSITION Find(ARG_TYPE searchValue, POSITION startAfter = NULL) const;
                // defaults to starting at the HEAD, return NULL if not found
        POSITION FindIndex(int nIndex) const;
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
        void Serialize(CArchive&);
#ifdef _DEBUG
        void Dump(CDumpContext&) const;
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
inline POSITION CList<TYPE, ARG_TYPE>::GetHeadPosition() const
        { return (POSITION) m_pNodeHead; }
template<class TYPE, class ARG_TYPE>
inline POSITION CList<TYPE, ARG_TYPE>::GetTailPosition() const
        { return (POSITION) m_pNodeTail; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CList<TYPE, ARG_TYPE>::GetNext(POSITION& rPosition) // return *Position++
        { CNode* pNode = (CNode*) rPosition;
                ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
                rPosition = (POSITION) pNode->pNext;
                return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE CList<TYPE, ARG_TYPE>::GetNext(POSITION& rPosition) const // return *Position++
        { CNode* pNode = (CNode*) rPosition;
                ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
                rPosition = (POSITION) pNode->pNext;
                return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CList<TYPE, ARG_TYPE>::GetPrev(POSITION& rPosition) // return *Position--
        { CNode* pNode = (CNode*) rPosition;
                ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
                rPosition = (POSITION) pNode->pPrev;
                return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE CList<TYPE, ARG_TYPE>::GetPrev(POSITION& rPosition) const // return *Position--
        { CNode* pNode = (CNode*) rPosition;
                ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
                rPosition = (POSITION) pNode->pPrev;
                return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CList<TYPE, ARG_TYPE>::GetAt(POSITION position)
        { CNode* pNode = (CNode*) position;
                ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
                return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE CList<TYPE, ARG_TYPE>::GetAt(POSITION position) const
        { CNode* pNode = (CNode*) position;
                ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
                return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline void CList<TYPE, ARG_TYPE>::SetAt(POSITION pos, ARG_TYPE newElement)
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

        // if no more elements, cleanup completely
        if (m_nCount == 0)
                RemoveAll();
}

template<class TYPE, class ARG_TYPE>
POSITION CList<TYPE, ARG_TYPE>::AddHead(ARG_TYPE newElement)
{
        ASSERT_VALID(this);

        CNode* pNewNode = NewNode(NULL, m_pNodeHead);
        pNewNode->data = newElement;
        if (m_pNodeHead != NULL)
                m_pNodeHead->pPrev = pNewNode;
        else
                m_pNodeTail = pNewNode;
        m_pNodeHead = pNewNode;
        return (POSITION) pNewNode;
}

template<class TYPE, class ARG_TYPE>
POSITION CList<TYPE, ARG_TYPE>::AddTail(ARG_TYPE newElement)
{
        ASSERT_VALID(this);

        CNode* pNewNode = NewNode(m_pNodeTail, NULL);
        pNewNode->data = newElement;
        if (m_pNodeTail != NULL)
                m_pNodeTail->pNext = pNewNode;
        else
                m_pNodeHead = pNewNode;
        m_pNodeTail = pNewNode;
        return (POSITION) pNewNode;
}

template<class TYPE, class ARG_TYPE>
void CList<TYPE, ARG_TYPE>::AddHead(CList* pNewList)
{
        ASSERT_VALID(this);

        ASSERT(pNewList != NULL);
        ASSERT_VALID(pNewList);

        // add a list of same elements to head (maintain order)
        POSITION pos = pNewList->GetTailPosition();
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
        POSITION pos = pNewList->GetHeadPosition();
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
POSITION CList<TYPE, ARG_TYPE>::InsertBefore(POSITION position, ARG_TYPE newElement)
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
        return (POSITION) pNewNode;
}

template<class TYPE, class ARG_TYPE>
POSITION CList<TYPE, ARG_TYPE>::InsertAfter(POSITION position, ARG_TYPE newElement)
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
        return (POSITION) pNewNode;
}

template<class TYPE, class ARG_TYPE>
void CList<TYPE, ARG_TYPE>::RemoveAt(POSITION position)
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
POSITION CList<TYPE, ARG_TYPE>::FindIndex(int nIndex) const
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
        return (POSITION) pNode;
}

template<class TYPE, class ARG_TYPE>
POSITION CList<TYPE, ARG_TYPE>::Find(ARG_TYPE searchValue, POSITION startAfter) const
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
                        return (POSITION)pNode;
        return NULL;
}

template<class TYPE, class ARG_TYPE>
void CList<TYPE, ARG_TYPE>::Serialize(CArchive& ar)
{
        ASSERT_VALID(this);
#ifndef unix
        // UNIX found decl commented out in coll.hxx
        CObject::Serialize(ar);
#else
	MwBugCheck();
#endif /* unix */
#ifndef unix
        if (ar.IsStoring())
        {
                ar.WriteCount(m_nCount);
                for (CNode* pNode = m_pNodeHead; pNode != NULL; pNode = pNode->pNext)
                {
                        ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
                        SerializeElements(ar, &pNode->data, 1);
                }
        }
        else
        {
                DWORD nNewCount = ar.ReadCount();
                TYPE newData;
                while (nNewCount--)
                {
                        SerializeElements(ar, &newData, 1);
                        AddTail(newData);
                }
        }
#endif /* unix */
}

#ifdef _DEBUG
template<class TYPE, class ARG_TYPE>
void CList<TYPE, ARG_TYPE>::Dump(CDumpContext& dc) const
{
        CObject::Dump(dc);

        dc << "with " << m_nCount << " elements";
        if (dc.GetDepth() > 0)
        {
                POSITION pos = GetHeadPosition();
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
        CObject::AssertValid();

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
#endif //LIST_DEFINED

/////////////////////////////////////////////////////////////////////////////
// CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
class CMap : public CObject
{
#ifndef unix
protected:
#else
  // If this was not made public we get complier warnings
    // that CMap<T,...>::CAssoc is not accessible from file scope
    // which means functions cant return CNode pointers
public:
#endif /* unix */
        // Association
        struct CAssoc
        {
                CAssoc* pNext;
                UINT nHashValue;  // needed for efficient iteration
                KEY key;
                VALUE value;
        };
public:
// Construction
        CMap(int nBlockSize = 10);

// Attributes
        // number of elements
        int GetCount() const;
        BOOL IsEmpty() const;

        // Lookup
        BOOL Lookup(ARG_KEY key, VALUE& rValue) const;

// Operations
        // Lookup and add if not there
        VALUE& operator[](ARG_KEY key);

        // add a new (key, value) pair
        void SetAt(ARG_KEY key, ARG_VALUE newValue);

        // removing existing (key, ?) pair
        BOOL RemoveKey(ARG_KEY key);
        void RemoveAll();

        // iterating all (key, value) pairs
        POSITION GetStartPosition() const;
        void GetNextAssoc(POSITION& rNextPosition, KEY& rKey, VALUE& rValue) const;

        // advanced features for derived classes
        UINT GetHashTableSize() const;
        void InitHashTable(UINT hashSize, BOOL bAllocNow = TRUE);

// Implementation
protected:
        CAssoc** m_pHashTable;
        UINT m_nHashTableSize;
        int m_nCount;
        CAssoc* m_pFreeList;
        struct CPlex* m_pBlocks;
        int m_nBlockSize;

        CAssoc* NewAssoc();
        void FreeAssoc(CAssoc*);
        CAssoc* GetAssocAt(ARG_KEY, UINT&) const;

public:
        ~CMap();
        void Serialize(CArchive&);
#ifdef _DEBUG
        void Dump(CDumpContext&) const;
        void AssertValid() const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// CMap<KEY, ARG_KEY, VALUE, ARG_VALUE> inline functions

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
inline int CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::GetCount() const
        { return m_nCount; }
template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
inline BOOL CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::IsEmpty() const
        { return m_nCount == 0; }
template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
inline void CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::SetAt(ARG_KEY key, ARG_VALUE newValue)
        { (*this)[key] = newValue; }
template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
inline POSITION CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::GetStartPosition() const
        { return (m_nCount == 0) ? NULL : BEFORE_START_POSITION; }
template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
inline UINT CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::GetHashTableSize() const
        { return m_nHashTableSize; }

/////////////////////////////////////////////////////////////////////////////
// CMap<KEY, ARG_KEY, VALUE, ARG_VALUE> out-of-line functions

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::CMap(int nBlockSize)
{
        ASSERT(nBlockSize > 0);

        m_pHashTable = NULL;
        m_nHashTableSize = 17;  // default size
        m_nCount = 0;
        m_pFreeList = NULL;
        m_pBlocks = NULL;
        m_nBlockSize = nBlockSize;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::InitHashTable(
        UINT nHashSize, BOOL bAllocNow)
//
// Used to force allocation of a hash table or to override the default
//   hash table size of (which is fairly small)
{
        ASSERT_VALID(this);
        ASSERT(m_nCount == 0);
        ASSERT(nHashSize > 0);

        if (m_pHashTable != NULL)
        {
                // free hash table
                delete[] m_pHashTable;
                m_pHashTable = NULL;
        }

        if (bAllocNow)
        {
                m_pHashTable = new CAssoc* [nHashSize];
                memset(m_pHashTable, 0, sizeof(CAssoc*) * nHashSize);
        }
        m_nHashTableSize = nHashSize;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::RemoveAll()
{
        ASSERT_VALID(this);

        if (m_pHashTable != NULL)
        {
                // destroy elements (values and keys)
                for (UINT nHash = 0; nHash < m_nHashTableSize; nHash++)
                {
                        CAssoc* pAssoc;
                        for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL;
                          pAssoc = pAssoc->pNext)
                        {
                                DestructElements(&pAssoc->value, 1);
                                DestructElements(&pAssoc->key, 1);
                        }
                }

            // free hash table
            delete[] m_pHashTable;
            m_pHashTable = NULL;
        }

        m_nCount = 0;
        m_pFreeList = NULL;
        if (m_pBlocks)
        {
            m_pBlocks->FreeDataChain();
            m_pBlocks = NULL;
        }
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::~CMap()
{
    if (m_nCount)
    {
        RemoveAll();
    }
    ASSERT(m_nCount == 0);
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::CAssoc*
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::NewAssoc()
{
        if (m_pFreeList == NULL)
        {
                // add another block
                CPlex* newBlock = CPlex::Create(m_pBlocks, m_nBlockSize, sizeof(CMap::CAssoc));
                // chain them into free list
                CMap::CAssoc* pAssoc = (CMap::CAssoc*) newBlock->data();
                // free in reverse order to make it easier to debug
                pAssoc += m_nBlockSize - 1;
                for (int i = m_nBlockSize-1; i >= 0; i--, pAssoc--)
                {
                        pAssoc->pNext = m_pFreeList;
                        m_pFreeList = pAssoc;
                }
        }
        ASSERT(m_pFreeList != NULL);  // we must have something

        CMap::CAssoc* pAssoc = m_pFreeList;
        m_pFreeList = m_pFreeList->pNext;
        m_nCount++;
        ASSERT(m_nCount > 0);  // make sure we don't overflow
        ConstructElements(&pAssoc->key, 1);
        ConstructElements(&pAssoc->value, 1);   // special construct values
        return pAssoc;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::FreeAssoc(CMap::CAssoc* pAssoc)
{
        DestructElements(&pAssoc->value, 1);
        DestructElements(&pAssoc->key, 1);
        pAssoc->pNext = m_pFreeList;
        m_pFreeList = pAssoc;
        m_nCount--;
        ASSERT(m_nCount >= 0);  // make sure we don't underflow

        // if no more elements, cleanup completely
        if (m_nCount == 0)
                RemoveAll();
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::CAssoc*
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::GetAssocAt(ARG_KEY key, UINT& nHash) const
// find association (or return NULL)
{
        nHash = HashKey(key) % m_nHashTableSize;

        if (m_pHashTable == NULL)
                return NULL;

        // see if it exists
        CAssoc* pAssoc;
        for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL; pAssoc = pAssoc->pNext)
        {
                if (CompareElements(&pAssoc->key, &key))
                        return pAssoc;
        }
        return NULL;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::Lookup(ARG_KEY key, VALUE& rValue) const
{
        ASSERT_VALID(this);

        UINT nHash;
        CAssoc* pAssoc = GetAssocAt(key, nHash);
        if (pAssoc == NULL)
                return FALSE;  // not in map

        rValue = pAssoc->value;
        return TRUE;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
VALUE& CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::operator[](ARG_KEY key)
{
        ASSERT_VALID(this);

        UINT nHash;
        CAssoc* pAssoc;
        if ((pAssoc = GetAssocAt(key, nHash)) == NULL)
        {
                if (m_pHashTable == NULL)
                        InitHashTable(m_nHashTableSize);

                // it doesn't exist, add a new Association
                pAssoc = NewAssoc();
                pAssoc->nHashValue = nHash;
                pAssoc->key = key;
                // 'pAssoc->value' is a constructed object, nothing more

                // put into hash table
                pAssoc->pNext = m_pHashTable[nHash];
                m_pHashTable[nHash] = pAssoc;
        }
        return pAssoc->value;  // return new reference
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::RemoveKey(ARG_KEY key)
// remove key - return TRUE if removed
{
        ASSERT_VALID(this);

        if (m_pHashTable == NULL)
                return FALSE;  // nothing in the table

        CAssoc** ppAssocPrev;
        ppAssocPrev = &m_pHashTable[HashKey(key) % m_nHashTableSize];

        CAssoc* pAssoc;
        for (pAssoc = *ppAssocPrev; pAssoc != NULL; pAssoc = pAssoc->pNext)
        {
                if (CompareElements(&pAssoc->key, &key))
                {
                        // remove it
                        *ppAssocPrev = pAssoc->pNext;  // remove from list
                        FreeAssoc(pAssoc);
                        return TRUE;
                }
                ppAssocPrev = &pAssoc->pNext;
        }
        return FALSE;  // not found
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::GetNextAssoc(POSITION& rNextPosition,
        KEY& rKey, VALUE& rValue) const
{
        ASSERT_VALID(this);
        ASSERT(m_pHashTable != NULL);  // never call on empty map

        CAssoc* pAssocRet = (CAssoc*)rNextPosition;
        ASSERT(pAssocRet != NULL);

        if (pAssocRet == (CAssoc*) BEFORE_START_POSITION)
        {
                // find the first association
                for (UINT nBucket = 0; nBucket < m_nHashTableSize; nBucket++)
                        if ((pAssocRet = m_pHashTable[nBucket]) != NULL)
                                break;
                ASSERT(pAssocRet != NULL);  // must find something
        }

        // find next association
        ASSERT(AfxIsValidAddress(pAssocRet, sizeof(CAssoc)));
        CAssoc* pAssocNext;
        if ((pAssocNext = pAssocRet->pNext) == NULL)
        {
                // go to next bucket
                for (UINT nBucket = pAssocRet->nHashValue + 1;
                  nBucket < m_nHashTableSize; nBucket++)
                        if ((pAssocNext = m_pHashTable[nBucket]) != NULL)
                                break;
        }

        rNextPosition = (POSITION) pAssocNext;

        // fill in return data
        rKey = pAssocRet->key;
        rValue = pAssocRet->value;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::Serialize(CArchive& ar)
{
        ASSERT_VALID(this);
#ifndef unix
        // UNIX found dedcl commented out in coll.hxx
        CObject::Serialize(ar);
#endif /* unix */

        if (ar.IsStoring())
        {
                ar.WriteCount(m_nCount);
                if (m_nCount == 0)
                        return;  // nothing more to do

                ASSERT(m_pHashTable != NULL);
                for (UINT nHash = 0; nHash < m_nHashTableSize; nHash++)
                {
                        CAssoc* pAssoc;
                        for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL;
                          pAssoc = pAssoc->pNext)
                        {
                                SerializeElements(ar, &pAssoc->key, 1);
                                SerializeElements(ar, &pAssoc->value, 1);
                        }
                }
        }
        else
        {
                DWORD nNewCount = ar.ReadCount();
                KEY newKey;
                VALUE newValue;
                while (nNewCount--)
                {
                        SerializeElements(ar, &newKey, 1);
                        SerializeElements(ar, &newValue, 1);
                        SetAt(newKey, newValue);
                }
        }
}

#ifdef _DEBUG
#ifndef unix
template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::Dump(CDumpContext& dc) const
{
        CObject::Dump(dc);

        dc << "with " << m_nCount << " elements";
        if (dc.GetDepth() > 0)
        {
                // Dump in format "[key] -> value"
                KEY key;
                VALUE val;

                POSITION pos = GetStartPosition();
                while (pos != NULL)
                {
                        GetNextAssoc(pos, key, val);
                        dc << "\n\t[";
                        DumpElements(dc, &key, 1);
                        dc << "] = ";
                        DumpElements(dc, &val, 1);
                }
        }

        dc << "\n";
}
#endif /* unix */

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::AssertValid() const
{
        CObject::AssertValid();

        ASSERT(m_nHashTableSize > 0);
        ASSERT(m_nCount == 0 || m_pHashTable != NULL);
                // non-empty map should have hash table
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTypedPtrArray<BASE_CLASS, TYPE>

template<class BASE_CLASS, class TYPE>
class CTypedPtrArray : public BASE_CLASS
{
public:
        // Accessing elements
        TYPE GetAt(int nIndex) const
                { return (TYPE)BASE_CLASS::GetAt(nIndex); }
        TYPE& ElementAt(int nIndex)
                { return (TYPE&)BASE_CLASS::ElementAt(nIndex); }
        void SetAt(int nIndex, TYPE ptr)
                { BASE_CLASS::SetAt(nIndex, ptr); }

        // Potentially growing the array
        void SetAtGrow(int nIndex, TYPE newElement)
           { BASE_CLASS::SetAtGrow(nIndex, newElement); }
        int Add(TYPE newElement)
           { return BASE_CLASS::Add(newElement); }
        int Append(const CTypedPtrArray<BASE_CLASS, TYPE>& src)
           { return BASE_CLASS::Append(src); }
        void Copy(const CTypedPtrArray<BASE_CLASS, TYPE>& src)
                { BASE_CLASS::Copy(src); }

        // Operations that move elements around
        void InsertAt(int nIndex, TYPE newElement, int nCount = 1)
                { BASE_CLASS::InsertAt(nIndex, newElement, nCount); }
        void InsertAt(int nStartIndex, CTypedPtrArray<BASE_CLASS, TYPE>* pNewArray)
           { BASE_CLASS::InsertAt(nStartIndex, pNewArray); }

        // overloaded operator helpers
        TYPE operator[](int nIndex) const
                { return (TYPE)BASE_CLASS::operator[](nIndex); }
        TYPE& operator[](int nIndex)
                { return (TYPE&)BASE_CLASS::operator[](nIndex); }
};

/////////////////////////////////////////////////////////////////////////////
// CTypedPtrList<BASE_CLASS, TYPE>

template<class BASE_CLASS, class TYPE>
class CTypedPtrList : public BASE_CLASS
{
public:
// Construction
        CTypedPtrList(int nBlockSize = 10)
                : BASE_CLASS(nBlockSize) { }

        // peek at head or tail
        TYPE& GetHead()
                { return (TYPE&)BASE_CLASS::GetHead(); }
        TYPE GetHead() const
                { return (TYPE)BASE_CLASS::GetHead(); }
        TYPE& GetTail()
                { return (TYPE&)BASE_CLASS::GetTail(); }
        TYPE GetTail() const
                { return (TYPE)BASE_CLASS::GetTail(); }

        // get head or tail (and remove it) - don't call on empty list!
        TYPE RemoveHead()
                { return (TYPE)BASE_CLASS::RemoveHead(); }
        TYPE RemoveTail()
                { return (TYPE)BASE_CLASS::RemoveTail(); }

        // add before head or after tail
        POSITION AddHead(TYPE newElement)
                { return BASE_CLASS::AddHead(newElement); }
        POSITION AddTail(TYPE newElement)
                { return BASE_CLASS::AddTail(newElement); }

        // add another list of elements before head or after tail
        void AddHead(CTypedPtrList<BASE_CLASS, TYPE>* pNewList)
                { BASE_CLASS::AddHead(pNewList); }
        void AddTail(CTypedPtrList<BASE_CLASS, TYPE>* pNewList)
                { BASE_CLASS::AddTail(pNewList); }

        // iteration
        TYPE& GetNext(POSITION& rPosition)
                { return (TYPE&)BASE_CLASS::GetNext(rPosition); }
        TYPE GetNext(POSITION& rPosition) const
                { return (TYPE)BASE_CLASS::GetNext(rPosition); }
        TYPE& GetPrev(POSITION& rPosition)
                { return (TYPE&)BASE_CLASS::GetPrev(rPosition); }
        TYPE GetPrev(POSITION& rPosition) const
                { return (TYPE)BASE_CLASS::GetPrev(rPosition); }

        // getting/modifying an element at a given position
        TYPE& GetAt(POSITION position)
                { return (TYPE&)BASE_CLASS::GetAt(position); }
        TYPE GetAt(POSITION position) const
                { return (TYPE)BASE_CLASS::GetAt(position); }
        void SetAt(POSITION pos, TYPE newElement)
                { BASE_CLASS::SetAt(pos, newElement); }
};

/////////////////////////////////////////////////////////////////////////////
// CTypedPtrMap<BASE_CLASS, KEY, VALUE>

template<class BASE_CLASS, class KEY, class VALUE>
class CTypedPtrMap : public BASE_CLASS
{
public:

// Construction
        CTypedPtrMap(int nBlockSize = 10)
                : BASE_CLASS(nBlockSize) { }

        // Lookup
        BOOL Lookup(BASE_CLASS::BASE_ARG_KEY key, VALUE& rValue) const
                { return BASE_CLASS::Lookup(key, (BASE_CLASS::BASE_VALUE&)rValue); }

        // Lookup and add if not there
        VALUE& operator[](BASE_CLASS::BASE_ARG_KEY key)
                { return (VALUE&)BASE_CLASS::operator[](key); }

        // add a new key (key, value) pair
        void SetAt(KEY key, VALUE newValue)
                { BASE_CLASS::SetAt(key, newValue); }

        // removing existing (key, ?) pair
        BOOL RemoveKey(KEY key)
                { return BASE_CLASS::RemoveKey(key); }

        // iteration
        void GetNextAssoc(POSITION& rPosition, KEY& rKey, VALUE& rValue) const
                { BASE_CLASS::GetNextAssoc(rPosition, (BASE_CLASS::BASE_KEY&)rKey,
                        (BASE_CLASS::BASE_VALUE&)rValue); }
};

/////////////////////////////////////////////////////////////////////////////

#undef THIS_FILE
#define THIS_FILE __FILE__

#undef new
#ifdef _REDEF_NEW
#define new DEBUG_NEW
#undef _REDEF_NEW
#endif

#ifdef _AFX_PACKING
#pragma pack(pop)
#endif

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, on)
#endif
#ifndef _AFX_FULLTYPEINFO
#pragma component(mintypeinfo, off)
#endif

#endif //__AFXTEMPL_H__

/////////////////////////////////////////////////////////////////////////////
 

