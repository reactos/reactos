/*
 *	OLEDLG library
 *
 *	Copyright 1998	Patrik Stridvall
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "oledlg.h"
#include "ole2.h"
#include "oledlg_private.h"
#include "resource.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

HINSTANCE OLEDLG_hInstance = 0;

UINT cf_embed_source;
UINT cf_embedded_object;
UINT cf_link_source;
UINT cf_object_descriptor;
UINT cf_link_src_descriptor;
UINT cf_ownerlink;
UINT cf_filename;
UINT cf_filenamew;

UINT oleui_msg_help;
UINT oleui_msg_enddialog;

static void register_clipboard_formats(void)
{
    /* These used to be declared in olestd.h, but that seems to have been removed from the api */
    static const WCHAR CF_EMBEDSOURCEW[]          = { 'E','m','b','e','d',' ','S','o','u','r','c','e',0 };
    static const WCHAR CF_EMBEDDEDOBJECTW[]       = { 'E','m','b','e','d','d','e','d',' ','O','b','j','e','c','t',0 };
    static const WCHAR CF_LINKSOURCEW[]           = { 'L','i','n','k',' ','S','o','u','r','c','e',0 };
    static const WCHAR CF_OBJECTDESCRIPTORW[]     = { 'O','b','j','e','c','t',' ','D','e','s','c','r','i','p','t','o','r',0 };
    static const WCHAR CF_LINKSRCDESCRIPTORW[]    = { 'L','i','n','k',' ','S','o','u','r','c','e',' ','D','e','s','c','r','i','p','t','o','r',0 };
    static const WCHAR CF_OWNERLINKW[]            = { 'O','w','n','e','r','L','i','n','k',0 };
    static const WCHAR CF_FILENAMEW[]             = { 'F','i','l','e','N','a','m','e',0 };
    static const WCHAR CF_FILENAMEWW[]            = { 'F','i','l','e','N','a','m','e','W',0 };

    /* Load in the same order as native to make debugging easier */
    cf_object_descriptor    = RegisterClipboardFormatW(CF_OBJECTDESCRIPTORW);
    cf_link_src_descriptor  = RegisterClipboardFormatW(CF_LINKSRCDESCRIPTORW);
    cf_embed_source         = RegisterClipboardFormatW(CF_EMBEDSOURCEW);
    cf_embedded_object      = RegisterClipboardFormatW(CF_EMBEDDEDOBJECTW);
    cf_link_source          = RegisterClipboardFormatW(CF_LINKSOURCEW);
    cf_ownerlink            = RegisterClipboardFormatW(CF_OWNERLINKW);
    cf_filename             = RegisterClipboardFormatW(CF_FILENAMEW);
    cf_filenamew            = RegisterClipboardFormatW(CF_FILENAMEWW);
}

static void register_messages(void)
{
    oleui_msg_help             = RegisterWindowMessageW(SZOLEUI_MSG_HELPW);
    oleui_msg_enddialog        = RegisterWindowMessageW(SZOLEUI_MSG_ENDDIALOGW);
}

/***********************************************************************
 *		DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%lx %p\n", hinstDLL, fdwReason, fImpLoad);

    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        OLEDLG_hInstance = hinstDLL;
        register_clipboard_formats();
        register_messages();
        break;
    }
    return TRUE;
}


/***********************************************************************
 *           OleUIAddVerbMenuA (OLEDLG.1)
 */
BOOL WINAPI OleUIAddVerbMenuA(IOleObject *object, LPCSTR shorttype,
    HMENU hMenu, UINT uPos, UINT uIDVerbMin, UINT uIDVerbMax,
    BOOL addConvert, UINT idConvert, HMENU *lphMenu)
{
    WCHAR *shorttypeW = NULL;
    BOOL ret;

    TRACE("(%p, %s, %p, %d, %d, %d, %d, %d, %p)\n", object, debugstr_a(shorttype),
        hMenu, uPos, uIDVerbMin, uIDVerbMax, addConvert, idConvert, lphMenu);

    if (shorttype)
    {
        INT len = MultiByteToWideChar(CP_ACP, 0, shorttype, -1, NULL, 0);
        shorttypeW = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        if (shorttypeW)
            MultiByteToWideChar(CP_ACP, 0, shorttype, -1, shorttypeW, len);
    }

    ret = OleUIAddVerbMenuW(object, shorttypeW, hMenu, uPos, uIDVerbMin, uIDVerbMax,
        addConvert, idConvert, lphMenu);
    HeapFree(GetProcessHeap(), 0, shorttypeW);
    return ret;
}

static inline BOOL is_verb_in_range(const OLEVERB *verb, UINT idmin, UINT idmax)
{
    if (idmax == 0) return TRUE;
    return (verb->lVerb + idmin <= idmax);
}

static HRESULT get_next_insertable_verb(IEnumOLEVERB *enumverbs, UINT idmin, UINT idmax, OLEVERB *verb)
{
    memset(verb, 0, sizeof(*verb));

    while (IEnumOLEVERB_Next(enumverbs, 1, verb, NULL) == S_OK) {
        if (is_verb_in_range(verb, idmin, idmax) && (verb->grfAttribs & OLEVERBATTRIB_ONCONTAINERMENU))
            return S_OK;

        CoTaskMemFree(verb->lpszVerbName);
        memset(verb, 0, sizeof(*verb));
    }

    return S_FALSE;
}

static void insert_verb_to_menu(HMENU menu, UINT idmin, const OLEVERB *verb)
{
    InsertMenuW(menu, ~0, verb->fuFlags | MF_BYPOSITION | MF_STRING, verb->lVerb + idmin, verb->lpszVerbName);
}

/***********************************************************************
 *           OleUIAddVerbMenuW (OLEDLG.14)
 */
BOOL WINAPI OleUIAddVerbMenuW(IOleObject *object, LPCWSTR shorttype,
    HMENU hMenu, UINT uPos, UINT idmin, UINT idmax, BOOL addConvert, UINT idConvert, HMENU *ret_submenu)
{
    IEnumOLEVERB *enumverbs = NULL;
    LPOLESTR usertype = NULL;
    OLEVERB firstverb, verb;
    WCHAR *objecttype;
    WCHAR resstrW[32]; /* should be enough */
    DWORD_PTR args[2];
    BOOL singleverb;
    HMENU submenu;
    WCHAR *str;

    TRACE("(%p, %s, %p, %d, %d, %d, %d, %d, %p)\n", object, debugstr_w(shorttype),
        hMenu, uPos, idmin, idmax, addConvert, idConvert, ret_submenu);

    if (ret_submenu)
        *ret_submenu = NULL;

    if (!hMenu || !ret_submenu)
        return FALSE;

    /* check if we can get verbs at all */
    if (object)
        IOleObject_EnumVerbs(object, &enumverbs);

    LoadStringW(OLEDLG_hInstance, IDS_VERBMENU_OBJECT, resstrW, ARRAY_SIZE(resstrW));
    /* no object, or object without enumeration support */
    if (!object || !enumverbs) {
        RemoveMenu(hMenu, uPos, MF_BYPOSITION);
        InsertMenuW(hMenu, uPos, MF_BYPOSITION|MF_STRING|MF_GRAYED, idmin, resstrW);
        return FALSE;
    }

    /* root entry string */
    if (!shorttype && (IOleObject_GetUserType(object, USERCLASSTYPE_SHORT, &usertype) == S_OK))
        objecttype = usertype;
    else
        objecttype = (WCHAR*)shorttype;

    /* iterate through verbs */

    /* find first suitable verb */
    get_next_insertable_verb(enumverbs, idmin, idmax, &firstverb);
    singleverb = get_next_insertable_verb(enumverbs, idmin, idmax, &verb) != S_OK;

    if (singleverb && !addConvert) {
        LoadStringW(OLEDLG_hInstance, IDS_VERBMENU_SINGLEVERB_OBJECT, resstrW, ARRAY_SIZE(resstrW));

        args[0] = (DWORD_PTR)firstverb.lpszVerbName;
        args[1] = (DWORD_PTR)objecttype;

        FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_ARGUMENT_ARRAY,
            resstrW, 0, 0, (WCHAR*)&str, 0, (va_list *)args);

        RemoveMenu(hMenu, uPos, MF_BYPOSITION);
        InsertMenuW(hMenu, uPos, MF_BYPOSITION|MF_STRING, idmin, str);
        CoTaskMemFree(firstverb.lpszVerbName);
        HeapFree(GetProcessHeap(), 0, str);
        IEnumOLEVERB_Release(enumverbs);
        CoTaskMemFree(usertype);
        return TRUE;
    }

    submenu = CreatePopupMenu();
    insert_verb_to_menu(submenu, idmin, &firstverb);
    CoTaskMemFree(firstverb.lpszVerbName);

    if (!singleverb) {
        insert_verb_to_menu(submenu, idmin, &verb);
        CoTaskMemFree(verb.lpszVerbName);
    }

    while (get_next_insertable_verb(enumverbs, idmin, idmax, &verb) == S_OK) {
        insert_verb_to_menu(submenu, idmin, &verb);
        CoTaskMemFree(verb.lpszVerbName);
    }

    /* convert verb is at the bottom of a popup, separated from verbs */
    if (addConvert) {
        LoadStringW(OLEDLG_hInstance, IDS_VERBMENU_CONVERT, resstrW, ARRAY_SIZE(resstrW));
        InsertMenuW(submenu, ~0, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
        InsertMenuW(submenu, ~0, MF_BYPOSITION|MF_STRING, idConvert, resstrW);
    }

    if (submenu)
        *ret_submenu = submenu;

    /* now submenu is ready, add root entry to original menu, attach submenu */
    LoadStringW(OLEDLG_hInstance, IDS_VERBMENU_OBJECT_WITH_NAME, resstrW, ARRAY_SIZE(resstrW));

    args[0] = (DWORD_PTR)objecttype;
    FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_ARGUMENT_ARRAY,
        resstrW, 0, 0, (WCHAR*)&str, 0, (va_list *)args);

    InsertMenuW(hMenu, uPos, MF_BYPOSITION|MF_POPUP|MF_STRING, (UINT_PTR)submenu, str);
    HeapFree(GetProcessHeap(), 0, str);
    IEnumOLEVERB_Release(enumverbs);
    CoTaskMemFree(usertype);
    return TRUE;
}

/***********************************************************************
 *           OleUICanConvertOrActivateAs (OLEDLG.2)
 */
BOOL WINAPI OleUICanConvertOrActivateAs(
    REFCLSID rClsid, BOOL fIsLinkedObject, WORD wFormat)
{
  FIXME("(%p, %d, %hd): stub\n",
    rClsid, fIsLinkedObject, wFormat
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           OleUIInsertObjectW (OLEDLG.20)
 */
UINT WINAPI OleUIInsertObjectW(LPOLEUIINSERTOBJECTW lpOleUIInsertObject)
{
  FIXME("(%p): stub\n", lpOleUIInsertObject);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIEditLinksA (OLEDLG.5)
 */
UINT WINAPI OleUIEditLinksA(LPOLEUIEDITLINKSA lpOleUIEditLinks)
{
  FIXME("(%p): stub\n", lpOleUIEditLinks);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIEditLinksW (OLEDLG.19)
 */
UINT WINAPI OleUIEditLinksW(LPOLEUIEDITLINKSW lpOleUIEditLinks)
{
  FIXME("(%p): stub\n", lpOleUIEditLinks);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIChangeIconA (OLEDLG.6)
 */
UINT WINAPI OleUIChangeIconA(
  LPOLEUICHANGEICONA lpOleUIChangeIcon)
{
  FIXME("(%p): stub\n", lpOleUIChangeIcon);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIChangeIconW (OLEDLG.16)
 */
UINT WINAPI OleUIChangeIconW(
  LPOLEUICHANGEICONW lpOleUIChangeIcon)
{
  FIXME("(%p): stub\n", lpOleUIChangeIcon);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIConvertA (OLEDLG.7)
 */
UINT WINAPI OleUIConvertA(LPOLEUICONVERTA lpOleUIConvert)
{
  FIXME("(%p): stub\n", lpOleUIConvert);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIConvertW (OLEDLG.18)
 */
UINT WINAPI OleUIConvertW(LPOLEUICONVERTW lpOleUIConvert)
{
  FIXME("(%p): stub\n", lpOleUIConvert);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIBusyA (OLEDLG.8)
 */
UINT WINAPI OleUIBusyA(LPOLEUIBUSYA lpOleUIBusy)
{
  FIXME("(%p): stub\n", lpOleUIBusy);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIBusyW (OLEDLG.15)
 */
UINT WINAPI OleUIBusyW(LPOLEUIBUSYW lpOleUIBusy)
{
  FIXME("(%p): stub\n", lpOleUIBusy);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIUpdateLinksA (OLEDLG.9)
 */
BOOL WINAPI OleUIUpdateLinksA(
  LPOLEUILINKCONTAINERA lpOleUILinkCntr,
  HWND hwndParent, LPSTR lpszTitle, INT cLinks)
{
  FIXME("(%p, %p, %s, %d): stub\n",
    lpOleUILinkCntr, hwndParent, debugstr_a(lpszTitle), cLinks
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           OleUIUpdateLinksW (OLEDLG.23)
 */
BOOL WINAPI OleUIUpdateLinksW(
  LPOLEUILINKCONTAINERW lpOleUILinkCntr,
  HWND hwndParent, LPWSTR lpszTitle, INT cLinks)
{
  FIXME("(%p, %p, %s, %d): stub\n",
    lpOleUILinkCntr, hwndParent, debugstr_w(lpszTitle), cLinks
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           OleUIPromptUserA (OLEDLG.10)
 */
INT WINAPIV OleUIPromptUserA(
  INT nTemplate, HWND hwndParent, ...)
{
  FIXME("(%d, %p, ...): stub\n", nTemplate, hwndParent);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIPromptUserW (OLEDLG.13)
 */
INT WINAPIV OleUIPromptUserW(
  INT nTemplate, HWND hwndParent, ...)
{
  FIXME("(%d, %p, ...): stub\n", nTemplate, hwndParent);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIObjectPropertiesA (OLEDLG.11)
 */
UINT WINAPI OleUIObjectPropertiesA(
  LPOLEUIOBJECTPROPSA lpOleUIObjectProps)
{
  FIXME("(%p): stub\n", lpOleUIObjectProps);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIObjectPropertiesW (OLEDLG.21)
 */
UINT WINAPI OleUIObjectPropertiesW(
  LPOLEUIOBJECTPROPSW lpOleUIObjectProps)
{
  FIXME("(%p): stub\n", lpOleUIObjectProps);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIChangeSourceA (OLEDLG.12)
 */
UINT WINAPI OleUIChangeSourceA(
  LPOLEUICHANGESOURCEA lpOleUIChangeSource)
{
  FIXME("(%p): stub\n", lpOleUIChangeSource);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIChangeSourceW (OLEDLG.17)
 */
UINT WINAPI OleUIChangeSourceW(
  LPOLEUICHANGESOURCEW lpOleUIChangeSource)
{
  FIXME("(%p): stub\n", lpOleUIChangeSource);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}
