//-------------------------------------------------------------------------//
//
//	TPriorityList.h
//
//-------------------------------------------------------------------------//
//	@doc

#ifndef	__TPRIORITYLIST_H__
#define __TPRIORITYLIST_H__

#ifndef ALL_WARNINGS
#	pragma warning(disable: 4114)
#endif

#include "cpool.h"

#if defined(_NTDDK_) || defined(_NTIFS_)
#	define NT_KERNELMODE
//#	pragma message( "\t\tTPriorityList.h - NT Kernel Mode build" )
#elif defined(_WIN32)
#	define WIN32_USERMODE
//#	pragma message( "\t\tTPriorityList.h - NT Kernel Mode build" )
#endif


#define TPRIORITYLIST_TEMPLATE	template <class ELEMENT_TYPE>
#define TPRIORITYLIST			TPriorityList<ELEMENT_TYPE>
#define DEFAULT_LIST_PRIORITY	0L 

//-------------------------------------------------------------------------//
//	@class Doubly-linked priority list collection.<nl><nl>
//	The TPriorityList template is a versatile collection class that 
//	can serve as a doubly- or singly-linked priority list, priority stack,
//	or a priority queue, or as a non-prioritized list, stack or queue.<nl><nl>
//	Priority is assigned to collection elements via an argument passed to the
//	<mf TPriorityList.InsertHead> and <mf TPriorityList.AppendTail> methods.
//	To implement a non-prioritized collection, use DEFAULT_LIST_PRIORITY as 
//	the value of the priority parameter.  Otherwise, elements will be sorted in 
//	descending priority when added to the list.
//	@tcarg class | ELEMENT_TYPE | Collection member type.
template <class ELEMENT_TYPE> class TPriorityList
//-------------------------------------------------------------------------//
{
public:

//	@access Public construction, destruction
#ifdef NT_KERNELMODE
	//	@cmember Constructs a TPriorityList instance.
	TPriorityList( POOL_TYPE PoolType = NonPagedPool, ULONG nBlockSize = 16 ) ;
#endif

#ifdef WIN32_USERMODE
	TPriorityList( ULONG nBlockSize = 16 ) ;
#endif

	virtual ~TPriorityList() ;

//	@access Public attributes
public:
	//	@cmember Returns the number of elements in the list.
	int		Count() const ;
	//	@cmember Reports whether the list is empty.
	BOOLEAN	IsEmpty() const ;

//	@access Thread synchronization <em-> Public members
public:
	//	@cmember Acquires a mutual exclusion lock on the list.
	//	This should be done immediately prior to each call to
	//	a non-const method.
	void AcquireMutex() ;

	//	@cmember Releases a mutual exclusion lock on the list.
	//	This should be done immediately following each call to
	//	a non-const method.
	void ReleaseMutex() ;


//	@access Element insertion <em-> Public methods
public:
	//	@cmember Inserts an element at the head of its priority group in the list.
	//	This call should be synchronized with Acquire/ReleaseMutex.
	BOOLEAN InsertHead( const ELEMENT_TYPE& val, ULONG priority=DEFAULT_LIST_PRIORITY ) ;
	
	//	@cmember Appends an element to the tail of its priority group in the list.
	//	This call should be synchronized with Acquire/ReleaseMutex.
	BOOLEAN AppendTail( const ELEMENT_TYPE& val, ULONG priority=DEFAULT_LIST_PRIORITY ) ;


//	@access Element removal <em-> Public members
public:
	//	@cmember Removes the element at the head of the list.
	//	This call should be synchronized with Acquire/ReleaseMutex.
	BOOLEAN PopHead( ELEMENT_TYPE* pVal=NULL ) ;

    //  @cmember Removes the specified element from the list.
    //  This call should be synchronized with Acquire/ReleaseMutex.
    BOOLEAN RemoveAt( HANDLE hEnumOrFind ) ;
	
	//	@cmember Removes the element at the tail of the list.
	//	This call should be synchronized with Acquire/ReleaseMutex.
	BOOLEAN PopTail( ELEMENT_TYPE* pVal=NULL ) ;
	
	//	@cmember Removes all elements of the list.
	virtual void Clear() ;

    //  @cmember Notifies derived class that an element has been freed.
    virtual void OnFreeElement( ELEMENT_TYPE& type ) {
        UNREFERENCED_PARAMETER( type ) ;
    }

//	@access Element retrieval and enumeration <em-> Public members
public:
	//	Note: An entire enumeration sequence (EnumHead/Tail, EnumNext/Prev, EndEnum)
	//	should be synchonized as a block; i.e., call AcquireMutex() before
	//	beginning enumeration, and don't release it until enumeration
	//	is finished.

	//	@cmember Begins enumeration from the head of the list.
	HANDLE	EnumHead( ELEMENT_TYPE& val, ULONG nPriority=DEFAULT_LIST_PRIORITY ) const ;
	//	@cmember Begins enumeration from the tail of the list.
	HANDLE  EnumTail( ELEMENT_TYPE& val, ULONG nPriority=DEFAULT_LIST_PRIORITY ) const ;
	//	@cmember Retrieves next element in enumeration
	BOOLEAN	EnumNext( HANDLE hEnum, ELEMENT_TYPE& val ) const ;
	//	@cmember Retrieves previous element in enumeration
	BOOLEAN	EnumPrev( HANDLE hEnum, ELEMENT_TYPE& val ) const ;
	//	@cmember Terminates an enumeration begun with EnumHead() or EnumTail().
	void	EndEnum( HANDLE hEnum ) const ;

	//	@cmember Retrieves the address of the element at the head of the list.
	//	This call should be synchronized with Acquire/ReleaseMutex.
	//	Multithread Warning: After releasing the list's mutex, 
	//	the head's value may change at any time!.
	BOOLEAN	PeekHead( ELEMENT_TYPE** ppVal ) ;
	
	//	@cmember Retrieves the address of the element at the tail of the list.
	//	This call should be synchronized with Acquire/ReleaseMutex.
	//	Multithread Warning: After releasing the list's mutex, 
	//	the tail's value may change at any time!.
	BOOLEAN	PeekTail( ELEMENT_TYPE** ppVal ) ;

//--- Implementation Details...

	//	Diagnostics
#ifdef _DEBUG
	void				Trace();
#endif _DEBUG

	struct ELEMENT {
		LIST_ENTRY		link ;
		ULONG			priority ;
		ELEMENT_TYPE	data ;
	} ;

protected:
	//	Protected enumeration structure.
	struct ENUM	{
		ELEMENT* pE ;
		ULONG	 priority ;
	} ;

	//	Helper routines and macros
	ELEMENT*			Head() const ;
	ELEMENT*			Tail() const ;
    ELEMENT*            Next( ELEMENT *pE ) const { return (ELEMENT*)pE->link.Flink ; }
	ELEMENT*            Prev( ELEMENT *pE )	const { return (ELEMENT*)pE->link.Blink ; }

	#define				AssignNext( pE, pOther ) (((PLIST_ENTRY)pE)->Flink = (PLIST_ENTRY)pOther)
	#define				AssignPrev( pE, pOther ) (((PLIST_ENTRY)pE)->Blink = (PLIST_ENTRY)pOther)
	ELEMENT*			AllocElement() ;
	void	            FreeElement( ELEMENT* ) ;
	void				InsertAfter( ELEMENT* pDest, ELEMENT* ) ;
	void				InsertBefore( ELEMENT* pDest, ELEMENT* ) ;

	//	Data members
	ULONG				m_cElements ;
	ELEMENT				*m_pHead,
						*m_pFreeHead ;
	struct CPool		*m_pBlocks ;
	ULONG				m_nBlockSize ;

#ifdef WIN32_USERMODE
	CRITICAL_SECTION	m_critsect ;
#endif

#ifdef NT_KERNELMODE
	POOL_TYPE			m_poolType ;
	BOOLEAN				m_fPassive ;
	KSPIN_LOCK			m_spinLock ;
	KIRQL				m_spinLockIrql ;
	FAST_MUTEX			m_fastmutex ;
#endif
} ;


#ifdef NT_KERNELMODE
//-------------------------------------------------------------------------//
TPRIORITYLIST_TEMPLATE
TPRIORITYLIST::TPriorityList( POOL_TYPE poolType, ULONG nBlockSize )
	:	m_poolType(poolType),
		m_fPassive(poolType==PagedPool||poolType==PagedPoolCacheAligned),
		m_spinLockIrql(PASSIVE_LEVEL),
		m_pHead(NULL),
		m_pFreeHead(NULL),
		m_pBlocks(NULL),
		m_nBlockSize(nBlockSize),
		m_cElements(0)
{
	if( m_fPassive )	{
		ExInitializeFastMutex( &m_fastmutex ) ;
	}
	else	{
		KeInitializeSpinLock( &m_spinLock ) ;
	}
}
#else
//-------------------------------------------------------------------------//
TPRIORITYLIST_TEMPLATE
TPRIORITYLIST::TPriorityList( ULONG nBlockSize )
	:	m_pHead(NULL),
		m_pFreeHead(NULL),
		m_pBlocks(NULL),
		m_nBlockSize(nBlockSize),
		m_cElements(0)
{
	InitializeCriticalSection( &m_critsect ) ;
}
#endif

//-------------------------------------------------------------------------//
TPRIORITYLIST_TEMPLATE
TPRIORITYLIST::~TPriorityList()
{
	Clear() ;
	ASSERT( m_cElements==0 ) ;

#ifdef WIN32_USERMODE
	DeleteCriticalSection( &m_critsect ) ;
#endif

}

//-------------------------------------------------------------------------//
//	Acquires a mutual exclusion lock on the list.
//	This should be done immediately prior to each call to
//	a non-const method.
TPRIORITYLIST_TEMPLATE
void TPRIORITYLIST::AcquireMutex()
{
#ifdef NT_KERNELMODE
	if( m_fPassive )	{
		ExAcquireFastMutex( &m_fastmutex ) ;
	}
	else	{
		KeAcquireSpinLock( &m_spinLock, &m_spinLockIrql ) ;
	}
#endif

#ifdef WIN32_USERMODE
	EnterCriticalSection( &m_critsect ) ;
#endif
}

//-------------------------------------------------------------------------//
//	Releases a mutual exclusion lock on the list.
//	This should be done immediately following each call to
//	a non-const method.
TPRIORITYLIST_TEMPLATE
void TPRIORITYLIST::ReleaseMutex()
{
#ifdef NT_KERNELMODE
	if( m_fPassive )	{
		ExReleaseFastMutex( &m_fastmutex ) ;
	}
	else	{
		KeReleaseSpinLock( &m_spinLock, m_spinLockIrql ) ;
	}
#endif

#ifdef WIN32_USERMODE
	LeaveCriticalSection( &m_critsect ) ;
#endif
}

//-------------------------------------------------------------------------//
TPRIORITYLIST_TEMPLATE
void TPRIORITYLIST::Clear()
{
	//	Kill our pools and reinitialize our stats.
	m_cElements = 0 ;
	m_pHead = NULL ;
	m_pFreeHead = NULL ;
	m_pBlocks->FreeChain() ;
	m_pBlocks = NULL ;
}

//-------------------------------------------------------------------------//
TPRIORITYLIST_TEMPLATE
inline int TPRIORITYLIST::Count() const	{ 
	return m_cElements ;
}

//-------------------------------------------------------------------------//
TPRIORITYLIST_TEMPLATE
inline BOOLEAN TPRIORITYLIST::IsEmpty() const	{ 
	return m_cElements == 0  ;
}


//-------------------------------------------------------------------------//
// Entry allocation helper
TPRIORITYLIST_TEMPLATE
TPRIORITYLIST::ELEMENT* TPRIORITYLIST::AllocElement()
{
	ELEMENT* pE ;

	if( m_pFreeHead == NULL )
	{
		// add another block

#if defined(WIN32_USERMODE)
//	user-mode version:
		CPool* newBlock = CPool::Create( m_pBlocks, m_nBlockSize, sizeof(ELEMENT) ) ;
#elif defined(NT_KERNELMODE)
//	kernel-mode version:
		CPool* newBlock = CPool::Create( m_poolType, m_pBlocks, m_nBlockSize, sizeof(ELEMENT) ) ;
#endif
		// chain them into free list
		pE = (ELEMENT*)newBlock->data() ;
		
		// free in reverse order to make it easier to debug
		pE += m_nBlockSize - 1 ;
		for ( int i = m_nBlockSize-1 ; i >= 0 ; i--, pE-- )
		{
			AssignNext( pE, m_pFreeHead ) ;
			m_pFreeHead = pE ;
		}
	}
	ASSERT( m_pFreeHead != NULL ) ; // we must have a list of free entries!

	//	Pull head element off the free list...
	pE =			m_pFreeHead ;	
	m_pFreeHead =	Next( pE ) ;	// Assign element's "next" as new free head
	AssignNext( pE, NULL ) ;		// orphan the element
	AssignPrev( pE, NULL ) ;		// ""
	m_cElements++ ;					// increment count of elements
	ASSERT( m_cElements > 0 ) ;		// make sure we don't overflow

	return pE ;
}

//-------------------------------------------------------------------------//
TPRIORITYLIST_TEMPLATE
void TPRIORITYLIST::FreeElement( ELEMENT* pE )
{
	if( !pE ) return ;

	//	splice element out of list.
	if( Prev( pE ) )
		AssignNext( Prev( pE ), Next( pE ) ) ;
	if( Next( pE ) )
		AssignPrev( Next( pE ), Prev( pE ) ) ;

	//	Reassign head as necessary
	if( pE==m_pHead )
		m_pHead = Next( pE ) ;

	OnFreeElement( pE->data ) ;
    
    //	Only one element?  Clear everything.
	if( !m_pHead )
		Clear() ;
	else
	{
		//	Clear the contents of the freed element.
		memset( &pE->data, 0, sizeof(pE->data) ) ;
		//	Make freed element head of free list.
		if( m_pFreeHead )
			AssignPrev( m_pFreeHead, pE ) ;
		AssignNext( pE, m_pFreeHead ) ;
		AssignPrev( pE, NULL ) ;
		m_pFreeHead = pE ;
		//	Decrement element count
		m_cElements-- ;
	}
}

//-------------------------------------------------------------------------//
TPRIORITYLIST_TEMPLATE
inline TPRIORITYLIST::ELEMENT* TPRIORITYLIST::Head() const
{
	return m_pHead ;
}

//-------------------------------------------------------------------------//
TPRIORITYLIST_TEMPLATE
inline TPRIORITYLIST::ELEMENT* TPRIORITYLIST::Tail() const
{
	ELEMENT	*pE, *pTail ;

	if( !m_pHead ) return NULL ;

	for( pE = Next( m_pHead ), pTail = m_pHead; pE ; 
		 pE = Next( pE ) )
		pTail = pE ;

	ASSERT( pTail ) ;
	return pTail ;
}

//-------------------------------------------------------------------------//
//	Inserts an element at the head of the list
TPRIORITYLIST_TEMPLATE
BOOLEAN TPRIORITYLIST::InsertHead( const ELEMENT_TYPE& val, ULONG priority )
{
	BOOLEAN	bRet = FALSE ;
	ELEMENT *pE ;
	
	if( (pE = AllocElement()) )
	{		
		pE->priority = priority ;
		pE->data = val ;

		if( !m_pHead )
		{
			m_pHead = pE ;
			bRet = TRUE ;
		}
		else
		{
			//	Walk list looking for insertion point
			ELEMENT* pHead ;
			for( pHead = m_pHead; pHead; pHead=Next( pHead ) )
			{
				if( pHead->priority<=priority )
				{
					InsertBefore( pHead, pE ) ;
					if( pHead==m_pHead )
						m_pHead = pE ;
					bRet = TRUE ;
					break ;
				}
			}
		}
		ASSERT( bRet ) ;
	}

	return bRet ;
}

//-------------------------------------------------------------------------//
//	Appends an element to the tail of the list
TPRIORITYLIST_TEMPLATE
BOOLEAN TPRIORITYLIST::AppendTail( const ELEMENT_TYPE& val, ULONG priority )
{
	BOOLEAN	bRet = FALSE ;
	ELEMENT	*pE, *pTail ;

	if( !m_pHead )
		return InsertHead( val, priority ) ;
	
	if( (pE = AllocElement()) )
	{
		pE->priority = priority ;
		pE->data = val ;
		
		//	Walk list looking for insertion point
		for( pTail=Tail(); pTail; pTail = Prev( pTail ) )
		{
			//	Have we encountered a priority higher than us?
			if( pTail->priority>=priority )
			{
				//	Yes, insert after
				InsertAfter( pTail, pE ) ;
				bRet = TRUE ;
				break ;
			}
			//	Otherwise, have we reached the head?
			else if( pTail==m_pHead )
			{
				//	Yes, insert as new head.
				InsertBefore( pTail, pE ) ;
				m_pHead = pE ;
				bRet = TRUE ;
				break ;
			}
		}
		ASSERT( bRet ) ;
	}

	return bRet ;
}

//-------------------------------------------------------------------------//
TPRIORITYLIST_TEMPLATE
void TPRIORITYLIST::InsertAfter( ELEMENT* pDest, ELEMENT* pSrc )
{
	ASSERT( pSrc ) ;
	if( pDest )
	{
		AssignPrev( pSrc, pDest ) ;
		if( AssignNext( pSrc, Next( pDest ) ) )
			AssignPrev( Next( pSrc ), pSrc ) ;
		AssignNext( pDest, pSrc ) ;
	}
	else
	{
		AssignPrev( pSrc, NULL ) ;
		AssignNext( pSrc, NULL ) ;
	}
}

//-------------------------------------------------------------------------//
TPRIORITYLIST_TEMPLATE
void TPRIORITYLIST::InsertBefore( ELEMENT* pDest, ELEMENT* pSrc )
{
	ASSERT( pSrc ) ;
	
	if( pDest )
	{
		AssignNext( pSrc, pDest ) ;
		if( AssignPrev( pSrc, Prev( pDest ) ) )
			AssignNext( Prev( pSrc ), pSrc ) ;
		AssignPrev( pDest, pSrc ) ;
	}
	else
	{
		AssignPrev( pSrc, NULL ) ;
		AssignNext( pSrc, NULL ) ;
	}
}

//-------------------------------------------------------------------------//
//	Retrieves the element at the head of the list.
TPRIORITYLIST_TEMPLATE
BOOLEAN	TPRIORITYLIST::PeekHead( ELEMENT_TYPE** ppVal )
{
	ELEMENT* pE ;
	ASSERT( ppVal ) ;
	*ppVal = NULL ;

	if( (pE = Head())==NULL )
		return FALSE ;
	
	*ppVal = &m_pHead->data ;
	return TRUE ;
}

//-------------------------------------------------------------------------//
//	Retrieves the element at the tail of the list.
TPRIORITYLIST_TEMPLATE
BOOLEAN	TPRIORITYLIST::PeekTail( ELEMENT_TYPE** ppVal )
{
	ELEMENT* pTail ;
	ASSERT( ppVal ) ;
	*ppVal = NULL ;

	if( (pTail = Tail())==NULL )
		return FALSE ;
	
	(*ppVal) = &pTail->data ;
	return TRUE ;
}

//-------------------------------------------------------------------------//
//	Removes the element at the head of the list.
TPRIORITYLIST_TEMPLATE
BOOLEAN TPRIORITYLIST::PopHead( ELEMENT_TYPE* pVal )
{
	ELEMENT* pE ; 

	if( (pE = Head())==NULL )
		return FALSE ;

	if( pVal )
		*pVal = pE->data ;
	FreeElement( pE ) ;
	return TRUE ;
}

//-------------------------------------------------------------------------//
//  
TPRIORITYLIST_TEMPLATE
BOOLEAN TPRIORITYLIST::RemoveAt( HANDLE hEnum )
{
	ENUM         *pEnum ;
    ELEMENT      *pE = NULL ; 
    ELEMENT_TYPE next ;

	if( !( (pEnum = (ENUM*)hEnum) && (pE = pEnum->pE) ) )
		return FALSE ;

    //  Skip past this one
    EnumNext( hEnum, next ) ;
    FreeElement( pE ) ;
    return TRUE ;
}

//-------------------------------------------------------------------------//
//	Removes the element at the tail of the list.
TPRIORITYLIST_TEMPLATE
BOOLEAN TPRIORITYLIST::PopTail( ELEMENT_TYPE* pVal )
{
	ELEMENT* pE ;
	if( (pE = Tail())==NULL )
		return FALSE ;

	if( pVal )
		*pVal = pE->data ;
	FreeElement( pE ) ;
	return TRUE ;
}

//-------------------------------------------------------------------------//
//	Begins enumeration from the head of the list.
TPRIORITYLIST_TEMPLATE
HANDLE TPRIORITYLIST::EnumHead( ELEMENT_TYPE& val, ULONG nPriority ) const
{
	ENUM* pEnum = NULL ;
	ELEMENT* pE ;

	if( !m_pHead )
		return pEnum ;

	for( pE = m_pHead; pE; pE = Next( pE ) )	{
		if( nPriority == DEFAULT_LIST_PRIORITY || pE->priority==nPriority )
			break ;
	}

	if( !(pE && (pEnum = new ENUM)) )
		return NULL ;
	
	memset( pEnum, 0, sizeof(*pEnum) ) ;
    pEnum->pE = pE ;
	pEnum->priority = nPriority ;
	val = pE->data ;
	return pEnum ;
}

//-------------------------------------------------------------------------//
//	Begins enumeration from the tail of the list.
TPRIORITYLIST_TEMPLATE
HANDLE TPRIORITYLIST::EnumTail( ELEMENT_TYPE& val, ULONG nPriority ) const
{
	ENUM	*pEnum = NULL ;
	ELEMENT	*pE, *pTail ;

	if( !(pTail=Tail()) )
		return pEnum ;

	for( pE = pTail; pE; pE = Prev( pE ) )	{
		if( nPriority==DEFAULT_LIST_PRIORITY || pE->priority==nPriority )
		{
			val = pE->data ;
			break ;
		}
	}

	if( !(pE && (pEnum = new ENUM)) )
		return NULL ;
	
	memset( pEnum, 0, sizeof(*pEnum) ) ;
    pEnum->pE = pE ;
	pEnum->priority = nPriority ;
	return pEnum ;
}

//-------------------------------------------------------------------------//
//	Retrieves next element in enumeration
TPRIORITYLIST_TEMPLATE
BOOLEAN TPRIORITYLIST::EnumNext( HANDLE hEnum, ELEMENT_TYPE& val ) const
{
	ENUM *pEnum ;
	if( !((pEnum = (ENUM*)hEnum) && pEnum->pE) )
		return FALSE ;

	for( pEnum->pE = Next( pEnum->pE ); 
		 pEnum->pE; 
		 pEnum->pE = Next( pEnum->pE ) )
	{
		if( pEnum->priority==DEFAULT_LIST_PRIORITY || 
			pEnum->pE->priority==pEnum->priority )
		{
			val = pEnum->pE->data ;
			break ;
		}
	}
	return pEnum->pE != NULL ;
}
//-------------------------------------------------------------------------//
//	Retrieves previous element in enumeration
TPRIORITYLIST_TEMPLATE
BOOLEAN TPRIORITYLIST::EnumPrev( HANDLE hEnum, ELEMENT_TYPE& val ) const
{
	ENUM *pEnum ;
	if( !((pEnum = (ENUM*)hEnum) && pEnum->pE) )
		return FALSE ;

	for( pEnum->pE = Prev( pEnum->pE ) ; 
		 pEnum->pE ; 
		 pEnum->pE = Prev( pEnum->pE ) )
	{
		if( pEnum->priority==DEFAULT_LIST_PRIORITY || 
			pEnum->pE->priority==pEnum->priority )
		{
			val = pEnum->pE->data ;
			break ;
		}
	}
	return pEnum->pE != NULL ;
}

//-------------------------------------------------------------------------//
//	Terminates an enumeration begun with EnumHead() or EnumTail().
TPRIORITYLIST_TEMPLATE
void TPRIORITYLIST::EndEnum( HANDLE hEnum ) const
{
	if( hEnum ) delete (ENUM*)hEnum ;
}

//-------------------------------------------------------------------------//
#ifdef _DEBUG
TPRIORITYLIST_TEMPLATE
void TPRIORITYLIST::Trace()
{
	ELEMENT* pE ;
	int		 i;
	for( i=0, pE = m_pHead; pE; pE = Next( pE ), i++ )
	{
		TRACE("Data[%d]: 0x%08lX, priority[%d]: %ld\n",
			  i, *(ULONG*)&pE->data, i, pE->priority ) ;
	}
}
#endif _DEBUG

//-------------------------------------------------------------------------//
#ifdef WIN32_USERMODE
#	undef WIN32_USERMODE
#endif
#ifdef NT_KERNELMODE
#	undef NT_KERNELMODE
#endif

#endif	__TPRIORITYLIST_H__