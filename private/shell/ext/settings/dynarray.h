#ifndef __DYNAMIC_ARRAYS_H
#define __DYNAMIC_ARRAYS_H
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

///////////////////////////////////////////////////////////////////////////////
// POINTER LIST
///////////////////////////////////////////////////////////////////////////////
class PointerList
{
    private:
        HDPA   m_hdpa;
        HANDLE m_hMutex;

    public:
        PointerList(void);

        virtual ~PointerList(void);

        UINT Count(void);

        VOID Insert(LPVOID pvItem, UINT index) throw(OutOfMemory);
        VOID Insert(LPVOID pvItem) throw (OutOfMemory)
            { Insert(pvItem, 0); }

        VOID Append(LPVOID pvItem, UINT index) throw(OutOfMemory)
            { Insert(pvItem, index + 1); }
        VOID Append(LPVOID pvItem) throw(OutOfMemory);

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

        void Lock(void)
            { WaitForSingleObject(m_hMutex, INFINITE); }
        void ReleaseLock(void)
            { ReleaseMutex(m_hMutex); }

        friend class PointerListIterator;
#ifdef DA_MULTITHREADED
        friend class AutoLock;
#endif
};



///////////////////////////////////////////////////////////////////////////////
// POINTER LIST ITERATOR
///////////////////////////////////////////////////////////////////////////////
class PointerListIterator {
    private:
        PointerList& m_List;     // Reference to list being iterated.
        INT          m_Index;    // "Current" signed index into list.
             
        HRESULT Advance(LPVOID *ppvOut, BOOL bForward);

        PointerListIterator operator = (const PointerListIterator& );
        PointerListIterator(const PointerListIterator& );

    public:
        enum { EndOfList = -1 };

        PointerListIterator(PointerList& List)
            : m_List(List), 
              m_Index(0) { }

        HRESULT Next(LPVOID *ppvOut)  
            { return Advance(ppvOut, TRUE); }

        HRESULT Prev(LPVOID *ppvOut)  
            { return Advance(ppvOut, FALSE); }

        void GotoFirst(void)
            { m_Index = 0; }

        void GotoLast(void)
            { m_Index = m_List.Count() - 1; }

        void LockList(void)
            { m_List.Lock(); }

        void ReleaseListLock(void)
            { m_List.ReleaseLock(); }
};


///////////////////////////////////////////////////////////////////////////////
// POINTER QUEUE
///////////////////////////////////////////////////////////////////////////////
class PointerQueue : public PointerList
{
    public:
        virtual ~PointerQueue(void) { }

        VOID Add(LPVOID pvItem) throw(OutOfMemory)
            { PointerList::Insert(pvItem); }
        BOOL Remove(LPVOID *ppvItem)
            { return PointerList::RemoveLast(ppvItem); }
};


///////////////////////////////////////////////////////////////////////////////
// STRUCTURE LIST
//
// BUGBUG; Add exception handling and AutoLock just like in PointerList.
//
///////////////////////////////////////////////////////////////////////////////
class StructureList
{
    private:
        HDSA   m_hdsa;
        HANDLE m_hMutex;

    public:
        StructureList(INT cbItem, INT cItemGrow);

        virtual ~StructureList(void);

        UINT    Count(void);

        VOID Insert(LPVOID pvItem, UINT index) throw(OutOfMemory);
        VOID Insert(LPVOID pvItem) throw(OutOfMemory)
            { Insert(pvItem, 0); }

        VOID Append(LPVOID pvItem, UINT index) throw(OutOfMemory)
            { Insert(pvItem, index + 1); }
        VOID Append(LPVOID pvItem) throw(OutOfMemory);

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
            { WaitForSingleObject(m_hMutex, INFINITE); }
        void ReleaseLock(void)
            { ReleaseMutex(m_hMutex); }

        friend class StructureListIterator;
#ifdef DA_MULTITHREADED
        friend class AutoLock;
#endif
};





///////////////////////////////////////////////////////////////////////////////
// STRUCTURE LIST ITERATOR
///////////////////////////////////////////////////////////////////////////////
class StructureListIterator {
    private:
        StructureList& m_List;     // Reference to list being iterated.
        INT            m_Index;    // "Current" signed index into list.
             
        HRESULT Advance(LPVOID *ppvOut, BOOL bForward);

        StructureListIterator operator = (const StructureListIterator& );
        StructureListIterator(const StructureListIterator& );

    public:
        enum { EndOfList = -1 };

        StructureListIterator(StructureList& List)
            : m_List(List), 
              m_Index(0) { }

        HRESULT Next(LPVOID *ppvOut)  
            { return Advance(ppvOut, TRUE); }

        HRESULT Prev(LPVOID *ppvOut)  
            { return Advance(ppvOut, FALSE); }

        void GotoFirst(void)
            { m_Index = 0; }

        void GotoLast(void)
            { m_Index = m_List.Count() - 1; }

        void LockList(void)
            { m_List.Lock(); }

        void ReleaseListLock(void)
            { m_List.ReleaseLock(); }
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

        VOID Add(LPVOID pvItem) throw(OutOfMemory)
            { StructureList::Insert(pvItem); }
        BOOL Remove(LPVOID pvItem)
            { return StructureList::RemoveLast(pvItem); }
};

#endif // __DYNAMIC_ARRAYS_H
