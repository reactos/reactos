/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/main.cpp
 * PURPOSE:     Initializing everything
 * PROGRAMMERS: Benedikt Freisen
 */

#include "precomp.h"

POINT start;
POINT last;

BOOL askBeforeEnlarging = FALSE;  // TODO: initialize from registry
HINSTANCE hProgInstance = NULL;
TCHAR filepathname[MAX_LONG_PATH] = { 0 };
BOOL isAFile = FALSE;
BOOL imageSaved = FALSE;
BOOL showGrid = FALSE;

CMainWindow mainWindow;

/* FUNCTIONS ********************************************************/

// get file name extension from filter string
static BOOL
FileExtFromFilter(LPTSTR pExt, OPENFILENAME *pOFN)
{
    LPTSTR pchExt = pExt;
    *pchExt = 0;

    DWORD nIndex = 1;
    for (LPCTSTR pch = pOFN->lpstrFilter; *pch; ++nIndex)
    {
        pch += lstrlen(pch) + 1;
        if (pOFN->nFilterIndex == nIndex)
        {
            for (++pch; *pch && *pch != _T(';'); ++pch)
            {
                *pchExt++ = *pch;
            }
            *pchExt = 0;
            CharLower(pExt);
            return TRUE;
        }
        pch += lstrlen(pch) + 1;
    }
    return FALSE;
}

// Hook procedure for OPENFILENAME to change the file name extension
static UINT_PTR APIENTRY
OFNHookProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hParent;
    OFNOTIFY *pon;
    switch (uMsg)
    {
    case WM_NOTIFY:
        pon = (OFNOTIFY *)lParam;
        if (pon->hdr.code == CDN_TYPECHANGE)
        {
            hParent = GetParent(hwnd);
            TCHAR Path[MAX_PATH];
            SendMessage(hParent, CDM_GETFILEPATH, _countof(Path), (LPARAM)Path);
            FileExtFromFilter(PathFindExtension(Path), pon->lpOFN);
            SendMessage(hParent, CDM_SETCONTROLTEXT, 0x047c, (LPARAM)PathFindFileName(Path));
            lstrcpyn(pon->lpOFN->lpstrFile, Path, pon->lpOFN->nMaxFile);
        }
        break;
    }
    return 0;
}

BOOL CMainWindow::GetOpenFileName(IN OUT LPTSTR pszFile, INT cchMaxFile)
{
    static OPENFILENAME ofn = { 0 };
    static CString strFilter;

    if (ofn.lStructSize == 0)
    {
        // The "All Files" item text
        CString strAllPictureFiles;
        strAllPictureFiles.LoadString(hProgInstance, IDS_ALLPICTUREFILES);

        // Get the import filter
        CSimpleArray<GUID> aguidFileTypesI;
        CImage::GetImporterFilterString(strFilter, aguidFileTypesI, strAllPictureFiles,
                                        CImage::excludeDefaultLoad, _T('\0'));

        // Initializing the OPENFILENAME structure for GetOpenFileName
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner   = m_hWnd;
        ofn.hInstance   = hProgInstance;
        ofn.lpstrFilter = strFilter;
        ofn.Flags       = OFN_EXPLORER | OFN_HIDEREADONLY;
        ofn.lpstrDefExt = L"png";
    }

    ofn.lpstrFile = pszFile;
    ofn.nMaxFile  = cchMaxFile;
    return ::GetOpenFileName(&ofn);
}

BOOL CMainWindow::GetSaveFileName(IN OUT LPTSTR pszFile, INT cchMaxFile)
{
    static OPENFILENAME sfn = { 0 };
    static CString strFilter;

    if (sfn.lStructSize == 0)
    {
        // Get the export filter
        CSimpleArray<GUID> aguidFileTypesE;
        CImage::GetExporterFilterString(strFilter, aguidFileTypesE, NULL,
                                        CImage::excludeDefaultSave, _T('\0'));

        // Initializing the OPENFILENAME structure for GetSaveFileName
        ZeroMemory(&sfn, sizeof(sfn));
        sfn.lStructSize = sizeof(sfn);
        sfn.hwndOwner   = m_hWnd;
        sfn.hInstance   = hProgInstance;
        sfn.lpstrFilter = strFilter;
        sfn.Flags       = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_ENABLEHOOK;
        sfn.lpfnHook    = OFNHookProc;
        sfn.lpstrDefExt = L"png";

        // Choose PNG
        for (INT i = 0; i < aguidFileTypesE.GetSize(); ++i)
        {
            if (aguidFileTypesE[i] == Gdiplus::ImageFormatPNG)
            {
                sfn.nFilterIndex = i + 1;
                break;
            }
        }
    }

    sfn.lpstrFile = pszFile;
    sfn.nMaxFile  = cchMaxFile;
    return ::GetSaveFileName(&sfn);
}

BOOL CMainWindow::ChooseColor(IN OUT COLORREF *prgbColor)
{
    static CHOOSECOLOR choosecolor = { 0 };
    static COLORREF custColors[16] =
    {
        0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff,
        0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff
    };

    if (choosecolor.lStructSize == 0)
    {
        // Initializing the CHOOSECOLOR structure for ChooseColor
        ZeroMemory(&choosecolor, sizeof(choosecolor));
        choosecolor.lStructSize  = sizeof(choosecolor);
        choosecolor.hwndOwner    = m_hWnd;
        choosecolor.lpCustColors = custColors;
    }

    choosecolor.rgbResult = *prgbColor;
    if (!::ChooseColor(&choosecolor))
        return FALSE;

    *prgbColor = choosecolor.rgbResult;
    return TRUE;
}

HWND CMainWindow::DoCreate()
{
    ::LoadString(hProgInstance, IDS_DEFAULTFILENAME, filepathname, _countof(filepathname));

    CString strTitle;
    strTitle.Format(IDS_WINDOWTITLE, PathFindFileName(filepathname));

    RECT& rc = registrySettings.WindowPlacement.rcNormalPosition;
    return Create(HWND_DESKTOP, rc, strTitle, WS_OVERLAPPEDWINDOW, WS_EX_ACCEPTFILES);
}

// entry point
INT WINAPI
_tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, INT nCmdShow)
{
#ifdef _DEBUG
    // Report any memory leaks on exit
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    hProgInstance = hInstance;

    // Initialize common controls library
    INITCOMMONCONTROLSEX iccx;
    iccx.dwSize = sizeof(iccx);
    iccx.dwICC = ICC_STANDARD_CLASSES | ICC_USEREX_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&iccx);

    // Load settings from registry
    registrySettings.Load(nCmdShow);

    // Create the main window
    if (!mainWindow.DoCreate())
    {
        MessageBox(NULL, TEXT("Failed to create main window."), NULL, MB_ICONERROR);
        return 1;
    }

    // Initialize imageModel
    imageModel.Crop(registrySettings.BMPWidth, registrySettings.BMPHeight);
    if (__argc >= 2)
        DoLoadImageFile(mainWindow, __targv[1], TRUE);
    imageModel.ClearHistory();

    // Make the window visible on the screen
    mainWindow.ShowWindow(registrySettings.WindowPlacement.showCmd);

    // Load the access keys
    HACCEL hAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(800));

    // The message loop
    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0))
    {
        if (fontsDialog.IsWindow() && fontsDialog.IsDialogMessage(&msg))
            continue;

        if (::TranslateAccelerator(mainWindow, hAccel, &msg))
            continue;

        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    // Unload the access keys
    ::DestroyAcceleratorTable(hAccel);

    // Write back settings to registry
    registrySettings.Store();

    // Return the value that PostQuitMessage() gave
    return (INT)msg.wParam;
}
