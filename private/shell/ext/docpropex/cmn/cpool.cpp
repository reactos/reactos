//-------------------------------------------------------------------------//
//
//	CPool.cpp
//
//-------------------------------------------------------------------------//
#include "pch.h"
#include "CPool.h"

//-------------------------------------------------------------------------//
//	Allocates a pool.
#if defined(_NTDDK_) || defined(_NTIFS_)
//	kernel-mode version
CPool* CPool::Create( POOL_TYPE type, CPool*& pHead, ULONG nMax, ULONG cbElement )
#else
//	user-mode version
CPool* CPool::Create( CPool*& pHead, ULONG nMax, ULONG cbElement )
#endif
{
	ASSERT( nMax > 0 && cbElement > 0 ) ;

	CPool* pBlock ;

#if defined(_NTDDK_) || defined(_NTIFS_)
//	kernel-mode version
	if( (pBlock = (CPool*) new(type) UCHAR[sizeof(CPool) + nMax * cbElement])==NULL )
#else
//	user-mode version
	if( (pBlock = (CPool*) new UCHAR[sizeof(CPool) + nMax * cbElement])==NULL )
#endif
		return NULL ;

	RtlZeroMemory( pBlock, sizeof(CPool) + nMax * cbElement ) ;
	pBlock->pNext = pHead ;
	pHead = pBlock ;  // change head (adds in reverse order for simplicity)
	return pBlock ;
}

//-------------------------------------------------------------------------//
//	Frees this all linked pools
void CPool::FreeChain() 
{
	CPool* pBlock = this ;
	while( pBlock )
	{
		UCHAR* bytes = (UCHAR*) pBlock ;
		CPool* pNext = pBlock->pNext ;
		delete[] bytes ;
		pBlock = pNext ;
	}
}
