#ifndef _INC_DSKQUOTA_DYNARRAY_H
#define _INC_DSKQUOTA_DYNARRAY_H
///////////////////////////////////////////////////////////////////////////////
/*  File: dynarray.h

    Description: Wrapper classes around the DPA_xxxxxxx and DSA_xxxxxx functions 
        provided by the common control's library.  The classes add value by 
        providing multi-threaded protection, iterators and automatic cleanup 
        semantics. 

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/14/96    Initial creation.                                    BrianAu
    09/03/96    Added exception handling.                            BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _WINDOWS_
#   include <windows.h>
#endif
#ifndef _INC_COMMCTRL
#   include <commctrl.h>
#endif
#ifndef _INC_COMCTRLP
#   include <comctrlp.h>
#endif
#ifndef _INC_DSKQUOTA_EXCEPT_H
#   include "except.h"
#endif


///////////////////////////////////////////////////////////////////////////////
// CONTAINER EXCEPTIONS
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// POINTER LIST
///////////////////////////////////////////////////////////////////////////////
class PointerList
{
    private:
        HDPA             m_hdpa;
        CRITICAL_SECTION m_cs;

    public:
        PointerList(INT cItemGrow = 0);

        virtual ~PointerList(void);

        UINT Count(void);

        VOID Insert(LPVOID pvItem, UINT index);
        VOID Insert(LPVOID pvItem)
            { Insert(pvItem, 0); }

        VOID Append(LPVOID pvItem, UINT index)
            { Insert(pvItem, index + 1); }
        VOID Append(LPVOID pvItem);

        BOOL Remove(LPVOID *ppvItem, UINT index);
        BOOL RemoveFirst(LPVOID *ppvItem)
            { return Remove(ppvItem, 0); }
        BOOL RemoveLast(LPVOID *ppvItem);

        BOOL Retrieve(LPVOID *ppvItem, UINT index);
        BOOL RetrieveFirst(LPVOID *ppvItem)
            { return Retrieve(ppvItem, 0); }
        BOOL RetrieveLast(LPVOID *ppvItem);

        BOOL Replace(LPVOID pvItem, UINT index);
        BOOL FindIndex(LPVOID pvItem, INT *pIndex);

        BOOL Sort(PFNDPACOMPARE pfnCompare, LPARAM lParam);
        BOOL Search(LPVOID pvKey,
                    PFNDPACOMPARE pfnCompare, 
                    UINT uOptions = 0, 
                    INT iStart = 0,
                    LPARAM lParam = 0);

        void Lock(void)
            { EnterCriticalSection(&m_cs); }
        void ReleaseLock(void)
            { LeaveCriticalSection(&m_cs); }

        friend class PointerListIterator;
        friend class AutoLock;
};



///////////////////////////////////////////////////////////////////////////////
// POINTER LIST ITERATOR
///////////////////////////////////////////////////////////////////////////////
class PointerListIterator {
    private:
        PointerList *m_pList;    // Pointer to list being iterated.
        INT          m_Index;    // "Current" signed index into list.
             
        HRESULT Advance(LPVOID *ppvOut, BOOL bForward);

    public:
        enum { EndOfList = -1 };

        PointerListIterator(PointerList& List)
            : m_pList(&List), 
              m_Index(0) { }

        PointerListIterator(const PointerListIterator& rhs)
            : m_pList(rhs.m_pList), 
              m_Index(rhs.m_Index) { }

        PointerListIterator& operator = (const PointerListIterator& rhs);

        HRESULT Next(LPVOID *ppvOut)  
            { return Advance(ppvOut, TRUE); }

        HRESULT Prev(LPVOID *ppvOut)  
            { return Advance(ppvOut, FALSE); }

        BOOL AtFirst(void)
            { return m_Index == 0; }

        BOOL AtLast(void)
            { return m_Index >= (INT)m_pList->Count() - 1; }
        
        void GotoFirst(void)
            { m_Index = 0; }

        void GotoLast(void)
            { m_Index = m_pList->Count() - 1; }

        void LockList(void)
            { m_pList->Lock(); }

        void ReleaseListLock(void)
            { m_pList->ReleaseLock(); }
};


///////////////////////////////////////////////////////////////////////////////
// POINTER QUEUE
///////////////////////////////////////////////////////////////////////////////
class PointerQueue : public PointerList
{
    public:
        virtual ~PointerQueue(void) { }

        VOID Add(LPVOID pvItem)
            { PointerList::Append(pvItem); }
        BOOL Remove(LPVOID *ppvItem)
            { return PointerList::RemoveFirst(ppvItem); }
};


///////////////////////////////////////////////////////////////////////////////
// STRUCTURE LIST
//
///////////////////////////////////////////////////////////////////////////////
class StructureList
{
    private:
        HDSA             m_hdsa;
        CRITICAL_SECTION m_cs;

    public:
        StructureList(INT cbItem, INT cItemGrow);

        virtual ~StructureList(void);

        UINT    Count(void);

        VOID Insert(LPVOID pvItem, UINT index);
        VOID Insert(LPVOID pvItem)
            { Insert(pvItem, 0); }

        VOID Append(LPVOID pvItem, UINT index)
            { Insert(pvItem, index + 1); }
        VOID Append(LPVOID pvItem);

        BOOL Remove(LPVOID pvItem, UINT index);
        BOOL RemoveFirst(LPVOID pvItem) 
            { return Remove(pvItem, 0); }
        BOOL RemoveLast(LPVOID pvItem); 

        BOOL Retrieve(LPVOID pvItem, UINT index);
        BOOL RetrieveFirst(LPVOID pvItem) 
            { return Retrieve(pvItem, 0); }
        BOOL RetrieveLast(LPVOID pvItem);

        BOOL Replace(LPVOID pvItem, UINT index);
        VOID Clear(VOID);

        void Lock(void)
            { EnterCriticalSection(&m_cs); }
        void ReleaseLock(void)
            { LeaveCriticalSection(&m_cs); }

        friend class StructureListIterator;
        friend class AutoLock;
};





///////////////////////////////////////////////////////////////////////////////
// STRUCTURE LIST ITERATOR
///////////////////////////////////////////////////////////////////////////////
class StructureListIterator {
    private:
        StructureList *m_pList;    // Pointer to list being iterated.
        INT            m_Index;    // "Current" signed index into list.
             
        HRESULT Advance(LPVOID *ppvOut, BOOL bForward);

    public:
        enum { EndOfList = -1 };

        StructureListIterator(StructureList& List)
            : m_pList(&List), 
              m_Index(0) { }

        StructureListIterator(const StructureListIterator& rhs)
            : m_pList(rhs.m_pList), 
              m_Index(rhs.m_Index) { }

        StructureListIterator& operator = (const StructureListIterator& rhs);

        HRESULT Next(LPVOID *ppvOut)  
            { return Advance(ppvOut, TRUE); }

        HRESULT Prev(LPVOID *ppvOut)  
            { return Advance(ppvOut, FALSE); }

        BOOL AtFirst(void)
            { return m_Index == 0; }

        BOOL AtLast(void)
            { return m_Index >= (INT)m_pList->Count() - 1; }
        
        void GotoFirst(void)
            { m_Index = 0; }

        void GotoLast(void)
            { m_Index = m_pList->Count() - 1; }

        void LockList(void)
            { m_pList->Lock(); }

        void ReleaseListLock(void)
            { m_pList->ReleaseLock(); }
};

///////////////////////////////////////////////////////////////////////////////
// STRUCTURE QUEUE
///////////////////////////////////////////////////////////////////////////////
class StructureQueue : public StructureList
{
    public:
        StructureQueue(INT cbItem, INT cItemGrow)
            : StructureList(cbItem, cItemGrow) { }

        virtual ~StructureQueue(void) { }

        VOID Add(LPVOID pvItem)
            { StructureList::Append(pvItem); }
        BOOL Remove(LPVOID pvItem)
            { return StructureList::RemoveFirst(pvItem); }
};

#endif // _INC_DSKQUOTA_DYNARRAY_H
