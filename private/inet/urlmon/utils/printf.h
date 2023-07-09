/***
*printf.h - print formatted
*
*       Copyright (c) 1985-1991, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines w4*printf() - print formatted data
*       defines w4v*printf() - print formatted output, get data from an
*                              argument ptr instead of explicit args.
*
*Revision History:
*       09-02-83  RN    original sprintf
*       06-17-85  TC    rewrote to use new varargs macros, and to be vsprintf
*       04-13-87  JCR   added const to declaration
*       11-07-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-27-88  PHG   Merged DLL and normal versions
*       06-13-88  JCR   Fake _iob entry is now static so that other routines
*                       can assume _iob entries are in DGROUP.
*       08-25-88  GJF   Define MAXSTR to be INT_MAX (from LIMITS.H).
*       06-06-89  JCR   386 mthread support
*       08-18-89  GJF   Clean up, now specific to OS/2 2.0 (i.e., 386 WIN32
*                       model). Also fixed copyright and indents.
*       02-16-90  GJF   Fixed copyright
*
*******************************************************************************/

#include <windows.h>
#include <wchar.h>
#include <stdarg.h>
#include <limits.h>
#include "w4io.h"

#if defined(_W4PRINTF_)
    static INT_PTR  fh;
//    extern long GetStdHandle(long);
//    extern void WriteFile(long fh, char *s, long cch, long * pcchret, long);
#   define _PRINTF_
#elif defined(_W4DPRINTF_)
#   define _pwritechar  _dwritechar
#   define _pflushbuf   _dflushbuf
#   define w4printf     w4dprintf
#   define w4vprintf    w4vdprintf
#   define _PRINTF_
#elif defined(_W4SPRINTF_)
#   define _pwritechar  _swritechar
#   define w4printf     w4sprintf
#   define w4vprintf    w4vsprintf
#elif defined(_W4WCSPRINTF_)
#   define _TCHAR_      wchar_t
#   define _PBUF_       pwcbuf
#   define _PSTART_     pwcstart
#   define w4printf     w4wcsprintf
#   define w4vprintf    w4vwcsprintf
#   define _pwritechar  _wwritechar
#else
#   error configuration problem
#endif

#ifndef _TCHAR_
#  define _TCHAR_       char
#  define _PBUF_        pchbuf
#  define _PSTART_      pchstart
#endif


#ifdef _PRINTF_
#   ifdef WIN32
#     undef  OutputDebugString
#     define OutputDebugString OutputDebugStringA
#   else
      extern void _pascal OutputDebugString(char *);
#   endif
    int _cdecl _pflushbuf(struct w4io *f);
#   define SPR(a)
#   define MAXSTR       128
#else
#   define SPR(a)       a,
#   define MAXSTR       INT_MAX
#endif

void _cdecl _pwritechar(int ch, int num, struct w4io *f, int *pcchwritten);
int _cdecl w4vprintf(SPR(_TCHAR_ *string) const char *format, va_list arglist);


/***
*int w4printf(format, ...) - print formatted data
*
*Purpose:
*       Prints formatted data using the format string to
*       format data and getting as many arguments as called for
*       Sets up a w4io so file i/o operations can be used.
*       w4iooutput does the real work here
*
*Entry:
*       char *format - format string to control data format/number
*       of arguments followed by list of arguments, number and type
*       controlled by format string
*
*Exit:
*       returns number of characters written
*
*Exceptions:
*
*******************************************************************************/


int _cdecl
w4printf(SPR(_TCHAR_ *string) const char *format, ...)
/*
 * 'PRINT', 'F'ormatted
 */
{
    va_list arglist;

    va_start(arglist, format);
    return(w4vprintf(SPR(string) format, arglist));
}


/***
*int w4vprintf(format, arglist) - print formatted data from arg ptr
*
*Purpose:
*       Prints formatted data, but gets data from an argument pointer.
*       Sets up a w4io so file i/o operations can be used, make string look
*       like a huge buffer to it, but _flsbuf will refuse to flush it if it
*       fills up. Appends '\0' to make it a true string.
*
*       Multi-thread: (1) Since there is no stream, this routine must never try
*       to get the stream lock (i.e., there is no stream lock either).  (2)
*       Also, since there is only one staticly allocated 'fake' iob, we must
*       lock/unlock to prevent collisions.
*
*Entry:
*       char *format    - format string, describes format of data
*       va_list arglist - varargs argument pointer
*
*Exit:
*       returns number of characters written
*
*Exceptions:
*
*******************************************************************************/

int _cdecl
w4vprintf(SPR(_TCHAR_ *string) const char *format, va_list arglist)
/*
 * 'V'ariable argument 'PRINT', 'F'ormatted
 */
{
    struct w4io outfile;
    register int retval;
#ifdef _PRINTF_
    char string[MAXSTR + 1];            // leave room for null termination
#else
    int dummy = 0;
#endif

#ifdef _W4PRINTF_
    long ldummy;

    if (fh == 0 || fh == -1)
    {
        ldummy = -11;                   // C7 bug workaround
        if ((fh = (INT_PTR)GetStdHandle(ldummy)) == 0 || fh == -1)
        {
            OutputDebugString("GetStdHandle in " __FILE__ " failed\n");
            return(-1);
        }
    }
#endif

    outfile._PBUF_ = outfile._PSTART_ = string;
    outfile.cchleft = MAXSTR;
    outfile.writechar = _pwritechar;

    retval = w4iooutput(&outfile, format, arglist);

#ifdef _PRINTF_
    if (_pflushbuf(&outfile) == -1) {
        return(-1);
    }
#else
    _pwritechar('\0', 1, &outfile, &dummy);
#endif
    return(retval);
}


void _cdecl _pwritechar(int ch, int num, struct w4io *f, int *pcchwritten)
{
    //printf("  char: ch=%c, cnt=%d, cch=%d\n", ch, num, *pcchwritten);
    while (num-- > 0) {
#ifdef _PRINTF_
        if (f->cchleft < 2 && _pflushbuf(f) == -1) {
            *pcchwritten = -1;
            return;
        }
#endif
#ifdef _W4DPRINTF_
#  ifndef WIN32
        if (ch == '\n')
        {
            *f->_PBUF_++ = '\r';
            f->cchleft--;
            (*pcchwritten)++;
        }
#  endif
#endif
        *f->_PBUF_++ = (char) ch;
        f->cchleft--;
        (*pcchwritten)++;
    }
}


#ifdef _PRINTF_
int _cdecl _pflushbuf(struct w4io *f)
{
    int cch;

    if (cch = (int)(f->pchbuf - f->pchstart))
    {
#ifdef _W4DPRINTF_
        *f->pchbuf = '\0';              // null terminate
        OutputDebugString(f->pchstart);
#else
        long cchret;

        //*f->pchbuf = '\0';            // null terminate
        //printf("%d chars: \"%s\"\n", cch, f->pchstart);
        WriteFile((HANDLE)fh, f->pchstart, cch, &cchret, 0);
        if (cch != cchret)
        {
            OutputDebugString("WriteFile in " __FILE__ " failed\n");
            return(-1);
        }
#endif
        f->pchbuf -= cch;               // reset pointer
        f->cchleft += cch;              // reset count
    }
    return(0);
}
#endif // _PRINTF_
