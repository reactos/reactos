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

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
    
#include <ctype.h>
#include <limits.h>
#include <winnt.h>
#include <winreg.h>
#include <assert.h>

#include "regproc.h"


#define REG_VAL_BUF_SIZE        4096

/* Delimiters used to parse the "value" to query queryValue*/
#define QUERY_VALUE_MAX_ARGS  1

/* maximal number of characters in hexadecimal data line,
   not including '\' character */
#define REG_FILE_HEX_LINE_LEN   76

/* Globals used by the api setValue, queryValue */
static LPSTR currentKeyName   = NULL;
static HKEY  currentKeyClass  = 0;
static HKEY  currentKeyHandle = 0;
static BOOL  bTheKeyIsOpen    = FALSE;

static CHAR *app_name = "UNKNOWN";

static CHAR *reg_class_names[] = {
    "HKEY_LOCAL_MACHINE", "HKEY_USERS", "HKEY_CLASSES_ROOT",
    "HKEY_CURRENT_CONFIG", "HKEY_CURRENT_USER"
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
#define CHECK_ENOUGH_MEMORY(p) \
    if (!(p)) \
    { \
        printf("%s: file %s, line %d: Not enough memory", \
               getAppName(), __FILE__, __LINE__); \
        exit(NOT_ENOUGH_MEMORY); \
    }

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
void get_file_name(CHAR **command_line, CHAR *file_name)
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
                printf("%s: Unexpected end of file name!\n", getAppName());
                exit(1);
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


/******************************************************************************
 * Converts a hex representation of a DWORD into a DWORD.
 */
DWORD convertHexToDWord(char* str, BYTE* buf)
{
    DWORD dw;
    char xbuf[9];

    memcpy(xbuf, str, 8);
    xbuf[8] = '\0';
    sscanf(xbuf, "%08lx", &dw);
    memcpy(buf, &dw, sizeof(DWORD));
    return sizeof(DWORD);
}

/******************************************************************************
 * Converts a hex buffer into a hex comma separated values
 */
char* convertHexToHexCSV(BYTE* buf, ULONG bufLen)
{
    char* str;
    char* ptrStr;
    BYTE* ptrBuf;

    ULONG current = 0;
    str = HeapAlloc(GetProcessHeap(), 0, (bufLen+1)*2);
    memset(str, 0, (bufLen+1)*2);
    ptrStr = str;  /* Pointer to result  */
    ptrBuf = buf;  /* Pointer to current */
    while (current < bufLen) {
        BYTE bCur = ptrBuf[current++];
        char res[3];
        sprintf(res, "%02x", (unsigned int)*&bCur);
        strcat(str, res);
        strcat(str, ",");
    }
    /* Get rid of the last comma */
    str[strlen(str)-1] = '\0';
    return str;
}

/******************************************************************************
 * Converts a hex buffer into a DWORD string
 */
char* convertHexToDWORDStr(BYTE* buf, ULONG bufLen)
{
    char* str;
    DWORD dw;

    if (bufLen != sizeof(DWORD)) return NULL;
    str = HeapAlloc(GetProcessHeap(), 0, (bufLen*2)+1);
    memcpy(&dw, buf, sizeof(DWORD));
    sprintf(str, "%08lx", dw);
    /* Get rid of the last comma */
    return str;
}

/******************************************************************************
 * Converts a hex comma separated values list into a hex list.
 * The Hex input string must be in exactly the correct form.
 */
DWORD convertHexCSVToHex(char* str, BYTE* buf, ULONG bufLen)
{
    char* s = str;  /* Pointer to current */
    char* b = buf;  /* Pointer to result  */
    ULONG strLen    = strlen(str);
    ULONG strPos    = 0;
    DWORD byteCount = 0;

    memset(buf, 0, bufLen);
    /*
     * warn the user if we are here with a string longer than 2 bytes that does
     * not contains ",".  It is more likely because the data is invalid.
     */
    if ((strLen > 2) && (strchr(str, ',') == NULL))
        printf("%s: WARNING converting CSV hex stream with no comma, "
               "input data seems invalid.\n", getAppName());
    if (strLen > 3*bufLen)
        printf ("%s: ERROR converting CSV hex stream.  Too long\n", getAppName());
    while (strPos < strLen) {
        char xbuf[3];
        char wc;
        memcpy(xbuf, s, 2);
        xbuf[3] = '\0';
        sscanf(xbuf, "%02x", (UINT*)&wc);
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
DWORD getDataType(LPSTR* lpValue, DWORD* parse_type)
{
    struct data_type { const char *tag; int len; int type; int parse_type; };

    static const struct data_type data_types[] =
    {                   /* actual type */  /* type to assume for parsing */
        { "\"",        1,   REG_SZ,              REG_SZ },
        { "str:\"",    5,   REG_SZ,              REG_SZ },
        { "str(2):\"", 8,   REG_EXPAND_SZ,       REG_SZ },
        { "hex:",      4,   REG_BINARY,          REG_BINARY },
        { "dword:",    6,   REG_DWORD,           REG_DWORD },
        { "hex(",      4,   -1,                  REG_BINARY },
        { NULL,        0,    0,                  0 }
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
            char* end;
            /* "hex(xx):" is special */
            type = (int)strtoul(*lpValue , &end, 16);
            if (**lpValue=='\0' || *end!=')' || *(end+1)!=':') {
                type = REG_NONE;
            } else {
                *lpValue = end + 2;
            }
        }
        return type;
    }
    return (**lpValue == '\0'?REG_SZ:REG_NONE);
}

/******************************************************************************
 * Returns an allocated buffer with a cleaned copy (removed the surrounding
 * dbl quotes) of the passed value.
 */
LPSTR getArg( LPSTR arg)
{
  LPSTR tmp = NULL;
  ULONG len;

  if (arg == NULL) return NULL;

  // Get rid of surrounding quotes
  len = strlen(arg);
  if (arg[len-1] == '\"') arg[len-1] = '\0';
  if (arg[0]     == '\"') arg++;
  tmp = HeapAlloc(GetProcessHeap(), 0, strlen(arg)+1);
  strcpy(tmp, arg);
  return tmp;
}

/******************************************************************************
 * Replaces escape sequences with the characters.
 */
void REGPROC_unescape_string(LPSTR str)
{
    int str_idx = 0;            /* current character under analysis */
    int val_idx = 0;            /* the last character of the unescaped string */
    int len = strlen(str);
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
                printf("Warning! Unrecognized escape sequence: \\%c'\n", str[str_idx]);
                str[val_idx] = str[str_idx];
                break;
            }
        } else {
            str[val_idx] = str[str_idx];
        }
    }
    str[val_idx] = '\0';
}

/******************************************************************************
 * Sets the value with name val_name to the data in val_data for the currently
 * opened key.
 *
 * Parameters:
 * val_name - name of the registry value
 * val_data - registry value data
 */
HRESULT setValue(LPSTR val_name, LPSTR val_data)
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

  if (dwParseType == REG_SZ) {       /* no conversion for string */
    dwLen = strlen(val_data);
    if (dwLen>0 && val_data[dwLen-1]=='"') {
      dwLen--;
      val_data[dwLen]='\0';
    }
    dwLen++;
    REGPROC_unescape_string(val_data);
    lpbData = val_data;
  } else if (dwParseType == REG_DWORD) { /* Convert the dword types */
    dwLen   = convertHexToDWord(val_data, convert);
    lpbData = convert;
  } else {                               /* Convert the hexadecimal types */
    int b_len = strlen (val_data)+2/3;
    if (b_len > KEY_MAX_LEN) {
      bBigBuffer = HeapAlloc (GetProcessHeap(), 0, b_len);
      CHECK_ENOUGH_MEMORY(bBigBuffer);
      dwLen = convertHexCSVToHex(val_data, bBigBuffer, b_len);
      lpbData = bBigBuffer;
    } else {
      dwLen   = convertHexCSVToHex(val_data, convert, KEY_MAX_LEN);
      lpbData = convert;
    }
  }
  hRes = RegSetValueExA(currentKeyHandle, val_name,
          0, /* Reserved */dwDataType, lpbData, dwLen);

  if (bBigBuffer)
      HeapFree(GetProcessHeap(), 0, bBigBuffer);
  return hRes;
}


/******************************************************************************
 * Open the key
 */
HRESULT openKey( LPSTR stdInput)
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

  hRes = RegCreateKeyExA(
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
LPSTR getRegKeyName(LPSTR lpLine)
{
  LPSTR keyNameBeg;
  char  lpLineCopy[KEY_MAX_LEN];

  if (lpLine == NULL)
    return NULL;

  strcpy(lpLineCopy, lpLine);
  keyNameBeg = strchr(lpLineCopy, '\\');    /* The key name start by '\' */
  if (keyNameBeg) {
      LPSTR keyNameEnd;

      keyNameBeg++;                             /* is not part of the name */
      keyNameEnd = strchr(lpLineCopy, ']');
      if (keyNameEnd) {
          *keyNameEnd = '\0';               /* remove ']' from the key name */
      }
  } else {
      keyNameBeg = lpLineCopy + strlen(lpLineCopy); /* branch - empty string */
  }
  currentKeyName = HeapAlloc(GetProcessHeap(), 0, strlen(keyNameBeg) + 1);
  CHECK_ENOUGH_MEMORY(currentKeyName);
  strcpy(currentKeyName, keyNameBeg);
   return currentKeyName;
}

/******************************************************************************
 * Extracts from [HKEY\some\key\path] or HKEY\some\key\path types of line
 * the key class (what ends before the first '\')
 */
HKEY getRegClass(LPSTR lpClass)
{
  LPSTR classNameEnd;
  LPSTR classNameBeg;
  int i;

  char  lpClassCopy[KEY_MAX_LEN];

  if (lpClass == NULL)
    return (HKEY)ERROR_INVALID_PARAMETER;

  strncpy(lpClassCopy, lpClass, KEY_MAX_LEN);

  classNameEnd  = strchr(lpClassCopy, '\\');    /* The class name ends by '\' */
  if (!classNameEnd) {                          /* or the whole string */
      classNameEnd = lpClassCopy + strlen(lpClassCopy);
      if (classNameEnd[-1] == ']') {
          classNameEnd--;
      }
  }
  *classNameEnd = '\0';                       /* Isolate the class name */
  if (lpClassCopy[0] == '[') {
      classNameBeg = lpClassCopy + 1;
  } else {
      classNameBeg = lpClassCopy;
  }
  for (i = 0; i < REG_CLASS_NUMBER; i++) {
      if (!strcmp(classNameBeg, reg_class_names[i])) {
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
void doSetValue(LPSTR stdInput)
{
  /*
   * We encoutered the end of the file, make sure we
   * close the opened key and exit
   */
  if (stdInput == NULL) {
    if (bTheKeyIsOpen != FALSE)
      closeKey();
    return;
  }

  if ( stdInput[0] == '[') {     /* We are reading a new key */
    if ( bTheKeyIsOpen != FALSE )
      closeKey();                    /* Close the previous key before */
    if ( openKey(stdInput) != ERROR_SUCCESS )
      printf ("%s: doSetValue failed to open key %s\n", getAppName(), stdInput);
  } else if (( bTheKeyIsOpen ) &&
            (( stdInput[0] == '@') || /* reading a default @=data pair */
             ( stdInput[0] == '\"'))) { /* reading a new value=data pair */
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
void doQueryValue(LPSTR stdInput) {
  /*
   * We encoutered the end of the file, make sure we
   * close the opened key and exit
   */
  if (stdInput == NULL) {
    if (bTheKeyIsOpen != FALSE)
      closeKey();
    return;
  }

  if (stdInput[0] == '[') {     /* We are reading a new key */
    if ( bTheKeyIsOpen != FALSE )
      closeKey();                    /* Close the previous key before */
    if ( openKey(stdInput) != ERROR_SUCCESS )
      printf ("%s: doSetValue failed to open key %s\n", getAppName(), stdInput);
  }
  else if( ( bTheKeyIsOpen ) &&
           (( stdInput[0] == '@') || /* reading a default @=data pair */
           ( stdInput[0] == '\"'))) { /* reading a new value=data pair */
    processQueryValue(stdInput);
  } else {                /* since we are assuming that the file format is */
    if ( bTheKeyIsOpen )  /* valid we must be reading a blank line which  */
      closeKey();         /* indicate end of this key processing */
  }
}

/******************************************************************************
 * This funtion is the main entry point to the deletetValue type of action.  It
 * receives the currently read line and dispatch the work depending on the
 * context.
 */
void doDeleteValue(LPSTR line) {
  printf ("%s: deleteValue not yet implemented\n", getAppName());
}

/******************************************************************************
 * This funtion is the main entry point to the deleteKey type of action.  It
 * receives the currently read line and dispatch the work depending on the
 * context.
 */
void doDeleteKey(LPSTR line)   {
  printf ("%s: deleteKey not yet implemented\n", getAppName());
}

/******************************************************************************
 * This funtion is the main entry point to the createKey type of action.  It
 * receives the currently read line and dispatch the work depending on the
 * context.
 */
void doCreateKey(LPSTR line)   {
  printf ("%s: createKey not yet implemented\n", getAppName());
}

/******************************************************************************
 * This function is a wrapper for the setValue function.  It prepares the
 * land and clean the area once completed.
 * Note: this function modifies the line parameter.
 *
 * line - registry file unwrapped line. Should have the registry value name and
 *      complete registry value data.
 */
void processSetValue(LPSTR line)
{
  LPSTR val_name;                   /* registry value name   */
  LPSTR val_data;                   /* registry value data   */

  int line_idx = 0;                 /* current character under analysis */
  HRESULT hRes = 0;

  /* get value name */
  if (line[line_idx] == '@' && line[line_idx + 1] == '=') {
      line[line_idx] = '\0';
      val_name = line;
      line_idx++;
  } else if (line[line_idx] == '\"') {
      line_idx++;
      val_name = line + line_idx;
      while (TRUE) {
          if (line[line_idx] == '\\') {  /* skip escaped character */
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
      if (line[line_idx] != '=') {
          line[line_idx] = '\"';
          printf("Warning! uncrecognized line:\n%s\n", line);
          return;
      }
  } else {
      printf("Warning! uncrecognized line:\n%s\n", line);
      return;
  }
  line_idx++;                   /* skip the '=' character */
  val_data = line + line_idx;
  REGPROC_unescape_string(val_name);
  hRes = setValue(val_name, val_data);
  if ( hRes != ERROR_SUCCESS )
    printf("%s: ERROR Key %s not created. Value: %s, Data: %s\n",
           getAppName(), currentKeyName, val_name, val_data);
}

/******************************************************************************
 * This function is a wrapper for the queryValue function.  It prepares the
 * land and clean the area once completed.
 */
void processQueryValue(LPSTR cmdline)
{
  printf("ERROR!!! - temporary disabled");
  exit(1);
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

  if ((keyValue[0] == '@') && (strlen(keyValue) == 1)) {
    LONG lLen = KEY_MAX_LEN;
    CHAR* lpsData = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,KEY_MAX_LEN);
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
        lpbData = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpbData, dwLen);
        hRes = RegQueryValueEx(currentKeyHandle, keyValue, NULL, &dwType, (LPBYTE)lpbData, &dwLen);
    }

    if (hRes == ERROR_SUCCESS) {
      /*
       * Convert the returned data to a displayable format
       */
      switch (dwType) {
        case REG_SZ:
        case REG_EXPAND_SZ:
          lpsRes = HeapAlloc(GetProcessHeap(), 0, dwLen);
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
  if (hRes == ERROR_SUCCESS)
    printf("%s: Value \"%s\" = \"%s\" in key [%s]\n",
      getAppName(), keyValue, lpsRes, currentKeyName);

  else
    printf("%s: ERROR Value \"%s\" not found. for key \"%s\"\n",
      getAppName(), keyValue, currentKeyName);

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
    LPSTR line     = NULL;  /* line read from input stream */
    ULONG lineSize = REG_VAL_BUF_SIZE;

    line = HeapAlloc(GetProcessHeap(), 0, lineSize);
    CHECK_ENOUGH_MEMORY(line);

    while (!feof(in)) {
        LPSTR s; /* The pointer into line for where the current fgets should read */
        s = line;
        for (;;) {
            size_t size_remaining;
            int size_to_get;
            char *s_eol; /* various local uses */

            /* Do we need to expand the buffer ? */
            assert (s >= line && s <= line + lineSize);
            size_remaining = lineSize - (s-line);
            if (size_remaining < 2) { /* room for 1 character and the \0 */
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
            if (NULL == fgets (s, size_to_get, in)) {
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
            s_eol = strchr (s, '\n');
            if (!feof (in) && !s_eol) {
                s = strchr (s, '\0');
                /* It should be s + size_to_get - 1 but this is safer */
                continue;
            }

            /* If it is a comment line then discard it and go around again */
            if (line [0] == '#') {
                s = line;
                continue;
            }

            /* Remove any line feed.  Leave s_eol on the \0 */
            if (s_eol) {
                *s_eol = '\0';
                if (s_eol > line && *(s_eol-1) == '\r')
                    *--s_eol = '\0';
            }
            else
                s_eol = strchr (s, '\0');

            /* If there is a concatenating \\ then go around again */
            if (s_eol > line && *(s_eol-1) == '\\') {
                int c;
                s = s_eol-1;
                /* The following error protection could be made more self-
                 * correcting but I thought it not worth trying.
                 */
                if ((c = fgetc (in)) == EOF || c != ' ' ||
                    (c = fgetc (in)) == EOF || c != ' ')
                    printf ("%s: ERROR - invalid continuation.\n", getAppName());
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
void doRegisterDLL(LPSTR stdInput)
{
    HMODULE theLib = 0;
    UINT retVal = 0;

    /* Check for valid input */
    if (stdInput == NULL) return;

    /* Load and register the library, then free it */
    theLib = LoadLibraryA(stdInput);
    if (theLib) {
        FARPROC lpfnDLLRegProc = GetProcAddress(theLib, "DllRegisterServer");
        if (lpfnDLLRegProc)
            retVal = (*lpfnDLLRegProc)();
        else
            printf("%s: Couldn't find DllRegisterServer proc in '%s'.\n", getAppName(), stdInput);
        if (retVal != S_OK)
            printf("%s: DLLRegisterServer error 0x%x in '%s'.\n", getAppName(), retVal, stdInput);
        FreeLibrary(theLib);
    } else {
        printf("%s: Could not load DLL '%s'.\n", getAppName(), stdInput);
    }
}

/******************************************************************************
 * This funtion is the main entry point to the unregisterDLL action.  It
 * receives the currently read line, then loads and unregisters the requested DLLs
 */
void doUnregisterDLL(LPSTR stdInput)
{
    HMODULE theLib = 0;
    UINT retVal = 0;

    /* Check for valid input */
    if (stdInput == NULL) return;

    /* Load and unregister the library, then free it */
    theLib = LoadLibraryA(stdInput);
    if (theLib) {
        FARPROC lpfnDLLRegProc = GetProcAddress(theLib, "DllUnregisterServer");
        if (lpfnDLLRegProc)
            retVal = (*lpfnDLLRegProc)();
        else
            printf("%s: Couldn't find DllUnregisterServer proc in '%s'.\n", getAppName(), stdInput);
        if (retVal != S_OK)
            printf("%s: DLLUnregisterServer error 0x%x in '%s'.\n", getAppName(), retVal, stdInput);
        FreeLibrary(theLib);
    } else {
        printf("%s: Could not load DLL '%s'.\n", getAppName(), stdInput);
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
        printf("%s: Cannot display message for error %ld, status %ld\n",
               getAppName(), error_code, GetLastError());
        exit(1);
    }
    puts(lpMsgBuf);
    LocalFree((HLOCAL)lpMsgBuf);
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
void REGPROC_resize_char_buffer(CHAR **buffer, DWORD *len, DWORD required_len)
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
void REGPROC_export_string(FILE *file, CHAR *str)
{
    size_t len = strlen(str);
    size_t i;

    /* escaping characters */
    for (i = 0; i < len; i++) {
        CHAR c = str[i];
        switch (c) {
        case '\\': fputs("\\\\", file); break;
        case '\"': fputs("\\\"", file); break;
        case '\n': fputs("\\\n", file); break;
        default:   fputc(c, file);      break;
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
                 CHAR **reg_key_name_buf, DWORD *reg_key_name_len,
                 CHAR **val_name_buf, DWORD *val_name_len,
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
    curr_len = strlen(*reg_key_name_buf);
    REGPROC_resize_char_buffer(reg_key_name_buf, reg_key_name_len, max_sub_key_len + curr_len + 1);
    REGPROC_resize_char_buffer(val_name_buf, val_name_len, max_val_name_len);
    if (max_val_size > *val_size) {
        *val_size = max_val_size;
        *val_buf = HeapReAlloc(GetProcessHeap(), 0, *val_buf, *val_size);
        CHECK_ENOUGH_MEMORY(val_buf);
    }

    /* output data for the current key */
    fputs("\n[", file);
    fputs(*reg_key_name_buf, file);
    fputs("]\n", file);
    /* print all the values */
    i = 0;
    more_data = TRUE;
    while (more_data) {
        DWORD value_type;
        DWORD val_name_len1 = *val_name_len;
        DWORD val_size1 = *val_size;
        ret = RegEnumValueA(key, i, *val_name_buf, &val_name_len1, NULL, &value_type, *val_buf, &val_size1);
        if (ret != ERROR_SUCCESS) {
            more_data = FALSE;
            if (ret != ERROR_NO_MORE_ITEMS) {
                REGPROC_print_error();
            }
        } else {
            i++;
            if ((*val_name_buf)[0]) {
                fputs("\"", file);
                REGPROC_export_string(file, *val_name_buf);
                fputs("\"=", file);
            } else {
                fputs("@=", file);
            }

            switch (value_type) {
            case REG_SZ:
            case REG_EXPAND_SZ:
                fputs("\"", file);
                REGPROC_export_string(file, *val_buf);
                fputs("\"\n", file);
                break;
            case REG_DWORD:
                fprintf(file, "dword:%08lx\n", *((DWORD *)*val_buf));
                break;
            default:
                printf("%s: warning - unsupported registry format '%ld', "
                       "treat as binary\n", getAppName(), value_type);
                printf("key name: \"%s\"\n", *reg_key_name_buf);
                printf("value name:\"%s\"\n\n", *val_name_buf);
                /* falls through */
            case REG_MULTI_SZ:
                /* falls through */
            case REG_BINARY:
            {
                DWORD i1;
                CHAR *hex_prefix;
                CHAR buf[20];
                int cur_pos;

                if (value_type == REG_BINARY) {
                    hex_prefix = "hex:";
                } else {
                    hex_prefix = buf;
                    sprintf(buf, "hex(%ld):", value_type);
                }

                /* position of where the next character will be printed */
                /* NOTE: yes, strlen("hex:") is used even for hex(x): */
                cur_pos = strlen("\"\"=") + strlen("hex:") +
                    strlen(*val_name_buf);

                fputs(hex_prefix, file);
                for (i1 = 0; i1 < val_size1; i1++) {
                    fprintf(file, "%02x", (unsigned int)(*val_buf)[i1]);
                    if (i1 + 1 < val_size1) {
                        fputs(",", file);
                    }
                    cur_pos += 3;

                    /* wrap the line */
                    if (cur_pos > REG_FILE_HEX_LINE_LEN) {
                        fputs("\\\n  ", file);
                        cur_pos = 2;
                    }
                }
                fputs("\n", file);
                break;
            }
            }
        }
    }

    i = 0;
    more_data = TRUE;
    (*reg_key_name_buf)[curr_len] = '\\';
    while (more_data) {
        DWORD buf_len = *reg_key_name_len - curr_len;

        ret = RegEnumKeyExA(key, i, *reg_key_name_buf + curr_len + 1, &buf_len, NULL, NULL, NULL, NULL);
        if (ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA) {
            more_data = FALSE;
            if (ret != ERROR_NO_MORE_ITEMS) {
                REGPROC_print_error();
            }
        } else {
            HKEY subkey;

            i++;
            if (RegOpenKeyA(key, *reg_key_name_buf + curr_len + 1, &subkey) == ERROR_SUCCESS) {
                export_hkey(file, subkey, reg_key_name_buf, reg_key_name_len, val_name_buf, val_name_len, val_buf, val_size);
                RegCloseKey(subkey);
            } else {
                REGPROC_print_error();
            }
        }
    }
    (*reg_key_name_buf)[curr_len] = '\0';
}

/******************************************************************************
 * Open file for export.
 */
FILE *REGPROC_open_export_file(CHAR *file_name)
{
    FILE *file = fopen(file_name, "w");
    if (!file) {
        perror("");
        printf("%s: Can't open file \"%s\"\n", getAppName(), file_name);
        exit(1);
    }
    fputs("REGEDIT4\n", file);
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
void export_registry_key(CHAR* file_name, CHAR* reg_key_name)
{
    HKEY reg_key_class;

    CHAR *reg_key_name_buf;
    CHAR *val_name_buf;
    BYTE *val_buf;
    DWORD reg_key_name_len = KEY_MAX_LEN;
    DWORD val_name_len = KEY_MAX_LEN;
    DWORD val_size = REG_VAL_BUF_SIZE;
    FILE *file = NULL;

    reg_key_name_buf = HeapAlloc(GetProcessHeap(), 0, reg_key_name_len * sizeof(*reg_key_name_buf));
    val_name_buf = HeapAlloc(GetProcessHeap(), 0, val_name_len * sizeof(*val_name_buf));
    val_buf = HeapAlloc(GetProcessHeap(), 0, val_size);
    CHECK_ENOUGH_MEMORY(reg_key_name_buf && val_name_buf && val_buf);

    if (reg_key_name && reg_key_name[0]) {
        CHAR *branch_name;
        HKEY key;

        REGPROC_resize_char_buffer(&reg_key_name_buf, &reg_key_name_len,
                                   strlen(reg_key_name));
        strcpy(reg_key_name_buf, reg_key_name);

        /* open the specified key */
        reg_key_class = getRegClass(reg_key_name);
        if (reg_key_class == (HKEY)ERROR_INVALID_PARAMETER) {
            printf("%s: Incorrect registry class specification in '%s'\n",
                   getAppName(), reg_key_name);
            exit(1);
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
        } else if (RegOpenKeyA(reg_key_class, branch_name, &key) == ERROR_SUCCESS) {
            file = REGPROC_open_export_file(file_name);
            export_hkey(file, key,
                        &reg_key_name_buf, &reg_key_name_len,
                        &val_name_buf, &val_name_len,
                        &val_buf, &val_size);
            RegCloseKey(key);
        } else {
            printf("%s: Can't export. Registry key '%s' does not exist!\n",
                   getAppName(), reg_key_name);
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
                strcpy(reg_key_name_buf, reg_class_names[i]);
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
    HeapFree(GetProcessHeap(), 0, reg_key_name);
    HeapFree(GetProcessHeap(), 0, val_buf);
}

/******************************************************************************
 * Recursive function which removes the registry key with all subkeys.
 */
void delete_branch(HKEY key, CHAR** reg_key_name_buf, DWORD* reg_key_name_len)
{
    HKEY branch_key;
    DWORD max_sub_key_len;
    DWORD subkeys;
    DWORD curr_len;
    LONG ret;
    long int i;

    if (RegOpenKeyA(key, *reg_key_name_buf, &branch_key) != ERROR_SUCCESS) {
        REGPROC_print_error();
    }

    /* get size information and resize the buffers if necessary */
    if (RegQueryInfoKey(branch_key, NULL, NULL, NULL, &subkeys, &max_sub_key_len,
                        NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
        REGPROC_print_error();
    }
    curr_len = strlen(*reg_key_name_buf);
    REGPROC_resize_char_buffer(reg_key_name_buf, reg_key_name_len, max_sub_key_len + curr_len + 1);

    (*reg_key_name_buf)[curr_len] = '\\';
    for (i = subkeys - 1; i >= 0; i--) {
        DWORD buf_len = *reg_key_name_len - curr_len;
        ret = RegEnumKeyExA(branch_key, i, *reg_key_name_buf + curr_len + 1, &buf_len, NULL, NULL, NULL, NULL);
        if (ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA && ret != ERROR_NO_MORE_ITEMS) {
            REGPROC_print_error();
        } else {
            delete_branch(key, reg_key_name_buf, reg_key_name_len);
        }
    }
    (*reg_key_name_buf)[curr_len] = '\0';
    RegCloseKey(branch_key);
    RegDeleteKeyA(key, *reg_key_name_buf);
}

/******************************************************************************
 * Removes the registry key with all subkeys. Parses full key name.
 *
 * Parameters:
 * reg_key_name - full name of registry branch to delete. Ignored if is NULL,
 *      empty, points to register key class, does not exist.
 */
void delete_registry_key(CHAR* reg_key_name)
{
    CHAR* branch_name;
    DWORD branch_name_len;
    HKEY reg_key_class;
    HKEY branch_key;

    if (!reg_key_name || !reg_key_name[0])
        return;
    /* open the specified key */
    reg_key_class = getRegClass(reg_key_name);
    if (reg_key_class == (HKEY)ERROR_INVALID_PARAMETER) {
        printf("%s: Incorrect registry class specification in '%s'\n", getAppName(), reg_key_name);
        exit(1);
    }
    branch_name = getRegKeyName(reg_key_name);
    CHECK_ENOUGH_MEMORY(branch_name);
    branch_name_len = strlen(branch_name);
    if (!branch_name[0]) {
        printf("%s: Can't delete registry class '%s'\n", getAppName(), reg_key_name);
        exit(1);
    }
    if (RegOpenKeyA(reg_key_class, branch_name, &branch_key) == ERROR_SUCCESS) {
        /* check whether the key exists */
        RegCloseKey(branch_key);
        delete_branch(reg_key_class, &branch_name, &branch_name_len);
    }
    HeapFree(GetProcessHeap(), 0, branch_name);
}

/******************************************************************************
 * Sets the application name. Then application name is used in the error
 * reporting.
 */
void setAppName(CHAR* name)
{
    app_name = name;
}

CHAR* getAppName(VOID)
{
    return app_name;
}

