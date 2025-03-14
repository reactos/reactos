/*
 * PROJECT:    ReactOS NetSh
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    Network Shell context management functions
 * COPYRIGHT:  Copyright 2023 Eric Kohl <eric.kohl@reactos.org>
               Copyright 2025 Curtis Wilson <LiquidFox1776@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

static PCONTEXT_ENTRY _pRootContext = NULL;
static PCONTEXT_ENTRY _pCurrentContext = NULL;

/* FUNCTIONS ******************************************************************/

PCONTEXT_ENTRY
GetCurrentContext(VOID)
{
    return _pCurrentContext;
}

void
SetCurrentContext(
    PCONTEXT_ENTRY pContext)
{
    if (pContext)
        _pCurrentContext = pContext;
}

PCONTEXT_ENTRY
GetRootContext(VOID)
{
    return _pRootContext;
}

static
void
SetRootContext(
    PCONTEXT_ENTRY pContext)
{
    if (pContext)
        _pRootContext = pContext;
}

static 
STACK *
_GetContextStack()
{
    static STACK *pContextStack = NULL;
    
    if (pContextStack == NULL)
        pContextStack = CreateStack();  
        
    return pContextStack;
}

/*
Search through every context until we find one with a matching GUID
or return NULL if we did not find anything
*/


static
PCONTEXT_ENTRY
GetContextFromGUID(
    const GUID *guidHelper)
{
    DPRINT("%s()\n", __FUNCTION__);
    PCONTEXT_ENTRY pCurrent = GetRootContext();
    
    if (pCurrent == NULL)
        return NULL;
        
    if (guidHelper == NULL)
        return NULL;
        
    STACK *stack = CreateStack();
    
    if (stack == NULL)
        return NULL;
        
    // push the root node 
    StackPush(stack, pCurrent);

    while (!IsStackEmpty(stack)) {
        PCONTEXT_ENTRY pCurrent = (PCONTEXT_ENTRY)StackPop(stack);
        
        // See if we have found a matching GUID
        if ((pCurrent != NULL) && (IsEqualGUID(&pCurrent->Guid, guidHelper)))
        {
            DPRINT1("%s() Found context with supplied GUID\n", __FUNCTION__);
            StackFree(stack, FALSE);
            return pCurrent;
        }
        
        // Push subcontexts onto the stack
        if ((pCurrent != NULL) && (pCurrent->pSubContextHead != NULL))
        {
            PCONTEXT_ENTRY pSubcontext = pCurrent->pSubContextHead;
            while (pSubcontext != NULL) 
            {
                StackPush(stack, pSubcontext);
                pSubcontext = pSubcontext->pNext;
            }
        }
        
        // Push helpers to stack
        if (pCurrent->pNext != NULL)
        {
            pCurrent = pCurrent->pNext;
            
            while (pCurrent)
            {
                StackPush(stack, pCurrent);
                pCurrent = pCurrent->pNext;
            }
         }
    }

    DPRINT1("%s() No context found with supplied GUID\n", __FUNCTION__);
    StackFree(stack, FALSE);
    return NULL;
}

DWORD
WINAPI
AbortCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    ConPrintf(StdOut, L"TODO NOT IMPLEMENTED...\n");
    return NO_ERROR;
}

DWORD
WINAPI
AliasCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    BOOL bReturn = FALSE;
    
    // alias
    if (dwArgCount == 1)
    {
        DisplayAliasTable();
        return NO_ERROR;
    }
    
    // alias <commamd>
    // show alias entry in alias the table
    if (dwArgCount == 2)
    {
        DisplayAliasEntry(argv[1], (wcslen(argv[1]) + 1) * sizeof(WCHAR));
        return NO_ERROR;
    }
    
    // todo check for built in command before adding
    
    wchar_t *pwszAliasValue = JoinStringsW(&argv[2], dwArgCount - 2, L" ");
    
    if (pwszAliasValue == NULL)
    {
        return GetLastError();
    }
    
    wchar_t *pwszAlias = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (wcslen(argv[1]) + 1) * sizeof(WCHAR));
    
    if (pwszAlias == NULL)
    {
        return GetLastError();
    }
    
    wcscpy(pwszAlias, argv[1]);
    bReturn = SetAliasEntry((PVOID)pwszAlias, (wcslen(argv[1]) + 1) * sizeof(WCHAR), (PVOID)pwszAliasValue);
    
    if (bReturn == TRUE)
    {
        // todo localize
        ConPrintf(StdOut, L"Ok.\n\n");
        return NO_ERROR;
    }
    
    return GetLastError();
}


PCOMMAND_ENTRY
AddGroupCommand(
    PCOMMAND_GROUP pGroup,
    LPCWSTR pwszCmdToken,
    PFN_HANDLE_CMD pfnCmdHandler,
    DWORD dwShortCmdHelpToken,
    DWORD dwCmdHlpToken,
    DWORD dwFlags,
    PNS_OSVERSIONCHECK pOsVersionCheck,
    HMODULE hModule);
    
PCONTEXT_ENTRY
AddContext(
    PCONTEXT_ENTRY pParentContext,
    PWSTR pszName,
    GUID *pGuid)
{
    PCONTEXT_ENTRY pEntry;

    if (pParentContext != NULL && pszName == NULL)
        return NULL;

    /* Allocate the entry */
    pEntry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CONTEXT_ENTRY));
    if (pEntry == NULL)
        return NULL;

    /* Allocate the name buffer */
    if (pszName != NULL)
    {
        pEntry->pszContextName = HeapAlloc(GetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           (wcslen(pszName) + 1) * sizeof(WCHAR));
        if (pEntry->pszContextName == NULL)
        {
            HeapFree(GetProcessHeap(), 0, pEntry);
            return NULL;
        }

        /* Fill the entry */
        wcscpy(pEntry->pszContextName, pszName);
    }

    pEntry->pParentContext = pParentContext;
    if (pGuid != NULL)
        CopyMemory(&pEntry->Guid, pGuid, sizeof(pEntry->Guid));

    /* Insert it */
    if (pParentContext != NULL)
    {
        if (pParentContext->pSubContextHead == NULL && pParentContext->pSubContextTail == NULL)
        {
            pParentContext->pSubContextHead = pEntry;
            pParentContext->pSubContextTail = pEntry;
        }
        else
        {
            pEntry->pPrev = pParentContext->pSubContextTail;
            pParentContext->pSubContextTail->pNext = pEntry;
            pParentContext->pSubContextTail = pEntry;
        }
    }

    return pEntry;
}


PCOMMAND_ENTRY
AddContextCommand(
    PCONTEXT_ENTRY pContext,
    LPCWSTR pwszCmdToken,
    PFN_HANDLE_CMD pfnCmdHandler,
    DWORD dwShortCmdHelpToken,
    DWORD dwCmdHlpToken,
    DWORD dwFlags)
{
    PCOMMAND_ENTRY pEntry;

    if (pfnCmdHandler == NULL)
        return NULL;

    /* Allocate the entry */
    pEntry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COMMAND_ENTRY));
    if (pEntry == NULL)
        return NULL;

    pEntry->pwszCmdToken = HeapAlloc(GetProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     (wcslen(pwszCmdToken) + 1) * sizeof(WCHAR));
    if (pEntry->pwszCmdToken == NULL)
    {
        HeapFree(GetProcessHeap(), 0, pEntry);
        return NULL;
    }

    wcscpy((LPWSTR)pEntry->pwszCmdToken, pwszCmdToken);
    pEntry->pfnCmdHandler = pfnCmdHandler;
    pEntry->dwShortCmdHelpToken = dwShortCmdHelpToken;
    pEntry->dwCmdHlpToken = dwCmdHlpToken;
    pEntry->dwFlags = dwFlags;

    if (pContext->pCommandListHead == NULL && pContext->pCommandListTail == NULL)
    {
        // insert the first node into the list
        pContext->pCommandListHead = pEntry;
        pContext->pCommandListTail = pEntry;
    }
    else
    {
        // insert an entry at the end of a linked list
        pEntry->pPrev = pContext->pCommandListTail;
        pContext->pCommandListTail->pNext = pEntry;
        pContext->pCommandListTail = pEntry;
    }

    return pEntry;
}


PCOMMAND_GROUP
AddCommandGroup(
    PCONTEXT_ENTRY pContext,
    LPCWSTR pwszCmdGroupToken,
    DWORD dwShortCmdHelpToken,
    ULONG ulCmdGroupSize,
    DWORD dwFlags,
    PCMD_ENTRY pCmdGroup,
    PNS_OSVERSIONCHECK pOsVersionCheck,
    HMODULE hModule
    )
{
    PCOMMAND_GROUP pEntry;

    /* Allocate the entry */
    pEntry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COMMAND_GROUP));
    if (pEntry == NULL)
        return NULL;

    pEntry->pwszCmdGroupToken = HeapAlloc(GetProcessHeap(),
                                          HEAP_ZERO_MEMORY,
                                          (wcslen(pwszCmdGroupToken) + 1) * sizeof(WCHAR));
    if (pEntry->pwszCmdGroupToken == NULL)
    {
        HeapFree(GetProcessHeap(), 0, pEntry);
        return NULL;
    }


    wcscpy((LPWSTR)pEntry->pwszCmdGroupToken, pwszCmdGroupToken);
    pEntry->dwShortCmdHelpToken = dwShortCmdHelpToken;
    pEntry->ulCmdGroupSize = ulCmdGroupSize;
    pEntry->dwFlags = dwFlags;
    pEntry->pOsVersionCheck = pOsVersionCheck;
    pEntry->hModule = hModule;

    if (pContext->pGroupListHead == NULL && pContext->pGroupListTail == NULL)
    {
        // insert the first node into the list
        pContext->pGroupListHead = pEntry;
        pContext->pGroupListTail = pEntry;
    }
    else
    {
       // insert node at end of list for command group entry
        pEntry->pPrev = pContext->pGroupListTail;
        pContext->pGroupListTail->pNext = pEntry;
        pContext->pGroupListTail = pEntry;
    }
    
    // Add the commands to the group
    for (size_t i = 0; i < pEntry->ulCmdGroupSize; i++)
    {
        DPRINT1("%s Adding group command: %ls\n", __FUNCTION__, pCmdGroup[i].pwszCmdToken);
        AddGroupCommand(pEntry, 
                        pCmdGroup[i].pwszCmdToken, 
                        pCmdGroup[i].pfnCmdHandler, 
                        pCmdGroup[i].dwShortCmdHelpToken,
                        pCmdGroup[i].dwCmdHlpToken, 
                        pCmdGroup[i].dwFlags, 
                        pCmdGroup[i].pOsVersionCheck,
                        pEntry->hModule);
    }
    return pEntry;
}


PCOMMAND_ENTRY
AddGroupCommand(
    PCOMMAND_GROUP pGroup,
    LPCWSTR pwszCmdToken,
    PFN_HANDLE_CMD pfnCmdHandler,
    DWORD dwShortCmdHelpToken,
    DWORD dwCmdHlpToken,
    DWORD dwFlags,
    PNS_OSVERSIONCHECK pOsVersionCheck,
    HMODULE hModule)
{
    PCOMMAND_ENTRY pEntry;

    if (pfnCmdHandler == NULL)
        return NULL;

    /* Allocate the entry */
    pEntry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COMMAND_ENTRY));
    if (pEntry == NULL)
        return NULL;

    pEntry->pwszCmdToken = HeapAlloc(GetProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     (wcslen(pwszCmdToken) + 1) * sizeof(WCHAR));
    if (pEntry->pwszCmdToken == NULL)
    {
        HeapFree(GetProcessHeap(), 0, pEntry);
        return NULL;
    }
    
    wcscpy((LPWSTR)pEntry->pwszCmdToken, pwszCmdToken);
    pEntry->pfnCmdHandler = pfnCmdHandler;
    pEntry->dwShortCmdHelpToken = dwShortCmdHelpToken;
    pEntry->dwCmdHlpToken = dwCmdHlpToken;
    pEntry->dwFlags = dwFlags;
    pEntry->pOsVersionCheck = pOsVersionCheck;
    pEntry->hModule = hModule;

    if (pGroup->pCommandListHead == NULL && pGroup->pCommandListTail == NULL)
    {
        pGroup->pCommandListHead = pEntry;
        pGroup->pCommandListTail = pEntry;
    }
    else
    {
        pEntry->pPrev = pGroup->pCommandListTail;
        pGroup->pCommandListTail->pNext = pEntry;
        pGroup->pCommandListTail = pEntry;
    }

    return pEntry;
}


DWORD
WINAPI
CommitCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    ConPrintf(StdOut, L"TODO NOT IMPLEMENTED...\n");
    return NO_ERROR;
}

VOID
DeleteContext(
    PWSTR pszName)
{
    /* Delete all commands */
    /* Delete the context */
}

DWORD
WINAPI
DumpCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    ConPrintf(StdOut, L"TODO NOT IMPLEMENTED...\n");
    return NO_ERROR;
}

DWORD
WINAPI
UpCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    PCONTEXT_ENTRY pCurrentContext = GetCurrentContext();

    if (pCurrentContext != GetRootContext())
        SetCurrentContext(pCurrentContext->pParentContext);

    return NO_ERROR;
}

DWORD
WINAPI
ExecCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    ConPrintf(StdOut, L"TODO NOT IMPLEMENTED...\n");
    return NO_ERROR;
}

DWORD
WINAPI
ExitCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    *pbDone = TRUE;
    return NO_ERROR;
}

/*
Pushes the currerent context onto the stack
and optionally runs a command
as specified by the optional arguments
*/
DWORD
WINAPI
PushdCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{   
    PCONTEXT_ENTRY pCurrentContext = GetCurrentContext();
    
    if (_GetContextStack() == NULL)
    {
        DPRINT1("%s ContextStack is NULL!\n", __FUNCTION__);
        return 1; 
    }
    
    if (pCurrentContext)
    {
        StackPush(_GetContextStack(), pCurrentContext);
    }
    
    // if pushd was called without arguments
    if (dwArgCount == 1)
        return NO_ERROR;

    DPRINT1("%s argv[1]: %ls (dwArgCount - dwCurrentIndex): %lu\n", __FUNCTION__, argv[1], dwArgCount - dwCurrentIndex);
    // call interpret command with the rest of the parameters
    InterpretCommand(&argv[1], dwArgCount - dwCurrentIndex);
    
    return NO_ERROR;
}

/*
Pops a context off of the stack
popd ignores any arguments supplied to it,
and does not produce errors when supplied
*/
DWORD
WINAPI
PopdCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    PCONTEXT_ENTRY pTempContext = NULL;
    
    if (_GetContextStack() == NULL)
    {
        DPRINT1("%s ContextStack is NULL!\n", __FUNCTION__);
        return 1; 
    }
         
    if (IsStackEmpty(_GetContextStack()))
        return NO_ERROR;
    
     pTempContext = StackPop(_GetContextStack());
     
     if (pTempContext)
     {
         SetCurrentContext(pTempContext);
         return NO_ERROR;
     }
     

    
    // TODO is there a better error message? 
    // How does netsh handle a null pointer on the context stack? 
    // Does it?
    return 1;//ERROR_BAD_STACK; 
}

DWORD
WINAPI
OfflineCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    SetMode(FALSE);
    return NO_ERROR;
}

DWORD
WINAPI
OnlineCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    SetMode(TRUE);
    return NO_ERROR;
}

DWORD
WINAPI
SetMachineCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    ConPrintf(StdOut, L"TODO NOT IMPLEMENTED...\n");
    return NO_ERROR;
}


DWORD
WINAPI
SetModeCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    ConPrintf(StdOut, L"TODO NOT IMPLEMENTED...\n");
    return NO_ERROR;
}


DWORD
WINAPI
UnaliasCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
        BOOL bReturn = FALSE;
    
    if (dwArgCount != 2)
    {
        // todo show error message
        // and display help
        return NO_ERROR;
    }
    
    bReturn = DeleteAliasEntry(argv[1], (wcslen(argv[1]) + 1) * sizeof(WCHAR));
    
    if (bReturn == TRUE)
    {
        // todo localize
        ConPrintf(StdOut, L"Ok.\n\n");
        return NO_ERROR;
    }
    
    return GetLastError();
}

DWORD
WINAPI
RemCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    return 0;
}


BOOL
CreateRootContext(VOID)
{
    PCOMMAND_GROUP pGroup;
    PCONTEXT_ENTRY pRootContext = NULL;
    
    pRootContext = AddContext(NULL, L"netsh", NULL);
    DPRINT1("pRootContext: %p\n", pRootContext);
    
    if (pRootContext == NULL)
        return FALSE;

    pRootContext->hModule = GetModuleHandle(NULL);

    AddContextCommand(pRootContext, L"..",      UpCommand, IDS_HLP_UP, IDS_HLP_UP_EX, 0);
    AddContextCommand(pRootContext, L"?",       HelpCommand, IDS_HLP_HELP, IDS_HLP_HELP_EX, 0);
    AddContextCommand(pRootContext, L"abort",   AbortCommand, IDS_HLP_ABORT, IDS_HLP_ABORT_EX, 0);
    AddContextCommand(pRootContext, L"alias",   AliasCommand, IDS_HLP_ALIAS, IDS_HLP_ALIAS_EX, 0);
    AddContextCommand(pRootContext, L"bye",     ExitCommand, IDS_HLP_EXIT, IDS_HLP_EXIT_EX, 0);
    AddContextCommand(pRootContext, L"commit",  CommitCommand, IDS_HLP_COMMIT, IDS_HLP_COMMIT_EX, 0);
    AddContextCommand(pRootContext, L"dump",    DumpCommand, IDS_HLP_DUMP, IDS_HLP_DUMP_EX, 0);
    AddContextCommand(pRootContext, L"exec",    ExecCommand, IDS_HLP_EXEC, IDS_HLP_EXEC_EX, CMD_FLAG_PRIVATE);
    AddContextCommand(pRootContext, L"exit",    ExitCommand, IDS_HLP_EXIT, IDS_HLP_EXIT_EX, 0);
    AddContextCommand(pRootContext, L"help",    HelpCommand, IDS_HLP_HELP, IDS_HLP_HELP_EX, 0);
    AddContextCommand(pRootContext, L"quit",    ExitCommand, IDS_HLP_EXIT, IDS_HLP_EXIT_EX, 0);
    AddContextCommand(pRootContext, L"pushd",   PushdCommand, IDS_HLP_PUSHD, IDS_HLP_PUSHD_EX, 0);
    AddContextCommand(pRootContext, L"popd",    PopdCommand, IDS_HLP_POPD, IDS_HLP_POPD_EX, 0);
    AddContextCommand(pRootContext, L"offline", OfflineCommand, IDS_HLP_OFFLINE, IDS_HLP_OFFLINE_EX, 0);
    AddContextCommand(pRootContext, L"online",  OnlineCommand, IDS_HLP_ONLINE, IDS_HLP_ONLINE_EX, 0);
    AddContextCommand(pRootContext, L"unalias", UnaliasCommand, IDS_HLP_UNALIAS, IDS_HLP_UNALIAS_EX, 0);
    
    pGroup = AddCommandGroup(pRootContext, L"add", IDS_HLP_GROUP_ADD, 0, 0, NULL, NULL, pRootContext->hModule);
    if (pGroup)
    {
        AddGroupCommand(pGroup, L"helper", AddHelperCommand, IDS_HLP_ADD_HELPER, IDS_HLP_ADD_HELPER_EX, 0, NULL, pRootContext->hModule);
    }

    pGroup = AddCommandGroup(pRootContext, L"delete", IDS_HLP_GROUP_DELETE, 0, 0, NULL, NULL, pRootContext->hModule);
    if (pGroup)
    {
        AddGroupCommand(pGroup, L"helper", DeleteHelperCommand, IDS_HLP_DEL_HELPER, IDS_HLP_DEL_HELPER_EX, 0, NULL, pRootContext->hModule);
    }
    
    pGroup = AddCommandGroup(pRootContext, L"set", IDS_HLP_GROUP_SET, 0, 0, NULL, NULL, pRootContext->hModule);
    if (pGroup)
    {
        AddGroupCommand(pGroup, L"machine", SetMachineCommand, IDS_HLP_SET_MACHINE, IDS_HLP_SET_MACHINE_EX, 0, NULL, pRootContext->hModule);
        AddGroupCommand(pGroup, L"mode", SetModeCommand, IDS_HLP_SET_MODE, IDS_HLP_SET_MODE_EX, 0, NULL, pRootContext->hModule);
    }
    
    pGroup = AddCommandGroup(pRootContext, L"show", IDS_HLP_GROUP_SHOW, 0, 0, NULL, NULL, pRootContext->hModule);
    if (pGroup)
    {
        //AddGroupCommand(pGroup, L"alias", ShowHelperCommand, IDS_HLP_SHOW_HELPER, IDS_HLP_SHOW_HELPER_EX, 0, NULL, pRootContext->hModule);
        //AddGroupCommand(pGroup, L"mode", ShowHelperCommand, IDS_HLP_SHOW_HELPER, IDS_HLP_SHOW_HELPER_EX, 0, NULL, pRootContext->hModule);
        AddGroupCommand(pGroup, L"helper", ShowHelperCommand, IDS_HLP_SHOW_HELPER, IDS_HLP_SHOW_HELPER_EX, 0, NULL, pRootContext->hModule);
    }

    SetCurrentContext(pRootContext);
    SetRootContext(pRootContext);
    return TRUE;
}


DWORD
WINAPI
RegisterContext(
    _In_ const NS_CONTEXT_ATTRIBUTES *pChildContext)
{
    PHELPER_ENTRY pHelper = NULL;
    PCONTEXT_ENTRY pContext = NULL;
    DWORD i;

    if (pChildContext == NULL)
    {
        DPRINT1("Invalid child context!\n");
        return ERROR_INVALID_PARAMETER;
    }

    if ((pChildContext->pwszContext == NULL) ||
        (wcslen(pChildContext->pwszContext) == 0) ||
        (wcschr(pChildContext->pwszContext, L' ') != 0) ||
        (wcschr(pChildContext->pwszContext, L'=') != 0))
    {
        DPRINT1("%s Invalid context name!\n", __FUNCTION__);
            return ERROR_INVALID_PARAMETER;
    }

    if (GetContextFromGUID(&pChildContext->guidHelper) != NULL)
    {
        DPRINT1("%s GUID already exists\n", __FUNCTION__);
        return ERROR_CONTEXT_ALREADY_REGISTERED;
    }
    
    DPRINT1("Name: %S\n", pChildContext->pwszContext);
    
    /* Attempt to find the module handle to the helper DLL so we can 
    load string resources from it later */
    pHelper = FindHelper(&pChildContext->guidHelper);
    DPRINT1("%s pHelper address is : 0x%p\n", __FUNCTION__, pHelper);
    
    if (pHelper != NULL)
    {
        // if we have a parent GUID try and find its context *********************************************************
        DPRINT1("%s if (pHelper->pguidParentHelper)\n", __FUNCTION__);
        if (pHelper->pguidParentHelper != NULL)
        {
            DPRINT1("%s if (pHelper->pguidParentHelper)\n", __FUNCTION__);
            PCONTEXT_ENTRY pTempContext = GetContextFromGUID(pHelper->pguidParentHelper);
            if (pTempContext != NULL)
            {
                DPRINT1("%s Found parent context.\n", __FUNCTION__);
                pContext = AddContext(pTempContext, pChildContext->pwszContext, (GUID*)&pChildContext->guidHelper);
            }
            else
            {
                DPRINT1("%s Cannot find parent context GUID!\n", __FUNCTION__);
                pContext = AddContext(GetRootContext(), pChildContext->pwszContext, (GUID*)&pChildContext->guidHelper);
            }
            
        }
        // we dont have a parent so add to the root context
        else
        {
            DPRINT1("%s Dont have a parent context\n", __FUNCTION__);
            pContext = AddContext(GetRootContext(), pChildContext->pwszContext, (GUID*)&pChildContext->guidHelper);
            
        }
        //*************************************************************************************************************
        
        // find a handle to the module ************************************************************************************
        DPRINT1("%s pHelper is not NULL\n", __FUNCTION__);
        if (pHelper->pDllEntry != NULL)
        {
            if(pHelper->pDllEntry->hModule != NULL)
            {
                DPRINT1("%s pContext->hModule = pHelper->pDllEntry->hModule;\n", __FUNCTION__);
                pContext->hModule = pHelper->pDllEntry->hModule;
                DPRINT1("%s AFTER pContext->hModule = pHelper->pDllEntry->hModule;\n", __FUNCTION__);
            }
            else
            {
                DPRINT1("%s pHelper->pDllEntry->hModule is NULL\n", __FUNCTION__);
                pContext->hModule = NULL;
            }
        }
        else
        {
            DPRINT1("%s pHelper->pDllEntry is NULL\n", __FUNCTION__);
            pContext->hModule = NULL;
        }
        //******************************************************************************************************************
    }
    else
    {
        pContext = AddContext(GetRootContext(), pChildContext->pwszContext, (GUID*)&pChildContext->guidHelper);
        DPRINT("%s pHelper is NULL\n", __FUNCTION__);
        pContext->hModule = NULL;
    }
    //*************************************************************************
    
    if (pContext != NULL)
    {
        // Add all top commands
        for (i = 0; i < pChildContext->ulNumTopCmds; i++)
        {
            DPRINT1("%s pChildContext->pTopCmds[i].pwszCmdToken: %ls\n", __FUNCTION__, pChildContext->pTopCmds[i].pwszCmdToken);
            AddContextCommand(pContext,
                pChildContext->pTopCmds[i].pwszCmdToken,
                pChildContext->pTopCmds[i].pfnCmdHandler,
                pChildContext->pTopCmds[i].dwShortCmdHelpToken,
                pChildContext->pTopCmds[i].dwCmdHlpToken,
                pChildContext->pTopCmds[i].dwFlags);
        }

        /* Add command groups */
        for (i = 0; i < pChildContext->ulNumGroups; i++)
        {
            DPRINT1("%s Adding command group: %ls\n", __FUNCTION__, pChildContext->pCmdGroups[i].pwszCmdGroupToken);
            AddCommandGroup(pContext,
                            pChildContext->pCmdGroups[i].pwszCmdGroupToken,
                            pChildContext->pCmdGroups[i].dwShortCmdHelpToken,
                            pChildContext->pCmdGroups[i].ulCmdGroupSize,
                            pChildContext->pCmdGroups[i].dwFlags,
                            pChildContext->pCmdGroups[i].pCmdGroup,
                            pChildContext->pCmdGroups[i].pOsVersionCheck,
                            pContext->hModule);
        }
                
    }

    return ERROR_SUCCESS;
}
