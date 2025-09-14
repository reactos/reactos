/*
 * PROJECT:    ReactOS NetSh
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    Network Shell command interpreter
 * COPYRIGHT:  Copyright 2023 Eric Kohl <eric.kohl@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

BOOL
ProcessCommand(
    _In_ PCONTEXT_ENTRY pContext,
    _In_ DWORD dwArgCount,
    _In_ LPWSTR *argv,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwHelpLevel,
    _Inout_ PBOOL bDone)
{
    PCONTEXT_ENTRY pSubContext;
    PCOMMAND_ENTRY pCommand;
    PCOMMAND_GROUP pGroup;
    DWORD dwError = ERROR_SUCCESS;

    /* If no args provided */
    if (dwArgCount == 0)
        return TRUE;

    if (pCurrentContext == NULL)
    {
        DPRINT1("InterpretCmd: invalid context %p\n", pCurrentContext);
        return FALSE;
    }

    if ((_wcsicmp(argv[dwArgCount - 1], L"?") == 0) ||
        (_wcsicmp(argv[dwArgCount - 1], L"help") == 0))
    {
        dwHelpLevel = dwArgCount - 1;
    }

    pContext = pCurrentContext;

    while (TRUE)
    {
        pCommand = pContext->pCommandListHead;
        while (pCommand != NULL)
        {
            if (_wcsicmp(argv[dwCurrentIndex], pCommand->pwszCmdToken) == 0)
            {
                dwError = pCommand->pfnCmdHandler(NULL, argv, dwCurrentIndex + 1, dwArgCount, 0, NULL, bDone);
                if (dwError != ERROR_SUCCESS)
                {
                    ConPrintf(StdOut, L"Error: %lu\n\n", dwError);
                    ConResPrintf(StdOut, pCommand->dwCmdHlpToken);
                }
                return TRUE;
            }

            pCommand = pCommand->pNext;
        }

        pGroup = pContext->pGroupListHead;
        while (pGroup != NULL)
        {
            if (_wcsicmp(argv[dwCurrentIndex], pGroup->pwszCmdGroupToken) == 0)
            {
                if (dwArgCount == 1)
                {
                    ProcessHelp(pContext, dwArgCount, argv, dwCurrentIndex, dwHelpLevel + 1);
                    return TRUE;
                }
                else
                {
                    pCommand = pGroup->pCommandListHead;
                    while (pCommand != NULL)
                    {
                        if ((dwArgCount > 1) && (_wcsicmp(argv[dwCurrentIndex + 1], pCommand->pwszCmdToken) == 0))
                        {
                            dwError = pCommand->pfnCmdHandler(NULL, argv, dwCurrentIndex + 2, dwArgCount, 0, NULL, bDone);
                            if (dwError != ERROR_SUCCESS)
                            {
                                ConPrintf(StdOut, L"Error: %lu\n\n", dwError);
                                ConResPrintf(StdOut, pCommand->dwCmdHlpToken);
                            }
                            return TRUE;
                        }

                        pCommand = pCommand->pNext;
                    }

                    return FALSE;
                }
            }

            pGroup = pGroup->pNext;
        }

        if (pContext == pCurrentContext)
        {
            pSubContext = pContext->pSubContextHead;
            while (pSubContext != NULL)
            {
                if (_wcsicmp(argv[0], pSubContext->pszContextName) == 0)
                {
                    DPRINT("%S ==> dwCurrentIndex: %lu  dwArgCount: %lu\n", argv[dwCurrentIndex], dwCurrentIndex, dwArgCount);
                    if (dwArgCount == dwCurrentIndex + 1)
                    {
                        pCurrentContext = pSubContext;
                        return TRUE;
                    }
                    else
                    {
                        return ProcessCommand(pSubContext,
                                              dwArgCount,
                                              argv,
                                              dwCurrentIndex + 1,
                                              dwHelpLevel,
                                              bDone);
                    }
                }

                pSubContext = pSubContext->pNext;
            }
        }

        if (pContext == pRootContext)
            break;

        pContext = pContext->pParentContext;
    }

    return FALSE;
}


BOOL
InterpretCommand(
    _In_ LPWSTR *argv,
    _In_ DWORD dwArgCount,
    _Inout_ PBOOL bDone)
{
    /* If no args provided */
    if (dwArgCount == 0)
        return TRUE;

    if (pCurrentContext == NULL)
    {
        DPRINT("InterpretCmd: invalid context %p\n", pCurrentContext);
        return FALSE;
    }

    if ((_wcsicmp(argv[dwArgCount - 1], L"?") == 0) ||
        (_wcsicmp(argv[dwArgCount - 1], L"help") == 0))
    {
        return ProcessHelp(pCurrentContext,
                           dwArgCount,
                           argv,
                           0,
                           dwArgCount - 1);
    }
    else
    {
        return ProcessCommand(pCurrentContext,
                              dwArgCount,
                              argv,
                              0,
                              0,
                              bDone);
    }
}


/*
 * InterpretScript(char *line):
 * The main function used for when reading commands from scripts.
 */
BOOL
InterpretScript(
    _In_ LPWSTR pszInputLine)
{
    LPWSTR args_vector[MAX_ARGS_COUNT];
    DWORD dwArgCount = 0;
    BOOL bWhiteSpace = TRUE;
    BOOL bDone = FALSE;
    LPWSTR ptr;

    memset(args_vector, 0, sizeof(args_vector));

    ptr = pszInputLine;
    while (*ptr != 0)
    {
        if (iswspace(*ptr) || *ptr == L'\n')
        {
            *ptr = 0;
            bWhiteSpace = TRUE;
        }
        else
        {
            if ((bWhiteSpace != FALSE) && (dwArgCount < MAX_ARGS_COUNT))
            {
                args_vector[dwArgCount] = ptr;
                dwArgCount++;
            }

            bWhiteSpace = FALSE;
        }

        ptr++;
    }

    /* sends the string to find the command */
    return InterpretCommand(args_vector, dwArgCount, &bDone) == ERROR_SUCCESS;
}


VOID
PrintPrompt(
    PCONTEXT_ENTRY pContext)
{
    if (pContext != pRootContext)
    {
        PrintPrompt(pContext->pParentContext);
        ConPuts(StdOut, L" ");
    }

    ConPuts(StdOut, pContext->pszContextName);
}


VOID
InterpretInteractive(VOID)
{
    WCHAR input_line[MAX_STRING_SIZE];
    LPWSTR args_vector[MAX_ARGS_COUNT];
    DWORD dwArgCount = 0;
    BOOL bWhiteSpace = TRUE;
    BOOL bDone = FALSE;
    LPWSTR ptr;

    for (;;)
    {
        dwArgCount = 0;
        memset(args_vector, 0, sizeof(args_vector));

        /* Shown just before the input where the user places commands */
        PrintPrompt(pCurrentContext);
        ConPuts(StdOut, L">");

        /* Get input from the user. */
        fgetws(input_line, MAX_STRING_SIZE, stdin);

        ptr = input_line;
        while (*ptr != 0)
        {
            if (iswspace(*ptr) || *ptr == L'\n')
            {
                *ptr = 0;
                bWhiteSpace = TRUE;
            }
            else
            {
                if ((bWhiteSpace != FALSE) && (dwArgCount < MAX_ARGS_COUNT))
                {
                    args_vector[dwArgCount] = ptr;
                    dwArgCount++;
                }
                bWhiteSpace = FALSE;
            }
            ptr++;
        }

        /* Send the string to find the command */
        if (InterpretCommand(args_vector, dwArgCount, &bDone) == FALSE)
        {
            ConResPrintf(StdErr, IDS_INVALID_COMMAND, input_line);
        }

        if (bDone)
            break;
    }
}
