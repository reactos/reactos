/*
 * PROJECT:    ReactOS NetSh
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    Network Shell main file
 * COPYRIGHT:  Copyright 2023 Eric Kohl <eric.kohl@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

BOOL
RunScript(
    _In_ LPCWSTR filename)
{
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
    DPRINT1("MatchEnumTag()\n");
    return 0;
}

BOOL
WINAPI
MatchToken(
    _In_ LPCWSTR pwszUserToken,
    _In_ LPCWSTR pwszCmdToken)
{
    DPRINT1("MatchToken %S %S\n", pwszUserToken, pwszCmdToken);
    return (_wcsicmp(pwszUserToken, pwszCmdToken) == 0) ? TRUE : FALSE;
}

DWORD
CDECL
PrintError(
    _In_opt_ HANDLE hModule,
    _In_ DWORD dwErrId,
    ...)
{
    DPRINT1("PrintError()\n");
    return 1;
}

DWORD
CDECL
PrintMessageFromModule(
    _In_ HANDLE hModule,
    _In_ DWORD  dwMsgId,
    ...)
{
    DPRINT1("PrintMessageFromModule()\n");
    return 1;
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
    Length = ConPrintf(StdOut, pwszFormat);
    va_end(ap);

    return Length;
}
