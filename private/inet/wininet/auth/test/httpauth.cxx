// ===========================================================================
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 1996 Microsoft Corporation.  All Rights Reserved.
// ===========================================================================
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <io.h>
#include <wininet.h>

BOOL g_fAllowCustomUI;
DWORD g_dwConnectFlags;
BOOL g_fPreload;
BOOL g_fMonolithicUpload = FALSE;
DWORD g_dwPort = INTERNET_INVALID_PORT_NUMBER;
LPSTR g_szVerb = NULL;

//==============================================================================
BOOL NeedAuth (HINTERNET hRequest, DWORD *pdwStatus)
{
    // Get status code.
    DWORD dwStatus;
    DWORD cbStatus = sizeof(dwStatus);
    HttpQueryInfo
    (
        hRequest,
        HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE,
        &dwStatus,
        &cbStatus,
        NULL
    );
    fprintf (stderr, "Status: %d\n", dwStatus);
    *pdwStatus = dwStatus;

    // Look for 401 or 407.
    DWORD dwFlags;
    switch (dwStatus)
    {
        case HTTP_STATUS_DENIED:
            dwFlags = HTTP_QUERY_WWW_AUTHENTICATE;
            break;
        case HTTP_STATUS_PROXY_AUTH_REQ:
            dwFlags = HTTP_QUERY_PROXY_AUTHENTICATE;
            break;            
        default:
            return FALSE;
    }

    // Enumerate the authentication types.
    BOOL fRet;
    char szScheme[64];
    DWORD dwIndex = 0;
    do
    {
        DWORD cbScheme = sizeof(szScheme);
        fRet = HttpQueryInfo
            (hRequest, dwFlags, szScheme, &cbScheme, &dwIndex);
        if (fRet)
            fprintf (stderr, "Found auth scheme: %s\n", szScheme);
    }
        while (fRet);

    return TRUE;
}


//==============================================================================
DWORD DoCustomUI (HINTERNET hRequest, BOOL fProxy)
{
    // Prompt for username and password.
    char  szUser[64], szPass[64];
    fprintf (stderr, "Enter Username: ");
    if (!fscanf (stdin, "%s", szUser))
        return ERROR_INTERNET_LOGIN_FAILURE;
    fprintf (stderr, "Enter Password: ");
    if (!fscanf (stdin, "%s", szPass))
        return ERROR_INTERNET_LOGIN_FAILURE;

    // Set the values in the handle.
    if (fProxy)
    {
        InternetSetOption
            (hRequest, INTERNET_OPTION_PROXY_USERNAME, szUser, sizeof(szUser));
        InternetSetOption
            (hRequest, INTERNET_OPTION_PROXY_PASSWORD, szPass, sizeof(szPass));
    }
    else
    {
        InternetSetOption
            (hRequest, INTERNET_OPTION_USERNAME, szUser, sizeof(szUser));
        InternetSetOption
            (hRequest, INTERNET_OPTION_PASSWORD, szPass, sizeof(szPass));
    }
    
    return ERROR_INTERNET_FORCE_RETRY;
}


//==============================================================================
int RequestLoop (int argc, char **argv)
{
    HINTERNET hInternet = NULL;
    HINTERNET hConnect  = NULL;
    HINTERNET hRequest  = NULL;
    PSTR pPostData = NULL;
    DWORD cbPostData = 0;

    PSTR pszErr = NULL;
    BOOL fRet;
    
#define CHECK_ERROR(cond, err) if (!(cond)) {pszErr=(err); goto done;}
    
    PSTR pszHost     = argv[0];
    PSTR pszObject   = argc >= 2 ? argv[1] : "/";
    PSTR pszUser     = argc >= 3 ? argv[2] : NULL;
    PSTR pszPass     = argc >= 4 ? argv[3] : NULL;
    PSTR pszPostFile = argc >= 5 ? argv[4] : NULL;

    if (pszPostFile)
        g_dwConnectFlags |= INTERNET_FLAG_RELOAD;


//#ifdef MONOLITHIC_UPLOAD

   if(g_fMonolithicUpload) 
   {

    // Read any POST data into a buffer.
    if (pszPostFile)
    {
        HANDLE hf =
            CreateFile
            (
                pszPostFile,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                NULL
            );
        if (hf != INVALID_HANDLE_VALUE)
        {
            cbPostData = GetFileSize (hf, NULL);
            pPostData = (PSTR) LocalAlloc (LMEM_FIXED, cbPostData + 1);
            if (pPostData)
                ReadFile (hf, pPostData, cbPostData, &cbPostData, NULL);
            pPostData[cbPostData] = 0;
            CloseHandle (hf);
        }
    }
  }  // g_fMonolithicUpload
//#endif

    // Initialize wininet.
    hInternet = InternetOpen
    (
        //"HttpAuth Sample",            // user agent
         "Mozilla/4.0 (compatible; MSIE 4.0b2; Windows 95",
        INTERNET_OPEN_TYPE_PRECONFIG, // access type
        NULL,                         // proxy server
        0,                            // proxy port
        0                             // flags
    );
    CHECK_ERROR (hInternet, "InternetOpen");


    // Connect to host.
    hConnect = InternetConnect
    (
        hInternet,                    // wininet handle,
        pszHost,                      // host
        g_dwPort,                            // port
        pszUser,                      // user
        NULL,                         // password
        INTERNET_SERVICE_HTTP,        // service
        g_dwConnectFlags,             // flags
        0                             // context
    );
    CHECK_ERROR (hConnect, "InternetConnect");

    // Use SetOption to set the password since it handles empty strings.
    if (pszPass)
    {
        InternetSetOption
            (hConnect, INTERNET_OPTION_PASSWORD, pszPass, lstrlen(pszPass)+1);
    }

    if(!g_szVerb) {
	if(pszPostFile) {
		g_szVerb = "PUT";
	} else {
		g_szVerb = "GET";
	}
    }

    // Create request.
    hRequest = HttpOpenRequest
    (
        hConnect,                     // connect handle
        g_szVerb, // pszPostFile? "PUT" : "GET",  // request method
        pszObject,                    // object name
        NULL,                         // version
        NULL,                         // referer
        NULL,                         // accept types
        g_dwConnectFlags              // flags
            | INTERNET_FLAG_KEEP_CONNECTION
            | SECURITY_INTERNET_MASK, // ignore SSL warnings
        0                             // context
    );
    CHECK_ERROR (hRequest, "HttpOpenRequest");
    
resend:

//    if (!pszPostFile || pPostData)
     if(g_fMonolithicUpload)
    {
        // Send request.
        fRet = HttpSendRequest
        (
            hRequest,                     // request handle
            "",                           // header string
            0,                            // header length
            pPostData,                    // post data
            cbPostData                    // post length
        );
    }
    else
    {
        HANDLE hf = CreateFile
            (
                pszPostFile,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                NULL
            );
        if (hf != INVALID_HANDLE_VALUE)
        {
            cbPostData = GetFileSize (hf, NULL);

            INTERNET_BUFFERS BufferIn;
            BufferIn.dwStructSize = sizeof( INTERNET_BUFFERSA );
            BufferIn.Next = NULL; 
            BufferIn.lpcszHeader = NULL;
            BufferIn.dwHeadersLength = 0;
            BufferIn.dwHeadersTotal = 0;
            BufferIn.lpvBuffer = NULL;
            BufferIn.dwBufferLength = 0;
            BufferIn.dwBufferTotal = cbPostData;
            BufferIn.dwOffsetLow = 0;
            BufferIn.dwOffsetHigh = 0;

            fRet = HttpSendRequestEx (hRequest, &BufferIn, NULL, 0, 0);
            CHECK_ERROR (fRet, "HttpSendRequestEx");

            while (1)
            {
                CHAR szTemp[512];
                DWORD cbRead;
                fRet = ReadFile (hf, szTemp, sizeof(szTemp), &cbRead, 0);
                CHECK_ERROR (fRet, "ReadFile");

                if (!fRet || !cbRead)
                    break;

                DWORD cbRead2;
                fRet = InternetWriteFile (hRequest, szTemp, cbRead, &cbRead2);
                CHECK_ERROR (fRet, "InternetWriteFile");
            }

            CloseHandle (hf);

            fRet = HttpEndRequest (hRequest, NULL, 0, 0);
            if (!fRet && GetLastError() == ERROR_INTERNET_FORCE_RETRY)
                goto resend;
        }
    }


    DWORD dwStatus;
   
    // Check if the status code is 401 or 407
    if (NeedAuth (hRequest, &dwStatus) && (g_fAllowCustomUI))
    {
        // Prompt for username and password.
        if (DoCustomUI (hRequest, dwStatus != HTTP_STATUS_DENIED))
            goto resend;
    }
    else
    {
        DWORD dwSendErr = fRet? ERROR_SUCCESS : GetLastError();

        DWORD dwDlgErr = InternetErrorDlg(
                GetDesktopWindow(),
                hRequest,  
                dwSendErr,
                FLAGS_ERROR_UI_FILTER_FOR_ERRORS  |     
                    FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS |
                    FLAGS_ERROR_UI_FLAGS_GENERATE_DATA, 
                NULL
            );
        switch (dwSendErr)
        {
            case ERROR_SUCCESS:
            case ERROR_INTERNET_NAME_NOT_RESOLVED:
            case ERROR_INTERNET_CANNOT_CONNECT:
                if (dwDlgErr == ERROR_INTERNET_FORCE_RETRY)
                    goto resend;
                else
                    break;

            case ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION:
            default:
                if (dwDlgErr == ERROR_SUCCESS)
                    goto resend;
                else
                    break;
        }
    }

    // Dump some bytes.
    BYTE bBuf[1024];
    DWORD cbBuf;
    DWORD cbRead;
    cbBuf = sizeof(bBuf);
    _setmode( _fileno( stdout ), _O_BINARY );
    while (InternetReadFile (hRequest, bBuf, cbBuf, &cbRead) && cbRead)
        fwrite (bBuf, 1, cbRead, stdout);
    
done: // Clean up.

    if (pszErr)
        fprintf (stderr, "Failed on %s, last error %d\n", pszErr, GetLastError());
    if (hRequest)
        InternetCloseHandle (hRequest);
    if (hConnect)
        InternetCloseHandle (hConnect);
    if (hInternet)
        InternetCloseHandle (hInternet);
    if (pPostData)
        LocalFree (pPostData);
    return 0;
}

//==============================================================================
void ParseArguments 
(
    LPSTR  InBuffer,
    LPSTR* CArgv,
    DWORD* CArgc
)
{
    LPSTR CurrentPtr = InBuffer;
    DWORD i = 0;
    DWORD Cnt = 0;

    for ( ;; ) {

        //
        // skip blanks.
        //

        while( *CurrentPtr == ' ' ) {
            CurrentPtr++;
        }

        if( *CurrentPtr == '\0' ) {
            break;
        }

        CArgv[i++] = CurrentPtr;

        //
        // go to next space.
        //

        while(  (*CurrentPtr != '\0') &&
                (*CurrentPtr != '\n') ) {
            if( *CurrentPtr == '"' ) {      // Deal with simple quoted args
                if( Cnt == 0 )
                    CArgv[i-1] = ++CurrentPtr;  // Set arg to after quote
                else
                    *CurrentPtr = '\0';     // Remove end quote
                Cnt = !Cnt;
            }
            if( (Cnt == 0) && (*CurrentPtr == ' ') ||   // If we hit a space and no quotes yet we are done with this arg
                (*CurrentPtr == '\0') )
                break;
            CurrentPtr++;
        }

        if( *CurrentPtr == '\0' ) {
            break;
        }

        *CurrentPtr++ = '\0';
    }

    *CArgc = i;
    return;
}

//==============================================================================
int __cdecl main (int argc, char **argv)
{
	g_fAllowCustomUI = 0; //FALSE;
    g_dwConnectFlags = 0;
	HMODULE hmodShlwapi = NULL;
	char * port    ;
	// Discard program arg.
    argv++;
    argc--;

    // Parse options.
    while (argc && argv[0][0] == '-')
    {
        switch (tolower(argv[0][1]))
        {
            case 'c':
                g_fAllowCustomUI = TRUE;
                break;
                
            case 's':
                g_dwConnectFlags = INTERNET_FLAG_SECURE;
                break;
                
            case 'p':
                g_fPreload = TRUE;
                break;
                
            case 'o':
		port = *argv;
		port +=2;
		fprintf(stderr,"Port: %s\n", port);
                if(port) {
			g_dwPort = atol(port);
		}
                break;
                
            case 'v':
		port = *argv;
		port +=2;
		fprintf(stderr,"Verb: %s\n", port);
                if(port) {
			g_szVerb = port;
		}
                break;
                
            case 'm':
                g_fMonolithicUpload = TRUE;
                break;
                
            default:
                fprintf (stderr, "\nUsage: httpauth [-c] [-s] <server> [<object> [<user> [<pass> [<POST-file>]]]]");
                fprintf (stderr, "\n  -c: Custom UI to prompt for user/pass");
                fprintf (stderr, "\n  -s: Secure connection (ssl or pct)");
                fprintf (stderr, "\n  -m: Monolithic upload");
                fprintf (stderr, "\n  -o<port#> : Port Number");
                exit (1);
        }
        
        argv++;
        argc--;
    }

	if (g_fPreload)
	{
	//Get the current directory in case the user selects -p to preload shlwapi from current dir 
		char buf[256];
		GetCurrentDirectory((DWORD)256, buf); 
		strcat(buf,"\\shlwapi.dll");
		fprintf(stderr, "\nPreloading shlwapi.dll from %s", buf);
		if (!(hmodShlwapi=LoadLibrary(buf))) fprintf(stderr, "\nPreload of shlwapi.dll failed");
	}

    if (argc)
        RequestLoop (argc, argv);
        
    else // Enter command prompt loop
    {
        fprintf (stderr, "\nUsage: <server> [<object> [<user> [<pass> [<POST-file>]]]]");
        fprintf (stderr, "\n  quit - exit the command loop");
        fprintf (stderr, "\n  flush - flush pwd cache and authenticated sockets");
        
        while (1)
        {
            char szIn[1024];
            DWORD argcIn;
            LPSTR argvIn[10];

            fprintf (stderr, "\nhttpauth> ");
            gets (szIn);

            if (!lstrcmpi (szIn, "quit"))
                break;
            else if (!lstrcmpi (szIn, "flush"))
            {
                InternetSetOption
                    (NULL, INTERNET_OPTION_END_BROWSER_SESSION, NULL, 0);
                continue;
            }
            
            argcIn = 0;
            ParseArguments (szIn, argvIn, &argcIn);
            if (!argcIn)
                break;
            RequestLoop (argcIn, argvIn);
        }                
    }

	//unload shlwapi if loaded
	if (hmodShlwapi) FreeLibrary(hmodShlwapi);
}        

