#include "ctlspriv.h"

#ifdef NEED_WOWGETNOTIFYSIZE_HELPER

#include <shsemip.h>                // SEN_* notifications
#include <commdlg.h>                // CDN_* notifications

// Miscellaneous hackery needed in order to include shlobjp.h
#define CONTROLINFO     OAIDL_CONTROLINFO
#define LPCONTROLINFO   LPOAIDL_CONTROLINFO
#include <shlobj.h>
#include <shlobjp.h>                // NMVIEWFOLDER structure
#undef CONTROLINFO
#undef LPCONTROLINFO

//
//  Helper function for WOW on NT.
//
//  WOW needs to know the size of the notify structure associated with a
//  notification.  If a 32-bit window has been subclassed by a 16-bit app,
//  WOW needs to copy the notify structure into 16-bit space, and then when
//  the 16-bit guy does a CallWindowProc(), they have to copy it back into
//  32-bit space.  Without the size information, you fault on the
//  32-bit side because the notify structure is incomplete.
//
//  Some notifications have multiple structures associated with them, in
//  which case you should return the largest possible valid structure.
//
STDAPI_(UINT) WOWGetNotifySize(UINT code)
{
    switch (code) {

    // Generic comctl32 notifications
    case NM_OUTOFMEMORY:        return sizeof(NMHDR);   // not used

    case NM_CLICK:              return max(max(
                        sizeof(NMHDR),       // tab, treeview
                        sizeof(NMCLICK)),    // toolbar, statusbar
                        sizeof(NMITEMACTIVATE)); // listview

    case NM_DBLCLK:             return max(max(
                        sizeof(NMHDR),       // tab, treeview
                        sizeof(NMCLICK)),    // toolbar, statusbar
                        sizeof(NMITEMACTIVATE)); // listview


    case NM_RETURN:             return sizeof(NMHDR);

    case NM_RCLICK:             return max(max(
                        sizeof(NMHDR),       // header, listview report mode, treeview
                        sizeof(NMCLICK)),    // toolbar, statusbar
                        sizeof(NMITEMACTIVATE)); // listview icon mode

    case NM_RDBLCLK:            return max(max(
                        sizeof(NMHDR),       // treeview
                        sizeof(NMCLICK)),    // toolbar, statusbar
                        sizeof(NMITEMACTIVATE)); // listview

    case NM_SETFOCUS:           return sizeof(NMHDR);
    case NM_KILLFOCUS:          return sizeof(NMHDR);
    case NM_STARTWAIT:          return sizeof(NMHDR);      // not used
    case NM_ENDWAIT:            return sizeof(NMHDR);      // not used
    case NM_BTNCLK:             return sizeof(NMHDR);      // not used
    case NM_CUSTOMDRAW:         return sizeof(NMCUSTOMDRAW);
    case NM_HOVER:              return sizeof(NMHDR);
    case NM_NCHITTEST:          return sizeof(NMMOUSE);
    case NM_KEYDOWN:            return sizeof(NMKEY);
    case NM_RELEASEDCAPTURE:    return sizeof(NMHDR);
    case NM_SETCURSOR:          return sizeof(NMMOUSE);
    case NM_CHAR:               return sizeof(NMCHAR);
    case NM_TOOLTIPSCREATED:    return sizeof(NMTOOLTIPSCREATED);
    case NM_LDOWN:              return sizeof(NMCLICK);
    case NM_RDOWN:              return sizeof(NMCLICK);     // not used

    // Listview notifications
    case LVN_ITEMCHANGING:      return sizeof(NMLISTVIEW);
    case LVN_ITEMCHANGED:       return sizeof(NMLISTVIEW);
    case LVN_INSERTITEM:        return sizeof(NMLISTVIEW);
    case LVN_DELETEITEM:        return sizeof(NMLISTVIEW);
    case LVN_DELETEALLITEMS:    return sizeof(NMLISTVIEW);
    case LVN_BEGINLABELEDITA:   return sizeof(NMLVDISPINFOA);
    case LVN_BEGINLABELEDITW:   return sizeof(NMLVDISPINFOW);
    case LVN_ENDLABELEDITA:     return sizeof(NMLVDISPINFOA);
    case LVN_ENDLABELEDITW:     return sizeof(NMLVDISPINFOW);
    case LVN_COLUMNCLICK:       return sizeof(NMLISTVIEW);
    case LVN_BEGINDRAG:         return sizeof(NMITEMACTIVATE);
    case LVN_BEGINRDRAG:        return sizeof(NMITEMACTIVATE); // not used
    case LVN_ENDDRAG:           return sizeof(NMITEMACTIVATE); // not used
    case LVN_ENDRDRAG:          return sizeof(NMITEMACTIVATE); // not used
    case LVN_ODCACHEHINT:       return sizeof(NMLVCACHEHINT);
    case LVN_ODFINDITEMA:       return sizeof(NMLVFINDITEMA);
    case LVN_ODFINDITEMW:       return sizeof(NMLVFINDITEMW);
    case LVN_ITEMACTIVATE:      return sizeof(NMITEMACTIVATE);
    case LVN_ODSTATECHANGED:    return sizeof(NMLVODSTATECHANGE);
//  case LVN_PEN:               // Pen Windows slackers
    case LVN_HOTTRACK:          return sizeof(NMLISTVIEW);
    case LVN_GETDISPINFOA:      return sizeof(NMLVDISPINFOA);
    case LVN_GETDISPINFOW:      return sizeof(NMLVDISPINFOW);
    case LVN_SETDISPINFOA:      return sizeof(NMLVDISPINFOA);
    case LVN_SETDISPINFOW:      return sizeof(NMLVDISPINFOW);
    case LVN_KEYDOWN:           return sizeof(NMLVKEYDOWN);
    case LVN_MARQUEEBEGIN:      return sizeof(NMITEMACTIVATE);
    case LVN_GETINFOTIPA:       return sizeof(NMLVGETINFOTIPA);
    case LVN_GETINFOTIPW:       return sizeof(NMLVGETINFOTIPW);
    case LVN_GETEMPTYTEXTA:     return sizeof(NMLVDISPINFOA);
    case LVN_GETEMPTYTEXTW:     return sizeof(NMLVDISPINFOW);
    case LVN_INCREMENTALSEARCHA:return sizeof(NMLVFINDITEMA);
    case LVN_INCREMENTALSEARCHW:return sizeof(NMLVFINDITEMW);

    // Property sheet notifications
    case PSN_SETACTIVE:         return sizeof(PSHNOTIFY);
    case PSN_KILLACTIVE:        return sizeof(PSHNOTIFY);
    case PSN_APPLY:             return sizeof(PSHNOTIFY);
    case PSN_RESET:             return sizeof(PSHNOTIFY);
    case PSN_HASHELP:           return sizeof(PSHNOTIFY);   // not used
    case PSN_HELP:              return sizeof(PSHNOTIFY);
    case PSN_WIZBACK:           return sizeof(PSHNOTIFY);
    case PSN_WIZNEXT:           return sizeof(PSHNOTIFY);
    case PSN_WIZFINISH:         return sizeof(PSHNOTIFY);
    case PSN_QUERYCANCEL:       return sizeof(PSHNOTIFY);
    case PSN_GETOBJECT:         return sizeof(NMOBJECTNOTIFY);
    case PSN_LASTCHANCEAPPLY:   return sizeof(PSHNOTIFY);
    case PSN_TRANSLATEACCELERATOR:
                                return sizeof(PSHNOTIFY);
    case PSN_QUERYINITIALFOCUS: return sizeof(PSHNOTIFY);

    // Header notifications
    case HDN_ITEMCHANGINGA:     return sizeof(NMHEADERA);
    case HDN_ITEMCHANGINGW:     return sizeof(NMHEADERW);
    case HDN_ITEMCHANGEDA:      return sizeof(NMHEADERA);
    case HDN_ITEMCHANGEDW:      return sizeof(NMHEADERW);
    case HDN_ITEMCLICKA:        return sizeof(NMHEADERA);
    case HDN_ITEMCLICKW:        return sizeof(NMHEADERW);
    case HDN_ITEMDBLCLICKA:     return sizeof(NMHEADERA);
    case HDN_ITEMDBLCLICKW:     return sizeof(NMHEADERW);
    case HDN_DIVIDERDBLCLICKA:  return sizeof(NMHEADERA);
    case HDN_DIVIDERDBLCLICKW:  return sizeof(NMHEADERW);
    case HDN_BEGINTRACKA:       return sizeof(NMHEADERA);
    case HDN_BEGINTRACKW:       return sizeof(NMHEADERW);
    case HDN_ENDTRACKA:         return sizeof(NMHEADERA);
    case HDN_ENDTRACKW:         return sizeof(NMHEADERW);
    case HDN_TRACKA:            return sizeof(NMHEADERA);
    case HDN_TRACKW:            return sizeof(NMHEADERW);
    case HDN_GETDISPINFOA:      return sizeof(NMHDDISPINFOA);
    case HDN_GETDISPINFOW:      return sizeof(NMHDDISPINFOW);
    case HDN_BEGINDRAG:         return sizeof(NMHEADER); // No strings
    case HDN_ENDDRAG:           return sizeof(NMHEADER); // No strings
    case HDN_FILTERCHANGE:      return sizeof(NMHEADER); // No strings
    case HDN_FILTERBTNCLICK:    return sizeof(NMHDFILTERBTNCLICK);

    // Treeview notifications
    case TVN_SELCHANGINGA:      return sizeof(NMTREEVIEWA);
    case TVN_SELCHANGINGW:      return sizeof(NMTREEVIEWW);
    case TVN_SELCHANGEDA:       return sizeof(NMTREEVIEWA);
    case TVN_SELCHANGEDW:       return sizeof(NMTREEVIEWW);
    case TVN_GETDISPINFOA:      return sizeof(NMTVDISPINFOA);
    case TVN_GETDISPINFOW:      return sizeof(NMTVDISPINFOW);
    case TVN_SETDISPINFOA:      return sizeof(NMTVDISPINFOA);
    case TVN_SETDISPINFOW:      return sizeof(NMTVDISPINFOW);
    case TVN_ITEMEXPANDINGA:    return sizeof(NMTREEVIEWA);
    case TVN_ITEMEXPANDINGW:    return sizeof(NMTREEVIEWW);
    case TVN_ITEMEXPANDEDA:     return sizeof(NMTREEVIEWA);
    case TVN_ITEMEXPANDEDW:     return sizeof(NMTREEVIEWW);
    case TVN_BEGINDRAGA:        return sizeof(NMTREEVIEWA);
    case TVN_BEGINDRAGW:        return sizeof(NMTREEVIEWW);
    case TVN_BEGINRDRAGA:       return sizeof(NMTREEVIEWA);
    case TVN_BEGINRDRAGW:       return sizeof(NMTREEVIEWW);
    case TVN_DELETEITEMA:       return sizeof(NMTREEVIEWA);
    case TVN_DELETEITEMW:       return sizeof(NMTREEVIEWW);
    case TVN_BEGINLABELEDITA:   return sizeof(NMTVDISPINFOA);
    case TVN_BEGINLABELEDITW:   return sizeof(NMTVDISPINFOW);
    case TVN_ENDLABELEDITA:     return sizeof(NMTVDISPINFOA);
    case TVN_ENDLABELEDITW:     return sizeof(NMTVDISPINFOW);
    case TVN_KEYDOWN:           return sizeof(NMTVKEYDOWN);
    case TVN_GETINFOTIPA:       return sizeof(NMTVGETINFOTIPA);
    case TVN_GETINFOTIPW:       return sizeof(NMTVGETINFOTIPW);
    case TVN_SINGLEEXPAND:      return sizeof(NMTREEVIEW); // No strings

    // Rundll32 notifications
    case RDN_TASKINFO:          return sizeof(RUNDLL_NOTIFY);

    // Tooltip notifications
    case TTN_GETDISPINFOA:      return sizeof(NMTTDISPINFOA);
    case TTN_GETDISPINFOW:      return sizeof(NMTTDISPINFOW);
    case TTN_SHOW:              return sizeof(NMTTSHOWINFO);
    case TTN_POP:               return sizeof(NMHDR);

    // Tab control notifications

    // WE ARE SUCH HORRIBLE SLACKERS!
    //
    //  Even though commctrl.h says that the shell reserved range is from
    //  -580 to -589, shsemip.h defines SEN_FIRST as -550, which conflicts
    //  with TCN_KEYDOWN, so now TCN_KEYDOWN and SEN_DDEEXECUTE have the
    //  same value.

#ifdef WINNT
    case TCN_KEYDOWN:           return max(sizeof(NMTCKEYDOWN),
                                           sizeof(NMVIEWFOLDERW));
#else
    case TCN_KEYDOWN:           return max(sizeof(NMTCKEYDOWN),
                                           sizeof(NMVIEWFOLDERA));
#endif
    case TCN_SELCHANGE:         return sizeof(NMHDR);
    case TCN_SELCHANGING:       return sizeof(NMHDR);
    case TCN_GETOBJECT:         return sizeof(NMOBJECTNOTIFY);
    case TCN_FOCUSCHANGE:       return sizeof(NMHDR);

    // Shell32 notifications
#if 0 // see comment at TCN_KEYDOWN
    case SEN_DDEEXECUTE:
#ifdef WINNT
                                return sizeof(NMVIEWFOLDERW);
#else
                                return sizeof(NMVIEWFOLDERA);
#endif
#endif

    // Comdlg32 notifications
    case CDN_INITDONE:          return max(sizeof(OFNOTIFYA),
                                           sizeof(OFNOTIFYW));
    case CDN_SELCHANGE:         return max(sizeof(OFNOTIFYA),
                                           sizeof(OFNOTIFYW));
    case CDN_FOLDERCHANGE:      return max(sizeof(OFNOTIFYA),
                                           sizeof(OFNOTIFYW));
    case CDN_SHAREVIOLATION:    return max(sizeof(OFNOTIFYA),
                                           sizeof(OFNOTIFYW));
    case CDN_HELP:              return max(sizeof(OFNOTIFYA),
                                           sizeof(OFNOTIFYW));
    case CDN_FILEOK:            return max(sizeof(OFNOTIFYA),
                                           sizeof(OFNOTIFYW));
    case CDN_TYPECHANGE:        return max(sizeof(OFNOTIFYA),
                                           sizeof(OFNOTIFYW));
    case CDN_INCLUDEITEM:       return max(sizeof(OFNOTIFYEXA),
                                           sizeof(OFNOTIFYEXW));

    // Toolbar notifications
    case TBN_GETBUTTONINFOA:    return sizeof(NMTOOLBARA);
    case TBN_GETBUTTONINFOW:    return sizeof(NMTOOLBARW);
    case TBN_BEGINDRAG:         return sizeof(NMTOOLBAR); // No strings
    case TBN_ENDDRAG:           return sizeof(NMTOOLBAR); // No strings
    case TBN_BEGINADJUST:       return sizeof(NMHDR);
    case TBN_ENDADJUST:         return sizeof(NMHDR);
    case TBN_RESET:             return sizeof(NMTBCUSTOMIZEDLG);
    case TBN_QUERYINSERT:       return sizeof(NMTOOLBAR); // No strings
    case TBN_QUERYDELETE:       return sizeof(NMTOOLBAR); // No strings
    case TBN_TOOLBARCHANGE:     return sizeof(NMHDR);
    case TBN_CUSTHELP:          return sizeof(NMHDR);
    case TBN_DROPDOWN:          return sizeof(NMTOOLBAR); // No strings
    case TBN_CLOSEUP:           return sizeof(NMHDR);     // not used
    case TBN_GETOBJECT:         return sizeof(NMOBJECTNOTIFY);
    case TBN_HOTITEMCHANGE:     return sizeof(NMTBHOTITEM);
    case TBN_DRAGOUT:           return sizeof(NMTOOLBAR); // No strings
    case TBN_DELETINGBUTTON:    return sizeof(NMTOOLBAR); // No strings
    case TBN_GETDISPINFOA:      return sizeof(NMTBDISPINFOA);
    case TBN_GETDISPINFOW:      return sizeof(NMTBDISPINFOW);
    case TBN_GETINFOTIPA:       return sizeof(NMTBGETINFOTIPA);
    case TBN_GETINFOTIPW:       return sizeof(NMTBGETINFOTIPW);
    case TBN_RESTORE:           return sizeof(NMTBRESTORE);

    // WE ARE SUCH HORRIBLE SLACKERS!
    //
    //  The TBN_FIRST/TBN_LAST range reserves 20 notifications for toolbar,
    //  and we overflowed that limit, so now UDN_DELTAPOS and
    //  TBN_SAVE have the same value.

    case TBN_SAVE:              return max(sizeof(NMTBSAVE),
                                           sizeof(NMUPDOWN));

    case TBN_INITCUSTOMIZE:     return sizeof(NMTBCUSTOMIZEDLG);
    case TBN_WRAPHOTITEM:       return sizeof(NMTBWRAPHOTITEM);
    case TBN_DUPACCELERATOR:    return sizeof(NMTBDUPACCELERATOR);
    case TBN_WRAPACCELERATOR:   return sizeof(NMTBWRAPACCELERATOR);
    case TBN_DRAGOVER:          return sizeof(NMTBHOTITEM);
    case TBN_MAPACCELERATOR:    return sizeof(NMCHAR);

    // Up-down control
#if 0 // see comment at TBN_SAVE
    case UDN_DELTAPOS:          return sizeof(NMUPDOWN);
#endif

    // Monthcal control
    case MCN_SELCHANGE:         return sizeof(NMSELCHANGE);
    case MCN_GETDAYSTATE:       return sizeof(NMDAYSTATE);
    case MCN_SELECT:            return sizeof(NMSELECT);

    // Date/time picker control
    case DTN_DATETIMECHANGE:    return sizeof(NMDATETIMECHANGE);
    case DTN_USERSTRINGA:       return sizeof(NMDATETIMESTRINGA);
    case DTN_USERSTRINGW:       return sizeof(NMDATETIMESTRINGW);
    case DTN_WMKEYDOWNA:        return sizeof(NMDATETIMEWMKEYDOWNA);
    case DTN_WMKEYDOWNW:        return sizeof(NMDATETIMEWMKEYDOWNW);
    case DTN_FORMATA:           return sizeof(NMDATETIMEFORMATA);
    case DTN_FORMATW:           return sizeof(NMDATETIMEFORMATW);
    case DTN_FORMATQUERYA:      return sizeof(NMDATETIMEFORMATQUERYA);
    case DTN_FORMATQUERYW:      return sizeof(NMDATETIMEFORMATQUERYW);
    case DTN_DROPDOWN:          return sizeof(NMHDR);
    case DTN_CLOSEUP:           return sizeof(NMHDR);

    // Comboex notifications
    case CBEN_GETDISPINFOA:     return sizeof(NMCOMBOBOXEXA);
    case CBEN_GETDISPINFOW:     return sizeof(NMCOMBOBOXEXW);
    case CBEN_INSERTITEM:       return sizeof(NMCOMBOBOXEX); // Random character set
    case CBEN_DELETEITEM:       return sizeof(NMCOMBOBOXEX); // No strings
    case CBEN_ITEMCHANGED:      return sizeof(NMCOMBOBOXEX); // Not used
    case CBEN_BEGINEDIT:        return sizeof(NMHDR);
    case CBEN_ENDEDITA:         return sizeof(NMCBEENDEDITA);
    case CBEN_ENDEDITW:         return sizeof(NMCBEENDEDITW);
    case CBEN_DRAGBEGINA:       return sizeof(NMCBEDRAGBEGINA);
    case CBEN_DRAGBEGINW:       return sizeof(NMCBEDRAGBEGINW);

    // Rebar notifications
    case RBN_HEIGHTCHANGE:      return sizeof(NMHDR);
    case RBN_GETOBJECT:         return sizeof(NMOBJECTNOTIFY);
    case RBN_LAYOUTCHANGED:     return sizeof(NMHDR);
    case RBN_AUTOSIZE:          return sizeof(NMRBAUTOSIZE);
    case RBN_BEGINDRAG:         return sizeof(NMREBAR);
    case RBN_DELETINGBAND:      return sizeof(NMREBAR);
    case RBN_DELETEDBAND:       return sizeof(NMREBAR);
    case RBN_CHILDSIZE:         return sizeof(NMREBARCHILDSIZE);

    // IP address control notification
    case IPN_FIELDCHANGED:      return sizeof(NMIPADDRESS);

    // Status bar notifications
    case SBN_SIMPLEMODECHANGE:  return sizeof(NMHDR);

    // Pager control notifications
    case PGN_SCROLL:            return sizeof(NMPGSCROLL);
    case PGN_CALCSIZE:          return sizeof(NMPGCALCSIZE);

    default:
        break;
    }

    //
    //  Categories of notifications we explicitly know nothing about.
    //

    if (code >= WMN_LAST && code <= WMN_FIRST) { // Internet Mail and News
        return 0;
    }

    if ((int)code >= 0) { // Application-specific notifications
        return 0;
    }

    //
    //  IF THIS ASSERT FIRES, YOU MUST FIX IT OR YOU WILL BREAK WOW!
    //
    AssertMsg(0, TEXT("Notification code %d must be added to WOWGetNotifySize"));
    return 0;
}

#endif // NEED_WOWGETNOTIFYSIZE_HELPER

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
    ci.bUnicode = BOOLIFY(bUnicode);
    ci.uiCodePage = CP_ACP;

    return CCSendNotify(&ci, code, pnmhdr);
}


#ifdef UNICODE

void StringBufferAtoW(UINT uiCodePage, LPVOID pvOrgPtr, DWORD dwOrgSize, CHAR **ppszText)
{
    if (pvOrgPtr == *ppszText)
    {
        // the pointer has not been changed by the callback...
        // must convert from A to W in-place

        if (dwOrgSize)
        {
            LPWSTR pszW = ProduceWFromA(uiCodePage, *ppszText);
            if (pszW)
            {
                lstrcpynW((WCHAR *)(*ppszText), pszW, dwOrgSize);   // this becomes a W buffer
                FreeProducedString(pszW);
            }
        }
    }
    else
    {
        // the pointer has been changed out from underneath us, copy
        // unicode back into the original buffer.

        ConvertAToWN(uiCodePage, pvOrgPtr, dwOrgSize, *ppszText, -1);
        *ppszText = pvOrgPtr;
    }
}

typedef struct tagTHUNKSTATE {
    LPVOID ts_pvThunk1;
    LPVOID ts_pvThunk2;
    DWORD ts_dwThunkSize;
} THUNKSTATE;

//
//  InOutWtoA/InOutAtoW is for thunking INOUT string parameters.
//
//  INOUT parameters suck.
//
// We need to save both the original ANSI and the
// original UNICODE strings, so that if the app doesn't
// change the ANSI string, we leave the original UNICODE
// string alone.  That way, UNICODE item names don't get
// obliterated by the thunk.
//
// The original buffer is saved in pvThunk1.
// We allocate two ANSI buffers.
// pvThunk2 contains the original ANSIfied string.
// pvThunk2+cchTextMax is the buffer we pass to the app.
// On the way back, we compare pvThunk2 with pvThunk2+cchTextMax.
// If they are different, then we unthunk the string; otherwise,
// we leave the original UNICODE buffer alone.

BOOL InOutWtoA(CONTROLINFO *pci, THUNKSTATE *pts, LPWSTR *ppsz, DWORD cchTextMax)
{
    pts->ts_pvThunk1 = *ppsz;               // Save original buffer
    pts->ts_dwThunkSize = cchTextMax;

    if (!IsFlagPtr(pts->ts_pvThunk1))
    {
        pts->ts_pvThunk2 = LocalAlloc(LPTR, cchTextMax * 2 * sizeof(char));
        if (!ConvertWToAN(pci->uiCodePage, (LPSTR)pts->ts_pvThunk2, pts->ts_dwThunkSize, (LPWSTR)pts->ts_pvThunk1, -1))
        {
            LocalFree(pts->ts_pvThunk2);
            return 0;
        }
        *ppsz = (LPWSTR)((LPSTR)pts->ts_pvThunk2 + cchTextMax);
        lstrcpyA((LPSTR)*ppsz, pts->ts_pvThunk2);
    }
    return TRUE;
}

void InOutAtoW(CONTROLINFO *pci, THUNKSTATE *pts, LPSTR *ppsz)
{
    if (!IsFlagPtr(pts->ts_pvThunk1))
    {
        if (!IsFlagPtr(*ppsz) &&
            lstrcmpA(pts->ts_pvThunk2, (LPSTR)*ppsz) != 0)
            StringBufferAtoW(pci->uiCodePage, pts->ts_pvThunk1, pts->ts_dwThunkSize, ppsz);
        LocalFree(pts->ts_pvThunk2);
    }
    *ppsz = pts->ts_pvThunk1;
}

#endif // UNICODE


LRESULT WINAPI CCSendNotify(CONTROLINFO * pci, int code, LPNMHDR pnmhdr)
{
    NMHDR nmhdr;
    LONG_PTR id;
#ifdef UNICODE
    THUNKSTATE ts = { 0 };
    #define pvThunk1 ts.ts_pvThunk1
    #define pvThunk2 ts.ts_pvThunk2
    #define dwThunkSize ts.ts_dwThunkSize
    LRESULT lRet;
    BOOL  bSet = FALSE;
#endif
    HWND hwndParent = pci->hwndParent;
    DWORD dwParentPid;

    // -1 means Requery on each notify
    if ( hwndParent == (HWND)-1 )
    {
        hwndParent = GetParent(pci->hwnd);
    }

    // unlikely but it can technically happen -- avoid the rips
    if ( hwndParent == NULL )
        return 0;

    //
    // If pci->hwnd is -1, then a WM_NOTIFY is being forwared
    // from one control to a parent.  EG:  Tooltips sent
    // a WM_NOTIFY to toolbar, and toolbar is forwarding it
    // to the real parent window.
    //

    if (pci->hwnd != (HWND) -1) {

        //
        // If this is a child then get its ID.  We need to go out of our way to
        // avoid calling GetDlgCtrlID on toplevel windows since it will return
        // a pseudo-random number (those of you who know what this number is
        // keep quiet).  Anyway it's kinda hard to figure this out in Windows
        // because of the following:
        //
        //  - a window can SetWindowLong(GWL_STYLE, WS_CHILD) but this only
        //    does about half the work - hence checking the style is out.
        //  - GetParent will return your OWNER if you are toplevel.
        //  - there is no GetWindow(...GW_HWNDPARENT) to save us.
        //
        // Hence we are stuck with calling GetParent and then checking to see
        // if it lied and gave us the owner instead.  Yuck.
        //
        id = 0;
        if (pci->hwnd) {
            HWND hwndParent = GetParent(pci->hwnd);

            if (hwndParent && (hwndParent != GetWindow(pci->hwnd, GW_OWNER))) {
                id = GetDlgCtrlID(pci->hwnd);
            }
        }

        if (!pnmhdr)
            pnmhdr = &nmhdr;

        pnmhdr->hwndFrom = pci->hwnd;
        pnmhdr->idFrom = id;
        pnmhdr->code = code;
    } else {

        id = pnmhdr->idFrom;
        code = pnmhdr->code;
    }


    // OLE in its massively componentized world sometimes creates
    // a control whose parent belongs to another process.  (For example,
    // when there is a local server embedding.)  WM_NOTIFY
    // messages can't cross process boundaries, so stop the message
    // from going there lest we fault the recipient.
    if (!GetWindowThreadProcessId(hwndParent, &dwParentPid) ||
        dwParentPid != GetCurrentProcessId())
    {
        TraceMsg(TF_WARNING, "nf: Not sending WM_NOTIFY %08x across processes", code);
        return 0;
    }

#ifdef NEED_WOWGETNOTIFYSIZE_HELPER
    ASSERT(code >= 0 || WOWGetNotifySize(code));
#endif // NEED_WOWGETNOTIFYSIZE_HELPER

#ifdef UNICODE
    /*
     * All the thunking for Notify Messages happens here
     */
    if (!pci->bUnicode) {
        BOOL fThunked = TRUE;
        switch( code ) {
        case LVN_ODFINDITEMW:
            pnmhdr->code = LVN_ODFINDITEMA;
            goto ThunkLV_FINDINFO;

        case LVN_INCREMENTALSEARCHW:
            pnmhdr->code = LVN_INCREMENTALSEARCHA;
            goto ThunkLV_FINDINFO;

        ThunkLV_FINDINFO:
            {
                LV_FINDINFO *plvfi;

                // Hack Alert!  This code assumes that all fields of LV_FINDINFOA and
                // LV_FINDINFOW are exactly the same except for the string pointers.
                COMPILETIME_ASSERT(sizeof(LV_FINDINFOA) == sizeof(LV_FINDINFOW));

                // Since WCHARs are bigger than char, we will just use the
                // wchar buffer to hold the chars, and not worry about the extra
                // room at the end.
                COMPILETIME_ASSERT(sizeof(WCHAR) >= sizeof(char));

                plvfi = &((PNM_FINDITEM)pnmhdr)->lvfi;
                if (plvfi->flags & (LVFI_STRING | LVFI_PARTIAL | LVFI_SUBSTRING))
                {
                    pvThunk1 = (PVOID)plvfi->psz;
                    dwThunkSize = lstrlen(pvThunk1) + 1;
                    plvfi->psz = (LPWSTR)ProduceAFromW(pci->uiCodePage, plvfi->psz);
                }
            }
            break;

        case LVN_GETDISPINFOW: {
            LV_ITEMW *pitem;

            pnmhdr->code = LVN_GETDISPINFOA;

            // Hack Alert!  This code assumes that all fields of LV_DISPINFOA and
            // LV_DISPINFOW are exactly the same except for the string pointers.

            COMPILETIME_ASSERT(sizeof(LV_DISPINFOA) == sizeof(LV_DISPINFOW));

            // Since WCHARs are bigger than char, we will just use the
            // wchar buffer to hold the chars, and not worry about the extra
            // room at the end.
            COMPILETIME_ASSERT(sizeof(WCHAR) >= sizeof(char));

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


        // LVN_ENDLABELEDIT uses an INOUT parameter, never explicitly
        // documented as such, but it just happened to be that way,
        // and I don't want to take the chance that somebody was relying
        // on it.

        case LVN_ENDLABELEDITW:
            pnmhdr->code = LVN_ENDLABELEDITA;
            goto ThunkLV_DISPINFO;

        case LVN_BEGINLABELEDITW:
            pnmhdr->code = LVN_BEGINLABELEDITA;
            goto ThunkLV_DISPINFO;

        case LVN_SETDISPINFOW:
            pnmhdr->code = LVN_SETDISPINFOA;
            goto ThunkLV_DISPINFO;

        case LVN_GETEMPTYTEXTW:
            pnmhdr->code = LVN_GETEMPTYTEXTA;
            goto ThunkLV_DISPINFO;

        ThunkLV_DISPINFO: {
            LV_ITEMW *pitem;

            COMPILETIME_ASSERT(sizeof(LV_ITEMA) == sizeof(LV_ITEMW));
            pitem = &(((LV_DISPINFOW *)pnmhdr)->item);

            if (pitem->mask & LVIF_TEXT) {
                if (!InOutWtoA(pci, &ts, &pitem->pszText, pitem->cchTextMax))
                    return 0;
            }
            break;
        }

        case LVN_GETINFOTIPW: {
            NMLVGETINFOTIPW *pgit = (NMLVGETINFOTIPW *)pnmhdr;

            COMPILETIME_ASSERT(sizeof(NMLVGETINFOTIPA) == sizeof(NMLVGETINFOTIPW));
            pnmhdr->code = LVN_GETINFOTIPA;

            if (!InOutWtoA(pci, &ts, &pgit->pszText, pgit->cchTextMax))
                return 0;
        }
        break;


        case TVN_GETINFOTIPW:
            {
                NMTVGETINFOTIPW *pgit = (NMTVGETINFOTIPW *)pnmhdr;

                pnmhdr->code = TVN_GETINFOTIPA;

                pvThunk1 = pgit->pszText;
                dwThunkSize = pgit->cchTextMax;
            }
            break;

        case TBN_GETINFOTIPW:
            {
                NMTBGETINFOTIPW *pgit = (NMTBGETINFOTIPW *)pnmhdr;

                pnmhdr->code = TBN_GETINFOTIPA;

                pvThunk1 = pgit->pszText;
                dwThunkSize = pgit->cchTextMax;
            }
            break;

        case TVN_SELCHANGINGW:
            pnmhdr->code = TVN_SELCHANGINGA;
            bSet = TRUE;
            // fall through
            
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
             
            // fall through

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
            // fall through

        case TVN_ITEMEXPANDINGW:
            if (!bSet) {
                pnmhdr->code = TVN_ITEMEXPANDINGA;
                bSet = TRUE;
            }
            // fall through

        case TVN_ITEMEXPANDEDW:
            if (!bSet) {
                pnmhdr->code = TVN_ITEMEXPANDEDA;
                bSet = TRUE;
            }
            // fall through

        case TVN_BEGINDRAGW:
            if (!bSet) {
                pnmhdr->code = TVN_BEGINDRAGA;
                bSet = TRUE;
            }
            // fall through

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
            goto ThunkTV_DISPINFO;

        case TVN_BEGINLABELEDITW:
            pnmhdr->code = TVN_BEGINLABELEDITA;
            goto ThunkTV_DISPINFO;


        // TVN_ENDLABELEDIT uses an INOUT parameter, never explicitly
        // documented as such, but it just happened to be that way,
        // and I don't want to take the chance that somebody was relying
        // on it.

        case TVN_ENDLABELEDITW:
            pnmhdr->code = TVN_ENDLABELEDITA;
            goto ThunkTV_DISPINFO;

        ThunkTV_DISPINFO: {
            /*
             * All these messages have a TV_DISPINFO in lParam.
             */

            LPTV_ITEMW pitem;

            pitem = &(((TV_DISPINFOW *)pnmhdr)->item);

            if (pitem->mask & TVIF_TEXT) {
                if (!InOutWtoA(pci, &ts, &pitem->pszText, pitem->cchTextMax))
                    return 0;
            }
            break;
        }

#if !defined(UNIX) || defined(ANSI_SHELL32_ON_UNIX)
/* UNIX shell32 TVN_GETDISPINFOA TVN_GETDISPINFOW does the same thing */
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
                pvThunk2 = LocalAlloc(LPTR, pitem->cchTextMax * sizeof(char));
                pitem->pszText = pvThunk2;
                pitem->pszText[0] = TEXT('\0');
            }

            break;
        }
#endif

        case HDN_ITEMCHANGINGW:
            pnmhdr->code = HDN_ITEMCHANGINGA;
            bSet = TRUE;
            // fall through

        case HDN_ITEMCHANGEDW:
            if (!bSet) {
                pnmhdr->code = HDN_ITEMCHANGEDA;
                bSet = TRUE;
            }
            // fall through

        case HDN_ITEMCLICKW:
            if (!bSet) {
                pnmhdr->code = HDN_ITEMCLICKA;
                bSet = TRUE;
            }
            // fall through

        case HDN_ITEMDBLCLICKW:
            if (!bSet) {
                pnmhdr->code = HDN_ITEMDBLCLICKA;
                bSet = TRUE;
            }
            // fall through

        case HDN_DIVIDERDBLCLICKW:
            if (!bSet) {
                pnmhdr->code = HDN_DIVIDERDBLCLICKA;
                bSet = TRUE;
            }
            // fall through

        case HDN_BEGINTRACKW:
            if (!bSet) {
                pnmhdr->code = HDN_BEGINTRACKA;
                bSet = TRUE;
            }
            // fall through

        case HDN_ENDTRACKW:
            if (!bSet) {
                pnmhdr->code = HDN_ENDTRACKA;
                bSet = TRUE;
            }
            // fall through

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


            if ( !IsFlagPtr(pitem) && (pitem->mask & HDI_FILTER) && pitem->pvFilter )
            {
                if ( !(pitem->type & HDFT_HASNOVALUE) &&
                        ((pitem->type & HDFT_ISMASK)==HDFT_ISSTRING) )
                {
                    LPHD_TEXTFILTER ptextFilter = (LPHD_TEXTFILTER)pitem->pvFilter;
                    pvThunk2 = ptextFilter->pszText;
                    dwThunkSize = ptextFilter->cchTextMax;
                    ptextFilter->pszText = (LPWSTR)ProduceAFromW(pci->uiCodePage, pvThunk2);
                }
            }


            break;
        }

        case CBEN_ENDEDITW:
        {
            LPNMCBEENDEDITW peew = (LPNMCBEENDEDITW) pnmhdr;
            LPNMCBEENDEDITA peea = LocalAlloc(LPTR, sizeof(NMCBEENDEDITA));

            if (!peea)
               return 0;

            peea->hdr  = peew->hdr;
            peea->hdr.code = CBEN_ENDEDITA;

            peea->fChanged = peew->fChanged;
            peea->iNewSelection = peew->iNewSelection;
            peea->iWhy = peew->iWhy;
            ConvertWToAN(pci->uiCodePage, peea->szText, ARRAYSIZE(peea->szText),
                         peew->szText, -1);

            pvThunk1 = pnmhdr;
            pnmhdr = &peea->hdr;
            ASSERT((LPVOID)pnmhdr == (LPVOID)peea);
            break;
        }

        case CBEN_DRAGBEGINW:
        {
            LPNMCBEDRAGBEGINW pdbw = (LPNMCBEDRAGBEGINW) pnmhdr;
            LPNMCBEDRAGBEGINA pdba = LocalAlloc(LPTR, sizeof(NMCBEDRAGBEGINA));

            if (!pdba)
               return 0;

            pdba->hdr  = pdbw->hdr;
            pdba->hdr.code = CBEN_DRAGBEGINA;
            pdba->iItemid = pdbw->iItemid;
            ConvertWToAN(pci->uiCodePage, pdba->szText, ARRAYSIZE(pdba->szText),
                         pdbw->szText, -1);

            pvThunk1 = pnmhdr;
            pnmhdr = &pdba->hdr;
            ASSERT((LPVOID)pnmhdr == (LPVOID)pdba);
            break;
        }


        case CBEN_GETDISPINFOW: {
            PNMCOMBOBOXEXW pnmcbe = (PNMCOMBOBOXEXW)pnmhdr;

            pnmhdr->code = CBEN_GETDISPINFOA;

            if (pnmcbe->ceItem.mask  & CBEIF_TEXT
                && !IsFlagPtr(pnmcbe->ceItem.pszText) && pnmcbe->ceItem.cchTextMax) {
                pvThunk1 = pnmcbe->ceItem.pszText;
                dwThunkSize = pnmcbe->ceItem.cchTextMax;
                pvThunk2 = LocalAlloc(LPTR, pnmcbe->ceItem.cchTextMax * sizeof(char));
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
            pHDDispInfoW->pszText = LocalAlloc (LPTR, pHDDispInfoW->cchTextMax * sizeof(char));

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
            pvThunk2 = LocalAlloc (LPTR, pTBNW->cchText * sizeof(char));

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

            lpTTTA = LocalAlloc(LPTR, sizeof(TOOLTIPTEXTA));

            if (!lpTTTA)
               return 0;

            lpTTTA->hdr = lpTTTW->hdr;
            lpTTTA->hdr.code = TTN_NEEDTEXTA;

            lpTTTA->lpszText = lpTTTA->szText;
            lpTTTA->hinst    = lpTTTW->hinst;
            lpTTTA->uFlags   = lpTTTW->uFlags;
            lpTTTA->lParam   = lpTTTW->lParam;

            WideCharToMultiByte(pci->uiCodePage, 0, lpTTTW->szText, -1, lpTTTA->szText, ARRAYSIZE(lpTTTA->szText), NULL, NULL);
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
            fThunked = FALSE;
            break;
        }

#ifdef NEED_WOWGETNOTIFYSIZE_HELPER
        ASSERT(code >= 0 || WOWGetNotifySize(code));
#endif // NEED_WOWGETNOTIFYSIZE_HELPER

        lRet = SendMessage(hwndParent, WM_NOTIFY, (WPARAM)id, (LPARAM)pnmhdr);

        /*
         * All the thunking for Notify Messages happens here
         */
        if (fThunked)
        {
        switch(pnmhdr->code) {
        case LVN_ODFINDITEMA:
        case LVN_INCREMENTALSEARCHA:
            {
                LV_FINDINFO *plvfi = &((PNM_FINDITEM)pnmhdr)->lvfi;
                if (pvThunk1)
                {
                    FreeProducedString((LPWSTR)plvfi->psz);
                    plvfi->psz = pvThunk1;
                }
            }
            break;

        case LVN_GETDISPINFOA:
            {
                LV_ITEMA *pitem = &(((LV_DISPINFOA *)pnmhdr)->item);

                // BUGBUG what if pointer is to large buffer?
                if (!IsFlagPtr(pitem) && (pitem->mask & LVIF_TEXT) && !IsFlagPtr(pitem->pszText))
                {
                    StringBufferAtoW(pci->uiCodePage, pvThunk1, dwThunkSize, &pitem->pszText);
                }
            }
            break;

        case LVN_ENDLABELEDITA:
        case LVN_BEGINLABELEDITA:
        case LVN_SETDISPINFOA:
        case LVN_GETEMPTYTEXTA:
            {
                LV_ITEMA *pitem = &(((LV_DISPINFOA *)pnmhdr)->item);
                InOutAtoW(pci, &ts, &pitem->pszText);
            }
            break;

        case LVN_GETINFOTIPA:
            {
                NMLVGETINFOTIPA *pgit = (NMLVGETINFOTIPA *)pnmhdr;
                InOutAtoW(pci, &ts, &pgit->pszText);
            }
            break;

        case TVN_GETINFOTIPA:
            {
                NMTVGETINFOTIPA *pgit = (NMTVGETINFOTIPA *)pnmhdr;
                StringBufferAtoW(pci->uiCodePage, pvThunk1, dwThunkSize, &pgit->pszText);
            }
            break;

        case TBN_GETINFOTIPA:
            {
                NMTBGETINFOTIPA *pgit = (NMTBGETINFOTIPA *)pnmhdr;
                StringBufferAtoW(pci->uiCodePage, pvThunk1, dwThunkSize, &pgit->pszText);
            }
            break;
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
            // fall through

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
        case TVN_ENDLABELEDITA:
            {
                LPTV_ITEMA pitem;
                pitem = &(((TV_DISPINFOA *)pnmhdr)->item);
                InOutAtoW(pci, &ts, &pitem->pszText);
            }
            break;

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

            if ( !IsFlagPtr(pitem) && (pitem->mask & HDI_FILTER) && pitem->pvFilter && pvThunk2 )
            {
                if ( !(pitem->type & HDFT_HASNOVALUE) &&
                        ((pitem->type & HDFT_ISMASK)==HDFT_ISSTRING) )
                {
                    LPHD_TEXTFILTER ptextFilter = (LPHD_TEXTFILTER)pitem->pvFilter;
                    ConvertAToWN(pci->uiCodePage, pvThunk2, dwThunkSize, (LPSTR)(ptextFilter->pszText), -1);
                    FreeProducedString(ptextFilter->pszText);
                    ptextFilter->pszText = pvThunk2;
                }
            }

            break;
        }

        case CBEN_ENDEDITA:
            {
            LPNMCBEENDEDITW peew = (LPNMCBEENDEDITW) pvThunk1;
            LPNMCBEENDEDITA peea = (LPNMCBEENDEDITA) pnmhdr;

            // Don't unthunk the string since that destroys unicode round-trip
            // and the client shouldn't be modifying it anyway.
            // ConvertAToWN(pci->uiCodePage, peew->szText, ARRAYSIZE(peew->szText),
            //              peea->szText, -1);
            LocalFree(peea);
            }
            break;

        case CBEN_DRAGBEGINA:
            {
            LPNMCBEDRAGBEGINW pdbw = (LPNMCBEDRAGBEGINW) pvThunk1;
            LPNMCBEDRAGBEGINA pdba = (LPNMCBEDRAGBEGINA) pnmhdr;

            // Don't unthunk the string since that destroys unicode round-trip
            // and the client shouldn't be modifying it anyway.
            // ConvertAToWN(pci->uiCodePage, pdbw->szText, ARRAYSIZE(pdbw->szText),
            //              pdba->szText, -1);
            LocalFree(pdba);
            }
            break;

        case CBEN_GETDISPINFOA:
        {
            PNMCOMBOBOXEXW pnmcbeW;

            pnmcbeW = (PNMCOMBOBOXEXW)pnmhdr;
            ConvertAToWN(pci->uiCodePage, pvThunk1, dwThunkSize, (LPSTR)(pnmcbeW->ceItem.pszText), -1);

            if (pvThunk2)
                LocalFree(pvThunk2);
            pnmcbeW->ceItem.pszText = pvThunk1;

        }
            break;


        case HDN_GETDISPINFOA:
            {
            LPNMHDDISPINFOW pHDDispInfoW;

            pHDDispInfoW = (LPNMHDDISPINFOW)pnmhdr;
            ConvertAToWN(pci->uiCodePage, pvThunk1, dwThunkSize, (LPSTR)(pHDDispInfoW->pszText), -1);

            LocalFree(pHDDispInfoW->pszText);
            pHDDispInfoW->pszText = pvThunk1;

            }
            break;

        case TBN_GETBUTTONINFOA:
            {
            LPTBNOTIFYW pTBNW;

            pTBNW = (LPTBNOTIFYW)pnmhdr;
            ConvertAToWN(pci->uiCodePage, pvThunk1, dwThunkSize, (LPSTR)(pTBNW->pszText), -1);

            pTBNW->pszText = pvThunk1;
            LocalFree(pvThunk2);

            }
            break;


        case TTN_NEEDTEXTA:
            {
            LPTOOLTIPTEXTA lpTTTA = (LPTOOLTIPTEXTA) pnmhdr;
            LPTOOLTIPTEXTW lpTTTW = (LPTOOLTIPTEXTW) pvThunk1;

            ThunkToolTipTextAtoW (lpTTTA, lpTTTW, pci->uiCodePage);
            LocalFree(lpTTTA);
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
        }
        return lRet;
    } else
#endif
        return(SendMessage(hwndParent, WM_NOTIFY, (WPARAM)id, (LPARAM)pnmhdr));

#undef pvThunk1
#undef pvThunk2
#undef dwThunkSize
}

LRESULT WINAPI SendNotify(HWND hwndTo, HWND hwndFrom, int code, NMHDR FAR* pnmhdr)
{
    CONTROLINFO ci;
    ci.hwndParent = hwndTo;
    ci.hwnd = hwndFrom;
    ci.bUnicode = FALSE;
    ci.uiCodePage = CP_ACP;

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
    dwRet = (DWORD) CCSendNotify(lpci, NM_CUSTOMDRAW, &lpnmcd->hdr);

    // validate the flags
    if (dwRet & ~CDRF_VALIDFLAGS)
        return CDRF_DODEFAULT;

    return dwRet;
}

//
//  Too many apps encounter strange behavior when we send out
//  NM_CUSTOMDRAW messages at times unrelated to painting.
//  E.g., NetMeeting and MFC recurse back into ListView_RecomputeLabelSize.
//  CryptUI will fault if it's asked to NM_CUSTOMDRAW before it gets
//  WM_INITDIALOG.  So all this fake customdraw stuff is v5 only.
//
//  And since it is very popular to call back into the control during
//  the handling of NM_CUSTOMDRAW, we protect against recursing ourselves
//  to death by blowing off nested fake customdraw messages.


DWORD CIFakeCustomDrawNotify(LPCONTROLINFO lpci, DWORD dwStage, LPNMCUSTOMDRAW lpnmcd)
{
    DWORD dwRet = CDRF_DODEFAULT;

    if (lpci->iVersion >= 5 && !lpci->bInFakeCustomDraw)
    {
        lpci->bInFakeCustomDraw = TRUE;
        dwRet = CICustomDrawNotify(lpci, dwStage, lpnmcd);
        ASSERT(lpci->bInFakeCustomDraw);
        lpci->bInFakeCustomDraw = FALSE;
    }

    return dwRet;
}

/*----------------------------------------------------------
Purpose: Release the capture and tell the parent we've done so.

Returns: Whether the control is still alive.
*/
BOOL CCReleaseCapture(CONTROLINFO * pci)
{
    HWND hwndCtl = pci->hwnd;
    NMHDR nmhdr = {0};

    ReleaseCapture();

    // Tell the parent we've released the capture
    CCSendNotify(pci, NM_RELEASEDCAPTURE, &nmhdr);

    return IsWindow(hwndCtl);
}


/*----------------------------------------------------------
Purpose: Set the capture.  If the hwndSet is NULL, it means
         the capture is being released, so tell the parent
         we've done so.

         Use this function if there's a possibility that the
         hwnd may be NULL.

*/
void CCSetCapture(CONTROLINFO * pci, HWND hwndSet)
{
    SetCapture(hwndSet);

    if (NULL == hwndSet)
    {
        NMHDR nmhdr = {0};

        // Tell the parent we've released the capture
        CCSendNotify(pci, NM_RELEASEDCAPTURE, &nmhdr);
    }
}
