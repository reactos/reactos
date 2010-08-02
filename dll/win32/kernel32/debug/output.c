/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/debug/debugger.c
 * PURPOSE:         OutputDebugString()
 * PROGRAMMER:      KJK::Hyperion <hackbunny@reactos.com>
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

/* FUNCTIONS *****************************************************************/

/* Open or create the mutex used to communicate with the debug monitor */
static
HANDLE
K32CreateDBMonMutex(void)
{
    static SID_IDENTIFIER_AUTHORITY siaNTAuth = {SECURITY_NT_AUTHORITY};
    static SID_IDENTIFIER_AUTHORITY siaWorldAuth = {SECURITY_WORLD_SID_AUTHORITY};
    HANDLE hMutex;

    /* SIDs to be used in the DACL */
    PSID psidSystem = NULL;
    PSID psidAdministrators = NULL;
    PSID psidEveryone = NULL;

    /* buffer for the DACL */
    PVOID pDaclBuf = NULL;

    /* minimum size of the DACL: an ACL descriptor and three ACCESS_ALLOWED_ACE
       headers. We'll add the size of SIDs when we'll know it
    */
    SIZE_T nDaclBufSize =
         sizeof(ACL) + (sizeof(ACCESS_ALLOWED_ACE) -
                        sizeof(((ACCESS_ALLOWED_ACE*)0)->SidStart)) * 3;

    /* security descriptor of the mutex */
    SECURITY_DESCRIPTOR sdMutexSecurity;

    /* attributes of the mutex object we'll create */
    SECURITY_ATTRIBUTES saMutexAttribs = {sizeof(saMutexAttribs),
                                          &sdMutexSecurity,
                                          TRUE};

    NTSTATUS nErrCode;

    /* first, try to open the mutex */
    hMutex = OpenMutexW (SYNCHRONIZE | READ_CONTROL | MUTANT_QUERY_STATE,
                         TRUE,
                         L"DBWinMutex");

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

    /* first, set up the mutex security */
    /* allocate the NT AUTHORITY\SYSTEM SID */
    nErrCode = RtlAllocateAndInitializeSid(&siaNTAuth,
                                           1,
                                           SECURITY_LOCAL_SYSTEM_RID,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           &psidSystem);

    /* failure */
    if(!NT_SUCCESS(nErrCode)) goto l_Cleanup;

    /* allocate the BUILTIN\Administrators SID */
    nErrCode = RtlAllocateAndInitializeSid(&siaNTAuth,
                                           2,
                                           SECURITY_BUILTIN_DOMAIN_RID,
                                           DOMAIN_ALIAS_RID_ADMINS,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           &psidAdministrators);

    /* failure */
    if(!NT_SUCCESS(nErrCode)) goto l_Cleanup;

    /* allocate the Everyone SID */
    nErrCode = RtlAllocateAndInitializeSid(&siaWorldAuth,
                                           1,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           &psidEveryone);

    /* failure */
    if(!NT_SUCCESS(nErrCode)) goto l_Cleanup;

    /* allocate space for the SIDs too */
    nDaclBufSize += RtlLengthSid(psidSystem);
    nDaclBufSize += RtlLengthSid(psidAdministrators);
    nDaclBufSize += RtlLengthSid(psidEveryone);

    /* allocate the buffer for the DACL */
    pDaclBuf = GlobalAlloc(GMEM_FIXED, nDaclBufSize);

    /* failure */
    if(pDaclBuf == NULL) goto l_Cleanup;

    /* create the DACL */
    nErrCode = RtlCreateAcl(pDaclBuf, nDaclBufSize, ACL_REVISION);

    /* failure */
    if(!NT_SUCCESS(nErrCode)) goto l_Cleanup;

    /* grant the minimum required access to Everyone */
    nErrCode = RtlAddAccessAllowedAce(pDaclBuf,
                                      ACL_REVISION,
                                      SYNCHRONIZE |
                                      READ_CONTROL |
                                      MUTANT_QUERY_STATE,
                                      psidEveryone);

    /* failure */
    if(!NT_SUCCESS(nErrCode)) goto l_Cleanup;

    /* grant full access to BUILTIN\Administrators */
    nErrCode = RtlAddAccessAllowedAce(pDaclBuf,
                                      ACL_REVISION,
                                      MUTANT_ALL_ACCESS,
                                      psidAdministrators);

    /* failure */
    if(!NT_SUCCESS(nErrCode)) goto l_Cleanup;

    /* grant full access to NT AUTHORITY\SYSTEM */
    nErrCode = RtlAddAccessAllowedAce(pDaclBuf,
                                      ACL_REVISION,
                                      MUTANT_ALL_ACCESS,
                                      psidSystem);

    /* failure */
    if(!NT_SUCCESS(nErrCode)) goto l_Cleanup;

    /* create the security descriptor */
    nErrCode = RtlCreateSecurityDescriptor(&sdMutexSecurity,
                                           SECURITY_DESCRIPTOR_REVISION);

    /* failure */
    if(!NT_SUCCESS(nErrCode)) goto l_Cleanup;

    /* set the descriptor's DACL to the ACL we created */
    nErrCode = RtlSetDaclSecurityDescriptor(&sdMutexSecurity,
                                            TRUE,
                                            pDaclBuf,
                                            FALSE);

    /* failure */
    if(!NT_SUCCESS(nErrCode)) goto l_Cleanup;

    /* create the mutex */
    hMutex = CreateMutexW(&saMutexAttribs, FALSE, L"DBWinMutex");

l_Cleanup:
    /* free the buffers */
    if(pDaclBuf) GlobalFree(pDaclBuf);
    if(psidEveryone) RtlFreeSid(psidEveryone);
    if(psidAdministrators) RtlFreeSid(psidAdministrators);
    if(psidSystem) RtlFreeSid(psidSystem);

	return hMutex;
}


/*
 * @implemented
 */
VOID
WINAPI
OutputDebugStringA(LPCSTR _OutputString)
{
	_SEH2_TRY
	{
		ULONG_PTR a_nArgs[2];

		a_nArgs[0] = (ULONG_PTR)(strlen(_OutputString) + 1);
		a_nArgs[1] = (ULONG_PTR)_OutputString;

		/* send the string to the user-mode debugger */
		RaiseException(DBG_PRINTEXCEPTION_C, 0, 2, a_nArgs);
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		/* no user-mode debugger: try the systemwide debug message monitor, or the
		   kernel debugger as a last resort */

		/* mutex used to synchronize invocations of OutputDebugString */
		static HANDLE s_hDBMonMutex = NULL;
		/* true if we already attempted to open/create the mutex */
		static BOOL s_bDBMonMutexTriedOpen = FALSE;

		/* local copy of the mutex handle */
		volatile HANDLE hDBMonMutex = s_hDBMonMutex;
		/* handle to the Section of the shared buffer */
		volatile HANDLE hDBMonBuffer = NULL;

		/* pointer to the mapped view of the shared buffer. It consist of the current
		   process id followed by the message string */
		struct { DWORD ProcessId; CHAR Buffer[1]; } * pDBMonBuffer = NULL;

		/* event: signaled by the debug message monitor when OutputDebugString can write
		   to the shared buffer */
		volatile HANDLE hDBMonBufferReady = NULL;

		/* event: to be signaled by OutputDebugString when it's done writing to the
		   shared buffer */
		volatile HANDLE hDBMonDataReady = NULL;

		/* mutex not opened, and no previous attempts to open/create it */
		if(hDBMonMutex == NULL && !s_bDBMonMutexTriedOpen)
		{
			/* open/create the mutex */
			hDBMonMutex = K32CreateDBMonMutex();
			/* store the handle */
			s_hDBMonMutex = hDBMonMutex;
		}

		_SEH2_TRY
		{
			volatile PCHAR a_cBuffer = NULL;

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
					pDBMonBuffer = MapViewOfFile(hDBMonBuffer,
												 SECTION_MAP_READ | SECTION_MAP_WRITE,
												 0,
												 0,
												 0);

					/* couldn't map the buffer: send the string to the kernel debugger */
					if(pDBMonBuffer == NULL) break;

					/* open the event signaling that the buffer can be accessed */
					hDBMonBufferReady = OpenEventW(SYNCHRONIZE, FALSE, L"DBWIN_BUFFER_READY");

					/* couldn't open the event: send the string to the kernel debugger */
					if(hDBMonBufferReady == NULL) break;

					/* open the event to be signaled when the buffer has been filled */
					hDBMonDataReady = OpenEventW(EVENT_MODIFY_STATE, FALSE, L"DBWIN_DATA_READY");
				}
				while(0);

				/* we couldn't connect to the system-wide debug message monitor: send the
				   string to the kernel debugger */
				if(hDBMonDataReady == NULL) ReleaseMutex(hDBMonMutex);
			}

			_SEH2_TRY
			{
				/* size of the current output block */
				volatile SIZE_T nRoundLen;

				/* size of the remainder of the string */
				volatile SIZE_T nOutputStringLen;

				/* output the whole string */
				nOutputStringLen = strlen(_OutputString);

				do
				{
					/* we're connected to the debug monitor:
					   write the current block to the shared buffer */
					if(hDBMonDataReady)
					{
						/* wait a maximum of 10 seconds for the debug monitor
						   to finish processing the shared buffer */
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
						memcpy(pDBMonBuffer->Buffer, _OutputString, nRoundLen);

						/* null-terminate the current block */
						pDBMonBuffer->Buffer[nRoundLen] = 0;

						/* signal that the data contains meaningful data and can be read */
						SetEvent(hDBMonDataReady);
					}
					/* else, send the current block to the kernel debugger */
					else
					{
						/* output in blocks of 512 characters */
						a_cBuffer = (CHAR*)HeapAlloc(GetProcessHeap(), 0, 512);

						if (!a_cBuffer)
						{
							DbgPrint("OutputDebugStringA: Failed\n");
							break;
						}

						/* write a maximum of 511 bytes */
						if(nOutputStringLen > 510)
							nRoundLen = 510;
						else
							nRoundLen = nOutputStringLen;

						/* copy the current block */
						memcpy(a_cBuffer, _OutputString, nRoundLen);

						/* null-terminate the current block */
						a_cBuffer[nRoundLen] = 0;

						/* send the current block to the kernel debugger */
						DbgPrint("%s", a_cBuffer);

						if (a_cBuffer)
						{
							HeapFree(GetProcessHeap(), 0, a_cBuffer);
							a_cBuffer = NULL;
						}
					}

					/* move to the next block */
					_OutputString += nRoundLen;
					nOutputStringLen -= nRoundLen;
				}
				/* repeat until the string has been fully output */
				while (nOutputStringLen > 0);
			}
			/* ignore access violations and let other exceptions fall through */
			_SEH2_EXCEPT((_SEH2_GetExceptionCode() == STATUS_ACCESS_VIOLATION) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
			{
				if (a_cBuffer)
					HeapFree(GetProcessHeap(), 0, a_cBuffer);

				/* string copied verbatim from Microsoft's kernel32.dll */
				DbgPrint("\nOutputDebugString faulted during output\n");
			}
			_SEH2_END;
		}
		_SEH2_FINALLY
		{
			/* close all the still open resources */
			if(hDBMonBufferReady) CloseHandle(hDBMonBufferReady);
			if(pDBMonBuffer) UnmapViewOfFile(pDBMonBuffer);
			if(hDBMonBuffer) CloseHandle(hDBMonBuffer);
			if(hDBMonDataReady) CloseHandle(hDBMonDataReady);

			/* leave the critical section */
			if(hDBMonDataReady != NULL)
				ReleaseMutex(hDBMonMutex);
		}
		_SEH2_END;
	}
	_SEH2_END;
}


/*
 * @implemented
 */
VOID
WINAPI
OutputDebugStringW(LPCWSTR _OutputString)
{
    UNICODE_STRING wstrOut;
    ANSI_STRING strOut;
    NTSTATUS nErrCode;

    /* convert the string in ANSI */
    RtlInitUnicodeString(&wstrOut, _OutputString);
    nErrCode = RtlUnicodeStringToAnsiString(&strOut, &wstrOut, TRUE);

    if(!NT_SUCCESS(nErrCode))
    {
		/* Microsoft's kernel32.dll always prints something, even in case the conversion fails */
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
