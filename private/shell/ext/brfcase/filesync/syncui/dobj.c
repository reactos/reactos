//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: dobj.c
//
//  This file contains support routines for the reconciliation-action 
//   control class code
//
//
// History:
//  09-13-93 ScottH     Extracted from recact.c
//
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////  INCLUDES

#include "brfprv.h"         // common headers
#include "res.h"
#include "recact.h"
#include "dobj.h"

/////////////////////////////////////////////////////  CONTROLLING DEFINES


/////////////////////////////////////////////////////  DEFINES

#define DT_CALCWRAP     (DT_CALCRECT | DT_CENTER | DT_WORDBREAK | DT_NOPREFIX)
#define DT_CALC         (DT_CALCRECT | DT_CENTER | DT_SINGLELINE | DT_NOPREFIX)

/*----------------------------------------------------------
Purpose: Formats the given path to the correct location format
Returns: --
Cond:    --
*/
void PRIVATE FormatLocationPath(
    LPCTSTR pszPath,
    LPTSTR pszBuffer)        // Must be MAX_PATH
    {
    UINT ids;
    TCHAR szBrfDir[MAX_PATH];
    LPCTSTR psz;
    LPTSTR pszMsg;

    //  The format for the directory location is:
    //
    //      Inside briefcase:       "In Briefcase"
    //      Below briefcase:        "In Briefcase\FolderName"
    //      Outside briefcase:      "In FullPath"
    //
    // We assume that paths outside the current briefcase
    //  never consist of a briefcase name of another.
    //
    if (PathGetLocality(pszPath, szBrfDir) != PL_FALSE)
        {
        // Inside the briefcase
        psz = &pszPath[lstrlen(szBrfDir)];
        ids = IDS_InBriefcase;
        }
    else
        {
        // Outside the briefcase
        psz = pszPath;
        ids = IDS_InLocation;
        }

    if (ConstructMessage(&pszMsg, g_hinst, MAKEINTRESOURCE(ids), psz))
        {
        lstrcpy(pszBuffer, pszMsg);
        GFree(pszMsg);
        }
    else
        *pszBuffer = 0;
    }


/*----------------------------------------------------------
Purpose: Return the string describing the status of this sideitem
Returns: ptr to status string
Cond:    --
*/
LPTSTR PRIVATE SideItem_GetStatus(
    LPSIDEITEM this,
    LPTSTR pszBuf,
    UINT cchBuf)
    {
    switch (this->uState)
        {
    case SI_CHANGED:
        return SzFromIDS(IDS_STATE_Changed, pszBuf, cchBuf);
    case SI_UNCHANGED:
        return SzFromIDS(IDS_STATE_Unchanged, pszBuf, cchBuf);
    case SI_NEW:
        return SzFromIDS(IDS_STATE_NewFile, pszBuf, cchBuf);
    case SI_UNAVAILABLE:
        return SzFromIDS(IDS_STATE_Unavailable, pszBuf, cchBuf);
    case SI_NOEXIST:
        return SzFromIDS(IDS_STATE_DoesNotExist, pszBuf, cchBuf);
    case SI_DELETED:
        return SzFromIDS(IDS_STATE_Deleted, pszBuf, cchBuf);
    default:
        ASSERT(0);
        return NULL;
        }
    }


/*----------------------------------------------------------
Purpose: Displays the 3-liner: location, status, and timestamp
Returns: --
Cond:    --
*/
void PRIVATE SideItem_Display(
    LPSIDEITEM this,
    HDC hdc,
    LPRECT prc,
    int cxEllipses,
    int cyText)
    {
    TCHAR sz[MAX_PATH];
    TCHAR szBuf[MAXBUFLEN];
    LPTSTR psz;
    RECT rc = *prc;

    // Directory location.  

    FormatLocationPath(this->pszDir, sz);
    MyDrawText(hdc, sz, &rc, MDT_LEFT | MDT_TRANSPARENT | MDT_ELLIPSES, 
        cyText, cxEllipses, CLR_DEFAULT, CLR_DEFAULT);

    // Status string
    psz = SideItem_GetStatus(this, szBuf, ARRAYSIZE(szBuf));
    if (psz)
        {
        // Only bother with these two lines if the file actually
        // exists.

        rc.top += cyText;
        MyDrawText(hdc, psz, &rc, MDT_LEFT | MDT_TRANSPARENT, 
            cyText, cxEllipses, CLR_DEFAULT, CLR_DEFAULT);

        // Date stamp.  Skip this if this is a folder or unavailable.
        //
        if (SI_DELETED != this->uState && 
            SI_NOEXIST != this->uState &&
            SI_UNAVAILABLE != this->uState &&
            FS_COND_UNAVAILABLE != this->fs.fscond) // hack for folders
            {
            FileTimeToDateTimeString(&this->fs.ftMod, sz, ARRAYSIZE(sz));

            rc.top += cyText;
            MyDrawText(hdc, sz, &rc, MDT_LEFT | MDT_TRANSPARENT, 
                cyText, cxEllipses, CLR_DEFAULT, CLR_DEFAULT);
            }
        }
    }


/*----------------------------------------------------------
Purpose: Return the bounding rect for a labelled image.

Returns: --
Cond:    --
*/
void PUBLIC ComputeImageRects(
    LPCTSTR psz,
    HDC hdc,
    LPPOINT pptInOut,       
    LPRECT prcWhole,        // May be NULL
    LPRECT prcLabel,        // May be NULL
    int cxIcon,
    int cyIcon,
    int cxIconSpacing,
    int cyText)
    {
    RECT rc;
    int yLabel;
    int cxLabel;
    int cyLabel;
    int cchLabel;
    POINT pt;

    ASSERT(psz);

    // Set our minimum rect size for icon spacing
    if (cxIconSpacing < cxIcon)
        cxIconSpacing = cxIcon + g_cxIconMargin * 2;

    // Upon entry, *pptInOut is expected to be the upper left corner of the 
    // icon-spacing rect.  This function will set it to the upper left
    // corner of the icon itself.

    pt.x = pptInOut->x + (cxIconSpacing - cxIcon) / 2;
    pt.y = pptInOut->y + g_cyIconMargin;

    // Determine rectangle of label with wrap

    rc.left = rc.top = rc.bottom = 0;
    rc.right = cxIconSpacing - g_cxLabelMargin * 2;

    cchLabel = lstrlen(psz);
    if (0 < cchLabel)
        {
        DrawText(hdc, psz, cchLabel, &rc, DT_CALCWRAP);
        }
    else
        {
        rc.bottom = rc.top + cyText;
        }

    yLabel = pptInOut->y + g_cyIconMargin + cyIcon + g_cyLabelSpace;
    cxLabel = (rc.right - rc.left) + 2 * g_cxLabelMargin;
    cyLabel = rc.bottom - rc.top;

    if (prcWhole)
        {
        prcWhole->left   = pptInOut->x;
        prcWhole->right  = prcWhole->left + cxIconSpacing;
        prcWhole->top    = pptInOut->y;
        prcWhole->bottom = max(prcWhole->top + g_cyIconSpacing,
                            yLabel + cyLabel + g_cyLabelSpace);
        }

    if (prcLabel)
        {
        prcLabel->left = pptInOut->x + ((cxIconSpacing - cxLabel) / 2);
        prcLabel->right = prcLabel->left + cxLabel;
        prcLabel->top = yLabel;
        prcLabel->bottom = prcLabel->top + cyLabel;
        }

    *pptInOut = pt;
    }


/*----------------------------------------------------------
Purpose: Set the colors for the given HDC.  The previous colors
          are stored in pcrText and pcrBk.

Returns: uStyle to pass to ImageList_Draw (specific to images only)
Cond:    --
*/
UINT PRIVATE Dobj_SetColors(
    LPDOBJ this,
    HDC hdc,
    UINT uState,
    COLORREF clrBkgnd)
    {
    COLORREF clrText;
    COLORREF clrBk;
    UINT uStyleILD = ILD_NORMAL;
    BOOL bSetColors = FALSE;
    BOOL bDiffer;
    BOOL bMenu;
    BOOL bDisabled;

    // Determine selection colors
    //
    bDiffer = IsFlagSet(this->uFlags, DOF_DIFFER);
    bMenu = IsFlagSet(this->uFlags, DOF_MENU);
    bDisabled = IsFlagSet(this->uFlags, DOF_DISABLED);

    switch (this->uKind)
        {
    case DOK_STRING:
    case DOK_IDS:
    case DOK_SIDEITEM:
        bSetColors = TRUE;
        break;
        }
    
    // Set the text and background colors
    //
    if (bSetColors)
        {
        if (bDiffer)
            {
            // Make the colors differ based on selection state
            //
            if (bMenu)
                {
                if (bDisabled)
                    clrText = GetSysColor(COLOR_GRAYTEXT);
                else
                    clrText = GetSysColor(ColorMenuText(uState));

                clrBk = GetSysColor(ColorMenuBk(uState));
                }
            else
                {
                if (bDisabled)
                    clrText = GetSysColor(COLOR_GRAYTEXT);
                else
                    clrText = GetSysColor(ColorText(uState));

                clrBk = GetSysColor(ColorBk(uState));
                }
            }
        else
            {
            // Transparent colors
            //
            if (bMenu)
                {
                if (bDisabled)
                    clrText = GetSysColor(COLOR_GRAYTEXT);
                else
                    clrText = GetSysColor(COLOR_MENUTEXT);

                clrBk = GetSysColor(COLOR_MENU);
                }
            else
                {
                if (bDisabled)
                    clrText = GetSysColor(COLOR_GRAYTEXT);
                else
                    clrText = GetSysColor(COLOR_WINDOWTEXT);

                clrBk = clrBkgnd;
                }
            }
        SetTextColor(hdc, clrText);
        SetBkColor(hdc, clrBk);
        }

    return uStyleILD;
    }


/*----------------------------------------------------------
Purpose: Draw the menu image and text
Returns: --
Cond:    --
*/
void PRIVATE Dobj_DrawMenuImage(
    LPDOBJ this,
    HDC hdc,
    UINT uState,
    int cyText,
    COLORREF clrBkgnd)
    {
    UINT uStyleILD;
    UINT uFlagsETO;
    LPCTSTR psz;
    TCHAR szIDS[MAXBUFLEN];
    int cch;
    HIMAGELIST himl = this->himl;
    COLORREF clrText;
    COLORREF clrBk;
    int x;
    int y;
    int cxIcon;
    RECT rc;

    if (IsFlagSet(this->uFlags, DOF_USEIDS))
        psz = SzFromIDS(PtrToUlong(this->lpvObject), szIDS, ARRAYSIZE(szIDS));
    else
        psz = (LPCTSTR)this->lpvObject;

    ASSERT(psz);

    cch = lstrlen(psz);
    ImageList_GetImageRect(himl, this->iImage, &rc);
    cxIcon = rc.right-rc.left;

    // Draw the text first

    uFlagsETO = ETO_OPAQUE | ETO_CLIPPED;
    x = this->rcLabel.left + g_cxMargin + cxIcon + g_cxMargin;
    y = this->rcLabel.top + ((this->rcLabel.bottom - this->rcLabel.top - cyText) / 2);

    if (IsFlagSet(this->uFlags, DOF_DISABLED) && 
        IsFlagClear(uState, ODS_SELECTED))
        {
        int imodeOld;
        COLORREF crOld;

        // For disabled menu strings (not selected), we draw the string 
        // twice.  The first is offset down and to the right and drawn 
        // in the 3D hilight color.  The second time is the disabled text
        // color in the normal offset.
        //
        crOld = SetTextColor(hdc, GetSysColor(COLOR_3DHILIGHT));
        imodeOld = SetBkMode(hdc, TRANSPARENT);
        ExtTextOut(hdc, x+1, y+1, uFlagsETO, &this->rcLabel, psz, cch, NULL);

        // Reset back to original color.  Also, turn off the opaqueness.
        //
        SetTextColor(hdc, crOld);
        uFlagsETO ^= ETO_OPAQUE;
        }

    if (IsFlagSet(this->uFlags, DOF_DISABLED))
        clrText = GetSysColor(COLOR_GRAYTEXT);
    else
        clrText = GetSysColor(ColorMenuText(uState));

    clrBk = GetSysColor(ColorMenuBk(uState));
    SetTextColor(hdc, clrText);
    SetBkColor(hdc, clrBk);

    ExtTextOut(hdc, x, y, uFlagsETO, &this->rcLabel, psz, cch, NULL);

    // Draw the image

    if (GetBkColor(hdc) == ImageList_GetBkColor(himl))
        uStyleILD = ILD_NORMAL;     // Paint quicker
    else
        uStyleILD = ILD_TRANSPARENT;

    ImageList_Draw(himl, this->iImage, hdc, this->x, this->y, uStyleILD);
    }


/*----------------------------------------------------------
Purpose: Draw the icon image and label
Returns: --
Cond:    --
*/
void PRIVATE Dobj_DrawIconImage(
    LPDOBJ this,
    HDC hdc,
    UINT uState,
    int cxEllipses,
    int cyText,
    COLORREF clrBkgnd)
    {
    UINT uStyleILD;
    UINT uFlagsMDT;
    LPCTSTR psz;
    TCHAR szIDS[MAXBUFLEN];

    if (IsFlagSet(this->uFlags, DOF_USEIDS))
        psz = SzFromIDS(PtrToUlong(this->lpvObject), szIDS, ARRAYSIZE(szIDS));
    else
        psz = (LPCTSTR)this->lpvObject;

    ASSERT(psz);

    // Draw the image
    //
    if (IsFlagClear(this->uFlags, DOF_IGNORESEL))
        {
        uStyleILD = GetImageDrawStyle(uState);
        uFlagsMDT = IsFlagSet(uState, ODS_SELECTED) ? MDT_SELECTED : MDT_DESELECTED;
        }
    else
        {
        uStyleILD = ILD_NORMAL;
        uFlagsMDT = MDT_DESELECTED;
        ClearFlag(uState, ODS_FOCUS);
        }

    ImageList_Draw(this->himl, this->iImage, hdc, this->x, this->y, uStyleILD);

    // Draw the file label.  Wrap if it is long.

    if (this->rcLabel.bottom - this->rcLabel.top > cyText)
        uFlagsMDT |= MDT_DRAWTEXT;
    
    MyDrawText(hdc, psz, &this->rcLabel, MDT_CENTER | uFlagsMDT, cyText, 
        cxEllipses, CLR_DEFAULT, clrBkgnd);

    // (uState may have been changed above)
    if (IsFlagSet(uState, ODS_FOCUS))
        DrawFocusRect(hdc, &this->rcLabel);
    }


#ifdef UNUSED
/*----------------------------------------------------------
Purpose: Draw a picture
Returns: --
Cond:    --
*/
void PRIVATE Dobj_DrawPicture(
    LPDOBJ this,
    HDC hdc,
    UINT uState,
    UINT uDrawStyle)
    {
    HIMAGELIST himl;
    HDC hdcMem;
    HBITMAP hbmp;
    BITMAP bm;
    RECT rc;
    int iImage;
    int cx;
    int x;
    int y;

    switch (this->uKind)
        {
    case DOK_BITMAP:
        hbmp = (HBITMAP)(DWORD)this->lpvObject;
        GetObject(hbmp, sizeof(BITMAP), &bm);
        cx = this->rcSrc.right - this->rcSrc.left;
        break;

    case DOK_ICON:
        cx = 32;
        break;
        }

    // We only align horizontally
    //
    y = this->y;
    if (IsFlagSet(this->uFlags, DOF_CENTER))
        x = this->x - (cx / 2);
    else if (IsFlagSet(this->uFlags, DOF_RIGHT))
        x = this->x - cx;
    else
        x = this->x;

    // Draw the object
    //
    switch (this->uKind)
        {
    case DOK_ICON:
        // BUGBUG: we don't handle DOF_DIFFER for icons
        DrawIcon(hdc, x, y, (HICON)(DWORD)this->lpvObject);
        break;

    case DOK_BITMAP:
        hdcMem = CreateCompatibleDC(hdc);
        if (hdcMem)
            {
            SIZE size;

            SelectBitmap(hdcMem, hbmp);
    
            size.cx = this->rcSrc.right - this->rcSrc.left;
            size.cy = this->rcSrc.bottom - this->rcSrc.top;

            if (IsFlagSet(this->uFlags, DOF_MENU) && 
                IsFlagSet(this->uFlags, DOF_DISABLED) && 
                IsFlagClear(uState, ODS_SELECTED))
                {
                COLORREF crOld;
    
                // For disabled menu strings (not selected), we draw the bitmap 
                //  twice.  The first is offset down and to the right and drawn 
                //  in the 3D hilight color.  The second time is the disabled 
                //  color in the normal offset.
                //
                crOld = SetTextColor(hdc, GetSysColor(COLOR_3DHILIGHT));
                BitBlt(hdc, x+1, y+1, size.cx, size.cy, hdcMem, this->rcSrc.left, 
                    this->rcSrc.top,  SRCCOPY);
    
                // Reset back to original color.  Also, turn off the opaqueness.
                //
                SetTextColor(hdc, crOld);
                }

            BitBlt(hdc, x, y, size.cx, size.cy, hdcMem, this->rcSrc.left, this->rcSrc.top,  SRCCOPY);
            DeleteDC(hdcMem);
            }
        break;
        }
    }
#endif

/*----------------------------------------------------------
Purpose: Draw a string
Returns: --
Cond:    --
*/
void PRIVATE Dobj_DrawString(
    LPDOBJ this,
    HDC hdc,
    UINT uState,
    int cxEllipses,
    int cyText)
    {
    UINT ufAlignSav;
                                               
    ASSERT(this);

    // Prep the alignment
    //
    if (this->uFlags & (DOF_LEFT | DOF_CENTER | DOF_RIGHT))
        {
        UINT ufMode;

        ufMode = IsFlagSet(this->uFlags, DOF_CENTER) ? TA_CENTER :
                 (IsFlagSet(this->uFlags, DOF_RIGHT) ? TA_RIGHT : TA_LEFT);
        ufAlignSav = SetTextAlign(hdc, ufMode);
        }

    // Draw the string
    //
    switch (this->uKind)
        {
    case DOK_IDS:
    case DOK_STRING:
        {
        TCHAR szBuf[MAXBUFLEN];
        LPTSTR lpsz;
        UINT uflag = ETO_OPAQUE;

        if (this->uKind == DOK_IDS)
            lpsz = SzFromIDS(PtrToUlong(this->lpvObject), szBuf, ARRAYSIZE(szBuf));
        else
            lpsz = (LPTSTR)this->lpvObject;

        if (!IsRectEmpty(&this->rcClip))
            uflag |= ETO_CLIPPED;
        
        if (IsFlagSet(this->uFlags, DOF_MENU) && 
            IsFlagSet(this->uFlags, DOF_DISABLED) && 
            IsFlagClear(uState, ODS_SELECTED))
            {
            int imodeOld;
            COLORREF crOld;

            // For disabled menu strings (not selected), we draw the string 
            //  twice.  The first is offset down and to the right and drawn 
            //  in the 3D hilight color.  The second time is the disabled text
            //  color in the normal offset.
            //
            crOld = SetTextColor(hdc, GetSysColor(COLOR_3DHILIGHT));
            imodeOld = SetBkMode(hdc, TRANSPARENT);
            ExtTextOut(hdc, this->x+1, this->y+1, uflag, &this->rcClip, lpsz,
                lstrlen(lpsz), NULL);

            // Reset back to original color.  Also, turn off the opaqueness.
            //
            SetTextColor(hdc, crOld);
            uflag ^= ETO_OPAQUE;
            }

        ExtTextOut(hdc, this->x, this->y, uflag, &this->rcClip, lpsz,
            lstrlen(lpsz), NULL);
        }
        break;

    case DOK_SIDEITEM:
        SideItem_Display((LPSIDEITEM)this->lpvObject, hdc, &this->rcClip, 
            cxEllipses, cyText);
        break;
        }

    // Clean up
    //
    if (this->uFlags & (DOF_LEFT | DOF_CENTER | DOF_RIGHT))
        {
        SetTextAlign(hdc, ufAlignSav);
        }
    }


/*----------------------------------------------------------
Purpose: Draw an object
Returns: --
Cond:    --
*/
void PUBLIC Dobj_Draw(
    HDC hdc,
    LPDOBJ rgdobj,
    int cItems,
    UINT uState,            // ODS_*
    int cxEllipses,
    int cyText,
    COLORREF clrBkgnd)
    {
    UINT uDrawStyle;
    LPDOBJ pdobj;
    int i;

    ASSERT(rgdobj);

    for (i = 0, pdobj = rgdobj; i < cItems; i++, pdobj++)
        {
        if (IsFlagSet(pdobj->uFlags, DOF_NODRAW))
            continue ;
    
        uDrawStyle = Dobj_SetColors(pdobj, hdc, uState, clrBkgnd);

        // Draw the object
        //
        switch (pdobj->uKind)
            {
        case DOK_IMAGE:
            if (IsFlagSet(pdobj->uFlags, DOF_MENU))
                Dobj_DrawMenuImage(pdobj, hdc, uState, cyText, clrBkgnd);
            else
                Dobj_DrawIconImage(pdobj, hdc, uState, cxEllipses, cyText, clrBkgnd);
            break;

#ifdef UNUSED
        case DOK_BITMAP:
        case DOK_ICON:
            Dobj_DrawPicture(pdobj, hdc, uState, uDrawStyle);
            break;
#endif
    
        case DOK_IDS:
        case DOK_STRING:
        case DOK_SIDEITEM:
            Dobj_DrawString(pdobj, hdc, uState, cxEllipses, cyText);
            break;
            }
        }
    }
