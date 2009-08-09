/*
 * ReactOS Sound Volume Control
 * Copyright (C) 2004-2005 Thomas Weidenmueller
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Sound Volume Control
 * FILE:        subsys/system/sndvol32/sndvol32.c
 * PROGRAMMERS: Thomas Weidenmueller <w3seek@reactos.com>
 */
#include <sndvol32.h>

HINSTANCE hAppInstance;
ATOM MainWindowClass;
HWND hMainWnd;
HANDLE hAppHeap;
LPTSTR lpAppTitle;

#define GetDialogData(hwndDlg, type) \
    ( P##type )GetWindowLongPtr((hwndDlg), DWLP_USER)
#define GetWindowData(hwnd, type) \
    ( P##type )GetWindowLongPtr((hwnd), GWL_USERDATA)

/******************************************************************************/

typedef struct _PREFERENCES_CONTEXT
{
    PMIXER_WINDOW MixerWindow;
    PSND_MIXER Mixer;
    HWND hwndDlg;

    UINT Selected;
    DWORD SelectedLine;
    DWORD PlaybackID;
    DWORD RecordingID;
    UINT OtherLines;
    TCHAR DeviceName[128];

    DWORD tmp;
} PREFERENCES_CONTEXT, *PPREFERENCES_CONTEXT;

typedef struct _PREFERENCES_FILL_DEVICES
{
    PPREFERENCES_CONTEXT PrefContext;
    HWND hComboBox;
    UINT Selected;
} PREFERENCES_FILL_DEVICES, *PPREFERENCES_FILL_DEVICES;

static BOOL CALLBACK
FillDeviceComboBox(PSND_MIXER Mixer,
                   UINT Id,
                   LPCTSTR ProductName,
                   PVOID Context)
{
    LRESULT lres;
    PPREFERENCES_FILL_DEVICES FillContext = (PPREFERENCES_FILL_DEVICES)Context;

    UNREFERENCED_PARAMETER(Mixer);

    lres = SendMessage(FillContext->hComboBox,
                       CB_ADDSTRING,
                       0,
                       (LPARAM)ProductName);
    if (lres != CB_ERR)
    {
        /* save the index so we don't screw stuff when the combobox is sorted... */
        SendMessage(FillContext->hComboBox,
                    CB_SETITEMDATA,
                    (WPARAM)lres,
                    Id);

        if (Id == FillContext->Selected)
        {
            SendMessage(FillContext->hComboBox,
                        CB_SETCURSEL,
                        (WPARAM)lres,
                        0);
        }
    }

    return TRUE;
}

static BOOL CALLBACK
PrefDlgAddLine(PSND_MIXER Mixer,
               LPMIXERLINE Line,
               UINT DisplayControls,
               PVOID Context)
{
    PPREFERENCES_CONTEXT PrefContext = (PPREFERENCES_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Mixer);
	UNREFERENCED_PARAMETER(DisplayControls);

    switch (Line->dwComponentType)
    {
        case MIXERLINE_COMPONENTTYPE_DST_SPEAKERS:
            if (PrefContext->PlaybackID == (DWORD)-1)
            {
                PrefContext->PlaybackID = Line->dwLineID;

                if (PrefContext->SelectedLine == (DWORD)-1)
                {
                    PrefContext->SelectedLine = Line->dwLineID;
                }
            }
            else
                goto AddToOthersLines;

            break;

        case MIXERLINE_COMPONENTTYPE_DST_WAVEIN:
            if (PrefContext->RecordingID == (DWORD)-1)
            {
                PrefContext->RecordingID = Line->dwLineID;

                if (PrefContext->SelectedLine == (DWORD)-1)
                {
                    PrefContext->SelectedLine = Line->dwLineID;
                }
            }
            else
                goto AddToOthersLines;

            break;

        default:
        {
            LRESULT lres;
            HWND hwndCbOthers;

            if (PrefContext->SelectedLine == (DWORD)-1)
            {
                PrefContext->SelectedLine = Line->dwLineID;
            }

AddToOthersLines:
            hwndCbOthers = GetDlgItem(PrefContext->hwndDlg,
                                      IDC_LINE);

            lres = SendMessage(hwndCbOthers,
                               CB_ADDSTRING,
                               0,
                               (LPARAM)Line->szName);
            if (lres != CB_ERR)
            {
                SendMessage(hwndCbOthers,
                            CB_SETITEMDATA,
                            (WPARAM)lres,
                            Line->dwLineID);

                PrefContext->OtherLines++;
            }
            break;
        }
    }

    return TRUE;
}

static BOOL CALLBACK
PrefDlgAddConnection(PSND_MIXER Mixer,
                     DWORD LineID,
                     LPMIXERLINE Line,
                     PVOID Context)
{
    PPREFERENCES_CONTEXT PrefContext = (PPREFERENCES_CONTEXT)Context;
    HWND hwndControls;
    LVITEM lvi;
    UINT i;

    UNREFERENCED_PARAMETER(Mixer);
	UNREFERENCED_PARAMETER(LineID);

    if (Line->cControls != 0)
    {
        hwndControls = GetDlgItem(PrefContext->hwndDlg,
                                  IDC_CONTROLS);

        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = PrefContext->tmp++;
        lvi.iSubItem = 0;
        lvi.pszText = Line->szName;
        lvi.lParam = (LPARAM)Line->dwSource;

        i = SendMessage(hwndControls,
                        LVM_INSERTITEM,
                        0,
                        (LPARAM)&lvi);
        if (i != (UINT)-1)
        {
            TCHAR LineName[MIXER_LONG_NAME_CHARS];
            DWORD Flags;
            BOOL SelLine = FALSE;

            if (SndMixerGetLineName(PrefContext->Mixer,
                                    PrefContext->SelectedLine,
                                    LineName,
                                    MIXER_LONG_NAME_CHARS,
                                    FALSE) == -1)
            {
                LineName[0] = TEXT('\0');
            }

            if (ReadLineConfig(PrefContext->DeviceName,
                               LineName,
                               Line->szName,
                               &Flags))
            {
                if (Flags != 0x4)
                {
                    SelLine = TRUE;
                }
            }

            ListView_SetCheckState(hwndControls,
                                   i,
                                   SelLine);
        }
    }

    return TRUE;
}

static VOID
UpdatePrefDlgControls(PPREFERENCES_CONTEXT Context,
                      DWORD LineID)
{
    INT OldID, MixerID = 0;
    INT DeviceCbIndex;

    /* select the mixer */
    DeviceCbIndex = SendDlgItemMessage(Context->hwndDlg,
                                       IDC_MIXERDEVICE,
                                       CB_GETCURSEL,
                                       0,
                                       0);
    if (DeviceCbIndex != CB_ERR)
    {
        MixerID = SendDlgItemMessage(Context->hwndDlg,
                                     IDC_MIXERDEVICE,
                                     CB_GETITEMDATA,
                                     DeviceCbIndex,
                                     0);
        if (MixerID == CB_ERR)
        {
            MixerID = 0;
        }
    }

    OldID = Context->Selected;
    if (MixerID != OldID &&
        SndMixerSelect(Context->Mixer,
                       MixerID))
    {
        Context->Selected = SndMixerGetSelection(Context->Mixer);

        /* update the controls */
        Context->PlaybackID = (DWORD)-1;
        Context->RecordingID = (DWORD)-1;
        Context->OtherLines = 0;
        Context->SelectedLine = (DWORD)-1;

        SndMixerGetProductName(Context->Mixer,
                               Context->DeviceName,
                               sizeof(Context->DeviceName) / sizeof(Context->DeviceName[0]));

        if (SndMixerEnumLines(Context->Mixer,
                              PrefDlgAddLine,
                              Context))
        {
            UINT SelBox = 0;

            /* enable/disable controls and make default selection */
            EnableWindow(GetDlgItem(Context->hwndDlg,
                                    IDC_PLAYBACK),
                         Context->PlaybackID != (DWORD)-1);
            CheckDlgButton(Context->hwndDlg,
                           IDC_PLAYBACK,
                           (Context->PlaybackID != (DWORD)-1 && SelBox++ == 0) ?
                               BST_CHECKED : BST_UNCHECKED);

            EnableWindow(GetDlgItem(Context->hwndDlg,
                                    IDC_RECORDING),
                         Context->RecordingID != (DWORD)-1);
            CheckDlgButton(Context->hwndDlg,
                           IDC_RECORDING,
                           (Context->RecordingID != (DWORD)-1 && SelBox++ == 0) ?
                               BST_CHECKED : BST_UNCHECKED);

            if (Context->OtherLines != 0)
            {
                /* select the first item in the other lines combo box by default */
                SendDlgItemMessage(Context->hwndDlg,
                                   IDC_LINE,
                                   CB_SETCURSEL,
                                   0,
                                   0);
            }
            EnableWindow(GetDlgItem(Context->hwndDlg,
                                    IDC_LINE),
                         FALSE);
            EnableWindow(GetDlgItem(Context->hwndDlg,
                                    IDC_OTHER),
                         Context->OtherLines != 0);
            CheckDlgButton(Context->hwndDlg,
                           IDC_LINE,
                           (Context->OtherLines != 0 && SelBox++ == 0) ?
                               BST_CHECKED : BST_UNCHECKED);

            /* disable the OK button if the device doesn't have any lines */
            EnableWindow(GetDlgItem(Context->hwndDlg,
                                    IDOK),
                         Context->PlaybackID != (DWORD)-1 ||
                         Context->RecordingID != (DWORD)-1 ||
                         Context->OtherLines != 0);

            LineID = Context->SelectedLine;
        }
    }

    /* update the line sources list */
    if ((MixerID != OldID && Context->SelectedLine != (DWORD)-1) ||
        (Context->SelectedLine != LineID && LineID != (DWORD)-1))
    {
        Context->SelectedLine = LineID;

        (void)ListView_DeleteAllItems(GetDlgItem(Context->hwndDlg,
                                      IDC_CONTROLS));

        Context->tmp = 0;
        SndMixerEnumConnections(Context->Mixer,
                                LineID,
                                PrefDlgAddConnection,
                                Context);
    }
}

static INT_PTR CALLBACK
DlgPreferencesProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    PPREFERENCES_CONTEXT Context;

    switch (uMsg)
    {
        case WM_COMMAND:
        {
            Context = GetDialogData(hwndDlg,
                                    PREFERENCES_CONTEXT);
            switch (LOWORD(wParam))
            {
                case IDC_MIXERDEVICE:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        UpdatePrefDlgControls(Context,
                                              (DWORD)-1);
                    }
                    break;
                }

                case IDC_LINE:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        INT LineID;
                        INT Index;

                        Index = SendDlgItemMessage(hwndDlg,
                                                   IDC_LINE,
                                                   CB_GETCURSEL,
                                                   0,
                                                   0);
                        if (Index != CB_ERR)
                        {
                            LineID = SendDlgItemMessage(hwndDlg,
                                                        IDC_LINE,
                                                        CB_GETITEMDATA,
                                                        Index,
                                                        0);
                            if (LineID != CB_ERR)
                            {
                                UpdatePrefDlgControls(Context,
                                                      LineID);
                            }
                        }
                    }
                    break;
                }

                case IDC_PLAYBACK:
                {
                    UpdatePrefDlgControls(Context,
                                          Context->PlaybackID);
                    EnableWindow(GetDlgItem(hwndDlg,
                                            IDC_LINE),
                                 FALSE);
                    break;
                }

                case IDC_RECORDING:
                {
                    UpdatePrefDlgControls(Context,
                                          Context->RecordingID);
                    EnableWindow(GetDlgItem(hwndDlg,
                                            IDC_LINE),
                                 FALSE);
                    break;
                }

                case IDC_OTHER:
                {
                    INT LineCbIndex;
                    INT LineID;

                    EnableWindow(GetDlgItem(hwndDlg,
                                            IDC_LINE),
                                 TRUE);

                    LineCbIndex = SendDlgItemMessage(hwndDlg,
                                                     IDC_LINE,
                                                     CB_GETCURSEL,
                                                     0,
                                                     0);
                    if (LineCbIndex != CB_ERR)
                    {
                        LineID = SendDlgItemMessage(hwndDlg,
                                                    IDC_LINE,
                                                    CB_GETITEMDATA,
                                                    LineCbIndex,
                                                    0);
                        if (LineID != CB_ERR)
                        {
                            UpdatePrefDlgControls(Context,
                                                  LineID);
                        }
                    }
                    break;
                }

                case IDOK:
                case IDCANCEL:
                {
                    EndDialog(hwndDlg,
                              LOWORD(wParam));
                    break;
                }
            }
            break;
        }

        case MM_MIXM_LINE_CHANGE:
        {
            DPRINT("MM_MIXM_LINE_CHANGE\n");
            break;
        }

        case MM_MIXM_CONTROL_CHANGE:
        {
            DPRINT("MM_MIXM_CONTROL_CHANGE\n");
            break;
        }

        case WM_INITDIALOG:
        {
            PREFERENCES_FILL_DEVICES FillDevContext;
            LVCOLUMN lvc;
            RECT rcClient;
            HWND hwndControls;

            SetWindowLongPtr(hwndDlg,
                             DWLP_USER,
                             (LONG_PTR)lParam);
            Context = (PPREFERENCES_CONTEXT)((LONG_PTR)lParam);
            Context->hwndDlg = hwndDlg;
            Context->Mixer = SndMixerCreate(hwndDlg);
            Context->Selected = (UINT)-1;

            FillDevContext.PrefContext = Context;
            FillDevContext.hComboBox = GetDlgItem(hwndDlg,
                                                  IDC_MIXERDEVICE);
            FillDevContext.Selected = SndMixerGetSelection(Context->Mixer);
            SndMixerEnumProducts(Context->Mixer,
                                 FillDeviceComboBox,
                                 &FillDevContext);

            /* initialize the list view control */
            hwndControls = GetDlgItem(hwndDlg,
                                      IDC_CONTROLS);
            (void)ListView_SetExtendedListViewStyle(hwndControls,
                                                    LVS_EX_CHECKBOXES);

            GetClientRect(hwndControls,
                          &rcClient);
            lvc.mask = LVCF_TEXT | LVCF_WIDTH;
            lvc.pszText = TEXT("");
            lvc.cx = rcClient.right;
            SendMessage(hwndControls,
                        LVM_INSERTCOLUMN,
                        0,
                        (LPARAM)&lvc);

            /* update all controls */
            UpdatePrefDlgControls(Context,
                                  (DWORD)-1);
            return TRUE;
        }

        case WM_DESTROY:
        {
            Context = GetDialogData(hwndDlg,
                                    PREFERENCES_CONTEXT);
            if (Context->Mixer != NULL)
            {
                SndMixerDestroy(Context->Mixer);
            }
            break;
        }

        case WM_CLOSE:
        {
            EndDialog(hwndDlg,
                      IDCANCEL);
            break;
        }
    }

    return 0;
}

/******************************************************************************/

static VOID
DeleteMixerWindowControls(PMIXER_WINDOW MixerWindow)
{
    UNREFERENCED_PARAMETER(MixerWindow);
}

static BOOL
RebuildMixerWindowControls(PMIXER_WINDOW MixerWindow)
{
    DeleteMixerWindowControls(MixerWindow);

    return TRUE;
}

static LRESULT CALLBACK
MainWindowProc(HWND hwnd,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    PMIXER_WINDOW MixerWindow;
    LRESULT Result = 0;

    switch (uMsg)
    {
        case WM_COMMAND:
        {
            MixerWindow = GetWindowData(hwnd,
                                        MIXER_WINDOW);

            switch (LOWORD(wParam))
            {
                case IDC_PROPERTIES:
                {
                    PREFERENCES_CONTEXT Preferences;

                    Preferences.MixerWindow = MixerWindow;
                    Preferences.Mixer = NULL;

                    if (DialogBoxParam(hAppInstance,
                                       MAKEINTRESOURCE(IDD_PREFERENCES),
                                       hwnd,
                                       DlgPreferencesProc,
                                       (LPARAM)&Preferences) == IDOK)
                    {
                        /* FIXME - update window */
                    }
                    break;
                }

                case IDC_EXIT:
                {
                    PostQuitMessage(0);
                    break;
                }

                case IDC_ABOUT:
                {
                    HICON hAppIcon = (HICON)GetClassLongPtrW(hwnd,
                                                             GCLP_HICON);
                    ShellAbout(hwnd,
                               lpAppTitle,
                               NULL,
                               hAppIcon);
                    break;
                }
            }
            break;
        }

        case MM_MIXM_LINE_CHANGE:
        {
            DPRINT("MM_MIXM_LINE_CHANGE\n");
            break;
        }

        case MM_MIXM_CONTROL_CHANGE:
        {
            DPRINT("MM_MIXM_CONTROL_CHANGE\n");
            break;
        }

        case WM_CREATE:
        {
            MixerWindow = ((LPCREATESTRUCT)lParam)->lpCreateParams;
            SetWindowLongPtr(hwnd,
                             GWL_USERDATA,
                             (LONG_PTR)MixerWindow);
            MixerWindow->hWnd = hwnd;
            MixerWindow->hStatusBar = CreateStatusWindow(WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS,
                                                         NULL,
                                                         hwnd,
                                                         0);
            if (MixerWindow->hStatusBar != NULL)
            {
                MixerWindow->Mixer = SndMixerCreate(MixerWindow->hWnd);
                if (MixerWindow->Mixer != NULL)
                {
                    TCHAR szProduct[MAXPNAMELEN];

                    if (SndMixerGetProductName(MixerWindow->Mixer,
                                               szProduct,
                                               sizeof(szProduct) / sizeof(szProduct[0])) > 0)
                    {
                        SendMessage(MixerWindow->hStatusBar,
                                    WM_SETTEXT,
                                    0,
                                    (LPARAM)szProduct);
                    }

                    if (!RebuildMixerWindowControls(MixerWindow))
                    {
                        DPRINT("Rebuilding mixer window controls failed!\n");
                        SndMixerDestroy(MixerWindow->Mixer);
                        MixerWindow->Mixer = NULL;
                        Result = -1;
                    }
                }
                else
                {
                    Result = -1;
                }
            }
            else
            {
                DPRINT("Failed to create status window!\n");
                Result = -1;
            }
            break;
        }

        case WM_DESTROY:
        {
            MixerWindow = GetWindowData(hwnd,
                                        MIXER_WINDOW);
            if (MixerWindow->Mixer != NULL)
            {
                SndMixerDestroy(MixerWindow->Mixer);
            }
            break;
        }

        case WM_CLOSE:
        {
            PostQuitMessage(0);
            break;
        }

        default:
        {
            Result = DefWindowProc(hwnd,
                                   uMsg,
                                   wParam,
                                   lParam);
            break;
        }
    }

    return Result;
}

static BOOL
RegisterApplicationClasses(VOID)
{
    WNDCLASSEX wc;

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(PMIXER_WINDOW);
    wc.hInstance = hAppInstance;
    wc.hIcon = LoadIcon(hAppInstance,
                        MAKEINTRESOURCE(IDI_MAINAPP));
    wc.hCursor = LoadCursor(NULL,
                            IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = SZ_APP_CLASS;
    wc.hIconSm = NULL;
    MainWindowClass = RegisterClassEx(&wc);

    return MainWindowClass != 0;
}

static VOID
UnregisterApplicationClasses(VOID)
{
    UnregisterClass(SZ_APP_CLASS,
                    hAppInstance);
}

static HWND
CreateApplicationWindow(VOID)
{
    HWND hWnd;

    PMIXER_WINDOW MixerWindow = HeapAlloc(hAppHeap,
                                          0,
                                          sizeof(MIXER_WINDOW));
    if (MixerWindow == NULL)
    {
        return NULL;
    }

    if (mixerGetNumDevs() > 0)
    {
        hWnd = CreateWindowEx(WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT,
                              SZ_APP_CLASS,
                              lpAppTitle,
                              WS_DLGFRAME | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
                              CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                              NULL,
                              LoadMenu(hAppInstance,
                                       MAKEINTRESOURCE(IDM_MAINMENU)),
                              hAppInstance,
                              MixerWindow);
    }
    else
    {
        LPTSTR lpErrMessage;

        /*
         * no mixer devices are available!
         */

        hWnd = NULL;
        if (AllocAndLoadString(&lpErrMessage,
                               hAppInstance,
                               IDS_NOMIXERDEVICES))
        {
            MessageBox(NULL,
                       lpErrMessage,
                       lpAppTitle,
                       MB_ICONINFORMATION);
            LocalFree(lpErrMessage);
        }
    }

    if (hWnd == NULL)
    {
        HeapFree(hAppHeap,
                 0,
                 MixerWindow);
    }

    return hWnd;
}

int WINAPI
_tWinMain(HINSTANCE hInstance,
          HINSTANCE hPrevInstance,
          LPTSTR lpszCmdLine,
          int nCmdShow)
{
    MSG Msg;
    int Ret = 1;

    UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpszCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

    hAppInstance = hInstance;
    hAppHeap = GetProcessHeap();

    if (InitAppConfig())
    {
        /* load the application title */
        if (!AllocAndLoadString(&lpAppTitle,
                                hAppInstance,
                                IDS_SNDVOL32))
        {
            lpAppTitle = NULL;
        }

        InitCommonControls();

        if (RegisterApplicationClasses())
        {
            hMainWnd = CreateApplicationWindow();
            if (hMainWnd != NULL)
            {
                BOOL bRet;
                while ((bRet =GetMessage(&Msg,
                                         NULL,
                                         0,
                                         0)) != 0)
                {
                    if (bRet != -1)
                    {
                        TranslateMessage(&Msg);
                        DispatchMessage(&Msg);
                    }
                }

                DestroyWindow(hMainWnd);
                Ret = 0;
            }
            else
            {
                DPRINT("Failed to create application window (LastError: %d)!\n", GetLastError());
            }

            UnregisterApplicationClasses();
        }
        else
        {
            DPRINT("Failed to register application classes (LastError: %d)!\n", GetLastError());
        }

        if (lpAppTitle != NULL)
        {
            LocalFree(lpAppTitle);
        }

        CloseAppConfig();
    }
    else
    {
        DPRINT("Unable to open the Volume Control registry key!\n");
    }

    return Ret;
}

