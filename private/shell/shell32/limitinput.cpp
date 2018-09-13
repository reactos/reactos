/* ************************************************************** *\
   ToddB's Super Cool Balloon ToolTip InputLimiter

   Copyright Microsoft 1998
\* ************************************************************** */

#include "shellprv.h"

#include "LimitInput.h"

#define IsTextPtr(pszText)      ((LPSTR_TEXTCALLBACK != pszText) && !IS_INTRESOURCE(pszText))
#define CHAR_IN_RANGE(ch,l,h)   ((ch >= l) && (ch <= h))

#define LIMITINPUTTIMERID       472

// ************************************************************************************************
// CInputLimiter class description
// ************************************************************************************************

class CInputLimiter : public tagLIMITINPUT
{
protected:
    CInputLimiter();
    ~CInputLimiter();

    BOOL SubclassEditControl(HWND hwnd, LPLIMITINPUT pli);
    BOOL OnChar( HWND hwnd, WPARAM & wParam, LPARAM lParam);
    LRESULT OnPaste(HWND hwnd, WPARAM wParam, LPARAM lParam);
    void ShowToolTip();
    void HideToolTip();
    void CreateToolTipWindow();
    BOOL IsValidChar(TCHAR ch, BOOL bPaste);

    static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uID, ULONG_PTR dwRefData);

    HWND        m_hwnd;             // the subclassed edit control hwnd
    HWND        m_hwndToolTip;      // the tooltip control
    UINT        m_uTimerID;         // the timer id
    BOOL        m_dwCallbacks;      // true if any data is callback data.

    friend BOOL SHLimitInputEdit( HWND hwndEdit, LPLIMITINPUT pli );
};

CInputLimiter::CInputLimiter()
{
    // our allocation function should have zeroed our memory.  Check to make sure:
    ASSERT(0==m_hwndToolTip);
    ASSERT(0==m_uTimerID);
}

CInputLimiter::~CInputLimiter()
{
    // we might have allocated some strings, if we did delete them
    if ( IsTextPtr(pszFilter) )
    {
        delete pszFilter;
    }
    if ( IsTextPtr(pszTitle) )
    {
        delete pszTitle;
    }
    if ( IsTextPtr(pszMessage) )
    {
        delete pszMessage;
    }
}

BOOL CInputLimiter::SubclassEditControl(HWND hwnd, LPLIMITINPUT pli)
{
    if ( !IsWindow(hwnd) )
    {
        // must have a valid hwnd
        TraceMsg(TF_WARNING, "Invalid HWND passed to CInputLimiter::SubclassEditControl");
        return FALSE;
    }

    m_hwnd = hwnd;

    // validate all the data passed in the pli structure.  Return false if
    // any of it is out of whack.
    dwMask = pli->dwMask;

    if ( LIM_FLAGS & dwMask )
    {
        dwFlags = pli->dwFlags;

        if ( (LIF_FORCEUPPERCASE|LIF_FORCELOWERCASE) == ((LIF_FORCEUPPERCASE|LIF_FORCELOWERCASE) & dwFlags) )
        {
            // cannot use both ForceUpperCase and ForceLowerCase flags
            TraceMsg(TF_WARNING, "cannot use both ForceUpperCase and ForceLowerCase flags");
            return FALSE;
        }
    }
    else
    {
        ASSERT(0==dwFlags);
    }

    if ( LIM_HINST & dwMask )
    {
        hinst = pli->hinst;
    }
    else
    {
        ASSERT(0==hinst);
    }

    // keep track of which fields require a valid hwndNotify
    ASSERT(0==m_dwCallbacks);

    if ( LIM_FILTER & dwMask )
    {
        if ( LIF_CATEGORYFILTER & dwFlags )
        {
            // category filters are not callbacks or int resources even though the data looks like it is.
            // The don't need any validation.
            pszFilter = pli->pszFilter;
        }
        else if ( LPSTR_TEXTCALLBACK == pli->pszFilter )
        {
            pszFilter = pli->pszFilter;
            m_dwCallbacks |= LIM_FILTER;
        }
        else if ( IS_INTRESOURCE(pli->pszFilter) )
        {
            if ( !hinst )
            {
                // must have valid hinst in order to use int resources
                TraceMsg(TF_WARNING, "must have valid hinst in order to use int resources for filter");
                return FALSE;
            }

            // We need to load the target string upfront and store it in a buffer.
            DWORD cchSize = 64;
            DWORD cchLoaded;

            for ( ;; )
            {
                pszFilter = new TCHAR[cchSize];
                if ( !pszFilter )
                {
                    // Out of memory
                    TraceMsg(TF_WARNING, "Out of memory in CInputLimiter::SubclassEditControl");
                    return FALSE;
                }

                cchLoaded = LoadString(hinst, (UINT)pli->pszFilter, pszFilter, cchSize);
                if ( 0 == cchLoaded )
                {
                    // Could not load filter resource, pszFilter will get deleted in our destructor
                    TraceMsg(TF_WARNING, "Could not load filter resource");
                    return FALSE;
                }
                else if ( cchLoaded >= cchSize-1 )
                {
                    // didn't fit in the given buffer, try a larger buffer
                    delete [] pszFilter;
                    cchSize *= 2;
                }
                else
                {
                    // the string loaded successfully
                    break;
                }
            }
                    
            ASSERT(IS_VALID_STRING_PTR(pszFilter,-1));
        }
        else
        {
            ASSERT(IS_VALID_STRING_PTR(pli->pszFilter,-1));
            pszFilter = new TCHAR[lstrlen(pli->pszFilter)+1];
            if ( !pszFilter )
            {
                // Out of memory
                TraceMsg(TF_WARNING, "CInputLimiter Out of memory");
                return FALSE;
            }
            StrCpy(pszFilter, pli->pszFilter);
        }
    }
    else
    {
        ASSERT(0==pszFilter);
    }

    if ( !(LIF_WARNINGOFF & dwFlags) && !((LIM_TITLE|LIM_MESSAGE) & dwMask) )
    {
        // if warnings are on then at least one of Title or Message is required.
        TraceMsg(TF_WARNING, "if warnings are on then at least one of Title or Message is required");
        return FALSE;
    }

    if ( LIM_TITLE & dwMask )
    {
        if ( LPSTR_TEXTCALLBACK == pli->pszTitle )
        {
            pszTitle = pli->pszTitle;
            m_dwCallbacks |= LIM_TITLE;
        }
        else if ( IS_INTRESOURCE(pli->pszTitle) )
        {
            if ( !hinst )
            {
                // must have valid hinst in order to use int resources
                TraceMsg(TF_WARNING, "must have valid hinst in order to use int resources for title");
                return FALSE;
            }
            // REVIEW: Does the title need to be laoded up fromt or will the ToolTip control do this
            // for us?
            pszTitle = pli->pszTitle;
        }
        else
        {
            ASSERT(IS_VALID_STRING_PTR(pli->pszTitle,-1));
            pszTitle = new TCHAR[lstrlen(pli->pszTitle)+1];
            StrCpy(pszTitle, pli->pszTitle);
        }
    }
    else
    {
        ASSERT(0==pszTitle);
    }

    if ( LIM_MESSAGE & dwMask )
    {
        if ( LPSTR_TEXTCALLBACK == pli->pszMessage )
        {
            pszMessage = pli->pszMessage;
            m_dwCallbacks |= LIM_MESSAGE;
        }
        else if ( IS_INTRESOURCE(pli->pszMessage) )
        {
            if ( !hinst )
            {
                // must have valid hinst in order to use int resources
                TraceMsg(TF_WARNING, "must have valid hinst in order to use int resources for message");
                return FALSE;
            }
            // We will let the ToolTip control load this string for us
            pszMessage = pli->pszMessage;
        }
        else
        {
            ASSERT(IS_VALID_STRING_PTR(pli->pszMessage,-1));
            pszMessage = new TCHAR[lstrlen(pli->pszMessage)+1];
            StrCpy(pszMessage, pli->pszMessage);
        }
    }
    else
    {
        ASSERT(0==pszMessage);
    }

    if ( LIM_ICON & dwMask )
    {
        hIcon = pli->hIcon;

        if ( I_ICONCALLBACK == hIcon )
        {
            m_dwCallbacks |= LIM_ICON;
        }
    }

    if ( LIM_NOTIFY & dwMask )
    {
        hwndNotify = pli->hwndNotify;
    }
    else
    {
        hwndNotify = GetParent( m_hwnd );
    }

    if ( m_dwCallbacks && !IsWindow(hwndNotify) )
    {
        // invalid notify window
        TraceMsg(TF_WARNING, "invalid notify window");
        return FALSE;
    }

    if ( LIM_TIMEOUT & dwMask )
    {
        iTimeout = pli->iTimeout;
    }
    else
    {
        iTimeout = 10000;
    }

    if ( LIM_TIPWIDTH & dwMask )
    {
        cxTipWidth = pli->cxTipWidth;
    }
    else
    {
        cxTipWidth = 500;
    }

    // everything in the *pli structure is valid
    TraceMsg(TF_GENERAL, "pli structure is valid");

    return SetWindowSubclass(hwnd, CInputLimiter::SubclassProc, 0, (LONG_PTR)this);
}

LRESULT CALLBACK CInputLimiter::SubclassProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uID, ULONG_PTR dwRefData)
{
    CInputLimiter * pthis = (CInputLimiter*)dwRefData;

    switch (uMsg)
    {
    case WM_CHAR:
        if (!pthis->OnChar(hwnd, wParam, lParam))
        {
            return 0;
        }
        break;

    case WM_KILLFOCUS:
        pthis->HideToolTip();
        break;

    case WM_PASTE:
        // Paste handler handles calling the super wnd proc when needed
        return pthis->OnPaste(hwnd, wParam, lParam);

    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, CInputLimiter::SubclassProc, uID);
        delete pthis;
        break;

    default:
        break;
    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

BOOL CInputLimiter::IsValidChar(TCHAR ch, BOOL bPaste)
{
    BOOL  bValidChar = FALSE;           // start by assuming the character is invalid

    if ( LIF_CATEGORYFILTER & dwFlags )
    {
        TraceMsg(TF_GENERAL, "Processing LIF_CATEGORYFILTER: <0x%08x>", (WORD)pszFilter );
        // pszFilter is actually a bit field with valid character types
        WORD CharType = 0;
#define GETSTRINGTYPEEX_MASK    0x1FF

        // We only need to call GetStringTypeEx if some of the CT_TYPE1 values are being asked for
        if ( ((WORD)pszFilter) & GETSTRINGTYPEEX_MASK )
        {
            TraceMsg(TF_GENERAL, "Calling GetStringTypeEx" );

            // We treat ch as a one character long string.
            // REVIEW: How are DBCS characters handled?  Is this fundamentally flawed for win9x?
            EVAL(GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, (LPTSTR)&ch, 1, &CharType));
        }

        if ( ((WORD)pszFilter) & (WORD)CharType)
        {
            TraceMsg(TF_GENERAL, "GetStringTypeEx matched a character" );
            // GetStringTypeEx found the string in one of the selected groups
            bValidChar = !( LIF_EXCLUDEFILTER & dwFlags );
        }
        else
        {
            TraceMsg(TF_GENERAL, "Checking the extra types not supported by GetStringTypeEx" );
            // check for the string in our special groups.  We will temporarily use bValidChar
            // to indicate whether the character was found, not whether it's valid.
            if ( LICF_BINARYDIGIT & (DWORD)pszFilter )
            {
                if ( CHAR_IN_RANGE(ch, TEXT('0'), TEXT('1')) )
                {
                    bValidChar = TRUE;
                    goto charWasFound;
                }
            }
            if ( LICF_OCTALDIGIT & (DWORD)pszFilter )
            {
                if ( CHAR_IN_RANGE(ch, TEXT('0'), TEXT('7')) )
                {
                    bValidChar = TRUE;
                    goto charWasFound;
                }
            }
            if ( LICF_ATOZUPPER & (DWORD)pszFilter )
            {
                if ( CHAR_IN_RANGE(ch, TEXT('A'), TEXT('Z')) )
                {
                    bValidChar = TRUE;
                    goto charWasFound;
                }
            }
            if ( LICF_ATOZLOWER & (DWORD)pszFilter )
            {
                if ( CHAR_IN_RANGE(ch, TEXT('a'), TEXT('z')) )
                {
                    bValidChar = TRUE;
                    goto charWasFound;
                }
            }

charWasFound:
            // right now we have perverted the meaning of bValidChar to indicate if the
            // character was found or not.  We now convert the meaning from "was the
            // character found" to "is the character valid" by considering LIF_EXCLUDEFILTER.
            if ( LIF_EXCLUDEFILTER & dwFlags )
            {
                bValidChar = !bValidChar;
            }
        }
    }
    else
    {
        TraceMsg(TF_GENERAL, "Processing string based filter" );
        // pszFilter points to a NULL terminated string of characters
        LPTSTR psz;
        
        psz = StrChr(pszFilter, ch);

        if ( LIF_EXCLUDEFILTER & dwFlags )
        {
            bValidChar = (NULL == psz);
        }
        else
        {
            bValidChar = (NULL != psz);
        }
    }

    return bValidChar;
}
      
BOOL CInputLimiter::OnChar( HWND hwnd, WPARAM & wParam, LPARAM lParam )
{
    // if the char is a good one return TRUE, this will pass the char on to the
    // default window proc.  For a bad character do a beep and then display the
    // ballon tooltip pointing at the control.
    TCHAR ch = (TCHAR)wParam;

    if ( LIM_FILTER & m_dwCallbacks )
    {
        // If we have callbacks then we need to update the filter and/or mask text.
        // Otherwise the filter and/or mask text is already correct.
        NMLIFILTERINFO lidi = {0};
        lidi.hdr.hwndFrom = m_hwnd;
        lidi.hdr.idFrom = GetWindowLong(m_hwnd, GWL_ID);
        lidi.hdr.code = LIN_GETFILTERINFO;
        lidi.li.dwMask = LIM_FILTER & m_dwCallbacks;

        SendMessage(hwndNotify, WM_NOTIFY, lidi.hdr.idFrom, (LPARAM)&lidi);

        pszFilter = lidi.li.pszFilter;

        // REVIEW: we should have a way for the notify hanlder to say "store this
        // result and stop asking me for the filter to use every time".
    }

    if ( LIF_FORCEUPPERCASE & dwFlags )
    {
        ch = (TCHAR)CharUpper((LPTSTR)ch);
    }
    else if ( LIF_FORCELOWERCASE & dwFlags )
    {
        ch = (TCHAR)CharLower((LPTSTR)ch);
    }

    if ( IsValidChar(ch, FALSE) )
    {
        if ( LIF_HIDETIPONVALID & dwFlags )
        {
            HideToolTip();
        }

        // We might have upper or lower cased ch, so reflect this in wParam.  Since
        // wParam was passed by reference this will effect the message we forward
        // on to the original window proc.
        wParam = (WPARAM)ch;

        return TRUE;
    }
    else
    {
        // if we get here then an invalid character was entered

        if ( LIF_NOTIFYONBADCHAR & dwFlags )
        {
            NMLIBADCHAR libc = {0};
            libc.hdr.hwndFrom = m_hwnd;
            libc.hdr.idFrom = GetWindowLong(m_hwnd, GWL_ID);
            libc.hdr.code = LIN_BADCHAR;
            libc.wParam = wParam;           // use the original, non case shifted wParam
            libc.lParam = lParam;

            SendMessage(hwndNotify, WM_NOTIFY, libc.hdr.idFrom, (LPARAM)&libc);
        }

        if ( !(LIF_SILENT & dwFlags) )
        {
            MessageBeep(MB_OK);
        }

        if ( !(LIF_WARNINGOFF & dwFlags) )
        {
            ShowToolTip();
        }

        return FALSE;
    }
}

LRESULT CInputLimiter::OnPaste(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    // There are hundreds of lines of code in user to successfully handle a paste into an edit control.
    // We need to leverage all that code while still disallowing invalid input to result from the paste.
    // As a result, what we need to do is to get the clip board data, validate that data, place the
    // valid data back onto the clipboard, call the default window proc to let user do it's thing, and
    // then restore the clipboard to it's original format.
    if ( OpenClipboard(hwnd) )
    {
        HANDLE hdata;
        UINT iFormat;
        DWORD cchBad = 0;           // count of the number of bad characters

        // REVIEW: Should this be based on the compile type or the window type?
        // Compile time check for the correct clipboard format to use:
        if ( sizeof(WCHAR) == sizeof(TCHAR) )
        {
            iFormat = CF_UNICODETEXT;
        }
        else
        {
            iFormat = CF_TEXT;
        }

        hdata = GetClipboardData(iFormat);

        if ( hdata )
        {
            LPTSTR pszData;
            pszData = (LPTSTR)GlobalLock(hdata);
            if ( pszData )
            {
                DWORD dwSize;
                HANDLE hClone;
                HANDLE hNew;

                // we need to copy the original data because the clipboard owns the hdata
                // pointer.  That data will be invalid after we call SetClipboardData.
                // We start by calculating the size of the data:
                dwSize = (DWORD)GlobalSize(hdata)+sizeof(TCHAR);

                // Use the prefered GlobalAlloc for clipboard data
                hClone = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, dwSize);
                hNew = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, dwSize);
                if ( hClone && hNew )
                {
                    LPTSTR pszClone;
                    LPTSTR pszNew;

                    pszClone = (LPTSTR)GlobalLock(hClone);
                    pszNew = (LPTSTR)GlobalLock(hNew);
                    if ( pszClone && pszNew )
                    {
                        int iNew = 0;

                        // copy the original data as-is
                        memcpy((LPVOID)pszClone, (LPVOID)pszData, (size_t)dwSize);
                        // ensure that it's NULL terminated
                        pszClone[ (dwSize/sizeof(TCHAR))-1 ] = NULL;

                        // For a paste, we only call the filter callback once, not once for each
                        // character.  Why?  Because.
                        if ( LIM_FILTER & m_dwCallbacks )
                        {
                            // If we have callbacks then we need to update the filter and/or mask text.
                            // Otherwise the filter and/or mask text is already correct.
                            NMLIFILTERINFO lidi = {0};
                            lidi.hdr.hwndFrom = m_hwnd;
                            lidi.hdr.idFrom = GetWindowLong(m_hwnd, GWL_ID);
                            lidi.hdr.code = LIN_GETFILTERINFO;
                            lidi.li.dwMask = LIM_FILTER & m_dwCallbacks;

                            SendMessage(hwndNotify, WM_NOTIFY, lidi.hdr.idFrom, (LPARAM)&lidi);

                            pszFilter = lidi.li.pszFilter;

                            // REVIEW: we should have a way for the notify hanlder to say "store this
                            // result and stop asking me for the filter to use every time".
                        }

                        for ( LPTSTR psz = pszClone; *psz; psz++ )
                        {
                            // we do the Upper/Lower casing one character at a time because we don't want to
                            // alter pszClone.  pszClone is used later to restore the ClipBoard.
                            if ( LIF_FORCEUPPERCASE & dwFlags )
                            {
                                pszNew[iNew] = (TCHAR)CharUpper((LPTSTR)*psz);  // yes, this funky cast is correct.
                            }
                            else if ( LIF_FORCELOWERCASE & dwFlags )
                            {
                                pszNew[iNew] = (TCHAR)CharLower((LPTSTR)*psz);  // yes, this funky cast is correct.
                            }
                            else
                            {
                                pszNew[iNew] = *psz;
                            }

                            if ( IsValidChar(pszNew[iNew], TRUE) )
                            {
                                iNew++;
                            }
                            else
                            {
                                if ( LIF_NOTIFYONBADCHAR & dwFlags )
                                {
                                    NMLIBADCHAR libc = {0};
                                    libc.hdr.hwndFrom = m_hwnd;
                                    libc.hdr.idFrom = GetWindowLong(m_hwnd, GWL_ID);
                                    libc.hdr.code = LIN_BADCHAR;
                                    libc.wParam = (WPARAM)pszClone[iNew + cchBad];  // use the original, non case shifted chat
                                    libc.lParam = lParam;

                                    SendMessage(hwndNotify, WM_NOTIFY, libc.hdr.idFrom, (LPARAM)&libc);
                                }

                                cchBad++;

                                if ( LIF_PASTECANCEL & dwFlags )
                                {
                                    iNew = 0;
                                    break;
                                }
                                if ( LIF_PASTESTOP & dwFlags )
                                {
                                    break;
                                }
                            }
                        }
                        pszNew[iNew] = NULL;

                        // If there are any characters in the paste buffer then we paste the validated string
                        if ( *pszNew )
                        {
                            // we always set the new string.  Worst case it's identical to the old string
                            GlobalUnlock(hNew);
                            pszNew = NULL;
                            SetClipboardData(iFormat, hNew);
                            hNew = NULL;

                            // call the super proc to do the paste
                            DefSubclassProc(hwnd, WM_PASTE, wParam, lParam);

                            // The above call will have closed the clipboard on us.  We try to re-open it.
                            // If this fails it's no big deal, that simply means the SetClipboardData
                            // call below will fail which is good if somebody else managed to open the
                            // clipboard in the mean time.
                            OpenClipboard(hwnd);

                            // and then we set it back to the original value.
                            GlobalUnlock(hClone);
                            pszClone = NULL;
                            if ( LIF_KEEPCLIPBOARD & dwFlags )
                            {
                                SetClipboardData(iFormat, hClone);
                                hClone = NULL;
                            }
                        }
                    }

                    if ( pszClone )
                    {
                        GlobalUnlock(hClone);
                    }

                    if ( pszNew )
                    {
                        GlobalUnlock(hNew);
                    }
                }

                if ( hClone )
                {
                    GlobalFree( hClone );
                }

                if ( hNew )
                {
                    GlobalFree( hNew );
                }

                // at this point we are done with hdata so unlock it
                GlobalUnlock(hdata);
            }
        }
        CloseClipboard();

        if ( 0 == cchBad )
        {
            // the entire paste was valid
            if ( LIF_HIDETIPONVALID & dwFlags )
            {
                HideToolTip();
            }
        }
        else
        {
            // if we get here then at least one invalid character was pasted
            if ( !(LIF_SILENT & dwFlags) )
            {
                MessageBeep(MB_OK);
            }

            if ( !(LIF_WARNINGOFF & dwFlags) )
            {
                ShowToolTip();
            }
        }
    }
    return TRUE;
}

void CInputLimiter::ShowToolTip()
{
    TraceMsg(TF_GENERAL, "About to show the tooltip");

    if ( !m_hwndToolTip )
    {
        CreateToolTipWindow();
    }

    // Set the tooltip display point
    RECT rc;
    GetWindowRect(m_hwnd, &rc);
    int x, y;
    x = (rc.left+rc.right)/2;
    if ( LIF_WARNINGABOVE & dwFlags )
    {
        y = rc.top;
    }
    else if ( LIF_WARNINGCENTERED & dwFlags )
    {
        y = (rc.top+rc.bottom)/2;
    }
    else
    {
        y = rc.bottom;
    }
    SendMessage(m_hwndToolTip, TTM_TRACKPOSITION, 0, MAKELONG(x,y));

    TOOLINFO ti = {0};
    ti.cbSize = sizeof(ti);
    ti.hwnd = m_hwnd;
    ti.uId = 1;
    if ( (LIM_TITLE|LIM_MESSAGE|LIM_ICON) & m_dwCallbacks )
    {
        // If we have callbacks then we need to update the tooltip text.
        // Otherwise the tooltip text is already correct.
        NMLIDISPINFO lidi = {0};
        lidi.hdr.hwndFrom = m_hwnd;
        lidi.hdr.idFrom = GetWindowLong(m_hwnd, GWL_ID);
        lidi.hdr.code = LIN_GETDISPINFO;
        lidi.li.dwMask = (LIM_TITLE|LIM_MESSAGE|LIM_ICON) & m_dwCallbacks;

        SendMessage(hwndNotify, WM_NOTIFY, lidi.hdr.idFrom, (LPARAM)&lidi);

        // BUGBUG: How do we use the icon, bold title, message style tooltips?
        // Until I learn how I'm just using the message string.

        ti.lpszText = lidi.li.pszMessage;

        SendMessage(m_hwndToolTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
        if ( lidi.li.pszTitle || lidi.li.hIcon )
        {
            SendMessage(m_hwndToolTip, TTM_SETTITLE, (WPARAM)lidi.li.hIcon, (LPARAM)lidi.li.pszTitle);
        }
    }

    // Show the tooltip
    SendMessage(m_hwndToolTip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);

    // Set a timer to hide the tooltip
    if ( m_uTimerID )
    {
        KillTimer(NULL,LIMITINPUTTIMERID);
    }
    m_uTimerID = SetTimer(m_hwnd, LIMITINPUTTIMERID, iTimeout, NULL);
}

// CreateToolTipWindow
//
// Creates our tooltip control.  We share this one tooltip control and use it for all invalid
// input messages.  The control is hiden when not in use and then shown when needed.
//
void CInputLimiter::CreateToolTipWindow()
{
    m_hwndToolTip = CreateWindow(
            TOOLTIPS_CLASS,
            NULL,
            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            m_hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);

    if (m_hwndToolTip)
    {
        SetWindowPos(m_hwndToolTip, HWND_TOPMOST,
                     0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        TOOLINFO ti = {0};
        RECT     rc = {2,2,2,2};

        ti.cbSize = sizeof(ti);
        ti.uFlags = TTF_TRACK | TTF_TRANSPARENT;
        ti.hwnd = m_hwnd;
        ti.uId = 1;
        ti.hinst = hinst;
        // BUGBUG: How do we use the icon, bold title, message style tooltips?
        // Until I learn how I'm just using the message string.
        ti.lpszText = pszMessage;

        // set the version so we can have non buggy mouse event forwarding
        SendMessage(m_hwndToolTip, CCM_SETVERSION, COMCTL32_VERSION, 0);
        SendMessage(m_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
        SendMessage(m_hwndToolTip, TTM_SETMAXTIPWIDTH, 0, cxTipWidth);
        SendMessage(m_hwndToolTip, TTM_SETMARGIN, 0, (LPARAM)&rc);
        if ( pszTitle || hIcon )
        {
            // BUGBUG: hIcon needs to be an image list index or some such.  Get details
            // on how this really works.
            SendMessage(m_hwndToolTip, TTM_SETTITLE, (WPARAM)hIcon, (LPARAM)pszTitle);
        }
    }
    else
    {
        // failed to create tool tip window, now what should we do?  Unsubclass ourselves?
        TraceMsg(TF_GENERAL, "Failed to create tooltip window");
    }
}

void CInputLimiter::HideToolTip()
{
    // When the timer fires we hide the tooltip window
    if ( m_uTimerID )
    {
        KillTimer(m_hwnd,LIMITINPUTTIMERID);
        m_uTimerID = 0;
    }
    if ( m_hwndToolTip )
    {
        SendMessage(m_hwndToolTip, TTM_TRACKACTIVATE, FALSE, 0);
    }
}


// ************************************************************************************************


//
//  LimitInput
//
//      Limits the characters that can be entered into an edit box.  It intercepts WM_CHAR
//    messages and only allows certain characters through.  Some characters, such as backspace
//    are always allowed through.
//
//  Args:
//      hwndEdit        Handle to an edit control.  Results will be unpredictable if any other window
//                      type is passed in.
//
//      pli             Pointer to a LIMITINPUT structure that determines how the input is limited.
//

BOOL SHLimitInputEdit( HWND hwndEdit, LPLIMITINPUT pli )
{
    ASSERT(IS_VALID_READ_BUFFER(pli, LIMITINPUT, 1));

    CInputLimiter * pInputLimiter = new CInputLimiter;

    if (!pInputLimiter)
    {
        return FALSE;
    }

    BOOL bResult = pInputLimiter->SubclassEditControl(hwndEdit, pli);

    if (!bResult)
    {
        delete pInputLimiter;
    }

    return bResult;
}

typedef struct tagCBLIMITINPUT
{
    BOOL            bResult;
    LPLIMITINPUT    pli;
} CBLIMITINPUT, * LPCBLIMITINPUT;

// Limiting the input on a combo box is special cased because you first
// have to find the edit box and then LimitInput on that.
BOOL CALLBACK FindTheEditBox( HWND hwnd, LPARAM lParam )
{
    // The combo box only has one child, subclass it
    LPCBLIMITINPUT pcbli = (LPCBLIMITINPUT)lParam;

    pcbli->bResult = SHLimitInputEdit(hwnd,pcbli->pli);
    return FALSE;
}

BOOL SHLimitInputCombo(HWND hwndComboBox, LPLIMITINPUT pli)
{
    CBLIMITINPUT cbli;
    cbli.bResult = FALSE;
    cbli.pli = pli;

    EnumChildWindows(hwndComboBox, FindTheEditBox, (LPARAM)&cbli);

    return cbli.bResult;
}
