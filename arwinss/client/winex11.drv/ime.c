/*
 * The IME for interfacing with XIM
 *
 * Copyright 2008 CodeWeavers, Aric Stewart
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * Notes:
 *  The normal flow for IMM/IME Processing is as follows.
 * 1) The Keyboard Driver generates key messages which are first passed to
 *    the IMM and then to IME via ImeProcessKey. If the IME returns 0  then
 *    it does not want the key and the keyboard driver then generates the
 *    WM_KEYUP/WM_KEYDOWN messages.  However if the IME is going to process the
 *    key it returns non-zero.
 * 2) If the IME is going to process the key then the IMM calls ImeToAsciiEx to
 *    process the key.  the IME modifies the HIMC structure to reflect the
 *    current state and generates any messages it needs the IMM to process.
 * 3) IMM checks the messages and send them to the application in question. From
 *    here the IMM level deals with if the application is IME aware or not.
 *
 *  This flow does not work well for the X11 driver and XIM.
 *   (It works fine for Mac)
 *  As such we will have to reroute step 1.  Instead the x11drv driver will
 *  generate an XIM events and call directly into this IME implimenetaion.
 *  As such we will have to use the alternative ImmGenerateMessage path to be
 *  generate the messages that we want the IMM layer to send to the application.
 */

#include "config.h"

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "wine/debug.h"
#include "imm.h"
#include "ddk/imm.h"
#include "winnls.h"
#include "x11drv.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

#define FROM_X11 ((HIMC)0xcafe1337)

typedef struct _IMEPRIVATE {
    BOOL bInComposition;
    BOOL bInternalState;
    HFONT textfont;
    HWND hwndDefault;
} IMEPRIVATE, *LPIMEPRIVATE;

typedef struct _tagTRANSMSG {
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
} TRANSMSG, *LPTRANSMSG;

static const WCHAR UI_CLASS_NAME[] = {'W','i','n','e','X','1','1','I','M','E',0};

static HIMC *hSelectedFrom = NULL;
static INT  hSelectedCount = 0;

/* MSIME messages */
static UINT WM_MSIME_SERVICE;
static UINT WM_MSIME_RECONVERTOPTIONS;
static UINT WM_MSIME_MOUSE;
static UINT WM_MSIME_RECONVERTREQUEST;
static UINT WM_MSIME_RECONVERT;
static UINT WM_MSIME_QUERYPOSITION;
static UINT WM_MSIME_DOCUMENTFEED;

static LRESULT WINAPI IME_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                          LPARAM lParam);

static HIMC RealIMC(HIMC hIMC)
{
    if (hIMC == FROM_X11)
    {
        INT i;
        HWND wnd = GetFocus();
        HIMC winHimc = ImmGetContext(wnd);
        for (i = 0; i < hSelectedCount; i++)
            if (winHimc == hSelectedFrom[i])
                return winHimc;
        return NULL;
    }
    else
        return hIMC;
}

static LPINPUTCONTEXT LockRealIMC(HIMC hIMC)
{
    HIMC real_imc = RealIMC(hIMC);
    if (real_imc)
        return ImmLockIMC(real_imc);
    else
        return NULL;
}

static BOOL UnlockRealIMC(HIMC hIMC)
{
    HIMC real_imc = RealIMC(hIMC);
    if (real_imc)
        return ImmUnlockIMC(real_imc);
    else
        return FALSE;
}

static void IME_RegisterClasses(void)
{
    static int done;
    WNDCLASSW wndClass;

    if (done) return;
    done = 1;

    ZeroMemory(&wndClass, sizeof(WNDCLASSW));
    wndClass.style = CS_GLOBALCLASS | CS_IME | CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = IME_WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 2 * sizeof(LONG_PTR);
    wndClass.hInstance = x11drv_module;
    wndClass.hCursor = LoadCursorW(NULL, (LPWSTR)IDC_ARROW);
    wndClass.hIcon = LoadIconW(NULL, (LPWSTR)IDI_APPLICATION);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW +1);
    wndClass.lpszMenuName   = 0;
    wndClass.lpszClassName = UI_CLASS_NAME;

    RegisterClassW(&wndClass);

    WM_MSIME_SERVICE = RegisterWindowMessageA("MSIMEService");
    WM_MSIME_RECONVERTOPTIONS = RegisterWindowMessageA("MSIMEReconvertOptions");
    WM_MSIME_MOUSE = RegisterWindowMessageA("MSIMEMouseOperation");
    WM_MSIME_RECONVERTREQUEST = RegisterWindowMessageA("MSIMEReconvertRequest");
    WM_MSIME_RECONVERT = RegisterWindowMessageA("MSIMEReconvert");
    WM_MSIME_QUERYPOSITION = RegisterWindowMessageA("MSIMEQueryPosition");
    WM_MSIME_DOCUMENTFEED = RegisterWindowMessageA("MSIMEDocumentFeed");
}

void IME_UnregisterClasses(void)
{
    UnregisterClassW(UI_CLASS_NAME, x11drv_module);
}

static HIMCC ImeCreateBlankCompStr(void)
{
    HIMCC rc;
    LPCOMPOSITIONSTRING ptr;
    rc = ImmCreateIMCC(sizeof(COMPOSITIONSTRING));
    ptr = ImmLockIMCC(rc);
    memset(ptr,0,sizeof(COMPOSITIONSTRING));
    ptr->dwSize = sizeof(COMPOSITIONSTRING);
    ImmUnlockIMCC(rc);
    return rc;
}

static int updateField(DWORD origLen, DWORD origOffset, DWORD currentOffset,
                       LPBYTE target, LPBYTE source, DWORD* lenParam,
                       DWORD* offsetParam, BOOL wchars )
{
     if (origLen > 0 && origOffset > 0)
     {
        int truelen = origLen;
        if (wchars)
            truelen *= sizeof(WCHAR);

        memcpy(&target[currentOffset], &source[origOffset], truelen);

        *lenParam = origLen;
        *offsetParam = currentOffset;
        currentOffset += truelen;
     }
     return currentOffset;
}

static HIMCC updateCompStr(HIMCC old, LPCWSTR compstr, DWORD len)
{
    /* we need to make sure the CompStr, CompClaus and CompAttr fields are all
     * set and correct */
    int needed_size;
    HIMCC   rc;
    LPBYTE newdata = NULL;
    LPBYTE olddata = NULL;
    LPCOMPOSITIONSTRING new_one;
    LPCOMPOSITIONSTRING lpcs = NULL;
    INT current_offset = 0;

    TRACE("%s, %i\n",debugstr_wn(compstr,len),len);

    if (old == NULL && compstr == NULL && len == 0)
        return NULL;

    if (compstr == NULL && len != 0)
    {
        ERR("compstr is NULL however we have a len!  Please report\n");
        len = 0;
    }

    if (old != NULL)
    {
        olddata = ImmLockIMCC(old);
        lpcs = (LPCOMPOSITIONSTRING)olddata;
    }

    needed_size = sizeof(COMPOSITIONSTRING) + len * sizeof(WCHAR) +
                  len + sizeof(DWORD) * 2;

    if (lpcs != NULL)
    {
        needed_size += lpcs->dwCompReadAttrLen;
        needed_size += lpcs->dwCompReadClauseLen;
        needed_size += lpcs->dwCompReadStrLen * sizeof(DWORD);
        needed_size += lpcs->dwResultReadClauseLen;
        needed_size += lpcs->dwResultReadStrLen * sizeof(DWORD);
        needed_size += lpcs->dwResultClauseLen;
        needed_size += lpcs->dwResultStrLen * sizeof(DWORD);
        needed_size += lpcs->dwPrivateSize;
    }
    rc = ImmCreateIMCC(needed_size);
    newdata = ImmLockIMCC(rc);
    new_one = (LPCOMPOSITIONSTRING)newdata;

    new_one->dwSize = needed_size;
    current_offset = sizeof(COMPOSITIONSTRING);
    if (lpcs != NULL)
    {
        current_offset = updateField(lpcs->dwCompReadAttrLen,
                                     lpcs->dwCompReadAttrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompReadAttrLen,
                                     &new_one->dwCompReadAttrOffset, FALSE);

        current_offset = updateField(lpcs->dwCompReadClauseLen,
                                     lpcs->dwCompReadClauseOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompReadClauseLen,
                                     &new_one->dwCompReadClauseOffset, FALSE);

        current_offset = updateField(lpcs->dwCompReadStrLen,
                                     lpcs->dwCompReadStrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompReadStrLen,
                                     &new_one->dwCompReadStrOffset, TRUE);

        /* new CompAttr, CompClause, CompStr, dwCursorPos */
        new_one->dwDeltaStart = 0;

        current_offset = updateField(lpcs->dwResultReadClauseLen,
                                     lpcs->dwResultReadClauseOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwResultReadClauseLen,
                                     &new_one->dwResultReadClauseOffset, FALSE);

        current_offset = updateField(lpcs->dwResultReadStrLen,
                                     lpcs->dwResultReadStrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwResultReadStrLen,
                                     &new_one->dwResultReadStrOffset, TRUE);

        current_offset = updateField(lpcs->dwResultClauseLen,
                                     lpcs->dwResultClauseOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwResultClauseLen,
                                     &new_one->dwResultClauseOffset, FALSE);

        current_offset = updateField(lpcs->dwResultStrLen,
                                     lpcs->dwResultStrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwResultStrLen,
                                     &new_one->dwResultStrOffset, TRUE);

        current_offset = updateField(lpcs->dwPrivateSize,
                                     lpcs->dwPrivateOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwPrivateSize,
                                     &new_one->dwPrivateOffset, FALSE);
    }

    /* set new data */
    /* CompAttr */
    new_one->dwCompAttrLen = len;
    if (len > 0)
    {
        new_one->dwCompAttrOffset = current_offset;
        memset(&newdata[current_offset],ATTR_INPUT,len);
        current_offset += len;
    }

    /* CompClause */
    if (len > 0)
    {
        new_one->dwCompClauseLen = sizeof(DWORD) * 2;
        new_one->dwCompClauseOffset = current_offset;
        *(DWORD*)(&newdata[current_offset]) = 0;
        current_offset += sizeof(DWORD);
        *(DWORD*)(&newdata[current_offset]) = len;
        current_offset += sizeof(DWORD);
    }

    /* CompStr */
    new_one->dwCompStrLen = len;
    if (len > 0)
    {
        new_one->dwCompStrOffset = current_offset;
        memcpy(&newdata[current_offset],compstr,len*sizeof(WCHAR));
    }

    /* CursorPos */
    new_one->dwCursorPos = len;

    ImmUnlockIMCC(rc);
    if (lpcs)
        ImmUnlockIMCC(old);

    return rc;
}

static HIMCC updateResultStr(HIMCC old, LPWSTR resultstr, DWORD len)
{
    /* we need to make sure the ResultStr and ResultClause fields are all
     * set and correct */
    int needed_size;
    HIMCC   rc;
    LPBYTE newdata = NULL;
    LPBYTE olddata = NULL;
    LPCOMPOSITIONSTRING new_one;
    LPCOMPOSITIONSTRING lpcs = NULL;
    INT current_offset = 0;

    TRACE("%s, %i\n",debugstr_wn(resultstr,len),len);

    if (old == NULL && resultstr == NULL && len == 0)
        return NULL;

    if (resultstr == NULL && len != 0)
    {
        ERR("resultstr is NULL however we have a len!  Please report\n");
        len = 0;
    }

    if (old != NULL)
    {
        olddata = ImmLockIMCC(old);
        lpcs = (LPCOMPOSITIONSTRING)olddata;
    }

    needed_size = sizeof(COMPOSITIONSTRING) + len * sizeof(WCHAR) +
                  sizeof(DWORD) * 2;

    if (lpcs != NULL)
    {
        needed_size += lpcs->dwCompReadAttrLen;
        needed_size += lpcs->dwCompReadClauseLen;
        needed_size += lpcs->dwCompReadStrLen * sizeof(DWORD);
        needed_size += lpcs->dwCompAttrLen;
        needed_size += lpcs->dwCompClauseLen;
        needed_size += lpcs->dwCompStrLen * sizeof(DWORD);
        needed_size += lpcs->dwResultReadClauseLen;
        needed_size += lpcs->dwResultReadStrLen * sizeof(DWORD);
        needed_size += lpcs->dwPrivateSize;
    }
    rc = ImmCreateIMCC(needed_size);
    newdata = ImmLockIMCC(rc);
    new_one = (LPCOMPOSITIONSTRING)newdata;

    new_one->dwSize = needed_size;
    current_offset = sizeof(COMPOSITIONSTRING);
    if (lpcs != NULL)
    {
        current_offset = updateField(lpcs->dwCompReadAttrLen,
                                     lpcs->dwCompReadAttrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompReadAttrLen,
                                     &new_one->dwCompReadAttrOffset, FALSE);

        current_offset = updateField(lpcs->dwCompReadClauseLen,
                                     lpcs->dwCompReadClauseOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompReadClauseLen,
                                     &new_one->dwCompReadClauseOffset, FALSE);

        current_offset = updateField(lpcs->dwCompReadStrLen,
                                     lpcs->dwCompReadStrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompReadStrLen,
                                     &new_one->dwCompReadStrOffset, TRUE);

        current_offset = updateField(lpcs->dwCompAttrLen,
                                     lpcs->dwCompAttrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompAttrLen,
                                     &new_one->dwCompAttrOffset, FALSE);

        current_offset = updateField(lpcs->dwCompClauseLen,
                                     lpcs->dwCompClauseOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompClauseLen,
                                     &new_one->dwCompClauseOffset, FALSE);

        current_offset = updateField(lpcs->dwCompStrLen,
                                     lpcs->dwCompStrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompStrLen,
                                     &new_one->dwCompStrOffset, TRUE);

        new_one->dwCursorPos = lpcs->dwCursorPos;
        new_one->dwDeltaStart = 0;

        current_offset = updateField(lpcs->dwResultReadClauseLen,
                                     lpcs->dwResultReadClauseOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwResultReadClauseLen,
                                     &new_one->dwResultReadClauseOffset, FALSE);

        current_offset = updateField(lpcs->dwResultReadStrLen,
                                     lpcs->dwResultReadStrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwResultReadStrLen,
                                     &new_one->dwResultReadStrOffset, TRUE);

        /* new ResultClause , ResultStr */

        current_offset = updateField(lpcs->dwPrivateSize,
                                     lpcs->dwPrivateOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwPrivateSize,
                                     &new_one->dwPrivateOffset, FALSE);
    }

    /* set new data */
    /* ResultClause */
    if (len > 0)
    {
        new_one->dwResultClauseLen = sizeof(DWORD) * 2;
        new_one->dwResultClauseOffset = current_offset;
        *(DWORD*)(&newdata[current_offset]) = 0;
        current_offset += sizeof(DWORD);
        *(DWORD*)(&newdata[current_offset]) = len;
        current_offset += sizeof(DWORD);
    }

    /* ResultStr */
    new_one->dwResultStrLen = len;
    if (len > 0)
    {
        new_one->dwResultStrOffset = current_offset;
        memcpy(&newdata[current_offset],resultstr,len*sizeof(WCHAR));
    }
    ImmUnlockIMCC(rc);
    if (lpcs)
        ImmUnlockIMCC(old);

    return rc;
}

static void GenerateIMEMessage(HIMC hIMC, UINT msg, WPARAM wParam,
                               LPARAM lParam)
{
    LPINPUTCONTEXT lpIMC;
    LPTRANSMSG lpTransMsg;

    lpIMC = LockRealIMC(hIMC);
    if (lpIMC == NULL)
        return;

    lpIMC->hMsgBuf = ImmReSizeIMCC(lpIMC->hMsgBuf, (lpIMC->dwNumMsgBuf + 1) *
                                    sizeof(TRANSMSG));
    if (!lpIMC->hMsgBuf)
        return;

    lpTransMsg = ImmLockIMCC(lpIMC->hMsgBuf);
    if (!lpTransMsg)
        return;

    lpTransMsg += lpIMC->dwNumMsgBuf;
    lpTransMsg->message = msg;
    lpTransMsg->wParam = wParam;
    lpTransMsg->lParam = lParam;

    ImmUnlockIMCC(lpIMC->hMsgBuf);
    lpIMC->dwNumMsgBuf++;

    ImmGenerateMessage(RealIMC(hIMC));
    UnlockRealIMC(hIMC);
}

static void GenerateIMECHARMessages(HIMC hIMC, LPWSTR String, DWORD length)
{
    LPINPUTCONTEXT lpIMC;
    LPTRANSMSG lpTransMsg;
    DWORD i;

    if (length <= 0)
        return;

    lpIMC = LockRealIMC(hIMC);
    if (lpIMC == NULL)
        return;

    lpIMC->hMsgBuf = ImmReSizeIMCC(lpIMC->hMsgBuf,
                                  (lpIMC->dwNumMsgBuf + length) *
                                    sizeof(TRANSMSG));
    if (!lpIMC->hMsgBuf)
        return;

    lpTransMsg = ImmLockIMCC(lpIMC->hMsgBuf);
    if (!lpTransMsg)
        return;

    lpTransMsg += lpIMC->dwNumMsgBuf;
    for (i = 0; i < length; i++)
    {
        lpTransMsg->message = WM_IME_CHAR;
        lpTransMsg->wParam = String[i];
        lpTransMsg->lParam = 1;
        lpTransMsg ++;
    }

    ImmUnlockIMCC(lpIMC->hMsgBuf);
    lpIMC->dwNumMsgBuf+=length;

    ImmGenerateMessage(RealIMC(hIMC));
    UnlockRealIMC(hIMC);
}

static BOOL IME_RemoveFromSelected(HIMC hIMC)
{
    int i;
    for (i = 0; i < hSelectedCount; i++)
        if (hSelectedFrom[i] == hIMC)
        {
            if (i < hSelectedCount - 1)
                memmove(&hSelectedFrom[i], &hSelectedFrom[i+1], (hSelectedCount - i - 1)*sizeof(HIMC));
            hSelectedCount --;
            return TRUE;
        }
    return FALSE;
}

static void IME_AddToSelected(HIMC hIMC)
{
    hSelectedCount++;
    if (hSelectedFrom)
        hSelectedFrom = HeapReAlloc(GetProcessHeap(), 0, hSelectedFrom, hSelectedCount*sizeof(HIMC));
    else
        hSelectedFrom = HeapAlloc(GetProcessHeap(), 0, sizeof(HIMC));
    hSelectedFrom[hSelectedCount-1] = hIMC;
}

BOOL WINAPI ImeInquire(LPIMEINFO lpIMEInfo, LPWSTR lpszUIClass,
                       LPCWSTR lpszOption)
{
    TRACE("\n");
    IME_RegisterClasses();
    lpIMEInfo->dwPrivateDataSize = sizeof (IMEPRIVATE);
    lpIMEInfo->fdwProperty = IME_PROP_UNICODE | IME_PROP_AT_CARET;
    lpIMEInfo->fdwConversionCaps = IME_CMODE_NATIVE;
    lpIMEInfo->fdwSentenceCaps = IME_SMODE_AUTOMATIC;
    lpIMEInfo->fdwUICaps = UI_CAP_2700;
    /* Tell App we cannot accept ImeSetCompositionString calls */
    lpIMEInfo->fdwSCSCaps = 0;
    lpIMEInfo->fdwSelectCaps = SELECT_CAP_CONVERSION;

    lstrcpyW(lpszUIClass,UI_CLASS_NAME);

    return TRUE;
}

BOOL WINAPI ImeConfigure(HKL hKL,HWND hWnd, DWORD dwMode, LPVOID lpData)
{
    FIXME("(%p, %p, %d, %p): stub\n", hKL, hWnd, dwMode, lpData);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

DWORD WINAPI ImeConversionList(HIMC hIMC, LPCWSTR lpSource,
                LPCANDIDATELIST lpCandList, DWORD dwBufLen, UINT uFlag)

{
    FIXME("(%p, %s, %p, %d, %d): stub\n", hIMC, debugstr_w(lpSource),
                                          lpCandList, dwBufLen, uFlag);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

BOOL WINAPI ImeDestroy(UINT uForce)
{
    TRACE("\n");
    HeapFree(GetProcessHeap(),0,hSelectedFrom);
    hSelectedFrom = NULL;
    hSelectedCount = 0;
    return TRUE;
}

LRESULT WINAPI ImeEscape(HIMC hIMC, UINT uSubFunc, LPVOID lpData)
{
    FIXME("(%p, %d, %p): stub\n", hIMC, uSubFunc, lpData);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

BOOL WINAPI ImeProcessKey(HIMC hIMC, UINT vKey, LPARAM lKeyData,
                             CONST LPBYTE lpbKeyState)
{
    /* See the comment at the head of this file */
    TRACE("We do no processing via this route\n");
    return FALSE;
}

BOOL WINAPI ImeSelect(HIMC hIMC, BOOL fSelect)
{
    LPINPUTCONTEXT lpIMC;
    TRACE("%p %s\n",hIMC,(fSelect)?"TRUE":"FALSE");

    if (hIMC == FROM_X11)
    {
        ERR("ImeSelect should never be called from X11\n");
        return FALSE;
    }

    if (!hIMC)
        return TRUE;

    /* not selected */
    if (!fSelect)
        return IME_RemoveFromSelected(hIMC);

    IME_AddToSelected(hIMC);

    /* Initialize our structures */
    lpIMC = LockRealIMC(hIMC);
    if (lpIMC != NULL)
    {
        LPIMEPRIVATE myPrivate;
        myPrivate = ImmLockIMCC(lpIMC->hPrivate);
        myPrivate->bInComposition = FALSE;
        myPrivate->bInternalState = FALSE;
        myPrivate->textfont = NULL;
        myPrivate->hwndDefault = NULL;
        ImmUnlockIMCC(lpIMC->hPrivate);
        UnlockRealIMC(hIMC);
    }

    return TRUE;
}

BOOL WINAPI ImeSetActiveContext(HIMC hIMC,BOOL fFlag)
{
    FIXME("(%p, %x): stub\n", hIMC, fFlag);
    return TRUE;
}

UINT WINAPI ImeToAsciiEx (UINT uVKey, UINT uScanCode,
                          CONST LPBYTE lpbKeyState, LPDWORD lpdwTransKey,
                          UINT fuState, HIMC hIMC)
{
    /* See the comment at the head of this file */
    TRACE("We do no processing via this route\n");
    return 0;
}

BOOL WINAPI NotifyIME(HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
    BOOL bRet = FALSE;
    LPINPUTCONTEXT lpIMC;

    TRACE("%p %i %i %i\n",hIMC,dwAction,dwIndex,dwValue);

    lpIMC = LockRealIMC(hIMC);
    if (lpIMC == NULL)
        return FALSE;

    switch (dwAction)
    {
        case NI_OPENCANDIDATE: FIXME("NI_OPENCANDIDATE\n"); break;
        case NI_CLOSECANDIDATE: FIXME("NI_CLOSECANDIDATE\n"); break;
        case NI_SELECTCANDIDATESTR: FIXME("NI_SELECTCANDIDATESTR\n"); break;
        case NI_CHANGECANDIDATELIST: FIXME("NI_CHANGECANDIDATELIST\n"); break;
        case NI_SETCANDIDATE_PAGESTART: FIXME("NI_SETCANDIDATE_PAGESTART\n"); break;
        case NI_SETCANDIDATE_PAGESIZE: FIXME("NI_SETCANDIDATE_PAGESIZE\n"); break;
        case NI_CONTEXTUPDATED:
            switch (dwValue)
            {
                case IMC_SETCOMPOSITIONWINDOW: FIXME("IMC_SETCOMPOSITIONWINDOW\n"); break;
                case IMC_SETCONVERSIONMODE: FIXME("IMC_SETCONVERSIONMODE\n"); break;
                case IMC_SETSENTENCEMODE: FIXME("IMC_SETSENTENCEMODE\n"); break;
                case IMC_SETCANDIDATEPOS: FIXME("IMC_SETCANDIDATEPOS\n"); break;
                case IMC_SETCOMPOSITIONFONT:
                    {
                        LPIMEPRIVATE myPrivate;
                        TRACE("IMC_SETCOMPOSITIONFONT\n");

                        myPrivate = ImmLockIMCC(lpIMC->hPrivate);
                        if (myPrivate->textfont)
                        {
                            DeleteObject(myPrivate->textfont);
                            myPrivate->textfont = NULL;
                        }
                        myPrivate->textfont = CreateFontIndirectW(&lpIMC->lfFont.W);
                        ImmUnlockIMCC(lpIMC->hPrivate);
                    }
                    break;
                case IMC_SETOPENSTATUS:
                    TRACE("IMC_SETOPENSTATUS\n");

                    /* Indirectly called from XIM callbacks */
                    if (ImmGetIMCCLockCount(lpIMC->hPrivate) > 0)
                    {
                        bRet = TRUE;
                        break;
                    }

                    bRet = X11DRV_SetPreeditState(lpIMC->hWnd, lpIMC->fOpen);
                    if (bRet)
                    {
                        if (!lpIMC->fOpen)
                        {
                            LPIMEPRIVATE myPrivate;

                            myPrivate = ImmLockIMCC(lpIMC->hPrivate);
                            if (myPrivate->bInComposition)
                            {
                                X11DRV_ForceXIMReset(lpIMC->hWnd);
                                GenerateIMEMessage(hIMC, WM_IME_ENDCOMPOSITION, 0, 0);
                                myPrivate->bInComposition = FALSE;
                            }
                            ImmUnlockIMCC(lpIMC->hPrivate);
                        }
                    }
                    else
                        lpIMC->fOpen = !lpIMC->fOpen;

                    break;
                default: FIXME("Unknown\n"); break;
            }
            break;
        case NI_COMPOSITIONSTR:
            switch (dwIndex)
            {
                case CPS_COMPLETE:
                {
                    HIMCC newCompStr;
                    DWORD cplen = 0;
                    LPWSTR cpstr;
                    LPCOMPOSITIONSTRING cs = NULL;
                    LPBYTE cdata = NULL;
                    LPIMEPRIVATE myPrivate;

                    TRACE("CPS_COMPLETE\n");

                    /* clear existing result */
                    newCompStr = updateResultStr(lpIMC->hCompStr, NULL, 0);

                    ImmDestroyIMCC(lpIMC->hCompStr);
                    lpIMC->hCompStr = newCompStr;

                    if (lpIMC->hCompStr)
                    {
                        cdata = ImmLockIMCC(lpIMC->hCompStr);
                        cs = (LPCOMPOSITIONSTRING)cdata;
                        cplen = cs->dwCompStrLen;
                        cpstr = (LPWSTR)&(cdata[cs->dwCompStrOffset]);
                        ImmUnlockIMCC(lpIMC->hCompStr);
                    }
                    if (cplen > 0)
                    {
                        WCHAR param = cpstr[0];

                        newCompStr = updateResultStr(lpIMC->hCompStr, cpstr, cplen);
                        ImmDestroyIMCC(lpIMC->hCompStr);
                        lpIMC->hCompStr = newCompStr;
                        newCompStr = updateCompStr(lpIMC->hCompStr, NULL, 0);
                        ImmDestroyIMCC(lpIMC->hCompStr);
                        lpIMC->hCompStr = newCompStr;

                        GenerateIMEMessage(hIMC, WM_IME_COMPOSITION, 0,
                                                  GCS_COMPSTR);

                        GenerateIMEMessage(hIMC, WM_IME_COMPOSITION, param,
                                            GCS_RESULTSTR|GCS_RESULTCLAUSE);
                    }

                    GenerateIMEMessage(hIMC,WM_IME_ENDCOMPOSITION, 0, 0);

                    myPrivate = ImmLockIMCC(lpIMC->hPrivate);
                    myPrivate->bInComposition = FALSE;
                    ImmUnlockIMCC(lpIMC->hPrivate);

                    bRet = TRUE;
                }
                break;
                case CPS_CONVERT: FIXME("CPS_CONVERT\n"); break;
                case CPS_REVERT: FIXME("CPS_REVERT\n"); break;
                case CPS_CANCEL:
                {
                    LPIMEPRIVATE myPrivate;

                    TRACE("CPS_CANCEL\n");

                    X11DRV_ForceXIMReset(lpIMC->hWnd);

                    if (lpIMC->hCompStr)
                        ImmDestroyIMCC(lpIMC->hCompStr);
                    lpIMC->hCompStr = ImeCreateBlankCompStr();

                    myPrivate = ImmLockIMCC(lpIMC->hPrivate);
                    if (myPrivate->bInComposition)
                    {
                        GenerateIMEMessage(hIMC, WM_IME_ENDCOMPOSITION, 0, 0);
                        myPrivate->bInComposition = FALSE;
                    }
                    ImmUnlockIMCC(lpIMC->hPrivate);
                    bRet = TRUE;
                }
                break;
                default: FIXME("Unknown\n"); break;
            }
            break;
        default: FIXME("Unknown Message\n"); break;
    }

    UnlockRealIMC(hIMC);
    return bRet;
}

BOOL WINAPI ImeRegisterWord(LPCWSTR lpszReading, DWORD dwStyle,
                            LPCWSTR lpszRegister)
{
    FIXME("(%s, %d, %s): stub\n", debugstr_w(lpszReading), dwStyle,
                                  debugstr_w(lpszRegister));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI ImeUnregisterWord(LPCWSTR lpszReading, DWORD dwStyle,
                              LPCWSTR lpszUnregister)
{
    FIXME("(%s, %d, %s): stub\n", debugstr_w(lpszReading), dwStyle,
                                  debugstr_w(lpszUnregister));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

UINT WINAPI ImeGetRegisterWordStyle(UINT nItem, LPSTYLEBUFW lpStyleBuf)
{
    FIXME("(%d, %p): stub\n", nItem, lpStyleBuf);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

UINT WINAPI ImeEnumRegisterWord(REGISTERWORDENUMPROCW lpfnEnumProc,
                                LPCWSTR lpszReading, DWORD dwStyle,
                                LPCWSTR lpszRegister, LPVOID lpData)
{
    FIXME("(%p, %s, %d, %s, %p): stub\n", lpfnEnumProc,
            debugstr_w(lpszReading), dwStyle, debugstr_w(lpszRegister),
            lpData);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

BOOL WINAPI ImeSetCompositionString(HIMC hIMC, DWORD dwIndex, LPCVOID lpComp,
                                    DWORD dwCompLen, LPCVOID lpRead,
                                    DWORD dwReadLen)
{
    LPINPUTCONTEXT lpIMC;
    DWORD flags = 0;
    WCHAR wParam  = 0;
    LPIMEPRIVATE myPrivate;

    TRACE("(%p, %d, %p, %d, %p, %d):\n",
         hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);


    if (hIMC != FROM_X11)
        FIXME("PROBLEM: This only sets the wine level string\n");

    /*
    * Explanation:
    *  this sets the composition string in the imm32.dll level
    *  of the composition buffer. we cannot manipulate the xim level
    *  buffer, which means that once the xim level buffer changes again
    *  any call to this function from the application will be lost
    */

    if (lpRead && dwReadLen)
        FIXME("Reading string unimplemented\n");

    lpIMC = LockRealIMC(hIMC);

    if (lpIMC == NULL)
        return FALSE;

    myPrivate = ImmLockIMCC(lpIMC->hPrivate);

    if (dwIndex == SCS_SETSTR)
    {
        HIMCC newCompStr;

        if (!myPrivate->bInComposition)
        {
            GenerateIMEMessage(hIMC, WM_IME_STARTCOMPOSITION, 0, 0);
            myPrivate->bInComposition = TRUE;
        }

        flags = GCS_COMPSTR;

        if (dwCompLen && lpComp)
        {
            newCompStr = updateCompStr(lpIMC->hCompStr, (LPCWSTR)lpComp, dwCompLen / sizeof(WCHAR));
            ImmDestroyIMCC(lpIMC->hCompStr);
            lpIMC->hCompStr = newCompStr;

             wParam = ((const WCHAR*)lpComp)[0];
             flags |= GCS_COMPCLAUSE | GCS_COMPATTR | GCS_DELTASTART;
        }
        else
        {
            newCompStr = updateCompStr(lpIMC->hCompStr, NULL, 0);
            ImmDestroyIMCC(lpIMC->hCompStr);
            lpIMC->hCompStr = newCompStr;
        }
    }

    GenerateIMEMessage(hIMC, WM_IME_COMPOSITION, wParam, flags);
    ImmUnlockIMCC(lpIMC->hPrivate);
    UnlockRealIMC(hIMC);

    return TRUE;
}

DWORD WINAPI ImeGetImeMenuItems(HIMC hIMC,  DWORD dwFlags,  DWORD dwType,
            LPIMEMENUITEMINFOW lpImeParentMenu, LPIMEMENUITEMINFOW lpImeMenu,
            DWORD dwSize)
{
    FIXME("(%p, %x %x %p %p %x): stub\n", hIMC, dwFlags, dwType,
                                lpImeParentMenu, lpImeMenu, dwSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/* Interfaces to XIM and other parts of winex11drv */

void IME_SetOpenStatus(BOOL fOpen, BOOL force)
{
    HIMC imc;
    LPINPUTCONTEXT lpIMC;
    LPIMEPRIVATE myPrivate;

    imc = RealIMC(FROM_X11);
    lpIMC = ImmLockIMC(imc);
    if (lpIMC == NULL)
        return;

    myPrivate = ImmLockIMCC(lpIMC->hPrivate);

    if (!fOpen && myPrivate->bInComposition)
    {
        ShowWindow(myPrivate->hwndDefault, SW_HIDE);
        ImmDestroyIMCC(lpIMC->hCompStr);
        lpIMC->hCompStr = ImeCreateBlankCompStr();
        myPrivate->bInComposition = FALSE;
        GenerateIMEMessage(imc, WM_IME_ENDCOMPOSITION, 0, 0);
    }

    if (lpIMC->fOpen && fOpen)
        ImmSetOpenStatus(imc, FALSE);

    if (fOpen || force)
        ImmSetOpenStatus(imc, fOpen);

    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMC(imc);
}

INT IME_GetCursorPos(void)
{
    LPINPUTCONTEXT lpIMC;
    INT rc = 0;
    LPCOMPOSITIONSTRING compstr;

    if (!hSelectedFrom)
        return rc;

    lpIMC = LockRealIMC(FROM_X11);
    if (lpIMC)
    {
        compstr = ImmLockIMCC(lpIMC->hCompStr);
        rc = compstr->dwCursorPos;
        ImmUnlockIMCC(lpIMC->hCompStr);
    }
    UnlockRealIMC(FROM_X11);
    return rc;
}

void IME_SetCursorPos(DWORD pos)
{
    LPINPUTCONTEXT lpIMC;
    LPCOMPOSITIONSTRING compstr;

    if (!hSelectedFrom)
        return;

    lpIMC = LockRealIMC(FROM_X11);
    if (!lpIMC)
        return;

    compstr = ImmLockIMCC(lpIMC->hCompStr);
    if (!compstr)
    {
        UnlockRealIMC(FROM_X11);
        return;
    }

    compstr->dwCursorPos = pos;
    ImmUnlockIMCC(lpIMC->hCompStr);
    UnlockRealIMC(FROM_X11);
    GenerateIMEMessage(FROM_X11, WM_IME_COMPOSITION, pos, GCS_CURSORPOS);
    return;
}

void IME_UpdateAssociation(HWND focus)
{
    ImmGetContext(focus);

    if (!focus || !hSelectedFrom)
        return;

    ImmAssociateContext(focus,RealIMC(FROM_X11));
}


BOOL IME_SetCompositionString(DWORD dwIndex, LPCVOID lpComp, DWORD dwCompLen,
                              LPCVOID lpRead, DWORD dwReadLen)
{
    return ImeSetCompositionString(FROM_X11, dwIndex, lpComp, dwCompLen,
                                    lpRead, dwReadLen);
}

void IME_SetResultString(LPWSTR lpResult, DWORD dwResultLen)
{
    HIMC imc;
    LPINPUTCONTEXT lpIMC;
    HIMCC newCompStr;
    LPIMEPRIVATE myPrivate;
    BOOL fOpen;

    imc = RealIMC(FROM_X11);
    lpIMC = ImmLockIMC(imc);
    if (lpIMC == NULL)
        return;

    newCompStr = updateResultStr(lpIMC->hCompStr, lpResult, dwResultLen);
    ImmDestroyIMCC(lpIMC->hCompStr);
    lpIMC->hCompStr = newCompStr;

    myPrivate = ImmLockIMCC(lpIMC->hPrivate);
    fOpen = lpIMC->fOpen;
    ImmSetOpenStatus(imc, TRUE);
    if (!myPrivate->bInComposition)
        GenerateIMEMessage(imc, WM_IME_STARTCOMPOSITION, 0, 0);
    GenerateIMEMessage(imc, WM_IME_COMPOSITION, 0, GCS_RESULTSTR);
    if (!myPrivate->bInComposition)
        GenerateIMEMessage(imc, WM_IME_ENDCOMPOSITION, 0, 0);
    if (!fOpen)
        ImmSetOpenStatus(imc, FALSE);
    ImmUnlockIMCC(lpIMC->hPrivate);

    ImmUnlockIMC(imc);
}

/*****
 * Internal functions to help with IME window management
 */
static void PaintDefaultIMEWnd(HIMC hIMC, HWND hwnd)
{
    PAINTSTRUCT ps;
    RECT rect;
    HDC hdc;
    LPCOMPOSITIONSTRING compstr;
    LPBYTE compdata = NULL;
    HMONITOR monitor;
    MONITORINFO mon_info;
    INT offX=0, offY=0;
    LPINPUTCONTEXT lpIMC;

    lpIMC = LockRealIMC(hIMC);
    if (lpIMC == NULL)
        return;

    hdc = BeginPaint(hwnd,&ps);

    GetClientRect(hwnd,&rect);
    FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));

    compdata = ImmLockIMCC(lpIMC->hCompStr);
    compstr = (LPCOMPOSITIONSTRING)compdata;

    if (compstr->dwCompStrLen && compstr->dwCompStrOffset)
    {
        SIZE size;
        POINT pt;
        HFONT oldfont = NULL;
        LPWSTR CompString;
        LPIMEPRIVATE myPrivate;

        CompString = (LPWSTR)(compdata + compstr->dwCompStrOffset);
        myPrivate = ImmLockIMCC(lpIMC->hPrivate);

        if (myPrivate->textfont)
            oldfont = SelectObject(hdc,myPrivate->textfont);

        ImmUnlockIMCC(lpIMC->hPrivate);

        GetTextExtentPoint32W(hdc, CompString, compstr->dwCompStrLen, &size);
        pt.x = size.cx;
        pt.y = size.cy;
        LPtoDP(hdc,&pt,1);

        /*
         * How this works based on tests on windows:
         * CFS_POINT: then we start our window at the point and grow it as large
         *    as it needs to be for the string.
         * CFS_RECT:  we still use the ptCurrentPos as a starting point and our
         *    window is only as large as we need for the string, but we do not
         *    grow such that our window exceeds the given rect.  Wrapping if
         *    needed and possible.   If our ptCurrentPos is outside of our rect
         *    then no window is displayed.
         * CFS_FORCE_POSITION: appears to behave just like CFS_POINT
         *    maybe becase the default MSIME does not do any IME adjusting.
         */
        if (lpIMC->cfCompForm.dwStyle != CFS_DEFAULT)
        {
            POINT cpt = lpIMC->cfCompForm.ptCurrentPos;
            ClientToScreen(lpIMC->hWnd,&cpt);
            rect.left = cpt.x;
            rect.top = cpt.y;
            rect.right = rect.left + pt.x;
            rect.bottom = rect.top + pt.y;
            monitor = MonitorFromPoint(cpt, MONITOR_DEFAULTTOPRIMARY);
        }
        else /* CFS_DEFAULT */
        {
            /* Windows places the default IME window in the bottom left */
            HWND target = lpIMC->hWnd;
            if (!target) target = GetFocus();

            GetWindowRect(target,&rect);
            rect.top = rect.bottom;
            rect.right = rect.left + pt.x + 20;
            rect.bottom = rect.top + pt.y + 20;
            offX=offY=10;
            monitor = MonitorFromWindow(target, MONITOR_DEFAULTTOPRIMARY);
        }

        if (lpIMC->cfCompForm.dwStyle == CFS_RECT)
        {
            RECT client;
            client =lpIMC->cfCompForm.rcArea;
            MapWindowPoints( lpIMC->hWnd, 0, (POINT *)&client, 2 );
            IntersectRect(&rect,&rect,&client);
            /* TODO:  Wrap the input if needed */
        }

        if (lpIMC->cfCompForm.dwStyle == CFS_DEFAULT)
        {
            /* make sure we are on the desktop */
            mon_info.cbSize = sizeof(mon_info);
            GetMonitorInfoW(monitor, &mon_info);

            if (rect.bottom > mon_info.rcWork.bottom)
            {
                int shift = rect.bottom - mon_info.rcWork.bottom;
                rect.top -= shift;
                rect.bottom -= shift;
            }
            if (rect.left < 0)
            {
                rect.right -= rect.left;
                rect.left = 0;
            }
            if (rect.right > mon_info.rcWork.right)
            {
                int shift = rect.right - mon_info.rcWork.right;
                rect.left -= shift;
                rect.right -= shift;
            }
        }

        SetWindowPos(hwnd, HWND_TOPMOST, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE);

        TextOutW(hdc, offX,offY, CompString, compstr->dwCompStrLen);

        if (oldfont)
            SelectObject(hdc,oldfont);
    }

    ImmUnlockIMCC(lpIMC->hCompStr);

    EndPaint(hwnd,&ps);
    UnlockRealIMC(hIMC);
}

static void UpdateDefaultIMEWindow(HIMC hIMC, HWND hwnd)
{
    LPCOMPOSITIONSTRING compstr;
    LPINPUTCONTEXT lpIMC;

    lpIMC = LockRealIMC(hIMC);
    if (lpIMC == NULL)
        return;

    if (lpIMC->hCompStr)
        compstr = ImmLockIMCC(lpIMC->hCompStr);
    else
        compstr = NULL;

    if (compstr == NULL || compstr->dwCompStrLen == 0)
        ShowWindow(hwnd,SW_HIDE);
    else
    {
        ShowWindow(hwnd,SW_SHOWNOACTIVATE);
        RedrawWindow(hwnd, NULL, NULL, RDW_ERASENOW | RDW_INVALIDATE);
    }

    if (compstr != NULL)
        ImmUnlockIMCC(lpIMC->hCompStr);

    lpIMC->hWnd = GetFocus();
    UnlockRealIMC(hIMC);
}

static void DefaultIMEComposition(HIMC hIMC, HWND hwnd, LPARAM lParam)
{
    TRACE("IME message WM_IME_COMPOSITION 0x%lx\n", lParam);
    if (lParam & GCS_RESULTSTR)
    {
        LPCOMPOSITIONSTRING compstr;
        LPBYTE compdata;
        LPWSTR ResultStr;
        HIMCC newCompStr;
        LPINPUTCONTEXT lpIMC;

        lpIMC = LockRealIMC(hIMC);
        if (lpIMC == NULL)
            return;

        TRACE("Posting result as IME_CHAR\n");
        compdata = ImmLockIMCC(lpIMC->hCompStr);
        compstr = (LPCOMPOSITIONSTRING)compdata;
        ResultStr = (LPWSTR)(compdata + compstr->dwResultStrOffset);
        GenerateIMECHARMessages(hIMC, ResultStr, compstr->dwResultStrLen);
        ImmUnlockIMCC(lpIMC->hCompStr);

        /* clear the buffer */
        newCompStr = updateResultStr(lpIMC->hCompStr, NULL, 0);
        ImmDestroyIMCC(lpIMC->hCompStr);
        lpIMC->hCompStr = newCompStr;
        UnlockRealIMC(hIMC);
    }
    else
        UpdateDefaultIMEWindow(hIMC, hwnd);
}

static void DefaultIMEStartComposition(HIMC hIMC, HWND hwnd )
{
    TRACE("IME message WM_IME_STARTCOMPOSITION\n");
    UpdateDefaultIMEWindow(hIMC, hwnd);
}

static LRESULT ImeHandleNotify(HIMC hIMC, HWND hwnd, UINT msg, WPARAM wParam,
                               LPARAM lParam)
{
    switch (wParam)
    {
        case IMN_OPENSTATUSWINDOW:
            FIXME("WM_IME_NOTIFY:IMN_OPENSTATUSWINDOW\n");
            break;
        case IMN_CLOSESTATUSWINDOW:
            FIXME("WM_IME_NOTIFY:IMN_CLOSESTATUSWINDOW\n");
            break;
        case IMN_OPENCANDIDATE:
            FIXME("WM_IME_NOTIFY:IMN_OPENCANDIDATE\n");
            break;
        case IMN_CHANGECANDIDATE:
            FIXME("WM_IME_NOTIFY:IMN_CHANGECANDIDATE\n");
            break;
        case IMN_CLOSECANDIDATE:
            FIXME("WM_IME_NOTIFY:IMN_CLOSECANDIDATE\n");
            break;
        case IMN_SETCONVERSIONMODE:
            FIXME("WM_IME_NOTIFY:IMN_SETCONVERSIONMODE\n");
            break;
        case IMN_SETSENTENCEMODE:
            FIXME("WM_IME_NOTIFY:IMN_SETSENTENCEMODE\n");
            break;
        case IMN_SETOPENSTATUS:
            TRACE("WM_IME_NOTIFY:IMN_SETOPENSTATUS\n");
            break;
        case IMN_SETCANDIDATEPOS:
            FIXME("WM_IME_NOTIFY:IMN_SETCANDIDATEPOS\n");
            break;
        case IMN_SETCOMPOSITIONFONT:
            FIXME("WM_IME_NOTIFY:IMN_SETCOMPOSITIONFONT\n");
            break;
        case IMN_SETCOMPOSITIONWINDOW:
            FIXME("WM_IME_NOTIFY:IMN_SETCOMPOSITIONWINDOW\n");
            break;
        case IMN_GUIDELINE:
            FIXME("WM_IME_NOTIFY:IMN_GUIDELINE\n");
            break;
        case IMN_SETSTATUSWINDOWPOS:
            FIXME("WM_IME_NOTIFY:IMN_SETSTATUSWINDOWPOS\n");
            break;
        default:
            FIXME("WM_IME_NOTIFY:<Unknown 0x%lx>\n",wParam);
            break;
    }
    return 0;
}

static LRESULT WINAPI IME_WindowProc(HWND hwnd, UINT msg, WPARAM wParam,
                                          LPARAM lParam)
{
    LRESULT rc = 0;
    HIMC    hIMC;

    TRACE("Incoming Message 0x%x  (0x%08lx, 0x%08lx)\n", msg, wParam, lParam);

    /*
     * Each UI window contains the current Input Context.
     * This Input Context can be obtained by calling GetWindowLong
     * with IMMGWL_IMC when the UI window receives a WM_IME_xxx message.
     * The UI window can refer to this Input Context and handles the
     * messages.
     */

    hIMC = (HIMC)GetWindowLongPtrW(hwnd,IMMGWL_IMC);
    if (!hIMC)
        hIMC = RealIMC(FROM_X11);

    /* if we have no hIMC there are many messages we cannot process */
    if (hIMC == NULL)
    {
        switch (msg) {
        case WM_IME_STARTCOMPOSITION:
        case WM_IME_ENDCOMPOSITION:
        case WM_IME_COMPOSITION:
        case WM_IME_NOTIFY:
        case WM_IME_CONTROL:
        case WM_IME_COMPOSITIONFULL:
        case WM_IME_SELECT:
        case WM_IME_CHAR:
            return 0L;
        default:
            break;
        }
    }

    switch(msg)
    {
        case WM_CREATE:
        {
            LPIMEPRIVATE myPrivate;
            LPINPUTCONTEXT lpIMC;

            SetWindowTextA(hwnd,"Wine Ime Active");

            lpIMC = LockRealIMC(hIMC);
            if (lpIMC)
            {
                myPrivate = ImmLockIMCC(lpIMC->hPrivate);
                myPrivate->hwndDefault = hwnd;
                ImmUnlockIMCC(lpIMC->hPrivate);
            }
            UnlockRealIMC(hIMC);

            return TRUE;
        }
        case WM_PAINT:
            PaintDefaultIMEWnd(hIMC, hwnd);
            return FALSE;

        case WM_NCCREATE:
            return TRUE;

        case WM_SETFOCUS:
            if (wParam)
                SetFocus((HWND)wParam);
            else
                FIXME("Received focus, should never have focus\n");
            break;
        case WM_IME_COMPOSITION:
            DefaultIMEComposition(hIMC, hwnd, lParam);
            break;
        case WM_IME_STARTCOMPOSITION:
            DefaultIMEStartComposition(hIMC, hwnd);
            break;
        case WM_IME_ENDCOMPOSITION:
            TRACE("IME message %s, 0x%lx, 0x%lx\n",
                    "WM_IME_ENDCOMPOSITION", wParam, lParam);
            ShowWindow(hwnd,SW_HIDE);
            break;
        case WM_IME_SELECT:
            TRACE("IME message %s, 0x%lx, 0x%lx\n","WM_IME_SELECT", wParam, lParam);
            break;
        case WM_IME_CONTROL:
            TRACE("IME message %s, 0x%lx, 0x%lx\n","WM_IME_CONTROL", wParam, lParam);
            rc = 1;
            break;
        case WM_IME_NOTIFY:
            rc = ImeHandleNotify(hIMC,hwnd,msg,wParam,lParam);
            break;
       default:
            TRACE("Non-standard message 0x%x\n",msg);
    }
    /* check the MSIME messages */
    if (msg == WM_MSIME_SERVICE)
    {
            TRACE("IME message %s, 0x%lx, 0x%lx\n","WM_MSIME_SERVICE", wParam, lParam);
            rc = FALSE;
    }
    else if (msg == WM_MSIME_RECONVERTOPTIONS)
    {
            TRACE("IME message %s, 0x%lx, 0x%lx\n","WM_MSIME_RECONVERTOPTIONS", wParam, lParam);
    }
    else if (msg == WM_MSIME_MOUSE)
    {
            TRACE("IME message %s, 0x%lx, 0x%lx\n","WM_MSIME_MOUSE", wParam, lParam);
    }
    else if (msg == WM_MSIME_RECONVERTREQUEST)
    {
            TRACE("IME message %s, 0x%lx, 0x%lx\n","WM_MSIME_RECONVERTREQUEST", wParam, lParam);
    }
    else if (msg == WM_MSIME_RECONVERT)
    {
            TRACE("IME message %s, 0x%lx, 0x%lx\n","WM_MSIME_RECONVERT", wParam, lParam);
    }
    else if (msg == WM_MSIME_QUERYPOSITION)
    {
            TRACE("IME message %s, 0x%lx, 0x%lx\n","WM_MSIME_QUERYPOSITION", wParam, lParam);
    }
    else if (msg == WM_MSIME_DOCUMENTFEED)
    {
            TRACE("IME message %s, 0x%lx, 0x%lx\n","WM_MSIME_DOCUMENTFEED", wParam, lParam);
    }
    /* DefWndProc if not an IME message */
    if (!rc && !((msg >= WM_IME_STARTCOMPOSITION && msg <= WM_IME_KEYLAST) ||
                      (msg >= WM_IME_SETCONTEXT && msg <= WM_IME_KEYUP)))
        rc = DefWindowProcW(hwnd,msg,wParam,lParam);

    return rc;
}
