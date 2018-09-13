//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#include "grpconv.h"
#include "util.h"
#include "rcids.h"
#include "group.h"
#include "gcinst.h"
#include <shellp.h>
#include <windowsx.h>
#include <regstr.h>
// #include <vmdapriv.h>

// we only call ImmDisableIME if we can sucessfully LoadLibrary
// and GetProcAddress it, since this function did not exist on NT4
// and win95.
extern BOOL WINAPI ImmDisableIME(DWORD);


//---------------------------------------------------------------------------
// Global to this file only...

const TCHAR g_szGRP[] = TEXT("grp");
const TCHAR c_szClassInfo[]     = STRINI_CLASSINFO;
const TCHAR g_szMSProgramGroup[] = TEXT("MSProgramGroup");
const TCHAR g_szSpacePercentOne[] = TEXT(" %1");
const TCHAR c_szGroups[] = TEXT("Groups");
const TCHAR c_szSettings[] = TEXT("Settings");
const TCHAR c_szWindow[] = TEXT("Window");
const TCHAR c_szNULL[] = TEXT("");
const TCHAR c_szRegGrpConv[] = REGSTR_PATH_GRPCONV;
const TCHAR c_szCLSID[] = TEXT("CLSID");
const CHAR c_szReporter[] = "reporter.exe -q";
const TCHAR c_szCheckAssociations[] = TEXT("CheckAssociations");
const TCHAR c_szRegExplorer[] = REGSTR_PATH_EXPLORER;
const TCHAR c_szDotDoc[] = TEXT(".doc");
const TCHAR c_szWordpadDocument[] = TEXT("wordpad.document");
const TCHAR c_szWordpadDocumentOne[] = TEXT("wordpad.document.1");
const TCHAR c_szUnicodeGroups[] = TEXT("UNICODE Program Groups");
const TCHAR c_szAnsiGroups[] = TEXT("Program Groups");
const TCHAR c_szCommonGroups[] = TEXT("SOFTWARE\\Program Groups");

HKEY g_hkeyGrpConv;

//---------------------------------------------------------------------------
// Global to the app...
HINSTANCE g_hinst;
TCHAR     g_szStartGroup[MAXGROUPNAMELEN + 1];
UINT      GC_TRACE = 0;       // Default no tracing
BOOL      g_fShowUI = TRUE;

// Forward declarations

int WinMainT(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow);


//---------------------------------------------------------------------------
BOOL InitApplication(HINSTANCE hInstance)
{
    TCHAR szTypeName[CCHSZNORMAL];
    TCHAR szPath[MAX_PATH];

    // Register this app as being able to handle progman groups.
    LoadString(hInstance, IDS_GROUPTYPENAME, szTypeName, ARRAYSIZE(szTypeName));
    // Get the path to this app.
    GetModuleFileName(hInstance, szPath, ARRAYSIZE(szPath));
    // Tag on the percent one thingy.
    lstrcat(szPath, g_szSpacePercentOne);
    // Regsiter the app.
    ShellRegisterApp(g_szGRP, g_szMSProgramGroup, szTypeName, szPath, TRUE);
    // Explorer key.
    RegCreateKey(HKEY_CURRENT_USER, c_szRegGrpConv, &g_hkeyGrpConv);

    Log(TEXT("Init Application."));

    return TRUE;
}

//---------------------------------------------------------------------------
void UnInitApplication(void)
{
    Log(TEXT("Uninit Application."));

    if (g_hkeyGrpConv)
        RegCloseKey(g_hkeyGrpConv);
}

// Do this here instead of in Explorer so we don't keep overwriting
// user settings.
#if 1
//----------------------------------------------------------------------------
const TCHAR c_szExplorer[] = TEXT("Explorer");
const TCHAR c_szRestrictions[] = TEXT("Restrictions");
const TCHAR c_szEditLevel[] = TEXT("EditLevel");
const TCHAR c_szNoRun[] = TEXT("NoRun");
const TCHAR c_szNoClose[] = TEXT("NoClose");
const TCHAR c_szNoSaveSettings[] = TEXT("NoSaveSettings");
const TCHAR c_szNoFileMenu[] = TEXT("NoFileMenu");
const TCHAR c_szShowCommonGroups[] = TEXT("ShowCommonGroups");
const TCHAR c_szNoCommonGroups[] = TEXT("NoCommonGroups");

//----------------------------------------------------------------------------
void Restrictions_Convert(LPCTSTR szIniFile)
{
        DWORD dw, cbData, dwType;
        HKEY hkeyPolicies, hkeyPMRestrict;

        DebugMsg(DM_TRACE, TEXT("c.cr: Converting restrictions..."));

        if (RegCreateKey(HKEY_CURRENT_USER, REGSTR_PATH_POLICIES, &hkeyPolicies) == ERROR_SUCCESS)
        {
                // Get them. Set them.
#ifdef WINNT

                if (RegOpenKeyEx(HKEY_CURRENT_USER,
                                 TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Program Manager\\Restrictions"),
                                 0, KEY_READ, &hkeyPMRestrict) == ERROR_SUCCESS) {

                    cbData = sizeof(dw);

                    dw = 0;
                    RegQueryValueEx(hkeyPMRestrict, c_szEditLevel, 0, &dwType, (LPBYTE)&dw, &cbData);
                    Reg_SetDWord(hkeyPolicies, c_szExplorer, c_szEditLevel, dw);

                    dw = 0;
                    RegQueryValueEx(hkeyPMRestrict, c_szNoRun, 0, &dwType, (LPBYTE)&dw, &cbData);
                    Reg_SetDWord(hkeyPolicies, c_szExplorer, c_szNoRun, dw);

                    dw = 0;
                    RegQueryValueEx(hkeyPMRestrict, c_szNoClose, 0, &dwType, (LPBYTE)&dw, &cbData);
                    Reg_SetDWord(hkeyPolicies, c_szExplorer, c_szNoClose, dw);

                    dw = 0;
                    RegQueryValueEx(hkeyPMRestrict, c_szNoSaveSettings, 0, &dwType, (LPBYTE)&dw, &cbData);
                    Reg_SetDWord(hkeyPolicies, c_szExplorer, c_szNoSaveSettings, dw);

                    dw = 0;
                    RegQueryValueEx(hkeyPMRestrict, c_szNoFileMenu, 0, &dwType, (LPBYTE)&dw, &cbData);
                    Reg_SetDWord(hkeyPolicies, c_szExplorer, c_szNoFileMenu, dw);

                    dw = 0;
                    if (RegQueryValueEx(hkeyPMRestrict, c_szShowCommonGroups, 0, &dwType, (LPBYTE)&dw, &cbData) == ERROR_SUCCESS) {
                        dw = !dw;
                    }
                    Reg_SetDWord(hkeyPolicies, c_szExplorer, c_szNoCommonGroups, dw);

                    RegCloseKey (hkeyPMRestrict);
                }


#else

                dw = GetPrivateProfileInt(c_szRestrictions, c_szEditLevel, 0, szIniFile);
                Reg_SetDWord(hkeyPolicies, c_szExplorer, c_szEditLevel, dw);

                dw = GetPrivateProfileInt(c_szRestrictions, c_szNoRun, 0, szIniFile);
                Reg_SetDWord(hkeyPolicies, c_szExplorer, c_szNoRun, dw);

                dw = GetPrivateProfileInt(c_szRestrictions, c_szNoClose, 0, szIniFile);
                Reg_SetDWord(hkeyPolicies, c_szExplorer, c_szNoClose, dw);

                dw = GetPrivateProfileInt(c_szRestrictions, c_szNoSaveSettings , 0, szIniFile);
                Reg_SetDWord(hkeyPolicies, c_szExplorer, c_szNoSaveSettings, dw);

                dw = GetPrivateProfileInt(c_szRestrictions, c_szNoFileMenu , 0, szIniFile);
                Reg_SetDWord(hkeyPolicies, c_szExplorer, c_szNoFileMenu, dw);
#endif

                RegCloseKey(hkeyPolicies);
        }
        else
        {
                DebugMsg(DM_ERROR, TEXT("gc.cr: Unable to create policy key for registry."));
                DebugMsg(DM_ERROR, TEXT("gc.cr: Restrictions can not be converted."));
        }
}
#endif

//----------------------------------------------------------------------------
void CALLBACK Group_EnumCallback(LPCTSTR lpszGroup)
{
    Group_Convert(NULL, lpszGroup, 0);
}

//----------------------------------------------------------------------------
// convert all 3.x groups to chicago directories and links
void DoAutoConvert(BOOL fModifiedOnly, BOOL bConvertGRPFiles)
{
    TCHAR szIniFile[MAX_PATH];
    int cb, cGroups = 0;


#ifdef WINNT

    //
    // NT's ProgMan settings are stored in the registry.
    // No need to find a progman.ini file.
    //

    Restrictions_Convert(NULL);

#else

    if (FindProgmanIni(szIniFile)) {
        Restrictions_Convert(szIniFile);
    }

#endif


#ifdef WINNT

    //
    // Convert Unicode NT groups
    //

    cGroups = Group_EnumNT(Group_EnumCallback, TRUE, fModifiedOnly,
                         HKEY_CURRENT_USER, c_szUnicodeGroups);


    if (cGroups == 0) {

        //
        // Try ANSI progman groups (Upgrade from NT 3.1)
        //

        cGroups = Group_EnumNT(Group_EnumCallback, TRUE, fModifiedOnly,
                             HKEY_CURRENT_USER, c_szAnsiGroups);
    }
#endif

    if (bConvertGRPFiles && (cGroups == 0)) {

        //
        // Convert .grp files
        //

        cGroups = Group_Enum(Group_EnumCallback, TRUE, fModifiedOnly);
    }
}

//----------------------------------------------------------------------------
void CALLBACK Group_ListApps(LPCTSTR lpszGroup)
{
    DebugMsg(DM_TRACE, TEXT("gc.g_la: %s"), lpszGroup);
    Group_Convert(NULL, lpszGroup, GC_BUILDLIST);
}

//----------------------------------------------------------------------------
// Grovel the old .grp files to build a list of all the old installed apps.
void AppList_Build(void)
{
    DebugMsg(DM_TRACE, TEXT("gc.bal: Building app list..."));
    AppList_Create();
    Group_EnumOldGroups(Group_ListApps, TRUE);
    AppList_AddCurrentStuff();
    AppList_WriteFile();
    AppList_Destroy();
}

// FILE_ATTRIBUTE_READONLY         0x00000001
// FILE_ATTRIBUTE_HIDDEN           0x00000002
// FILE_ATTRIBUTE_SYSTEM           0x00000004

void DoDelete(LPCTSTR pszPath, LPCTSTR pszLongName)
{
    TCHAR szTo[MAX_PATH], szTemp[MAX_PATH];
    BOOL fDir = FALSE;

    // if the first character is an asterisk, it means to
    // treat the name as a directory
    if(*pszLongName == TEXT('*'))
    {
        fDir = TRUE;
        pszLongName = CharNext(pszLongName);
    }

    if(ParseField(pszLongName, 1, szTemp, ARRAYSIZE(szTemp)))
    {
        PathCombine(szTo, pszPath, szTemp);

        if(fDir)
        {
            // NOTE: RemoveDirectory fails if the directory
            // is not empty.  It is by design that we do not
            // recursively delete every file and directory.
            RemoveDirectory(szTo);
        }
        else
        {
            DeleteFile(szTo);
        }
    }
}

void DoRenameSetAttrib(LPCTSTR pszPath, LPCTSTR pszShortName, LPCTSTR pszLongName, BOOL bLFN)
{
    DWORD dwAttributes;
    TCHAR szFrom[MAX_PATH], szTo[MAX_PATH], szTemp[MAX_PATH];

    if (bLFN && (ParseField(pszLongName, 1, szTemp, ARRAYSIZE(szTemp))))
    {
        PathCombine(szFrom, pszPath, pszShortName);
        PathCombine(szTo, pszPath, szTemp);
        if (!MoveFile(szFrom, szTo))
        {
            DWORD dwError = GetLastError();
            DebugMsg(DM_TRACE, TEXT("c.rsa: Rename %s Failed %x"), szFrom, dwError);

            // Does the destination already exist?
            if (dwError == ERROR_ALREADY_EXISTS)
            {
                // Delete it.
                if (DeleteFile(szTo))
                {
                    if (!MoveFile(szFrom, szTo))
                    {
                        dwError = GetLastError();
                        DebugMsg(DM_TRACE, TEXT("c.rsa: Rename after Delete %s Failed %x"), szFrom, dwError);
                    }
                }
            }
        }
    }
    else
    {
        // use this to set the attributes on
        PathCombine(szTo, pszPath, pszShortName);
    }

    ParseField(pszLongName, 2, szTemp, ARRAYSIZE(szTemp));
    dwAttributes = (DWORD)StrToInt(szTemp);
    if (dwAttributes)
        SetFileAttributes(szTo, dwAttributes);
}

const TCHAR c_szDeleteRoot[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\DeleteFiles");
const TCHAR c_szRenameRoot[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\RenameFiles");
const TCHAR c_szPreRenameRoot[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\PreConvRenameFiles");

#ifdef WINNT
//
// this was stolen from shlwapi\reg.c, we cant link to it since we are "grpconv.exe",
// and we do not move in the same social circles as shlwapi.
//
DWORD NT5RegDeleteKey(HKEY hkey, LPCTSTR pszSubKey)
{
    DWORD dwRet;
    HKEY hkSubKey;

    // Open the subkey so we can enumerate any children
    dwRet = RegOpenKeyEx(hkey, pszSubKey, 0, KEY_ALL_ACCESS, &hkSubKey);
    if (ERROR_SUCCESS == dwRet)
    {
        DWORD   dwIndex;
        TCHAR   szSubKeyName[MAX_PATH + 1];
        DWORD   cchSubKeyName = ARRAYSIZE(szSubKeyName);
        TCHAR   szClass[MAX_PATH];
        DWORD   cbClass = ARRAYSIZE(szClass);

        // I can't just call RegEnumKey with an ever-increasing index, because
        // I'm deleting the subkeys as I go, which alters the indices of the
        // remaining subkeys in an implementation-dependent way.  In order to
        // be safe, I have to count backwards while deleting the subkeys.

        // Find out how many subkeys there are
        dwRet = RegQueryInfoKey(hkSubKey,
                                szClass,
                                &cbClass,
                                NULL,
                                &dwIndex, // The # of subkeys -- all we need
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL);

        if (NO_ERROR == dwRet)
        {
            // dwIndex is now the count of subkeys, but it needs to be
            // zero-based for RegEnumKey, so I'll pre-decrement, rather
            // than post-decrement.
            while (ERROR_SUCCESS == RegEnumKey(hkSubKey, --dwIndex, szSubKeyName, cchSubKeyName))
            {
                NT5RegDeleteKey(hkSubKey, szSubKeyName);
            }
        }

        RegCloseKey(hkSubKey);

        dwRet = RegDeleteKey(hkey, pszSubKey);
    }

    return dwRet;
}
#endif

//----------------------------------------------------------------------------
void DoFileRenamesOrDeletes(LPCTSTR pszKey, BOOL fDelete)
{
    HKEY hkey;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, pszKey, &hkey) == ERROR_SUCCESS)
    {
        TCHAR szKey[32];
        int iKey;

        for (iKey = 0; RegEnumKey(hkey, iKey, szKey, ARRAYSIZE(szKey)) == ERROR_SUCCESS; iKey++)
        {
            HKEY hkeyEnum;

            // each key under here lists files to be renamed in a certain folder

            if (RegOpenKey(hkey, szKey, &hkeyEnum) == ERROR_SUCCESS)
            {
                DWORD cbValue;
                TCHAR szPath[MAX_PATH];

                // get the path where these files are
                cbValue = SIZEOF(szPath);
                if ((RegQueryValue(hkey, szKey, szPath, &cbValue) == ERROR_SUCCESS) && szPath[0])
                {
                    TCHAR szShortName[13], szLongName[MAX_PATH];
                    DWORD cbData, cbValue, dwType, iValue;
                    BOOL bLFN = IsLFNDrive(szPath);

                    for (iValue = 0; cbValue = ARRAYSIZE(szShortName), cbData = SIZEOF(szLongName),
                         (RegEnumValue(hkeyEnum, iValue, szShortName, &cbValue, NULL, &dwType, (LPBYTE)szLongName, &cbData) == ERROR_SUCCESS);
                         iValue++)
                    {
                        if (szShortName[0] && ( dwType == REG_SZ ) )
                        {
                            if (fDelete)
                                DoDelete(szPath, szLongName);
                            else
                                DoRenameSetAttrib(szPath, szShortName, szLongName, bLFN);
                        }
                    }
                }
                RegCloseKey(hkeyEnum);
            }
        }
        // Toast this whole section so we don't ever try to do renames or deletes twice.
#ifdef WINNT
        // We need to call NT5RegDeleteKey since on NT we dont nuke it if subkeys exist, but this helper does.
        NT5RegDeleteKey(HKEY_LOCAL_MACHINE, pszKey);
#else
        // on win95 just use the real api, which nukes everything for us.
        RegDeleteKey(HKEY_LOCAL_MACHINE, pszKey);
#endif
        RegCloseKey(hkey);
    }
}

//----------------------------------------------------------------------------
void DoFileRenames(LPCTSTR pszKey)
{
    DoFileRenamesOrDeletes(pszKey, FALSE);
}

//----------------------------------------------------------------------------
void DoFileDeletes(LPCTSTR pszKey)
{
    DoFileRenamesOrDeletes(pszKey, TRUE);
}

//----------------------------------------------------------------------------
const TCHAR c_szLinksRoot[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Links");

//----------------------------------------------------------------------------
void DoCopyLinks()
{
    HKEY hkey;
    BOOL bLFN;
    LPTSTR szSrcName, szDstName, szGroupFolder, szLinkName, szCmd;

    // DebugBreak();

    // Allocate buffer
    //
    if ((szSrcName = (LPTSTR)LocalAlloc(LPTR, 6*MAX_PATH)) == NULL)
      return;
    szDstName     = szSrcName+MAX_PATH;
    szGroupFolder = szDstName+MAX_PATH;
    szLinkName    = szGroupFolder+MAX_PATH;
    szCmd         = szLinkName+MAX_PATH;

    // Get the path to the special folder
    //
    SHGetSpecialFolderPath(NULL, szGroupFolder, CSIDL_PROGRAMS, TRUE);
    bLFN = IsLFNDrive(szGroupFolder);

    // Enumerate each link
    //
    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szLinksRoot, &hkey) == ERROR_SUCCESS)
    {
        DWORD cbData, cbValue, dwType, iValue;

        for (iValue = 0; cbValue = MAX_PATH, cbData = 2*MAX_PATH*SIZEOF(TCHAR),
             (RegEnumValue(hkey, iValue, szLinkName, &cbValue, NULL, &dwType, (LPBYTE)szCmd, &cbData) == ERROR_SUCCESS);
             iValue++)
        {
            if (szLinkName[0] && (dwType == REG_SZ))
            {
                // Build the destination name
                //
                lstrcpy(szDstName, szGroupFolder);
                ParseField(szCmd, 1, szSrcName, MAX_PATH);
                PathAppend(szDstName, szSrcName);

                // Check the volume type
                //
                if (bLFN)
                {
                    PathAppend(szDstName, szLinkName);
                    lstrcat(szDstName, TEXT(".lnk"));
                    ParseField(szCmd, 2, szSrcName, MAX_PATH);
                }
                else
                {
                    ParseField(szCmd, 2, szSrcName, MAX_PATH);
                    PathAppend(szDstName, PathFindFileName(szSrcName));
                }

                MoveFile(szSrcName, szDstName);
            }
        }
        // Nuke this section so we don't do copies twice.
        RegDeleteKey(HKEY_LOCAL_MACHINE, c_szLinksRoot);

        RegCloseKey(hkey);
    }

    LocalFree((HLOCAL)szSrcName);
}

// do the actual linking to the floppy
void CreateLinkToFloppy(IShellLink *psl, IPersistFile *ppf, LPCITEMIDLIST pidlDrives, LPITEMIDLIST pidlFloppy, LPITEMIDLIST pidlSendTo, STRRET * psrDisplayName)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szFileName[MAX_PATH];
    WCHAR wszPath[MAX_PATH];
    LPITEMIDLIST pidlTarget = ILCombine(pidlDrives, pidlFloppy);

    memset(szFileName, 0, SIZEOF(szFileName));
    if (pidlTarget) {

        DWORD dwType;
        SHGetPathFromIDList(pidlTarget, szPath);
        dwType = GetDriveType(szPath);

        SHGetPathFromIDList(pidlSendTo, szPath);

        StrRetToStrN(szFileName,ARRAYSIZE(szFileName),psrDisplayName,pidlFloppy);

        lstrcat(szFileName, TEXT(".lnk"));
        PathCleanupSpec(szPath, szFileName);

        if (PathCombine(szPath, szPath, szFileName)) {
            if (!PathFileExists(szPath)) {
                psl->lpVtbl->SetIDList(psl, pidlTarget);

                // Clear out old stuff.
                psl->lpVtbl->SetArguments(psl, c_szNULL);
                psl->lpVtbl->SetWorkingDirectory(psl, c_szNULL);
                psl->lpVtbl->SetIconLocation(psl, c_szNULL, 0);
                psl->lpVtbl->SetHotkey(psl, 0);
                psl->lpVtbl->SetShowCmd(psl, SW_SHOWNORMAL);
                psl->lpVtbl->SetDescription(psl, c_szNULL);

                StrToOleStr(wszPath, szPath);
                ppf->lpVtbl->Save(ppf, wszPath, TRUE);
            }
        }
        ILFree(pidlTarget);
    }
}

// we'd much rather have new firm links than old floppy ones
// clear out any old ones made by previous runs of setup
void DeleteOldFloppyLinks(LPSHELLFOLDER psfDesktop, LPITEMIDLIST pidlSendTo, IPersistFile* ppf, IShellLink *psl)
{
    LPSHELLFOLDER psfSendTo;

    if (SUCCEEDED(psfDesktop->lpVtbl->BindToObject(psfDesktop, pidlSendTo, NULL, &IID_IShellFolder, &psfSendTo))) {
        LPENUMIDLIST penum;
        if (SUCCEEDED(psfSendTo->lpVtbl->EnumObjects(psfSendTo, NULL, SHCONTF_NONFOLDERS, &penum))) {

            LPITEMIDLIST pidl;
            ULONG celt;

            while ((penum->lpVtbl->Next(penum, 1, &pidl, &celt) == NOERROR) && (celt == 1)) {
                TCHAR szPath[MAX_PATH];
                WCHAR wszPath[MAX_PATH];
                LPITEMIDLIST pidlFullPath;

                DWORD dwAttribs = SFGAO_LINK;
                // is it a link???
                if (SUCCEEDED(psfSendTo->lpVtbl->GetAttributesOf(psfSendTo, 1, &pidl, &dwAttribs)) && (dwAttribs & (SFGAO_LINK))) {

                    DebugMsg(DM_TRACE, TEXT("YES! It's a link"));
                    // get the target
                    pidlFullPath = ILCombine(pidlSendTo, pidl);
                    if (pidlFullPath) {
                        LPITEMIDLIST pidlTarget;
                        SHGetPathFromIDList(pidlFullPath, szPath);
                        StrToOleStr(wszPath, szPath);
                        ppf->lpVtbl->Load(ppf, wszPath, 0);
                        ILFree(pidlFullPath);
                        if (SUCCEEDED(psl->lpVtbl->GetIDList(psl, &pidlTarget))) {

                            TCHAR szTargetPath[MAX_PATH];
                            SHGetPathFromIDList(pidlTarget, szTargetPath);
                            DebugMsg(DM_TRACE, TEXT("Found target lik path of %s"), szTargetPath);
                            if (PathIsRoot(szTargetPath) && DriveType(PathGetDriveNumber(szTargetPath)) == DRIVE_REMOVABLE) {
                                DebugMsg(DM_TRACE, TEXT("Target is removeable... deleting"));
                                Win32DeleteFile(szPath);
                            } else {
                                DebugMsg(DM_TRACE, TEXT("Target is NOT removeable... NOT deleting"));
                            }
                            ILFree(pidlTarget);
                        }
                    }
                } else {
                    DebugMsg(DM_TRACE, TEXT("No, it's not a link"));
                }
                ILFree(pidl);
            }
        }
    }
}

void BuildFloppyLinks()
{
    IShellLink *psl;

    // get a link interface
    if (SUCCEEDED(ICoCreateInstance(&CLSID_ShellLink, &IID_IShellLink, &psl))) {
        IPersistFile *ppf;

        if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, &ppf))) {
            LPSHELLFOLDER psfDesktop;

            // get the desktop folder.
            if (SUCCEEDED(ICoCreateInstance(&CLSID_ShellDesktop, &IID_IShellFolder, &psfDesktop))) {

                LPITEMIDLIST pidlDrives = SHCloneSpecialIDList(NULL, CSIDL_DRIVES, FALSE);
                LPSHELLFOLDER psfDrives;

                // from that get the drives container
                if (pidlDrives && SUCCEEDED(psfDesktop->lpVtbl->BindToObject(psfDesktop, pidlDrives, NULL, &IID_IShellFolder, &psfDrives))) {
                    LPENUMIDLIST penum;
                    LPITEMIDLIST pidlSendTo = SHCloneSpecialIDList(NULL, CSIDL_SENDTO, TRUE);
                    if (pidlSendTo) {

                        DeleteOldFloppyLinks(psfDesktop, pidlSendTo, ppf, psl);

                        if (SUCCEEDED(psfDrives->lpVtbl->EnumObjects(psfDrives, NULL, SHCONTF_FOLDERS, &penum))) {

                            LPITEMIDLIST pidl;
                            ULONG celt;

                            while ((penum->lpVtbl->Next(penum, 1, &pidl, &celt) == NOERROR) && (celt == 1)) {


                                // verify that it's a pidl we're interested in.

                                DWORD dwAttribs = SFGAO_FILESYSANCESTOR | SFGAO_REMOVABLE;

                                if (SUCCEEDED(psfDrives->lpVtbl->GetAttributesOf(psfDrives, 1, &pidl, &dwAttribs)) && (dwAttribs == (SFGAO_FILESYSANCESTOR | SFGAO_REMOVABLE))) {
                                    // BUILD THE LINKNAME
                                    STRRET srDisplayName;
                                    if (SUCCEEDED(psfDrives->lpVtbl->GetDisplayNameOf(psfDrives, pidl,
                                                                                      SHGDN_INFOLDER, &srDisplayName)))
                                    {
                                        CreateLinkToFloppy(psl, ppf, pidlDrives, pidl, pidlSendTo, &srDisplayName);
                                    }
                                }
                                ILFree(pidl);
                            }
                        }
                        penum->lpVtbl->Release(penum);
                        ILFree(pidlSendTo);
                    }
                    ILFree(pidlDrives);
                    psfDrives->lpVtbl->Release(psfDrives);
                }
                psfDesktop->lpVtbl->Release(psfDesktop);
            }
            ppf->lpVtbl->Release(ppf);
        }
        psl->lpVtbl->Release(psl);
    }
}

// makes sure the current user's metrics are stored in scalable units
void PASCAL
ConvertMetricsToScalableUnits(BOOL fKeepBradsSettings)
{
    NONCLIENTMETRICS ncm;
    LOGFONT lf;
    HDC screen;
    int value;
    int floor = 0;

    // USER always writes out font sizes in points and metrics in twips
    // get and set everything of interest

    ncm.cbSize = SIZEOF( NONCLIENTMETRICS );
    SystemParametersInfo( SPI_GETNONCLIENTMETRICS, SIZEOF( ncm ),
        (void far *)(LPNONCLIENTMETRICS)&ncm, FALSE );
    SystemParametersInfo( SPI_SETNONCLIENTMETRICS, SIZEOF( ncm ),
        (void far *)(LPNONCLIENTMETRICS)&ncm, SPIF_UPDATEINIFILE );

    SystemParametersInfo( SPI_GETICONTITLELOGFONT, SIZEOF( lf ),
        (void far *)(LPLOGFONT)&lf, FALSE );
    SystemParametersInfo( SPI_SETICONTITLELOGFONT, SIZEOF( lf ),
        (void far *)(LPLOGFONT)&lf, SPIF_UPDATEINIFILE );

    // HACK: Win3x users could get into 120 DPI without upping the icon spacing
    // they need the equivalent of 75 pixels in the current logical resolution
    if (!fKeepBradsSettings)
    {
        screen = GetDC( NULL );
        floor = MulDiv( 75, GetDeviceCaps( screen, LOGPIXELSX ), 96 );
        ReleaseDC( NULL, screen );
        value = GetSystemMetrics( SM_CXICONSPACING );
        SystemParametersInfo( SPI_ICONHORIZONTALSPACING, max( value, floor ),
            NULL, SPIF_UPDATEINIFILE );

        value = GetSystemMetrics( SM_CYICONSPACING );
        SystemParametersInfo( SPI_ICONVERTICALSPACING, max( value, floor ),
            NULL, SPIF_UPDATEINIFILE );
    }

}

//----------------------------------------------------------------------------
// We need to nuke progman's window settings on first boot so it doesn't
// fill the screen and obscure the tray if we're in Win3.1 UI mode.
void NukeProgmanSettings(void)
{
    WritePrivateProfileString(c_szSettings, c_szWindow, NULL, c_szProgmanIni);
}

//----------------------------------------------------------------------------
// Tells Explorer to check the win.ini extensions section.
void ExplorerCheckAssociations(void)
{
    DWORD dw = 1;

    Reg_Set(HKEY_CURRENT_USER, c_szRegExplorer, c_szCheckAssociations,
        REG_BINARY, &dw, SIZEOF(dw));
}

//----------------------------------------------------------------------------
// The setup flag is set for first boot stuff (-s) and not for maintenance
// mode (-o).
void DoRandomOtherStuff(BOOL fSetup, BOOL fKeepBradsSettings)
{
    Log(TEXT("dros: ..."));

    Log(TEXT("dros: Renames."));
    DoFileRenames(c_szRenameRoot);
    Log(TEXT("dros: Copies."));
    DoCopyLinks();
    Log(TEXT("dros: Deletes."));
    DoFileDeletes(c_szDeleteRoot);

    if (fSetup)
    {
        Log(TEXT("dros: Floppy links."));
        BuildFloppyLinks();
        Log(TEXT("dros: Converting metrics."));
        ConvertMetricsToScalableUnits(fKeepBradsSettings);
        Log(TEXT("dros: Nuking Progman settings."));
        NukeProgmanSettings();
        // GenerateSetupExitEvent();
        ExplorerCheckAssociations();
    }

    Log(TEXT("dros: Done."));
}

//---------------------------------------------------------------------------
void DoConversion(HINSTANCE hinst, LPTSTR lpszCmdLine)
{
    HKEY hKey;
    DWORD Err, DataType, DataSize = sizeof(DWORD);
    DWORD Value;
    TCHAR szFile[MAX_PATH];
    TCHAR szFilters[CCHSZNORMAL];
    TCHAR szTitle[CCHSZNORMAL];
    HCURSOR hCursor;
    UINT olderrormode;

    GetWindowsDirectory(szFile, ARRAYSIZE(szFile));
    PathAddBackslash(szFile);

    // set the error mode to ignore noopenfileerrorbox so on japanese PC-98 machines
    // whose hard drive is A: we dont ask for a floppy when running grpconv.
    olderrormode = SetErrorMode(0);
    SetErrorMode(olderrormode | SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);

    // Is GUI Setup currently running?
    if((Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           TEXT("System\\Setup"),
                           0,
                           KEY_READ,
                           &hKey)) == ERROR_SUCCESS) {

        Err = RegQueryValueEx(
                    hKey,
                    TEXT("SystemSetupInProgress"),
                    NULL,
                    &DataType,
                    (LPBYTE)&Value,
                    &DataSize);

        RegCloseKey(hKey);
    }

    if( (Err == NO_ERROR) && Value ) {
        g_fShowUI = FALSE;
    }


    if (!lstrcmpi(lpszCmdLine, TEXT("/m")) || !lstrcmpi(lpszCmdLine, TEXT("-m")))
    {
        // manual mode

        // Get something from a commdlg....
        LoadString(hinst, IDS_FILTER, szFilters, ARRAYSIZE(szFilters));
        ConvertHashesToNulls(szFilters);
        LoadString(hinst, IDS_COMMDLGTITLE, szTitle, ARRAYSIZE(szTitle));
        // Keep going till they hit cancel.
        while (GetFileNameFromBrowse(NULL, szFile, ARRAYSIZE(szFile), NULL, g_szGRP, szFilters, szTitle))
        {
            Group_CreateProgressDlg();
            Group_Convert(NULL, szFile, GC_PROMPTBEFORECONVERT | GC_REPORTERROR | GC_OPENGROUP);
            Group_DestroyProgressDlg();
        }
    }
    else if (!lstrcmpi(lpszCmdLine, TEXT("/s")) || !lstrcmpi(lpszCmdLine, TEXT("-s")))
    {
        // Rebuild - without the logo.
        hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
        DoFileRenames(c_szPreRenameRoot);
        DoAutoConvert(FALSE, TRUE);
        BuildDefaultGroups();
        DoRandomOtherStuff(TRUE, FALSE);
        SetCursor(hCursor);
    }
    else if (!lstrcmpi(lpszCmdLine, TEXT("/n")) || !lstrcmpi(lpszCmdLine, TEXT("-n")))
    {
        //
        // Used by NT setup
        //
        // 1) Converts ProgMan common groups
        // 2) Builds floppy links
        //
        g_fDoingCommonGroups = TRUE;
        Group_EnumNT(Group_EnumCallback, FALSE, FALSE,
                     HKEY_LOCAL_MACHINE, c_szCommonGroups);
        BuildFloppyLinks();
    }

    else if (!lstrcmpi(lpszCmdLine, TEXT("/c")) || !lstrcmpi(lpszCmdLine, TEXT("-c")))
    {
        // Convert NT common progman groups only
        hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
        g_fDoingCommonGroups = TRUE;
        Group_EnumNT(Group_EnumCallback, TRUE, FALSE,
                     HKEY_LOCAL_MACHINE, c_szCommonGroups);
        SetCursor(hCursor);
    }

    else if (!lstrcmpi(lpszCmdLine, TEXT("/p")) || !lstrcmpi(lpszCmdLine, TEXT("-p")))
    {
        // Convert NT personal progman groups only
        // This switch is used by NT setup via userdiff
        hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
        DoAutoConvert(FALSE, FALSE);
        SetCursor(hCursor);
    }

    else if (!lstrcmpi(lpszCmdLine, TEXT("/t")) || !lstrcmpi(lpszCmdLine, TEXT("-t")))
    {
        // Same as -s but only coverts modified groups (used on a re-install).
        hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
        DoFileRenames(c_szPreRenameRoot);
        DoAutoConvert(TRUE, TRUE);
        BuildDefaultGroups();
        DoRandomOtherStuff(TRUE, TRUE);
        SetCursor(hCursor);
    }
    else if (!lstrcmpi(lpszCmdLine, TEXT("/q")) || !lstrcmpi(lpszCmdLine, TEXT("-q")))
    {
        // Question and answer stuff.
        AppList_Build();
        // Restart the reporter tool.
        WinExec(c_szReporter, SW_NORMAL);
    }
    else if (!lstrcmpi(lpszCmdLine, TEXT("/o")) || !lstrcmpi(lpszCmdLine, TEXT("-o")))
    {
        // Optional component GrpConv (ie don't look at Progman groups).
        hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
        DoFileRenames(c_szPreRenameRoot);
        BuildDefaultGroups();
        DoRandomOtherStuff(FALSE, FALSE);
        SetCursor(hCursor);
    }
    else if (!lstrcmpi(lpszCmdLine, TEXT("/u")) || !lstrcmpi(lpszCmdLine, TEXT("-u")))
    {
        // Display NO UI (ie no progress dialog) and process
        // Optional components (ie don't look at Progman groups),
        g_fShowUI = FALSE;
        hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
        DoFileRenames(c_szPreRenameRoot);
        BuildDefaultGroups();
        DoRandomOtherStuff(FALSE, FALSE);
        SetCursor(hCursor);
    }
    else if (*lpszCmdLine)
    {
        // file specified, convert just it
        Group_CreateProgressDlg();
        Group_Convert(NULL, lpszCmdLine, GC_REPORTERROR | GC_OPENGROUP);    // REVIEW, maybe silent?
        Group_DestroyProgressDlg();
    }
    else
    {
        hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
        DoFileRenames(c_szPreRenameRoot);
        DoAutoConvert(TRUE, TRUE);
        DoRandomOtherStuff(FALSE, FALSE);
        SetCursor(hCursor);
    }
}

// stolen from the CRT, used to shirink our code

int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFO si;
    LPTSTR pszCmdLine = GetCommandLine();

    if ( *pszCmdLine == TEXT('\"') ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine
             != TEXT('\"')) );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == TEXT('\"') )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > TEXT(' '))
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfo(&si);

    i = WinMainT(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
    return i;   // We never comes here.
}

//---------------------------------------------------------------------------
int WinMainT(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    LCID lcid;
    HMODULE hLibImm;

    BOOL (WINAPI *ImmDisableIME)(DWORD) = NULL;

    lcid = GetThreadLocale();

    // we have to LoadLibaray/GetProcAddress ImmDisableIME because
    // this is not exported on win95 gold or NT4.
    hLibImm = LoadLibrary(TEXT("imm.dll"));
    if (hLibImm)
    {
        (FARPROC) *ImmDisableIME = GetProcAddress(hLibImm, "ImmDisableIME");
        if (ImmDisableIME != NULL)
        {
            if ( (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_JAPANESE) ||
                 (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_KOREAN)   ||
                 (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_CHINESE) )
            {
                ImmDisableIME(0);
            }
        }
        FreeLibrary(hLibImm);
    }

    g_hinst = hInstance;
    if (InitApplication(hInstance))
    {
            // We do all the work on InitInst
            InitCommonControls();
            DoConversion(hInstance, lpCmdLine);
            UnInitApplication();
    }
    return TRUE;
}
