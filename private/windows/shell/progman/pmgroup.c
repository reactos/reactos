#include "progman.h"
#include "ime.h"

BOOL FAR PASCAL IMEStringWindow(HWND,HANDLE);
int FAR PASCAL IMEWindowGetCnt(LPTSTR,LPTSTR);

#define HK_SHIFT    0x0100
#define HK_CONTROL  0x0200
#define HK_ALT      0x0400
#define HK_EXT      0x0800

#define F_EXT       0x01000000L

#define OBJ_ITEM                1


BOOL bNoScrollCalc = FALSE;
RECT rcArrangeRect;
HWND hwndScrolling = NULL;

BOOL APIENTRY InQuotes(LPTSTR sz);
void NEAR PASCAL ViewActiveItem(PGROUP pGroup);

/* Make the first item in a list the last and return a pointer to it.*/

PITEM PASCAL MakeFirstItemLast(PGROUP pGroup)
{
    PITEM pItemCur;

    /* Just quit if there's no list.*/

    if ((pItemCur = pGroup->pItems) == NULL)
        return NULL;

    /* Find the end of the list.*/
    for( ; pItemCur->pNext ; pItemCur = pItemCur->pNext)
    ;

    /* End of the list.*/
    /* This works even if there is only one item in the */
    /* list, it's a waste of time though.*/
    pItemCur->pNext = pGroup->pItems;
    pGroup->pItems = pGroup->pItems->pNext;
    pItemCur->pNext->pNext = NULL;

    return pItemCur->pNext;
}


VOID PASCAL GetItemText(PGROUP pGroup, PITEM pItem, LPTSTR lpT, int index)
{
    LPITEMDEF lpid;
    LPGROUPDEF lpgd;

    lpid = LockItem(pGroup,pItem);
    if (!lpid) {
        UnlockGroup(pGroup->hwnd);
        *lpT = 0;
        return;
    }

    lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);

    switch (index) {

    case 0:
        lstrcpy(lpT, (LPTSTR) PTR(lpgd, lpid->pName));
        break;

    case 1:
        lstrcpy(lpT, (LPTSTR) PTR(lpgd, lpid->pCommand));
        break;

    case 2:
        lstrcpy(lpT, (LPTSTR) PTR(lpgd, lpid->pIconPath));
        break;

    default:
        *lpT = 0;
        break;
    }

    GlobalUnlock(pGroup->hGroup);

    UnlockGroup(pGroup->hwnd);
}


ULONG Color16Palette[] = {
    0x00000000, 0x00800000, 0x00008000, 0x00808000,
    0x00000080, 0x00800080, 0x00008080, 0x00808080,
    0x00c0c0c0, 0x00ff0000, 0x0000ff00, 0x00ffff00,
    0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
};

ULONG Color256Palette[] = {
    0x00000000, 0x00800000, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
    0x00c0dcc0, 0x00a6caf0, 0x00cccccc, 0x00580800, 0x00600800, 0x00680800, 0x00700800, 0x00780800,
    0x00801000, 0x00881000, 0x00901000, 0x00981000, 0x00a01000, 0x00a81000, 0x00b01000, 0x00b81000,
    0x00c01800, 0x00c81800, 0x00d01800, 0x00d81800, 0x00e01800, 0x00e81800, 0x00f01800, 0x00f81800,
    0x00002000, 0x00082000, 0x00102000, 0x00182000, 0x00202000, 0x00282000, 0x00302000, 0x00382000,
    0x00402800, 0x00482800, 0x00502800, 0x00582800, 0x00602800, 0x00682800, 0x00702800, 0x00782800,
    0x00803000, 0x00883000, 0x00903000, 0x00983000, 0x00a03000, 0x00a83000, 0x00b03000, 0x00b83000,
    0x00c03800, 0x00c83800, 0x00d03800, 0x00d83800, 0x00e03800, 0x00e83800, 0x00f03800, 0x00f83800,
    0x00004010, 0x00084010, 0x00104010, 0x00184010, 0x00204010, 0x00284010, 0x00304010, 0x00384010,
    0x00404810, 0x00484810, 0x00504810, 0x00584810, 0x00604810, 0x00684810, 0x00704810, 0x00784810,
    0x00805010, 0x00885010, 0x00905010, 0x00985010, 0x00a05010, 0x00a85010, 0x00b05010, 0x00b85010,
    0x00c05810, 0x00c85810, 0x00d05810, 0x00d85810, 0x00e05810, 0x00e85810, 0x00f05810, 0x00f85810,
    0x00006010, 0x00086010, 0x00106010, 0x00186010, 0x00206010, 0x00286010, 0x00306010, 0x00386010,
    0x00406810, 0x00486810, 0x00506810, 0x00586810, 0x00606810, 0x00686810, 0x00706810, 0x00786810,
    0x00807010, 0x00887010, 0x00907010, 0x00987010, 0x00a07010, 0x00a87010, 0x00b07010, 0x00b87010,
    0x00c07810, 0x00c87810, 0x00d07810, 0x00d87810, 0x00e07810, 0x00e87810, 0x00f07810, 0x00f87810,
    0x00008020, 0x00088020, 0x00108020, 0x00188020, 0x00208020, 0x00288020, 0x00308020, 0x00388020,
    0x00408820, 0x00488820, 0x00508820, 0x00588820, 0x00608820, 0x00688820, 0x00708820, 0x00788820,
    0x00809020, 0x00889020, 0x00909020, 0x00989020, 0x00a09020, 0x00a89020, 0x00b09020, 0x00b89020,
    0x00c09820, 0x00c89820, 0x00d09820, 0x00d89820, 0x00e09820, 0x00e89820, 0x00f09820, 0x00f89820,
    0x0000a020, 0x0008a020, 0x0010a020, 0x0018a020, 0x0020a020, 0x0028a020, 0x0030a020, 0x0038a020,
    0x0040a820, 0x0048a820, 0x0050a820, 0x0058a820, 0x0060a820, 0x0068a820, 0x0070a820, 0x0078a820,
    0x0080b020, 0x0088b020, 0x0090b020, 0x0098b020, 0x00a0b020, 0x00a8b020, 0x00b0b020, 0x00b8b020,
    0x00c0b820, 0x00b820c8, 0x00b820d0, 0x00b820d8, 0x00b820e0, 0x00b820e8, 0x00b820f0, 0x00b820f8,
    0x0000c030, 0x00c03008, 0x00c03010, 0x00c03018, 0x00c03020, 0x00c03028, 0x00c03030, 0x00c03038,
    0x0040c830, 0x00c83048, 0x00c83050, 0x00c83058, 0x00c83060, 0x00c83068, 0x00c83070, 0x00c83078,
    0x0080d030, 0x00d03088, 0x00d03090, 0x00d03098, 0x00d030a0, 0x00d030a8, 0x00d030b0, 0x00d030b8,
    0x00c0d830, 0x00c8d830, 0x00d0d830, 0x00d8d830, 0x00e0d830, 0x00e8d830, 0x00f0d830, 0x00f8d830,
    0x0000e030, 0x0008e030, 0x0010e030, 0x0018e030, 0x0020e030, 0x0028e030, 0x0030e030, 0x0038e030,
    0x0040e830, 0x0048e830, 0x0050e830, 0x0058e830, 0x0060e830, 0x0068e830, 0x0070e830, 0x0078e830,
    0x0080f030, 0x0088f030, 0x0090f030, 0x0098f030, 0x00a0f030, 0x00a8f030, 0x00fffbf0, 0x00a0a0a4,
    0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
};


HICON APIENTRY GetItemIcon(HWND hwnd, PITEM pitem)
{

    LPGROUPDEF lpgd;
    LPITEMDEF lpid;
    HICON hIcon = NULL;
    DWORD dwVer;
    HANDLE h;
    LPBYTE p;
    INT id = 0;
    INT cb;
    DWORD colors, size;
    PBITMAPINFOHEADER pbih, pbihNew;
    LPVOID palette;

    lpgd = LockGroup(hwnd);
    if (!lpgd)
        return DuplicateIcon(hAppInstance, hItemIcon);
        //return hItemIcon;
    lpid = ITEM(lpgd,pitem->iItem);

    if ((SHORT)lpid->cbIconRes > 0) {
        if (lpid->wIconVer == 2)
            dwVer = 0x00020000;
        else
            dwVer = 0x00030000;

        pbihNew = NULL;
        pbih = (PBITMAPINFOHEADER)PTR(lpgd, lpid->pIconRes);
        size = lpid->cbIconRes;

        if (pbih->biClrUsed == -1) {
            colors = (1 << (pbih->biPlanes * pbih->biBitCount));
            size += colors * sizeof(RGBQUAD);

            if (colors == 16) {
                palette = Color16Palette;
            } else if (colors == 256) {
                palette = Color256Palette;
            } else {
                palette = NULL;
            }

            if (palette != NULL)
                pbihNew = (PBITMAPINFOHEADER)LocalAlloc(LPTR, size);

            if (pbihNew != NULL) {
                RtlCopyMemory(pbihNew, pbih, sizeof( *pbih ));
                pbihNew->biClrUsed = 0;
                RtlCopyMemory((pbihNew+1), palette, colors * sizeof(RGBQUAD));
                RtlCopyMemory((PCHAR)(pbihNew+1) + (colors * sizeof(RGBQUAD)),
                              (pbih+1),
                              lpid->cbIconRes - sizeof(*pbih)
                             );

                pbih = pbihNew;
            }
            else {
                //
                // reset size
                //
                size = lpid->cbIconRes;
            }
        }

        hIcon = CreateIconFromResource((PBYTE)pbih, size, TRUE, dwVer);
        if (pbihNew != NULL)
            LocalFree(pbihNew);
    }

    if (!hIcon) {
      if (h = FindResource(hAppInstance, (LPTSTR) MAKEINTRESOURCE(ITEMICON), RT_GROUP_ICON)) {
          h = LoadResource(hAppInstance, h);
          p = LockResource(h);
          id = LookupIconIdFromDirectory(p, TRUE);
          UnlockResource(h);
          FreeResource(h);
      }
      if (h = FindResource(hAppInstance, (LPTSTR)  MAKEINTRESOURCE(id), (LPTSTR) MAKEINTRESOURCE(RT_ICON))) {
          cb = SizeofResource(hAppInstance, h);
          h = LoadResource(hAppInstance, h);
          p = LockResource(h);
          hIcon = CreateIconFromResource(p, cb, TRUE, 0x00030000);
          UnlockResource(h);
          FreeResource(h);
      }

    }
    UnlockGroup(hwnd);
    return hIcon;
}


COLORREF PASCAL GetTitleTextColor(VOID)
{
    COLORREF color;

    color = GetSysColor(COLOR_WINDOW);
    if (((WORD)LOBYTE(LOWORD(color)) +
             (WORD)HIBYTE(LOWORD(color)) +
         (WORD)LOBYTE(HIWORD(color))) >= 3*127)
      {
        return RGB(0,0,0);
      }
    else
      {
        return RGB(255,255,255);
      }
}

#define REVERSEPAINT

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/


void NEAR PASCAL ReverseGroupList(PGROUP pGroup)
{
    PITEM pitem, p1, p2;

    for (p1 = pGroup->pItems, p2 = p1->pNext, p1->pNext = NULL; p2; ) {
	    pitem = p2->pNext;
	    p2->pNext = p1;
	    p1 = p2;
	    p2 = pitem;
	}
    pGroup->pItems = p1;
}

VOID PASCAL PaintGroup(HWND hwnd)
{
    register HDC hdc;
    PGROUP pGroup;
    PITEM pitem;
    PAINTSTRUCT ps;
    LPGROUPDEF lpgd;
    LPITEMDEF lpid;
    int fontheight;
    HBRUSH hbrTitle;
    HFONT hFontT;
    RECT rcWin;
    TEXTMETRIC tm;

    HDC hdcMem;
    HBITMAP hbmTemp;
    HBRUSH hbr, hbrTemp;
    INT nMax;
    HICON hIcon;

    pGroup = (PGROUP)GetWindowLongPtr(hwnd, GWLP_PGROUP);

    hdc = BeginPaint(hwnd, &ps);
    if (!pGroup->pItems) {
        goto Exit;
    }

    GetClientRect(hwnd, &rcWin);
#ifdef WEIRDBUG
/* DavidPe - 05/15/91
 *  For some reason RectVisible() will return FALSE in
 *  situations it shouldn't.  Since this is only a
 *  performance optimization, we can ignore the problem
 *  for now.
 */
    if (!RectVisible(hdc, &rcWin))
        goto Exit;
#endif

    if (!(lpgd = LockGroup(hwnd)))
        goto Exit;

    hFontT = SelectObject(hdc, hFontTitle);

    GetTextMetrics(hdc, &tm);

    // ToddB: This seems like a good point, I don't see why tmExternalLeading should ever
    //      need to be considered.  The result of decreasing the FontHeight is that DrawText
    //      will be used instead of TextOut in some cases, which should be harmless but might
    //      effect the apearence of some single line icon titles.

//#ifdef JAPAN
    // Why we should think about ExternalLeading though we calculate the
    // title rectange by DrawText() without DT_EXTERNALLEADING. -YN
    fontheight = tm.tmHeight;
//#else
//    fontheight = tm.tmHeight + tm.tmExternalLeading;
//#endif

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, GetTitleTextColor());
    hbrTitle = NULL;

    hdcMem = CreateCompatibleDC(hdc);
    if (!hdcMem)
        goto BitmapSetupComplete;

    if (pGroup->hbm) {
        hbmTemp = SelectObject(hdcMem, pGroup->hbm);
        if (hbmTemp)
            goto BitmapSetupComplete;
        else
            DeleteObject(pGroup->hbm);
    }

    for (nMax = 1, pitem = pGroup->pItems; pitem; pitem = pitem->pNext) {
	    if (nMax <= pitem->iItem)
	        nMax = pitem->iItem + 1;
    }

    pGroup->hbm = CreateDiscardableBitmap(hdc, cxIcon*lpgd->cItems, cyIcon);
    if (!pGroup->hbm)
        goto NukeMemDC;

    hbmTemp = SelectObject(hdcMem, pGroup->hbm);
    if (!hbmTemp)
        goto NukeBitmap;

    hbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    if (!hbr)
        goto NukeBitmap;

    hbrTemp = SelectObject(hdcMem, hbr);
    if (!hbrTemp)
        goto NukeBrush;

    PatBlt(hdcMem, 0, 0, cxIcon * lpgd->cItems, cyIcon, PATCOPY);
    SelectObject(hdcMem, hbrTemp);
    DeleteObject(hbr);

    for (pitem = pGroup->pItems; pitem != NULL; pitem = pitem->pNext) {
        if (hIcon = GetItemIcon(hwnd, pitem)) {
            DrawIcon(hdcMem, pitem->iItem * cxIcon, 0, hIcon);
            DestroyIcon(hIcon);
        } else {
            goto DeselectAndNukeBitmap;
        }
    }

    goto BitmapSetupComplete;

NukeBrush:
     DeleteObject(hbr);

DeselectAndNukeBitmap:
    SelectObject(hdcMem, hbmTemp);

NukeBitmap:
     DeleteObject(pGroup->hbm);
     pGroup->hbm = NULL;

NukeMemDC:
     DeleteDC(hdcMem);
     hdcMem = NULL;

BitmapSetupComplete:

    ReverseGroupList(pGroup); // reverse the icon list

    /* Paint the icons */
    for (pitem = pGroup->pItems; pitem; pitem = pitem->pNext) {
      	if (!pitem->pNext
    	      && pGroup == pCurrentGroup
	          && hwndProgman == GetActiveWindow()) {
	        hbrTitle = (HANDLE)1;       // Use it as a flag
    	}
    	else
    	    hbrTitle = (HANDLE)0;

    	lpid = ITEM(lpgd,pitem->iItem);

	    if (!bMove || !hbrTitle) {
	        if (hdcMem) {
        		BitBlt(hdc, pitem->rcIcon.left + cxOffset,
		               pitem->rcIcon.top + cyOffset,
        		       lpgd->cxIcon, lpgd->cyIcon, hdcMem,
		               lpgd->cxIcon*pitem->iItem, 0,
        		       SRCCOPY);
	        }
            else {
	    	    if (RectVisible(hdc,&pitem->rcIcon)) {
		            if (hIcon = GetItemIcon(hwnd,pitem)) {
		            	DrawIcon(hdc, pitem->rcIcon.left + cxOffset,
                				 pitem->rcIcon.top + cyOffset, hIcon);
        		    }
                    else {
		            	PatBlt(hdc,pitem->rcIcon.left + cxOffset,
            			       pitem->rcIcon.top + cyOffset,
			                   cxIcon, cyIcon, BLACKNESS);
        		    }
		        }
    	    }
	    }
    }

    /* Paint the titles. */
    for (pitem = pGroup->pItems; pitem; pitem = pitem->pNext) {
        /* test for the active icon */

      	if (!pitem->pNext
    	      && pGroup == pCurrentGroup
	          && hwndProgman == GetActiveWindow()) {
    	    SetTextColor(hdc, GetSysColor(COLOR_CAPTIONTEXT));
	        hbrTitle = CreateSolidBrush(GetSysColor(COLOR_ACTIVECAPTION));
    	}
        else {
	        hbrTitle = (HANDLE)0;
    	}

	    lpid = ITEM(lpgd,pitem->iItem);

    	if (!bMove || !hbrTitle) {
	        if (hbrTitle)
        		FillRect(hdc, &(pitem->rcTitle), hbrTitle);

	        /* draw multi line titles like USER does */

    	    if (pitem->rcTitle.bottom - pitem->rcTitle.top < fontheight*2)
                TextOut(hdc, pitem->rcTitle.left+cxOffset, pitem->rcTitle.top,
                        (LPTSTR) PTR(lpgd, lpid->pName), lstrlen((LPTSTR) PTR(lpgd, lpid->pName)));
            else {
                if (RectVisible(hdc,&pitem->rcTitle)) {
	            DrawText(hdc,
                             (LPTSTR)PTR(lpgd, lpid->pName), -1,
                             &(pitem->rcTitle),
                       	     bIconTitleWrap ?
                    	        DT_CENTER | DT_WORDBREAK | DT_NOPREFIX :
                                DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_SINGLELINE);

                }
    	    }
        }


        if (hbrTitle) {
            SetTextColor(hdc, GetTitleTextColor());
            DeleteObject(hbrTitle);
            hbrTitle = NULL;
        }
    }

    ReverseGroupList(pGroup);	// re-reverse the icon list

    if (hFontT) {
        SelectObject(hdc, hFontT);
    }

    if (hdcMem) {
        SelectObject(hdcMem, hbmTemp);
        DeleteDC(hdcMem);
    }

    UnlockGroup(hwnd);

Exit:
    SetBkMode(hdc, OPAQUE);
    EndPaint(hwnd, &ps);

#ifdef DEBUG
    ProfStop();
    {
    TCHAR buf[80];
    wsprintf(buf, TEXT("msec to paint group = %ld\r\n"), GetTickCount() - dwStartTime);
    OutputDebugString(buf);
    }
#endif
}

/*-------------------------------------------------------------------------*/
/*                                                                         */
/* Draw the group icon.                                                    */
/*                                                                         */
/*-------------------------------------------------------------------------*/
VOID DrawGroupIcon(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hDC;
    PGROUP pGroup;

    hDC = BeginPaint(hwnd, &ps);
    pGroup = (PGROUP)GetWindowLongPtr(hwnd,GWLP_PGROUP);
    if (pGroup->fCommon) {
        DrawIcon(hDC, cxOffset, cyOffset, hCommonGrpIcon);
    }
    else {
        DrawIcon(hDC, cxOffset, cyOffset, hGroupIcon);
    }
    EndPaint(hwnd, &ps);
}


PITEM PASCAL ItemHitTest(
    PGROUP pGroup,
    POINTS pts)
{
    PITEM pItem;
    POINT pt;

    pt.x = (int)pts.x;
    pt.y = (int)pts.y;

    for (pItem = pGroup->pItems; pItem; pItem = pItem->pNext) {
        if (PtInRect(&pItem->rcIcon, pt) || PtInRect(&pItem->rcTitle, pt)) {
            break;
        }
    }

    return pItem;
}


HRGN PASCAL IconExcludedRgn(PGROUP pGroup, PITEM pItem)
{
    RECT rc;
    PITEM pItemT;
    HRGN hrgn, hrgnT;

    hrgn = CreateRectRgn(0,0,0,0);

    if (!hrgn)
        return NULL;

    hrgnT = CreateRectRgn(0,0,0,0);
    if (!hrgnT)
      {
        return hrgn;
      }

    for (pItemT = pGroup->pItems;
         pItemT && pItemT != pItem;
         pItemT = pItemT->pNext)
      {
        if (IntersectRect(&rc,&pItem->rcIcon,&pItemT->rcIcon))
          {
            SetRectRgn(hrgnT,rc.left,rc.top,rc.right,rc.bottom);
            CombineRgn(hrgn,hrgn,hrgnT,RGN_OR);
          }
        if (IntersectRect(&rc,&pItem->rcIcon,&pItemT->rcTitle))
          {
            SetRectRgn(hrgnT,rc.left,rc.top,rc.right,rc.bottom);
            CombineRgn(hrgn,hrgn,hrgnT,RGN_OR);
          }
      }

    DeleteObject(hrgnT);

    return hrgn;
}

VOID APIENTRY InvalidateIcon(PGROUP pGroup, PITEM pItem)
{
    RECT rc;

    if (!pGroup || !pItem)
        return;
    UnionRect(&rc, &pItem->rcIcon, &pItem->rcTitle);
    if (bAutoArranging)
        UnionRect(&rcArrangeRect, &rcArrangeRect, &rc);
    else
        InvalidateRect(pGroup->hwnd,&rc,TRUE);
}

VOID PASCAL BringItemToTop(PGROUP pGroup, PITEM pItem, BOOL fUpdate)
{
    PITEM pItemT;
    HRGN hrgn;

    if (pItem == pGroup->pItems) {
        return;
    }

    if (hrgn = IconExcludedRgn(pGroup, pItem)) {
        InvalidateRgn(pGroup->hwnd, hrgn, TRUE);
        DeleteObject(hrgn);
    }

    /*
     * At this point we know there is at least two items, and we're not the
     * first one...
     */

    for (pItemT = pGroup->pItems; pItemT->pNext != pItem; pItemT = pItemT->pNext)
        ;

    pItemT->pNext = pItem->pNext;
    pItem->pNext = pGroup->pItems;
    pGroup->pItems = pItem;

    /*
     * Invalidate the whole titles in order to change the color.
     */
    if (fUpdate) {
        InvalidateRect(pGroup->hwnd, &pItem->rcTitle, TRUE);
        InvalidateRect(pGroup->hwnd, &pItem->pNext->rcTitle, TRUE);
    }
}

VOID PASCAL ClickOn(HWND hwnd, POINTS pts)
{
    PGROUP pGroup;
    PITEM pItem;
    POINT pt;

    pGroup = (PGROUP)GetWindowLongPtr(hwnd, GWLP_PGROUP);

    pItem = ItemHitTest(pGroup, pts);
    if (!pItem) {
        return;
    }

    BringItemToTop(pGroup, pItem, TRUE);
    ViewActiveItem(pGroup);

    pt.x = (int)pts.x;
    pt.y = (int)pts.y;

    *(LPPOINT)&rcDrag.left = pt;
    *(LPPOINT)&rcDrag.right = pt;
    hwndDrag = hwnd;

    InflateRect(&rcDrag, GetSystemMetrics(SM_CXDOUBLECLK) / 2,
            GetSystemMetrics(SM_CYDOUBLECLK) / 2);
}


VOID PASCAL DragItem(HWND hwnd)
{
    PGROUP pGroup;
    PITEM pItem;
    HICON hIcon;

    pGroup = (PGROUP)GetWindowLongPtr(hwnd,GWLP_PGROUP);
    pItem = pGroup->pItems;

    if (!pItem || hwndDrag != hwnd)
        goto ProcExit;

    /* If the control key isn't down, do a Move operation. */
    bMove = !(GetKeyState(VK_CONTROL) & 0x8000);

    /* Don't allow "moves" from RO groups. */
    if (pGroup->fRO && bMove == TRUE)
        goto ProcExit;

    /*
     * Redraw the window minus the item we're moving.
     * REVIEW - if you just painted the background colour into the
     * pItem->rcIcon area then you could remove the bMove code from
     * PaintGroup().
     */
    if (bMove) {
        InvalidateIcon(pGroup,pItem);
        UpdateWindow(hwnd);
    }

    hIcon = GetItemIcon(hwnd,pItem);

    // BUG BUG  MAKELONG(pGroup,pItem) doesn't make sense since all
    // pointers all LOMG in WIN32. Will need to change the parameters!
    // johannec 08-19-91
    if (DragObject(hwndMDIClient, hwnd, (UINT)OBJ_ITEM,
                   MAKELONG(pGroup,pItem), hIcon) == DRAG_COPY) {
        if (bMove)
            DeleteItem(pGroup,pItem);
    }
    else {
        /* Drag was SWP or drag failed... just show the item. */
        if (bMove) {
            bMove = FALSE;
            InvalidateIcon(pGroup,pItem);
        }
    }
    DestroyIcon(hIcon);
ProcExit:
    bMove = FALSE;
}


void PASCAL GetRealClientRect(
    HWND   hwnd,
    DWORD  dwStyle,
    LPRECT lprcClient)
{
    DWORD Style;

    Style = GetWindowLong(hwnd, GWL_STYLE);

        /*BUG BUG will GWL_STYLE work???*/

    SetWindowLong(hwnd,GWL_STYLE,dwStyle);
    GetWindowRect(hwnd,lprcClient);
    ScreenToClient(hwnd,(LPPOINT)&lprcClient->left);
    ScreenToClient(hwnd,(LPPOINT)&lprcClient->right);
    SendMessage(hwnd,WM_NCCALCSIZE,0,(LPARAM)lprcClient);
}


VOID APIENTRY CalcGroupScrolls(HWND hwnd)
{
    register PGROUP pGroup;
    register PITEM pItem;
    RECT rcClient;
    RECT rcRange;
    RECT rcT;
    DWORD dwStyle, dwStyleNew, dwStyleT;
    int iMinPos, iMaxPos;

    if (bNoScrollCalc || IsIconic(hwnd))
        return;

    // Stop re-entrance of this routine.
    bNoScrollCalc = TRUE;

    pGroup = (PGROUP)GetWindowLongPtr(hwnd,GWLP_PGROUP);

    if (!pGroup->pItems) {
        // No items...
        SetRectEmpty(&rcRange);
    	goto ChangeStyle;
    }

    hwndScrolling = hwnd;

    // If the user has selected auto arranging then make
    // the next item in the z-order visable.
    if (bAutoArrange)
        ViewActiveItem(pGroup);

    SetRectEmpty(&rcRange);

    for (pItem = pGroup->pItems; pItem; pItem = pItem->pNext)
      {
        UnionRect(&rcRange,&rcRange,&pItem->rcIcon);
    	rcT.top = pItem->rcTitle.top;		// don't include the
	    rcT.bottom = pItem->rcTitle.bottom;	// title overhang part
    	rcT.left = pItem->rcIcon.left;
	    rcT.right = pItem->rcIcon.right;
        UnionRect(&rcRange,&rcRange,&rcT);
      }

    if (rcRange.left != rcRange.right)
      {
        // Add on a bit for the left border here.
        rcRange.left -= ((cxArrange-cxIconSpace)/2)+cxOffset;
        // Don't add on a right border so we can cram as many icons in as poss.
        // rcRange.right += ((cxArrange-cxIconSpace)/2);

        // ADJUST THE RECT SO THAT WE DON'T GET SCROLL BARS IF ONLY THE BORDERS
        // OF TEXT ARE NOT VISIBLE ~~~
      }

ChangeStyle:

    dwStyleNew = dwStyle = GetWindowLong(hwnd,GWL_STYLE);

    dwStyleNew &= ~(WS_HSCROLL | WS_VSCROLL);

    for (;;)
      {
        dwStyleT = dwStyleNew;
        GetRealClientRect(hwnd, dwStyleNew, &rcClient);

        if (rcRange.left < rcClient.left || rcRange.right > rcClient.right)
            dwStyleNew |= WS_HSCROLL;
        if (rcRange.top < rcClient.top || rcRange.bottom > rcClient.bottom)
            dwStyleNew |= WS_VSCROLL;

        if (dwStyleNew == dwStyleT)
            break;
      }

    if (dwStyleNew == dwStyle && !(dwStyle & (WS_VSCROLL|WS_HSCROLL)))
      {
        /* none there and don't need to add 'em!
         */
        goto ProcExit;
      }

    UnionRect(&rcRange,&rcClient,&rcRange);

    /* union garantees that left==right or top==bottom in case of no
     * scrollbar.
     */
    rcRange.right -= rcClient.right-rcClient.left;
    rcRange.bottom -= rcClient.bottom-rcClient.top;

    /* if the style changed, don't redraw in sb code, just redraw the
     * frame at the end cause the whole ncarea has to be repainted
     * if it hasn't changed, just move the thumb
     */

    if (dwStyleNew==dwStyle)
      {
        if (dwStyleNew & WS_HSCROLL)
          {
            if (GetScrollPos(hwnd,SB_HORZ)!=0)
                goto SetScrollInfo;
            GetScrollRange(hwnd,SB_HORZ,&iMinPos,&iMaxPos);
            if ((iMinPos != rcRange.left) || (iMaxPos != rcRange.right))
                goto SetScrollInfo;
          }
        if (dwStyleNew & WS_VSCROLL)
          {
            if (GetScrollPos(hwnd,SB_VERT)!=0)
                goto SetScrollInfo;
            GetScrollRange(hwnd,SB_VERT,&iMinPos,&iMaxPos);
            if ((iMinPos != rcRange.top) || (iMaxPos != rcRange.bottom))
                goto SetScrollInfo;
          }
        goto ProcExit;
      }

SetScrollInfo:
    SetScrollPos(hwnd,SB_HORZ,0,FALSE);
    SetScrollPos(hwnd,SB_VERT,0,FALSE);
    SetScrollRange(hwnd,SB_HORZ,rcRange.left,rcRange.right,FALSE);
    SetScrollRange(hwnd,SB_VERT,rcRange.top,rcRange.bottom,FALSE);

    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE
                                         | SWP_NOMOVE | SWP_NOACTIVATE
                                         | SWP_DRAWFRAME);
ProcExit:
    // Allow other scroll calculations.
    bNoScrollCalc = FALSE;
}


VOID PASCAL ScrollGroup(PGROUP pGroup, int xMove, int yMove, BOOL fCalc)
{
    register PITEM pItem;

    for (pItem = pGroup->pItems; pItem; pItem = pItem->pNext)
      {
        OffsetRect(&pItem->rcIcon,xMove,yMove);
        OffsetRect(&pItem->rcTitle,xMove,yMove);
      }
    ScrollWindow(pGroup->hwnd,xMove,yMove,NULL,NULL);

    UpdateWindow(pGroup->hwnd);

    if (fCalc)
        CalcGroupScrolls(pGroup->hwnd);
}

VOID APIENTRY ViewActiveItem(PGROUP pGroup)
{
    RECT rcClient, rc;
    int xMove = 0, yMove = 0;

    GetClientRect(pGroup->hwnd,&rcClient);

    UnionRect(&rc, &pGroup->pItems->rcIcon, &pGroup->pItems->rcTitle);
    // Clip width to that of icon i.e. ignore width of text.
    rc.left = pGroup->pItems->rcIcon.left;
    rc.right = pGroup->pItems->rcIcon.right;


    if (rc.left < rcClient.left)
        xMove = rcClient.left - rc.left;
    else if (rc.right > rcClient.right)
        xMove = rcClient.right - rc.right;

    if (rc.top < rcClient.top)
        yMove = rcClient.top - rc.top;
    else if (rc.bottom > rcClient.bottom)
        yMove = rcClient.bottom - rc.bottom;

    if (xMove || yMove)
        ScrollGroup(pGroup, xMove, yMove,TRUE);
}


BOOL FAR PASCAL CheckHotKey(WPARAM wParam, LPARAM lParam)
{
    HWND hwndT;
    PGROUP pGroup;
    PITEM pItem;

    switch (wParam)
    {
    case VK_SHIFT:
    case VK_CONTROL:
    case VK_MENU:
    case VK_RETURN:
        return FALSE;
    }

    if (GetKeyState(VK_SHIFT) < 0) {
    	// DBG((" + SHIFT"));
	    wParam |= HK_SHIFT;
    }
    if (GetKeyState(VK_CONTROL) < 0) {
    	// DBG((" + CONTROL"));
	    wParam |= HK_CONTROL;
    }
    if (GetKeyState(VK_MENU) < 0) {
    	// DBG((" + ALT"));
	    wParam |= HK_ALT;
    }
    if (lParam & F_EXT) {
    	// DBG((" EXTENDED"));
	    wParam |= HK_EXT;
    }

    // DBG(("... Full code %4.4X...\r\n",wParam));

    for (hwndT = GetWindow(hwndMDIClient,GW_CHILD);
          hwndT;
          hwndT = GetWindow(hwndT,GW_HWNDNEXT)) {
        if (GetWindow(hwndT,GW_OWNER))
            continue;

        pGroup = (PGROUP)GetWindowLongPtr(hwndT,GWLP_PGROUP);

        for (pItem = pGroup->pItems; pItem; pItem = pItem->pNext) {
    	    // DBG(("Checking (%4.4X,%4.4X)...\r\n",pGroup,pItem));
            if (GroupFlag(pGroup,pItem,(WORD)ID_HOTKEY) == (WORD)wParam) {
	        	// DBG(("F*O*U*N*D\r\n"));
                ExecItem(pGroup,pItem,FALSE,FALSE);
                return TRUE;
            }
        }
    }

    return FALSE;
}


VOID APIENTRY KeyWindow(HWND hwnd, WORD wDirection)
{
    int     wT;
    int     wNext;
    RECT    rc;
    RECT    rcT;
    POINT   ptA;
    POINT   ptT;
    PGROUP  pGroup;
    PITEM   pItem, pItemNext;

    pGroup = (PGROUP)GetWindowLongPtr(hwnd,GWLP_PGROUP);

    if (!pGroup->pItems)
        return;

    wNext = 0x7FFF;
    pItemNext = NULL;
    CopyRect(&rc,&pGroup->pItems->rcIcon);

    for (pItem = pGroup->pItems->pNext; pItem; pItem = pItem->pNext) {
        CopyRect(&rcT,&pItem->rcIcon);
        ptT.x = rcT.left - rc.left;
        ptT.y = rcT.top - rc.top;
        ptA.x = (ptT.x < 0) ? -ptT.x : ptT.x;
        ptA.y = (ptT.y < 0) ? -ptT.y : ptT.y;

        switch (wDirection) {
            case VK_LEFT:
                if ((ptT.x >= 0) || (ptA.x < ptA.y))
                    continue;
                break;

            case VK_RIGHT:
                if ((ptT.x <= 0) || (ptA.x < ptA.y))
                    continue;
                break;

            case VK_DOWN:
                if ((ptT.y <= 0) || (ptA.y < ptA.x))
                    continue;
                break;

            case VK_UP:
                if ((ptT.y >= 0) || (ptA.y < ptA.x))
                    continue;
                break;

            default:
                /* illegal key
                 */
                return;
        }

        wT = ptA.y + ptA.x;

        if (wT <= wNext) {
            wNext = wT;
            pItemNext = pItem;
        }
    }

    if (pItemNext) {
        BringItemToTop(pGroup,pItemNext, TRUE);
        ViewActiveItem(pGroup);
    }
}


VOID APIENTRY CharWindow(register HWND hwnd, register WORD wChar)
{
    LPGROUPDEF lpgd;
    LPITEMDEF lpid;
    PGROUP pGroup;
    PITEM pItem, pItemLast;

    pGroup = (PGROUP)GetWindowLongPtr(hwnd,GWLP_PGROUP);

    if (!pGroup->pItems)
        return;

    lpgd = LockGroup(hwnd);
    if (!lpgd)
        return;

    /* Search for item, skip the currently selected one.*/
    for ( pItem = pGroup->pItems->pNext; pItem; pItem=pItem->pNext)
      {
        lpid = ITEM(lpgd,pItem->iItem);
        if (CharUpper((LPTSTR)(DWORD)wChar)
          == CharUpper((LPTSTR)(DWORD)(BYTE)*PTR(lpgd, lpid->pName)))
          {
            pItemLast = MakeFirstItemLast(pGroup);
            BringItemToTop(pGroup,pItem, FALSE);
            /* Handle updates.*/
            InvalidateRect(pGroup->hwnd,&pItem->rcTitle,TRUE);
            InvalidateRect(pGroup->hwnd,&pItemLast->rcTitle,TRUE);
            ViewActiveItem(pGroup);
            break;
          }
      }
    UnlockGroup(hwnd);
}


VOID APIENTRY ScrollMessage(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    int           wMin;
    int           wMax;
    int           wPos;
    int           wInc;
    int           wPage;
    int           wNewPos;
    RECT          rcClient;
    int           yMove;
    int           xMove;
    BOOL          fTemp;

    GetClientRect(hwnd, &rcClient);

    if (uiMsg == WM_HSCROLL) {
        GetScrollRange(hwnd, SB_HORZ, &wMin, &wMax);
        wPos = GetScrollPos(hwnd, SB_HORZ);
        wInc = cxIconSpace + cxArrange / 2;
        wPage = rcClient.right-rcClient.left;
    }
    else {
        GetScrollRange(hwnd, SB_VERT, &wMin, &wMax);
        wPos = GetScrollPos(hwnd, SB_VERT);
        wInc = cyArrange;
        wPage = rcClient.bottom-rcClient.top;
    }

    switch (GET_WM_VSCROLL_CODE(wParam, lParam)) {
        case SB_BOTTOM:
            wNewPos = wMax;
            break;

        case SB_TOP:
            wNewPos = wMin;
            break;

        case SB_LINEDOWN:
            wNewPos = wPos + wInc;
            break;

        case SB_LINEUP:
            wNewPos = wPos - wInc;
            break;

        case SB_PAGEDOWN:
            wNewPos = wPos + wPage;
            break;

        case SB_PAGEUP:
            wNewPos = wPos - wPage;
            break;

        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            wNewPos = (INT)(SHORT)GET_WM_VSCROLL_POS(wParam, lParam);
            break;

        case SB_ENDSCROLL:
            // We might suddenly not need the scroll bars anymore so
            // check now.
            // Stop CGS from moving the view.
            fTemp = bAutoArrange;
            bAutoArrange = FALSE;
    	    CalcGroupScrolls(hwnd);
            bAutoArrange = fTemp;

            /*** FALL THRU ***/

        default:
            return;
    }

    if (wNewPos < wMin)
        wNewPos = wMin;
    else if (wNewPos > wMax)
        wNewPos = wMax;

    if (uiMsg == WM_VSCROLL) {
        SetScrollPos(hwnd, SB_VERT, wNewPos, TRUE);
        yMove = wPos - wNewPos;
        xMove = 0;
    }
    else {
        SetScrollPos(hwnd, SB_HORZ, wNewPos, TRUE);
        yMove = 0;
        xMove = wPos - wNewPos;
    }

    ScrollGroup((PGROUP)GetWindowLongPtr(hwnd,GWLP_PGROUP),xMove,yMove,FALSE);
}


VOID PASCAL OfficialRect(
    LPRECT lprc,
    int x,
    int y,
    int xOffset,
    int yOffset)
{
    // Work out were the icon should go in the icon grid, taking
    // note of where the scroll bars are.

    lprc->right = (lprc->left = x-xOffset + (cxIconSpace - cxArrange) / 2) +
            cxArrange - 1;
    lprc->bottom = (lprc->top = y-yOffset) + cyArrange - 1;
}


BOOL PASCAL IconOverlaps(
    PGROUP pGroup,
    LPRECT lprc,
    int xOffset,
    int yOffset)
{
    PITEM pItem;
    RECT rcT;

    for (pItem = pGroup->pItems; pItem; pItem = pItem->pNext) {
        // Ignore icons at -1. This is where icon's get put when
        // we don't know where to put them and they will get moved
        // later.
        if (pItem->rcIcon.left == -1) {
            continue;
        }

        OfficialRect(&rcT, pItem->rcIcon.left, pItem->rcIcon.top, xOffset, yOffset);
        if (IntersectRect(&rcT, &rcT, lprc)) {
            return TRUE;
        }
    }
    return FALSE;
}


/*
 * NB This is called for every icon at init time so put anything to do with
 * finding icon positions inside the `if' because that's skipped on init.
 * If you don't it'll get tres slow.
 */

VOID PASCAL ComputeIconPosition(
    PGROUP pGroup,
    POINT pt,
    LPRECT lprcIcon,
    LPRECT lprcTitle,
    LPTSTR lpText)
{
    HDC hdc;
    int cch;
    RECT rcClient, rcT;
    HFONT hFontT;
    int xsp, ysp;       // Current position of scrollbar.
    int vMax, vMin;     // Range.
    int hMax, hMin;     // Range.
    int xOffset, yOffset;
    DWORD dwStyle;

    if (pt.x == -1) {
        /*
         * Icon is in "find me a default position" mode...
         * so search the icon space for it...
         */
        // Get the current window style.
        dwStyle = GetWindowLong(pGroup->hwnd,GWL_STYLE);

        if (dwStyle & WS_MINIMIZE) {
            // DBG(("PM.CIP: Window Minimised\n\r"));
            // We want to use the restored state of the window.
            GetInternalWindowPos(pGroup->hwnd, &rcClient, NULL);
            // Convert from screen coords to client coords.
            OffsetRect(&rcClient, -rcClient.left, -rcClient.top);
        }
        else {
            // DBG(("PM.CIP: Window normal or maxed.\n\r"));
            // Take into account scroll bars.
            GetClientRect(pGroup->hwnd, &rcClient);
        }

        if (dwStyle & WS_HSCROLL) {
             xsp = GetScrollPos(pGroup->hwnd, SB_HORZ);
             GetScrollRange(pGroup->hwnd, SB_HORZ, &hMin, &hMax);
             xOffset = xsp-hMin;     // Offset icon grid to match scroll bar pos.
        }
        else {
             xOffset = 0;
        }

        if (dwStyle & WS_VSCROLL) {
             ysp = GetScrollPos(pGroup->hwnd, SB_VERT);
             GetScrollRange(pGroup->hwnd, SB_VERT, &vMin, &vMax);
             yOffset = ysp-vMin;     // Offset icon grid.
        }
        else {
             yOffset = 0;
        }

        pt.x = (cxArrange - cxIconSpace) / 2 + cxOffset - xOffset;
        pt.y = 0 - yOffset;
        /* Set this icon's left to -1 so that it'll be excluded
         * by the IconOverlaps check.
         */
        lprcIcon->left = -1;

        for (;;) {
            OfficialRect(&rcT, pt.x, pt.y, xOffset, yOffset);

            if (!IconOverlaps(pGroup, &rcT, xOffset, yOffset)) {
                break;
            }

            if (rcT.right + cxArrange > rcClient.right) {
                pt.x = (cxArrange-cxIconSpace)/2 + cxOffset - xOffset;
                pt.y += cyArrange;
            }
            else {
                pt.x += cxArrange;
            }
        }
    }

    SetRect(lprcIcon, pt.x, pt.y, pt.x+cxIconSpace, pt.y+cyIconSpace);

    if (IsRectEmpty(lprcTitle)) {
        cch = lstrlen(lpText);

        hdc = GetDC(pGroup->hwnd);
        hFontT = SelectObject(hdc, hFontTitle);

        /*
         * Compute the icon rect using DrawText.
         */
        lprcTitle->right = cxArrange - (2 * cxOffset);
        DrawText(hdc, lpText, -1, lprcTitle, bIconTitleWrap ?
            (WORD)(DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX) :
            (WORD)(DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX | DT_SINGLELINE));

        if (hFontT) {
            SelectObject(hdc, hFontT);
        }
        ReleaseDC(pGroup->hwnd, hdc);
        lprcTitle->right += cxOffset * 2;
        lprcTitle->bottom += dyBorder * 2;

    }
    else {
        SetRect(lprcTitle, 0, 0, lprcTitle->right - lprcTitle->left,
                lprcTitle->bottom - lprcTitle->top);
    }


    OffsetRect(lprcTitle, pt.x+(cxIconSpace/2)-((lprcTitle->right-lprcTitle->left)/2),
                  pt.y + cyIconSpace - dyBorder);

// REVIEW Very expensive to do this here.
//    if ((bAutoArrange) && (!bAutoArranging))
//	      ArrangeItems(pGroup->hwnd);
}

VOID APIENTRY MoveItem(PGROUP pGroup, PITEM pItem, POINT pt)
{
    LPITEMDEF lpid;
    LPGROUPDEF lpgd;

    lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);

    /*
     * If the position is the same, ignore
     */
    if ((pt.x == pItem->rcIcon.left) && (pt.y == pItem->rcIcon.top)) {
        GlobalUnlock(pGroup->hGroup);
        return;
    }

    /*
     * Repaint the original position
     */
    InvalidateIcon(pGroup, pItem);

    lpid = LockItem(pGroup,pItem);
    if (!lpid) {
        GlobalUnlock(pGroup->hGroup);
        return;
    }

    ComputeIconPosition(pGroup, pt, &pItem->rcIcon, &pItem->rcTitle,
            (LPTSTR) PTR(lpgd, lpid->pName));


    UnlockGroup(pGroup->hwnd);
    GlobalUnlock(pGroup->hGroup);

    /*
     * Repaint the new position
     */
    InvalidateIcon(pGroup,pItem);

//  CalcGroupScrolls(pGroup->hwnd);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DropObject() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

LONG NEAR PASCAL DropObject(HWND hWnd, LPDROPSTRUCT lpds)
{
  BOOL          fNC;
  BOOL          fOk;
  POINT         pt;
  LPPOINT       lppt;
  PGROUP        pGroup;
  PITEM         pItem;
  RECT          rcClient;

  pGroup = pCurrentGroup;
  pItem = pGroup->pItems;

  pt = lpds->ptDrop;

  // A drop anywhere in the window is valid.
  GetWindowRect(hWnd, &rcClient);
  // Convert to client coords.
  ScreenToClient(hWnd,(LPPOINT)&(rcClient.left));
  ScreenToClient(hWnd,(LPPOINT)&(rcClient.right));

  if (pt.x >= rcClient.left && pt.y >= rcClient.top && pt.x <= rcClient.right
                                                && pt.y <= rcClient.bottom) {
      /* Dropped in given point of client area. */
      fNC = FALSE;
      pt.x -= (GetSystemMetrics(SM_CXICON) / 2) + 2;
      pt.y -= (GetSystemMetrics(SM_CYICON) / 2) + 2;
      lppt = &pt;
  }
  else {
      /* Dropped in nonclient area. */
      fNC = TRUE;
      lppt = NULL;
  }

  /* Are we iconic ? */
  if (IsIconic(hWnd)) {
      // Yep, we'll need to use default positioning.
      fNC = TRUE;
      lppt = NULL;
  }

#if 0
  // this if statement code if obsolete, it is never called. - johannec 8/11/93
  if (lpds->wFmt == DOF_EXECUTABLE || lpds->wFmt == DOF_DOCUMENT) {

      BuildDescription(szNameField, szPathName);

      return((LONG)(CreateNewItem(hWnd,
                szNameField, szPathName, szPathName, TEXT(""),
                0, FALSE, 0, 0, NULL, lppt, CI_SET_DOS_FULLSCRN) != NULL));
  }
#endif

  if ((hWnd == pGroup->hwnd) && (bMove)) {
      /* Don't drop on our own non-client area. */
      if (fNC)
          return 0L;

      /* We are just moving the item within its own group.
       * Hide it first so the icon title is treated correctly.
       */
      MoveItem(pGroup,pItem, pt);
      if ((bAutoArrange) && (!bAutoArranging))
          ArrangeItems(pGroup->hwnd);
      else if (!bAutoArranging)
          CalcGroupScrolls(pGroup->hwnd);

      return(DRAG_SWP);
  }
  else {
      /* Copy the item to the new group...  Set the hourglass
       * cursor (it will get unset after the message returns),
       * select the new group, and add the item at the specified
       * point.
       */
      fOk = DuplicateItem(pGroup,pItem,
                  (PGROUP)GetWindowLongPtr(hWnd,GWLP_PGROUP),lppt) != NULL;

      /*
       * Re-Arrange items within the destination group.
       * NB The source will been taken care of by the DeleteItem routine
       * called from DragItem.
       */
      if ((bAutoArrange) && (!bAutoArranging)) {
          /* Destination */
          ArrangeItems(hWnd);
      }
      else if (!bAutoArranging) {
          /* Destination */
          CalcGroupScrolls(hWnd);
      }

      /* View the current item. */
      BringItemToTop(pGroup,pItem, TRUE);
      ViewActiveItem(pGroup);

      /* If the dest isn't minimised then move the focus to it. */
      if (!IsIconic(hWnd))
           SetFocus(hWnd);
      return (fOk ? DRAG_COPY : 0L);
  }
}

LONG APIENTRY DropFiles(HWND hwnd, HANDLE hDrop)
{
    POINT pt;
    LPPOINT lppt;
    UINT i;
    HCURSOR hCursor;
    DWORD dwRet;
    DWORD dwFlags = CI_ACTIVATE | CI_SET_DOS_FULLSCRN;

    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    ShowCursor(TRUE);

    if (!DragQueryPoint(hDrop, &pt) ||
                   DragQueryFile(hDrop, (UINT)-1, NULL, 0) != 1) {
        lppt = NULL;
    }
    else {
        pt.x -= (GetSystemMetrics(SM_CXICON) / 2) + 2;
        pt.y -= (GetSystemMetrics(SM_CYICON) / 2) + 2;
        lppt = &pt;
    }

    for (i=0; DragQueryFile(hDrop, i, szPathField, MAXITEMPATHLEN); i++) {
        //
        // if filename or directory have spaces, put the path
        // between quotes.
        //
        CheckEscapes(szPathField, MAXITEMPATHLEN+1);

        /* Verify the file's existance... */
        dwRet = ValidatePath(hwndProgman, szPathField, NULL, szIconPath);
        if (dwRet == PATH_INVALID) {
            continue;
	    }
	else if (dwRet == PATH_INVALID_OK) {
	    dwFlags |= CI_NO_ASSOCIATION;
	    }

        BuildDescription(szNameField,szPathField);

        GetDirectoryFromPath(szPathField, szDirField);
        if (!InQuotes(szDirField)) {
            CheckEscapes(szDirField, MAXITEMPATHLEN+1);
        }

        HandleDosApps(szIconPath);

        if (!CreateNewItem(hwnd,
                      szNameField,                /* name*/
                      szPathField,                /* command*/
                      szIconPath ,                /* icon path*/
                      szDirField,                 /* no default dir*/
                      0,0,                        /* no hotkey, no min on run*/
                      0,0,0,                      /* default icon*/
                      lppt,                       /* at this point*/
                      dwFlags))
                break;
    }

    DragFinish(hDrop);

    ShowCursor(FALSE);
    SetCursor(hCursor);

    if ((bAutoArrange) && (!bAutoArranging))
        ArrangeItems(hwnd);
    else if (!bAutoArranging)
        CalcGroupScrolls(hwnd);

    return 1L;
}

LRESULT APIENTRY GroupWndProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    POINT pt;

    switch (uiMsg) {

    case WM_CREATE:
    {
        LPMDICREATESTRUCT lpmdics;

        lpmdics = (LPMDICREATESTRUCT)(((LPCREATESTRUCT)lParam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_PGROUP, lpmdics->lParam);
        DragAcceptFiles(hwnd,TRUE);
        SetFocus(hwnd);
        break;
    }

    case WM_ERASEBKGND:
        if (IsIconic(hwnd)) {
            //
            // Erase background with the APPWORKSPACE color
            //
            RECT rc;

	    if (!hbrWorkspace)
		hbrWorkspace = CreateSolidBrush(GetSysColor(COLOR_APPWORKSPACE));
            GetUpdateRect(hwnd, &rc, FALSE);
            if (IsRectEmpty(&rc)) {
                GetClientRect(hwnd, &rc);
            }
            FillRect((HDC)wParam, &rc, hbrWorkspace);
        }
        else {
            goto DefProc;
        }
        break;

    case WM_PAINT:
        if (IsIconic(hwnd)) {
            DrawGroupIcon(hwnd);
        }
        else {
            PaintGroup(hwnd);
        }
        break;

    case WM_QUERYDRAGICON:
    {
        PGROUP pGroup;
        HICON hIcon = NULL;

        pGroup = (PGROUP)GetWindowLongPtr(hwnd, GWLP_PGROUP);

        if (pGroup->fCommon) {
            hIcon = LoadIcon(hAppInstance, (LPTSTR) MAKEINTRESOURCE(COMMGROUPICON));
        }
        else {
            hIcon = LoadIcon(hAppInstance, (LPTSTR) MAKEINTRESOURCE(PERSGROUPICON));
        }

        if (hIcon) {
            return((LRESULT)hIcon);
        }
        else {
            goto DefProc;
        }

        break;
    }

    case WM_LBUTTONDOWN:
        ClickOn(hwnd, MAKEPOINTS(lParam));
        break;

    case WM_MOUSEMOVE:
        if (wParam & MK_LBUTTON) {
            pt.x = (int)(MAKEPOINTS(lParam).x);
            pt.y = (int)(MAKEPOINTS(lParam).y);
            if (!IsRectEmpty(&rcDrag) && !PtInRect(&rcDrag, pt)
                                    && !fNoFileMenu && (dwEditLevel < 2)) {
                SetRect(&rcDrag,0,0,0,0);
                DragItem(hwnd);
            }
        }
        else {
            SetRect(&rcDrag,0,0,0,0);
        }
        break;

    case WM_LBUTTONUP:
        SetRect(&rcDrag,0,0,0,0);
        break;

    case WM_NCLBUTTONDBLCLK:
        if (IsIconic(hwnd) && (GetKeyState(VK_MENU) < 0)) {
            PostMessage(hwndProgman, WM_COMMAND, IDM_PROPS, 0L);
        } else {
            goto DefProc;
        }
        break;

    case WM_LBUTTONDBLCLK:

        if (ItemHitTest((PGROUP)GetWindowLongPtr(hwnd,GWLP_PGROUP),
                        MAKEPOINTS(lParam))) {
            if (GetKeyState(VK_MENU) < 0) {
                if (!fNoFileMenu)
                    PostMessage(hwndProgman,WM_COMMAND,IDM_PROPS,0L);
            } else {
                PostMessage(hwndProgman,WM_COMMAND,IDM_OPEN,0L);
            }

        } else {
            /*
             * Check for Alt-dblclk on nothing to get new item.
             */
            if (GetKeyState(VK_MENU) < 0 && !fNoFileMenu &&
                             (dwEditLevel <= 1) && !(pCurrentGroup->fRO) ) {
                MyDialogBox(ITEMDLG, hwndProgman, NewItemDlgProc);
            }
        }
        break;

    case WM_VSCROLL:
    case WM_HSCROLL:
        ScrollMessage(hwnd,uiMsg,wParam,lParam);
        break;

    case WM_CLOSE:
        SendMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0L);
        break;

    case WM_SYSCOMMAND:
    {
        PGROUP pGroup;
        LPGROUPDEF lpgd;
        TCHAR szCommonGroupSuffix[MAXKEYLEN];
        TCHAR szCommonGroupTitle[2*MAXKEYLEN];

        if (wParam == SC_MINIMIZE) {
            //
            // if the group is common remove the common suffix from the group
            // window title
            //
            pGroup = (PGROUP)GetWindowLongPtr(hwnd, GWLP_PGROUP);
            if (pGroup->fCommon) {
                lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);
                SetWindowText(hwnd, (LPTSTR) PTR(lpgd, lpgd->pName));
                GlobalUnlock(pGroup->hGroup);
            }
        }

        if (wParam == SC_RESTORE) {
            if (!LockGroup(hwnd)) {
                if (wLockError == LOCK_LOWMEM) {
                    MyMessageBox(hwndProgman, IDS_GROUPFILEERR, IDS_LOWMEM, NULL, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
                    break;
                }
                else {
                    /*
                     * Lock failed for some other reason - hopefully just
                     * a group change.  Stop the icon group from being
                     * restored.
                     */
                    break;
                }
            }
            else {
        		    UnlockGroup(hwnd);
		      }
        }
    	  if ((wParam == SC_MAXIMIZE) || (wParam == SC_RESTORE)) {
            //
            // if the group is common add the common suffix to the group
            // window title
            //
            pGroup = (PGROUP)GetWindowLongPtr(hwnd, GWLP_PGROUP);
            if (pGroup->fCommon) {
                lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);

                if (!lpgd)
                   goto DefProc;

                lstrcpy(szCommonGroupTitle, (LPTSTR) PTR(lpgd, lpgd->pName));
                GlobalUnlock(pGroup->hGroup);
                if (LoadString(hAppInstance, IDS_COMMONGRPSUFFIX, szCommonGroupSuffix,
                               CharSizeOf(szCommonGroupSuffix))) {
                    lstrcat(szCommonGroupTitle, szCommonGroupSuffix);
                }
                SetWindowText(pGroup->hwnd, szCommonGroupTitle);
           }
	        InvalidateRect(hwnd, NULL, 0);
        }
        if (wParam == SC_MAXIMIZE)
	         SetWindowLong(hwnd, GWL_STYLE,
	              (GetWindowLong(hwnd,GWL_STYLE) & ~(WS_HSCROLL | WS_VSCROLL)));
    	  goto DefProc;
			
    }
    case WM_SYSKEYDOWN:
        if (!CheckHotKey(wParam,lParam))
            goto DefProc;
        break;

    case WM_KEYDOWN:
        if (!CheckHotKey(wParam,lParam))
            KeyWindow(hwnd,(WORD)wParam);
        break;

    //IME Support
    //by yutakas 1992.10.22
    // When user input DBCS, go and activate icon which has that
    // DBCS charcter in the first of description.
    case WM_IME_REPORT:
        switch (wParam)
        {
            case IR_STRING:
                IMEStringWindow(hwnd,(HANDLE)lParam);
                return TRUE;
            default:
                goto DefProc;
        }
        break;

    case WM_CHAR:
        CharWindow(hwnd, (WORD) wParam);
        break;

    case WM_QUERYDROPOBJECT:
    {
        #define lpds ((LPDROPSTRUCT)lParam)

        PGROUP pGroup;

        pGroup = (PGROUP)GetWindowLongPtr(hwnd,GWLP_PGROUP);

        if (pGroup->fRO) {
            return FALSE;
        }

    	if (lpds->wFmt == OBJ_ITEM) {
    	    return TRUE;
        }
        #undef lpds
    	goto DefProc;
    }

    case WM_DROPOBJECT:
        #define lpds ((LPDROPSTRUCT)lParam)

        if (lpds->wFmt == OBJ_ITEM)
            return DropObject(hwnd, lpds);
        #undef lpds
        goto DefProc;

    case WM_DROPFILES:
        return DropFiles(hwnd,(HANDLE)wParam);

    case WM_NCACTIVATE:
    {
        PGROUP pGroup = (PGROUP)GetWindowLongPtr(hwnd,GWLP_PGROUP);

        if (pGroup->pItems != NULL) {
            InvalidateRect(hwnd,&pGroup->pItems->rcTitle,TRUE);
        }
        goto DefProc;
    }

    case WM_QUERYOPEN:
    {
        PGROUP pGroup;
        LPGROUPDEF lpgd;
        TCHAR szCommonGroupSuffix[MAXKEYLEN];
        TCHAR szCommonGroupTitle[2*MAXKEYLEN];

        //
        // if the group is common add the common suffix to the group
        // window title
        //
        pGroup = (PGROUP)GetWindowLongPtr(hwnd, GWLP_PGROUP);
        if (pGroup->fCommon) {
            lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);

            if (!lpgd)
               goto DefProc;

            lstrcpy(szCommonGroupTitle,(LPTSTR) PTR(lpgd, lpgd->pName));
            GlobalUnlock(pGroup->hGroup);
            if (LoadString(hAppInstance, IDS_COMMONGRPSUFFIX, szCommonGroupSuffix,
                           CharSizeOf(szCommonGroupSuffix))) {
                lstrcat(szCommonGroupTitle, szCommonGroupSuffix);
            }
            SetWindowText(pGroup->hwnd, szCommonGroupTitle);
        }
        goto DefProc;
    }

    case WM_SIZE:
        lParam = DefMDIChildProc(hwnd, uiMsg, wParam, lParam);
        if (wParam != SIZEICONIC) {
            if ((bAutoArrange) && (!bAutoArranging)) {
                ArrangeItems(hwnd);
            } else if (!bArranging) {
                CalcGroupScrolls(hwnd);
            }
        }
        else {
            PGROUP pGroup;
            LPGROUPDEF lpgd;

            //
            // reset window text of common groups
            //
            pGroup = (PGROUP)GetWindowLongPtr(hwnd, GWLP_PGROUP);
            if (pGroup->fCommon) {
                lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);
                SetWindowText(pGroup->hwnd, (LPTSTR) PTR(lpgd, lpgd->pName));
                GlobalUnlock(pGroup->hGroup);
            }
        }
        return lParam;

    case WM_MDIACTIVATE:
        if (!pCurrentGroup) {
            goto DefProc;
        }

        /*
         * If we are de-activating this window...
         */
        if (lParam == 0) {
            /*
             * We're the last window... punt.
             */
            pCurrentGroup = NULL;

        } else if (hwnd == (HWND)wParam) {
            /*
             * We're being deactivated.  Update pCurrentGroup
             * to the node being activated.
             */
            pCurrentGroup = (PGROUP)GetWindowLongPtr((HWND)lParam, GWLP_PGROUP);

        } else {
            SetFocus(hwnd);
        }

        goto DefProc;

    case WM_MENUSELECT:
        //
        // to handle F1 on group window system menu
        //

        if (lParam) {        /*make sure menu handle isn't null*/
            wMenuID =  GET_WM_COMMAND_ID(wParam, lParam);    /*get cmd from loword of wParam*/
            hSaveMenuHandle = (HANDLE)lParam;    /*Save hMenu into one variable*/
            wSaveFlags = HIWORD(wParam);/*Save flags into another*/
            bFrameSysMenu = FALSE;
        }

        break;

    default:
DefProc:
        return DefMDIChildProc(hwnd, uiMsg, wParam, lParam);
    }

    return 0L;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ArrangeItems() -                                                        */
/*                                                                          */
/* Arranges iconic windows within a group.                                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID FAR PASCAL ArrangeItems(HWND hwnd)
{
    PGROUP pGroup;
    register PITEM pItem;
    PITEM pItemT;
    int xSlots;
    register int i;
    int j,k;
    RECT rc;
    LPGROUPDEF lpgd;
    PITEM rgpitem[CITEMSMAX];
    POINT pt;
    int t1, t2;
    LONG style;

    if (bAutoArranging || IsIconic(hwnd))
        return;

    pGroup = (PGROUP)GetWindowLongPtr(hwnd,GWLP_PGROUP);
    if (!pGroup)
        return;

    /*
     * If the group is RO then don't rearrange the items, just update the
     * scroll bars
     */
    if (!GroupCheck(pGroup)) {
        CalcGroupScrolls(hwnd);
        return;
    }

    bAutoArranging = TRUE;
    SetRectEmpty(&rcArrangeRect);

    style = GetWindowLong(hwnd,GWL_STYLE);
    GetRealClientRect(hwnd,style&~(WS_VSCROLL|WS_HSCROLL),&rc);
    SetWindowLong(hwnd,GWL_STYLE,style);

    xSlots = (rc.right - rc.left)/cxArrange;
    if (xSlots < 1)
        xSlots = 1;

    /* sort the items by x location within a row, or by row if the
     * rows are different
     */
    k = 0;
    for (pItem = pGroup->pItems; pItem; pItem = pItem->pNext) {
        /* find nearest row
         */
        t1 = pItem->rcIcon.top + cyArrange/2;
        if (t1 >= 0)
	        t1 -= t1 % cyArrange;
        else
	        t1 += t1 % cyArrange;

        for (i = 0; i < k; i++) {
            pItemT = rgpitem[i];

            t2 = pItemT->rcIcon.top + cyArrange/2;
            if (t2 >= 0)
	            t2 -= t2 % cyArrange;
            else
	            t2 += t2 % cyArrange;

            if (t2 > t1)
                break;
            else if (t2 == t1 && pItemT->rcIcon.left > pItem->rcIcon.left)
                break;
        }

        for (j = k; j > i; j--) {
            rgpitem[j] = rgpitem[j-1];
        }

        rgpitem[i] = pItem;

        k++;
    }

    lpgd = LockGroup(hwnd);
    if (!lpgd) {
        bAutoArranging = FALSE;
        return;
    }

    bNoScrollCalc = TRUE;
    for (i = 0; i < k; i++) {
        pItem = rgpitem[i];

        /* cxOffset necessary to match (buggy???) win 3 USER
         */
        pt.x = (i%xSlots)*cxArrange + (cxArrange-cxIconSpace)/2 + cxOffset;
        pt.y = (i/xSlots)*cyArrange;

        MoveItem(pGroup,pItem,pt);
    }

    if (!IsRectEmpty(&rcArrangeRect))
        InvalidateRect(pGroup->hwnd,&rcArrangeRect,TRUE);

    UnlockGroup(hwnd);
    bNoScrollCalc = FALSE;
    CalcGroupScrolls(hwnd);

    bAutoArranging = FALSE;
}

/****************************************************************************
 *
 *  IMEStringWindow(hwnd,hstr)
 *
 *  Change activate item by the strings come from IME.
 *  When Get WM_IME_REPORT with IR_STRING,this function is called.
 *
 *                         by yutakas 1992.10.22
 *
 ****************************************************************************/
BOOL FAR PASCAL IMEStringWindow(HWND hwnd, HANDLE hStr)
{
    LPTSTR lpStr;
    LPGROUPDEF lpgd;
    LPITEMDEF lpid;
    PGROUP pGroup;
    PITEM pItem = NULL;
    PITEM pItemLast,pTItem;
    int nCnt = 0;
    int nTCnt = 0;
    BOOL ret = FALSE;

    if (!hStr)
        return ret;

    if (!(lpStr = GlobalLock(hStr)))
        return ret;

    pGroup = (PGROUP)GetWindowLongPtr(hwnd,GWLP_PGROUP);

    if (!pGroup->pItems)
        return ret;

    lpgd = LockGroup(hwnd);
    if (!lpgd)
        return ret;

#ifdef _DEBUG
{
TCHAR szDev[80];
OutputDebugString((LPTSTR)TEXT("In IME Winsdow\r\n"));
wsprintf ((LPTSTR)szDev,TEXT("IMEStringWindow: lpStr is %s \r\n"),lpStr);
OutputDebugString((LPSTR)szDev);
}
#endif


    // Search for item, skip the currently selected one.
    for ( pTItem = pGroup->pItems->pNext; pTItem; pTItem=pTItem->pNext)
    {
        lpid = ITEM(lpgd,pTItem->iItem);
        nTCnt = IMEWindowGetCnt(lpStr,(LPTSTR)PTR(lpgd,lpid->pName));
        if (nCnt < nTCnt)
        {
            nCnt = nTCnt;
            pItem = pTItem;
        }
    }

    lpid = ITEM(lpgd,pGroup->pItems->iItem);
    nTCnt = IMEWindowGetCnt(lpStr,(LPTSTR)PTR(lpgd,lpid->pName));

    if ((nCnt >= nTCnt) && pItem)
      {
        pItemLast = MakeFirstItemLast(pGroup);
        BringItemToTop(pGroup,pItem, FALSE);
        // Handle updates.
        InvalidateRect(pGroup->hwnd,&pItem->rcTitle,TRUE);
        InvalidateRect(pGroup->hwnd,&pItemLast->rcTitle,TRUE);
        ViewActiveItem(pGroup);
        ret = TRUE;
      }


    GlobalUnlock(hStr);
    UnlockGroup(hwnd);

#ifdef _DEBUG
{
TCHAR szDev[80];
wsprintf ((LPTSTR)szDev,TEXT("IMEStringWindow: ret is %s \r\n"),ret);
OutputDebugString((LPTSTR)szDev);
}
#endif
    return ret;
}

/****************************************************************************
 *
 *  IMEWindowGetCnt(LPSTR,LPSTR)
 *
 *  Compare strings from ahead and return the number of same character
 *
 *                         by yutakas 1992.10.22
 *
 *
 ****************************************************************************/
int FAR PASCAL IMEWindowGetCnt(LPTSTR lp1, LPTSTR lp2)
{
    int cnt = 0;

    while (*lp1 && *lp2)
    {
        // ToddB: This typecasting is to prevent lp1 and lp2 from being modified
        //      by CharUpper'ing one char at a time instead of the whole string
        if (CharUpper((LPTSTR)(DWORD)(BYTE)*lp1) ==
                CharUpper((LPTSTR)(DWORD)(BYTE)*lp2))
        {
            cnt++;
        }
        else
            break;

        lp1++;
        lp2++;
    }

    return (*lp1 ? 0 : cnt);
}
