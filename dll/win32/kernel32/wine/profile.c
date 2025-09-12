/*
 * Profile functions
 *
 * Copyright 1993 Miguel de Icaza
 * Copyright 1996 Alexandre Julliard
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

#ifdef __REACTOS__

#include <k32.h>

#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(profile);

#else /* __REACTOS__ */

#include "config.h"
#include "wine/port.h"

#include <string.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winerror.h"
#include "winternl.h"
#include "wine/unicode.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(profile);

#endif /* __REACTOS__ */

static const char bom_utf8[] = {0xEF,0xBB,0xBF};

typedef enum
{
    ENCODING_ANSI = 1,
    ENCODING_UTF8,
    ENCODING_UTF16LE,
    ENCODING_UTF16BE
} ENCODING;

typedef struct tagPROFILEKEY
{
    WCHAR                 *value;
    struct tagPROFILEKEY  *next;
    WCHAR                  name[1];
} PROFILEKEY;

typedef struct tagPROFILESECTION
{
    struct tagPROFILEKEY       *key;
    struct tagPROFILESECTION   *next;
    WCHAR                       name[1];
} PROFILESECTION;


typedef struct
{
    BOOL             changed;
    PROFILESECTION  *section;
    WCHAR           *filename;
    FILETIME LastWriteTime;
    ENCODING encoding;
} PROFILE;


#define N_CACHED_PROFILES 10

/* Cached profile files */
static PROFILE *MRUProfile[N_CACHED_PROFILES]={NULL};

#define CurProfile (MRUProfile[0])

/* Check for comments in profile */
#define IS_ENTRY_COMMENT(str)  ((str)[0] == ';')

static const WCHAR emptystringW[] = {0};
static const WCHAR wininiW[] = { 'w','i','n','.','i','n','i',0 };

#ifdef __REACTOS__

static RTL_CRITICAL_SECTION PROFILE_CritSect;
static RTL_CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &PROFILE_CritSect,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { 0 }
};
static RTL_CRITICAL_SECTION PROFILE_CritSect = { &critsect_debug, -1, 0, 0, 0, 0 };

#else /* __REACTOS__ */

static CRITICAL_SECTION PROFILE_CritSect;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &PROFILE_CritSect,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": PROFILE_CritSect") }
};
static CRITICAL_SECTION PROFILE_CritSect = { &critsect_debug, -1, 0, 0, 0, 0 };

#endif /* __REACTOS__ */

static const char hex[16] = "0123456789ABCDEF";

/***********************************************************************
 *           PROFILE_CopyEntry
 *
 * Copy the content of an entry into a buffer, removing quotes, and possibly
 * translating environment variables.
 */
static void PROFILE_CopyEntry( LPWSTR buffer, LPCWSTR value, int len,
                               BOOL strip_quote )
{
    WCHAR quote = '\0';

    if(!buffer) return;

    if (strip_quote && ((*value == '\'') || (*value == '\"')))
    {
        if (value[1] && (value[strlenW(value)-1] == *value)) quote = *value++;
    }

    lstrcpynW( buffer, value, len );
    if (quote && (len >= lstrlenW(value))) buffer[strlenW(buffer)-1] = '\0';
}

/* byte-swaps shorts in-place in a buffer. len is in WCHARs */
static inline void PROFILE_ByteSwapShortBuffer(WCHAR * buffer, int len)
{
    int i;
    USHORT * shortbuffer = buffer;
    for (i = 0; i < len; i++)
        shortbuffer[i] = RtlUshortByteSwap(shortbuffer[i]);
}

/* writes any necessary encoding marker to the file */
static inline void PROFILE_WriteMarker(HANDLE hFile, ENCODING encoding)
{
    DWORD dwBytesWritten;
    WCHAR bom;
    switch (encoding)
    {
    case ENCODING_ANSI:
        break;
    case ENCODING_UTF8:
        WriteFile(hFile, bom_utf8, sizeof(bom_utf8), &dwBytesWritten, NULL);
        break;
    case ENCODING_UTF16LE:
        bom = 0xFEFF;
        WriteFile(hFile, &bom, sizeof(bom), &dwBytesWritten, NULL);
        break;
    case ENCODING_UTF16BE:
        bom = 0xFFFE;
        WriteFile(hFile, &bom, sizeof(bom), &dwBytesWritten, NULL);
        break;
    }
}

static void PROFILE_WriteLine( HANDLE hFile, WCHAR * szLine, int len, ENCODING encoding)
{
    char * write_buffer;
    int write_buffer_len;
    DWORD dwBytesWritten;

    TRACE("writing: %s\n", debugstr_wn(szLine, len));

    switch (encoding)
    {
    case ENCODING_ANSI:
        write_buffer_len = WideCharToMultiByte(CP_ACP, 0, szLine, len, NULL, 0, NULL, NULL);
        write_buffer = HeapAlloc(GetProcessHeap(), 0, write_buffer_len);
        if (!write_buffer) return;
        len = WideCharToMultiByte(CP_ACP, 0, szLine, len, write_buffer, write_buffer_len, NULL, NULL);
        WriteFile(hFile, write_buffer, len, &dwBytesWritten, NULL);
        HeapFree(GetProcessHeap(), 0, write_buffer);
        break;
    case ENCODING_UTF8:
        write_buffer_len = WideCharToMultiByte(CP_UTF8, 0, szLine, len, NULL, 0, NULL, NULL);
        write_buffer = HeapAlloc(GetProcessHeap(), 0, write_buffer_len);
        if (!write_buffer) return;
        len = WideCharToMultiByte(CP_UTF8, 0, szLine, len, write_buffer, write_buffer_len, NULL, NULL);
        WriteFile(hFile, write_buffer, len, &dwBytesWritten, NULL);
        HeapFree(GetProcessHeap(), 0, write_buffer);
        break;
    case ENCODING_UTF16LE:
        WriteFile(hFile, szLine, len * sizeof(WCHAR), &dwBytesWritten, NULL);
        break;
    case ENCODING_UTF16BE:
        PROFILE_ByteSwapShortBuffer(szLine, len);
        WriteFile(hFile, szLine, len * sizeof(WCHAR), &dwBytesWritten, NULL);
        break;
    default:
        FIXME("encoding type %d not implemented\n", encoding);
    }
}

/***********************************************************************
 *           PROFILE_Save
 *
 * Save a profile tree to a file.
 */
static void PROFILE_Save( HANDLE hFile, const PROFILESECTION *section, ENCODING encoding )
{
    PROFILEKEY *key;
    WCHAR *buffer, *p;

    PROFILE_WriteMarker(hFile, encoding);

    for ( ; section; section = section->next)
    {
        int len = 0;

        if (section->name[0]) len += strlenW(section->name) + 4;

        for (key = section->key; key; key = key->next)
        {
            len += strlenW(key->name) + 2;
            if (key->value) len += strlenW(key->value) + 1;
        }

        buffer = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!buffer) return;

        p = buffer;
        if (section->name[0])
        {
            *p++ = '[';
            strcpyW( p, section->name );
            p += strlenW(p);
            *p++ = ']';
            *p++ = '\r';
            *p++ = '\n';
        }

        for (key = section->key; key; key = key->next)
        {
            strcpyW( p, key->name );
            p += strlenW(p);
            if (key->value)
            {
                *p++ = '=';
                strcpyW( p, key->value );
                p += strlenW(p);
            }
            *p++ = '\r';
            *p++ = '\n';
        }
        PROFILE_WriteLine( hFile, buffer, len, encoding );
        HeapFree(GetProcessHeap(), 0, buffer);
    }
}


/***********************************************************************
 *           PROFILE_Free
 *
 * Free a profile tree.
 */
static void PROFILE_Free( PROFILESECTION *section )
{
    PROFILESECTION *next_section;
    PROFILEKEY *key, *next_key;

    for ( ; section; section = next_section)
    {
        for (key = section->key; key; key = next_key)
        {
            next_key = key->next;
            HeapFree( GetProcessHeap(), 0, key->value );
            HeapFree( GetProcessHeap(), 0, key );
        }
        next_section = section->next;
        HeapFree( GetProcessHeap(), 0, section );
    }
}

/* returns TRUE if a whitespace character, else FALSE */
static inline BOOL PROFILE_isspaceW(WCHAR c)
{
	/* ^Z (DOS EOF) is a space too  (found on CD-ROMs) */
	return isspaceW(c) || c == 0x1a;
}

static inline ENCODING PROFILE_DetectTextEncoding(const void * buffer, int * len)
{
    int flags = IS_TEXT_UNICODE_SIGNATURE |
                IS_TEXT_UNICODE_REVERSE_SIGNATURE |
                IS_TEXT_UNICODE_ODD_LENGTH;
    if (*len >= sizeof(bom_utf8) && !memcmp(buffer, bom_utf8, sizeof(bom_utf8)))
    {
        *len = sizeof(bom_utf8);
        return ENCODING_UTF8;
    }
    RtlIsTextUnicode(buffer, *len, &flags);
    if (flags & IS_TEXT_UNICODE_SIGNATURE)
    {
        *len = sizeof(WCHAR);
        return ENCODING_UTF16LE;
    }
    if (flags & IS_TEXT_UNICODE_REVERSE_SIGNATURE)
    {
        *len = sizeof(WCHAR);
        return ENCODING_UTF16BE;
    }
    *len = 0;
    return ENCODING_ANSI;
}


/***********************************************************************
 *           PROFILE_Load
 *
 * Load a profile tree from a file.
 */
static PROFILESECTION *PROFILE_Load(HANDLE hFile, ENCODING * pEncoding)
{
    void *buffer_base, *pBuffer;
    WCHAR * szFile;
    const WCHAR *szLineStart, *szLineEnd;
    const WCHAR *szValueStart, *szEnd, *next_line;
    int line = 0, len;
    PROFILESECTION *section, *first_section;
    PROFILESECTION **next_section;
    PROFILEKEY *key, *prev_key, **next_key;
    DWORD dwFileSize;
    
    TRACE("%p\n", hFile);
    
    dwFileSize = GetFileSize(hFile, NULL);
    if (dwFileSize == INVALID_FILE_SIZE || dwFileSize == 0)
        return NULL;

    buffer_base = HeapAlloc(GetProcessHeap(), 0 , dwFileSize);
    if (!buffer_base) return NULL;
    
    if (!ReadFile(hFile, buffer_base, dwFileSize, &dwFileSize, NULL))
    {
        HeapFree(GetProcessHeap(), 0, buffer_base);
        WARN("Error %d reading file\n", GetLastError());
        return NULL;
    }
    len = dwFileSize;
    *pEncoding = PROFILE_DetectTextEncoding(buffer_base, &len);
    /* len is set to the number of bytes in the character marker.
     * we want to skip these bytes */
    pBuffer = (char *)buffer_base + len;
    dwFileSize -= len;
    switch (*pEncoding)
    {
    case ENCODING_ANSI:
        TRACE("ANSI encoding\n");

        len = MultiByteToWideChar(CP_ACP, 0, pBuffer, dwFileSize, NULL, 0);
        szFile = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!szFile)
        {
            HeapFree(GetProcessHeap(), 0, buffer_base);
            return NULL;
        }
        MultiByteToWideChar(CP_ACP, 0, pBuffer, dwFileSize, szFile, len);
        szEnd = szFile + len;
        break;
    case ENCODING_UTF8:
        TRACE("UTF8 encoding\n");

        len = MultiByteToWideChar(CP_UTF8, 0, pBuffer, dwFileSize, NULL, 0);
        szFile = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!szFile)
        {
            HeapFree(GetProcessHeap(), 0, buffer_base);
            return NULL;
        }
        MultiByteToWideChar(CP_UTF8, 0, pBuffer, dwFileSize, szFile, len);
        szEnd = szFile + len;
        break;
    case ENCODING_UTF16LE:
        TRACE("UTF16 Little Endian encoding\n");
        szFile = pBuffer;
        szEnd = (WCHAR *)((char *)pBuffer + dwFileSize);
        break;
    case ENCODING_UTF16BE:
        TRACE("UTF16 Big Endian encoding\n");
        szFile = pBuffer;
        szEnd = (WCHAR *)((char *)pBuffer + dwFileSize);
        PROFILE_ByteSwapShortBuffer(szFile, dwFileSize / sizeof(WCHAR));
        break;
    default:
        FIXME("encoding type %d not implemented\n", *pEncoding);
        HeapFree(GetProcessHeap(), 0, buffer_base);
        return NULL;
    }

    first_section = HeapAlloc( GetProcessHeap(), 0, sizeof(*section) );
    if(first_section == NULL)
    {
        if (szFile != pBuffer)
            HeapFree(GetProcessHeap(), 0, szFile);
        HeapFree(GetProcessHeap(), 0, buffer_base);
        return NULL;
    }
    first_section->name[0] = 0;
    first_section->key  = NULL;
    first_section->next = NULL;
    next_section = &first_section->next;
    next_key     = &first_section->key;
    prev_key     = NULL;
    next_line    = szFile;

    while (next_line < szEnd)
    {
        szLineStart = next_line;
        next_line = memchrW(szLineStart, '\n', szEnd - szLineStart);
        if (!next_line) next_line = memchrW(szLineStart, '\r', szEnd - szLineStart);
        if (!next_line) next_line = szEnd;
        else next_line++;
        szLineEnd = next_line;

        line++;

        /* get rid of white space */
        while (szLineStart < szLineEnd && PROFILE_isspaceW(*szLineStart)) szLineStart++;
        while ((szLineEnd > szLineStart) && PROFILE_isspaceW(szLineEnd[-1])) szLineEnd--;

        if (szLineStart >= szLineEnd) continue;

        if (*szLineStart == '[')  /* section start */
        {
            const WCHAR * szSectionEnd;
            if (!(szSectionEnd = memrchrW( szLineStart, ']', szLineEnd - szLineStart )))
            {
                WARN("Invalid section header at line %d: %s\n",
                    line, debugstr_wn(szLineStart, (int)(szLineEnd - szLineStart)) );
            }
            else
            {
                szLineStart++;
                len = (int)(szSectionEnd - szLineStart);
                /* no need to allocate +1 for NULL terminating character as
                 * already included in structure */
                if (!(section = HeapAlloc( GetProcessHeap(), 0, sizeof(*section) + len * sizeof(WCHAR) )))
                    break;
                memcpy(section->name, szLineStart, len * sizeof(WCHAR));
                section->name[len] = '\0';
                section->key  = NULL;
                section->next = NULL;
                *next_section = section;
                next_section  = &section->next;
                next_key      = &section->key;
                prev_key      = NULL;

                TRACE("New section: %s\n", debugstr_w(section->name));

                continue;
            }
        }

        /* get rid of white space after the name and before the start
         * of the value */
        len = szLineEnd - szLineStart;
        if ((szValueStart = memchrW( szLineStart, '=', szLineEnd - szLineStart )) != NULL)
        {
            const WCHAR *szNameEnd = szValueStart;
            while ((szNameEnd > szLineStart) && PROFILE_isspaceW(szNameEnd[-1])) szNameEnd--;
            len = szNameEnd - szLineStart;
            szValueStart++;
            while (szValueStart < szLineEnd && PROFILE_isspaceW(*szValueStart)) szValueStart++;
        }

        if (len || !prev_key || *prev_key->name)
        {
            /* no need to allocate +1 for NULL terminating character as
             * already included in structure */
            if (!(key = HeapAlloc( GetProcessHeap(), 0, sizeof(*key) + len * sizeof(WCHAR) ))) break;
            memcpy(key->name, szLineStart, len * sizeof(WCHAR));
            key->name[len] = '\0';
            if (szValueStart)
            {
                len = (int)(szLineEnd - szValueStart);
                key->value = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) );
                memcpy(key->value, szValueStart, len * sizeof(WCHAR));
                key->value[len] = '\0';
            }
            else key->value = NULL;

           key->next  = NULL;
           *next_key  = key;
           next_key   = &key->next;
           prev_key   = key;

#ifdef __REACTOS__
           TRACE("New key: name=%s, value=%s\n",
                 debugstr_w(key->name),
                 key->value ? debugstr_w(key->value) : L"(none)");
#else
           TRACE("New key: name=%s, value=%s\n",
               debugstr_w(key->name), key->value ? debugstr_w(key->value) : "(none)");
#endif
        }
    }
    if (szFile != pBuffer)
        HeapFree(GetProcessHeap(), 0, szFile);
    HeapFree(GetProcessHeap(), 0, buffer_base);
    return first_section;
}


/***********************************************************************
 *           PROFILE_DeleteSection
 *
 * Delete a section from a profile tree.
 */
static BOOL PROFILE_DeleteSection( PROFILESECTION **section, LPCWSTR name )
{
    while (*section)
    {
        if (!strcmpiW( (*section)->name, name ))
        {
            PROFILESECTION *to_del = *section;
            *section = to_del->next;
            to_del->next = NULL;
            PROFILE_Free( to_del );
            return TRUE;
        }
        section = &(*section)->next;
    }
    return FALSE;
}


/***********************************************************************
 *           PROFILE_DeleteKey
 *
 * Delete a key from a profile tree.
 */
static BOOL PROFILE_DeleteKey( PROFILESECTION **section,
			       LPCWSTR section_name, LPCWSTR key_name )
{
    while (*section)
    {
        if (!strcmpiW( (*section)->name, section_name ))
        {
            PROFILEKEY **key = &(*section)->key;
            while (*key)
            {
                if (!strcmpiW( (*key)->name, key_name ))
                {
                    PROFILEKEY *to_del = *key;
                    *key = to_del->next;
                    HeapFree( GetProcessHeap(), 0, to_del->value);
                    HeapFree( GetProcessHeap(), 0, to_del );
                    return TRUE;
                }
                key = &(*key)->next;
            }
        }
        section = &(*section)->next;
    }
    return FALSE;
}


/***********************************************************************
 *           PROFILE_DeleteAllKeys
 *
 * Delete all keys from a profile tree.
 */
static void PROFILE_DeleteAllKeys( LPCWSTR section_name)
{
    PROFILESECTION **section= &CurProfile->section;
    while (*section)
    {
        if (!strcmpiW( (*section)->name, section_name ))
        {
            PROFILEKEY **key = &(*section)->key;
            while (*key)
            {
                PROFILEKEY *to_del = *key;
		*key = to_del->next;
                HeapFree( GetProcessHeap(), 0, to_del->value);
		HeapFree( GetProcessHeap(), 0, to_del );
		CurProfile->changed =TRUE;
            }
        }
        section = &(*section)->next;
    }
}


/***********************************************************************
 *           PROFILE_Find
 *
 * Find a key in a profile tree, optionally creating it.
 */
static PROFILEKEY *PROFILE_Find( PROFILESECTION **section, LPCWSTR section_name,
                                 LPCWSTR key_name, BOOL create, BOOL create_always )
{
    LPCWSTR p;
    int seclen = 0, keylen = 0;

    while (PROFILE_isspaceW(*section_name)) section_name++;
    if (*section_name)
    {
        p = section_name + strlenW(section_name) - 1;
        while ((p > section_name) && PROFILE_isspaceW(*p)) p--;
        seclen = p - section_name + 1;
    }

    while (PROFILE_isspaceW(*key_name)) key_name++;
    if (*key_name)
    {
        p = key_name + strlenW(key_name) - 1;
        while ((p > key_name) && PROFILE_isspaceW(*p)) p--;
        keylen = p - key_name + 1;
    }

    while (*section)
    {
        if (!strncmpiW((*section)->name, section_name, seclen) &&
            ((*section)->name)[seclen] == '\0')
        {
            PROFILEKEY **key = &(*section)->key;

            while (*key)
            {
                /* If create_always is FALSE then we check if the keyname
                 * already exists. Otherwise we add it regardless of its
                 * existence, to allow keys to be added more than once in
                 * some cases.
                 */
                if(!create_always)
                {
                    if ( (!(strncmpiW( (*key)->name, key_name, keylen )))
                         && (((*key)->name)[keylen] == '\0') )
                        return *key;
                }
                key = &(*key)->next;
            }
            if (!create) return NULL;
            if (!(*key = HeapAlloc( GetProcessHeap(), 0, sizeof(PROFILEKEY) + strlenW(key_name) * sizeof(WCHAR) )))
                return NULL;
            strcpyW( (*key)->name, key_name );
            (*key)->value = NULL;
            (*key)->next  = NULL;
            return *key;
        }
        section = &(*section)->next;
    }
    if (!create) return NULL;
    *section = HeapAlloc( GetProcessHeap(), 0, sizeof(PROFILESECTION) + strlenW(section_name) * sizeof(WCHAR) );
    if(*section == NULL) return NULL;
    strcpyW( (*section)->name, section_name );
    (*section)->next = NULL;
    if (!((*section)->key  = HeapAlloc( GetProcessHeap(), 0,
                                        sizeof(PROFILEKEY) + strlenW(key_name) * sizeof(WCHAR) )))
    {
        HeapFree(GetProcessHeap(), 0, *section);
        return NULL;
    }
    strcpyW( (*section)->key->name, key_name );
    (*section)->key->value = NULL;
    (*section)->key->next  = NULL;
    return (*section)->key;
}


/***********************************************************************
 *           PROFILE_FlushFile
 *
 * Flush the current profile to disk if changed.
 */
static BOOL PROFILE_FlushFile(void)
{
    HANDLE hFile = NULL;
    FILETIME LastWriteTime;

    if(!CurProfile)
    {
        WARN("No current profile!\n");
        return FALSE;
    }

    if (!CurProfile->changed) return TRUE;

    hFile = CreateFileW(CurProfile->filename, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        WARN("could not save profile file %s (error was %d)\n", debugstr_w(CurProfile->filename), GetLastError());
        return FALSE;
    }

    TRACE("Saving %s\n", debugstr_w(CurProfile->filename));
    PROFILE_Save( hFile, CurProfile->section, CurProfile->encoding );
    if(GetFileTime(hFile, NULL, NULL, &LastWriteTime))
       CurProfile->LastWriteTime=LastWriteTime;
    CloseHandle( hFile );
    CurProfile->changed = FALSE;
    return TRUE;
}


/***********************************************************************
 *           PROFILE_ReleaseFile
 *
 * Flush the current profile to disk and remove it from the cache.
 */
static void PROFILE_ReleaseFile(void)
{
    PROFILE_FlushFile();
    PROFILE_Free( CurProfile->section );
    HeapFree( GetProcessHeap(), 0, CurProfile->filename );
    CurProfile->changed = FALSE;
    CurProfile->section = NULL;
    CurProfile->filename  = NULL;
    CurProfile->encoding = ENCODING_ANSI;
    ZeroMemory(&CurProfile->LastWriteTime, sizeof(CurProfile->LastWriteTime));
}

/***********************************************************************
 *
 * Compares a file time with the current time. If the file time is
 * at least 2.1 seconds in the past, return true.
 *
 * Intended as cache safety measure: The time resolution on FAT is
 * two seconds, so files that are not at least two seconds old might
 * keep their time even on modification, so don't cache them.
 */
static BOOL is_not_current(FILETIME * ft)
{
    FILETIME Now;
    LONGLONG ftll, nowll;
    GetSystemTimeAsFileTime(&Now);
    ftll = ((LONGLONG)ft->dwHighDateTime << 32) + ft->dwLowDateTime;
    nowll = ((LONGLONG)Now.dwHighDateTime << 32) + Now.dwLowDateTime;
    TRACE("%08x;%08x\n",(unsigned)ftll+21000000,(unsigned)nowll);
    return ftll + 21000000 < nowll;
}

/***********************************************************************
 *           PROFILE_Open
 *
 * Open a profile file, checking the cached file first.
 */
static BOOL PROFILE_Open( LPCWSTR filename, BOOL write_access )
{
    WCHAR buffer[MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;
    FILETIME LastWriteTime;
    int i,j;
    PROFILE *tempProfile;
    
    ZeroMemory(&LastWriteTime, sizeof(LastWriteTime));

    /* First time around */

    if(!CurProfile)
       for(i=0;i<N_CACHED_PROFILES;i++)
       {
          MRUProfile[i]=HeapAlloc( GetProcessHeap(), 0, sizeof(PROFILE) );
          if(MRUProfile[i] == NULL) break;
          MRUProfile[i]->changed=FALSE;
          MRUProfile[i]->section=NULL;
          MRUProfile[i]->filename=NULL;
          MRUProfile[i]->encoding=ENCODING_ANSI;
          ZeroMemory(&MRUProfile[i]->LastWriteTime, sizeof(FILETIME));
       }

    if (!filename)
	filename = wininiW;

#ifdef __REACTOS__
    if ((RtlDetermineDosPathNameType_U(filename) == RtlPathTypeRelative) &&
#else
    if ((RtlDetermineDosPathNameType_U(filename) == RELATIVE_PATH) &&
#endif
        !strchrW(filename, '\\') && !strchrW(filename, '/'))
    {
        static const WCHAR wszSeparator[] = {'\\', 0};
        WCHAR windirW[MAX_PATH];
        GetWindowsDirectoryW( windirW, MAX_PATH );
        strcpyW(buffer, windirW);
        strcatW(buffer, wszSeparator);
        strcatW(buffer, filename);
    }
    else
    {
        LPWSTR dummy;
        GetFullPathNameW(filename, sizeof(buffer)/sizeof(buffer[0]), buffer, &dummy);
    }
        
    TRACE("path: %s\n", debugstr_w(buffer));

    hFile = CreateFileW(buffer, GENERIC_READ | (write_access ? GENERIC_WRITE : 0),
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if ((hFile == INVALID_HANDLE_VALUE) && (GetLastError() != ERROR_FILE_NOT_FOUND))
    {
        WARN("Error %d opening file %s\n", GetLastError(), debugstr_w(buffer));
        return FALSE;
    }

    for(i=0;i<N_CACHED_PROFILES;i++)
    {
        if ((MRUProfile[i]->filename && !strcmpiW( buffer, MRUProfile[i]->filename )))
        {
            TRACE("MRU Filename: %s, new filename: %s\n", debugstr_w(MRUProfile[i]->filename), debugstr_w(buffer));
            if(i)
            {
                PROFILE_FlushFile();
                tempProfile=MRUProfile[i];
                for(j=i;j>0;j--)
                    MRUProfile[j]=MRUProfile[j-1];
                CurProfile=tempProfile;
            }

            if (hFile != INVALID_HANDLE_VALUE)
            {
                GetFileTime(hFile, NULL, NULL, &LastWriteTime);
                if (!memcmp( &CurProfile->LastWriteTime, &LastWriteTime, sizeof(FILETIME) ) &&
                    is_not_current(&LastWriteTime))
                    TRACE("(%s): already opened (mru=%d)\n",
                          debugstr_w(buffer), i);
                else
                {
                    TRACE("(%s): already opened, needs refreshing (mru=%d)\n",
                          debugstr_w(buffer), i);
                    PROFILE_Free(CurProfile->section);
                    CurProfile->section = PROFILE_Load(hFile, &CurProfile->encoding);
                    CurProfile->LastWriteTime = LastWriteTime;
                }
                CloseHandle(hFile);
                return TRUE;
            }
            else TRACE("(%s): already opened, not yet created (mru=%d)\n",
                       debugstr_w(buffer), i);
        }
    }

    /* Flush the old current profile */
    PROFILE_FlushFile();

    /* Make the oldest profile the current one only in order to get rid of it */
    if(i==N_CACHED_PROFILES)
      {
       tempProfile=MRUProfile[N_CACHED_PROFILES-1];
       for(i=N_CACHED_PROFILES-1;i>0;i--)
          MRUProfile[i]=MRUProfile[i-1];
       CurProfile=tempProfile;
      }
    if(CurProfile->filename) PROFILE_ReleaseFile();

    /* OK, now that CurProfile is definitely free we assign it our new file */
    CurProfile->filename  = HeapAlloc( GetProcessHeap(), 0, (strlenW(buffer)+1) * sizeof(WCHAR) );
    strcpyW( CurProfile->filename, buffer );

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CurProfile->section = PROFILE_Load(hFile, &CurProfile->encoding);
        GetFileTime(hFile, NULL, NULL, &CurProfile->LastWriteTime);
        CloseHandle(hFile);
    }
    else
    {
        /* Does not exist yet, we will create it in PROFILE_FlushFile */
        WARN("profile file %s not found\n", debugstr_w(buffer) );
    }
    return TRUE;
}


/***********************************************************************
 *           PROFILE_GetSection
 *
 * Returns all keys of a section.
 * If return_values is TRUE, also include the corresponding values.
 */
static INT PROFILE_GetSection( PROFILESECTION *section, LPCWSTR section_name,
#ifdef __REACTOS__
			       LPWSTR buffer, DWORD len, BOOL return_values )
#else
			       LPWSTR buffer, UINT len, BOOL return_values )
#endif
{
    PROFILEKEY *key;

    if(!buffer) return 0;

    TRACE("%s,%p,%u\n", debugstr_w(section_name), buffer, len);

    while (section)
    {
        if (!strcmpiW( section->name, section_name ))
        {
            UINT oldlen = len;
            for (key = section->key; key; key = key->next)
            {
                if (len <= 2) break;
                if (!*key->name) continue;  /* Skip empty lines */
                if (IS_ENTRY_COMMENT(key->name)) continue;  /* Skip comments */
                if (!return_values && !key->value) continue;  /* Skip lines w.o. '=' */
                PROFILE_CopyEntry( buffer, key->name, len - 1, 0 );
                len -= strlenW(buffer) + 1;
                buffer += strlenW(buffer) + 1;
		if (len < 2)
		    break;
		if (return_values && key->value) {
			buffer[-1] = '=';
			PROFILE_CopyEntry ( buffer, key->value, len - 1, 0 );
			len -= strlenW(buffer) + 1;
			buffer += strlenW(buffer) + 1;
                }
            }
            *buffer = '\0';
            if (len <= 1)
                /*If either lpszSection or lpszKey is NULL and the supplied
                  destination buffer is too small to hold all the strings,
                  the last string is truncated and followed by two null characters.
                  In this case, the return value is equal to cchReturnBuffer
                  minus two. */
            {
		buffer[-1] = '\0';
                return oldlen - 2;
            }
            return oldlen - len;
        }
        section = section->next;
    }
    buffer[0] = buffer[1] = '\0';
    return 0;
}

/* See GetPrivateProfileSectionNamesA for documentation */
#ifdef __REACTOS__
static INT PROFILE_GetSectionNames( LPWSTR buffer, DWORD len )
#else
static INT PROFILE_GetSectionNames( LPWSTR buffer, UINT len )
#endif
{
    LPWSTR buf;
    UINT buflen,tmplen;
    PROFILESECTION *section;

    TRACE("(%p, %d)\n", buffer, len);

    if (!buffer || !len)
        return 0;
    if (len==1) {
        *buffer='\0';
        return 0;
    }

    buflen=len-1;
    buf=buffer;
    section = CurProfile->section;
    while ((section!=NULL)) {
        if (section->name[0]) {
            tmplen = strlenW(section->name)+1;
            if (tmplen >= buflen) {
                if (buflen > 0) {
                    memcpy(buf, section->name, (buflen-1) * sizeof(WCHAR));
                    buf += buflen-1;
                    *buf++='\0';
                }
                *buf='\0';
                return len-2;
            }
            memcpy(buf, section->name, tmplen * sizeof(WCHAR));
            buf += tmplen;
            buflen -= tmplen;
        }
        section = section->next;
    }
    *buf='\0';
    return buf-buffer;
}


/***********************************************************************
 *           PROFILE_GetString
 *
 * Get a profile string.
 *
 * Tests with GetPrivateProfileString16, W95a,
 * with filled buffer ("****...") and section "set1" and key_name "1" valid:
 * section	key_name	def_val		res	buffer
 * "set1"	"1"		"x"		43	[data]
 * "set1"	"1   "		"x"		43	[data]		(!)
 * "set1"	"  1  "'	"x"		43	[data]		(!)
 * "set1"	""		"x"		1	"x"
 * "set1"	""		"x   "		1	"x"		(!)
 * "set1"	""		"  x   "	3	"  x"		(!)
 * "set1"	NULL		"x"		6	"1\02\03\0\0"
 * "set1"	""		"x"		1	"x"
 * NULL		"1"		"x"		0	""		(!)
 * ""		"1"		"x"		1	"x"
 * NULL		NULL		""		0	""
 *
 *
 */
static INT PROFILE_GetString( LPCWSTR section, LPCWSTR key_name,
#ifdef __REACTOS__
                              LPCWSTR def_val, LPWSTR buffer, DWORD len )
#else
                              LPCWSTR def_val, LPWSTR buffer, UINT len )
#endif
{
    PROFILEKEY *key = NULL;
    static const WCHAR empty_strW[] = { 0 };

    if(!buffer || !len) return 0;

    if (!def_val) def_val = empty_strW;
    if (key_name)
    {
        key = PROFILE_Find( &CurProfile->section, section, key_name, FALSE, FALSE);
        PROFILE_CopyEntry( buffer, (key && key->value) ? key->value : def_val,
                           len, TRUE );
        TRACE("(%s,%s,%s): returning %s\n",
              debugstr_w(section), debugstr_w(key_name),
              debugstr_w(def_val), debugstr_w(buffer) );
        return strlenW( buffer );
    }
    /* no "else" here ! */
    if (section)
    {
        INT ret = PROFILE_GetSection(CurProfile->section, section, buffer, len, FALSE);
        if (!buffer[0]) /* no luck -> def_val */
        {
            PROFILE_CopyEntry(buffer, def_val, len, TRUE);
            ret = strlenW(buffer);
        }
        return ret;
    }
    buffer[0] = '\0';
    return 0;
}


/***********************************************************************
 *           PROFILE_SetString
 *
 * Set a profile string.
 */
static BOOL PROFILE_SetString( LPCWSTR section_name, LPCWSTR key_name,
                               LPCWSTR value, BOOL create_always )
{
    if (!key_name)  /* Delete a whole section */
    {
        TRACE("(%s)\n", debugstr_w(section_name));
        CurProfile->changed |= PROFILE_DeleteSection( &CurProfile->section,
                                                      section_name );
        return TRUE;         /* Even if PROFILE_DeleteSection() has failed,
                                this is not an error on application's level.*/
    }
    else if (!value)  /* Delete a key */
    {
        TRACE("(%s,%s)\n", debugstr_w(section_name), debugstr_w(key_name) );
        CurProfile->changed |= PROFILE_DeleteKey( &CurProfile->section,
                                                  section_name, key_name );
        return TRUE;          /* same error handling as above */
    }
    else  /* Set the key value */
    {
        PROFILEKEY *key = PROFILE_Find(&CurProfile->section, section_name,
                                        key_name, TRUE, create_always );
        TRACE("(%s,%s,%s):\n",
              debugstr_w(section_name), debugstr_w(key_name), debugstr_w(value) );
        if (!key) return FALSE;

        /* strip the leading spaces. We can safely strip \n\r and
         * friends too, they should not happen here anyway. */
        while (PROFILE_isspaceW(*value)) value++;

        if (key->value)
        {
            if (!strcmpW( key->value, value ))
            {
                TRACE("  no change needed\n" );
                return TRUE;  /* No change needed */
            }
            TRACE("  replacing %s\n", debugstr_w(key->value) );
            HeapFree( GetProcessHeap(), 0, key->value );
        }
        else TRACE("  creating key\n" );
        key->value = HeapAlloc( GetProcessHeap(), 0, (strlenW(value)+1) * sizeof(WCHAR) );
        strcpyW( key->value, value );
        CurProfile->changed = TRUE;
    }
    return TRUE;
}


/********************* API functions **********************************/


/***********************************************************************
 *           GetProfileIntA   (KERNEL32.@)
 */
UINT WINAPI GetProfileIntA( LPCSTR section, LPCSTR entry, INT def_val )
{
    return GetPrivateProfileIntA( section, entry, def_val, "win.ini" );
}

/***********************************************************************
 *           GetProfileIntW   (KERNEL32.@)
 */
UINT WINAPI GetProfileIntW( LPCWSTR section, LPCWSTR entry, INT def_val )
{
    return GetPrivateProfileIntW( section, entry, def_val, wininiW );
}

/***********************************************************************
 *           GetPrivateProfileStringW   (KERNEL32.@)
 */
#ifdef __REACTOS__
DWORD WINAPI GetPrivateProfileStringW( LPCWSTR section, LPCWSTR entry,
				     LPCWSTR def_val, LPWSTR buffer,
				     DWORD len, LPCWSTR filename )
#else
INT WINAPI GetPrivateProfileStringW( LPCWSTR section, LPCWSTR entry,
				     LPCWSTR def_val, LPWSTR buffer,
				     UINT len, LPCWSTR filename )
#endif
{
    int		ret;
    LPWSTR	defval_tmp = NULL;

    TRACE("%s,%s,%s,%p,%u,%s\n", debugstr_w(section), debugstr_w(entry),
          debugstr_w(def_val), buffer, len, debugstr_w(filename));

    /* strip any trailing ' ' of def_val. */
    if (def_val)
    {
        LPCWSTR p = def_val + strlenW(def_val) - 1;

        while (p > def_val && *p == ' ')
            p--;

        if (p >= def_val)
        {
            int vlen = (int)(p - def_val) + 1;

            defval_tmp = HeapAlloc(GetProcessHeap(), 0, (vlen + 1) * sizeof(WCHAR));
            memcpy(defval_tmp, def_val, vlen * sizeof(WCHAR));
            defval_tmp[vlen] = '\0';
            def_val = defval_tmp;
        }
    }

    RtlEnterCriticalSection( &PROFILE_CritSect );

    if (PROFILE_Open( filename, FALSE )) {
	if (section == NULL)
            ret = PROFILE_GetSectionNames(buffer, len);
	else 
	    /* PROFILE_GetString can handle the 'entry == NULL' case */
            ret = PROFILE_GetString( section, entry, def_val, buffer, len );
    } else if (buffer && def_val) {
       lstrcpynW( buffer, def_val, len );
       ret = strlenW( buffer );
    }
    else
       ret = 0;

    RtlLeaveCriticalSection( &PROFILE_CritSect );

    HeapFree(GetProcessHeap(), 0, defval_tmp);

    TRACE("returning %s, %d\n", debugstr_w(buffer), ret);

    return ret;
}

/***********************************************************************
 *           GetPrivateProfileStringA   (KERNEL32.@)
 */
#ifdef __REACTOS__
DWORD WINAPI GetPrivateProfileStringA( LPCSTR section, LPCSTR entry,
				     LPCSTR def_val, LPSTR buffer,
				     DWORD len, LPCSTR filename )
#else
INT WINAPI GetPrivateProfileStringA( LPCSTR section, LPCSTR entry,
				     LPCSTR def_val, LPSTR buffer,
				     UINT len, LPCSTR filename )
#endif
{
    UNICODE_STRING sectionW, entryW, def_valW, filenameW;
    LPWSTR bufferW;
    INT retW, ret = 0;

    bufferW = buffer ? HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR)) : NULL;
    if (section) RtlCreateUnicodeStringFromAsciiz(&sectionW, section);
    else sectionW.Buffer = NULL;
    if (entry) RtlCreateUnicodeStringFromAsciiz(&entryW, entry);
    else entryW.Buffer = NULL;
    if (def_val) RtlCreateUnicodeStringFromAsciiz(&def_valW, def_val);
    else def_valW.Buffer = NULL;
    if (filename) RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else filenameW.Buffer = NULL;

    retW = GetPrivateProfileStringW( sectionW.Buffer, entryW.Buffer,
                                     def_valW.Buffer, bufferW, len,
                                     filenameW.Buffer);
    if (len && buffer)
    {
        if (retW)
        {
            ret = WideCharToMultiByte(CP_ACP, 0, bufferW, retW, buffer, len - 1, NULL, NULL);
            if (!ret)
                ret = len - 1;
        }
        buffer[ret] = 0;
    }

    RtlFreeUnicodeString(&sectionW);
    RtlFreeUnicodeString(&entryW);
    RtlFreeUnicodeString(&def_valW);
    RtlFreeUnicodeString(&filenameW);
    HeapFree(GetProcessHeap(), 0, bufferW);
    return ret;
}

/***********************************************************************
 *           GetProfileStringA   (KERNEL32.@)
 */
#ifdef __REACTOS__
DWORD WINAPI GetProfileStringA( LPCSTR section, LPCSTR entry, LPCSTR def_val,
			      LPSTR buffer, DWORD len )
#else
INT WINAPI GetProfileStringA( LPCSTR section, LPCSTR entry, LPCSTR def_val,
			      LPSTR buffer, UINT len )
#endif
{
    return GetPrivateProfileStringA( section, entry, def_val,
                                     buffer, len, "win.ini" );
}

/***********************************************************************
 *           GetProfileStringW   (KERNEL32.@)
 */
#ifdef __REACTOS__
DWORD WINAPI GetProfileStringW( LPCWSTR section, LPCWSTR entry,
			      LPCWSTR def_val, LPWSTR buffer, DWORD len )
#else
INT WINAPI GetProfileStringW( LPCWSTR section, LPCWSTR entry,
			      LPCWSTR def_val, LPWSTR buffer, UINT len )
#endif
{
    return GetPrivateProfileStringW( section, entry, def_val,
				     buffer, len, wininiW );
}

/***********************************************************************
 *           WriteProfileStringA   (KERNEL32.@)
 */
BOOL WINAPI WriteProfileStringA( LPCSTR section, LPCSTR entry,
				 LPCSTR string )
{
    return WritePrivateProfileStringA( section, entry, string, "win.ini" );
}

/***********************************************************************
 *           WriteProfileStringW   (KERNEL32.@)
 */
BOOL WINAPI WriteProfileStringW( LPCWSTR section, LPCWSTR entry,
                                     LPCWSTR string )
{
    return WritePrivateProfileStringW( section, entry, string, wininiW );
}


/***********************************************************************
 *           GetPrivateProfileIntW   (KERNEL32.@)
 */
UINT WINAPI GetPrivateProfileIntW( LPCWSTR section, LPCWSTR entry,
                                   INT def_val, LPCWSTR filename )
{
    WCHAR buffer[30];
    UNICODE_STRING bufferW;
    ULONG result;

    if (GetPrivateProfileStringW( section, entry, emptystringW,
                                   buffer, sizeof(buffer)/sizeof(WCHAR),
                                   filename ) == 0)
        return def_val;

    /* FIXME: if entry can be found but it's empty, then Win16 is
     * supposed to return 0 instead of def_val ! Difficult/problematic
     * to implement (every other failure also returns zero buffer),
     * thus wait until testing framework avail for making sure nothing
     * else gets broken that way. */
    if (!buffer[0]) return (UINT)def_val;

    RtlInitUnicodeString( &bufferW, buffer );
    RtlUnicodeStringToInteger( &bufferW, 0, &result);
    return result;
}

/***********************************************************************
 *           GetPrivateProfileIntA   (KERNEL32.@)
 *
 * FIXME: rewrite using unicode
 */
UINT WINAPI GetPrivateProfileIntA( LPCSTR section, LPCSTR entry,
				   INT def_val, LPCSTR filename )
{
    UNICODE_STRING entryW, filenameW, sectionW;
    UINT res;
    if(entry) RtlCreateUnicodeStringFromAsciiz(&entryW, entry);
    else entryW.Buffer = NULL;
    if(filename) RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else filenameW.Buffer = NULL;
    if(section) RtlCreateUnicodeStringFromAsciiz(&sectionW, section);
    else sectionW.Buffer = NULL;
    res = GetPrivateProfileIntW(sectionW.Buffer, entryW.Buffer, def_val,
                                filenameW.Buffer);
    RtlFreeUnicodeString(&sectionW);
    RtlFreeUnicodeString(&filenameW);
    RtlFreeUnicodeString(&entryW);
    return res;
}

/***********************************************************************
 *           GetPrivateProfileSectionW   (KERNEL32.@)
 */
#ifdef __REACTOS__
DWORD WINAPI GetPrivateProfileSectionW( LPCWSTR section, LPWSTR buffer,
#else
INT WINAPI GetPrivateProfileSectionW( LPCWSTR section, LPWSTR buffer,
#endif
				      DWORD len, LPCWSTR filename )
{
    int ret = 0;

    if (!section || !buffer)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    TRACE("(%s, %p, %d, %s)\n", debugstr_w(section), buffer, len, debugstr_w(filename));

    RtlEnterCriticalSection( &PROFILE_CritSect );

    if (PROFILE_Open( filename, FALSE ))
        ret = PROFILE_GetSection(CurProfile->section, section, buffer, len, TRUE);

    RtlLeaveCriticalSection( &PROFILE_CritSect );

    return ret;
}

/***********************************************************************
 *           GetPrivateProfileSectionA   (KERNEL32.@)
 */
#ifdef __REACTOS__
DWORD WINAPI GetPrivateProfileSectionA( LPCSTR section, LPSTR buffer,
#else
INT WINAPI GetPrivateProfileSectionA( LPCSTR section, LPSTR buffer,
#endif
                                      DWORD len, LPCSTR filename )
{
    UNICODE_STRING sectionW, filenameW;
    LPWSTR bufferW;
    INT retW, ret = 0;

    if (!section || !buffer)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    bufferW = HeapAlloc(GetProcessHeap(), 0, len * 2 * sizeof(WCHAR));
    RtlCreateUnicodeStringFromAsciiz(&sectionW, section);
    if (filename) RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else filenameW.Buffer = NULL;

    retW = GetPrivateProfileSectionW(sectionW.Buffer, bufferW, len * 2, filenameW.Buffer);
    if (retW)
    {
        if (retW == len * 2 - 2) retW++;  /* overflow */
        ret = WideCharToMultiByte(CP_ACP, 0, bufferW, retW + 1, buffer, len, NULL, NULL);
        if (!ret || ret == len)  /* overflow */
        {
            ret = len - 2;
            buffer[len-2] = 0;
            buffer[len-1] = 0;
        }
        else ret--;
    }
    else
    {
        buffer[0] = 0;
        buffer[1] = 0;
    }

    RtlFreeUnicodeString(&sectionW);
    RtlFreeUnicodeString(&filenameW);
    HeapFree(GetProcessHeap(), 0, bufferW);
    return ret;
}

/***********************************************************************
 *           GetProfileSectionA   (KERNEL32.@)
 */
#ifdef __REACTOS__
DWORD WINAPI GetProfileSectionA( LPCSTR section, LPSTR buffer, DWORD len )
#else
INT WINAPI GetProfileSectionA( LPCSTR section, LPSTR buffer, DWORD len )
#endif
{
    return GetPrivateProfileSectionA( section, buffer, len, "win.ini" );
}

/***********************************************************************
 *           GetProfileSectionW   (KERNEL32.@)
 */
#ifdef __REACTOS__
DWORD WINAPI GetProfileSectionW( LPCWSTR section, LPWSTR buffer, DWORD len )
#else
INT WINAPI GetProfileSectionW( LPCWSTR section, LPWSTR buffer, DWORD len )
#endif
{
    return GetPrivateProfileSectionW( section, buffer, len, wininiW );
}


/***********************************************************************
 *           WritePrivateProfileStringW   (KERNEL32.@)
 */
BOOL WINAPI WritePrivateProfileStringW( LPCWSTR section, LPCWSTR entry,
					LPCWSTR string, LPCWSTR filename )
{
    BOOL ret = FALSE;

    RtlEnterCriticalSection( &PROFILE_CritSect );

    if (!section && !entry && !string) /* documented "file flush" case */
    {
        if (!filename || PROFILE_Open( filename, TRUE ))
        {
            if (CurProfile) PROFILE_ReleaseFile();  /* always return FALSE in this case */
        }
    }
    else if (PROFILE_Open( filename, TRUE ))
    {
        if (!section) {
            SetLastError(ERROR_FILE_NOT_FOUND);
        } else {
            ret = PROFILE_SetString( section, entry, string, FALSE);
            if (ret) ret = PROFILE_FlushFile();
        }
    }

    RtlLeaveCriticalSection( &PROFILE_CritSect );
    return ret;
}

/***********************************************************************
 *           WritePrivateProfileStringA   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WritePrivateProfileStringA( LPCSTR section, LPCSTR entry,
                                                          LPCSTR string, LPCSTR filename )
{
    UNICODE_STRING sectionW, entryW, stringW, filenameW;
    BOOL ret;

    if (section) RtlCreateUnicodeStringFromAsciiz(&sectionW, section);
    else sectionW.Buffer = NULL;
    if (entry) RtlCreateUnicodeStringFromAsciiz(&entryW, entry);
    else entryW.Buffer = NULL;
    if (string) RtlCreateUnicodeStringFromAsciiz(&stringW, string);
    else stringW.Buffer = NULL;
    if (filename) RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else filenameW.Buffer = NULL;

    ret = WritePrivateProfileStringW(sectionW.Buffer, entryW.Buffer,
                                     stringW.Buffer, filenameW.Buffer);
    RtlFreeUnicodeString(&sectionW);
    RtlFreeUnicodeString(&entryW);
    RtlFreeUnicodeString(&stringW);
    RtlFreeUnicodeString(&filenameW);
    return ret;
}

/***********************************************************************
 *           WritePrivateProfileSectionW   (KERNEL32.@)
 */
BOOL WINAPI WritePrivateProfileSectionW( LPCWSTR section,
                                         LPCWSTR string, LPCWSTR filename )
{
    BOOL ret = FALSE;
    LPWSTR p;

    RtlEnterCriticalSection( &PROFILE_CritSect );

    if (!section && !string)
    {
        if (!filename || PROFILE_Open( filename, TRUE ))
        {
            if (CurProfile) PROFILE_ReleaseFile();  /* always return FALSE in this case */
        }
    }
    else if (PROFILE_Open( filename, TRUE )) {
        if (!string) {/* delete the named section*/
	    ret = PROFILE_SetString(section,NULL,NULL, FALSE);
        } else {
	    PROFILE_DeleteAllKeys(section);
	    ret = TRUE;
	    while(*string && ret) {
                LPWSTR buf = HeapAlloc( GetProcessHeap(), 0, (strlenW(string)+1) * sizeof(WCHAR) );
                strcpyW( buf, string );
                if((p = strchrW( buf, '='))) {
                    *p='\0';
                    ret = PROFILE_SetString( section, buf, p+1, TRUE);
                }
                HeapFree( GetProcessHeap(), 0, buf );
                string += strlenW(string)+1;
            }
        }
        if (ret) ret = PROFILE_FlushFile();
    }

    RtlLeaveCriticalSection( &PROFILE_CritSect );
    return ret;
}

/***********************************************************************
 *           WritePrivateProfileSectionA   (KERNEL32.@)
 */
BOOL WINAPI WritePrivateProfileSectionA( LPCSTR section,
                                         LPCSTR string, LPCSTR filename)

{
    UNICODE_STRING sectionW, filenameW;
    LPWSTR stringW;
    BOOL ret;

    if (string)
    {
        INT lenA, lenW;
        LPCSTR p = string;

        while(*p) p += strlen(p) + 1;
        lenA = p - string + 1;
        lenW = MultiByteToWideChar(CP_ACP, 0, string, lenA, NULL, 0);
        if ((stringW = HeapAlloc(GetProcessHeap(), 0, lenW * sizeof(WCHAR))))
            MultiByteToWideChar(CP_ACP, 0, string, lenA, stringW, lenW);
    }
    else stringW = NULL;
    if (section) RtlCreateUnicodeStringFromAsciiz(&sectionW, section);
    else sectionW.Buffer = NULL;
    if (filename) RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else filenameW.Buffer = NULL;

    ret = WritePrivateProfileSectionW(sectionW.Buffer, stringW, filenameW.Buffer);

    HeapFree(GetProcessHeap(), 0, stringW);
    RtlFreeUnicodeString(&sectionW);
    RtlFreeUnicodeString(&filenameW);
    return ret;
}

/***********************************************************************
 *           WriteProfileSectionA   (KERNEL32.@)
 */
BOOL WINAPI WriteProfileSectionA( LPCSTR section, LPCSTR keys_n_values)

{
    return WritePrivateProfileSectionA( section, keys_n_values, "win.ini");
}

/***********************************************************************
 *           WriteProfileSectionW   (KERNEL32.@)
 */
BOOL WINAPI WriteProfileSectionW( LPCWSTR section, LPCWSTR keys_n_values)
{
   return WritePrivateProfileSectionW(section, keys_n_values, wininiW);
}


/***********************************************************************
 *           GetPrivateProfileSectionNamesW  (KERNEL32.@)
 *
 * Returns the section names contained in the specified file.
 * FIXME: Where do we find this file when the path is relative?
 * The section names are returned as a list of strings with an extra
 * '\0' to mark the end of the list. Except for that the behavior
 * depends on the Windows version.
 *
 * Win95:
 * - if the buffer is 0 or 1 character long then it is as if it was of
 *   infinite length.
 * - otherwise, if the buffer is too small only the section names that fit
 *   are returned.
 * - note that this means if the buffer was too small to return even just
 *   the first section name then a single '\0' will be returned.
 * - the return value is the number of characters written in the buffer,
 *   except if the buffer was too small in which case len-2 is returned
 *
 * Win2000:
 * - if the buffer is 0, 1 or 2 characters long then it is filled with
 *   '\0' and the return value is 0
 * - otherwise if the buffer is too small then the first section name that
 *   does not fit is truncated so that the string list can be terminated
 *   correctly (double '\0')
 * - the return value is the number of characters written in the buffer
 *   except for the trailing '\0'. If the buffer is too small, then the
 *   return value is len-2
 * - Win2000 has a bug that triggers when the section names and the
 *   trailing '\0' fit exactly in the buffer. In that case the trailing
 *   '\0' is missing.
 *
 * Wine implements the observed Win2000 behavior (except for the bug).
 *
 * Note that when the buffer is big enough then the return value may be any
 * value between 1 and len-1 (or len in Win95), including len-2.
 */
DWORD WINAPI GetPrivateProfileSectionNamesW( LPWSTR buffer, DWORD size,
					     LPCWSTR filename)
{
    DWORD ret = 0;

    RtlEnterCriticalSection( &PROFILE_CritSect );

    if (PROFILE_Open( filename, FALSE ))
        ret = PROFILE_GetSectionNames(buffer, size);

    RtlLeaveCriticalSection( &PROFILE_CritSect );

    return ret;
}


/***********************************************************************
 *           GetPrivateProfileSectionNamesA  (KERNEL32.@)
 */
DWORD WINAPI GetPrivateProfileSectionNamesA( LPSTR buffer, DWORD size,
					     LPCSTR filename)
{
    UNICODE_STRING filenameW;
    LPWSTR bufferW;
    INT retW, ret = 0;

    bufferW = buffer ? HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR)) : NULL;
    if (filename) RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else filenameW.Buffer = NULL;

    retW = GetPrivateProfileSectionNamesW(bufferW, size, filenameW.Buffer);
    if (retW && size)
    {
        ret = WideCharToMultiByte(CP_ACP, 0, bufferW, retW+1, buffer, size-1, NULL, NULL);
        if (!ret)
        {
            ret = size-2;
            buffer[size-1] = 0;
        }
        else
          ret = ret-1;
    }
    else if(size)
        buffer[0] = '\0';

    RtlFreeUnicodeString(&filenameW);
    HeapFree(GetProcessHeap(), 0, bufferW);
    return ret;
}

/***********************************************************************
 *           GetPrivateProfileStructW (KERNEL32.@)
 *
 * Should match Win95's behaviour pretty much
 */
BOOL WINAPI GetPrivateProfileStructW (LPCWSTR section, LPCWSTR key,
                                      LPVOID buf, UINT len, LPCWSTR filename)
{
    BOOL	ret = FALSE;

    RtlEnterCriticalSection( &PROFILE_CritSect );

    if (PROFILE_Open( filename, FALSE )) {
        PROFILEKEY *k = PROFILE_Find ( &CurProfile->section, section, key, FALSE, FALSE);
	if (k) {
	    TRACE("value (at %p): %s\n", k->value, debugstr_w(k->value));
	    if (((strlenW(k->value) - 2) / 2) == len)
	    {
		LPWSTR end, p;
		BOOL valid = TRUE;
		WCHAR c;
		DWORD chksum = 0;

	        end  = k->value + strlenW(k->value); /* -> '\0' */
	        /* check for invalid chars in ASCII coded hex string */
	        for (p=k->value; p < end; p++)
		{
                    if (!isxdigitW(*p))
		    {
			WARN("invalid char '%x' in file %s->[%s]->%s !\n",
                             *p, debugstr_w(filename), debugstr_w(section), debugstr_w(key));
		        valid = FALSE;
		        break;
		    }
		}
		if (valid)
		{
		    BOOL highnibble = TRUE;
		    BYTE b = 0, val;
                    LPBYTE binbuf = buf;

	            end -= 2; /* don't include checksum in output data */
	            /* translate ASCII hex format into binary data */
                    for (p=k->value; p < end; p++)
            	    {
	        	c = toupperW(*p);
			val = (c > '9') ?
				(c - 'A' + 10) : (c - '0');

			if (highnibble)
		    	    b = val << 4;
			else
			{
		    	    b += val;
		    	    *binbuf++ = b; /* feed binary data into output */
		    	    chksum += b; /* calculate checksum */
			}
			highnibble ^= 1; /* toggle */
            	    }
		    /* retrieve stored checksum value */
		    c = toupperW(*p++);
		    b = ( (c > '9') ? (c - 'A' + 10) : (c - '0') ) << 4;
		    c = toupperW(*p);
		    b +=  (c > '9') ? (c - 'A' + 10) : (c - '0');
	            if (b == (chksum & 0xff)) /* checksums match ? */
                        ret = TRUE;
                }
            }
	}
    }
    RtlLeaveCriticalSection( &PROFILE_CritSect );

    return ret;
}

/***********************************************************************
 *           GetPrivateProfileStructA (KERNEL32.@)
 */
BOOL WINAPI GetPrivateProfileStructA (LPCSTR section, LPCSTR key,
				      LPVOID buffer, UINT len, LPCSTR filename)
{
    UNICODE_STRING sectionW, keyW, filenameW;
    INT ret;

    if (section) RtlCreateUnicodeStringFromAsciiz(&sectionW, section);
    else sectionW.Buffer = NULL;
    if (key) RtlCreateUnicodeStringFromAsciiz(&keyW, key);
    else keyW.Buffer = NULL;
    if (filename) RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else filenameW.Buffer = NULL;

    ret = GetPrivateProfileStructW(sectionW.Buffer, keyW.Buffer, buffer, len,
                                   filenameW.Buffer);
    /* Do not translate binary data. */

    RtlFreeUnicodeString(&sectionW);
    RtlFreeUnicodeString(&keyW);
    RtlFreeUnicodeString(&filenameW);
    return ret;
}



/***********************************************************************
 *           WritePrivateProfileStructW (KERNEL32.@)
 */
BOOL WINAPI WritePrivateProfileStructW (LPCWSTR section, LPCWSTR key,
                                        LPVOID buf, UINT bufsize, LPCWSTR filename)
{
    BOOL ret = FALSE;
    LPBYTE binbuf;
    LPWSTR outstring, p;
    DWORD sum = 0;

    if (!section && !key && !buf)  /* flush the cache */
        return WritePrivateProfileStringW( NULL, NULL, NULL, filename );

    /* allocate string buffer for hex chars + checksum hex char + '\0' */
    outstring = HeapAlloc( GetProcessHeap(), 0, (bufsize*2 + 2 + 1) * sizeof(WCHAR) );
    p = outstring;
    for (binbuf = (LPBYTE)buf; binbuf < (LPBYTE)buf+bufsize; binbuf++) {
      *p++ = hex[*binbuf >> 4];
      *p++ = hex[*binbuf & 0xf];
      sum += *binbuf;
    }
    /* checksum is sum & 0xff */
    *p++ = hex[(sum & 0xf0) >> 4];
    *p++ = hex[sum & 0xf];
    *p++ = '\0';

    RtlEnterCriticalSection( &PROFILE_CritSect );

    if (PROFILE_Open( filename, TRUE )) {
        ret = PROFILE_SetString( section, key, outstring, FALSE);
        if (ret) ret = PROFILE_FlushFile();
    }

    RtlLeaveCriticalSection( &PROFILE_CritSect );

    HeapFree( GetProcessHeap(), 0, outstring );

    return ret;
}

/***********************************************************************
 *           WritePrivateProfileStructA (KERNEL32.@)
 */
BOOL WINAPI WritePrivateProfileStructA (LPCSTR section, LPCSTR key,
					LPVOID buf, UINT bufsize, LPCSTR filename)
{
    UNICODE_STRING sectionW, keyW, filenameW;
    INT ret;

    if (section) RtlCreateUnicodeStringFromAsciiz(&sectionW, section);
    else sectionW.Buffer = NULL;
    if (key) RtlCreateUnicodeStringFromAsciiz(&keyW, key);
    else keyW.Buffer = NULL;
    if (filename) RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else filenameW.Buffer = NULL;

    /* Do not translate binary data. */
    ret = WritePrivateProfileStructW(sectionW.Buffer, keyW.Buffer, buf, bufsize,
                                     filenameW.Buffer);

    RtlFreeUnicodeString(&sectionW);
    RtlFreeUnicodeString(&keyW);
    RtlFreeUnicodeString(&filenameW);
    return ret;
}


/***********************************************************************
 *           OpenProfileUserMapping   (KERNEL32.@)
 */
BOOL WINAPI OpenProfileUserMapping(void) {
    FIXME("(), stub!\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           CloseProfileUserMapping   (KERNEL32.@)
 */
BOOL WINAPI CloseProfileUserMapping(void) {
    FIXME("(), stub!\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
