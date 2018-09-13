/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    add.c

Abstract:

    This file contains UI elements for adding new programs.

Author:

    Dave Hastings (daveh) creation-date 12-May-1997

Notes:
    bugbug need to adjust dialog contents based on
    hardware availability.

    bugbug what about SendDlgItemMessage?

Revision History:

--*/

#include <windows.h>
#include <ole2.h>
// bugbug
#include <userenv.h>
#include <commctrl.h>
#include <msi.h>
#include <dbt.h>
#include "resource.h"
#include "pip.h"
#include "appmgr.h"

static LPWSTR
AddStartRadioButtonsSelected(
    HWND DialogWindow
    );

static VOID
DisableControls(
    HWND Dialog
    );

static BOOL
MediaSelectRadioButtonsSelected(
    HWND DialogWindow
    );

static VOID
PopulateAddListView(
    HWND ListViewWindow
    );

BOOL
MediaChangeProc(
    HWND Dialog,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT CALLBACK
SubClassProc(
    HWND PropertySheet,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

ULONG SelectedMedia;
ULONG ChangedDriveMap;

WCHAR SetupPath[MAX_PATH];

HWND AddSourceToolTip;

// bugbug leaks!
PACKAGEDISPINFO Products[50];
ULONG ProductCount;

BOOL
AddSourceInitProc(
    HWND DialogWindow,
	UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    This function handles initialization for IDD_ADD_SOURCE.  Currently,
    it just creates the tool tip for the unimplemented functionality.

Arguments:

    DialogWindow - Supplies the handle for the dialog.
    wParam - Supplies the wParam for the WM_INIT message
    lParam - Supplies the lParam for the WM_INIT message
    
Return Value:

    TRUE for success.

--*/
{
    TOOLINFO ToolInfo;
    RECT r;
    POINT p;
	static BOOL ComInitialized;
	HRESULT HResult;
    HWND Control;

	switch (Msg) {
		case WM_INITDIALOG:

#if 0
			//
			// Create the tool tip window
			//
			AddSourceToolTip = CreateWindowEx(
				0,
				TOOLTIPS_CLASS,
				NULL,
				TTS_ALWAYSTIP,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				DialogWindow,
				NULL,
				Instance,
				NULL
				);

			//
			// Attach the unimplemented feature tip to the internet radio button
			//
			UnimplementedFeature(DialogWindow, AddSourceToolTip, IDC_RADIO_INTERNET);
#endif
            if (!(FeatureMask & FEATURE_ADD_CORPNET)) {

			    //
			    // Initialize Com so we can get the published apps
			    //
			    HResult = CoInitialize(NULL);

			    if (SUCCEEDED(HResult)) {
				    ComInitialized = TRUE;
			    } else {
				    // bugbug report error
				    ComInitialized = FALSE;
			    }
            }
			return TRUE;

		case WM_DESTROY:

			if (ComInitialized) {
				CoUninitialize();
			}

			return TRUE;

		default: ;
			// bugbug internal error
	}
    return TRUE;
}

INT
AddSourceNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    )
/*++

Routine Description:

    This function handles the WM_NOTIFY messages for the Welcome page.

Arguments:

    DialogWindow - Supplies the handle of the dialog window.
    wParam - Supplies the wParam for this notify message
    NotifyHeader - Supplies a pointer to the notify header for this message
    PageIndex - Supplies the index of this page in the PageInfo structure

Return Value:

    non-zero if the message was handled.  DWL_MSGRESULT is set to the 
    return value for the message

--*/
{
    DWORD ActiveButtons;
    DWORD RetVal;
    LPWSTR NextPage;
    HRESULT HResult;
    IEnumPackage *IEnumPackage;

    switch (NotifyHeader->code) {
        case PSN_SETACTIVE:

            //
            // Wait until the configuration information is available
            //
            RetVal = WaitForSingleObject(ConfigSyncHandle, 2*60*1000);

            if (RetVal != WAIT_OBJECT_0) {
                //
                // The wait failed.  This is an internal error
                //
                // bugbug
            }

            //
            // Disable controls for unavailable options
            //
            DisableControls(DialogWindow);

            //
            // set the correct wizard buttons
            //
            ActiveButtons = PSWIZB_BACK;

            if (AddStartRadioButtonsSelected(DialogWindow)) {
                //
                // One of the radio buttons is selected
                // so we can display the next button
                //
                ActiveButtons |= PSWIZB_NEXT;
            }

            PostMessage(GetParent(DialogWindow), PSM_SETWIZBUTTONS, 0, ActiveButtons);
            CheckSetWindowLong(DialogWindow, DWL_MSGRESULT, 0);
            return TRUE;

        case PSN_WIZNEXT:
            //
            // Send us to the correct page
            //
            NextPage = AddStartRadioButtonsSelected(DialogWindow);

            // bugbug maybe resid is not the best thing to return
            // from AddStartRadioButtonsSelected
            if (NextPage == MAKEINTRESOURCE(IDD_ADD_PROGRAM)) {
                ULONG i;
                HWND AnimationWindow;
                HWND TextWindow;
                WCHAR OperationText[255];

                //
                // Enable the animation, and give the user some 
                // feedback
                //
                AnimationWindow = GetDlgItem(DialogWindow, IDC_ANIMATE1);

                TextWindow = GetDlgItem(
                    DialogWindow, 
                    IDC_OPERATION_TEXT
                    );

                LoadString(
                    Instance,
                    IDS_OPERATION_APPLIST,
                    OperationText,
                    255);

                ShowWindow(TextWindow, SW_SHOWNOACTIVATE);

                SetWindowText(
                    TextWindow,
                    OperationText
                    );

                UpdateWindow(TextWindow);

                ShowWindow(AnimationWindow, SW_SHOWNOACTIVATE);

                Animate_Open(
                    AnimationWindow, 
                    MAKEINTRESOURCE(IDA_FINDCOMP)
                    );

                Animate_Play(
                    AnimationWindow,
                    0,
                    -1,
                    -1
                    );

				//
				// Get published apps
				//
				ProductCount = 50;

#if 0
                HResult = CoGetPublishedAppInfo(
					APPINFO_PUBLISHED,
					&ProductCount,
					&Products
					);
#endif
                ProductCount = 50;

                HResult = GetPublishedApps(
                    Products,
                    &ProductCount
                    );

                Animate_Stop(
                    AnimationWindow
                    );

				if (SUCCEEDED(HResult)) {
					if (ProductCount == 0) {
						NextPage = MAKEINTRESOURCE(IDD_ADD_BROWSE);
					}
				} else {
					// bugbug error reporting
                    ProductCount = 0;
				}


                ShowWindow(
                    AnimationWindow,
                    SW_HIDE
                    );

                ShowWindow(
                    TextWindow,
                    SW_HIDE
                    );
    
            }

            CheckSetWindowLong(DialogWindow, DWL_MSGRESULT, (ULONG)NextPage);

            return TRUE;

        case PSN_WIZBACK:
            //
            // Send us back to the start page
            //
            CheckSetWindowLong(
                DialogWindow, 
                DWL_MSGRESULT, 
                (ULONG)MAKEINTRESOURCE(IDD_WELCOME)
                );

            return TRUE;

#if DBG
        case PSN_QUERYCANCEL:
            //
            // We shouldn't be here.  Let's complain
            //
            OutputDebugString(L"AppMgr: PSN_QUERYCANCEL handling error\n");

            return FALSE;
#endif

        default:
            return FALSE;
    }
}


static LPWSTR
AddStartRadioButtonsSelected(
    HWND DialogWindow
    )
/*++

Routine Description:

    This routine check the state of the radio buttons to figure out if one of them is selected.

Arguments:

    DialogWindow - Supplies the handle of the dialog window.
    
Return Value:

    TRUE if a radio button is selected

--*/
{
    HWND Control;
    LRESULT ButtonState;

    Control = GetDlgItem(DialogWindow, IDC_RADIO_LOCALMEDIA);

    ButtonState = SendMessage(Control, BM_GETCHECK, 0, 0);

    if (ButtonState == BST_CHECKED) {
        return MAKEINTRESOURCE(IDD_ADD_MEDIASELECT);
    }

    Control = GetDlgItem(DialogWindow, IDC_RADIO_CORPNET);

    ButtonState = SendMessage(Control, BM_GETCHECK, 0, 0);

    if (ButtonState == BST_CHECKED) {
        // BUGBUG
        return MAKEINTRESOURCE(IDD_ADD_PROGRAM);
    }

    Control = GetDlgItem(DialogWindow, IDC_RADIO_INTERNET);

    ButtonState = SendMessage(Control, BM_GETCHECK, 0, 0);

    if (ButtonState == BST_CHECKED) {
        // BUGBUG
        return MAKEINTRESOURCE(IDD_ADD_FINISH_INTERNET);
    }

    return FALSE;
}

INT
AddSourceCommandProc(
    HWND Dialog,
    WPARAM wParam,
    LPARAM lParam,
    UINT Index
    )
/*++

Routine Description:

    This routine handles WM_COMMAND processing for the add program start page.

Arguments:

    Dialog - Supplies the handle of the dialog window.
    wParam - Supplies the wParam for this WM_COMMAND message.
    lParam - Supplies the lParam for this WM_COMMAND message.
    Index - Supplies the index to the information for this page

Return Value:

    see WM_COMMAND documentation.

--*/
{
    switch (LOWORD(wParam)) {
        case IDC_RADIO_LOCALMEDIA:
        case IDC_RADIO_CORPNET:
        case IDC_RADIO_INTERNET:
            SendMessage(
                GetParent(Dialog), 
                PSM_SETWIZBUTTONS, 
                0, 
                PSWIZB_NEXT | PSWIZB_BACK
                );
            return TRUE;

        default:
            return FALSE;
    }
}

INT
AddMediaSelectNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    )
/*++

Routine Description:

    This function handles the WM_NOTIFY messages for the Media select page.

Arguments:

    DialogWindow - Supplies the handle of the dialog window.
    wParam - Supplies the wParam for this notify message
    NotifyHeader - Supplies a pointer to the notify header for this message
    PageIndex - Supplies the index of this page in the PageInfo structure

Return Value:

    non-zero if the message was handled.  DWL_MSGRESULT is set to the 
    return value for the message

--*/
{
    DWORD ActiveButtons;
	BOOL Result;
    LPWSTR NextPage;

    switch (NotifyHeader->code) {
        case PSN_SETACTIVE:
            //
            // set the correct wizard buttons
            //
            ActiveButtons = PSWIZB_BACK;

            if (MediaSelectRadioButtonsSelected(DialogWindow)) {
                //
                // One of the radio buttons is selected
                // so we can display the next button
                //
                ActiveButtons |= PSWIZB_NEXT;
            }

            PostMessage(GetParent(DialogWindow), PSM_SETWIZBUTTONS, 0, ActiveButtons);

            CheckSetWindowLong(DialogWindow, DWL_MSGRESULT, 0);
            return TRUE;

        case PSN_WIZNEXT:
			//
			// Find an install program
			//
			Result = FindInstallProgram(
				SelectedMedia,
				SetupPath,
				MAX_PATH
				);

			if (Result == FALSE) {
				//
				// bugbug error page here
				//
				CheckSetWindowLong(
					DialogWindow,
					DWL_MSGRESULT,
					(ULONG)MAKEINTRESOURCE(IDD_ADD_NOMEDIA)
					);

				return TRUE;
			}


            //
            // Send us to the correct page
            //

            CheckSetWindowLong(
                DialogWindow, 
                DWL_MSGRESULT, 
                (ULONG)MAKEINTRESOURCE(IDD_FINISH_MEDIA)
                );

            return TRUE;

        case PSN_WIZBACK:
            //
            // Send us back to the start page
            //
            CheckSetWindowLong(
                DialogWindow, 
                DWL_MSGRESULT, 
                (ULONG)MAKEINTRESOURCE(IDD_ADD_SOURCE)
                );

            return TRUE;

#if DBG
        case PSN_QUERYCANCEL:
            //
            // We shouldn't be here.  Let's complain
            //
            OutputDebugString(L"AppMgr: PSN_QUERYCANCEL handling error\n");

            return FALSE;
#endif

        default:
            return FALSE;
    }
}

INT
AddMediaSelectCommandProc(
    HWND Dialog,
    WPARAM wParam,
    LPARAM lParam,
    UINT Index
    )
/*++

Routine Description:

    This routine handles WM_COMMAND processing for the add program start page.

Arguments:

    Dialog - Supplies the handle of the dialog window.
    wParam - Supplies the wParam for this WM_COMMAND message.
    lParam - Supplies the lParam for this WM_COMMAND message.
    Index - Supplies the index to the information for this page

Return Value:

    see WM_COMMAND documentation.

--*/
{
    HWND TextWindow;
    WCHAR MediaText[255];
    DWORD MediaTextID;

    switch (LOWORD(wParam)) {
        case IDC_BUTTON_CDROM:
        case IDC_BUTTON_FLOPPY:

            //
            // Get the handle for the static text
            //
            TextWindow = GetDlgItem(Dialog, IDC_MEDIA_TEXT);

            if (LOWORD(wParam) == IDC_BUTTON_CDROM) {
                SelectedMedia = DRIVE_CDROM;
                MediaTextID = IDS_MEDIATEXT_CDROM;
            } else {
                SelectedMedia = DRIVE_REMOVABLE;
                MediaTextID = IDS_MEDIATEXT_FLOPPY;
            }

            SendMessage(
                GetParent(Dialog), 
                PSM_SETWIZBUTTONS, 
                0, 
                PSWIZB_NEXT | PSWIZB_BACK
                );

            //
            // Set the appropriate text
            //
            LoadString(
                Instance,
                MediaTextID,
                MediaText,
                255
                );

            SendMessage(
                TextWindow,
                WM_SETTEXT,
                0,
                (LPARAM)MediaText
                );

            ShowWindow(GetDlgItem(Dialog, IDC_MEDIA_TEXT), SW_SHOWNOACTIVATE);

            return TRUE;

        default:
            return FALSE;
    }
}

static BOOL
MediaSelectRadioButtonsSelected(
    HWND DialogWindow
    )
/*++

Routine Description:

    This routine check the state of the radio buttons to figure out if one of them is selected.

Arguments:

    DialogWindow - Supplies the handle of the dialog window.
    
Return Value:

    TRUE if a radio button is selected

--*/
{
    HWND Control;
    LRESULT ButtonState;

    Control = GetDlgItem(DialogWindow, IDC_BUTTON_CDROM);

    ButtonState = SendMessage(Control, BM_GETCHECK, 0, 0);

    if (ButtonState == BST_CHECKED) {
        return TRUE;
    }

    Control = GetDlgItem(DialogWindow, IDC_BUTTON_FLOPPY);

    ButtonState = SendMessage(Control, BM_GETCHECK, 0, 0);

    if (ButtonState == BST_CHECKED) {
        return TRUE;
    }

    return FALSE;
}

INT
FinishMediaNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    )
/*++

Routine Description:

    This function handles the WM_NOTIFY messages for the Add finish page.

Arguments:

    DialogWindow - Supplies the handle of the dialog window.
    wParam - Supplies the wParam for this notify message
    NotifyHeader - Supplies a pointer to the notify header for this message
    PageIndex - Supplies the index of this page in the PageInfo structure

Return Value:

    non-zero if the message was handled.  DWL_MSGRESULT is set to the 
    return value for the message

--*/
{
    DWORD ActiveButtons;
    LPWSTR NextPage;
	UINT MediaId;
	WCHAR MediaString[127];

    switch (NotifyHeader->code) {
        case PSN_SETACTIVE:
            //
            // set the correct wizard buttons
            //
            ActiveButtons = PSWIZB_BACK | PSWIZB_FINISH;

            PostMessage(GetParent(DialogWindow), PSM_SETWIZBUTTONS, 0, ActiveButtons);

			//
			// Set the correct text
			//
			if (SelectedMedia == DRIVE_CDROM) {
				MediaId = IDS_MEDIA_CDROM;
			} else {
				MediaId = IDS_MEDIA_FLOPPY;
			}

			LoadString(
				Instance,
				MediaId,
				MediaString,
				127
				);

			SendMessage(
				GetDlgItem(DialogWindow, IDC_TEXT_SOURCE),
				WM_SETTEXT,
				0,
				(LPARAM)MediaString
				);

			CheckSetWindowLong(DialogWindow, DWL_MSGRESULT, 0);
            return TRUE;

        case PSN_WIZBACK:
            //
            // Send us back to the start page
            //
            CheckSetWindowLong(
                DialogWindow, 
                DWL_MSGRESULT, 
                (ULONG)MAKEINTRESOURCE(IDD_ADD_MEDIASELECT)
                );

            return TRUE;

		case PSN_WIZFINISH:
			//
			// Start the setup program
			//
			ExecSetup(SetupPath, DialogWindow, Instance);

			CheckSetWindowLong(
				DialogWindow,
				DWL_MSGRESULT,
				0
				);

			return TRUE;
			
#if DBG
        case PSN_QUERYCANCEL:
            //
            // We shouldn't be here.  Let's complain
            //
            OutputDebugString(L"AppMgr: PSN_QUERYCANCEL handling error\n");

            return FALSE;
#endif

        default:
            return FALSE;
    }
}

static VOID
DisableControls(
    HWND Dialog
    )

/*++

Routine Description:

    This routine disables the raido buttons for the unavailable
    options.

Arguments:

    None.

Return Value:

    None.

--*/
{
    BOOL WindowState;

    if (FeatureMask & FEATURE_ADD_MEDIA) {
        WindowState = FALSE;
    } else {
        if (
            (AppMgrConfig.Floppy == Available) || 
            (AppMgrConfig.CdRom == Available)
        ) {
            WindowState = TRUE;
        } else {
            WindowState = FALSE;
        }
    }

    EnableWindow(GetDlgItem(Dialog, IDC_RADIO_LOCALMEDIA), WindowState);

    if (FeatureMask & FEATURE_ADD_CORPNET) {
        WindowState = FALSE;
    } else {
        WindowState = TRUE;
    }
    
    EnableWindow(GetDlgItem(Dialog, IDC_RADIO_CORPNET), WindowState);
    
    if (FeatureMask & FEATURE_ADD_INTERNET) {
        WindowState = FALSE;
    } else {
        WindowState = TRUE;
    }
    EnableWindow(GetDlgItem(Dialog, IDC_RADIO_INTERNET), WindowState);
}

INT
AddNoMediaNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    )
/*++

Routine Description:

    This function handles the WM_NOTIFY messages for the Media error page.

Arguments:

    DialogWindow - Supplies the handle of the dialog window.
    wParam - Supplies the wParam for this notify message
    NotifyHeader - Supplies a pointer to the notify header for this message
    PageIndex - Supplies the index of this page in the PageInfo structure

Return Value:

    non-zero if the message was handled.  DWL_MSGRESULT is set to the 
    return value for the message

--*/
{
    DWORD ActiveButtons;
	BOOL Result;
    LPWSTR NextPage;
	UINT MediaID;
	WCHAR MediaString[255];
    HWND TextWindow;
    

    switch (NotifyHeader->code) {
        case PSN_SETACTIVE:
            //
            // set the correct wizard buttons
            // We start out with the Next button enabled,
            // because the user may just have to insert media
            // in the drive, and we don't necessarily have any
            // way of detecting that they have
            //
            ActiveButtons = PSWIZB_BACK | PSWIZB_NEXT;

            PostMessage(GetParent(DialogWindow), PSM_SETWIZBUTTONS, 0, ActiveButtons);

            CheckSetWindowLong(DialogWindow, DWL_MSGRESULT, 0);

            //
            // Set the appropriate text
            //
            TextWindow = GetDlgItem(DialogWindow, IDC_NOMEDIA_TEXT);

            if (SelectedMedia == DRIVE_CDROM) {
                MediaID = IDS_MEDIAERROR_CD;
            } else {
                MediaID = IDS_MEDIAERROR_FLOPPY;
            }

            LoadString(
                Instance,
                MediaID,
                MediaString,
                255
                );

            SendMessage(
                TextWindow,
                WM_SETTEXT,
                0,
                (LPARAM)MediaString
                );
            
            return TRUE;

        case PSN_WIZNEXT:

            if (SetupPath[0] == L'\0') {
			//
			// Find an install program
			//
			    Result = FindInstallProgram(
				    SelectedMedia,
				    SetupPath,
				    MAX_PATH
				    );

			    if (Result == FALSE) {
                    WCHAR ErrorString[255];

                    //
                    // We need to display an error box to allow the user to try again
                    //
                    LoadString(
                        Instance,
                        IDS_BROWSE_STILLNOMEDIA,
                        ErrorString,
                        255
                        );

                    MessageBox(
                        DialogWindow,
                        ErrorString,
                        L"Application Manager",
                        MB_OK | MB_ICONERROR
                        );

                    CheckSetWindowLong(
                        DialogWindow,
                        DWL_MSGRESULT,
                        -1
                        );
                    return TRUE;
                }
            }

            CheckSetWindowLong(
                DialogWindow, 
                DWL_MSGRESULT, 
                (ULONG)MAKEINTRESOURCE(IDD_FINISH_MEDIA)
                );

            return TRUE;

        case PSN_WIZBACK:
            //
            // Send us back to the start page
            //
            CheckSetWindowLong(
                DialogWindow, 
                DWL_MSGRESULT, 
                (ULONG)MAKEINTRESOURCE(IDD_ADD_MEDIASELECT)
                );

            return TRUE;

#if DBG
        case PSN_QUERYCANCEL:
            //
            // We shouldn't be here.  Let's complain
            //
            OutputDebugString(L"AppMgr: PSN_QUERYCANCEL handling error\n");

            return FALSE;
#endif

        default:
            return FALSE;
    }
}


INT
AddNoMediaCommandProc(
    HWND Dialog,
    WPARAM wParam,
    LPARAM lParam,
    UINT Index
    )
/*++

Routine Description:

    This routine handles WM_COMMAND processing for the media error page.

Arguments:

    Dialog - Supplies the handle of the dialog window.
    wParam - Supplies the wParam for this WM_COMMAND message.
    lParam - Supplies the lParam for this WM_COMMAND message.
    Index - Supplies the index to the information for this page

Return Value:

    see WM_COMMAND documentation.

--*/
{
    BOOL Result;

    switch (LOWORD(wParam)) {
        case IDC_MEDIA_BROWSE:
            //
            // Put up a dialog to allow the user to select an install program
            //
            Result = BrowseLocalMedia(
                SelectedMedia,
                SetupPath,
                Dialog
                );

            return TRUE;

        default:
            return FALSE;
    }
}

// bugbug
WCHAR BrowseSetupPath[MAX_PATH];

INT AddBrowseNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    )
/*++

Routine Description:

    This function handles the WM_NOTIFY messages for the IDD_ADD_BROWSE page.

Arguments:

    DialogWindow - Supplies the handle of the dialog window.
    wParam - Supplies the wParam for this notify message
    NotifyHeader - Supplies a pointer to the notify header for this message
    PageIndex - Supplies the index of this page in the PageInfo structure

Return Value:

    non-zero if the message was handled.  DWL_MSGRESULT is set to the 
    return value for the message

--*/
{
    DWORD ActiveButtons;
    INT TextLength;
    HWND EditWindow;
    WCHAR PathString[MAX_PATH];

    switch (NotifyHeader->code) {

        case PSN_SETACTIVE:
            //
            // set the appropriate wizard buttons
            //
            ActiveButtons = PSWIZB_BACK;

            //
            // Check to see if we have a setup path
            //
            EditWindow = GetDlgItem(DialogWindow, IDC_SETUPPATH);
            TextLength = GetWindowText(EditWindow, PathString, MAX_PATH);

            if (TextLength != 0) {
                //
                // We do, so allow the user to go on to the next page.
                // The path will be verified when the next button is
                // pressed
                //
                ActiveButtons |= PSWIZB_NEXT;
            }

            PostMessage(GetParent(DialogWindow), PSM_SETWIZBUTTONS, 0, ActiveButtons);

            CheckSetWindowLong(DialogWindow, DWL_MSGRESULT, 0);
            return TRUE;

        case PSN_WIZNEXT:
            //
            // If the file exists, go on to the next page
            //
            if (VerifySetupName(BrowseSetupPath)) {
                CheckSetWindowLong(
                    DialogWindow, 
                    DWL_MSGRESULT,
                    (LONG)MAKEINTRESOURCE(IDD_ADD_FINISH_BROWSE)
                    );
            } else {
                WCHAR ErrorString[255];

                //
                // We need to display an error box to allow the user to try again
                //
                LoadString(
                    Instance,
                    IDS_BROWSE_INVALIDNAME,
                    ErrorString,
                    255
                    );

                MessageBox(
                    DialogWindow,
                    ErrorString,
                    L"Application Manager",
                    MB_OK | MB_ICONERROR
                    );

                CheckSetWindowLong(
                    DialogWindow,
                    DWL_MSGRESULT,
                    -1
                    );
            }

            return TRUE;


        case PSN_WIZBACK:
            //
            // Send us back to the start page
            //
            CheckSetWindowLong(
                DialogWindow, 
                DWL_MSGRESULT, 
                (ULONG)MAKEINTRESOURCE(IDD_ADD_SOURCE)
                );

            return TRUE;

#if DBG
        case PSN_QUERYCANCEL:
            //
            // We shouldn't be here.  Let's complain
            //
            OutputDebugString(L"AppMgr: PSN_QUERYCANCEL handling error\n");

            return FALSE;
#endif

        default:

            return FALSE;
    }
}

INT
AddBrowseCommandProc(
    HWND Dialog,
    WPARAM wParam,
    LPARAM lParam,
    UINT PageIndex
    )
/*++

Routine Description:

    This routine handles WM_COMMAND processing for the IDD_ADD_BROWSE page.

Arguments:

    Dialog - Supplies the handle of the dialog window.
    wParam - Supplies the wParam for this WM_COMMAND message.
    lParam - Supplies the lParam for this WM_COMMAND message.
    Index - Supplies the index to the information for this page

Return Value:

    see WM_COMMAND documentation.

--*/
{
    ULONG TextLength;
    DWORD ActiveButtons;
    OPENFILENAME FileNameInfo;

    switch (LOWORD(wParam)) {
        case IDC_SETUPPATH:

            switch (HIWORD(wParam)) {

                case EN_CHANGE:
                    //
                    // The user has typed in the edit control.  Grab the text and
                    // stuff it into our local buffer (for later use).  If there is
                    // some text, Enable the next button.  If not, disable it.
                    //
                    if (LOWORD(wParam) == IDC_SETUPPATH) {

                        TextLength = GetWindowText((HWND)lParam, BrowseSetupPath, MAX_PATH);
                        ActiveButtons = PSWIZB_BACK;

                        if (TextLength != 0) {
                            ActiveButtons |= PSWIZB_NEXT;
                        }

                        PostMessage(GetParent(Dialog), PSM_SETWIZBUTTONS, 0, ActiveButtons);

                    }

                    return 0;

                default:

                    return 1;
            }

        case IDC_ADDBROWSE_BROWSE:

            //
            // BUGBUG this should be revisited.  Can we import
            // the text from the edit window successfully?
            //
            SetWindowText(GetDlgItem(Dialog, IDC_SETUPPATH), L"");

            BrowseSetupPath[0] = L'\0';

            //
            // The user wants to browse, so put up a common file dialog with
            // the path that's currently in the edit window
            //
            FileNameInfo.lStructSize = sizeof(OPENFILENAME);
            FileNameInfo.hwndOwner = Dialog;
            FileNameInfo.hInstance = Instance;
            FileNameInfo.lpstrFilter = NULL; // bugbug
            FileNameInfo.lpstrCustomFilter = NULL; 
            FileNameInfo.nMaxCustFilter = 0;
            FileNameInfo.nFilterIndex = 0;
            FileNameInfo.lpstrFile = BrowseSetupPath;
            FileNameInfo.nMaxFile = MAX_PATH;
            FileNameInfo.lpstrFileTitle = NULL;
            FileNameInfo.nMaxFileTitle = 0;
            // bugbug the following causes drive not ready if someone
            // click browse without media in the drive
            // FileNameInfo.lpstrInitialDir = CurrentDrive;
            FileNameInfo.lpstrInitialDir = NULL;
            FileNameInfo.lpstrTitle = NULL; //bugbug
            FileNameInfo.Flags = OFN_FILEMUSTEXIST | OFN_LONGNAMES;
            FileNameInfo.nFileOffset = 0;
            FileNameInfo.nFileExtension = 0;
            FileNameInfo.lpstrDefExt = NULL; // bugbug
            FileNameInfo.lCustData = 0;
            FileNameInfo.lpfnHook = NULL;
            FileNameInfo.lpTemplateName = NULL;

            if (GetOpenFileName(&FileNameInfo)) {

                SetWindowText(
                    GetDlgItem(Dialog, IDC_SETUPPATH), 
                    BrowseSetupPath
                    );
                
                PostMessage(
                    GetParent(Dialog), 
                    PSM_SETWIZBUTTONS, 
                    0, 
                    PSWIZB_NEXT | PSWIZB_BACK
                    );
            }
            
            return 0;

        default:

            return 1;
    }
}

INT AddFinishBrowseNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    )
/*++

Routine Description:

    This function handles the WM_NOTIFY messages for the IDD_ADD_FINISH_BROWSE page.

Arguments:

    DialogWindow - Supplies the handle of the dialog window.
    wParam - Supplies the wParam for this notify message
    NotifyHeader - Supplies a pointer to the notify header for this message
    PageIndex - Supplies the index of this page in the PageInfo structure

Return Value:

    non-zero if the message was handled.  DWL_MSGRESULT is set to the 
    return value for the message

--*/
{
    switch (NotifyHeader->code) {

        case PSN_SETACTIVE:
            //
            // Set the appropriate wizard buttons
            //
            PostMessage(
                GetParent(DialogWindow), 
                PSM_SETWIZBUTTONS, 
                0, 
                PSWIZB_BACK | PSWIZB_FINISH
                );

            //
            // Put the setup app name on the dialog
            //
            SetWindowText(GetDlgItem(DialogWindow, IDC_TEXT_SOURCE), BrowseSetupPath);

            return TRUE;

        case PSN_WIZFINISH:
            //
            // Start the setup program
            //
			ExecSetup(BrowseSetupPath, DialogWindow, Instance);

			CheckSetWindowLong(
				DialogWindow,
				DWL_MSGRESULT,
				0
				);

			return TRUE;

#if DBG
        case PSN_QUERYCANCEL:
            //
            // We shouldn't be here.  Let's complain
            //
            OutputDebugString(L"AppMgr: PSN_QUERYCANCEL handling error\n");

            return FALSE;
#endif

        default:

            return FALSE;
    }
}

BOOL
AddProgramInitProc(
    HWND DialogWindow,
	UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    This function handles initialization for IDD_ADD_PROGRAM.

Arguments:

    DialogWindow - Supplies the handle for the dialog.
    wParam - Supplies the wParam for the WM_INIT message
    lParam - Supplies the lParam for the WM_INIT message
    
Return Value:

    TRUE for success.

--*/
{
	    HWND ListViewWindow;
    LV_COLUMN Column;
    LV_ITEM Item;
    TCHAR Buffer[256];

	switch (Msg) {
		case WM_INITDIALOG:

			ListViewWindow = GetDlgItem(DialogWindow, IDC_APPLICATION_LIST);

			//
			// Set up the columns
			//
			Column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
			Column.cx = 300;
			LoadString(Instance, IDS_COLUMN_NAME, Buffer, 256);
			Column.pszText = Buffer;
			Column.iSubItem = 0;
			// bugbug return value
			ListView_InsertColumn(ListViewWindow, 0, &Column);

			Column.mask = LVCF_TEXT | LVCF_WIDTH;
			Column.cx = 100;
			LoadString(Instance, IDS_COLUMN_VERSION, Buffer, 256);
			Column.pszText = Buffer;
			Column.iSubItem = 1;
			// bugbug return value
			ListView_InsertColumn(ListViewWindow, 1, &Column);


			PopulateAddListView(ListViewWindow);

            PageInfo[GetWindowLong(DialogWindow, DWL_USER)].Dialog = DialogWindow;

			return TRUE;

		default:
			return TRUE;
	}
}

static VOID
PopulateAddListView(
    HWND ListViewWindow
    )
/*++

Routine Description:

    This routine enumerates the installed applications on the 
    machine, and populates the list view with the information.

Arguments:

    ListViewWindow - Supplies the handle of the listview to populate.
    
Return Value:

    None.

--*/
{
    LONG RetVal;
    DWORD Capabilities;
    LV_ITEM Item;
    LONG Index;
	ULONG i;

	for (i = 0; i < ProductCount; i++) {

        //
        // Insert the product in the list
        //
        Item.mask = LVIF_TEXT | LVIF_PARAM;
        Item.iItem = 0;
        Item.iSubItem = 0;
        Item.pszText = Products[i].pszPackageName;
        Item.lParam = i;
        Index = ListView_InsertItem(ListViewWindow, &Item);

#if 0
            //
            // Insert the version number
            //
            if (Index == -1) {
                //
                // Didn't insert the item bugbug report error?
                //
                continue;
            }
            Item.mask = LVIF_TEXT;
            Item.iItem = Index;
            Item.iSubItem = 1;
            Item.pszText = VersionString;
            ListView_SetItem(ListViewWindow, &Item);
#endif
    } 
}


BOOL AppsInstalled = FALSE;

INT AddProgramNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    )
/*++

Routine Description:

    This function handles the WM_NOTIFY messages for the IDD_ADD_FINISH_BROWSE page.

Arguments:

    DialogWindow - Supplies the handle of the dialog window.
    wParam - Supplies the wParam for this notify message
    NotifyHeader - Supplies a pointer to the notify header for this message
    PageIndex - Supplies the index of this page in the PageInfo structure

Return Value:

    non-zero if the message was handled.  DWL_MSGRESULT is set to the 
    return value for the message

--*/
{
	DWORD ActiveButtons;

    switch (NotifyHeader->code) {

        case PSN_SETACTIVE:
            //
            // set the appropriate wizard buttons
            //
            ActiveButtons = PSWIZB_BACK;

            if (AppsInstalled) {
                //
                // We do, so allow the user to go on to the next page.
                // The path will be verified when the next button is
                // pressed
                //
                ActiveButtons |= PSWIZB_NEXT;
            }

            PostMessage(GetParent(DialogWindow), PSM_SETWIZBUTTONS, 0, ActiveButtons);

            CheckSetWindowLong(DialogWindow, DWL_MSGRESULT, 0);
            return TRUE;

        case PSN_WIZNEXT:
           
            CheckSetWindowLong(
                DialogWindow, 
                DWL_MSGRESULT, 
                (ULONG)MAKEINTRESOURCE(IDD_ADD_FINISH_CORP)
                );

            return TRUE;


        case PSN_WIZBACK:
            //
            // Send us back to the start page
            //
            CheckSetWindowLong(
                DialogWindow, 
                DWL_MSGRESULT, 
                (ULONG)MAKEINTRESOURCE(IDD_ADD_SOURCE)
                );

            return TRUE;

#if DBG
        case PSN_QUERYCANCEL:
            //
            // We shouldn't be here.  Let's complain
            //
            OutputDebugString(L"AppMgr: PSN_QUERYCANCEL handling error\n");

            return FALSE;
#endif

        default:

            return FALSE;
    }
}

INT
AddProgramCommandProc(
    HWND Dialog,
    WPARAM wParam,
    LPARAM lParam,
    UINT PageIndex
    )
/*++

Routine Description:

    This routine handles WM_COMMAND processing for the IDD_ADD_BROWSE page.

Arguments:

    Dialog - Supplies the handle of the dialog window.
    wParam - Supplies the wParam for this WM_COMMAND message.
    lParam - Supplies the lParam for this WM_COMMAND message.
    Index - Supplies the index to the information for this page

Return Value:

    see WM_COMMAND documentation.

--*/
{
	UINT i;
	ULONG Index;
    UINT ItemCount;
    HWND ListView;
	UINT RetVal;
    LVITEM Item;
    WCHAR ProductCode[39];
    LANGID LangId;
    DWORD Version;
    WCHAR Name[255];
    DWORD NameLength;
    WCHAR Package[255];
    DWORD PackageLength;
    HRESULT HResult;
    uCLSSPEC ClassSpec;
    // bugbug this may be bad
    WCHAR InstalledString[256];
    WCHAR ResString[255];
    HCURSOR OriginalCursor;

    switch (LOWORD(wParam)) {

		case IDC_BUTTON_ADD:

			ListView = GetDlgItem(Dialog, IDC_APPLICATION_LIST);
			ItemCount = ListView_GetItemCount(ListView);

			Index = 0xFFFFFFFF;

			//
			// If any of the items are selected, we can show the next button
			//
			for (i = 0; i < ItemCount; i++) {
				if(ListView_GetItemState(ListView, i, LVIS_SELECTED)) {
					Index = i;
				}
			}

			if (Index == 0xFFFFFFFF) {
				return FALSE;
			}

			//
			// Get the package name for the desired package
			//
            Item.mask = LVIF_PARAM;
            Item.iItem = Index;
            Item.iSubItem = 0;
            ListView_GetItem(
                ListView,
                &Item
                );

            OriginalCursor = SetCursor(
                LoadCursor(
                    NULL,
                    IDC_WAIT
                    )
                );

            ClassSpec.tyspec = TYSPEC_PACKAGENAME;
            ClassSpec.tagged_union.ByName.pPackageName = Products[Item.lParam].pszPackageName;

            HResult = CoGetClassInfo(
                &ClassSpec,
                NULL
                );

            if (SUCCEEDED(HResult)) {

                LoadString(
                    Instance,
                    IDS_INSTALLSUCCESS,
                    ResString,
                    255
                    );
                    
                wsprintf(
                    InstalledString, 
                    ResString, 
                    Products[Item.lParam].pszPackageName
                    );

                MessageBox(
                    Dialog,
                    InstalledString,
                    L"Programs Wizard",
                    MB_OK | MB_ICONINFORMATION | MB_APPLMODAL
                    );
            } else {

                LoadString(
                    Instance,
                    IDS_INSTALLFAIL,
                    ResString,
                    255
                    );
                    
                wsprintf(
                    InstalledString, 
                    ResString, 
                    Products[Item.lParam].pszPackageName
                    );

                MessageBox(
                    Dialog,
                    InstalledString,
                    L"Programs Wizard",
                    MB_OK | MB_ICONINFORMATION | MB_APPLMODAL
                    );
            }

            SetCursor(OriginalCursor);

			AppsInstalled = TRUE;

            PostMessage(
				GetParent(Dialog), 
				PSM_SETWIZBUTTONS, 
				0, 
				PSWIZB_BACK | PSWIZB_NEXT
				);

            PostUpdateMessage();

			return TRUE;

        case IDC_BUTTON_FIND:

            //
            // Bring up the application query 
            //
            FindApplication(GetParent(Dialog));

            return TRUE;

		case IDC_BUTTON_BROWSE:

			//
			// Go to the browse path (bugbug -- the back button will do
			// something unexpected)
			//
            PropSheet_SetCurSelByID(GetParent(Dialog), IDD_ADD_BROWSE);

			return TRUE;

		default:
			return FALSE;

	}
}
