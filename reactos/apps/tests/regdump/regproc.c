/*
 * Registry processing routines. Routines, common for registry
 * processing frontends.
 *
 * Copyright 1999 Sylvain St-Germain
 * Copyright 2002 Andriy Palamarchuk
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

#ifdef WIN32_REGDBG
#include <windows.h>
#include <tchar.h>
#ifndef __GNUC__
#include <ntsecapi.h>
#else
#include <ctype.h>
#endif
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
//#include <winreg.h>
#include "regdump.h"
#else

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#include <wchar.h>
    
#include <ctype.h>
#include <limits.h>
#include <winnt.h>
#include <winreg.h>
#include <assert.h>

#endif

#include "regproc.h"


#define REG_VAL_BUF_SIZE        4096

/* Delimiters used to parse the "value" to query queryValue*/
#define QUERY_VALUE_MAX_ARGS  1

/* maximal number of characters in hexadecimal data line,
   not including '\' character */
#define REG_FILE_HEX_LINE_LEN   76

/* Globals used by the api setValue, queryValue */
static LPTSTR currentKeyName   = NULL;
static HKEY  currentKeyClass  = 0;
static HKEY  currentKeyHandle = 0;
static BOOL  bTheKeyIsOpen    = FALSE;

static TCHAR *reg_class_names[] = {
    _T("HKEY_LOCAL_MACHINE"),
    _T("HKEY_USERS"),
    _T("HKEY_CLASSES_ROOT"),
    _T("HKEY_CURRENT_CONFIG"),
    _T("HKEY_CURRENT_USER")
};

#define REG_CLASS_NUMBER (sizeof(reg_class_names) / sizeof(reg_class_names[0]))

static HKEY reg_class_keys[REG_CLASS_NUMBER] = {
    HKEY_LOCAL_MACHINE, HKEY_USERS, HKEY_CLASSES_ROOT,
    HKEY_CURRENT_CONFIG, HKEY_CURRENT_USER
};

/* return values */
#define NOT_ENOUGH_MEMORY     1
#define IO_ERROR              2

/* processing macros */

/* common check of memory allocation results */
#ifdef UNICODE
#define CHECK_ENOUGH_MEMORY(p) \
    if (!(p)) \
    { \
        _tprintf(_T("file %S, line %d: Not enough memory"), __FILE__, __LINE__); \
        assert(0);\
        exit(NOT_ENOUGH_MEMORY); \
    }
#else
#define CHECK_ENOUGH_MEMORY(p) \
    if (!(p)) \
    { \
        _tprintf(_T("file %s, line %d: Not enough memory"), __FILE__, __LINE__); \
        assert(0);\
        exit(NOT_ENOUGH_MEMORY); \
    }
#endif

#ifdef UNICODE
#define _TEOF WEOF 
#else
#define _TEOF EOF 
#endif

/******************************************************************************
 * This is a replacement for strsep which is not portable (missing on Solaris).
 */
#if 0
/* DISABLED */
char* getToken(char** str, const char* delims)
{
    char* token;

    if (*str==NULL) {
        /* No more tokens */
        return NULL;
    }

    token=*str;
    while (**str!='\0') {
        if (strchr(delims,**str)!=NULL) {
            **str='\0';
            (*str)++;
            return token;
        }
        (*str)++;
    }
    /* There is no other token */
    *str=NULL;
    return token;
}
#endif

/******************************************************************************
 * Copies file name from command line string to the buffer.
 * Rewinds the command line string pointer to the next non-spece character
 * after the file name.
 * Buffer contains an empty string if no filename was found;
 *
 * params:
 * command_line - command line current position pointer
 *      where *s[0] is the first symbol of the file name.
 * file_name - buffer to write the file name to.
 */
void get_file_nameA(CHAR **command_line, CHAR *file_name, int max_filename)
{
    CHAR *s = *command_line;
    int pos = 0;                /* position of pointer "s" in *command_line */
    file_name[0] = 0;

    if (!s[0]) {
        return;
    }
    if (s[0] == '"') {
        s++;
        (*command_line)++;
        while (s[0] != '"') {
            if (!s[0]) {
                _tprintf(_T("Unexpected end of file name!\n"));
                assert(0);
                //exit(1);
            }
            s++;
            pos++;
        }
    } else {
        while (s[0] && !isspace(s[0])) {
            s++;
            pos++;
        }
    }
    memcpy(file_name, *command_line, pos * sizeof((*command_line)[0]));
    /* remove the last backslash */
    if (file_name[pos - 1] == '\\') {
        file_name[pos - 1] = '\0';
    } else {
        file_name[pos] = '\0';
    }
    if (s[0]) {
        s++;
        pos++;
    }
    while (s[0] && isspace(s[0])) {
        s++;
        pos++;
    }
    (*command_line) += pos;
}

void get_file_nameW(CHAR** command_line, WCHAR* filename, int max_filename)
{
    CHAR filenameA[_MAX_PATH];
    int len;

    get_file_nameA(command_line, filenameA, _MAX_PATH);
    len = strlen(filenameA);
    OemToCharBuffW(filenameA, filename, max_filename);
    filename[len] = _T('\0');
/*
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    CHAR filenameA[_MAX_PATH];

    get_file_nameA(command_line, filenameA, _MAX_PATH);

    //RtlInitAnsiString(&AnsiString, filenameA);
    UnicodeString.Buffer = filename;
    UnicodeString.MaximumLength = max_filename;//MAX_PATH;
    RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, FALSE);
 */
}

/******************************************************************************
 * Converts a hex representation of a DWORD into a DWORD.
 */
DWORD convertHexToDWord(TCHAR* str, BYTE* buf)
{
    DWORD dw;
    TCHAR xbuf[9];

    memcpy(xbuf, str, 8 * sizeof(TCHAR));
    xbuf[88 * sizeof(TCHAR)] = '\0';
    _stscanf(xbuf, _T("%08lx"), &dw);
    memcpy(buf, &dw, sizeof(DWORD));
    return sizeof(DWORD);
}

/******************************************************************************
 * Converts a hex buffer into a hex comma separated values
 */
TCHAR* convertHexToHexCSV(BYTE* buf, ULONG bufLen)
{
    TCHAR* str;
    TCHAR* ptrStr;
    BYTE* ptrBuf;

    ULONG current = 0;
    str = HeapAlloc(GetProcessHeap(), 0, (bufLen+1)*2*sizeof(TCHAR));
    memset(str, 0, (bufLen+1)*2);
    ptrStr = str;  /* Pointer to result  */
    ptrBuf = buf;  /* Pointer to current */
    while (current < bufLen) {
        BYTE bCur = ptrBuf[current++];
        TCHAR res[3];
        _stprintf(res, _T("%02x"), (unsigned int)*&bCur);
        _tcscat(str, res);
        _tcscat(str, _T(","));
    }
    /* Get rid of the last comma */
    str[_tcslen(str)-1] = _T('\0');
    return str;
}

/******************************************************************************
 * Converts a hex buffer into a DWORD string
 */
TCHAR* convertHexToDWORDStr(BYTE* buf, ULONG bufLen)
{
    TCHAR* str;
    DWORD dw;

    if (bufLen != sizeof(DWORD)) return NULL;
    str = HeapAlloc(GetProcessHeap(), 0, ((bufLen*2)+1)*sizeof(TCHAR));
    memcpy(&dw, buf, sizeof(DWORD));
    _stprintf(str, _T("%08lx"), dw);
    /* Get rid of the last comma */
    return str;
}

/******************************************************************************
 * Converts a hex comma separated values list into a hex list.
 * The Hex input string must be in exactly the correct form.
 */
DWORD convertHexCSVToHex(TCHAR* str, BYTE* buf, ULONG bufLen)
{
    TCHAR* s = str;  /* Pointer to current */
    CHAR* b = buf;  /* Pointer to result  */
    ULONG strLen    = _tcslen(str);
    ULONG strPos    = 0;
    DWORD byteCount = 0;

    memset(buf, 0, bufLen);
    /*
     * warn the user if we are here with a string longer than 2 bytes that does
     * not contains ",".  It is more likely because the data is invalid.
     */
    if ((strLen > 2) && (_tcschr(str, _T(',')) == NULL)) {
        _tprintf(_T("WARNING converting CSV hex stream with no comma, ") \
                 _T("input data seems invalid.\n"));
    }
    if (strLen > 3*bufLen) {
        _tprintf(_T("ERROR converting CSV hex stream.  Too long\n"));
    }
    while (strPos < strLen) {
        TCHAR xbuf[3];
        TCHAR wc;
        memcpy(xbuf, s, 2);
        xbuf[3] = _T('\0');
        _stscanf(xbuf, _T("%02x"), (UINT*)&wc);
        if (byteCount < bufLen)
            *b++ = (unsigned char)wc;
        s += 3;
        strPos += 3;
        ++byteCount;
    }
    return byteCount;
}

/******************************************************************************
 * This function returns the HKEY associated with the data type encoded in the
 * value.  It modifies the input parameter (key value) in order to skip this
 * "now useless" data type information.
 *
 * Note: Updated based on the algorithm used in 'server/registry.c'
 */
DWORD getDataType(LPTSTR* lpValue, DWORD* parse_type)
{
    struct data_type { const TCHAR *tag; int len; int type; int parse_type; };

    static const struct data_type data_types[] =
    {                   /* actual type */  /* type to assume for parsing */
        { _T("\""),        1,   REG_SZ,              REG_SZ },
        { _T("str:\""),    5,   REG_SZ,              REG_SZ },
//        { _T("str(2):\""), 8,   REG_EXPAND_SZ,       REG_SZ },
        { _T("expand:\""), 8,   REG_EXPAND_SZ,       REG_EXPAND_SZ },
        { _T("hex:"),      4,   REG_BINARY,          REG_BINARY },
        { _T("dword:"),    6,   REG_DWORD,           REG_DWORD },
        { _T("hex("),      4,   -1,                  REG_BINARY },
        { NULL,            0,    0,                  0 }
    };

    const struct data_type *ptr;
    int type;

    for (ptr = data_types; ptr->tag; ptr++) {
        if (memcmp(ptr->tag, *lpValue, ptr->len))
            continue;

        /* Found! */
        *parse_type = ptr->parse_type;
        type = ptr->type;
        *lpValue += ptr->len;
        if (type == -1) {
            TCHAR* end;
            /* "hex(xx):" is special */
            type = (int)_tcstoul(*lpValue , &end, 16);
            if (**lpValue == _T('\0') || *end != _T(')') || *(end+1) != _T(':')) {
                type = REG_NONE;
            } else {
                *lpValue = end + 2;
            }
        }
        return type;
    }
    return (**lpValue == _T('\0') ? REG_SZ : REG_NONE);
}

/******************************************************************************
 * Returns an allocated buffer with a cleaned copy (removed the surrounding
 * dbl quotes) of the passed value.
 */
LPTSTR getArg(LPTSTR arg)
{
  LPTSTR tmp = NULL;
  ULONG len;

  if (arg == NULL) return NULL;

  // Get rid of surrounding quotes
  len = _tcslen(arg);
  if (arg[len-1] == _T('\"')) arg[len-1] = _T('\0');
  if (arg[0]     == _T('\"')) arg++;
  tmp = HeapAlloc(GetProcessHeap(), 0, (_tcslen(arg)+1) * sizeof(TCHAR));
  _tcscpy(tmp, arg);
  return tmp;
}

/******************************************************************************
 * Replaces escape sequences with the characters.
 */
void REGPROC_unescape_string(LPTSTR str)
{
    int str_idx = 0;            /* current character under analysis */
    int val_idx = 0;            /* the last character of the unescaped string */
    int len = _tcslen(str);
    for (str_idx = 0; str_idx < len; str_idx++, val_idx++) {
        if (str[str_idx] == _T('\\')) {
            str_idx++;
            switch (str[str_idx]) {
            case _T('n'):
                str[val_idx] = _T('\n');
                break;
            case _T('\\'):
            case _T('"'):
                str[val_idx] = str[str_idx];
                break;
            default:
                _tprintf(_T("Warning! Unrecognized escape sequence: \\%c'\n"), str[str_idx]);
                str[val_idx] = str[str_idx];
                break;
            }
        } else {
            str[val_idx] = str[str_idx];
        }
    }
    str[val_idx] = _T('\0');
}

/******************************************************************************
 * Sets the value with name val_name to the data in val_data for the currently
 * opened key.
 *
 * Parameters:
 * val_name - name of the registry value
 * val_data - registry value data
 */
HRESULT setValue(LPTSTR val_name, LPTSTR val_data)
{
  HRESULT hRes;
  DWORD   dwDataType, dwParseType;
  LPBYTE lpbData;
  BYTE   convert[KEY_MAX_LEN];
  BYTE *bBigBuffer = 0;
  DWORD  dwLen;

  if ((val_name == NULL) || (val_data == NULL))
    return ERROR_INVALID_PARAMETER;

  /* Get the data type stored into the value field */
  dwDataType = getDataType(&val_data, &dwParseType);

//  if (dwParseType == REG_EXPAND_SZ) {
//  }
//  if (dwParseType == REG_SZ || dwParseType == REG_EXPAND_SZ) { /* no conversion for string */

  if (dwParseType == REG_SZ) {       /* no conversion for string */
    dwLen = _tcslen(val_data);
    if (dwLen > 0 && val_data[dwLen-1] == _T('"')) {
      dwLen--;
      val_data[dwLen] = _T('\0');
    }
    dwLen++;
    dwLen *= sizeof(TCHAR);
    REGPROC_unescape_string(val_data);
    lpbData = val_data;
  } else if (dwParseType == REG_DWORD) { /* Convert the dword types */
    dwLen   = convertHexToDWord(val_data, convert);
    lpbData = convert;
  } else {                               /* Convert the hexadecimal types */
    int b_len = _tcslen(val_data)+2/3;
    if (b_len > KEY_MAX_LEN) {
      bBigBuffer = HeapAlloc (GetProcessHeap(), 0, b_len * sizeof(TCHAR));
      if (bBigBuffer == NULL) {
          return ERROR_REGISTRY_IO_FAILED;
      }
      CHECK_ENOUGH_MEMORY(bBigBuffer);
      dwLen = convertHexCSVToHex(val_data, bBigBuffer, b_len);
      lpbData = bBigBuffer;
    } else {
      dwLen   = convertHexCSVToHex(val_data, convert, KEY_MAX_LEN);
      lpbData = convert;
    }
  }
  hRes = RegSetValueEx(currentKeyHandle, val_name,
          0, /* Reserved */dwDataType, lpbData, dwLen);

    _tprintf(_T("  Value: %s, Data: %s\n"), val_name, lpbData);


  if (bBigBuffer)
      HeapFree(GetProcessHeap(), 0, bBigBuffer);
  return hRes;
}


/******************************************************************************
 * Open the key
 */
HRESULT openKey(LPTSTR stdInput)
{
  DWORD   dwDisp;
  HRESULT hRes;

  /* Sanity checks */
  if (stdInput == NULL)
    return ERROR_INVALID_PARAMETER;

  /* Get the registry class */
  currentKeyClass = getRegClass(stdInput); /* Sets global variable */
  if (currentKeyClass == (HKEY)ERROR_INVALID_PARAMETER)
    return (HRESULT)ERROR_INVALID_PARAMETER;

  /* Get the key name */
  currentKeyName = getRegKeyName(stdInput); /* Sets global variable */
  if (currentKeyName == NULL)
    return ERROR_INVALID_PARAMETER;

  hRes = RegCreateKeyEx(
          currentKeyClass,          /* Class     */
          currentKeyName,           /* Sub Key   */
          0,                        /* MUST BE 0 */
          NULL,                     /* object type */
          REG_OPTION_NON_VOLATILE,  /* option, REG_OPTION_NON_VOLATILE ... */
          KEY_ALL_ACCESS,           /* access mask, KEY_ALL_ACCESS */
          NULL,                     /* security attribute */
          &currentKeyHandle,        /* result */
          &dwDisp);                 /* disposition, REG_CREATED_NEW_KEY or
                                                    REG_OPENED_EXISTING_KEY */

  if (hRes == ERROR_SUCCESS)
    bTheKeyIsOpen = TRUE;

  return hRes;

}

/******************************************************************************
 * Extracts from [HKEY\some\key\path] or HKEY\some\key\path types of line
 * the key name (what starts after the first '\')
 */
LPTSTR getRegKeyName(LPTSTR lpLine)
{
  LPTSTR keyNameBeg;
  TCHAR  lpLineCopy[KEY_MAX_LEN];

  if (lpLine == NULL)
    return NULL;

  _tcscpy(lpLineCopy, lpLine);
  keyNameBeg = _tcschr(lpLineCopy, _T('\\'));    /* The key name start by '\' */
  if (keyNameBeg) {
      LPTSTR keyNameEnd;

      keyNameBeg++;                             /* is not part of the name */
      keyNameEnd = _tcschr(lpLineCopy, _T(']'));
      if (keyNameEnd) {
          *keyNameEnd = _T('\0');               /* remove ']' from the key name */
      }
  } else {
      keyNameBeg = lpLineCopy + _tcslen(lpLineCopy); /* branch - empty string */
  }
  currentKeyName = HeapAlloc(GetProcessHeap(), 0, (_tcslen(keyNameBeg)+1)*sizeof(TCHAR));
  CHECK_ENOUGH_MEMORY(currentKeyName);
  _tcscpy(currentKeyName, keyNameBeg);
  return currentKeyName;
}

/******************************************************************************
 * Extracts from [HKEY\some\key\path] or HKEY\some\key\path types of line
 * the key class (what ends before the first '\')
 */
HKEY getRegClass(LPTSTR lpClass)
{
  LPTSTR classNameEnd;
  LPTSTR classNameBeg;
  int i;

  TCHAR lpClassCopy[KEY_MAX_LEN];

  if (lpClass == NULL)
    return (HKEY)ERROR_INVALID_PARAMETER;

  _tcsncpy(lpClassCopy, lpClass, KEY_MAX_LEN);

  classNameEnd  = _tcschr(lpClassCopy, _T('\\'));    /* The class name ends by '\' */
  if (!classNameEnd) {                          /* or the whole string */
      classNameEnd = lpClassCopy + _tcslen(lpClassCopy);
      if (classNameEnd[-1] == _T(']')) {
          classNameEnd--;
      }
  }
  *classNameEnd = _T('\0');                       /* Isolate the class name */
  if (lpClassCopy[0] == _T('[')) {
      classNameBeg = lpClassCopy + 1;
  } else {
      classNameBeg = lpClassCopy;
  }
  for (i = 0; i < REG_CLASS_NUMBER; i++) {
      if (!_tcscmp(classNameBeg, reg_class_names[i])) {
          return reg_class_keys[i];
      }
  }
  return (HKEY)ERROR_INVALID_PARAMETER;
}

/******************************************************************************
 * Close the currently opened key.
 */
void closeKey(VOID)
{
    RegCloseKey(currentKeyHandle);
    HeapFree(GetProcessHeap(), 0, currentKeyName); /* Allocated by getKeyName */
    bTheKeyIsOpen    = FALSE;
    currentKeyName   = NULL;
    currentKeyClass  = 0;
    currentKeyHandle = 0;
}

/******************************************************************************
 * This function is the main entry point to the setValue type of action.  It
 * receives the currently read line and dispatch the work depending on the
 * context.
 */
void doSetValue(LPTSTR stdInput)
{
  /*
   * We encountered the end of the file, make sure we
   * close the opened key and exit
   */
  if (stdInput == NULL) {
    if (bTheKeyIsOpen != FALSE)
      closeKey();
    return;
  }

  if (stdInput[0] == _T('[')) {     /* We are reading a new key */
      if (bTheKeyIsOpen != FALSE) {
          closeKey();                    /* Close the previous key before */
      }
      if (openKey(stdInput) != ERROR_SUCCESS) {
          _tprintf(_T("doSetValue failed to open key %s\n"), stdInput);
      }
  } else if ((bTheKeyIsOpen) &&
            ((stdInput[0] == _T('@')) || /* reading a default @=data pair */
             (stdInput[0] == _T('\"')))) { /* reading a new value=data pair */
    processSetValue(stdInput);
  } else {             /* since we are assuming that the file format is */
    if (bTheKeyIsOpen) /* valid we must be reading a blank line which  */
      closeKey();      /* indicate end of this key processing */
  }
}

/******************************************************************************
 * This funtion is the main entry point to the queryValue type of action.  It
 * receives the currently read line and dispatch the work depending on the
 * context.
 */
void doQueryValue(LPTSTR stdInput) {
  /*
   * We encoutered the end of the file, make sure we
   * close the opened key and exit
   */
  if (stdInput == NULL) {
    if (bTheKeyIsOpen != FALSE)
      closeKey();
    return;
  }

  if (stdInput[0] == _T('[')) {     /* We are reading a new key */
    if (bTheKeyIsOpen != FALSE)
      closeKey();                    /* Close the previous key before */
    if (openKey(stdInput) != ERROR_SUCCESS ) {
      _tprintf(_T("doQueryValue failed to open key %s\n"), stdInput);
    }
  }
  else if( (bTheKeyIsOpen) &&
           ((stdInput[0] == _T('@')) || /* reading a default @=data pair */
           (stdInput[0] == _T('\"')))) { /* reading a new value=data pair */
    processQueryValue(stdInput);
  } else {                /* since we are assuming that the file format is */
    if (bTheKeyIsOpen)    /* valid we must be reading a blank line which  */
      closeKey();         /* indicate end of this key processing */
  }
}

/******************************************************************************
 * This funtion is the main entry point to the deletetValue type of action.  It
 * receives the currently read line and dispatch the work depending on the
 * context.
 */
void doDeleteValue(LPTSTR line) {
  _tprintf(_T("deleteValue not yet implemented\n"));
}

/******************************************************************************
 * This funtion is the main entry point to the deleteKey type of action.  It
 * receives the currently read line and dispatch the work depending on the
 * context.
 */
void doDeleteKey(LPTSTR line)   {
  _tprintf(_T("deleteKey not yet implemented\n"));
}

/******************************************************************************
 * This funtion is the main entry point to the createKey type of action.  It
 * receives the currently read line and dispatch the work depending on the
 * context.
 */
void doCreateKey(LPTSTR line)   {
  _tprintf(_T("createKey not yet implemented\n"));
}

/******************************************************************************
 * This function is a wrapper for the setValue function.  It prepares the
 * land and clean the area once completed.
 * Note: this function modifies the line parameter.
 *
 * line - registry file unwrapped line. Should have the registry value name and
 *      complete registry value data.
 */
void processSetValue(LPTSTR line)
{
  LPTSTR val_name;                   /* registry value name   */
  LPTSTR val_data;                   /* registry value data   */

  int line_idx = 0;                 /* current character under analysis */
  HRESULT hRes = 0;

  /* get value name */
  if (line[line_idx] == _T('@') && line[line_idx + 1] == _T('=')) {
      line[line_idx] = _T('\0');
      val_name = line;
      line_idx++;
  } else if (line[line_idx] == _T('\"')) {
      line_idx++;
      val_name = line + line_idx;
      while (TRUE) {
          if (line[line_idx] == _T('\\')) {  /* skip escaped character */
              line_idx += 2;
          } else {
              if (line[line_idx] == _T('\"')) {
                  line[line_idx] = _T('\0');
                  line_idx++;
                  break;
              } else {
                  line_idx++;
              }
          }
      }
      if (line[line_idx] != _T('=')) {
          line[line_idx] = _T('\"');
          _tprintf(_T("Warning! uncrecognized line:\n%s\n"), line);
          return;
      }
  } else {
      _tprintf(_T("Warning! unrecognized line:\n%s\n"), line);
      return;
  }
  line_idx++;                   /* skip the '=' character */
  val_data = line + line_idx;
  REGPROC_unescape_string(val_name);

    _tprintf(_T("Key: %s, Value: %s, Data: %s\n"), currentKeyName, val_name, val_data);

  hRes = setValue(val_name, val_data);
  if (hRes != ERROR_SUCCESS) {
    _tprintf(_T("ERROR Key %s not created. Value: %s, Data: %s\n"), currentKeyName, val_name, val_data);
  }
}

/******************************************************************************
 * This function is a wrapper for the queryValue function.  It prepares the
 * land and clean the area once completed.
 */
void processQueryValue(LPTSTR cmdline)
{
  _tprintf(_T("ERROR!!! - temporary disabled"));
  //exit(1);
  return;
#if 0
  LPSTR   argv[QUERY_VALUE_MAX_ARGS];/* args storage    */
  LPSTR   token      = NULL;         /* current token analized */
  ULONG   argCounter = 0;            /* counter of args */
  INT     counter;
  HRESULT hRes       = 0;
  LPSTR   keyValue   = NULL;
  LPSTR   lpsRes     = NULL;

  /*
   * Init storage and parse the line
   */
  for (counter = 0; counter < QUERY_VALUE_MAX_ARGS; counter++)
    argv[counter] = NULL;

  while ((token = getToken(&cmdline, queryValueDelim[argCounter])) != NULL) {
    argv[argCounter++] = getArg(token);
    if (argCounter == QUERY_VALUE_MAX_ARGS)
      break;  /* Stop processing args no matter what */
  }

  /* The value we look for is the first token on the line */
  if (argv[0] == NULL)
    return; /* SHOULD NOT HAPPEN */
  else
    keyValue = argv[0];

  if ((keyValue[0] == '@') && (_tcslen(keyValue) == 1)) {
    LONG lLen = KEY_MAX_LEN;
    TCHAR* lpsData = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,KEY_MAX_LEN);
    /*
     * We need to query the key default value
     */
    hRes = RegQueryValue(currentKeyHandle, currentKeyName, (LPBYTE)lpsData, &lLen);
    if (hRes == ERROR_MORE_DATA) {
        lpsData = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpsData, lLen);
        hRes = RegQueryValue(currentKeyHandle, currentKeyName, (LPBYTE)lpsData, &lLen);
    }
    if (hRes == ERROR_SUCCESS) {
      lpsRes = HeapAlloc(GetProcessHeap(), 0, lLen);
      strncpy(lpsRes, lpsData, lLen);
      lpsRes[lLen-1]='\0';
    }
  } else {
    DWORD dwLen = KEY_MAX_LEN;
    BYTE* lpbData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, KEY_MAX_LEN);
    DWORD dwType;
    /*
     * We need to query a specific value for the key
     */
    hRes = RegQueryValueEx(
             currentKeyHandle,
             keyValue,
             0,
             &dwType,
             (LPBYTE)lpbData,
             &dwLen);

    if (hRes == ERROR_MORE_DATA) {
        lpbData = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpbData, dwLen * sizeof(TCHAR));
        hRes = RegQueryValueEx(currentKeyHandle, keyValue, NULL, &dwType, (LPBYTE)lpbData, &dwLen);
    }

    if (hRes == ERROR_SUCCESS) {
      /*
       * Convert the returned data to a displayable format
       */
      switch (dwType) {
        case REG_SZ:
        case REG_EXPAND_SZ:
          lpsRes = HeapAlloc(GetProcessHeap(), 0, dwLen * sizeof(TCHAR));
          strncpy(lpsRes, lpbData, dwLen);
          lpsRes[dwLen-1] = '\0';
          break;
        case REG_DWORD:
          lpsRes = convertHexToDWORDStr(lpbData, dwLen);
          break;
        default:
          lpsRes = convertHexToHexCSV(lpbData, dwLen);
          break;
      }
    }

    HeapFree(GetProcessHeap(), 0, lpbData);
  }
  if (hRes == ERROR_SUCCESS) {
    _tprintf(_T("Value \"%s\" = \"%s\" in key [%s]\n"), keyValue, lpsRes, currentKeyName);

  } else {
    _tprintf(_T("ERROR Value \"%s\" not found. for key \"%s\"\n"), keyValue, currentKeyName);
  }

  /*
   * Do some cleanup
   */
  for (counter=0; counter<argCounter; counter++)
    if (argv[counter] != NULL)
      HeapFree(GetProcessHeap(), 0, argv[counter]);

  if (lpsRes != NULL)
    HeapFree(GetProcessHeap(), 0, lpsRes);
#endif
}

/******************************************************************************
 * Calls command for each line of a registry file.
 * Correctly processes comments (in # form), line continuation.
 *
 * Parameters:
 *   in - input stream to read from
 *   command - command to be called for each line
 */
void processRegLines(FILE *in, CommandAPI command)
{
    LPTSTR line     = NULL;  /* line read from input stream */
    ULONG lineSize = REG_VAL_BUF_SIZE;

    line = HeapAlloc(GetProcessHeap(), 0, lineSize * sizeof(TCHAR));
    CHECK_ENOUGH_MEMORY(line);

    while (!feof(in)) {
        LPTSTR s; /* The pointer into line for where the current fgets should read */
        s = line;
        for (;;) {
            size_t size_remaining;
            int size_to_get;
            TCHAR *s_eol; /* various local uses */

            /* Do we need to expand the buffer ? */
            assert (s >= line && s <= line + lineSize);
            size_remaining = lineSize - (s-line);
            if (size_remaining < 2) { /* room for 1 character and the \0 */
                TCHAR *new_buffer;
                size_t new_size = lineSize + REG_VAL_BUF_SIZE;
                if (new_size > lineSize) /* no arithmetic overflow */
                    new_buffer = HeapReAlloc (GetProcessHeap(), 0, line, new_size * sizeof(TCHAR));
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
            if (NULL == _fgetts(s, size_to_get, in)) {
                if (ferror(in)) {
                    //_tperror(_T("While reading input"));
                    perror ("While reading input");
                    //exit(IO_ERROR);
                    return;
                } else {
                    assert (feof(in));
                    *s = _T('\0');
                    /* It is not clear to me from the definition that the
                     * contents of the buffer are well defined on detecting
                     * an eof without managing to read anything.
                     */
                }
            }

            /* If we didn't read the eol nor the eof go around for the rest */
            s_eol = _tcschr (s, _T('\n'));
            if (!feof (in) && !s_eol) {
                s = _tcschr (s, _T('\0'));
                /* It should be s + size_to_get - 1 but this is safer */
                continue;
            }

            /* If it is a comment line then discard it and go around again */
            if (line [0] == _T('#')) {
                s = line;
                continue;
            }

            /* Remove any line feed.  Leave s_eol on the \0 */
            if (s_eol) {
                *s_eol = _T('\0');
                if (s_eol > line && *(s_eol-1) == _T('\r'))
                    *--s_eol = _T('\0');
            } else {
                s_eol = _tcschr (s, _T('\0'));
            }
            /* If there is a concatenating \\ then go around again */
            if (s_eol > line && *(s_eol-1) == _T('\\')) {
                int c;
                s = s_eol-1;
                /* The following error protection could be made more self-
                 * correcting but I thought it not worth trying.
                 */

                if ((c = _fgettc(in)) == _TEOF || c != _T(' ') ||
                    (c = _fgettc(in)) == _TEOF || c != _T(' '))
                    _tprintf(_T("ERROR - invalid continuation.\n"));
                continue;
            }
            break; /* That is the full virtual line */
        }
        command(line);
    }
    command(NULL);
    HeapFree(GetProcessHeap(), 0, line);
}

/******************************************************************************
 * This funtion is the main entry point to the registerDLL action.  It
 * receives the currently read line, then loads and registers the requested DLLs
 */
void doRegisterDLL(LPTSTR stdInput)
{
    HMODULE theLib = 0;
    UINT retVal = 0;

    /* Check for valid input */
    if (stdInput == NULL) return;

    /* Load and register the library, then free it */
    theLib = LoadLibrary(stdInput);
    if (theLib) {
        FARPROC lpfnDLLRegProc = GetProcAddress(theLib, "DllRegisterServer");
        if (lpfnDLLRegProc) {
            retVal = (*lpfnDLLRegProc)();
        } else {
            _tprintf(_T("Couldn't find DllRegisterServer proc in '%s'.\n"), stdInput);
        }
        if (retVal != S_OK) {
            _tprintf(_T("Couldn't find DllRegisterServer proc in '%s'.\n"), stdInput);
        }
        FreeLibrary(theLib);
    } else {
        _tprintf(_T("Could not load DLL '%s'.\n"), stdInput);
    }
}

/******************************************************************************
 * This funtion is the main entry point to the unregisterDLL action.  It
 * receives the currently read line, then loads and unregisters the requested DLLs
 */
void doUnregisterDLL(LPTSTR stdInput)
{
    HMODULE theLib = 0;
    UINT retVal = 0;

    /* Check for valid input */
    if (stdInput == NULL) return;

    /* Load and unregister the library, then free it */
    theLib = LoadLibrary(stdInput);
    if (theLib) {
        FARPROC lpfnDLLRegProc = GetProcAddress(theLib, "DllUnregisterServer");
        if (lpfnDLLRegProc) {
            retVal = (*lpfnDLLRegProc)();
        } else {
            _tprintf(_T("Couldn't find DllUnregisterServer proc in '%s'.\n"), stdInput);
        }
        if (retVal != S_OK) {
            _tprintf(_T("DLLUnregisterServer error 0x%x in '%s'.\n"), retVal, stdInput);
        }
        FreeLibrary(theLib);
    } else {
        _tprintf(_T("Could not load DLL '%s'.\n"), stdInput);
    }
}

/****************************************************************************
 * REGPROC_print_error
 *
 * Print the message for GetLastError
 */

void REGPROC_print_error(VOID)
{
    LPVOID lpMsgBuf;
    DWORD error_code;
    int status;

    error_code = GetLastError ();
    status = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL, error_code, 0, (LPTSTR) &lpMsgBuf, 0, NULL);
    if (!status) {
        _tprintf(_T("Cannot display message for error %ld, status %ld\n"), error_code, GetLastError());
    } else {
        _tprintf(_T("REGPROC_print_error() - "));
        puts(lpMsgBuf);
        LocalFree((HLOCAL)lpMsgBuf);
    }
    //exit(1);
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
void REGPROC_resize_char_buffer(TCHAR **buffer, DWORD *len, DWORD required_len)
{
    required_len++;
    if (required_len > *len) {
        *len = required_len;
        *buffer = HeapReAlloc(GetProcessHeap(), 0, *buffer, *len * sizeof(**buffer));
        CHECK_ENOUGH_MEMORY(*buffer);
    }
}

/******************************************************************************
 * Prints string str to file
 */
void REGPROC_export_string(FILE *file, TCHAR *str)
{
    size_t len = _tcslen(str);
    size_t i;

    /* escaping characters */
    for (i = 0; i < len; i++) {
        TCHAR c = str[i];
        switch (c) {
        //case _T('\\'): _fputts(_T("\\\\"), file); break;
        case _T('\"'): _fputts(_T("\\\""), file); break;
        case _T('\n'): _fputts(_T("\\\n"), file); break;
        default:       _fputtc(c, file);          break;
        }
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
 * reg_key_name_len - length of the buffer for the registry class in characters.
 * val_name_buf - buffer for storing value name.
 *      Is resized if necessary.
 * val_name_len - length of the buffer for storing value names in characters.
 * val_buf - buffer for storing values while extracting.
 *      Is resized if necessary.
 * val_size - size of the buffer for storing values in bytes.
 */
void export_hkey(FILE *file, HKEY key,
                 TCHAR **reg_key_name_buf, DWORD *reg_key_name_len,
                 TCHAR **val_name_buf, DWORD *val_name_len,
                 BYTE **val_buf, DWORD *val_size)
{
    DWORD max_sub_key_len;
    DWORD max_val_name_len;
    DWORD max_val_size;
    DWORD curr_len;
    DWORD i;
    BOOL more_data;
    LONG ret;

    /* get size information and resize the buffers if necessary */
    if (RegQueryInfoKey(key, NULL, NULL, NULL, NULL, &max_sub_key_len, NULL,
                        NULL, &max_val_name_len, &max_val_size, NULL, NULL) != ERROR_SUCCESS) {
        REGPROC_print_error();
    }
    curr_len = _tcslen(*reg_key_name_buf);
    REGPROC_resize_char_buffer(reg_key_name_buf, reg_key_name_len, max_sub_key_len + curr_len + 1);
    REGPROC_resize_char_buffer(val_name_buf, val_name_len, max_val_name_len);
    if (max_val_size > *val_size) {
        *val_size = max_val_size;
        *val_buf = HeapReAlloc(GetProcessHeap(), 0, *val_buf, *val_size * sizeof(TCHAR));
        CHECK_ENOUGH_MEMORY(val_buf);
    }
    /* output data for the current key */
    _fputts(_T("\n["), file);
    _fputts(*reg_key_name_buf, file);
    _fputts(_T("]\n"), file);
    /* print all the values */
    i = 0;
    more_data = TRUE;
    while (more_data) {
        DWORD value_type;
        DWORD val_name_len1 = *val_name_len;
        DWORD val_size1 = *val_size;
        ret = RegEnumValue(key, i, *val_name_buf, &val_name_len1, NULL, &value_type, *val_buf, &val_size1);
        if (ret != ERROR_SUCCESS) {
            more_data = FALSE;
            if (ret != ERROR_NO_MORE_ITEMS) {
                REGPROC_print_error();
            }
        } else {
            i++;
            if ((*val_name_buf)[0]) {
                _fputts(_T("\""), file);
                REGPROC_export_string(file, *val_name_buf);
                _fputts(_T("\"="), file);
            } else {
                _fputts(_T("@="), file);
            }
            switch (value_type) {
            case REG_EXPAND_SZ:
                _fputts(_T("expand:"), file);
            case REG_SZ:
                _fputts(_T("\""), file);
                REGPROC_export_string(file, *val_buf);
                _fputts(_T("\"\n"), file);
                break;
            case REG_DWORD:
                _ftprintf(file, _T("dword:%08lx\n"), *((DWORD *)*val_buf));
                break;
            default:
/*
                _tprintf(_T("warning - unsupported registry format '%ld', ") \
                         _T("treating as binary\n"), value_type);
                _tprintf(_T("key name: \"%s\"\n"), *reg_key_name_buf);
                _tprintf(_T("value name:\"%s\"\n\n"), *val_name_buf);
 */
                /* falls through */
            case REG_MULTI_SZ:
                /* falls through */
            case REG_BINARY:
            {
                DWORD i1;
                TCHAR *hex_prefix;
                TCHAR buf[20];
                int cur_pos;

                if (value_type == REG_BINARY) {
                    hex_prefix = _T("hex:");
                } else {
                    hex_prefix = buf;
                    _stprintf(buf, _T("hex(%ld):"), value_type);
                }
                /* position of where the next character will be printed */
                /* NOTE: yes, _tcslen("hex:") is used even for hex(x): */
                cur_pos = _tcslen(_T("\"\"=")) + _tcslen(_T("hex:")) +
                    _tcslen(*val_name_buf);
                _fputts(hex_prefix, file);
                for (i1 = 0; i1 < val_size1; i1++) {
                    _ftprintf(file, _T("%02x"), (unsigned int)(*val_buf)[i1]);
                    if (i1 + 1 < val_size1) {
                        _fputts(_T(","), file);
                    }
                    cur_pos += 3;
                    /* wrap the line */
                    if (cur_pos > REG_FILE_HEX_LINE_LEN) {
                        _fputts(_T("\\\n  "), file);
                        cur_pos = 2;
                    }
                }
                _fputts(_T("\n"), file);
                break;
            }
            }
        }
    }
    i = 0;
    more_data = TRUE;
    (*reg_key_name_buf)[curr_len] = _T('\\');
    while (more_data) {
        DWORD buf_len = *reg_key_name_len - curr_len;
        ret = RegEnumKeyEx(key, i, *reg_key_name_buf + curr_len + 1, &buf_len, NULL, NULL, NULL, NULL);
        if (ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA) {
            more_data = FALSE;
            if (ret != ERROR_NO_MORE_ITEMS) {
                REGPROC_print_error();
            }
        } else {
            HKEY subkey;

            i++;
            if (RegOpenKey(key, *reg_key_name_buf + curr_len + 1, &subkey) == ERROR_SUCCESS) {
                export_hkey(file, subkey, reg_key_name_buf, reg_key_name_len, val_name_buf, val_name_len, val_buf, val_size);
                RegCloseKey(subkey);
            } else {
                REGPROC_print_error();
            }
        }
    }
    (*reg_key_name_buf)[curr_len] = _T('\0');
}
/*
#define REG_NONE                    ( 0 )   // No value type
#define REG_SZ                      ( 1 )   // Unicode nul terminated string
#define REG_EXPAND_SZ               ( 2 )   // Unicode nul terminated string
                                            // (with environment variable references)
#define REG_BINARY                  ( 3 )   // Free form binary
#define REG_DWORD                   ( 4 )   // 32-bit number
#define REG_DWORD_LITTLE_ENDIAN     ( 4 )   // 32-bit number (same as REG_DWORD)
#define REG_DWORD_BIG_ENDIAN        ( 5 )   // 32-bit number
#define REG_LINK                    ( 6 )   // Symbolic Link (unicode)
#define REG_MULTI_SZ                ( 7 )   // Multiple Unicode strings
#define REG_RESOURCE_LIST           ( 8 )   // Resource list in the resource map
#define REG_FULL_RESOURCE_DESCRIPTOR ( 9 )  // Resource list in the hardware description
#define REG_RESOURCE_REQUIREMENTS_LIST ( 10 )

 */
/******************************************************************************
 * Open file for export.
 */
FILE *REGPROC_open_export_file(TCHAR *file_name)
{
//_CRTIMP FILE * __cdecl _wfopen(const wchar_t *, const wchar_t *);

//FILE*	fopen (const char* szFileName, const char* szMode);
//FILE*	_wfopen(const wchar_t *file, const wchar_t *mode);

    FILE *file = _tfopen(file_name, _T("w"));
    if (!file) {
        perror("");
        _tprintf(_T("REGPROC_open_export_file(%s) - Can't open file.\n"), file_name);
        //exit(1);
        return NULL;
    }
    _fputts(_T("REGEDIT4\n"), file);
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
BOOL export_registry_key(TCHAR* file_name, TCHAR* reg_key_name)
{
    HKEY reg_key_class;

    TCHAR *reg_key_name_buf;
    TCHAR *val_name_buf;
    BYTE *val_buf;
    DWORD reg_key_name_len = KEY_MAX_LEN;
    DWORD val_name_len = KEY_MAX_LEN;
    DWORD val_size = REG_VAL_BUF_SIZE;
    FILE *file = NULL;

    //_tprintf(_T("export_registry_key(%s, %s)\n"), reg_key_name, file_name);

    reg_key_name_buf = HeapAlloc(GetProcessHeap(), 0, reg_key_name_len * sizeof(*reg_key_name_buf));
    val_name_buf = HeapAlloc(GetProcessHeap(), 0, val_name_len * sizeof(*val_name_buf));
    val_buf = HeapAlloc(GetProcessHeap(), 0, val_size);
    CHECK_ENOUGH_MEMORY(reg_key_name_buf && val_name_buf && val_buf);

    if (reg_key_name && reg_key_name[0]) {
        TCHAR *branch_name;
        HKEY key;

        REGPROC_resize_char_buffer(&reg_key_name_buf, &reg_key_name_len,
                                   _tcslen(reg_key_name));
        _tcscpy(reg_key_name_buf, reg_key_name);

        /* open the specified key */
        reg_key_class = getRegClass(reg_key_name);
        if (reg_key_class == (HKEY)ERROR_INVALID_PARAMETER) {
            _tprintf(_T("Incorrect registry class specification in '%s\n"), reg_key_name);
            //exit(1);
            return FALSE;
        }
        branch_name = getRegKeyName(reg_key_name);
        CHECK_ENOUGH_MEMORY(branch_name);
        if (!branch_name[0]) {
            /* no branch - registry class is specified */
            file = REGPROC_open_export_file(file_name);
            export_hkey(file, reg_key_class,
                        &reg_key_name_buf, &reg_key_name_len,
                        &val_name_buf, &val_name_len,
                        &val_buf, &val_size);
        } else if (RegOpenKey(reg_key_class, branch_name, &key) == ERROR_SUCCESS) {
            file = REGPROC_open_export_file(file_name);
            export_hkey(file, key,
                        &reg_key_name_buf, &reg_key_name_len,
                        &val_name_buf, &val_name_len,
                        &val_buf, &val_size);
            RegCloseKey(key);
        } else {
            _tprintf(_T("Can't export. Registry key '%s does not exist!\n"), reg_key_name);
            REGPROC_print_error();
        }
        HeapFree(GetProcessHeap(), 0, branch_name);
    } else {
        int i;

        /* export all registry classes */
        file = REGPROC_open_export_file(file_name);
        for (i = 0; i < REG_CLASS_NUMBER; i++) {
            /* do not export HKEY_CLASSES_ROOT */
            if (reg_class_keys[i] != HKEY_CLASSES_ROOT &&
                reg_class_keys[i] != HKEY_CURRENT_USER &&
                reg_class_keys[i] != HKEY_CURRENT_CONFIG) {
                _tcscpy(reg_key_name_buf, reg_class_names[i]);
                export_hkey(file, reg_class_keys[i],
                            &reg_key_name_buf, &reg_key_name_len,
                            &val_name_buf, &val_name_len,
                            &val_buf, &val_size);
            }
        }
    }
    if (file) {
        fclose(file);
    }
//    HeapFree(GetProcessHeap(), 0, reg_key_name);
    HeapFree(GetProcessHeap(), 0, val_buf);
    HeapFree(GetProcessHeap(), 0, val_name_buf);
    HeapFree(GetProcessHeap(), 0, reg_key_name_buf);
    return TRUE;
}

/******************************************************************************
 * Reads contents of the specified file into the registry.
 */
BOOL import_registry_file(LPTSTR filename)
{
    FILE* reg_file = _tfopen(filename, _T("r"));

    if (reg_file) {
        processRegLines(reg_file, doSetValue);
        return TRUE;
    }
    return FALSE;
}

/******************************************************************************
 * Recursive function which removes the registry key with all subkeys.
 */
BOOL delete_branch(HKEY key, TCHAR** reg_key_name_buf, DWORD* reg_key_name_len)
{
    HKEY branch_key;
    DWORD max_sub_key_len;
    DWORD subkeys;
    DWORD curr_len;
    LONG ret;
    long int i;

    if (RegOpenKey(key, *reg_key_name_buf, &branch_key) != ERROR_SUCCESS) {
        REGPROC_print_error();
        return FALSE;
    }

    /* get size information and resize the buffers if necessary */
    if (RegQueryInfoKey(branch_key, NULL, NULL, NULL, &subkeys, &max_sub_key_len,
                        NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
        REGPROC_print_error();
        RegCloseKey(branch_key);
        return FALSE;
    }
    curr_len = _tcslen(*reg_key_name_buf);
    REGPROC_resize_char_buffer(reg_key_name_buf, reg_key_name_len, max_sub_key_len + curr_len + 1);

    (*reg_key_name_buf)[curr_len] = '\\';
    for (i = subkeys - 1; i >= 0; i--) {
        DWORD buf_len = *reg_key_name_len - curr_len;
        ret = RegEnumKeyEx(branch_key, i, *reg_key_name_buf + curr_len + 1, &buf_len, NULL, NULL, NULL, NULL);
        if (ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA && ret != ERROR_NO_MORE_ITEMS) {
            REGPROC_print_error();
            RegCloseKey(branch_key);
            return FALSE;
        } else {
            delete_branch(key, reg_key_name_buf, reg_key_name_len);
        }
    }
    (*reg_key_name_buf)[curr_len] = '\0';
    RegCloseKey(branch_key);
    RegDeleteKey(key, *reg_key_name_buf);
    return TRUE;
}

/******************************************************************************
 * Removes the registry key with all subkeys. Parses full key name.
 *
 * Parameters:
 * reg_key_name - full name of registry branch to delete. Ignored if is NULL,
 *      empty, points to register key class, does not exist.
 */
void delete_registry_key(TCHAR* reg_key_name)
{
    TCHAR* branch_name;
    DWORD branch_name_len;
    HKEY reg_key_class;
    HKEY branch_key;

    if (!reg_key_name || !reg_key_name[0]) {
        return;
    }
    /* open the specified key */
    reg_key_class = getRegClass(reg_key_name);
    if (reg_key_class == (HKEY)ERROR_INVALID_PARAMETER) {
        _tprintf(_T("Incorrect registry class specification in '%s'\n"), reg_key_name);
        //exit(1);
        return;
    }
    branch_name = getRegKeyName(reg_key_name);
    CHECK_ENOUGH_MEMORY(branch_name);
    branch_name_len = _tcslen(branch_name);
    if (!branch_name[0]) {
        _tprintf(_T("Can't delete registry class '%s'\n"), reg_key_name);
        //exit(1);
        return;
    }
    if (RegOpenKey(reg_key_class, branch_name, &branch_key) == ERROR_SUCCESS) {
        /* check whether the key exists */
        RegCloseKey(branch_key);
        delete_branch(reg_key_class, &branch_name, &branch_name_len);
    }
    HeapFree(GetProcessHeap(), 0, branch_name);
}

