/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUDLG.C
 *  WOW32 16-bit User API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wudlg.c);

extern DOSWOWDATA DosWowData;

// SendDlgItemMessage cache
extern HWND  hdlgSDIMCached ;

/*++
    void CheckDlgButton(<hDlg>, <nIDButton>, <wCheck>)
    HWND <hDlg>;
    int <nIDButton>;
    WORD <wCheck>;

    The %CheckDlgButton% function places a checkmark next to or removes a
    checkmark from a button control, or changes the state of a three-state
    button. The %CheckDlgButton% function sends a BM_SETCHECK message to the
    button control that has the specified ID in the given dialog box.

    <hDlg>
        Identifies the dialog box that contains the button.

    <nIDButton>
        Specifies the button control to be modified.

    <wCheck>
        Specifies the action to take. If the <wCheck> parameter is
        nonzero, the %CheckDlgButton% function places a checkmark next to the
        button; if zero, the checkmark is removed. For three-state buttons, if
        <wCheck> is 2, the button is grayed; if <wCheck> is 1, it is checked; if
        <wCheck> is 0, the checkmark is removed.

    This function does not return a value.
--*/

ULONG FASTCALL WU32CheckDlgButton(PVDMFRAME pFrame)
{
    register PCHECKDLGBUTTON16 parg16;

    GETARGPTR(pFrame, sizeof(CHECKDLGBUTTON16), parg16);

    CheckDlgButton(
    HWND32(parg16->f1),
    WORD32(parg16->f2),
    WORD32(parg16->f3)
    );

    FREEARGPTR(parg16);
    RETURN(0);
}


/*++
    void CheckRadioButton(<hDlg>, <nIDFirstButton>, <nIDLastButton>,
        <nIDCheckButton>)
    HWND <hDlg>;
    int <nIDFirstButton>;
    int <nIDLastButton>;
    int <nIDCheckButton>;

    The %CheckRadioButton% function checks the radio button specified by the
    <nIDCheckButton> parameter and removes the checkmark from all other radio
    buttons in the group of buttons specified by the <nIDFirstButton> and
    <nIDLastButton> parameters. The %CheckRadioButton% function sends a
    BM_SETCHECK message to the radio-button control that has the specified ID in
    the given dialog box.

    <hDlg>
        Identifies the dialog box.

    <nIDFirstButton>
        Specifies the integer identifier of the first radio button in the
        group.

    <nIDLastButton>
        Specifies the integer identifier of the last radio button in the
        group.

    <nIDCheckButton>
        Specifies the integer identifier of the radio button to be
        checked.

    This function does not return a value.
--*/

ULONG FASTCALL WU32CheckRadioButton(PVDMFRAME pFrame)
{
    register PCHECKRADIOBUTTON16 parg16;

    GETARGPTR(pFrame, sizeof(CHECKRADIOBUTTON16), parg16);

    CheckRadioButton(
    HWND32(parg16->f1),
    WORD32(parg16->f2),
    WORD32(parg16->f3),
    WORD32(parg16->f4)
    );

    FREEARGPTR(parg16);
    RETURN(0);
}

//***************************************************************************
// HWND    WINAPI CreateDialog(HINSTANCE, LPCSTR, HWND, DLGPROC);
// HWND    WINAPI CreateDialogIndirect(HINSTANCE, const void FAR*, HWND, DLGPROC);
// HWND    WINAPI CreateDialogParam(HINSTANCE, LPCSTR, HWND, DLGPROC, LPARAM);
// HWND    WINAPI CreateDialogIndirectParam(HINSTANCE, const void FAR*, HWND, DLGPROC, LPARAM);
//
// int     WINAPI DialogBox(HINSTANCE, LPCSTR, HWND, DLGPROC);
// int     WINAPI DialogBoxIndirect(HINSTANCE, HGLOBAL, HWND, DLGPROC);
// int     WINAPI DialogBoxParam(HINSTANCE, LPCSTR, HWND, DLGPROC, LPARAM);
// int     WINAPI DialogBoxIndirectParam(HINSTANCE, HGLOBAL, HWND, DLGPROC, LPARAM);
//
// This is a common entry point for all the apis above. We distinguish
// between 'create' and 'dialogbox' apis by a bool flag (parg16->f7).
// TRUE implies 'dialogbox' apis else 'create' apis.
//
//                                                       - nanduri
//***************************************************************************

ULONG FASTCALL WU32DialogBoxParam(PVDMFRAME pFrame)
{
    ULONG    ul;
    PVOID    pDlg;
    DWORD    cb, cb16;
    register PDIALOGBOXPARAM16 parg16;
    BYTE     abT[1024];
    WNDPROC  vpDlgProc = NULL;

    GETARGPTR(pFrame, sizeof(DIALOGBOXPARAM16), parg16);

    if (DWORD32(parg16->f4)) {
        // mark the proc as WOW proc and save the high bits in the RPL
        MarkWOWProc (parg16->f4,vpDlgProc);
    }

    if (!(cb16 = parg16->f6)) {
        cb = ConvertDialog16(NULL, DWORD32(parg16->f2), 0, cb16);
    }
    else {
        // The idea is eliminate a call to ConverDialog16
        //
        // the maximum size that 32bit dlgtemplate would be is twice
        // the 16bit dlgtemplate.
        //
        // this assumption is true cause - we convert most words to dwords
        // and ansi strings to unicode strings - since we know that a
        // DWORD is twice the sizeof a WORD a unicode character is 2bytes
        // therefore maxsize of dlgtemplate cannot exceed cb * 2.
        //
        //                                                      - nanduri

        cb = cb16 * max(sizeof(DWORD) / sizeof(WORD), sizeof(WCHAR)/sizeof(BYTE));
        WOW32ASSERT(cb >= ConvertDialog16(NULL, DWORD32(parg16->f2), 0, cb16));
    }

    pDlg = (cb > sizeof(abT)) ? malloc_w(cb) : (PVOID)abT;
    if (cb && pDlg) {
        cb = ConvertDialog16(pDlg, DWORD32(parg16->f2), cb, cb16);

        if (parg16->f7) {
            ul = GETINT16(DialogBoxIndirectParamAorW(HMODINST32(parg16->f1),
                            pDlg, HWND32(parg16->f3),
                            vpDlgProc,
                            (LPARAM) DWORD32(parg16->f5), SCDLG_ANSI));
        }
        else {
            ul = GETHWND16((pfnOut.pfnServerCreateDialog)(HMODINST32(parg16->f1), (LPDLGTEMPLATE)pDlg,
                            cb,  HWND32(parg16->f3),
                            vpDlgProc,
                            (LPARAM) DWORD32(parg16->f5),  SCDLG_CLIENT | SCDLG_ANSI | SCDLG_NOREVALIDATE));
        }

        if (pDlg != (PVOID)abT) {
            free_w (pDlg);
        }

    }

    // Invalidate SendDlgItemMessage cache
    hdlgSDIMCached = NULL ;

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    int DlgDirList(<hDlg>, <lpPathSpec>, <nIDListBox>, <nIDStaticPath>,
        <wFiletype>)
    HWND <hDlg>;
    LPSTR <lpPathSpec>;
    int <nIDListBox>;
    int <nIDStaticPath>;
    WORD <wFiletype>;

    The %DlgDirList% function fills a list-box control with a file or directory
    listing. It fills the list box specified by the <nIDListBox> parameter with
    the names of all files matching the pathname given by the <lpPathSpec>
    parameter.

    The %DlgDirList% function shows subdirectories enclosed in square brackets
    ([ ]), and shows drives in the form [-<x>-], where <x> is the drive letter.

    The <lpPathSpec> parameter has the following form:

    [drive:] [ [\u]directory[\idirectory]...\u] [filename]

    In this example, <drive> is a drive letter, <directory> is a valid directory
    name, and <filename> is a valid filename that must contain at least one
    wildcard character. The wildcard characters are a question mark (?), meaning
    match any character, and an asterisk (*), meaning match any number of
    characters.

    If the <lpPathSpec> parameter includes a drive and/or directory name, the
    current drive and directory are changed to the designated drive and
    directory before the list box is filled. The text control identified by the
    <nIDStaticPath> parameter is also updated with the new drive and/or
    directory name.

    After the list box is filled, <lpPathSpec> is updated by removing the drive
    and/or directory portion of the pathname.

    %DlgDirList% sends LB_RESETCONTENT and LB_DIR messages to the list box.

    <hDlg>
        Identifies the dialog box that contains the list box.

    <lpPathSpec>
        Points to a pathname string. The string must be a
        null-terminated character string.

    <nIDListBox>
        Specifies the identifier of a list-box control. If <nIDListBox> is
        zero, %DlgDirList% assumes that no list box exists and does not attempt
        to fill it.

    <nIDStaticPath>
        Specifies the identifier of the static-text control used for
        displaying the current drive and directory. If <nIDStaticPath> is zero,
        %DlgDirList% assumes that no such text control is present.

    <wFiletype>
        Specifies the attributes of the files to be displayed. It can be any
        combination of the following values:

    0x0000
        Read/write data files with no additional attributes

    0x0001
        Read-only files

    0x0002
        Hidden files

    0x0004
        System files

    0x0010
        Subdirectories

    0x0020
        Archives

    0x2000
        LB_DIR flag. If the LB_DIR flag is set, Windows places the messages
        generated by %DlgDirList% in the application's queue; otherwise they are
        sent directly to the dialog function.

    0x4000
        Drives

    0x8000
        Exclusive bit. If the exclusive bit is set, only files of the specified
        type are listed. Otherwise, files of the specified type are listed in
        addition to normal files.

    The return value specifies the outcome of the function. It is nonzero if a
    listing was made, even an empty listing. A zero return value implies that
    the input string did not contain a valid search path.

    The <wFiletype> parameter specifies the DOS attributes of the files to be
    listed. Table 4.6 describes these attributes.
--*/

ULONG FASTCALL WU32DlgDirList(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz2;
    register PDLGDIRLIST16 parg16;

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    GETARGPTR(pFrame, sizeof(DLGDIRLIST16), parg16);
    GETPSZPTR(parg16->f2, psz2);

    //
    // KidPix passes an invalid filetype flag (0x1000) that Win3.1 doesn't
    // check for.  Win32 does, and fails the API, so mask that flag off here.
    //  John Vert (jvert) 11-Jun-1993
    //

    ul = GETINT16(DlgDirList(
    HWND32(parg16->f1),
    psz2,
    WORD32(parg16->f3),
    WORD32(parg16->f4),
    WORD32(parg16->f5) & DDL_VALID
    ));

    UpdateDosCurrentDirectory(DIR_NT_TO_DOS);

    FREEPSZPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    int DlgDirListComboBox(<hDlg>, <lpPathSpec>, <nIDComboBox>, <nIDStaticPath>,
        <wFiletype>)
    HWND <hDlg>;
    LPSTR <lpPathSpec>;
    int <nIDComboBox>;
    int <nIDStaticPath>;
    WORD <wFiletype>;

    The %DlgDirListComboBox% function fills the list box of a combo-box control
    with a file or directory listing. It fills the list box of the combo box
    specified by the <nIDComboBox> parameter with the names of all files
    matching the pathname given by the <lpPathSpec> parameter.

    The %DlgDirListComboBox% function shows subdirectories enclosed in square
    brackets ([ ]), and shows drives in the form [-<x>-], where <x> is the drive
    letter.

    The <lpPathSpec> parameter has the following form:

    [drive:] [ [\u]directory[\idirectory]...\u] [filename]

    In this example, <drive> is a drive letter, <directory> is a valid directory
    name, and <filename> is a valid filename that must contain at least one
    wildcard character. The wildcard characters are a question mark (?), meaning
    match any character, and an asterisk (*), meaning match any number of
    characters.

    If the <lpPathSpec> parameter includes a drive and/or directory name, the
    current drive and directory are changed to the designated drive and
    directory before the list box is filled. The text control identified by the
    <nIDStaticPath> parameter is also updated with the new drive and/or
    directory name.

    After the combo-box list box is filled, <lpPathSpec> is updated by removing
    the drive and/or directory portion of the pathname.

    %DlgDirListComboBox% sends CB_RESETCONTENT and CB_DIR messages to the combo
    box.

    <hDlg>
        Identifies the dialog box that contains the combo box.

    <lpPathSpec>
        Points to a pathname string. The string must be a
        null-terminated string.

    <nIDComboBox>
        Specifies the identifier of a combo-box control in a dialog box.
        If <nIDComboBox> is zero, %DlgDirListComboBox% assumes that no combo box
        exists and does not attempt to fill it.

    <nIDStaticPath>
        Specifies the identifier of the static-text control used for
        displaying the current drive and directory. If <nIDStaticPath> is zero,
        %DlgDirListComboBox% assumes that no such text control is present.

    <wFiletype>
        Specifies DOS file attributes of the files to be displayed. It
        can be any combination of the following values:

    The return value specifies the outcome of the function. It is nonzero if a
    listing was made, even an empty listing. A zero return value implies that
    the input string did not contain a valid search path.
--*/

ULONG FASTCALL WU32DlgDirListComboBox(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz2;
    register PDLGDIRLISTCOMBOBOX16 parg16;

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    GETARGPTR(pFrame, sizeof(DLGDIRLISTCOMBOBOX16), parg16);
    GETPSZPTR(parg16->f2, psz2);

    ul = GETINT16(DlgDirListComboBox(
    HWND32(parg16->f1),
    psz2,
    WORD32(parg16->f3),
    WORD32(parg16->f4),
    WORD32(parg16->f5)
    ));

    UpdateDosCurrentDirectory(DIR_NT_TO_DOS);


    FREEPSZPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL DlgDirSelectEx(<hDlg>, <lpString>, <nIDListBox>)
    HWND <hDlg>;
    LPSTR <lpString>;
    int <nIDListBox>;

    The %DlgDirSelectEx% function retrieves the current selection from a list
    box. It assumes that the list box has been filled by the %DlgDirList%
    function and that the selection is a drive letter, a file, or a directory
    name.

    The %DlgDirSelectEx% function copies the selection to the buffer given by the
    <lpString> parameter. If the current selection is a directory name or drive
    letter, %DlgDirSelectEx% removes the enclosing square brackets (and hyphens,
    for drive letters) so that the name or letter is ready to be inserted into a
    new pathname. If there is no selection, <lpString> does not change.

    %DlgDirSelectEx% sends LB_GETCURSEL and LB_GETTEXT messages to the list box.

    <hDlg>
        Identifies the dialog box that contains the list box.

    <lpString>
        Points to a buffer that is to receive the selected pathname.

    <nIDListBox>
        Specifies the integer ID of a list-box control in the dialog box.

    The return value specifies the status of the current list-box selection. It
    is TRUE if the current selection is a directory name. Otherwise, it is
    FALSE.

    The %DlgDirSelectEx% function does not allow more than one filename to be
    returned from a list box.

    The list box must not be a multiple-selection list box. If it is, this
    function will not return a zero value and <lpString> will remain unchanged.
--*/

ULONG FASTCALL WU32DlgDirSelect(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz2;
    VPVOID vp;
    register PDLGDIRSELECT16 parg16;

    GETARGPTR(pFrame, sizeof(DLGDIRSELECT16), parg16);
    ALLOCVDMPTR(parg16->f2, MAX_VDMFILENAME, psz2);
    vp = parg16->f2;

    ul = GETBOOL16(DlgDirSelectEx(
    HWND32(parg16->f1),
    psz2,
    SIZE_BOGUS, 
    WORD32(parg16->f3)
    ));

    // special case to keep common dialog structs in sync (see wcommdlg.c)
    Check_ComDlg_pszptr(CURRENTPTD()->CommDlgTd, vp);

    FLUSHVDMPTR(parg16->f2, strlen(psz2)+1, psz2);
    FREEVDMPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL DlgDirSelectComboBoxEx(<hDlg>, <lpString>, <nIDComboBox>)
    HWND <hDlg>;
    LPSTR <lpString>;
    int <nIDComboBox>;

    The %DlgDirSelectComboBoxEx% function retrieves the current selection from the
    list box of a combo box created with the CBS_SIMPLE style. It cannot be used
    with combo boxes created with either the CBS_DROPDOWN or CBS_DROPDOWNLIST
    style. It assumes that the list box has been filled by the
    %DlgDirListComboBox% function and that the selection is a drive letter, a
    file, or a directory name.

    The %DlgDirSelectComboBoxEx% function copies the selection to the buffer given
    by the <lpString> parameter. If the current selection is a directory name or
    drive letter, %DlgDirSelectComboBoxEx% removes the enclosing square brackets
    (and hyphens, for drive letters) so that the name or letter is ready to be
    inserted into a new pathname. If there is no selection, <lpString> does not
    change.

    %DlgDirSelectComboBoxEx% sends CB_GETCURSEL and CB_GETLBTEXT messages to the
    combo box.

    <hDlg>
        Identifies the dialog box that contains the combo box.

    <lpString>
        Points to a buffer that is to receive the selected pathname.

    <nIDComboBox>
        Specifies the integer ID of the combo-box control in the dialog
        box.

    The return value specifies the status of the current combo-box selection. It
    is TRUE if the current selection is a directory name. Otherwise, it is
    FALSE.

    The %DlgDirSelectComboBoxEx% function does not allow more than one filename to
    be returned from a combo box.
--*/

ULONG FASTCALL WU32DlgDirSelectComboBox(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz2;
    VPVOID vp;
    register PDLGDIRSELECTCOMBOBOX16 parg16;

    GETARGPTR(pFrame, sizeof(DLGDIRSELECTCOMBOBOX16), parg16);
    ALLOCVDMPTR(parg16->f2, MAX_VDMFILENAME, psz2);
    vp = parg16->f2;

    ul = GETBOOL16(DlgDirSelectComboBoxEx(
    HWND32(parg16->f1),
    psz2,
    SIZE_BOGUS,
    WORD32(parg16->f3)
    ));

    // special case to keep common dialog structs in sync (see wcommdlg.c)
    Check_ComDlg_pszptr(CURRENTPTD()->CommDlgTd, vp);

    FLUSHVDMPTR(parg16->f2, strlen(psz2)+1, psz2);
    FREEVDMPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    void EndDialog(<hDlg>, <nResult>)
    HWND <hDlg>;
    int <nResult>;

    The %EndDialog% function terminates a modal dialog box and returns the given
    result to the %DialogBox% function that created the dialog box. The
    %EndDialog% function is required to complete processing whenever the
    %DialogBox% function is used to create a modal dialog box. The function must
    be used in the dialog function of the modal dialog box and should not be
    used for any other purpose.

    The dialog function can call %EndDialog% at any time, even during the
    processing of the WM_INITDIALOG message. If called during the WM_INITDIALOG
    message, the dialog box is terminated before it is shown or before the input
    focus is set.

    %EndDialog% does not terminate the dialog box immediately. Instead, it sets
    a flag that directs the dialog box to terminate as soon as the dialog
    function ends. The %EndDialog% function returns to the dialog function, so
    the dialog function must return control to Windows.

    <hDlg>
        Identifies the dialog box to be destroyed.

    <nResult>
        Specifies the value to be returned from the dialog box to the
        %DialogBox% function that created it.

    This function does not return a value.
--*/

ULONG FASTCALL WU32EndDialog(PVDMFRAME pFrame)
{
    HWND     hwnd;
    register PENDDIALOG16 parg16;

    GETARGPTR(pFrame, sizeof(ENDDIALOG16), parg16);

    hwnd = HWND32(parg16->f1);

    EndDialog(hwnd, INT32(parg16->f2));

    FREEARGPTR(parg16);
    RETURN(0);
}


/*++
    LONG GetDialogBaseUnits(VOID)

    The %GetDialogBaseUnits% function returns the dialog base units used by
    Windows when creating dialog boxes. An application should use these values
    to calculate the average width of characters in the system font.

    This function has no parameters.

    The return value specifies the dialog base units. The high-order word
    contains the height in pixels of the current dialog base height unit derived
    from the height of the system font, and the low-order word contains the
    width in pixels of the current dialog base width unit derived from the width
    of the system font.

    The values returned represent dialog base units before being scaled to
    actual dialog units. The actual dialog unit in the <x> direction is
    1/4th of the width returned by %GetDialogBaseUnits%. The actual dialog
    unit in the <y> direction is 1/8th of the height returned by the
    function.

    To determine the actual height and width in pixels of a control, given the
    height (x) and width (y) in dialog units and the return value
    (lDlgBaseUnits) from calling %GetDialogBaseUnits%, use the following
    formula:

    (x * LOWORD(lDlgBaseUnits))/4
    (y * HIWORD(lDlgBaseUnits))/8

    To avoid rounding problems, perform the multiplication before the division
    in case the dialog base units are not evenly divisible by four.
--*/

ULONG FASTCALL WU32GetDialogBaseUnits(PVDMFRAME pFrame)
{
    ULONG ul;

    UNREFERENCED_PARAMETER(pFrame);

    ul = GETLONG16(GetDialogBaseUnits());

    RETURN(ul);
}


/*++
    int GetDlgCtrlID(<hwnd>)
    HWND <hwnd>;

    The %GetDlgCtrlID% function returns the ID value of the child window
    identified by the <hwnd> parameter.

    <hwnd>
        Identifies the child window.

    The return value is the numeric identifier of the child window if the
    function is successful. If the function fails, or if <hwnd> is not a valid
    window handle, the return value is NULL.

    Since top-level windows do not have an ID value, the return value of this
    function is invalid if the <hwnd> parameter identifies a top-level window.
--*/

ULONG FASTCALL WU32GetDlgCtrlID(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETDLGCTRLID16 parg16;

    GETARGPTR(pFrame, sizeof(GETDLGCTRLID16), parg16);

    ul = GETINT16(GetDlgCtrlID(
    HWND32(parg16->f1)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    WORD GetDlgItemInt(<hDlg>, <nIDDlgItem>, <lpTranslated>, <bSigned>)
    HWND <hDlg>;
    int <nIDDlgItem>;
    BOOL FAR *<lpTranslated>;
    BOOL <bSigned>;

    The %GetDlgItemInt% function translates the text of a control in the given
    dialog box into an integer value. The %GetDlgItemInt% function retrieves the
    text of the control identified by the <nIDDlgItem> parameter. It translates
    the text by stripping any extra spaces at the beginning of the text and
    converting decimal digits, stopping the translation when it reaches the end
    of the text or encounters any nonnumeric character. If the <bSigned>
    parameter is TRUE, %GetDlgItemInt% checks for a minus sign (-) at the
    beginning of the text and translates the text into a signed number.
    Otherwise, it creates an unsigned value.

    %GetDlgItemInt% returns zero if the translated number is greater than 32,767
    (for signed numbers) or 65,535 (for unsigned). When errors occur, such as
    encountering nonnumeric characters and exceeding the given maximum,
    %GetDlgItemInt% copies zero to the location pointed to by the <lpTranslated>
    parameter. If there are no errors, <lpTranslated> receives a nonzero value.
    If <lpTranslated> is NULL, %GetDlgItemInt% does not warn about errors.
    %GetDlgItemInt% sends a WM_GETTEXT message to the control.

    <hDlg>
        Identifies the dialog box.

    <nIDDlgItem>
        Specifies the integer identifier of the dialog-box item to be
        translated.

    <lpTranslated>
        Points to the Boolean variable that is to receive the
        translated flag.

    <bSigned>
        Specifies whether the value to be retrieved is signed.

    The return value specifies the translated value of the dialog-box item text.
    Since zero is a valid return value, the <lpTranslated> parameter must be
    used to detect errors. If a signed return value is desired, it should be
    cast as an %int% type.
--*/

ULONG FASTCALL WU32GetDlgItemInt(PVDMFRAME pFrame)
{
    ULONG ul;
    BOOL t3;
    register PGETDLGITEMINT16 parg16;

    GETARGPTR(pFrame, sizeof(GETDLGITEMINT16), parg16);

    ul = GETWORD16(GetDlgItemInt(
    HWND32(parg16->f1),
    WORD32(parg16->f2),     // see comment in wu32getdlgitem
    &t3,
    BOOL32(parg16->f4)
    ));

    PUTBOOL16(parg16->f3, t3);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    int GetDlgItemText(<hDlg>, <nIDDlgItem>, <lpString>, <nMaxCount>)
    HWND <hDlg>;
    int <nIDDlgItem>;
    LPSTR <lpString>;
    int <nMaxCount>;

    The %GetDlgItemText% function retrieves the caption or text associated with
    a control in a dialog box. The %GetDlgItemText% function copies the text to
    the location pointed to by the <lpString> parameter and returns a count of
    the number of characters it copies.

    %GetDlgItemText% sends a WM_GETTEXT message to the control.

    <hDlg>
        Identifies the dialog box that contains the control.

    <nIDDlgItem>
        Specifies the integer identifier of the dialog-box item whose
        caption or text is to be retrieved.

    <lpString>
        Points to the buffer to receive the text.

    <nMaxCount>
        Specifies the maximum length (in bytes) of the string to be copied
        to <lpString>. If the string is longer than <nMaxCount>, it is
        truncated.

    The return value specifies the actual number of characters copied to the
    buffer. It is zero if no text is copied.
--*/

ULONG FASTCALL WU32GetDlgItemText(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz3;
    VPVOID vp;
    register PGETDLGITEMTEXT16 parg16;

    GETARGPTR(pFrame, sizeof(GETDLGITEMTEXT16), parg16);
    ALLOCVDMPTR(parg16->f3, parg16->f4, psz3);
    vp = parg16->f3;

    ul = GETINT16(GetDlgItemText(
    HWND32(parg16->f1),
    WORD32(parg16->f2), // see comment in wu32getdlgitem
    psz3,
    WORD32(parg16->f4)
    ));

    // special case to keep common dialog structs in sync (see wcommdlg.c)
    Check_ComDlg_pszptr(CURRENTPTD()->CommDlgTd, vp);

    FLUSHVDMPTR(parg16->f3, strlen(psz3)+1, psz3);
    FREEVDMPTR(psz3);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    HWND GetNextDlgGroupItem(<hDlg>, <hCtl>, <bPrevious>)
    HWND <hDlg>;
    HWND <hCtl>;
    BOOL <bPrevious>;

    The %GetNextDlgGroupItem% function searches for the next (or previous)
    control within a group of controls in the dialog box identified by the
    <hDlg> parameter. A group of controls consists of one or more controls with
    WS_GROUP style.

    <hDlg>
        Identifies the dialog box being searched.

    <hCtl>
        Identifies the control in the dialog box where the search starts.

    <bPrevious>
        Specifies how the function is to search the group of controls in the
        dialog box. If the <bPrevious> parameter is zero, the function searches
        for the previous control in the group. If -<bPrevious> is TRUE, the
        function searches for the next control in the group.

    The return value identifies the next or previous control in the group.

    If the current item is the last item in the group and <bPrevious> is FALSE,
    the %GetNextDlgGroupItem% function returns the window handle of the first
    item in the group. If the current item is the first item in the group and
    <bPrevious> is TRUE, %GetNextDlgGroupItem% returns the window handle of the
    last item in the group.
--*/

ULONG FASTCALL WU32GetNextDlgGroupItem(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETNEXTDLGGROUPITEM16 parg16;

    GETARGPTR(pFrame, sizeof(GETNEXTDLGGROUPITEM16), parg16);

    ul = GETHWND16(GetNextDlgGroupItem(HWND32(parg16->f1),
                                       HWND32(parg16->f2),
                                       BOOL32(parg16->f3)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    HWND GetNextDlgTabItem(<hDlg>, <hCtl>, <bPrevious>)
    HWND <hDlg>;
    HWND <hCtl>;
    BOOL <bPrevious>;

    The %GetNextDlgTabItem% function obtains the handle of the first control
    that has the WS_TABSTOP style that precedes (or follows) the control
    identified by the <hCtl> parameter.

    <hDlg>
        Identifies the dialog box being searched.

    <hCtl>
        Identifies the control to be used as a starting point for the
        search.

    <bPrevious>
        Specifies how the function is to search the dialog box. If the
        <bPrevious> parameter is FALSE, the function searches for the previous
        control in the dialog box. If <bPrevious> is TRUE, the function searches
        for the next control in the dialog box. Identifies the control to be
        used as a starting point for the search.

    The return value identifies the previous (or next) control that has the
    WS_TABSTOP style set.
--*/

ULONG FASTCALL WU32GetNextDlgTabItem(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETNEXTDLGTABITEM16 parg16;

    GETARGPTR(pFrame, sizeof(GETNEXTDLGTABITEM16), parg16);

    ul = GETHWND16(GetNextDlgTabItem(HWND32(parg16->f1),
                                     HWND32(parg16->f2),
                                     BOOL32(parg16->f3)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL IsDialogMessage(<hDlg>, <lpMsg>)
    HWND <hDlg>;
    LPMSG <lpMsg>;

    The %IsDialogMessage% function determines whether the given message is
    intended for the modeless dialog box specified by the <hDlg> parameter, and
    automatically processes the message if it is. When the %IsDialogMessage%
    function processes a message, it checks for keyboard messages and converts
    them into selection commands for the corresponding dialog box. For example,
    the ^TAB^ key selects the next control or group of controls, and the ^DOWN^
    key selects the next control in a group.

    If a message is processed by %IsDialogMessage%, it must not be passed to the
    %TranslateMessage% or %DispatchMessage% function. This is because
    %IsDialogMessage% performs all necessary translating and dispatching of
    messages.

    %IsDialogMessage% sends WM_GETDLGCODE messages to the dialog function to
    determine which keys should be processed.

    <hDlg>
        Identifies the dialog box.

    <lpMsg>
        Points to an %MSG% structure that contains the message to
        be checked.

    The return value specifies whether or not the given message has been
    processed. It is TRUE if the message has been processed. Otherwise, it is
    FALSE.

    Although %IsDialogMessage% is intended for modeless dialog boxes, it can be
    used with any window that contains controls to provide the same keyboard
    selection as in a dialog box.
--*/

ULONG FASTCALL WU32IsDialogMessage(PVDMFRAME pFrame)
{
    ULONG ul;
    MSG t2;
    register PISDIALOGMESSAGE16 parg16;
    MSGPARAMEX mpex;
    PMSG16 pMsg16;

    GETARGPTR(pFrame, sizeof(ISDIALOGMESSAGE16), parg16);
    GETMISCPTR(parg16->f2, pMsg16);

    mpex.Parm16.WndProc.hwnd = pMsg16->hwnd;
    mpex.Parm16.WndProc.wMsg = pMsg16->message;
    mpex.Parm16.WndProc.wParam = pMsg16->wParam;
    mpex.Parm16.WndProc.lParam = pMsg16->lParam;
    mpex.iMsgThunkClass = WOWCLASS_WIN16;

    ThunkMsg16(&mpex);

    GETFRAMEPTR(((PTD)CURRENTPTD())->vpStack, pFrame);
    GETARGPTR(pFrame, sizeof(ISDIALOGMESSAGE16), parg16);

    t2.message   = mpex.uMsg;
    t2.wParam    = mpex.uParam;
    t2.lParam    = mpex.lParam;
    t2.hwnd      = HWND32(FETCHWORD(pMsg16->hwnd));
    t2.time      = FETCHLONG(pMsg16->time);
    t2.pt.x      = FETCHSHORT(pMsg16->pt.x);
    t2.pt.y      = FETCHSHORT(pMsg16->pt.y);

    ul = GETBOOL16(IsDialogMessage(
    HWND32(parg16->f1),
    &t2
    ));

    if (MSG16NEEDSTHUNKING(&mpex)) {
        mpex.uMsg   = t2.message;
        mpex.uParam = t2.wParam;
        mpex.lParam = t2.lParam;
        (mpex.lpfnUnThunk16)(&mpex);
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    WORD IsDlgButtonChecked(<hDlg>, <nIDButton>)
    HWND <hDlg>;
    int <nIDButton>;

    The %IsDlgButtonChecked% function determines whether a button control has a
    checkmark next to it, and whether a three-state button control is grayed,
    checked, or neither. The %IsDlgButtonChecked% function sends a BM_GETCHECK
    message to the button control.

    <hDlg>
        Identifies the dialog box that contains the button control.

    <nIDButton>
        Specifies the integer identifier of the button control.

    The return value specifies the outcome of the function. It is nonzero if the
    given control has a checkmark next to it. Otherwise, it is zero. For
    three-state buttons, the return value is 2 if the button is grayed, 1 if the
    button has a checkmark next to it, and zero otherwise.
--*/

ULONG FASTCALL WU32IsDlgButtonChecked(PVDMFRAME pFrame)
{
    ULONG ul;
    register PISDLGBUTTONCHECKED16 parg16;

    GETARGPTR(pFrame, sizeof(ISDLGBUTTONCHECKED16), parg16);

    ul = GETWORD16(IsDlgButtonChecked(
    HWND32(parg16->f1),
    WORD32(parg16->f2)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    void MapDialogRect(<hDlg>, <lpRect>)
    HDLG <hDlg>;
    LPRECT <lpRect>;

    The %MapDialogRect% function converts the dialog-box units given in the
    <lpRect> parameter to screen units. Dialog-box units are stated in terms of
    the current dialog base unit derived from the average width and height of
    characters in the system font. One horizontal unit is one-fourth of the
    dialog base width unit, and one vertical unit is one-eighth of the dialog
    base height unit. The %GetDialogBaseUnits% function returns the dialog base
    units in pixels.

    The %MapDialogRect% function replaces the dialog-box units in <lpRect> with
    screen units (pixels), so that the rectangle can be used to create a dialog
    box or position a control within a box.

    <hDlg>
        Identifies a dialog box.

    <lpRect>
        Points to a %RECT% structure that contains the dialog-box
        coordinates to be converted.

    This function does not return a value.

    The <hDlg> parameter must be created by using the %CreateDialog% or
    %DialogBox% function.
--*/

ULONG FASTCALL WU32MapDialogRect(PVDMFRAME pFrame)
{
    RECT t2;
    register PMAPDIALOGRECT16 parg16;

    GETARGPTR(pFrame, sizeof(MAPDIALOGRECT16), parg16);
    WOW32VERIFY(GETRECT16(parg16->f2, &t2));

    MapDialogRect(
        HWND32(parg16->f1),
        &t2
        );

    PUTRECT16(parg16->f2, &t2);
    FREEARGPTR(parg16);
    RETURN(0);
}


/*++
    int MessageBox(<hwndParent>, <lpText>, <lpCaption>, <wType>)
    HWND <hwndParent>;
    LPSTR <lpText>;
    LPSTR <lpCaption>;
    WORD <wType>;

    The %MessageBox% function creates and displays a window that contains an
    application-supplied message and caption, plus any combination of the
    predefined icons and push buttons described in the following list.

    <hwndParent>
        Identifies the window that owns the message box.

    <lpText>
        Points to a null-terminated string containing the message to be
        displayed.

    <lpCaption>
        Points to a null-terminated string to be used for the dialog-box
        caption. If the <lpCaption> parameter is NULL, the default caption Error
        is used.

    <wType>
        Specifies the contents of the dialog box. It can be any
        combination of the following values:

    MB_ABORTRETRYIGNORE
        Message box contains three push buttons: Abort, Retry, and Ignore.

    MB_APPLMODAL
        The user must respond to the message box before continuing work in the
        window identified by the <hwndParent> parameter. However, the user can
        move to the windows of other applications and work in those windows.
        MB_APPLMODAL is the default if neither MB_SYSTEMMODAL nor MB_TASKMODAL
        are specified.

    MB_DEFBUTTON1
        First button is the default. Note that the first button is always the
        default unless MB_DEFBUTTON2 or MB_DEFBUTTON3 is specified.

    MB_DEFBUTTON2
        Second button is the default.

    MB_DEFBUTTON3
        Third button is the default.

    MB_ICONASTERISK
        Same as MB_ICONINFORMATION.

    MB_ICONEXCLAMATION
        An exclamation-point icon appears in the message box.

    MB_ICONHAND
        Same as MB_ICONSTOP.

    MB_ICONINFORMATION
        An icon consisting of a lowercase i in a circle appears in the message
        box.

    MB_ICONQUESTION
        A question-mark icon appears in the message box.

    MB_ICONSTOP
        A stop sign icon appears in the message box.

    MB_OK
        Message box contains one push button: OK.

    MB_OKCANCEL
        Message box contains two push buttons: OK and Cancel.

    MB_RETRYCANCEL
        Message box contains two push buttons: Retry and Cancel.

    MB_SYSTEMMODAL
        All applications are suspended until the user responds to the message
        box. Unless the application specifies MB_ICONHAND, the message box does
        not become modal until after it is created; consequently, the parent
        window and other windows continue to receive messages resulting from its
        activation. System-modal message boxes are used to notify the user of
        serious, potentially damaging errors that require immediate attention
        (for example, running out of memory).

    MB_TASKMODAL
        Same as MB_APPMODAL except that all the top-level windows belonging to
        the current task are disabled if the <hwndOwner> parameter is NULL. This
        flag should be used when the calling application or library does not
        have a window handle available, but still needs to prevent input to
        other windows in the current application without suspending other
        applications.

    MB_YESNO
        Message box contains two push buttons: Yes and No.

    MB_YESNOCANCEL
        Message box contains three push buttons: Yes, No, and Cancel.

    The return value specifies the outcome of the function. It is zero if there
    is not enough memory to create the message box. Otherwise, it is one of the
    following menu-item values returned by the dialog box:

    IDABORT   Abort button pressed.
    IDCANCEL  Cancel button pressed.
    IDIGNORE  Ignore button pressed.
    IDNO      No button pressed.
    IDOK      OK button pressed.
    IDRETRY   Retry button pressed.
    IDYES     Yes button pressed.

    If a message box has a Cancel button, the IDCANCEL value will be returned if
    either the ^ESCAPE^ key or Cancel button is pressed. If the message box has
    no Cancel button, pressing the ^ESCAPE^ key has no effect.

    When a system-modal message box is created to indicate that the system is
    low on memory, the strings passed as the <lpText> and <lpCaption> parameters
    should not be taken from a resource file, since an attempt to load the
    resource may fail.

    When an application calls the %MessageBox% function and specifies the
    MB_ICONHAND and MB_SYSTEMMODAL flags for the <wType> parameter, Windows will
    display the resulting message box regardless of available memory. When these
    flags are specified, Windows limits the length of the message-box text to
    one line.

    If a message box is created while a dialog box is present, use the handle of
    the dialog box as the <hwndParent> parameter. The <hwndParent> parameter
    should not identify a child window, such as a dialog-box control.
--*/

ULONG FASTCALL WU32MessageBox(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz2;
    PSZ psz3;
    register PMESSAGEBOX16 parg16;

    GETARGPTR(pFrame, sizeof(MESSAGEBOX16), parg16);
    GETPSZPTR(parg16->f2, psz2);
    GETPSZPTR(parg16->f3, psz3);

    ul = GETINT16(MessageBox(
    HWND32(parg16->f1),
    psz2,
    psz3,
    WORD32(parg16->f4)
    ));

    FREEPSZPTR(psz2);
    FREEPSZPTR(psz3);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    void SetDlgItemInt(<hDlg>, <nIDDlgItem>, <wValue>, <bSigned>)
    HWND <hDlg>;
    int <nIDDlgItem>;
    WORD <wValue>;
    BOOL <bSigned>;

    The %SetDlgItemInt% function sets the text of a control in the given dialog
    box to the string that represents the integer value given by the <wValue>
    parameter. The %SetDlgItemInt% function converts <wValue> to a string that
    consists of decimal digits, and then copies the string to the control. If
    the <bSigned> parameter is TRUE, <wValue> is assumed to be signed. If
    <wValue> is signed and less than zero, the function places a minus sign
    before the first digit in the string.

    %SetDlgItemInt% sends a WM_SETTEXT message to the given control.

    <hDlg>
        Identifies the dialog box that contains the control.

    <nIDDlgItem>
        Specifies the control to be modified.

    <wValue>
        Specifies the value to be set.

    <bSigned>
        Specifies whether or not the integer value is signed.

    This function does not return a value.
--*/

ULONG FASTCALL WU32SetDlgItemInt(PVDMFRAME pFrame)
{
    HWND     hwnd;
    register PSETDLGITEMINT16 parg16;

    GETARGPTR(pFrame, sizeof(SETDLGITEMINT16), parg16);

    hwnd = HWND32(parg16->f1);

    SetDlgItemInt(
        hwnd,
        WORD32(parg16->f2),         // see comment in wu32getdlgitem
        (parg16->f4) ? INT32(parg16->f3) : WORD32(parg16->f3),
        BOOL32(parg16->f4)
        );

    FREEARGPTR(parg16);
    RETURN(0);
}


/*++
    void SetDlgItemText(<hDlg>, <nIDDlgItem>, <lpString>)
    HWND <hDlg>;
    int <nIDDlgItem>;
    LPSTR <lpString>;

    The %SetDlgItemText% function sets the caption or text of a control in the
    dialog box specified by the <hDlg> parameter. The %SetDlgItemText% function
    sends a WM_SETTEXT message to the given control.

    <hDlg>
        Identifies the dialog box that contains the control.

    <nIDDlgItem>
        Specifies the control whose text is to be set.

    <lpString>
        Points to the null-terminated string that is to be copied to the
        control.

    This function does not return a value.
--*/

ULONG FASTCALL WU32SetDlgItemText(PVDMFRAME pFrame)
{
    HWND hwnd;
    PSZ psz3;
    register PSETDLGITEMTEXT16 parg16;

    GETARGPTR(pFrame, sizeof(SETDLGITEMTEXT16), parg16);
    GETPSZPTR(parg16->f3, psz3);

    hwnd = HWND32(parg16->f1);

    SetDlgItemText(
    hwnd,
    WORD32(parg16->f2),     // see comment in wu32getdlgitem
    psz3
    );

    FREEPSZPTR(psz3);
    FREEARGPTR(parg16);
    RETURN(0);
}


/*++
    No REF header file
--*/

ULONG FASTCALL WU32SysErrorBox(PVDMFRAME pFrame)
{
    DWORD dwExitCode;
    PSZ pszText;
    PSZ pszCaption;
    register PSYSERRORBOX16 parg16;

    GETARGPTR(pFrame, sizeof(SYSERRORBOX16), parg16);

    // WARNING - If things go wrong during boot, this routine can be called in
    // real mode (v86 mode).   So be very careful which GetPtr routines you
    // use to convert from 16:16 to flat pointers

    pszText = WOWGetVDMPointer(FETCHDWORD(parg16->vpszText),0,fWowMode);
    pszCaption = WOWGetVDMPointer(FETCHDWORD(parg16->vpszCaption),0,fWowMode);

    LOGDEBUG(5,("    SYSERRORBOX: %s\n", pszText));

    dwExitCode = WOWSysErrorBox(
                     pszCaption,
                     pszText,
                     parg16->sBtn1,
                     parg16->sBtn2,
                     parg16->sBtn3
                     );

    FREEPSZPTR(pszCaption);
    FREEPSZPTR(pszText);
    FREEARGPTR(parg16);
    RETURN(dwExitCode);
}
