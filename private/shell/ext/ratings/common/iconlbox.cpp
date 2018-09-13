/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1993                         **/
/***************************************************************************/

/****************************************************************************

    ICONLBOX.C

    Implementation for IconListBox class

    May 93, JimH

    See ICONLBOX.H for details on use.

****************************************************************************/

#include "npcommon.h"
#include <windows.h>
#include <memory.h>
#include <iconlbox.h>

#pragma intrinsic(memcpy)

#if defined(DEBUG)
static const char szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include <npassert.h>


/****************************************************************************

IconListBox constructor

Some initialization is done here, and some is done in SetHeight
(when window handles are known.)

****************************************************************************/

IconListBox::IconListBox(HINSTANCE hInst, int nCtlID,
                    int iconWidth, int iconHeight) :
                    _nCtlID(nCtlID), _hInst(hInst),
                    _iconWidth(iconWidth), _iconHeight(iconHeight),
                    _hbrSelected(NULL), _hbrUnselected(NULL),
                    _fCombo(FALSE), _cIcons(0), _cTabs(0),_iCurrentMaxHorzExt(0),
                    _hwndDialog(NULL), _hwndListBox(NULL)
{
    _colSel       = ::GetSysColor(COLOR_HIGHLIGHT);
    _colSelText   = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
    _colUnsel     = ::GetSysColor(COLOR_WINDOW);
    _colUnselText = ::GetSysColor(COLOR_WINDOWTEXT);
}


/****************************************************************************

IconListBox destructor

deletes any GDI objects created by IconListBox

****************************************************************************/

IconListBox::~IconListBox()
{
    for (int i = 0; i < _cIcons; i++)
    {
        if (_aIcons[i].hbmSelected)
        {
            if (_aIcons[i].hbmSelected)
            {
                ::DeleteObject(_aIcons[i].hbmSelected);
                ::DeleteObject(_aIcons[i].hbmUnselected);
            }

            // Subsequent _aIcons may have used the same bitmap.
            // Mark those as already deleted.

            for (int j = i + 1; j < _cIcons; j++)
            {
                if (_aIcons[j].nResID == _aIcons[i].nResID)
                {
                    _aIcons[j].hbmSelected = NULL;
                    _aIcons[j].hbmUnselected = NULL;
                }
            }
        }
    }

    if (_hbrSelected)
        ::DeleteObject(_hbrSelected);

    if (_hbrUnselected)
        ::DeleteObject(_hbrUnselected);
}


/****************************************************************************

IconListBox::SetHeight

This function MUST be called in reponse to the WM_MEASUREITEM message.
It creates some GDI objects, and initializes class variables not known
at construction time.

****************************************************************************/

void IconListBox::SetHeight(HWND hwndDlg,
                        LPMEASUREITEMSTRUCT lpm,
                        int itemHeight)             // defaults to 16
{
    ASSERT(hwndDlg != NULL);
    ASSERT((int)lpm->CtlID == _nCtlID);

    _hwndDialog  = hwndDlg;
    _hwndListBox = ::GetDlgItem(_hwndDialog, _nCtlID);

    // Determine if this is a combo box

    char    szClass[32];
    GetClassName(_hwndListBox,szClass,sizeof(szClass));
    if (::lstrcmpi(szClass,"combobox") == 0 )
         _fCombo = TRUE;


    // Create the background brushes used for filling listbox entries...

    _hbrSelected   = ::CreateSolidBrush(_colSel);
    _hbrUnselected = ::CreateSolidBrush(_colUnsel);

    // Calculate how to centre the text vertically in the listbox item.

    TEXTMETRIC  tm;
    HDC         hDC = ::GetDC(hwndDlg);

    GetTextMetrics(hDC, &tm);

    // Set the only lpm entry that matters

	// allow larger height if passed in - but at least large enough
	// to fit font.

	lpm->itemHeight = max( itemHeight, tm.tmHeight + tm.tmExternalLeading );

    _nTextOffset = tm.tmExternalLeading / 2 + 1;

    ::ReleaseDC(hwndDlg, hDC);
}


/****************************************************************************

IconListBox::DrawItem

This function MUST be called in response to the WM_DRAWITEM message.
It takes care of drawing listbox items in selected or unselected state.

Drawing and undrawing the focus rectangle takes advantage of the fact
that DrawFocusRect uses an XOR pen, and Windows is nice enough to assume
this in the order of the ODA_FOCUS messages.

****************************************************************************/

void IconListBox::DrawItem(LPDRAWITEMSTRUCT lpd)
{
    ASSERT(_hwndDialog != NULL);    // make sure SetHeight has been called

    char string[MAXSTRINGLEN];
    BOOL bSelected = (lpd->itemState & ODS_SELECTED);

    GetString(lpd->itemID, string);

    // fill entire rectangle with background color

    ::FillRect(lpd->hDC, &(lpd->rcItem),
                            bSelected ? _hbrSelected : _hbrUnselected);

    // Look for registered icon to display, and paint it if found

    for (int id = 0; id < _cIcons; id++)
        if (_aIcons[id].nID == (int) lpd->itemData)
            break;

    if (id != _cIcons)              // if we found a bitmap to display
    {
        HDC hdcMem = ::CreateCompatibleDC(lpd->hDC);
        HBITMAP hOldBitmap = (HBITMAP)::SelectObject(hdcMem,
            bSelected ? _aIcons[id].hbmSelected : _aIcons[id].hbmUnselected);

        // draw bitmap ICONSPACE pixels from left and centred vertically

        int x = lpd->rcItem.left + ICONSPACE;
        int y = ((lpd->rcItem.bottom - lpd->rcItem.top) - _iconHeight) / 2;
        y += lpd->rcItem.top;

        ::BitBlt(lpd->hDC, x, y, _iconWidth, _iconHeight, hdcMem,
                    _aIcons[id].x, _aIcons[id].y, SRCCOPY);

        ::SelectObject(hdcMem, hOldBitmap);
        ::DeleteDC(hdcMem);
    }

	if (lpd->itemState & ODS_FOCUS)
        ::DrawFocusRect(lpd->hDC, &(lpd->rcItem));


    lpd->rcItem.left += (_iconWidth + (2 * ICONSPACE));

    // Paint string

    ::SetTextColor(lpd->hDC, bSelected ? _colSelText : _colUnselText);
    ::SetBkColor(lpd->hDC, bSelected ? _colSel : _colUnsel);
    (lpd->rcItem.top) += _nTextOffset;

    if (_cTabs == 0)        // if no tabs registered
    {
        ::DrawText(lpd->hDC, string, lstrlen(string), &(lpd->rcItem),
                        DT_LEFT | DT_EXPANDTABS);
    }
    else
    {
        ::TabbedTextOut(lpd->hDC, lpd->rcItem.left, lpd->rcItem.top,
                string, lstrlen(string), _cTabs, _aTabs, 0);
    }
}


/****************************************************************************

IconListBox::RegisterIcons

Icons must be registered before they can be referenced in AddString.

Note that if you are using several icons from the same bitmap (with different
x and y offsets) they must all have the same background color.

****************************************************************************/

void IconListBox::RegisterIcon( int nIconID,            // caller's code
                                int nResID,             // RC file id
                                int x, int y,           // top left corner
                                COLORREF colTransparent)  // def. bright green
{
    ASSERT( _cIcons < MAXICONS );

    _aIcons[_cIcons].nID    = nIconID;
    _aIcons[_cIcons].nResID = nResID;
    _aIcons[_cIcons].x = x;
    _aIcons[_cIcons].y = y;

    // Check to see if we already have bitmaps for this resource ID
    // (which may have different x and y offsets.)

    for (int i = 0; i < _cIcons; i++)
    {
        if (_aIcons[i].nResID == nResID)
        {
            _aIcons[_cIcons].hbmSelected   = _aIcons[i].hbmSelected;
            _aIcons[_cIcons].hbmUnselected = _aIcons[i].hbmUnselected;
            _cIcons++;
            return;
        }
    }

    // Otherwise, create new selected and unselected bitmaps

    // Get pointer to DIB

    HRSRC h = ::FindResource(_hInst, MAKEINTRESOURCE(nResID), RT_BITMAP);
    if (h == NULL)
        return;

    HANDLE hRes = ::LoadResource(_hInst, h);
    if (hRes == NULL)
        return;

    LPBITMAPINFOHEADER lpInfo = (LPBITMAPINFOHEADER) LockResource(hRes);

    // Get pointers to start of color table, and start of actual bitmap bits

    // Note that we make a copy of the bitmap header info and the color
    // table.  This is so applications that use iconlistbox can keep their
    // resource segments read only.

    LPBYTE lpBits = (LPBYTE)
                (lpInfo + 1) + (1 << (lpInfo->biBitCount)) * sizeof(RGBQUAD);

    int cbCopy = (int) (lpBits - (LPBYTE)lpInfo);

    BYTE *lpCopy = new BYTE[cbCopy];

    if (!lpCopy)
        return;

    memcpy(lpCopy, lpInfo, cbCopy);

    RGBQUAD FAR *lpRGBQ =
                    (RGBQUAD FAR *) ((LPSTR)lpCopy + lpInfo->biSize);

    // Find transparent color in color table

    BOOL bFound = FALSE;            // did we find a transparent match?

    int nColorTableSize = (int) (lpBits - (LPBYTE)lpRGBQ);
    nColorTableSize /= sizeof(RGBQUAD);

    for (i = 0; i < nColorTableSize; i++)
    {
        if (colTransparent ==
                RGB(lpRGBQ[i].rgbRed, lpRGBQ[i].rgbGreen, lpRGBQ[i].rgbBlue))
        {
            bFound = TRUE;
            break;
        }
    }

    // Replace the transparent color with the background for selected and
    // unselected entries.  Use these to create selected and unselected
    // bitmaps, and restore color table.

    RGBQUAD rgbqTemp;                       // color table entry to replace
    HDC hDC = ::GetDC(_hwndDialog);

    if (bFound)
    {
        rgbqTemp = lpRGBQ[i];
        lpRGBQ[i].rgbRed   = GetRValue(_colUnsel);
        lpRGBQ[i].rgbBlue  = GetBValue(_colUnsel);
        lpRGBQ[i].rgbGreen = GetGValue(_colUnsel);
    }
    _aIcons[_cIcons].hbmUnselected = ::CreateDIBitmap(hDC,
                            (LPBITMAPINFOHEADER)lpCopy, CBM_INIT, lpBits,
                            (LPBITMAPINFO)lpCopy, DIB_RGB_COLORS);

    if (bFound)
    {
        lpRGBQ[i].rgbRed   = GetRValue(_colSel);
        lpRGBQ[i].rgbBlue  = GetBValue(_colSel);
        lpRGBQ[i].rgbGreen = GetGValue(_colSel);
    }
    _aIcons[_cIcons].hbmSelected = ::CreateDIBitmap(hDC,
                            (LPBITMAPINFOHEADER)lpCopy, CBM_INIT, lpBits,
                            (LPBITMAPINFO)lpCopy, DIB_RGB_COLORS);

    if (bFound)
        lpRGBQ[i] = rgbqTemp;           // restore original color table entry

    ::ReleaseDC(_hwndDialog, hDC);
    ::FreeResource(hRes);
    delete lpCopy;

    _cIcons++;
}


/****************************************************************************

IconListBox::SetTabStops

Since this is an owner-draw listbox, we can't rely on LB_SETTABS.
Instead, tabs are registered here and TabbedTextOut is used to display
strings.  Dialogbox units have to be converted to pixels.

****************************************************************************/

void IconListBox::SetTabStops(int cTabs, const int *pTabs)
{
    ASSERT(cTabs <= MAXTABS);

    int nSize  = (int) LOWORD(GetDialogBaseUnits());

    for (int i = 0; i < cTabs; i++)
        _aTabs[i] = ((nSize * pTabs[i]) / 4);

    _cTabs = cTabs;
}

/****************************************************************************

IconListBox::UpdateHorizontalExtent

****************************************************************************/
int IconListBox::UpdateHorizontalExtent(int	nIcon,const char *string)
{
    ASSERT(_hwndDialog != NULL);    // make sure SetHeight has been called

	if (!string)
		return 0;
	// Calculate width in pixels for given string, taking into account icon, spacing and tabs
    int iItemWidth = ICONSPACE + (_iconWidth + (2 * ICONSPACE));
    HDC	hDC = ::GetDC(_hwndDialog);
	iItemWidth += LOWORD(GetTabbedTextExtent(hDC,string,::lstrlen(string),_cTabs, _aTabs));
    ::ReleaseDC(_hwndDialog, hDC);

	// Update maximum value
    _iCurrentMaxHorzExt = max(_iCurrentMaxHorzExt,iItemWidth);

	return (int)SendDlgItemMessage(_hwndDialog,_nCtlID,
								LB_SETHORIZONTALEXTENT,
								(WPARAM)_iCurrentMaxHorzExt,0L);

}
