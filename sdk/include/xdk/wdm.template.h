/*
 * wdm.h
 *
 * Windows NT WDM Driver Developer Kit
 *
 * This file is part of the ReactOS DDK package.
 *
 * Contributors:
 *   Amine Khaldi (amine.khaldi@reactos.org)
 *   Timo Kreuzer (timo.kreuzer@reactos.org)
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
#pragma once

#ifndef _WDMDDK_
#define _WDMDDK_

#define WDM_MAJORVERSION        0x06
#define WDM_MINORVERSION        0x00

/* Included via ntddk.h? */
#ifndef _NTDDK_
#define _NTDDK_
#define _WDM_INCLUDED_
#define _DDK_DRIVER_
#define NO_INTERLOCKED_INTRINSICS
#endif /* _NTDDK_ */

/* Dependencies */
#define NT_INCLUDED
#include <excpt.h>
#include <ntdef.h>
#include <ntstatus.h>
#include <kernelspecs.h>
#include <ntiologc.h>
#include <suppress.h>

#ifndef GUID_DEFINED
#include <guiddef.h>
#endif

#ifdef _MAC
#ifndef _INC_STRING
#include <string.h>
#endif /* _INC_STRING */
#else
#include <string.h>
#endif /* _MAC */

#ifndef _KTMTYPES_
typedef GUID UOW, *PUOW;
#endif

typedef GUID *PGUID;

#if (NTDDI_VERSION >= NTDDI_WINXP)
#include <dpfilter.h>
#endif

#include "intrin.h"

__internal_kernel_driver
__drv_Mode_impl(WDM_INCLUDED)

#ifdef __cplusplus
extern "C" {
#endif

$define(UCHAR=UCHAR)
$define(ULONG=ULONG)
$define(USHORT=USHORT)

#if !defined(_NTHALDLL_) && !defined(_BLDR_)
#define NTHALAPI DECLSPEC_IMPORT
#else
#define NTHALAPI
#endif

/* For ReactOS */
#if !defined(_NTOSKRNL_) && !defined(_BLDR_) && !defined(_NTSYSTEM_)
#define NTKERNELAPI DECLSPEC_IMPORT
#else
#define NTKERNELAPI
#endif

/* For statically-linked ntoskrnl_vista library */
#if defined(NTKRNLVISTA)
#define NTKRNLVISTAAPI
#else
#define NTKRNLVISTAAPI NTKERNELAPI
#endif

#if defined(_X86_) && !defined(_NTHAL_)
#define _DECL_HAL_KE_IMPORT  DECLSPEC_IMPORT
#elif defined(_X86_)
#define _DECL_HAL_KE_IMPORT
#else
#define _DECL_HAL_KE_IMPORT NTKERNELAPI
#endif

#if defined(_WIN64)
#define POINTER_ALIGNMENT DECLSPEC_ALIGN(8)
#else
#define POINTER_ALIGNMENT
#endif

/* Helper macro to enable gcc's extension */
#ifndef __GNU_EXTENSION
#ifdef __GNUC__
#define __GNU_EXTENSION __extension__
#else
#define __GNU_EXTENSION
#endif
#endif

#if defined(_MSC_VER)

/* Disable some warnings */
#pragma warning(disable:4115) /* Named type definition in parentheses */
#pragma warning(disable:4201) /* Nameless unions and structs */
#pragma warning(disable:4214) /* Bit fields of other types than int */
#pragma warning(disable:4820) /* Padding added, due to alignment requirement */

/* Indicate if #pragma alloc_text() is supported */
#if defined(_M_IX86) || defined(_M_AMD64) || defined(_M_IA64)
#define ALLOC_PRAGMA 1
#endif

/* Indicate if #pragma data_seg() is supported */
#if defined(_M_IX86) || defined(_M_AMD64)
#define ALLOC_DATA_PRAGMA 1
#endif

#endif /* _MSC_VER */

/* These macros are used to create aliases for imported data. We need to do
   this to have declarations that are compatible with MS DDK */
#ifdef _M_IX86
#define __SYMBOL(_Name) "_"#_Name
#define __IMPORTSYMBOL(_Name) "__imp__"#_Name
#define __IMPORTNAME(_Name) __imp__##_Name
#else
#define __SYMBOL(_Name) #_Name
#define __IMPORTSYMBOL(_Name) "__imp_"#_Name
#define __IMPORTNAME(_Name) __imp_##_Name
#endif
#if defined(_MSC_VER) && !defined(__clang__)
#define __CREATE_NTOS_DATA_IMPORT_ALIAS(_Name) \
    __pragma(comment(linker, "/alternatename:"__SYMBOL(_Name) "=" __IMPORTSYMBOL(_Name)))
#else /* !_MSC_VER */
#ifndef __STRINGIFY
#define __STRINGIFY(_exp) #_exp
#endif
#define _Pragma_redefine_extname(_Name, _Target) _Pragma(__STRINGIFY(redefine_extname _Name _Target))
#define __CREATE_NTOS_DATA_IMPORT_ALIAS(_Name) \
    _Pragma_redefine_extname(_Name,__IMPORTNAME(_Name))
#endif

#if defined(_WIN64)
#if !defined(USE_DMA_MACROS) && !defined(_NTHAL_)
#define USE_DMA_MACROS
#endif
#if !defined(NO_LEGACY_DRIVERS) && !defined(__REACTOS__)
#define NO_LEGACY_DRIVERS
#endif
#endif /* defined(_WIN64) */

/* Forward declarations */
struct _IRP;
struct _MDL;
struct _KAPC;
struct _KDPC;
struct _FILE_OBJECT;
struct _DMA_ADAPTER;
struct _DEVICE_OBJECT;
struct _DRIVER_OBJECT;
struct _IO_STATUS_BLOCK;
struct _DEVICE_DESCRIPTION;
struct _SCATTER_GATHER_LIST;
struct _DRIVE_LAYOUT_INFORMATION;
struct _COMPRESSED_DATA_INFO;
struct _IO_RESOURCE_DESCRIPTOR;

/* Structures not exposed to drivers */
typedef struct _OBJECT_TYPE *POBJECT_TYPE;
typedef struct _CALLBACK_OBJECT *PCALLBACK_OBJECT;
typedef struct _EPROCESS *PEPROCESS;
typedef struct _ETHREAD *PETHREAD;
typedef struct _IO_TIMER *PIO_TIMER;
typedef struct _KINTERRUPT *PKINTERRUPT;
typedef struct _KPROCESS *PKPROCESS;
typedef struct _KTHREAD *PKTHREAD, *PRKTHREAD;
typedef struct _CONTEXT *PCONTEXT;

#if defined(USE_DMA_MACROS) && !defined(_NTHAL_)
typedef struct _DMA_ADAPTER *PADAPTER_OBJECT;
#elif defined(_WDM_INCLUDED_)
typedef struct _DMA_ADAPTER *PADAPTER_OBJECT;
#else
typedef struct _ADAPTER_OBJECT *PADAPTER_OBJECT;
#endif

#ifndef DEFINE_GUIDEX
#ifdef _MSC_VER
#define DEFINE_GUIDEX(name) EXTERN_C const CDECL GUID name
#else
#define DEFINE_GUIDEX(name) EXTERN_C const GUID name
#endif
#endif /* DEFINE_GUIDEX */

#ifndef STATICGUIDOF
#define STATICGUIDOF(guid) STATIC_##guid
#endif

/* GUID Comparison */
#ifndef __IID_ALIGNED__
#define __IID_ALIGNED__
#ifdef __cplusplus
inline int IsEqualGUIDAligned(REFGUID guid1, REFGUID guid2)
{
    return ( (*(PLONGLONG)(&guid1) == *(PLONGLONG)(&guid2)) &&
             (*((PLONGLONG)(&guid1) + 1) == *((PLONGLONG)(&guid2) + 1)) );
}
#else
#define IsEqualGUIDAligned(guid1, guid2) \
           ( (*(PLONGLONG)(guid1) == *(PLONGLONG)(guid2)) && \
             (*((PLONGLONG)(guid1) + 1) == *((PLONGLONG)(guid2) + 1)) )
#endif /* __cplusplus */
#endif /* !__IID_ALIGNED__ */


$define (_WDMDDK_)
$include (interlocked.h)
$include (rtltypes.h)
$include (ketypes.h)
$include (mmtypes.h)
$include (extypes.h)
$include (setypes.h)
$include (potypes.h)
$include (cmtypes.h)
$include (iotypes.h)
$include (obtypes.h)
$include (pstypes.h)
$include (wmitypes.h)

$include (kdfuncs.h)
$include (kefuncs.h)
$include (rtlfuncs.h)
$include (mmfuncs.h)
$include (sefuncs.h)
$include (cmfuncs.h)
$include (iofuncs.h)
$include (pofuncs.h)
$include (exfuncs.h)
$include (obfuncs.h)
$include (psfuncs.h)
$include (wmifuncs.h)
$include (halfuncs.h)
$include (nttmapi.h)
$include (zwfuncs.h)

#ifdef __cplusplus
}
#endif

#endif /* !_WDMDDK_ */
