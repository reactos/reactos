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

/* Fixed precision types */
typedef signed char         INT8, *PINT8;
typedef signed short        INT16, *PINT16;
typedef signed int          INT32, *PINT32;
typedef signed long long    INT64, *PINT64;
typedef unsigned char       UINT8, *PUINT8;
typedef unsigned short      UINT16, *PUINT16;
typedef unsigned int        UINT32, *PUINT32;
typedef unsigned long long  UINT64, *PUINT64;

typedef signed long int     LONG32, *PLONG32;
typedef unsigned long int   ULONG32, *PULONG32;
typedef unsigned long int   DWORD32, *PDWORD32;


#ifdef _WIN64

/* 64-bit architecture */

typedef INT64 INT, *PINT;
typedef LONG64 LONG, *PLONG;
typedef DWORD64 DWORD, *PDWORD;
typedef UINT64 UINT, *PUINT;
typedef ULONG64 ULONG, *PULONG;

/* Pointer precision types */
typedef long long           INT_PTR, *PINT_PTR;
typedef unsigned long long  UINT_PTR, *PUINT_PTR;
typedef long long           LONG_PTR, *PLONG_PTR;
typedef unsigned long long  ULONG_PTR, *PULONG_PTR;
typedef unsigned long long  HANDLE_PTR;
typedef unsigned int        UHALF_PTR, *PUHALF_PTR;
typedef int                 HALF_PTR, *PHALF_PTR;

#else /* _WIN64 */

/* 32-bit architecture */

typedef INT32 INT, *PINT;
typedef LONG32 LONG, *PLONG;
typedef DWORD32 DWORD, *PDWORD;
typedef UINT32 UINT, *PUINT;
typedef ULONG32 ULONG, *PULONG;


/* Pointer precision types */
typedef int               INT_PTR, *PINT_PTR;
typedef unsigned int      UINT_PTR, *PUINT_PTR;
typedef long              LONG_PTR, *PLONG_PTR;
typedef unsigned long     ULONG_PTR, *PULONG_PTR;
typedef unsigned short    UHALF_PTR, *PUHALF_PTR;
typedef short             HALF_PTR, *PHALF_PTR;
typedef unsigned long     HANDLE_PTR;

#endif /* _WIN64 */

typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

typedef long long LONG64, *PLONG64;

typedef unsigned long long ULONG64, *PULONG64;
typedef unsigned long long DWORD64, *PDWORD64;


typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned short WCHAR;
typedef unsigned short WORD;
typedef int BOOL;
typedef unsigned char BOOLEAN;
typedef BOOLEAN* PBOOLEAN;
typedef unsigned short *LPWSTR;
typedef unsigned short *PWSTR;
typedef unsigned char *PUCHAR;
typedef unsigned short *PUSHORT;
typedef void *PVOID;
typedef unsigned char BYTE;
typedef void *LPVOID;
typedef float *PFLOAT;
typedef unsigned short *PWCH;
typedef unsigned short *PWORD;

typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
typedef long long *PLONGLONG;
typedef unsigned long long *PULONGLONG;

/* Check VOID before defining CHAR, SHORT */
#ifndef VOID
#define VOID void
typedef char CHAR;
typedef short SHORT;
#endif

typedef CHAR *PCHAR;
typedef CHAR *PCH;
typedef void *HANDLE;
typedef char CCHAR;
typedef CCHAR *PCCHAR;


#define FALSE 0
#define TRUE 1

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif /* __cplusplus */
#endif /* NULL */

typedef const unsigned short *PCWSTR;

typedef char* PCSZ;

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

#define CONST const

#ifdef i386
#define STDCALL     __attribute__ ((stdcall))
#define CDECL       __attribute((cdecl))
#define CALLBACK    WINAPI
#define PASCAL      WINAPI
#else
#define STDCALL
#define CDECL
#define CALLBACK
#define PASCAL
#endif

typedef struct _LIST_ENTRY {
  struct _LIST_ENTRY *Flink;
  struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY;

typedef struct _SINGLE_LIST_ENTRY {
  struct _SINGLE_LIST_ENTRY *Next;
} SINGLE_LIST_ENTRY, *PSINGLE_LIST_ENTRY;

typedef DWORD STDCALL (*PTHREAD_START_ROUTINE) (LPVOID);
typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;

typedef unsigned short *PWCHAR;

#ifdef __PPC__
#define CONTEXT_CONTROL         1L
#define CONTEXT_FLOATING_POINT  2L
#define CONTEXT_INTEGER         4L
#define CONTEXT_DEBUG_REGISTERS	8L

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_FLOATING_POINT | CONTEXT_INTEGER)
#define CONTEXT_DEBUGGER (CONTEXT_FULL)

#else /* x86 */
/* The doc refered me to winnt.h, so I had to look... */
#define SIZE_OF_80387_REGISTERS      80

/* Values for contextflags */
#define CONTEXT_i386    0x10000
#define CONTEXT_CONTROL         (CONTEXT_i386 | 1)	
#define CONTEXT_INTEGER         (CONTEXT_i386 | 2)	
#define CONTEXT_SEGMENTS        (CONTEXT_i386 | 4)	
#define CONTEXT_FLOATING_POINT  (CONTEXT_i386 | 8)	
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_i386 | 0x10)
#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS)

/* our own invention */
#define FLAG_TRACE_BIT 0x100
#define CONTEXT_DEBUGGER (CONTEXT_FULL | CONTEXT_FLOATING_POINT)

#endif

#ifdef __i386__

typedef struct _FLOATING_SAVE_AREA {
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

typedef struct _CONTEXT {
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
} CONTEXT, *PCONTEXT, *LPCONTEXT;

#else /* __ppc__ */

typedef struct
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

    /* Integer registers returned when CONTEXT_INTEGER is set.  */
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

    DWORD Cr;			/* Condition register */
    DWORD Xer;			/* Fixed point exception register */

    /* The following are set when CONTEXT_CONTROL is set.  */
    DWORD Msr;			/* Machine status register */
    DWORD Iar;			/* Instruction address register */
    DWORD Lr;			/* Link register */
    DWORD Ctr;			/* Control register */

    /* Control which context values are returned */
    DWORD ContextFlags;
    DWORD Fill[3];

    /* Registers returned if CONTEXT_DEBUG_REGISTERS is set.  */
    DWORD Dr0;                          /* Breakpoint Register 1 */
    DWORD Dr1;                          /* Breakpoint Register 2 */
    DWORD Dr2;                          /* Breakpoint Register 3 */
    DWORD Dr3;                          /* Breakpoint Register 4 */
    DWORD Dr4;                          /* Breakpoint Register 5 */
    DWORD Dr5;                          /* Breakpoint Register 6 */
    DWORD Dr6;                          /* Debug Status Register */
    DWORD Dr7;                          /* Debug Control Register */
} CONTEXT, *PCONTEXT, *LPCONTEXT;
#endif

typedef HANDLE *PHANDLE;

typedef struct value_ent {
    LPWSTR   ve_valuename;
    DWORD ve_valuelen;
    DWORD ve_valueptr;
    DWORD ve_type;
} WVALENT, *PWVALENT;

#define EXCEPTION_MAXIMUM_PARAMETERS	(15)

typedef struct _EXCEPTION_RECORD {
  DWORD ExceptionCode;
  DWORD ExceptionFlags;
  struct _EXCEPTION_RECORD *ExceptionRecord;
  PVOID ExceptionAddress;
  DWORD NumberParameters;
  DWORD ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD, *PEXCEPTION_RECORD, *LPEXCEPTION_RECORD;

typedef const void *LPCVOID;
typedef BYTE *LPBYTE, *PBYTE;

typedef BOOL *PBOOL;

typedef DWORD LCID;
typedef DWORD *PLCID;

typedef const char *LPCSTR;

typedef char *LPSTR;

typedef const unsigned short *LPCWSTR;

typedef unsigned short RTL_ATOM;
typedef unsigned short *PRTL_ATOM;
typedef WORD ATOM;

typedef struct _COORD {
  SHORT X;
  SHORT Y;
} COORD;

typedef struct _SMALL_RECT {
  SHORT Left;
  SHORT Top;
  SHORT Right;
  SHORT Bottom;
} SMALL_RECT, *PSMALL_RECT;


typedef VOID STDCALL
(*PTIMERAPCROUTINE)(
	LPVOID lpArgToCompletionRoutine,
	DWORD dwTimerLowValue,
	DWORD dwTimerHighValue
	);

#endif /* __INCLUDE_TYPES_H */
