/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REG1632.H
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        06 Apr 1994
*
*  Win32 and MS-DOS compatibility macros for the Registry Editor.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  06 Apr 1994 TCS Original implementation.
*
*******************************************************************************/

#ifndef _INC_REG1632
#define _INC_REG1632

#ifndef LPCHAR
typedef CHAR FAR*                       LPCHAR;
#endif

#ifdef WIN32
#define WSPRINTF(x)                     wsprintf ##x
#else
#define WSPRINTF(x)                     sprintf ##x
#endif

#ifdef WIN32
#define STRCMP(string1, string2)        lstrcmp(string1, string2)
#else
#define STRCMP(string1, string2)        _fstrcmp(string1, string2)
#endif

#ifdef WIN32
#define STRCPY(string1, string2)        lstrcpy(string1, string2)
#else
#define STRCPY(string1, string2)        _fstrcpy(string1, string2)
#endif

#ifdef WIN32
#define STRLEN(string)                  lstrlen(string)
#else
#define STRLEN(string)                  _fstrlen(string)
#endif

#ifdef WIN32
#define STRCHR(string, character)       StrChr(string, character)
#else
#define STRCHR(string, character)       _fstrchr(string, character)
#endif

#ifdef WIN32
#define CHARNEXT(string)                CharNext(string)
#else
#define CHARNEXT(string)                (string + 1)
#endif

#ifdef WIN32
#define CHARUPPERSTRING(string)         CharUpper(string)
#else
#define CHARUPPERSTRING(string)         _fstrupr(string)
#endif

#ifdef WIN32
#define FILE_HANDLE                     HANDLE
#else
#define FILE_HANDLE                     int
#endif

#ifdef WIN32
#define FILE_NUMBYTES                   DWORD
#else
#define FILE_NUMBYTES                   unsigned
#endif

#ifdef WIN32
#define OPENREADFILE(pfilename, handle)                                     \
    ((handle = CreateFile(pfilename, GENERIC_READ, FILE_SHARE_READ,         \
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) !=               \
        INVALID_HANDLE_VALUE)
#else
#define OPENREADFILE(pfilename, handle)                                     \
    (_dos_open(pfilename, _O_RDONLY, &handle) == 0)
#endif

#ifdef WIN32
#define OPENWRITEFILE(pfilename, handle)                                    \
    ((handle = CreateFile(pfilename, GENERIC_WRITE, 0,                      \
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) !=               \
        INVALID_HANDLE_VALUE)
#else
#define OPENWRITEFILE(pfilename, handle)                                    \
    (_dos_creat(pfilename, _A_NORMAL, &handle) == 0)
#endif

#ifdef WIN32
#define READFILE(handle, buffer, count, pnumbytes)                          \
    ReadFile(handle, buffer, count, pnumbytes, NULL)
#else
#define READFILE(handle, buffer, count, pnumbytes)                          \
    (_dos_read(handle, buffer, count, pnumbytes) == 0)
#endif

#ifdef WIN32
#define WRITEFILE(handle, buffer, count, pnumbytes)                         \
    WriteFile(handle, buffer, count, pnumbytes, NULL)
#else
#define WRITEFILE(handle, buffer, count, pnumbytes)                         \
    (_dos_write(handle, buffer, count, pnumbytes) == 0)
#endif

#ifdef WIN32
#define SEEKCURRENTFILE(handle, count)                                      \
    (SetFilePointer(handle, (LONG) count, NULL, FILE_CURRENT))
#else
#define SEEKCURRENTFILE(handle, count)                                      \
    (_dos_seek(handle, (unsigned long) count, SEEK_CUR))
#endif

#ifdef WIN32
#define CLOSEFILE(handle)               CloseHandle(handle)
#else
#define CLOSEFILE(handle)               _dos_close(handle)
#endif

#ifdef WIN32
#define GETFILESIZE(handle)             GetFileSize(handle, NULL)
#else
DWORD
NEAR PASCAL
GetFileSize(
    FILE_HANDLE hFile
    );
#define GETFILESIZE(handle)             GetFileSize(handle)
#endif

#endif // _INC_REG1632
