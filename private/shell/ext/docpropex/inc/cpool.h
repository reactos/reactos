//-------------------------------------------------------------------------//
//
//	CPool.h
//
//	Copyright © 1997, Scott R. Hanggie
//	All rights under U.S. and international trademark and copyright 
//	law reserved.
//
//-------------------------------------------------------------------------//

#ifndef __CPOOL_H__
#define __CPOOL_H__

//-------------------------------------------------------------------------//
struct CPool     // warning variable length structure
//-------------------------------------------------------------------------//
{
#if defined(_NTDDK_) || defined(_NTIFS_)
	// Kernel-mode allocation
	static CPool* Create( POOL_TYPE type, CPool*& pHead, ULONG nMax, ULONG cbElement ) ;
#else 
	// User-mode allocation:
	static CPool* Create( CPool*& pHead, ULONG nMax, ULONG cbElement ) ;
#endif
			
	void* data() { return this+1 ; }
	void FreeChain()  ;  // free this block and all chained blocks.

	CPool* pNext ;				// next pool in list.
	ULONG dwReserved[1] ;		// align on 8 byte boundary
} ;


#endif __CPOOL_H__