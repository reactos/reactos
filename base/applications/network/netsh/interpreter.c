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
InterpretCommand(
    _In_ LPWSTR *argv,
    _In_ DWORD dwArgCount)
{
    PCONTEXT_ENTRY pContext, pSubContext;
    PCOMMAND_ENTRY pCommand;
    PCOMMAND_GROUP pGroup;
    BOOL bDone = FALSE;
    DWORD dwHelpLevel = 0;
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
            if (_wcsicmp(argv[0], pCommand->pwszCmdToken) == 0)
            {
                if (dwHelpLevel == 1)
                {
                    ConResPrintf(StdOut, pCommand->dwCmdHlpToken);
                    return TRUE;
                }
                else
                {
                    dwError = pCommand->pfnCmdHandler(NULL, argv, 0, dwArgCount, 0, NULL, &bDone);
                    if (dwError != ERROR_SUCCESS)
                    {
                        ConPrintf(StdOut, L"Error: %lu\n\n");
                        ConResPrintf(StdOut, pCommand->dwCmdHlpToken);
                    }
                    return !bDone;
                }
            }

            pCommand = pCommand->pNext;
        }

        pGroup = pContext->pGroupListHead;
        while (pGroup != NULL)
        {
            if (_wcsicmp(argv[0], pGroup->pwszCmdGroupToken) == 0)
            {
                if (dwHelpLevel == 1)
                {
                    HelpGroup(pGroup);
                    return TRUE;
                }

                pCommand = pGroup->pCommandListHead;
                while (pCommand != NULL)
                {
                    if ((dwArgCount > 1) && (_wcsicmp(argv[1], pCommand->pwszCmdToken) == 0))
                    {
                        if (dwHelpLevel == 2)
                        {
                            ConResPrintf(StdOut, pCommand->dwCmdHlpToken);
                            return TRUE;
                        }
                        else
                        {
                            dwError = pCommand->pfnCmdHandler(NULL, argv, 1, dwArgCount, 0, NULL, &bDone);
                            if (dwError != ERROR_SUCCESS)
                            {
                                ConPrintf(StdOut, L"Error: %lu\n\n");
                                ConResPrintf(StdOut, pCommand->dwCmdHlpToken);
                                return TRUE;
                            }
                            return !bDone;
                        }
                    }

                    pCommand = pCommand->pNext;
                }

                HelpGroup(pGroup);
                return TRUE;
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
                    pCurrentContext = pSubContext;
                    return TRUE;
                }

                pSubContext = pSubContext->pNext;
            }
        }

        if (pContext == pRootContext)
            break;

        pContext = pContext->pParentContext;
    }

    ConResPrintf(StdErr, IDS_INVALID_COMMAND, argv[0]);

    return TRUE;
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
    return InterpretCommand(args_vector, dwArgCount);
}


VOID
InterpretInteractive(VOID)
{
    WCHAR input_line[MAX_STRING_SIZE];
    LPWSTR args_vector[MAX_ARGS_COUNT];
    DWORD dwArgCount = 0;
    BOOL bWhiteSpace = TRUE;
    BOOL bRun = TRUE;
    LPWSTR ptr;

    while (bRun != FALSE)
    {
        dwArgCount = 0;
        memset(args_vector, 0, sizeof(args_vector));

        /* Shown just before the input where the user places commands */
//        ConResPuts(StdOut, IDS_APP_PROMPT);
        ConPuts(StdOut, L"netsh");
        if (pCurrentContext != pRootContext)
        {
            ConPuts(StdOut, L" ");
            ConPuts(StdOut, pCurrentContext->pszContextName);
        }
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
        bRun = InterpretCommand(args_vector, dwArgCount);
    }
}
