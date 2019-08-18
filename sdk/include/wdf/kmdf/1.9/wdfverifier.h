/*
 * wdfverifier.h
 *
 * Windows Driver Framework - Driver Framework Verifier definitioons
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
#ifndef _WDFVERIFIER_H_
#define _WDFVERIFIER_H_

#if (NTDDI_VERSION >= NTDDI_WIN2K)

/* 
WDF Function: WdfVerifierDbgBreakPoint
*/
typedef
NTAPI
WDFAPI
VOID
(*PFN_WDFVERIFIERDBGBREAKPOINT)(
    _In_ PWDF_DRIVER_GLOBALS DriverGlobals
);

VOID
FORCEINLINE
WdfVerifierDbgBreakPoint(VOID)
{
    ((PFN_WDFVERIFIERDBGBREAKPOINT)WdfFunctions[WdfVerifierDbgBreakPointTableIndex])(WdfDriverGlobals);
}

/* 
WDF Function: WdfVerifierKeBugCheck
*/
typedef
NTAPI
WDFAPI
VOID
(*PFN_WDFVERIFIERKEBUGCHECK)(
    _In_ PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_ ULONG BugCheckCode,
    _In_ ULONG_PTR BugCheckParameter1,
    _In_ ULONG_PTR BugCheckParameter2,
    _In_ ULONG_PTR BugCheckParameter3,
    _In_ ULONG_PTR BugCheckParameter4
);

NTAPI
VOID
FORCEINLINE
WdfVerifierKeBugCheck(
    _In_ ULONG BugCheckCode,
    _In_ ULONG_PTR BugCheckParameter1,
    _In_ ULONG_PTR BugCheckParameter2,
    _In_ ULONG_PTR BugCheckParameter3,
    _In_ ULONG_PTR BugCheckParameter4
)
{
    ((PFN_WDFVERIFIERKEBUGCHECK)WdfFunctions[WdfVerifierKeBugCheckTableIndex])(WdfDriverGlobals, BugCheckCode, BugCheckParameter1, BugCheckParameter2, BugCheckParameter3, BugCheckParameter4);
}
#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#endif /* _WDFVERIFIER_H_ */
