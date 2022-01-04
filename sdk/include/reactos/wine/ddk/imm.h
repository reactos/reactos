
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

struct IME_STATE;

typedef struct INPUTCONTEXTDX /* unconfirmed */
{
    INPUTCONTEXT;
    UINT nVKey;
    BOOL bNeedsTrans;
    DWORD dwUnknown1;
    DWORD dwUIFlags;
    DWORD dwUnknown2;
    struct IME_STATE *pState;
    DWORD dwChange;
    DWORD dwUnknown5;
} INPUTCONTEXTDX, *LPINPUTCONTEXTDX;

#ifndef _WIN64
C_ASSERT(offsetof(INPUTCONTEXTDX, nVKey) == 0x140);
C_ASSERT(offsetof(INPUTCONTEXTDX, bNeedsTrans) == 0x144);
C_ASSERT(offsetof(INPUTCONTEXTDX, dwUIFlags) == 0x14c);
C_ASSERT(offsetof(INPUTCONTEXTDX, pState) == 0x154);
C_ASSERT(offsetof(INPUTCONTEXTDX, dwChange) == 0x158);
C_ASSERT(sizeof(INPUTCONTEXTDX) == 0x160);
#endif

// bits of fdwInit of INPUTCONTEXT
#define INIT_STATUSWNDPOS               0x00000001
#define INIT_CONVERSION                 0x00000002
#define INIT_SENTENCE                   0x00000004
#define INIT_LOGFONT                    0x00000008
#define INIT_COMPFORM                   0x00000010
#define INIT_SOFTKBDPOS                 0x00000020

// bits for INPUTCONTEXTDX.dwChange
#define INPUTCONTEXTDX_CHANGE_OPEN          0x1
#define INPUTCONTEXTDX_CHANGE_CONVERSION    0x2
#define INPUTCONTEXTDX_CHANGE_SENTENCE      0x4
#define INPUTCONTEXTDX_CHANGE_FORCE_OPEN    0x100

#ifndef WM_IME_REPORT
    #define WM_IME_REPORT 0x280
#endif

// WM_IME_REPORT wParam
#define IR_STRINGSTART   0x100
#define IR_STRINGEND     0x101
#define IR_OPENCONVERT   0x120
#define IR_CHANGECONVERT 0x121
#define IR_CLOSECONVERT  0x122
#define IR_FULLCONVERT   0x123
#define IR_IMESELECT     0x130
#define IR_STRING        0x140
#define IR_DBCSCHAR      0x160
#define IR_UNDETERMINE   0x170
#define IR_STRINGEX      0x180
#define IR_MODEINFO      0x190

// for IR_UNDETERMINE
typedef struct tagUNDETERMINESTRUCT
{
    DWORD dwSize;
    UINT  uDefIMESize;
    UINT  uDefIMEPos;
    UINT  uUndetTextLen;
    UINT  uUndetTextPos;
    UINT  uUndetAttrPos;
    UINT  uCursorPos;
    UINT  uDeltaStart;
    UINT  uDetermineTextLen;
    UINT  uDetermineTextPos;
    UINT  uDetermineDelimPos;
    UINT  uYomiTextLen;
    UINT  uYomiTextPos;
    UINT  uYomiDelimPos;
} UNDETERMINESTRUCT, *PUNDETERMINESTRUCT, *LPUNDETERMINESTRUCT;

LPINPUTCONTEXT WINAPI ImmLockIMC(HIMC);

typedef struct IME_SUBSTATE
{
    struct IME_SUBSTATE *pNext;
    HKL hKL;
    DWORD dwValue;
} IME_SUBSTATE, *PIME_SUBSTATE;

#ifndef _WIN64
C_ASSERT(sizeof(IME_SUBSTATE) == 0xc);
#endif

typedef struct IME_STATE
{
    struct IME_STATE *pNext;
    WORD wLang;
    WORD fOpen;
    DWORD dwConversion;
    DWORD dwSentence;
    DWORD dwInit;
    PIME_SUBSTATE pSubState;
} IME_STATE, *PIME_STATE;

#ifndef _WIN64
C_ASSERT(sizeof(IME_STATE) == 0x18);
#endif

#endif /* _WINE_IMM_H_ */
