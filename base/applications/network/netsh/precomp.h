/*
 * PROJECT:    ReactOS NetSh
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    Network Shell main header file
 * COPYRIGHT:  Copyright 2023 Eric Kohl <eric.kohl@reactos.org>
               Copyright 2025 Curtis Wilson <LiquidFox1776@gmail.com>
 */

#ifndef PRECOMP_H
#define PRECOMP_H

/* INCLUDES ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wincon.h>
#include <winuser.h>

#include <errno.h>

#include <conutils.h>
#include <strsafe.h>
#include <netsh.h>

#include "resource.h"
#include "hashtable.h"

/* DEFINES *******************************************************************/

#define MAX_STRING_SIZE 1024
#define MAX_ARGS_COUNT 256

#define REG_NETSH_PATH L"Software\\Microsoft\\NetSh"

//todo add to netsh.h
#define NETSH_ERROR_BASE                        15000
#define ERROR_NO_ENTRIES                        (NETSH_ERROR_BASE + 0)
#define ERROR_INVALID_SYNTAX                    (NETSH_ERROR_BASE + 1)
#define ERROR_PROTOCOL_NOT_IN_TRANSPORT         (NETSH_ERROR_BASE + 2)
#define ERROR_NO_CHANGE                         (NETSH_ERROR_BASE + 3)
#define ERROR_CMD_NOT_FOUND                     (NETSH_ERROR_BASE + 4)
#define ERROR_ENTRY_PT_NOT_FOUND                (NETSH_ERROR_BASE + 5)
#define ERROR_DLL_LOAD_FAILED                   (NETSH_ERROR_BASE + 6)
#define ERROR_INIT_DISPLAY                      (NETSH_ERROR_BASE + 7)
#define ERROR_TAG_ALREADY_PRESENT               (NETSH_ERROR_BASE + 8)
#define ERROR_INVALID_OPTION_TAG                (NETSH_ERROR_BASE + 9)
#define ERROR_NO_TAG                            (NETSH_ERROR_BASE + 10)
#define ERROR_MISSING_OPTION                    (NETSH_ERROR_BASE + 11)
#define ERROR_TRANSPORT_NOT_PRESENT             (NETSH_ERROR_BASE + 12)
#define ERROR_SHOW_USAGE                        (NETSH_ERROR_BASE + 13)
#define ERROR_INVALID_OPTION_VALUE              (NETSH_ERROR_BASE + 14)
#define ERROR_OKAY                              (NETSH_ERROR_BASE + 15)
#define ERROR_CONTINUE_IN_PARENT_CONTEXT        (NETSH_ERROR_BASE + 16)
#define ERROR_SUPPRESS_OUTPUT                   (NETSH_ERROR_BASE + 17)
#define ERROR_HELPER_ALREADY_REGISTERED         (NETSH_ERROR_BASE + 18)
#define ERROR_CONTEXT_ALREADY_REGISTERED        (NETSH_ERROR_BASE + 19)
#define ERROR_PARSING_FAILURE                   (NETSH_ERROR_BASE + 20) 
#define NETSH_ERROR_END                ERROR_CONTEXT_ALREADY_REGISTERED

/* TYPEDEFS ******************************************************************/

// TODO move to netsh.h
enum NS_CMD_FLAGS
{
    CMD_FLAG_PRIVATE     = 0x01, // not valid in sub-contexts
    CMD_FLAG_INTERACTIVE = 0x02, // not valid from outside netsh
    CMD_FLAG_LOCAL       = 0x08, // not valid from a remote machine
    CMD_FLAG_ONLINE      = 0x10, // not valid in offline/non-commit mode
    CMD_FLAG_HIDDEN      = 0x20, // hide from help but allow execution
    CMD_FLAG_LIMIT_MASK  = 0xffff,
    CMD_FLAG_PRIORITY    = 0x80000000 // ulPriority field is used*/
};


typedef struct _LIST_NODE 
{
    PVOID Data;
    struct _LIST_NODE *Next;
} LIST_NODE, *PLIST_NODE;

typedef struct _LIST 
{
    PLIST_NODE Head;
    size_t NodeCount;
    BOOL (*Add)(struct _LIST *self, PVOID data);
    BOOL (*Remove)(struct _LIST *self, PVOID data);
    BOOL (*RemoveItem)(struct _LIST *self, size_t index);
    size_t (*Count)(struct _LIST *self);
    PVOID (*Pop)(struct _LIST *self);
    VOID (*Free)(struct _LIST *self);
    PLIST_NODE (*GetItem)(struct _LIST *self, size_t index);
} LIST, *PLIST;


typedef struct _STACK_NODE 
{
    void *pData;
    struct _STACK_NODE* pNext;
} STATCK_NODE;


typedef struct _STACK {
    STATCK_NODE* pTop; 
} STACK;

typedef struct _DLL_LIST_ENTRY
{
    struct _DLL_LIST_ENTRY *pPrev;
    struct _DLL_LIST_ENTRY *pNext;

    PWSTR pszDllName;
    PWSTR pszShortName;
    PWSTR pszValueName;

    HMODULE hModule;

} DLL_LIST_ENTRY, *PDLL_LIST_ENTRY;

typedef struct _HELPER_ENTRY
{
    struct _HELPER_ENTRY *pPrev;
    struct _HELPER_ENTRY *pNext;

    NS_HELPER_ATTRIBUTES Attributes;

    PDLL_LIST_ENTRY pDllEntry;
    BOOL bStarted;
    const GUID *pguidParentHelper;

    struct _HELPER_ENTRY *pSubHelperHead;
    struct _HELPER_ENTRY *pSubHelperTail;

} HELPER_ENTRY, *PHELPER_ENTRY;

typedef struct _COMMAND_ENTRY
{
    struct _COMMAND_ENTRY *pPrev;
    struct _COMMAND_ENTRY *pNext;

    LPCWSTR pwszCmdToken;
    PFN_HANDLE_CMD pfnCmdHandler;
    DWORD dwShortCmdHelpToken;
    DWORD dwCmdHlpToken;
    DWORD dwFlags;
    PNS_OSVERSIONCHECK pOsVersionCheck;
    HMODULE hModule;
    
} COMMAND_ENTRY, *PCOMMAND_ENTRY;

typedef struct _COMMAND_GROUP
{
    struct _COMMAND_GROUP *pPrev;
    struct _COMMAND_GROUP *pNext;

    LPCWSTR pwszCmdGroupToken;
    DWORD dwShortCmdHelpToken;
    ULONG ulCmdGroupSize;
    DWORD dwFlags;
    PNS_OSVERSIONCHECK pOsVersionCheck;
    HMODULE hModule;
    
    PCOMMAND_ENTRY pCommandListHead;
    PCOMMAND_ENTRY pCommandListTail;
} COMMAND_GROUP, *PCOMMAND_GROUP;

typedef struct _CONTEXT_ENTRY
{
    struct _CONTEXT_ENTRY *pPrev;
    struct _CONTEXT_ENTRY *pNext;

    struct _CONTEXT_ENTRY *pParentContext;

    PWSTR pszContextName;
    GUID Guid;
    HMODULE hModule;

    PCOMMAND_ENTRY pCommandListHead;
    PCOMMAND_ENTRY pCommandListTail;

    PCOMMAND_GROUP pGroupListHead;
    PCOMMAND_GROUP pGroupListTail;

    struct _CONTEXT_ENTRY *pSubContextHead;
    struct _CONTEXT_ENTRY *pSubContextTail;
} CONTEXT_ENTRY, *PCONTEXT_ENTRY;


/* GLOBAL VARIABLES ***********************************************************/

//extern PCONTEXT_ENTRY pRootContext;
//extern PCONTEXT_ENTRY pCurrentContext;


/* PROTOTYPES *****************************************************************/

/* context.c */

BOOL
CreateRootContext(VOID);

PCONTEXT_ENTRY
GetCurrentContext(VOID);

void
SetCurrentContext(
    PCONTEXT_ENTRY pContext);

PCONTEXT_ENTRY
GetRootContext(VOID);


/* help.c */
DWORD
WINAPI
HelpCommand(
    LPCWSTR pwszMachine,
    LPWSTR *ppwcArguments,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone);
    
VOID
HelpContext(
    PCONTEXT_ENTRY pContext);


VOID
HelpGroup(
    PCOMMAND_GROUP pGroup);


/* helper.c */
VOID
LoadHelpers(VOID);

VOID
UnloadHelpers(VOID);


DWORD
WINAPI
AddHelperCommand(
    LPCWSTR pwszMachine,
    LPWSTR *ppwcArguments,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone);

DWORD
WINAPI
DeleteHelperCommand(
    LPCWSTR pwszMachine,
    LPWSTR *ppwcArguments,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone);
    
    
PHELPER_ENTRY
FindHelper(const GUID *pguidHelper);

DWORD
WINAPI
ShowHelperCommand(
    LPCWSTR pwszMachine,
    PWSTR *ppwcArguments,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone);


/* interpreter.c */
BOOL
CallOsVersionCheck(
    PNS_OSVERSIONCHECK NsOsversioncheck);

BOOL
InterpretScript(
    LPWSTR pszFileName);

BOOL
InterpretCommand(
    LPWSTR *argv,
    DWORD dwArgCount);


VOID
DisplayAliasEntry(
    PVOID pKey, 
    SIZE_T nKeyLength);


VOID
DisplayAliasTable();


BOOL
SetAliasEntry(
    PVOID pKey, 
    SIZE_T nKeyLength, 
    PVOID pValue);


BOOL
DeleteAliasEntry(
    PVOID pKey, 
    SIZE_T nKeyLength);
    

VOID
InterpretInteractive(VOID);

/* list.c */
PLIST 
CreateList();

/* netsh.c */
void
SetMode(
    BOOL bOnline);
    
    
BOOL
GetMode(VOID);

/* stack.c */
STACK* 
CreateStack(void);


void 
StackPush(
    STACK *pStack, 
    void *pData) ;


void* 
StackPop(
    STACK *pStack) ;


void* 
StackPeek(
    STACK* pStack);


void 
StackFree(
    STACK *pStack, 
    BOOL bFreeData);


BOOL 
IsStackEmpty(STACK *pStack);

/* utility.c */
PWSTR JoinStringsW(
    PWSTR* ppwszStrings, 
    size_t sizeCount, 
    PCWSTR pcwszSeparator);

#endif /* PRECOMP_H */
