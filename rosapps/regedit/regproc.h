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

typedef void (*CommandAPI)(LPSTR lpsLine);

void doSetValue(LPSTR lpsLine);
void doDeleteValue(LPSTR lpsLine);
void doCreateKey(LPSTR lpsLine);
void doDeleteKey(LPSTR lpsLine);
void doQueryValue(LPSTR lpsLine);
void doRegisterDLL(LPSTR lpsLine);
void doUnregisterDLL(LPSTR lpsLine);

void export_registry_key(CHAR* file_name, CHAR* reg_key_name);
void delete_registry_key(CHAR* reg_key_name);

void setAppName(CHAR* name);
CHAR* getAppName(VOID);

void processRegLines(FILE* in, CommandAPI command);

/*
 * Generic prototypes
 */
char*   getToken(char** str, const char* delims);
void    get_file_name(CHAR** command_line, CHAR* filename);
DWORD   convertHexToDWord(char* str, BYTE* buf);
DWORD   convertHexCSVToHex(char* str, BYTE* buf, ULONG bufLen);
LPSTR   convertHexToHexCSV(BYTE* buf, ULONG len);
LPSTR   convertHexToDWORDStr(BYTE* buf, ULONG len);
LPSTR   getRegKeyName(LPSTR lpLine);
HKEY    getRegClass(LPSTR lpLine);
DWORD   getDataType(LPSTR* lpValue, DWORD* parse_type);
LPSTR   getArg(LPSTR arg);
HRESULT openKey(LPSTR stdInput);
void    closeKey(VOID);

/*
 * api setValue prototypes
 */
void    processSetValue(LPSTR cmdline);
HRESULT setValue(LPSTR val_name, LPSTR val_data);

/*
 * api queryValue prototypes
 */
void    processQueryValue(LPSTR cmdline);

