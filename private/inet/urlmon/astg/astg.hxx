//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	astg.hxx
//
//  Contents:	Common header file for async docfiles
//
//  Classes:	
//
//  Functions:	
//
//  History:	19-Dec-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifndef __ASTG_HXX__
#define __ASTG_HXX__

#if DBG == 1
DECLARE_DEBUG(astg);
#endif

#if DBG == 1

#define astgDebugOut(x) astgInlineDebugOut x
#ifndef REF
#define astgAssert(e) Win4Assert(e)
#define astgVerify(e) Win4Assert(e)
#else
#include <assert.h>
#define astgAssert(e) assert(e)
#define astgVerify(e) assert(e)
#endif //!REF

#else

#define astgDebugOut(x)
#define astgAssert(e)
#define astgVerify(e) (e)

#endif

#define astgErr(l, e) ErrJmp(astg, l, e, sc)
#define astgChkTo(l, e) if (FAILED(sc = (e))) astgErr(l, sc) else 1
#define astgHChkTo(l, e) if (FAILED(sc = GetScode(e))) astgErr(l, sc) else 1
#define astgChk(e) astgChkTo(Err, e)
#define astgHChk(e) astgHChkTo(Err, e)
#define astgMemTo(l, e) if ((e) == NULL) astgErr(l, STG_E_INSUFFICIENTMEMORY) else 1
#define astgMem(e) astgMemTo(Err, e)


#define IsValidHugePtrIn(pv, n)  (((pv) == NULL) || !IsBadHugeReadPtr(pv, n))
#define IsValidHugePtrOut(pv, n) (!IsBadHugeWritePtr(pv, n))

#define ValidateBuffer(pv, n) \
    (((pv) == NULL || !IsValidPtrIn(pv, n)) ? STG_E_INVALIDPOINTER : S_OK)

#define ValidatePtrBuffer(pv) \
    ValidateBuffer(pv, sizeof(void *))

#define ValidateHugeBuffer(pv, n) \
    (((pv) == NULL || !IsValidHugePtrIn(pv, n)) ? STG_E_INVALIDPOINTER : S_OK)

#define ValidateOutBuffer(pv, n) \
    (!IsValidPtrOut(pv, n) ? STG_E_INVALIDPOINTER : S_OK)

#define ValidateOutPtrBuffer(pv) \
    ValidateOutBuffer(pv, sizeof(void *))

#define ValidateHugeOutBuffer(pv, n) \
    (!IsValidHugePtrOut(pv, n) ? STG_E_INVALIDPOINTER : S_OK)

#endif // #ifndef __ASTG_HXX__
