/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/types.h
 * PURPOSE:      Types used by all the parts of the system
 * PROGRAMMER:   David Welch <welch@cwcom.net>
 * DEFINES:      _WIN64: 64-bit architecture
 *               _WIN32: 32-bit architecture (default)
 * UPDATE HISTORY:
 *               27/06/00: Created
 *               01/05/01: Portabillity changes
 */

#ifndef __INCLUDE_TYPES_H
#define __INCLUDE_TYPES_H

#define EXPORTED __declspec(dllexport)
#define IMPORTED __declspec(dllimport)

#include <basetsd.h>

#ifdef __GNUC__
#define STDCALL_FUNC STDCALL
#else
#define STDCALL_FUNC(a) (__stdcall a )
#endif /*__GNUC__*/

/* Fixed precision types */
typedef signed char         INT8, *PINT8;
typedef signed short        INT16, *PINT16;

typedef unsigned char       UINT8, *PUINT8;
typedef unsigned short      UINT16, *PUINT16;


/* Check VOID before defining CHAR, SHORT */
#ifndef VOID
#define VOID void
typedef char CHAR;
typedef short SHORT;
#endif


#ifndef __USE_W32API

#ifdef i386
#define STDCALL     __attribute__ ((stdcall))
#define CDECL       __attribute__ ((cdecl))
#define CALLBACK    WINAPI
#define PASCAL      WINAPI
#else

#ifdef __GNUC__
#define STDCALL
#define CDECL
#define CALLBACK
#define PASCAL
#else
#define STDCALL __stdcall
#define CDECL __cdecl
#define CALLBACK
#define PASCAL
#endif /*__GNUC__*/

#endif /*i386*/

typedef INT32 INT, *PINT;
typedef LONG32 LONG, *PLONG;
typedef ULONG32 DWORD, *PDWORD;
typedef UINT32 UINT, *PUINT;
typedef ULONG32 ULONG, *PULONG;

#ifndef _WCHAR_T_
#define _WCHAR_T_
#define _WCHAR_T
#define _WCHAR_T_DEFINED
#ifndef __WCHAR_TYPE__
#define __WCHAR_TYPE__      short unsigned int
#endif

#ifndef __cplusplus
typedef __WCHAR_TYPE__      wchar_t;
#endif  /* C++ */

#endif  /* wchar_t not already defined */


//#ifndef __cplusplus
//#ifndef _WCHAR_T_DEFINED
//#define _WCHAR_T_DEFINED
//typedef unsigned short wchar_t;
//#endif
//#endif

typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef wchar_t WCHAR;
typedef unsigned short WORD;
typedef int BOOL;
typedef unsigned char BOOLEAN;
typedef BOOLEAN* PBOOLEAN;
typedef wchar_t *LPWSTR;
typedef wchar_t *PWSTR;
typedef unsigned char *PUCHAR;
typedef unsigned short *PUSHORT;
typedef void *PVOID;
typedef unsigned char BYTE;
typedef void *LPVOID;
typedef float *PFLOAT;
typedef wchar_t *PWCH;
typedef unsigned short *PWORD;

#include <msvcrt/crttypes.h> // for definition of LONGLONG, PLONGLONG etc

typedef const void *LPCVOID;
typedef BYTE *LPBYTE, *PBYTE;
typedef BOOL *PBOOL;
typedef DWORD LCID;
typedef DWORD *PLCID;
typedef const char *LPCSTR;
typedef char *LPSTR;
typedef const wchar_t *LPCWSTR;
typedef CHAR *PCHAR;
typedef CHAR *PCH;
typedef void *HANDLE;
typedef HANDLE *PHANDLE;
typedef char CCHAR;
typedef CCHAR *PCCHAR;
typedef wchar_t *PWCHAR;
typedef ULONG WAIT_TYPE;
typedef USHORT CSHORT;
typedef const wchar_t *PCWSTR;
typedef char* PCSZ;

#ifdef __GNUC__
typedef DWORD STDCALL (*PTHREAD_START_ROUTINE) (LPVOID);
#else
typedef DWORD (STDCALL *PTHREAD_START_ROUTINE) (LPVOID);
#endif /*__GNUC__*/

typedef union _LARGE_INTEGER
{
  struct
  {
    DWORD LowPart;
    LONG  HighPart;
  } u;
#ifdef ANONYMOUSUNIONS
  struct
  {
    DWORD LowPart;
    LONG  HighPart;
  };
#endif /* ANONYMOUSUNIONS */
  LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef union _ULARGE_INTEGER
{
  struct
  {
    DWORD LowPart;
    DWORD HighPart;
  } u;
#ifdef ANONYMOUSUNIONS
  struct
  {
    DWORD LowPart;
    DWORD HighPart;
  };
#endif /* ANONYMOUSUNIONS */
  ULONGLONG QuadPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;

typedef struct _FILETIME
{
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} FILETIME, *LPFILETIME, *PFILETIME;

typedef struct _LIST_ENTRY
{
  struct _LIST_ENTRY *Flink;
  struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY;

typedef struct _SINGLE_LIST_ENTRY
{
  struct _SINGLE_LIST_ENTRY *Next;
} SINGLE_LIST_ENTRY, *PSINGLE_LIST_ENTRY;

#define SLIST_ENTRY SINGLE_LIST_ENTRY
#define PSLIST_ENTRY PSINGLE_LIST_ENTRY

typedef struct _UNICODE_STRING
{
  USHORT Length;
  USHORT MaximumLength;
  PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef const UNICODE_STRING* PCUNICODE_STRING;

typedef struct _FLOATING_SAVE_AREA
{
  DWORD   ControlWord;
  DWORD   StatusWord;
  DWORD   TagWord;
  DWORD   ErrorOffset;
  DWORD   ErrorSelector;
  DWORD   DataOffset;
  DWORD   DataSelector;
  BYTE    RegisterArea[80];
  DWORD   Cr0NpxState;
} FLOATING_SAVE_AREA;

typedef unsigned short RTL_ATOM;
typedef unsigned short *PRTL_ATOM;

#else /* __USE_W32API */

#include <windows.h>

#endif /* __USE_W32API */

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif /* __cplusplus */
#endif /* NULL */

#define CONST const

#ifdef __PPC__
#define CONTEXT_CONTROL         1L
#define CONTEXT_FLOATING_POINT  2L
#define CONTEXT_INTEGER         4L
#define CONTEXT_DEBUG_REGISTERS	8L

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_FLOATING_POINT | CONTEXT_INTEGER)
#define CONTEXT_DEBUGGER (CONTEXT_FULL)

#else /* x86 */

#define SIZE_OF_80387_REGISTERS      80

/* Values for contextflags */
#define CONTEXT_i386    0x10000

#ifndef __USE_W32API

#define CONTEXT_CONTROL         (CONTEXT_i386 | 1)
#define CONTEXT_INTEGER         (CONTEXT_i386 | 2)
#define CONTEXT_SEGMENTS        (CONTEXT_i386 | 4)
#define CONTEXT_FLOATING_POINT  (CONTEXT_i386 | 8)
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_i386 | 0x10)
#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS)

#endif /* !__USE_W32API */

/* our own invention */
#define FLAG_TRACE_BIT 0x100
#define CONTEXT_DEBUGGER (CONTEXT_FULL | CONTEXT_FLOATING_POINT)

#endif

typedef struct _CONTEXT_X86
{
  DWORD ContextFlags;

  DWORD   Dr0;
  DWORD   Dr1;
  DWORD   Dr2;
  DWORD   Dr3;
  DWORD   Dr6;
  DWORD   Dr7;

  FLOATING_SAVE_AREA FloatSave;

  DWORD   SegGs;
  DWORD   SegFs;
  DWORD   SegEs;
  DWORD   SegDs;

  DWORD   Edi;
  DWORD   Esi;
  DWORD   Ebx;
  DWORD   Edx;
  DWORD   Ecx;
  DWORD   Eax;

  DWORD   Ebp;
  DWORD   Eip;
  DWORD   SegCs;
  DWORD   EFlags;
  DWORD   Esp;
  DWORD   SegSs;
} CONTEXT_X86, *PCONTEXT_X86, *LPCONTEXT_X86;

typedef struct _CONTEXT_PPC
{
  /* Floating point registers returned when CONTEXT_FLOATING_POINT is set */
  double Fpr0;
  double Fpr1;
  double Fpr2;
  double Fpr3;
  double Fpr4;
  double Fpr5;
  double Fpr6;
  double Fpr7;
  double Fpr8;
  double Fpr9;
  double Fpr10;
  double Fpr11;
  double Fpr12;
  double Fpr13;
  double Fpr14;
  double Fpr15;
  double Fpr16;
  double Fpr17;
  double Fpr18;
  double Fpr19;
  double Fpr20;
  double Fpr21;
  double Fpr22;
  double Fpr23;
  double Fpr24;
  double Fpr25;
  double Fpr26;
  double Fpr27;
  double Fpr28;
  double Fpr29;
  double Fpr30;
  double Fpr31;
  double Fpscr;

  /* Integer registers returned when CONTEXT_INTEGER is set. */
  DWORD Gpr0;
  DWORD Gpr1;
  DWORD Gpr2;
  DWORD Gpr3;
  DWORD Gpr4;
  DWORD Gpr5;
  DWORD Gpr6;
  DWORD Gpr7;
  DWORD Gpr8;
  DWORD Gpr9;
  DWORD Gpr10;
  DWORD Gpr11;
  DWORD Gpr12;
  DWORD Gpr13;
  DWORD Gpr14;
  DWORD Gpr15;
  DWORD Gpr16;
  DWORD Gpr17;
  DWORD Gpr18;
  DWORD Gpr19;
  DWORD Gpr20;
  DWORD Gpr21;
  DWORD Gpr22;
  DWORD Gpr23;
  DWORD Gpr24;
  DWORD Gpr25;
  DWORD Gpr26;
  DWORD Gpr27;
  DWORD Gpr28;
  DWORD Gpr29;
  DWORD Gpr30;
  DWORD Gpr31;

  DWORD Cr;	/* Condition register */
  DWORD Xer;	/* Fixed point exception register */

  /* The following are set when CONTEXT_CONTROL is set. */
  DWORD Msr;	/* Machine status register */
  DWORD Iar;	/* Instruction address register */
  DWORD Lr;	/* Link register */
  DWORD Ctr;	/* Control register */

  /* Control which context values are returned */
  DWORD ContextFlags;
  DWORD Fill[3];

  /* Registers returned if CONTEXT_DEBUG_REGISTERS is set. */
  DWORD Dr0;	/* Breakpoint Register 1 */
  DWORD Dr1;	/* Breakpoint Register 2 */
  DWORD Dr2;	/* Breakpoint Register 3 */
  DWORD Dr3;	/* Breakpoint Register 4 */
  DWORD Dr4;	/* Breakpoint Register 5 */
  DWORD Dr5;	/* Breakpoint Register 6 */
  DWORD Dr6;	/* Debug Status Register */
  DWORD Dr7;	/* Debug Control Register */
} CONTEXT_PPC, *PCONTEXT_PPC, *LPCONTEXT_PPC;

typedef struct value_ent
{
  LPWSTR ve_valuename;
  DWORD  ve_valuelen;
  DWORD  ve_valueptr;
  DWORD  ve_type;
} WVALENT, *PWVALENT;

//#include "except.h"

#ifndef __USE_W32API

typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;

#ifdef __i386__

typedef CONTEXT_X86 CONTEXT;
typedef PCONTEXT_X86 PCONTEXT;
typedef LPCONTEXT_X86 LPCONTEXT;

#else /* __ppc__ */

typedef CONTEXT_PPC CONTEXT;
typedef PCONTEXT_PPC PCONTEXT;
typedef LPCONTEXT_PPC LPCONTEXT;

#endif

typedef WORD ATOM;

typedef struct _COORD
{
  SHORT X;
  SHORT Y;
} COORD;

typedef struct _SMALL_RECT
{
  SHORT Left;
  SHORT Top;
  SHORT Right;
  SHORT Bottom;
} SMALL_RECT, *PSMALL_RECT;


#ifdef __GNUC__
typedef VOID STDCALL
(*PTIMERAPCROUTINE)(
	LPVOID lpArgToCompletionRoutine,
	DWORD dwTimerLowValue,
	DWORD dwTimerHighValue
	);
#else
typedef VOID
(STDCALL *PTIMERAPCROUTINE)(
	LPVOID lpArgToCompletionRoutine,
	DWORD dwTimerLowValue,
	DWORD dwTimerHighValue
	);
#endif /*__GNUC__*/

#include "except.h"

#else /* __USE_W32API */

typedef LPTHREAD_START_ROUTINE PTHREAD_START_ROUTINE;

#include <ddk/ntapi.h>

#endif /* __USE_W32API */

typedef struct _ADDRESS_RANGE
{
   ULONG BaseAddrLow;
   ULONG BaseAddrHigh;
   ULONG LengthLow;
   ULONG LengthHigh;
   ULONG Type;
} ADDRESS_RANGE, *PADDRESS_RANGE;

#define MB_FLAGS_MEM_INFO         (0x1)
#define MB_FLAGS_BOOT_DEVICE      (0x2)
#define MB_FLAGS_COMMAND_LINE     (0x4)
#define MB_FLAGS_MODULE_INFO      (0x8)
#define MB_FLAGS_AOUT_SYMS        (0x10)
#define MB_FLAGS_ELF_SYMS         (0x20)
#define MB_FLAGS_MMAP_INFO        (0x40)
#define MB_FLAGS_DRIVES_INFO      (0x80)
#define MB_FLAGS_CONFIG_TABLE     (0x100)
#define MB_FLAGS_BOOT_LOADER_NAME (0x200)
#define MB_FLAGS_APM_TABLE        (0x400)
#define MB_FLAGS_GRAPHICS_TABLE   (0x800)

typedef struct _LOADER_MODULE
{
   ULONG ModStart;
   ULONG ModEnd;
   ULONG String;
   ULONG Reserved;
} LOADER_MODULE, *PLOADER_MODULE;

typedef struct _LOADER_PARAMETER_BLOCK
{
   ULONG Flags;
   ULONG MemLower;
   ULONG MemHigher;
   ULONG BootDevice;
   ULONG CommandLine;
   ULONG ModsCount;
   ULONG ModsAddr;
   UCHAR Syms[12];
   ULONG MmapLength;
   ULONG MmapAddr;
   ULONG DrivesCount;
   ULONG DrivesAddr;
   ULONG ConfigTable;
   ULONG BootLoaderName;
} LOADER_PARAMETER_BLOCK, *PLOADER_PARAMETER_BLOCK;

typedef enum _KAPC_ENVIRONMENT
{
  OriginalApcEnvironment,
  AttachedApcEnvironment,
  CurrentApcEnvironment
} KAPC_ENVIRONMENT;

/* FIXME: Are these official values ?? */
#define STATUS_FS_QUERY_REQUIRED                    ((NTSTATUS)0xC1000001)
#define STATUS_HANDLE_NOT_WAITABLE                  ((NTSTATUS)0xC1000002)
#define STATUS_OBJECT_FILE_MISMATCH                 ((NTSTATUS)0xC1000003)
#define STATUS_INVALID_PARAMETER_MAX                ((NTSTATUS)0xC1000004)
#define STATUS_CONFLICTING_ADDRESS                  ((NTSTATUS)0xC1000005)
#define STATUS_NO_MEDIA_IN_DRIVE                    ((NTSTATUS)0xC1000006)

#define NTSTAT_SEVERITY_SHIFT 30
#define NTSTAT_SEVERITY_MASK  0x00000003
#define NTSTAT_FACILITY_SHIFT 16
#define NTSTAT_FACILITY_MASK  0x00000FFF
#define NTSTAT_CUSTOMER_MASK  0x20000000

#define NT_SEVERITY(StatCode) (((StatCode) >> NTSTAT_SEVERITY_SHIFT) & NTSTAT_SEVERITY_MASK)
#define NT_FACILITY(StatCode) (((StatCode) >> NTSTAT_FACILITY_SHIFT) & NTSTAT_FACILITY_MASK)
#define NT_CUSTOMER(StatCode) ((StatCode) & NTSTAT_CUSTOMER_MASK)

#endif /* __INCLUDE_TYPES_H */
