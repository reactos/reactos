/*****************************************************************/
/**             Microsoft Windows for Workgroups                **/
/**         Copyright (C) Microsoft Corp., 1991-1992            **/
/*****************************************************************/

/*
    npassert.h
    NP environment independent assertion/logging routines

    Usage:

        ASSERT(exp)     Evaluates its argument.  If "exp" evals to
                        FALSE, then the app will terminate, naming
                        the file name and line number of the assertion
                        in the source.

        UIASSERT(exp)   Synonym for ASSERT.

        ASSERTSZ(exp,sz) As ASSERT, except will also print the message
                        "sz" with the assertion message should it fail.

        REQUIRE(exp)    As ASSERT, except that its expression is still
                        evaluated in retail versions.  (Other versions
                        of ASSERT disappear completely in retail builds.)

        ANSI_ASSERTABLE(sz) Declares "sz" to be a string buffer which
                        can be used with the ASSERT_ANSI and ASSERT_OEM
                        macros (effectively declares a debug-only BOOL
                        associated with the string).

        ASSERT_ANSI(sz) Asserts that sz is in the ANSI character set.

        ASSERT_OEM(sz)  Asserts that sz is in the OEM character set.

        IS_ANSI(sz)     Declares that sz is in the ANSI character set
                        (e.g., if it's just come back from a GetWindowText).

        IS_OEM(sz)      Declares that sz is in the OEM character set.

        TO_ANSI(sz)     Does OemToAnsi in place.

        TO_OEM(sz)      Does AnsiToOem in place.

        COPY_TO_ANSI(src,dest)  Does OemToAnsi, not in place.

        COPY_TO_OEM(src,dest)   Does AnsiToOem, not in place.

        NOTE: the latter two, just like the APIs themselves, have the
        source first and destination second, opposite from strcpy().

    The ASSERT macros expect a symbol _FILENAME_DEFINED_ONCE, and will
    use the value of that symbol as the filename if found; otherwise,
    they will emit a new copy of the filename, using the ANSI C __FILE__
    macro.  A client sourcefile may therefore define __FILENAME_DEFINED_ONCE
    in order to minimize the DGROUP footprint of a number of ASSERTs.

    FILE HISTORY:
        Johnl   11/15/90    Converted from CAssert to general purpose
        Johnl   12/06/90    Changed _FAR_ to _far in _assert prototype
        beng    04/30/91    Made C-includable
        beng    08/05/91    Made assertions occupy less dgroup; withdrew
                            explicit heapchecking (which was crt
                            dependent anyway)
        beng    09/17/91    Removed additional consistency checks;
                            rewrote to minimize dgroup footprint,
                            check expression in-line
        beng    09/19/91    Fixed my own over-cleverness
        beng    09/25/91    Fixed stupid bug in retail REQUIRE
        gregj   03/23/93    Ported to Chicago environment
        gregj   05/11/93    Added ANSI/OEM asserting routines
        gregj   05/25/93    Added COPY_TO_ANSI and COPY_TO_OEM
*/


#ifndef _NPASSERT_H_
#define _NPASSERT_H_

#if defined(__cplusplus)
extern "C"
{
#else
extern
#endif

VOID UIAssertHelper( const CHAR* pszFileName, UINT nLine );
VOID UIAssertSzHelper( const CHAR* pszMessage, const CHAR* pszFileName, UINT nLine );
extern const CHAR szShouldBeAnsi[];
extern const CHAR szShouldBeOEM[];

#if defined(__cplusplus)
}
#endif

#if defined(DEBUG)

# if defined(_FILENAME_DEFINED_ONCE)

#  define ASSERT(exp) \
    { if (!(exp)) UIAssertHelper(_FILENAME_DEFINED_ONCE, __LINE__); }

#  define ASSERTSZ(exp, sz) \
    { if (!(exp)) UIAssertSzHelper((sz), _FILENAME_DEFINED_ONCE, __LINE__); }

# else

#  define ASSERT(exp) \
    { if (!(exp)) UIAssertHelper(__FILE__, __LINE__); }

#  define ASSERTSZ(exp, sz) \
    { if (!(exp)) UIAssertSzHelper((sz), __FILE__, __LINE__); }

# endif

# define UIASSERT(exp)  ASSERT(exp)
# define REQUIRE(exp)   ASSERT(exp)

#define EXTERN_ANSI_ASSERTABLE(sz)  extern BOOL fAnsiIs##sz;
#define ANSI_ASSERTABLE(sz) BOOL fAnsiIs##sz=FALSE;
#define ASSERT_ANSI(sz)     ASSERTSZ(fAnsiIs##sz,szShouldBeAnsi)
#define ASSERT_OEM(sz)      ASSERTSZ(!fAnsiIs##sz,szShouldBeOEM)
#define IS_ANSI(sz)         fAnsiIs##sz = TRUE;
#define IS_OEM(sz)          fAnsiIs##sz = FALSE;
#define TO_ANSI(sz)         { ASSERT_OEM(sz); ::OemToAnsi(sz,sz); IS_ANSI(sz); }
#define TO_OEM(sz)          { ASSERT_ANSI(sz); ::AnsiToOem(sz,sz); IS_OEM(sz); }
#define COPY_TO_ANSI(s,d)   { ASSERT_OEM(s); ::OemToAnsi(s,d); IS_ANSI(d); }
#define COPY_TO_OEM(s,d)    { ASSERT_ANSI(s); ::AnsiToOem(s,d); IS_OEM(d); }

#else // !DEBUG

# define ASSERT(exp)        ;
# define UIASSERT(exp)      ;
# define ASSERTSZ(exp, sz)  ;
# define REQUIRE(exp)       { (exp); }

#define EXTERN_ANSI_ASSERTABLE(sz)  ;
#define ANSI_ASSERTABLE(sz) ;
#define ASSERT_ANSI(sz)     ;
#define ASSERT_OEM(sz)      ;
#define IS_ANSI(sz)         ;
#define IS_OEM(sz)          ;
#define TO_ANSI(sz)         ::OemToAnsi(sz,sz)
#define TO_OEM(sz)          ::AnsiToOem(sz,sz)
#define COPY_TO_ANSI(s,d)   ::OemToAnsi(s,d)
#define COPY_TO_OEM(s,d)    ::AnsiToOem(s,d)

#endif // DEBUG


// Debug mask APIs

// NOTE: You can #define your own DM_* values using bits in the HI BYTE

#define DM_TRACE    0x0001      // Trace messages
#define DM_WARNING  0x0002      // Warning
#define DM_ERROR    0x0004      // Error
#define DM_ASSERT   0x0008      // Assertions

#define	DM_LOG_FILE 0x0100
#define	DM_PREFIX 	0x0200


#if !defined(NetDebugMsg)

//
// DebugMsg(mask, msg, args...) - Generate wsprintf-formatted msg using
//                          specified debug mask.  System debug mask
//                          governs whether message is output.
//

#if defined(__cplusplus)
extern "C"
{
#else
extern
#endif

#define REGVAL_STR_DEBUGMASK	"DebugMask"

void __cdecl NetDebugMsg(UINT mask, LPCSTR psz, ...);

UINT WINAPI  NetSetDebugParameters(PSTR pszName,PSTR pszLogFile);
UINT WINAPI  NetSetDebugMask(UINT mask);
UINT WINAPI  NetGetDebugMask(void);

#if defined(__cplusplus)
}
#endif

#endif

#ifdef	DEBUG

#define Break() 		{_asm _emit 0xcc}
//#define	Trap()			{_asm {_emit 0xcc}}
//#define	TrapC(c)		{if(c) {Trap()}}

#define DPRINTF  NetDebugMsg

#else

#define Break()
#define	Trap()
#define	TrapC(c)

// Nb: Following definition is needed to avoid compiler complaining
// about empty function name in expression. In retail builds using this macro
// will cause string parameters not appear in executable
#define DPRINTF 	1?(void)0 : (void)

#endif

#endif // _NPASSERT_H_
