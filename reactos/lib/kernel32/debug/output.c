/* $Id: output.c,v 1.2 2003/07/10 18:50:50 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/debug/debugger.c
 * PURPOSE:         OutputDebugString()
 * PROGRAMMER:      KJK::Hyperion <noog@libero.it>
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

/* FUNCTIONS *****************************************************************/

/* Open or create the mutex used to communicate with the debug monitor */
HANDLE K32CreateDBMonMutex(void)
{
 static SID_IDENTIFIER_AUTHORITY siaNTAuth = SECURITY_NT_AUTHORITY;
 static SID_IDENTIFIER_AUTHORITY siaWorldAuth = SECURITY_WORLD_SID_AUTHORITY;

 HANDLE hMutex;

 /* SIDs to be used in the DACL */
 PSID psidSystem = NULL;
 PSID psidAdministrators = NULL;
 PSID psidEveryone = NULL;
 
 /* buffer for the DACL */
 PVOID pDaclBuf = NULL;

 /*
  minimum size of the DACL: an ACL descriptor and three ACCESS_ALLOWED_ACE
  headers. We'll add the size of SIDs when we'll know it
 */
 SIZE_T nDaclBufSize =
  sizeof(ACL) +
  (
   sizeof(ACCESS_ALLOWED_ACE) -
   sizeof(((ACCESS_ALLOWED_ACE*)0)->SidStart)
  ) * 3;

 /* security descriptor of the mutex */
 SECURITY_DESCRIPTOR sdMutexSecurity;
 
 /* attributes of the mutex object we'll create */
 SECURITY_ATTRIBUTES saMutexAttribs =
 {
  sizeof(saMutexAttribs),
  &sdMutexSecurity,
  TRUE
 };

 NTSTATUS nErrCode;

 /* first, try to open the mutex */
 hMutex = OpenMutexW
 (
  SYNCHRONIZE | READ_CONTROL | MUTANT_QUERY_STATE,
  TRUE,
  L"DBWinMutex"
 );

 if(hMutex != NULL)
 {
  /* success */
  return hMutex;
 }
 /* error other than the mutex not being found */
 else if(GetLastError() != ERROR_FILE_NOT_FOUND)
 {
  /* failure */
  return NULL;
 }

 /* if the mutex doesn't exist, create it */
#if 0 /* please uncomment when GCC supports SEH */
 __try
 {
#else
#define __leave goto l_Cleanup
#endif
  /* first, set up the mutex security */
  /* allocate the NT AUTHORITY\SYSTEM SID */
  nErrCode = RtlAllocateAndInitializeSid
  (
   &siaNTAuth,
   1,
   SECURITY_LOCAL_SYSTEM_RID,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   &psidSystem
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) __leave;

  /* allocate the BUILTIN\Administrators SID */
  nErrCode = RtlAllocateAndInitializeSid
  (
   &siaNTAuth,
   2,
   SECURITY_BUILTIN_DOMAIN_RID,
   DOMAIN_ALIAS_RID_ADMINS,
   0,
   0,
   0,
   0,
   0,
   0,
   &psidAdministrators
  );
  
  /* failure */
  if(!NT_SUCCESS(nErrCode)) __leave;

  /* allocate the Everyone SID */
  nErrCode = RtlAllocateAndInitializeSid
  (
   &siaWorldAuth,
   1,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   &psidEveryone
  );
  
  /* failure */
  if(!NT_SUCCESS(nErrCode)) __leave;

  /* allocate space for the SIDs too */
  nDaclBufSize += RtlLengthSid(psidSystem);
  nDaclBufSize += RtlLengthSid(psidAdministrators);
  nDaclBufSize += RtlLengthSid(psidEveryone);

  /* allocate the buffer for the DACL */
  pDaclBuf = GlobalAlloc(GMEM_FIXED, nDaclBufSize);

  /* failure */
  if(pDaclBuf == NULL) __leave;

  /* create the DACL */
  nErrCode = RtlCreateAcl(pDaclBuf, nDaclBufSize, ACL_REVISION);

  /* failure */
  if(!NT_SUCCESS(nErrCode)) __leave;

  /* grant the minimum required access to Everyone */
  nErrCode = RtlAddAccessAllowedAce
  (
   pDaclBuf,
   ACL_REVISION,
   SYNCHRONIZE | READ_CONTROL | MUTANT_QUERY_STATE,
   psidEveryone
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) __leave;

  /* grant full access to BUILTIN\Administrators */
  nErrCode = RtlAddAccessAllowedAce
  (
   pDaclBuf,
   ACL_REVISION,
   MUTANT_ALL_ACCESS,
   psidAdministrators
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) __leave;

  /* grant full access to NT AUTHORITY\SYSTEM */
  nErrCode = RtlAddAccessAllowedAce
  (
   pDaclBuf,
   ACL_REVISION,
   MUTANT_ALL_ACCESS,
   psidSystem
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) __leave;

  /* create the security descriptor */
  nErrCode = RtlCreateSecurityDescriptor
  (     
   &sdMutexSecurity,
   SECURITY_DESCRIPTOR_REVISION
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) __leave;

  /* set the descriptor's DACL to the ACL we created */
  nErrCode = RtlSetDaclSecurityDescriptor
  (
   &sdMutexSecurity,
   TRUE,
   pDaclBuf,
   FALSE
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) __leave;

  /* create the mutex */
  hMutex = CreateMutexW(&saMutexAttribs, FALSE, L"DBWinMutex");
#if 0
 }
 __finally
 {
#else
l_Cleanup:
#endif
  /* free the buffers */
  if(pDaclBuf) GlobalFree(pDaclBuf);
  if(psidEveryone) RtlFreeSid(psidEveryone);
  if(psidAdministrators) RtlFreeSid(psidAdministrators);
  if(psidSystem) RtlFreeSid(psidSystem);
#if 0
 }
#endif
 
 return hMutex;
}


/*
 * @implemented
 */
VOID WINAPI OutputDebugStringA(LPCSTR _OutputString)
{
#if 0
/* FIXME: this will be pointless until GCC does SEH */
 __try
 {
  ULONG_PTR a_nArgs[2];

  a_nArgs[0] = (ULONG_PTR)(strlen(_OutputString) + 1);
  a_nArgs[1] = (ULONG_PTR)_OutputString;

  /* send the string to the user-mode debugger */
  RaiseException(0x40010006, 0, 2, a_nArgs);
 }
 __except(EXCEPTION_EXECUTE_HANDLER)
 {
#endif
  /*
   no user-mode debugger: try the systemwide debug message monitor, or the
   kernel debugger as a last resort
  */

  /* mutex used to synchronize invocations of OutputDebugString */
  static HANDLE s_hDBMonMutex = NULL;
  /* true if we already attempted to open/create the mutex */
  static BOOL s_bDBMonMutexTriedOpen = FALSE;

  /* local copy of the mutex handle */
  HANDLE hDBMonMutex = s_hDBMonMutex;
  /* handle to the Section of the shared buffer */
  HANDLE hDBMonBuffer = NULL;
  /*
   pointer to the mapped view of the shared buffer. It consist of the current
   process id followed by the message string
  */
  struct { DWORD ProcessId; CHAR Buffer[1]; } * pDBMonBuffer = NULL;
  /*
   event: signaled by the debug message monitor when OutputDebugString can write
   to the shared buffer
  */
  HANDLE hDBMonBufferReady = NULL;
  /*
   event: to be signaled by OutputDebugString when it's done writing to the
   shared buffer
  */
  HANDLE hDBMonDataReady = NULL;

  /* mutex not opened, and no previous attempts to open/create it */
  if(hDBMonMutex == NULL && !s_bDBMonMutexTriedOpen)
  {
   /* open/create the mutex */
   hDBMonMutex = K32CreateDBMonMutex();
   /* store the handle */
   s_hDBMonMutex = hDBMonMutex;
  }

#if 0
  __try
  {
#endif
   /* opening the mutex failed */
   if(hDBMonMutex == NULL)
   {
    /* remember next time */
    s_bDBMonMutexTriedOpen = TRUE;
   }
   /* opening the mutex succeeded */
   else
   {
    do
    {
     /* synchronize with other invocations of OutputDebugString */
     WaitForSingleObject(hDBMonMutex, INFINITE);
  
     /* buffer of the system-wide debug message monitor */
     hDBMonBuffer = OpenFileMappingW(SECTION_MAP_WRITE, FALSE, L"DBWIN_BUFFER");
  
     /* couldn't open the buffer: send the string to the kernel debugger */
     if(hDBMonBuffer == NULL) break;
  
     /* map the buffer */
     pDBMonBuffer = MapViewOfFile
     (
      hDBMonBuffer,
      SECTION_MAP_READ | SECTION_MAP_WRITE,
      0,
      0,
      0
     );
  
     /* couldn't map the buffer: send the string to the kernel debugger */
     if(pDBMonBuffer == NULL) break;
  
     /* open the event signaling that the buffer can be accessed */
     hDBMonBufferReady = OpenEventW(SYNCHRONIZE, FALSE, L"DBWIN_BUFFER_READY");
  
     /* couldn't open the event: send the string to the kernel debugger */
     if(hDBMonBufferReady == NULL) break;
  
     /* open the event to be signaled when the buffer has been filled */
     hDBMonDataReady =
      OpenEventW(EVENT_MODIFY_STATE, FALSE, L"DBWIN_DATA_READY");
    }
    while(0);

    /*
     we couldn't connect to the system-wide debug message monitor: send the
     string to the kernel debugger
    */
    if(hDBMonDataReady == NULL) ReleaseMutex(hDBMonMutex);
   }

#if 0
   __try
#else
   do
#endif
   {
    /* size of the current output block */
    SIZE_T nRoundLen;
    /* size of the remainder of the string */
    SIZE_T nOutputStringLen;

    for
    (
     /* output the whole string */
     nOutputStringLen = strlen(_OutputString);
     /* repeat until the string has been fully output */
     nOutputStringLen > 0;
     /* move to the next block */
     _OutputString += nRoundLen, nOutputStringLen -= nRoundLen
    )
    {
     /*
      we're connected to the debug monitor: write the current block to the
      shared buffer
     */
     if(hDBMonDataReady)
     {
      /*
       wait a maximum of 10 seconds for the debug monitor to finish processing
       the shared buffer
      */
      if(WaitForSingleObject(hDBMonBufferReady, 10000) != WAIT_OBJECT_0)
      {
       /* timeout or failure: give up */
       break;
      }

      /* write the process id into the buffer */
      pDBMonBuffer->ProcessId = GetCurrentProcessId();

      /* write only as many bytes as they fit in the buffer */
      if(nOutputStringLen > (PAGE_SIZE - sizeof(DWORD) - 1))
       nRoundLen = PAGE_SIZE - sizeof(DWORD) - 1;
      else
       nRoundLen = nOutputStringLen;
 
      /* copy the current block into the buffer */
      memcpy(pDBMonBuffer->Buffer, _OutputString, nOutputStringLen);
 
      /* null-terminate the current block */
      pDBMonBuffer->Buffer[nOutputStringLen] = 0;
 
      /* signal that the data contains meaningful data and can be read */
      SetEvent(hDBMonDataReady);
     }
     /* else, send the current block to the kernel debugger */
     else
     {
      /* output in blocks of 512 characters */
      CHAR a_cBuffer[512];
 
      /* write a maximum of 511 bytes */
      if(nOutputStringLen > (sizeof(a_cBuffer) - 1))
       nRoundLen = sizeof(a_cBuffer) - 1;
      else
       nRoundLen = nOutputStringLen;
 
      /* copy the current block */
      memcpy(a_cBuffer, _OutputString, nRoundLen);
 
      /* null-terminate the current block */
      a_cBuffer[nRoundLen] = 0;
 
      /* send the current block to the kernel debugger */
      DbgPrint("%s", a_cBuffer);
     }
    }
   }
#if 0
   /* ignore access violations and let other exceptions fall through */
   __except
   (
    (GetExceptionCode() == STATUS_ACCESS_VIOLATION) ?
    EXCEPTION_EXECUTE_HANDLER :
    EXCEPTION_CONTINUE_SEARCH
   )
   {
    /* string copied verbatim from Microsoft's kernel32.dll */
    DbgPrint("\nOutputDebugString faulted during output\n");
   }
#else
   while(0);
#endif

#if 0
  }
  __finally
  {
#endif
   /* close all the still open resources */
   if(hDBMonBufferReady) CloseHandle(hDBMonBufferReady);
   if(pDBMonBuffer) UnmapViewOfFile(pDBMonBuffer);
   if(hDBMonBuffer) CloseHandle(hDBMonBuffer);
   if(hDBMonDataReady) CloseHandle(hDBMonDataReady);

   /* leave the critical section */
   ReleaseMutex(hDBMonMutex);
#if 0
  }
 }
#endif
}


/*
 * @implemented
 */
VOID WINAPI OutputDebugStringW(LPCWSTR _OutputString)
{
 UNICODE_STRING wstrOut;
 ANSI_STRING strOut;
 NTSTATUS nErrCode;

 /* convert the string in ANSI */
 RtlInitUnicodeString(&wstrOut, _OutputString);
 nErrCode = RtlUnicodeStringToAnsiString(&strOut, &wstrOut, TRUE);

 if(!NT_SUCCESS(nErrCode))
 {
  /*
   Microsoft's kernel32.dll always prints something, even in case the conversion
   fails
  */
  OutputDebugStringA("");
 }
 else
 {
  /* output the converted string */
  OutputDebugStringA(strOut.Buffer);
  
  /* free the converted string */
  RtlFreeAnsiString(&strOut);
 }
}

/* EOF */
