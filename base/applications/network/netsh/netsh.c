/*
 * PROJECT:    ReactOS NetSh
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    Network Shell main file
 * COPYRIGHT:  Copyright 2023 Eric Kohl <eric.kohl@reactos.org>
               Copyright 2025 Curtis Wilson <LiquidFox1776@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>


/* Globals *********************************************************************/

static BOOL gbOnline = TRUE; 

/* FUNCTIONS ******************************************************************/


/*
bOnline TRUE to turn online
bOnline FALSE to turn offline
*/
void
SetMode(
    BOOL bOnline)
{
    gbOnline = bOnline;
}

BOOL
GetMode(VOID)
{
    return gbOnline;
}

BOOL
RunScript(
    _In_ LPCWSTR filename)
{
    DPRINT("%s\n", __FUNCTION__);
    FILE *script;
    WCHAR tmp_string[MAX_STRING_SIZE];

    /* Open the file for processing */
    script = _wfopen(filename, L"r");
    if (script == NULL)
    {
        ConResPrintf(StdErr, IDS_OPEN_FAILED, filename);
        return FALSE;
    }

    /* Read and process the script */
    while (fgetws(tmp_string, MAX_STRING_SIZE, script) != NULL)
    {
        if (InterpretScript(tmp_string) == FALSE)
        {
            fclose(script);
            return FALSE;
        }
    }

    /* Close the file */
    fclose(script);

    return TRUE;
}

/*
 * wmain():
 * Main entry point of the application.
 */
int
wmain(
    _In_ int argc,
    _In_ const LPWSTR argv[])
{
    DPRINT("%s\n", __FUNCTION__);
    LPCWSTR tmpBuffer = NULL;
    LPCWSTR pszFileName = NULL;
    int index;
    int result = EXIT_SUCCESS;

    DPRINT("main()\n");

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    /* FIXME: Init code goes here */
    CreateRootContext();
    LoadHelpers();

    if (argc < 2)
    {
        /* If there are no command arguments, then go straight to the interpreter */
        InterpretInteractive();
    }
    else
    {
        /* If there are command arguments, then process them */
        for (index = 1; index < argc; index++)
        {
            if ((argv[index][0] == '/')||
                (argv[index][0] == '-'))
            {
                tmpBuffer = argv[index] + 1;
            }
            else
            {
                if (pszFileName != NULL)
                {
                    ConResPuts(StdOut, IDS_APP_USAGE);
                    result = EXIT_FAILURE;
                    goto done;
                }

                /* Run a command from the command line */
                if (InterpretCommand((LPWSTR*)&argv[index], argc - index) == FALSE)
                    result = EXIT_FAILURE;
                goto done;
            }

            if (_wcsicmp(tmpBuffer, L"?") == 0)
            {
                /* Help option */
                ConResPuts(StdOut, IDS_APP_USAGE);
                result = EXIT_SUCCESS;
                goto done;
            }
            else if (_wcsicmp(tmpBuffer, L"a") == 0)
            {
                /* Aliasfile option */
                if ((index + 1) < argc)
                {
                    index++;
                    ConPuts(StdOut, L"\nThe -a option is not implemented yet\n");
//                    aliasfile = argv[index];
                }
                else
                {
                    ConResPuts(StdOut, IDS_APP_USAGE);
                    result = EXIT_FAILURE;
                }
            }
            else if (_wcsicmp(tmpBuffer, L"c") == 0)
            {
                /* Context option */
                if ((index + 1) < argc)
                {
                    index++;
                    ConPuts(StdOut, L"\nThe -c option is not implemented yet\n");
//                    context = argv[index];
                }
                else
                {
                    ConResPuts(StdOut, IDS_APP_USAGE);
                    result = EXIT_FAILURE;
                }
            }
            else if (_wcsicmp(tmpBuffer, L"f") == 0)
            {
                /* File option */
                if ((index + 1) < argc)
                {
                    index++;
                    pszFileName = argv[index];
                }
                else
                {
                    ConResPuts(StdOut, IDS_APP_USAGE);
                    result = EXIT_FAILURE;
                }
            }
            else if (_wcsicmp(tmpBuffer, L"r") == 0)
            {
                /* Remote option */
                /*
                LogonUserW(lpszUsername, lpszDomain, lpszPassword ,LOGON32_LOGON_NEW_CREDENTIALS, LOGON32_PROVIDER_DEFAULT, phToken);
                ImpersonateLoggedOnUser(phToken);
                RevertToSelf();
                */
                if ((index + 1) < argc)
                {
                    index++;
                    ConPuts(StdOut, L"\nThe -r option is not implemented yet\n");
//                    remote = argv[index];
                }
                else
                {
                    ConResPuts(StdOut, IDS_APP_USAGE);
                    result = EXIT_FAILURE;
                }
            }
            else
            {
                /* Invalid command */
                ConResPrintf(StdOut, IDS_INVALID_COMMAND, argv[index]);
                result = EXIT_FAILURE;
                goto done;
            }
        }

        /* Now we process the filename if it exists */
        if (pszFileName != NULL)
        {
            if (RunScript(pszFileName) == FALSE)
            {
                result = EXIT_FAILURE;
                goto done;
            }
        }
    }

done:
    /* FIXME: Cleanup code goes here */
    UnloadHelpers();

    return result;
}


DWORD
WINAPI
MatchEnumTag(
    _In_ HANDLE hModule,
    _In_ LPCWSTR pwcArg,
    _In_ DWORD dwNumArg,
    _In_ const TOKEN_VALUE *pEnumTable,
    _Out_ PDWORD pdwValue)
{
    DPRINT("MatchEnumTag()\n");
    return 0;
}

/*
The MatchToken function determines whether a user-entered string matches a specific string. 
A match exists if the user-entered string is a case-insensitive prefix of the specific string.
*/
BOOL
WINAPI
MatchToken(
    _In_ LPCWSTR pwszUserToken,
    _In_ LPCWSTR pwszCmdToken) 
{

    // Check for NULL pointers
    if (pwszUserToken == NULL || pwszCmdToken == NULL) 
    {
        DPRINT1("%s string pointers are NULL!\n", __FUNCTION__);
        return FALSE;
    }
    
    DPRINT("MatchToken %S %S\n", pwszUserToken, pwszCmdToken);
    
    // Get the lengths of lpszStr and lpszSubstr
    size_t nUserTokenLength = wcslen(pwszUserToken);
    size_t nCmdTokenLength = wcslen(pwszCmdToken);
    
    if (nUserTokenLength > nCmdTokenLength) 
    {
        DPRINT1("%s nUserTokenLength > nCmdTokenLength\n", __FUNCTION__);
        return FALSE;
    }

    return (_wcsnicmp(pwszCmdToken, pwszUserToken, nUserTokenLength) == 0) ? TRUE : FALSE;
}

DWORD
CDECL
PrintError(
    _In_opt_ HANDLE hModule,
    _In_ DWORD dwErrId,
    ...)
{
    WCHAR *lpBuffer = NULL;
    INT Length;
    
    // if hModule is not null print a string from a module
    if (hModule != NULL)
    {
        va_list ap;
        va_start(ap, dwErrId);
        Length = PrintMessageFromModule(hModule, dwErrId, ap);
        va_end(ap);   
    }
    // if hModule is NULL then we need to treat dwErrId as an error code
    // found in the system-message table
    else
    {
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                      NULL,
                      dwErrId,
                      0,
                      (LPWSTR)&lpBuffer,
                      0,
                      NULL);
        // print the message
        va_list ap;
        va_start(ap, dwErrId);
        Length = ConPrintf(StdOut, lpBuffer);
        va_end(ap);
        // When FORMAT_MESSAGE_ALLOCATE_BUFFER is used with FormatMessage
        // its up to the caller to free it with LocalFree
        LocalFree(lpBuffer);
    }
    return Length;
    
}

DWORD
CDECL
PrintMessageFromModule(
    _In_ HANDLE hModule,
    _In_ DWORD  dwMsgId,
    ...)
{
    DPRINT1("%s()\n", __FUNCTION__);
    WCHAR *lpBuffer = NULL;
    INT Length = 0;
    int nBytesRead = 0;
    
    if (hModule == NULL)
        goto end;
    
    // LoadStringW when given a size of 0 returns a read only pointer
    // to a resource string    
    nBytesRead = LoadStringW(hModule, dwMsgId, (LPWSTR)&lpBuffer, 0);
 
    if ( lpBuffer != NULL)
    {

        // check if the resource string is NULL terminated
        if(lpBuffer[nBytesRead] == UNICODE_NULL)
        {  
            va_list ap;
            va_start(ap, dwMsgId);
            Length = ConPrintfV(StdOut, lpBuffer, ap);
            va_end(ap);
        }
        // if its not null terminated, create a null terminated copy of the string to print
        else
        {
            // Allocate a string buffer of size nBytesREad + 1 because LoadStringW
            // does not count the NULL character
            WCHAR *lpBuffer2 = (WCHAR *)HeapAlloc(GetProcessHeap(), 
                                                  HEAP_ZERO_MEMORY, 
                                                  (nBytesRead + 1) * sizeof(WCHAR));
            if (lpBuffer2 == NULL)
            {
                DPRINT("%s space for lpBuffer2 could not be allocated\n", __FUNCTION__); 
                SetLastError(ERROR_OUTOFMEMORY);
                Length = 0;
                goto end;
            }
            
            // copy the lpBuffer to lpBuffer2
            StringCchCopyW(lpBuffer2, nBytesRead + 1, lpBuffer);

            va_list ap;
            va_start(ap, dwMsgId);
            Length = ConPrintfV(StdOut, lpBuffer2, ap);
            va_end(ap);
            HeapFree(GetProcessHeap(), 0, lpBuffer2);
        }    
    }

    end:
    return Length;
}

DWORD
CDECL
PrintMessage(
    _In_ LPCWSTR pwszFormat,
    ...)
{
    INT Length;
    va_list ap;

    va_start(ap, pwszFormat);
    Length = ConPrintfV(StdOut, pwszFormat, ap);
    va_end(ap);

    return Length;
}
