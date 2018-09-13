/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module contains some utility functions.

Author:

    Dave Hastings (daveh) creation-date 03-Jun-1997

Revision History:

--*/

#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>
#include "pip.h"
#include "appmgr.h"
#include "resource.h"

BOOL
ScreenRectToClientRect(
    PRECT Rect,
    HWND Window
    )
/*++

Routine Description:

    This routine takes a rectangle in screen coordinates and converts
    it to a rectangle relative to the specified window.

Arguments:

    Rect - Supplies and returns the rectangle.
    Window - Supplies the window.
    
Return Value:

    TRUE for success

--*/
{
    POINT p;
    RECT r;
    BOOL Result;
    
    r = *Rect;

    //
    // Convert to client coordinates
    //
    p.x = r.left;
    p.y = r.top;

    Result = ScreenToClient(Window, &p);
    if (!Result) {
        return FALSE;
    }

    r.left = p.x;
    r.top = p.y;

    p.x = r.right;
    p.y = r.bottom;

    Result = ScreenToClient(Window, &p);
    if (!Result) {
        return FALSE;
    }

    r.right = p.x;
    r.bottom = p.y;
        
    *Rect = r;
}

VOID
UnimplementedFeature(
    HWND Dialog,
    HWND ToolTip,
    UINT ControlId
    )
/*++

Routine Description:

    This routine attaches the unimplemented feature tool tip
    to the specified control.

Arguments:

    Dialog - Supplies the dialog window.
    ToolTip - Supplies the tool tip window.
    ControlId - Supplies the resource ID of the control.

Return Value:

    TRUE for success

--*/
{
    RECT r;
    TOOLINFO ToolInfo;

    //
    // Get the rectangle for the internet radio button and convert
    // to dialog relative coordinates
    //
    GetWindowRect(GetDlgItem(Dialog, ControlId), &r);

    ScreenRectToClientRect(&r, Dialog);

    //
    // Connect the tool tip to the internet radio button
    //

    ToolInfo.cbSize = sizeof(TOOLINFO);
    ToolInfo.uFlags = TTF_SUBCLASS;
    ToolInfo.hwnd = Dialog;
    ToolInfo.uId = ControlId;
    ToolInfo.rect = r;
    ToolInfo.hinst = Instance;
    ToolInfo.lpszText = MAKEINTRESOURCE(IDS_NOT_IMPLEMENTED);

    SendMessage(
        ToolTip,
        TTM_ADDTOOL,
        0,
        (LPARAM)&ToolInfo
        );
}

BOOL
VerifySetupName(
    PWCHAR FileName
    )
/*++

Routine Description:

    This routine verifies that the specified file exists.
    BUGBUG We should probably verify that it could actually
    be an install program?

Arguments:

    FileName - Supplies the fully qualified name of the file to verify.

Return Value:

    TRUE if the file exists

--*/
{
    HANDLE FileHandle;

    FileHandle = CreateFile(
        FileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if (FileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(FileHandle);
        return TRUE;
    }

    // bugbug we may want to consider filtering error codes here.
    // look at what comdlg did for ofn
    return FALSE;
}
#if 0
PWCHAR
StrStr(
	PWCHAR Source,
	PWCHAR SearchString
	)
/*++

Routine Description:

    This routine determines if the specified string is a sub-string
	of the source string.

Arguments:

    Source - Supplies the string to search in.
    SearchString - Supplies the string to search for.
    
Return Value:

    Pointer to the first occurance of the specified string

--*/
{
	PWCHAR Match;
	PWCHAR TempSource, TempSearch;
	
	//
	// If the string to search for is longer than the string to
	// search in, we won't find a match
	//
	if (lstrlen(SearchString) > (lstrlen(Source)) {
		return NULL;
	}

	Match = Source;
	while (*Match) {
		TempSource = Match;
		TempSearch = SearchString;

		while (*TempSearch) {
			if (*TempSource != *TempSearch) {
				Match = TempSource + 1;
				break;
			}
			TempSource += 1;
			TempSearch += 1;
		}

		if (!(*TempSearch)) {
			return Match;
		}
	}

	return NULL;
}

#endif

VOID
CreateApplicationListViewColumns(
	HWND ListViewWindow
	)
/*++

Routine Description:

    This routine creates the columns for the standar application
	list view in the programs wizard.

Arguments:

	ListViewWindow -- Supplies the handle of the list view window.

Return Value:

    None.

--*/
{
    TCHAR Buffer[256];
    LV_COLUMN Column;

    Column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    Column.cx = 293;
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
}

VOID
PopulateApplicationListView(
    HWND ListViewWindow,
    DWORD DesiredCapabilities
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
    HANDLE Pip;
    PIPAPPLICATIONINFORMATION AppInfo;
    PIPACTIONS Actions;
    WCHAR ProductName[256];
    WCHAR VersionString[256];
    LONG RetVal;
    DWORD Capabilities;
    LV_ITEM Item;
    LONG Index;
    PAPPLICATIONDESCRIPTOR Descriptor;

    //
    // Set up the columns
    //

    Pip = PipInitialize();

    if (Pip == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        //
        // Enumerate the applications
        //
        AppInfo.ProductName = ProductName;
        AppInfo.ProductNameSize = 256;
        AppInfo.InstalledVersion = VersionString;
        AppInfo.InstalledVersionSize = 256;

        RetVal = PipFindNextProduct(
            Pip,
            &Capabilities,
            &Actions,
            &AppInfo
            );

        if (
            (RetVal == ERROR_SUCCESS) && 
            (Capabilities & DesiredCapabilities)
        ) {
            //
            // Save the relavent information
            //
            Descriptor = HeapAlloc(
                GetProcessHeap(),
                0,
                sizeof(APPLICATIONDESCRIPTOR)
                );
            
            Descriptor->Actions = Actions;
            Descriptor->Identifier = AppInfo.h;
            Descriptor->Capabilities = Capabilities;

            //
            // Insert the product in the list
            //
            Item.mask = LVIF_TEXT | LVIF_PARAM;
            Item.iItem = 0;
            Item.iSubItem = 0;
            Item.pszText = ProductName;
            Item.lParam = (LPARAM)Descriptor;
            Index = ListView_InsertItem(ListViewWindow, &Item);

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
        }
        //
        // Until there aren't any more
        //
    } while (RetVal == ERROR_SUCCESS);
    
    PipUninitialize(Pip);

    // bugbug Yick!! duplicated code!!
    Pip = PipDarwinInitialize();

    if (Pip == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        //
        // Enumerate the applications
        //
        AppInfo.ProductName = ProductName;
        AppInfo.ProductNameSize = 256;
        AppInfo.InstalledVersion = VersionString;
        AppInfo.InstalledVersionSize = 256;

        RetVal = PipDarwinFindNextProduct(
            Pip,
            &Capabilities,
            &Actions,
            &AppInfo
            );

        if (
            (RetVal == ERROR_SUCCESS) && 
            (Capabilities & DesiredCapabilities)
        ) {
            //
            // Save the relavent information
            //
            Descriptor = HeapAlloc(
                GetProcessHeap(),
                0,
                sizeof(APPLICATIONDESCRIPTOR)
                );
            
            Descriptor->Actions = Actions;
            Descriptor->Identifier = AppInfo.h;    
            Descriptor->Capabilities = Capabilities;

            //
            // Insert the product in the list
            //
            Item.mask = LVIF_TEXT | LVIF_PARAM;
            Item.iItem = 0;
            Item.iSubItem = 0;
            Item.pszText = ProductName;
            Item.lParam = (LPARAM)Descriptor;
            Index = ListView_InsertItem(ListViewWindow, &Item);

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
        }
        //
        // Until there aren't any more
        //
    } while (RetVal == ERROR_SUCCESS);
    
    PipDarwinUninitialize(Pip);
}

BOOL
HandleQueryCancel(
    HWND Dialog,              
    UINT NextPageID
    )
/*++

Routine Description:

    This function is a generic handler for PSN_QUERYCANCEL.
    Nearly every page has a cancel button and in nearly every
    case, we set the DWL_MSGRESULT to prevent the cancel, and 
    change to the appropriate cancel page.

Arguments:

    Dialog - Supplies the handle for the window for current page.
    NextPageID -- Supplies the resource ID for the cancel page.
    .

Return Value:

    TRUE if handled, FALSE otherwise

--*/
{
    BOOL Success;

    //
    // bugbug error reporting.  Also, what if the SetWindowLong fails?
    //
    Success = PropSheet_SetCurSelByID(GetParent(Dialog), NextPageID);

    CheckSetWindowLong(Dialog, DWL_MSGRESULT, TRUE);

    return Success;
}

VOID
DisplayError(
    DWORD ErrorCode
    )
/*++

Routine Description:

    This function displays an error box, using format message.

Arguments:

    ErrorCode - Supplies the win32 error code.

Return Value:

    None.

--*/
{
    PWCHAR Message;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        ErrorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)(&Message),
        0,
        NULL
        );

    MessageBox(NULL, Message, L"", MB_OK | MB_ICONINFORMATION);

    LocalFree(Message);

    return;
}

LONG
CheckSetWindowLong(
    HWND hWnd,
    INT nIndex,
    LONG dwNewLong
    )
/*++

Routine Description:

    This function checks that the SetWindowLong succeeded.  There are a
    large number of SetWindowLongCalls, so it made sense to create a function
    rather than putting them all inline.

Arguments:

    See SetWindowLong documentation

Return Value:

    See SetWindowLong documentation

--*/
{
    // bugbug probably should be debug only?
    
    LONG RetVal;
    DWORD Error;

    SetLastError(0);

    RetVal = SetWindowLong(hWnd, nIndex, dwNewLong);

    //
    // Check to see if we have an error, and put up an error box if so
    //
    if (RetVal == 0) {
        Error = GetLastError();
        if (Error != ERROR_SUCCESS) {
            DisplayError(Error);
        }
    }

    return RetVal;
}

BOOL
ItemSelected(
    HWND DialogWindow,
    PULONG Index
    )
/*++

Routine Description:

    This routine checks to see if any of the items in the list view are selected.
    BUGBUG this is probably used in several spots

Arguments:

    DialogWindow - Supplies the handle of the dialog the list view belongs to

Return Value:

    TRUE if an item is selected.  False otherwise.

--*/
{
    UINT i;
    UINT ItemCount;
    HWND ListView;

    ListView = GetDlgItem(DialogWindow, IDC_APPLICATION_LIST);
    ItemCount = ListView_GetItemCount(ListView);

    //
    // If any of the items are selected, we can show the next button
    //
    for (i = 0; i < ItemCount; i++) {
        if(ListView_GetItemState(ListView, i, LVIS_SELECTED)) {
            *Index = i;
            return TRUE;
        }
    }

    *Index = 0xFFFFFFFF;
    return FALSE;
}

BOOL
SetUpFonts(
    HINSTANCE Instance,
    HWND Window,
    HFONT *LargeFont,
    HFONT *SmallFont
    )
/*++

Routine Description:

    This routine creates the bold logfonts for the exterior pages of the
    wizard.

Arguments:

    Instance -- Supplies the instance handle
    Window -- Supplies the control panel window handle
    LargeFont -- Returns the logfont for the large bold font
    SmallFont -- Returns the logfont for the small bold font

Return Value:

    TRUE for Sucess

--*/
{
    NONCLIENTMETRICS NonClientMetrics;
    LOGFONT Large, Small;
    BOOL Success;
    ULONG FontNameSize;
    WCHAR FontSizeString[24];
    ULONG FontSizeSize;
    LONG FontSize;
    HDC DC;

    NonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS);

    Success = SystemParametersInfo(
        SPI_GETNONCLIENTMETRICS,
        sizeof(NONCLIENTMETRICS),
        &NonClientMetrics,
        FALSE
        );

    if (!Success) {
        return FALSE;
    }

    //
    // We want both fonts bold
    //
    NonClientMetrics.lfMessageFont.lfWeight = FW_BOLD;

    //
    // Initialize the LOGFONT structures
    //
    Large = NonClientMetrics.lfMessageFont;
    Small = NonClientMetrics.lfMessageFont;
    
    //
    // Get the fontname and size for the large font
    //
    FontNameSize = LoadString(
        Instance,
        IDS_LARGEFONTNAME,
        Large.lfFaceName,
        LF_FACESIZE
        );

    if (FontNameSize == 0) {
        return FALSE;
    }

    FontSizeSize = LoadString(
        Instance,
        IDS_LARGEFONTSIZE,
        FontSizeString,
        24
        );

    if (FontSizeSize == 0) {
        return FALSE;
    }

    FontSize = StrToLong(FontSizeString);

    //
    // Actually create the fonts
    //
    DC = GetDC(Window);

    if (DC == NULL) {
        return FALSE;
    }

    Large.lfHeight = 0 - (GetDeviceCaps(DC,LOGPIXELSY) * FontSize / 72);

    *LargeFont = CreateFontIndirect(&Large);
    *SmallFont = CreateFontIndirect(&Small);

    return TRUE;
}

VOID
SetExteriorTitleFont(
    HWND Dialog
    )
/*++

Routine Description:

    This routine set the fonts for the title and subtitle correctly.

Arguments:

    Dialog -- Supplies the handle of the window containing the controls.

Return Value:

    None.

--*/
{
    HWND Control;

    //
    // Set the correct fonts for the subtitle and title.
    // Why couldn't the wizard code do this??
    //
    Control = GetDlgItem(Dialog, IDC_TITLE_SMALL);

    SetWindowFont(
        Control,
        SmallTitleFont,
        TRUE
        );

    Control = GetDlgItem(Dialog, IDC_TITLE_LARGE);

    SetWindowFont(
        Control,
        LargeTitleFont,
        TRUE
        );

}

BOOL
CancelInitProc(
    HWND Dialog,
	UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    This function is a generic function for initializing cancel pages
    (and finish pages that don't have any special initialization).

Arguments:

    Dialog -- Supplies a window handle for the dialog.
    wParam -- Supplies the wparam from the WM_INITDIALOG message.
    lParam -- Supplies the lparam from the WM_INITDIALOG message.

Return Value:

    See WM_INITDIALOG documentation

--*/
{
	switch (Msg) {
		case WM_INITDIALOG:

			SetExteriorTitleFont(Dialog);

			return TRUE;

		default:

			return TRUE;
	}
}

INT
CancelNotifyProc(
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
    HWND Cancel;

    switch (NotifyHeader->code) {
    
        case PSN_SETACTIVE:

            //
            // Disable the cancel button.  This is really not a very good design
            //
            DisableCancelButton(DialogWindow);

            PostMessage(GetParent(DialogWindow), PSM_SETWIZBUTTONS, 0, PSWIZB_FINISH);

            CheckSetWindowLong(DialogWindow, DWL_MSGRESULT, 0);

            return TRUE;

        default:

            return FALSE;
    }
}

VOID
PostUpdateMessage(
    VOID
    )
/*++

Routine Description:

    This routine posts the update message to all of the interested
    windows.

Arguments:

    none
    
Return Value:

    None.

--*/
{
    ULONG PageIndex;

    for (PageIndex = 0; PageIndex < NUM_PAGES; PageIndex++) {

        if (PageInfo[PageIndex].Dialog != NULL) {

            PostMessage(
                PageInfo[PageIndex].Dialog,
                WM_UPDATELISTVIEW,
                0,
                (LPARAM)0
                );
        }
    }
}

VOID
UpdateApplicationLists(
    HWND ListView,
    DWORD DesiredCapabilities
    )
/*++

Routine Description:

    This routine updates the contents of the specified list view window.  Note:
    it can only be used with listviews that are populated using 
    PopulateApplicationListView.

Arguments:

    ListView - Supplies the handle of the list view window.
    DesiredCapabilities - Supplies the capabilities desired for this list view

Return Value:

    None.

--*/
{
    ULONG NumberOfItems;
    LV_ITEM Item;
    PAPPLICATIONDESCRIPTOR Descriptor;

    NumberOfItems = ListView_GetItemCount(
        ListView
        );

    while (NumberOfItems > 0) {
        NumberOfItems--;

        Item.iItem = NumberOfItems;
        Item.mask = LVIF_PARAM;
        ListView_GetItem(
            ListView,
            &Item
            );

        Descriptor = (PAPPLICATIONDESCRIPTOR)Item.lParam;

        Descriptor->Actions.FreeHandle(Descriptor->Identifier);

        HeapFree(
            GetProcessHeap(),
            0,
            Descriptor
            );
    }

    ListView_DeleteAllItems(
        ListView
        );

    PopulateApplicationListView(ListView, DesiredCapabilities);

}

LONG
ReadAndExpandRegString(
    HKEY RegKey,
    HANDLE Heap,
    PWCHAR ValueName,
    PWCHAR *Value
    )
/*++

Routine Description:

    This function queries the specified value, and if it is a REG_EXPAND_SZ,
    performs the expansion.  The value is returned in a buffer allocated from
    the specified heap.

Arguments:

    RegKey - Supplies the handle of the key to query the value from.
    Heap - Supplies the handle of the heap to allocate the result string
        from.
    ValueName - Supplies the name of the value to query.
    Value - Returns the value

Return Value:

    Return value from RegQueryValueEx, or ERROR_NOT_ENOUGH_MEMORY.

--*/
{
    WCHAR ValueBuffer[256];
    PWCHAR ValueBufferPointer, StringBuffer;
    ULONG ValueBufferSize, DesiredStringLength;
    LONG RetVal;
    DWORD Type;

    //
    // Get the Value
    //
    ValueBufferSize = 256;
    RetVal = RegQueryValueEx(
        RegKey,
        ValueName,
        0,
        &Type,
        (PBYTE)ValueBuffer,
        &ValueBufferSize
        );

    //
    // If the value was longer than our buffer, allocate a buffer of 
    // appropriate length and try again
    //
    if (RetVal == ERROR_MORE_DATA) {
        
        ValueBufferPointer = HeapAlloc(
            Heap,
            0,
            ValueBufferSize
            );

        RetVal = RegQueryValueEx(
            RegKey,
            ValueName,
            0,
            &Type,
            (PBYTE)ValueBuffer,
            &ValueBufferSize
            );
        
        if (RetVal != ERROR_SUCCESS) {
            //
            // We've got some sort of problem
            //
            HeapFree(
                Heap,
                0,
                ValueBufferPointer
                );

            return RetVal;
        }
    } else {

        ValueBufferPointer = ValueBuffer;

    }

    //
    // Did we get a value?
    //
    if (RetVal != ERROR_SUCCESS) {
        return RetVal;
    }

    //
    // Expand the string if appropriate.
    //
    if (Type == REG_EXPAND_SZ) {

        //
        // Try an expand to get the appropriate size
        //
        DesiredStringLength = ExpandEnvironmentStrings(
            ValueBufferPointer,
            NULL,
            0
            );

        StringBuffer = HeapAlloc(
            Heap,
            0,
            DesiredStringLength * sizeof(WCHAR)
            );

        if (StringBuffer == NULL) {
            RetVal = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        //
        // Expand it for real
        //
        ExpandEnvironmentStrings(
            ValueBufferPointer,
            StringBuffer,
            DesiredStringLength
            );

    } else if (Type == REG_SZ) {

        //
        // We just need to get the length and copy it (maybe)
        //
        if (ValueBufferPointer != ValueBuffer) {

            //
            // We already allocated space in the heap. We can just 
            // return that
            //
            StringBuffer = ValueBufferPointer;
            ValueBufferPointer = ValueBuffer;

        } else {

            DesiredStringLength = lstrlen(ValueBufferPointer) + 1;
            StringBuffer = HeapAlloc(
                Heap,
                0,
                DesiredStringLength * sizeof(WCHAR)
                );

            if (StringBuffer == NULL) {
                RetVal = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }

            lstrcpy(StringBuffer, ValueBufferPointer);
        }

    } else {

        //
        // Wrong type for the value
        //
        RetVal = ERROR_INVALID_DATA;
        goto cleanup;
    }

    RetVal = ERROR_SUCCESS;

    *Value = StringBuffer;

cleanup:

    if (ValueBufferPointer != ValueBuffer) {

        HeapFree(
            Heap,
            0,
            ValueBufferPointer
            );    
    }

    return RetVal;
}    

VOID
DisableCancelButton(
    HWND Dialog
    )
{
    HWND Cancel;

    Cancel = GetDlgItem(
        GetParent(Dialog),
        IDCANCEL
        );

    EnableWindow(Cancel, FALSE);
}