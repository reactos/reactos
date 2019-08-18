/*
 * wdfassert.h
 *
 * Windows Driver Framework - Prototypes for Runtime Asserts
 *
 * This file is part of the ReactOS WDF package.
 *
 * Contributors:
 *   Created by Benjamin Aerni <admin@bennottelling.com>
 *
 * Intended Usecase:
 *   Kernel mode drivers
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef _WDFASSERT_H_
#define _WDFASSERT_H_


#if (NTDDI_VERSION >= NTDDI_WIN2K)

/* RtlAssert isn't defined in XP and Win2k headers so we do so here */
#ifndef _RTLFUNCS_H
NTSYSAPI
VOID
RtlAssert(
    _In_ PVOID FailedAssertion,
    _In_ PVOID FileName,
    _In_ ULONG LineNumber,
    _In_opt_ PSTR Message
);
#endif

/* Only active if WDF verifier is turned on */
#define WDFVERIFY(exp){                                                      \
    if ((WdfDriverGlobals->DriverFlags & WdfVerifyOn) && !(exp)){            \
        RtlAssert(#exp, __FILE__, __LINE__, NULL);                           \
    }                                                                        \
}

#define VERIFY_IS_IRQL_PASSIVE_LEVEL() WDFVERIFY(KeGetCurrentIrql() == PASSIVE_LEVEL)

/* Macro is obsolete */
#define IS_AT_PASSIVE()                WDFVERIFY(KeGetCurrentIrql() == PASSIVE_LEVEL)

/* MSVC Error Supression */
#ifdef _MSC_VER

#define WDFCASSERT(c){                \
   _pragma(warning(supress: 6326))    \
   switch(0) case(c): case 0: ;       \
}
#endif /* _MSC_VER */

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#endif /* _WDFASSERT_H_ */
