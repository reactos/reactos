/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Two-column Start menu in the style of Windows 7:
 *              left:  pinned and recent programs, "All Programs" (a tree of the
 *                     Start Menu\Programs folders) and a search box that finds
 *                     programs and settings;
 *              right: the user, personal folders, Computer, Settings, Control
 *                     Panel, Run... and a Shut down button with more options.
 *              The classic menu is still used when the registry value
 *              HKCU\...\Explorer\Advanced\ReactOSClassicStartMenu is 1.
 */

#include "precomp.h"

#define SM7_CLASS       L"ReactOS_StartMenu7"
#define SM7_LEFT_W      300     /* at 96 DPI */
#define SM7_RIGHT_W     180
#define SM7_HEIGHT      540
#define SM7_PAD         10
#define SM7_ITEM_H      40      /* pinned/recent rows: 32px icon */
#define SM7_RIGHT_H     32
#define SM7_ALLPROG_H   34
#define SM7_SEARCH_H    34
#define SM7_MAX_RECENT  4
#define SM7_RESULT_H    30      /* search results: small icons */
#define SM7_FILE        2       /* SM7_ITEM.Value of a found file */

#define IDC_SM7_SEARCH  100
#define IDC_SM7_TREE    101

#define IDM_SM7_LOGOFF  1
#define IDM_SM7_LOCK    2
#define IDM_SM7_RESTART 3
#define IDM_SM7_OPTIONS 4

#define RECENT_KEY      L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartPage"
#define RECENT_VALUE    L"ReactOSRecentPrograms"

enum { ACT_NONE, ACT_RUN, ACT_FOLDER, ACT_TRAYCMD, ACT_HEADER, ACT_SEPARATOR, ACT_ALLPROGRAMS };

struct SM7_ITEM
{
    CStringW Name;
    CStringW Path;      /* file/command, or keywords for search-only entries */
    CStringW Args;
    INT Action;
    INT Value;          /* CSIDL for ACT_FOLDER, TRAYCMD_* for ACT_TRAYCMD */
    INT iIcon;
};

/* Programs pinned to the top of the menu */
static const struct { LPCWSTR Name, Exe; } s_Pinned[] =
{
    { L"File Explorer", L"explorer.exe" },
    { L"Settings", L"settings.exe" },
    { L"Command Prompt", L"cmd.exe" },
    { L"Notepad", L"notepad.exe" },
    { L"Paint", L"mspaint.exe" },
    { L"Applications Manager", L"rapps.exe" },
};

/* Settings found by the search box: name, command, arguments, extra words to match */
static const struct { LPCWSTR Name, Cmd, Args, Keywords; } s_SearchSettings[] =
{
    { L"Settings", L"settings.exe", NULL, L"options preferences configure" },
    { L"Control Panel", L"control.exe", NULL, L"settings" },
    { L"Display", L"control.exe", L"desk.cpl", L"screen resolution scale dpi monitor wallpaper background theme appearance" },
    { L"Screen resolution", L"control.exe", L"desk.cpl,,3", L"display monitor size dpi scale" },
    { L"Sound", L"control.exe", L"mmsys.cpl", L"audio volume speakers microphone" },
    { L"Mouse", L"control.exe", L"main.cpl", L"pointer cursor click" },
    { L"Keyboard", L"control.exe", L"main.cpl @1", L"typing repeat" },
    { L"Network Connections", L"control.exe", L"ncpa.cpl", L"internet ethernet wifi wireless adapter ip" },
    { L"Date and Time", L"control.exe", L"timedate.cpl", L"clock time zone calendar" },
    { L"Programs and Features", L"control.exe", L"appwiz.cpl", L"uninstall remove install applications apps" },
    { L"System", L"control.exe", L"sysdm.cpl", L"computer name about performance environment variables" },
    { L"Power Options", L"control.exe", L"powercfg.cpl", L"battery sleep hibernate energy" },
    { L"Region and Language", L"control.exe", L"intl.cpl", L"locale format keyboard layout input" },
    { L"Accessibility Options", L"control.exe", L"access.cpl", L"ease of access magnifier contrast" },
    { L"Internet Options", L"control.exe", L"inetcpl.cpl", L"proxy browser web" },
    { L"Fonts", L"control.exe", L"fonts", L"typeface text" },
    { L"Device Manager", L"devmgmt.msc", NULL, L"hardware drivers devices" },
    { L"Task Manager", L"taskmgr.exe", NULL, L"processes performance kill" },
};

static INT GetSysIconIndex(LPCWSTR pszPath)
{
    SHFILEINFOW sfi = { 0 };
    if (!SHGetFileInfoW(pszPath, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON))
        return -1;
    return sfi.iIcon;
}

/* "notepad.exe" -> "C:\ReactOS\system32\notepad.exe" */
static CStringW FindExe(LPCWSTR pszExe)
{
    WCHAR szPath[MAX_PATH];
    if (SearchPathW(NULL, pszExe, NULL, _countof(szPath), szPath, NULL))
        return szPath;
    return pszExe;
}

static BOOL HasExtension(LPCWSTR pszName, LPCWSTR pszExt)
{
    SIZE_T cch = wcslen(pszName), cchExt = wcslen(pszExt);
    return cch > cchExt && !_wcsicmp(pszName + cch - cchExt, pszExt);
}

/* ---- the Start Menu\Programs folders (user + all users, merged) --------- */

struct SM7_ENTRY
{
    CStringW Name;
    CStringW Paths;     /* one or more paths separated by '|' (merged folders) */
    BOOL bFolder;
};

static VOID EnumMerged(const CStringW& Paths, CAtlArray<SM7_ENTRY>& Out)
{
    INT iStart = 0;
    CStringW Dir = Paths.Tokenize(L"|", iStart);
    while (!Dir.IsEmpty())
    {
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileW(Dir + L"\\*", &fd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (fd.cFileName[0] == L'.' || (fd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)))
                    continue;

                BOOL bFolder = !!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                CStringW Name = fd.cFileName, Path = Dir + L"\\" + fd.cFileName;
                if (!bFolder)
                {
                    if (!HasExtension(Name, L".lnk") && !HasExtension(Name, L".url") &&
                        !HasExtension(Name, L".exe") && !HasExtension(Name, L".pif"))
                    {
                        continue;
                    }
                    Name = Name.Left(Name.ReverseFind(L'.'));
                }

                SIZE_T i;
                for (i = 0; i < Out.GetCount(); i++)
                {
                    if (Out[i].bFolder == bFolder && !Out[i].Name.CompareNoCase(Name))
                        break;
                }
                if (i < Out.GetCount())
                {
                    if (bFolder)
                        Out[i].Paths += L"|" + Path;
                }
                else
                {
                    SM7_ENTRY e;
                    e.Name = Name;
                    e.Paths = Path;
                    e.bFolder = bFolder;
                    Out.Add(e);
                }
            } while (FindNextFileW(hFind, &fd));
            FindClose(hFind);
        }
        Dir = Paths.Tokenize(L"|", iStart);
    }

    /* Shortcuts first, then folders, each alphabetically (like Windows 7) */
    for (SIZE_T i = 1; i < Out.GetCount(); i++)
    {
        for (SIZE_T j = i; j > 0; j--)
        {
            SM7_ENTRY &a = Out[j - 1], &b = Out[j];
            if (a.bFolder < b.bFolder || (a.bFolder == b.bFolder && a.Name.CompareNoCase(b.Name) <= 0))
                break;
            SM7_ENTRY t = a;
            a = b;
            b = t;
        }
    }
}

static CStringW GetProgramsPaths()
{
    WCHAR szUser[MAX_PATH] = L"", szCommon[MAX_PATH] = L"";
    SHGetSpecialFolderPathW(NULL, szUser, CSIDL_PROGRAMS, FALSE);
    SHGetSpecialFolderPathW(NULL, szCommon, CSIDL_COMMON_PROGRAMS, FALSE);
    return CStringW(szUser) + L"|" + szCommon;
}

/* ---- the window --------------------------------------------------------- */

class CStartMenu7 :
    public CWindowImpl<CStartMenu7, CWindow, CWinTraits<WS_POPUP | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW | WS_EX_TOPMOST>>
{
    HWND m_hwndTray = NULL, m_hwndStartButton = NULL;
    HWND m_hwndSearch = NULL, m_hwndTree = NULL;
    HFONT m_hFont = NULL, m_hFontBold = NULL, m_hFontBig = NULL;
    HIMAGELIST m_himlSmall = NULL, m_himlLarge = NULL;
    INT m_Dpi = 96;
    BOOL m_bAllPrograms = FALSE, m_bSearching = FALSE, m_bInMenu = FALSE;

    CAtlArray<SM7_ITEM> m_Left, m_Right, m_SearchIndex, m_FileIndex;
    BOOL m_bFilesIndexed = FALSE;
    CAtlArray<CStringW> m_TreePaths;    /* lParam of tree items: index; folders start with '>' */
    INT m_HotLeft = -1, m_HotRight = -1, m_SelLeft = -1;
    BOOL m_HotShutdown = FALSE, m_HotArrow = FALSE, m_HotAllProg = FALSE;
    RECT m_rcLeftList = { 0 }, m_rcAllProg = { 0 }, m_rcSearch = { 0 }, m_rcUser = { 0 },
         m_rcShutdown = { 0 }, m_rcArrow = { 0 };
    INT m_cyRightTop = 0;
    WCHAR m_szUser[64] = L"";

public:
    DWORD m_dwHiddenTick = 0;

    DECLARE_WND_CLASS_EX(SM7_CLASS, CS_DROPSHADOW, COLOR_WINDOW)

    INT S(INT v) { return MulDiv(v, m_Dpi, 96); }

    /* ---- setup ---- */

    VOID CreateFonts()
    {
        HDC hdc = ::GetDC(NULL);
        INT Dpi = GetDeviceCaps(hdc, LOGPIXELSY);
        ::ReleaseDC(NULL, hdc);
        if (m_hFont && Dpi == m_Dpi)
            return;
        m_Dpi = Dpi;
        if (m_hFont)
        {
            DeleteObject(m_hFont);
            DeleteObject(m_hFontBold);
            DeleteObject(m_hFontBig);
        }

        LOGFONTW lf = { 0 };
        lf.lfQuality = DEFAULT_QUALITY;
        StringCchCopyW(lf.lfFaceName, _countof(lf.lfFaceName), L"Selawik");
        lf.lfHeight = -MulDiv(9, m_Dpi, 72);
        m_hFont = CreateFontIndirectW(&lf);
        StringCchCopyW(lf.lfFaceName, _countof(lf.lfFaceName), L"Selawik Semibold");
        lf.lfWeight = FW_SEMIBOLD;
        m_hFontBold = CreateFontIndirectW(&lf);
        lf.lfHeight = -MulDiv(11, m_Dpi, 72);
        m_hFontBig = CreateFontIndirectW(&lf);

        if (m_hwndSearch)
            ::SendMessageW(m_hwndSearch, WM_SETFONT, (WPARAM)m_hFont, FALSE);
        if (m_hwndTree)
        {
            ::SendMessageW(m_hwndTree, WM_SETFONT, (WPARAM)m_hFont, FALSE);
            TreeView_SetItemHeight(m_hwndTree, S(26));
        }
    }

    VOID AddItem(CAtlArray<SM7_ITEM>& List, LPCWSTR pszName, LPCWSTR pszPath, LPCWSTR pszArgs,
                 INT Action, INT Value = 0, INT iIcon = -1)
    {
        SM7_ITEM it;
        it.Name = pszName;
        it.Path = pszPath ? pszPath : L"";
        it.Args = pszArgs ? pszArgs : L"";
        it.Action = Action;
        it.Value = Value;
        it.iIcon = iIcon;
        List.Add(it);
    }

    VOID LoadRecent(CAtlArray<CStringW>& Recent)
    {
        WCHAR szBuf[4096];
        DWORD cb = sizeof(szBuf) - 2 * sizeof(WCHAR);
        ZeroMemory(szBuf, sizeof(szBuf));
        if (SHGetValueW(HKEY_CURRENT_USER, RECENT_KEY, RECENT_VALUE, NULL, szBuf, &cb) != ERROR_SUCCESS)
            return;
        for (LPCWSTR p = szBuf; *p; p += wcslen(p) + 1)
            Recent.Add(p);
    }

    /* Most recent first, no duplicates, at most 10 */
    VOID AddRecent(LPCWSTR pszPath)
    {
        CAtlArray<CStringW> Recent;
        LoadRecent(Recent);
        CStringW Data = CStringW(pszPath) + L'\0';
        INT n = 1;
        for (SIZE_T i = 0; i < Recent.GetCount() && n < 10; i++)
        {
            if (Recent[i].CompareNoCase(pszPath))
            {
                Data += Recent[i] + L'\0';
                n++;
            }
        }
        Data += L'\0';
        SHSetValueW(HKEY_CURRENT_USER, RECENT_KEY, RECENT_VALUE, REG_MULTI_SZ,
                    (LPCWSTR)Data, Data.GetLength() * sizeof(WCHAR));
    }

    /* Left column when not searching: pinned programs, a separator, recent programs */
    VOID BuildHome()
    {
        m_Left.SetCount(0);
        for (SIZE_T i = 0; i < _countof(s_Pinned); i++)
        {
            CStringW Path = FindExe(s_Pinned[i].Exe);
            if (GetFileAttributesW(Path) != INVALID_FILE_ATTRIBUTES)
                AddItem(m_Left, s_Pinned[i].Name, Path, NULL, ACT_RUN, 0, GetSysIconIndex(Path));
        }

        CAtlArray<CStringW> Recent;
        LoadRecent(Recent);
        INT n = 0;
        for (SIZE_T i = 0; i < Recent.GetCount() && n < SM7_MAX_RECENT; i++)
        {
            if (GetFileAttributesW(Recent[i]) == INVALID_FILE_ATTRIBUTES)
                continue;
            CStringW Name = Recent[i].Mid(Recent[i].ReverseFind(L'\\') + 1);
            if (Name.ReverseFind(L'.') > 0)
                Name = Name.Left(Name.ReverseFind(L'.'));
            BOOL bPinned = FALSE;
            for (SIZE_T j = 0; j < m_Left.GetCount(); j++)
                bPinned |= !m_Left[j].Name.CompareNoCase(Name);
            if (bPinned)
                continue;
            if (n == 0)
                AddItem(m_Left, L"", NULL, NULL, ACT_SEPARATOR);
            AddItem(m_Left, Name, Recent[i], NULL, ACT_RUN, 0, GetSysIconIndex(Recent[i]));
            n++;
        }
        m_SelLeft = -1;
    }

    VOID BuildRight()
    {
        m_Right.SetCount(0);
        AddItem(m_Right, L"Documents", NULL, NULL, ACT_FOLDER, CSIDL_PERSONAL);
        AddItem(m_Right, L"Pictures", NULL, NULL, ACT_FOLDER, CSIDL_MYPICTURES);
        AddItem(m_Right, L"Music", NULL, NULL, ACT_FOLDER, CSIDL_MYMUSIC);
        AddItem(m_Right, L"", NULL, NULL, ACT_SEPARATOR);
        AddItem(m_Right, L"Computer", NULL, NULL, ACT_FOLDER, CSIDL_DRIVES);
        AddItem(m_Right, L"Network", NULL, NULL, ACT_FOLDER, CSIDL_NETWORK);
        AddItem(m_Right, L"", NULL, NULL, ACT_SEPARATOR);
        AddItem(m_Right, L"Settings", FindExe(L"settings.exe"), NULL, ACT_RUN);
        AddItem(m_Right, L"Control Panel", NULL, NULL, ACT_TRAYCMD, TRAYCMD_CONTROL_PANEL);
        AddItem(m_Right, L"Devices and Printers", NULL, NULL, ACT_TRAYCMD, TRAYCMD_PRINTERS_AND_FAXES);
        AddItem(m_Right, L"", NULL, NULL, ACT_SEPARATOR);
        AddItem(m_Right, L"Help and Support", NULL, NULL, ACT_TRAYCMD, TRAYCMD_HELP_AND_SUPPORT);
        AddItem(m_Right, L"Run...", NULL, NULL, ACT_TRAYCMD, TRAYCMD_RUN_DIALOG);
    }

    /* Everything the search box can find */
    VOID IndexFolder(const CStringW& Paths, INT Depth)
    {
        CAtlArray<SM7_ENTRY> Entries;
        EnumMerged(Paths, Entries);
        for (SIZE_T i = 0; i < Entries.GetCount(); i++)
        {
            if (Entries[i].bFolder)
            {
                if (Depth < 4)
                    IndexFolder(Entries[i].Paths, Depth + 1);
            }
            else
            {
                AddItem(m_SearchIndex, Entries[i].Name, Entries[i].Paths, NULL, ACT_RUN);
            }
        }
    }

    /* Files and folders in the user's own folders (bounded: depth and count) */
    VOID IndexFiles(const CStringW& Dir, INT Depth)
    {
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileW(Dir + L"\\*", &fd);
        if (hFind == INVALID_HANDLE_VALUE)
            return;
        do
        {
            if (fd.cFileName[0] == L'.' || (fd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)))
                continue;
            CStringW Path = Dir + L"\\" + fd.cFileName;
            BOOL bDup = FALSE;
            for (SIZE_T i = 0; i < m_FileIndex.GetCount() && !bDup; i++)
                bDup = !m_FileIndex[i].Path.CompareNoCase(Path);
            if (bDup)
                continue;
            AddItem(m_FileIndex, fd.cFileName, Path, NULL, ACT_RUN, SM7_FILE);
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && Depth < 5 && m_FileIndex.GetCount() < 5000)
                IndexFiles(Path, Depth + 1);
        } while (FindNextFileW(hFind, &fd) && m_FileIndex.GetCount() < 5000);
        FindClose(hFind);
    }

    VOID BuildFileIndex()
    {
        static const INT Folders[] = { CSIDL_DESKTOPDIRECTORY, CSIDL_COMMON_DESKTOPDIRECTORY, CSIDL_PERSONAL, CSIDL_MYPICTURES,
                                       CSIDL_MYMUSIC, CSIDL_MYVIDEO };
        WCHAR szPath[MAX_PATH];
        m_FileIndex.SetCount(0);
        for (SIZE_T i = 0; i < _countof(Folders); i++)
        {
            if (SHGetSpecialFolderPathW(NULL, szPath, Folders[i], FALSE))
                IndexFiles(szPath, 0);
        }
        if (SHGetSpecialFolderPathW(NULL, szPath, CSIDL_PROFILE, FALSE))
            IndexFiles(CStringW(szPath) + L"\\Downloads", 0);
        m_bFilesIndexed = TRUE;
    }

    VOID BuildSearchIndex()
    {
        m_SearchIndex.SetCount(0);
        IndexFolder(GetProgramsPaths(), 0);
        for (SIZE_T i = 0; i < _countof(s_Pinned); i++)
        {
            CStringW Path = FindExe(s_Pinned[i].Exe);
            if (GetFileAttributesW(Path) != INVALID_FILE_ATTRIBUTES)
                AddItem(m_SearchIndex, s_Pinned[i].Name, Path, NULL, ACT_RUN);
        }
    }

    /* ---- search ---- */

    /* 3: name starts with the query, 2: every query word starts a word of the
     * name, 1: ...or of the keywords / name contains the query, 0: no match */
    static INT MatchScore(const CStringW& Query, const CStringW& Name, LPCWSTR pszKeywords)
    {
        CStringW q = Query, n = Name, k = pszKeywords ? pszKeywords : L"";
        q.MakeLower();
        n.MakeLower();
        k.MakeLower();
        q.Trim();
        if (q.IsEmpty())
            return 0;
        if (n.Find(q) == 0)
            return 3;

        CStringW Words = L" " + n + L" ", AllWords = Words + k + L" ";
        BOOL bAllInName = TRUE, bAll = TRUE;
        INT iStart = 0;
        for (CStringW w = q.Tokenize(L" ", iStart); !w.IsEmpty(); w = q.Tokenize(L" ", iStart))
        {
            if (Words.Find(L" " + w) < 0)
                bAllInName = FALSE;
            if (AllWords.Find(L" " + w) < 0)
                bAll = FALSE;
        }
        if (bAllInName)
            return 2;
        if (bAll || n.Find(q) >= 0)
            return 1;
        return 0;
    }

    VOID DoSearch()
    {
        WCHAR szQuery[128];
        ::GetWindowTextW(m_hwndSearch, szQuery, _countof(szQuery));
        CStringW Query = szQuery;
        Query.Trim();

        m_bSearching = !Query.IsEmpty();
        if (!m_bSearching)
        {
            BuildHome();
            ShowAllPrograms(FALSE);
            InvalidateRect(NULL, FALSE);
            return;
        }
        ShowAllPrograms(FALSE);
        m_Left.SetCount(0);

        /* Programs, best matches first */
        INT nFound = 0;
        for (INT Score = 3; Score >= 1; Score--)
        {
            for (SIZE_T i = 0; i < m_SearchIndex.GetCount() && nFound < 5; i++)
            {
                SM7_ITEM& it = m_SearchIndex[i];
                if (MatchScore(Query, it.Name, NULL) != Score)
                    continue;
                BOOL bDup = FALSE;
                for (SIZE_T j = 0; j < m_Left.GetCount(); j++)
                    bDup |= !m_Left[j].Name.CompareNoCase(it.Name);
                if (bDup)
                    continue;
                if (nFound++ == 0)
                    AddItem(m_Left, L"Applications", NULL, NULL, ACT_HEADER);
                if (it.iIcon < 0)
                    it.iIcon = GetSysIconIndex(it.Path);
                m_Left.Add(it);
            }
        }

        /* Files: indexed the first time the user searches after opening the menu */
        if (!m_bFilesIndexed)
            BuildFileIndex();
        INT nFiles = 0;
        for (INT Score = 3; Score >= 1; Score--)
        {
            for (SIZE_T i = 0; i < m_FileIndex.GetCount() && nFiles < 4; i++)
            {
                SM7_ITEM& it = m_FileIndex[i];
                if (MatchScore(Query, it.Name, NULL) != Score)
                    continue;
                if (nFiles++ == 0)
                    AddItem(m_Left, L"Files", NULL, NULL, ACT_HEADER);
                if (it.iIcon < 0)
                    it.iIcon = GetSysIconIndex(it.Path);
                m_Left.Add(it);
            }
        }

        /* Settings */
        INT nSettings = 0;
        CStringW Control = FindExe(L"control.exe");
        for (INT Score = 3; Score >= 1; Score--)
        {
            for (SIZE_T i = 0; i < _countof(s_SearchSettings) && nSettings < 3; i++)
            {
                if (MatchScore(Query, s_SearchSettings[i].Name, s_SearchSettings[i].Keywords) != Score)
                    continue;
                if (nSettings++ == 0)
                    AddItem(m_Left, L"Settings", NULL, NULL, ACT_HEADER);
                CStringW Cmd = FindExe(s_SearchSettings[i].Cmd);
                AddItem(m_Left, s_SearchSettings[i].Name, Cmd, s_SearchSettings[i].Args, ACT_RUN, 0,
                        GetSysIconIndex(GetFileAttributesW(Cmd) != INVALID_FILE_ATTRIBUTES ? Cmd : Control));
            }
        }

        /* Nothing found: Enter runs the text as a command, like Windows 7 */
        if ((m_Left.GetCount() == 0))
        {
            AddItem(m_Left, L"No items match your search.", NULL, NULL, ACT_HEADER);
            CStringW Run;
            Run.Format(L"Run \"%s\"", (LPCWSTR)Query);
            AddItem(m_Left, Run, Query, NULL, ACT_RUN, 1, GetSysIconIndex(FindExe(L"cmd.exe")));
        }

        /* The first result is selected: Enter opens it */
        m_SelLeft = -1;
        for (SIZE_T i = 0; i < m_Left.GetCount() && m_SelLeft < 0; i++)
        {
            if (m_Left[i].Action == ACT_RUN)
                m_SelLeft = (INT)i;
        }
        m_HotLeft = -1;
        InvalidateRect(NULL, FALSE);
    }

    /* ---- All Programs tree ---- */

    VOID FillTree(HTREEITEM hParent, const CStringW& Paths)
    {
        CAtlArray<SM7_ENTRY> Entries;
        EnumMerged(Paths, Entries);
        for (SIZE_T i = 0; i < Entries.GetCount(); i++)
        {
            CStringW FirstPath = Entries[i].Paths;
            if (FirstPath.Find(L'|') > 0)
                FirstPath = FirstPath.Left(FirstPath.Find(L'|'));

            TVINSERTSTRUCTW tvis = { hParent, TVI_LAST };
            tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
            tvis.item.pszText = (LPWSTR)(LPCWSTR)Entries[i].Name;
            tvis.item.iImage = tvis.item.iSelectedImage = GetSysIconIndex(FirstPath);
            tvis.item.cChildren = Entries[i].bFolder ? 1 : 0;
            tvis.item.lParam = (LPARAM)m_TreePaths.Add((Entries[i].bFolder ? L">" : L"") + Entries[i].Paths);
            TreeView_InsertItem(m_hwndTree, &tvis);
        }
    }

    VOID ShowAllPrograms(BOOL bShow)
    {
        if (bShow && !m_bAllPrograms)
        {
            TreeView_DeleteAllItems(m_hwndTree);
            m_TreePaths.SetCount(0);
            FillTree(TVI_ROOT, GetProgramsPaths());
        }
        m_bAllPrograms = bShow;
        ::ShowWindow(m_hwndTree, bShow ? SW_SHOWNA : SW_HIDE);
        InvalidateRect(NULL, FALSE);
    }

    VOID TreeActivate(HTREEITEM hItem)
    {
        TVITEMW tvi = { TVIF_PARAM | TVIF_STATE, hItem };
        if (!hItem || !TreeView_GetItem(m_hwndTree, &tvi))
            return;
        CStringW Path = m_TreePaths[tvi.lParam];
        if (Path[0] == L'>')
            TreeView_Expand(m_hwndTree, hItem, TVE_TOGGLE);
        else
            Launch(Path, NULL, TRUE);
    }

    /* ---- actions ---- */

    VOID Launch(LPCWSTR pszPath, LPCWSTR pszArgs, BOOL bRemember)
    {
        Hide();
        if (bRemember)
            AddRecent(pszPath);
        ShellExecuteW(NULL, NULL, pszPath, (pszArgs && *pszArgs) ? pszArgs : NULL, NULL, SW_SHOWNORMAL);
    }

    VOID Activate(const SM7_ITEM& it, BOOL bFromSearch)
    {
        switch (it.Action)
        {
            case ACT_RUN:
            {
                /* remember programs, not settings or typed commands */
                BOOL bProgram = it.Args.IsEmpty() && it.Value == 0 && HasExtension(it.Path, L".lnk");
                Launch(it.Path, it.Args, bProgram);
                break;
            }
            case ACT_FOLDER:
            {
                LPITEMIDLIST pidl = NULL;
                Hide();
                if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, it.Value, &pidl)))
                {
                    SHELLEXECUTEINFOW sei = { sizeof(sei), SEE_MASK_IDLIST };
                    sei.lpIDList = pidl;
                    sei.nShow = SW_SHOWNORMAL;
                    ShellExecuteExW(&sei);
                    ILFree(pidl);
                }
                break;
            }
            case ACT_TRAYCMD:
                Hide();
                ::PostMessageW(m_hwndTray, TWM_TRAYCOMMAND, it.Value, 0);
                break;
        }
    }

    static VOID ExitWindowsWithPrivilege(UINT uFlags)
    {
        HANDLE hToken;
        TOKEN_PRIVILEGES tp = { 1 };
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        {
            LookupPrivilegeValueW(NULL, SE_SHUTDOWN_NAME, &tp.Privileges[0].Luid);
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL);
            CloseHandle(hToken);
        }
        ExitWindowsEx(uFlags, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_FLAG_PLANNED);
    }

    VOID ShowPowerMenu()
    {
        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, IDM_SM7_LOGOFF, L"&Log off");
        AppendMenuW(hMenu, MF_STRING, IDM_SM7_LOCK, L"Loc&k");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, MF_STRING, IDM_SM7_RESTART, L"&Restart");
        AppendMenuW(hMenu, MF_STRING, IDM_SM7_OPTIONS, L"&More options...");

        POINT pt = { m_rcArrow.right, m_rcArrow.top };
        ClientToScreen(&pt);
        m_bInMenu = TRUE;
        UINT uCmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN,
                                   pt.x, pt.y, 0, m_hWnd, NULL);
        m_bInMenu = FALSE;
        DestroyMenu(hMenu);

        switch (uCmd)
        {
            case IDM_SM7_LOGOFF:
                Hide();
                ExitWindowsEx(EWX_LOGOFF, 0);
                break;
            case IDM_SM7_LOCK:
                Hide();
                LockWorkStation();
                break;
            case IDM_SM7_RESTART:
                Hide();
                ExitWindowsWithPrivilege(EWX_REBOOT);
                break;
            case IDM_SM7_OPTIONS:
                Hide();
                ::PostMessageW(m_hwndTray, TWM_TRAYCOMMAND, TRAYCMD_SHUTDOWN_DIALOG, 0);
                break;
        }
    }

    /* ---- layout and painting ---- */

    INT LeftItemHeight(const SM7_ITEM& it)
    {
        if (it.Action == ACT_SEPARATOR)
            return S(9);
        if (it.Action == ACT_HEADER)
            return S(26);
        return m_bSearching ? S(SM7_RESULT_H) : S(SM7_ITEM_H);
    }

    INT RightItemHeight(const SM7_ITEM& it)
    {
        return it.Action == ACT_SEPARATOR ? S(9) : S(SM7_RIGHT_H);
    }

    VOID Layout()
    {
        INT W = S(SM7_LEFT_W + SM7_RIGHT_W), H = S(SM7_HEIGHT), xSplit = S(SM7_LEFT_W);

        SetRect(&m_rcSearch, S(SM7_PAD) + S(4), H - S(SM7_PAD) - S(SM7_SEARCH_H),
                xSplit - S(SM7_PAD) - S(4), H - S(SM7_PAD));
        SetRect(&m_rcAllProg, S(SM7_PAD), m_rcSearch.top - S(8) - S(SM7_ALLPROG_H),
                xSplit - S(SM7_PAD), m_rcSearch.top - S(8));
        SetRect(&m_rcLeftList, S(SM7_PAD), S(SM7_PAD), xSplit - S(SM7_PAD), m_rcAllProg.top - S(6));

        ::MoveWindow(m_hwndSearch, m_rcSearch.left + S(10), m_rcSearch.top + (S(SM7_SEARCH_H) - S(18)) / 2,
                     m_rcSearch.right - m_rcSearch.left - S(36), S(18), FALSE);
        ::MoveWindow(m_hwndTree, m_rcLeftList.left, m_rcLeftList.top,
                     m_rcLeftList.right - m_rcLeftList.left, m_rcLeftList.bottom - m_rcLeftList.top, FALSE);

        /* right column: user tile, folder links, power buttons at the bottom */
        SetRect(&m_rcUser, xSplit + S(SM7_PAD), S(SM7_PAD), W - S(SM7_PAD), S(SM7_PAD) + S(64));
        m_cyRightTop = m_rcUser.bottom + S(10);
        INT cyBtn = S(30), xBtn = xSplit + S(SM7_PAD) + S(6);
        SetRect(&m_rcShutdown, xBtn, H - S(SM7_PAD) - cyBtn - S(2), xBtn + S(100), H - S(SM7_PAD) - S(2));
        SetRect(&m_rcArrow, m_rcShutdown.right, m_rcShutdown.top, m_rcShutdown.right + S(28), m_rcShutdown.bottom);
    }

    VOID FillRound(HDC hdc, const RECT *prc, COLORREF crFill, COLORREF crBorder, INT r)
    {
        HBRUSH hbr = CreateSolidBrush(crFill);
        HPEN hpen = CreatePen(PS_SOLID, 1, crBorder);
        HGDIOBJ ob = SelectObject(hdc, hbr), op = SelectObject(hdc, hpen);
        RoundRect(hdc, prc->left, prc->top, prc->right, prc->bottom, r, r);
        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        DeleteObject(hbr);
        DeleteObject(hpen);
    }

    VOID Text(HDC hdc, LPCWSTR psz, RECT rc, HFONT hFont, COLORREF cr, UINT uFormat)
    {
        SelectObject(hdc, hFont);
        SetTextColor(hdc, cr);
        DrawTextW(hdc, psz, -1, &rc, uFormat | DT_NOPREFIX | DT_SINGLELINE);
    }

    /* Windows 7 like hover: pale blue fill with a blue border */
    VOID Highlight(HDC hdc, const RECT *prc, BOOL bStrong)
    {
        FillRound(hdc, prc, bStrong ? UIC(RGB(204, 228, 247)) : UIC(RGB(229, 241, 251)),
                  bStrong ? UIC(RGB(153, 201, 239)) : UIC(RGB(204, 228, 247)), S(6));
    }

    /* Small filled triangle pointing right (bLeft: left), centered on (cx, cy) */
    VOID Arrow(HDC hdc, INT cx, INT cy, BOOL bLeft, COLORREF cr)
    {
        INT d = S(4);
        POINT pt[3];
        if (bLeft)
        {
            pt[0].x = cx + d / 2; pt[0].y = cy - d;
            pt[1].x = cx + d / 2; pt[1].y = cy + d;
            pt[2].x = cx - d / 2; pt[2].y = cy;
        }
        else
        {
            pt[0].x = cx - d / 2; pt[0].y = cy - d;
            pt[1].x = cx - d / 2; pt[1].y = cy + d;
            pt[2].x = cx + d / 2; pt[2].y = cy;
        }
        HBRUSH hbr = CreateSolidBrush(cr);
        HGDIOBJ ob = SelectObject(hdc, hbr), op = SelectObject(hdc, GetStockObject(NULL_PEN));
        Polygon(hdc, pt, 3);
        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        DeleteObject(hbr);
    }

    VOID Separator(HDC hdc, INT x1, INT x2, INT y)
    {
        RECT rc = { x1, y, x2, y + 1 };
        HBRUSH hbr = CreateSolidBrush(UIC(RGB(222, 226, 232)));
        FillRect(hdc, &rc, hbr);
        DeleteObject(hbr);
    }

    VOID PaintLeft(HDC hdc)
    {
        if (!m_bAllPrograms)
        {
            INT y = m_rcLeftList.top;
            for (SIZE_T i = 0; i < m_Left.GetCount(); i++)
            {
                const SM7_ITEM& it = m_Left[i];
                RECT rc = { m_rcLeftList.left, y, m_rcLeftList.right, y + LeftItemHeight(it) };
                y = rc.bottom;
                if (rc.bottom > m_rcLeftList.bottom)
                    break;

                if (it.Action == ACT_SEPARATOR)
                {
                    Separator(hdc, rc.left + S(8), rc.right - S(8), (rc.top + rc.bottom) / 2);
                    continue;
                }
                if (it.Action == ACT_HEADER)
                {
                    rc.left += S(8);
                    Text(hdc, it.Name, rc, m_hFontBold, UIC(RGB(30, 57, 91)), DT_VCENTER);
                    continue;
                }

                if ((INT)i == m_HotLeft || (INT)i == m_SelLeft)
                    Highlight(hdc, &rc, (INT)i == m_HotLeft && (INT)i == m_SelLeft);

                INT cxIcon = GetSystemMetrics(m_bSearching ? SM_CXSMICON : SM_CXICON);
                if (it.iIcon >= 0)
                    ImageList_Draw(m_bSearching ? m_himlSmall : m_himlLarge, it.iIcon, hdc, rc.left + S(6),
                                   rc.top + (rc.bottom - rc.top - cxIcon) / 2, ILD_TRANSPARENT);
                rc.left += S(6) + cxIcon + S(10);
                rc.right -= S(6);
                if (it.Value == SM7_FILE)
                {
                    /* the folder it is in, dimmed on the right */
                    CStringW Folder = it.Path.Left(it.Path.ReverseFind(L'\\'));
                    Folder = Folder.Mid(Folder.ReverseFind(L'\\') + 1);
                    RECT rcFolder = rc;
                    rcFolder.left = rc.left + (rc.right - rc.left) * 3 / 5;
                    Text(hdc, Folder, rcFolder, m_hFont, UIC(RGB(120, 120, 120)), DT_VCENTER | DT_RIGHT | DT_END_ELLIPSIS);
                    rc.right = rcFolder.left - S(6);
                }
                Text(hdc, it.Name, rc, m_hFont, UIC(RGB(26, 26, 26)), DT_VCENTER | DT_END_ELLIPSIS);
            }
        }

        /* "All Programs" / "Back" */
        Separator(hdc, m_rcAllProg.left + S(8), m_rcAllProg.right - S(8), m_rcAllProg.top - S(3));
        if (!m_bSearching)
        {
            if (m_HotAllProg)
                Highlight(hdc, &m_rcAllProg, FALSE);
            RECT rc = m_rcAllProg;
            INT cy = (rc.top + rc.bottom) / 2;
            if (m_bAllPrograms)
            {
                Arrow(hdc, rc.left + S(16), cy, TRUE, UIC(RGB(26, 26, 26)));
                rc.left += S(28);
                Text(hdc, L"Back", rc, m_hFontBold, UIC(RGB(26, 26, 26)), DT_VCENTER);
            }
            else
            {
                rc.left += S(12);
                Text(hdc, L"All Programs", rc, m_hFontBold, UIC(RGB(26, 26, 26)), DT_VCENTER);
                SIZE size;
                SelectObject(hdc, m_hFontBold);
                GetTextExtentPoint32W(hdc, L"All Programs", 12, &size);
                Arrow(hdc, rc.left + size.cx + S(12), cy, FALSE, UIC(RGB(26, 26, 26)));
            }
        }

        /* Search box: rounded field, cue text drawn by the edit subclass */
        FillRound(hdc, &m_rcSearch, UIC(RGB(255, 255, 255)), UIC(RGB(171, 173, 179)), S(8));
        RECT rcLine = { m_rcSearch.left + S(6), m_rcSearch.bottom - S(2), m_rcSearch.right - S(6), m_rcSearch.bottom - S(1) };
        HBRUSH hbrAccent = CreateSolidBrush(::GetFocus() == m_hwndSearch ? GetSysColor(COLOR_HOTLIGHT) : UIC(RGB(171, 173, 179)));
        FillRect(hdc, &rcLine, hbrAccent);
        DeleteObject(hbrAccent);

        /* magnifier glyph */
        LOGBRUSH lb = { BS_SOLID, UIC(RGB(96, 96, 96)), 0 };
        HPEN hpen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND, max(1, S(2) - 1) + 1, &lb, 0, NULL);
        HGDIOBJ op = SelectObject(hdc, hpen), ob = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        INT cx = m_rcSearch.right - S(18), cy = (m_rcSearch.top + m_rcSearch.bottom) / 2 - S(1);
        Ellipse(hdc, cx - S(5), cy - S(5), cx + S(4), cy + S(4));
        MoveToEx(hdc, cx + S(3), cy + S(3), NULL);
        LineTo(hdc, cx + S(7), cy + S(7));
        SelectObject(hdc, op);
        SelectObject(hdc, ob);
        DeleteObject(hpen);
    }

    VOID PaintRight(HDC hdc)
    {
        /* user: picture (initial in an accent circle) and name */
        RECT rcPic = { m_rcUser.left + S(6), m_rcUser.top + S(8), m_rcUser.left + S(6) + S(48), m_rcUser.top + S(56) };
        HBRUSH hbr = CreateSolidBrush(GetSysColor(COLOR_HOTLIGHT));
        HGDIOBJ ob = SelectObject(hdc, hbr), op = SelectObject(hdc, GetStockObject(NULL_PEN));
        Ellipse(hdc, rcPic.left, rcPic.top, rcPic.right, rcPic.bottom);
        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        DeleteObject(hbr);
        WCHAR szInitial[2] = { (WCHAR)(ULONG_PTR)CharUpperW((LPWSTR)(ULONG_PTR)(m_szUser[0] ? m_szUser[0] : L'?')), 0 };
        Text(hdc, szInitial, rcPic, m_hFontBig, RGB(255, 255, 255), DT_CENTER | DT_VCENTER);
        RECT rcName = { rcPic.right + S(10), m_rcUser.top, m_rcUser.right, m_rcUser.bottom };
        if (m_HotRight == -2)
            Highlight(hdc, &m_rcUser, FALSE);
        Text(hdc, m_szUser, rcName, m_hFontBig, UIC(RGB(26, 26, 26)), DT_VCENTER | DT_END_ELLIPSIS);

        INT y = m_cyRightTop;
        for (SIZE_T i = 0; i < m_Right.GetCount(); i++)
        {
            const SM7_ITEM& it = m_Right[i];
            RECT rc = { m_rcUser.left, y, m_rcUser.right, y + RightItemHeight(it) };
            y = rc.bottom;
            if (it.Action == ACT_SEPARATOR)
            {
                Separator(hdc, rc.left + S(8), rc.right - S(8), (rc.top + rc.bottom) / 2);
                continue;
            }
            if ((INT)i == m_HotRight)
                Highlight(hdc, &rc, FALSE);
            rc.left += S(12);
            Text(hdc, it.Name, rc, m_hFont, UIC(RGB(26, 26, 26)), DT_VCENTER | DT_END_ELLIPSIS);
        }

        /* Shut down [>] */
        FillRound(hdc, &m_rcShutdown, m_HotShutdown ? UIC(RGB(229, 241, 251)) : UIC(RGB(253, 253, 253)),
                  m_HotShutdown ? UIC(RGB(153, 201, 239)) : UIC(RGB(190, 194, 200)), S(6));
        Text(hdc, L"Shut down", m_rcShutdown, m_hFont, UIC(RGB(26, 26, 26)), DT_CENTER | DT_VCENTER);
        RECT rcArrow = m_rcArrow;
        rcArrow.left -= 1;
        FillRound(hdc, &rcArrow, m_HotArrow ? UIC(RGB(229, 241, 251)) : UIC(RGB(253, 253, 253)),
                  m_HotArrow ? UIC(RGB(153, 201, 239)) : UIC(RGB(190, 194, 200)), S(6));
        Arrow(hdc, (rcArrow.left + rcArrow.right) / 2, (rcArrow.top + rcArrow.bottom) / 2, FALSE, UIC(RGB(26, 26, 26)));
    }

    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        PAINTSTRUCT ps;
        HDC hdcWnd = BeginPaint(&ps);
        RECT rcClient;
        GetClientRect(&rcClient);

        /* double buffered: no flicker while hovering */
        HDC hdc = CreateCompatibleDC(hdcWnd);
        HBITMAP hbm = CreateCompatibleBitmap(hdcWnd, rcClient.right, rcClient.bottom);
        HGDIOBJ obm = SelectObject(hdc, hbm);
        SetBkMode(hdc, TRANSPARENT);

        HBRUSH hbr = CreateSolidBrush(UIC(RGB(255, 255, 255)));
        FillRect(hdc, &rcClient, hbr);
        DeleteObject(hbr);
        RECT rcRight = { S(SM7_LEFT_W), 0, rcClient.right, rcClient.bottom };
        hbr = CreateSolidBrush(UIC(RGB(238, 242, 248)));
        FillRect(hdc, &rcRight, hbr);
        DeleteObject(hbr);
        RECT rcSplit = { rcRight.left, 0, rcRight.left + 1, rcClient.bottom };
        hbr = CreateSolidBrush(UIC(RGB(218, 224, 232)));
        FillRect(hdc, &rcSplit, hbr);
        DeleteObject(hbr);

        PaintLeft(hdc);
        PaintRight(hdc);

        /* border */
        HPEN hpen = CreatePen(PS_SOLID, 1, UIC(RGB(170, 178, 190)));
        HGDIOBJ op = SelectObject(hdc, hpen), ob = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        RoundRect(hdc, 0, 0, rcClient.right, rcClient.bottom, S(12), S(12));
        SelectObject(hdc, op);
        SelectObject(hdc, ob);
        DeleteObject(hpen);

        BitBlt(hdcWnd, 0, 0, rcClient.right, rcClient.bottom, hdc, 0, 0, SRCCOPY);
        SelectObject(hdc, obm);
        DeleteObject(hbm);
        DeleteDC(hdc);
        EndPaint(&ps);
        return 0;
    }

    /* ---- hit testing ---- */

    INT HitLeft(POINT pt)
    {
        if (m_bAllPrograms || !PtInRect(&m_rcLeftList, pt))
            return -1;
        INT y = m_rcLeftList.top;
        for (SIZE_T i = 0; i < m_Left.GetCount(); i++)
        {
            INT cy = LeftItemHeight(m_Left[i]);
            if (pt.y >= y && pt.y < y + cy)
                return m_Left[i].Action == ACT_RUN || m_Left[i].Action == ACT_FOLDER ? (INT)i : -1;
            y += cy;
        }
        return -1;
    }

    INT HitRight(POINT pt)
    {
        if (PtInRect(&m_rcUser, pt))
            return -2;
        if (pt.x < m_rcUser.left || pt.x >= m_rcUser.right)
            return -1;
        INT y = m_cyRightTop;
        for (SIZE_T i = 0; i < m_Right.GetCount(); i++)
        {
            INT cy = RightItemHeight(m_Right[i]);
            if (pt.y >= y && pt.y < y + cy)
                return m_Right[i].Action == ACT_SEPARATOR ? -1 : (INT)i;
            y += cy;
        }
        return -1;
    }

    LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        INT HotLeft = HitLeft(pt), HotRight = HitRight(pt);
        BOOL HotShutdown = PtInRect(&m_rcShutdown, pt), HotArrow = PtInRect(&m_rcArrow, pt);
        BOOL HotAllProg = !m_bSearching && PtInRect(&m_rcAllProg, pt);

        if (HotLeft != m_HotLeft || HotRight != m_HotRight || HotShutdown != m_HotShutdown ||
            HotArrow != m_HotArrow || HotAllProg != m_HotAllProg)
        {
            m_HotLeft = HotLeft;
            m_HotRight = HotRight;
            m_HotShutdown = HotShutdown;
            m_HotArrow = HotArrow;
            m_HotAllProg = HotAllProg;
            if (HotLeft >= 0)
                m_SelLeft = HotLeft;
            InvalidateRect(NULL, FALSE);
        }

        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, m_hWnd };
        TrackMouseEvent(&tme);
        return 0;
    }

    LRESULT OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        m_HotLeft = m_HotRight = -1;
        m_HotShutdown = m_HotArrow = m_HotAllProg = FALSE;
        InvalidateRect(NULL, FALSE);
        return 0;
    }

    LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        INT i;

        if ((i = HitLeft(pt)) >= 0)
        {
            Activate(m_Left[i], m_bSearching);
        }
        else if ((i = HitRight(pt)) >= 0)
        {
            Activate(m_Right[i], FALSE);
        }
        else if (i == -2)
        {
            SM7_ITEM it;
            it.Action = ACT_FOLDER;
            it.Value = CSIDL_PROFILE;
            Activate(it, FALSE);
        }
        else if (!m_bSearching && PtInRect(&m_rcAllProg, pt))
        {
            ShowAllPrograms(!m_bAllPrograms);
            ::SetFocus(m_bAllPrograms ? m_hwndTree : m_hwndSearch);
        }
        else if (PtInRect(&m_rcShutdown, pt))
        {
            Hide();
            ExitWindowsWithPrivilege(EWX_SHUTDOWN | EWX_POWEROFF);
        }
        else if (PtInRect(&m_rcArrow, pt))
        {
            ShowPowerMenu();
        }
        else if (PtInRect(&m_rcSearch, pt))
        {
            ::SetFocus(m_hwndSearch);
        }
        return 0;
    }

    /* ---- keyboard (the search box has the focus) ---- */

    VOID MoveSelection(INT Delta)
    {
        INT n = (INT)m_Left.GetCount(), i = m_SelLeft;
        for (INT tries = 0; tries < n; tries++)
        {
            i = (i < 0) ? (Delta > 0 ? 0 : n - 1) : (i + Delta + n) % n;
            if (m_Left[i].Action == ACT_RUN || m_Left[i].Action == ACT_FOLDER)
            {
                m_SelLeft = i;
                InvalidateRect(NULL, FALSE);
                return;
            }
        }
    }

    BOOL OnSearchKey(WPARAM vk)
    {
        switch (vk)
        {
            case VK_ESCAPE:
                if (::GetWindowTextLengthW(m_hwndSearch) > 0)
                    ::SetWindowTextW(m_hwndSearch, L"");
                else if (m_bAllPrograms)
                    ShowAllPrograms(FALSE);
                else
                    Hide();
                return TRUE;
            case VK_RETURN:
                if (m_SelLeft >= 0 && m_SelLeft < (INT)m_Left.GetCount())
                    Activate(m_Left[m_SelLeft], m_bSearching);
                return TRUE;
            case VK_DOWN:
            case VK_UP:
                if (!m_bAllPrograms)
                    MoveSelection(vk == VK_DOWN ? 1 : -1);
                return TRUE;
        }
        return FALSE;
    }

    static LRESULT CALLBACK SearchProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                       UINT_PTR uId, DWORD_PTR dwRef)
    {
        CStartMenu7 *This = (CStartMenu7 *)dwRef;
        switch (uMsg)
        {
            case WM_KEYDOWN:
                if (This->OnSearchKey(wParam))
                    return 0;
                break;
            case WM_CHAR:
                if (wParam == VK_RETURN || wParam == VK_ESCAPE)
                    return 0;
                break;
            case WM_GETDLGCODE:
                return DLGC_WANTALLKEYS;
            case WM_SETFOCUS:
            case WM_KILLFOCUS:
                This->InvalidateRect(&This->m_rcSearch, FALSE);
                break;
            case WM_PAINT:
            {
                /* cue text: "Search programs and settings" */
                LRESULT lRet = DefSubclassProc(hwnd, uMsg, wParam, lParam);
                if (::GetWindowTextLengthW(hwnd) == 0)
                {
                    HDC hdc = ::GetDC(hwnd);
                    RECT rc;
                    ::GetClientRect(hwnd, &rc);
                    SetBkMode(hdc, TRANSPARENT);
                    SelectObject(hdc, This->m_hFont);
                    SetTextColor(hdc, UIC(RGB(118, 118, 118)));
                    rc.left += 2;
                    DrawTextW(hdc, L"Search programs and settings", -1, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
                    ::ReleaseDC(hwnd, hdc);
                }
                return lRet;
            }
        }
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);
    }

    /* ---- messages ---- */

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_TREEVIEW_CLASSES };
        InitCommonControlsEx(&icc);

        SHFILEINFOW sfi;
        m_himlSmall = (HIMAGELIST)SHGetFileInfoW(L"", 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
        m_himlLarge = (HIMAGELIST)SHGetFileInfoW(L"", 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_LARGEICON);

        m_hwndSearch = CreateWindowExW(0, WC_EDITW, NULL, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                       0, 0, 0, 0, m_hWnd, (HMENU)IDC_SM7_SEARCH, hExplorerInstance, NULL);
        SetWindowSubclass(m_hwndSearch, SearchProc, 1, (DWORD_PTR)this);

        m_hwndTree = CreateWindowExW(0, WC_TREEVIEWW, NULL,
                                     WS_CHILD | WS_VSCROLL | TVS_FULLROWSELECT | TVS_TRACKSELECT | TVS_NOHSCROLL |
                                     TVS_SHOWSELALWAYS,
                                     0, 0, 0, 0, m_hWnd, (HMENU)IDC_SM7_TREE, hExplorerInstance, NULL);
        TreeView_SetImageList(m_hwndTree, m_himlSmall, TVSIL_NORMAL);
        TreeView_SetIndent(m_hwndTree, S(16));

        CreateFonts();
        DWORD cch = _countof(m_szUser);
        if (!GetUserNameW(m_szUser, &cch))
            m_szUser[0] = UNICODE_NULL;
        BuildRight();
        return 0;
    }

    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        RemoveWindowSubclass(m_hwndSearch, SearchProc, 1);
        DeleteObject(m_hFont);
        DeleteObject(m_hFontBold);
        DeleteObject(m_hFontBig);
        return 0;
    }

    LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return 1;
    }

    LRESULT OnCtlColorEdit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        static HBRUSH s_hbr = NULL;
        static COLORREF s_cr = CLR_INVALID;
        COLORREF crBack = UIC(RGB(255, 255, 255));
        if (crBack != s_cr)
        {
            if (s_hbr)
                DeleteObject(s_hbr);
            s_hbr = CreateSolidBrush(crBack);
            s_cr = crBack;
        }
        SetTextColor((HDC)wParam, UIC(RGB(26, 26, 26)));
        SetBkColor((HDC)wParam, crBack);
        return (LRESULT)s_hbr;
    }

    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (LOWORD(wParam) == IDC_SM7_SEARCH && HIWORD(wParam) == EN_CHANGE)
            DoSearch();
        return 0;
    }

    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LPNMHDR pnm = (LPNMHDR)lParam;
        if (pnm->hwndFrom != m_hwndTree)
            return 0;

        switch (pnm->code)
        {
            case TVN_ITEMEXPANDINGW:
            {
                LPNMTREEVIEWW pnmtv = (LPNMTREEVIEWW)lParam;
                if (pnmtv->action == TVE_EXPAND && !TreeView_GetChild(m_hwndTree, pnmtv->itemNew.hItem))
                    FillTree(pnmtv->itemNew.hItem, m_TreePaths[pnmtv->itemNew.lParam].Mid(1));
                break;
            }
            case NM_CLICK:
            {
                TVHITTESTINFO hti = { 0 };
                GetCursorPos(&hti.pt);
                ::ScreenToClient(m_hwndTree, &hti.pt);
                TreeView_HitTest(m_hwndTree, &hti);
                if (hti.hItem && (hti.flags & (TVHT_ONITEM | TVHT_ONITEMRIGHT | TVHT_ONITEMINDENT)))
                {
                    TreeView_SelectItem(m_hwndTree, hti.hItem);
                    TreeActivate(hti.hItem);
                }
                return TRUE;
            }
            case TVN_KEYDOWN:
            {
                LPNMTVKEYDOWN pnmk = (LPNMTVKEYDOWN)lParam;
                if (pnmk->wVKey == VK_RETURN)
                    TreeActivate(TreeView_GetSelection(m_hwndTree));
                else if (pnmk->wVKey == VK_ESCAPE || pnmk->wVKey == VK_BACK)
                {
                    ShowAllPrograms(FALSE);
                    ::SetFocus(m_hwndSearch);
                }
                break;
            }
        }
        return 0;
    }

    LRESULT OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        /* clicking anywhere else closes the menu */
        if (LOWORD(wParam) == WA_INACTIVE && IsWindowVisible() && !m_bInMenu)
            Hide();
        return 0;
    }

    BEGIN_MSG_MAP(CStartMenu7)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnCtlColorEdit)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
    END_MSG_MAP()

    /* ---- show / hide ---- */

    VOID Show(HWND hwndTray, HWND hwndStartButton)
    {
        m_hwndTray = hwndTray;
        m_hwndStartButton = hwndStartButton;
        CreateFonts();

        ::SetWindowTextW(m_hwndSearch, L"");
        m_bSearching = FALSE;
        m_bAllPrograms = FALSE;
        ::ShowWindow(m_hwndTree, SW_HIDE);
        BuildHome();
        BuildSearchIndex();
        m_bFilesIndexed = FALSE;
        m_HotLeft = m_HotRight = -1;
        m_HotShutdown = m_HotArrow = m_HotAllProg = FALSE;
        Layout();

        /* Above the Start button, inside the work area of its monitor */
        RECT rcStart, rcWork;
        ::GetWindowRect(hwndStartButton, &rcStart);
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfoW(MonitorFromRect(&rcStart, MONITOR_DEFAULTTONEAREST), &mi);
        rcWork = mi.rcWork;
        INT W = S(SM7_LEFT_W + SM7_RIGHT_W), H = min(S(SM7_HEIGHT), (INT)(rcWork.bottom - rcWork.top) - S(16));
        INT x = max(rcWork.left + S(8), min((INT)rcStart.left, (INT)rcWork.right - W - S(8)));
        INT y = rcWork.bottom - H - S(8);
        if (rcStart.top < rcWork.top)   /* taskbar at the top */
            y = rcWork.top + S(8);

        SetWindowPos(HWND_TOPMOST, x, y, W, H, SWP_SHOWWINDOW);
        SetWindowRgn(CreateRoundRectRgn(0, 0, W + 1, H + 1, S(12), S(12)), TRUE);
        ::SetForegroundWindow(m_hWnd);
        ::SetFocus(m_hwndSearch);
        ::SendMessageW(m_hwndStartButton, BM_SETSTATE, TRUE, 0);
        InvalidateRect(NULL, FALSE);
    }

    VOID Hide()
    {
        if (!IsWindowVisible())
            return;
        ShowWindow(SW_HIDE);
        m_dwHiddenTick = GetTickCount();
        ::SendMessageW(m_hwndStartButton, BM_SETSTATE, FALSE, 0);
    }
};

static CStartMenu7 *g_pStartMenu7 = NULL;

BOOL StartMenu7_IsEnabled()
{
    DWORD dwValue = 0, cb = sizeof(dwValue);
    SHGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
                L"ReactOSClassicStartMenu", NULL, &dwValue, &cb);
    return dwValue == 0;
}

/* Start button or the Windows key: open, or close if open */
VOID StartMenu7_Toggle(HWND hwndTray, HWND hwndStartButton)
{
    if (!g_pStartMenu7)
    {
        g_pStartMenu7 = new CStartMenu7();
        if (!g_pStartMenu7->Create(hwndTray, NULL, NULL))
        {
            delete g_pStartMenu7;
            g_pStartMenu7 = NULL;
            return;
        }
    }

    /* The click that deactivated (= closed) the menu must not reopen it */
    if (g_pStartMenu7->IsWindowVisible() || GetTickCount() - g_pStartMenu7->m_dwHiddenTick < 300)
    {
        g_pStartMenu7->Hide();
        return;
    }
    g_pStartMenu7->Show(hwndTray, hwndStartButton);
}

VOID StartMenu7_Hide()
{
    if (g_pStartMenu7)
        g_pStartMenu7->Hide();
}
