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

typedef enum _INTERPRETER_STATE
{
    STATE_ANALYZE,
    STATE_COMMAND,
    STATE_GROUP,
    STATE_CONTEXT,
    STATE_PARENT_CONTEXT,
    STATE_DONE
} INTERPRETER_STATE;


/* FUNCTIONS *****************************************************************/

static
PCOMMAND_ENTRY
GetGroupCommand(
    PWSTR pszCommand,
    PCOMMAND_GROUP pGroup)
{
    PCOMMAND_ENTRY pCommand;

    DPRINT("GetGroupCommand(%S %p)\n", pszCommand, pGroup);

    if (pszCommand == NULL)
        return NULL;

    pCommand = pGroup->pCommandListHead;
    while (pCommand)
    {
        if (MatchToken(pszCommand, pCommand->pwszCmdToken))
            return pCommand;

        pCommand = pCommand->pNext;
    }

    return NULL;
}


static
PCOMMAND_ENTRY
GetContextCommand(
    PWSTR pszCommand,
    PCONTEXT_ENTRY pContext)
{
    PCOMMAND_ENTRY pCommand;

    DPRINT("GetContextCommand(%S %p)\n", pszCommand, pContext);

    if (pszCommand == NULL)
        return NULL;

    pCommand = pContext->pCommandListHead;
    while (pCommand)
    {
        if (MatchToken(pszCommand, pCommand->pwszCmdToken))
            return pCommand;

        pCommand = pCommand->pNext;
    }

    return NULL;
}


static
PCOMMAND_GROUP
GetContextGroup(
    PWSTR pszGroup,
    PCONTEXT_ENTRY pContext)
{
    PCOMMAND_GROUP pGroup;

    DPRINT("GetContextGroup(%S %p)\n", pszGroup, pContext);

    if (pszGroup == NULL)
        return NULL;

    pGroup = pContext->pGroupListHead;
    while (pGroup)
    {
        if (MatchToken(pszGroup, pGroup->pwszCmdGroupToken))
            return pGroup;

        pGroup = pGroup->pNext;
    }

    return NULL;
}


static
PCONTEXT_ENTRY
GetContextSubContext(
    PWSTR pszContext,
    PCONTEXT_ENTRY pContext)
{
    PCONTEXT_ENTRY pSubContext;

    DPRINT("GetContextSubContext(%S %p)\n", pszContext, pContext);

    if (pszContext == NULL)
        return NULL;

    pSubContext = pContext->pSubContextHead;
    while (pSubContext)
    {
        if (MatchToken(pszContext, pSubContext->pszContextName))
            return pSubContext;

        pSubContext = pSubContext->pNext;
    }

    return NULL;
}


static
DWORD
InterpretCommand(
    _In_ LPWSTR *argv,
    _In_ DWORD dwArgCount,
    _Inout_ PBOOL bDone)
{
    PCONTEXT_ENTRY pTempContext, pTempSubContext = NULL;
    PCOMMAND_GROUP pGroup = NULL;
    PCOMMAND_ENTRY pCommand = NULL;
    INTERPRETER_STATE State = STATE_ANALYZE;
    DWORD dwArgIndex = 0;
    DWORD dwError = ERROR_SUCCESS;

    /* If no args provided */
    if (dwArgCount == 0)
        return ERROR_SUCCESS;

    if (pCurrentContext == NULL)
        pCurrentContext = pRootContext;

    /* Check for help keywords */
    if ((dwArgCount == 1) &&
        ((_wcsicmp(argv[0], L"?") == 0) || (_wcsicmp(argv[0], L"help") == 0)))
    {
        PrintContextHelp(pCurrentContext);
        return ERROR_SUCCESS;
    }

    pTempContext = pCurrentContext;
    while (dwArgIndex < dwArgCount)
    {
        switch (State)
        {
            case STATE_ANALYZE:
                DPRINT("STATE_ANALYZE\n");

                pCommand = GetContextCommand(argv[dwArgIndex], pTempContext);
                if (pCommand != NULL)
                {
                    State = STATE_COMMAND;
                    break;
                }

                pGroup = GetContextGroup(argv[dwArgIndex], pTempContext);
                if (pGroup != NULL)
                {
                    State = STATE_GROUP;
                    break;
                }

                pTempSubContext = GetContextSubContext(argv[dwArgIndex], pTempContext);
                if (pTempSubContext != NULL)
                {
                    State = STATE_CONTEXT;
                    break;
                }

                State = STATE_PARENT_CONTEXT;
                break;

            case STATE_COMMAND:
                DPRINT("STATE_COMMAND\n");

                /* Check for help keywords */
                if (((dwArgIndex + 1) == (dwArgCount - 1)) &&
                    ((_wcsicmp(argv[dwArgIndex + 1], L"?") == 0) || (_wcsicmp(argv[dwArgIndex + 1], L"help") == 0)))
                {
                    PrintCommandHelp(pTempContext, pGroup, pCommand);
                    State = STATE_DONE;
                    break;
                }

                if (pCommand->pfnCmdHandler != NULL)
                {
                    dwArgIndex++;
                    dwError = pCommand->pfnCmdHandler(pszMachine, argv, dwArgIndex, dwArgCount, 0, NULL, bDone);
                    if (dwError != ERROR_SUCCESS)
                    {
                        if (dwError == ERROR_SHOW_USAGE)
                        {
                            PrintCommandHelp(pTempContext, pGroup, pCommand);
                            dwError = ERROR_SUPPRESS_OUTPUT;
                        }
                    }
                    else
                    {
                        /* Execute the commands following a pushd command */
                        if ((_wcsicmp(pCommand->pwszCmdToken, L"pushd") == 0) &&
                            (dwArgIndex < dwArgCount))
                        {
                            State = STATE_ANALYZE;
                            break;
                        }
                    }
                }

                State = STATE_DONE;
                break;

            case STATE_GROUP:
                DPRINT("STATE_GROUP\n");

                /* Check for group without command */
                if (dwArgIndex == (dwArgCount - 1))
                {
                    PrintGroupHelp(pTempContext, pGroup->pwszCmdGroupToken, TRUE);
                    State = STATE_DONE;
                    break;
                }

                /* Check for help keywords */
                if (((dwArgIndex + 1) <= (dwArgCount - 1)) &&
                    ((_wcsicmp(argv[dwArgIndex + 1], L"?") == 0) || (_wcsicmp(argv[dwArgIndex + 1], L"help") == 0)))
                {
                    PrintGroupHelp(pTempContext, pGroup->pwszCmdGroupToken, TRUE);
                    State = STATE_DONE;
                    break;
                }

                dwArgIndex++;
                pCommand = GetGroupCommand(argv[dwArgIndex], pGroup);
                if (pCommand != NULL)
                {
                    State = STATE_COMMAND;
                    break;
                }

                dwError = ERROR_CMD_NOT_FOUND;
                State = STATE_DONE;
                break;

            case STATE_CONTEXT:
                DPRINT("STATE_CONTEXT\n");

                if (pTempSubContext == pCurrentContext)
                {
                    if (dwArgIndex != (dwArgCount - 1))
                        dwError = ERROR_CMD_NOT_FOUND;

                    State = STATE_DONE;
                    break;
                }

                if (dwArgIndex == (dwArgCount - 1))
                {
                    DPRINT("Set current context\n");
                    pCurrentContext = pTempSubContext;
                    if (pCurrentContext->pfnConnectFn)
                        dwError = pCurrentContext->pfnConnectFn(pszMachine);
                    State = STATE_DONE;
                    break;
                }

                /* Check for help keywords */
                if (((dwArgIndex + 1) <= (dwArgCount - 1)) &&
                    ((_wcsicmp(argv[dwArgIndex + 1], L"?") == 0) || (_wcsicmp(argv[dwArgIndex + 1], L"help") == 0)))
                {
                    PrintContextHelp(pTempSubContext);
                    State = STATE_DONE;
                    break;
                }

                DPRINT("Change temorary context\n");
                pTempContext = pTempSubContext;
                State = STATE_ANALYZE;
                dwArgIndex++;
                break;

            case STATE_PARENT_CONTEXT:
                DPRINT("STATE_PARENT_CONTEXT\n");

                if (pTempContext->pParentContext == NULL)
                {
                    dwError = ERROR_CMD_NOT_FOUND;
                    State = STATE_DONE;
                    break;
                }

                DPRINT("Change temorary context\n");
                pTempContext = pTempContext->pParentContext;
                State = STATE_ANALYZE;
                dwArgIndex = 0;
                break;

            case STATE_DONE:
                DPRINT("STATE_DONE dwError %ld\n", dwError);
                return dwError;
        }
    }

    /* Done */
    return ERROR_SUCCESS;
} 


/*
 * InterpretScript(char *line):
 * The main function used for when reading commands from scripts.
 */
DWORD
InterpretLine(
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

    return InterpretCommand(args_vector, dwArgCount, &bDone);
}


VOID
PrintPrompt(
    _In_ PCONTEXT_ENTRY pContext)
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
    DWORD dwError = ERROR_SUCCESS;

    for (;;)
    {
        dwArgCount = 0;
        memset(args_vector, 0, sizeof(args_vector));

        /* Shown just before the input where the user places commands */
        if (pszMachine)
            ConPrintf(StdOut, L"[%s] ", pszMachine);
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

        dwError = InterpretCommand(args_vector, dwArgCount, &bDone);
        if ((dwError != ERROR_SUCCESS) && (dwError != ERROR_SUPPRESS_OUTPUT))
        {
            PWSTR pszCommandString = MergeStrings(args_vector, dwArgCount);
            PrintError(NULL, dwError, pszCommandString);
            HeapFree(GetProcessHeap(), 0, pszCommandString);
        }
        if (dwError != ERROR_SUPPRESS_OUTPUT)
            ConPuts(StdOut, L"\n");

        if (bDone)
            break;
    }
}
