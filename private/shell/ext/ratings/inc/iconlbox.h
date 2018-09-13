/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1993                         **/
/***************************************************************************/

/****************************************************************************

    ICONLBOX.H

    Header file for IconListBox class (hard tabs at 4)

    May 93,         JimH (combobox support added by VladS)

    IconListBox is constructed by specifying the application's hInst and
    the ID of the listbox control.  All the properties of the listbox (font,
    single or muliple selection, etc.) are defined in the .RC file.  It must
    include the LBS_OWNERDRAWFIXED style.

    Icons are really bitmaps or parts of bitmaps.  They must all be the same
    size.  If they are not 16 by 16 this size must be specified in the
    constructor.  By default, bright green (RGB(0,255,0)) is the transparent
    color but this can be overridden for each individual icon.

    Construction of the IconListBox object should occur when the dialog is
    created.  You cannot wait until WM_INITDIALOG time because you need to
    handle the WM_MEASUREITEM message which comes first by passing to the
    constructed IconListBox.  Similarly, the IconListBox cannot be destructed
    during OK or Cancel handling because you will have to handle subsequent
    WM_DRAWITEM messages.

    Follow these steps to use IconListBox (assumes 16 by 16 icons)

    1.  Construct IconListBox(hInst, IDC_MYLISTBOX);

    2.  Handle WM_MEASUREITEM by calling

        mylistbox->SetHeight(hwndDlg, (MEASUREITESTRUCT FAR *)lParam);

        You MUST do this step even if you are using the default height of 16
        pixels.

    3.  During WM_INITDIALOG processing, register each possible icon by
        specifying your internal identifier for this icon, a bitmap id, and
        if necessary, the x and y offset in the bitmap of this icon.
        Don't load the bitmap yourself.  Just pass the .RC file id.

        mylistbox->RegisterIcon(typeFission, IDB_MushroomCloud);

        Note that you can register several icons from the same .RC file
        bitmap.  These may or may not have different x and y offsets but
        they must all have the same transparent color.  There is no
        extra overhead associated with reusing bitmaps or parts of bitmaps.

    4   Also during WM_INITDIALOG, add the listbox entries.

        mylistbox->AddString(typeColdFusion, "Pons & Fleishman");

    5.  Handle the WM_DRAWITEM message by calling

        if (wParam == IDC_MYLISTBOX)
            mylistbox->DrawItem((DRAWITEMSTRUCT FAR *)lParam);

    6.  When handling OK, do whatever listbox stuff you have to do.

        n = mylistbox->GetCurrentSelection();
        mylistbox->GetString(n, pbuffer);

    7.  Destruct IconListBox

    Inline wrappers for common listbox functions are at the end of this file.

****************************************************************************/

#ifndef _ICONLBOX_H_
#define _ICONLBOX_H_

struct IconList {
    int             nID;                // ID icon was registered with
    int             nResID;             // .RC file resource ID
    int             x, y;               // offset within specified icon
    HBITMAP         hbmSelected;
    HBITMAP         hbmUnselected;
};

const int MAXICONS      = 10;   // max number that can be registered
const int MAXTABS       = 10;   // max number of tabs in string
const int MAXSTRINGLEN  = MAX_PATH;  // AddString and InsertString limit
const int ICONSPACE     = 3;    // whitespace around Icons in listbox

class IconListBox {

    public:
        IconListBox(HINSTANCE hInst, int nID,
        int iconWidth = 16, int iconHeight = 16);
        ~IconListBox();

        int  AddString(int nIcon, const char far *string);
        void Clear();
        void DeleteString(int nIndex);
        virtual void DrawItem(LPDRAWITEMSTRUCT lpd);
        int  FindString(const char far *string, int nIndexStart = -1) const;
        int  FindStringExact(const char far *string, int nIndexStart = -1) const;
        int  GetCount();
        int  GetCurrentSelection(void) const;
        int  GetIconID(int nIndex) const;
        BOOL GetSel(int nIndex);
        int  GetSelCount() const;
        int  GetSelItems(int cItems, int FAR *lpItems) const;
        int  GetString(int nIndex, char far *string) const;
        int  InsertString(int nIcon, const char far *string, int nIndex);
        void RegisterIcon(int nIconID, int nResID, int x=0, int y=0,
                            COLORREF colTransparent = RGB(0, 255, 0));
        int  SelectString(int nIndex, const char far *string);
        int  SetCurrentSelection(int nIndex = -1) const;
        void SetHeight(HWND hwndDlg, LPMEASUREITEMSTRUCT lpm, int height=16);
        void SetRedraw(BOOL bRedraw = TRUE) const;
        void SetSel(int nIndex, BOOL bSelected = TRUE) const;
        void SetTabStops(int cTabs, const int *pTabs);

    protected:
        int         SetItemData(int nIndex, int nIconID) const;
		int			UpdateHorizontalExtent(int	nIconID,const char *string);

        int         _cIcons;                    // number of icons registered
        IconList    _aIcons[MAXICONS];          // registered icons
        int         _cTabs;                     // number of tabs registered
        int         _aTabs[MAXTABS];            // registered tabs
        int         _iconWidth, _iconHeight;    // size of icons
		int			_iCurrentMaxHorzExt;		// Currently maximum horizontal extent

        COLORREF    _colSel, _colSelText, _colUnsel, _colUnselText;

        HINSTANCE   _hInst;                     // application's hInst
        int         _nCtlID;                    // id of listbox control
        int         _nTextOffset;               // vertical DrawText offset

        BOOL        _fCombo;                    // Dropdown combo box ?

        HWND        _hwndDialog;
        HWND        _hwndListBox;

        HBRUSH      _hbrSelected;               // background colours
        HBRUSH      _hbrUnselected;
};


// AddString - returns index of new string, or LB_ERR or LB_ERRSPACE

inline int IconListBox::AddString(int nIcon, const char far *string)
{
    int nIndex =  (int) ::SendDlgItemMessage(_hwndDialog, _nCtlID,
                            _fCombo ? CB_ADDSTRING : LB_ADDSTRING,
                             0, (LPARAM) ((LPSTR) string));
    SetItemData(nIndex, nIcon);
	UpdateHorizontalExtent(nIcon,string);

    return nIndex;
}


// Clear - clears contents of listbox

inline void IconListBox::Clear()
{
    ::SendMessage(_hwndListBox,
                  _fCombo ? CB_RESETCONTENT : LB_RESETCONTENT, 0, 0);

    _iCurrentMaxHorzExt = 0;
}


// DeleteString - removes a string specified by the index

inline void IconListBox::DeleteString(int nIndex)
{
    ::SendMessage(_hwndListBox,
                  _fCombo ? CB_DELETESTRING : LB_DELETESTRING, nIndex, 0);

	// May be horizontal extent changed - recalculate again
	UpdateHorizontalExtent(0,NULL);

}

// FindString & FindStringExact
//
// These functions find a listbox entry that begins with the characters
// specifed in string (FindString) or exactly matches string (FindStringExact)
// They return LB_ERR if the string is not found.  Otherwise, you can call
// GetString on the returned index.
//
// nIndexStart defaults to -1 which means search from the beginning of the
// listbox or combobox.

inline int IconListBox::FindString(const char far *string, int nIndexStart) const
{
    return (int) ::SendDlgItemMessage(_hwndDialog, _nCtlID,
                _fCombo ? CB_FINDSTRING : LB_FINDSTRING, nIndexStart, (LPARAM)string);
}
inline int IconListBox::FindStringExact(const char far *string, int nIndexStart) const
{
    return (int) ::SendDlgItemMessage(_hwndDialog, _nCtlID,
                        _fCombo ? CB_FINDSTRINGEXACT : LB_FINDSTRINGEXACT,
                        nIndexStart, (LPARAM)string);
}

// GetCount - returns the current number of listbox entries

inline int IconListBox::GetCount()
{
    return (int) ::SendMessage(_hwndListBox,
                              _fCombo ? CB_GETCOUNT : LB_GETCOUNT, 0, 0);
}


// GetCurrentSelection - returns index or LB_ERR if no selection.
// This function is not useful for multi-select listboxen.

inline int IconListBox::GetCurrentSelection() const
{
    return (int) ::SendMessage(_hwndListBox,
                               _fCombo ? CB_GETCURSEL : LB_GETCURSEL, 0, 0);
}


// GetItemData - retrieve ICON id

inline int IconListBox::GetIconID(int nIndex) const
{
    return (int) ::SendMessage(_hwndListBox,
                     _fCombo ? CB_GETITEMDATA : LB_GETITEMDATA, nIndex, 0);
}


// GetSel - returns nonzero if nIndex is selected

inline BOOL IconListBox::GetSel(int nIndex)
{
    return (BOOL) ::SendMessage(_hwndListBox, LB_GETSEL, nIndex, 0);
}


// GetSelCount - returns number of selected entries in multi-select listbox

inline int IconListBox::GetSelCount() const
{
    return (int) ::SendMessage(_hwndListBox, LB_GETSELCOUNT, 0, 0);
}


// GetSelItems - places index of each selected item in array.  Returns
// LB_ERR if not multi-select listbox, otherwise number of items in array.

inline int IconListBox::GetSelItems(int cItems, int FAR *lpItems) const
{
    return (int)
        ::SendMessage(_hwndListBox, LB_GETSELITEMS, cItems, (LPARAM) lpItems);
}


// GetString - returns length of string returned or LB_ERR if nIndex invalid

inline int IconListBox::GetString(int nIndex, char far *string) const
{
    return (int) ::SendDlgItemMessage(_hwndDialog, _nCtlID,
                        _fCombo ? CB_GETLBTEXT : LB_GETTEXT, nIndex,
                        (LPARAM) ((LPSTR) string));
}


// InsertString - same returns as AddString

inline int IconListBox::InsertString(int nIcon, const char far *string, int nIndex)
{
    int nNewIndex =  (int) ::SendDlgItemMessage(_hwndDialog, _nCtlID,
                 _fCombo ? CB_INSERTSTRING : LB_INSERTSTRING,
                 (WPARAM) nIndex, (LPARAM) ((LPSTR) string));

    SetItemData(nNewIndex, nIcon);
	UpdateHorizontalExtent(nIcon,string);

    return(nNewIndex);
}


// SelectString
// nIndex specifies where to start searching (-1 means from top).
// string specifies the initial characters of the string to match.
// Function returns index or LB_ERR if string not found

inline int IconListBox::SelectString(int nIndex, const char far *string)
{
    return (int) ::SendMessage(_hwndListBox,
                              _fCombo ? CB_SELECTSTRING : LB_SELECTSTRING,
                               nIndex,(LRESULT)string);
}


// SetCurrentSelection -
// This function sets the current selection in a single-select style
// listbox.  It returns LB_ERR if an error occurs, or if nIndex is -1
// (the default) meaning no current selection.

inline int IconListBox::SetCurrentSelection(int nIndex) const
{
    return (int) ::SendMessage(_hwndListBox,
                        _fCombo ? CB_SETCURSEL : LB_SETCURSEL, nIndex, 0);
}


// SetItemData - used to store icon id, returns LB_ERR if error occurs
// Use GetIconID to retrieve this id later.

inline int IconListBox::SetItemData(int nIndex, int nData) const
{
    return (int) ::SendMessage(_hwndListBox,
                              _fCombo ? CB_SETITEMDATA : LB_SETITEMDATA,
                               nIndex, nData);
}


// SetRedraw - turn on (TRUE) or off (FALSE) visual updates

inline void IconListBox::SetRedraw(BOOL bRedraw) const
{
    ::SendMessage(_hwndListBox, WM_SETREDRAW, bRedraw, 0);
}


// SetSel - used in multiselect listboxen.  TRUE selects, FALSE deselects.
// bSelected defaults to TRUE.  nIndex == -1 means select all.

inline void IconListBox::SetSel(int nIndex, BOOL bSelected) const
{
    ::SendMessage(_hwndListBox, LB_SETSEL, bSelected, nIndex);
}


#endif  // _ICONLBOX_H_
