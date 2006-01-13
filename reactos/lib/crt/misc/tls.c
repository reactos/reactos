#include <precomp.h>
#include <stdlib.h>
#include <internal/tls.h>
#include <internal/rterror.h>


static DWORD TlsIndex = TLS_OUT_OF_INDEXES;


static void InitThreadData(PTHREADDATA ThreadData)
{
   ThreadData->terrno = 0;
   ThreadData->tdoserrno = 0;

   ThreadData->fpecode = 0;

   /* FIXME: init more thread local data */

}


int SetThreadData(PTHREADDATA ThreadData)
{
   if(TlsIndex == TLS_OUT_OF_INDEXES ||
      !TlsSetValue(TlsIndex, ThreadData))
     return FALSE;

   InitThreadData(ThreadData);

   return TRUE;
}


int CreateThreadData(void)
{
   TlsIndex = TlsAlloc();
   return (TlsIndex != TLS_OUT_OF_INDEXES);
}


void DestroyThreadData(void)
{
   if (TlsIndex != TLS_OUT_OF_INDEXES)
     {
	TlsFree(TlsIndex);
	TlsIndex = TLS_OUT_OF_INDEXES;
     }
}


void FreeThreadData(PTHREADDATA ThreadData)
{
   if (TlsIndex != TLS_OUT_OF_INDEXES)
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

             ThreadData->hThread = GetCurrentThread();
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

