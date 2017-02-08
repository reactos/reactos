#define WIN32_NO_STATUS
#include <precomp.h>
#define NTOS_MODE_USER
#include <setjmp.h>
#include <ndk/umtypes.h>
#include <ndk/extypes.h>
#include <ndk/rtlfuncs.h>

/* VC++ extensions to Win32 SEH */
typedef struct _SCOPETABLE
{
  int previousTryLevel;
  int (*lpfnFilter)(PEXCEPTION_POINTERS);
  int (*lpfnHandler)(void);
} SCOPETABLE, *PSCOPETABLE;

typedef struct _MSVCRT_EXCEPTION_FRAME
{
  PEXCEPTION_REGISTRATION_RECORD *prev;
  void (*handler)(PEXCEPTION_RECORD, PEXCEPTION_REGISTRATION_RECORD,
                  PCONTEXT, PEXCEPTION_RECORD);
  PSCOPETABLE scopetable;
  int trylevel;
  int _ebp;
  PEXCEPTION_POINTERS xpointers;
} MSVCRT_EXCEPTION_FRAME;

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
