/* tls.c */

#include <windows.h>
#include <msvcrt/internal/tls.h>
#include <msvcrt/internal/rterror.h>


static unsigned long TlsIndex = (unsigned long)-1;


static void InitThreadData(PTHREADDATA ThreadData)
{
   ThreadData->terrno = 0;
   ThreadData->tdoserrno = 0;

   ThreadData->fpecode = 0;

   /* FIXME: init more thread local data */

}


int CreateThreadData(void)
{
   PTHREADDATA ThreadData;

   TlsIndex = TlsAlloc();
   if (TlsIndex == (unsigned long)-1)
     return FALSE;

   ThreadData = (PTHREADDATA)calloc(1, sizeof(THREADDATA));
   if (ThreadData == NULL)
     return FALSE;

   if(!TlsSetValue(TlsIndex, (LPVOID)ThreadData))
     return FALSE;

   InitThreadData(ThreadData);

   return TRUE;
}


void DestroyThreadData(void)
{
   if (TlsIndex != (unsigned long)-1)
     {
	TlsFree(TlsIndex);
	TlsIndex = (unsigned long)-1;
     }
}


void FreeThreadData(PTHREADDATA ThreadData)
{
   if (TlsIndex != (unsigned long)-1)
     {
	if (ThreadData == NULL)
	   ThreadData = TlsGetValue(TlsIndex);

	if (ThreadData != NULL)
	  {
	     /* FIXME: free more thread local data */

	     free(ThreadData);
	  }

	TlsSetValue(TlsIndex, NULL);
     }
}


PTHREADDATA GetThreadData(void)
{
   PTHREADDATA ThreadData;
   DWORD LastError;

   LastError = GetLastError();
   ThreadData = TlsGetValue(TlsIndex);
   if (ThreadData == NULL)
     {
	ThreadData = (PTHREADDATA)calloc(1, sizeof(THREADDATA));
	if (ThreadData != NULL)
	  {
	     TlsSetValue(TlsIndex, (LPVOID)ThreadData);

	     InitThreadData(ThreadData);
	  }
	else
	  {
	    _amsg_exit(_RT_THREAD); /* write message and die */
	  }
     }

   SetLastError(LastError);

   return ThreadData;
}

/* EOF */

