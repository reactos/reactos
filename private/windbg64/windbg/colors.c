/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Colors.c

Abstract:

    This module contains the implementation for Windbg's color support.

Author:

    David J. Gilman (davegi) 30-Jul-1992
         Griffith Wm. Kadnier (v-griffk) 01-Sep-1992

Environment:

    CRT, Windows, User Mode

--*/

//
// Get access to OEM bitmap constants.
//

#define OEMRESOURCE

#include "precomp.h"
#pragma hdrstop


extern HWND GetWatchHWND(void);




LRESULT SendMessageNZ (HWND,UINT,WPARAM,LPARAM);


//
// Common dialog IDs
//

//
// Helper macros to enable / disable 'Set' buttons.
//

//
//  VOID DisableSetButtons()
//

#define DisableSetButtons( )                                            \
    {                                                                   \
        EnableWindow(                                                   \
            GetDlgItem( hDlg, IDC_PUSH_DEFAULT ),                       \
            FALSE                                                       \
            );                                                          \
        EnableWindow(                                                   \
            GetDlgItem( hDlg, IDC_PUSH_SET_FOREGROUND ),                \
            FALSE                                                       \
            );                                                          \
        EnableWindow(                                                   \
            GetDlgItem( hDlg, IDC_PUSH_SET_BACKGROUND ),                \
            FALSE                                                       \
            );                                                          \
    }

//
//  VOID  ToggleSetButtons( )
//

#define ToggleSetButtons( )                                             \
    {                                                                   \
        EnableWindow(                                                   \
            GetDlgItem( hDlg, IDC_PUSH_DEFAULT ),                       \
            ( Selections == 0 ) ? FALSE : TRUE                          \
            );                                                          \
        EnableWindow(                                                   \
            GetDlgItem( hDlg, IDC_PUSH_SET_FOREGROUND ),                \
            ( Selections == 0 ) ? FALSE : TRUE                          \
            );                                                          \
        EnableWindow(                                                   \
            GetDlgItem( hDlg, IDC_PUSH_SET_BACKGROUND ),                \
            ( Selections == 0 ) ? FALSE : TRUE                          \
            );                                                          \
    }


//
// Registered message for hooking CHOOSECOLOR's OK button.
//

UINT    WmColorOk = 0;

//
// Resource text for "Select All" / "Clear All" button text.
//

TCHAR   SelectAllText[ MAX_PATH ];
TCHAR   ClearAllText[ MAX_PATH ];

//
// CHOOSECOLOR's dialog hook proc.
//

UINT_PTR APIENTRY ChooseColorHookProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

//
// Color values used by colorizable strings.
//

#define UBLACK           RGB (000, 000, 000)
#define DARK_RED        RGB (128, 000, 000)
#define DARK_GREEN      RGB (000, 128, 000)
#define DARK_YELLOW     RGB (128, 128, 000)
#define DARK_BLUE       RGB (000, 000, 128)
#define DARK_MAGENTA    RGB (128, 000, 128)
#define DARK_CYAN       RGB (000, 128, 128)
#define DARK_GRAY       RGB (128, 128, 128)
#define LIGHT_GRAY      RGB (192, 192, 192)
#define LIGHT_RED       RGB (255, 000, 000)
#define LIGHT_GREEN     RGB (000, 255, 000)
#define LIGHT_YELLOW    RGB (255, 255, 000)
#define LIGHT_BLUE      RGB (000, 000, 255)
#define LIGHT_MAGENTA   RGB (255, 000, 255)
#define LIGHT_CYAN      RGB (000, 255, 255)
#define UWHITE           RGB (255, 255, 255)

//
// Colorizable string IDs.
//

UINT
StringIds[ ] = {

    IDS_SOURCE_WINDOW,
    IDS_DUMMY_WINDOW,
    IDS_WATCH_WINDOW,
    IDS_LOCALS_WINDOW,
    IDS_CPU_WINDOW,
    IDS_DISASSEMBLER_WINDOW,
    IDS_COMMAND_WINDOW,
    IDS_FLOAT_WINDOW,
    IDS_MEMORY_WINDOW,
    IDS_CALLS_WINDOW,
    IDS_BREAKPOINT_LINE,
    IDS_CURRENT_LINE,
    IDS_CURRENTBREAK_LINE,
    IDS_UNINSTANTIATEDBREAK,
    IDS_TAGGED_LINE,
    IDS_TEXT_SELECTION,
    IDS_KEYWORD,
    IDS_IDENTIFIER,
    IDS_COMMENT,
    IDS_NUMBER,
    IDS_REAL,
    IDS_STRING,
    IDS_ACTIVEEDIT,
    IDS_CHANGEHISTORY

};

//
// Resource text for colorizable strings.
//

STRINGTEXT
StringText[ MAX_STRINGS ];

//
// Default colorizable string colors.
//

STRINGCOLORS
DefaultStringColors[ ] = {

//
//  Forground       Background
//  ---------       ----------
//

//
//          Windows
//          -------
//

    UBLACK,          UWHITE,              // Source
    UBLACK,          UWHITE,              // Dummy--DO NOT REMOVE!!GWK
    UBLACK,          UWHITE,              // Watch
    UBLACK,          UWHITE,              // Locals
    UBLACK,          UWHITE,              // Registers
    UBLACK,          UWHITE,              // Disassembler
    UBLACK,          UWHITE,              // Command
    UBLACK,          UWHITE,              // Floating Point Registers
    UBLACK,          UWHITE,              // Memory
    UBLACK,          UWHITE,              // Calls

//
//          Lines
//          -----
//

    UWHITE,          LIGHT_RED,          // Breakpoints
    UBLACK,          LIGHT_YELLOW,       // Current
    UBLACK,          LIGHT_GREEN,        // Current&Break
    UBLACK,          LIGHT_MAGENTA,      // UninstatiatedBreak
    LIGHT_RED,       LIGHT_CYAN,         // Tagged
    UWHITE,          UBLACK,             // Selection

//
//          Syntax
//          ------
//

    DARK_MAGENTA,    UWHITE,              // Keyword
    UBLACK,          UWHITE,              // Identifier
    UBLACK,          LIGHT_GRAY,          // Comment
    LIGHT_BLUE,      UWHITE,              // Number
    DARK_GRAY,       UWHITE,              // Real
    DARK_CYAN,       UWHITE,              // String
    DARK_BLUE,       UWHITE,              // ActiveEdit
    LIGHT_RED,       UWHITE               // ChangeHistory
};

//
// Colorizable string colors, initialized to DefaultStringColors.
//

STRINGCOLORS
StringColors[ ] = {

//
//  Forground       Background
//  ---------       ----------
//

//
//          Windows
//          -------
//

    UBLACK,          UWHITE,              // Source
    UBLACK,          UWHITE,              // Dummy--DO NOT REMOVE!!GWK
    UBLACK,          UWHITE,              // Watch
    UBLACK,          UWHITE,              // Locals
    UBLACK,          UWHITE,              // Registers
    UBLACK,          UWHITE,              // Disassembler
    UBLACK,          UWHITE,              // Command
    UBLACK,          UWHITE,              // Floating Point Registers
    UBLACK,          UWHITE,              // Memory
    UBLACK,          UWHITE,              // Calls

//
//          Lines
//          -----
//

    UWHITE,          LIGHT_RED,          // Breakpoints
    UBLACK,          LIGHT_YELLOW,       // Current
    UBLACK,          LIGHT_GREEN,        // Current&Break
    UBLACK,          LIGHT_MAGENTA,      // UninstatiatedBreak
    LIGHT_RED,       LIGHT_CYAN,         // Tagged
    UWHITE,          UBLACK,             // Selection

//
//          Syntax
//          ------
//

    DARK_MAGENTA,   UWHITE,              // Keyword
    UBLACK,         UWHITE,              // Identifier
    UBLACK,         LIGHT_GRAY,          // Comment
    LIGHT_BLUE,     UWHITE,              // Number
    DARK_GRAY,      UWHITE,              // Real
    DARK_CYAN,      UWHITE,              // String
    DARK_BLUE,      UWHITE,              // ActiveEdit
    LIGHT_RED,      UWHITE               // ChangeHistory

};


//
// User defined custom colors.
//

COLORREF
CustomColors[ ] = {

    UBLACK,
    DARK_RED,
    DARK_GREEN,
    DARK_YELLOW,
    DARK_BLUE,
    DARK_MAGENTA,
    DARK_CYAN,
    DARK_GRAY,
    LIGHT_GRAY,
    LIGHT_RED,
    LIGHT_GREEN,
    LIGHT_YELLOW,
    LIGHT_BLUE,
    LIGHT_MAGENTA,
    LIGHT_CYAN,
    UWHITE
};


/*++

Routine Description:

Arguments:

Return Value:

Notes:

    Even though the IDC_LIST_ITEMS control is owner draw, the
    WM_MEASUREITEM is not handled as the default sizes are acceptable.

--*/


UINT_PTR APIENTRY
ChooseColorHookProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UINT            i,j;
    PDRAWITEMSTRUCT DrawItemStruct;
    LPCHOOSECOLOR   ChooseColor;
    PTCHAR          ButtonText;
    UINT            SelectedStringIds[MAX_STRINGS];
    HWND            hDlgItem;

    static WORD     SetColorButtonId;
    static BOOL     ApplyFlag;
    static HBITMAP  hBitmap;
    static HDC      hDCMem;
    static BITMAP   Bitmap;
    static UINT     Selections;
    static HDC      mHdc;       //hdc of windbg


    switch (message)
    {
    case WM_INITDIALOG:

        mHdc = GetWindowDC (hwndFrame);

        if ((hDlgItem = GetDlgItem (hDlg,IDC_LIST_ITEMS)) == (HWND)NULL)
        {
            return (FALSE);
        }

        // Initialize the list of items to colorize.

        for ( i = 0; i < (MAX_STRINGS - 1); i++ )
        {
            SendMessage (hDlgItem,LB_ADDSTRING,0,( LPARAM ) StringText[i].Text);
        }


        // Initialize the 'selected' (i.e. checkmark) bitmap.

        if ((hBitmap = LoadBitmap( NULL, MAKEINTRESOURCE( OBM_CHECK ))) == (HBITMAP)NULL)
        {
            return FALSE;
        }


        // Get the size of the 'selected' (i.e. checkmark) bitmap.

        if ( GetObject( hBitmap, sizeof( BITMAP ), &Bitmap ) == 0 )
        {
            return FALSE;
        }


        // Remember that the compatible DC has not been created.

        hDCMem = NULL;


        // Default to setting the foreground color.
        SetColorButtonId = IDC_PUSH_SET_FOREGROUND;


        // Default to being able to Select All.

        Selections = 0;


        // Disable the 'Set' buttons when there is no selections.

        DisableSetButtons( );


        // Default to OK button meaning OK (see IDC_PUSH_SET*).

        ApplyFlag = FALSE;

        return TRUE;

    case WM_DESTROY:

        // Delete the objects needed to draw the 'selected' bitmap.

        ReleaseDC (hwndFrame,mHdc); //release the frame DC

        DeleteDC( hDCMem );
        DeleteObject( hBitmap );

        //
        // schedule everything for redraw
        //
        RedrawWindow(g_hwndMDIClient,
                     NULL,
                     NULL, 
                     RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN
                     );  

        return FALSE;

    case WM_COMMAND:

        switch ( LOWORD( wParam ))
        {
        case IDWINDBGHELP:
        case pshHelp:

            Dbg(WinHelp(hDlg, 
                        szHelpFileName, 
                        (UINT) HELP_CONTEXT, 
                        (ULONG_PTR) ID_COLORDLG_HELP
                        ));
            return TRUE;

        case IDC_PUSH_DEFAULT:
            // Reinitialize the colors to their default values and
            // repaint the colorizable strings listbox.

            // Get the IDs for each selected string.

            SendDlgItemMessage(hDlg,IDC_LIST_ITEMS,LB_GETSELITEMS,( WPARAM ) MAX_STRINGS,(LPARAM) SelectedStringIds);

            // For each of the selected strings reset the colors to their default values.

            for ( i = 0; i < Selections; i++ )
            {
                if (SelectedStringIds[i] > 0)
                {
                    j = SelectedStringIds[i] + 1;
                }
                else
                {
                    j = 0;
                }


                StringColors[j].FgndColor = DefaultStringColors[j].FgndColor;
                StringColors[j].BkgndColor = DefaultStringColors[j].BkgndColor;
            }


            RedrawWindow (GetDlgItem( hDlg, IDC_LIST_ITEMS ),NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW);

            SendMessageNZ( GetCpuHWND(),   WU_CLR_FORECHANGE, 0, 0L);
            SendMessageNZ( GetFloatHWND(), WU_CLR_FORECHANGE, 0, 0L);
            SendMessageNZ( GetLocalHWND(), WU_CLR_FORECHANGE, 0, 0L);
            SendMessageNZ( GetWatchHWND(), WU_CLR_FORECHANGE, 0, 0L);
            SendMessageNZ( GetCallsHWND(), WU_CLR_FORECHANGE, 0, 0L);

            SendMessageNZ( GetCpuHWND(),   WU_CLR_BACKCHANGE, 0, 0L);
            SendMessageNZ( GetFloatHWND(), WU_CLR_BACKCHANGE, 0, 0L);
            SendMessageNZ( GetLocalHWND(), WU_CLR_BACKCHANGE, 0, 0L);
            SendMessageNZ( GetWatchHWND(), WU_CLR_BACKCHANGE, 0, 0L);
            SendMessageNZ( GetCallsHWND(), WU_CLR_BACKCHANGE, 0, 0L);


            return TRUE;

        case IDC_PUSH_SET_FOREGROUND:
        case IDC_PUSH_SET_BACKGROUND:

            // Simulate a push of the OK button so that the current
            // CHOOSECOLOR structure can be grabbed.

            ApplyFlag   = TRUE;

            SetColorButtonId = LOWORD( wParam );

            SendMessage(hDlg,WM_COMMAND,IDOK,0L);

            return TRUE;

        case IDC_PUSH_SELECT_ALL:

            // Toggle the Select All / Clear All state.

            if ( Selections == (MAX_STRINGS-1) )
            {
                Selections = 0;
                ButtonText = SelectAllText;
            }
            else
            {
                Selections = (MAX_STRINGS-1);
                ButtonText = ClearAllText;
            }

            // Select / deselect all of the colorizable strings.

            SendDlgItemMessage (hDlg,IDC_LIST_ITEMS,LB_SETSEL,(WPARAM) Selections,(LPARAM) -1);

            // Set the appropriate text for the "Select All / Clear All" button.

            SendDlgItemMessage (hDlg,IDC_PUSH_SELECT_ALL,WM_SETTEXT,0,(LPARAM) ButtonText);

            // Disable the 'Set' buttons when there is no selections, otherwise enable them.

            ToggleSetButtons( );

            return TRUE;

        }
        break;

        case WM_DRAWITEM:

            DrawItemStruct = (PDRAWITEMSTRUCT) lParam;

            // If the message is intended for the colorizable strings list box, draw it.

            if (DrawItemStruct->CtlID == IDC_LIST_ITEMS)
            {
                // Treat drawing the entire item and drawing a selected item the same.

                if (DrawItemStruct->itemAction & (ODA_DRAWENTIRE | ODA_SELECT))
                {
                    // Set the appropriate foreground and background colors for this item.


                    if (DrawItemStruct->itemID > 0)
                    {
                        i=DrawItemStruct->itemID + 1;
                    }
                    else
                    {
                        i=0;
                    }

                    SetTextColor (DrawItemStruct->hDC, StringColors[i].FgndColor);

                    SetBkColor (DrawItemStruct->hDC, StringColors[i].BkgndColor);

                    // Draw the text, leaving room for the 'selected' bitmap.

                    ExtTextOut (DrawItemStruct->hDC,
                        (int)(DrawItemStruct->rcItem.left + Bitmap.bmWidth * 1.5),
                        DrawItemStruct->rcItem.top,
                        ETO_OPAQUE,
                        &(DrawItemStruct->rcItem),
                        StringText[DrawItemStruct->itemID].Text,
                        StringText[DrawItemStruct->itemID].Length,
                        NULL);


                    // If the item is selected draw the selection bitmap.

                    if (DrawItemStruct->itemState & ODS_SELECTED)
                    {

                        // Create the compatible DC if it was never created.

                        if (hDCMem == NULL)
                        {
                            hDCMem = CreateCompatibleDC (DrawItemStruct->hDC);

                            // Couldn't create the memory DC.

                            if (hDCMem == NULL)
                            {
                                return FALSE;
                            }

                            // Couldn't select the bitmap into the DC.

                            if (SelectObject (hDCMem, hBitmap) == NULL)
                            {
                                return FALSE;
                            }
                        }

                        // Blt the 'selected' bitmap to the left of the item.
                        // Always use black unless the background is black,
                        // then use white.

                        SetTextColor (DrawItemStruct->hDC,
                            (StringColors[ i ].BkgndColor == UBLACK)
                            ? UWHITE
                            : UBLACK);

                        if (BitBlt (DrawItemStruct->hDC,
                            DrawItemStruct->rcItem.left,
                            DrawItemStruct->rcItem.top,
                            DrawItemStruct->rcItem.right - DrawItemStruct->rcItem.left,
                            DrawItemStruct->rcItem.bottom - DrawItemStruct->rcItem.top,
                            hDCMem,
                            0,
                            0,
                            SRCCOPY) == FALSE )
                        {
                            return FALSE;
                        }
                    }

                    // If the item is being selected or deselected.

                    if (DrawItemStruct->itemAction & ODA_SELECT)
                    {
                        // If the item is being selected...

                        if (DrawItemStruct->itemState & ODS_SELECTED)
                        {
                            // One more item selected.

                            Selections++;

                            // If the last item was just selected, set the
                            // 'Clear All' button text.

                            if (Selections == (MAX_STRINGS - 1))
                            {
                                ButtonText = ClearAllText;

                                SendDlgItemMessage (hDlg,
                                    IDC_PUSH_SELECT_ALL,
                                    WM_SETTEXT,
                                    0,
                                    (LPARAM) ButtonText);
                            }

                        }
                        else
                        {
                            // If the item is being deselected...

                            // If the last item was just de-selected, set the
                            // 'Select All' button text.

                            // Check for MAX_STRINGS before the decrement so
                            // that only transitions cause the WM_SETTEXT.

                            if (Selections == (MAX_STRINGS - 1))
                            {
                                ButtonText = SelectAllText;

                                SendDlgItemMessage (hDlg,
                                    IDC_PUSH_SELECT_ALL,
                                    WM_SETTEXT,
                                    0,
                                    (LPARAM) ButtonText);
                            }

                            // One less item selected.

                            Selections--;
                        }

                        // Disable the 'Set' buttons when there is no
                        // selections, otherwise enable them.

                        ToggleSetButtons( );
                    }
                 }

                 // If the item has the focus, draw the focus rectangle.

                 if (DrawItemStruct->itemAction & ODA_FOCUS)
                 {
                     DrawFocusRect (DrawItemStruct->hDC,&(DrawItemStruct->rcItem));
                 }

                 // Set the appropriate text for the "Select All / Clear All" button.

                 return TRUE;
            }
            break;

    default:

        // Handle the registered OK button message if it was
        // generated due to one of the 'Set Foreground / Set Background'
        // buttons being pressed.

        if ((message == WmColorOk) &&  ApplyFlag )
        {
            // Reset the flag so a real press of OK will work.

            ApplyFlag = FALSE;

            ChooseColor = (LPCHOOSECOLOR) lParam;

            // Get the IDs for each selected string.

            SendDlgItemMessage (hDlg,
                IDC_LIST_ITEMS,
                LB_GETSELITEMS,
                (WPARAM) MAX_STRINGS,
                (LPARAM) SelectedStringIds);

            // For each of the selected strings set the appropriate color
            // based on the foreground / background radio buttons.

            for (i = 0; i < Selections; i++)
            {
                if (SelectedStringIds[i] > 0)
                {
                    j = SelectedStringIds[i] + 1;
                }
                else
                {
                    j = 0;
                }


                if (SetColorButtonId == IDC_PUSH_SET_FOREGROUND)
                {
                    StringColors[j].FgndColor = GetNearestColor (mHdc, ChooseColor->rgbResult);
                }
                else
                {
                    StringColors[j].BkgndColor = GetNearestColor (mHdc, ChooseColor->rgbResult);
                }

            }

            // Repaint the colorizable strings listbox.





            RedrawWindow (GetDlgItem( hDlg, IDC_LIST_ITEMS ),
                NULL,
                NULL,
                RDW_INVALIDATE | RDW_UPDATENOW);

            SendMessageNZ( GetCpuHWND(),   WU_CLR_FORECHANGE, 0, 0L);
            SendMessageNZ( GetFloatHWND(), WU_CLR_FORECHANGE, 0, 0L);
            SendMessageNZ( GetLocalHWND(), WU_CLR_FORECHANGE, 0, 0L);
            SendMessageNZ( GetWatchHWND(), WU_CLR_FORECHANGE, 0, 0L);
            SendMessageNZ( GetCallsHWND(), WU_CLR_FORECHANGE, 0, 0L);

            SendMessageNZ( GetCpuHWND(),   WU_CLR_BACKCHANGE, 0, 0L);
            SendMessageNZ( GetFloatHWND(), WU_CLR_BACKCHANGE, 0, 0L);
            SendMessageNZ( GetLocalHWND(), WU_CLR_BACKCHANGE, 0, 0L);
            SendMessageNZ( GetWatchHWND(), WU_CLR_BACKCHANGE, 0, 0L);
            SendMessageNZ( GetCallsHWND(), WU_CLR_BACKCHANGE, 0, 0L);

            return TRUE;
        }
   }

   return FALSE;
}

/*++

Routine Description:

Arguments:

Return Value:

--*/


BOOL SelectColor(HWND hWnd)
{
    UINT            i;
    INT             Length;
    CHOOSECOLOR     Cc;
    STRINGCOLORS    TmpStringColors[MAX_STRINGS];
    COLORREF        TmpCustomColors[MAX_STRINGS];

    // If WmColorOK is zero then the message has never been registered.
    // Use this as a trigger for performing one time initialization.

    if (WmColorOk == 0)
    {
        if ((WmColorOk = RegisterWindowMessage (COLOROKSTRING)) == 0)
        {
            return(FALSE);  // Couldn't register the message
        }

        // Load the text for the colorizable strings.

        for (i = 0; i < MAX_STRINGS; i++)
        {
            if ((StringText[i].Length = LoadString (g_hInst,
                StringIds[i],
                StringText[i].Text,
                sizeof (StringText[i].Text))) == 0)
            {
                return(FALSE); // Couldn't load the string resource.
            }

        }

        for (i = 1; i < (MAX_STRINGS - 1); i++)
        {
            StringText[i].Length = StringText[i+1].Length;
            strcpy (StringText[i].Text, StringText[i+1].Text);

            //memcpy (&StringText[i], &StringText[i+1], sizeof(StringText));
        }




        // Load the "Select All" / "Clear All" button text.

        if ((Length = LoadString (g_hInst,
            IDS_SELECT_ALL,
            SelectAllText,
            sizeof (SelectAllText))) == 0)
        {
            return(FALSE); // Couldn't load the string resource.
        }


        if ((Length = LoadString (g_hInst,
            IDS_CLEAR_ALL,
            ClearAllText,
            sizeof (ClearAllText))) == 0)
        {
            return(FALSE); // Couldn't load the string resource.
        }

    }

    // Make copies of the volatile data in case the user presses Cancel.

    // Make sure we don't perform half-assed copies
    Assert(sizeof( TmpStringColors ) == sizeof( CustomColors ));

    memcpy( TmpStringColors, StringColors, sizeof( TmpStringColors ));
    memcpy( TmpCustomColors, CustomColors, sizeof( CustomColors ));

    // Initialize the CHOOSECOLOR structure.

    Cc.lStructSize      = sizeof( CHOOSECOLOR );
    Cc.hwndOwner        = hWnd;
    Cc.hInstance        = NULL;
    Cc.rgbResult        = RGB (64, 188, 188);
    Cc.lpCustColors     = CustomColors;
    Cc.Flags            = CC_ENABLEHOOK
        | CC_ENABLETEMPLATE
        | CC_FULLOPEN
        | CC_RGBINIT
        | CC_SHOWHELP;
    Cc.lCustData        = 0;
    Cc.lpfnHook         = ChooseColorHookProc;
    Cc.lpTemplateName   = MAKEINTRESOURCE( DLG_CHOOSECOLOR );

    // Let the user choose colors.

    if (ChooseColor (&Cc))
    {
        // The user pressed OK.

        return TRUE;
    }
    else
    {
        // The user pressed Cancel or closed the dialog.
        // Restore the user's original string and custom colors.

        memcpy( StringColors, TmpStringColors, sizeof( StringColors ));
        memcpy( CustomColors, TmpCustomColors, sizeof( CustomColors ));

        SendMessageNZ( GetCpuHWND(),   WU_CLR_FORECHANGE, 0, 0L);
        SendMessageNZ( GetFloatHWND(), WU_CLR_FORECHANGE, 0, 0L);
        SendMessageNZ( GetLocalHWND(), WU_CLR_FORECHANGE, 0, 0L);
        SendMessageNZ( GetWatchHWND(), WU_CLR_FORECHANGE, 0, 0L);
        SendMessageNZ( GetCallsHWND(), WU_CLR_FORECHANGE, 0, 0L);

        SendMessageNZ( GetCpuHWND(),   WU_CLR_BACKCHANGE, 0, 0L);
        SendMessageNZ( GetFloatHWND(), WU_CLR_BACKCHANGE, 0, 0L);
        SendMessageNZ( GetLocalHWND(), WU_CLR_BACKCHANGE, 0, 0L);
        SendMessageNZ( GetWatchHWND(), WU_CLR_BACKCHANGE, 0, 0L);
        SendMessageNZ( GetCallsHWND(), WU_CLR_BACKCHANGE, 0, 0L);

        return FALSE;
    }
}
