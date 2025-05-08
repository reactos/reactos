// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.



// include file added to each precompiled header
// please put stuff here, only when it is clear that almost every
// cpp file will need these definitions.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#if defined(_MANAGED)
  // The current version of PREFast doesn't understand override
  // keyword so when it is defined leave the define in place.
  // This version may need to go up - just using the current version we know.
  // Check %SDXROOT%\tools\analysis\*\OACR\prefast\bin\parser\cl.exe
  //  and  %SDXROOT%\tools\analysis\*\prefast\bin\parser\cl.exe
  #if !(defined(_PREFAST_) && ( _MSC_FULL_VER <= 13102337 ))
#undef override
  #endif
#endif

//
// Make HRESULT_FROM_WIN32 an inline method rather than a macro
//
#define INLINE_HRESULT_FROM_WIN32

// We need nt.h, but in devdiv it disables the loading of winnt.h so
// we'll copy over things from winnt.h as needed...
// <winnt copying>
#ifndef DECLSPEC_NOTHROW
#if (_MSC_VER >= 1200) && !defined(MIDL_PASS)
#define DECLSPEC_NOTHROW   __declspec(nothrow)
#else
#define DECLSPEC_NOTHROW
#endif
#endif

// "NTAPI" macro removed since it is ignored: "warning C4229: anachronism used : modifiers on data are ignored" 
typedef void (__stdcall *PFLS_CALLBACK_FUNCTION)(void *lpFlsData);

//
// For compilers that don't support nameless unions/structs
//
#ifndef DUMMYUNIONNAME
#if defined(NONAMELESSUNION) || !defined(_MSC_EXTENSIONS)
#define DUMMYUNIONNAME   u
#define DUMMYUNIONNAME2  u2
#define DUMMYUNIONNAME3  u3
#define DUMMYUNIONNAME4  u4
#define DUMMYUNIONNAME5  u5
#define DUMMYUNIONNAME6  u6
#define DUMMYUNIONNAME7  u7
#define DUMMYUNIONNAME8  u8
#define DUMMYUNIONNAME9  u9
#else
#define DUMMYUNIONNAME
#define DUMMYUNIONNAME2
#define DUMMYUNIONNAME3
#define DUMMYUNIONNAME4
#define DUMMYUNIONNAME5
#define DUMMYUNIONNAME6
#define DUMMYUNIONNAME7
#define DUMMYUNIONNAME8
#define DUMMYUNIONNAME9
#endif
#endif // DUMMYUNIONNAME

#ifndef DUMMYSTRUCTNAME
#if defined(NONAMELESSUNION) || !defined(_MSC_EXTENSIONS)
#define DUMMYSTRUCTNAME  s
#define DUMMYSTRUCTNAME2 s2
#define DUMMYSTRUCTNAME3 s3
#define DUMMYSTRUCTNAME4 s4
#define DUMMYSTRUCTNAME5 s5
#else
#define DUMMYSTRUCTNAME
#define DUMMYSTRUCTNAME2
#define DUMMYSTRUCTNAME3
#define DUMMYSTRUCTNAME4
#define DUMMYSTRUCTNAME5
#endif
#endif // DUMMYSTRUCTNAME

// </winnt copying>

//
// Get NT headers.
//

#include <windef.h>

#include <dwmapi.h>
#define _MilMatrix3x2D_DEFINED


#include <windows.h>
#include <wtypes.h>
#include <stddef.h>     // For offsetof
#include <tchar.h>


typedef struct _RTL_SPLAY_LINKS {
    struct _RTL_SPLAY_LINKS *Parent;
    struct _RTL_SPLAY_LINKS *LeftChild;
    struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS;
typedef RTL_SPLAY_LINKS *PRTL_SPLAY_LINKS;

typedef enum _RTL_GENERIC_COMPARE_RESULTS {
    GenericLessThan,
    GenericGreaterThan,
    GenericEqual
} RTL_GENERIC_COMPARE_RESULTS;

typedef
__drv_sameIRQL
__drv_functionClass(RTL_GENERIC_COMPARE_ROUTINE)
RTL_GENERIC_COMPARE_RESULTS
NTAPI
RTL_GENERIC_COMPARE_ROUTINE (
    __in struct _RTL_GENERIC_TABLE *Table,
    __in PVOID FirstStruct,
    __in PVOID SecondStruct
    );
typedef RTL_GENERIC_COMPARE_ROUTINE *PRTL_GENERIC_COMPARE_ROUTINE;

typedef ULONG CLONG;

typedef
__drv_sameIRQL
__drv_functionClass(RTL_GENERIC_ALLOCATE_ROUTINE)
__drv_allocatesMem(Mem)
PVOID
NTAPI
RTL_GENERIC_ALLOCATE_ROUTINE (
    __in struct _RTL_GENERIC_TABLE *Table,
    __in CLONG ByteSize
    );
typedef RTL_GENERIC_ALLOCATE_ROUTINE *PRTL_GENERIC_ALLOCATE_ROUTINE;

typedef
__drv_sameIRQL
__drv_functionClass(RTL_GENERIC_FREE_ROUTINE)
VOID
NTAPI
RTL_GENERIC_FREE_ROUTINE (
    __in struct _RTL_GENERIC_TABLE *Table,
    __in __drv_freesMem(Mem) __post_invalid PVOID Buffer
    );
typedef RTL_GENERIC_FREE_ROUTINE *PRTL_GENERIC_FREE_ROUTINE;

typedef struct _RTL_GENERIC_TABLE {
    PRTL_SPLAY_LINKS TableRoot;
    LIST_ENTRY InsertOrderList;
    PLIST_ENTRY OrderedPointer;
    ULONG WhichOrderedElement;
    ULONG NumberGenericTableElements;
    PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine;
    PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_GENERIC_FREE_ROUTINE FreeRoutine;
    PVOID TableContext;
} RTL_GENERIC_TABLE;
typedef RTL_GENERIC_TABLE *PRTL_GENERIC_TABLE;

#ifdef __cplusplus
extern "C" {
#endif

FORCEINLINE
VOID
InitializeListHead(
    __out PLIST_ENTRY ListHead
    )
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

FORCEINLINE
VOID
InsertHeadList(
    __inout PLIST_ENTRY ListHead,
    __inout __drv_aliasesMem PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Flink;

    Flink = ListHead->Flink;
    Entry->Flink = Flink;
    Entry->Blink = ListHead;
    Flink->Blink = Entry;
    ListHead->Flink = Entry;
}

FORCEINLINE
VOID
InsertTailList(
    __inout PLIST_ENTRY ListHead,
    __inout __drv_aliasesMem PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Blink;

    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
}

FORCEINLINE
VOID
AppendTailList(
    __inout PLIST_ENTRY ListHead,
    __inout PLIST_ENTRY ListToAppend
    )
{
    PLIST_ENTRY ListEnd = ListHead->Blink;

    ListHead->Blink->Flink = ListToAppend;
    ListHead->Blink = ListToAppend->Blink;
    ListToAppend->Blink->Flink = ListHead;
    ListToAppend->Blink = ListEnd;
}

PLIST_ENTRY
FORCEINLINE
RemoveHeadList(
    IN PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Flink;
    Flink = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink = ListHead;
    return Entry;
}

FORCEINLINE
PLIST_ENTRY
RemoveTailList(
    __inout PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Blink;
    Blink = Entry->Blink;
    ListHead->Blink = Blink;
    Blink->Flink = ListHead;
    return Entry;
}

FORCEINLINE
BOOLEAN
RemoveEntryList(
    __in PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Flink;

    Flink = Entry->Flink;
    Blink = Entry->Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;
    return (BOOLEAN)(Flink == Blink);
}

__checkReturn
BOOLEAN
FORCEINLINE
IsListEmpty(
    __in const LIST_ENTRY * ListHead
    )
{
    return (BOOLEAN)(ListHead->Flink == ListHead);
}

NTSYSAPI
VOID
NTAPI
RtlAssert(
    __in PVOID VoidFailedAssertion,
    __in PVOID VoidFileName,
    __in ULONG LineNumber,
    __in_opt PSTR MutableMessage
    );

#ifdef __cplusplus
}       // extern "C"
#endif

#if DBG
#define ASSERT( exp ) \
    ((!(exp)) ? \
        (RtlAssert( #exp, __FILE__, __LINE__, NULL ),FALSE) : \
        TRUE)
#else
#define ASSERT( exp )         ((void) 0)
#endif

//
// Include the avalon debug stuff. This is for Mt, TraceTag and meter heap.
//

#include "avalondebugp.h"


#ifdef _PREFIX_
    // __pfx_assume and __pfx_assert are not automatically declared
    #if __cplusplus
        extern "C" void __pfx_assert(bool, __in PCSTR);
        extern "C" void __pfx_assume(bool, __in PCSTR);
    #else
        void __pfx_assert(int, __in PCSTR);
        void __pfx_assume(int, __in PCSTR);
    #endif
#else
    #define __pfx_assert(Exp, Msg) do {} while ( UNCONDITIONAL_EXPR(false) )
    #define __pfx_assume(Exp, Msg) do {} while ( UNCONDITIONAL_EXPR(false) )
#endif


#include <basetsd.h>

#define COM_NO_WINDOWS_H
#include <objbase.h>

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <time.h>
#include <cmath>
#include <float.h>
#include <stdio.h>
#include <fcntl.h>

#pragma push_macro("UnsignedMultiply128")
#undef UnsignedMultiply128
#include <intsafe.h>
#pragma pop_macro("UnsignedMultiply128")

#pragma warning(push)
#pragma warning (disable:4005) // suppress macro redefinition warning
#include <D2D1.h>
#include <d3d9.h>
#include "dwrite.h"
#pragma warning(pop)

#ifdef __cplusplus
extern "C" {
#endif

#include <wincodec.h>

#ifdef __cplusplus
}
#endif

#include <Wincodec_private.h>
#include <wincodecsdk.h>
#include <wgx_core_types.h>
#include <wgx_render.h>

#include "avalonutilp.h"

//
// This header file must be included last because it changes the structure 
// packing to 1 before including other headers. That can lead to
// misalignment issues. For example, this caused UPDATELAYEREDWINDOWINFO to
// be too small on amd64 causing UpdateLayeredWindowIndirect to fail.
//

#include <dwmapi.h>

//
// C4201: nonstandard extension used : nameless struct/union
//
// The non-standard use of nameless struct/union makes our code less portable
// however it doesn't help catch any real bugs. We make use of this extension
// in a number of places, namely GpCC and some protocol struct definitions.
//

#pragma warning (disable : 4201)

//
// C4100: unreferenced formal parameter
//
// This warning seldom produces useful feedback, but it is very noisy. Turn
// it off because we're not getting any value from it.
//

#pragma warning (disable : 4100)

//
// Disable warnings for compiling HW and Geometry directories
//

//
// C4511: copy constructor could not be generated
//

#pragma warning (disable : 4511)

//
// C4512: assignment operator could not be generated
//

#pragma warning (disable : 4512)

//
// C4514: "unreferenced inline function has been removed"
//

#pragma warning (disable : 4514)

// Please only use CMILMatrix.
//
// The MIL has standardized on a single matrix 4x4 type, CMILMatrix.
// Except for D3DMATRIX definitions in windows/published and implementations of interfaces
// that use D3DMATRIX, these types have been permanently removed, and shouldn't be
// re-introduced.
#pragma deprecated(D3DMATRIX, D3DXMATRIX, MILMatrix, GpMatrix, MIL_MATRIXF)

