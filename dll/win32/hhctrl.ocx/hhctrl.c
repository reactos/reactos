/*
 * hhctrl implementation
 *
 * Copyright 2004 Krzysztof Foltman
 * Copyright 2007 Jacek Caban for CodeWeavers
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

#include "wine/debug.h"

#define INIT_GUID
#include "hhctrl.h"

WINE_DEFAULT_DEBUG_CHANNEL(htmlhelp);

HINSTANCE hhctrl_hinstance;
BOOL hh_process = FALSE;

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p,%d,%p)\n", hInstance, fdwReason, lpvReserved);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        hhctrl_hinstance = hInstance;
        DisableThreadLibraryCalls(hInstance);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

static const char *command_to_string(UINT command)
{
#define X(x) case x: return #x
    switch (command)
    {
        X( HH_DISPLAY_TOPIC );
        X( HH_DISPLAY_TOC );
        X( HH_DISPLAY_INDEX );
        X( HH_DISPLAY_SEARCH );
        X( HH_SET_WIN_TYPE );
        X( HH_GET_WIN_TYPE );
        X( HH_GET_WIN_HANDLE );
        X( HH_ENUM_INFO_TYPE );
        X( HH_SET_INFO_TYPE );
        X( HH_SYNC );
        X( HH_RESERVED1 );
        X( HH_RESERVED2 );
        X( HH_RESERVED3 );
        X( HH_KEYWORD_LOOKUP );
        X( HH_DISPLAY_TEXT_POPUP );
        X( HH_HELP_CONTEXT );
        X( HH_TP_HELP_CONTEXTMENU );
        X( HH_TP_HELP_WM_HELP );
        X( HH_CLOSE_ALL );
        X( HH_ALINK_LOOKUP );
        X( HH_GET_LAST_ERROR );
        X( HH_ENUM_CATEGORY );
        X( HH_ENUM_CATEGORY_IT );
        X( HH_RESET_IT_FILTER );
        X( HH_SET_INCLUSIVE_FILTER );
        X( HH_SET_EXCLUSIVE_FILTER );
        X( HH_INITIALIZE );
        X( HH_UNINITIALIZE );
        X( HH_SAFE_DISPLAY_TOPIC );
        X( HH_PRETRANSLATEMESSAGE );
        X( HH_SET_GLOBAL_PROPERTY );
    default: return "???";
    }
#undef X
}

/******************************************************************
 *		HtmlHelpW (HHCTRL.OCX.15)
 */
HWND WINAPI HtmlHelpW(HWND caller, LPCWSTR filename, UINT command, DWORD_PTR data)
{
    TRACE("(%p, %s, command=%s, data=%lx)\n",
          caller, debugstr_w( filename ),
          command_to_string( command ), data);

    switch (command)
    {
    case HH_DISPLAY_TOPIC:
    case HH_DISPLAY_TOC:
    case HH_DISPLAY_SEARCH:{
        static const WCHAR delimW[] = {':',':',0};
        HHInfo *info;
        BOOL res;
        WCHAR chm_file[MAX_PATH];
        const WCHAR *index;

        FIXME("Not all HH cases handled correctly\n");

        if (!filename)
            return NULL;

        index = strstrW(filename, delimW);
        if (index)
        {
            memcpy(chm_file, filename, (index-filename)*sizeof(WCHAR));
            chm_file[index-filename] = 0;
            filename = chm_file;
            index += 2; /* advance beyond "::" for calling NavigateToChm() later */
        }
        else
        {
            if (command!=HH_DISPLAY_SEARCH) /* FIXME - use HH_FTS_QUERYW structure in data */
                index = (const WCHAR*)data;
        }

        info = CreateHelpViewer(filename);
        if(!info)
            return NULL;

        if(!index)
            index = info->WinType.pszFile;

        res = NavigateToChm(info, info->pCHMInfo->szFile, index);
        if(!res)
        {
            ReleaseHelpViewer(info);
            return NULL;
        }
        return info->WinType.hwndHelp;
    }
    case HH_HELP_CONTEXT: {
        HHInfo *info;
        LPWSTR url;

        if (!filename)
            return NULL;

        info = CreateHelpViewer(filename);
        if(!info)
            return NULL;

        url = FindContextAlias(info->pCHMInfo, data);
        if(!url)
        {
            ReleaseHelpViewer(info);
            return NULL;
        }

        NavigateToUrl(info, url);
        heap_free(url);
        return info->WinType.hwndHelp;
    }
    case HH_PRETRANSLATEMESSAGE: {
        static BOOL warned = FALSE;

        if (!warned)
        {
            FIXME("HH_PRETRANSLATEMESSAGE unimplemented\n");
            warned = TRUE;
        }
        return 0;
    }
    default:
        FIXME("HH case %s not handled.\n", command_to_string( command ));
    }

    return 0;
}

/******************************************************************
 *		HtmlHelpA (HHCTRL.OCX.14)
 */
HWND WINAPI HtmlHelpA(HWND caller, LPCSTR filename, UINT command, DWORD_PTR data)
{
    WCHAR *wfile = NULL, *wdata = NULL;
    DWORD len;
    HWND result;

    if (filename)
    {
        len = MultiByteToWideChar( CP_ACP, 0, filename, -1, NULL, 0 );
        wfile = heap_alloc(len*sizeof(WCHAR));
        MultiByteToWideChar( CP_ACP, 0, filename, -1, wfile, len );
    }

    if (data)
    {
        switch(command)
        {
        case HH_ALINK_LOOKUP:
        case HH_DISPLAY_SEARCH:
        case HH_DISPLAY_TEXT_POPUP:
        case HH_GET_LAST_ERROR:
        case HH_GET_WIN_TYPE:
        case HH_KEYWORD_LOOKUP:
        case HH_SET_WIN_TYPE:
        case HH_SYNC:
            FIXME("structures not handled yet\n");
            break;

        case HH_DISPLAY_INDEX:
        case HH_DISPLAY_TOPIC:
        case HH_DISPLAY_TOC:
        case HH_GET_WIN_HANDLE:
        case HH_SAFE_DISPLAY_TOPIC:
            len = MultiByteToWideChar( CP_ACP, 0, (const char*)data, -1, NULL, 0 );
            wdata = heap_alloc(len*sizeof(WCHAR));
            MultiByteToWideChar( CP_ACP, 0, (const char*)data, -1, wdata, len );
            break;

        case HH_CLOSE_ALL:
        case HH_HELP_CONTEXT:
        case HH_INITIALIZE:
        case HH_PRETRANSLATEMESSAGE:
        case HH_TP_HELP_CONTEXTMENU:
        case HH_TP_HELP_WM_HELP:
        case HH_UNINITIALIZE:
            /* either scalar or pointer to scalar - do nothing */
            break;

        default:
            FIXME("Unknown command: %s (%d)\n", command_to_string(command), command);
            break;
        }
    }

    result = HtmlHelpW( caller, wfile, command, wdata ? (DWORD_PTR)wdata : data );

    heap_free(wfile);
    heap_free(wdata);
    return result;
}

/******************************************************************
 *		doWinMain (HHCTRL.OCX.13)
 */
int WINAPI doWinMain(HINSTANCE hInstance, LPSTR szCmdLine)
{
    MSG msg;

    hh_process = TRUE;

    /* FIXME: Check szCmdLine for bad arguments */
    HtmlHelpA(GetDesktopWindow(), szCmdLine, HH_DISPLAY_TOPIC, 0);

    while (GetMessageW(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}

/******************************************************************
 *		DllGetClassObject (HHCTRL.OCX.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    FIXME("(%s %s %p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
