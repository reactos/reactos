
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
    HWND                hWnd;
    BOOL                fOpen;
    POINT               ptStatusWndPos;
    POINT               ptSoftKbdPos;
    DWORD               fdwConversion;
    DWORD               fdwSentence;
    union   {
        LOGFONTA        A;
        LOGFONTW        W;
    } lfFont;
    COMPOSITIONFORM     cfCompForm;
    CANDIDATEFORM       cfCandForm[4];
    HIMCC               hCompStr;
    HIMCC               hCandInfo;
    HIMCC               hGuideLine;
    HIMCC               hPrivate;
    DWORD               dwNumMsgBuf;
    HIMCC               hMsgBuf;
    DWORD               fdwInit;
    DWORD               dwReserve[3];
} INPUTCONTEXT, *LPINPUTCONTEXT;

#ifdef _WIN64
C_ASSERT(offsetof(INPUTCONTEXT, hWnd) == 0x0);
C_ASSERT(offsetof(INPUTCONTEXT, fOpen) == 0x8);
C_ASSERT(offsetof(INPUTCONTEXT, ptStatusWndPos) == 0xc);
C_ASSERT(offsetof(INPUTCONTEXT, ptSoftKbdPos) == 0x14);
C_ASSERT(offsetof(INPUTCONTEXT, fdwConversion) == 0x1c);
C_ASSERT(offsetof(INPUTCONTEXT, fdwSentence) == 0x20);
C_ASSERT(offsetof(INPUTCONTEXT, lfFont) == 0x24);
C_ASSERT(offsetof(INPUTCONTEXT, cfCompForm) == 0x80);
C_ASSERT(offsetof(INPUTCONTEXT, cfCandForm) == 0x9c);
C_ASSERT(offsetof(INPUTCONTEXT, hCompStr) == 0x120);
C_ASSERT(offsetof(INPUTCONTEXT, hCandInfo) == 0x128);
C_ASSERT(offsetof(INPUTCONTEXT, hGuideLine) == 0x130);
C_ASSERT(offsetof(INPUTCONTEXT, hPrivate) == 0x138);
C_ASSERT(offsetof(INPUTCONTEXT, dwNumMsgBuf) == 0x140);
C_ASSERT(offsetof(INPUTCONTEXT, hMsgBuf) == 0x148);
C_ASSERT(offsetof(INPUTCONTEXT, fdwInit) == 0x150);
C_ASSERT(offsetof(INPUTCONTEXT, dwReserve) == 0x154);
C_ASSERT(sizeof(INPUTCONTEXT) == 0x160);
#else
C_ASSERT(offsetof(INPUTCONTEXT, hWnd) == 0x0);
C_ASSERT(offsetof(INPUTCONTEXT, fOpen) == 0x4);
C_ASSERT(offsetof(INPUTCONTEXT, ptStatusWndPos) == 0x8);
C_ASSERT(offsetof(INPUTCONTEXT, ptSoftKbdPos) == 0x10);
C_ASSERT(offsetof(INPUTCONTEXT, fdwConversion) == 0x18);
C_ASSERT(offsetof(INPUTCONTEXT, fdwSentence) == 0x1c);
C_ASSERT(offsetof(INPUTCONTEXT, lfFont) == 0x20);
C_ASSERT(offsetof(INPUTCONTEXT, cfCompForm) == 0x7c);
C_ASSERT(offsetof(INPUTCONTEXT, cfCandForm) == 0x98);
C_ASSERT(offsetof(INPUTCONTEXT, hCompStr) == 0x118);
C_ASSERT(offsetof(INPUTCONTEXT, hCandInfo) == 0x11c);
C_ASSERT(offsetof(INPUTCONTEXT, hGuideLine) == 0x120);
C_ASSERT(offsetof(INPUTCONTEXT, hPrivate) == 0x124);
C_ASSERT(offsetof(INPUTCONTEXT, dwNumMsgBuf) == 0x128);
C_ASSERT(offsetof(INPUTCONTEXT, hMsgBuf) == 0x12c);
C_ASSERT(offsetof(INPUTCONTEXT, fdwInit) == 0x130);
C_ASSERT(offsetof(INPUTCONTEXT, dwReserve) == 0x134);
C_ASSERT(sizeof(INPUTCONTEXT) == 0x140);
#endif

LPINPUTCONTEXT WINAPI ImmLockIMC(HIMC);

#endif /* _WINE_IMM_H_ */
