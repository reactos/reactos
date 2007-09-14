#ifndef __IMM_H
#define __IMM_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _COMPOSITIONSTRING
{
    DWORD dwSize;
    DWORD dwCompReadAttrLen;
    DWORD dwCompReadAttrOffset;
    DWORD dwCompReadClauseLen;
    DWORD dwCompReadClauseOffset;
    DWORD dwCompReadStrLen;
    DWORD dwCompReadStrOffset;
    DWORD dwCompAttrLen;
    DWORD dwCompAttrOffset;
    DWORD dwCompClauseLen;
    DWORD dwCompClauseOffset;
    DWORD dwCompStrLen;
    DWORD dwCompStrOffset;
    DWORD dwCursorPos;
    DWORD dwDeltaStart;
    DWORD dwResultReadClauseLen;
    DWORD dwResultReadClauseOffset;
    DWORD dwResultReadStrLen;
    DWORD dwResultReadStrOffset;
    DWORD dwResultClauseLen;
    DWORD dwResultClauseOffset;
    DWORD dwResultStrLen;
    DWORD dwResultStrOffset;
    DWORD dwPrivateSize;
    DWORD dwPrivateOffset;
} COMPOSITIONSTRING, *LPCOMPOSITIONSTRING;

typedef struct _INPUTCONTEXT
{
    HWND hWnd;
    BOOL fOpen;
    HWND hwndImeInUse;
    POINT ptStatusWndPos;
    POINT ptSoftKbdPos;
    DWORD fdwConversion;
    DWORD fdwSentence;
    union
    {
        LOGFONTA A;
        LOGFONTW W;
    } lfFont;
    COMPOSITIONFORM cfCompForm;
    CANDIDATEFORM cfCandForm[4];
    HIMCC hCompStr;
    HIMCC hCandInfo;
    HIMCC hGuideLine;
    HIMCC hPrivate;
    DWORD dwNumMsgBuf;
    HIMCC hMsgBuf;
    DWORD fdwInit;
    DWORD dwReserve[3]
} INPUTCONTEXT, *PINPUTCONTEXT, *LPINPUTCONTEXT;

HIMCC WINAPI ImmCreateIMCC(DWORD dwSize);
HIMCC WINAPI ImmDestroyIMCC(HIMCC hIMCC);
LPVOID WINAPI ImmLockIMCC(HIMCC hIMCC);
BOOL WINAPI ImmUnlockIMCC(HIMCC hIMCC);

#ifdef __cplusplus
}
#endif

#endif /* __IMM_H */
