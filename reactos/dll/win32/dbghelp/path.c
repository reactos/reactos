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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dbghelp_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

static __inline BOOL is_sep(char ch) {return ch == '/' || ch == '\\';}

static __inline char* file_name(char* str)
{
    char*       p;

    for (p = str + strlen(str) - 1; p >= str && !is_sep(*p); p--);
    return p + 1;
}

/******************************************************************
 *		FindDebugInfoFile (DBGHELP.@)
 *
 */
HANDLE WINAPI FindDebugInfoFile(PSTR FileName, PSTR SymbolPath, PSTR DebugFilePath)
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
HANDLE WINAPI FindDebugInfoFileEx(PSTR FileName, PSTR SymbolPath,
                                  PSTR DebugFilePath,
                                  PFIND_DEBUG_FILE_CALLBACK Callback,
                                  PVOID CallerData)
{
    FIXME("(%s %s %p %p %p): stub\n",
          FileName, SymbolPath, DebugFilePath, Callback, CallerData);
    return NULL;
}

/******************************************************************
 *		FindExecutableImage (DBGHELP.@)
 *
 */
HANDLE WINAPI FindExecutableImage(PSTR FileName, PSTR SymbolPath, PSTR ImageFilePath)
{
    HANDLE h;
    if (!SearchPathA(SymbolPath, FileName, NULL, MAX_PATH, ImageFilePath, NULL))
        return NULL;
    h = CreateFileA(ImageFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    return (h == INVALID_HANDLE_VALUE) ? NULL : h;
}

/***********************************************************************
 *           MakeSureDirectoryPathExists (DBGHELP.@)
 */
BOOL WINAPI MakeSureDirectoryPathExists(LPCSTR DirPath)
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
 *		SymMatchFileName (DBGHELP.@)
 *
 */
BOOL WINAPI SymMatchFileName(char* file, char* match,
                             char** filestop, char** matchstop)
{
    char*       fptr;
    char*       mptr;

    TRACE("(%s %s %p %p)\n", file, match, filestop, matchstop);

    fptr = file + strlen(file) - 1;
    mptr = match + strlen(match) - 1;

    while (fptr >= file && mptr >= match)
    {
        if (toupper(*fptr) != toupper(*mptr) && !(is_sep(*fptr) && is_sep(*mptr)))
            break;
        fptr--; mptr--;
    }
    if (filestop) *filestop = fptr;
    if (matchstop) *matchstop = mptr;

    return mptr == match - 1;
}

static BOOL do_search(const char* file, char* buffer,
                      PENUMDIRTREE_CALLBACK cb, void* user)
{
    HANDLE              h;
    WIN32_FIND_DATAA    fd;
    unsigned            pos;
    BOOL                found = FALSE;

    pos = strlen(buffer);
    if (buffer[pos - 1] != '\\') buffer[pos++] = '\\';
    strcpy(buffer + pos, "*.*");
    if ((h = FindFirstFileA(buffer, &fd)) == INVALID_HANDLE_VALUE)
        return FALSE;
    /* doc doesn't specify how the tree is enumerated...
     * doing a depth first based on, but may be wrong
     */
    do
    {
        if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..")) continue;

        strcpy(buffer + pos, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            found = do_search(file, buffer, cb, user);
        else if (SymMatchFileName(buffer, (char*)file, NULL, NULL))
        {
            if (!cb || cb(buffer, user)) found = TRUE;
        }
    } while (!found && FindNextFileA(h, &fd));
    if (!found) buffer[--pos] = '\0';
    FindClose(h);

    return found;
}

/***********************************************************************
 *           SearchTreeForFile (DBGHELP.@)
 */
BOOL WINAPI SearchTreeForFile(LPSTR root, LPSTR file, LPSTR buffer)
{
    TRACE("(%s, %s, %p)\n",
          debugstr_a(root), debugstr_a(file), buffer);
    strcpy(buffer, root);
    return do_search(file, buffer, NULL, NULL);
}

/******************************************************************
 *		EnumDirTree (DBGHELP.@)
 *
 *
 */
BOOL WINAPI EnumDirTree(HANDLE hProcess, PCSTR root, PCSTR file,
                        LPSTR buffer, PENUMDIRTREE_CALLBACK cb, PVOID user)
{
    TRACE("(%p %s %s %p %p %p)\n", hProcess, root, file, buffer, cb, user);

    strcpy(buffer, root);
    return do_search(file, buffer, cb, user);
}

struct sffip
{
    PVOID                       id;
    DWORD                       two;
    DWORD                       three;
    DWORD                       flags;
    PFINDFILEINPATHCALLBACK     cb;
    void*                       user;
};

static BOOL CALLBACK sffip_cb(LPCSTR buffer, void* user)
{
    struct sffip*       s = (struct sffip*)user;

    /* FIXME: should check that id/two/three match the file pointed
     * by buffer
     */
    /* yes, EnumDirTree and SymFindFileInPath callbacks use the opposite
     * convention to stop/continue enumeration. sigh.
     */
    return !(s->cb)((char*)buffer, s->user);
}

/******************************************************************
 *		SymFindFileInPath (DBGHELP.@)
 *
 */
BOOL WINAPI SymFindFileInPath(HANDLE hProcess, LPSTR searchPath, LPSTR file,
                              PVOID id, DWORD two, DWORD three, DWORD flags,
                              LPSTR buffer, PFINDFILEINPATHCALLBACK cb,
                              PVOID user)
{
    struct sffip        s;
    struct process*     pcs = process_find_by_handle(hProcess);
    char                tmp[MAX_PATH];
    char*               ptr;

    TRACE("(%p %s %s %p %08lx %08lx %08lx %p %p %p)\n",
          hProcess, searchPath, file, id, two, three, flags,
          buffer, cb, user);

    if (!pcs) return FALSE;
    if (!searchPath) searchPath = pcs->search_path;

    s.id = id;
    s.two = two;
    s.three = three;
    s.flags = flags;
    s.cb = cb;
    s.user = user;

    file = file_name(file);

    while (searchPath)
    {
        ptr = strchr(searchPath, ';');
        if (ptr)
        {
            memcpy(tmp, searchPath, ptr - searchPath);
            tmp[ptr - searchPath] = 0;
            searchPath = ptr + 1;
        }
        else
        {
            strcpy(tmp, searchPath);
            searchPath = NULL;
        }
        if (EnumDirTree(hProcess, tmp, file, buffer, sffip_cb, &s)) return TRUE;
    }
    return FALSE;
}
