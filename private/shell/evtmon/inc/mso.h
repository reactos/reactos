/*----------------------------------------------------------------------------
    %%File: mso.h
    %%Unit: Event Monitor
    %%Contact: daleg

    Typedef file for Rules Engine of Event Monitor.
----------------------------------------------------------------------------*/

#ifndef MSO_H
#define MSO_H

#ifndef MSOEXTERN_C_BEGIN
#if defined(__cplusplus)
    #define MSOEXTERN_C extern "C"
    #define MSOEXTERN_C_BEGIN extern "C" {
    #define MSOEXTERN_C_END }
#else
    #define MSOEXTERN_C
    #define MSOEXTERN_C_BEGIN
    #define MSOEXTERN_C_END
#endif
#endif // MSOEXTERN_C_BEGIN

#define FEWER_SEGS(sz)
#define VSZASSERT       static unsigned char    vszAssertFile[] = __FILE__;

typedef long IDS;                                       // emkwd.h
#define OFC_CALLBACK        MSOAPICALLTYPE
typedef unsigned short WCHAR;
typedef unsigned short WORD;
typedef unsigned int UINT;
#ifndef _WINDEF_    // windef.h makes uchar (same as mso.h w/ -J)
typedef char BYTE;
#endif
typedef int BOOL;
typedef unsigned long DWORD;

#ifndef cbXchar
#ifndef ANSI_XCHAR
typedef unsigned short XCHAR;
#define cbXchar    2

#else /* ANSI_XCHAR */

typedef unsigned char XCHAR;
#define cbXchar    1
#endif /* !ANSI_XCHAR */
#endif /* !cbXchar */

typedef WORD LID;
typedef DWORD LCID;

#define MSOAPICALLTYPE __stdcall
#define MSOCDECLCALLTYPE __cdecl

#ifndef MSOAPI_
#define MSOAPI_(t)              t MSOAPICALLTYPE
#endif /* !MSOAPI_ */

#ifndef MSOAPIX_
#define MSOAPIX_(t)             t MSOAPICALLTYPE
#endif /* !MSOAPIX_ */

#ifndef MSOCDECLAPI_
#define MSOCDECLAPI_(t)         t MSOCDECLCALLTYPE
#endif /* !MSOCDECLAPI_ */

#ifndef MSOMACAPI_
#define MSOMACAPI_(t)   t
#endif /* !MSOMACAPI_ */

#if !defined(WIN)  &&  defined(NT)
#define WIN
#endif /* !WIN  &&  NT */

#ifdef WIN
#define Win(foo) foo
#define WinMac(win,mac) win
#define WinElse(win,foo) win
#define Nt(foo) foo
#define NtElse(nt,notnt) nt
#else
#define Win(foo)
#define WinMac(win,mac) mac
#define WinElse(win,foo) foo
#define Nt(foo)
#define NtElse(nt,notnt) notnt
#endif

#ifdef MAC
#define Mac(foo) foo
#define MacElse(mac, notmac) mac
#define NotMac(foo)
#else
#define Mac(foo)
#define MacElse(mac, notmac) notmac
#define NotMac(foo) foo
#endif

#define _MAX_PATH   260 /* max. length of full pathname */
#define MsoStrcpy strcpy
#define MsoStrcat strcat
#define MsoStrlen strlen
#define MsoSzIndexRight strrchr
#define MsoMemcpy memcpy
#define MsoMemset memset
#define MsoMemcmp memcmp
#define MsoMemmove memmove

#define MsoCchSzLen(sz)         (strlen(sz))
#define MsoCchWzLen(xsz)        (wcslen(xsz))

#define MsoRgwchToRgch(rgchFrom, cchFrom, rgchTo, cchTo) \
            (PbCopyRgb((rgchFrom), (rgchTo), (cchFrom)), (cchFrom))


MSOAPI_(WCHAR) MsoWchToUpper(WCHAR wch);

#ifdef RULE_COMPILER
#define LANGIDFROMLCID(lcid)   ((WORD  )(lcid))
#define msoStrip 0x01                                   // REVIEW PREHASH
#endif /* RULE_COMPILER */

#define MsoWzToSz(p, rgch) (p)

#ifdef STANDALONE_WORD
#include "word.h"

#define TRUE    1
#define FALSE   0

#else /* !STANDALONE_WORD */

#include <windows.h>
#include <string.h>
///#include "sys.h"

#endif /* STANDALONE_WORD */

#include <ctype.h>

_inline ChUpper(unsigned char ch)
{
    return (islower(ch) ? toupper(ch) : ch);
}

void __cdecl Fail(const char *lsz, ...);                // Print failure msg

#ifndef FReportLszProc
#define FReportLszProc(lszExtra, lszFile, line)             TRUE
#endif /* !FReportLszProc */

/* Breaks into the debugger.  Works (more or less) on all supported
    systems. */
#ifndef MAC
    #define MsoDebugBreakInline() {__asm int 3}
#else /* MAC */
    #define MsoDebugBreakInline() Debugger()
#endif /* !MAC */
#endif /* !MSO_H */
