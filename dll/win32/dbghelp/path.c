/*
 * File path.c - managing path in debugging environments
 *
 * Copyright (C) 2004, Eric Pouech
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

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dbghelp_private.h"
#include "winnls.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

static inline BOOL is_sep(char ch) {return ch == '/' || ch == '\\';}
static inline BOOL is_sepW(WCHAR ch) {return ch == '/' || ch == '\\';}

static inline const char* file_name(const char* str)
{
    const char*       p;

    for (p = str + strlen(str) - 1; p >= str && !is_sep(*p); p--);
    return p + 1;
}

static inline const WCHAR* file_nameW(const WCHAR* str)
{
    const WCHAR*      p;

    for (p = str + strlenW(str) - 1; p >= str && !is_sepW(*p); p--);
    return p + 1;
}

/******************************************************************
 *		FindDebugInfoFile (DBGHELP.@)
 *
 */
HANDLE WINAPI FindDebugInfoFile(PCSTR FileName, PCSTR SymbolPath, PSTR DebugFilePath)
{
    HANDLE      h;

    h = CreateFileA(DebugFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
    {
        if (!SearchPathA(SymbolPath, file_name(FileName), NULL, MAX_PATH, DebugFilePath, NULL))
            return NULL;
        h = CreateFileA(DebugFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    return (h == INVALID_HANDLE_VALUE) ? NULL : h;
}
 
/******************************************************************
 *		FindDebugInfoFileEx (DBGHELP.@)
 *
 */
HANDLE WINAPI FindDebugInfoFileEx(PCSTR FileName, PCSTR SymbolPath,
                                  PSTR DebugFilePath, 
                                  PFIND_DEBUG_FILE_CALLBACK Callback,
                                  PVOID CallerData)
{
    FIXME("(%s %s %p %p %p): stub\n", 
          FileName, SymbolPath, DebugFilePath, Callback, CallerData);
    return NULL;
}

/******************************************************************
 *		FindExecutableImageExW (DBGHELP.@)
 *
 */
HANDLE WINAPI FindExecutableImageExW(PCWSTR FileName, PCWSTR SymbolPath, PWSTR ImageFilePath,
                                     PFIND_EXE_FILE_CALLBACKW Callback, PVOID user)
{
    HANDLE h;

    if (Callback) FIXME("Unsupported callback yet\n");
    if (!SearchPathW(SymbolPath, FileName, NULL, MAX_PATH, ImageFilePath, NULL))
        return NULL;
    h = CreateFileW(ImageFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    return (h == INVALID_HANDLE_VALUE) ? NULL : h;
}

/******************************************************************
 *		FindExecutableImageEx (DBGHELP.@)
 *
 */
HANDLE WINAPI FindExecutableImageEx(PCSTR FileName, PCSTR SymbolPath, PSTR ImageFilePath,
                                    PFIND_EXE_FILE_CALLBACK Callback, PVOID user)
{
    HANDLE h;

    if (Callback) FIXME("Unsupported callback yet\n");
    if (!SearchPathA(SymbolPath, FileName, NULL, MAX_PATH, ImageFilePath, NULL))
        return NULL;
    h = CreateFileA(ImageFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    return (h == INVALID_HANDLE_VALUE) ? NULL : h;
}

/******************************************************************
 *		FindExecutableImage (DBGHELP.@)
 *
 */
HANDLE WINAPI FindExecutableImage(PCSTR FileName, PCSTR SymbolPath, PSTR ImageFilePath)
{
    return FindExecutableImageEx(FileName, SymbolPath, ImageFilePath, NULL, NULL);
}

/***********************************************************************
 *           MakeSureDirectoryPathExists (DBGHELP.@)
 */
BOOL WINAPI MakeSureDirectoryPathExists(PCSTR DirPath)
{
    char path[MAX_PATH];
    const char *p = DirPath;
    int  n;

    if (p[0] && p[1] == ':') p += 2;
    while (*p == '\\') p++; /* skip drive root */
    while ((p = strchr(p, '\\')) != NULL)
    {
       n = p - DirPath + 1;
       memcpy(path, DirPath, n);
       path[n] = '\0';
       if( !CreateDirectoryA(path, NULL)            &&
           (GetLastError() != ERROR_ALREADY_EXISTS))
           return FALSE;
       p++;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS)
       SetLastError(ERROR_SUCCESS);

    return TRUE;
}

/******************************************************************
 *		SymMatchFileNameW (DBGHELP.@)
 *
 */
BOOL WINAPI SymMatchFileNameW(PCWSTR file, PCWSTR match,
                              PWSTR* filestop, PWSTR* matchstop)
{
    PCWSTR fptr;
    PCWSTR mptr;

    TRACE("(%s %s %p %p)\n",
          debugstr_w(file), debugstr_w(match), filestop, matchstop);

    fptr = file + strlenW(file) - 1;
    mptr = match + strlenW(match) - 1;

    while (fptr >= file && mptr >= match)
    {
        if (toupperW(*fptr) != toupperW(*mptr) && !(is_sepW(*fptr) && is_sepW(*mptr)))
            break;
        fptr--; mptr--;
    }
    if (filestop) *filestop = (PWSTR)fptr;
    if (matchstop) *matchstop = (PWSTR)mptr;

    return mptr == match - 1;
}

/******************************************************************
 *		SymMatchFileName (DBGHELP.@)
 *
 */
BOOL WINAPI SymMatchFileName(PCSTR file, PCSTR match,
                             PSTR* filestop, PSTR* matchstop)
{
    PCSTR fptr;
    PCSTR mptr;

    TRACE("(%s %s %p %p)\n", file, match, filestop, matchstop);

    fptr = file + strlen(file) - 1;
    mptr = match + strlen(match) - 1;

    while (fptr >= file && mptr >= match)
    {
        if (toupper(*fptr) != toupper(*mptr) && !(is_sep(*fptr) && is_sep(*mptr)))
            break;
        fptr--; mptr--;
    }
    if (filestop) *filestop = (PSTR)fptr;
    if (matchstop) *matchstop = (PSTR)mptr;

    return mptr == match - 1;
}

static BOOL do_searchW(PCWSTR file, PWSTR buffer, BOOL recurse,
                       PENUMDIRTREE_CALLBACKW cb, PVOID user)
{
    HANDLE              h;
    WIN32_FIND_DATAW    fd;
    unsigned            pos;
    BOOL                found = FALSE;
    static const WCHAR  S_AllW[] = {'*','.','*','\0'};
    static const WCHAR  S_DotW[] = {'.','\0'};
    static const WCHAR  S_DotDotW[] = {'.','\0'};

    pos = strlenW(buffer);
    if (buffer[pos - 1] != '\\') buffer[pos++] = '\\';
    strcpyW(buffer + pos, S_AllW);
    if ((h = FindFirstFileW(buffer, &fd)) == INVALID_HANDLE_VALUE)
        return FALSE;
    /* doc doesn't specify how the tree is enumerated...
     * doing a depth first based on, but may be wrong
     */
    do
    {
        if (!strcmpW(fd.cFileName, S_DotW) || !strcmpW(fd.cFileName, S_DotDotW)) continue;

        strcpyW(buffer + pos, fd.cFileName);
        if (recurse && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            found = do_searchW(file, buffer, TRUE, cb, user);
        else if (SymMatchFileNameW(buffer, (WCHAR*)file, NULL, NULL))
        {
            if (!cb || cb(buffer, user)) found = TRUE;
        }
    } while (!found && FindNextFileW(h, &fd));
    if (!found) buffer[--pos] = '\0';
    FindClose(h);

    return found;
}

/***********************************************************************
 *           SearchTreeForFileW (DBGHELP.@)
 */
BOOL WINAPI SearchTreeForFileW(PCWSTR root, PCWSTR file, PWSTR buffer)
{
    TRACE("(%s, %s, %p)\n",
          debugstr_w(root), debugstr_w(file), buffer);
    strcpyW(buffer, root);
    return do_searchW(file, buffer, TRUE, NULL, NULL);
}

/***********************************************************************
 *           SearchTreeForFile (DBGHELP.@)
 */
BOOL WINAPI SearchTreeForFile(PCSTR root, PCSTR file, PSTR buffer)
{
    WCHAR       rootW[MAX_PATH];
    WCHAR       fileW[MAX_PATH];
    WCHAR       bufferW[MAX_PATH];
    BOOL        ret;

    MultiByteToWideChar(CP_ACP, 0, root, -1, rootW, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, file, -1, fileW, MAX_PATH);
    ret = SearchTreeForFileW(rootW, fileW, bufferW);
    if (ret)
        WideCharToMultiByte(CP_ACP, 0, bufferW, -1, buffer, MAX_PATH, NULL, NULL);
    return ret;
}

/******************************************************************
 *		EnumDirTreeW (DBGHELP.@)
 *
 *
 */
BOOL WINAPI EnumDirTreeW(HANDLE hProcess, PCWSTR root, PCWSTR file,
                        PWSTR buffer, PENUMDIRTREE_CALLBACKW cb, PVOID user)
{
    TRACE("(%p %s %s %p %p %p)\n",
          hProcess, debugstr_w(root), debugstr_w(file), buffer, cb, user);

    strcpyW(buffer, root);
    return do_searchW(file, buffer, TRUE, cb, user);
}

/******************************************************************
 *		EnumDirTree (DBGHELP.@)
 *
 *
 */
struct enum_dir_treeWA
{
    PENUMDIRTREE_CALLBACK       cb;
    void*                       user;
    char                        name[MAX_PATH];
};

static BOOL CALLBACK enum_dir_treeWA(PCWSTR name, PVOID user)
{
    struct enum_dir_treeWA*     edt = user;

    WideCharToMultiByte(CP_ACP, 0, name, -1, edt->name, MAX_PATH, NULL, NULL);
    return edt->cb(edt->name, edt->user);
}

BOOL WINAPI EnumDirTree(HANDLE hProcess, PCSTR root, PCSTR file,
                        PSTR buffer, PENUMDIRTREE_CALLBACK cb, PVOID user)
{
    WCHAR                       rootW[MAX_PATH];
    WCHAR                       fileW[MAX_PATH];
    WCHAR                       bufferW[MAX_PATH];
    struct enum_dir_treeWA      edt;
    BOOL                        ret;

    edt.cb = cb;
    edt.user = user;
    MultiByteToWideChar(CP_ACP, 0, root, -1, rootW, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, file, -1, fileW, MAX_PATH);
    if ((ret = EnumDirTreeW(hProcess, rootW, fileW, bufferW, enum_dir_treeWA, &edt)))
        WideCharToMultiByte(CP_ACP, 0, bufferW, -1, buffer, MAX_PATH, NULL, NULL);
    return ret;
}

struct sffip
{
    enum module_type            kind;
    /* pe:  id  -> DWORD:timestamp
     *      two -> size of image (from PE header)
     * pdb: id  -> PDB signature
     *            I think either DWORD:timestamp or GUID:guid depending on PDB version
     *      two -> PDB age ???
     * elf: id  -> DWORD:CRC 32 of ELF image (Wine only)
     */
    PVOID                       id;
    DWORD                       two;
    DWORD                       three;
    DWORD                       flags;
    PFINDFILEINPATHCALLBACKW    cb;
    void*                       user;
};

/* checks that buffer (as found by matching the name) matches the info
 * (information is based on file type)
 * returns TRUE when file is found, FALSE to continue searching
 * (NB this is the opposite convention of SymFindFileInPathProc)
 */
static BOOL CALLBACK sffip_cb(PCWSTR buffer, PVOID user)
{
    struct sffip*       s = (struct sffip*)user;
    DWORD               size, checksum;

    /* FIXME: should check that id/two/three match the file pointed
     * by buffer
     */
    switch (s->kind)
    {
    case DMT_PE:
        {
            HANDLE  hFile, hMap;
            void*   mapping;
            DWORD   timestamp;

            timestamp = ~(DWORD_PTR)s->id;
            size = ~s->two;
            hFile = CreateFileW(buffer, GENERIC_READ, FILE_SHARE_READ, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile == INVALID_HANDLE_VALUE) return FALSE;
            if ((hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != NULL)
            {
                if ((mapping = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) != NULL)
                {
                    IMAGE_NT_HEADERS*   nth = RtlImageNtHeader(mapping);
                    timestamp = nth->FileHeader.TimeDateStamp;
                    size = nth->OptionalHeader.SizeOfImage;
                    UnmapViewOfFile(mapping);
                }
                CloseHandle(hMap);
            }
            CloseHandle(hFile);
            if (timestamp != (DWORD_PTR)s->id || size != s->two)
            {
                WARN("Found %s, but wrong size or timestamp\n", debugstr_w(buffer));
                return FALSE;
            }
        }
        break;
    case DMT_ELF:
        if (elf_fetch_file_info(buffer, 0, &size, &checksum))
        {
            if (checksum != (DWORD_PTR)s->id)
            {
                WARN("Found %s, but wrong checksums: %08x %08lx\n",
                     debugstr_w(buffer), checksum, (DWORD_PTR)s->id);
                return FALSE;
            }
        }
        else
        {
            WARN("Couldn't read %s\n", debugstr_w(buffer));
            return FALSE;
        }
        break;
    case DMT_PDB:
        {
            struct pdb_lookup   pdb_lookup;
            char                fn[MAX_PATH];

            WideCharToMultiByte(CP_ACP, 0, buffer, -1, fn, MAX_PATH, NULL, NULL);
            pdb_lookup.filename = fn;

            if (!pdb_fetch_file_info(&pdb_lookup)) return FALSE;
            switch (pdb_lookup.kind)
            {
            case PDB_JG:
                if (s->flags & SSRVOPT_GUIDPTR)
                {
                    WARN("Found %s, but wrong PDB version\n", debugstr_w(buffer));
                    return FALSE;
                }
                if (pdb_lookup.u.jg.timestamp != (DWORD_PTR)s->id)
                {
                    WARN("Found %s, but wrong signature: %08x %08lx\n",
                         debugstr_w(buffer), pdb_lookup.u.jg.timestamp, (DWORD_PTR)s->id);
                    return FALSE;
                }
                break;
            case PDB_DS:
                if (!(s->flags & SSRVOPT_GUIDPTR))
                {
                    WARN("Found %s, but wrong PDB version\n", debugstr_w(buffer));
                    return FALSE;
                }
                if (memcmp(&pdb_lookup.u.ds.guid, (GUID*)s->id, sizeof(GUID)))
                {
                    WARN("Found %s, but wrong GUID: %s %s\n",
                         debugstr_w(buffer), debugstr_guid(&pdb_lookup.u.ds.guid),
                         debugstr_guid((GUID*)s->id));
                    return FALSE;
                }
                break;
            }
            if (pdb_lookup.age != s->two)
            {
                WARN("Found %s, but wrong age: %08x %08x\n",
                     debugstr_w(buffer), pdb_lookup.age, s->two);
                return FALSE;
            }
        }
        break;
    default:
        FIXME("What the heck??\n");
        return FALSE;
    }
    /* yes, EnumDirTree/do_search and SymFindFileInPath callbacks use the opposite
     * convention to stop/continue enumeration. sigh.
     */
    return !(s->cb)((WCHAR*)buffer, s->user);
}

/******************************************************************
 *		SymFindFileInPathW (DBGHELP.@)
 *
 */
BOOL WINAPI SymFindFileInPathW(HANDLE hProcess, PCWSTR searchPath, PCWSTR full_path,
                               PVOID id, DWORD two, DWORD three, DWORD flags,
                               PWSTR buffer, PFINDFILEINPATHCALLBACKW cb,
                               PVOID user)
{
    struct sffip        s;
    struct process*     pcs = process_find_by_handle(hProcess);
    WCHAR               tmp[MAX_PATH];
    WCHAR*              ptr;
    const WCHAR*        filename;

    TRACE("(%p %s %s %p %08x %08x %08x %p %p %p)\n",
          hProcess, debugstr_w(searchPath), debugstr_w(full_path),
          id, two, three, flags, buffer, cb, user);

    if (!pcs) return FALSE;
    if (!searchPath) searchPath = pcs->search_path;

    s.id = id;
    s.two = two;
    s.three = three;
    s.flags = flags;
    s.cb = cb;
    s.user = user;

    filename = file_nameW(full_path);
    s.kind = module_get_type_by_name(filename);

    /* first check full path to file */
    if (sffip_cb(full_path, &s))
    {
        strcpyW(buffer, full_path);
        return TRUE;
    }

    while (searchPath)
    {
        ptr = strchrW(searchPath, ';');
        if (ptr)
        {
            memcpy(tmp, searchPath, (ptr - searchPath) * sizeof(WCHAR));
            tmp[ptr - searchPath] = 0;
            searchPath = ptr + 1;
        }
        else
        {
            strcpyW(tmp, searchPath);
            searchPath = NULL;
        }
        if (do_searchW(filename, tmp, FALSE, sffip_cb, &s))
        {
            strcpyW(buffer, tmp);
            return TRUE;
        }
    }
    return FALSE;
}

/******************************************************************
 *		SymFindFileInPath (DBGHELP.@)
 *
 */
BOOL WINAPI SymFindFileInPath(HANDLE hProcess, PCSTR searchPath, PCSTR full_path,
                              PVOID id, DWORD two, DWORD three, DWORD flags,
                              PSTR buffer, PFINDFILEINPATHCALLBACK cb,
                              PVOID user)
{
    WCHAR                       searchPathW[MAX_PATH];
    WCHAR                       full_pathW[MAX_PATH];
    WCHAR                       bufferW[MAX_PATH];
    struct enum_dir_treeWA      edt;
    BOOL                        ret;

    /* a PFINDFILEINPATHCALLBACK and a PENUMDIRTREE_CALLBACK have actually the
     * same signature & semantics, hence we can reuse the EnumDirTree W->A
     * conversion helper
     */
    edt.cb = cb;
    edt.user = user;
    if (searchPath)
        MultiByteToWideChar(CP_ACP, 0, searchPath, -1, searchPathW, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, full_path, -1, full_pathW, MAX_PATH);
    if ((ret =  SymFindFileInPathW(hProcess, searchPath ? searchPathW : NULL, full_pathW,
                                   id, two, three, flags,
                                   bufferW, enum_dir_treeWA, &edt)))
        WideCharToMultiByte(CP_ACP, 0, bufferW, -1, buffer, MAX_PATH, NULL, NULL);
    return ret;
}
