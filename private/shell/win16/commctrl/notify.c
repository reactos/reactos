#include "ctlspriv.h"

LRESULT WINAPI SendNotifyEx(HWND hwndTo, HWND hwndFrom, int code, NMHDR FAR* pnmhdr, BOOL bUnicode)
{
    CONTROLINFO ci;
    
    if (!hwndTo) {
        if (IsWindow(hwndFrom))
            hwndTo = GetParent(hwndFrom);
        if (!hwndTo)
            return 0;
    }
    
    
    ci.hwndParent = hwndTo;
    ci.hwnd = hwndFrom;
    ci.bUnicode = bUnicode;
#if TODO
    ci.uiCodePage = CP_ACP;
#endif

    return CCSendNotify(&ci, code, pnmhdr);
}

LRESULT WINAPI CCSendNotify(CONTROLINFO FAR * pci, int code, LPNMHDR pnmhdr)
{
    NMHDR nmhdr;
    int id;
#ifdef UNICODE
    LPVOID pvThunk1 = NULL;
    LPVOID pvThunk2 = NULL;
    DWORD dwThunkSize = 0;
    LRESULT lRet;
    BOOL  bSet = FALSE;
#endif

    // unlikely but it can technically happen -- avoid the rips
    if (pci->hwndParent == NULL)
        return 0;

    //
    // If pci->hwnd is -1, then a WM_NOTIFY is being forwared
    // from one control to a parent.  EG:  Tooltips sent
    // a WM_NOTIFY to toolbar, and toolbar is forwarding it
    // to the real parent window.
    //

    if (pci->hwnd != (HWND) -1) {

        id = pci->hwnd ? GetDlgCtrlID(pci->hwnd) : 0;

        if (!pnmhdr)
            pnmhdr = &nmhdr;

        pnmhdr->hwndFrom = pci->hwnd;
        pnmhdr->idFrom = id;
        pnmhdr->code = code;
    } else {

        id = pnmhdr->idFrom;
        code = pnmhdr->code;
    }

#ifdef UNICODE
    /*
     * All the thunking for Notify Messages happens here
     */
    if (!pci->bUnicode) {
        switch( code ) {
        case LVN_ODFINDITEMW:
            {
                LV_FINDINFO *plvfi;

                pnmhdr->code = LVN_ODFINDITEMA;

                // Hack Alert!  This code assumes that all fields of LV_FINDINFOA and
                // LV_FINDINFOW are exactly the same except for the string pointers.
                Assert(sizeof(LV_FINDINFOA) == sizeof(LV_FINDINFOW));

                // Since WCHARs are bigger than char, we will just use the
                // wchar buffer to hold the chars, and not worry about the extra
                // room at the end.
                Assert(sizeof(WCHAR) >= sizeof(char));

                plvfi = &((PNM_FINDITEM)pnmhdr)->lvfi;
                pvThunk1 = (PVOID)plvfi->psz;
                dwThunkSize = lstrlen(pvThunk1) + 1;
                plvfi->psz = (LPWSTR)ProduceAFromW(pci->uiCodePage, plvfi->psz);
            }
            break;

        case LVN_GETDISPINFOW: {
            LV_ITEMW *pitem;

            pnmhdr->code = LVN_GETDISPINFOA;

            // Hack Alert!  This code assumes that all fields of LV_DISPINFOA and
            // LV_DISPINFOW are exactly the same except for the string pointers.

            Assert(sizeof(LV_DISPINFOA) == sizeof(LV_DISPINFOW));

            // Since WCHARs are bigger than char, we will just use the
            // wchar buffer to hold the chars, and not worry about the extra
            // room at the end.
            Assert(sizeof(WCHAR) >= sizeof(char));

            //
            // Some sleazebag code (shell32.dll) just changes the pszText
            // pointer to point to the name, so capture the original pointer
            // so we can detect this and not smash their data.
            //
            pitem = &(((LV_DISPINFOW *)pnmhdr)->item);
            if (!IsFlagPtr(pitem) && (pitem->mask & LVIF_TEXT) && !IsFlagPtr(pitem->pszText)) {
                pvThunk1 = pitem->pszText;
                dwThunkSize = pitem->cchTextMax;
            }
            break;
        }


        case LVN_ENDLABELEDITW:
            pnmhdr->code = LVN_ENDLABELEDITA;
            bSet = TRUE;
            // Fall through...

        case LVN_BEGINLABELEDITW:
            if (!bSet) {
                pnmhdr->code = LVN_BEGINLABELEDITA;
                bSet = TRUE;
            }

            // Fall through...

        case LVN_SETDISPINFOW: {
            LV_ITEMW *pitem;

            if (!bSet) {
                pnmhdr->code = LVN_SETDISPINFOA;
            }

            pitem = &(((LV_DISPINFOW *)pnmhdr)->item);

            if (!IsFlagPtr(pitem) && (pitem->mask & LVIF_TEXT) && !IsFlagPtr(pitem->pszText)) {
                pvThunk1 = pitem->pszText;
                dwThunkSize = pitem->cchTextMax;

                pvThunk2 = ProduceAFromW(pci->uiCodePage, pitem->pszText);
                pitem->pszText = (LPWSTR)pvThunk2;
            }
            break;
        }

        case TVN_SELCHANGINGW:
            pnmhdr->code = TVN_SELCHANGINGA;
            bSet = TRUE;

        case TVN_SELCHANGEDW:
            if (!bSet) {
                pnmhdr->code = TVN_SELCHANGEDA;
                bSet = TRUE;
            }

            /*
             * These msgs have a NM_TREEVIEW with both TV_ITEMs filled in
             *
             * FALL THROUGH TO TVN_DELETEITEM to thunk itemOld then go on for
             * the other structure.
             */

        case TVN_DELETEITEMW: {
            /*
             * This message has a NM_TREEVIEW in lParam with itemOld filled in
             */
            LPTV_ITEMW pitem;

            if (!bSet) {
                pnmhdr->code = TVN_DELETEITEMA;
                bSet = TRUE;
            }

            pitem = &(((LPNM_TREEVIEWW)pnmhdr)->itemOld);

            // thunk itemOld
            if ( (pitem->mask & TVIF_TEXT) && !IsFlagPtr(pitem->pszText)) {
                pvThunk2 = pitem->pszText;
                pitem->pszText = (LPWSTR)ProduceAFromW(pci->uiCodePage, pvThunk2);
            }

            // if this is deleteitem then we are done
            if (pnmhdr->code == TVN_DELETEITEMA)
                break;

            /* FALL THROUGH TO TVN_ITEMEXPANDING to thunk itemNew */
        }

        case TVN_ITEMEXPANDINGW:
            if (!bSet) {
                pnmhdr->code = TVN_ITEMEXPANDINGA;
                bSet = TRUE;
            }

        case TVN_ITEMEXPANDEDW:
            if (!bSet) {
                pnmhdr->code = TVN_ITEMEXPANDEDA;
                bSet = TRUE;
            }

        case TVN_BEGINDRAGW:
            if (!bSet) {
                pnmhdr->code = TVN_BEGINDRAGA;
                bSet = TRUE;
            }

        case TVN_BEGINRDRAGW: {
            /* these msgs have a NM_TREEVIEW with itemNew TV_ITEM filled in */
            LPTV_ITEMW pitem;

            if (!bSet) {
                pnmhdr->code = TVN_BEGINRDRAGA;
            }

            pitem = &(((LPNM_TREEVIEWW)pnmhdr)->itemNew);

            if ( (pitem->mask & TVIF_TEXT) && !IsFlagPtr(pitem->pszText)) {
                pvThunk1 = pitem->pszText;
                pitem->pszText = (LPWSTR)ProduceAFromW(pci->uiCodePage, pvThunk1);
            }

            break;
        }

        case TVN_SETDISPINFOW:
            pnmhdr->code = TVN_SETDISPINFOA;
            bSet = TRUE;

        case TVN_BEGINLABELEDITW:
            if (!bSet) {
                pnmhdr->code = TVN_BEGINLABELEDITA;
                bSet = TRUE;
            }

        case TVN_ENDLABELEDITW: {
            /*
             * All these messages have a TV_DISPINFO in lParam.
             */
            LPTV_ITEMW pitem;

            if (!bSet) {
                pnmhdr->code = TVN_ENDLABELEDITA;
            }

            pitem = &(((TV_DISPINFOW *)pnmhdr)->item);

            if ((pitem->mask & TVIF_TEXT) && !IsFlagPtr(pitem->pszText)) {
                pvThunk1 = pitem->pszText;
                dwThunkSize = pitem->cchTextMax;

                pvThunk2 = ProduceAFromW(pci->uiCodePage, pitem->pszText);
                pitem->pszText = (LPWSTR)pvThunk2;
            }
            break;
        }

        case TVN_GETDISPINFOW: {
            /*
             * All these messages have a TV_DISPINFO in lParam.
             */
            LPTV_ITEMW pitem;

            pnmhdr->code = TVN_GETDISPINFOA;

            pitem = &(((TV_DISPINFOW *)pnmhdr)->item);

            if ((pitem->mask & TVIF_TEXT) && !IsFlagPtr(pitem->pszText) && pitem->cchTextMax) {
                pvThunk1 = pitem->pszText;
                dwThunkSize = pitem->cchTextMax;
                pvThunk2 = LocalAlloc(LMEM_FIXED, pitem->cchTextMax * sizeof(char));
                pitem->pszText = pvThunk2;
                pitem->pszText[0] = TEXT('\0');
            }

            break;
        }

        case HDN_ITEMCHANGINGW:
            pnmhdr->code = HDN_ITEMCHANGINGA;
            bSet = TRUE;

        case HDN_ITEMCHANGEDW:
            if (!bSet) {
                pnmhdr->code = HDN_ITEMCHANGEDA;
                bSet = TRUE;
            }

        case HDN_ITEMCLICKW:
            if (!bSet) {
                pnmhdr->code = HDN_ITEMCLICKA;
                bSet = TRUE;
            }

        case HDN_ITEMDBLCLICKW:
            if (!bSet) {
                pnmhdr->code = HDN_ITEMDBLCLICKA;
                bSet = TRUE;
            }

        case HDN_DIVIDERDBLCLICKW:
            if (!bSet) {
                pnmhdr->code = HDN_DIVIDERDBLCLICKA;
                bSet = TRUE;
            }

        case HDN_BEGINTRACKW:
            if (!bSet) {
                pnmhdr->code = HDN_BEGINTRACKA;
                bSet = TRUE;
            }

        case HDN_ENDTRACKW:
            if (!bSet) {
                pnmhdr->code = HDN_ENDTRACKA;
                bSet = TRUE;
            }

        case HDN_TRACKW: {
            HD_ITEMW *pitem;

            if (!bSet) {
                pnmhdr->code = HDN_TRACKA;
            }

            pitem = ((HD_NOTIFY *)pnmhdr)->pitem;

            if ( !IsFlagPtr(pitem) && (pitem->mask & HDI_TEXT) && !IsFlagPtr(pitem->pszText)) {
                pvThunk1 = pitem->pszText;
                dwThunkSize = pitem->cchTextMax;
                pitem->pszText = (LPWSTR)ProduceAFromW(pci->uiCodePage, pvThunk1);
            }

            break;
        }
            
        case CBEN_GETDISPINFOW: {
            PNMCOMBOBOXEXW pnmcbe = (PNMCOMBOBOXEXW)pnmhdr;
            
            pnmhdr->code = CBEN_GETDISPINFOA;

            if (pnmcbe->ceItem.mask  & CBEIF_TEXT 
                && !IsFlagPtr(pnmcbe->ceItem.pszText) && pnmcbe->ceItem.cchTextMax) {
                pvThunk1 = pnmcbe->ceItem.pszText;
                dwThunkSize = pnmcbe->ceItem.cchTextMax;
                pvThunk2 = LocalAlloc(LMEM_FIXED, pnmcbe->ceItem.cchTextMax * sizeof(char));
                pnmcbe->ceItem.pszText = pvThunk2;
                pnmcbe->ceItem.pszText[0] = TEXT('\0');
            }
            
            break;
        }

        case HDN_GETDISPINFOW: {
            LPNMHDDISPINFOW pHDDispInfoW;

            pnmhdr->code = HDN_GETDISPINFOA;

            pHDDispInfoW = (LPNMHDDISPINFOW) pnmhdr;

            pvThunk1 = pHDDispInfoW->pszText;
            dwThunkSize = pHDDispInfoW->cchTextMax;
            pHDDispInfoW->pszText = GlobalAlloc (GPTR, pHDDispInfoW->cchTextMax * sizeof(char));

            if (!pHDDispInfoW->pszText) {
                pHDDispInfoW->pszText = (LPWSTR) pvThunk1;
                break;
            }

            WideCharToMultiByte(pci->uiCodePage, 0, (LPWSTR)pvThunk1, -1,
                               (LPSTR)pHDDispInfoW->pszText, pHDDispInfoW->cchTextMax,
                               NULL, NULL);
            break;
        }


        case TBN_GETBUTTONINFOW:
            {
            LPTBNOTIFYW pTBNW;

            pnmhdr->code = TBN_GETBUTTONINFOA;

            pTBNW = (LPTBNOTIFYW)pnmhdr;

            pvThunk1 = pTBNW->pszText;
            dwThunkSize = pTBNW->cchText;
            pvThunk2 = GlobalAlloc (GPTR, pTBNW->cchText * sizeof(char));

            if (!pvThunk2) {
                break;
            }
            pTBNW->pszText = pvThunk2;

            WideCharToMultiByte(pci->uiCodePage, 0, (LPWSTR)pvThunk1, -1,
                               (LPSTR)pTBNW->pszText, pTBNW->cchText,
                               NULL, NULL);

            }
            break;

        case TTN_NEEDTEXTW:
            {
            LPTOOLTIPTEXTA lpTTTA;
            LPTOOLTIPTEXTW lpTTTW = (LPTOOLTIPTEXTW) pnmhdr;

            lpTTTA = GlobalAlloc(GPTR, sizeof(TOOLTIPTEXTA));

            if (!lpTTTA)
               return 0;

            lpTTTA->hdr.hwndFrom = lpTTTW->hdr.hwndFrom;
            lpTTTA->hdr.idFrom   = lpTTTW->hdr.idFrom;
            lpTTTA->hdr.code     = TTN_NEEDTEXTA;

            lpTTTA->lpszText = lpTTTA->szText;
            lpTTTA->hinst    = lpTTTW->hinst;
            lpTTTA->uFlags   = lpTTTW->uFlags;

            pvThunk1 = pnmhdr;
            pnmhdr = (NMHDR FAR *)lpTTTA;
            }
            break;

        case DTN_USERSTRINGW:
            {
            LPNMDATETIMESTRINGW lpDateTimeString = (LPNMDATETIMESTRINGW) pnmhdr;

            pnmhdr->code = DTN_USERSTRINGA;

            pvThunk1 = ProduceAFromW(pci->uiCodePage, lpDateTimeString->pszUserString);
            lpDateTimeString->pszUserString = (LPWSTR) pvThunk1;
            }
            break;

        case DTN_WMKEYDOWNW:
            {
            LPNMDATETIMEWMKEYDOWNW lpDateTimeWMKeyDown =
                                               (LPNMDATETIMEWMKEYDOWNW) pnmhdr;

            pnmhdr->code = DTN_WMKEYDOWNA;

            pvThunk1 = ProduceAFromW(pci->uiCodePage, lpDateTimeWMKeyDown->pszFormat);
            lpDateTimeWMKeyDown->pszFormat = (LPWSTR) pvThunk1;
            }
            break;

        case DTN_FORMATQUERYW:
            {
            LPNMDATETIMEFORMATQUERYW lpDateTimeFormatQuery =
                                               (LPNMDATETIMEFORMATQUERYW) pnmhdr;

            pnmhdr->code = DTN_FORMATQUERYA;

            pvThunk1 = ProduceAFromW(pci->uiCodePage, lpDateTimeFormatQuery->pszFormat);
            lpDateTimeFormatQuery->pszFormat = (LPWSTR) pvThunk1;
            }
            break;

        case DTN_FORMATW:
            {
            LPNMDATETIMEFORMATW lpDateTimeFormat =
                                               (LPNMDATETIMEFORMATW) pnmhdr;

            pnmhdr->code = DTN_FORMATA;

            pvThunk1 = ProduceAFromW(pci->uiCodePage, lpDateTimeFormat->pszFormat);
            lpDateTimeFormat->pszFormat = (LPWSTR) pvThunk1;
            }
            break;

        default:
            /* No thunking needed */
            break;
        }

        lRet = SendMessage(pci->hwndParent, WM_NOTIFY, (WPARAM)id, (LPARAM)pnmhdr);

        /*
         * All the thunking for Notify Messages happens here
         */
        switch(pnmhdr->code) {
        case LVN_ODFINDITEMA:
            {
                LV_FINDINFO *plvfi;

                plvfi = &((PNM_FINDITEM)pnmhdr)->lvfi;
                FreeProducedString( (LPWSTR)plvfi->psz);
                plvfi->psz = pvThunk1;
            }
            break;

        case LVN_GETDISPINFOA: {
            LPWSTR pszW;
            LV_ITEMW *pitem;

            pitem = &(((LV_DISPINFOW *)pnmhdr)->item);

            if (!IsFlagPtr(pitem) && (pitem->mask & LVIF_TEXT) && !IsFlagPtr(pitem->pszText)) {
                if (pvThunk1 == pitem->pszText) {
                    pszW = ProduceWFromA(pci->uiCodePage, (LPSTR)(pitem->pszText));

                    if (pszW)
                        lstrcpy( pitem->pszText, pszW );

                    FreeProducedString(pszW);
                } else {
                    //
                    // The pointer has been changed out from underneath us, copy
                    // unicode back into the original buffer.
                    //
                    ConvertAToWN(pci->uiCodePage, pvThunk1, dwThunkSize, (LPSTR)pitem->pszText, -1);
                    pitem->pszText = pvThunk1;
                }
            }
            break;
        }

        case LVN_ENDLABELEDITA:
        case LVN_BEGINLABELEDITA:
        case LVN_SETDISPINFOA: {
            LV_ITEMW *pitem;

            if (!IsFlagPtr(pvThunk1)) {
                pitem = &(((LV_DISPINFOW *)pnmhdr)->item);

                if (!IsFlagPtr((LPSTR)pitem->pszText)) {
                    ConvertAToWN(pci->uiCodePage, pvThunk1, dwThunkSize, (LPSTR)(pitem->pszText), -1);
                }

                FreeProducedString(pvThunk2);

                pitem->pszText = pvThunk1;
            }
            break;
        }

        case TVN_SELCHANGINGA:
        case TVN_SELCHANGEDA:
        case TVN_DELETEITEMA: {
            LPTV_ITEMW pitem;

            if ( !IsFlagPtr(pvThunk2) ) {
                pitem = &(((LPNM_TREEVIEWW)pnmhdr)->itemOld);

                FreeProducedString(pitem->pszText);
                pitem->pszText = pvThunk2;
            }

            // if this is delitem, then we are done
            if (code == TVN_DELETEITEM)
                break;

            /* FALL THROUGH TO TVN_ITEMEXPANDING to unthunk itemNew */
        }

        case TVN_ITEMEXPANDINGA:
        case TVN_ITEMEXPANDEDA:
        case TVN_BEGINDRAGA:
        case TVN_BEGINRDRAGA: {
            /* these msgs have a NM_TREEVIEW with itemNew TV_ITEM filled in */
            LPTV_ITEMW pitem;

            if (!IsFlagPtr(pvThunk1)) {
                pitem = &(((LPNM_TREEVIEWW)pnmhdr)->itemNew);

                FreeProducedString(pitem->pszText);
                pitem->pszText = pvThunk1;
            }

            break;
        }

        case TVN_SETDISPINFOA:
        case TVN_BEGINLABELEDITA:
        case TVN_ENDLABELEDITA: {
            /* All these messages have a TV_DISPINFO in lParam */
            LPTV_ITEMW pitem;

            if (!IsFlagPtr(pvThunk1)) {
                pitem = &(((TV_DISPINFOW *)pnmhdr)->item);

                if (!IsFlagPtr((LPSTR)pitem->pszText)) {
                    ConvertAToWN(pci->uiCodePage, pvThunk1, dwThunkSize, (LPSTR)(pitem->pszText), -1);
                }

                FreeProducedString(pvThunk2);
                pitem->pszText = pvThunk1;
            }

            break;
        }

        case TVN_GETDISPINFOA: {
            /*
             * This message has a TV_DISPINFO in lParam that wass filled in
             * during the callback and needs to be unthunked.
             */
            LPTV_ITEMW pitem;

            pitem = &(((TV_DISPINFOW *)pnmhdr)->item);

            if (!IsFlagPtr(pvThunk1) && (pitem->mask & TVIF_TEXT) && !IsFlagPtr(pitem->pszText)) {
                ConvertAToWN(pci->uiCodePage, pvThunk1, dwThunkSize, (LPSTR)pitem->pszText, -1);
                pitem->pszText = pvThunk1;
                LocalFree(pvThunk2);
            }

            break;
        }

        case HDN_ITEMCHANGINGA:
        case HDN_ITEMCHANGEDA:
        case HDN_ITEMCLICKA:
        case HDN_ITEMDBLCLICKA:
        case HDN_DIVIDERDBLCLICKA:
        case HDN_BEGINTRACKA:
        case HDN_ENDTRACKA:
        case HDN_TRACKA: {
            HD_ITEMW *pitem;

            pitem = ((HD_NOTIFY *)pnmhdr)->pitem;

            if ( !IsFlagPtr(pitem) && (pitem->mask & HDI_TEXT) && !IsFlagPtr(pvThunk1)) {
                ConvertAToWN(pci->uiCodePage, pvThunk1, dwThunkSize, (LPSTR)(pitem->pszText), -1);

                FreeProducedString(pitem->pszText);
                pitem->pszText = pvThunk1;
            }

            break;
        }

        case CBEN_GETDISPINFOA:
            {
            PNMCOMBOBOXEXW pnmcbeW;

            pnmcbeW = (PNMCOMBOBOXEXW)pnmhdr;
            ConvertAToWN(pci->uiCodePage, pvThunk1, dwThunkSize, (LPSTR)(pnmcbeW->ceItem.pszText), -1);

            GlobalFree(pnmcbeW->ceItem.pszText);
            pnmcbeW->ceItem.pszText = pvThunk1;

            }
            break;

            
        case HDN_GETDISPINFOA:
            {
            LPNMHDDISPINFOW pHDDispInfoW;

            pHDDispInfoW = (LPNMHDDISPINFOW)pnmhdr;
            ConvertAToWN(pci->uiCodePage, pvThunk1, dwThunkSize, (LPSTR)(pHDDispInfoW->pszText), -1);

            GlobalFree(pHDDispInfoW->pszText);
            pHDDispInfoW->pszText = pvThunk1;

            }
            break;

        case TBN_GETBUTTONINFOA:
            {
            LPTBNOTIFYW pTBNW;

            pTBNW = (LPTBNOTIFYW)pnmhdr;
            ConvertAToWN(pci->uiCodePage, pvThunk1, dwThunkSize, (LPSTR)(pTBNW->pszText), -1);

            pTBNW->pszText = pvThunk1;
            GlobalFree(pvThunk2);

            }
            break;


        case TTN_NEEDTEXTA:
            {
            LPTOOLTIPTEXTA lpTTTA = (LPTOOLTIPTEXTA) pnmhdr;
            LPTOOLTIPTEXTW lpTTTW = (LPTOOLTIPTEXTW) pvThunk1;

            ThunkToolTipTextAtoW (lpTTTA, lpTTTW, pci->uiCodePage);
            GlobalFree(lpTTTA);
            }
            break;

        case DTN_USERSTRINGA:
        case DTN_WMKEYDOWNA:
        case DTN_FORMATQUERYA:
            {
            FreeProducedString (pvThunk1);
            }
            break;

        case DTN_FORMATA:
            {
            LPNMDATETIMEFORMATA lpDateTimeFormat = (LPNMDATETIMEFORMATA) pnmhdr;

            FreeProducedString (pvThunk1);

            //
            // pszDisplay and szDisplay are special cases.
            //

            if (lpDateTimeFormat->pszDisplay && *lpDateTimeFormat->pszDisplay) {

                //
                // if pszDisplay still points at szDisplay then thunk
                // in place.  Otherwise allocate memory and copy the
                // display string.  This buffer will be freeded in monthcal.c
                //

                if (lpDateTimeFormat->pszDisplay == lpDateTimeFormat->szDisplay) {
                    CHAR szDisplay[64];

                    lstrcpynA (szDisplay, lpDateTimeFormat->szDisplay, 64);

                    ConvertAToWN (pci->uiCodePage, (LPWSTR)lpDateTimeFormat->szDisplay, 64,
                                  szDisplay, -1);
                } else {
                    lpDateTimeFormat->pszDisplay =
                             (LPSTR) ProduceWFromA (pci->uiCodePage, lpDateTimeFormat->pszDisplay);
                }

            }

            }
            break;

        default:
            /* No thunking needed */
            break;
        }

        return lRet;
    } else
#endif
        return(SendMessage(pci->hwndParent, WM_NOTIFY, (WPARAM)id, (LPARAM)pnmhdr));
}

LRESULT WINAPI SendNotify(HWND hwndTo, HWND hwndFrom, int code, NMHDR FAR* pnmhdr)
{
    CONTROLINFO ci;
    ci.hwndParent = hwndTo;
    ci.hwnd = hwndFrom;
    ci.bUnicode = FALSE;
#if TODO
    ci.uiCodePage = CP_ACP;
#endif
    
    //
    // SendNotify is obsolete.  New code should call CCSendNotify
    // instead.  However, if something does call SendNotify,
    // it will call SendNotifyEx with FALSE as the Unicode parameter,
    // because it probably is ANSI code.
    //

    return CCSendNotify(&ci, code, pnmhdr);
}


DWORD NEAR PASCAL CICustomDrawNotify(LPCONTROLINFO lpci, DWORD dwStage, LPNMCUSTOMDRAW lpnmcd)
{
    DWORD dwRet = CDRF_DODEFAULT;
    
    
    // bail if... 
    
    
    // this is an item notification, but an item notification wasn't asked for
    if ((dwStage & CDDS_ITEM) && !(lpci->dwCustom & CDRF_NOTIFYITEMDRAW)) {
        return dwRet;
    }
    
    lpnmcd->dwDrawStage = dwStage;
    dwRet = CCSendNotify(lpci, NM_CUSTOMDRAW, &lpnmcd->hdr);

    // validate the flags
    if (dwRet & ~CDRF_VALIDFLAGS)
        return CDRF_DODEFAULT;
    
    return dwRet;
}
