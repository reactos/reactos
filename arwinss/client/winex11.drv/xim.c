/*
 * Functions for further XIM control
 *
 * Copyright 2003 CodeWeavers, Aric Stewart
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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "winnls.h"
#include "x11drv.h"
#include "imm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xim);

#ifndef HAVE_XICCALLBACK_CALLBACK
#define XICCallback XIMCallback
#define XICProc XIMProc
#endif

BOOL ximInComposeMode=FALSE;

/* moved here from imm32 for dll separation */
static DWORD dwCompStringLength = 0;
static LPBYTE CompositionString = NULL;
static DWORD dwCompStringSize = 0;

#define STYLE_OFFTHESPOT (XIMPreeditArea | XIMStatusArea)
#define STYLE_OVERTHESPOT (XIMPreeditPosition | XIMStatusNothing)
#define STYLE_ROOT (XIMPreeditNothing | XIMStatusNothing)
/* this uses all the callbacks to utilize full IME support */
#define STYLE_CALLBACK (XIMPreeditCallbacks | XIMStatusNothing)
/* in order to enable deadkey support */
#define STYLE_NONE (XIMPreeditNothing | XIMStatusNothing)

static XIMStyle ximStyle = 0;
static XIMStyle ximStyleRoot = 0;
static XIMStyle ximStyleRequest = STYLE_CALLBACK;

static void X11DRV_ImmSetInternalString(DWORD dwOffset,
                                        DWORD selLength, LPWSTR lpComp, DWORD dwCompLen)
{
    /* Composition strings are edited in chunks */
    unsigned int byte_length = dwCompLen * sizeof(WCHAR);
    unsigned int byte_offset = dwOffset * sizeof(WCHAR);
    unsigned int byte_selection = selLength * sizeof(WCHAR);
    int byte_expansion = byte_length - byte_selection;
    LPBYTE ptr_new;

    TRACE("( %i, %i, %p, %d):\n", dwOffset, selLength, lpComp, dwCompLen );

    if (byte_expansion + dwCompStringLength >= dwCompStringSize)
    {
        if (CompositionString)
            ptr_new = HeapReAlloc(GetProcessHeap(), 0, CompositionString,
                                  dwCompStringSize + byte_expansion);
        else
            ptr_new = HeapAlloc(GetProcessHeap(), 0,
                                dwCompStringSize + byte_expansion);

        if (ptr_new == NULL)
        {
            ERR("Couldn't expand composition string buffer\n");
            return;
        }

        CompositionString = ptr_new;
        dwCompStringSize += byte_expansion;
    }

    ptr_new = CompositionString + byte_offset;
    memmove(ptr_new + byte_length, ptr_new + byte_selection,
            dwCompStringLength - byte_offset - byte_selection);
    memcpy(ptr_new, lpComp, byte_length);
    dwCompStringLength += byte_expansion;

    IME_SetCompositionString(SCS_SETSTR, CompositionString,
                             dwCompStringLength, NULL, 0);
}

void X11DRV_XIMLookupChars( const char *str, DWORD count )
{
    DWORD dwOutput;
    WCHAR *wcOutput;
    HWND focus;

    TRACE("%p %u\n", str, count);

    dwOutput = MultiByteToWideChar(CP_UNIXCP, 0, str, count, NULL, 0);
    wcOutput = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * dwOutput);
    if (wcOutput == NULL)
        return;
    MultiByteToWideChar(CP_UNIXCP, 0, str, count, wcOutput, dwOutput);

    if ((focus = GetFocus()))
        IME_UpdateAssociation(focus);

    IME_SetResultString(wcOutput, dwOutput);
    HeapFree(GetProcessHeap(), 0, wcOutput);
}

static BOOL XIMPreEditStateNotifyCallback(XIC xic, XPointer p, XPointer data)
{
    const struct x11drv_win_data * const win_data = (struct x11drv_win_data *)p;
    const XIMPreeditState state = ((XIMPreeditStateNotifyCallbackStruct *)data)->state;

    TRACE("xic = %p, win = %lx, state = %lu\n", xic, win_data->whole_window, state);
    switch (state)
    {
    case XIMPreeditEnable:
        IME_SetOpenStatus(TRUE, TRUE);
        break;
    case XIMPreeditDisable:
        IME_SetOpenStatus(FALSE, TRUE);
        break;
    default:
        break;
    }
    return TRUE;
}

static int XIMPreEditStartCallback(XIC ic, XPointer client_data, XPointer call_data)
{
    TRACE("PreEditStartCallback %p\n",ic);
    IME_SetOpenStatus(TRUE, FALSE);
    ximInComposeMode = TRUE;
    return -1;
}

static void XIMPreEditDoneCallback(XIC ic, XPointer client_data, XPointer call_data)
{
    TRACE("PreeditDoneCallback %p\n",ic);
    ximInComposeMode = FALSE;
    if (dwCompStringSize)
        HeapFree(GetProcessHeap(), 0, CompositionString);
    dwCompStringSize = 0;
    dwCompStringLength = 0;
    CompositionString = NULL;
    IME_SetOpenStatus(FALSE, FALSE);
}

static void XIMPreEditDrawCallback(XIM ic, XPointer client_data,
                                   XIMPreeditDrawCallbackStruct *P_DR)
{
    TRACE("PreEditDrawCallback %p\n",ic);

    if (P_DR)
    {
        int sel = P_DR->chg_first;
        int len = P_DR->chg_length;
        if (P_DR->text)
        {
            if (! P_DR->text->encoding_is_wchar)
            {
                DWORD dwOutput;
                WCHAR *wcOutput;

                TRACE("multibyte\n");
                dwOutput = MultiByteToWideChar(CP_UNIXCP, 0,
                           P_DR->text->string.multi_byte, -1,
                           NULL, 0);
                wcOutput = HeapAlloc(GetProcessHeap(), 0, sizeof (WCHAR) * dwOutput);
                if (wcOutput)
                {
                    dwOutput = MultiByteToWideChar(CP_UNIXCP, 0,
                               P_DR->text->string.multi_byte, -1,
                               wcOutput, dwOutput);

                    /* ignore null */
                    dwOutput --;
                    X11DRV_ImmSetInternalString (sel, len, wcOutput, dwOutput);
                    HeapFree(GetProcessHeap(), 0, wcOutput);
                }
            }
            else
            {
                FIXME("wchar PROBIBILY WRONG\n");
                X11DRV_ImmSetInternalString (sel, len,
                                             (LPWSTR)P_DR->text->string.wide_char,
                                             P_DR->text->length);
            }
        }
        else
            X11DRV_ImmSetInternalString (sel, len, NULL, 0);
        IME_SetCursorPos(P_DR->caret);
    }
    TRACE("Finished\n");
}

static void XIMPreEditCaretCallback(XIC ic, XPointer client_data,
                                    XIMPreeditCaretCallbackStruct *P_C)
{
    TRACE("PreeditCaretCallback %p\n",ic);

    if (P_C)
    {
        int pos = IME_GetCursorPos();
        TRACE("pos: %d\n", pos);
        switch(P_C->direction)
        {
            case XIMForwardChar:
            case XIMForwardWord:
                pos++;
                break;
            case XIMBackwardChar:
            case XIMBackwardWord:
                pos--;
                break;
            case XIMLineStart:
                pos = 0;
                break;
            case XIMAbsolutePosition:
                pos = P_C->position;
                break;
            case XIMDontChange:
                P_C->position = pos;
                return;
            case XIMCaretUp:
            case XIMCaretDown:
            case XIMPreviousLine:
            case XIMNextLine:
            case XIMLineEnd:
                FIXME("Not implemented\n");
                break;
        }
        IME_SetCursorPos(pos);
        P_C->position = pos;
    }
    TRACE("Finished\n");
}

void X11DRV_ForceXIMReset(HWND hwnd)
{
    XIC ic = X11DRV_get_ic(hwnd);
    if (ic)
    {
        char* leftover;
        TRACE("Forcing Reset %p\n",ic);
        wine_tsx11_lock();
        leftover = XmbResetIC(ic);
        XFree(leftover);
        wine_tsx11_unlock();
    }
}

BOOL X11DRV_SetPreeditState(HWND hwnd, BOOL fOpen)
{
    XIC ic;
    XIMPreeditState state;
    XVaNestedList attr_set, attr_get;
    BOOL ret;

    ic = X11DRV_get_ic(hwnd);
    if (!ic)
        return FALSE;

    if (fOpen)
        state = XIMPreeditEnable;
    else
        state = XIMPreeditDisable;

    ret = FALSE;
    wine_tsx11_lock();

    attr_set = XVaCreateNestedList(0, XNPreeditState, state, NULL);
    if (attr_set == NULL)
        goto error1;

    attr_get = XVaCreateNestedList(0, XNPreeditState, &state, NULL);
    if (attr_get == NULL)
        goto error2;

    if (XSetICValues(ic, XNPreeditAttributes, attr_set, NULL) != NULL)
        goto error3;

    /* SCIM claims it supports XNPreeditState, but seems to ignore */
    state = XIMPreeditUnKnown;
    ret = XGetICValues(ic, XNPreeditAttributes, attr_get, NULL) == NULL &&
          ((fOpen && state == XIMPreeditEnable) ||
           (!fOpen && state == XIMPreeditDisable));
error3:
    XFree(attr_get);
error2:
    XFree(attr_set);
error1:
    wine_tsx11_unlock();
    return ret;
}


/***********************************************************************
 *           X11DRV_InitXIM
 *
 * Process-wide XIM initialization.
 */
BOOL X11DRV_InitXIM( const char *input_style )
{
    BOOL ret;

    if (!strcasecmp(input_style, "offthespot"))
        ximStyleRequest = STYLE_OFFTHESPOT;
    else if (!strcasecmp(input_style, "overthespot"))
        ximStyleRequest = STYLE_OVERTHESPOT;
    else if (!strcasecmp(input_style, "root"))
        ximStyleRequest = STYLE_ROOT;

    wine_tsx11_lock();
    if (!(ret = XSupportsLocale()))
    {
        WARN("X does not support locale.\n");
    }
    else if (XSetLocaleModifiers("") == NULL)
    {
        WARN("Could not set locale modifiers.\n");
        ret = FALSE;
    }
    wine_tsx11_unlock();
    return ret;
}


static void open_xim_callback( Display *display, XPointer ptr, XPointer data );

static void X11DRV_DestroyIM(XIM xim, XPointer p, XPointer data)
{
    struct x11drv_thread_data *thread_data = x11drv_thread_data();

    TRACE("xim = %p, p = %p\n", xim, p);
    thread_data->xim = NULL;
    ximStyle = 0;
    wine_tsx11_lock();
    XRegisterIMInstantiateCallback( thread_data->display, NULL, NULL, NULL, open_xim_callback, NULL );
    wine_tsx11_unlock();
}

/***********************************************************************
 *           X11DRV Ime creation
 *
 * Should always be called with the x11 lock held
 */
static BOOL open_xim( Display *display )
{
    struct x11drv_thread_data *thread_data = x11drv_thread_data();
    XIMStyle ximStyleCallback, ximStyleNone;
    XIMStyles *ximStyles = NULL;
    INT i;
    XIM xim;
    XIMCallback destroy;

    xim = XOpenIM(display, NULL, NULL, NULL);
    if (xim == NULL)
    {
        WARN("Could not open input method.\n");
        return FALSE;
    }

    destroy.client_data = NULL;
    destroy.callback = X11DRV_DestroyIM;
    if (XSetIMValues(xim, XNDestroyCallback, &destroy, NULL))
    {
        WARN("Could not set destroy callback.\n");
    }

    TRACE("xim = %p\n", xim);
    TRACE("X display of IM = %p\n", XDisplayOfIM(xim));
    TRACE("Using %s locale of Input Method\n", XLocaleOfIM(xim));

    XGetIMValues(xim, XNQueryInputStyle, &ximStyles, NULL);
    if (ximStyles == 0)
    {
        WARN("Could not find supported input style.\n");
        XCloseIM(xim);
        return FALSE;
    }
    else
    {
        TRACE("ximStyles->count_styles = %d\n", ximStyles->count_styles);

        ximStyleRoot = 0;
        ximStyleNone = 0;
        ximStyleCallback = 0;

        for (i = 0; i < ximStyles->count_styles; ++i)
        {
            int style = ximStyles->supported_styles[i];
            TRACE("ximStyles[%d] = %s%s%s%s%s\n", i,
                        (style&XIMPreeditArea)?"XIMPreeditArea ":"",
                        (style&XIMPreeditCallbacks)?"XIMPreeditCallbacks ":"",
                        (style&XIMPreeditPosition)?"XIMPreeditPosition ":"",
                        (style&XIMPreeditNothing)?"XIMPreeditNothing ":"",
                        (style&XIMPreeditNone)?"XIMPreeditNone ":"");
            if (!ximStyle && (ximStyles->supported_styles[i] ==
                                ximStyleRequest))
            {
                ximStyle = ximStyleRequest;
                TRACE("Setting Style: ximStyle = ximStyleRequest\n");
            }
            else if (!ximStyleRoot &&(ximStyles->supported_styles[i] ==
                     STYLE_ROOT))
            {
                ximStyleRoot = STYLE_ROOT;
                TRACE("Setting Style: ximStyleRoot = STYLE_ROOT\n");
            }
            else if (!ximStyleCallback &&(ximStyles->supported_styles[i] ==
                     STYLE_CALLBACK))
            {
                ximStyleCallback = STYLE_CALLBACK;
                TRACE("Setting Style: ximStyleCallback = STYLE_CALLBACK\n");
            }
            else if (!ximStyleNone && (ximStyles->supported_styles[i] ==
                     STYLE_NONE))
            {
                TRACE("Setting Style: ximStyleNone = STYLE_NONE\n");
                ximStyleNone = STYLE_NONE;
            }
        }
        XFree(ximStyles);

        if (ximStyle == 0)
            ximStyle = ximStyleRoot;

        if (ximStyle == 0)
            ximStyle = ximStyleNone;

        if (ximStyleCallback == 0)
        {
            TRACE("No callback style avalable\n");
            ximStyleCallback = ximStyle;
        }

    }

    thread_data->xim = xim;

    if ((ximStyle & (XIMPreeditNothing | XIMPreeditNone)) == 0 ||
        (ximStyle & (XIMStatusNothing | XIMStatusNone)) == 0)
    {
        char **list;
        int count;
        thread_data->font_set = XCreateFontSet(display, "fixed",
                          &list, &count, NULL);
        TRACE("ximFontSet = %p\n", thread_data->font_set);
        TRACE("list = %p, count = %d\n", list, count);
        if (list != NULL)
        {
            int i;
            for (i = 0; i < count; ++i)
                TRACE("list[%d] = %s\n", i, list[i]);
            XFreeStringList(list);
        }
    }
    else
        thread_data->font_set = NULL;

    wine_tsx11_unlock();
    IME_UpdateAssociation(NULL);
    wine_tsx11_lock();
    return TRUE;
}

static void open_xim_callback( Display *display, XPointer ptr, XPointer data )
{
    if (open_xim( display ))
        XUnregisterIMInstantiateCallback( display, NULL, NULL, NULL, open_xim_callback, NULL);
}

void X11DRV_SetupXIM(void)
{
    Display *display = thread_display();

    wine_tsx11_lock();
    if (!open_xim( display ))
        XRegisterIMInstantiateCallback( display, NULL, NULL, NULL, open_xim_callback, NULL );
    wine_tsx11_unlock();
}

static BOOL X11DRV_DestroyIC(XIC xic, XPointer p, XPointer data)
{
    struct x11drv_win_data *win_data = (struct x11drv_win_data *)p;
    TRACE("xic = %p, win = %lx\n", xic, win_data->whole_window);
    win_data->xic = NULL;
    return TRUE;
}


XIC X11DRV_CreateIC(XIM xim, struct x11drv_win_data *data)
{
    XPoint spot = {0};
    XVaNestedList preedit = NULL;
    XVaNestedList status = NULL;
    XIC xic;
    XICCallback destroy = {(XPointer)data, (XICProc)X11DRV_DestroyIC};
    XICCallback P_StateNotifyCB, P_StartCB, P_DoneCB, P_DrawCB, P_CaretCB;
    LANGID langid = PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale()));
    Window win = data->whole_window;
    XFontSet fontSet = x11drv_thread_data()->font_set;

    TRACE("xim = %p\n", xim);

    wine_tsx11_lock();

    /* use complex and slow XIC initialization method only for CJK */
    if (langid != LANG_CHINESE &&
        langid != LANG_JAPANESE &&
        langid != LANG_KOREAN)
    {
        xic = XCreateIC(xim,
                        XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                        XNClientWindow, win,
                        XNFocusWindow, win,
                        XNDestroyCallback, &destroy,
                        NULL);
        wine_tsx11_unlock();
        data->xic = xic;
        return xic;
    }

    /* create callbacks */
    P_StateNotifyCB.client_data = (XPointer)data;
    P_StartCB.client_data = NULL;
    P_DoneCB.client_data = NULL;
    P_DrawCB.client_data = NULL;
    P_CaretCB.client_data = NULL;
    P_StateNotifyCB.callback = (XICProc)XIMPreEditStateNotifyCallback;
    P_StartCB.callback = (XICProc)XIMPreEditStartCallback;
    P_DoneCB.callback = (XICProc)XIMPreEditDoneCallback;
    P_DrawCB.callback = (XICProc)XIMPreEditDrawCallback;
    P_CaretCB.callback = (XICProc)XIMPreEditCaretCallback;

    if ((ximStyle & (XIMPreeditNothing | XIMPreeditNone)) == 0)
    {
        preedit = XVaCreateNestedList(0,
                        XNFontSet, fontSet,
                        XNSpotLocation, &spot,
                        XNPreeditStateNotifyCallback, &P_StateNotifyCB,
                        XNPreeditStartCallback, &P_StartCB,
                        XNPreeditDoneCallback, &P_DoneCB,
                        XNPreeditDrawCallback, &P_DrawCB,
                        XNPreeditCaretCallback, &P_CaretCB,
                        NULL);
        TRACE("preedit = %p\n", preedit);
    }
    else
    {
        preedit = XVaCreateNestedList(0,
                        XNPreeditStateNotifyCallback, &P_StateNotifyCB,
                        XNPreeditStartCallback, &P_StartCB,
                        XNPreeditDoneCallback, &P_DoneCB,
                        XNPreeditDrawCallback, &P_DrawCB,
                        XNPreeditCaretCallback, &P_CaretCB,
                        NULL);

        TRACE("preedit = %p\n", preedit);
    }

    if ((ximStyle & (XIMStatusNothing | XIMStatusNone)) == 0)
    {
        status = XVaCreateNestedList(0,
            XNFontSet, fontSet,
            NULL);
        TRACE("status = %p\n", status);
     }

    if (preedit != NULL && status != NULL)
    {
        xic = XCreateIC(xim,
              XNInputStyle, ximStyle,
              XNPreeditAttributes, preedit,
              XNStatusAttributes, status,
              XNClientWindow, win,
              XNFocusWindow, win,
              XNDestroyCallback, &destroy,
              NULL);
     }
    else if (preedit != NULL)
    {
        xic = XCreateIC(xim,
              XNInputStyle, ximStyle,
              XNPreeditAttributes, preedit,
              XNClientWindow, win,
              XNFocusWindow, win,
              XNDestroyCallback, &destroy,
              NULL);
    }
    else if (status != NULL)
    {
        xic = XCreateIC(xim,
              XNInputStyle, ximStyle,
              XNStatusAttributes, status,
              XNClientWindow, win,
              XNFocusWindow, win,
              XNDestroyCallback, &destroy,
              NULL);
    }
    else
    {
        xic = XCreateIC(xim,
              XNInputStyle, ximStyle,
              XNClientWindow, win,
              XNFocusWindow, win,
              XNDestroyCallback, &destroy,
              NULL);
    }

    TRACE("xic = %p\n", xic);
    data->xic = xic;

    if (preedit != NULL)
        XFree(preedit);
    if (status != NULL)
        XFree(status);

    wine_tsx11_unlock();

    return xic;
}
