/*
 * PROJECT:   .inf file parser
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Ge van Geldorp <gvg@reactos.org>
 */

#ifdef INFLIB_HOST

/* Definitions native to the host on which we're building */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define FALSE 0
#define TRUE  1

#define FREE(Area) free(Area)
#define MALLOC(Size) malloc(Size)
#define ZEROMEMORY(Area, Size) memset((Area), '\0', (Size))
#define MEMCPY(Dest, Src, Size) memcpy((Dest), (Src), (Size))

#define INF_STATUS_SUCCESS           0
#define INF_STATUS_NO_MEMORY         ENOMEM
#define INF_STATUS_INVALID_PARAMETER EINVAL
#define INF_STATUS_NOT_FOUND         ENOENT
#define INF_STATUS_BUFFER_OVERFLOW   E2BIG
#define INF_SUCCESS(x) (0 == (x))

typedef char CHAR, *PCHAR;
typedef unsigned char UCHAR, *PUCHAR;
typedef void VOID, *PVOID;
typedef UCHAR BOOLEAN, *PBOOLEAN;
#include <typedefs64.h>
typedef LONG *PLONG;

typedef char TCHAR, *PTCHAR, *PTSTR;
#define _T(x) x
#define _tcsicmp strcasecmp
#define _tcslen strlen
#define _tcscpy strcpy
#define _tcstoul strtoul
#define _tcstol strtol
#define STRFMT "%s"

#ifdef _MSC_VER
#define strcasecmp stricmp
#endif

extern ULONG DbgPrint(char *Fmt, ...);

#else /* ! defined(INFLIB_HOST) */

/* ReactOS definitions */

#define UNICODE
#define _UNICODE
#include <tchar.h>
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

extern PVOID InfpHeap;

#define FREE(Area) RtlFreeHeap(InfpHeap, 0, (Area))
#define MALLOC(Size) RtlAllocateHeap(InfpHeap, 0, (Size))
#define ZEROMEMORY(Area, Size) RtlZeroMemory((Area), (Size))
#define MEMCPY(Dest, Src, Size) RtlCopyMemory((Dest), (Src), (Size))

#define INF_STATUS_SUCCESS           STATUS_SUCCESS
#define INF_STATUS_NO_MEMORY         STATUS_NO_MEMORY
#define INF_STATUS_INVALID_PARAMETER STATUS_INVALID_PARAMETER
#define INF_STATUS_NOT_FOUND         STATUS_NOT_FOUND
#define INF_STATUS_BUFFER_OVERFLOW   STATUS_BUFFER_OVERFLOW
#define INF_SUCCESS(x) (0 <= (x))

#define STRFMT "%S"

#endif /* INFLIB_HOST */

typedef const TCHAR *PCTSTR;

/* EOF */
