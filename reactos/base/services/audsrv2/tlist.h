//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    tlist.h
//
// Abstract:    
//      This is the header and implementation file for a template list class
//



typedef void* LISTPOS;

template <class T> 
class TItem
{
public:
    TItem();
    ~TItem();
    TItem<T> * pNext;
    TItem<T> * pPrev;
    T* pData;
};

template <class T>
TItem<T>::TItem() :
   pNext(NULL),
   pPrev(NULL),
   pData(NULL)
{
    return;
}

template <class T>
TItem<T>::~TItem() 
{
    return;
}


template <class T>
class TList
{
 public:
    TList();
    ~TList();
    BOOL GetHead( T** ppHead ) const;
    LISTPOS GetHeadPosition() const;
    LISTPOS AddHead(T* pNewHead);
    LISTPOS AddTail(T* pNewTail);
    BOOL RemoveTail( T** ppOldTail);
    BOOL RemoveHead(T** ppOldHead);
    void RemoveAll();
    LISTPOS Find( T* pSearchValue ) const;
    void RemoveAt(LISTPOS pos);
    BOOL GetNext(LISTPOS& rPos, T**ppNext) const;
    BOOL IsEmpty() const;
    DWORD GetCount() const;
    HRESULT Initialize(DWORD dwValue);
    BOOL GetAt( LISTPOS pos, T** ppElement) const;
    LISTPOS InsertBefore(LISTPOS pos, T* pNewElement);
private:
    TItem<T>* m_pHead;
    TItem<T>* m_pTail;
    LONG m_lCount;
};

template <class T>
TList<T>::TList() :
    m_pHead(NULL),
    m_pTail(NULL),
    m_lCount(0)
{
    return;
}

template <class T>
TList<T>::~TList() 
{
    assert(NULL == m_pHead);
    assert(NULL == m_pTail);
    assert(0 == m_lCount);
    return;
}

template <class T>
BOOL TList<T>::GetHead(T** ppHead) const
{
    if (!m_pHead)
    {
        *ppHead = NULL;
        return FALSE;
    }
    
    *ppHead = m_pHead->pData;
    return TRUE;
}

template <class T>
LISTPOS TList<T>::GetHeadPosition() const
{
    return reinterpret_cast<LISTPOS>(m_pHead);
}

template <class T>
LISTPOS TList<T>::AddTail(T* pNewTail)
{
    LISTPOS pos = NULL;
    TItem<T>* pNewItem;

    pNewItem = new TItem<T>;
    if(NULL == pNewItem)
    {
        goto DONE;
    }

    pNewItem->pData = pNewTail;
    pNewItem->pPrev = m_pTail;

    if (NULL == m_pTail)
    {
        m_pHead = pNewItem;
    }
    else
    {
        m_pTail->pNext = pNewItem;
    }

    m_pTail = pNewItem;
        
    m_lCount++;
    
    DONE: 
        
    return reinterpret_cast<LISTPOS>(pNewItem);
}

template <class T>
LISTPOS TList<T>::AddHead(T* pNewHead)
{
    TItem<T> *pNewItem;
    pNewItem = new TItem<T>;
    if( !pNewItem )
    {
        return( reinterpret_cast<LISTPOS>( NULL ) );
    }

    pNewItem->pData = pNewHead;
    pNewItem->pNext = m_pHead;
    pNewItem->pPrev = NULL;

    if( NULL != m_pHead )
    {
        m_pHead->pPrev = pNewItem;
    }
    else
    {
        m_pTail = pNewItem;
    }

    m_pHead = pNewItem;
    m_lCount++;

    return( reinterpret_cast<LISTPOS>( pNewItem) );
    
}

template <class T>    
BOOL TList<T>::RemoveTail( T** ppOldTail)
{

    if (!ppOldTail || !m_pTail)
    {
        return FALSE;
    }

    TItem<T> *pOldItem = m_pTail;
    *ppOldTail = pOldItem->pData;

    m_pTail = pOldItem->pPrev;

    if( NULL != m_pTail )
    {
        m_pTail->pNext = NULL;
    }
    else
    {
        m_pHead = NULL;
    }

    delete pOldItem;
    m_lCount--;
    assert(m_lCount >= 0);
    
    return( TRUE );
}

template <class T>
BOOL TList<T>::RemoveHead(T** ppOldHead)
{
   if( !ppOldHead || !m_pHead )
    {
        return( FALSE );
    }

    TItem<T> *pOldItem = m_pHead;
    *ppOldHead = pOldItem->pData;

    m_pHead = pOldItem->pNext;

    if( NULL != m_pHead )
    {
        m_pHead->pPrev = NULL;
    }
    else
    {
        m_pTail = NULL;
    }

    delete pOldItem;
    m_lCount--;
    assert(m_lCount >= 0);
    
    return( TRUE );
}

template <class T>
void TList<T>::RemoveAll()
{
    TItem<T> *pNext;

    while( NULL != m_pHead )
    {
        pNext = m_pHead->pNext;
        delete m_pHead;
        m_pHead = pNext;
    }

    m_lCount = 0;
    m_pTail = NULL;

    return;
}

template <class T>
LISTPOS TList<T>::Find( T* pSearchValue ) const
{
    TItem<T> *pItem = m_pHead;

    for ( ; NULL != pItem; pItem = pItem->pNext )
    {
        if ( pItem->pData == pSearchValue )
        {
            return( reinterpret_cast<LISTPOS>(pItem) );
        }
    }

    return( reinterpret_cast<LISTPOS>(NULL) );
}

template <class T>
void TList<T>::RemoveAt(LISTPOS pos)
{
    // ASSERT( NULL != pos )
    TItem<T> *pOldItem = reinterpret_cast< TItem<T>* >(pos);

    // remove pOldItem from list
    if( pOldItem == m_pHead )
    {
        m_pHead = pOldItem->pNext;
    }
    else
    {
        assert( pOldItem->pPrev );
        pOldItem->pPrev->pNext = pOldItem->pNext;
    }

    if (pOldItem == m_pTail)
    {
        m_pTail = pOldItem->pPrev;
    }
    else
    {
        assert( pOldItem->pNext );
        pOldItem->pNext->pPrev = pOldItem->pPrev;
    }

    delete pOldItem;
    m_lCount--;
    assert(m_lCount >= 0);
    return;
}

template <class T>
BOOL TList<T>::GetNext(LISTPOS& rPos, T**ppNext) const
{
    TItem<T> *pItem = reinterpret_cast< TItem<T>* >(rPos);

    if( !pItem )
    {
        return( FALSE );
    }

    rPos = reinterpret_cast<LISTPOS>(pItem->pNext);
    *ppNext = pItem->pData;

    return( TRUE );
}

template <class T>
BOOL TList<T>::IsEmpty() const
{
    return (0 == m_lCount);
}

template <class T>
HRESULT TList<T>::Initialize(DWORD dwValue)
{
    HRESULT hr = S_OK;
    return hr;
}

template <class T>
DWORD TList<T>::GetCount() const
{
    return static_cast<DWORD>(m_lCount);
}


template <class T>
 LISTPOS TList<T>::InsertBefore(LISTPOS pos, T* pNewElement)
{
    if( ( NULL == pos ) || ( ( reinterpret_cast<TItem<T>*>(pos) )->pPrev == NULL ) )
    {
        return( AddHead( pNewElement ) ); // insert before nothing -> head of the list
    }

    // Insert it before pos
    TItem<T> *pOldItem = reinterpret_cast<TItem<T>*>(pos);

    TItem<T> *pNewItem;

    pNewItem = new TItem<T>;
    if( !pNewItem )
    {
        return( reinterpret_cast<LISTPOS>(NULL) );
    }

    pNewItem->pData = pNewElement;
    pNewItem->pPrev = pOldItem->pPrev;
    pNewItem->pNext = pOldItem;

    pOldItem->pPrev->pNext = pNewItem;
    pOldItem->pPrev = pNewItem;

    m_lCount++;

    return( reinterpret_cast<LISTPOS>(pNewItem) );
}


template <class T>
BOOL TList<T>::GetAt( LISTPOS pos, T** ppElement) const
{
    TItem<T> *pItem = reinterpret_cast<TItem<T>*>(pos);

    if( !pItem )
    {
        return( FALSE );
    }

    *ppElement = pItem->pData;

    return( TRUE );
}

