
//
//  This is where the declarations for the memlist functions live.
//  Use this instead of embedded declarations and inclusion of linklist.h
//


#ifndef _MEMLIST_H_
#define _MEMLIST_H_

#include "linklist.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void FAR *PASCAL LDSFmalloc (size_t);
void FAR *PASCAL LDSFrealloc (void FAR * buffer, size_t size);
void FAR PASCAL LDSFfree (void FAR *);
void PASCAL LDSFinit ();

BOOL    PASCAL  BMInit ( VOID );
LPVOID  PASCAL  BMLock ( HMEM hv );
VOID    PASCAL  BMUnlock ( HMEM hv );
BOOL    PASCAL  BMIsLocked ( HMEM hv );
VOID    PASCAL  BMFree ( HMEM hv );
HMEM    PASCAL  BMFindAvail ( void );


#ifdef __cplusplus
};
#endif // __cplusplus

#endif // _MEMLIST_H_
