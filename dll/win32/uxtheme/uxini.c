/*
 * Win32 5.1 uxtheme ini file processing
 *
 * Copyright (C) 2004 Kevin Koltzau
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxtheme);

/***********************************************************************
 * Defines and global variables
 */

static const WCHAR szTextFileResource[] = {
    'T','E','X','T','F','I','L','E','\0'
};

typedef struct _UXINI_FILE {
    LPCWSTR lpIni;
    LPCWSTR lpCurLoc;
    LPCWSTR lpEnd;
} UXINI_FILE, *PUXINI_FILE;

/***********************************************************************/

/**********************************************************************
 *      UXINI_LoadINI
 *
 * Load a theme INI file out of resources from the specified
 * theme
 *
 * PARAMS
 *     tf                  Theme to load INI file out of resources
 *     lpName              Resource name of the INI file
 *
 * RETURNS
 *     INI file, or NULL if not found
 */
PUXINI_FILE UXINI_LoadINI(HMODULE hTheme, LPCWSTR lpName) {
    HRSRC hrsc;
    LPCWSTR lpThemesIni = NULL;
    PUXINI_FILE uf;
    DWORD dwIniSize;

    TRACE("Loading resource INI %s\n", debugstr_w(lpName));

    if((hrsc = FindResourceW(hTheme, lpName, szTextFileResource))) {
        if(!(lpThemesIni = LoadResource(hTheme, hrsc))) {
            TRACE("%s resource not found\n", debugstr_w(lpName));
            return NULL;
        }
    }

    dwIniSize = SizeofResource(hTheme, hrsc) / sizeof(WCHAR);
    uf = HeapAlloc(GetProcessHeap(), 0, sizeof(UXINI_FILE));
    uf->lpIni = lpThemesIni;
    uf->lpCurLoc = lpThemesIni;
    uf->lpEnd = lpThemesIni + dwIniSize;
    return uf;
}

/**********************************************************************
 *      UXINI_CloseINI
 *
 * Close an open theme INI file
 *
 * PARAMS
 *     uf                  Theme INI file to close
 */
void UXINI_CloseINI(PUXINI_FILE uf)
{
    HeapFree(GetProcessHeap(), 0, uf);
}

/**********************************************************************
 *      UXINI_eof
 *
 * Determines if we are at the end of the INI file
 *
 * PARAMS
 *     uf                  Theme INI file to test
 */
static inline BOOL UXINI_eof(PUXINI_FILE uf)
{
    return uf->lpCurLoc >= uf->lpEnd;
}

/**********************************************************************
 *      UXINI_isspace
 *
 * Check if a character is a space character
 *
 * PARAMS
 *     c                   Character to test
 */
static inline BOOL UXINI_isspace(WCHAR c)
{
    if (isspace(c)) return TRUE;
    if (c=='\r') return TRUE;
    return FALSE;
}

/**********************************************************************
 *      UXINI_GetNextLine
 *
 * Get the next line in the INI file, non NULL terminated
 * removes whitespace at beginning and end of line, and removes comments
 *
 * PARAMS
 *     uf                  INI file to retrieve next line
 *     dwLen               Location to store pointer to line length
 *
 * RETURNS
 *     The section name, non NULL terminated
 */
static LPCWSTR UXINI_GetNextLine(PUXINI_FILE uf, DWORD *dwLen)
{
    LPCWSTR lpLineEnd;
    LPCWSTR lpLineStart;
    DWORD len;
    do {
        if(UXINI_eof(uf)) return NULL;
        /* Skip whitespace and empty lines */
        while(!UXINI_eof(uf) && (UXINI_isspace(*uf->lpCurLoc) || *uf->lpCurLoc == '\n')) uf->lpCurLoc++;
        lpLineStart = uf->lpCurLoc;
        lpLineEnd = uf->lpCurLoc;
        while(!UXINI_eof(uf) && *uf->lpCurLoc != '\n' && *uf->lpCurLoc != ';') lpLineEnd = ++uf->lpCurLoc;
        /* If comment was found, skip the rest of the line */
        if(*uf->lpCurLoc == ';')
            while(!UXINI_eof(uf) && *uf->lpCurLoc != '\n') uf->lpCurLoc++;
        len = (lpLineEnd - lpLineStart);
        if(*lpLineStart != ';' && len == 0)
            return NULL;
    } while(*lpLineStart == ';');
    /* Remove whitespace from end of line */
    while(UXINI_isspace(lpLineStart[len-1])) len--;
    *dwLen = len;

    return lpLineStart;
}

static inline void UXINI_UnGetToLine(PUXINI_FILE uf, LPCWSTR lpLine)
{
    uf->lpCurLoc = lpLine;
}

/**********************************************************************
 *      UXINI_GetNextSection
 *
 * Locate the next section in the ini file, and return pointer to
 * section name, non NULL terminated. Use dwLen to determine length
 *
 * PARAMS
 *     uf                  INI file to search, search starts at current location
 *     dwLen               Location to store pointer to section name length
 *
 * RETURNS
 *     The section name, non NULL terminated
 */
LPCWSTR UXINI_GetNextSection(PUXINI_FILE uf, DWORD *dwLen)
{
    LPCWSTR lpLine;
    while((lpLine = UXINI_GetNextLine(uf, dwLen))) {
        /* Assuming a ']' ending to the section name */
        if(lpLine[0] == '[') {
            lpLine++;
            *dwLen -= 2;
            break;
        }
    }
    return lpLine;
}

/**********************************************************************
 *      UXINI_FindSection
 *
 * Locate a section with the specified name, search starts
 * at current location in ini file
 *
 * PARAMS
 *     uf                  INI file to search, search starts at current location
 *     lpName              Name of the section to locate
 *
 * RETURNS
 *     TRUE if section was found, FALSE otherwise
 */
BOOL UXINI_FindSection(PUXINI_FILE uf, LPCWSTR lpName)
{
    LPCWSTR lpSection;
    DWORD dwLen;
    while((lpSection = UXINI_GetNextSection(uf, &dwLen))) {
        if(CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, lpSection, dwLen, lpName, -1) == CSTR_EQUAL) {
            return TRUE;
        }
    }
    return FALSE;
}

/**********************************************************************
 *      UXINI_GetNextValue
 *
 * Locate the next value in the current section
 *
 * PARAMS
 *     uf                  INI file to search, search starts at current location
 *     dwNameLen            Location to store pointer to value name length
 *     lpValue              Location to store pointer to the value
 *     dwValueLen           Location to store pointer to value length
 *
 * RETURNS
 *     The value name, non NULL terminated
 */
LPCWSTR UXINI_GetNextValue(PUXINI_FILE uf, DWORD *dwNameLen, LPCWSTR *lpValue, DWORD *dwValueLen)
{
    LPCWSTR lpLine;
    LPCWSTR lpLineEnd;
    LPCWSTR name = NULL;
    LPCWSTR value = NULL;
    DWORD vallen = 0;
    DWORD namelen = 0;
    DWORD dwLen;
    lpLine = UXINI_GetNextLine(uf, &dwLen);
    if(!lpLine)
        return NULL;
    if(lpLine[0] == '[') {
        UXINI_UnGetToLine(uf, lpLine);
        return NULL;
    }
    lpLineEnd = lpLine + dwLen;

    name = lpLine;
    while(namelen < dwLen && *lpLine != '=') {
        lpLine++;
        namelen++;
    }
    if(*lpLine != '=') return NULL;
    lpLine++;

    /* Remove whitespace from end of name */
    while(UXINI_isspace(name[namelen-1])) namelen--;
    /* Remove whitespace from beginning of value */
    while(UXINI_isspace(*lpLine) && lpLine < lpLineEnd) lpLine++;
    value = lpLine;
    vallen = dwLen-(value-name);

    *dwNameLen = namelen;
    *dwValueLen = vallen;
    *lpValue = value;

    return name;
}

/**********************************************************************
 *      UXINI_FindValue
 *
 * Locate a value by name
 *
 * PARAMS
 *     uf                   INI file to search, search starts at current location
 *     lpName               Value name to locate
 *     lpValue              Location to store pointer to the value
 *     dwValueLen           Location to store pointer to value length
 *
 * RETURNS
 *     The value name, non NULL terminated
 */
BOOL UXINI_FindValue(PUXINI_FILE uf, LPCWSTR lpName, LPCWSTR *lpValue, DWORD *dwValueLen)
{
    LPCWSTR name;
    DWORD namelen;

    while((name = UXINI_GetNextValue(uf, &namelen, lpValue, dwValueLen))) {
        if(CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, namelen, lpName, -1) == CSTR_EQUAL) {
            return TRUE;
        }
    }
    return FALSE;
}
