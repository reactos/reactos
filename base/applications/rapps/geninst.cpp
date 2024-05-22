/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Act as installer and uninstaller for applications distributed in archives
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#include "defines.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <setupapi.h>
#include <commctrl.h>
#include "resource.h"
#include "appdb.h"
#include "appinfo.h"
#include "misc.h"
#include "configparser.h"
#include "unattended.h"

#include "minizip/ioapi.h"
#include "minizip/iowin32.h"
#include "minizip/unzip.h"
extern "C" {
    #include "minizip/ioapi.c"
    #include "minizip/iowin32.c"
    #include "minizip/unzip.c"
}

#define REGPATH_UNINSTALL L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"

#define DB_GENINST_FILES L"Files"
#define DB_GENINST_DIR L"Dir"
#define DB_GENINST_ICON L"Icon"
#define DB_GENINST_LNK L"Lnk"
#define DB_GENINST_DELFILE L"DelFile" // Delete files generated by the application
#define DB_GENINST_DELDIR L"DelDir"
#define DB_GENINST_DELDIREMPTY L"DelDirEmpty"
#define DB_GENINST_DELREG L"DelReg"
#define DB_GENINST_DELREGEMPTY L"DelRegEmpty"

enum {
    UNOP_FILE = 'F',
    UNOP_DIR = 'D',
    UNOP_EMPTYDIR = 'd',
    UNOP_REGKEY = 'K',
    UNOP_EMPTYREGKEY = 'k',
};

static int
ExtractFilesFromZip(LPCWSTR Archive, const CStringW &OutputDir,
                    EXTRACTCALLBACK Callback, void *Cookie)
{
    const UINT pkzefsutf8 = 1 << 11; // APPNOTE; APPENDIX D
    zlib_filefunc64_def zff;
    fill_win32_filefunc64W(&zff);
    unzFile hzf = unzOpen2_64(Archive, &zff);
    if (!hzf)
        return UNZ_BADZIPFILE;
    CStringA narrow;
    CStringW path, dir;
    int zerr = unzGoToFirstFile(hzf);
    for (; zerr == UNZ_OK; zerr = unzGoToNextFile(hzf))
    {
        unz_file_info64 fi;
        zerr = unzGetCurrentFileInfo64(hzf, &fi, NULL, 0, NULL, 0, NULL, 0);
        if (zerr != UNZ_OK)
            break;
        LPSTR file = narrow.GetBuffer(fi.size_filename);
        zerr = unzGetCurrentFileInfo64(hzf, &fi, file, narrow.GetAllocLength(), NULL, 0, NULL, 0);
        if (zerr != UNZ_OK)
            break;
        narrow.ReleaseBuffer(fi.size_filename);
        narrow.Replace('/', '\\');
        while (narrow[0] == '\\')
            narrow = narrow.Mid(1);
        UINT codepage = (fi.flag & pkzefsutf8) ? CP_UTF8 : 1252;
        UINT cch = MultiByteToWideChar(codepage, 0, narrow, -1, NULL, 0);
        cch = MultiByteToWideChar(codepage, 0, narrow, -1, path.GetBuffer(cch - 1), cch);
        if (!cch)
            break;
        path.ReleaseBuffer(cch - 1);
        DWORD fileatt = FILE_ATTRIBUTE_NORMAL, err;
        BYTE attsys = HIBYTE(fi.version), dos = 0, ntfs = 10, vfat = 14;
        if ((attsys == dos || attsys == ntfs || attsys == vfat) && LOBYTE(fi.external_fa))
            fileatt = LOBYTE(fi.external_fa);

        if (!NotifyFileExtractCallback(path, fi.uncompressed_size, fileatt,
                                       Callback, Cookie))
            continue; // Skip file

        path = BuildPath(OutputDir, path);
        SplitFileAndDirectory(path, &dir);
        if (!dir.IsEmpty() && (err = CreateDirectoryTree(dir)))
        {
            zerr = UNZ_ERRNO;
            SetLastError(err);
            break;
        }

        if ((zerr = unzOpenCurrentFile(hzf)) != UNZ_OK)
            break;
        zerr = UNZ_ERRNO;
        HANDLE hFile = CreateFileW(path, GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_DELETE,
                                   NULL, CREATE_ALWAYS, fileatt, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD len = 1024 * 4, cb;
            LPSTR buf = narrow.GetBuffer(len);
            for (zerr = UNZ_OK; zerr == UNZ_OK;)
            {
                len = zerr = unzReadCurrentFile(hzf, buf, len);
                if (zerr <= 0)
                    break;
                zerr = WriteFile(hFile, buf, len, &cb, NULL) && cb == len ? UNZ_OK : UNZ_ERRNO;
            }
            CloseHandle(hFile);
        }
        unzCloseCurrentFile(hzf);
    }
    unzClose(hzf);
    return zerr == UNZ_END_OF_LIST_OF_FILE ? UNZ_OK : zerr;
}

static UINT
ExtractZip(LPCWSTR Archive, const CStringW &OutputDir,
           EXTRACTCALLBACK Callback, void *Cookie)
{
    int zerr = ExtractFilesFromZip(Archive, OutputDir, Callback, Cookie);
    return zerr == UNZ_ERRNO ? GetLastError() : zerr ? ERROR_INTERNAL_ERROR : 0;
}

static UINT
ExtractCab(LPCWSTR Archive, const CStringW &OutputDir,
           EXTRACTCALLBACK Callback, void *Cookie)
{
    if (ExtractFilesFromCab(Archive, OutputDir, Callback, Cookie))
        return ERROR_SUCCESS;
    UINT err = GetLastError();
    return err ? err : ERROR_INTERNAL_ERROR;
}

enum { IM_STARTPROGRESS = WM_APP, IM_PROGRESS, IM_END };

static struct CommonInfo
{
    LPCWSTR AppName;
    HWND hDlg;
    DWORD Error, Count;
    BOOL Silent;
    CRegKey *ArpKey, Entries;

    CommonInfo(LPCWSTR DisplayName, BOOL IsSilent = FALSE)
    : AppName(DisplayName), hDlg(NULL), Error(0), Count(0), Silent(IsSilent), ArpKey(NULL)
    {
    }
    inline HWND GetGuiOwner() const { return Silent ? NULL : hDlg; }
} *g_pInfo;

struct InstallInfo : CommonInfo
{
    CConfigParser &Parser;
    LPCWSTR ArchivePath, InstallDir, ShortcutFile;
    CStringW UninstFile, MainApp;
    UINT InstallDirLen, EntryCount;
    BOOL PerUser;

    InstallInfo(LPCWSTR AppName, CConfigParser &CP, LPCWSTR Archive)
    : CommonInfo(AppName), Parser(CP), ArchivePath(Archive)
    {
        EntryCount = 0;
    }
};

static UINT
ErrorBox(UINT Error = GetLastError())
{
    if (!Error)
        Error = ERROR_INTERNAL_ERROR;
    WCHAR buf[400];
    UINT fmf = FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM;
    FormatMessageW(fmf, NULL, Error, 0, buf, _countof(buf), NULL);
    MessageBoxW(g_pInfo->GetGuiOwner(), buf, 0, MB_OK | MB_ICONSTOP);
    g_pInfo->Error = Error;
    return Error;
}

static LPCWSTR
GetCommonString(LPCWSTR Name, CStringW &Output, LPCWSTR Default = L"")
{
    InstallInfo &Info = *static_cast<InstallInfo *>(g_pInfo);
    BOOL found = Info.Parser.GetString(Name, Output);
    return (found && !Output.IsEmpty() ? Output : Output = Default).GetString();
}

static LPCWSTR
GetGenerateString(LPCWSTR Name, CStringW &Output, LPCWSTR Default = L"")
{
    InstallInfo &Info = *static_cast<InstallInfo *>(g_pInfo);
    UINT r = Info.Parser.GetSectionString(DB_GENINSTSECTION, Name, Output);
    return (r ? Output : Output = Default).GetString();
}

static void
WriteArpEntry(LPCWSTR Name, LPCWSTR Value, UINT Type = REG_SZ)
{
    // Write a "Add/Remove programs" value if we have a valid uninstaller key
    if (g_pInfo->ArpKey)
    {
        UINT err = g_pInfo->ArpKey->SetStringValue(Name, Value, Type);
        if (err)
            ErrorBox(err);
    }
}

static BOOL
AddEntry(WCHAR Type, LPCWSTR Value)
{
    InstallInfo &Info = *static_cast<InstallInfo *>(g_pInfo);
    CStringW name;
    name.Format(L"%c%u", Type, ++Info.EntryCount);
    UINT err = Info.Entries.SetStringValue(name, Value);
    if (err)
        ErrorBox(err);
    return !err;
}

static HRESULT
GetCustomIconPath(InstallInfo &Info, CStringW &Path)
{
    if (*GetGenerateString(DB_GENINST_ICON, Path))
    {
        Path = BuildPath(Info.InstallDir, Path);
        int idx = PathParseIconLocation(Path.GetBuffer());
        Path.ReleaseBuffer();
        HICON hIco = NULL;
        if (ExtractIconExW(Path, idx, &hIco, NULL, 1))
            DestroyIcon(hIco);
        if (idx)
            Path.AppendFormat(L",%d", idx);
        return hIco ? S_OK : S_FALSE;
    }
    return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
}

static BOOL
GetLocalizedSMFolderName(LPCWSTR WinVal, LPCWSTR RosInf, LPCWSTR RosVal, CStringW &Output)
{
    CRegKey key;
    if (key.Open(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion", KEY_READ) == ERROR_SUCCESS &&
        GetRegString(key, WinVal, Output) && !Output.IsEmpty())
    {
        return TRUE;
    }
    WCHAR windir[MAX_PATH];
    GetWindowsDirectoryW(windir, _countof(windir));
    CStringW path = BuildPath(BuildPath(windir, L"inf"), RosInf), section;
    DWORD lang = 0, lctype = LOCALE_ILANGUAGE | LOCALE_RETURN_NUMBER;
    if (GetLocaleInfoW(GetUserDefaultLCID(), lctype, (LPWSTR) &lang, sizeof(lang) / sizeof(WCHAR)))
    {
        section.Format(L"Strings.%.4x", lang);
        if (ReadIniValue(path, section, RosVal, Output) > 0)
            return TRUE;
        section.Format(L"Strings.%.2x", PRIMARYLANGID(lang));
        if (ReadIniValue(path, section, RosVal, Output) > 0)
            return TRUE;
    }
    return ReadIniValue(path, L"Strings", RosVal, Output) > 0;
}

static BOOL
CreateShortcut(const CStringW &Target)
{
    InstallInfo &Info = *static_cast<InstallInfo *>(g_pInfo);
    UINT csidl = Info.PerUser ? CSIDL_PROGRAMS : CSIDL_COMMON_PROGRAMS;
    CStringW rel = Info.ShortcutFile, path, dir, tmp;

    if (FAILED(GetSpecialPath(csidl, path, Info.GetGuiOwner())))
        return TRUE; // Pretend everything is OK

    int cat;
    if (Info.Parser.GetInt(DB_CATEGORY, cat) && cat == ENUM_CAT_GAMES)
    {
        // Try to find the name of the Games folder in the Start Menu
        if (GetLocalizedSMFolderName(L"SM_GamesName", L"shortcuts.inf", L"Games", tmp))
        {
            path = BuildPath(path, tmp);
        }
    }

    // SHPathPrepareForWrite will prepare the necessary directories.
    // Windows and ReactOS SHPathPrepareForWrite do not support '/'.
    rel.Replace('/', '\\');
    path = BuildPath(path, rel.GetString());
    UINT SHPPFW = SHPPFW_DIRCREATE | SHPPFW_IGNOREFILENAME;
    HRESULT hr = SHPathPrepareForWriteW(Info.GetGuiOwner(), NULL, path, SHPPFW);
    if ((Info.Error = ErrorFromHResult(hr)) != 0)
    {
        ErrorBox(Info.Error);
        return FALSE;
    }

    CComPtr<IShellLinkW> link;
    hr = link.CoCreateInstance(CLSID_ShellLink, IID_IShellLinkW);
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr = link->SetPath(Target)))
        {
            if (SUCCEEDED(GetCustomIconPath(Info, tmp)))
            {
                LPWSTR p = tmp.GetBuffer();
                int idx = PathParseIconLocation(p);
                link->SetIconLocation(p, idx);
            }
            CComPtr<IPersistFile> persist;
            if (SUCCEEDED(hr = link->QueryInterface(IID_IPersistFile, (void**)&persist)))
            {
                hr = persist->Save(path, FALSE);
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        if (AddEntry(UNOP_FILE, path))
        {
            SplitFileAndDirectory(path, &dir);
            AddEntry(UNOP_EMPTYDIR, dir);
        }
    }
    else
    {
        ErrorBox(ErrorFromHResult(hr));
    }
    return !Info.Error;
}

static BOOL
InstallFiles(const CStringW &SourceDirBase, const CStringW &Spec,
             const CStringW &DestinationDir)
{
    InstallInfo &Info = *static_cast<InstallInfo *>(g_pInfo);
    CStringW sourcedir, filespec;
    filespec = SplitFileAndDirectory(Spec, &sourcedir); // Split "[OptionalDir\]*.ext"
    sourcedir = BuildPath(SourceDirBase, sourcedir);
    BOOL success = TRUE;
    WIN32_FIND_DATAW wfd;
    HANDLE hFind = FindFirstFileW(BuildPath(sourcedir, filespec), &wfd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND)
            return TRUE;
        else
            return !ErrorBox(error);
    }

    for (;;)
    {
        CStringW from = BuildPath(sourcedir, wfd.cFileName);
        CStringW to = BuildPath(DestinationDir, wfd.cFileName);
        CStringW uninstpath = to.Mid(Info.InstallDirLen - 1);
        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            LPWSTR p = wfd.cFileName;
            BOOL dots = p[0] == '.' && (!p[1] || (p[1] == '.' && !p[2]));
            if (!dots)
            {
                Info.Error = CreateDirectoryTree(to);
                if (Info.Error)
                {
                    success = !ErrorBox(Info.Error);
                }
                else
                {
                    success = AddEntry(UNOP_EMPTYDIR, uninstpath);
                }

                if (success)
                {
                    success = InstallFiles(from, filespec, to);
                }
            }
        }
        else
        {
            success = MoveFileEx(from, to, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
            if (success)
            {
                if (Info.MainApp.IsEmpty())
                {
                    Info.MainApp = to;
                }
                success = AddEntry(UNOP_FILE, uninstpath);
            }
            else
            {
                ErrorBox();
            }
            SendMessage(g_pInfo->hDlg, IM_PROGRESS, 0, 0);
        }

        if (!success || !FindNextFileW(hFind, &wfd))
            break;
    }
    FindClose(hFind);
    return success;
}

static void
AddUninstallOperationsFromDB(LPCWSTR Name, WCHAR UnOp, CStringW PathPrefix = CStringW(L""))
{
    CStringW item, tmp;
    if (*GetGenerateString(Name, tmp))
    {
        for (int pos = 1; pos > 0;)
        {
            pos = tmp.Find(L'|');
            item = pos <= 0 ? tmp : tmp.Left(pos);
            tmp = tmp.Mid(pos + 1);
            AddEntry(UnOp, PathPrefix + item);
        }
    }
}

static BOOL CALLBACK
ExtractCallback(const EXTRACTCALLBACKINFO &, void *Cookie)
{
    InstallInfo &Info = *(InstallInfo *) Cookie;
    Info.Count += 1;
    return TRUE;
}

static DWORD CALLBACK
ExtractAndInstallThread(LPVOID Parameter)
{
    const BOOL PerUserModeDefault = TRUE;
    InstallInfo &Info = *static_cast<InstallInfo *>(g_pInfo);
    LPCWSTR AppName = Info.AppName, Archive = Info.ArchivePath, None = L"!";
    CStringW installdir, tempdir, files, shortcut, tmp;
    HRESULT hr;
    CRegKey arpkey;
    Info.ArpKey = &arpkey;

    if (!*GetGenerateString(DB_GENINST_FILES, files, L"*.exe|*.*"))
        return ErrorBox(ERROR_BAD_FORMAT);

    GetCommonString(DB_SCOPE, tmp);
    if (tmp.CompareNoCase(L"User") == 0)
        Info.PerUser = TRUE;
    else if (tmp.CompareNoCase(L"Machine") == 0)
        Info.PerUser = FALSE;
    else
        Info.PerUser = PerUserModeDefault;

    hr = GetProgramFilesPath(installdir, Info.PerUser, Info.GetGuiOwner());
    if ((Info.Error = ErrorFromHResult(hr)) != 0)
        return ErrorBox(Info.Error);

    GetGenerateString(DB_GENINST_DIR, tmp);
    if (tmp.Find('%') == 0 && ExpandEnvStrings(tmp))
        installdir = tmp;
    else if (tmp.Compare(None))
        installdir = BuildPath(installdir, tmp.IsEmpty() ? AppName : tmp.GetString());
    Info.InstallDir = installdir.GetString();
    Info.InstallDirLen = installdir.GetLength() + 1;
    hr = SHPathPrepareForWriteW(Info.GetGuiOwner(), NULL, installdir, SHPPFW_DIRCREATE);
    if ((Info.Error = ErrorFromHResult(hr)) != 0)
        return ErrorBox(Info.Error);

    // Create the destination directory, and inside it, a temporary directory
    // where we will extract the archive to before moving the files to their
    // final location (adding uninstall entries as we go)
    tempdir.Format(L"%s\\~RAM%u.tmp", installdir.GetString(), GetCurrentProcessId());
    Info.Error = CreateDirectoryTree(tempdir.GetString());
    if (Info.Error)
        return ErrorBox(Info.Error);

    if (!*GetGenerateString(DB_GENINST_LNK, shortcut))
        shortcut.Format(L"%s.lnk", AppName);
    Info.ShortcutFile = shortcut.Compare(None) ? shortcut.GetString() : NULL;

    // Create the uninstall registration key
    LPCWSTR arpkeyname = AppName;
    tmp = BuildPath(REGPATH_UNINSTALL, arpkeyname);
    HKEY hRoot = Info.PerUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
    REGSAM regsam = KEY_READ | KEY_WRITE | (IsSystem64Bit() ? KEY_WOW64_64KEY : KEY_WOW64_32KEY);
    Info.Error = arpkey.Create(hRoot, tmp, NULL, REG_OPTION_NON_VOLATILE, regsam);
    if (!Info.Error)
    {
        arpkey.RecurseDeleteKey(GENERATE_ARPSUBKEY);
        Info.Error = Info.Entries.Create(arpkey, GENERATE_ARPSUBKEY);
    }
    if (Info.Error)
        ErrorBox(Info.Error);

    if (!Info.Error)
    {
        BOOL isCab = SplitFileAndDirectory(Archive).Right(4).CompareNoCase(L".cab") == 0;
        Info.Error = isCab ? ExtractCab(Archive, tempdir, ExtractCallback, &Info)
                           : ExtractZip(Archive, tempdir, ExtractCallback, &Info);
    }

    if (!Info.Error)
    {
        // We now know how many files we extracted, change from marquee to normal progress bar.
        SendMessage(Info.hDlg, IM_STARTPROGRESS, 0, 0);

        for (int pos = 1; pos > 0 && !Info.Error;)
        {
            pos = files.Find(L'|');
            CStringW item = pos <= 0 ? files : files.Left(pos);
            files = files.Mid(pos + 1);
            InstallFiles(tempdir, item, installdir);
        }

        if (!Info.MainApp.IsEmpty())
        {
            AddUninstallOperationsFromDB(DB_GENINST_DELREG, UNOP_REGKEY);
            AddUninstallOperationsFromDB(DB_GENINST_DELREGEMPTY, UNOP_EMPTYREGKEY);
            AddUninstallOperationsFromDB(DB_GENINST_DELFILE, UNOP_FILE, L"\\");
            AddUninstallOperationsFromDB(DB_GENINST_DELDIR, UNOP_DIR, L"\\");
            AddUninstallOperationsFromDB(DB_GENINST_DELDIREMPTY, UNOP_EMPTYDIR, L"\\");
            AddEntry(UNOP_EMPTYDIR, L"\\");

            WriteArpEntry(L"DisplayName", AppName);
            WriteArpEntry(L"InstallLocation", Info.InstallDir); // Note: This value is used by the uninstaller!

            LPWSTR p = tmp.GetBuffer(1 + MAX_PATH);
            p[0] = L'\"';
            GetModuleFileName(NULL, p + 1, MAX_PATH);
            tmp.ReleaseBuffer();
            UINT cch = tmp.GetLength(), bitness = IsSystem64Bit() ? 64 : 32;
            WCHAR modechar = Info.PerUser ? 'U' : 'M';
            LPCWSTR unparamsfmt = L"\" /" CMD_KEY_UNINSTALL L" /K%s \"%c%d\\%s\"";
            (tmp = tmp.Mid(0, cch)).AppendFormat(unparamsfmt, L"", modechar, bitness, arpkeyname);
            WriteArpEntry(L"UninstallString", tmp);
            (tmp = tmp.Mid(0, cch)).AppendFormat(unparamsfmt, L" /S", modechar, bitness, arpkeyname);
            WriteArpEntry(L"QuietUninstallString", tmp);

            if (GetCustomIconPath(Info, tmp) != S_OK)
                tmp = Info.MainApp;
            WriteArpEntry(L"DisplayIcon", tmp);

            if (*GetCommonString(DB_VERSION, tmp))
                WriteArpEntry(L"DisplayVersion", tmp);

            SYSTEMTIME st;
            GetSystemTime(&st);
            tmp.Format(L"%.4u%.2u%.2u", st.wYear, st.wMonth, st.wDay);
            WriteArpEntry(L"InstallDate", tmp);

            if (*GetCommonString(DB_PUBLISHER, tmp))
                WriteArpEntry(L"Publisher", tmp);

#if DBG
            tmp.Format(L"sys64=%d rapps%d", IsSystem64Bit(), sizeof(void*) * 8);
            WriteArpEntry(L"_DEBUG", tmp);
#endif
        }

        if (!Info.Error && Info.ShortcutFile)
        {
            CreateShortcut(Info.MainApp);
        }
    }

    DeleteDirectoryTree(tempdir.GetString());
    RemoveDirectory(installdir.GetString()); // This is harmless even if we installed something
    return 0;
}

static DWORD CALLBACK
WorkerThread(LPVOID Parameter)
{
    ((LPTHREAD_START_ROUTINE)Parameter)(NULL);
    return SendMessage(g_pInfo->hDlg, IM_END, 0, 0);
}

static INT_PTR CALLBACK
UIDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hPB = GetDlgItem(hDlg, 1);
    switch(uMsg)
    {
        case IM_STARTPROGRESS:
            SetWindowLongPtr(hPB, GWL_STYLE, WS_CHILD | WS_VISIBLE);
            SendMessageW(hPB, PBM_SETMARQUEE, FALSE, 0);
            SendMessageW(hPB, PBM_SETRANGE32, 0, g_pInfo->Count);
            SendMessageW(hPB, PBM_SETPOS, 0, 0);
            break;
        case IM_PROGRESS:
            SendMessageW(hPB, PBM_DELTAPOS, 1, 0);
            break;
        case IM_END:
            DestroyWindow(hDlg);
            break;
        case WM_INITDIALOG:
        {
            SendMessageW(hPB, PBM_SETMARQUEE, TRUE, 0);
            g_pInfo->hDlg = hDlg;
            HICON hIco = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_MAIN));
            SendMessageW(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIco);
            SendMessageW(hDlg, WM_SETTEXT, 0, (LPARAM)g_pInfo->AppName);
            if (!SHCreateThread(WorkerThread, (void*)lParam, CTF_COINIT, NULL))
            {
                ErrorBox();
                SendMessageW(hDlg, IM_END, 0, 0);
            }
            break;
        }
        case WM_CLOSE:
            return TRUE;
        case WM_DESTROY:
            PostMessage(NULL, WM_QUIT, 0, 0);
            break;
    }
    return FALSE;
}

static BOOL
CreateUI(BOOL Silent, LPTHREAD_START_ROUTINE ThreadProc)
{
    enum { DLGW = 150, DLGH = 20, PAD = 4, PADx2 = PAD * 2, CHILDCOUNT = 1 };
    const UINT DlgStyle = WS_CAPTION | DS_MODALFRAME | DS_NOFAILCREATE | DS_CENTER;
    static const WORD DlgTmpl[] =
    {
        LOWORD(DlgStyle), HIWORD(DlgStyle), 0, 0, CHILDCOUNT, 0, 0, DLGW, DLGH, 0, 0, 0,
        PBS_MARQUEE, HIWORD(WS_CHILD | WS_VISIBLE), 0, 0, PAD, PAD, DLGW - PADx2, DLGH - PADx2, 1,
        'm', 's', 'c', 't', 'l', 's', '_', 'p', 'r', 'o', 'g', 'r', 'e', 's', 's', '3', '2', 0, 0,
    };
    HWND hWnd = CreateDialogIndirectParamW(NULL, (LPCDLGTEMPLATE)DlgTmpl, NULL,
                                           UIDlgProc, (LPARAM)ThreadProc);
    if (!hWnd)
    {
        ErrorBox();
        return FALSE;
    }
    else if (!Silent)
    {
        ShowWindow(hWnd, SW_SHOW);
    }
    MSG Msg;
    while (GetMessageW(&Msg, NULL, 0, 0))
    {
        TranslateMessage(&Msg);
        DispatchMessageW(&Msg);
    }
    return TRUE;
}

BOOL
ExtractAndRunGeneratedInstaller(const CAvailableApplicationInfo &AppInfo, LPCWSTR Archive)
{
    InstallInfo Info(AppInfo.szDisplayName, *AppInfo.GetConfigParser(), Archive);
    g_pInfo = &Info;
    return CreateUI(Info.Silent, ExtractAndInstallThread) ? !Info.Error : FALSE;
}

struct UninstallInfo : CommonInfo
{
    CInstalledApplicationInfo &AppInfo;
    UninstallInfo(CInstalledApplicationInfo &Info, BOOL IsSilent)
    : CommonInfo(Info.szDisplayName, IsSilent), AppInfo(Info)
    {
        ArpKey = &Info.GetRegKey();
    }
};

enum UninstallStage
{
    US_ITEMS,
    US_CONTAINERS,
    UINSTALLSTAGECOUNT
};

static DWORD CALLBACK
UninstallThread(LPVOID Parameter)
{
    UninstallInfo &Info = *static_cast<UninstallInfo *>(g_pInfo);

    CStringW tmp, path, installdir;
    path.LoadString(IDS_INSTGEN_CONFIRMUNINST);
    tmp.Format(path.GetString(), Info.AppName);
    if (!Info.Silent &&
        MessageBox(Info.GetGuiOwner(), tmp, Info.AppName, MB_YESNO | MB_ICONQUESTION) != IDYES)
    {
        Info.Error = ERROR_CANCELLED;
        SendMessage(Info.hDlg, IM_END, 0, 0);
        return 0;
    }

    Info.Error = Info.Entries.Open(*Info.ArpKey, GENERATE_ARPSUBKEY, KEY_READ);
    if (Info.Error)
        return ErrorBox(Info.Error);

    RegQueryInfoKey(Info.Entries, NULL, NULL, NULL, NULL, NULL, NULL, &Info.Count, NULL, NULL, NULL, NULL);
    Info.Count *= UINSTALLSTAGECOUNT;
    SendMessage(Info.hDlg, IM_STARTPROGRESS, 0, 0);

    if (!GetRegString(*Info.ArpKey, L"InstallLocation", installdir) || installdir.IsEmpty())
        return ErrorBox(ERROR_INVALID_NAME);

    for (UINT stage = 0; stage < UINSTALLSTAGECOUNT; ++stage)
    {
        for (UINT vi = 0;; ++vi)
        {
            WCHAR value[MAX_PATH], data[MAX_PATH * 2];
            DWORD valsize = _countof(value), size = sizeof(data) - sizeof(WCHAR), rt;
            data[_countof(data) - 1] = UNICODE_NULL;
            UINT err = RegEnumValue(Info.Entries, vi, value, &valsize, NULL, &rt, (BYTE*)data, &size);
            if (err)
            {
                if (err != ERROR_NO_MORE_ITEMS)
                {
                    return ErrorBox(err);
                }
                break;
            }

            LPCWSTR str = data;
            WORD op = value[0];
            switch(*data ? MAKEWORD(stage, op) : 0)
            {
                case MAKEWORD(US_ITEMS, UNOP_REGKEY):
                case MAKEWORD(US_CONTAINERS, UNOP_EMPTYREGKEY):
                {
                    REGSAM wowsam = 0;
                    HKEY hKey = NULL;
                    path.Format(L"%.4s", data);
                    if (path.CompareNoCase(L"HKCR") == 0)
                        hKey = HKEY_CLASSES_ROOT;
                    else if (path.CompareNoCase(L"HKCU") == 0)
                        hKey = HKEY_CURRENT_USER;
                    else if (path.CompareNoCase(L"HKLM") == 0)
                        hKey = HKEY_LOCAL_MACHINE;

                    if (data[4] == '6' && data[5] == '4')
                        wowsam = KEY_WOW64_64KEY;
                    else if (data[4] == '3' && data[5] == '2')
                        wowsam = KEY_WOW64_32KEY;

                    str = &data[wowsam ? 6 : 4];
                    if (!hKey || *str != L'\\')
                        break;
                    tmp = SplitFileAndDirectory(++str, &path);
                    if (!tmp.IsEmpty() && !path.IsEmpty())
                    {
                        CRegKey key;
                        err = key.Open(hKey, path, DELETE | wowsam);
                        if (err == ERROR_SUCCESS)
                        {
                            if (op == UNOP_REGKEY)
                                err = key.RecurseDeleteKey(tmp);
                            else if (RegKeyHasValues(hKey, str, wowsam) == S_FALSE)
                                key.DeleteSubKey(tmp); // DelRegEmpty ignores errors
                        }
                        switch(err)
                        {
                            case ERROR_SUCCESS:
                            case ERROR_FILE_NOT_FOUND:
                            case ERROR_PATH_NOT_FOUND:
                                break;
                            default:
                                return ErrorBox(err);
                        }
                    }
                    break;
                }

                case MAKEWORD(US_ITEMS, UNOP_FILE):
                {
                    if (*data == L'\\')
                        str = (path = BuildPath(installdir, data));

                    if (!DeleteFile(str))
                    {
                        err = GetLastError();
                        if (err != ERROR_FILE_NOT_FOUND)
                        {
                            return ErrorBox(err);
                        }
                    }
                    break;
                }

                case MAKEWORD(US_CONTAINERS, UNOP_EMPTYDIR):
                case MAKEWORD(US_CONTAINERS, UNOP_DIR):
                {
                    if (*data == L'\\')
                        str = (path = BuildPath(installdir, data));

                    if (op == UNOP_DIR)
                        DeleteDirectoryTree(str, Info.GetGuiOwner());
                    else
                        RemoveDirectory(str);
                    break;
                }
            }
            SendMessage(Info.hDlg, IM_PROGRESS, 0, 0);
        }
    }
    if (!Info.Error)
    {
        Info.Error = CAppDB::RemoveInstalledAppFromRegistry(&Info.AppInfo);
        if (Info.Error)
            return ErrorBox(Info.Error);
    }
    return 0;
}

BOOL
UninstallGenerated(CInstalledApplicationInfo &AppInfo, UninstallCommandFlags Flags)
{
    UninstallInfo Info(AppInfo, Flags & UCF_SILENT);
    g_pInfo = &Info;
    return CreateUI(Info.Silent, UninstallThread) ? !Info.Error : FALSE;
}