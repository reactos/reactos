/****************************************************************************/
/*                                                                          */
/*  UNICONV.H -                                                             */
/*                                                                          */
/*       My special Unicode workaround file                                 */
/*                                                                          */
/*       Created by      :       Diane K. Oh                                */
/*       On Date         :       June 11, 1992                              */
/*                                                                          */
/*       File was created to Unicode enable special function calls of       */
/*       WIN32 Shell applications.                                          */
/*                                                                          */
/****************************************************************************/

#include <string.h>
#include <tchar.h>
#ifdef UNICODE
#include <wchar.h>
#else
#include <ctype.h>
#endif


/*--------------------------------------------------------------------------*/
/*  Macros                                                                  */
/*--------------------------------------------------------------------------*/

#define CharSizeOf(x)   (sizeof(x) / sizeof(*x))
#define ByteCountOf(x)  ((x) * sizeof(TCHAR))

/*--------------------------------------------------------------------------*/
/*  Constants                                                               */
/*--------------------------------------------------------------------------*/

#define   UNICODE_FONT_NAME        TEXT("Lucida Sans Unicode")
#define   UNICODE_FIXED_FONT_NAME  TEXT("Lucida Console")

/*--------------------------------------------------------------------------*/
/*  Function Templates                                                      */
/*--------------------------------------------------------------------------*/

INT      MyAtoi          (LPTSTR string);
LONG     MyAtol          (LPTSTR  string);
LPTSTR   MyItoa          (INT value, LPTSTR string, INT radix);
LPTSTR   MyUltoa         (unsigned long value, LPTSTR string, INT radix);
VOID     MySplitpath     (LPTSTR path, LPTSTR drive, LPTSTR dir,
                          LPTSTR fname, LPTSTR ext);

LPTSTR   SkipProgramName (LPTSTR lpCmdLine);


HANDLE   MyOpenFile      (LPTSTR lpszFile, TCHAR * lpszPath, DWORD fuMode);
BOOL     MyCloseFile     (HANDLE hFile);
BOOL     MyAnsiReadFile  (HANDLE hFile, UINT uCodePage, LPVOID lpBuffer, DWORD nChars);
BOOL     MyByteReadFile  (HANDLE hFile, LPVOID lpBuffer, DWORD nBytes);
BOOL     MyAnsiWriteFile (HANDLE hFile, UINT uCodePage, LPVOID lpBuffer, DWORD nChars);
BOOL     MyByteWriteFile (HANDLE hFile, LPVOID lpBuffer, DWORD nBytes);
LONG     MyFileSeek      (HANDLE hFile, LONG lDistanceToMove, DWORD dwMoveMethod);

