// app.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <windows.h>
#include <stdio.h>
#include <shlobj.h>
#include <shfolder.h>
#include <windowsx.h>
#include <resource.h>

#define chHANDLE_DLGMSG(hwnd, message, fn)                           \
   case (message): return (SetDlgMsgResult(hwnd, uMsg,               \
      HANDLE_##message((hwnd), (wParam), (lParam), (fn))))


#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))
#define SHAnsiToUnicode(psz, pwsz, cchwsz)  MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, cchwsz);
#define SHUnicodeToAnsi(pwsz, psz, cchsz)   WideCharToMultiByte(CP_ACP, 0, pwsz, -1, psz, cchsz, NULL, NULL);

#ifdef UNICODE

#else
#define SHTCharToUnicode(t, w, cc) SHAnsiToUnicode(t, w, cc)
#endif

#define Q(x) #x,x
#define HKCU HKEY_CURRENT_USER
#define HKLM HKEY_LOCAL_MACHINE
struct {
    CHAR *pszName;
    UINT csidl;
    HKEY hkey;
    char *pszRegValue;
} Folders[] = {
   Q(CSIDL_DESKTOP), 0,0,
   Q(CSIDL_INTERNET),0,0,
   Q(CSIDL_PROGRAMS), HKCU,"Programs",
   Q(CSIDL_CONTROLS), 0,0,
   Q(CSIDL_PRINTERS), 0,0,
   Q(CSIDL_PERSONAL), HKCU, "Personal",
   Q(CSIDL_FAVORITES), HKCU, "Favorites",
   Q(CSIDL_STARTUP), HKCU, "Startup",
   Q(CSIDL_RECENT), HKCU, "Recent",
   Q(CSIDL_SENDTO), HKCU, "SendTo", 
   Q(CSIDL_BITBUCKET),0,0,
   Q(CSIDL_STARTMENU), HKCU, "Start Menu",
   Q(CSIDL_DESKTOPDIRECTORY), HKCU, "Desktop",
   Q(CSIDL_DRIVES), 0,0,
   Q(CSIDL_NETWORK), 0,0,
   Q(CSIDL_NETHOOD), HKCU, "NetHood",
   Q(CSIDL_FONTS), HKCU, "Fonts",
   Q(CSIDL_TEMPLATES), HKCU, "Templates",
   Q(CSIDL_COMMON_STARTMENU),HKLM, "Common Start Menu",
   Q(CSIDL_COMMON_PROGRAMS), HKLM, "Common Programs",
   Q(CSIDL_COMMON_STARTUP), HKLM, "Common Startup",
   Q(CSIDL_COMMON_DESKTOPDIRECTORY), HKLM, "Common Desktop",
   Q(CSIDL_APPDATA), HKCU, "AppData",
   Q(CSIDL_PRINTHOOD), HKCU, "PrintHood",
   Q(CSIDL_LOCAL_APPDATA), HKCU, "Local AppData",
   Q(CSIDL_ALTSTARTUP), HKCU, "AltStartup",
   Q(CSIDL_COMMON_ALTSTARTUP), HKLM, "Common AltStartup",
   Q(CSIDL_COMMON_FAVORITES), HKLM, "Common Favorites",
   Q(CSIDL_INTERNET_CACHE), HKCU, "Cache",
   Q(CSIDL_COOKIES), HKCU, "Cookies",
   Q(CSIDL_HISTORY), HKCU, "History",
   Q(CSIDL_COMMON_APPDATA), HKLM, "Common AppData",
   Q(CSIDL_WINDOWS),0,0,
   Q(CSIDL_SYSTEM),0,0,
   Q(CSIDL_PROGRAM_FILES),0,0,
   Q(CSIDL_MYPICTURES),HKCU, "My Pictures",
   Q(CSIDL_PROFILE),0,0,
   Q(CSIDL_SYSTEMX86),0,0,
   Q(CSIDL_PROGRAM_FILESX86),0,0,
   Q(CSIDL_PROGRAM_FILES_COMMON),0,0,
   Q(CSIDL_PROGRAM_FILES_COMMONX86),0,0,
   Q(CSIDL_COMMON_TEMPLATES),HKLM, "Common Templates",
   Q(CSIDL_COMMON_DOCUMENTS), HKLM, "Common Documents",
   Q(CSIDL_COMMON_ADMINTOOLS), HKLM, "Common Administrative Tools",
   Q(CSIDL_ADMINTOOLS), HKCU, "Administrative Tools",
/*
*/
    "Invalid",0xff,0,0,
};

HANDLE GetCurrentUserToken()
{
    HANDLE hToken;
    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken) ||
        OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return hToken;
    return NULL;
}

const CHAR c_szUSF[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders";
const CHAR c_szSF[]  = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";


void AddToList(HWND hwndList, UINT iItem, HRESULT hr, UINT iFolder, BOOL fUnicode, LPBYTE pzPath, BOOL fCreate)
{
    LVITEMA lva;
    LVITEMW lvw;
    CHAR sz[MAX_PATH];
    WCHAR wsz[MAX_PATH];
    
    ZeroMemory(&lva, sizeof(lva));
    ZeroMemory(&lvw, sizeof(lvw));
    lva.mask = lvw.mask = LVIF_TEXT;
    lva.iItem = lvw.iItem = iItem;

    wsprintf(sz, "%x", hr);
    lva.pszText = sz;
    lva.iSubItem = 0;
    if (iItem != SendMessageA(hwndList, LVM_INSERTITEMA, 0, (LPARAM)&lva))
        MessageBox(0, "debug",0, MB_OK);

    if (fUnicode)
    {
        lvw.pszText = (WCHAR*)pzPath;
        lvw.iSubItem = 1;
        SendMessageW(hwndList,LVM_SETITEMW,0, (LPARAM)&lvw);
        lva.pszText = "1";
        lva.iSubItem = 4;
        SendMessageA(hwndList, LVM_SETITEMA, 0, (LPARAM)&lva);

    } 
    else
    {
        lva.pszText = (CHAR*)pzPath;
        lva.iSubItem = 1;
        SendMessageA(hwndList,LVM_SETITEMA,0, (LPARAM)&lva);
        lva.pszText = "0";
        lva.iSubItem = 4;
        SendMessageA(hwndList,LVM_SETITEMA, 0, (LPARAM)&lva);
    }

    if (Folders[iFolder].hkey) 
    {
        HKEY hkey;
        DWORD dwType;
        DWORD dwSize;
        wsz[0] = 0;
        sz[0] = 0;
        if (ERROR_SUCCESS == RegOpenKey(Folders[iFolder].hkey, c_szUSF, &hkey))
        {
            if (fUnicode) 
            {
                WCHAR wszRegValue[MAX_PATH];
                dwSize = sizeof(sz);
                SHAnsiToUnicode( Folders[iFolder].pszRegValue, wszRegValue, MAX_PATH);
                RegQueryValueExW(hkey, wszRegValue, 0, &dwType, (LPBYTE) wsz, &dwSize);
                lvw.iSubItem = 2;
                lvw.pszText = wsz;
                SendMessageW(hwndList, LVM_SETITEMW, 0, (LPARAM)&lvw);
            } 
            else
            {
                DWORD dwSize = sizeof(sz);
                RegQueryValueExA(hkey, Folders[iFolder].pszRegValue, 0, &dwType, (LPBYTE)sz,&dwSize );
                lva.iSubItem = 2;
                lva.pszText = sz;
                SendMessageA(hwndList, LVM_SETITEMA, 0, (LPARAM)&lva);
            }


            RegCloseKey(hkey);
        }
        if (ERROR_SUCCESS == RegOpenKey(Folders[iFolder].hkey, c_szSF, &hkey))
        {
            if (fUnicode) 
            {
                WCHAR wszRegValue[MAX_PATH];
                dwSize = sizeof(wsz);
                SHAnsiToUnicode( Folders[iFolder].pszRegValue, wszRegValue, MAX_PATH);
                RegQueryValueExW(hkey, wszRegValue, 0, &dwType, (LPBYTE) wsz, &dwSize);
                lvw.iSubItem = 3;
                lvw.pszText = wsz;
                SendMessageW(hwndList, LVM_SETITEMW, 0, (LPARAM)&lvw);
            } 
            else
            {
                DWORD dwSize = sizeof(sz);
                RegQueryValueExA(hkey, Folders[iFolder].pszRegValue, 0, &dwType, (LPBYTE)sz, &dwSize);
                lva.iSubItem = 3;
                lva.pszText = sz;
                SendMessageA(hwndList, LVM_SETITEMA, 0, (LPARAM)&lva);
            }


            RegCloseKey(hkey);
        }


    }
    else
    {
        lva.pszText = "N/A";
        lva.iSubItem = 2;
        SendMessageA(hwndList, LVM_SETITEMA, 0, (LPARAM)&lva);
        lva.iSubItem = 3;
        SendMessageA(hwndList, LVM_SETITEMA, 0, (LPARAM)&lva);
    }

    lva.iSubItem = 5;
    if (fCreate)
        lva.pszText = "1";
    else
        lva.pszText = "0";
    SendMessageA(hwndList, LVM_SETITEMA, 0, (LPARAM)&lva);

}

void CheckFolders( HWND hList)
{
    int i;
    int j;
    static HINSTANCE hmod;
    PFNSHGETFOLDERPATHA GetFolderPathA;
    PFNSHGETFOLDERPATHW GetFolderPathW;
    CHAR szDll[] = "shfolder.dll";
    hmod = LoadLibrary(szDll);
    if( !hmod) {
        MessageBoxA(0,"couldn't find your dll %s\n", 0, MB_OK);
        ExitProcess(1);
    }
    GetFolderPathA = (PFNSHGETFOLDERPATHA) GetProcAddress( hmod, "SHGetFolderPathA");
    GetFolderPathW = (PFNSHGETFOLDERPATHW) GetProcAddress( hmod, "SHGetFolderPathW");
    // SendMessageA(hList, LB_RESETCONTENT,0,0);
    SendMessage(hList, LVM_DELETEALLITEMS,0,0);
    
    

    for ( i=0,j=0; i< ARRAYSIZE(Folders); i++)
    {
        CHAR szPath[MAX_PATH];
        WCHAR wszPath[MAX_PATH];
        HRESULT hr;
        CHAR szOut[MAX_PATH];
        WCHAR wszOut[MAX_PATH];
        WCHAR wszConv[MAX_PATH];
        LPBYTE pzPath;
        int k;
        
        for (k=0; k<6; k++) 
        {
            BOOL fCreate = (k == 3) || (k == 2);
            if (k%2) 
            {
                hr = GetFolderPathW(NULL, Folders[i].csidl |fCreate, NULL,0, wszPath);
                pzPath = (LPBYTE) wszPath;
            }    
            else
            {
                hr = GetFolderPathA(NULL, Folders[i].csidl | fCreate, NULL, 0, szPath);
                pzPath = (LPBYTE) szPath;
            }
            
            AddToList(hList, j++ ,hr, i, k%2, pzPath, fCreate);

        }
    }
}

BOOL Dlg_OnInitDialog (HWND hwnd, HWND hwndFocus,
   LPARAM lParam) 
{

   RECT rc;
   LVCOLUMN lvc;
   HWND hwndList = GetDlgItem(hwnd, IDC_LIST1);

   ZeroMemory(&lvc, sizeof(lvc));
   lvc.mask = LVCF_FMT | LVCF_ORDER |LVCF_SUBITEM | LVCF_TEXT;
   lvc.fmt = LVCFMT_LEFT;
   
   lvc.pszText = "HRESULT";
   lvc.iOrder = 0;
   lvc.iSubItem = 0;
   ListView_InsertColumn(hwndList, 0, &lvc);
 

   lvc.pszText = "Value";
   lvc.iOrder = 1;
   lvc.iSubItem = 1;
   ListView_InsertColumn(hwndList, 1, &lvc);

   lvc.pszText ="USF value";
   lvc.iOrder =2 ;
   lvc.iSubItem = 2;
   ListView_InsertColumn(hwndList, 2, &lvc);

   lvc.pszText = "SF value";
   lvc.iOrder = 3;
   lvc.iSubItem = 3;
   ListView_InsertColumn(hwndList, 3, &lvc);

   lvc.pszText = "Unicode";
   lvc.iOrder = 4;
   lvc.iSubItem = 4;
   ListView_InsertColumn(hwndList, 4, &lvc);
   
   lvc.pszText = "Create";
   lvc.iOrder = 5;
   lvc.iSubItem = 5;
   ListView_InsertColumn(hwndList, 5, &lvc); 

    
   // Associate an icon with the dialog box.
   CheckFolders(hwndList);

   GetClientRect(hwnd, &rc);
   SetWindowPos(GetDlgItem(hwnd, IDC_LIST1), NULL,
       0, 0, rc.right, rc.bottom, SWP_NOZORDER);
   return(TRUE);
}

void Dlg_OnSize (HWND hwnd, UINT state, int cx, int cy) {
//   SetWindowPos(GetDlgItem(hwnd, IDC_LIST1), NULL, 0, 0,
      // cx, cy, SWP_NOZORDER);
}

void Dlg_OnCommand (HWND hwnd, int id, HWND hwndCtl,
   UINT codeNotify) 
{

   switch (id) 
   {
      case IDCANCEL:
         EndDialog(hwnd, id);
         break;

      case IDOK:
         // Call the recursive routine to walk the tree.
         CheckFolders(GetDlgItem(hwnd, IDC_LIST1));
         break;
   }
}



BOOL CALLBACK Dlg_Proc (HWND hwnd, UINT uMsg,
   WPARAM wParam, LPARAM lParam) 
{

   switch (uMsg) {
      chHANDLE_DLGMSG(hwnd, WM_INITDIALOG,  Dlg_OnInitDialog);
      chHANDLE_DLGMSG(hwnd, WM_SIZE,        Dlg_OnSize);
      chHANDLE_DLGMSG(hwnd, WM_COMMAND,     Dlg_OnCommand);
   }
   return(FALSE);
}

int  WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
        LPSTR pszArgs, INT command)
{
#if 0
    if (argc > 1)
        SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, argv[1], NULL);

    Sleep(6000);
#endif
    CoInitialize(0);

    DialogBox( hInstance, MAKEINTRESOURCE(IDD_DIALOG1), 0, Dlg_Proc);
    GetLastError();

#if 0
    SHFILEINFO fi;
    SHGetFileInfo(TEXT("\\\\chrisg06\\public"), FILE_ATTRIBUTE_DIRECTORY, &fi, sizeof(fi), SHGFI_USEFILEATTRIBUTES | SHGFI_DISPLAYNAME | SHGFI_ICON);

    {
        TCHAR szPath[MAX_PATH];

        SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, GetCurrentUserToken(), 0, szPath);

        SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, 0, szPath);
        SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES_COMMON, NULL, 0, szPath);

        SHGetFolderPath(NULL, CSIDL_COMMON_DOCUMENTS | CSIDL_FLAG_CREATE, NULL, 0, szPath);
        SHGetFolderPath(NULL, CSIDL_COMMON_TEMPLATES | CSIDL_FLAG_CREATE, NULL, 0, szPath);

        SHGetFolderPath(NULL, CSIDL_RECENT, NULL, 0, szPath);
        SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, 0, szPath);
        SHGetFolderPath(NULL, CSIDL_MYPICTURES | CSIDL_FLAG_CREATE, NULL, 0, szPath);
        SHGetFolderPath(NULL, CSIDL_MYPICTURES, NULL, 0, szPath);


        // SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, szPath);
        // SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPath);
    }

#endif
    CoUninitialize();
    return 0;
}


