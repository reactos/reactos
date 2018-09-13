/* Template class for doing a simple linked list ...
 */
 
#ifndef _TLIST_H
#define _TLIST_H

// the enum marker that remembers the current position
typedef void * CLISTPOS;

// template class for providing a doubly linked of pointers to nodes
template< class NODETYPE >
class CList
{
    protected:
        struct CNode
        {
            NODETYPE * m_pData;
            CNode * m_pPrev;
            CNode * m_pNext;
        };
        
    public:
    CList();
    ~CList();


    CLISTPOS GetHeadPosition();
    NODETYPE * GetNext( CLISTPOS & rpCurPos );
    int GetCount();
    void RemoveAt( CLISTPOS pPos );
    void RemoveAll( void );
    CLISTPOS FindIndex( int iIndex );
    CLISTPOS AddTail( NODETYPE * pData );
    CLISTPOS AddBefore( CLISTPOS pPos, NODETYPE * pData );

#ifdef DEBUG
    void ValidateList();
#define VALIDATELIST()    ValidateList()
#else
#define VALIDATELIST()
#endif
    
    protected:
        CNode * m_pHead;
        CNode * m_pTail;
};

/////////////////////////////////////////////////////////////////////////////////////////
template< class NODETYPE >
CList<NODETYPE>::CList()
{
    m_pHead = NULL;
    m_pTail = NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
template< class NODETYPE >
CList<NODETYPE>::~CList()
{
    RemoveAll();
}

/////////////////////////////////////////////////////////////////////////////////////////
template< class NODETYPE >
CLISTPOS CList<NODETYPE>::GetHeadPosition( )
{
    return (CLISTPOS) m_pHead;
}

/////////////////////////////////////////////////////////////////////////////////////////
template< class NODETYPE >
NODETYPE * CList<NODETYPE>::GetNext( CLISTPOS & rpCurPos )
{
    Assert( rpCurPos != NULL );
    CNode * pCur = (CNode *) rpCurPos;

    NODETYPE * pData = pCur->m_pData;
    rpCurPos = (CLISTPOS) pCur->m_pNext;
        
    return pData;
}

/////////////////////////////////////////////////////////////////////////////////////////
template< class NODETYPE >
int CList<NODETYPE>::GetCount()
{
    int iLength = 0;
    CNode * pCur = m_pHead;

    while ( pCur != NULL )
    {
        pCur = pCur->m_pNext;
        iLength ++;
    }

    return iLength;
}

/////////////////////////////////////////////////////////////////////////////////////////
template< class NODETYPE >
void CList<NODETYPE>::RemoveAt( CLISTPOS pPos )
{
    Assert( pPos != NULL );
    
#ifdef _DEBUG
    // scan the list to ensure the marker is valid....
    CNode * pCur = m_pHead;

    while ( pCur != NULL )
    {
        if ( pCur == (CNode *) pPos )
        {
            break;
        }
        pCur = pCur->m_pNext;
    }
    Assert( pCur != NULL )
#endif

    CNode * pRealPos = (CNode *) pPos;
    if ( pRealPos->m_pPrev == NULL )
    {
        // we are at the start of the list
        m_pHead = pRealPos->m_pNext;
    }
    else
    {
        // link the prev one to the next one (bypassing this one)
        pRealPos->m_pPrev->m_pNext = pRealPos->m_pNext;
    }
    
    if ( pRealPos->m_pNext == NULL )
    {
        // we are at the end of the list
        m_pTail = pRealPos->m_pPrev;
    }
    else
    {
        // link the next to the prev (bypassing this one)
        pRealPos->m_pNext->m_pPrev = pRealPos->m_pPrev;
    }

    LocalFree( pRealPos );

    VALIDATELIST();
}

/////////////////////////////////////////////////////////////////////////////////////////
template< class NODETYPE >
CLISTPOS CList<NODETYPE>::FindIndex( int iIndex )
{
    Assert( iIndex >= 0 );

    CNode * pCur = m_pHead;
    while ( iIndex > 0 && pCur != NULL )
    {
        pCur = pCur->m_pNext;
        iIndex --;
    }

    return (CLISTPOS)(iIndex == 0 ? pCur : NULL );
}

/////////////////////////////////////////////////////////////////////////////////////////
template< class NODETYPE >
void CList<NODETYPE>::RemoveAll( void )
{
    // note we will not free the data elements, the client must do this...
    CNode * pCur = m_pHead;

    while (pCur != NULL )
    {
        CNode * pTmp = pCur->m_pNext;

        LocalFree( pCur );
        pCur = pTmp;
    }

    m_pHead = m_pTail = NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
template< class NODETYPE >
CLISTPOS CList<NODETYPE>::AddTail( NODETYPE * pData )
{
    CNode * pCurTail = m_pTail;
    CNode * pNewNode = (CNode * ) LocalAlloc( GPTR, sizeof( CNode ));

    if ( pNewNode == NULL )
    {
        return NULL;
    }

    pNewNode->m_pData = pData;
    pNewNode->m_pPrev = pCurTail;
    pNewNode->m_pNext = NULL;
    
    m_pTail = pNewNode;
    
    if ( pCurTail != NULL )
    {
        // we are not an empty list
        pCurTail->m_pNext = pNewNode;
    }
    else
    {
        m_pHead = pNewNode;
    }

    VALIDATELIST();
    
    return (CLISTPOS) pNewNode;
}


/////////////////////////////////////////////////////////////////////////////////////////
template< class NODETYPE >
CLISTPOS CList<NODETYPE>::AddBefore( CLISTPOS pPos, NODETYPE * pData )
{
    if ( !pPos )
    {
        return NULL;
    }

    CNode * pPrev = (CNode *) pPos;
    CNode * pNewNode = (CNode * ) LocalAlloc( GPTR, sizeof( CNode ));
    if ( pNewNode == NULL )
    {
        return NULL;
    }

    pNewNode->m_pData = pData;
    pNewNode->m_pPrev = pPrev->m_pPrev;
    pNewNode->m_pNext = pPrev;

    if ( pPrev->m_pPrev != NULL )
    {
        pPrev->m_pPrev->m_pNext = pNewNode;
    }
    else
    {
        // must be at the start of the list...
        m_pHead = pNewNode;
    }
    
    pPrev->m_pPrev = pNewNode;

    VALIDATELIST();
    
    return (CLISTPOS) pNewNode;
}

/////////////////////////////////////////////////////////////////////////////////////////
#ifdef DEBUG
template< class NODETYPE >
void CList<NODETYPE>::ValidateList( )
{
    CNode * pPos = m_pHead;
    while ( pPos )
    {
        Assert( pPos->m_pData );
        if ( pPos != m_pHead )
        {
            Assert( pPos->m_pPrev );
        }
        pPos = pPos->m_pNext;
    }

    pPos = m_pTail;
    while ( pPos )
    {
        Assert( pPos->m_pData );
        if ( pPos != m_pTail )
        {
            Assert( pPos->m_pNext );
        }
        pPos = pPos->m_pPrev;
    }
    if ( m_pHead || m_pTail )
    {
        Assert( !m_pHead->m_pPrev );
        Assert( m_pTail );
        Assert( m_pHead );
        Assert( !m_pTail->m_pNext );
    }
}

#endif
#endif
