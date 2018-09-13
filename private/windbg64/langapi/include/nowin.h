/***
*nowin.h -
*
*	Copyright (c) 1996, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Declare misc. types, macros, etc. for non-Windows platforms that would
*       normally be declared in windows.h
*
*Revision History:
*       04-25-96  PNT	Module Created.
*
****/

#if !defined(_NOWIN_INCLUDED)
#define _NOWIN_INCLUDED

typedef unsigned int            UINT;
typedef long                    LONG;
typedef unsigned long           ULONG;
typedef ULONG                   *PULONG;
typedef unsigned short          USHORT;
typedef USHORT                  *PUSHORT;
typedef char                    CHAR;
typedef CHAR                    *PCHAR;
typedef unsigned char           UCHAR;
typedef UCHAR                   *PUCHAR;
typedef char                    *PSZ;
typedef unsigned char           BYTE;
typedef unsigned short          WORD;
typedef unsigned long           DWORD;
typedef int                     BOOL;
typedef unsigned char           BOOLEAN;
typedef BOOLEAN                 *PBOOLEAN;
typedef void                    VOID;
typedef VOID                    *PVOID;

#define FALSE	                0
#define TRUE	                1

#define IN
#define OUT
#define OPTIONAL

#define FAR
#define WINAPI
typedef int                     (FAR WINAPI *FARPROC)();

typedef ULONG                   NTSTATUS;

#ifndef NOMINMAX

#ifndef max
#define max(a, b)               (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b)               (((a) < (b)) ? (a) : (b))
#endif

#endif  /* NOMINMAX */

// Determine if an argument is present by testing the value of the pointer
// to the argument value.
//

#define ARGUMENT_PRESENT(ArgumentPointer)    (\
    (CHAR *)(ArgumentPointer) != (CHAR *)(NULL) )

//
// Exception record definition.
//

#define EXCEPTION_MAXIMUM_PARAMETERS 15 // maximum number of exception parameters

typedef struct _EXCEPTION_RECORD {
    /*lint -e18 */  // Don't complain about different definitions
    LONG ExceptionCode;
    /*lint +e18 */  // Resume checking for different definitions
    ULONG ExceptionFlags;
    struct _EXCEPTION_RECORD *ExceptionRecord;
    PVOID ExceptionAddress;
    ULONG NumberParameters;
    ULONG ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD;

typedef EXCEPTION_RECORD *PEXCEPTION_RECORD;

#if defined(_M_MPPC)
//
// Context Frame
//
//  N.B. This frame must be exactly a multiple of 16 bytes in length.
//
//  This frame has a several purposes: 1) it is used as an argument to
//  NtContinue, 2) it is used to constuct a call frame for APC delivery,
//  3) it is used to construct a call frame for exception dispatching
//  in user mode, and 4) it is used in the user level thread creation
//  routines.
//
//  Requires at least 8-byte alignment (double)
//

typedef struct _CONTEXT {

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_FLOATING_POINT.
    //

    double Fpr0;                        // Floating registers 0..31
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
    double Fpscr;                       // Floating point status/control reg

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_INTEGER.
    //

    ULONG Gpr0;                         // General registers 0..31
    ULONG Gpr1;
    ULONG Gpr2;
    ULONG Gpr3;
    ULONG Gpr4;
    ULONG Gpr5;
    ULONG Gpr6;
    ULONG Gpr7;
    ULONG Gpr8;
    ULONG Gpr9;
    ULONG Gpr10;
    ULONG Gpr11;
    ULONG Gpr12;
    ULONG Gpr13;
    ULONG Gpr14;
    ULONG Gpr15;
    ULONG Gpr16;
    ULONG Gpr17;
    ULONG Gpr18;
    ULONG Gpr19;
    ULONG Gpr20;
    ULONG Gpr21;
    ULONG Gpr22;
    ULONG Gpr23;
    ULONG Gpr24;
    ULONG Gpr25;
    ULONG Gpr26;
    ULONG Gpr27;
    ULONG Gpr28;
    ULONG Gpr29;
    ULONG Gpr30;
    ULONG Gpr31;

    ULONG Cr;                           // Condition register
    ULONG Xer;                          // Fixed point exception register

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_CONTROL.
    //

    ULONG Msr;                          // Machine status register
    ULONG Iar;                          // Instruction address register
    ULONG Lr;                           // Link register
    ULONG Ctr;                          // Count register

    //
    // The flags values within this flag control the contents of
    // a CONTEXT record.
    //
    // If the context record is used as an input parameter, then
    // for each portion of the context record controlled by a flag
    // whose value is set, it is assumed that that portion of the
    // context record contains valid context. If the context record
    // is being used to modify a thread's context, then only that
    // portion of the threads context will be modified.
    //
    // If the context record is used as an IN OUT parameter to capture
    // the context of a thread, then only those portions of the thread's
    // context corresponding to set flags will be returned.
    //
    // The context record is never used as an OUT only parameter.
    //

    ULONG ContextFlags;

    ULONG Fill[3];                      // Pad out to multiple of 16 bytes

    //
    // This section is specified/returned if CONTEXT_DEBUG_REGISTERS is
    // set in ContextFlags.  Note that CONTEXT_DEBUG_REGISTERS is NOT
    // included in CONTEXT_FULL.
    //
    ULONG Dr0;                          // Breakpoint Register 1
    ULONG Dr1;                          // Breakpoint Register 2
    ULONG Dr2;                          // Breakpoint Register 3
    ULONG Dr3;                          // Breakpoint Register 4
    ULONG Dr4;                          // Breakpoint Register 5
    ULONG Dr5;                          // Breakpoint Register 6
    ULONG Dr6;                          // Debug Status Register
    ULONG Dr7;                          // Debug Control Register

} CONTEXT, *PCONTEXT;

//
// Nonvolatile context pointer record.
//

typedef struct _KNONVOLATILE_CONTEXT_POINTERS {
    PULONG FloatingContext[32];
    PULONG FpscrContext;
    PULONG IntegerContext[32];
    PULONG CrContext;
    PULONG XerContext;
    PULONG MsrContext;
    PULONG IarContext;
    PULONG LrContext;
    PULONG CtrContext;
} KNONVOLATILE_CONTEXT_POINTERS, *PKNONVOLATILE_CONTEXT_POINTERS;
#endif

#endif
