/*++

Copyright (c) 1988-1992  Microsoft Corporation

Module Name:

    Align.h

Abstract:

    BUGBUG

Author:

    John Rogers (JohnRo) 15-May-1991

Environment:

    This code assumes that sizeof(DWORD) >= sizeof(LPVOID).

Revision History:

    15-May-1991 JohnRo
        Created align.h for NT/LAN from OS/2 1.2 HPFS pbmacros.h.
    19-Jun-1991 JohnRo
        Make sure pointer-to-wider-then-byte doesn't get messed up.
    10-Jul-1991 JohnRo
        Added ALIGN_BYTE and ALIGN_CHAR for completeness.
    21-Aug-1991 CliffV
        Fix ROUND_DOWN_* to include ~
    03-Dec-1991 JohnRo
        Worst-case on MIPS is 8-byte alignment.
        Added COUNT_IS_ALIGNED() and POINTER_IS_ALIGNED() macros.
    26-Jun-1992 JohnRo
        RAID 9933: ALIGN_WORST should be 8 for x86 builds.

--*/

#ifndef _ALIGN_
#define _ALIGN_


// BOOL
// COUNT_IS_ALIGNED(
//     IN DWORD Count,
//     IN DWORD Pow2      // undefined if this isn't a power of 2.
//     );
//
#define COUNT_IS_ALIGNED(Count,Pow2) \
        ( ( ( (Count) & (((Pow2)-1)) ) == 0) ? TRUE : FALSE )

// BOOL
// POINTER_IS_ALIGNED(
//     IN LPVOID Ptr,
//     IN DWORD Pow2      // undefined if this isn't a power of 2.
//     );
//
#define POINTER_IS_ALIGNED(Ptr,Pow2) \
        ( ( ( ((ULONG_PTR)(Ptr)) & (((Pow2)-1)) ) == 0) ? TRUE : FALSE )


#define ROUND_DOWN_COUNT(Count,Pow2) \
        ( (Count) & (~(((LONG)(Pow2))-1)) )

#define ROUND_DOWN_POINTER(Ptr,Pow2) \
        ( (LPVOID) ROUND_DOWN_COUNT( ((ULONG_PTR)(Ptr)), (Pow2) ) )


// If Count is not already aligned, then
// round Count up to an even multiple of "Pow2".  "Pow2" must be a power of 2.
//
// DWORD
// ROUND_UP_COUNT(
//     IN DWORD Count,
//     IN DWORD Pow2
//     );
#define ROUND_UP_COUNT(Count,Pow2) \
        ( ((Count)+(Pow2)-1) & (~(((LONG)(Pow2))-1)) )

// LPVOID
// ROUND_UP_POINTER(
//     IN LPVOID Ptr,
//     IN DWORD Pow2
//     );

// If Ptr is not already aligned, then round it up until it is.
#define ROUND_UP_POINTER(Ptr,Pow2) \
        ( (LPVOID) ( (((ULONG_PTR)(Ptr))+(Pow2)-1) & (~(((LONG)(Pow2))-1)) ) )


// Usage: myPtr = ROUND_UP_POINTER( unalignedPtr, ALIGN_LPVOID )

#define ALIGN_BYTE              sizeof(UCHAR)
#define ALIGN_CHAR              sizeof(CHAR)
#define ALIGN_DESC_CHAR         sizeof(DESC_CHAR)
#define ALIGN_DWORD             sizeof(DWORD)
#define ALIGN_LONG              sizeof(LONG)
#define ALIGN_LPBYTE            sizeof(LPBYTE)
#define ALIGN_LPDWORD           sizeof(LPDWORD)
#define ALIGN_LPSTR             sizeof(LPSTR)
#define ALIGN_LPTSTR            sizeof(LPTSTR)
#define ALIGN_LPVOID            sizeof(LPVOID)
#define ALIGN_LPWORD            sizeof(LPWORD)
#define ALIGN_TCHAR             sizeof(TCHAR)
#define ALIGN_WCHAR             sizeof(WCHAR)
#define ALIGN_WORD              sizeof(WORD)

//
// For now, use a hardcoded constant. however, this should be visited again
// and maybe changed to sizeof(QUAD).
//

#define ALIGN_QUAD              8

#if defined(_X86_)

#define ALIGN_WORST             8

#elif defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

//
// Worst-case alignment on MIPS is 8 bytes (for double).  Specify this here,
// in case our allocator is used for structures containing this.  (That is,
// even though NT/LAN doesn't need this for our data structures, let's be
// permissive.) The alignment requirements apply to Alpha.
//

#define ALIGN_WORST             8

#elif defined(_IA64_)

//
// IA64 note for QUAD: The NT QUAD type definition is NOT the EM 16byte quad type.
//                     Because of some NT constraints, QUAD type size cannot be changed.
//

#define ALIGN_WORST             16

#else  // none of the above

#error "Unknown alignment requirements for align.h"

#endif  // none of the above

#endif  // _ALIGN_
