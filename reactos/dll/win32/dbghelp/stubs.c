/*
 * File stubs.c - stubs for exported functions
 *
 * Copyright (C) 2007, Timo Kreuzer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_NO_STATUS
#include <windows.h>
#include <ndk/ntndk.h>

#include "dbghelp_private.h"

#define UNIMPLEMENTED DbgPrint("DBGHELP: %s is unimplemented, please try again later.\n", __FUNCTION__);

BOOL WINAPI
DbgHelpCreateUserDump(LPSTR pszFileName,
                      PDBGHELP_CREATE_USER_DUMP_CALLBACK Callback,
                      PVOID pUserData)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
DbgHelpCreateUserDumpW(LPWSTR pszFileName,
                      PDBGHELP_CREATE_USER_DUMP_CALLBACK Callback,
                      PVOID pUserData)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
FindFileInPath(HANDLE hProcess,
		       LPSTR pszSearchPath,
               LPSTR pszFileName,
               PVOID id,
               DWORD two,
               DWORD three,
               DWORD dwFlags,
               LPSTR pszFilePath)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
FindFileInSearchPath(HANDLE hProcess,
                     LPSTR pszSearchPath,
                     LPSTR pszFileName,
                     DWORD one,
                     DWORD two,
                     DWORD three,
                     LPSTR pszFilePath)
{
    UNIMPLEMENTED;
	return FALSE;
}

PVOID WINAPI
ImageDirectoryEntryToDataEx(PVOID pModuleBase,
                            BOOLEAN bMappedAsImage,
                            USHORT DirectoryEntry,
                            PULONG pulSize,
                            PIMAGE_SECTION_HEADER *FoundHeader)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL WINAPI
SymAddSymbol(HANDLE hProcess,
             ULONG64 ModBase,
             PCSTR pszName,
             DWORD64 Address,
             DWORD dwSize,
             DWORD dwFlags)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymAddSymbolW(HANDLE hProcess,
              ULONG64 ModBase,
              PCWSTR pszName,
              DWORD64 Address,
              DWORD dwSize,
              DWORD dwFlags)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymDeleteSymbol(HANDLE hProcess,
                ULONG64 ModBase,
                PCSTR pszName,
                DWORD64 Address,
                DWORD dwFlags)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymDeleteSymbolW(HANDLE hProcess,
                 ULONG64 ModBase,
                 PCWSTR pszName,
                 DWORD64 Address,
                 DWORD dwFlags)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymEnumLinesW(HANDLE hProcess,
              ULONG64 ModBase,
              PCWSTR pszObj,
              PCWSTR pszFile,
              PSYM_ENUMLINES_CALLBACKW Callback,
              PVOID pUserContext)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL SymEnumProcesses(PSYM_ENUMPROCESSES_CALLBACK Callback,
                      PVOID pUserContext)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymEnumSourceFilesW(HANDLE hProcess,
                    ULONG64 ModBase,
                    PCWSTR pszMask,
                    PSYM_ENUMSOURCEFILES_CALLBACKW Callback,
                    PVOID pUserContext)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymEnumSourceLines(HANDLE hProcess,
                   ULONG64 ModBase,
                   PCSTR pszObject,
                   PCSTR pszFile,
                   DWORD dwLine,
                   DWORD dwFlags,
                   PSYM_ENUMLINES_CALLBACK Callback,
                   PVOID pUserContext)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymEnumSourceLinesW(HANDLE hProcess,
                    ULONG64 ModBase,
                    PCWSTR pszObject,
                    PCWSTR pszFile,
                    DWORD dwLine,
                    DWORD dwFlags,
                    PSYM_ENUMLINES_CALLBACKW Callback,
                    PVOID pUserContext)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymEnumSym(HANDLE hProcess,
           ULONG64 ModBase,
           PSYM_ENUMERATESYMBOLS_CALLBACK Callback,
           PVOID pUserContext)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymEnumSymbolsForAddr(HANDLE hProcess,
                      DWORD64 Address,
                      PSYM_ENUMERATESYMBOLS_CALLBACK Callback,
                      PVOID pUserContext)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymEnumSymbolsForAddrW(HANDLE hProcess,
                       DWORD64 Address,
                       PSYM_ENUMERATESYMBOLS_CALLBACKW Callback,
                       PVOID pUserContext)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymEnumerateSymbols64(HANDLE hProcess,
                      DWORD64 ModBase,
                      PSYM_ENUMSYMBOLS_CALLBACK64 Callback,
                      PVOID pUserContext)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymEnumerateSymbolsW64(HANDLE hProcess,  
             DWORD64 ModBase,  
             PSYM_ENUMSYMBOLS_CALLBACK64W Callback,  
             PVOID pUserContext)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymEnumerateSymbolsW(HANDLE hProcess,
             DWORD ModBase,
             PSYM_ENUMSYMBOLS_CALLBACKW Callback,
             PVOID pUserContext)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymFromIndex(HANDLE hProcess,
             ULONG64 ModBase,
             DWORD Index,
             PSYMBOL_INFO Symbol)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymFromIndexW(HANDLE hProcess,
              ULONG64 ModBase,
              DWORD Index,
              PSYMBOL_INFOW Symbol)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymFromNameW(HANDLE hProcess,
             PCWSTR pszName,
             PSYMBOL_INFOW Symbol)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymFromToken(HANDLE hProcess,
             DWORD64 ModBase,
             DWORD Token,
             PSYMBOL_INFO Symbol)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymFromTokenW(HANDLE hProcess,
              DWORD64 ModBase,
			  DWORD Token,
              PSYMBOL_INFOW Symbol)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymGetFileLineOffsets64(HANDLE hProcess,
                        PSTR pszModuleName,
                        PSTR pszFileName,
                        PDWORD64 pBuffer,
                        ULONG BufferLines)
{
    UNIMPLEMENTED;
	return FALSE;
}

PCHAR WINAPI
SymGetHomeDirectory(DWORD dwType,
                    PSTR pszDir,
                    size_t Size)
{
    UNIMPLEMENTED;
	return NULL;
}

PWCHAR WINAPI
SymGetHomeDirectoryW(DWORD dwType,
                     PWSTR pszDir,
                     size_t Size)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL WINAPI
SymGetLineFromName64(HANDLE hProcess,  
                     PCSTR pszModuleName,  
                     PCSTR pszFileName,  
                     DWORD dwLineNumber,  
                     PLONG plDisplacement,  
                     PIMAGEHLP_LINE64 Line)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymGetLineFromName(HANDLE hProcess,
                   PCSTR pszModuleName,
                   PCSTR pszFileName,
                   DWORD dwLineNumber,
                   PLONG plDisplacement,
                   PIMAGEHLP_LINE Line)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymGetLineFromNameW64(HANDLE hProcess,
                           PCWSTR pszModuleName,
                           PCWSTR pszFileName,
                           DWORD dwLineNumber,
                           PLONG lpDisplacement,
                           PIMAGEHLP_LINEW64 Line)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymGetLineNextW64(HANDLE hProcess,
                  PIMAGEHLP_LINEW64 Line)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymGetLinePrevW64(HANDLE hProcess,
                  PIMAGEHLP_LINEW64 Line)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymGetScope(HANDLE hProcess,
            ULONG64 ModBase,
            DWORD Index,
            PSYMBOL_INFO Symbol)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymGetScopeW(HANDLE hProcess,
             ULONG64 ModBase,
             DWORD Index,
             PSYMBOL_INFOW Symbol)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymGetSourceFile(HANDLE hProcess,
                 ULONG64 ModBase,
                 PCSTR pszParams,
                 PCSTR pszFileSpec,
                 PSTR pszFilePath,
                 DWORD Size)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymGetSourceFileFromToken(HANDLE hProcess,
                          PVOID Token,
                          PCSTR pszParams,
                          PSTR pszFilePath,
                          DWORD Size)
{
    UNIMPLEMENTED;
	return FALSE;
}


BOOL WINAPI
SymGetSourceFileFromTokenW(HANDLE hProcess,
                           PVOID Token,
                           PCWSTR pszParams,
                           PWSTR pszFilePath,
                           DWORD Size)
{
    UNIMPLEMENTED;
	return FALSE;
}

// SymGetSourceFileW
// SymGetSourceVarFromToken
// SymGetSourceVarFromTokenW

BOOL WINAPI
SymGetSymFromName64(HANDLE hProcess,  
                    PCSTR pszName,  
                    PIMAGEHLP_SYMBOL64 Symbol)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymGetSymNext64(HANDLE hProcess,  
                PIMAGEHLP_SYMBOL64 Symbol)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymGetSymPrev64(HANDLE hProcess,
                PIMAGEHLP_SYMBOL64 Symbol)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymGetSymbolFile(HANDLE hProcess,
                 PCSTR pszSymPath,
                 PCSTR pszImageFile,
                 DWORD Type,
                 PSTR SymbolFile,
                 size_t cSymbolFile,
                 PSTR DbgFile,
                 size_t cDbgFile)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymGetSymbolFileW(HANDLE hProcess,
                  PCWSTR pszSymPath,
                  PCWSTR pszImageFile,
                  DWORD Type,
                  PWSTR pszSymbolFile,
                  size_t cSymbolFile,
                  PWSTR pszDbgFile,
                  size_t cDbgFile)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymGetTypeFromNameW(HANDLE hProcess,
                    ULONG64 ModBase,
                    PCWSTR pszName,
                    PSYMBOL_INFOW Symbol)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymGetTypeInfoEx(HANDLE hProcess,
                 DWORD64 ModBase,
                 PIMAGEHLP_GET_TYPE_INFO_PARAMS Params)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymMatchStringA(PCSTR pszString,
                PCSTR pszExpression,
                BOOL bCaseSensitiv)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymMatchStringW(PCWSTR pszString,
                PCWSTR pszExpression,
                BOOL bCaseSensitiv)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymNext(HANDLE hProcess,
        PSYMBOL_INFO Symbol)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymNextW(HANDLE hProcess,
         PSYMBOL_INFOW Symbol)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymPrev(HANDLE hProcess,
        PSYMBOL_INFO Symbol)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymPrevW(HANDLE hProcess,
         PSYMBOL_INFOW Symbol)
{
    UNIMPLEMENTED;
	return FALSE;
}

// SymRefreshModuleList

PCHAR WINAPI
SymSetHomeDirectory(HANDLE hProcess,
                    PCSTR pszDir)
{
    UNIMPLEMENTED;
	return NULL;
}

PWCHAR WINAPI
SymSetHomeDirectoryW(HANDLE hProcess,
                     PCWSTR pszDir)
{
    UNIMPLEMENTED;
	return NULL;
}

// SymSetSymWithAddr64

PCSTR WINAPI
SymSrvDeltaName(HANDLE hProcess,
                PCSTR pszSymPath,
                PCSTR pszType,
                PCSTR pszFile1,
                PCSTR pszFile2)
{
    UNIMPLEMENTED;
	return NULL;
}

PCWSTR WINAPI
SymSrvDeltaNameW(HANDLE hProcess,
                 PCWSTR pszSymPath,
                 PCWSTR pszType,
                 PCWSTR pszFile1,
                 PCWSTR pszFile2)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL WINAPI
SymSrvGetFileIndexString(HANDLE hProcess,
                         PCSTR pszSrvPath,
                         PCSTR pszFile,
                         PSTR pszIndex,
                         size_t Size,
                         DWORD dwFlags)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymSrvGetFileIndexStringW(HANDLE hProcess,
                          PCWSTR pszSrvPath,
                          PCWSTR pszFile,
                          PWSTR pszIndex,
                          size_t Size,
                          DWORD dwFlags)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymSrvGetFileIndexes(PCSTR File,
                     GUID* Id,
                     DWORD* Val1,
                     DWORD* Val2,
                     DWORD dwFlags)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymSrvGetFileIndexesW(PCWSTR File,
                      GUID* Id,
                      DWORD* Val1,
                      DWORD* Val2,
                      DWORD dwFlags)
{
    UNIMPLEMENTED;
	return FALSE;
}

PCSTR WINAPI
SymSrvGetSupplement(HANDLE hProcess,
                    PCSTR pszSymPath,
                    PCSTR pszNode,
                    PCSTR pszFile)
{
    UNIMPLEMENTED;
	return NULL;
}

PCWSTR WINAPI
SymSrvGetSupplementW(HANDLE hProcess,
                     PCWSTR pszSymPath,
                     PCWSTR pszNode,
                     PCWSTR pszFile)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL WINAPI
SymSrvIsStore(HANDLE hProcess,
              PCSTR pszPath)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL WINAPI
SymSrvIsStoreW(HANDLE hProcess,
               PCWSTR pszPath)
{
    UNIMPLEMENTED;
	return FALSE;
}

PCSTR WINAPI
SymSrvStoreFile(HANDLE hProcess,
                PCSTR pszSrvPath,
                PCSTR pszFile,
                DWORD pszFlags)
{
    UNIMPLEMENTED;
	return NULL;
}

PCWSTR WINAPI
SymSrvStoreFileW(HANDLE hProcess,
                 PCWSTR pszSrvPath,
                 PCWSTR pszFile,
                 DWORD dwFlags)
{
    UNIMPLEMENTED;
	return NULL;
}

PCSTR WINAPI
SymSrvStoreSupplement(HANDLE hProcess,
                      PCSTR pszSymPath,
                      PCSTR pszNode,
                      PCSTR pszFile,
                      DWORD dwFlags)
{
    UNIMPLEMENTED;
	return NULL;
}

PCWSTR WINAPI
SymSrvStoreSupplementW(HANDLE hProcess,
                       PCWSTR pszSymPath,
                       PCWSTR pszNode,
                       PCWSTR pszFile,
                       DWORD dwFlags)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL WINAPI
SymUnDName64(PIMAGEHLP_SYMBOL64 Symbol,
             PSTR pszUndecoratedName,
             DWORD dwUndecoratedNameLength)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD WINAPI
UnDecorateSymbolNameW(PCWSTR DecoratedName,
                      PWSTR pszUnDecoratedName,
                      DWORD dwUndecoratedLength,
                      DWORD dwFlags)
{
    UNIMPLEMENTED;
	return 0;
}


