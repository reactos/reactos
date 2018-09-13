#include <ole2.h>

#include "valid.h"
#include "debug.h"

#if 1
// we cannot turn this off until we remove from the export list!
#undef IsValidPtrIn
#undef IsValidPtrOut

// BUGBUG: The following two functions are MACRO's in 2.01 code
// but we need them for now because we only run with a storage
// that uses ole232.dll. When we get rid of this these may die.

STDAPI_(BOOL) IsValidPtrIn( const void FAR* pv, UINT cb )
{                                                                                               //      NULL is acceptable
        if (pv && IsBadReadPtr(pv,cb))
        {
//              AssertSz(FALSE, "Invalid in pointer");
                return FALSE;
        }
        return TRUE;
}



STDAPI_(BOOL) IsValidPtrOut( void FAR* pv, UINT cb )
                                                                                //      NULL is not acceptable
{
        if (IsBadWritePtr(pv,cb))
        {
//              AssertSz(FALSE, "Invalid out pointer");
                return FALSE;
        }
        return TRUE;
}
#endif


STDAPI_(BOOL) IsValidInterface( void FAR* pv )
{
//
// There is nothing to do about it on UNIX.
//
#ifndef UNIX
        DWORD_PTR FAR*          pVtbl;
        BYTE FAR*               pFcn;
        volatile BYTE   bInstr;
        int                             i;

        __try {
                pVtbl = *(DWORD_PTR FAR* FAR*)pv;           // pVtbl now points to beginning of vtable

#if DBG==1
                for (i=0;i<3;++i)                                       // loop through qi,addref,rel
#else
                i=1;                                                            // in retail, just do AddRef
#endif
                {
                        pFcn = *(BYTE FAR* FAR*) &pVtbl[i];     // pFcn now points to beginning of QI,Addref, or Release
#if DBG==1
                        if (IsBadCodePtr((FARPROC FAR)pFcn)) {
                                return FALSE;
                        }
#endif
                        bInstr = *(BYTE FAR*) pFcn;             // get 1st byte of 1st instruction
                }

        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
                return FALSE;
        }
#endif /* !unix */

        return TRUE;
}


// #if DBG==1
// we cannot remove IsValidIID fcn until we remove from export list!

// This function is NOT called in retail builds.
// Its former implementation always returned TRUE thus doing NO validation.
// It now validates in debug build and is not called in retail build

#if DBG==0
#ifdef IsValidIid
#undef IsValidIid
STDAPI_(BOOL) IsValidIid( REFIID iid );
#endif
#endif

STDAPI_(BOOL) IsValidIid( REFIID iid )
{
#if DBG==1
        if (IsBadReadPtr((void*) &iid, 16)) {
                AssertSz(FALSE, "Invalid iid");
                return FALSE;
        }
#endif
        return TRUE;
}
// #endif // DBG==1

