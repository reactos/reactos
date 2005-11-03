/*
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

/******************************************************************************
 * Defines and consts
 */
#define KEY_MAX_LEN             1024

/* Return values */
#define SUCCESS               0
#define KEY_VALUE_ALREADY_SET 2

typedef void (*CommandAPI)(LPTSTR lpsLine);

void doSetValue(LPTSTR lpsLine);
void doDeleteValue(LPTSTR lpsLine);
void doCreateKey(LPTSTR lpsLine);
void doDeleteKey(LPTSTR lpsLine);
void doQueryValue(LPTSTR lpsLine);
void doRegisterDLL(LPTSTR lpsLine);
void doUnregisterDLL(LPTSTR lpsLine);

BOOL export_registry_key(TCHAR* file_name, TCHAR* reg_key_name);
BOOL import_registry_file(LPTSTR filename);
void delete_registry_key(TCHAR* reg_key_name);

void processRegLines(FILE* in, CommandAPI command);

/*
 * Generic prototypes
 */
#ifdef _UNICODE
#define get_file_name get_file_nameW
#else
#define get_file_name get_file_nameA
#endif

char*   getToken(char** str, const char* delims);
void    get_file_nameA(CHAR** command_line, CHAR* filename, int max_filename);
void    get_file_nameW(CHAR** command_line, WCHAR* filename, int max_filename);
DWORD   convertHexToDWord(TCHAR* str, BYTE* buf);
DWORD   convertHexCSVToHex(TCHAR* str, BYTE* buf, ULONG bufLen);
LPTSTR  convertHexToHexCSV(BYTE* buf, ULONG len);
LPTSTR  convertHexToDWORDStr(BYTE* buf, ULONG len);
LPTSTR  getRegKeyName(LPTSTR lpLine);
HKEY    getRegClass(LPTSTR lpLine);
DWORD   getDataType(LPTSTR* lpValue, DWORD* parse_type);
LPTSTR  getArg(LPTSTR arg);
HRESULT openKey(LPTSTR stdInput);
void    closeKey(VOID);

/*
 * api setValue prototypes
 */
void    processSetValue(LPTSTR cmdline);
HRESULT setValue(LPTSTR val_name, LPTSTR val_data);

/*
 * api queryValue prototypes
 */
void    processQueryValue(LPTSTR cmdline);

#ifdef __GNUC__
#ifdef WIN32_REGDBG
//typedef UINT_PTR SIZE_T, *PSIZE_T;
//#define _MAX_PATH   260 /* max. length of full pathname */
#endif /*WIN32_REGDBG*/

#ifdef UNICODE
#define _tfopen     _wfopen
#else
#define _tfopen     fopen
#endif

#endif /*__GNUC__*/

LPVOID RegHeapAlloc(
  HANDLE hHeap,   // handle to private heap block
  DWORD dwFlags,  // heap allocation control
  SIZE_T dwBytes  // number of bytes to allocate
);

LPVOID RegHeapReAlloc(
  HANDLE hHeap,   // handle to heap block
  DWORD dwFlags,  // heap reallocation options
  LPVOID lpMem,   // pointer to memory to reallocate
  SIZE_T dwBytes  // number of bytes to reallocate
);

BOOL RegHeapFree(
  HANDLE hHeap,  // handle to heap
  DWORD dwFlags, // heap free options
  LPVOID lpMem   // pointer to memory
);
