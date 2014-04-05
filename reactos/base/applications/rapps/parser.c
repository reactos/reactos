/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/parser.c
 * PURPOSE:         Parser functions
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "rapps.h"

typedef LONG NTSTATUS;

typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

void WINAPI RtlInitUnicodeString(PUNICODE_STRING,PCWSTR);
NTSTATUS WINAPI RtlUnicodeStringToInteger(const UNICODE_STRING*,ULONG,ULONG*);
BOOLEAN WINAPI RtlIsTextUnicode(LPCVOID,INT,INT*);

static const char bom_utf8[] = {0xEF,0xBB,0xBF};

typedef enum
{
    ENCODING_UTF8 = 1,
    ENCODING_UTF16LE,
    ENCODING_UTF16BE
} ENCODING;

typedef struct tagSECTIONKEY
{
    WCHAR *value;
    struct tagSECTIONKEY *next;
    WCHAR name[1];
} SECTIONKEY;

typedef struct tagSECTION
{
    struct tagSECTIONKEY *key;
    struct tagSECTION *next;
    WCHAR name[1];
} SECTION;

typedef struct
{
    BOOL changed;
    SECTION *section;
    WCHAR *filename;
    ENCODING encoding;
} ITEMS;


#define N_CACHED_ITEMS 10
static ITEMS *ItemsArray[N_CACHED_ITEMS] = {NULL};
#define CurProfile (ItemsArray[0])
#define IS_ENTRY_COMMENT(str)  ((str)[0] == ';')
#define ParserIsSpace(c) (iswspace(c) || c == 0x1a)


static
WCHAR*
memchrW(const WCHAR *ptr, WCHAR ch, size_t n)
{
    const WCHAR *end;
    for (end = ptr + n; ptr < end; ptr++)
        if (*ptr == ch)
            return (WCHAR *)(ULONG_PTR)ptr;
    return NULL;
}

static
WCHAR
*memrchrW(const WCHAR *ptr, WCHAR ch, size_t n)
{
    const WCHAR *end;
    WCHAR *ret = NULL;
    for (end = ptr + n; ptr < end; ptr++)
        if (*ptr == ch)
            ret = (WCHAR *)(ULONG_PTR)ptr;
    return ret;
}

static
void
ParserCopyEntry(LPWSTR buffer, LPCWSTR value, int len, BOOL strip_quote)
{
    WCHAR quote = '\0';

    if (!buffer) return;

    if (strip_quote && ((*value == '\'') || (*value == '\"')))
    {
        if (value[1] && (value[wcslen(value)-1] == *value))
            quote = *value++;
    }

    lstrcpynW(buffer, value, len);
    if (quote && (len >= (int)wcslen(value))) buffer[wcslen(buffer)-1] = '\0';
}

static
void
ParserByteSwapShortBuffer(WCHAR * buffer, int len)
{
    int i;
    USHORT * shortbuffer = buffer;
    for (i = 0; i < len; i++)
        shortbuffer[i] = (shortbuffer[i] >> 8) | (shortbuffer[i] << 8);
}

static
void
ParserWriteMarker(HANDLE hFile, ENCODING encoding)
{
    DWORD dwBytesWritten;
    WCHAR bom;

    switch (encoding)
    {
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

static
void
ParserWriteLine(HANDLE hFile, WCHAR * szLine, int len, ENCODING encoding)
{
    char * write_buffer;
    int write_buffer_len;
    DWORD dwBytesWritten;

    switch (encoding)
    {
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
            ParserByteSwapShortBuffer(szLine, len);
            WriteFile(hFile, szLine, len * sizeof(WCHAR), &dwBytesWritten, NULL);
            break;
    }
}

static
void
ParserSave(HANDLE hFile, const SECTION *section, ENCODING encoding)
{
    SECTIONKEY *key;
    WCHAR *buffer, *p;

    ParserWriteMarker(hFile, encoding);

    for ( ; section; section = section->next)
    {
        size_t len = 0;
        size_t remaining;

        if (section->name[0]) len += wcslen(section->name) + 4;

        for (key = section->key; key; key = key->next)
        {
            len += wcslen(key->name) + 2;
            if (key->value) len += wcslen(key->value) + 1;
        }

        buffer = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!buffer) return;

        p = buffer;
        remaining = len;
        if (section->name[0])
        {
            StringCchPrintfExW(p, remaining, &p, &remaining, 0,
                               L"[%ls]\r\n",
                               section->name);
        }

        for (key = section->key; key; key = key->next)
        {
            if (key->value)
            {
                StringCchPrintfExW(p, remaining, &p, &remaining, 0,
                                   L"%ls=%ls\r\n",
                                   key->name, key->value);
            }
            else
            {
                StringCchPrintfExW(p, remaining, &p, &remaining, 0,
                                   L"%ls\r\n",
                                   key->name);
            }
        }
        ParserWriteLine(hFile, buffer, len, encoding);
        HeapFree(GetProcessHeap(), 0, buffer);
    }
}

static
void
ParserFree(SECTION *section)
{
    SECTION *next_section;
    SECTIONKEY *key, *next_key;

    for ( ; section; section = next_section)
    {
        for (key = section->key; key; key = next_key)
        {
            next_key = key->next;
            HeapFree(GetProcessHeap(), 0, key->value);
            HeapFree(GetProcessHeap(), 0, key);
        }
        next_section = section->next;
        HeapFree(GetProcessHeap(), 0, section);
    }
}

static
ENCODING
ParserDetectTextEncoding(const void * buffer, int * len)
{
    INT flags = IS_TEXT_UNICODE_SIGNATURE |
                IS_TEXT_UNICODE_REVERSE_SIGNATURE |
                IS_TEXT_UNICODE_ODD_LENGTH;

    if (*len >= sizeof(bom_utf8) && !memcmp(buffer, bom_utf8, sizeof(bom_utf8)))
    {
        *len = sizeof(bom_utf8);
        return ENCODING_UTF8;
    }

    RtlIsTextUnicode((void *)buffer, *len, &flags);

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

    return ENCODING_UTF8;
}

static
SECTION
*ParserLoad(HANDLE hFile, ENCODING * pEncoding)
{
    void *buffer_base, *pBuffer;
    WCHAR * szFile;
    const WCHAR *szLineStart, *szLineEnd;
    const WCHAR *szValueStart, *szEnd, *next_line;
    int line = 0, len;
    SECTION *section, *first_section;
    SECTION **next_section;
    SECTIONKEY *key, *prev_key, **next_key;
    DWORD dwFileSize;

    dwFileSize = GetFileSize(hFile, NULL);
    if (dwFileSize == INVALID_FILE_SIZE || dwFileSize == 0)
        return NULL;

    buffer_base = HeapAlloc(GetProcessHeap(), 0 , dwFileSize);
    if (!buffer_base)
        return NULL;

    if (!ReadFile(hFile, buffer_base, dwFileSize, &dwFileSize, NULL))
    {
        HeapFree(GetProcessHeap(), 0, buffer_base);
        return NULL;
    }

    len = dwFileSize;
    *pEncoding = ParserDetectTextEncoding(buffer_base, &len);

    pBuffer = (char *)buffer_base + len;
    dwFileSize -= len;

    switch (*pEncoding)
    {
        case ENCODING_UTF8:
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
            szFile = pBuffer;
            szEnd = (WCHAR *)((char *)pBuffer + dwFileSize);
            break;

        case ENCODING_UTF16BE:
            szFile = pBuffer;
            szEnd = (WCHAR *)((char *)pBuffer + dwFileSize);
            ParserByteSwapShortBuffer(szFile, dwFileSize / sizeof(WCHAR));
            break;

        default:
            HeapFree(GetProcessHeap(), 0, buffer_base);
            return NULL;
    }

    first_section = HeapAlloc(GetProcessHeap(), 0, sizeof(*section));
    if (first_section == NULL)
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
    next_key = &first_section->key;
    prev_key = NULL;
    next_line = szFile;

    while (next_line < szEnd)
    {
        szLineStart = next_line;
        next_line = memchrW(szLineStart, '\n', szEnd - szLineStart);
        if (!next_line) next_line = memchrW(szLineStart, '\r', szEnd - szLineStart);
        if (!next_line) next_line = szEnd;
        else next_line++;
        szLineEnd = next_line;

        line++;

        while (szLineStart < szLineEnd && ParserIsSpace(*szLineStart)) szLineStart++;
        while ((szLineEnd > szLineStart) && ParserIsSpace(szLineEnd[-1])) szLineEnd--;

        if (szLineStart >= szLineEnd)
            continue;

        if (*szLineStart == '[')
        {
            const WCHAR * szSectionEnd;
            if ((szSectionEnd = memrchrW(szLineStart, ']', szLineEnd - szLineStart)))
            {
                szLineStart++;
                len = (int)(szSectionEnd - szLineStart);
                if (!(section = HeapAlloc(GetProcessHeap(), 0, sizeof(*section) + len * sizeof(WCHAR))))
                    break;
                memcpy(section->name, szLineStart, len * sizeof(WCHAR));
                section->name[len] = '\0';
                section->key  = NULL;
                section->next = NULL;
                *next_section = section;
                next_section = &section->next;
                next_key = &section->key;
                prev_key = NULL;

                continue;
            }
        }

        len = szLineEnd - szLineStart;
        if ((szValueStart = memchrW(szLineStart, '=', szLineEnd - szLineStart)) != NULL)
        {
            const WCHAR *szNameEnd = szValueStart;
            while ((szNameEnd > szLineStart) && ParserIsSpace(szNameEnd[-1])) szNameEnd--;
            len = szNameEnd - szLineStart;
            szValueStart++;
            while (szValueStart < szLineEnd && ParserIsSpace(*szValueStart)) szValueStart++;
        }

        if (len || !prev_key || *prev_key->name)
        {
            if (!(key = HeapAlloc(GetProcessHeap(), 0, sizeof(*key) + len * sizeof(WCHAR)))) break;
            memcpy(key->name, szLineStart, len * sizeof(WCHAR));
            key->name[len] = '\0';
            if (szValueStart)
            {
                len = (int)(szLineEnd - szValueStart);
                key->value = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
                memcpy(key->value, szValueStart, len * sizeof(WCHAR));
                key->value[len] = '\0';
            }
            else key->value = NULL;

           key->next  = NULL;
           *next_key  = key;
           next_key   = &key->next;
           prev_key   = key;
        }
    }

    if (szFile != pBuffer)
        HeapFree(GetProcessHeap(), 0, szFile);
    HeapFree(GetProcessHeap(), 0, buffer_base);

    return first_section;
}

static
SECTIONKEY
*ParserFind(SECTION **section, LPCWSTR section_name, LPCWSTR key_name, BOOL create, BOOL create_always)
{
    LPCWSTR p;
    DWORD cch;
    int seclen, keylen;

    while (ParserIsSpace(*section_name)) section_name++;
    if (*section_name)
        p = section_name + wcslen(section_name) - 1;
    else
        p = section_name;

    while ((p > section_name) && ParserIsSpace(*p)) p--;
    seclen = p - section_name + 1;

    while (ParserIsSpace(*key_name)) key_name++;
    if (*key_name)
        p = key_name + wcslen(key_name) - 1;
    else
        p = key_name;

    while ((p > key_name) && ParserIsSpace(*p)) p--;
    keylen = p - key_name + 1;

    while (*section)
    {
        if (((*section)->name[0])
             && (!(_wcsnicmp((*section)->name, section_name, seclen)))
             && (((*section)->name)[seclen] == '\0'))
        {
            SECTIONKEY **key = &(*section)->key;

            while (*key)
            {
                if(!create_always)
                {
                    if ((!(_wcsnicmp((*key)->name, key_name, keylen)))
                         && (((*key)->name)[keylen] == '\0'))
                        return *key;
                }
                key = &(*key)->next;
            }
            if (!create)
                return NULL;
            cch = wcslen(key_name) + 1;
            if (!(*key = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(SECTIONKEY, name) + cch * sizeof(WCHAR))))
                return NULL;
            StringCchCopyW((*key)->name, cch, key_name);
            (*key)->value = NULL;
            (*key)->next  = NULL;
            return *key;
        }
        section = &(*section)->next;
    }
    if (!create) return NULL;
    cch = wcslen(section_name) + 1;
    *section = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(SECTION, name) + cch * sizeof(WCHAR));
    if (*section == NULL) return NULL;
    StringCchCopyW((*section)->name, cch, section_name);
    (*section)->next = NULL;
    cch = wcslen(key_name) + 1;
    if (!((*section)->key  = HeapAlloc(GetProcessHeap(), 0,
                                        FIELD_OFFSET(SECTIONKEY, name) + cch * sizeof(WCHAR))))
    {
        HeapFree(GetProcessHeap(), 0, *section);
        return NULL;
    }
    StringCchCopyW((*section)->key->name, cch, key_name);
    (*section)->key->value = NULL;
    (*section)->key->next  = NULL;
    return (*section)->key;
}

static
BOOL
ParserFlushFile(void)
{
    HANDLE hFile = NULL;

    if (!CurProfile) return FALSE;

    if (!CurProfile->changed) return TRUE;

    hFile = CreateFileW(CurProfile->filename, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    ParserSave(hFile, CurProfile->section, CurProfile->encoding);

    CloseHandle(hFile);
    CurProfile->changed = FALSE;
    return TRUE;
}

static
void
ParserReleaseFile(void)
{
    ParserFlushFile();
    ParserFree(CurProfile->section);
    HeapFree(GetProcessHeap(), 0, CurProfile->filename);
    CurProfile->changed = FALSE;
    CurProfile->section = NULL;
    CurProfile->filename = NULL;
    CurProfile->encoding = ENCODING_UTF8;
}

static
BOOL
ParserOpen(LPCWSTR filename, BOOL write_access)
{
    WCHAR szDir[MAX_PATH];
    WCHAR buffer[MAX_PATH];
    DWORD cch;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    int i, j;
    ITEMS *tempProfile;

    if (!CurProfile)
        for (i = 0; i < N_CACHED_ITEMS; i++)
        {
            ItemsArray[i] = HeapAlloc(GetProcessHeap(), 0, sizeof(ITEMS));
            if (ItemsArray[i] == NULL) break;
            ItemsArray[i]->changed = FALSE;
            ItemsArray[i]->section = NULL;
            ItemsArray[i]->filename = NULL;
            ItemsArray[i]->encoding = ENCODING_UTF8;
        }

    if (!GetStorageDirectory(szDir, sizeof(szDir) / sizeof(szDir[0])))
        return FALSE;

    if (FAILED(StringCbPrintfW(buffer, sizeof(buffer),
                               L"%ls\\rapps\\%ls",
                               szDir, filename)))
    {
        return FALSE;
    }

    hFile = CreateFileW(buffer, GENERIC_READ | (write_access ? GENERIC_WRITE : 0),
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if ((hFile == INVALID_HANDLE_VALUE) && (GetLastError() != ERROR_FILE_NOT_FOUND))
    {
        return FALSE;
    }

    for (i = 0; i < N_CACHED_ITEMS; i++)
    {
        if ((ItemsArray[i]->filename && !wcscmp(buffer, ItemsArray[i]->filename)))
        {
            if (i)
            {
                ParserFlushFile();
                tempProfile = ItemsArray[i];
                for (j = i; j > 0; j--)
                    ItemsArray[j] = ItemsArray[j - 1];
                CurProfile = tempProfile;
            }
            if (hFile != INVALID_HANDLE_VALUE)
            {
                CloseHandle(hFile);
            }

            return TRUE;
        }
    }

    ParserFlushFile();

    if (i == N_CACHED_ITEMS)
    {
        tempProfile = ItemsArray[N_CACHED_ITEMS - 1];
        for (i = N_CACHED_ITEMS - 1; i > 0; i--)
            ItemsArray[i] = ItemsArray[i - 1];
        CurProfile = tempProfile;
    }

    if (CurProfile->filename) ParserReleaseFile();

    cch = wcslen(buffer) + 1;
    CurProfile->filename = HeapAlloc(GetProcessHeap(), 0, cch * sizeof(WCHAR));
    if (CurProfile->filename == NULL)
    {
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
        return FALSE;
    }

    StringCchCopyW(CurProfile->filename, cch, buffer);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CurProfile->section = ParserLoad(hFile, &CurProfile->encoding);
        CloseHandle(hFile);
    }
    return TRUE;
}

static
INT
ParserGetSection(SECTION *section, LPCWSTR section_name, LPWSTR buffer, UINT len, BOOL return_values)
{
    SECTIONKEY *key;

    if (!buffer)
        return 0;

    while (section)
    {
        if (section->name[0] && !_wcsicmp(section->name, section_name))
        {
            UINT oldlen = len;
            for (key = section->key; key; key = key->next)
            {
                if (len <= 2) break;
                if (!*key->name) continue;  /* Skip empty lines */
                if (IS_ENTRY_COMMENT(key->name)) continue;  /* Skip comments */
                if (!return_values && !key->value) continue;  /* Skip lines w.o. '=' */

                ParserCopyEntry(buffer, key->name, len - 1, 0);
                len -= wcslen(buffer) + 1;
                buffer += wcslen(buffer) + 1;

                if (len < 2) break;
                if (return_values && key->value)
                {
                    buffer[-1] = '=';
                    ParserCopyEntry(buffer, key->value, len - 1, 0);
                    len -= wcslen(buffer) + 1;
                    buffer += wcslen(buffer) + 1;
                }
            }
            *buffer = '\0';
            if (len <= 1)
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

static
INT
ParserInternalGetString(LPCWSTR section, LPCWSTR key_name, LPWSTR buffer, UINT len)
{
    SECTIONKEY *key = NULL;
    static const WCHAR empty_strW[] = { 0 };

    if (!buffer || !len) return 0;

    if (key_name)
    {
        if (!key_name[0])
        {
            ParserCopyEntry(buffer, empty_strW, len, TRUE);
            return wcslen(buffer);
        }
        key = ParserFind(&CurProfile->section, section, key_name, FALSE, FALSE);
        ParserCopyEntry(buffer, (key && key->value) ? key->value : empty_strW,
                           len, TRUE);
        return wcslen(buffer);
    }

    if (section && section[0])
    {
        INT ret = ParserGetSection(CurProfile->section, section, buffer, len, FALSE);
        if (!buffer[0])
        {
            ParserCopyEntry(buffer, empty_strW, len, TRUE);
            ret = wcslen(buffer);
        }
        return ret;
    }

    buffer[0] = '\0';
    return 0;
}

INT
ParserGetString(LPCWSTR Section, LPCWSTR ValueName, LPWSTR Buffer, UINT Len, LPCWSTR FileName)
{
    if (Section == NULL) return 0;

    if (ParserOpen(FileName, FALSE))
        return ParserInternalGetString(Section, ValueName, Buffer, Len);

    return 0;
}

UINT
ParserGetInt(LPCWSTR Section, LPCWSTR ValueName, LPCWSTR FileName)
{
    WCHAR Buffer[30];
    UNICODE_STRING BufferW;
    ULONG Result;

    if (!ParserGetString(Section,
                         ValueName,
                         Buffer,
                         sizeof(Buffer) / sizeof(WCHAR),
                         FileName))
        return -1;

    if (!Buffer[0]) return -1;

    RtlInitUnicodeString(&BufferW, Buffer);
    RtlUnicodeStringToInteger(&BufferW, 0, &Result);
    return Result;
}
