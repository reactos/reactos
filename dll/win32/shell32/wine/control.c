/* Control Panel management
 *
 * Copyright 2001 Eric Pouech
 * Copyright 2008 Owen Rudge
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

#include <assert.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS

#include <windef.h>
#include <winbase.h>
#define NO_SHLWAPI_REG
#include <shlwapi.h>
#include <shellapi.h>
#include <wine/debug.h>

#include <strsafe.h>

#include "cpanel.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(shlctrl);

void Control_UnloadApplet(CPlApplet* applet)
{
    unsigned	i;

    for (i = 0; i < applet->count; i++)
        applet->proc(applet->hWnd, CPL_STOP, i, applet->info[i].data);

    if (applet->proc) applet->proc(applet->hWnd, CPL_EXIT, 0L, 0L);
    FreeLibrary(applet->hModule);
#ifndef __REACTOS__
    list_remove( &applet->entry );
#endif
    HeapFree(GetProcessHeap(), 0, applet->cmd);
    HeapFree(GetProcessHeap(), 0, applet);
}

CPlApplet*	Control_LoadApplet(HWND hWnd, LPCWSTR cmd, CPanel* panel)
{
#ifdef __REACTOS__
    ACTCTXW ActCtx = {sizeof(ACTCTX), ACTCTX_FLAG_RESOURCE_NAME_VALID};
    ULONG_PTR cookie;
    BOOL bActivated;
    WCHAR fileBuffer[MAX_PATH];
#endif
    CPlApplet*	applet;
    DWORD len;
    unsigned 	i;
    CPLINFO	info;
    NEWCPLINFOW newinfo;

    if (!(applet = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*applet))))
       return applet;

    len = ExpandEnvironmentStringsW(cmd, NULL, 0);
    if (len > 0)
    {
        if (!(applet->cmd = HeapAlloc(GetProcessHeap(), 0, (len+1) * sizeof(WCHAR))))
        {
            WARN("Cannot allocate memory for applet path\n");
            goto theError;
        }
        ExpandEnvironmentStringsW(cmd, applet->cmd, len+1);
    }
    else
    {
        WARN("Cannot expand applet path\n");
        goto theError;
    }

    applet->hWnd = hWnd;

#ifdef __REACTOS__
    StringCchCopy(fileBuffer, MAX_PATH, applet->cmd);
    if (PathIsFileSpecW(fileBuffer))
        SearchPath(NULL, fileBuffer, NULL, MAX_PATH, fileBuffer, NULL);
    ActCtx.lpSource = fileBuffer;
    ActCtx.lpResourceName = (LPCWSTR)123;
    applet->hActCtx = CreateActCtx(&ActCtx);
    bActivated = (applet->hActCtx != INVALID_HANDLE_VALUE ? ActivateActCtx(applet->hActCtx, &cookie) : FALSE);
#endif

    if (!(applet->hModule = LoadLibraryW(applet->cmd))) {
        WARN("Cannot load control panel applet %s\n", debugstr_w(applet->cmd));
	goto theError;
    }
    if (!(applet->proc = (APPLET_PROC)GetProcAddress(applet->hModule, "CPlApplet"))) {
        WARN("Not a valid control panel applet %s\n", debugstr_w(applet->cmd));
	goto theError;
    }
    if (!applet->proc(hWnd, CPL_INIT, 0L, 0L)) {
        WARN("Init of applet has failed\n");
	goto theError;
    }
    if ((applet->count = applet->proc(hWnd, CPL_GETCOUNT, 0L, 0L)) == 0) {
        WARN("No subprogram in applet\n");
        applet->proc(applet->hWnd, CPL_EXIT, 0, 0);
	goto theError;
    }

    applet = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, applet,
                         FIELD_OFFSET( CPlApplet, info[applet->count] ));

    for (i = 0; i < applet->count; i++) {
       ZeroMemory(&newinfo, sizeof(newinfo));
       newinfo.dwSize = sizeof(NEWCPLINFOA);
       applet->info[i].helpfile[0] = 0;
       /* proc is supposed to return a null value upon success for
	* CPL_INQUIRE and CPL_NEWINQUIRE
	* However, real drivers don't seem to behave like this
	* So, use introspection rather than return value
	*/
       applet->proc(hWnd, CPL_INQUIRE, i, (LPARAM)&info);
       applet->info[i].data = info.lData;
#ifdef __REACTOS__
       applet->info[i].idIcon = info.idIcon;
#endif

       if (info.idIcon != CPL_DYNAMIC_RES)
	   applet->info[i].icon = LoadIconW(applet->hModule, MAKEINTRESOURCEW(info.idIcon));
       if (info.idName != CPL_DYNAMIC_RES)
	   LoadStringW(applet->hModule, info.idName,
		       applet->info[i].name, sizeof(applet->info[i].name) / sizeof(WCHAR));
       if (info.idInfo != CPL_DYNAMIC_RES)
	   LoadStringW(applet->hModule, info.idInfo,
		       applet->info[i].info, sizeof(applet->info[i].info) / sizeof(WCHAR));

       /* some broken control panels seem to return incorrect values in CPL_INQUIRE,
          but proper data in CPL_NEWINQUIRE. if we get an empty string or a null
          icon, see what we can get from CPL_NEWINQUIRE */

       if (!applet->info[i].name[0]) info.idName = CPL_DYNAMIC_RES;

       /* zero-length szInfo may not be a buggy applet, but it doesn't hurt for us
          to check anyway */

       if (!applet->info[i].info[0]) info.idInfo = CPL_DYNAMIC_RES;

       if (applet->info[i].icon == NULL)
           info.idIcon = CPL_DYNAMIC_RES;

       if ((info.idIcon == CPL_DYNAMIC_RES) || (info.idName == CPL_DYNAMIC_RES) ||
           (info.idInfo == CPL_DYNAMIC_RES)) {
	   applet->proc(hWnd, CPL_NEWINQUIRE, i, (LPARAM)&newinfo);

	   applet->info[i].data = newinfo.lData;
	   if (info.idIcon == CPL_DYNAMIC_RES) {
	       if (!newinfo.hIcon) WARN("couldn't get icon for applet %u\n", i);
	       applet->info[i].icon = newinfo.hIcon;
	   }
	   if (newinfo.dwSize == sizeof(NEWCPLINFOW)) {
	       if (info.idName == CPL_DYNAMIC_RES)
	           memcpy(applet->info[i].name, newinfo.szName, sizeof(newinfo.szName));
	       if (info.idInfo == CPL_DYNAMIC_RES)
	           memcpy(applet->info[i].info, newinfo.szInfo, sizeof(newinfo.szInfo));
	       memcpy(applet->info[i].helpfile, newinfo.szHelpFile, sizeof(newinfo.szHelpFile));
	   } else {
	       if (info.idName == CPL_DYNAMIC_RES)
                   MultiByteToWideChar(CP_ACP, 0, ((LPNEWCPLINFOA)&newinfo)->szName,
	                               sizeof(((LPNEWCPLINFOA)&newinfo)->szName) / sizeof(CHAR),
			               applet->info[i].name, sizeof(applet->info[i].name) / sizeof(WCHAR));
	       if (info.idInfo == CPL_DYNAMIC_RES)
                   MultiByteToWideChar(CP_ACP, 0, ((LPNEWCPLINFOA)&newinfo)->szInfo,
	                               sizeof(((LPNEWCPLINFOA)&newinfo)->szInfo) / sizeof(CHAR),
			               applet->info[i].info, sizeof(applet->info[i].info) / sizeof(WCHAR));
               MultiByteToWideChar(CP_ACP, 0, ((LPNEWCPLINFOA)&newinfo)->szHelpFile,
	                           sizeof(((LPNEWCPLINFOA)&newinfo)->szHelpFile) / sizeof(CHAR),
			           applet->info[i].helpfile,
                                   sizeof(applet->info[i].helpfile) / sizeof(WCHAR));
           }
       }
    }

#ifdef __REACTOS__
    if (bActivated)
        DeactivateActCtx(0, cookie);
#endif

#ifndef __REACTOS__
    list_add_head( &panel->applets, &applet->entry );
#endif

    return applet;

 theError:
    FreeLibrary(applet->hModule);
    HeapFree(GetProcessHeap(), 0, applet->cmd);
    HeapFree(GetProcessHeap(), 0, applet);
    return NULL;
}

#ifndef __REACTOS__

#define IDC_LISTVIEW        1000
#define IDC_STATUSBAR       1001

#define NUM_COLUMNS            2
#define LISTVIEW_DEFSTYLE   (WS_CHILD | WS_VISIBLE | WS_TABSTOP |\
                             LVS_SORTASCENDING | LVS_AUTOARRANGE | LVS_SINGLESEL)

static BOOL Control_CreateListView (CPanel *panel)
{
    RECT ws, sb;
    WCHAR empty_string[] = {0};
    WCHAR buf[MAX_STRING_LEN];
    LVCOLUMNW lvc;

    /* Create list view */
    GetClientRect(panel->hWndStatusBar, &sb);
    GetClientRect(panel->hWnd, &ws);

    panel->hWndListView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW,
                          empty_string, LISTVIEW_DEFSTYLE | LVS_ICON,
                          0, 0, ws.right - ws.left, ws.bottom - ws.top -
                          (sb.bottom - sb.top), panel->hWnd,
                          (HMENU) IDC_LISTVIEW, panel->hInst, NULL);

    if (!panel->hWndListView)
        return FALSE;

    /* Create image lists for list view */
    panel->hImageListSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON), ILC_COLOR32 | ILC_MASK, 1, 1);
    panel->hImageListLarge = ImageList_Create(GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON), ILC_COLOR32 | ILC_MASK, 1, 1);

    SendMessageW(panel->hWndListView, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)panel->hImageListSmall);
    SendMessageW(panel->hWndListView, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)panel->hImageListLarge);

    /* Create columns for list view */
    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    lvc.pszText = buf;
    lvc.fmt = LVCFMT_LEFT;

    /* Name column */
    lvc.iSubItem = 0;
    lvc.cx = (ws.right - ws.left) / 3;
    LoadStringW(shell32_hInstance, IDS_CPANEL_NAME, buf, sizeof(buf) / sizeof(buf[0]));

    if (ListView_InsertColumnW(panel->hWndListView, 0, &lvc) == -1)
        return FALSE;

    /* Description column */
    lvc.iSubItem = 1;
    lvc.cx = ((ws.right - ws.left) / 3) * 2;
    LoadStringW(shell32_hInstance, IDS_CPANEL_DESCRIPTION, buf, sizeof(buf) /
        sizeof(buf[0]));

    if (ListView_InsertColumnW(panel->hWndListView, 1, &lvc) == -1)
        return FALSE;

    return(TRUE);
}

static void 	 Control_WndProc_Create(HWND hWnd, const CREATESTRUCTW* cs)
{
   CPanel* panel = cs->lpCreateParams;
   HMENU hMenu, hSubMenu;
   CPlApplet* applet;
   MENUITEMINFOW mii;
   unsigned int i;
   int menucount, index;
   CPlItem *item;
   LVITEMW lvItem;
   INITCOMMONCONTROLSEX icex;
   INT sb_parts;
   int itemidx;

   SetWindowLongPtrW(hWnd, 0, (LONG_PTR)panel);
   panel->hWnd = hWnd;

   /* Initialise common control DLL */
   icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
   icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
   InitCommonControlsEx(&icex);

   /* create the status bar */
   if (!(panel->hWndStatusBar = CreateStatusWindowW(WS_CHILD | WS_VISIBLE | CCS_BOTTOM | SBARS_SIZEGRIP, NULL, hWnd, IDC_STATUSBAR)))
       return;

   sb_parts = -1;
   SendMessageW(panel->hWndStatusBar, SB_SETPARTS, 1, (LPARAM) &sb_parts);

   /* create the list view */
   if (!Control_CreateListView(panel))
       return;

   hMenu = LoadMenuW(shell32_hInstance, MAKEINTRESOURCEW(MENU_CPANEL));

   /* insert menu items for applets */
   hSubMenu = GetSubMenu(hMenu, 0);
   menucount = 0;

   LIST_FOR_EACH_ENTRY( applet, &panel->applets, CPlApplet, entry )
   {
      for (i = 0; i < applet->count; i++) {
         /* set up a CPlItem for this particular subprogram */
         item = HeapAlloc(GetProcessHeap(), 0, sizeof(CPlItem));

         if (!item)
            continue;

         item->applet = applet;
         item->id = i;

         mii.cbSize = sizeof(MENUITEMINFOW);
         mii.fMask = MIIM_ID | MIIM_STRING | MIIM_DATA;
         mii.dwTypeData = applet->info[i].name;
         mii.cch = sizeof(applet->info[i].name) / sizeof(WCHAR);
         mii.wID = IDM_CPANEL_APPLET_BASE + menucount;
         mii.dwItemData = (ULONG_PTR)item;

         if (InsertMenuItemW(hSubMenu, menucount, TRUE, &mii)) {
            /* add the list view item */
            HICON icon = applet->info[i].icon;
            if (!icon) icon = LoadImageW( 0, (LPCWSTR)IDI_WINLOGO, IMAGE_ICON, 0, 0, LR_SHARED );
            index = ImageList_AddIcon(panel->hImageListLarge, icon);
            ImageList_AddIcon(panel->hImageListSmall, icon);

            lvItem.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
            lvItem.iItem = menucount;
            lvItem.iSubItem = 0;
            lvItem.pszText = applet->info[i].name;
            lvItem.iImage = index;
            lvItem.lParam = (LPARAM) item;

            itemidx = ListView_InsertItemW(panel->hWndListView, &lvItem);

            /* add the description */
            ListView_SetItemTextW(panel->hWndListView, itemidx, 1, applet->info[i].info);

            /* update menu bar, increment count */
            DrawMenuBar(hWnd);
            menucount++;
         }
      }
   }

   panel->total_subprogs = menucount;

   /* check the "large items" icon in the View menu */
   hSubMenu = GetSubMenu(hMenu, 1);
   CheckMenuRadioItem(hSubMenu, FCIDM_SHVIEW_BIGICON, FCIDM_SHVIEW_REPORTVIEW,
      FCIDM_SHVIEW_BIGICON, MF_BYCOMMAND);

   SetMenu(hWnd, hMenu);
}

static void Control_FreeCPlItems(HWND hWnd, CPanel *panel)
{
    HMENU hMenu, hSubMenu;
    MENUITEMINFOW mii;
    unsigned int i;

    /* get the File menu */
    hMenu = GetMenu(hWnd);

    if (!hMenu)
        return;

    hSubMenu = GetSubMenu(hMenu, 0);

    if (!hSubMenu)
        return;

    /* loop and free the item data */
    for (i = IDM_CPANEL_APPLET_BASE; i <= IDM_CPANEL_APPLET_BASE + panel->total_subprogs; i++)
    {
        mii.cbSize = sizeof(MENUITEMINFOW);
        mii.fMask = MIIM_DATA;

        if (!GetMenuItemInfoW(hSubMenu, i, FALSE, &mii))
            continue;

        HeapFree(GetProcessHeap(), 0, (LPVOID) mii.dwItemData);
    }
}

static void Control_UpdateListViewStyle(CPanel *panel, UINT style, UINT id)
{
    HMENU hMenu, hSubMenu;

    SetWindowLongW(panel->hWndListView, GWL_STYLE, LISTVIEW_DEFSTYLE | style);

    /* update the menu */
    hMenu = GetMenu(panel->hWnd);
    hSubMenu = GetSubMenu(hMenu, 1);

    CheckMenuRadioItem(hSubMenu, FCIDM_SHVIEW_BIGICON, FCIDM_SHVIEW_REPORTVIEW,
        id, MF_BYCOMMAND);
}

static CPlItem* Control_GetCPlItem_From_MenuID(HWND hWnd, UINT id)
{
    HMENU hMenu, hSubMenu;
    MENUITEMINFOW mii;

    /* retrieve the CPlItem structure from the menu item data */
    hMenu = GetMenu(hWnd);

    if (!hMenu)
        return NULL;

    hSubMenu = GetSubMenu(hMenu, 0);

    if (!hSubMenu)
        return NULL;

    mii.cbSize = sizeof(MENUITEMINFOW);
    mii.fMask = MIIM_DATA;

    if (!GetMenuItemInfoW(hSubMenu, id, FALSE, &mii))
        return NULL;

    return (CPlItem *) mii.dwItemData;
}

static CPlItem* Control_GetCPlItem_From_ListView(CPanel *panel)
{
    LVITEMW lvItem;
    int selitem;

    selitem = SendMessageW(panel->hWndListView, LVM_GETNEXTITEM, -1, LVNI_FOCUSED
        | LVNI_SELECTED);

    if (selitem != -1)
    {
        lvItem.iItem = selitem;
        lvItem.mask = LVIF_PARAM;

        if (SendMessageW(panel->hWndListView, LVM_GETITEMW, 0, (LPARAM) &lvItem))
            return (CPlItem *) lvItem.lParam;
    }

    return NULL;
}

static void Control_StartApplet(HWND hWnd, CPlItem *item)
{
    WCHAR verbOpen[] = {'c','p','l','o','p','e','n',0};
    WCHAR format[] = {'@','%','d',0};
    WCHAR param[MAX_PATH];

    /* execute the applet if item is valid */
    if (item)
    {
        wsprintfW(param, format, item->id);
        ShellExecuteW(hWnd, verbOpen, item->applet->cmd, param, NULL, SW_SHOW);
    }
}

static LRESULT WINAPI	Control_WndProc(HWND hWnd, UINT wMsg,
					WPARAM lParam1, LPARAM lParam2)
{
   CPanel*	panel = (CPanel*)GetWindowLongPtrW(hWnd, 0);

   if (panel || wMsg == WM_CREATE) {
      switch (wMsg) {
      case WM_CREATE:
	 Control_WndProc_Create(hWnd, (CREATESTRUCTW*)lParam2);
	 return 0;
      case WM_DESTROY:
         {
             CPlApplet *applet, *next;
             LIST_FOR_EACH_ENTRY_SAFE( applet, next, &panel->applets, CPlApplet, entry )
                 Control_UnloadApplet(applet);
         }
         Control_FreeCPlItems(hWnd, panel);
         PostQuitMessage(0);
	 break;
      case WM_COMMAND:
         switch (LOWORD(lParam1))
         {
             case IDM_CPANEL_EXIT:
                 SendMessageW(hWnd, WM_CLOSE, 0, 0);
                 return 0;

             case IDM_CPANEL_ABOUT:
                 {
                     WCHAR appName[MAX_STRING_LEN];
                     HICON icon = LoadImageW(shell32_hInstance, MAKEINTRESOURCEW(IDI_SHELL_CONTROL_PANEL),
                                             IMAGE_ICON, 48, 48, LR_SHARED);

                     LoadStringW(shell32_hInstance, IDS_CPANEL_TITLE, appName,
                         sizeof(appName) / sizeof(appName[0]));
                     ShellAboutW(hWnd, appName, NULL, icon);

                     return 0;
                 }

             case FCIDM_SHVIEW_BIGICON:
                 Control_UpdateListViewStyle(panel, LVS_ICON, FCIDM_SHVIEW_BIGICON);
                 return 0;

             case FCIDM_SHVIEW_SMALLICON:
                 Control_UpdateListViewStyle(panel, LVS_SMALLICON, FCIDM_SHVIEW_SMALLICON);
                 return 0;

             case FCIDM_SHVIEW_LISTVIEW:
                 Control_UpdateListViewStyle(panel, LVS_LIST, FCIDM_SHVIEW_LISTVIEW);
                 return 0;

             case FCIDM_SHVIEW_REPORTVIEW:
                 Control_UpdateListViewStyle(panel, LVS_REPORT, FCIDM_SHVIEW_REPORTVIEW);
                 return 0;

             default:
                 /* check if this is an applet */
                 if ((LOWORD(lParam1) >= IDM_CPANEL_APPLET_BASE) &&
                     (LOWORD(lParam1) <= IDM_CPANEL_APPLET_BASE + panel->total_subprogs))
                 {
                     Control_StartApplet(hWnd, Control_GetCPlItem_From_MenuID(hWnd, LOWORD(lParam1)));
                     return 0;
                 }

                 break;
         }

         break;

      case WM_NOTIFY:
      {
          LPNMHDR nmh = (LPNMHDR) lParam2;

          switch (nmh->idFrom)
          {
              case IDC_LISTVIEW:
                  switch (nmh->code)
                  {
                      case NM_RETURN:
                      case NM_DBLCLK:
                      {
                          Control_StartApplet(hWnd, Control_GetCPlItem_From_ListView(panel));
                          return 0;
                      }
                      case LVN_ITEMCHANGED:
                      {
                          CPlItem *item = Control_GetCPlItem_From_ListView(panel);

                          /* update the status bar if item is valid */
                          if (item)
                              SetWindowTextW(panel->hWndStatusBar, item->applet->info[item->id].info);
                          else
                              SetWindowTextW(panel->hWndStatusBar, NULL);

                          return 0;
                      }
                  }

                  break;
          }

          break;
      }

      case WM_MENUSELECT:
          /* check if this is an applet */
          if ((LOWORD(lParam1) >= IDM_CPANEL_APPLET_BASE) &&
              (LOWORD(lParam1) <= IDM_CPANEL_APPLET_BASE + panel->total_subprogs))
          {
              CPlItem *item = Control_GetCPlItem_From_MenuID(hWnd, LOWORD(lParam1));

              /* update the status bar if item is valid */
              if (item)
                  SetWindowTextW(panel->hWndStatusBar, item->applet->info[item->id].info);
          }
          else if ((HIWORD(lParam1) == 0xFFFF) && (lParam2 == 0))
          {
              /* reset status bar description to that of the selected icon */
              CPlItem *item = Control_GetCPlItem_From_ListView(panel);

              if (item)
                  SetWindowTextW(panel->hWndStatusBar, item->applet->info[item->id].info);
              else
                  SetWindowTextW(panel->hWndStatusBar, NULL);

              return 0;
          }
          else
              SetWindowTextW(panel->hWndStatusBar, NULL);

          return 0;

      case WM_SIZE:
      {
          HDWP hdwp;
          RECT sb;

          hdwp = BeginDeferWindowPos(2);

          if (hdwp == NULL)
              break;

          GetClientRect(panel->hWndStatusBar, &sb);

          hdwp = DeferWindowPos(hdwp, panel->hWndListView, NULL, 0, 0,
              LOWORD(lParam2), HIWORD(lParam2) - (sb.bottom - sb.top),
              SWP_NOZORDER | SWP_NOMOVE);

          if (hdwp == NULL)
              break;

          hdwp = DeferWindowPos(hdwp, panel->hWndStatusBar, NULL, 0, 0,
              LOWORD(lParam2), LOWORD(lParam1), SWP_NOZORDER | SWP_NOMOVE);

          if (hdwp != NULL)
              EndDeferWindowPos(hdwp);

          return 0;
      }
     }
   }

   return DefWindowProcW(hWnd, wMsg, lParam1, lParam2);
}

static void    Control_DoInterface(CPanel* panel, HWND hWnd, HINSTANCE hInst)
{
    WNDCLASSEXW wc;
    MSG		msg;
    WCHAR appName[MAX_STRING_LEN];
    const WCHAR className[] = {'S','h','e','l','l','_','C','o','n','t','r','o',
        'l','_','W','n','d','C','l','a','s','s',0};

    LoadStringW(shell32_hInstance, IDS_CPANEL_TITLE, appName, sizeof(appName) / sizeof(appName[0]));

    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc = Control_WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(CPlApplet*);
    wc.hInstance = panel->hInst = hInst;
    wc.hIcon = LoadIconW( shell32_hInstance, MAKEINTRESOURCEW(IDI_SHELL_CONTROL_PANEL) );
    wc.hCursor = LoadCursorW( 0, (LPWSTR)IDC_ARROW );
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = className;
    wc.hIconSm = LoadImageW( shell32_hInstance, MAKEINTRESOURCEW(IDI_SHELL_CONTROL_PANEL), IMAGE_ICON,
                             GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);

    if (!RegisterClassExW(&wc)) return;

    CreateWindowExW(0, wc.lpszClassName, appName,
		    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		    CW_USEDEFAULT, CW_USEDEFAULT,
		    CW_USEDEFAULT, CW_USEDEFAULT,
		    hWnd, NULL, hInst, panel);
    if (!panel->hWnd) return;

    while (GetMessageW(&msg, panel->hWnd, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

static void Control_RegisterRegistryApplets(HWND hWnd, CPanel *panel, HKEY hkey_root, LPCWSTR szRepPath)
{
    WCHAR name[MAX_PATH];
    WCHAR value[MAX_PATH];
    HKEY hkey;

    if (RegOpenKeyW(hkey_root, szRepPath, &hkey) == ERROR_SUCCESS)
    {
        int idx = 0;

        for(;; ++idx)
        {
            DWORD nameLen = MAX_PATH;
            DWORD valueLen = MAX_PATH;

            if (RegEnumValueW(hkey, idx, name, &nameLen, NULL, NULL, (LPBYTE)value, &valueLen) != ERROR_SUCCESS)
                break;

            Control_LoadApplet(hWnd, value, panel);
        }
        RegCloseKey(hkey);
    }
}

static	void	Control_DoWindow(CPanel* panel, HWND hWnd, HINSTANCE hInst)
{
    static const WCHAR wszRegPath[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Cpls";
    HANDLE		h;
    WIN32_FIND_DATAW	fd;
    WCHAR		buffer[MAX_PATH];
    WCHAR *p;

    /* first add .cpl files in the system directory */
    GetSystemDirectoryW( buffer, MAX_PATH );
    p = buffer + strlenW(buffer);
    *p++ = '\\';
    lstrcpyW(p, L"*.cpl");

    if ((h = FindFirstFileW(buffer, &fd)) != INVALID_HANDLE_VALUE) {
        do {
	   lstrcpyW(p, fd.cFileName);
	   Control_LoadApplet(hWnd, buffer, panel);
	} while (FindNextFileW(h, &fd));
	FindClose(h);
    }

    /* now check for cpls in the registry */
    Control_RegisterRegistryApplets(hWnd, panel, HKEY_LOCAL_MACHINE, wszRegPath);
    Control_RegisterRegistryApplets(hWnd, panel, HKEY_CURRENT_USER, wszRegPath);

    Control_DoInterface(panel, hWnd, hInst);
}

#else
static	void	Control_DoWindow(CPanel* panel, HWND hWnd, HINSTANCE hInst)
{
    ShellExecuteW(NULL,
                  L"open",
                  L"explorer.exe",
                  L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}",
                  NULL,
                  SW_SHOWDEFAULT);
}
#endif

static	void	Control_DoLaunch(CPanel* panel, HWND hWnd, LPCWSTR wszCmd)
   /* forms to parse:
    *	foo.cpl,@sp,str
    *	foo.cpl,@sp
    *	foo.cpl,,str
    *	foo.cpl @sp
    *	foo.cpl str
    *   "a path\foo.cpl"
    */
{
    LPWSTR	buffer;
    LPWSTR	beg = NULL;
    LPWSTR	end;
    WCHAR	ch;
    LPWSTR       ptr;
    signed 	sp = -1;
    LPWSTR	extraPmtsBuf = NULL;
    LPWSTR	extraPmts = NULL;
    BOOL        quoted = FALSE;
    CPlApplet *applet;

    buffer = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(wszCmd) + 1) * sizeof(*wszCmd));
    if (!buffer) return;

    end = lstrcpyW(buffer, wszCmd);

    for (;;) {
        ch = *end;
        if (ch == '"') quoted = !quoted;
        if (!quoted && (ch == ' ' || ch == ',' || ch == '\0')) {
            *end = '\0';
            if (beg) {
                if (*beg == '@') {
                    sp = atoiW(beg + 1);
                } else if (*beg == '\0') {
                    sp = -1;
                } else {
                    extraPmtsBuf = beg;
                }
            }
            if (ch == '\0') break;
            beg = end + 1;
            if (ch == ' ') while (end[1] == ' ') end++;
        }
        end++;
    }
    while ((ptr = StrChrW(buffer, '"')))
	memmove(ptr, ptr+1, lstrlenW(ptr)*sizeof(WCHAR));

    /* now check for any quotes in extraPmtsBuf and remove */
    if (extraPmtsBuf != NULL)
    {
        beg = end = extraPmtsBuf;
        quoted = FALSE;

        for (;;) {
            ch = *end;
            if (ch == '"') quoted = !quoted;
            if (!quoted && (ch == ' ' || ch == ',' || ch == '\0')) {
                *end = '\0';
                if (beg) {
                    if (*beg != '\0') {
                        extraPmts = beg;
                    }
                }
                if (ch == '\0') break;
                beg = end + 1;
                if (ch == ' ') while (end[1] == ' ') end++;
            }
            end++;
        }

        while ((ptr = StrChrW(extraPmts, '"')))
            memmove(ptr, ptr+1, lstrlenW(ptr)*sizeof(WCHAR));

        if (extraPmts == NULL)
            extraPmts = extraPmtsBuf;
    }

    /* Now check if there had been a numerical value in the extra params */
    if ((extraPmts) && (*extraPmts == '@') && (sp == -1)) {
        sp = atoiW(extraPmts + 1);
    }

    TRACE("cmd %s, extra %s, sp %d\n", debugstr_w(buffer), debugstr_w(extraPmts), sp);

    applet = Control_LoadApplet(hWnd, buffer, panel);
    if (applet)
    {
#ifdef __REACTOS__
    ULONG_PTR cookie;
    BOOL bActivated;
#endif
        /* we've been given a textual parameter (or none at all) */
        if (sp == -1) {
            while ((++sp) != applet->count) {
                TRACE("sp %d, name %s\n", sp, debugstr_w(applet->info[sp].name));

                if (StrCmpIW(extraPmts, applet->info[sp].name) == 0)
                    break;
            }
        }

        if (sp >= applet->count) {
            WARN("Out of bounds (%u >= %u), setting to 0\n", sp, applet->count);
            sp = 0;
        }

#ifdef __REACTOS__
        bActivated = (applet->hActCtx != INVALID_HANDLE_VALUE ? ActivateActCtx(applet->hActCtx, &cookie) : FALSE);
#endif

        if (!applet->proc(applet->hWnd, CPL_STARTWPARMSW, sp, (LPARAM)extraPmts))
            applet->proc(applet->hWnd, CPL_DBLCLK, sp, applet->info[sp].data);

        Control_UnloadApplet(applet);

#ifdef __REACTOS__
    if (bActivated)
        DeactivateActCtx(0, cookie);
#endif

    }

    HeapFree(GetProcessHeap(), 0, buffer);
}

/*************************************************************************
 * Control_RunDLLW			[SHELL32.@]
 *
 */
void WINAPI Control_RunDLLW(HWND hWnd, HINSTANCE hInst, LPCWSTR cmd, DWORD nCmdShow)
{
    CPanel	panel;

    TRACE("(%p, %p, %s, 0x%08x)\n",
	  hWnd, hInst, debugstr_w(cmd), nCmdShow);

#ifndef __REACTOS__
    memset(&panel, 0, sizeof(panel));
    list_init( &panel.applets );
#endif

    if (!cmd || !*cmd) {
        Control_DoWindow(&panel, hWnd, hInst);
    } else {
        Control_DoLaunch(&panel, hWnd, cmd);
    }
}

/*************************************************************************
 * Control_RunDLLA			[SHELL32.@]
 *
 */
void WINAPI Control_RunDLLA(HWND hWnd, HINSTANCE hInst, LPCSTR cmd, DWORD nCmdShow)
{
    DWORD len = MultiByteToWideChar(CP_ACP, 0, cmd, -1, NULL, 0 );
    LPWSTR wszCmd = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (wszCmd && MultiByteToWideChar(CP_ACP, 0, cmd, -1, wszCmd, len ))
    {
        Control_RunDLLW(hWnd, hInst, wszCmd, nCmdShow);
    }
    HeapFree(GetProcessHeap(), 0, wszCmd);
}

/*************************************************************************
 * Control_FillCache_RunDLLW			[SHELL32.@]
 *
 */
HRESULT WINAPI Control_FillCache_RunDLLW(HWND hWnd, HANDLE hModule, DWORD w, DWORD x)
{
    FIXME("%p %p 0x%08x 0x%08x stub\n", hWnd, hModule, w, x);
    return S_OK;
}

/*************************************************************************
 * Control_FillCache_RunDLLA			[SHELL32.@]
 *
 */
HRESULT WINAPI Control_FillCache_RunDLLA(HWND hWnd, HANDLE hModule, DWORD w, DWORD x)
{
    return Control_FillCache_RunDLLW(hWnd, hModule, w, x);
}


#ifdef __REACTOS__
/*************************************************************************
 * RunDll_CallEntry16                [SHELL32.122]
 * the name is OK (when written with Dll, and not DLL as in Wine!)
 */
void WINAPI RunDll_CallEntry16( DWORD proc, HWND hwnd, HINSTANCE inst,
                                LPCSTR cmdline, INT cmdshow )
{
    FIXME( "proc %lx hwnd %p inst %p cmdline %s cmdshow %d\n",
           proc, hwnd, inst, debugstr_a(cmdline), cmdshow );
}
#endif

/*************************************************************************
 * CallCPLEntry16				[SHELL32.166]
 *
 * called by desk.cpl on "Advanced" with:
 * hMod("DeskCp16.Dll"), pFunc("CplApplet"), 0, 1, 0xc, 0
 *
 */
#ifndef __REACTOS__
DWORD WINAPI CallCPLEntry16(HMODULE hMod, FARPROC pFunc, DWORD dw3, DWORD dw4, DWORD dw5, DWORD dw6)
#else
DECLARE_HANDLE(FARPROC16);
LRESULT WINAPI CallCPLEntry16(HINSTANCE hMod, FARPROC16 pFunc, HWND dw3, UINT dw4, LPARAM dw5, LPARAM dw6)
#endif
{
    FIXME("(%p, %p, %08x, %08x, %08x, %08x): stub.\n", hMod, pFunc, dw3, dw4, dw5, dw6);
    return 0x0deadbee;
}
