//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#include "grpconv.h"    
#include "gcinst.h"
#include "util.h"
#include <shellp.h>
#include <trayp.h>
#include <regstr.h>
#include <shguidp.h>
#include <windowsx.h>
#include "rcids.h"
#include "group.h"
#include <..\..\inc\krnlcmn.h>  // GetProcessDword

#ifdef WINNT
// NT is unicode so use a larger buffer since sizeof(TCHAR) is > than on win95
#define BUFSIZES 40960
#else
// on win95 GetPrivateProfileSection has a 32767 char limit, so
// we make this a bit smaller
#define BUFSIZES 20480
#endif // WINNT

// Define checkbox states for listview
#define LVIS_GCNOCHECK  0x1000
#define LVIS_GCCHECK    0x2000

#define HSZNULL         0
#define HCONVNULL       0
#define HCONVLISTNULL   0
#define DDETIMEOUT      20*1000

extern UINT GC_TRACE;
extern const TCHAR c_szMapGroups[];
BOOL g_fDoProgmanDde = FALSE;
BOOL g_fInitDDE = FALSE;

#define CH_COLON        TEXT(':')

//---------------------------------------------------------------------------
// Global to this file only...
static const TCHAR c_szGrpConvInf[] = TEXT("setup.ini");
static const TCHAR c_szGrpConvInfOld[] = TEXT("setup.old");
static const TCHAR c_szExitProgman[] = TEXT("[ExitProgman(1)]");
static const TCHAR c_szAppProgman[] = TEXT("AppProgman");
static const TCHAR c_szEnableDDE[] = TEXT("EnableDDE");
static const TCHAR c_szProgmanOnly[] = TEXT("progman.only");
static const TCHAR c_szProgmanGroups[] = TEXT("progman.groups");
static const TCHAR c_szDesktopGroups[] = TEXT("desktop.groups");
static const TCHAR c_szStartupGroups[] = TEXT("startup.groups");
static const TCHAR c_szSendToGroups[] = TEXT("sendto.groups");
static const TCHAR c_szRecentDocsGroups[] = TEXT("recentdocs.groups");

//---------------------------------------------------------------------------
const TCHAR c_szProgmanIni[] = TEXT("progman.ini");
const TCHAR c_szStartup[] = TEXT("Startup");
const TCHAR c_szProgmanExe[] = TEXT("progman.exe");
const TCHAR c_szProgman[] = TEXT("Progman");

// NB This must match the one in cabinet.
static const TCHAR c_szRUCabinet[] = TEXT("[ConfirmCabinetID]");

typedef struct
{
        DWORD dwInst;
        HCONVLIST hcl;
        HCONV hconv;
        BOOL fStartedProgman;
} PMDDE, *PPMDDE;

//
// This function grovles HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\ShellFolders
// and creates a DPA with strings of all the speial folders
//
BOOL CreateSpecialFolderDPA(HDPA* phdpaSF)
{
    HKEY hkSP;
    TCHAR szValueName[MAX_PATH];
    DWORD cbValueName;
    DWORD cbData;
    DWORD dwIndex = 0;
    LONG lRet = ERROR_SUCCESS;

    // we should only ever be called once to populate the dpa
    if (*phdpaSF != NULL)
        return FALSE;

    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"),
                     0,
                     KEY_QUERY_VALUE,
                     &hkSP) != ERROR_SUCCESS)
    {
        // couldnt open the key, so bail
        return FALSE;
    }

    *phdpaSF = DPA_Create(4);

    do
    {
        cbValueName = ARRAYSIZE(szValueName);

        lRet = RegEnumValue(hkSP,
                           dwIndex,
                           szValueName,
                           &cbValueName,
                           NULL,
                           NULL,
                           NULL,
                           &cbData);
        
        if (lRet == ERROR_SUCCESS)
        {
            LPTSTR pszValueData = LocalAlloc(LPTR, cbData);

            if (!pszValueData)
                break;
            
            if (RegQueryValueEx(hkSP,
                                szValueName,
                                NULL,
                                NULL,
                                (LPBYTE)pszValueData,
                                &cbData) == ERROR_SUCCESS)
            {
                DPA_AppendPtr(*phdpaSF, pszValueData);
            }
        }

        dwIndex++;

    } while (lRet != ERROR_NO_MORE_ITEMS);

    return TRUE;
}


//
// SafeRemoveDirectory checks to make sure that we arent removing a "special"
// folder. On win95 when we remove the last shortcut from the %windir%\desktop folder,
// we go and remove that as well. This causes the shell to hang among other bad things.
//
BOOL SafeRemoveDirectory(LPCTSTR pszDir)
{
    static HDPA hdpaSF = NULL;
    int iMax;
    int iIndex;

    if (!hdpaSF && !CreateSpecialFolderDPA(&hdpaSF))
    {
        // if we cant read the special folders, error on the
        // side of caution
        return FALSE;
    }

    iMax = DPA_GetPtrCount(hdpaSF);

    for (iIndex = 0; iIndex < iMax; iIndex++)
    {
        LPTSTR pszSpecialFolder = DPA_GetPtr(hdpaSF, iIndex);

        if (!pszSpecialFolder)
            continue;

        if (lstrcmpi(pszDir, pszSpecialFolder) == 0)
            return FALSE;
    }
   
    // no special folders matched, so its ok to delete it
    return Win32RemoveDirectory(pszDir);
}


//---------------------------------------------------------------------------
void Progman_ReplaceItem(PPMDDE ppmdde, LPCTSTR szName, LPCTSTR pszCL,
        LPCTSTR szArgs, LPCTSTR szIP, int iIcon, LPCTSTR szWD)
{
        TCHAR szBuf[512];

        if (g_fDoProgmanDde)
        {
            wsprintf(szBuf, TEXT("[ReplaceItem(\"%s\")]"), szName);
            DdeClientTransaction((LPBYTE)szBuf, 
                                 (lstrlen(szBuf)+1)*SIZEOF(TCHAR),
                                 ppmdde->hconv,
                                 HSZNULL,
                                 0,
                                 XTYP_EXECUTE,
                                 DDETIMEOUT,
                                 NULL);

            wsprintf(szBuf, TEXT("[AddItem(\"%s %s\",\"%s\",%s,%d,-1,-1,%s)]"), pszCL, szArgs,
                    szName, szIP, iIcon, szWD);
            DdeClientTransaction((LPBYTE)szBuf, (lstrlen(szBuf)+1)*SIZEOF(TCHAR), ppmdde->hconv, HSZNULL, 0,
                    XTYP_EXECUTE, DDETIMEOUT, NULL);
        }
}

//---------------------------------------------------------------------------
void Progman_DeleteItem(PPMDDE ppmdde, LPCTSTR szName)
{
        // NB Progman only support 256 char commands.
        TCHAR szBuf[256];

        if (g_fDoProgmanDde)
        {
            wsprintf(szBuf, TEXT("[DeleteItem(%s)]"), szName);
            DdeClientTransaction((LPBYTE)szBuf, (lstrlen(szBuf)+1)*SIZEOF(TCHAR), ppmdde->hconv, HSZNULL, 0,
                    XTYP_EXECUTE, DDETIMEOUT, NULL);
        }
}

//---------------------------------------------------------------------------
void Reg_SetMapGroupEntry(LPCTSTR pszOld, LPCTSTR pszNew)
{
    Reg_SetString(g_hkeyGrpConv, c_szMapGroups, pszOld, pszNew);
    DebugMsg(DM_TRACE, TEXT("gc.r_cmge: From %s to %s"), pszOld, pszNew);
}

//---------------------------------------------------------------------------
void GetProperGroupName(LPCTSTR pszGroupPath, LPTSTR pszGroup, int cchGroup)
{    
    LPTSTR pszGroupName;
    
   // Progman only supports a single level hierachy so...
    pszGroupName = PathFindFileName(pszGroupPath);

    // NB If we do have a group within a group then we should add a 
    // MapGroup entry to the registry so running GrpConv in the
    // future won't cause groups to get duplicated.
    if (lstrcmpi(pszGroupName, pszGroupPath) != 0)
    {
        Reg_SetMapGroupEntry(pszGroupName, pszGroupPath);
    }
        
    // A missing group name implies use a default.
    if (!pszGroupName || !*pszGroupName)
    {
        LoadString(g_hinst, IDS_PROGRAMS, pszGroup, cchGroup);
    }
    else
    {
        lstrcpyn(pszGroup, pszGroupName, cchGroup);
    }
}

//---------------------------------------------------------------------------
BOOL Progman_CreateGroup(PPMDDE ppmdde, LPCTSTR pszGroupPath)
{
        // NB Progman only support 256 char commands.
        TCHAR szBuf[256];
        TCHAR szGroup[MAX_PATH];
    HDDEDATA hdata;

    GetProperGroupName(pszGroupPath, szGroup, ARRAYSIZE(szGroup));
    
    if (g_fDoProgmanDde)
    {
        wsprintf(szBuf, TEXT("[CreateGroup(%s)]"), szGroup);
        hdata = DdeClientTransaction((LPBYTE)szBuf, (lstrlen(szBuf)+1)*SIZEOF(TCHAR), ppmdde->hconv, HSZNULL, 0,
                XTYP_EXECUTE, DDETIMEOUT, NULL);
        Assert(hdata);
    }
    else
        return FALSE;
    
    return hdata ? TRUE : FALSE;
}

//---------------------------------------------------------------------------
BOOL Progman_ShowGroup(PPMDDE ppmdde, LPCTSTR pszGroupPath)
{
    // NB Progman only support 256 char commands.
    TCHAR szBuf[256];
    TCHAR szGroup[MAX_PATH];
    HDDEDATA hdata;
 
    GetProperGroupName(pszGroupPath, szGroup, sizeof(szGroup));

    if (g_fDoProgmanDde)
    {
        wsprintf(szBuf, TEXT("[ShowGroup(%s, %d)]"), szGroup, SW_SHOWNORMAL);
        hdata = DdeClientTransaction((LPBYTE)szBuf, (lstrlen(szBuf)+1)*SIZEOF(TCHAR), ppmdde->hconv, HSZNULL, 0,
            XTYP_EXECUTE, DDETIMEOUT, NULL);
        Assert(hdata);
    }
    else
        return FALSE;
    
    return hdata ? TRUE : FALSE;
}


// Given a string that potentially could be "::{GUID}:DATA::....::{GUID}:DATA::Path",
// return the pointer to the path.  This starts after the last double-colon sequence.
// (Darwin and Logo3 uses this format.)
LPTSTR FindPathSection(LPCTSTR pszPath)
{
    LPCTSTR psz = pszPath;
    LPCTSTR pszFirstColon = NULL;
    LPCTSTR pszDblColon = NULL;

    // Find the last double-colon sequence
    while (*psz)
    {
        if (*psz == CH_COLON)
        {
            // Was the previous character a colon too?
            if (pszFirstColon)
            {
                // Yes; remember that position
                pszDblColon = pszFirstColon;
                pszFirstColon = NULL;
            }
            else
            {
                // No; remember this as a potential for being the first colon
                // in a double-colon sequence
                pszFirstColon = psz;
            }
        }
        else
            pszFirstColon = NULL;

        psz = CharNext(psz);
    }

    if (pszDblColon)
        return (LPTSTR)pszDblColon+2;       // skip the double-colon

    return (LPTSTR)pszPath;
}


#define BG_DELETE_EMPTY                 0x0001
#define BG_PROG_GRP_CREATED             0x0002
#define BG_PROG_GRP_SHOWN               0x0004
#define BG_SEND_TO_GRP                  0x0008
#define BG_LFN                          0x0010
#define BG_RECENT_DOCS                  0x0020
#define BG_SET_PROGRESS_TEXT            0x0040
#define BG_FORCE_DESKTOP                0x0080
#define BG_FORCE_STARTUP                0x0100
#define BG_FORCE_RECENT                 0x0200
#define BG_FORCE_SENDTO                 0x0400

//---------------------------------------------------------------------------
void BuildGroup(LPCTSTR lpszIniFileName, LPCTSTR lpszSection, 
        LPCTSTR lpszGroupName, PPMDDE ppmdde, BOOL fUpdFolder, DWORD dwFlags)
{
    // Data associated with readining in section.
    HGLOBAL hg;
    LPTSTR lpBuf;       // Pointer to buffer to read section into
    int cb;
    LPTSTR pszLine;
    IShellLink *psl;
    TCHAR szName[MAX_PATH];
    TCHAR szCL[3*MAX_PATH]; // we make this 3*MAX_PATH so that DARWIN and LOGO3 callers can pass the extra information
    TCHAR szIP[2*MAX_PATH];
    TCHAR szArgs[2*MAX_PATH];
    TCHAR szGroupFolder[MAX_PATH];
    TCHAR szSpecialGrp[32];
    WCHAR wszPath[2*MAX_PATH];
    TCHAR szWD[2*MAX_PATH];
    TCHAR szDesc[3*MAX_PATH];
    TCHAR szNum[8];      // Should never exceed this!
    LPTSTR lpszArgs;
    TCHAR szCLPathPart[3*MAX_PATH]; // this 3*MAX_PATH because we use them to twiddle with szCL
    TCHAR szCLSpecialPart[3*MAX_PATH]; // this 3*MAX_PATH because we use them to twiddle with szCL
    int iLen;
    int iIcon;
    LPTSTR pszExt;
    // DWORD dwFlags = BG_DELETE_EMPTY;
    
    // BOOL fDeleteEmpty = TRUE;
    // BOOL fProgGrpCreated = FALSE;
    // BOOL fProgGrpShown = FALSE;
    // BOOL fSendToGrp = FALSE;
    // BOOL fLFN;


    Log(TEXT("Setup.Ini: %s"), lpszGroupName);
        
    DebugMsg(GC_TRACE, TEXT("gc.bg: Rebuilding %s"), (LPTSTR) lpszGroupName);

    // Special case [SendTo] section name - this stuff doesn't
    // need to be added to progman.
    LoadString(g_hinst, IDS_SENDTO, szSpecialGrp, ARRAYSIZE(szSpecialGrp));
    if ((dwFlags & BG_FORCE_SENDTO) || (lstrcmpi(lpszSection, szSpecialGrp) == 0))
    {
        DebugMsg(GC_TRACE, TEXT("gc.bg: SendTo section - no Progman group"));
        // fSendToGrp = TRUE;
        dwFlags |= BG_SEND_TO_GRP;
    }

    // Now lets read in the section for the group from the ini file
    // First allocate a buffer to read the section into
    hg  = GlobalAlloc(GPTR, BUFSIZES);  // Should never exceed 64K?
    if (hg)
    {
        lpBuf = GlobalLock(hg);

        // Special case the startup group. 
        LoadString(g_hinst, IDS_STARTUP, szSpecialGrp, ARRAYSIZE(szSpecialGrp));
        // Is this the startup group?
        szGroupFolder[0] = TEXT('\0');
        if ((dwFlags & BG_FORCE_STARTUP) || (lstrcmpi(szSpecialGrp, lpszGroupName) == 0))
        {
            DebugMsg(DM_TRACE, TEXT("gc.bg: Startup group..."));
            // Yep, Try to get the new location.
            Reg_GetString(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER_SHELLFOLDERS, c_szStartup,
            szGroupFolder, SIZEOF(szGroupFolder));
            // fDeleteEmpty = FALSE;
            dwFlags &= ~BG_DELETE_EMPTY;
        }
           
        // Is this the desktop folder?
        LoadString(g_hinst, IDS_DESKTOP, szSpecialGrp, ARRAYSIZE(szSpecialGrp));
        if ((dwFlags & BG_FORCE_RECENT) || (lstrcmp(szSpecialGrp, PathFindFileName(lpszGroupName)) == 0))
        {
            DebugMsg(DM_TRACE, TEXT("gc.bg: Desktop group..."));
            // fDeleteEmpty = FALSE;
            dwFlags &= ~BG_DELETE_EMPTY;
        }

        // Special case the recent folder.
        LoadString(g_hinst, IDS_RECENT, szSpecialGrp, ARRAYSIZE(szSpecialGrp));
        if (lstrcmp(szSpecialGrp, lpszGroupName) == 0)
        {
            DebugMsg(DM_TRACE, TEXT("gc.bg: Recent group..."));
            dwFlags |= BG_RECENT_DOCS;
            dwFlags &= ~BG_DELETE_EMPTY;
        }
        
        if (SUCCEEDED(ICoCreateInstance(&CLSID_ShellLink, &IID_IShellLink, &psl)))
        {
            IPersistFile *ppf;
            psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, &ppf);


            // now Read in the secint into our buffer
            cb = GetPrivateProfileSection(lpszSection, lpBuf, BUFSIZES/SIZEOF(TCHAR), lpszIniFileName);

            if (cb > 0)
            {
                pszLine = lpBuf;

                // Create the folder...
                // Use a generic name until we get items to add so we
                // don't stick group names like "AT&T" in users faces
                // when all we're trying to do is delete items from them.
                Group_SetProgressNameAndRange((LPCTSTR)-1, cb);

                // Did we fill in the szGroupFolder yet?
                if (!*szGroupFolder)
                {
                    // some people pass us a fully qualified path for lpszGroupName (eg c:\foo\bar or \\pyrex\user\foo)
                    // if that is the case, then use the path they specify
                    if ((PathGetDriveNumber((LPTSTR)lpszGroupName) != -1) || PathIsUNC((LPTSTR)lpszGroupName))
                    {
                        lstrcpy(szGroupFolder, lpszGroupName);
                        iLen = 2; // let PathRemoveIllegalChars validate the whole string after "c:" or "\\"
                    }
                    else
                    {
                        // non-fully qualified groupname, so just construct it under startmenu\programs
                        SHGetSpecialFolderPath(NULL, szGroupFolder, CSIDL_PROGRAMS, TRUE);

                        iLen = lstrlen(szGroupFolder);
                        PathAppend(szGroupFolder, lpszGroupName);
                    }

                    PathRemoveIllegalChars(szGroupFolder, iLen, PRICF_ALLOWSLASH);
                    // This should take care of mapping it if machine does not support LFNs.
                    PathQualify(szGroupFolder);
                }
                else
                {
                    DebugMsg(DM_TRACE, TEXT("gc.bg: Startup group mapped to %s."), szGroupFolder);
                }

                if (fUpdFolder && !(dwFlags & BG_RECENT_DOCS))
                {
                    if (!PathFileExists(szGroupFolder))
                    {
                        if (SHCreateDirectory(NULL, szGroupFolder) != 0)
                        {
                            DebugMsg(DM_ERROR, TEXT("gc.bg: Can't create %s folder."), (LPTSTR) szGroupFolder);
                        }
                    }
                }

                // Keep track if we can create LFN link names on this drive.
                // fLFN = IsLFNDrive(szGroupFolder);
                if (IsLFNDrive((LPCTSTR)szGroupFolder))
                    dwFlags |= BG_LFN;
#ifdef DEBUG                
                if (!(dwFlags & BG_LFN))
                    DebugMsg(DM_TRACE, TEXT("gc.bg: Using short names for this group."), szName);
#endif
                        
                // Add the items...
                //
                // Warning: it appears like the data in the setup.ini file does not
                // match the standard x=y, but is simpy x or x,y,z so we must
                // 1 bias the indexes to ParseField
                while (*pszLine)
                {
                    // Set progress on how many bytes we have processed.
                    Group_SetProgress((int)(pszLine-lpBuf));
                    DebugMsg(GC_TRACE, TEXT("gc.bg: Create Link:%s"), (LPTSTR)pszLine);

                    // Add item.
                    // Get the short name if we're on a SFN drive.
                    szName[0] = TEXT('\0');
                    if (!(dwFlags & BG_LFN))
                        ParseField(pszLine, 7, szName, ARRAYSIZE(szName));
                    // Get the long name if we're not on an SFN drive
                    // or if there is no short name.                   
                    if (!*szName)
                        ParseField(pszLine, 1, szName, ARRAYSIZE(szName));

                    DebugMsg(GC_TRACE, TEXT("  Link:%s"), (LPTSTR)szName);

                    
                    // Dutch/French sometimes have illegal chars in their ini files.
                    // NB Progman needs the unmangled names so only remove illegal chars
                    // from the Explorer string, not szName.
                    // NB Names can contain slashes so PathFindFileName() isn't very
                    // useful here.
                    iLen = lstrlen(szGroupFolder);
                    PathAppend(szGroupFolder, szName);
                    PathRemoveIllegalChars(szGroupFolder, iLen+1, PRICF_NORMAL);

                    // Handle LFNs on a SFN volume.
                    PathQualify(szGroupFolder);

                    if (ParseField(pszLine, 2, szCL, ARRAYSIZE(szCL)) && (*szCL != 0))
                    {
                        // assume that this is not a DARWIN or LOGO3 special link, and thus
                        // the path is just what we just read (szCL)
                        lstrcpy(szCLPathPart, szCL);
                        lstrcpy(szCLSpecialPart, szCL);

                        // We're going to have to add something to the group,
                        // switch to using it's real name.
                        if (!(dwFlags & BG_SET_PROGRESS_TEXT))
                        {
                            dwFlags |= BG_SET_PROGRESS_TEXT;
                            Group_SetProgressNameAndRange(lpszGroupName, cb);
                        }

                        // see if we have ":: or just :: which indicates a special link.
                        // special links have a path that is of the form:
                        //
                        //      ::{GUID1}:data1::{GUID2}:data2::fullpathtolinktarget
                        //
                        // where there could be any number of guid+data sections and the full
                        // path to the link target at the end is optional.
                        //
                        // We seperate this out into the "special" part which contains the guids
                        // and the "path" part which has the fullpathtolinktarget at the end.

                        if (szCLSpecialPart[0]==TEXT('"') && szCLSpecialPart[1]==TEXT(':') && szCLSpecialPart[2]==TEXT(':'))
                        {
                            // the string was quoted and it is a special string
                            LPTSTR pszRealPathBegins;
                            int cch = lstrlen(szCLSpecialPart)+1;

                            // get rid of the leading "
                            hmemcpy(szCLSpecialPart, szCLSpecialPart+1, cch * SIZEOF(TCHAR));
                       
                            // find where the real path begins
                            pszRealPathBegins = FindPathSection(szCLSpecialPart);

                            if (*pszRealPathBegins)
                            {
                                // a path part exists, so add a leading ", and copy
                                // the real fullpathtolinktarget there.
                                lstrcpy(szCLPathPart, TEXT("\""));
                                lstrcat(szCLPathPart, pszRealPathBegins);

                                // terminate the special part after the last ::
                                *pszRealPathBegins = TEXT('\0');
                            }
                            else
                            {
                                // no there is no real path, just special info
                                *szCLPathPart = TEXT('\0');
                            }
                        }
                        else if (szCLSpecialPart[0]==TEXT(':') && szCLSpecialPart[1]==TEXT(':'))
                        {
                            // the string was not quoted and it is a special string
                            LPTSTR pszRealPathBegins = FindPathSection(szCLSpecialPart);

                            if (*pszRealPathBegins)
                            {
                                // we have a real path, so save it
                                lstrcpy(szCLPathPart, pszRealPathBegins);

                                // terminate the special part after the last ::
                                *pszRealPathBegins = TEXT('\0');
                            }
                            else
                            {
                                // no there is no real path, just special info
                                *szCLPathPart = TEXT('\0');
                            }
                        }
                        else
                        {
                            // not a "special" link
                            *szCLSpecialPart = TEXT('\0');
                        }
                            
                        if (*szCLPathPart)
                        {
                            // we have a command line so check for args
                            szArgs[0] = TEXT('\0');
                            lpszArgs = PathGetArgs(szCLPathPart);
                            if (*lpszArgs)
                            {
                                *(lpszArgs-1) = TEXT('\0');
                                lstrcpyn(szArgs, lpszArgs, ARRAYSIZE(szArgs));
                                DebugMsg(GC_TRACE, TEXT("   Cmd Args:%s"), szArgs);
                            }
                            psl->lpVtbl->SetArguments(psl, szArgs);       // arguments

                            PathUnquoteSpaces(szCLPathPart);
                            PathResolve(szCLPathPart, NULL, 0);

                            DebugMsg(GC_TRACE, TEXT("   cmd:%s"), (LPTSTR)szCLPathPart);
                        }

                        if (*szCLPathPart && (dwFlags & BG_RECENT_DOCS))
                        {
                            SHAddToRecentDocs(SHARD_PATH, szCLPathPart);

                            // Progman is just going to get a group called "Documents".
                            if (!(dwFlags & BG_PROG_GRP_CREATED))
                            {
                                if (Progman_CreateGroup(ppmdde, lpszGroupName))
                                    dwFlags |= BG_PROG_GRP_CREATED;
                            }
                            
                            if (dwFlags & BG_PROG_GRP_CREATED)
                                Progman_ReplaceItem(ppmdde, szName, szCLPathPart, NULL, NULL, 0, NULL);
                        }
                        else if (*szCLPathPart || *szCLSpecialPart)
                        {
                            // all we need to call is setpath, it takes care of creating the
                            // pidl for us.  We have to put back the special / path portions here
                            // so we can pass the full DARWIN or LOGO3 information.
                            lstrcpy(szCL, szCLSpecialPart);
                            lstrcat(szCL, szCLPathPart);

                            psl->lpVtbl->SetPath(psl, szCL);
                            // Icon file.
                            ParseField(pszLine, 3, szIP, ARRAYSIZE(szIP));
                            ParseField(pszLine, 4, szNum, ARRAYSIZE(szNum));
                            iIcon = StrToInt(szNum);

                            DebugMsg(GC_TRACE, TEXT("   Icon:%s"), (LPTSTR)szIP);

                            psl->lpVtbl->SetIconLocation(psl, szIP, iIcon);
                            lstrcat(szGroupFolder, TEXT(".lnk"));


                            // NB Field 5 is dependancy stuff that we don't
                            // care about.

                            // WD
#ifdef WINNT
                            /* For NT default to the users home directory, not nothing (which results in
                            /  the current directory, which is unpredictable) */
                            lstrcpy( szWD, TEXT("%HOMEDRIVE%%HOMEPATH%") );
#else
                            szWD[0] = TEXT('\0');
#endif
                            ParseField(pszLine, 6, szWD, ARRAYSIZE(szWD));
                            psl->lpVtbl->SetWorkingDirectory(psl, szWD);

                            // Field 8 is description for the link
                            ParseField(pszLine, 8, szDesc, ARRAYSIZE(szDesc));
                            DebugMsg(GC_TRACE, TEXT("    Description:%s"), (LPTSTR)szDesc);
                            psl->lpVtbl->SetDescription(psl, szDesc);
                            
                            StrToOleStrN(wszPath, ARRAYSIZE(wszPath), szGroupFolder, -1);
                            if (fUpdFolder)
                                ppf->lpVtbl->Save(ppf, wszPath, TRUE);
                                
                            // We've added stuff so don't bother trying to delete the folder
                            // later.
                            // fDeleteEmpty = FALSE;
                            dwFlags &= ~BG_DELETE_EMPTY;
                            
                            // Defer group creation.
                            // if (!fSendToGrp && !fProgGrpCreated)
                            if (!(dwFlags & BG_SEND_TO_GRP) && !(dwFlags & BG_PROG_GRP_CREATED))
                            {
                                if (Progman_CreateGroup(ppmdde, lpszGroupName))
                                    dwFlags |= BG_PROG_GRP_CREATED;
                            }
                            
                            // if (fProgGrpCreated)
                            if (dwFlags & BG_PROG_GRP_CREATED)
                            {
                                // use szCLPathPart for good ol'e progman
                                Progman_ReplaceItem(ppmdde, szName, szCLPathPart, szArgs, szIP, iIcon, szWD);
                            }
                        }
                        else
                        {
                            // NB The assumption is that setup.ini will only contain links
                            // to files that exist. If they don't exist we assume we have
                            // a bogus setup.ini and skip to the next item.
                            DebugMsg(DM_ERROR, TEXT("gc.bg: Bogus link info for item %s in setup.ini"), szName);
                        }
                    }
                    else
                    {
                        // Delete all links with this name.
                        // NB We need to get this from the registry eventually.
                        if (fUpdFolder)
                        {
                            pszExt = szGroupFolder + lstrlen(szGroupFolder);
                            lstrcpy(pszExt, TEXT(".lnk"));
                            Win32DeleteFile(szGroupFolder);
                            lstrcpy(pszExt, TEXT(".pif"));
                            Win32DeleteFile(szGroupFolder);
                        }
                        
                        // Tell progman too. Be careful not to create empty groups just
                        // to try to delete items from it.
                        // if (!fProgGrpShown)
                        if (!(dwFlags & BG_PROG_GRP_SHOWN))
                        {
                            // Does the group already exist?
                            if (Progman_ShowGroup(ppmdde, lpszGroupName))
                               dwFlags |= BG_PROG_GRP_SHOWN;
                               
                            // if (fProgGrpShown)
                            if (dwFlags & BG_PROG_GRP_SHOWN)
                            {
                                // Yep, activate it.
                               Progman_CreateGroup(ppmdde, lpszGroupName);
                            }
                        }

                        // If it exists, then delete the item otherwise don't bother.    
                        // if (fProgGrpShown)
                        if (dwFlags & BG_PROG_GRP_SHOWN)
                            Progman_DeleteItem(ppmdde, szName);
                    }

                    PathRemoveFileSpec(szGroupFolder);       // rip the link name off for next link

                    // Now point to the next line
                    pszLine += lstrlen(pszLine) + 1;
                }
            }

            // The group might now be empty now - try to delete it, if there's still
            // stuff in there then this will safely fail. NB We don't delete empty
            // Startup groups to give users a clue that it's something special.
            
            // if (fUpdFolder && fDeleteEmpty && *szGroupFolder)
            if (fUpdFolder && (dwFlags & BG_DELETE_EMPTY) && *szGroupFolder)
            {
                DebugMsg(DM_TRACE, TEXT("gc.bg: Deleting %s"), szGroupFolder);
                
                // keep trying to remove any directories up the path,
                // so we dont leave an empty directory tree structure.
                //
                // SafeRemoveDirectory fails if the directory is a special folder
                if(SafeRemoveDirectory(szGroupFolder))
                {
                    while(PathRemoveFileSpec(szGroupFolder))
                    {
                        if (!SafeRemoveDirectory(szGroupFolder))
                            break;
                    }
                }
            }

            ppf->lpVtbl->Release(ppf);
            psl->lpVtbl->Release(psl);
        }
    }

    GlobalFree(hg);

    Log(TEXT("Setup.Ini: %s done."), lpszGroupName);
}

//---------------------------------------------------------------------------
HDDEDATA CALLBACK DdeCallback(UINT uType, UINT uFmt, HCONV hconv, HSZ hsz1, 
        HSZ hsz2, HDDEDATA hdata, ULONG_PTR dwData1, ULONG_PTR dwData2)
{
        return (HDDEDATA) NULL;
}

//---------------------------------------------------------------------------
BOOL _PartnerIsCabinet(HCONV hconv)
{
    //
    // (reinerf)
    // this sends the magical string [ConfirmCabinetID] to our current DDE partner.
    // Explorer.exe will return TRUE here, so we can distinguish it from progman.exe
    // which returns FALSE.
    //
        if (DdeClientTransaction((LPBYTE)c_szRUCabinet, SIZEOF(c_szRUCabinet),
                hconv, HSZNULL, 0, XTYP_EXECUTE, DDETIMEOUT, NULL))
        {
                return TRUE;
        }
        else
        {
                return FALSE;
        }
}

//---------------------------------------------------------------------------
// If progman is not the shell then it will be refusing DDE messages so we
// have to enable it here.
void _EnableProgmanDDE(void)
{
        HWND hwnd;

        hwnd = FindWindow(c_szProgman, NULL);
        while (hwnd)
        {
                // Is it progman?
                if (GetProp(hwnd, c_szAppProgman))
                {
                        DebugMsg(DM_TRACE, TEXT("gc.epd: Found progman, enabling dde."));
                        // NB Progman will clean this up at terminate time.
                        SetProp(hwnd, c_szEnableDDE, (HANDLE)TRUE);
                        break;
                }
                hwnd = GetWindow(hwnd, GW_HWNDNEXT);
        }
}

//---------------------------------------------------------------------------
// Will the real progman please stand up?
BOOL Progman_DdeConnect(PPMDDE ppmdde, HSZ hszService, HSZ hszTopic)
{
        HCONV hconv = HCONVNULL;
        
        Assert(ppmdde);

        DebugMsg(DM_TRACE, TEXT("gc.p_dc: Looking for progman..."));

        _EnableProgmanDDE();

        ppmdde->hcl = DdeConnectList(ppmdde->dwInst, hszService, hszTopic, HCONVLISTNULL, NULL);
        if (ppmdde->hcl)
        {
                hconv = DdeQueryNextServer(ppmdde->hcl, hconv);
                while (hconv)
                {       
                        // DdeQueryConvInfo(hconv, QID_SYNC, &ci);
                        if (!_PartnerIsCabinet(hconv))
                        {
                                DebugMsg(DM_TRACE, TEXT("gc.p_dc: Found likely candidate %x"), hconv);
                                ppmdde->hconv = hconv;
                                return TRUE;
                        }
                        else
                        {
                                DebugMsg(DM_TRACE, TEXT("gc.p_dc: Ignoring %x"), hconv);
                        }
                        hconv = DdeQueryNextServer(ppmdde->hcl, hconv);
                }
        }
        DebugMsg(DM_TRACE, TEXT("gc.p_dc: Couldn't find it."));
        return FALSE;
}

//---------------------------------------------------------------------------
BOOL Window_CreatedBy16bitProcess(HWND hwnd)
{
    DWORD idProcess;

#ifdef WINNT
    return( LOWORD(GetWindowLongPtr(hwnd,GWLP_HINSTANCE)) != 0 );
#else
    GetWindowThreadProcessId(hwnd, &idProcess);
    return GetProcessDword(idProcess, GPD_FLAGS) & GPF_WIN16_PROCESS;
#endif
}

//---------------------------------------------------------------------------
// (reinerf)
// 
// check what the user has as their shell= set to (this is in the
// registry on NT and in the win.ini on win95/memphis.
BOOL IsShellExplorer()
{
    TCHAR szShell[MAX_PATH];

#ifdef WINNT
    {
        HKEY hKeyWinlogon;
        DWORD dwSize;

        szShell[0] = TEXT('\0');

        // Starting with NT4 Service Pack 3, NT honors the value in HKCU over
        // the one in HKLM, so read that first.
        if (RegOpenKeyEx(HKEY_CURRENT_USER,
                         TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                         0L,
                         KEY_QUERY_VALUE,
                         &hKeyWinlogon) == ERROR_SUCCESS)
        {
            dwSize = SIZEOF(szShell);
            RegQueryValueEx(hKeyWinlogon, TEXT("shell"), NULL, NULL, (LPBYTE)szShell, &dwSize);
            RegCloseKey(hKeyWinlogon);
        }

        if (!szShell[0])
        {
            // no HKCU value, so check HKLM
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                             0L,
                             KEY_QUERY_VALUE,
                             &hKeyWinlogon) == ERROR_SUCCESS)
            {
                dwSize = SIZEOF(szShell);
                RegQueryValueEx(hKeyWinlogon, TEXT("shell"), NULL, NULL, (LPBYTE)szShell, &dwSize);
                RegCloseKey(hKeyWinlogon);
            }
        }
    }
#else
    {
        // on win95 we need to read the shell= line from the win.ini
        GetPrivateProfileString(TEXT("boot"),
                                TEXT("shell"),
                                TEXT("explorer.exe"),
                                szShell,
                                MAX_PATH,
                                TEXT("system.ini"));
    }
#endif

    if (lstrcmpi(TEXT("explorer.exe"), szShell) == 0)
        return TRUE;
    else
        return FALSE;
}

//---------------------------------------------------------------------------
BOOL Progman_IsRunning(void)
{
    HWND hwnd;
    TCHAR sz[MAX_PATH];

    hwnd = GetWindow(GetDesktopWindow(), GW_CHILD);
    while (hwnd)
    {
        GetClassName(hwnd, sz, ARRAYSIZE(sz));
#ifdef WINNT
        if (lstrcmpi(sz, c_szProgman) == 0)
#else
        if (Window_CreatedBy16bitProcess(hwnd) && 
            (lstrcmpi(sz, c_szProgman) == 0))
#endif
        {
            return TRUE;
        }
        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }
    return FALSE;
}

//---------------------------------------------------------------------------
BOOL Progman_Startup(PPMDDE ppmdde)
{
    HSZ hszService, hszTopic;
    TCHAR szWindowsDir[MAX_PATH];
    int i = 0;
    
    Assert(ppmdde);
        
    // if the users shell is explorer, we dont bother
    // launching progman.exe, or doing any DDE bullshit
    if (IsShellExplorer())
    {
        g_fInitDDE = FALSE;
        g_fDoProgmanDde = FALSE;
        ppmdde->fStartedProgman = FALSE;
        return FALSE;
    }

    // Is Progman running?
    if (Progman_IsRunning())
    {
        // Yep.
        DebugMsg(DM_TRACE, TEXT("gc.p_s: Progman is already running."));
        ppmdde->fStartedProgman = FALSE;
    }        
    else
    {
        // Nope - we'll try to startit.
        DebugMsg(DM_TRACE, TEXT("gc.p_s: Starting Progman..."));
        ppmdde->fStartedProgman = TRUE;


        GetWindowsDirectory(szWindowsDir, MAX_PATH);
#ifdef UNICODE
        // on WINNT progman lives in %windir%\system32
        lstrcat(szWindowsDir, TEXT("\\System32\\"));
#else
        // on win95 & memphis, progman lives in %windir%
        lstrcat(szWindowsDir, TEXT("\\"));
#endif
        lstrcat(szWindowsDir, c_szProgmanExe);

#ifdef UNICODE
        {
            STARTUPINFO si;
            PROCESS_INFORMATION pi;

            si.cb              = SIZEOF(si);
            si.lpReserved      = NULL;
            si.lpDesktop       = NULL;
            si.lpTitle         = NULL;
            si.dwX             = (DWORD)CW_USEDEFAULT;
            si.dwY             = (DWORD)CW_USEDEFAULT;
            si.dwXSize         = (DWORD)CW_USEDEFAULT;
            si.dwYSize         = (DWORD)CW_USEDEFAULT;
            si.dwXCountChars   = 0;
            si.dwYCountChars   = 0;
            si.dwFillAttribute = 0;
            si.dwFlags         = STARTF_USESHOWWINDOW;
            si.wShowWindow     = SW_HIDE;
            si.cbReserved2     = 0;
            si.lpReserved2     = 0;
            si.hStdInput       = NULL;
            si.hStdOutput      = NULL;
            si.hStdError       = NULL;

            if (CreateProcess(szWindowsDir, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
#else
        WinExec(szWindowsDir, SW_HIDE);
#endif
        // Give progman a bit of time to startup but bail after 10s.
        while (!Progman_IsRunning() && (i < 10))
        {
            Sleep(1000);
            i++;
        }
    }

    // Just a bit longer.
    Sleep(1000);
    
    // Grab the focus back?
    if (g_hwndProgress)
            SetForegroundWindow(g_hwndProgress);

    // we are going to try to do DDE, so set g_fInitDDE = TRUE,
    // so that we know to call DdeUninitialize later
    g_fInitDDE = TRUE;

    ppmdde->dwInst = 0;
    DdeInitialize(&ppmdde->dwInst, DdeCallback, APPCLASS_STANDARD|APPCMD_CLIENTONLY, 0);
    hszService = DdeCreateStringHandle(ppmdde->dwInst, (LPTSTR)c_szProgman, CP_WINNEUTRAL);
    hszTopic = DdeCreateStringHandle(ppmdde->dwInst, (LPTSTR)c_szProgman, CP_WINNEUTRAL);
    g_fDoProgmanDde = Progman_DdeConnect(ppmdde, hszService, hszTopic);
    DdeFreeStringHandle(ppmdde->dwInst, hszService);
    DdeFreeStringHandle(ppmdde->dwInst, hszTopic);
    
    return g_fDoProgmanDde;
}

//---------------------------------------------------------------------------
BOOL FindProgmanIni(LPTSTR pszPath)
{
    OFSTRUCT os;
#ifdef UNICODE
    LPTSTR   lpszFilePart;
#endif


    // NB Don't bother looking for the old windows directory, in the case of
    // an upgrade it will be the current windows directory.


    GetWindowsDirectory(pszPath, MAX_PATH);
    PathAppend(pszPath, c_szProgmanIni);

    if (PathFileExists(pszPath))
    {
        return TRUE;
    }
#ifdef UNICODE
    else if (SearchPath(NULL, c_szProgmanIni, NULL, MAX_PATH, pszPath, &lpszFilePart) != 0)
    {
        return TRUE;
    }
#else
    else if (OpenFile(c_szProgmanIni, &os, OF_EXIST) != -1)
    {
        lstrcpy(pszPath, os.szPathName);
        return TRUE;
    }
#endif

    DebugMsg(DM_ERROR, TEXT("Can't find progman.ini"));
    return FALSE;
}

//---------------------------------------------------------------------------
void UpdateTimeStampCallback(LPCTSTR lpszGroup)
{
    WIN32_FIND_DATA fd;
    HANDLE hff;

    DebugMsg(DM_TRACE, TEXT("gc.utc: Updating timestamp for %s."), lpszGroup);

    hff = FindFirstFile(lpszGroup, &fd);
    if (hff != INVALID_HANDLE_VALUE)
    {
        Group_WriteLastModDateTime(lpszGroup,fd.ftLastWriteTime.dwLowDateTime);
        FindClose(hff);
    }
}

//---------------------------------------------------------------------------
void Progman_Shutdown(PPMDDE ppmdde)
{
    TCHAR szIniFile[MAX_PATH];
    
    // only shutdown progman if we actually started it and we 
    // were doing DDE with it.
    if (ppmdde->fStartedProgman && g_fDoProgmanDde)
    {
        Log(TEXT("p_s: Shutting down progman..."));
    
        Log(TEXT("p_s: DdeClientTransaction."));
        DebugMsg(DM_TRACE, TEXT("gc.p_s: Shutting down progman."));
        DdeClientTransaction((LPBYTE)c_szExitProgman, SIZEOF(c_szExitProgman),
                ppmdde->hconv, HSZNULL, 0, XTYP_EXECUTE, DDETIMEOUT, NULL);
    }
        
    // if we initialzied DDE then uninit it now...
    if (g_fInitDDE)
    {
        Log(TEXT("p_s: DdeDisconnect."));
        DdeDisconnectList(ppmdde->hcl);

        Log(TEXT("p_s: DdeUnitialize."));
        DdeUninitialize(ppmdde->dwInst);
    }

    // We just went and modified all progman groups so update the time stamps.
    FindProgmanIni(szIniFile);
    Log(TEXT("p_s: Updating time stamps."));
    Group_Enum(UpdateTimeStampCallback, FALSE, TRUE);
    // Re-do the timestamp so that cab32 won't do another gprconv.
    UpdateTimeStampCallback(szIniFile);

    Log(TEXT("p_s: Done."));
}

//----------------------------------------------------------------------------
void BuildSectionGroups(LPCTSTR lpszIniFile, LPCTSTR lpszSection, 
    PPMDDE ppmdde, BOOL fUpdFolder, DWORD dwFlags)
{
    int cb = 0;
    LPTSTR pszLine;
    TCHAR szSectName[CCHSZSHORT];
    TCHAR szGroupName[2*MAX_PATH];
    LPTSTR lpBuf;
    
    // First allocate a buffer to read the section into
    lpBuf = (LPTSTR) GlobalAlloc(GPTR, BUFSIZES);  // Should never exceed 64K?
    if (lpBuf)
    {
        // Now Read in the secint into our buffer.
        if (PathFileExists(lpszIniFile))
            cb = GetPrivateProfileSection(lpszSection, lpBuf, BUFSIZES/SIZEOF(TCHAR), lpszIniFile);
            
        if (cb > 0)
        {
            Group_SetProgressDesc(IDS_CREATINGNEWSCS);
            pszLine = lpBuf;
            while (*pszLine)
            {
                // Make sure we did not fall off the deep end
                if (cb < (int)(pszLine - lpBuf))
                {
                    Assert(FALSE);
                    break;
                }

                // Now lets extract the fields off of the line
                ParseField(pszLine, 0, szSectName, ARRAYSIZE(szSectName));
                ParseField(pszLine, 1, szGroupName, ARRAYSIZE(szGroupName));

                // Pass off to build that group and update progman.
                BuildGroup(lpszIniFile, szSectName, szGroupName, ppmdde, fUpdFolder, dwFlags);

                // Now setup process the next line in the section
                pszLine += lstrlen(pszLine) + 1;
            }
        }
        GlobalFree((HGLOBAL)lpBuf);
        SHChangeNotify( 0, SHCNF_FLUSH, NULL, NULL);    // Kick tray into updating for real
    }
}

#ifdef WINNT
typedef UINT (__stdcall * PFNGETSYSTEMWINDOWSDIRECTORYW)(LPWSTR pwszBuffer, UINT cchSize);

//
// we need a wrapper for this since it only exists on NT5
//
UINT Wrap_GetSystemWindowsDirectoryW(LPWSTR pszBuffer, UINT cchBuff)
{
    static PFNGETSYSTEMWINDOWSDIRECTORYW s_pfn = (PFNGETSYSTEMWINDOWSDIRECTORYW)-1;

    if (s_pfn)
    {
        HINSTANCE hinst = GetModuleHandle(TEXT("KERNEL32.DLL"));

        if (hinst)
            s_pfn = (PFNGETSYSTEMWINDOWSDIRECTORYW)GetProcAddress(hinst, "GetSystemWindowsDirectoryW");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(pszBuffer, cchBuff);
    
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}
#endif // WINNT

//
// We now look for setup.ini in 3 places: first in %userprofile%, next GetWindowsDirectory(),
// and finally in the GetWindowsSystemDirectory() (since hydra can change the return value for
// GetWindowsDirectory but apps still might be putting stuff there).
//
// The reason we look in the %USERPROFILE% directory is that Win2000's new high-security model
// does not give default users write permission to %windir%, so apps will not be able to even
// create a setup.ini in that location. This breaks the per-user install stubs (ie4uinit.exe),
// who are now going to create the setup.ini in %USERPROFILE%, where the user will always have
// write permission.
//
void FindSetupIni(LPTSTR szSetupIniPath, int cchSetupIniPath)
{
    TCHAR szPath[MAX_PATH];

    ExpandEnvironmentStrings(TEXT("%USERPROFILE%"), szPath, ARRAYSIZE(szPath));
    PathAppend(szPath, c_szGrpConvInf);

    if (PathFileExists(szPath))
    {
        lstrcpyn(szSetupIniPath, szPath, cchSetupIniPath);
        return;
    }

    // next try GetWindowsDirectory()
    GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
    PathAppend(szPath, c_szGrpConvInf);

    if (PathFileExists(szPath))
    {
        lstrcpyn(szSetupIniPath, szPath, cchSetupIniPath);
        return;
    }

#ifdef WINNT
    // finally if we are on NT try GetWindowsSystemDirectory()
    if (Wrap_GetSystemWindowsDirectoryW(szPath, ARRAYSIZE(szPath)))
    {
        PathAppend(szPath, c_szGrpConvInf);

        if (PathFileExists(szPath))
        {
            lstrcpyn(szSetupIniPath, szPath, cchSetupIniPath);
            return;
        }
    }
#endif

    // We faild to find it! For compat reasons, we just do what the old code
    // does: GetWindowsDirectory() and PathAppend() and plow ahead...
    GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
    PathAppend(szPath, c_szGrpConvInf);
    return;
}


//---------------------------------------------------------------------------
// This parses the grpconv.inf file and creates the appropriate programs
// folders.
void BuildDefaultGroups(void)
{
    TCHAR szPath[MAX_PATH];
    PMDDE pmdde;

    Log(TEXT("bdg: ..."));
   
    // seek and you will find...
    FindSetupIni(szPath, ARRAYSIZE(szPath));

    // Now lets walk through the different items in this section
    Group_CreateProgressDlg();
    
    // Change the text in the progress dialog so people don't think we're
    // doing the same thing twice.
    // Group_SetProgressDesc(IDS_CREATINGNEWSCS);
    
    // Crank up Progman.
    Progman_Startup(&pmdde);
    // Build the stuff.
    BuildSectionGroups(szPath, c_szProgmanGroups, &pmdde, TRUE, BG_DELETE_EMPTY);
    BuildSectionGroups(szPath, c_szProgmanOnly, &pmdde, FALSE, BG_DELETE_EMPTY);
    // Custom sections.
    BuildSectionGroups(szPath, c_szDesktopGroups, &pmdde, FALSE, BG_FORCE_DESKTOP);
    BuildSectionGroups(szPath, c_szStartupGroups, &pmdde, FALSE, BG_FORCE_STARTUP);
    BuildSectionGroups(szPath, c_szSendToGroups, &pmdde, FALSE, BG_FORCE_SENDTO);
    BuildSectionGroups(szPath, c_szRecentDocsGroups, &pmdde, FALSE, BG_FORCE_RECENT);

    // Close down progman.
    Progman_Shutdown(&pmdde);
    Group_DestroyProgressDlg();
    // HACKHACK (reinerf) - we cant rename setup.ini -> setup.old because this causes problems
    // the second time when it will fail because setup.old already exists (and we possibly dont
    // have acls to overwrite it), and when it fails we orpan setup.ini as well (because the
    // rename failed!!). This after this, all fututre attempts to create a setup.ini will fail,
    // because one already exists, and we may not have acls to overwrite it. So, we just always
    // delete setup.ini when we are done.
    Win32DeleteFile(szPath);
        
    Log(TEXT("bdg: Done."));
}
