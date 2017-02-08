/*
 * Registry processing routines. Routines, common for registry
 * processing frontends.
 *
 * Copyright 1999 Sylvain St-Germain
 * Copyright 2002 Andriy Palamarchuk
 * Copyright 2008 Alexander N. Sørnes <alex@thehandofagony.com>
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

#include "regedit.h"

#include <assert.h>
#include <fcntl.h>
#include <io.h>
#include <wine/unicode.h>

#define REG_VAL_BUF_SIZE        4096

/* maximal number of characters in hexadecimal data line,
 * including the indentation, but not including the '\' character
 */
#define REG_FILE_HEX_LINE_LEN   (2 + 25 * 3)

const WCHAR* reg_class_namesW[] =
{
    L"HKEY_LOCAL_MACHINE", L"HKEY_USERS", L"HKEY_CLASSES_ROOT",
    L"HKEY_CURRENT_CONFIG", L"HKEY_CURRENT_USER", L"HKEY_DYN_DATA"
};

static HKEY reg_class_keys[] = {
            HKEY_LOCAL_MACHINE, HKEY_USERS, HKEY_CLASSES_ROOT,
            HKEY_CURRENT_CONFIG, HKEY_CURRENT_USER, HKEY_DYN_DATA
        };

#define REG_CLASS_NUMBER (sizeof(reg_class_keys) / sizeof(reg_class_keys[0]))

/* return values */
#define NOT_ENOUGH_MEMORY     1
#define IO_ERROR              2

/* processing macros */

/* common check of memory allocation results */
#define CHECK_ENOUGH_MEMORY(p) \
if (!(p)) \
{ \
    fprintf(stderr,"%S: file %s, line %d: Not enough memory\n", \
            getAppName(), __FILE__, __LINE__); \
    exit(NOT_ENOUGH_MEMORY); \
}

/******************************************************************************
 * Allocates memory and converts input from multibyte to wide chars
 * Returned string must be freed by the caller
 */
WCHAR* GetWideString(const char* strA)
{
    if(strA)
    {
        WCHAR* strW;
        int len = MultiByteToWideChar(CP_ACP, 0, strA, -1, NULL, 0);

        strW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        CHECK_ENOUGH_MEMORY(strW);
        MultiByteToWideChar(CP_ACP, 0, strA, -1, strW, len);
        return strW;
    }
    return NULL;
}

/******************************************************************************
 * Allocates memory and converts input from multibyte to wide chars
 * Returned string must be freed by the caller
 */
static WCHAR* GetWideStringN(const char* strA, int chars, DWORD *len)
{
    if(strA)
    {
        WCHAR* strW;
        *len = MultiByteToWideChar(CP_ACP, 0, strA, chars, NULL, 0);

        strW = HeapAlloc(GetProcessHeap(), 0, *len * sizeof(WCHAR));
        CHECK_ENOUGH_MEMORY(strW);
        MultiByteToWideChar(CP_ACP, 0, strA, chars, strW, *len);
        return strW;
    }
    *len = 0;
    return NULL;
}

/******************************************************************************
 * Allocates memory and converts input from wide chars to multibyte
 * Returned string must be freed by the caller
 */
char* GetMultiByteString(const WCHAR* strW)
{
    if(strW)
    {
        char* strA;
        int len = WideCharToMultiByte(CP_ACP, 0, strW, -1, NULL, 0, NULL, NULL);

        strA = HeapAlloc(GetProcessHeap(), 0, len);
        CHECK_ENOUGH_MEMORY(strA);
        WideCharToMultiByte(CP_ACP, 0, strW, -1, strA, len, NULL, NULL);
        return strA;
    }
    return NULL;
}

/******************************************************************************
 * Allocates memory and converts input from wide chars to multibyte
 * Returned string must be freed by the caller
 */
static char* GetMultiByteStringN(const WCHAR* strW, int chars, DWORD* len)
{
    if(strW)
    {
        char* strA;
        *len = WideCharToMultiByte(CP_ACP, 0, strW, chars, NULL, 0, NULL, NULL);

        strA = HeapAlloc(GetProcessHeap(), 0, *len);
        CHECK_ENOUGH_MEMORY(strA);
        WideCharToMultiByte(CP_ACP, 0, strW, chars, strA, *len, NULL, NULL);
        return strA;
    }
    *len = 0;
    return NULL;
}

/******************************************************************************
 * Converts a hex representation of a DWORD into a DWORD.
 */
static BOOL convertHexToDWord(WCHAR* str, DWORD *dw)
{
    char buf[9];
    char dummy;

    WideCharToMultiByte(CP_ACP, 0, str, -1, buf, 9, NULL, NULL);
    if (lstrlenW(str) > 8 || sscanf(buf, "%lx%c", dw, &dummy) != 1) {
        fprintf(stderr,"%S: ERROR, invalid hex value\n", getAppName());
        return FALSE;
    }
    return TRUE;
}

/******************************************************************************
 * Converts a hex comma separated values list into a binary string.
 */
static BYTE* convertHexCSVToHex(WCHAR *str, DWORD *size)
{
    WCHAR *s;
    BYTE *d, *data;

    /* The worst case is 1 digit + 1 comma per byte */
    *size=(lstrlenW(str)+1)/2;
    data=HeapAlloc(GetProcessHeap(), 0, *size);
    CHECK_ENOUGH_MEMORY(data);

    s = str;
    d = data;
    *size=0;
    while (*s != '\0') {
        UINT wc;
        WCHAR *end;

        wc = strtoulW(s,&end,16);
        if (end == s || wc > 0xff || (*end && *end != ',')) {
            char* strA = GetMultiByteString(s);
            fprintf(stderr,"%S: ERROR converting CSV hex stream. Invalid value at '%s'\n",
                    getAppName(), strA);
            HeapFree(GetProcessHeap(), 0, data);
            HeapFree(GetProcessHeap(), 0, strA);
            return NULL;
        }
        *d++ =(BYTE)wc;
        (*size)++;
        if (*end) end++;
        s = end;
    }

    return data;
}

/******************************************************************************
 * This function returns the HKEY associated with the data type encoded in the
 * value.  It modifies the input parameter (key value) in order to skip this
 * "now useless" data type information.
 *
 * Note: Updated based on the algorithm used in 'server/registry.c'
 */
static DWORD getDataType(LPWSTR *lpValue, DWORD* parse_type)
{
    struct data_type { const WCHAR *tag; int len; int type; int parse_type; };

    static const WCHAR quote[] = {'"'};
    static const WCHAR str[] = {'s','t','r',':','"'};
    static const WCHAR str2[] = {'s','t','r','(','2',')',':','"'};
    static const WCHAR hex[] = {'h','e','x',':'};
    static const WCHAR dword[] = {'d','w','o','r','d',':'};
    static const WCHAR hexp[] = {'h','e','x','('};

    static const struct data_type data_types[] = {                   /* actual type */  /* type to assume for parsing */
                { quote,       1,   REG_SZ,              REG_SZ },
                { str,         5,   REG_SZ,              REG_SZ },
                { str2,        8,   REG_EXPAND_SZ,       REG_SZ },
                { hex,         4,   REG_BINARY,          REG_BINARY },
                { dword,       6,   REG_DWORD,           REG_DWORD },
                { hexp,        4,   -1,                  REG_BINARY },
                { NULL,        0,    0,                  0 }
            };

    const struct data_type *ptr;
    int type;

    for (ptr = data_types; ptr->tag; ptr++) {
        if (strncmpW( ptr->tag, *lpValue, ptr->len ))
            continue;

        /* Found! */
        *parse_type = ptr->parse_type;
        type=ptr->type;
        *lpValue+=ptr->len;
        if (type == -1) {
            WCHAR* end;

            /* "hex(xx):" is special */
            type = (int)strtoulW( *lpValue , &end, 16 );
            if (**lpValue=='\0' || *end!=')' || *(end+1)!=':') {
                type=REG_NONE;
            } else {
                *lpValue = end + 2;
            }
        }
        return type;
    }
    *parse_type=REG_NONE;
    return REG_NONE;
}

/******************************************************************************
 * Replaces escape sequences with the characters.
 */
static void REGPROC_unescape_string(WCHAR* str)
{
    int str_idx = 0;            /* current character under analysis */
    int val_idx = 0;            /* the last character of the unescaped string */
    int len = lstrlenW(str);
    for (str_idx = 0; str_idx < len; str_idx++, val_idx++) {
        if (str[str_idx] == '\\') {
            str_idx++;
            switch (str[str_idx]) {
            case 'n':
                str[val_idx] = '\n';
                break;
            case '\\':
            case '"':
                str[val_idx] = str[str_idx];
                break;
            default:
                fprintf(stderr,"Warning! Unrecognized escape sequence: \\%c'\n",
                        str[str_idx]);
                str[val_idx] = str[str_idx];
                break;
            }
        } else {
            str[val_idx] = str[str_idx];
        }
    }
    str[val_idx] = '\0';
}

static BOOL parseKeyName(LPWSTR lpKeyName, HKEY *hKey, LPWSTR *lpKeyPath)
{
    WCHAR* lpSlash = NULL;
    unsigned int i, len;

    if (lpKeyName == NULL)
        return FALSE;

    for(i = 0; *(lpKeyName+i) != 0; i++)
    {
        if(*(lpKeyName+i) == '\\')
        {
            lpSlash = lpKeyName+i;
            break;
        }
    }

    if (lpSlash)
    {
        len = lpSlash-lpKeyName;
    }
    else
    {
        len = lstrlenW(lpKeyName);
        lpSlash = lpKeyName+len;
    }
    *hKey = NULL;

    for (i = 0; i < REG_CLASS_NUMBER; i++) {
        if (CompareStringW(LOCALE_USER_DEFAULT, 0, lpKeyName, len, reg_class_namesW[i], len) == CSTR_EQUAL &&
            len == lstrlenW(reg_class_namesW[i])) {
            *hKey = reg_class_keys[i];
            break;
        }
    }

    if (*hKey == NULL)
        return FALSE;


    if (*lpSlash != '\0')
        lpSlash++;
    *lpKeyPath = lpSlash;
    return TRUE;
}

/* Globals used by the setValue() & co */
static LPSTR currentKeyName;
static HKEY  currentKeyHandle = NULL;

/******************************************************************************
 * Sets the value with name val_name to the data in val_data for the currently
 * opened key.
 *
 * Parameters:
 * val_name - name of the registry value
 * val_data - registry value data
 */
static LONG setValue(WCHAR* val_name, WCHAR* val_data, BOOL is_unicode)
{
    LONG res;
    DWORD  dwDataType, dwParseType;
    LPBYTE lpbData;
    DWORD  dwData, dwLen;
    WCHAR del[] = {'-',0};

    if ( (val_name == NULL) || (val_data == NULL) )
        return ERROR_INVALID_PARAMETER;

    if (lstrcmpW(val_data, del) == 0)
    {
        res=RegDeleteValueW(currentKeyHandle,val_name);
        return (res == ERROR_FILE_NOT_FOUND ? ERROR_SUCCESS : res);
    }

    /* Get the data type stored into the value field */
    dwDataType = getDataType(&val_data, &dwParseType);

    if (dwParseType == REG_SZ)          /* no conversion for string */
    {
        REGPROC_unescape_string(val_data);
        /* Compute dwLen after REGPROC_unescape_string because it may
         * have changed the string length and we don't want to store
         * the extra garbage in the registry.
         */
        dwLen = lstrlenW(val_data);
        if(val_data[dwLen-1] != '"')
            return ERROR_INVALID_DATA;
        if (dwLen>0 && val_data[dwLen-1]=='"')
        {
            dwLen--;
            val_data[dwLen]='\0';
        }
        lpbData = (BYTE*) val_data;
        dwLen++;  /* include terminating null */
        dwLen = dwLen * sizeof(WCHAR); /* size is in bytes */
    }
    else if (dwParseType == REG_DWORD)  /* Convert the dword types */
    {
        if (!convertHexToDWord(val_data, &dwData))
            return ERROR_INVALID_DATA;
        lpbData = (BYTE*)&dwData;
        dwLen = sizeof(dwData);
    }
    else if (dwParseType == REG_BINARY) /* Convert the binary data */
    {
        lpbData = convertHexCSVToHex(val_data, &dwLen);
        if (!lpbData)
            return ERROR_INVALID_DATA;

        if((dwDataType == REG_MULTI_SZ || dwDataType == REG_EXPAND_SZ) && !is_unicode)
        {
            LPBYTE tmp = lpbData;
            lpbData = (LPBYTE)GetWideStringN((char*)lpbData, dwLen, &dwLen);
            dwLen *= sizeof(WCHAR);
            HeapFree(GetProcessHeap(), 0, tmp);
        }
    }
    else                                /* unknown format */
    {
        fprintf(stderr,"%S: ERROR, unknown data format\n", getAppName());
        return ERROR_INVALID_DATA;
    }

    res = RegSetValueExW(
               currentKeyHandle,
               val_name,
               0,                  /* Reserved */
               dwDataType,
               lpbData,
               dwLen);
    if (dwParseType == REG_BINARY)
        HeapFree(GetProcessHeap(), 0, lpbData);
    return res;
}

/******************************************************************************
 * A helper function for processRegEntry() that opens the current key.
 * That key must be closed by calling closeKey().
 */
static LONG openKeyW(WCHAR* stdInput)
{
    HKEY keyClass;
    WCHAR* keyPath;
    DWORD dwDisp;
    LONG res;

    /* Sanity checks */
    if (stdInput == NULL)
        return ERROR_INVALID_PARAMETER;

    /* Get the registry class */
    if (!parseKeyName(stdInput, &keyClass, &keyPath))
        return ERROR_INVALID_PARAMETER;

    res = RegCreateKeyExW(
               keyClass,                 /* Class     */
               keyPath,                  /* Sub Key   */
               0,                        /* MUST BE 0 */
               NULL,                     /* object type */
               REG_OPTION_NON_VOLATILE,  /* option, REG_OPTION_NON_VOLATILE ... */
               KEY_ALL_ACCESS,           /* access mask, KEY_ALL_ACCESS */
               NULL,                     /* security attribute */
               &currentKeyHandle,        /* result */
               &dwDisp);                 /* disposition, REG_CREATED_NEW_KEY or
                                                        REG_OPENED_EXISTING_KEY */

    if (res == ERROR_SUCCESS)
        currentKeyName = GetMultiByteString(stdInput);
    else
        currentKeyHandle = NULL;

    return res;

}

/******************************************************************************
 * Close the currently opened key.
 */
static void closeKey(void)
{
    if (currentKeyHandle)
    {
        HeapFree(GetProcessHeap(), 0, currentKeyName);
        RegCloseKey(currentKeyHandle);
        currentKeyHandle = NULL;
    }
}

/******************************************************************************
 * This function is a wrapper for the setValue function.  It prepares the
 * land and cleans the area once completed.
 * Note: this function modifies the line parameter.
 *
 * line - registry file unwrapped line. Should have the registry value name and
 *      complete registry value data.
 */
static void processSetValue(WCHAR* line, BOOL is_unicode)
{
    WCHAR* val_name;                   /* registry value name   */
    WCHAR* val_data;                   /* registry value data   */
    int line_idx = 0;                 /* current character under analysis */
    LONG res;

    /* get value name */
    while ( isspaceW(line[line_idx]) ) line_idx++;
    if (line[line_idx] == '@' && line[line_idx + 1] == '=') {
        line[line_idx] = '\0';
        val_name = line;
        line_idx++;
    } else if (line[line_idx] == '\"') {
        line_idx++;
        val_name = line + line_idx;
        while (line[line_idx]) {
            if (line[line_idx] == '\\')   /* skip escaped character */
            {
                line_idx += 2;
            } else {
                if (line[line_idx] == '\"') {
                    line[line_idx] = '\0';
                    line_idx++;
                    break;
                } else {
                    line_idx++;
                }
            }
        }
        while ( isspaceW(line[line_idx]) ) line_idx++;
        if (!line[line_idx]) {
            fprintf(stderr, "%S: warning: unexpected EOL\n", getAppName());
            return;
        }
        if (line[line_idx] != '=') {
            char* lineA;
            line[line_idx] = '\"';
            lineA = GetMultiByteString(line);
            fprintf(stderr,"%S: warning: unrecognized line: '%s'\n", getAppName(), lineA);
            HeapFree(GetProcessHeap(), 0, lineA);
            return;
        }

    } else {
        char* lineA = GetMultiByteString(line);
        fprintf(stderr,"%S: warning: unrecognized line: '%s'\n", getAppName(), lineA);
        HeapFree(GetProcessHeap(), 0, lineA);
        return;
    }
    line_idx++;                   /* skip the '=' character */

    while ( isspaceW(line[line_idx]) ) line_idx++;
    val_data = line + line_idx;
    /* trim trailing blanks */
    line_idx = strlenW(val_data);
    while (line_idx > 0 && isspaceW(val_data[line_idx-1])) line_idx--;
    val_data[line_idx] = '\0';

    REGPROC_unescape_string(val_name);
    res = setValue(val_name, val_data, is_unicode);
    if ( res != ERROR_SUCCESS )
    {
        char* val_nameA = GetMultiByteString(val_name);
        char* val_dataA = GetMultiByteString(val_data);
        fprintf(stderr,"%S: ERROR Key %s not created. Value: %s, Data: %s\n",
                getAppName(),
                currentKeyName,
                val_nameA,
                val_dataA);
        HeapFree(GetProcessHeap(), 0, val_nameA);
        HeapFree(GetProcessHeap(), 0, val_dataA);
    }
}

/******************************************************************************
 * This function receives the currently read entry and performs the
 * corresponding action.
 * isUnicode affects parsing of REG_MULTI_SZ values
 */
static void processRegEntry(WCHAR* stdInput, BOOL isUnicode)
{
    /*
     * We encountered the end of the file, make sure we
     * close the opened key and exit
     */
    if (stdInput == NULL) {
        closeKey();
        return;
    }

    if      ( stdInput[0] == '[')      /* We are reading a new key */
    {
        WCHAR* keyEnd;
        closeKey();                    /* Close the previous key */

        /* Get rid of the square brackets */
        stdInput++;
        keyEnd = strrchrW(stdInput, ']');
        if (keyEnd)
            *keyEnd='\0';

        /* delete the key if we encounter '-' at the start of reg key */
        if ( stdInput[0] == '-')
        {
            delete_registry_key(stdInput + 1);
        } else if ( openKeyW(stdInput) != ERROR_SUCCESS )
        {
            char* stdInputA = GetMultiByteString(stdInput);
            fprintf(stderr,"%S: setValue failed to open key %s\n",
                    getAppName(), stdInputA);
            HeapFree(GetProcessHeap(), 0, stdInputA);
        }
    } else if( currentKeyHandle &&
               (( stdInput[0] == '@') || /* reading a default @=data pair */
                ( stdInput[0] == '\"'))) /* reading a new value=data pair */
    {
        processSetValue(stdInput, isUnicode);
    } else
    {
        /* Since we are assuming that the file format is valid we must be
         * reading a blank line which indicates the end of this key processing
         */
        closeKey();
    }
}

/******************************************************************************
 * Processes a registry file.
 * Correctly processes comments (in # and ; form), line continuation.
 *
 * Parameters:
 *   in - input stream to read from
 *   first_chars - beginning of stream, read due to Unicode check
 */
static void processRegLinesA(FILE *in, char* first_chars)
{
    LPSTR line           = NULL;  /* line read from input stream */
    ULONG lineSize       = REG_VAL_BUF_SIZE;

    line = HeapAlloc(GetProcessHeap(), 0, lineSize);
    CHECK_ENOUGH_MEMORY(line);
    memcpy(line, first_chars, 2);

    while (!feof(in)) {
        LPSTR s; /* The pointer into line for where the current fgets should read */
        WCHAR* lineW;
        s = line;

        if(first_chars)
        {
            s += 2;
            first_chars = NULL;
        }

        for (;;) {
            size_t size_remaining;
            int size_to_get, i;
            char *s_eol; /* various local uses */

            /* Do we need to expand the buffer ? */
            assert (s >= line && s <= line + lineSize);
            size_remaining = lineSize - (s-line);
            if (size_remaining < 2) /* room for 1 character and the \0 */
            {
                char *new_buffer;
                size_t new_size = lineSize + REG_VAL_BUF_SIZE;
                if (new_size > lineSize) /* no arithmetic overflow */
                    new_buffer = HeapReAlloc (GetProcessHeap(), 0, line, new_size);
                else
                    new_buffer = NULL;
                CHECK_ENOUGH_MEMORY(new_buffer);
                line = new_buffer;
                s = line + lineSize - size_remaining;
                lineSize = new_size;
                size_remaining = lineSize - (s-line);
            }

            /* Get as much as possible into the buffer, terminated either by
             * eof, error, eol or getting the maximum amount.  Abort on error.
             */
            size_to_get = (size_remaining > INT_MAX ? INT_MAX : size_remaining);

            /* get a single line. note that `i' must be one past the last
             * meaningful character in `s' when this loop exits */
            for(i = 0; i < size_to_get-1; ++i){
                int xchar;

                xchar = fgetc(in);
                s[i] = xchar;
                if(xchar == EOF){
                    if(ferror(in)){
                        perror("While reading input");
                        exit(IO_ERROR);
                    }else
                        assert(feof(in));
                    break;
                }
                if(s[i] == '\r'){
                    /* read the next character iff it's \n */
                    if(i+2 >= size_to_get){
                        /* buffer too short, so put back the EOL char to
                         * read next cycle */
                        ungetc('\r', in);
                        break;
                    }
                    s[i+1] = fgetc(in);
                    if(s[i+1] != '\n'){
                        ungetc(s[i+1], in);
                        i = i+1;
                    }else
                        i = i+2;
                    break;
                }
                if(s[i] == '\n'){
                    i = i+1;
                    break;
                }
            }
            s[i] = '\0';

            /* If we didn't read the eol nor the eof go around for the rest */
            s_eol = strpbrk (s, "\r\n");
            if (!feof (in) && !s_eol) {
                s = strchr (s, '\0');
                continue;
            }

            /* If it is a comment line then discard it and go around again */
            if (line [0] == '#' || line [0] == ';') {
                s = line;
                continue;
            }

            /* Remove any line feed.  Leave s_eol on the first \0 */
            if (s_eol) {
               if (*s_eol == '\r' && *(s_eol+1) == '\n')
                   *(s_eol+1) = '\0';
               *s_eol = '\0';
            } else
                s_eol = strchr (s, '\0');

            /* If there is a concatenating \\ then go around again */
            if (s_eol > line && *(s_eol-1) == '\\') {
                int c;
                s = s_eol-1;

                do
                {
                    c = fgetc(in);
                } while(c == ' ' || c == '\t');

                if(c == EOF)
                {
                    fprintf(stderr,"%S: ERROR - invalid continuation.\n",
                            getAppName());
                }
                else
                {
                    *s = c;
                    s++;
                }
                continue;
            }

            lineW = GetWideString(line);

            break; /* That is the full virtual line */
        }

        processRegEntry(lineW, FALSE);
        HeapFree(GetProcessHeap(), 0, lineW);
    }
    processRegEntry(NULL, FALSE);

    HeapFree(GetProcessHeap(), 0, line);
}

static void processRegLinesW(FILE *in)
{
    WCHAR* buf           = NULL;  /* line read from input stream */
    ULONG lineSize       = REG_VAL_BUF_SIZE;
    size_t CharsInBuf = -1;

    WCHAR* s; /* The pointer into buf for where the current fgets should read */
    WCHAR* line; /* The start of the current line */

    buf = HeapAlloc(GetProcessHeap(), 0, lineSize * sizeof(WCHAR));
    CHECK_ENOUGH_MEMORY(buf);

    s = buf;
    line = buf;

    while(!feof(in)) {
        size_t size_remaining;
        int size_to_get;
        WCHAR *s_eol = NULL; /* various local uses */

        /* Do we need to expand the buffer ? */
        assert (s >= buf && s <= buf + lineSize);
        size_remaining = lineSize - (s-buf);
        if (size_remaining < 2) /* room for 1 character and the \0 */
        {
            WCHAR *new_buffer;
            size_t new_size = lineSize + (REG_VAL_BUF_SIZE / sizeof(WCHAR));
            if (new_size > lineSize) /* no arithmetic overflow */
                new_buffer = HeapReAlloc (GetProcessHeap(), 0, buf, new_size * sizeof(WCHAR));
            else
                new_buffer = NULL;
            CHECK_ENOUGH_MEMORY(new_buffer);
            buf = new_buffer;
            line = buf;
            s = buf + lineSize - size_remaining;
            lineSize = new_size;
            size_remaining = lineSize - (s-buf);
        }

        /* Get as much as possible into the buffer, terminated either by
        * eof, error or getting the maximum amount.  Abort on error.
        */
        size_to_get = (size_remaining > INT_MAX ? INT_MAX : size_remaining);

        CharsInBuf = fread(s, sizeof(WCHAR), size_to_get - 1, in);
        s[CharsInBuf] = 0;

        if (CharsInBuf == 0) {
            if (ferror(in)) {
                perror ("While reading input");
                exit (IO_ERROR);
            } else {
                assert (feof(in));
                *s = '\0';
                /* It is not clear to me from the definition that the
                * contents of the buffer are well defined on detecting
                * an eof without managing to read anything.
                */
            }
        }

        /* If we didn't read the eol nor the eof go around for the rest */
        while(1)
        {
            const WCHAR line_endings[] = {'\r','\n',0};
            s_eol = strpbrkW(line, line_endings);

            if(!s_eol) {
                /* Move the stub of the line to the start of the buffer so
                 * we get the maximum space to read into, and so we don't
                 * have to recalculate 'line' if the buffer expands */
                MoveMemory(buf, line, (strlenW(line)+1) * sizeof(WCHAR));
                line = buf;
                s = strchrW(line, '\0');
                break;
            }

            /* If it is a comment line then discard it and go around again */
            if (*line == '#' || *line == ';') {
                if (*s_eol == '\r' && *(s_eol+1) == '\n')
                    line = s_eol + 2;
                else
                    line = s_eol + 1;
                continue;
            }

            /* If there is a concatenating \\ then go around again */
            if (*(s_eol-1) == '\\') {
                WCHAR* NextLine = s_eol + 1;

                if(*s_eol == '\r' && *(s_eol+1) == '\n')
                    NextLine++;

                while(*(NextLine+1) == ' ' || *(NextLine+1) == '\t')
                    NextLine++;

                MoveMemory(s_eol - 1, NextLine, (CharsInBuf - (NextLine - s) + 1)*sizeof(WCHAR));
                CharsInBuf -= NextLine - s_eol + 1;
                s_eol = 0;
                continue;
            }

            /* Remove any line feed.  Leave s_eol on the last \0 */
            if (*s_eol == '\r' && *(s_eol + 1) == '\n')
                *s_eol++ = '\0';
            *s_eol = '\0';

            processRegEntry(line, TRUE);
            line = s_eol + 1;
            s_eol = 0;
            continue; /* That is the full virtual line */
        }
    }

    processRegEntry(NULL, TRUE);

    HeapFree(GetProcessHeap(), 0, buf);
}

/****************************************************************************
 * REGPROC_print_error
 *
 * Print the message for GetLastError
 */

static void REGPROC_print_error(void)
{
    LPVOID lpMsgBuf;
    DWORD error_code;
    int status;

    error_code = GetLastError ();
    status = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL, error_code, 0, (LPSTR) &lpMsgBuf, 0, NULL);
    if (!status) {
        fprintf(stderr,"%S: Cannot display message for error %lu, status %lu\n",
                getAppName(), error_code, GetLastError());
        exit(1);
    }
    puts(lpMsgBuf);
    LocalFree(lpMsgBuf);
    exit(1);
}

/******************************************************************************
 * Checks whether the buffer has enough room for the string or required size.
 * Resizes the buffer if necessary.
 *
 * Parameters:
 * buffer - pointer to a buffer for string
 * len - current length of the buffer in characters.
 * required_len - length of the string to place to the buffer in characters.
 *   The length does not include the terminating null character.
 */
static void REGPROC_resize_char_buffer(WCHAR **buffer, DWORD *len, DWORD required_len)
{
    required_len++;
    if (required_len > *len) {
        *len = required_len;
        if (!*buffer)
            *buffer = HeapAlloc(GetProcessHeap(), 0, *len * sizeof(**buffer));
        else
            *buffer = HeapReAlloc(GetProcessHeap(), 0, *buffer, *len * sizeof(**buffer));
        CHECK_ENOUGH_MEMORY(*buffer);
    }
}

/******************************************************************************
 * Same as REGPROC_resize_char_buffer() but on a regular buffer.
 *
 * Parameters:
 * buffer - pointer to a buffer
 * len - current size of the buffer in bytes
 * required_size - size of the data to place in the buffer in bytes
 */
static void REGPROC_resize_binary_buffer(BYTE **buffer, DWORD *size, DWORD required_size)
{
    if (required_size > *size) {
        *size = required_size;
        if (!*buffer)
            *buffer = HeapAlloc(GetProcessHeap(), 0, *size);
        else
            *buffer = HeapReAlloc(GetProcessHeap(), 0, *buffer, *size);
        CHECK_ENOUGH_MEMORY(*buffer);
    }
}

/******************************************************************************
 * Prints string str to file
 */
static void REGPROC_export_string(WCHAR **line_buf, DWORD *line_buf_size, DWORD *line_len, WCHAR *str, DWORD str_len)
{
    DWORD i, pos;
    DWORD extra = 0;

    REGPROC_resize_char_buffer(line_buf, line_buf_size, *line_len + str_len + 10);

    /* escaping characters */
    pos = *line_len;
    for (i = 0; i < str_len; i++) {
        WCHAR c = str[i];
        switch (c) {
        case '\n':
            extra++;
            REGPROC_resize_char_buffer(line_buf, line_buf_size, *line_len + str_len + extra);
            (*line_buf)[pos++] = '\\';
            (*line_buf)[pos++] = 'n';
            break;

        case '\\':
        case '"':
            extra++;
            REGPROC_resize_char_buffer(line_buf, line_buf_size, *line_len + str_len + extra);
            (*line_buf)[pos++] = '\\';
            /* Fall through */

        default:
            (*line_buf)[pos++] = c;
            break;
        }
    }
    (*line_buf)[pos] = '\0';
    *line_len = pos;
}

static void REGPROC_export_binary(WCHAR **line_buf, DWORD *line_buf_size, DWORD *line_len, DWORD type, BYTE *value, DWORD value_size, BOOL unicode)
{
    DWORD hex_pos, data_pos;
    const WCHAR *hex_prefix;
    const WCHAR hex[] = {'h','e','x',':',0};
    WCHAR hex_buf[17];
    const WCHAR concat[] = {'\\','\r','\n',' ',' ',0};
    DWORD concat_prefix, concat_len;
    const WCHAR newline[] = {'\r','\n',0};
    CHAR* value_multibyte = NULL;

    if (type == REG_BINARY) {
        hex_prefix = hex;
    } else {
        const WCHAR hex_format[] = {'h','e','x','(','%','x',')',':',0};
        hex_prefix = hex_buf;
        sprintfW(hex_buf, hex_format, type);
        if ((type == REG_SZ || type == REG_EXPAND_SZ || type == REG_MULTI_SZ) && !unicode)
        {
            value_multibyte = GetMultiByteStringN((WCHAR*)value, value_size / sizeof(WCHAR), &value_size);
            value = (BYTE*)value_multibyte;
        }
    }

    concat_len = lstrlenW(concat);
    concat_prefix = 2;

    hex_pos = *line_len;
    *line_len += lstrlenW(hex_prefix);
    data_pos = *line_len;
    *line_len += value_size * 3;
    /* - The 2 spaces that concat places at the start of the
     *   line effectively reduce the space available for data.
     * - If the value name and hex prefix are very long
     *   ( > REG_FILE_HEX_LINE_LEN) or *line_len divides
     *   without a remainder then we may overestimate
     *   the needed number of lines by one. But that's ok.
     * - The trailing '\r' takes the place of a comma so
     *   we only need to add 1 for the trailing '\n'
     */
    *line_len += *line_len / (REG_FILE_HEX_LINE_LEN - concat_prefix) * concat_len + 1;
    REGPROC_resize_char_buffer(line_buf, line_buf_size, *line_len);
    lstrcpyW(*line_buf + hex_pos, hex_prefix);
    if (value_size)
    {
        const WCHAR format[] = {'%','0','2','x',0};
        DWORD i, column;

        column = data_pos; /* no line wrap yet */
        i = 0;
        while (1)
        {
            sprintfW(*line_buf + data_pos, format, (unsigned int)value[i]);
            data_pos += 2;
            if (++i == value_size)
                break;

            (*line_buf)[data_pos++] = ',';
            column += 3;

            /* wrap the line */
            if (column >= REG_FILE_HEX_LINE_LEN) {
                lstrcpyW(*line_buf + data_pos, concat);
                data_pos += concat_len;
                column = concat_prefix;
            }
        }
    }
    lstrcpyW(*line_buf + data_pos, newline);
    HeapFree(GetProcessHeap(), 0, value_multibyte);
}

/******************************************************************************
 * Writes the given line to a file, in multi-byte or wide characters
 */
static void REGPROC_write_line(FILE *file, const WCHAR* str, BOOL unicode)
{
    if(unicode)
    {
        fwrite(str, sizeof(WCHAR), lstrlenW(str), file);
    } else
    {
        char* strA = GetMultiByteString(str);
        fputs(strA, file);
        HeapFree(GetProcessHeap(), 0, strA);
    }
}

/******************************************************************************
 * Writes contents of the registry key to the specified file stream.
 *
 * Parameters:
 * file - writable file stream to export registry branch to.
 * key - registry branch to export.
 * reg_key_name_buf - name of the key with registry class.
 *      Is resized if necessary.
 * reg_key_name_size - length of the buffer for the registry class in characters.
 * val_name_buf - buffer for storing value name.
 *      Is resized if necessary.
 * val_name_size - length of the buffer for storing value names in characters.
 * val_buf - buffer for storing values while extracting.
 *      Is resized if necessary.
 * val_size - size of the buffer for storing values in bytes.
 */
static void export_hkey(FILE *file, HKEY key,
                 WCHAR **reg_key_name_buf, DWORD *reg_key_name_size,
                 WCHAR **val_name_buf, DWORD *val_name_size,
                 BYTE **val_buf, DWORD *val_size,
                 WCHAR **line_buf, DWORD *line_buf_size,
                 BOOL unicode)
{
    DWORD max_sub_key_len;
    DWORD max_val_name_len;
    DWORD max_val_size;
    DWORD curr_len;
    DWORD i;
    BOOL more_data;
    LONG ret;
    WCHAR key_format[] = {'\r','\n','[','%','s',']','\r','\n',0};

    /* get size information and resize the buffers if necessary */
    if (RegQueryInfoKeyW(key, NULL, NULL, NULL, NULL,
                        &max_sub_key_len, NULL,
                        NULL, &max_val_name_len, &max_val_size, NULL, NULL
                       ) != ERROR_SUCCESS) {
        REGPROC_print_error();
    }
    curr_len = strlenW(*reg_key_name_buf);
    REGPROC_resize_char_buffer(reg_key_name_buf, reg_key_name_size,
                               max_sub_key_len + curr_len + 1);
    REGPROC_resize_char_buffer(val_name_buf, val_name_size,
                               max_val_name_len);
    REGPROC_resize_binary_buffer(val_buf, val_size, max_val_size);
    REGPROC_resize_char_buffer(line_buf, line_buf_size, lstrlenW(*reg_key_name_buf) + 4);
    /* output data for the current key */
    sprintfW(*line_buf, key_format, *reg_key_name_buf);
    REGPROC_write_line(file, *line_buf, unicode);

    /* print all the values */
    i = 0;
    more_data = TRUE;
    while(more_data) {
        DWORD value_type;
        DWORD val_name_size1 = *val_name_size;
        DWORD val_size1 = *val_size;
        ret = RegEnumValueW(key, i, *val_name_buf, &val_name_size1, NULL,
                           &value_type, *val_buf, &val_size1);
        if (ret == ERROR_MORE_DATA) {
            /* Increase the size of the buffers and retry */
            REGPROC_resize_char_buffer(val_name_buf, val_name_size, val_name_size1);
            REGPROC_resize_binary_buffer(val_buf, val_size, val_size1);
        } else if (ret != ERROR_SUCCESS) {
            more_data = FALSE;
            if (ret != ERROR_NO_MORE_ITEMS) {
                REGPROC_print_error();
            }
        } else {
            DWORD line_len;
            i++;

            if ((*val_name_buf)[0]) {
                const WCHAR val_start[] = {'"','%','s','"','=',0};

                line_len = 0;
                REGPROC_export_string(line_buf, line_buf_size, &line_len, *val_name_buf, lstrlenW(*val_name_buf));
                REGPROC_resize_char_buffer(val_name_buf, val_name_size, lstrlenW(*line_buf) + 1);
                lstrcpyW(*val_name_buf, *line_buf);

                line_len = 3 + lstrlenW(*val_name_buf);
                REGPROC_resize_char_buffer(line_buf, line_buf_size, line_len);
                sprintfW(*line_buf, val_start, *val_name_buf);
            } else {
                const WCHAR std_val[] = {'@','=',0};
                line_len = 2;
                REGPROC_resize_char_buffer(line_buf, line_buf_size, line_len);
                lstrcpyW(*line_buf, std_val);
            }

            switch (value_type) {
            case REG_SZ:
            {
                WCHAR* wstr = (WCHAR*)*val_buf;

                if (val_size1 < sizeof(WCHAR) || val_size1 % sizeof(WCHAR) ||
                    wstr[val_size1 / sizeof(WCHAR) - 1]) {
                    REGPROC_export_binary(line_buf, line_buf_size, &line_len, value_type, *val_buf, val_size1, unicode);
                } else {
                    const WCHAR start[] = {'"',0};
                    const WCHAR end[] = {'"','\r','\n',0};
                    DWORD len;

                    len = lstrlenW(start);
                    REGPROC_resize_char_buffer(line_buf, line_buf_size, line_len + len);
                    lstrcpyW(*line_buf + line_len, start);
                    line_len += len;

                    /* At this point we know wstr is '\0'-terminated
                     * so we can subtract 1 from the size
                     */
                    REGPROC_export_string(line_buf, line_buf_size, &line_len, wstr, val_size1 / sizeof(WCHAR) - 1);

                    REGPROC_resize_char_buffer(line_buf, line_buf_size, line_len + lstrlenW(end));
                    lstrcpyW(*line_buf + line_len, end);
                }
                break;
            }

            case REG_DWORD:
            {
                WCHAR format[] = {'d','w','o','r','d',':','%','0','8','x','\r','\n',0};

                REGPROC_resize_char_buffer(line_buf, line_buf_size, line_len + 15);
                sprintfW(*line_buf + line_len, format, *((DWORD *)*val_buf));
                break;
            }

            default:
            {
                char* key_nameA = GetMultiByteString(*reg_key_name_buf);
                char* value_nameA = GetMultiByteString(*val_name_buf);
                fprintf(stderr,"%S: warning - unsupported registry format '%ld', "
                        "treat as binary\n",
                        getAppName(), value_type);
                fprintf(stderr,"key name: \"%s\"\n", key_nameA);
                fprintf(stderr,"value name:\"%s\"\n\n", value_nameA);
                HeapFree(GetProcessHeap(), 0, key_nameA);
                HeapFree(GetProcessHeap(), 0, value_nameA);
            }
                /* falls through */
            case REG_EXPAND_SZ:
            case REG_MULTI_SZ:
                /* falls through */
            case REG_BINARY:
                REGPROC_export_binary(line_buf, line_buf_size, &line_len, value_type, *val_buf, val_size1, unicode);
            }
            REGPROC_write_line(file, *line_buf, unicode);
        }
    }

    i = 0;
    more_data = TRUE;
    (*reg_key_name_buf)[curr_len] = '\\';
    while(more_data) {
        DWORD buf_size = *reg_key_name_size - curr_len - 1;

        ret = RegEnumKeyExW(key, i, *reg_key_name_buf + curr_len + 1, &buf_size,
                           NULL, NULL, NULL, NULL);
        if (ret == ERROR_MORE_DATA) {
            /* Increase the size of the buffer and retry */
            REGPROC_resize_char_buffer(reg_key_name_buf, reg_key_name_size, curr_len + 1 + buf_size);
        } else if (ret != ERROR_SUCCESS) {
            more_data = FALSE;
            if (ret != ERROR_NO_MORE_ITEMS) {
                REGPROC_print_error();
            }
        } else {
            HKEY subkey;

            i++;
            if (RegOpenKeyW(key, *reg_key_name_buf + curr_len + 1,
                           &subkey) == ERROR_SUCCESS) {
                export_hkey(file, subkey, reg_key_name_buf, reg_key_name_size,
                            val_name_buf, val_name_size, val_buf, val_size,
                            line_buf, line_buf_size, unicode);
                RegCloseKey(subkey);
            } else {
                REGPROC_print_error();
            }
        }
    }
    (*reg_key_name_buf)[curr_len] = '\0';
}

/******************************************************************************
 * Open file in binary mode for export.
 */
static FILE *REGPROC_open_export_file(WCHAR *file_name, BOOL unicode)
{
    FILE *file;
    WCHAR dash = '-';

    if (strncmpW(file_name,&dash,1)==0) {
        file=stdout;
        _setmode(_fileno(file), _O_BINARY);
    } else
    {
        CHAR* file_nameA = GetMultiByteString(file_name);
        file = fopen(file_nameA, "wb");
        if (!file) {
            perror("");
            fprintf(stderr,"%S: Can't open file \"%s\"\n", getAppName(), file_nameA);
            HeapFree(GetProcessHeap(), 0, file_nameA);
            exit(1);
        }
        HeapFree(GetProcessHeap(), 0, file_nameA);
    }
    if(unicode)
    {
        const BYTE unicode_seq[] = {0xff,0xfe};
        const WCHAR header[] = {'W','i','n','d','o','w','s',' ','R','e','g','i','s','t','r','y',' ','E','d','i','t','o','r',' ','V','e','r','s','i','o','n',' ','5','.','0','0','\r','\n'};
        fwrite(unicode_seq, sizeof(BYTE), sizeof(unicode_seq)/sizeof(unicode_seq[0]), file);
        fwrite(header, sizeof(WCHAR), sizeof(header)/sizeof(header[0]), file);
    } else
    {
        fputs("REGEDIT4\r\n", file);
    }

    return file;
}

/******************************************************************************
 * Writes contents of the registry key to the specified file stream.
 *
 * Parameters:
 * file_name - name of a file to export registry branch to.
 * reg_key_name - registry branch to export. The whole registry is exported if
 *      reg_key_name is NULL or contains an empty string.
 */
BOOL export_registry_key(WCHAR *file_name, WCHAR *reg_key_name, DWORD format)
{
    WCHAR *reg_key_name_buf;
    WCHAR *val_name_buf;
    BYTE *val_buf;
    WCHAR *line_buf;
    DWORD reg_key_name_size = KEY_MAX_LEN;
    DWORD val_name_size = KEY_MAX_LEN;
    DWORD val_size = REG_VAL_BUF_SIZE;
    DWORD line_buf_size = KEY_MAX_LEN + REG_VAL_BUF_SIZE;
    FILE *file = NULL;
    BOOL unicode = (format == REG_FORMAT_5);

    reg_key_name_buf = HeapAlloc(GetProcessHeap(), 0,
                                 reg_key_name_size  * sizeof(*reg_key_name_buf));
    val_name_buf = HeapAlloc(GetProcessHeap(), 0,
                             val_name_size * sizeof(*val_name_buf));
    val_buf = HeapAlloc(GetProcessHeap(), 0, val_size);
    line_buf = HeapAlloc(GetProcessHeap(), 0, line_buf_size * sizeof(*line_buf));
    CHECK_ENOUGH_MEMORY(reg_key_name_buf && val_name_buf && val_buf && line_buf);

    if (reg_key_name && reg_key_name[0]) {
        HKEY reg_key_class;
        WCHAR *branch_name = NULL;
        HKEY key;

        REGPROC_resize_char_buffer(&reg_key_name_buf, &reg_key_name_size,
                                   lstrlenW(reg_key_name));
        lstrcpyW(reg_key_name_buf, reg_key_name);

        /* open the specified key */
        if (!parseKeyName(reg_key_name, &reg_key_class, &branch_name)) {
            CHAR* key_nameA = GetMultiByteString(reg_key_name);
            fprintf(stderr,"%S: Incorrect registry class specification in '%s'\n",
                    getAppName(), key_nameA);
            HeapFree(GetProcessHeap(), 0, key_nameA);
            exit(1);
        }
        if (!branch_name[0]) {
            /* no branch - registry class is specified */
            file = REGPROC_open_export_file(file_name, unicode);
            export_hkey(file, reg_key_class,
                        &reg_key_name_buf, &reg_key_name_size,
                        &val_name_buf, &val_name_size,
                        &val_buf, &val_size, &line_buf,
                        &line_buf_size, unicode);
        } else if (RegOpenKeyW(reg_key_class, branch_name, &key) == ERROR_SUCCESS) {
            file = REGPROC_open_export_file(file_name, unicode);
            export_hkey(file, key,
                        &reg_key_name_buf, &reg_key_name_size,
                        &val_name_buf, &val_name_size,
                        &val_buf, &val_size, &line_buf,
                        &line_buf_size, unicode);
            RegCloseKey(key);
        } else {
            CHAR* key_nameA = GetMultiByteString(reg_key_name);
            fprintf(stderr,"%S: Can't export. Registry key '%s' does not exist!\n",
                    getAppName(), key_nameA);
            HeapFree(GetProcessHeap(), 0, key_nameA);
            REGPROC_print_error();
        }
    } else {
        unsigned int i;

        /* export all registry classes */
        file = REGPROC_open_export_file(file_name, unicode);
        for (i = 0; i < REG_CLASS_NUMBER; i++) {
            /* do not export HKEY_CLASSES_ROOT */
            if (reg_class_keys[i] != HKEY_CLASSES_ROOT &&
                    reg_class_keys[i] != HKEY_CURRENT_USER &&
                    reg_class_keys[i] != HKEY_CURRENT_CONFIG &&
                    reg_class_keys[i] != HKEY_DYN_DATA) {
                lstrcpyW(reg_key_name_buf, reg_class_namesW[i]);
                export_hkey(file, reg_class_keys[i],
                            &reg_key_name_buf, &reg_key_name_size,
                            &val_name_buf, &val_name_size,
                            &val_buf, &val_size, &line_buf,
                            &line_buf_size, unicode);
            }
        }
    }

    if (file) {
        fclose(file);
    }
    HeapFree(GetProcessHeap(), 0, reg_key_name);
    HeapFree(GetProcessHeap(), 0, val_name_buf);
    HeapFree(GetProcessHeap(), 0, val_buf);
    HeapFree(GetProcessHeap(), 0, line_buf);
    return TRUE;
}

/******************************************************************************
 * Reads contents of the specified file into the registry.
 */
BOOL import_registry_file(FILE* reg_file)
{
    if (reg_file)
    {
        BYTE s[2];
        if (fread( s, 2, 1, reg_file) == 1)
        {
            if (s[0] == 0xff && s[1] == 0xfe)
            {
                processRegLinesW(reg_file);
            } else
            {
                processRegLinesA(reg_file, (char*)s);
            }
        }
        return TRUE;
    }
    return FALSE;
}

/******************************************************************************
 * Removes the registry key with all subkeys. Parses full key name.
 *
 * Parameters:
 * reg_key_name - full name of registry branch to delete. Ignored if is NULL,
 *      empty, points to register key class, does not exist.
 */
void delete_registry_key(WCHAR *reg_key_name)
{
    WCHAR *key_name = NULL;
    HKEY key_class;

    if (!reg_key_name || !reg_key_name[0])
        return;

    if (!parseKeyName(reg_key_name, &key_class, &key_name)) {
        char* reg_key_nameA = GetMultiByteString(reg_key_name);
        fprintf(stderr,"%S: Incorrect registry class specification in '%s'\n",
                getAppName(), reg_key_nameA);
        HeapFree(GetProcessHeap(), 0, reg_key_nameA);
        exit(1);
    }
    if (!*key_name) {
        char* reg_key_nameA = GetMultiByteString(reg_key_name);
        fprintf(stderr,"%S: Can't delete registry class '%s'\n",
                getAppName(), reg_key_nameA);
        HeapFree(GetProcessHeap(), 0, reg_key_nameA);
        exit(1);
    }

    SHDeleteKey(key_class, key_name);
}
