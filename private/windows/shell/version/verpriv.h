/***************************************************************************
 *  VERPRIV.H
 *
 *    Private include file for the version stamping API.  Includes
 *    WINDOWS.H if this is being made to be a DLL.
 *
 ***************************************************************************/

#ifndef VERPRIV_H
#define VERPRIV_H
#undef DBCS

#include <stdlib.h>
#include <windows.h>

#define SEEK_START      0
#define SEEK_CURRENT 1
#define SEEK_END     2

/* ----- Function prototypes ----- */
#define FileClose(a)       LZClose(a)
#define FileRead(a, b, c)  LZRead((a), (b), (c))
#define FileSeek(a, b, c)  LZSeek((a), (b), (c))


BOOL
APIENTRY
VerpQueryValue(
   const LPVOID pb,
   LPVOID lpSubBlockX,
   INT    nIndex,
   LPVOID *lplpKey,
   LPVOID *lplpBuffer,
   PUINT  puLen,
   BOOL  bUnicodeNeeded
   );

#endif /* VERPRIV_H */


#if 0
#define LOG_DATA 1
#endif


#ifdef LOG_DATA
#   pragma message(__FILE__"(43) : warning !!!! : remove debug code before checking in" )
extern void LogThisData( DWORD id, char *szMsg, DWORD dwLine, DWORD dwData );

#   define LogData( sz, dw1, dw2 )   LogThisData(GetCurrentThreadId(), sz "            ", dw1, dw2 )
#else
#   define LogData( sz, dw1, dw2 )
#endif



#ifndef ARRAYSIZE
#   define ARRAYSIZE(sz)   (sizeof(sz) / sizeof((sz)[0]))
#endif
