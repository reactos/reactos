
#ifndef _WINE_IMM_H_
#define _WINE_IMM_H_

#include <wingdi.h>

#ifdef WINE_NO_UNICODE_MACROS
# define WINELIB_NAME_AW(func) \
    func##_must_be_suffixed_with_W_or_A_in_this_context \
    func##_must_be_suffixed_with_W_or_A_in_this_context
#else  /* WINE_NO_UNICODE_MACROS */
# ifdef UNICODE
#  define WINELIB_NAME_AW(func) func##W
# else
#  define WINELIB_NAME_AW(func) func##A
# endif
#endif  /* WINE_NO_UNICODE_MACROS */

#ifdef WINE_NO_UNICODE_MACROS
# define DECL_WINELIB_TYPE_AW(type)  /* nothing */
#else
# define DECL_WINELIB_TYPE_AW(type)  typedef WINELIB_NAME_AW(type) type;
#endif

#include <psdk/imm.h>

typedef struct _tagINPUTCONTEXT {
    HWND                hWnd;               /* offset 0x0 */
    BOOL                fOpen;              /* offset 0x4 */
    POINT               ptStatusWndPos;     /* offset 0x8 */
    POINT               ptSoftKbdPos;       /* offset 0x10 */
    DWORD               fdwConversion;      /* offset 0x18 */
    DWORD               fdwSentence;        /* offset 0x1c */
    union   {
        LOGFONTA        A;
        LOGFONTW        W;
    } lfFont;                               /* offset 0x20 */
    COMPOSITIONFORM     cfCompForm;         /* offset 0x7c */
    CANDIDATEFORM       cfCandForm[4];      /* offset 0x98 */
    HIMCC               hCompStr;           /* offset 0x118 */
    HIMCC               hCandInfo;          /* offset 0x11c */
    HIMCC               hGuideLine;         /* offset 0x120 */
    HIMCC               hPrivate;           /* offset 0x124 */
    DWORD               dwNumMsgBuf;        /* offset 0x128 */
    HIMCC               hMsgBuf;            /* offset 0x12c */
    DWORD               fdwInit;            /* offset 0x130 */
    DWORD               dwReserve[3];       /* offset 0x134 */
} INPUTCONTEXT, *LPINPUTCONTEXT;            /* size 0x140 */

LPINPUTCONTEXT WINAPI ImmLockIMC(HIMC);

#endif /* _WINE_IMM_H_ */
