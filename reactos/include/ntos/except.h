/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/ntos/except.h
 * PURPOSE:      Exception handling structures
 * PROGRAMMER:   Casper S. Hornstrup <chorns@users.sourceforge.net>
 */

#ifndef __INCLUDE_EXCEPT_H
#define __INCLUDE_EXCEPT_H

typedef enum {
	ExceptionContinueExecution = 0,
	ExceptionContinueSearch,
	ExceptionNestedException,
	ExceptionCollidedUnwind,
  ExceptionDismiss  // ???
} EXCEPTION_DISPOSITION;


struct _EXCEPTION_RECORD;
struct _EXCEPTION_REGISTRATION;

/*
 * The type of function that is expected as an exception handler to be
 * installed with _try1.
 */
#ifdef __GNUC__
typedef EXCEPTION_DISPOSITION (CDECL *PEXCEPTION_HANDLER)(
  struct _EXCEPTION_RECORD* ExceptionRecord,
  struct _EXCEPTION_REGISTRATION* ExceptionRegistration,
  PCONTEXT Context,
  PVOID DispatcherContext);
#else
typedef EXCEPTION_DISPOSITION (CDECL *PEXCEPTION_HANDLER)(
  struct _EXCEPTION_RECORD* ExceptionRecord,
  struct _EXCEPTION_REGISTRATION* ExceptionRegistration,
  PCONTEXT Context,
  PVOID DispatcherContext);
#endif /*__GNUC__*/

#ifndef __USE_W32API

#define EXCEPTION_MAXIMUM_PARAMETERS	(15)

typedef struct _EXCEPTION_RECORD {
  DWORD ExceptionCode;
  DWORD ExceptionFlags;
  struct _EXCEPTION_RECORD *ExceptionRecord;
  PVOID ExceptionAddress;
  DWORD NumberParameters;
  DWORD ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD, *PEXCEPTION_RECORD, *LPEXCEPTION_RECORD;

#endif /* !__USE_W32API */

/* ExceptionFlags */
#ifndef _GNU_H_WINDOWS32_DEFINES
#ifdef __NTOSKRNL__
#define	EXCEPTION_NONCONTINUABLE	0x01
#endif /* __NTOSKRNL__ */
#endif /* _GNU_H_WINDOWS32_DEFINES */
#define	EXCEPTION_UNWINDING       0x02
#define	EXCEPTION_EXIT_UNWIND		  0x04
#define	EXCEPTION_STACK_INVALID	  0x08
#define	EXCEPTION_NESTED_CALL		  0x10


typedef struct _EXCEPTION_REGISTRATION
{
	struct _EXCEPTION_REGISTRATION*	prev;
	PEXCEPTION_HANDLER		handler;
} EXCEPTION_REGISTRATION, *PEXCEPTION_REGISTRATION;

typedef EXCEPTION_REGISTRATION EXCEPTION_REGISTRATION_RECORD;
typedef PEXCEPTION_REGISTRATION PEXCEPTION_REGISTRATION_RECORD;


/*
 * A macro which installs the supplied exception handler.
 * Push the pointer to the new handler onto the stack,
 * then push the pointer to the old registration structure (at fs:0)
 * onto the stack, then put a pointer to the new registration
 * structure (i.e. the current stack pointer) at fs:0.
 */
#define __try1(pHandler) \
	__asm__ ("pushl %0;pushl %%fs:0;movl %%esp,%%fs:0;" : : "g" (pHandler));


/*
 * A macro which (dispite its name) *removes* an installed
 * exception handler. Should be used only in conjunction with the above
 * install routine __try1.
 * Move the pointer to the old reg. struct (at the current stack
 * position) to fs:0, replacing the pointer we installed above,
 * then add 8 to the stack pointer to get rid of the space we
 * used when we pushed on our new reg. struct above. Notice that
 * the stack must be in the exact state at this point that it was
 * after we did _try1 or this will smash things.
 */
#define	__except1	\
	__asm__ ("movl (%%esp),%%eax;movl %%eax,%%fs:0;addl $8,%%esp;" \
	 : : : "%eax");


#if 1

// Runtime DLL structures

#ifndef _GNU_H_WINDOWS32_DEFINES
#ifdef __NTOSKRNL__
#define EXCEPTION_EXECUTE_HANDLER     1
#define EXCEPTION_CONTINUE_SEARCH     0
#define EXCEPTION_CONTINUE_EXECUTION -1
#endif /* __NTOSKRNL__ */
#endif /* _GNU_H_WINDOWS32_DEFINES */

// Functions of the following prototype return one of the above constants
#ifdef __GNUC__
typedef DWORD CDECL (*PSCOPE_EXCEPTION_FILTER)(VOID);
typedef VOID CDECL (*PSCOPE_EXCEPTION_HANDLER)(VOID);
#else
typedef DWORD (CDECL *PSCOPE_EXCEPTION_FILTER)(VOID);
typedef VOID (CDECL *PSCOPE_EXCEPTION_HANDLER)(VOID);
#endif /*__GNUC__*/

typedef struct _SCOPETABLE_ENTRY
{
  DWORD                    PreviousTryLevel;
  PSCOPE_EXCEPTION_FILTER  FilterRoutine;
  PSCOPE_EXCEPTION_HANDLER HandlerRoutine;
} SCOPETABLE_ENTRY, *PSCOPETABLE_ENTRY;

/*
   Other structures preceeding this structure:
     ULONG_PTR              StandardESPInFrame;
     LPEXCEPTION_POINTERS   ExceptionPointers;
 */
typedef struct _RTL_EXCEPTION_REGISTRATION_I386
{
  EXCEPTION_REGISTRATION OS;
  PSCOPETABLE_ENTRY ScopeTable;
  DWORD             TryLevel;
  /* Value of EBP before the EXCEPTION_REGISTRATION was created */
  ULONG_PTR         Ebp;
} RTL_EXCEPTION_REGISTRATION_I386, *PRTL_EXCEPTION_REGISTRATION_I386;

#define TRYLEVEL_NONE -1

typedef RTL_EXCEPTION_REGISTRATION_I386 RTL_EXCEPTION_REGISTRATION;
typedef PRTL_EXCEPTION_REGISTRATION_I386 PRTL_EXCEPTION_REGISTRATION;

#endif

#ifndef __USE_W32API

#define EXCEPTION_MAXIMUM_PARAMETERS	(15)

typedef struct _EXCEPTION_POINTERS { 
  PEXCEPTION_RECORD ExceptionRecord; 
  PCONTEXT ContextRecord; 
} EXCEPTION_POINTERS, *PEXCEPTION_POINTERS, *LPEXCEPTION_POINTERS; 

#endif /* !__USE_W32API */

#endif /* __INCLUDE_EXCEPT_H */
