/*** bm.h - Memory Manager routines
*
*   Copyright <C> 1990, Microsoft Corporation
*
* Purpose:  handle the near and far memory requests of dlls & linked list
*
*
*************************************************************************/

BOOL PASCAL  BMInit ( VOID );
HMEM PASCAL  BMAlloc ( UINT );
HMEM PASCAL  BMRealloc ( HMEM, UINT );
VOID PASCAL  BMFree ( HMEM );
LPV  PASCAL  BMLock ( HMEM );
VOID PASCAL  BMUnlock ( HMEM );
BOOL PASCAL  BMIsLocked ( HMEM );
