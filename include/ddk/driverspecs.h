/*
 * PROJECT:         ReactOS DDK
 * COPYRIGHT:       This file is in the Public Domain.
 * FILE:            driverspecs.h
 * ABSTRACT:        This header stubs out Driver Verifier annotations to
 *                  allow drivers using them to compile with our header set.
 */

//
// Stubs
//
#define __drv_dispatchType(x)
#define __drv_dispatchType_other

//
// FIXME: These annotations are not driver-only and does not belong here
//
#define __in
#define __in_bcount(Size)
#define __in_ecount(Size)

#define __out
#define __out_bcount(Size)
#define __out_bcount_part(Size, Length)
#define __out_ecount(Size)

#define __inout

#define __deref_out_ecount(Size)