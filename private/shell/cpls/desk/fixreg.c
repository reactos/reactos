///////////////////////////////////////////////////////////////////////////////
// FixReg.c
//
// PROBLEM:
//
// Win95 loaded extensions from only one place in the registry
//
//      location #1
//      HKLM\Software\Microsoft\Windows\CurrentVersion\Controls Folder\Display
//
// the flaw with this method is two types of extensions exist, ones that
// are display adapter specific (ie ATI or Diamond "custom" setting pages)
// and extensions that are not about your display, like the Plus! tab.
// we now load extsions from three places
//
//      location #1 (same old place)
//      HKLM\Software\Microsoft\Windows\CurrentVersion\Controls Folder\Display
//      this is where extensions go that are NOT display specific
//
//      location #2 (non hardware specific handlers)
//      HKLM\Software\Microsoft\Windows\CurrentVersion\Controls Folder\Advanced
//      this is where extensions go that ARE display specific, these
//      handlers get placed on the Advanced property sheet of ALL display adapters
//
//      location #3 (hardware specific handlers, PnP software key)
//      HKR\shellex\PropertySheetHandlers
//      this is where extensions go that ARE hardware specific
//
// SOLUTION:
//
// This code needs to move hardware-specific extensions from location #1
// to the correct software key (location #3) we do this in two ways
//
//      method #1
//      we have a hardcoded list if GUIDs,drivers used by display OEMs
//      if we find one of these GUIDs we move the handler to the
//      correct software key.
//
//      method #2
//      if the GUID is not in our list, we search all the display
//      adapters INF files trying to locate this GUID.  if we find
//      it we move the extension to the apropiate software key.
//      NOTE this assumes the OEM CPL extenstions are installed
//      in the driver INF, not via some custom setup program.
//
//  if cant associate a display device with a CPL extension
//  we leave the extension where it is.
//
///////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "winuser.h"
#pragma hdrstop
#include "cplext.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
BOOL MoveDisplayCPL(LPCTSTR szKey, LPCTSTR szGUID, LPCTSTR szOemCplInf, LPCTSTR szRegPath);
void MoveRegistryHandlers(LPCTSTR szRegPath);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef DEBUG
void DPF(char *sz,...)
{
    char ach[512]; 
    va_list va;
    va_start(va, sz);

    wvsprintfA(ach,sz,va);
    lstrcatA(ach,"\r\n");
    OutputDebugStringA(ach);

    va_end(va);
}
#else
    #define DPF ; / ## /
#endif

///////////////////////////////////////////////////////////////////////////////
// location of prop sheet hookers in the registry
///////////////////////////////////////////////////////////////////////////////

#pragma data_seg(".text")
static const TCHAR sc_szRegDisplay[]          = REGSTR_PATH_CONTROLSFOLDER TEXT("\\Display");
static const TCHAR sc_szRegAdvanced[]         = REGSTR_PATH_CONTROLSFOLDER TEXT("\\Device");
static const TCHAR sc_szRegDisplayHandlers[]  = REGSTR_PATH_CONTROLSFOLDER TEXT("\\Display\\shellex\\PropertySheetHandlers");
static const TCHAR sc_szRegAdvancedHandlers[] = REGSTR_PATH_CONTROLSFOLDER TEXT("\\Device\\shellex\\PropertySheetHandlers");
static const TCHAR sc_szTag[]                 = TEXT("Tag");
static const TCHAR sc_szNull[]                = TEXT("");
static const TCHAR sc_szOEMCPL[]              = TEXT("OEMCPL");
static const TCHAR sc_szOemCplInf[]           = TEXT("\\INF\\OEMCPL.INF");
#pragma data_seg()

///////////////////////////////////////////////////////////////////////////////
// FixupRegistryHandlers
///////////////////////////////////////////////////////////////////////////////
void FixupRegistryHandlers()
{
    TCHAR tmp[80];

    if (GetSystemMetrics(SM_CLEANBOOT) || !GetDisplayKey(0, tmp, sizeof(tmp)))
        return;

    MoveRegistryHandlers(sc_szRegDisplayHandlers);
    MoveRegistryHandlers(sc_szRegAdvancedHandlers);
}

///////////////////////////////////////////////////////////////////////////////
// MoveRegistryHandlers
///////////////////////////////////////////////////////////////////////////////
void MoveRegistryHandlers(LPCTSTR szRegPath)
{
    HKEY hkeyRoot;
    HKEY hkey;
    DWORD n;
    DWORD cb;
    BOOL  fMove;
    TCHAR guid[80];
    TCHAR key[MAX_PATH];
    TCHAR szOemCplInf[MAX_PATH];
    WIN32_FIND_DATA info;
    HANDLE h;
    DWORD dw;
    DWORD dwInf;
    DWORD dwTag;

    GetWindowsDirectory(szOemCplInf, sizeof(szOemCplInf));
    lstrcat(szOemCplInf, sc_szOemCplInf);

#ifdef DEBUG
    if (GetFileAttributes(szOemCplInf) == -1)
        lstrcpy(szOemCplInf, TEXT("c:\\ie\\private\\shell\\cpls\\desk\\oemcpl.inf"));
#endif

    ZeroMemory(&info, sizeof(info));
    if ((h = FindFirstFile(szOemCplInf, &info)) != INVALID_HANDLE_VALUE)
        FindClose(h);

    //
    // compute a tag value based on the when the OEMCPL.INF was modified
    // this way we will re-check CPL extensions when ever this file changes
    // and OEMs cant hardcode this value into thier INF
    //
    dwInf = info.nFileSizeHigh + info.nFileSizeLow  +
            info.ftCreationTime.dwLowDateTime       +
            info.ftCreationTime.dwHighDateTime      +
            info.ftLastWriteTime.dwLowDateTime      +
            info.ftLastWriteTime.dwHighDateTime     ;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, szRegPath, &hkeyRoot) != ERROR_SUCCESS)
        return;

again:
    for (n=0; RegEnumKey(hkeyRoot, n, key, sizeof(key)) == 0; n++)
    {
        if (RegOpenKey(hkeyRoot, key, &hkey) != ERROR_SUCCESS)
            continue;

        //
        // get the GUID of the CPL extension
        //
        guid[0] = 0; cb = SIZEOF(guid);
        if (key[0] == TEXT('{'))
            lstrcpy(guid, key);
        else
            RegQueryValueEx(hkey, NULL, NULL, NULL, (BYTE*)guid, &cb);

        //
        // see if we have already decided not to move this extension
        //
        dwTag = dwInf + ((DWORD*)guid)[0] + ((DWORD*)guid)[1];
        dw = 0; cb = sizeof(dw);
        RegQueryValueEx(hkey, sc_szTag, NULL, NULL, (BYTE*)&dw, &cb);

        if (dw == dwTag)
            continue;

        //
        // move this extension to the right display registry key
        //
        fMove = MoveDisplayCPL(key, guid, szOemCplInf, szRegPath);

        if (fMove)
        {
            //
            // delete the extension from the registry and restart from the top
            //
            RegCloseKey(hkey);
            RegDeleteKey(hkeyRoot, key);    //BUGBUG use SHRegDeleteKeyW on NT
            goto again; // start over
        }
        else
        {
            //
            // write the tag into the registry so we know not to move next time
            //
            RegSetValueEx(hkey,sc_szTag,0,REG_DWORD,(BYTE*)&dwTag,sizeof(dwTag));
            RegCloseKey(hkey);
        }
    }

    RegCloseKey(hkeyRoot);
}



///////////////////////////////////////////////////////////////////////////////
//
//  GetStr
//
//  get the Nth string in a comma separated string
//
///////////////////////////////////////////////////////////////////////////////
BOOL GetStr(LPCTSTR sz, int n, LPTSTR buf)
{
    for(; n>0; n--)
    {
        while(*sz && *sz!=TEXT(','))
            sz++;
        if (*sz)
            sz++;
    }

    while(*sz==TEXT(' '))
        sz++;

    if (*sz == 0)
        return FALSE;

    while(*sz && *sz!=TEXT(','))
        *buf++ = *sz++;

    *buf = 0;
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// GetDisplayKey
//
// get the Nth display adapters software registry key path
//
///////////////////////////////////////////////////////////////////////////////
BOOL GetDisplayKey(int i, LPTSTR szKey, DWORD cb)
{
    struct {
        DWORD   cb;
        TCHAR   DeviceName[32];
        TCHAR   DeviceString[128];
        DWORD   StateFlags;
        TCHAR   DeviceID[128];
        TCHAR   DeviceKey[128];
    }   dd;

    //
    // -1 is a special flag to get the primary display adapters key
    //
    if (i == -1)
    {
        ZeroMemory(&dd, sizeof(dd));
        dd.cb = sizeof(dd);

        for (i=0; EnumDisplayDevices(NULL, i, (DISPLAY_DEVICE*)&dd, 0); i++)
        {
            if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
                break;
        }
    }

    ZeroMemory(&dd, sizeof(dd));
    dd.cb = sizeof(dd);
    EnumDisplayDevices(NULL, i, (DISPLAY_DEVICE*)&dd, 0);
    lstrcpyn(szKey, dd.DeviceKey, cb);

    return szKey[0] != 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// GetDisplayDriverFileName
//
// given the registry path of a display adapter return the display driver
// file name. this code needs to be different on Win9x and WinNT
//
///////////////////////////////////////////////////////////////////////////////
BOOL GetDisplayDriverFileName(LPCTSTR szDeviceKey, LPTSTR szDriver, DWORD cb)
{
#ifdef WINNT
    //
    // on NT we need to read from a different key, etc
    //
    return FALSE;
#else
    //
    // on Win9x read the DEFAULT/drv key
    //
    HKEY  hkey;
    TCHAR key[128];

    lstrcpy(key, szDeviceKey);
    lstrcat(key, TEXT("\\DEFAULT"));

    if (RegOpenKey(HKEY_LOCAL_MACHINE, key, &hkey) == ERROR_SUCCESS)
    {
        szDriver[0] = 0;
        RegQueryValueEx(hkey, TEXT("drv"), NULL, NULL, (BYTE*)szDriver, &cb);
        RegCloseKey(hkey);
    }
    return szDriver[0] != 0;
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
// GetDisplayInstallInfFileName
//
// given the registry path of a display adapter return the driver install INF
// file name. this code needs to be different on Win9x and WinNT
//
///////////////////////////////////////////////////////////////////////////////
BOOL GetDisplayInstallInfFileName(LPCTSTR szDeviceKey, LPTSTR szInfPath, DWORD cbSize)
{
#ifdef WINNT
    //
    // on NT we need to read from a different key, etc
    //
    return FALSE;
#else
    HKEY  hkey;
    TCHAR inf[MAX_PATH];
    DWORD dw;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, szDeviceKey, &hkey) != ERROR_SUCCESS)
        return FALSE;

    inf[0] = 0; dw = sizeof(inf);
    RegQueryValueEx(hkey, TEXT("InfPath"), NULL, NULL, (BYTE*)inf, &dw);
    RegCloseKey(hkey);

    GetWindowsDirectory(szInfPath, cbSize);
    lstrcat(szInfPath, "\\INF\\");
    lstrcat(szInfPath, inf);

    if ((dw = GetFileAttributes(szInfPath)) == -1)
    {
        GetWindowsDirectory(szInfPath, cbSize);
        lstrcat(szInfPath, "\\INF\\OTHER\\");
        lstrcat(szInfPath, inf);
        dw = GetFileAttributes(szInfPath);
    }

    return dw != -1;
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
//  MapFile - memory map a file into memory
//
///////////////////////////////////////////////////////////////////////////////

LPVOID MapFile(LPCTSTR szFile, DWORD * pFileLength)
{
    LPVOID pFile;
    HANDLE hFile;
    HANDLE h;
    DWORD  FileLength;

    hFile = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (hFile == INVALID_HANDLE_VALUE)
        return 0;

    FileLength = (LONG)GetFileSize(hFile, NULL);

    if (pFileLength)
       *pFileLength = FileLength;

    h = CreateFileMapping(hFile, NULL, PAGE_WRITECOPY, 0, 0, NULL);

    CloseHandle(hFile);

    if (!h)
        return 0;

    pFile = MapViewOfFile(h, FILE_MAP_COPY, 0, 0, 0);
    CloseHandle(h);

    if (pFile == NULL)
        return 0;

    return pFile;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FindStringInFile
//
///////////////////////////////////////////////////////////////////////////////
BOOL FindStringInFile(LPCTSTR szFile, LPCTSTR szString)
{
    LPVOID  pFile;
    DWORD   FileLength;
    int     len;
    int	    i;
    LPCSTR  string;
    LPCSTR  file;
    char    str[128];
    BOOL    result = FALSE;

#ifdef UNICODE
    char    tmp[128];
    WideCharToMultiByte(CP_ACP, 0, szString, -1, tmp, ARRAYSIZE(tmp), NULL, NULL);
    string = tmp;
#else
    string = szString;
#endif

    DPF("FindStringInFile: '%s' in %s", szString, szFile);

    pFile = MapFile(szFile, &FileLength);

    if (pFile == NULL)
        return FALSE;

    len = lstrlenA(string);
    file = (char *)pFile;

    for (i=0; i<(int)FileLength-len; i++)
    {
        if (file[i] == string[0] && file[i+len-1] == string[len-1])
        {
            CopyMemory(str, file+i, len);
            str[len]=0;

            if (lstrcmpiA(str,string) == 0)
            {
                result = TRUE;
                break;
            }
        }
    }

    if (result)
        DPF("   Found!");
    else
        DPF("   not found");

    UnmapViewOfFile(pFile);
    return result;
}

///////////////////////////////////////////////////////////////////////////////
//
// CompareStr
//
// compare two strings, return TRUE if the strings match
//
// if the first string ends in a '*' then only compare the characters
// up to the * (ie foo* will match foobar and foosmag)
//
///////////////////////////////////////////////////////////////////////////////
BOOL CompareStr(LPCTSTR szMatch, LPCTSTR szCmp)
{
    int len = lstrlen(szMatch);

    if (szMatch[len-1] == TEXT('*'))
    {
        TCHAR tmp[80];
        lstrcpy(tmp,szCmp);
        tmp[len-1] = TEXT('*');
        tmp[len] = 0;
        return lstrcmpi(szMatch, tmp) == 0;
    }
    else
    {
        return lstrcmpi(szMatch, szCmp) == 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// FindDisplayDriver
//
// walk all the display adapters in the system looking for one that
// uses a given driver.
//
// ENTRY
//      szDriver    - driver(s) to look for, this is a comma separated
//                    list of driver filenames or the following special forms
//
//                    "DISPLAY" - will match the primary display adapter
//
//                    "*"       - will match "all" display adapters
//
//                    "abc*"    - will match any driver starting with "abc"
//
//      n           - adapter number to start the search (0 = first adapter)
//
//      szDriverKey - the driver software registry key of any matching
//                    display adapter will be paced here
//
// RETURNS
//      0           - no adapter uses the specifed driver(s)
//      >0          - the adapter number where the search should continue
//
///////////////////////////////////////////////////////////////////////////////

int FindDisplayDriver(LPCTSTR szDrivers, int n, LPTSTR szDriverKey)
{
    //
    //  "DISPLAY" means the primary/vga display driver
    //
    if (lstrcmpi(szDrivers, TEXT("DISPLAY")) == 0)
    {
        if (n == 0 && GetDisplayKey(-1, szDriverKey, MAX_PATH))
            return 1;
        else
            return 0;
    }

    //
    //  "*" means all display(s) move this guy to the common section
    //
    if (lstrcmpi(szDrivers, TEXT("*")) == 0)
    {
        if (n == 0 && lstrcpy(szDriverKey, sc_szRegAdvanced))
            return 1;
        else
            return 0;
    }

    //
    //  walk all display keys looking for a driver match
    //
    for (; GetDisplayKey(n, szDriverKey, MAX_PATH); n++)
    {
        TCHAR drv[80];
        TCHAR str[80];
        int  i;

        if (GetDisplayDriverFileName(szDriverKey, drv, sizeof(drv)))
        {
            for (i=0; GetStr(szDrivers, i, str); i++)
            {
                DPF("    compare '%s' with '%s'",str,drv);

                if (CompareStr(str, drv))
                {
                    return n+1; // start here next time.
                }
            }
        }
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int FindDisplayGUID(LPCTSTR szGUID, int n, LPTSTR szDriverKey)
{
    //
    //  walk all display keys looking for a GUID match
    //
    for (; GetDisplayKey(n, szDriverKey, MAX_PATH); n++)
    {
        HKEY hkey;
        TCHAR key[MAX_PATH];
        TCHAR guid[80];
        DWORD cb;
        int i;

        //
        // search the existing handlers for the GUID
        //
        wsprintf(key, TEXT("%s\\shellex\\PropertySheetHandlers"), szDriverKey);

        if (RegOpenKey(HKEY_LOCAL_MACHINE, key, &hkey) == ERROR_SUCCESS)
        {
            for (i=0; RegEnumKey(hkey, i, key, sizeof(key)) == 0; i++)
            {
                guid[0] = 0; cb = sizeof(guid);

                if (key[0] == TEXT('{'))
                    lstrcpy(guid, key);
                else
                    RegQueryValue(hkey, key, guid, &cb);

                if (lstrcmpi(guid, szGUID) == 0)
                {
                    RegCloseKey(hkey);
                    return n+1;
                }
            }

            RegCloseKey(hkey);
        }

        //
        // search the driver install INF looking for the GUID
        //
        if (GetDisplayInstallInfFileName(szDriverKey, key, sizeof(key)))
        {
            if (FindStringInFile(key, szGUID))
            {
                return n+1;
            }
        }
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

BOOL MoveCPL(LPCTSTR szKey, LPCTSTR szGUID, LPCTSTR szDriverKey)
{
    BOOL result = FALSE;
    HKEY hkey;
    TCHAR key[MAX_PATH];

    wsprintf(key, TEXT("%s\\shellex\\PropertySheetHandlers\\%s"), szDriverKey, szKey);

    if (RegCreateKey(HKEY_LOCAL_MACHINE, key, &hkey) == ERROR_SUCCESS)
    {
        DPF("    MoveCPL: '%s' to HKLM\\%s",szKey,szDriverKey);

        if (lstrcmp(szKey, szGUID) != 0)
        {
            RegSetValue(hkey, NULL, REG_SZ, szGUID, 0);
        }

        RegCloseKey(hkey);
        result = TRUE;
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

BOOL MoveDisplayCPL(LPCTSTR szKey, LPCTSTR szGUID, LPCTSTR szOemCplInf, LPCTSTR szRegPath)
{
    BOOL    fMove = FALSE;
    TCHAR   szDriverKey[MAX_PATH];
    TCHAR   szDrivers[MAX_PATH];
    int     i;

    if (GetPrivateProfileString(sc_szOEMCPL, szGUID, sc_szNull, szDrivers, ARRAYSIZE(szDrivers), szOemCplInf))
    {
        DPF("GUID found in OEMCPL.INF %s=%s",szGUID,szDrivers);

        if (lstrcmpi(szRegPath,sc_szRegAdvancedHandlers) == 0 &&
            lstrcmpi(szDrivers, TEXT("*")) == 0)
        {
            return FALSE;   // dont move to the same key
        }

        for (i=0; i=FindDisplayDriver(szDrivers, i, szDriverKey); )
        {
            MoveCPL(szKey, szGUID, szDriverKey);
            fMove = TRUE;
        }

        if (!fMove && FindDisplayDriver(TEXT("DISPLAY"), 0, szDriverKey))
        {
            MoveCPL(szKey, szGUID, szDriverKey);
            fMove = TRUE;
        }
    }
    else
    {
        DPF("GUID not found in OEMCPL.INF %s",szGUID);

        for (i=0; i=FindDisplayGUID(szGUID, i, szDriverKey); )
        {
            MoveCPL(szKey, szGUID, szDriverKey);
            fMove = TRUE;
        }
    }

    return fMove;
}

///////////////////////////////////////////////////////////////////////////////
//
// NukeDisplaySettings
//
// PROBLEM:
//
// if a bad display driver, or bad settings, or the user selects
// a bad refresh rate or mode, the system can become unusable
// or crash.  When this happens the system boots in "SafeMode".
// in SafeMode we might not know exactly what display device or
// driver is causing the problem.
//
// SOLUTION:
//
// walk all places in the registy where device settings and
// refresh rates are stored and set them back to a default
// state (640x480, 16 Color, Default Refresh) this will allow
// the user to boot the system in normal mode and fix the problem
// any secondary displays are disbled, so the system will boot up
// with only a single monitor.
//
// this function is only called when the user hits Apply
// when running in a "SafeMode" situation
//
// settings are stored in the following places on Win9x
//
//  HKEY_CURRENT_CONFIG\Display\Settings
//  HKEY_CURRENT_USER\Display\Settings
//  HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\Class\Display
//
//      AttachToDesktop     - set to 0
//      BitsPerPixel        - set to 4
//      Resolution          - set to 640,480
//      RefreshRate         - set to 0 (or deleted)
//
// settings are stored in the following places on NT5
//
//  HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
//  HKEY_CURRENT_CONFIG\System\CurrentControlSet\Services
//
//      DefaultSettings.BitsPerPel
//      DefaultSettings.XResolution
//      DefaultSettings.YResolution
//      DefaultSettings.VRefresh
//      Attach.ToDesktop
//
///////////////////////////////////////////////////////////////////////////////

void _NukeDisplaySettings(HKEY root, LPTSTR key);

void NukeDisplaySettings()
{
    DPF("NukeDisplaySettings");

#ifdef WINNT
    _NukeDisplaySettings(HKEY_LOCAL_MACHINE,  TEXT("System\\CurrentControlSet\\Services"));
    _NukeDisplaySettings(HKEY_CURRENT_CONFIG, TEXT("System\\CurrentControlSet\\Services"));
#else
    _NukeDisplaySettings(HKEY_CURRENT_CONFIG, TEXT("Display\\Settings"));
    _NukeDisplaySettings(HKEY_CURRENT_USER,   TEXT("Display\\Settings"));
    _NukeDisplaySettings(HKEY_LOCAL_MACHINE,  TEXT("System\\CurrentControlSet\\Services\\Class\\Display"));

    // we might have missed some settings because HKEY_CURRENT_CONFIG or
    // HKEY_CURRENT_USER might not be set right
    // should we walk *all* configs on the machine?
    // should we walk *all* user profiles on the machine?
    _NukeDisplaySettings(HKEY_LOCAL_MACHINE,  TEXT("Config"));
    _NukeDisplaySettings(HKEY_USERS,TEXT(""));
#endif
}

void _NukeValueDW(HKEY hkey, LPTSTR szValue, DWORD dwNuke)
{
    DWORD dw;
    DWORD cb = sizeof(dw);
    DWORD type = REG_DWORD;

    if (RegQueryValueEx(hkey, szValue, NULL, &type, (BYTE*)&dw, &cb) == 0 && type == REG_DWORD)
    {
        DPF("*** Nuking DWORD value %s=%d ==> %d", szValue, dw, dwNuke);
        RegSetValueEx(hkey, szValue, 0, REG_DWORD, (BYTE*)&dwNuke, sizeof(DWORD));
    }
}

void _NukeValueSZ(HKEY hkey, LPTSTR szValue, LPTSTR szNuke)
{
    TCHAR ach[80];
    DWORD cb = sizeof(ach);
    DWORD type = REG_SZ;

    if (RegQueryValueEx(hkey, szValue, NULL, &type, (BYTE*)ach, &cb) == 0 && type == REG_SZ)
    {
        if (szNuke == NULL)
        {
            DPF("*** Nuking string value %s=%s", szValue, ach);
            RegDeleteValue(hkey, szValue);
        }
        else
        {
            DPF("*** Nuking string value %s=%s ==> %s", szValue, ach, szNuke);
            RegSetValueEx(hkey, szValue, 0, REG_SZ, (BYTE*)szNuke, lstrlen(szNuke));
        }
    }
}

void _NukeDisplaySettings(HKEY root, LPTSTR key)
{
    HKEY hk;
    TCHAR ach[128];
    int  i;

    DPF("NukeDisplaySettings: %08X\\%s", root, key);

    if (RegOpenKey(root, key, &hk) == 0)
    {
        // we need to nuke different values on NT vs Win9x
#ifdef WINNT
        _NukeValueDW(hk, TEXT("DefaultSettings.BitsPerPel"),  4);
        _NukeValueDW(hk, TEXT("DefaultSettings.XResolution"), 640);
        _NukeValueDW(hk, TEXT("DefaultSettings.YResolution"), 480);
        _NukeValueDW(hk, TEXT("DefaultSettings.VRefresh"),    60);
        _NukeValueDW(hk, TEXT("Attach.ToDesktop"),            0);
#else
        _NukeValueSZ(hk, TEXT("AttachToDesktop"), TEXT("0"));
        _NukeValueSZ(hk, TEXT("BitsPerPixel"),    TEXT("4"));
        _NukeValueSZ(hk, TEXT("Resolution"),      TEXT("640,480"));

        //
        // only keep the refresh settings in the DEFAULT key
        // delete all others.
        //
        if (lstrcmpi(key, TEXT("DEFAULT")) == 0)
            _NukeValueSZ(hk, TEXT("RefreshRate"), TEXT("0"));
        else
            _NukeValueSZ(hk, TEXT("RefreshRate"), NULL);
#endif
        for (i=0; RegEnumKey(hk, i, ach, sizeof(ach)) == 0; i++)
        {
            _NukeDisplaySettings(hk, ach);
        }
        RegCloseKey(hk);
    }
}
