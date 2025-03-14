/*
 * PROJECT:    ReactOS NetSh
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    Network Shell command interpreter
 * COPYRIGHT:  Copyright 2023 Eric Kohl <eric.kohl@reactos.org>
               Copyright 2025 Curtis Wilson <LiquidFox1776@gmail.com>
 */

/* DEFINES *******************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>

#define HASH_TABLE_INITIAL_SLOT_SIZE 101
#define HASH_TABLE_LOAD_FACTOR 0.50

enum INTERPRET_EVENT
{
    INTERPRET_EVENT_SEARCH_FOR_TOKENS,
    INTERPRET_EVENT_GROUP,
    INTERPRET_EVENT_CONTEXT,
    INTERPRET_EVENT_COMMAND,
    INTERPRET_EVENT_SWITCH_TO_PARENT,
    INTERPRET_EVENT_NOTHING_FOUND,
    INTERPRET_EVENT_FINISHED
};

static PHASH_TABLE g_pHashTable = NULL;


/* FUNCTIONS *****************************************************************/

static
BOOL
CreateAliasTable()
{
    DOUBLE dLoadFactor = HASH_TABLE_LOAD_FACTOR;
    SIZE_T nNumberOfInitalSlots = HASH_TABLE_INITIAL_SLOT_SIZE;
    g_pHashTable = CreateHashTable(nNumberOfInitalSlots, dLoadFactor);
    
    if (g_pHashTable == NULL)
    {
        return FALSE;
    }
    
    return TRUE;
}


static
BOOL
DeleteAliasTable()
{
    if (g_pHashTable != NULL)
        return FreeHashTable(&g_pHashTable, TRUE, TRUE);
    else
        return TRUE;
}


VOID
DisplayAliasEntry(
    PVOID pKey, 
    SIZE_T nKeyLength)
{
    if (g_pHashTable == NULL)
    {
        // todo localize
        ConPrintf(StdOut, L"The following alias was not found: %ls.\n\n", (PWSTR)pKey);
        return;
    }
        
    LPCWSTR pwszValue = (LPCWSTR)g_pHashTable->GetValue(g_pHashTable, pKey, nKeyLength);
    
    if (pwszValue != NULL)
        ConPrintf(StdOut, L"%ls\n\n", pwszValue);
    else
        // todo localize
        ConPrintf(StdOut, L"The following alias was not found: %ls.\n\n", (PWSTR)pKey);
}


VOID
DisplayAliasTable()
{
    if (g_pHashTable == NULL)
    {
        ConPrintf(StdOut, L"\n");
        return;
    }
        
    PHASH_ENTRY pHashEntry = NULL;
    HANDLE hIterator = g_pHashTable->GetFirstEntry(g_pHashTable, &pHashEntry);

    // print all of the entries and their values
    while (pHashEntry)
    {
        ConPrintf(StdOut, L"%ls %ls\n", (wchar_t*)pHashEntry->pKey, (wchar_t *)pHashEntry->pValue);
        g_pHashTable->GetNextEntry(&hIterator, &pHashEntry);
    }
    
    ConPrintf(StdOut, L"\n");
}


BOOL
SetAliasEntry(
    PVOID pKey, 
    SIZE_T nKeyLength, 
    PVOID pValue)
{
    if (g_pHashTable == NULL)
    {
        if (CreateAliasTable() == FALSE)
            return FALSE;
    }
    
    if (g_pHashTable->SetEntry(&g_pHashTable, pKey, nKeyLength, pValue) == FALSE)
    {
        // todo investigate how netsh reports keys that can't be added
        return FALSE;
    }
    
    return TRUE;
}


BOOL
DeleteAliasEntry(
    PVOID pKey, 
    SIZE_T nKeyLength)
{
    if (g_pHashTable == NULL)
    {
        return FALSE;
    }

    // see if the entry was deleted
    if (g_pHashTable->DeleteEntry(g_pHashTable, pKey, nKeyLength, FALSE, FALSE) == FALSE)
    {
        return FALSE;
    }

    // if the table is empty delete it and free memory
    if (g_pHashTable->GetNumberOfEntries(g_pHashTable) == 0)
    {
        DeleteAliasTable();
    }
    
    return TRUE;
}


/*
TODO When WMI is implemented this function will have to be updated to utilize WMI
to supply the arguments for the OsVersionCheck call back function.
*/
BOOL
CallOsVersionCheck(PNS_OSVERSIONCHECK NsOsversioncheck)
{
  UINT CIMOSType = 0;
  UINT CIMOSProductSuite = 0;
  LPCWSTR CIMOSVersion = L"0.0.0";
  LPCWSTR CIMOSBuildNumber = L"00000";
  LPCWSTR CIMServicePackMajorVersion = L"0";
  LPCWSTR CIMServicePackMinorVersion = L"0";
  UINT uiReserved = 0;
  DWORD dwReserved = 0;

DPRINT("%s()\n", __FUNCTION__);
if (NsOsversioncheck != NULL)
{
    return NsOsversioncheck(CIMOSType,
                            CIMOSProductSuite,
                            CIMOSVersion,
                            CIMOSBuildNumber,
                            CIMServicePackMajorVersion,
                            CIMServicePackMinorVersion,
                            uiReserved,
                            dwReserved);
}

return TRUE;                    
}


/*
Searches a linked list for a node that has a member 
named pwszCmdToken for a string that starts with 
pwszCmdToken and returns a pointer to the node if 
found, otherwise return NULL. Ideally pCommandListHead 
should point to the head of a command list.
*/
static
PCOMMAND_ENTRY
_SearchCommand(
    LPCWSTR pwszCmdToken,
    PCOMMAND_ENTRY pCommandListHead)
{
    DPRINT1("%s \n", __FUNCTION__);
    PCOMMAND_ENTRY pCurrentCommand = pCommandListHead;
    
    if (pwszCmdToken == NULL)
        {
            DPRINT1("%s pwszCmdToken == NULL\n", __FUNCTION__);
            return NULL;
        }
    // search through the linked list and try to find a node
    // where the pwszCmdToken string is contained        
    while(pCurrentCommand)
    {
        // if true we have found are matched, so we can exit
        if (MatchToken(pwszCmdToken, pCurrentCommand->pwszCmdToken))
        {
            DPRINT1("%s pCurrentCommand->pwszCmdToken: %ls\n", __FUNCTION__, pCurrentCommand->pwszCmdToken);
            return pCurrentCommand;
        }
            
        // goto the next node in the list    
        pCurrentCommand = pCurrentCommand->pNext;
    }
    
    DPRINT1("%s returning NULL\n", __FUNCTION__);
    return NULL;
}


static
PCOMMAND_GROUP
_SearchGroup(
    LPCWSTR pwszCmdGroupToken,
    PCOMMAND_GROUP pGroupListHead)
{
    DPRINT1("%s \n", __FUNCTION__);
    PCOMMAND_GROUP pCurrentGroup = pGroupListHead;
    
    if (pwszCmdGroupToken == NULL)
        {
            DPRINT1("%s pwszCmdGroupToken == NULL\n", __FUNCTION__);
            return NULL;
        }
            
    while(pCurrentGroup)
    {
        if (MatchToken(pwszCmdGroupToken, pCurrentGroup->pwszCmdGroupToken))
        {
            DPRINT1("%s pCurrentGroup->pwszCmdGroupToken: %ls\n", __FUNCTION__, pCurrentGroup->pwszCmdGroupToken);
            return pCurrentGroup;
        }
        pCurrentGroup = pCurrentGroup->pNext;
    }
    
    DPRINT1("%s returning NULL\n", __FUNCTION__);
    return NULL;
}


static
PCONTEXT_ENTRY
_SearchSubContext(
    PWSTR pszContextName,
    PCONTEXT_ENTRY pContextHead)
{

    if (pszContextName == NULL)
    {
        DPRINT1("%s pszContextName == NULL\n", __FUNCTION__);
        return NULL;
    }
        
    PCONTEXT_ENTRY pCurrentContext = NULL;
    
    if (pContextHead)
    {
        DPRINT1("%s pSubContextHead\n", __FUNCTION__);
        pCurrentContext = pContextHead->pSubContextHead;
    }
    
    while(pCurrentContext)
    {
        if (MatchToken(pszContextName, pCurrentContext->pszContextName))
        {
            DPRINT1("%s pCurrentContext->pszContextName: %ls\n", __FUNCTION__, pCurrentContext->pszContextName);
            return pCurrentContext;
        }
        
        pCurrentContext = pCurrentContext->pNext;
    }
    
    DPRINT1("%s returning NULL\n", __FUNCTION__);
    return NULL;
}


/*
* TODO need to refactor, move code in switch cases to their
* own functions.
*/
BOOL
InterpretCommand(
    _In_ LPWSTR *argv,
    _In_ DWORD dwArgCount)
{
    /************* Declerations *******************************/
    BOOL bIsHelpRequest = FALSE;
    PCONTEXT_ENTRY pContext = NULL;
    PCONTEXT_ENTRY pCurrentContext = NULL;
    PCOMMAND_GROUP pGroup = NULL;
    PCOMMAND_GROUP pCurrentGroup = NULL;
    PCOMMAND_ENTRY pCommand = NULL;
    PCOMMAND_ENTRY pCurrentCommand = NULL;
    PLIST pVisitedContextList;
    BOOL bIsInteractiveMode = TRUE;
    BOOL bIsOffline = !GetMode();
    BOOL bInherited = FALSE;
    enum INTERPRET_EVENT event;
    DWORD dwArgIndex = 0;
    BOOL bDone = FALSE;
    DWORD dwError = ERROR_SUCCESS;
    PCONTEXT_ENTRY pRootContext = GetRootContext();
    /**********************************************************/
    
    /**************** Check input argumemts *******************/
    if (argv == NULL)
    {
        DPRINT1("%s argv == NULL\n", __FUNCTION__);
        return TRUE;
    }
        
    if (dwArgCount == 0)
    {
        DPRINT1("%s dwArgCount == 0\n", __FUNCTION__);
        return TRUE;
    }
    
    if (GetCurrentContext() == NULL)
    {
        DPRINT1("%s GetCurrentContext IS Null\n", __FUNCTION__);
        SetCurrentContext(pRootContext);  
    }
    /**********************************************************/
    
    
    // ********************************************************************************************    
    //                                Check for comment
    // ********************************************************************************************
    if (_wcsicmp(argv[0], L"#") == 0)
    {
        return TRUE;
    }
    // ********************************************************************************************
    
    // ********************************************************************************************    
    //                                Check for help token
    // ********************************************************************************************
    bIsHelpRequest = ((_wcsicmp(argv[0], L"?") == 0) && (dwArgCount == 1));
    bIsHelpRequest |= ((_wcsicmp(argv[0], L"help") == 0) && (dwArgCount == 1));
    
    if (bIsHelpRequest)
    {   
        HelpContext(GetCurrentContext());
        return TRUE;
    }
    
    
    pVisitedContextList = CreateList();
    if (pVisitedContextList == NULL)
    {
        DPRINT1("%s pVisitedContextList is NULL!\n", __FUNCTION__);
        return TRUE;
    }
    
    pCurrentContext = GetCurrentContext();
    pCurrentGroup = pCurrentContext->pGroupListHead;
    pCurrentCommand = pCurrentContext->pCommandListHead;
                    
    event = INTERPRET_EVENT_SEARCH_FOR_TOKENS;
    
    while(dwArgIndex < dwArgCount)
    {                                   
        switch(event)
        {
            case INTERPRET_EVENT_SEARCH_FOR_TOKENS:
                DPRINT1("%s INTERPRET_EVENT_SEARCH_FOR_TOKENS\n", __FUNCTION__);
                    
                pCommand = _SearchCommand(argv[dwArgIndex], pCurrentCommand);
                if (pCommand != NULL)
                {
                    event = INTERPRET_EVENT_COMMAND;
                    break;
                }
                
                pGroup = _SearchGroup(argv[dwArgIndex], pCurrentGroup);
                if (pGroup != NULL)
                {
                    event = INTERPRET_EVENT_GROUP;
                    break;
                }
                
                pContext = _SearchSubContext(argv[dwArgIndex], pCurrentContext);
                if (pContext != NULL)
                {
                    event = INTERPRET_EVENT_CONTEXT;
                    break;
                }
                

                DPRINT1("%s Checking if context was already visited\n", __FUNCTION__);
                for (PLIST_NODE tNode = pVisitedContextList->Head; tNode; tNode = tNode->Next)
                {
                    DPRINT1("%s Comparing pointers\n", __FUNCTION__);
                    if ((PCONTEXT_ENTRY)tNode->Data == pCurrentContext)
                    {
                        DPRINT1("%s Context already visited\n", __FUNCTION__);
                        if (pCurrentContext->pParentContext && pCurrentContext->pParentContext->pParentContext)
                            pCurrentContext = pCurrentContext->pParentContext->pParentContext;
                        else
                        {
                            DPRINT1("%s Parent or grandparent is NULL\n", __FUNCTION__);
                            event = INTERPRET_EVENT_NOTHING_FOUND;
                            goto finish_token_search;
                        }
                    }
                }
                
                pVisitedContextList->Add(pVisitedContextList, pCurrentContext);
                   
                // nothing was found
                event = INTERPRET_EVENT_SWITCH_TO_PARENT;
                finish_token_search:
                break;
                
            case INTERPRET_EVENT_COMMAND:
                DPRINT1("%s INTERPRET_EVENT_COMMAND\n", __FUNCTION__);
                
                if (pCommand->pOsVersionCheck != NULL)
                {
                    if (!CallOsVersionCheck(pCommand->pOsVersionCheck))
                    {
                        event = INTERPRET_EVENT_NOTHING_FOUND;
                        break;
                    }
                }
                

                if ((bInherited == TRUE) && (pCurrentContext != GetCurrentContext()) && 
                    ((pCommand->dwFlags & CMD_FLAG_PRIVATE) == CMD_FLAG_PRIVATE))
                {
                    event = INTERPRET_EVENT_NOTHING_FOUND;
                    break;
                }
                
                
                if (bIsInteractiveMode && ((pCommand->dwFlags & CMD_FLAG_INTERACTIVE) == CMD_FLAG_INTERACTIVE))
                {
                    event = INTERPRET_EVENT_NOTHING_FOUND;
                    break;
                }
                
                
                if (bIsOffline && ((pCommand->dwFlags & CMD_FLAG_ONLINE) == CMD_FLAG_ONLINE))
                {
                    event = INTERPRET_EVENT_NOTHING_FOUND;
                    break;
                }
                
                // help requests for commands must have either the ? or help token immediately
                // after the command, it must also be the last element of argv
                bIsHelpRequest = FALSE;
                if ((dwArgIndex + 1) <= (dwArgCount - 1))
                {
                    bIsHelpRequest = ((_wcsicmp(argv[dwArgIndex + 1], L"?") == 0) && (dwArgIndex + 1 == (dwArgCount - 1)));
                    bIsHelpRequest |= ((_wcsicmp(argv[dwArgIndex + 1], L"help") == 0) && (dwArgIndex + 1 == (dwArgCount - 1)));
                }

                if (bIsHelpRequest)
                {
                    DPRINT1("%s PrintMessageFromModule help request\n", __FUNCTION__);   
                    PrintMessageFromModule(pCurrentContext->hModule, pCommand->dwCmdHlpToken);  
                    event = INTERPRET_EVENT_FINISHED;
                    break;
                }
                
                if (pCommand->pfnCmdHandler != NULL)
                {
                    DPRINT("%s Calling pfnCmdHandler\n", __FUNCTION__);
                    dwArgIndex++;
                    dwError = pCommand->pfnCmdHandler(NULL, argv, dwArgIndex, dwArgCount, 0, NULL, &bDone);
                    SetLastError(dwError);
                    return !bDone;
                }
                    
                event = INTERPRET_EVENT_FINISHED;
                break;
                
            case INTERPRET_EVENT_GROUP:
                DPRINT1("%s INTERPRET_EVENT_GROUP\n", __FUNCTION__);

                if (pGroup->pOsVersionCheck != NULL)
                {
                    if (!CallOsVersionCheck(pGroup->pOsVersionCheck))
                    {
                        DPRINT1("%s !CallOsVersionCheck(pGroup->pOsVersionCheck) is TRUE\n", __FUNCTION__);
                        event = INTERPRET_EVENT_NOTHING_FOUND;
                        break;
                    }
                }
                
                if ((bInherited == TRUE) && (pCurrentContext != GetCurrentContext()) && 
                    ((pGroup->dwFlags & CMD_FLAG_PRIVATE) == CMD_FLAG_PRIVATE))
                {
                    event = INTERPRET_EVENT_NOTHING_FOUND;
                    break;
                }
                
                
                if (bIsInteractiveMode && ((pGroup->dwFlags & CMD_FLAG_INTERACTIVE) == CMD_FLAG_INTERACTIVE))
                {
                    event = INTERPRET_EVENT_NOTHING_FOUND;
                    break;
                }
                
                
                if (bIsOffline && ((pGroup->dwFlags & CMD_FLAG_ONLINE) == CMD_FLAG_ONLINE))
                {
                    event = INTERPRET_EVENT_NOTHING_FOUND;
                    break;
                }
                
                // help requests for groups must have either the ? or help token immediately
                // after the group
                bIsHelpRequest = FALSE;
                if ((dwArgIndex + 1) <= (dwArgCount - 1))
                {
                    bIsHelpRequest = (_wcsicmp(argv[dwArgIndex + 1], L"?") == 0);
                    bIsHelpRequest |= (_wcsicmp(argv[dwArgIndex + 1], L"help") == 0);
                }
                
                // if argv ends with a group token show help for that group
                if (dwArgIndex == (dwArgCount - 1))
                    bIsHelpRequest = TRUE;
                
                    
                if (bIsHelpRequest)
                {   
                    HelpGroup(pGroup);
                    event = INTERPRET_EVENT_FINISHED;
                    break;
                }
                
                dwArgIndex++;
                pCurrentGroup = pGroup;
                pCurrentCommand = pCurrentGroup->pCommandListHead;
                
                pCommand = _SearchCommand(argv[dwArgIndex], pCurrentCommand);
                if (pCommand != NULL)
                {
                    event = INTERPRET_EVENT_COMMAND;
                    break;
                }
                
                event = INTERPRET_EVENT_NOTHING_FOUND;
                break; 
                               
            case INTERPRET_EVENT_CONTEXT:
                DPRINT1("%s INTERPRET_EVENT_CONTEXT\n", __FUNCTION__);
                
                
                if (pContext != GetCurrentContext())
                {
                    DPRINT1("%s dwArgIndex: %lu, dwArgCount - 1: %lu\n", __FUNCTION__, 
                                                                             dwArgIndex, 
                                                                            (dwArgCount - 1));
            
                    // check to see if the context is the last token
                    // if so switch to that context
                    if (dwArgIndex == (dwArgCount - 1))
                    {
                        DPRINT1("%s Switching context\n", __FUNCTION__);
                        SetCurrentContext(pContext);
                        event = INTERPRET_EVENT_FINISHED;
                        break;
                    }
                    
                    // help requests for contexts must have either the ? or help token immediately
                    // after the context
                    bIsHelpRequest = FALSE;
                    if ((dwArgIndex + 1) <= (dwArgCount - 1))
                    {
                        bIsHelpRequest = (_wcsicmp(argv[dwArgIndex + 1], L"?") == 0);
                        bIsHelpRequest |= (_wcsicmp(argv[dwArgIndex + 1], L"help") == 0);
                    }
                
                
                    if (bIsHelpRequest)
                    {   
                        HelpContext(pContext);
                        event = INTERPRET_EVENT_FINISHED;
                        break;
                    }
                    
                    // change the local context and update
                    // the current command and group lists    
                    DPRINT1("%s Found a context\n", __FUNCTION__);
                    pCurrentContext = pContext;
                    pCurrentGroup = pCurrentContext->pGroupListHead;
                    pCurrentCommand = pCurrentContext->pCommandListHead;
                    
                    event = INTERPRET_EVENT_SEARCH_FOR_TOKENS;
                    dwArgIndex++;
                    break;
                }
                
                // if we are here we are refering to ourselves
                if (dwArgIndex == (dwArgCount - 1))
                {
                    event = INTERPRET_EVENT_FINISHED;
                }
                // netsh does not allow us to call commamds by refering to ourselves
                else
                {
                    event = INTERPRET_EVENT_NOTHING_FOUND;
                }
                    
                break;
                
            case INTERPRET_EVENT_SWITCH_TO_PARENT:
                DPRINT1("%s INTERPRET_EVENT_SWITCH_TO_PARENT\n", __FUNCTION__);
                
                if (pCurrentContext->pParentContext == NULL)
                {
                    event =  INTERPRET_EVENT_NOTHING_FOUND;
                    break;
                }
                
                bInherited = TRUE;
                pCurrentContext = pCurrentContext->pParentContext;
                pCurrentGroup = pCurrentContext->pGroupListHead;
                pCurrentCommand = pCurrentContext->pCommandListHead;
                event = INTERPRET_EVENT_SEARCH_FOR_TOKENS;
                dwArgIndex = 0;
                break;
                
            case INTERPRET_EVENT_NOTHING_FOUND:
                DPRINT1("%s INTERPRET_EVENT_NOTHING_FOUND\n", __FUNCTION__);
                DPRINT1("%s Context, group, or command not found\n", __FUNCTION__);
                SetLastError(dwError);
    
                LPWSTR lpwszLine = JoinStringsW(argv, dwArgCount, L" ");
                if (lpwszLine != NULL)
                    ConResPrintf(StdErr, IDS_INVALID_COMMAND, lpwszLine);    
                else
                    ConResPrintf(StdErr, IDS_INVALID_COMMAND, L""); 
    
                ConPrintf(StdErr, L"\n");
                
                pVisitedContextList->Free(pVisitedContextList);
                return TRUE;
                break;
                
            case INTERPRET_EVENT_FINISHED:
                DPRINT1("%s INTERPRET_EVENT_FINISHED\n", __FUNCTION__);
                pVisitedContextList->Free(pVisitedContextList);
                return TRUE;
                break;
        } // end switch
    } // end while   
    
    // we should not get here
    pVisitedContextList->Free(pVisitedContextList);
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


static
LPWSTR
CreatePromptStringFromContext(
    PCONTEXT_ENTRY pContext)
{
    size_t nPromptLength = 0;
    STACK *pStack = NULL;
    LPWSTR pwszPrompt = NULL;
    pStack = CreateStack();
    PCONTEXT_ENTRY pTempContext = NULL;
    
    if(pStack == NULL)
        return NULL;
    // todo add machine name with following format to prompt string
    // [machine_name] netsh context>
     
    // start from the current node and work backwards to root node
    // calculating the size needed for the promt string
    while(pContext)
    {
        nPromptLength += wcslen(pContext->pszContextName) + (1 * sizeof(WCHAR)); // + 1 for an extra space
        StackPush(pStack, pContext);
 
        // climb the tree
        pContext = pContext->pParentContext;
    }
    
    nPromptLength += (1 * sizeof(WCHAR)); // to account for "\0"
    pwszPrompt = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nPromptLength * sizeof(WCHAR));
    
    // copy the context names to the prompt string
    while(!IsStackEmpty(pStack))
    {
       pTempContext = StackPop(pStack);
       if (pTempContext)
       {
           wcscat(pwszPrompt, pTempContext->pszContextName);
           wcscat(pwszPrompt, L" ");
       }
    }
    
    // replace the last character with the prompt end character
    pwszPrompt[wcslen(pwszPrompt) - 1] = L'>';
    
    StackFree(pStack, FALSE);
    return pwszPrompt;
}


static
LPCWSTR
GetCommandAliasFromInput(
    LPWSTR input_line,
    SIZE_T *index)
{
    *index = 0;
    LPWSTR pwszAliasValue = NULL;
    WCHAR wChar = L'\0';
    BOOL bRevert = FALSE;

    while (input_line[*index] != L'\0')
    {
        wChar = input_line[*index];
        if (iswspace(input_line[*index]))
        {
            input_line[*index] = L'\0';
            bRevert = TRUE;
            break;
        }

        (*index)++;
    }

    if (g_pHashTable != NULL)
    {
        pwszAliasValue = g_pHashTable->GetValue(g_pHashTable, input_line, ((*index) + 1) * sizeof(WCHAR));
    }

    if (bRevert == TRUE)
        input_line[*index] = wChar;

    return pwszAliasValue;
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
    LPWSTR alias_with_input_line = NULL;
    LPWSTR pwszPrompt = NULL;
    PCONTEXT_ENTRY pLocalCurrentContext = NULL;
    PCONTEXT_ENTRY pRootContext = GetRootContext();
    SIZE_T index = 0;

    pLocalCurrentContext = pRootContext; // used to detect context switches
    pwszPrompt = CreatePromptStringFromContext(pLocalCurrentContext);
    
    if (pwszPrompt == NULL)
        return;
        
    while (bRun != FALSE)
    {
        dwArgCount = 0;
        memset(args_vector, 0, sizeof(args_vector));

        /* Shown just before the input where the user places commands */
        if (GetCurrentContext() != pLocalCurrentContext)
        {
            HeapFree(GetProcessHeap(), 0, pwszPrompt);
            pwszPrompt = CreatePromptStringFromContext(GetCurrentContext());
            if (pwszPrompt == NULL)
            {
                ConPrintf(StdOut, L"Failed to create prompt.\n");
                return;
            }
                
            pLocalCurrentContext = GetCurrentContext();
        }
        
        // display the prompt string
        ConPrintf(StdOut, L"%ls", pwszPrompt);
        
        /* Get input from the user. */
        fgetws(input_line, MAX_STRING_SIZE, stdin);
        
        LPCWSTR pwszAliasValue = GetCommandAliasFromInput(input_line, &index);
        
        // if it is an alias concat the value of the alias with input_line
        if (pwszAliasValue != NULL)
        {
            // Allcoate the space needed to concat the strings
            size_t nStrSize = wcslen(pwszAliasValue) + wcslen(&input_line[index]) + 2; // two strings + space + null characters
            alias_with_input_line = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nStrSize * sizeof(WCHAR));

            if (alias_with_input_line == NULL)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
            }

            // copy the alias value and input_line into alias_with_input_line
            swprintf(alias_with_input_line, L"%ls %ls", pwszAliasValue, &input_line[index]);
            ptr = alias_with_input_line;
        }
        else
        {
            // no alias so we can use input_line
            ptr = input_line;
        }
        
        // vectorize the string       
        while (*ptr != L'\0')
        {
            if (iswspace(*ptr))
            {
                *ptr = 0;
                bWhiteSpace = TRUE;
            }
            else
            {
                if ((bWhiteSpace == TRUE) && (dwArgCount < MAX_ARGS_COUNT))
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
        if (alias_with_input_line  != NULL)
        {
            HeapFree(GetProcessHeap(), 0, alias_with_input_line);
            alias_with_input_line  = NULL;
        }
    }
}
