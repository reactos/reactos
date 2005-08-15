#include "precomp.h"
#include <winternl.h>

/*
 * @implemented
 */
void __cdecl
_global_unwind2(PEXCEPTION_REGISTRATION RegistrationFrame)
{
#ifdef __GNUC__
   RtlUnwind(RegistrationFrame, &&__ret_label, NULL, 0);
__ret_label:
   // return is important
   return;
#else
#endif
}


// This is dragged over from WINE:

typedef struct __EXCEPTION_FRAME
{
  struct __EXCEPTION_FRAME *Prev;
  PEXCEPTION_HANDLER       Handler;
} EXCEPTION_FRAME, *PEXCEPTION_FRAME;

/* VC++ extensions to Win32 SEH */
typedef struct _SCOPETABLE
{
  int previousTryLevel;
  int (*lpfnFilter)(PEXCEPTION_POINTERS);
  int (*lpfnHandler)(void);
} SCOPETABLE, *PSCOPETABLE;

typedef struct _MSVCRT_EXCEPTION_FRAME
{
  EXCEPTION_FRAME *prev;
  void (*handler)(PEXCEPTION_RECORD, PEXCEPTION_FRAME,
                  PCONTEXT, PEXCEPTION_RECORD);
  PSCOPETABLE scopetable;
  int trylevel;
  int _ebp;
  PEXCEPTION_POINTERS xpointers;
} MSVCRT_EXCEPTION_FRAME;


typedef struct __JUMP_BUFFER
{
    unsigned long Ebp;
    unsigned long Ebx;
    unsigned long Edi;
    unsigned long Esi;
    unsigned long Esp;
    unsigned long Eip;
    unsigned long Registration;
    unsigned long TryLevel;
    /* Start of new struct members */
    unsigned long Cookie;
    unsigned long UnwindFunc;
    unsigned long UnwindData[6];
} _JUMP_BUFFER;

void
_local_unwind2(MSVCRT_EXCEPTION_FRAME *RegistrationFrame,
		      LONG TryLevel);

/*
 * @implemented
*/

void __stdcall _seh_longjmp_unwind(_JUMP_BUFFER *jmp)
{
    _local_unwind2((MSVCRT_EXCEPTION_FRAME*) jmp->Registration, jmp->TryLevel);
}
