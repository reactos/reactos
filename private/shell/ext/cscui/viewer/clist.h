//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       clist.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCVIEW_CLIST_H
#define _INC_CSCVIEW_CLIST_H
///////////////////////////////////////////////////////////////////////////////
/*  File: clist.h

    Description: A simple doubly-linked list template class.

        classes:  CList<T>          - Generic list.
                  CQueueAsList<T>   - Queue implemented as a list.
                  CStackAsList<T>   - Stack implemented as a list.
                  CListIterator<T>  - Iterator for walking a list.
            
    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/16/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
template <class T> class CListIterator; // fwd decl.

template <class T>
class CListEntry
{
    public:
        CListEntry(void)
            : m_pNext(NULL),
              m_pPrev(NULL) { }

        CListEntry(const T& item)
            : m_pNext(NULL),
              m_pPrev(NULL),
              m_item(item) { }

        CListEntry<T> *m_pNext;
        CListEntry<T> *m_pPrev;
        T              m_item;

    private:
        CListEntry(const CListEntry<T>& rhs);
        CListEntry<T>& operator = (const CListEntry<T>& rhs);
};


template <class T>
class CList 
{

    public:
        CList(void) throw();
        virtual ~CList(void) throw();

        bool Append(const T& item);
        bool Insert(const T& item);
        bool RetrieveHead(T *pItem, bool bRemove = false) throw();
        bool RetrieveTail(T *pItem, bool bRemove = false) throw();
        bool RemoveHead(T *pItem) throw();
        bool RemoveTail(T *pItem) throw();

        int Count(void) const throw()
            { return m_cItems; }
        void Clear(void) throw();

    private:
        CListEntry<T> m_ZNode;
        int   m_cItems;

        bool Retrieve(T *pItem, CListEntry<T> *pEntry, bool bRemove) throw();
        CListEntry<T> *FindEntry(const T& key) throw();
        void Unlink(CListEntry<T> *pEntry) throw();
        void Link(CListEntry<T> *pEntry, CListEntry<T> *pPrev) throw();

        friend class CListIterator<T>;
};


template <class T>
CList<T>::CList(
    void
    ) throw()
      : m_cItems(0)
{
    m_ZNode.m_pNext = m_ZNode.m_pPrev = &m_ZNode;
}

template <class T>
CList<T>::~CList(
    void
    ) throw()
{
    Clear();
}


template <class T>
void
CList<T>::Unlink(
    CListEntry<T> *pEntry
    ) throw()
{
    pEntry->m_pNext->m_pPrev = pEntry->m_pPrev;
    pEntry->m_pPrev->m_pNext = pEntry->m_pNext;
    pEntry->m_pPrev = pEntry->m_pNext = NULL;
    m_cItems--;
}


template <class T>
void
CList<T>::Link(
    CListEntry<T> *pEntry,
    CListEntry<T> *pPrev
    ) throw()
{
    pEntry->m_pPrev = pPrev;
    pEntry->m_pNext = pPrev->m_pNext;

    pPrev->m_pNext = pPrev->m_pNext->m_pPrev = pEntry;
    m_cItems++;
}

template <class T>
void
CList<T>::Clear(
    void
    ) throw()
{
    CListEntry<T> *pDelThis = m_ZNode.m_pNext;
    while(&m_ZNode != pDelThis)
    {
        Unlink(pDelThis);
        delete pDelThis;
        pDelThis = m_ZNode.m_pNext;
    }
}


//
// Add item before ZNode.
//
template <class T>
bool
CList<T>::Append(
    const T& item
    )
{
    CListEntry<T> *pEntry = new CListEntry<T>(item);
    if (NULL != pEntry)
    {
        Link(pEntry, m_ZNode->m_pPrev);
        return true;
    }
    return false;
}


//
// Add item after ZNode.
//
template <class T>
bool
CList<T>::Insert(
    const T& item
    )
{
    CListEntry<T> *pEntry = new CListEntry<T>(item);
    if (NULL != pEntry)
    {
        Link(pEntry, &m_ZNode);
        return true;
    }
    return false;
}



template <class T>
CListEntry<T> *
CList<T>::FindEntry(
    const T& key
    ) throw()
{
    for (CListEntry<T> *pEntry = m_ZNode.m_pNext; &m_ZNode != pEntry; pEntry = pEntry->m_pNext)
    {
        if (pEntry->m_item == key)
            return pEntry;
    }
    return NULL;
}


template <class T>
bool
CList<T>::RetrieveHead(
    T *pItem,
    bool bRemove
    ) throw()
{
    return Retrieve(pItem, m_ZNode.m_pNext, bRemove);
}


template <class T>
bool
CList<T>::RetrieveTail(
    T *pItem,
    bool bRemove
    ) throw()
{
    return Retrieve(pItem, m_ZNode.m_pPrev, bRemove);
}


template <class T>
bool
CList<T>::Retrieve(
    T *pItem,
    CListEntry<T> *pEntry,
    bool bRemove
    ) throw()
{
    DBGASSERT((NULL != pItem));
    if (pEntry != &m_ZNode)
    {
        *pItem = pEntry->m_item;
        if (bRemove)
        {
            Unlink(pEntry);
            delete pEntry;
        }

        return true;
    }
    return false;
}


template <class T>
bool
CList<T>::RemoveHead(
    T *pItem
    ) throw()
{
    return RetrieveHead(pItem, true);
}

template <class T>
bool
CList<T>::RemoveTail(
    T *pItem
    ) throw()
{
    return RetrieveTail(pItem, true);
}


template <class T>
class CQueueAsList : public CList<T>
{
    public:
        CQueueAsList(void) { }
        ~CQueueAsList(void) { }

        bool Add(const T& item)
            { return Insert(item); }
        bool Remove(T *pItem)
            { return RemoveTail(pItem); }
        bool Retrieve(T *pItem)
            { return RetrieveTail(pItem); }
};


template <class T>
class CStackAsList : public CList<T>
{
    public:
        CStackAsList(void) { }
        ~CStackAsList(void) { }

        bool Push(const T& item)
            { return Insert(item); }
        bool Pop(T *pItem)
            { return RemoveHead(pItem); }
        bool Peek(T *pItem)
            { return RetrieveHead(pItem); }
};


template <class T>
class CListIterator
{
    public:
        CListIterator(const CList<T>& list)
            : m_list(list),
              m_pEntry(NULL) { Reset(); }

        void Reset(void)
            { m_pEntry = m_list.m_ZNode.m_pNext; }

        bool Next(T **ppItem);

    private:
        const CList<T>& m_list;
        CListEntry<T> *m_pEntry;

};
              
template <class T>
bool
CListIterator<T>::Next(
    T **ppItem
    )
{
    DBGASSERT((NULL != ppItem));
    if (m_pEntry != &m_list.m_ZNode)
    {
        *ppItem = &(m_pEntry->m_item);
        m_pEntry = m_pEntry->m_pNext;
        return true;
    }
    return false;
}

#endif // _INC_CSCVIEW_CLIST_H
