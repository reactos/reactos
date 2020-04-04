/* NFSv4.1 client for Windows
 * Copyright © 2012 The Regents of the University of Michigan
 *
 * Olga Kornievskaia <aglo@umich.edu>
 * Casey Bodley <cbodley@umich.edu>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of merchantability
 * or fitness for a particular purpose.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 */

#include <wintirpc.h>
#include <rpc/rpc.h>
#include <stdio.h>
#ifndef __REACTOS__
#include <winsock.h>
#else
#include <winsock2.h>
#endif

WSADATA WSAData;

static int init = 0;
static DWORD dwTlsIndex;

extern void multithread_init(void);

VOID
tirpc_report(LPTSTR lpszMsg)
{
	WCHAR    chMsg[256];
	HANDLE   hEventSource;
	LPCWSTR  lpszStrings[2];

	// Use event logging to log the error.
	//
	hEventSource = RegisterEventSource(NULL,
									   TEXT("tirpc.dll"));

#ifndef __REACTOS__
	swprintf_s(chMsg, sizeof(chMsg), L"tirpc report: %d", GetLastError());
#else
	_snwprintf(chMsg, sizeof(chMsg) / sizeof(WCHAR), L"tirpc report: %d", GetLastError());
#endif
	lpszStrings[0] = (LPCWSTR)chMsg;
	lpszStrings[1] = lpszMsg;

	if (hEventSource != NULL) {
		ReportEvent(hEventSource, // handle of event source
			EVENTLOG_WARNING_TYPE, // event type
			0,                    // event category
			0,                    // event ID
			NULL,                 // current user's SID
			2,                    // strings in lpszStrings
			0,                    // no bytes of raw data
			lpszStrings,          // array of error strings
			NULL);                // no raw data

		(VOID) DeregisterEventSource(hEventSource);
	}
}

void tirpc_criticalsection_init(void) {
	multithread_init();
}

BOOL winsock_init(void)
{
	int err;
	err = WSAStartup(MAKEWORD( 3, 3 ), &WSAData);	// XXX THIS SHOULD BE FAILING!!!!!!!!!!!!!!!!!
	if (err != 0) {
		init = 0;
		tirpc_report(L"WSAStartup failed!\n");
		WSACleanup();
		return FALSE;
	}
	return TRUE;
}

BOOL winsock_fini(void)
{
	WSACleanup();
	return TRUE;
}

#ifdef __REACTOS__
char NETCONFIG[MAX_PATH] = "";
#endif
BOOL WINAPI DllMain/*tirpc_main*/(HINSTANCE hinstDLL,	// DLL module handle
					   DWORD fdwReason,	// reason called
					   LPVOID lpvReserved)	// reserved
{
	LPVOID lpvData; 
	BOOL fIgnore; 

//	if (init++)
//		return TRUE;

	// Deal with Thread Local Storage initialization!!
	switch (fdwReason) 
    {
		// The DLL is loading due to process
		// initialization or a call to LoadLibrary. 
        case DLL_PROCESS_ATTACH:
#ifdef __REACTOS__
            if (!GetSystemDirectoryA(NETCONFIG, ARRAYSIZE(NETCONFIG)))
                return FALSE;

            lstrcatA(NETCONFIG, "\\drivers\\etc\\netconfig");
#endif
			
			// Initialize socket library
			if (winsock_init() == FALSE)
				return FALSE;

			// Initialize CriticalSections
			tirpc_criticalsection_init();

            // Allocate a TLS index. 
            if ((dwTlsIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES) 
                return FALSE; 
 
            // No break: Initialize the index for first thread.
 
        // The attached process creates a new thread. 
        case DLL_THREAD_ATTACH: 
 
            // Initialize the TLS index for this thread
            lpvData = (LPVOID) LocalAlloc(LPTR, 256); 
            if (lpvData != NULL) 
                fIgnore = TlsSetValue(dwTlsIndex, lpvData); 
 
            break; 
 
        // The thread of the attached process terminates.
        case DLL_THREAD_DETACH: 
 
            // Release the allocated memory for this thread.
            lpvData = TlsGetValue(dwTlsIndex); 
            if (lpvData != NULL) 
                LocalFree((HLOCAL) lpvData); 
 
            break; 
 
        // DLL unload due to process termination or FreeLibrary. 
        case DLL_PROCESS_DETACH: 
 
            // Release the allocated memory for this thread.
            lpvData = TlsGetValue(dwTlsIndex); 
            if (lpvData != NULL) 
                LocalFree((HLOCAL) lpvData); 
 
            // Release the TLS index.
            TlsFree(dwTlsIndex);

			// Clean up winsock stuff
			winsock_fini();

            break; 
 
        default: 
            break; 
    } 


	return TRUE;
}

int tirpc_exit(void)
{
	if (init == 0 || --init > 0)
		return 0;

	return WSACleanup();
}


void wintirpc_debug(char *fmt, ...)
{
#ifdef _DEBUG
	char buffer[2048];
#else
	static int triedToOpen = 0;
	static FILE *dbgFile = NULL;
#endif

	va_list vargs;
	va_start(vargs, fmt);

#ifdef _DEBUG
	vsprintf(buffer, fmt, vargs);
	OutputDebugStringA(buffer);
#else
	if (dbgFile == NULL && triedToOpen == 0) {
		triedToOpen = 1;
		dbgFile = fopen("c:\\etc\\rpcsec_gss_debug.txt", "w");
	}
	if (dbgFile != NULL) {
		vfprintf(dbgFile, fmt, vargs);
		fflush(dbgFile);
	}
#endif

	va_end(vargs);
}
