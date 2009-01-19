/* tls.h */

#ifndef __CRT_INTERNAL_TLS_H
#define __CRT_INTERNAL_TLS_H

#ifndef _CRT_PRECOMP_H
#error DO NOT INCLUDE THIS HEADER DIRECTLY
#endif

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winnt.h>

#include <stddef.h>

typedef struct _ThreadData
{
  int terrno;                   /* *nix error code */
  unsigned long tdoserrno;      /* Win32 error code (for I/O only) */
  unsigned __int64 tnext;       /* used by rand/srand */

  char *lasttoken;              /* used by strtok */
  wchar_t *wlasttoken;          /* used by wcstok */


  int fpecode;                  /* fp exception code */

  EXCEPTION_RECORD *exc_record; /* Head of exception record list */
  
  struct tm tmbuf;              /* Used by gmtime, mktime, mkgmtime, localtime */
  char asctimebuf[26];          /* Buffer for asctime and ctime */
  wchar_t wasctimebuf[26];      /* Buffer for wasctime and wctime */

} THREADDATA, *PTHREADDATA;


int CreateThreadData(void);
void DestroyThreadData(void);

void FreeThreadData(PTHREADDATA ptd);
PTHREADDATA GetThreadData(void);

#endif /* __MSVCRT_INTERNAL_TLS_H */

/* EOF */

