/*--Author:

    Griffith Wm. Kadnier (v-griffk) 01-Aug-1992

Environment:

    Win32, User Mode

--*/



#include "precomp.h"
#pragma hdrstop

#include "include\cntxthlp.h"

/**********************************************************************/

#define MAX_LOCATION    255
#define MAX_EXPRESSION  255
#define MAX_COMMAND     255

#define WNDPROC_HISTORY_SIZE 32

/**********************************************************************/

static  char            rgchT[255];
        char FAR        WndProcHistory[WNDPROC_HISTORY_SIZE][MAX_LOCATION];
        int  FAR        NumHistoricalWndProcs = 0;
        char            MsgBuffer[255];
        char            szLocation[255];
        char            szExpression[255];
        char            szLength[20];
        char            szWndProc[255];
        char            szPass[20];
        char            szPassLeft[20];
        char            szProcess[20];
        char            szThread[20];
        char            szCommand[255];

static INT_PTR TopIndex;


VOID
EnableBPButtons(
    HWND hDlg,
    int iBP
    );

VOID
SetChooseClass(
    HWND,
    BOOL
    );

VOID
FillMsgCombo(
    HWND
    );

HBPT
BPLB_HbptOfI(
    int iBP
    );



/***    FillWndProcCombo
**
**  Synopsis:
**      void = FillWndProcCombo(hDlg)
**
**  Entry:
**      hDlg - handle to dialog box for combo box
**
**  Returns:
**      Nothing
**
**  Description:
**      This function is used to fill in the combo box which contains the
**      list of window procedures which are known to the system.  This is
**      based on the last N items which had a breakpoint of a window proc
**      type. (i.e. on message)
*/

VOID
FillWndProcCombo(
    HWND hDlg
    )
{
    int i;

    for (i = 0; i < NumHistoricalWndProcs; i++) {
        SendDlgItemMessage(hDlg, ID_SETBREAK_WNDPROC,
                CB_ADDSTRING, 0, (LPARAM)WndProcHistory[i]);
    }
    return;
}                                       /* FillWndProcCombo() */

/***    StoreWndProcHistory
**
**  Synopsis:
**      void = StoreWndProcHistory(hDlg)
**
**  Entry:
**      hDlg -  handle to current breakpoint dialog
**
**  Returns:
**      Nothing
**
**  Description:
**      This function is used to retreive the strings which contain the
**      names of the latest window procedure names back from the dialog
**      box and store them into the array
*/

VOID
StoreWndProcHistory(
    HWND hDlg
    )
{
    int i;

    NumHistoricalWndProcs = (int)SendDlgItemMessage(hDlg, ID_SETBREAK_WNDPROC,
          CB_GETCOUNT, 0, 0L);

    NumHistoricalWndProcs = min(WNDPROC_HISTORY_SIZE,
                                NumHistoricalWndProcs);

    for (i = 0; i < NumHistoricalWndProcs; i++) {
        SendDlgItemMessage(hDlg, ID_SETBREAK_WNDPROC,
                CB_GETLBTEXT, i, (LPARAM)WndProcHistory[i]);
    }
    return;
}                                       /* StoreWndProcHistory() */



BOOL
ClearWndProcHistory (
    void
    )
/*++

Routine Description:

    Clears the WndProc history

Arguments:

    None

Return Value:

    BOOL -   TRUE if cleared

--*/
{
    int i;

    for (i = 0; i < NumHistoricalWndProcs; i++) {
        WndProcHistory[i][0] = '\0';
    }

    NumHistoricalWndProcs = 0;

    return TRUE;
}




BOOL
SetWndProcHistory (
    LPSTR   List,
    DWORD   ListLength
    )
/*++

Routine Description:

    Adds the WndProcs from a multistring to the end of the current
    WndProc history.

Arguments:

    List        -   Supplies the multistring of WndProcs
    ListLength  -   Supplies length of multistring

Return Value:

    BOOL - TRUE if ALL WndProcs in the list were added.

--*/
{
    LPSTR   WndProc;
    DWORD   Next = 0;
    BOOL    Ok   = FALSE;

    while ( NumHistoricalWndProcs < WNDPROC_HISTORY_SIZE ) {

        if ( WndProc = GetNextStringFromMultiString( List, ListLength, &Next )  ) {

            strcpy( WndProcHistory[ NumHistoricalWndProcs++ ], WndProc );

        } else {

            Ok = TRUE;
            break;
        }
    }

    return Ok;
}



LPSTR
GetWndProcHistory (
    DWORD  *ListLength
    )
/*++

Routine Description:

    Generates a multistring with the WndProc history. MRU WndProc goes
    first.

Arguments:

    ListLength  -   Supplies pointer to length of multistring

Return Value:

    LPSTR   -   Multistring with WndProc history. May be NULL if
                no history.

--*/
{
    LPSTR   List    = NULL;
    DWORD   Length  = 0;
    int     i;

    for (i = 0; i < NumHistoricalWndProcs; i++) {
        AddToMultiString( &List, &Length, WndProcHistory[i] );
    }

    *ListLength = Length;
    return List;
}





/***    BreakDefPushButton
**
**  Synopsis:
**      void = BreakDefPushButton(hDlg, ButtonId)
**
**  Entry:
**      hDlg    - handle to breakpoint dialog box
**      ButtonId - Button to set as the default button
**
**  Returns:
**      nothing
**
**  Description:
**      This function is used to set the default button in the breakpoint
**      dialog box.  Which button is to be the default is passed in as
**      ButtonId.
*/

VOID
BreakDefPushButton(
    HWND  hDlg,
    INT   ButtonId
    )
{
    SendMessage( hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg,ButtonId), (LPARAM)TRUE );
    return;
}                                       /* BreakDefPushButton() */

/***    SetbreakControls
**
**  Synopsis:
**      void = SetbreakControls(hDlg, bpType)
**
**  Entry:
**      hDlg
**      bpType
**
**  Returns:
**      Nothing
**
**  Description:
**      Initialize the controls according to the
**      value of bpType.  (CB_ERR means no Action selected.)
**
*/

VOID
SetbreakControls(
    HWND hDlg,
    int bpType
    )
{
    BOOL fEnableLocation, fEnableWndProc,
        fEnableExpression, fEnableLength,
        fEnableMessages;

    fEnableLocation = FALSE;
    fEnableWndProc = FALSE;
    fEnableExpression = FALSE;
    fEnableLength = FALSE;
    fEnableMessages = FALSE;

    switch (bpType)
    {
      case BPLOC:
        // Break at Location
        fEnableLocation = TRUE;
        break;

      case BPLOCEXPRTRUE:
        // Break at Location if Expression true
        fEnableLocation = TRUE;
        fEnableExpression = TRUE;
        break;

      case BPLOCEXPRCHGD:
        // Break at Location if Expression changed
        fEnableLocation = TRUE;
        fEnableExpression = TRUE;
        fEnableLength = TRUE;
        break;

      case BPEXPRTRUE:
        // Break when Expression true
        fEnableExpression = TRUE;
        break;

      case BPEXPRCHGD:
        // Break when Expression changed
        fEnableExpression = TRUE;
        fEnableLength = TRUE;
        break;

      case BPWNDPROC:
        // Break at Wnd Proc
        fEnableWndProc = TRUE;
        break;

      case BPWNDPROCEXPRTRUE:
        // Break at Wnd Proc if Expression true
        fEnableWndProc = TRUE;
        fEnableExpression = TRUE;
        break;

      case BPWNDPROCEXPRCHGD:
        // Break at Wnd Proc if Expression changed
        fEnableWndProc = TRUE;
        fEnableExpression = TRUE;
        fEnableLength = TRUE;
        break;

      case BPWNDPROCMSGRCVD:
        // Break at Wnd Proc if Message Received
        fEnableWndProc = TRUE;
        fEnableMessages = TRUE;
        break;

      case CB_ERR:
        // no action selected - all controls disabled
        break;

      default:
        Dbg(FALSE);
    }

    EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_SLOCATION), fEnableLocation);
    EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_LOCATION), fEnableLocation);
    EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_SWNDPROC), fEnableWndProc);
    EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_WNDPROC), fEnableWndProc);
    EnableWindow(GetWindow(GetDlgItem(hDlg, ID_SETBREAK_WNDPROC), GW_CHILD),
          fEnableWndProc);
    EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_SEXPRESSION), fEnableExpression);
    EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_EXPRESSION), fEnableExpression);
    EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_SLENGTH), fEnableLength);
    EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_LENGTH), fEnableLength);
    EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_MESSAGES), fEnableMessages);

    return;
}                                       /* SetbreakControls() */

/***    fGetSetbreakControls
**
**  Synopsis:
**      bool = fGetSetbreakControls( hDlg )
**
**  Entry:
**      hDlg    - handle to current dialog box
**
**  Returns:
**      TRUE on success and FALSE otherwise
**
**  Description:
**      Retrieve and validate what's in the controls.
**      Return TRUE if ok, FALSE otherwise.
*/

BOOL
fGetSetbreakControls(
    HWND hDlg
    )
{
    int         type;
    HWND hAction, hLocation, hWndProc, hExpression, hLength, hMessages;
    char *      pch;
    BOOL     fLoc     = FALSE;
    BOOL     fWndProc = FALSE;
    char BigBuffer[255];
    char Buffer[255];
    LRESULT  i;

    hAction = GetDlgItem(hDlg, ID_SETBREAK_ACTION);
    hLocation = GetDlgItem(hDlg, ID_SETBREAK_LOCATION);
    hWndProc = GetDlgItem(hDlg, ID_SETBREAK_WNDPROC);
    hExpression = GetDlgItem(hDlg, ID_SETBREAK_EXPRESSION);
    hLength = GetDlgItem(hDlg, ID_SETBREAK_LENGTH);
    hMessages = GetDlgItem(hDlg, ID_SETBREAK_MESSAGES);

    /*
    **  Step 1.  clean out the string
    */

    rgchT[0] = 0;

    /*
    **  Step 2.  Determine the breakpoint type.  This tells us which
    **          fields we need to get information from.  If no type has been
    **          selected then error.
    */

    type = (int)SendMessage(hAction, CB_GETCURSEL, 0, 0L);
    if (type == CB_ERR)
    {
        // no action specified
        MessageBeep(0);
        SetFocus(hAction);
        return FALSE;
    }

    /*
    **  Step 3.  If a location field can be specified get the location field.
    */

    rgchT[0]   = 0;
    *BigBuffer = '\0';

    if ((type == BPLOC) || (type == BPLOCEXPRTRUE) || (type == BPLOCEXPRCHGD)) {

        fLoc = TRUE;
        GetDlgItemText(hDlg, ID_SETBREAK_LOCATION,
              BigBuffer, sizeof(BigBuffer)-1);

    } else if ((type == BPWNDPROC) || (type == BPWNDPROCEXPRTRUE) || (type == BPWNDPROCEXPRCHGD) ||
               (type == BPWNDPROCMSGRCVD) ) {

        fWndProc = TRUE;
        SendMessage( hWndProc, WM_GETTEXT, sizeof( BigBuffer ),
                    (LPARAM)(LPSTR)BigBuffer );
    }

    if ( fLoc || fWndProc ) {

        //
        //      It can not be an empty string and it and a trailing space
        //      must fit into the command string buffer.
        //
        pch = BigBuffer;
        while (*pch == ' ') pch++;
        if (*pch == 0 || (strlen(pch) > sizeof(rgchT)-2)) {
            MessageBeep(0);
            SetFocus(hLocation);
            return FALSE;
        }

        strcpy(rgchT, pch);
        _fstrcat(rgchT, " ");

        if ( fWndProc ) {

            //
            //  Add the WndProc to the list
            //

            //
            //  If the WndProc is already in the list, remove it.
            //
            i = (int) SendMessage( hWndProc, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)BigBuffer );
            if ( i != CB_ERR ) {
                SendMessage( hWndProc, CB_GETLBTEXT, i, (LPARAM)Buffer );
                if ( !strcmp( Buffer, BigBuffer )) {
                    SendMessage( hWndProc,
                                 CB_DELETESTRING,
                                 i,
                                 0L ) ;

                }
            }

            //
            //  Insert the WndProc at the top of the list
            //
            SendMessage( hWndProc,
                         CB_INSERTSTRING,
                         0,
                         (LPARAM)(LPSTR)BigBuffer ) ;

            //
            //  We only keep track of the last WNDPROC_HISTORY_SIZE
            //  strings.
            //
            while ( SendMessage(hWndProc, CB_GETCOUNT, 0, 0L) > WNDPROC_HISTORY_SIZE ) {
                SendMessage( hWndProc,
                             CB_DELETESTRING,
                             WNDPROC_HISTORY_SIZE-1,
                             0L ) ;
            }


            //
            //  Add message (or message class) if one specified.
            //
            if (type == BPWNDPROCMSGRCVD ) {
                if ( *MsgBuffer == '\0' ) {
                    MessageBeep(0);
                    SetFocus(hLocation);
                    return FALSE;
                }
                _fstrcat(rgchT, "/M");
                _fstrcat(rgchT, MsgBuffer );
                _fstrcat(rgchT, " " );
            }
        }
    }

    /*
    **  Grab a memory expression if one should exist
    */

    if ((type == BPLOCEXPRTRUE) || (type == BPEXPRTRUE) ||
        (type == BPWNDPROCEXPRTRUE)) {
        *BigBuffer = 0;
        GetDlgItemText(hDlg, ID_SETBREAK_EXPRESSION, BigBuffer, sizeof(BigBuffer)-1);

        /*
        **      It can not be empty and it must fix in the buffer
        */

        pch = BigBuffer;
        while (*pch == ' ') pch++;
        if ((*pch == 0) || (strlen(pch) > sizeof(rgchT)-2)) {
            MessageBeep(0);
            SetFocus(hExpression);
            return FALSE;
        }

        _fstrcat(rgchT, "?\"");
        _fstrcat(rgchT, pch);
        _fstrcat(rgchT, "\" ");
    }

    /*
    **  Grab a change memory expression if one should exist
    */

    if ((type == BPLOCEXPRCHGD) || (type == BPEXPRCHGD)) {
        *BigBuffer = 0;
        GetDlgItemText(hDlg, ID_SETBREAK_EXPRESSION, BigBuffer, sizeof(BigBuffer)-1);

        /*
        **      It can not be empty and it must fix in the buffer
        */

        pch = BigBuffer;
        while (*pch == ' ') pch++;
        if ((*pch == 0) || (strlen(pch) > sizeof(rgchT)-2)) {
            MessageBeep(0);
            SetFocus(hExpression);
            return FALSE;
        }

        _fstrcat(rgchT, "=\"");
        _fstrcat(rgchT, pch);
        _fstrcat(rgchT, "\" ");

        /*
        **      Now get the length
        */

        *BigBuffer = 0;
        GetDlgItemText(hDlg, ID_SETBREAK_LENGTH, BigBuffer, sizeof(BigBuffer)-1);
        pch = BigBuffer;
        while (*pch == ' ') pch++;
        if (*pch != 0) {
            if (strlen(pch) > sizeof(rgchT)-4) {
                MessageBeep(0);
                SetFocus(hLength);
                return FALSE;
            }

            _fstrcat(rgchT, "/R");
            _fstrcat(rgchT, pch);
            _fstrcat(rgchT, " ");
        }
    }


    /*
    **  Grab a pass count if it exists
    */

    *BigBuffer = 0;
    GetDlgItemText(hDlg, ID_SETBREAK_PASS, BigBuffer, sizeof(BigBuffer)-1);
    pch = BigBuffer;
    while (*pch == ' ') pch++;
    if (*pch != 0) {
        if (strlen(pch) > sizeof(rgchT)-4) {
            MessageBeep(0);
            SetFocus(hLocation);        //M00QUEST
            return FALSE;
        }

        _fstrcat(rgchT, "/P");
        _fstrcat(rgchT, pch);
        _fstrcat(rgchT, " ");
    }

    /*
    **  Grab a process if it exists
    */

    *BigBuffer = 0;
    GetDlgItemText(hDlg, ID_SETBREAK_PROCESS, BigBuffer, sizeof(BigBuffer)-1);
    pch = BigBuffer;
    while (*pch == ' ') pch++;
    if (*pch != 0) {
        if (strlen(pch) > sizeof(rgchT)-4) {
            MessageBeep(0);
            SetFocus(hLocation);        //M00QUEST
            return FALSE;
        }

        _fstrcat(rgchT, "/H");
        _fstrcat(rgchT, pch);
        _fstrcat(rgchT, " ");
    }

    /*
    **  Grab a thread if it exists
    */

    *BigBuffer = 0;
    GetDlgItemText(hDlg, ID_SETBREAK_THREAD, BigBuffer, sizeof(BigBuffer)-1);
    pch = BigBuffer;
    while (*pch == ' ') pch++;
    if (*pch != 0) {
        if (strlen(pch) > sizeof(rgchT)-4) {
            MessageBeep(0);
            SetFocus(hLocation);        //M00QUEST
            return FALSE;
        }

        _fstrcat(rgchT, "/T");
        _fstrcat(rgchT, pch);
        _fstrcat(rgchT, " ");
    }

    /*
    **  Grab a set of commands if exist
    */

    *BigBuffer = 0;
    GetDlgItemText( hDlg, ID_SETBREAK_CMDS, BigBuffer, sizeof(BigBuffer)-1);
    pch = BigBuffer;
    while (*pch == ' ') pch++;
    if (*pch != 0) {
        if (strlen(pch) > sizeof(rgchT)-6) {
            MessageBeep(0);
            SetFocus(hLocation);        // M00QUEST
            return FALSE;
        }

        _fstrcat(rgchT, "/C\"");
        _fstrcat(rgchT, pch);
        _fstrcat(rgchT, "\" ");
    }


    //
    //  If WndProc, add WndProc switch
    //
    if ( fWndProc ) {
        _fstrcat(rgchT, "/W ");
    }


    // If get to here the Breakpoint action has necessary data
    return TRUE;
}                                       /* fGetSetbreakControls() */

/***    FillBPListbox
**
**  Synopsis:
**      void = FillBPListbox(hDlg)
**
**  Entry:
**      hDlg    - handle to dialog box
**
**  Returns:
**      nothing
**
**  Description:
**      Fill the breakpoint list box with the current breakpoints.
**
*/

VOID
FillBPListbox(
    HWND hDlg
    )
{
    HBPT        hBpt = 0;
    HBPT        hBpt2;
    char        szBigBuffer[256];
    int         LargestString = 0;
    SIZE        Size;
    HDC         hdc;
    int         Count;


    Dbg(BPNextHbpt(&hBpt, bptNext) == BPNOERROR);

    if (hBpt == NULL) {
        /*
        **      No breakpoints so grey out the Clear All button
        */

        EnableWindow( GetDlgItem(hDlg, ID_SETBREAK_CLEARALL), FALSE);
    } else {

        hdc = GetDC(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT) );
        Count = 0;

        while (hBpt != NULL) {
            /*
            **  M00BUG: check to see if it is marked as deleted and
            **          if not then display it
            **          Fix with DeleteBPListbox
            */

            Dbg(BPGetFinalHbpt( hBpt, &hBpt2 ) == BPNOERROR);
            Dbg(BPFormatHbpt( hBpt2, szBigBuffer, sizeof(szBigBuffer), BPFCF_ADD_DELETE|BPFCF_ITEM_COUNT) == BPNOERROR);

            GetTextExtentPoint(hdc, szBigBuffer, strlen(szBigBuffer), &Size );

            if ( Size.cx > LargestString ) {

                LargestString = Size.cx;

                SendMessage(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),
                        LB_SETHORIZONTALEXTENT,
                        (WPARAM)LargestString,
                        0 );
            }

            SendMessage(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),
                  LB_ADDSTRING, 0, (LPARAM)(LPSTR)szBigBuffer);

            Count++;

            /*
            **  Get the next breakpoint
            */

            Dbg(BPNextHbpt(&hBpt, bptNext) == BPNOERROR);
        }

        ReleaseDC(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT), hdc );

        if ( TopIndex != LB_ERR && TopIndex <= Count ) {
            SendMessage( GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),
                         LB_SETTOPINDEX, TopIndex, 0L);
        }
    }

    return;
}                                       /* FillBPListbox() */


/***    AddBPToListBox
**
**  Synopsis:
**      void = AddBPToListBox( hDlg, hBpt )
**
**  Entry:
**      hDlg    - handle of dialog box containning listbox
**      hBpt    - handle of breakpoint to be added
**
**  Returns:
**      Nothing
**
**  Description:
**      Updates the breakpoint list box with the just added breakpoint.
*/

VOID
AddBPToListBox(
    HWND hDlg,
    HBPT hBpt
    )
{
    char szBigBuffer[255];
    WORD wRet = 0;

    Unused(wRet);

    Dbg(BPFormatHbpt( hBpt, szBigBuffer, sizeof(szBigBuffer), BPFCF_ADD_DELETE) == BPNOERROR);
    SendMessage(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),
          LB_RESETCONTENT, 0, 0L);
    FillBPListbox(hDlg);

    // When a BP has been added the OK button becomes the default

    BreakDefPushButton(hDlg, IDOK);

    // Clear All must be valid

    EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_CLEARALL), TRUE);

    return;
}                                       /* AddBPToListBox() */


/***    DeleteBPListbox
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**      Mark the passed bp as deleted and update the breakpoint list box.
*/

VOID
DeleteBPListbox(
    HWND hDlg,
    int BPNum
    )
{
    HBPT        hBpt = 0;

    Dbg(BPNextHbpt( & hBpt, bptNext) == BPNOERROR);

    Assert(hBpt != NULL);           // Must be something or we should not
                                        // attempt to delete anything

    do {
        /**** M00BUG -- Need to do a filter for previously delete breakpoints
        **
        **      Force refill of listbox
        */

        if (BPNum == 0) {

            BPDelete( hBpt );
            SendMessage(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),
                  LB_RESETCONTENT, 0, 0L);

            SendMessage(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),
                  LB_ADDSTRING, 0, (LPARAM)(LPSTR)"");

            SendMessage(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),
                  LB_RESETCONTENT, 0, 0L);

            FillBPListbox(hDlg);
            return;
        }
        /*
        **      Move to the next breakpoint
        */

        BPNum -= 1;
        BPNextHbpt( &hBpt, bptNext);
    } while (hBpt != NULL);

    Assert(FALSE);                      /* Should never get here        */

    return;
Unused(hDlg);
}                                       /* DeleteBPListbox() */

/***    ClearAllBP
**
**  Synopsis:
**      void = ClearAllBP( hDlg )
**
**  Entry:
**      hDlg - handle to dialog box
**
**  Returns:
**      Nothing
**
**  Description:
**      Mark all the bps as deleted and clear the listbox
*/

VOID
ClearAllBP(
    HWND hDlg
    )
{
    BPDeleteAll();

    /*
    **  update the breakpoint list box
    */

    SendMessage(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),  LB_RESETCONTENT, 0, 0L);
    FillBPListbox(hDlg);

    SetFocus(GetDlgItem(hDlg, ID_SETBREAK_ACTION));

    /*
    **  Grey Delete and Clear All
    */

    EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_CLEARALL), FALSE);
    EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_DELETE), FALSE);

    return;
}                                       /* ClearAllBP() */

/***    EnableBPListbox
**
**  Synopsis:
**      void = EnableBPListbox(hDlg, iBP, fEnable)
**
**  Entry:
**      hDlg    - handle to the dialog box
**      iBP     - Index of breakpoint to be enabled/disabled
**      fEnable - enable (TRUE) or diable (FALSE)
**
**  Return:
**      Nothing
**
**  Description:
**      This function is used to either enable or diable an item in the
**      listbox.
**
*/

VOID
EnableBPListbox(
    HWND hDlg,
    int iBP,
    BOOL fEnable
    )
{
    HBPT        hBpt = BPLB_HbptOfI(iBP);

    if (hBpt != NULL) {
        if (fEnable) {
            BPEnable( hBpt );
        } else {
            BPDisable( hBpt );
        }
    }

    return;
}                                       /* EnableBPListbox() */

/***    BPLB_HbptOfI
**
**  Synopsis:
**      hbpt = BPLB_HbptOfI(iBP)
**
**  Entry:
**      iBP     - index of the breakpoint in the listbox to be retrieved
**
**  Returns:
**      Handle of the iBP-th breakpoint in the list of breakpoints -- it is
**      assumed that the list box and the list of breakpoints has a one-to-
**      one correspondence.
**
*/

HBPT
BPLB_HbptOfI(
    int iBP
    )
{
    HBPT        hbpt = 0;

    //  THere must be some record or we should never have been called.

    Dbg(BPNextHbpt(&hbpt, bptNext) == BPNOERROR);
    Assert( hbpt != NULL);

    for (;(iBP > 0) && (hbpt != NULL); iBP-= 1) {
        Dbg( BPNextHbpt( &hbpt, bptNext ) == BPNOERROR );
        Assert( hbpt != NULL);
    }

    return hbpt;
}                                       /* BPLB_HbptOfI() */

/***    OKBP
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**      Delete any BPs marked for deletion, and reset the BP
**      editing flags.  Update the editor screen to reflect any
**      changes.
*/

VOID
OKBP(void)
{
    BPCommit();
}                                       /* OKBP() */

/***    CancelBP
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**      Delete any BPs marked as added and reset the BP editing flags.
*/

VOID
CancelBP(
    VOID
    )
{
    BPUnCommit();

    return;
}                                       /* CancelBP() */


BOOL
BreakFieldsChanged(
    HWND hDlg
    )
{
    HWND        hLocation;
    HWND        hWndProc;
    HWND        hExpression;
    HWND        hLength;
    HWND        hPass;
    HWND        hThread;
    HWND        hProcess;
    HWND        hCommand;
    char        szLocation1[255];
    char        szExpression1[255];
    char        szWndProc1[255];
    char        szPass1[20];
    char        szProcess1[20];
    char        szThread1[20];
    char        szCommand1[255];
    BOOL        Ok = FALSE;

    //
    //  Get the handles to the dialog actions
    //
    hLocation   = GetDlgItem(hDlg, ID_SETBREAK_LOCATION);
    hWndProc    = GetDlgItem(hDlg, ID_SETBREAK_WNDPROC);
    hExpression = GetDlgItem(hDlg, ID_SETBREAK_EXPRESSION);
    hLength     = GetDlgItem(hDlg, ID_SETBREAK_LENGTH);
    hPass       = GetDlgItem(hDlg, ID_SETBREAK_PASS);
    hProcess    = GetDlgItem(hDlg, ID_SETBREAK_PROCESS);
    hThread     = GetDlgItem(hDlg, ID_SETBREAK_THREAD);
    hCommand    = GetDlgItem(hDlg, ID_SETBREAK_CMDS);

    SendMessage( hLocation,     WM_GETTEXT, sizeof(szLocation1  ), (LPARAM)(LPSTR)szLocation1   );
    SendMessage( hExpression,   WM_GETTEXT, sizeof(szExpression1), (LPARAM)(LPSTR)szExpression1 );
    SendMessage( hWndProc,      WM_GETTEXT, sizeof(szWndProc1   ), (LPARAM)(LPSTR)szWndProc1    );
    SendMessage( hPass,         WM_GETTEXT, sizeof(szPass1      ), (LPARAM)(LPSTR)szPass1       );
    SendMessage( hProcess,      WM_GETTEXT, sizeof(szProcess1   ), (LPARAM)(LPSTR)szProcess1    );
    SendMessage( hThread,       WM_GETTEXT, sizeof(szThread1    ), (LPARAM)(LPSTR)szThread1     );
    SendMessage( hCommand,      WM_GETTEXT, sizeof(szCommand1   ), (LPARAM)(LPSTR)szCommand1    );

    if ( strcmp( szLocation  ,szLocation1    ) ||
         strcmp( szExpression,szExpression1  ) ||
         strcmp( szWndProc   ,szWndProc1     ) ||
         strcmp( szPass      ,szPass1        ) ||
         strcmp( szProcess   ,szProcess1     ) ||
         strcmp( szThread    ,szThread1      ) ||
         strcmp( szCommand   ,szCommand1     )
       ) {

        Ok = TRUE;
    }

    return Ok;
}



/***    SetbreakFields
**
**  Synopsis:
**      void = SetbreakFields( hDlg, bpIndex );
**
**  Entry:
**      hDlg    - handle to breakpoint dialog box
**      bpIndex - index of breakpoint to have its fields displayed
**
**  Returns:
**      Nothing
**
**  Description:
**      Set the contents of the various fields to match the passed bpIndex.
*/

VOID
SetbreakFields(
    HWND hDlg,
    int bpIndex
    )
{
    HWND        hAction;
    HWND        hLocation;
    HWND        hWndProc;
    HWND        hExpression;
    HWND        hLength;
    HWND        hPass;
    HWND        hThread;
    HWND        hProcess;
    HWND        hPassLeft;
    HWND        hCommand;
    int         bpType;
    HBPT        hBpt;
    HBPT        hBpt2;
    char        *pWndProc;
    LRESULT     Index;

    //
    //  Get the handles to the dialog actions
    //
    hAction     = GetDlgItem(hDlg, ID_SETBREAK_ACTION);
    hLocation   = GetDlgItem(hDlg, ID_SETBREAK_LOCATION);
    hWndProc    = GetDlgItem(hDlg, ID_SETBREAK_WNDPROC);
    hExpression = GetDlgItem(hDlg, ID_SETBREAK_EXPRESSION);
    hLength     = GetDlgItem(hDlg, ID_SETBREAK_LENGTH);
    hPass       = GetDlgItem(hDlg, ID_SETBREAK_PASS);
    hPassLeft   = GetDlgItem(hDlg, ID_SETBREAK_PASSESLEFTCOUNT);
    hProcess    = GetDlgItem(hDlg, ID_SETBREAK_PROCESS);
    hThread     = GetDlgItem(hDlg, ID_SETBREAK_THREAD);
    hCommand    = GetDlgItem(hDlg, ID_SETBREAK_CMDS);

    //
    //  Initialize strings
    //
    *szLocation     = '\0';
    *szExpression   = '\0';
    *szLength       = '\0';
    *szWndProc      = '\0';
    *szPass         = '\0';
    *szPassLeft     = '\0';
    *szProcess      = '\0';
    *szThread       = '\0';
    *szCommand      = '\0';

    if ( bpIndex == LB_ERR ) {

        //
        //  Nothing selected, clear all and set default action
        //
        bpType = BPLOC;

        SendMessage( hAction, CB_SETCURSEL, bpType, 0L );

        SendMessage( hLocation,     WM_SETTEXT, 0, (LPARAM)(LPSTR)szLocation );
        SendMessage( hExpression,   WM_SETTEXT, 0, (LPARAM)(LPSTR)szExpression );
        SendMessage( hLength,       WM_SETTEXT, 0, (LPARAM)(LPSTR)szLength );
        SendMessage( hWndProc,      WM_SETTEXT, 0, (LPARAM)(LPSTR)szWndProc );
        SendMessage( hPass,         WM_SETTEXT, 0, (LPARAM)(LPSTR)szPass );
        SendMessage( hPassLeft,     WM_SETTEXT, 0, (LPARAM)(LPSTR)szPassLeft );
        SendMessage( hProcess,      WM_SETTEXT, 0, (LPARAM)(LPSTR)szProcess );
        SendMessage( hThread,       WM_SETTEXT, 0, (LPARAM)(LPSTR)szThread );
        SendMessage( hCommand,      WM_SETTEXT, 0, (LPARAM)(LPSTR)szCommand );

    } else {

        //
        //  Get the i-th breakpoint from the breakpoint list
        //
        Dbg( BPNextHbpt( &hBpt2, bptFirst) == BPNOERROR);
        while ( bpIndex-- ) {
            Dbg( BPNextHbpt( &hBpt2, bptNext) == BPNOERROR);
        }

        Assert( hBpt2 != NULL );

        Dbg(BPGetFinalHbpt( hBpt2, &hBpt ) == BPNOERROR);
        BPQueryBPTypeOfHbpt( hBpt2, &bpType );

        //
        //  Set the action field:
        //
        SendMessage(hAction, CB_SETCURSEL, bpType, 0L);

        //
        //  Set the other fields depending on the type:
        //
        switch ( bpType ) {
          case BPLOC:
            BPQueryLocationOfHbpt( hBpt, szLocation, sizeof(szLocation));
            break;

          case BPLOCEXPRTRUE:
            BPQueryLocationOfHbpt( hBpt, szLocation, sizeof(szLocation));
            BPQueryExprOfHbpt( hBpt, szExpression, sizeof(szExpression));
            break;

          case BPLOCEXPRCHGD:
            BPQueryLocationOfHbpt( hBpt, szLocation, sizeof(szLocation));
            BPQueryMemoryOfHbpt( hBpt, szExpression, sizeof(szExpression));
            BPQueryMemorySizeOfHbpt( hBpt, szLength, sizeof(szLength));
            break;

          case BPEXPRTRUE:
            BPQueryExprOfHbpt( hBpt, szExpression, sizeof(szExpression));
            break;

          case BPEXPRCHGD:
            BPQueryMemoryOfHbpt( hBpt, szExpression, sizeof(szExpression));
            BPQueryMemorySizeOfHbpt( hBpt, szLength, sizeof(szLength));
            break;

          case BPWNDPROC:
            BPQueryLocationOfHbpt( hBpt, szWndProc, sizeof(szWndProc));
            break;

          case BPWNDPROCEXPRTRUE:
            BPQueryLocationOfHbpt( hBpt, szWndProc, sizeof(szWndProc));
            BPQueryExprOfHbpt( hBpt, szExpression, sizeof(szExpression));
            break;

          case BPWNDPROCEXPRCHGD:
            BPQueryLocationOfHbpt( hBpt, szWndProc, sizeof(szWndProc));
            BPQueryMemoryOfHbpt( hBpt, szExpression, sizeof(szExpression));
            BPQueryMemorySizeOfHbpt( hBpt, szLength, sizeof(szLength));
            break;

          case BPWNDPROCMSGRCVD:
            BPQueryLocationOfHbpt( hBpt, szWndProc, sizeof(szWndProc));
            BPQueryMessageOfHbpt( hBpt, MsgBuffer, sizeof( MsgBuffer ));
            break;
        }

        if ( *szWndProc ) {
            if ( pWndProc = (PSTR) strchr( (PUCHAR) szWndProc, '}' ) ) {
                pWndProc++;
            } else {
                pWndProc = szWndProc;
            }
            Index = SendMessage( hWndProc, CB_SELECTSTRING, 0, (LPARAM)pWndProc );
            if ( Index == LB_ERR ) {
                SendMessage( hWndProc, CB_ADDSTRING, 0, (LPARAM)pWndProc );
                SendMessage( hWndProc, CB_SELECTSTRING, 0, (LPARAM)pWndProc );
            }
        } else {
            SendMessage( hWndProc,      WM_SETTEXT, 0, (LPARAM)(LPSTR)szWndProc );
        }

        BPQueryPassCntOfHbpt( hBpt, szPass, sizeof(szPass));
        BPQueryPassLeftOfHbpt( hBpt, szPassLeft, sizeof(szPassLeft));
        BPQueryProcessOfHbpt( hBpt, szProcess, sizeof(szProcess));
        BPQueryThreadOfHbpt( hBpt, szThread, sizeof(szThread));
        BPQueryCmdOfHbpt( hBpt, szCommand, sizeof(szCommand) );

        SendMessage( hLocation,     WM_SETTEXT, 0, (LPARAM)(LPSTR)szLocation );
        SendMessage( hExpression,   WM_SETTEXT, 0, (LPARAM)(LPSTR)szExpression );
        SendMessage( hLength,       WM_SETTEXT, 0, (LPARAM)(LPSTR)szLength );
        SendMessage( hPass,         WM_SETTEXT, 0, (LPARAM)(LPSTR)szPass );
        SendMessage( hPassLeft,     WM_SETTEXT, 0, (LPARAM)(LPSTR)szPassLeft );
        SendMessage( hProcess,      WM_SETTEXT, 0, (LPARAM)(LPSTR)szProcess );
        SendMessage( hThread,       WM_SETTEXT, 0, (LPARAM)(LPSTR)szThread );
        SendMessage( hCommand,      WM_SETTEXT, 0, (LPARAM)(LPSTR)szCommand );
    }

    SetbreakControls(hDlg, bpType );

    return;
}



VOID
FillMsgCombo(
    HWND hCombo
    )
{
    // array subscript counter used for accessing the MsgInfo inside the MsgMap structure.
    DWORD dwCurMsg;
    XOSD        xosd;
    LPMESSAGEMAP    MsgMap = NULL;

    xosd = OSDGetMessageMap( LppdCur->hpid, LptdCur->htid, &MsgMap );

    if ( xosd == xosdNone ) {

        DWORD   Msg = 0;

        //LPMESSAGEINFO   MsgInfo;
        //
        //  Check that messages are sorted
        //
        //MsgInfo = MsgMap->MsgInfo;
        //while ( MsgInfo && MsgInfo->MsgText ) {
        //
        //    if ( Msg > MsgInfo->Msg ) {
        //       sprintf( "Msg %s is out of sequence!", MsgInfo->MsgText );
        //       MsgBox(GetActiveWindow(), Bf, MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
        //    }
        //    Msg = MsgInfo->Msg;
        //    MsgInfo++;
        //}

        SendMessage( hCombo, CB_RESETCONTENT, 0, 0L );

        for (dwCurMsg=0; dwCurMsg < MsgMap->dwCount; dwCurMsg++) {
            SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)MsgMap->lpMsgInfo[dwCurMsg].lszMsgText);
        }
    }
}


VOID
SetChooseClass(
    HWND    hDlg,
    BOOL    Enable
    )
{
    SendMessage( GetDlgItem(hDlg, ID_MESSAGE_CHOOSEMESSAGE),
                 BM_SETCHECK, !Enable, 0L );

    EnableWindow(GetDlgItem(hDlg, ID_MESSAGE_MESSAGE), !Enable);

    SendMessage( GetDlgItem(hDlg, ID_MESSAGE_CHOOSECLASS),
                 BM_SETCHECK, Enable, 0L );

    SendMessage( GetDlgItem(hDlg, ID_MESSAGE_CLASSMOUSE),
                 BM_SETCHECK, Enable, 0L );

    SendMessage( GetDlgItem(hDlg, ID_MESSAGE_CLASSWINDOW),
                 BM_SETCHECK, Enable, 0L );

    SendMessage( GetDlgItem(hDlg, ID_MESSAGE_CLASSINPUT),
                 BM_SETCHECK, Enable, 0L );

    SendMessage( GetDlgItem(hDlg, ID_MESSAGE_CLASSSYSTEM),
                 BM_SETCHECK, Enable, 0L );

    SendMessage( GetDlgItem(hDlg, ID_MESSAGE_CLASSINIT),
                 BM_SETCHECK, Enable, 0L );

    SendMessage( GetDlgItem(hDlg, ID_MESSAGE_CLASSCLIPBOARD),
                 BM_SETCHECK, Enable, 0L );

    SendMessage( GetDlgItem(hDlg, ID_MESSAGE_CLASSDDE),
                 BM_SETCHECK, Enable, 0L );

    SendMessage( GetDlgItem(hDlg, ID_MESSAGE_CLASSNONCLIENT),
                 BM_SETCHECK, Enable, 0L );
}


INT_PTR
CALLBACK
DlgMessage(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
/*++

Routine Description:

    Processes mesages for the "Message" dialog box

Arguments:

    The usual

Return Value:

    The usual

--*/
{
    char    Buffer[ MAX_PATH ];

    static DWORD HelpArray[]=
    {
       ID_MESSAGE_CHOOSEMESSAGE, IDH_SINGLE,
       ID_MESSAGE_CHOOSECLASS, IDH_CLASS,
       ID_MESSAGE_MESSAGETITLE, IDH_MSG,
       ID_MESSAGE_MESSAGE, IDH_MSG,
       ID_MESSAGE_CLASSTITLE, IDH_MSGCLASS,
       ID_MESSAGE_CLASSMOUSE, IDH_MSGCLASS,
       ID_MESSAGE_CLASSWINDOW, IDH_MSGCLASS,
       ID_MESSAGE_CLASSINPUT, IDH_MSGCLASS,
       ID_MESSAGE_CLASSSYSTEM, IDH_MSGCLASS,
       ID_MESSAGE_CLASSINIT, IDH_MSGCLASS,
       ID_MESSAGE_CLASSCLIPBOARD, IDH_MSGCLASS,
       ID_MESSAGE_CLASSDDE, IDH_MSGCLASS,
       ID_MESSAGE_CLASSNONCLIENT, IDH_MSGCLASS,
       0, 0
    };

    switch (message) {

        case WM_INITDIALOG:
            MsgBuffer[0] = '\0';
            //
            //  Fill the message combo box
            //
            FillMsgCombo( GetDlgItem(hDlg, ID_MESSAGE_MESSAGE) );

            //
            //  Enable/disable the appropriate stuff.
            //
            SetChooseClass( hDlg, TRUE );
            return TRUE;

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
               (DWORD_PTR)(LPVOID) HelpArray );
            return TRUE;

        case WM_CONTEXTMENU:
            WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
               (DWORD_PTR)(LPVOID) HelpArray );
            return TRUE;

        case WM_COMMAND:

            switch (LOWORD(wParam)) {

                case ID_MESSAGE_CHOOSEMESSAGE:
                    SetChooseClass( hDlg, FALSE );
                    SendMessage( GetDlgItem(hDlg,ID_MESSAGE_MESSAGE), CB_SETCURSEL, 0, (LONG)0 );
                    break;

                case ID_MESSAGE_CHOOSECLASS:
                    SetChooseClass( hDlg, TRUE );
                    break;

                case IDOK:

                    //
                    //  Fill the global MsgBuffer with the
                    //  appropriate message/class string
                    //
                    MsgBuffer[0] = '\0';
                    if ( SendMessage( GetDlgItem( hDlg, ID_MESSAGE_CHOOSEMESSAGE ),
                                      BM_GETCHECK, 0, 0L ) ) {

                        //
                        //  Single message, make sure that a message was selected
                        //  from the message combo.
                        //
                        SendMessage( GetDlgItem(hDlg, ID_MESSAGE_MESSAGE ),
                                     WM_GETTEXT, sizeof( MsgBuffer ), (LPARAM)(LPSTR)MsgBuffer );

                        if ( MsgBuffer[0] == '\0' ) {
                            MessageBeep(0);
                            Dbg(LoadString( g_hInst, DLG_NoMsg, Buffer, sizeof(Buffer)));
                            MsgBox(GetActiveWindow(), Buffer, MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
                            break;
                        }

                    } else {

                        //
                        //  Message class. Check all checkboxes and form
                        //  a class string.
                        //
                        if ( SendMessage( GetDlgItem( hDlg, ID_MESSAGE_CLASSMOUSE ),
                                          BM_GETCHECK, 0, 0L ) ) {

                            strcat( MsgBuffer, "M" );
                        }

                        if ( SendMessage( GetDlgItem( hDlg, ID_MESSAGE_CLASSWINDOW ),
                                          BM_GETCHECK, 0, 0L ) ) {

                            strcat( MsgBuffer, "W" );
                        }

                        if ( SendMessage( GetDlgItem( hDlg, ID_MESSAGE_CLASSINPUT ),
                                          BM_GETCHECK, 0, 0L ) ) {

                            strcat( MsgBuffer, "N" );
                        }

                        if ( SendMessage( GetDlgItem( hDlg, ID_MESSAGE_CLASSSYSTEM ),
                                          BM_GETCHECK, 0, 0L ) ) {

                            strcat( MsgBuffer, "S" );
                        }

                        if ( SendMessage( GetDlgItem( hDlg, ID_MESSAGE_CLASSINIT ),
                                          BM_GETCHECK, 0, 0L ) ) {

                            strcat( MsgBuffer, "I" );
                        }

                        if ( SendMessage( GetDlgItem( hDlg, ID_MESSAGE_CLASSCLIPBOARD ),
                                          BM_GETCHECK, 0, 0L ) ) {

                            strcat( MsgBuffer, "C" );
                        }

                        if ( SendMessage( GetDlgItem( hDlg, ID_MESSAGE_CLASSDDE ),
                                          BM_GETCHECK, 0, 0L ) ) {

                            strcat( MsgBuffer, "D" );
                        }

                        if ( SendMessage( GetDlgItem( hDlg, ID_MESSAGE_CLASSNONCLIENT ),
                                          BM_GETCHECK, 0, 0L ) ) {

                            strcat( MsgBuffer, "Z" );
                        }
                    }

                    EndDialog(hDlg, FALSE);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                case IDWINDBGHELP:
                    Dbg( WinHelp( hDlg, szHelpFileName, HELP_CONTEXT, ID_MESSAGE_HELP) );
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }

    return FALSE;
}

VOID
EnableButtons(
    HWND   hDlg,
    DWORD  LastSelected
    )
{
    EnableWindow( GetDlgItem(hDlg, ID_SETBREAK_DELETE),  FALSE );
    EnableWindow( GetDlgItem(hDlg, ID_SETBREAK_ENABLE),  FALSE );
    EnableWindow( GetDlgItem(hDlg, ID_SETBREAK_DISABLE), FALSE );
    EnableWindow( GetDlgItem(hDlg, ID_SETBREAK_ADD),     TRUE  );
    EnableWindow( GetDlgItem(hDlg, ID_SETBREAK_CHANGE),  LastSelected != LB_ERR);
}


/***    DlgSetBreak
**
**  Synopsis:
**      long = DlgSetBreak(hWnd, msg, wParam, lParam);
**
**  Entry:
**
**  Returns:
**
**  Description:
**      Processes messages for "SETBREAK" dialog box
**              (Debug Setbreakpoint menu Option)
**
**      MESSAGES:
**
**              WM_INITDIALOG - Initialize dialog box
**              WM_COMMAND- Input received
**
*/


INT_PTR
CALLBACK
DlgSetBreak(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    char         szBuffer[MAX_MSG_TXT];
    int          LastAction;
    int          i;
    INT_PTR      dwRetVal;
    HWND         hAction   = GetDlgItem(hDlg, ID_SETBREAK_ACTION);
    HWND         hWndProc  = GetDlgItem(hDlg, ID_SETBREAK_WNDPROC);
    INT_PTR      NumItems;
    BOOL         Ok;
    static int   LastSelected;
    static BOOL  fEdited;
    int          bpType;
    HBPT         hbpt;
    BPSTATUS     bpstat;
    int          BPNum;
    long         ibp;

    static DWORD HelpArray[]=
    {
       ID_SETBREAK_ACTION, IDH_BP,
       ID_SETBREAK_SLOCATION, IDH_LOCATION,
       ID_SETBREAK_LOCATION, IDH_LOCATION,
       ID_SETBREAK_SWNDPROC, IDH_WNDPROC,
       ID_SETBREAK_WNDPROC, IDH_WNDPROC,
       ID_SETBREAK_SEXPRESSION, IDH_EXPR,
       ID_SETBREAK_EXPRESSION, IDH_EXPR,
       ID_SETBREAK_SLENGTH, IDH_LEN,
       ID_SETBREAK_LENGTH, IDH_LEN,
       ID_SETBREAK_MESSAGES, IDH_BPMSG,
       ID_SETBREAK_PASS, IDH_COUNT,
       ID_SETBREAK_PASSESLEFTCOUNT, IDH_LEFT,
       ID_SETBREAK_PROCESS, IDH_PROC,
       ID_SETBREAK_THREAD, IDH_THRD,
       ID_SETBREAK_CMDS, IDH_CMD,
       ID_SETBREAK_BREAKPOINT, IDH_BREAKPTLIST,
       ID_SETBREAK_ADD, IDH_BPADD,
       ID_SETBREAK_DELETE, IDH_BPCLEAR,
       ID_SETBREAK_CLEARALL, IDH_BPCLEARALL,
       ID_SETBREAK_DISABLE, IDH_BPDISABLE,
       ID_SETBREAK_ENABLE, IDH_BPENABLE,
       ID_SETBREAK_CHANGE, IDH_BPMODIFY,
       0, 0
    };

    switch (message) {
    case WM_INITDIALOG:

        LastSelected = LB_ERR;
        TopIndex     = LB_ERR;
        fEdited      = FALSE;

        //
        // Initialise Action combobox
        //
        LastAction = DebuggeeActive() ? DBG_Brk_End_Actions : DBG_Brk_End_Actions - 1;
        for (i = DBG_Brk_Start_Actions; i <= LastAction; i++) {
            Dbg(LoadString(g_hInst, i, (LPSTR)szBuffer, sizeof(szBuffer)));
            dwRetVal = SendMessage(hAction, CB_ADDSTRING, 0, (LPARAM)(LPSTR)szBuffer);
            Dbg((dwRetVal!=CB_ERR)||(dwRetVal!=CB_ERRSPACE));
        }

        //
        // Initialise WndProc combobox
        //
        FillWndProcCombo(hDlg);

        //
        // Set edit control limits
        //
        Dbg(SendDlgItemMessage(hDlg, ID_SETBREAK_LOCATION, EM_LIMITTEXT, MAX_LOCATION-1, 0L));
        Dbg(SendDlgItemMessage(hDlg, ID_SETBREAK_EXPRESSION, EM_LIMITTEXT, MAX_EXPRESSION-1, 0L));
        Dbg(SendDlgItemMessage(hDlg, ID_SETBREAK_WNDPROC, CB_LIMITTEXT, MAX_LOCATION-1, 0L));
        Dbg(SendDlgItemMessage(hDlg, ID_SETBREAK_CMDS, EM_LIMITTEXT, MAX_COMMAND-1, 0L));

        //
        // Initialise control contents:
        //
        // Location field is set to current line number in current
        // editor file
        //
        if (hwndActiveEdit != NULL)
           if (Views[curView].Doc >= 0)
              if (Docs[Views[curView].Doc].docType == DOC_WIN)
                 {

                  char rgch[MAX_PATH];

                  sprintf( rgch, "{,%s,}@", Docs[Views[curView].Doc].szFileName );

                  _itoa( Views[curView].Y+1, rgch + strlen(rgch), 10);
                  SetDlgItemText(hDlg, ID_SETBREAK_LOCATION, rgch);
                 }

        SendDlgItemMessage(hDlg, ID_SETBREAK_WNDPROC, CB_SETCURSEL, 0, 0L);

        //
        // Length
        //
        _itoa((int)(sizeof(BYTE)), szTmp, 10);
        SetDlgItemText(hDlg, ID_SETBREAK_LENGTH, szTmp);

        //
        // Initialise the breakpoint listbox
        //
        FillBPListbox(hDlg);
        NumItems = SendMessage(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),
                               LB_GETCOUNT, 0, 0L);
        if ( NumItems == 0 ) {
            EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT), FALSE );
        }



        //
        // Initialise message and message class specifier
        //

        //
        // Initialise controls according to current selection
        // in action list box.  We start with Break At
        // Location as default.
        //
        SendMessage(hAction, CB_SETCURSEL, BPLOC, 0L);

        SetbreakControls(hDlg, BPLOC);

        SetFocus(hAction);

        return TRUE;

    case WM_DESTROY:
        return TRUE;

    case WU_INFO:
        InformationBox((int) wParam);
        return TRUE;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
           (DWORD_PTR)(LPVOID) HelpArray );
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
           (DWORD_PTR)(LPVOID) HelpArray );
        return TRUE;

    case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
            case ID_SETBREAK_ACTION:
                switch (HIWORD(wParam)) {
                case CBN_SETFOCUS:
                    EnableButtons( hDlg, LastSelected );
                    break;

                case  CBN_SELCHANGE:
                    //
                    // Initialise other controls depending on what
                    // action is selected
                    //
                    bpType = (int) SendMessage(hAction, CB_GETCURSEL, 0, 0L);
                    SetbreakControls(hDlg, bpType);
                    //BreakDefPushButton(hDlg, ID_SETBREAK_ADD);
                    fEdited = TRUE;
                    break;
                }
                break;

            case ID_SETBREAK_LOCATION:
            case ID_SETBREAK_EXPRESSION:
            case ID_SETBREAK_LENGTH:
            case ID_SETBREAK_PASS:
            case ID_SETBREAK_PROCESS:
            case ID_SETBREAK_THREAD:
            case ID_SETBREAK_CMDS:
                switch (HIWORD(wParam)) {
                    case EN_SETFOCUS:
                        EnableButtons( hDlg, LastSelected );
                        break;

                    case EN_CHANGE:
                        EnableButtons( hDlg, LastSelected );
                        fEdited = TRUE;
                        break;
                }
                break;


            case ID_SETBREAK_WNDPROC:
                switch (HIWORD(wParam)) {
                    case CBN_SETFOCUS:
                        EnableButtons( hDlg, LastSelected );
                        break;

                    case CBN_SELCHANGE:
                    case CBN_EDITCHANGE:
                        EnableButtons( hDlg, LastSelected );
                        fEdited = TRUE;
                        break;
                }
                break;

            case ID_SETBREAK_ADD:
SetbreakAdd:
                if (fGetSetbreakControls(hDlg)) {
                    bpstat = BPParse( &hbpt, rgchT, NULL, NULL,
                                         (LppdCur != NULL) ? LppdCur->hpid : 0);

                    if (bpstat != BPNOERROR) {
                        MessageBeep(0);
                    } else {

                        if (DebuggeeActive()) {
                            bpstat = BPBindHbpt( hbpt, NULL );
                        } else {
                            bpstat = BPError;
                        }

                        if ( bpstat == BPCancel ) {
                            MessageBeep(0);
                            Dbg(LoadString( g_hInst, ERR_Breakpoint_Not_Set, szBuffer, sizeof(szBuffer)));
                            MsgBox(GetActiveWindow(), szBuffer, MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
                        } else {
                            Dbg(BPAddToList( hbpt, -1 ) == BPNOERROR);
                            if ( (bpstat != BPNOERROR) && DebuggeeActive() ) {
                                MessageBeep(0);
                                Dbg(LoadString(g_hInst,
                                               ERR_Breakpoint_Not_Instantiated,
                                               szBuffer,
                                               sizeof(szBuffer)));
                                MsgBox(GetActiveWindow(),
                                       szBuffer,
                                   MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
                            }

                            AddBPToListBox( hDlg, hbpt );
                        }

                        EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),
                                                                         TRUE );
                        LastSelected = LB_ERR;
                        SetbreakFields(hDlg, LastSelected);
                        SetFocus(hAction);

                    }
                }

                fEdited = FALSE;

                if (LOWORD(wParam) == IDOK) {
                    goto OKPressed;
                }

                return TRUE;

            case ID_SETBREAK_DELETE:
SetbreakDelete:
                BPNum = LastSelected;

                if (BPNum != LB_ERR) {

                    TopIndex = SendMessage( GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),
                                            LB_GETTOPINDEX, 0, 0L);
                    DeleteBPListbox(hDlg, BPNum);
                    NumItems = SendMessage(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),
                                           LB_GETCOUNT, 0, 0L);
                    if ( NumItems == 0 ) {
                        EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT), FALSE );
                    }

                    //
                    //  After deleting the breakpoint, we must change
                    //  the focus and enable the appropriate buttons.
                    //
                    LastSelected = LB_ERR;
                    SetbreakFields(hDlg, LastSelected);
                    SetFocus(hAction);

                } else {

                    MessageBeep(0);
                }

                fEdited = FALSE;
                return TRUE;

            case ID_SETBREAK_CLEARALL:
                LastSelected = LB_ERR;
                ClearAllBP(hDlg);
                EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT), FALSE );
                fEdited = FALSE;
                return TRUE;

            case ID_SETBREAK_ENABLE:
                if (LastSelected != LB_ERR) {
                    TopIndex = SendMessage( GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),
                                            LB_GETTOPINDEX, 0, 0L);
                    EnableBPListbox(hDlg, LastSelected, TRUE);
                    SendMessage(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),
                                LB_RESETCONTENT, 0, 0L);
                    FillBPListbox(hDlg);
                    LastSelected = LB_ERR;
                    SetbreakFields(hDlg, LastSelected);
                    SetFocus(hAction);
                } else {
                    MessageBeep(0);
                }
                fEdited = FALSE;
                return TRUE;

            case ID_SETBREAK_DISABLE:
                if (LastSelected != LB_ERR) {
                    TopIndex = SendMessage( GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),
                                            LB_GETTOPINDEX, 0, 0L);
                    EnableBPListbox(hDlg, LastSelected, FALSE);
                    SendMessage(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),
                                LB_RESETCONTENT, 0, 0L);
                    FillBPListbox(hDlg);
                    LastSelected = LB_ERR;
                    SetbreakFields(hDlg, LastSelected);
                    SetFocus(hAction);
                } else {
                    MessageBeep(0);
                }
                fEdited = FALSE;
                return TRUE;


            case ID_SETBREAK_CHANGE:
                if ( (LastSelected != LB_ERR) && fGetSetbreakControls(hDlg)) {
                    bpstat = BPParse( &hbpt, rgchT, NULL, NULL, (LppdCur != NULL) ? LppdCur->hpid : 0);
                    if (bpstat == BPNOERROR) {
                        DWORD dwTemp = 0;

                        BPIFromHbpt(&dwTemp, BPLB_HbptOfI(LastSelected));

                        ibp = dwTemp;

                        Dbg(BPChange( hbpt, ibp ) == BPNOERROR);

                        if (DebuggeeActive()) {
                            bpstat = BPBindHbpt( hbpt, NULL );
                        } else {
                            bpstat = BPError;
                        }

                        if ( (bpstat != BPNOERROR) && DebuggeeActive() ) {
                            MessageBeep(0);
                            MsgBox(GetActiveWindow(), "Breakpoint not instantiated", MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
                        }

                        TopIndex = SendMessage( GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),
                                                LB_GETTOPINDEX, 0, 0L);
                        SendMessage(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),
                                    LB_RESETCONTENT, 0, 0L);

                        FillBPListbox(hDlg);
                        LastSelected = LB_ERR;
                        SetbreakFields(hDlg, LastSelected);
                        SetFocus(hAction);

                    } else {
                        MessageBeep(0);
                    }
                }
                fEdited = FALSE;
                return TRUE;


            case ID_SETBREAK_BREAKPOINT:
                switch (HIWORD(wParam)) {
                case LBN_SETFOCUS:

                    LastSelected = (int) SendMessage(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),
                                               LB_GETCURSEL, 0, 0L);

                    if ( LastSelected == LB_ERR ) {
                        NumItems = SendMessage(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT),
                                               LB_GETCOUNT, 0, 0L);
                        if ( NumItems > 0 ) {
                            LastSelected = 0;
                        }
                    }

                    if ( LastSelected != LB_ERR ) {
                        EnableBPButtons( hDlg, LastSelected );
                    }
                    break;


                case LBN_DBLCLK:
                    //
                    // Try to delete
                    //
                    goto SetbreakDelete;

                case LBN_SELCHANGE:
                    LastSelected = (int) SendMessage(GetDlgItem(hDlg, ID_SETBREAK_BREAKPOINT), LB_GETCURSEL, 0, 0L);
                    if ((LastSelected != LB_ERR)) {
                        SetbreakFields(hDlg, LastSelected);
                    }
                    EnableBPButtons( hDlg, LastSelected );
                    break;
                }
                return TRUE;

                //
                // Important.  The tests are done in this order to
                // get around a bug in Windows whereby more than
                // button can be set as the default.  In our case
                // this always the OK button that can be set by
                // mistake so we test this case last.
                //
            case IDOK:
                if ((HWND)lParam == GetDlgItem(hDlg, IDOK)) {
                    goto OKPressed;

                } else if (LOWORD(GetWindowLong(GetDlgItem(hDlg, ID_SETBREAK_DELETE),
                        GWL_STYLE)) == LOWORD(BS_DEFPUSHBUTTON)) {

                    //
                    //  Take the focus out of the list box and set it
                    //  to the breakpoint dialog box.
                    //
                    SetFocus( hDlg );
                    goto SetbreakDelete;

                } else if (LOWORD(GetWindowLong(GetDlgItem(hDlg, ID_SETBREAK_ADD),
                        GWL_STYLE)) == LOWORD(BS_DEFPUSHBUTTON)) {
                    goto SetbreakAdd;

                } else if (LOWORD(GetWindowLong(GetDlgItem(hDlg, IDOK),
                        GWL_STYLE)) == LOWORD(BS_DEFPUSHBUTTON)) {
OKPressed:

                    //
                    //  If Edited fields, confirm before ending the dialog
                    //
                    if ( fEdited && BreakFieldsChanged( hDlg ) ) {
                        goto SetbreakAdd;
                        //Ok = (QuestionBox(ERR_BP_Edited,MB_YESNO) == IDYES);
                    } else {
                        Ok = TRUE;
                    }

                    if ( Ok ) {
                        OKBP();
                        StoreWndProcHistory(hDlg);
                        EndDialog(hDlg, TRUE);
                    }
                }
                return TRUE;

            case IDCANCEL:
                CancelBP();
                EndDialog(hDlg, FALSE);
                fEdited = FALSE;
                return TRUE;

            case ID_SETBREAK_MESSAGES:
                StartDialog( DLG_MESSAGES, DlgMessage );
                break;

            case IDHELP:
                WinHelp( hDlg, "windbg.hlp", HELP_CONTEXT, IDH_BREAKPNTS );
                break;
            }
            break;
        }
    }

    return FALSE;
}                                       /* DlgSetBreak() */

VOID
EnableBPButtons(
    IN  HWND hDlg,
    IN  int  iBP
    )
/*++

Routine Description:

    Enables/disables the "Clear", "Enable" and "Disable" windows
    depending on whether a breakpoint has been marked for deletion
    or not.

Arguments:

    hDlg    -   Supplies the dialog handle
    iBP     -   Supplies the breakpoint index

Return Value:

    None.


--*/
{
    HBPT    hBpt = 0;
    BOOL    Deleted;

    Dbg(BPNextHbpt( &hBpt, bptNext ) == BPNOERROR);

    do {
        if (iBP == 0) {

            Deleted = BPIsMarkedForDeletion( hBpt );

            EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_DELETE),  !Deleted);
            EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_ENABLE),  !Deleted);
            EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_DISABLE), !Deleted);
            EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_CHANGE),  FALSE);
            EnableWindow(GetDlgItem(hDlg, ID_SETBREAK_ADD),     FALSE);
            return;
        }

        iBP -= 1;
        Dbg( BPNextHbpt( &hBpt, bptNext ) == BPNOERROR );

    } while (hBpt != NULL);
}
