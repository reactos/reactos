/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Zip extraction
 * COPYRIGHT:   Copyright 2017-2019 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

class CZipExtract :
    public IZip
{
    CStringW m_Filename;
    CStringW m_Directory;
    CStringA m_Password;
    bool m_DirectoryChanged;
    unzFile uf;
public:
    CZipExtract(PCWSTR Filename)
        :m_DirectoryChanged(false)
        ,uf(NULL)
    {
        m_Filename = Filename;
        m_Directory = m_Filename;
        PWSTR Dir = m_Directory.GetBuffer();
        PathRemoveExtensionW(Dir);
        m_Directory.ReleaseBuffer();
    }

    ~CZipExtract()
    {
        if (uf)
        {
            DPRINT1("WARNING: uf not closed!\n");
            Close();
        }
    }

    void Close()
    {
        if (uf)
            unzClose(uf);
        uf = NULL;
    }

    // *** IZip methods ***
    STDMETHODIMP QueryInterface(REFIID riid, void  **ppvObject)
    {
        if (riid == IID_IUnknown)
        {
            *ppvObject = this;
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef(void)
    {
        return 2;
    }
    STDMETHODIMP_(ULONG) Release(void)
    {
        return 1;
    }
    STDMETHODIMP_(unzFile) getZip()
    {
        return uf;
    }

    class CExtractSettingsPage : public CPropertyPageImpl<CExtractSettingsPage>
    {
    private:
        HANDLE m_hExtractionThread;
        bool m_bExtractionThreadCancel;

        CZipExtract* m_pExtract;
        CStringA* m_pPassword;
        CStringW m_OldStatus;

    public:
        CExtractSettingsPage(CZipExtract* extract, CStringA* password)
            :CPropertyPageImpl<CExtractSettingsPage>(MAKEINTRESOURCE(IDS_WIZ_TITLE))
            ,m_hExtractionThread(NULL)
            ,m_bExtractionThreadCancel(false)
            ,m_pExtract(extract)
            ,m_pPassword(password)
        {
            m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_WIZ_DEST_TITLE);
            m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_WIZ_DEST_SUBTITLE);
            m_psp.dwFlags |= PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        }

        int OnSetActive()
        {
            SetDlgItemTextW(IDC_DIRECTORY, m_pExtract->m_Directory);
            m_pExtract->m_DirectoryChanged = false;
            GetParent().CenterWindow(::GetDesktopWindow());
            SetWizardButtons(PSWIZB_NEXT);
            return 0;
        }

        int OnWizardNext()
        {
            if (m_hExtractionThread != NULL)
            {
                /* We enter here when extraction has finished, and go to next page if it succeeded */
                WaitForSingleObject(m_hExtractionThread, INFINITE);
                CloseHandle(m_hExtractionThread);
                m_hExtractionThread = NULL;
                m_pExtract->Release();
                if (!m_bExtractionThreadCancel)
                {
                    return 0;
                }
                else
                {
                    SetWindowLongPtr(DWLP_MSGRESULT, -1);
                    return TRUE;
                }
            }

            /* We end up here if the user manually clicks Next: start extraction */
            m_bExtractionThreadCancel = false;

            /* Grey out every control during extraction to prevent user interaction */
            ::EnableWindow(GetDlgItem(IDC_BROWSE), FALSE);
            ::EnableWindow(GetDlgItem(IDC_DIRECTORY), FALSE);
            ::EnableWindow(GetDlgItem(IDC_PASSWORD), FALSE);
            SetWizardButtons(0);

            ::GetWindowTextW(GetDlgItem(IDC_STATUSTEXT), m_OldStatus.GetBuffer(MAX_PATH), MAX_PATH);
            m_OldStatus.ReleaseBuffer();
            CStringW strExtracting(MAKEINTRESOURCEW(IDS_EXTRACTING));
            SetDlgItemTextW(IDC_STATUSTEXT, strExtracting);

            if (m_pExtract->m_DirectoryChanged)
                UpdateDirectory();

            m_pExtract->AddRef();

            m_hExtractionThread = CreateThread(NULL, 0,
                                               &CExtractSettingsPage::ExtractEntry,
                                               this,
                                               0, NULL);
            if (!m_hExtractionThread)
            {
                /* Extraction thread creation failed, do not go to the next page */
                DWORD err = GetLastError();
                DPRINT1("ERROR, m_hExtractionThread: CreateThread failed: 0x%x\n", err);
                m_pExtract->Release();

                SetWindowLongPtr(DWLP_MSGRESULT, -1);

                ::EnableWindow(GetDlgItem(IDC_BROWSE), TRUE);
                ::EnableWindow(GetDlgItem(IDC_DIRECTORY), TRUE);
                ::EnableWindow(GetDlgItem(IDC_PASSWORD), TRUE);
                SetWizardButtons(PSWIZB_NEXT);
            }
            return TRUE;
        }

        void WizardReset()
        {
            SetDlgItemTextW(IDC_STATUSTEXT, m_OldStatus);
        }

        static DWORD WINAPI ExtractEntry(LPVOID lpParam)
        {
            CExtractSettingsPage* pPage = (CExtractSettingsPage*)lpParam;
            bool res = pPage->m_pExtract->Extract(pPage->m_hWnd, pPage->GetDlgItem(IDC_PROGRESS), &(pPage->m_bExtractionThreadCancel));
            /* Failing and cancelling extraction both mean we stay on the same property page */
            pPage->m_bExtractionThreadCancel = !res;

            pPage->SetWizardButtons(PSWIZB_NEXT);
            if (!res)
            {
                /* Extraction failed/cancelled: the page becomes interactive again */
                ::EnableWindow(pPage->GetDlgItem(IDC_BROWSE), TRUE);
                ::EnableWindow(pPage->GetDlgItem(IDC_DIRECTORY), TRUE);
                ::EnableWindow(pPage->GetDlgItem(IDC_PASSWORD), TRUE);

                /* Reset the progress bar's appearance */
                CWindow Progress(pPage->GetDlgItem(IDC_PROGRESS));
                Progress.SendMessage(PBM_SETRANGE32, 0, 1);
                Progress.SendMessage(PBM_SETPOS, 0, 0);
                pPage->WizardReset();
            }
            SendMessageCallback(pPage->GetParent().m_hWnd, PSM_PRESSBUTTON, PSBTN_NEXT, 0, NULL, NULL);

            return 0;
        }

        BOOL OnQueryCancel()
        {
            if (m_hExtractionThread != NULL)
            {
                /* Extraction will check the value of m_bExtractionThreadCancel between each file in the archive */
                m_bExtractionThreadCancel = true;
                return TRUE;
            }
            return FALSE;
        }

        struct browse_info
        {
            HWND hWnd;
            LPCWSTR Directory;
        };

        static INT CALLBACK s_BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lp, LPARAM pData)
        {
            if (uMsg == BFFM_INITIALIZED)
            {
                browse_info* info = (browse_info*)pData;
                CWindow dlg(hWnd);
                dlg.SendMessage(BFFM_SETSELECTION, TRUE, (LPARAM)info->Directory);
                dlg.CenterWindow(info->hWnd);
            }
            return 0;
        }

        LRESULT OnBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
        {
            BROWSEINFOW bi = { m_hWnd };
            WCHAR path[MAX_PATH];
            bi.pszDisplayName = path;
            bi.lpfn = s_BrowseCallbackProc;
            bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
            CStringW title(MAKEINTRESOURCEW(IDS_WIZ_BROWSE_TITLE));
            bi.lpszTitle = title;

            if (m_pExtract->m_DirectoryChanged)
                UpdateDirectory();

            browse_info info = { m_hWnd, m_pExtract->m_Directory.GetString() };
            bi.lParam = (LPARAM)&info;

            CComHeapPtr<ITEMIDLIST> pidl;
            pidl.Attach(SHBrowseForFolderW(&bi));

            WCHAR tmpPath[MAX_PATH];
            if (pidl && SHGetPathFromIDListW(pidl, tmpPath))
            {
                m_pExtract->m_Directory = tmpPath;
                SetDlgItemTextW(IDC_DIRECTORY, m_pExtract->m_Directory);
                m_pExtract->m_DirectoryChanged = false;
            }
            return 0;
        }

        LRESULT OnEnChangeDirectory(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
        {
            m_pExtract->m_DirectoryChanged = true;
            return 0;
        }

        LRESULT OnPassword(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
        {
            CStringA Password;
            if (_CZipAskPassword(m_hWnd, NULL, Password) == eAccept)
            {
                *m_pPassword = Password;
            }
            return 0;
        }

        void UpdateDirectory()
        {
            GetDlgItemText(IDC_DIRECTORY, m_pExtract->m_Directory);
            m_pExtract->m_DirectoryChanged = false;
        }

    public:
        enum { IDD = IDD_PROPPAGEDESTINATION };

        BEGIN_MSG_MAP(CCompleteSettingsPage)
            COMMAND_ID_HANDLER(IDC_BROWSE, OnBrowse)
            COMMAND_ID_HANDLER(IDC_PASSWORD, OnPassword)
            COMMAND_HANDLER(IDC_DIRECTORY, EN_CHANGE, OnEnChangeDirectory)
            CHAIN_MSG_MAP(CPropertyPageImpl<CExtractSettingsPage>)
        END_MSG_MAP()
    };


    class CCompleteSettingsPage : public CPropertyPageImpl<CCompleteSettingsPage>
    {
    private:
        CZipExtract* m_pExtract;

    public:
        CCompleteSettingsPage(CZipExtract* extract)
            :CPropertyPageImpl<CCompleteSettingsPage>(MAKEINTRESOURCE(IDS_WIZ_TITLE))
            , m_pExtract(extract)
        {
            m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_WIZ_COMPL_TITLE);
            m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_WIZ_COMPL_SUBTITLE);
            m_psp.dwFlags |= PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        }


        int OnSetActive()
        {
            SetWizardButtons(PSWIZB_FINISH);
            CStringW Path = m_pExtract->m_Directory;
            PWSTR Ptr = Path.GetBuffer(MAX_PATH);
            RECT rc;
            ::GetWindowRect(GetDlgItem(IDC_DESTDIR), &rc);
            HDC dc = GetDC();
            PathCompactPathW(dc, Ptr, rc.right - rc.left);
            ReleaseDC(dc);
            Path.ReleaseBuffer();
            SetDlgItemTextW(IDC_DESTDIR, Path);
            CheckDlgButton(IDC_SHOW_EXTRACTED, BST_CHECKED);
            return 0;
        }
        BOOL OnWizardFinish()
        {
            if (IsDlgButtonChecked(IDC_SHOW_EXTRACTED) == BST_CHECKED)
            {
                ShellExecuteW(NULL, L"explore", m_pExtract->m_Directory, NULL, NULL, SW_SHOW);
            }
            return FALSE;
        }

    public:
        enum { IDD = IDD_PROPPAGECOMPLETE };

        BEGIN_MSG_MAP(CCompleteSettingsPage)
            CHAIN_MSG_MAP(CPropertyPageImpl<CCompleteSettingsPage>)
        END_MSG_MAP()
    };


    void runWizard()
    {
        PROPSHEETHEADERW psh = { sizeof(psh), 0 };
        psh.dwFlags = PSH_WIZARD97 | PSH_HEADER;
        psh.hInstance = _AtlBaseModule.GetResourceInstance();

        CExtractSettingsPage extractPage(this, &m_Password);
        CCompleteSettingsPage completePage(this);
        HPROPSHEETPAGE hpsp[] =
        {
            extractPage.Create(),
            completePage.Create()
        };

        psh.phpage = hpsp;
        psh.nPages = _countof(hpsp);
        psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
        psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);

        PropertySheetW(&psh);
    }

    eZipExtractError ExtractSingle(
        HWND hDlg,
        LPCSTR FullPath,
        bool is_dir,
        unz_file_info64* Info,
        CStringA Name,
        CStringA Password,
        bool* bOverwriteAll,
        const bool* bCancel,
        int* ErrorCode
    )
    {
        int err;
        BYTE Buffer[2048];
        DWORD dwFlags = SHPPFW_DIRCREATE | (is_dir ? SHPPFW_NONE : SHPPFW_IGNOREFILENAME);
        HRESULT hr = SHPathPrepareForWriteA(hDlg, NULL, FullPath, dwFlags);
        if (FAILED_UNEXPECTEDLY(hr))
        {
            *ErrorCode = hr;
            return eDirectoryError;
        }
        if (is_dir)
            return eNoError;

        if (Info->flag & MINIZIP_PASSWORD_FLAG)
        {
            eZipPasswordResponse Response = eAccept;
            do
            {
                /* If there is a password set, try it */
                if (!Password.IsEmpty())
                {
                    err = unzOpenCurrentFilePassword(uf, Password);
                    if (err == UNZ_OK)
                    {
                        /* Try to read some bytes, because unzOpenCurrentFilePassword does not return failure */
                        char Buf[10];
                        err = unzReadCurrentFile(uf, Buf, sizeof(Buf));
                        unzCloseCurrentFile(uf);
                        if (err >= UNZ_OK)
                        {
                            /* 're'-open the file so that we can begin to extract */
                            err = unzOpenCurrentFilePassword(uf, Password);
                            break;
                        }
                    }
                }
                Response = _CZipAskPassword(hDlg, Name, Password);
            } while (Response == eAccept);

            if (Response == eSkip)
            {
                return eNoError;
            }
            else if (Response == eAbort)
            {
                return eExtractAbort;
            }
        }
        else
        {
            err = unzOpenCurrentFile(uf);
        }

        if (err != UNZ_OK)
        {
            DPRINT1("ERROR, unzOpenCurrentFilePassword: 0x%x\n", err);
            *ErrorCode = err;
            return eOpenError;
        }

        HANDLE hFile = CreateFileA(FullPath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            DWORD dwErr = GetLastError();
            if (dwErr == ERROR_FILE_EXISTS)
            {
                bool bOverwrite = *bOverwriteAll;
                if (!*bOverwriteAll)
                {
                    eZipConfirmResponse Result = _CZipAskReplace(hDlg, FullPath);
                    switch (Result)
                    {
                    case eYesToAll:
                        *bOverwriteAll = true;
                        /* fall through */
                    case eYes:
                        bOverwrite = true;
                        break;
                    case eNo:
                        break;
                    case eCancel:
                        unzCloseCurrentFile(uf);
                        return eExtractAbort;
                    }
                }

                if (bOverwrite)
                {
                    hFile = CreateFileA(FullPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hFile == INVALID_HANDLE_VALUE)
                    {
                        dwErr = GetLastError();
                    }
                }
                else
                {
                    unzCloseCurrentFile(uf);
                    return eNoError;
                }
            }
            if (hFile == INVALID_HANDLE_VALUE)
            {
                unzCloseCurrentFile(uf);
                DPRINT1("ERROR, CreateFileA: 0x%x (%s)\n", dwErr, *bOverwriteAll ? "Y" : "N");
                *ErrorCode = dwErr;
                return eFileError;
            }
        }

        do
        {
            if (*bCancel)
            {
                CloseHandle(hFile);
                BOOL deleteResult = DeleteFileA(FullPath);
                if (!deleteResult)
                    DPRINT1("ERROR, DeleteFileA: 0x%x\n", GetLastError());
                return eExtractAbort;
            }

            err = unzReadCurrentFile(uf, Buffer, sizeof(Buffer));

            if (err < 0)
            {
                DPRINT1("ERROR, unzReadCurrentFile: 0x%x\n", err);
                break;
            }
            else if (err > 0)
            {
                DWORD dwWritten;
                if (!WriteFile(hFile, Buffer, err, &dwWritten, NULL))
                {
                    DPRINT1("ERROR, WriteFile: 0x%x\n", GetLastError());
                    break;
                }
                if (dwWritten != (DWORD)err)
                {
                    DPRINT1("ERROR, WriteFile: dwWritten:%d err:%d\n", dwWritten, err);
                    break;
                }
            }

        } while (err > 0);

        /* Update Filetime */
        FILETIME LocalFileTime;
        DosDateTimeToFileTime((WORD)(Info->dosDate >> 16), (WORD)Info->dosDate, &LocalFileTime);
        FILETIME FileTime;
        LocalFileTimeToFileTime(&LocalFileTime, &FileTime);
        SetFileTime(hFile, &FileTime, &FileTime, &FileTime);

        /* Done */
        CloseHandle(hFile);

        if (err)
        {
            unzCloseCurrentFile(uf);
            DPRINT1("ERROR, unzReadCurrentFile2: 0x%x\n", err);
            *ErrorCode = err;
            return eUnpackError;
        }
        else
        {
            err = unzCloseCurrentFile(uf);
            if (err != UNZ_OK)
            {
                DPRINT1("ERROR(non-fatal), unzCloseCurrentFile: 0x%x\n", err);
            }
        }
        return eNoError;
    }

    bool Extract(HWND hDlg, HWND hProgress, const bool* bCancel)
    {
        unz_global_info64 gi;
        uf = unzOpen2_64(m_Filename.GetString(), &g_FFunc);
        int err = unzGetGlobalInfo64(uf, &gi);
        if (err != UNZ_OK)
        {
            DPRINT1("ERROR, unzGetGlobalInfo64: 0x%x\n", err);
            Close();
            return false;
        }

        CZipEnumerator zipEnum;
        if (!zipEnum.initialize(this))
        {
            DPRINT1("ERROR, zipEnum.initialize\n");
            Close();
            return false;
        }

        CWindow Progress(hProgress);
        Progress.SendMessage(PBM_SETRANGE32, 0, gi.number_entry);
        Progress.SendMessage(PBM_SETPOS, 0, 0);

        CStringA BaseDirectory = m_Directory;
        CStringA Name;
        CStringA Password = m_Password;
        unz_file_info64 Info;
        int CurrentFile = 0;
        bool bOverwriteAll = false;
        while (zipEnum.next(Name, Info))
        {
            if (*bCancel)
            {
                Close();
                return false;
            }

            bool is_dir = Name.GetLength() > 0 && Name[Name.GetLength()-1] == '/';

            char CombinedPath[MAX_PATH * 2] = { 0 };
            PathCombineA(CombinedPath, BaseDirectory, Name);
            CStringA FullPath = CombinedPath;
            FullPath.Replace('/', '\\');    /* SHPathPrepareForWriteA does not handle '/' */
        Retry:
            eZipExtractError Result = ExtractSingle(hDlg, FullPath, is_dir, &Info, Name, Password, &bOverwriteAll, bCancel, &err);
            if (Result != eDirectoryError)
                CurrentFile++;
            switch (Result)
            {
                case eNoError:
                    break;

                case eExtractAbort:
                case eUnpackError:
                {
                    Close();
                    return false;
                }

                case eDirectoryError:
                {
                    char StrippedPath[MAX_PATH] = { 0 };

                    StrCpyNA(StrippedPath, FullPath, _countof(StrippedPath));
                    if (!is_dir)
                        PathRemoveFileSpecA(StrippedPath);
                    PathStripPathA(StrippedPath);
                    if (ShowExtractError(hDlg, (LPCSTR)&StrippedPath, err, eDirectoryError) == IDRETRY)
                        goto Retry;
                    Close();
                    return false;
                }

                case eFileError:
                {
                    int Result = ShowExtractError(hDlg, FullPath, err, eFileError);
                    switch (Result)
                    {
                    case IDABORT:
                        Close();
                        return false;
                    case IDRETRY:
                        CurrentFile--;
                        goto Retry;
                    case IDIGNORE:
                        break;
                    }
                    break;
                }

                case eOpenError:
                {
                    if (err == UNZ_BADZIPFILE &&
                        Info.compression_method != 0 &&
                        Info.compression_method != Z_DEFLATED &&
                        Info.compression_method != Z_BZIP2ED)
                    {
                        if (ShowExtractError(hDlg, FullPath, Info.compression_method, eOpenError) == IDYES)
                            break;
                    }
                    Close();
                    return false;
                }
            }
            if (Result == eNoError && is_dir)
                continue;
            Progress.SendMessage(PBM_SETPOS, CurrentFile, 0);
        }

        Close();
        return true;
    }

    int ShowExtractError(HWND hDlg, LPCSTR path, int Error, eZipExtractError ErrorType)
    {
        CStringA strTitle(MAKEINTRESOURCEW(IDS_ERRORTITLE));
        CStringA strErr, strText;
        PSTR Win32ErrorString;

        if (ErrorType == eFileError || ErrorType == eOpenError)
            strText.LoadString(IDS_CANTEXTRACTFILE);
        else
            strText.LoadString(GetModuleHandleA("shell32.dll"), 128); // IDS_CREATEFOLDER_DENIED

        strText.FormatMessage(strText.GetString(), path);

        if (ErrorType == eFileError || HRESULT_FACILITY(Error) == FACILITY_WIN32)
        {
            if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                               NULL, ErrorType == eFileError ? Error : HRESULT_CODE(Error), 0,
                               (PSTR)&Win32ErrorString, 0, NULL) != 0)
            {
                strErr.SetString(Win32ErrorString);
                LocalFree(Win32ErrorString);
            }
        }
        if (ErrorType == eOpenError)
            strErr.Format(IDS_DECOMPRESSERROR, Error);
        else if (strErr.GetLength() == 0)
            strErr.Format(IDS_UNKNOWNERROR, Error);

        strText.Append("\r\n\r\n" + strErr);

        UINT mbFlags = MB_ICONWARNING;
        if (ErrorType == eDirectoryError)
            mbFlags |= MB_RETRYCANCEL;
        else if (ErrorType == eFileError)
            mbFlags |= MB_ABORTRETRYIGNORE;
        else if (ErrorType == eOpenError)
            mbFlags |= MB_YESNO;

        return MessageBoxA(hDlg, strText, strTitle, mbFlags);
    }
};


void _CZipExtract_runWizard(PCWSTR Filename)
{
    CZipExtract extractor(Filename);
    extractor.runWizard();
}

