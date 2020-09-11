/*
 * File path.c - managing path in debugging environments
 *
 * Copyright (C) 2004,2008, Eric Pouech
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

static inline void file_pathW(const WCHAR *src, WCHAR *dst)
{
    int len;

    for (len = strlenW(src) - 1; (len > 0) && (!is_sepW(src[len])); len--);
    memcpy( dst, src, len * sizeof(WCHAR) );
    dst[len] = 0;
}

/******************************************************************
 *		FindDebugInfoFile (DBGHELP.@)
 *
 */
HANDLE WINAPI FindDebugInfoFile(PCSTR FileName, PCSTR SymbolPath, PSTR DebugFilePath)
{
    HANDLE      h;

    h = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, NULL,
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
    FIXME("(%s %s %s %p %p): stub\n", debugstr_a(FileName), debugstr_a(SymbolPath),
            debugstr_a(DebugFilePath), Callback, CallerData);
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

    TRACE("(%s %s %p %p)\n", debugstr_a(file), debugstr_a(match), filestop, matchstop);

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
    static const WCHAR  S_DotDotW[] = {'.','.','\0'};

    pos = strlenW(buffer);
    if (pos == 0) return FALSE;
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
        else if (SymMatchFileNameW(buffer, file, NULL, NULL))
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
    struct sffip*       s = user;

    if (!s->cb) return TRUE;
    /* yes, EnumDirTree/do_search and SymFindFileInPath callbacks use the opposite
     * convention to stop/continue enumeration. sigh.
     */
    return !(s->cb)(buffer, s->user);
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

    TRACE("(hProcess = %p, searchPath = %s, full_path = %s, id = %p, two = 0x%08x, three = 0x%08x, flags = 0x%08x, buffer = %p, cb = %p, user = %p)\n",
          hProcess, debugstr_w(searchPath), debugstr_w(full_path),
          id, two, three, flags, buffer, cb, user);

    if (!pcs) return FALSE;
    if (!searchPath) searchPath = pcs->search_path;

    s.cb = cb;
    s.user = user;

    filename = file_nameW(full_path);

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

struct module_find
{
    enum module_type            kind;
    /* pe:  dw1         DWORD:timestamp
     *      dw2         size of image (from PE header)
     * pdb: guid        PDB guid (if DS PDB file)
     *      or dw1      PDB timestamp (if JG PDB file)
     *      dw2         PDB age
     * elf: dw1         DWORD:CRC 32 of ELF image (Wine only)
     */
    const GUID*                 guid;
    DWORD                       dw1;
    DWORD                       dw2;
    WCHAR                       filename[MAX_PATH];
    unsigned                    matched;
};

/* checks that buffer (as found by matching the name) matches the info
 * (information is based on file type)
 * returns TRUE when file is found, FALSE to continue searching
 * (NB this is the opposite convention of SymFindFileInPathProc)
 */
static BOOL CALLBACK module_find_cb(PCWSTR buffer, PVOID user)
{
    struct module_find* mf = user;
    DWORD               size, checksum, timestamp;
    unsigned            matched = 0;

    /* the matching weights:
     * +1 if a file with same name is found and is a decent file of expected type
     * +1 if first parameter and second parameter match
     */

    /* FIXME: should check that id/two match the file pointed
     * by buffer
     */
    switch (mf->kind)
    {
    case DMT_PE:
        {
            HANDLE  hFile, hMap;
            void*   mapping;

            timestamp = ~mf->dw1;
            size = ~mf->dw2;
            hFile = CreateFileW(buffer, GENERIC_READ, FILE_SHARE_READ, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile == INVALID_HANDLE_VALUE) return FALSE;
            if ((hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != NULL)
            {
                if ((mapping = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) != NULL)
                {
                    IMAGE_NT_HEADERS*   nth = RtlImageNtHeader(mapping);
                    if (!nth)
                    {
                        UnmapViewOfFile(mapping);
                        CloseHandle(hMap);
                        CloseHandle(hFile);
                        return FALSE;
                    }
                    matched++;
                    timestamp = nth->FileHeader.TimeDateStamp;
                    size = nth->OptionalHeader.SizeOfImage;
                    UnmapViewOfFile(mapping);
                }
                CloseHandle(hMap);
            }
            CloseHandle(hFile);
            if (timestamp != mf->dw1)
                WARN("Found %s, but wrong timestamp\n", debugstr_w(buffer));
            if (size != mf->dw2)
                WARN("Found %s, but wrong size\n", debugstr_w(buffer));
            if (timestamp == mf->dw1 && size == mf->dw2) matched++;
        }
        break;
    case DMT_ELF:
        if (elf_fetch_file_info(buffer, 0, &size, &checksum))
        {
            matched++;
            if (checksum == mf->dw1) matched++;
            else
                WARN("Found %s, but wrong checksums: %08x %08x\n",
                     debugstr_w(buffer), checksum, mf->dw1);
        }
        else
        {
            WARN("Couldn't read %s\n", debugstr_w(buffer));
            return FALSE;
        }
        break;
    case DMT_MACHO:
        if (macho_fetch_file_info(NULL, buffer, 0, 0, &size, &checksum))
        {
            matched++;
            if (checksum == mf->dw1) matched++;
            else
                WARN("Found %s, but wrong checksums: %08x %08x\n",
                     debugstr_w(buffer), checksum, mf->dw1);
        }
        else
        {
            WARN("Couldn't read %s\n", debugstr_w(buffer));
            return FALSE;
        }
        break;
    case DMT_PDB:
        {
            struct pdb_lookup           pdb_lookup;
            char                        fn[MAX_PATH];

            WideCharToMultiByte(CP_ACP, 0, buffer, -1, fn, MAX_PATH, NULL, NULL);
            pdb_lookup.filename = fn;

            if (mf->guid)
            {
                pdb_lookup.kind = PDB_DS;
                pdb_lookup.timestamp = 0;
                pdb_lookup.guid = *mf->guid;
            }
            else
            {
                pdb_lookup.kind = PDB_JG;
                pdb_lookup.timestamp = mf->dw1;
                /* pdb_loopkup.guid = */
            }
            pdb_lookup.age = mf->dw2;

            if (!pdb_fetch_file_info(&pdb_lookup, &matched)) return FALSE;
        }
        break;
    case DMT_DBG:
        {
            HANDLE  hFile, hMap;
            void*   mapping;

            timestamp = ~mf->dw1;
            hFile = CreateFileW(buffer, GENERIC_READ, FILE_SHARE_READ, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile == INVALID_HANDLE_VALUE) return FALSE;
            if ((hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != NULL)
            {
                if ((mapping = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) != NULL)
                {
                    const IMAGE_SEPARATE_DEBUG_HEADER*  hdr;
                    hdr = mapping;

                    if (hdr->Signature == IMAGE_SEPARATE_DEBUG_SIGNATURE)
                    {
                        matched++;
                        timestamp = hdr->TimeDateStamp;
                    }
                    UnmapViewOfFile(mapping);
                }
                CloseHandle(hMap);
            }
            CloseHandle(hFile);
            if (timestamp == mf->dw1) matched++;
            else WARN("Found %s, but wrong timestamp\n", debugstr_w(buffer));
        }
        break;
    default:
        FIXME("What the heck??\n");
        return FALSE;
    }
    if (matched > mf->matched)
    {
        strcpyW(mf->filename, buffer);
        mf->matched = matched;
    }
    /* yes, EnumDirTree/do_search and SymFindFileInPath callbacks use the opposite
     * convention to stop/continue enumeration. sigh.
     */
    return mf->matched == 2;
}

BOOL path_find_symbol_file(const struct process* pcs, const struct module* module,
                           PCSTR full_path, const GUID* guid, DWORD dw1, DWORD dw2,
                           WCHAR *buffer, BOOL* is_unmatched)
{
    struct module_find  mf;
    WCHAR               full_pathW[MAX_PATH];
    WCHAR*              ptr;
    const WCHAR*        filename;
    WCHAR*              searchPath = pcs->search_path;

    TRACE("(pcs = %p, full_path = %s, guid = %s, dw1 = 0x%08x, dw2 = 0x%08x, buffer = %p)\n",
          pcs, debugstr_a(full_path), debugstr_guid(guid), dw1, dw2, buffer);

    mf.guid = guid;
    mf.dw1 = dw1;
    mf.dw2 = dw2;
    mf.matched = 0;

    MultiByteToWideChar(CP_ACP, 0, full_path, -1, full_pathW, MAX_PATH);
    filename = file_nameW(full_pathW);
    mf.kind = module_get_type_by_name(filename);
    *is_unmatched = FALSE;

    /* first check full path to file */
    if (module_find_cb(full_pathW, &mf))
    {
        strcpyW( buffer, full_pathW );
        return TRUE;
    }

    /* FIXME: Use Environment-Variables (see MS docs)
                 _NT_SYMBOL_PATH and _NT_ALT_SYMBOL_PATH
       FIXME: Implement "Standard Path Elements" (Path) ... (see MS docs)
              do a search for (every?) path-element like this ...
              <path>
              <path>\dll
              <path>\symbols\dll
              (dll may be exe, or sys depending on the file extension)   */

    /* 2. check module-path */
    file_pathW(module->module.LoadedImageName, buffer);
    if (do_searchW(filename, buffer, FALSE, module_find_cb, &mf)) return TRUE;

    while (searchPath)
    {
        ptr = strchrW(searchPath, ';');
        if (ptr)
        {
            memcpy(buffer, searchPath, (ptr - searchPath) * sizeof(WCHAR));
            buffer[ptr - searchPath] = '\0';
            searchPath = ptr + 1;
        }
        else
        {
            strcpyW(buffer, searchPath);
            searchPath = NULL;
        }
        /* return first fully matched file */
        if (do_searchW(filename, buffer, FALSE, module_find_cb, &mf)) return TRUE;
    }
    /* if no fully matching file is found, return the best matching file if any */
    if ((dbghelp_options & SYMOPT_LOAD_ANYTHING) && mf.matched)
    {
        strcpyW( buffer, mf.filename );
        *is_unmatched = TRUE;
        return TRUE;
    }
    return FALSE;
}
