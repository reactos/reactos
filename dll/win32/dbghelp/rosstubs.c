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

#include <assert.h>

#include "dbghelp_private.h"
#include "image_private.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

#define PDBGHELP_CREATE_USER_DUMP_CALLBACK PVOID
#define PSYM_ENUMPROCESSES_CALLBACK PVOID
#define PENUMSOURCEFILETOKENSCALLBACK PVOID
#define PSYMSRV_INDEX_INFO PVOID
#define POMAP PVOID

BOOL WINAPI
EnumerateLoadedModulesEx(
    IN HANDLE hProcess,
    IN PENUMLOADED_MODULES_CALLBACK64 EnumLoadedModulesCallback,
    IN PVOID UserContext OPTIONAL)
{
    UNIMPLEMENTED;
    return FALSE;
}

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

BOOL
WINAPI
FindFileInPath(
    IN HANDLE hProcess,
    IN PCSTR pszSearchPath,
    IN PCSTR pszFileName,
    IN PVOID id,
    IN DWORD two,
    IN DWORD three,
    IN DWORD flags,
    OUT PSTR FilePath,
    IN PFINDFILEINPATHCALLBACK callback,
    IN PVOID context)
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

BOOL WINAPI
SymEnumProcesses(PSYM_ENUMPROCESSES_CALLBACK Callback,
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

ULONG WINAPI
SymGetFileLineOffsets64(HANDLE hProcess,
                        PCSTR pszModuleName,
                        PCSTR pszFileName,
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

BOOL
WINAPI
SymGetSourceFileW(
    HANDLE hProcess,
    ULONG64 Base,
    PCWSTR Params,
    PCWSTR FileSpec,
    PWSTR FilePath,
    DWORD Size)
{
    UNIMPLEMENTED;
    return FALSE;
}


BOOL
WINAPI
SymGetSourceVarFromToken(
    HANDLE hProcess,
    PVOID Token,
    PCSTR Params,
    PCSTR VarName,
    PSTR Value,
    DWORD Size)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
WINAPI
SymGetSourceVarFromTokenW(
    HANDLE hProcess,
    PVOID Token,
    PCWSTR Params,
    PCWSTR VarName,
    PWSTR Value,
    DWORD Size)
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

BOOL
WINAPI
SymSetScopeFromIndex(
    HANDLE hProcess,
    ULONG64 BaseOfDll,
    DWORD Index)
{
    UNIMPLEMENTED;
    return FALSE;
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

HANDLE
WINAPI
FindDebugInfoFileExW(
    PCWSTR FileName,
    PCWSTR SymbolPath,
    PWSTR DebugFilePath,
    PFIND_DEBUG_FILE_CALLBACKW Callback,
    PVOID CallerData)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
WINAPI
SymAddSourceStream(
    HANDLE hProcess,
    ULONG64 Base,
    PCSTR StreamFile,
    PBYTE Buffer,
    size_t Size)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
WINAPI
SymAddSourceStreamW(
    HANDLE hProcess,
    ULONG64 Base,
    PCWSTR FileSpec,
    PBYTE Buffer,
    size_t Size)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
WINAPI
SymEnumSourceFileTokens(
    HANDLE hProcess,
    ULONG64 Base,
    PENUMSOURCEFILETOKENSCALLBACK Callback)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
WINAPI
SymAddSourceStreamA(
    HANDLE hProcess,
    ULONG64 Base,
    PCSTR StreamFile,
    PBYTE Buffer,
    size_t Size)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
WINAPI
SymEnumTypesByName(
    HANDLE hProcess,
    ULONG64 BaseOfDll,
    PCSTR mask,
    PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
    PVOID UserContext)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
WINAPI
SymEnumTypesByNameW(
    HANDLE hProcess,
    ULONG64 BaseOfDll,
    PCWSTR mask,
    PSYM_ENUMERATESYMBOLS_CALLBACKW EnumSymbolsCallback,
    PVOID UserContext)
{
    UNIMPLEMENTED;
    return FALSE;
}


HANDLE
WINAPI
SymFindDebugInfoFile(
    HANDLE hProcess,
    PCSTR FileName,
    PSTR DebugFilePath,
    PFIND_DEBUG_FILE_CALLBACK Callback,
    PVOID CallerData)
{
    UNIMPLEMENTED;
    return 0;
}


HANDLE
WINAPI
SymFindDebugInfoFileW(
    HANDLE hProcess,
    PCWSTR FileName,
    PWSTR DebugFilePath,
    PFIND_DEBUG_FILE_CALLBACKW Callback,
    PVOID CallerData)
{
    UNIMPLEMENTED;
    return 0;
}


HANDLE
WINAPI
SymFindExecutableImage(
    HANDLE hProcess,
    PCSTR FileName,
    PSTR ImageFilePath,
    PFIND_EXE_FILE_CALLBACK Callback,
    PVOID CallerData)
{
    UNIMPLEMENTED;
    return 0;
}


HANDLE
WINAPI
SymFindExecutableImageW(
    HANDLE hProcess,
    PCWSTR FileName,
    PWSTR ImageFilePath,
    PFIND_EXE_FILE_CALLBACKW Callback,
    PVOID CallerData)
{
    UNIMPLEMENTED;
    return 0;
}


BOOL
WINAPI
SymSrvGetFileIndexInfo(
    PCSTR File,
    PSYMSRV_INDEX_INFO Info,
    DWORD Flags)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
WINAPI
SymSrvGetFileIndexInfoW(
    PCWSTR File,
    PSYMSRV_INDEX_INFO Info,
    DWORD Flags)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
WINAPI
SymGetOmaps(
    HANDLE hProcess,
    DWORD64 BaseOfDll,
    POMAP *OmapTo,
    PDWORD64 cOmapTo,
    POMAP *OmapFrom,
    PDWORD64 cOmapFrom)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
WINAPI
SymGetUnwindInfo(
    HANDLE hProcess,
    DWORD64 Address,
    PVOID Buffer,
    PULONG Size)
{
    UNIMPLEMENTED;
    return FALSE;
}
