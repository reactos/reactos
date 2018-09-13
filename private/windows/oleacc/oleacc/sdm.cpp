// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  SDM.CPP
//
//  This knows how to talk to SDM, used by Office.
//
// --------------------------------------------------------------------------
//
//  Temporary Hack - BUGBUGBUGBUGBUGBUGBUGBUGBUGBUGBUGBUGBUGBUGBUGBUGBUGBUGBUG
//
//  Because this is eventually going to be a seperate DLL, and because 16 bit
//  SDM is not going to work on NT anyways (see comments at MyMapLS), all code
//  within the 16 bit sections are going to assume that pointers are from
//  regular old win-95 style shared memory, and assignments can just be done
//  with =, rather than using SharedRead and SharedWrite. 
//
//  Summary: This works on Win95 for both 16 and 32 bit SDM
//           This works on WinNT for 32 bit SDM only. IT WILL CRASH WITH 16 bit!!!
//           It can be made not to crash on NT/16bit SDm combination, but
//           cannot be made to work until NT or this module has something like 
//           the MapLS function.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "client.h"
#include "sdm95.h"
#include "sdm.h"

// --------------------------------------------------------------------------
//
//  MyMapLS()
//  This is used to convert a 0:32 type pointer to a 16:16 type pointer. 
//  When 32-bit dudes (like OLEACC) allocate memory, they get back a 0:32
//  type pointer. 16-bit apps (like Word 6 SDM) expect pointers to be in
//  16:16 format. So we have to convert to make them happy.
//
//  One of the things the function does is allocate a new selector for the 
//  first part of the 16:16 address, so when you call MapLS, you should also
//  call UnMapLS to free the selector.
//
//  MapLS is a function in Win95 KERNEL. It does not exist on NT, so the
//  SDM proxy will not work when running 16-bit SDM on NT.
//
// --------------------------------------------------------------------------
LPVOID MyMapLS(LPVOID lp32BitPtr)
{
    if (!lpfnMapLS)
        return(NULL);
    
    return((* lpfnMapLS)(lp32BitPtr));
}

// --------------------------------------------------------------------------
//
//  MyUnMapLS()
//
//  Fress the selector allocated by MapLS.
//
// --------------------------------------------------------------------------
VOID MyUnMapLS(LPVOID lp32BitPtr)
{
    if (lpfnUnMapLS)
        (* lpfnUnMapLS)(lp32BitPtr);
    return;
}



#ifdef UNICODE

// --------------------------------------------------------------------------
//
//  WCharLocalAllocString()
//
//  Similar to TCharSysAllocString() - but this specifically operates on
//  an ANSI string, LocalAlloc'ing a UNICODE copy.
//  Used to convert the ANSI text we get back from SDM to UNICODE.
//
// --------------------------------------------------------------------------
LPWSTR WCharLocalAllocString(LPSTR pszString)
{
    LPWSTR  pszWideString;
    int     cChars;

    // do the call first with 0 to get the size needed
    cChars = MultiByteToWideChar(CP_ACP, 0, pszString, -1, NULL, 0);
    pszWideString = (LPWSTR)LocalAlloc(LPTR,sizeof(WCHAR)*cChars);
    if (pszWideString == NULL)
    {
        return NULL;
    }

	cChars = MultiByteToWideChar(CP_ACP, 0, pszString, -1, pszWideString, cChars);
    return pszWideString;
}

#endif



// --------------------------------------------------------------------------
//
//  CreateSdmClientA()
//
// --------------------------------------------------------------------------
HRESULT CreateSdmClientA(HWND hwnd, long idCurChild, REFIID riid, void** ppvSdm)
{
    CSdm32* psdm;
    HRESULT hr;
    
    InitPv(ppvSdm);
    
    psdm = new CSdm32(hwnd, idCurChild);
    if (!psdm)
        return(E_OUTOFMEMORY);
    
    hr = psdm->QueryInterface(riid, ppvSdm);
    if (!SUCCEEDED(hr))
        delete psdm;
    
    return(hr);
}



// --------------------------------------------------------------------------
//
//  CSdm32::CSdm32()
//
// --------------------------------------------------------------------------
CSdm32::CSdm32(HWND hwnd, long idCurChild)
{
    WINDOWINFO  wi;
    
    Initialize(hwnd, idCurChild);
    
    if (MyGetWindowInfo(hwnd, &wi) && (wi.wCreatorVersion < 0x0400))
    {
        m_f16Bits = TRUE;
        m_cbWctlSize = sizeof(WCTL16);
        m_cbWtxiSize = sizeof(WTXI16);
    }
    else
    {
        m_f16Bits = FALSE;
        m_cbWctlSize = sizeof(WCTL32);
        m_cbWtxiSize = sizeof(WTXI32);
    }
}



// --------------------------------------------------------------------------
//
//  CSdm32::SetupChildren()
//
// --------------------------------------------------------------------------
void CSdm32::SetupChildren(void)
{
    long    cbTotal;
    
    // This is a huge hack, but the only way to do this I know of.  We ask
    // for the total size of structs needed for all the children.  We then
    // divide by the size of one control structure.  This results in the 
    // number of children!
    
    cbTotal = SendMessageINT(m_hwnd, WM_GETCOUNT, wVerWord, 0);
    m_cChildren = (cbTotal / m_cbWctlSize);
    
}


// --------------------------------------------------------------------------
// This takes an SDM id and returns the child ID (1..n) of that control.
//
//	Parameters:
//		lSdmID		This is SDM's ID for the child (wct.wId)
//	Returns:
//		long indicating the child ID (1..n) as used by IAccessible interfaces.
//		0 indicates failure.
//
// --------------------------------------------------------------------------
long CSdm32::GetSdmChildIdFromSdmId (long lSdmId)
{
    LPWCTL32    rgwc32Shared;
    LPWCTL16    lpwT;
    LPVOID      lpv;
    long        cbAlloc;
    BOOL        fReturn;
    long		i = 0;
    HANDLE      hProcess;
    WORD        wIdLocal;
    
    SetupChildren();
    
    cbAlloc = m_cChildren * m_cbWctlSize;
    rgwc32Shared = (LPWCTL32)SharedAlloc(cbAlloc,m_hwnd,&hProcess);
    if (!rgwc32Shared)
        return(0);
    
    fReturn = FALSE;
    
    // Map the pointer to 16-bits if need be.
    if (m_f16Bits)
    {
        lpv = MyMapLS(rgwc32Shared);
        if (!lpv)
            goto MemFailure;
    }
    else
        lpv = rgwc32Shared;
    
    //
    // Make sure return value is as much as we asked for.  
    // HACK!  We fill in cbAlloc into the first field of the array.
    //
    //rgwc32Shared->wtp = (WORD)cbAlloc;
    SharedWrite (&cbAlloc,&rgwc32Shared->wtp,sizeof(WORD),hProcess);
    if (SendMessage(m_hwnd, WM_GETCONTROLS, wVerWord, (LPARAM)lpv) == m_cChildren)
    {
        if (m_f16Bits)
        {
            // since we know we are in 16 bit land, and 16 bit only
            // works on Win95 for now, just do regular memory access, 
            // instead of SharedRead. If we can figure out a way to 
            // get MapLS to work on NT, this is a BUGBUG!
            for (i = 0; i < m_cChildren;i++)
            {
                // Translate structure to 32-bit one
                lpwT = &((LPWCTL16)rgwc32Shared)[i];
                if (lpwT->wId == lSdmId)
                {
                    fReturn = TRUE;
                    break;
                }
            }
        }
        else    // 32 bits
        {
            for (i = 0; i < m_cChildren;i++)
            {
                SharedRead (&rgwc32Shared[i].wId,&wIdLocal,sizeof(WORD),hProcess);
                if (wIdLocal == lSdmId)
                {
                    fReturn = TRUE;
                    break;
                }
            }
        }
    }
    
    if (m_f16Bits)
        MyUnMapLS(lpv);
    
MemFailure:
    SharedFree(rgwc32Shared,hProcess);
    
    return(fReturn ? i : 0);
    
}

// --------------------------------------------------------------------------
//
//  CSdm32::GetSdmControl()
//
//  SDM's interface is weird, not sure why the Office guys are so keen
//  on it.  We have to allocate a shared ARRAY up to and including the 
//  item we want, then call GetControls to fill in the WCTL data for 'em all.
//  Then copy over only the one we want.
//
//  NOTE:  The "index" is actually index+1 so all the callers don't have to
//  pass in varChild.lVal-1.
//
//	Parameters:
//		lVal			This is the ChildID of the control (1..n)
//		lpwc			Pointer to the WCTL control you want filled in
//		plFirstRadio	A pointer to an integer that gets filled in with
//						the ChildID of the first radio button if this
//						child is a radio button. Can be NULL if you don't care.
//
// --------------------------------------------------------------------------
BOOL CSdm32::GetSdmControl(long lVal, LPWCTL32 lpwc, long* plFirstRadio)
{
    LPWCTL32    rgwc32Shared;
    LPWCTL16    lpwT;
    LPVOID      lpv;
    long        cbAlloc;
    BOOL        fReturn;
    HANDLE      hProcess;
    WORD        wtpLocal;
    
    --lVal;
    
    if (plFirstRadio)
        *plFirstRadio = 0;
    
    // Figure out how many bytes we need to alloc for the entire list of controls
    cbAlloc = m_cChildren * m_cbWctlSize;
    
    // Allocate the memory we going to need
    rgwc32Shared = (LPWCTL32)SharedAlloc(cbAlloc,m_hwnd,&hProcess);
    if (!rgwc32Shared)
        return(FALSE);
    
    // Set our return value to false in case we need to get out without wrapping up
    fReturn = FALSE;
    
    // Map the pointer to 16-bits if need be.
    if (m_f16Bits)
    {
        // lpv is generic pointer to rgwc32Shared and is passed to SDM to fill in.
        lpv = MyMapLS(rgwc32Shared);
        if (!lpv)
            goto MemFailure;
    }
    else
        lpv = rgwc32Shared;
    
    //
    // Make sure return value is as much as we asked for.  
    // HACK!  We fill in cbAlloc into the first field of the array.
    //
    //rgwc32Shared->wtp = (WORD)cbAlloc;
    SharedWrite (&cbAlloc,&rgwc32Shared->wtp,sizeof(WORD),hProcess);
    if (SendMessage(m_hwnd, WM_GETCONTROLS, wVerWord, (LPARAM)lpv) == m_cChildren)
    {
        fReturn = TRUE;
        
        if (m_f16Bits)
        {
            // Translate structure to 32-bit one
            
            // since we know we are in 16 bit land, and 16 bit only
            // works on Win95 for now, just do regular memory access, 
            // instead of SharedRead. If we can figure out a way to 
            // get MapLS to work on NT, this is a BUGBUG!
            lpwT = &((LPWCTL16)rgwc32Shared)[lVal];
            
            lpwc->wtp = lpwT->wtp;
            lpwc->wId = lpwT->wId;
            lpwc->wState = lpwT->wState;
            lpwc->cchText = lpwT->cchText;
            lpwc->cchTitle = lpwT->cchTitle;
            
            lpwc->rect.left = lpwT->left;
            lpwc->rect.top = lpwT->top;
            lpwc->rect.right = lpwT->right;
            lpwc->rect.bottom = lpwT->bottom;
            
            lpwc->fHasState = lpwT->fHasState;
            lpwc->fHasText = lpwT->fHasText;
            lpwc->fHasTitle = lpwT->fHasTitle;
            lpwc->fEnabled = lpwT->fEnabled;
            lpwc->fVisible = lpwT->fVisible;
            lpwc->fCombo = lpwT->fCombo;
            lpwc->fSpin = lpwT->fSpin;
            lpwc->fOwnerDraw = lpwT->fOwnerDraw;
            lpwc->fCanFocus = lpwT->fCanFocus;
            lpwc->fHasFocus = lpwT->fHasFocus;
            lpwc->fList = lpwT->fList;
            lpwc->lReserved = lpwT->lReserved;
            
            lpwc->wParam1 = lpwc->wParam2 = lpwc->wParam3 = 0;
        }
        else    // 32 bitness
            SharedRead (&rgwc32Shared[lVal],lpwc,sizeof(WCTL32),hProcess);
        
        if (plFirstRadio && (lpwc->wtp == wtpRadioButton))
        {
            // Get the ID of the first radio button
            *plFirstRadio = lVal+1;
            
            while (lVal > 0)
            {
                --lVal;
                if (m_f16Bits)
                {
                    // since we know we are in 16 bit land, and 16 bit only
                    // works on Win95 for now, just do regular memory access, 
                    // instead of SharedRead. If we can figure out a way to 
                    // get MapLS to work on NT, this is a BUGBUG!
                    if (((LPWCTL16)rgwc32Shared)[lVal].wtp == wtpRadioButton)
                        *plFirstRadio = lVal+1;
                    else
                        break;
                }
                else
                {
                    SharedRead (&rgwc32Shared[lVal].wtp,&wtpLocal,
                        sizeof(WORD),hProcess);
                    if (wtpLocal == wtpRadioButton)
                        *plFirstRadio = lVal+1;
                    else
                        break;
                }
            }
            
        }
        
    }
    
    if (m_f16Bits)
        MyUnMapLS(lpv);
    
MemFailure:
    SharedFree(rgwc32Shared,hProcess);
    
    return(fReturn);
}





// --------------------------------------------------------------------------
//
//  CSdm32::GetSdmControlName()
//
//  This is used by both get_accName() and get_accKeyboardShortcut()
//
// --------------------------------------------------------------------------
BOOL CSdm32::GetSdmControlName(long lChild, LPTSTR * ppszName)
{
    WCTL32  wc;
    LPWTXI32 lpwtxShared;
    LPVOID  lpvTx;
    LPSTR   lpszTitle; // ANSI, not TCHAR
    LPVOID  lpvSz;
    int     wmTitle;
    int     nSomeInt;
    HANDLE  hProcess1;
    HANDLE  hProcess2;
    LPSTR   lpszLocal; // ANSI, not TCHAR
	BOOL	bRetVal = FALSE;
    
    InitPv(ppszName);
    
    //
    // First, get the wID and title info of the control.
    //
    if (!GetSdmControl(lChild, &wc, NULL))
        return(FALSE);
    
    wmTitle = WM_GETCTLTITLE;
    
    switch (wc.wtp)
    {
    case wtpGeneralPicture:
        // This is a list of page tabs
        if (wc.fList)
            goto HasATitle;
        
        // FALL THRU
        
    default:
        // Use text if there is any, otherwise use title.
        if (wc.fHasText)
        {
            wc.cchTitle = wc.cchText;
            wmTitle = WM_GETCTLTEXT;
            break;
        }
        // FALL THRU
        
    case wtpEdit:
    case wtpListBox:
    case wtpDropList:
    case wtpScroll:
HasATitle:
        // Check title only.
        if (!wc.fHasTitle)
            wc.cchTitle = 0;
        break;
    } // end switch
    
    // At this point, we have either set wmTitle to the message we want to send and
    // wc.cchTitle has the # of characters, or else there is no name for the thing
    // and wc.cchTitle is 0.
    
    if (wc.cchTitle == 0)
    {
        // check the one before this - if it is static text, use that.
        if (--lChild == 0)
            return(FALSE);
        //lChild--;
        
        if (!GetSdmControl(lChild, &wc, NULL))
            return(FALSE);
        if (wc.wtp == wtpStaticText || wc.wtp == wtpFormattedText)
        {
            // Use text if there is any, otherwise use title.
            if (wc.fHasText)
            {
                wc.cchTitle = wc.cchText;
                wmTitle = WM_GETCTLTEXT;
            }
        }
        else	// the thing before this control is not static text. Bail!
            return(FALSE);
    }
    
    // One last test - if wm.cchTitle is STILL 0, just bail out
    if (wc.cchTitle == 0)
        return (FALSE);
    
    // 
    // cchTitle includes the NULL terminator.
    //

	// BOGUS HACK!
	// SDM doesn't return the correct len for DBCS (eg. Japanese) - seems
	// to return Num_of_printable_chars+1, instead of number of bytes needed...
	// We get around this by using twice the amount of space suggested by
	// SDM, so our buffer is big enough for SDM to write the whole string,
	// instead of a truncated version. Since the buffer is just a temp
	// buf (freed soon after), we're not actually wasting space here.
	// (paranoia check to make sure we don't overflow. If an SDM control
	// ever gives us back text more than 0x4000 len, there's probably something
	// else wrong somewhere... We could probably compare against 0x8000,
	// but I'm not sure I'd trust SDM with the resulting looks-like-a-negative-int
	// result - so I'll stick to 0x4000.)
	// (we're assuming cchTitle is a WORD...)
	Assert( sizeof( wc.cchTitle ) == sizeof( WORD ) );
	Assert( wc.cchTitle < 0x4000 );
	if( wc.cchTitle < 0x4000 )
	    wc.cchTitle *= 2;
    
    //
    // Now, allocate a WTXI structure and use the ID to get the title.
    //
    lpwtxShared = (LPWTXI32)SharedAlloc(m_cbWtxiSize,m_hwnd,&hProcess1);
    if (!lpwtxShared)
        return(FALSE);
    
    // Map the WTXI to 16-bits if needed.
    if (m_f16Bits)
    {
        lpvTx = MyMapLS(lpwtxShared);
        if (!lpvTx)
            goto End;
    }
    else
        lpvTx = lpwtxShared;
    
    lpszTitle = (LPSTR)SharedAlloc((wc.cchTitle)*sizeof(CHAR),m_hwnd,&hProcess2);
    if (!lpszTitle)
        goto End;
    
    //*lpszTitle = 0;
    nSomeInt = 0;
    SharedWrite (&nSomeInt,lpszTitle,sizeof(CHAR),hProcess2);
    
    //
    // Get a 16-bit pointer if this is to a 16-bit control.
    //
    if (m_f16Bits)
    {
        // since we know we are in 16 bit land, and 16 bit only
        // works on Win95 for now, just do regular memory access, 
        // instead of SharedRead. If we can figure out a way to 
        // get MapLS to work on NT, this is a BUGBUG!
        lpvSz = MyMapLS(lpszTitle);
        if (!lpvSz)
            goto TextEnd;
		// We'll never get here under NT/UNICODE, since our MyMapLS returns NULL
        
        ((LPWTXI16)lpwtxShared)->lpszBuffer = (LPSTR)lpvSz;
        ((LPWTXI16)lpwtxShared)->cch = wc.cchTitle;
        ((LPWTXI16)lpwtxShared)->wId = wc.wId;
        ((LPWTXI16)lpwtxShared)->wIndex = 0;
    }
    else
    {
        lpvSz = lpszTitle;
        
        //lpwtxShared->lpszBuffer = lpszTitle;
        SharedWrite (&lpszTitle,&lpwtxShared->lpszBuffer,sizeof(LPSTR),hProcess1);
        //lpwtxShared->cch = wc.cchTitle;
        SharedWrite (&wc.cchTitle,&lpwtxShared->cch,sizeof(int),hProcess1);
        //lpwtxShared->wId = wc.wId;
        SharedWrite (&wc.wId,&lpwtxShared->wId,sizeof(WORD),hProcess1);
        //lpwtxShared->wIndex = 0;
        nSomeInt = 0;
        SharedWrite (&nSomeInt,&lpwtxShared->wIndex,sizeof(WORD),hProcess1);
    }
    
    SendMessage(m_hwnd, wmTitle, wVerWord, (LPARAM)lpvTx);
    
    // UnMap pointers
    if (m_f16Bits)
    {
        MyUnMapLS(lpvSz);
        MyUnMapLS(lpvTx);
    }
    
    lpszLocal = (LPSTR)LocalAlloc (LPTR,(wc.cchTitle)*sizeof(CHAR));
    if (!lpszLocal)
        goto TextEnd;
    
    SharedRead (lpszTitle,lpszLocal,(wc.cchTitle)*sizeof(CHAR),hProcess2);
    

	if (*lpszLocal)
	{
		// Ensure that the string is NUL-terminated
		// (It's possible that SDM may not have added a nul - it shouldn't
		// omit it, but, well, sometimes SDM does things it shouldn't....)
		lpszLocal[ wc.cchTitle - 1 ] = '\0';
#ifdef UNICODE
        *ppszName = WCharLocalAllocString(lpszLocal);
        LocalFree(lpszLocal);
#else
        *ppszName = lpszLocal;
#endif
		bRetVal = TRUE;
	}
    else
	{
		// empty string - won't be using it - so free it and return FALSE.
		LocalFree( lpszLocal );
		bRetVal = FALSE;
	}

TextEnd:
    SharedFree(lpszTitle,hProcess2);
    
End:
    SharedFree(lpwtxShared,hProcess1);
    
    return bRetVal;
}


// --------------------------------------------------------------------------
//
//  CSdm32::get_accChild()
//
//  For fList controls, we create a real object so that the list items can
//  be accessed.
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdm32::get_accChild(VARIANT varChild, IDispatch** ppdispChild)
{
    WCTL32 wctl;
    
    InitPv(ppdispChild);
    
    if (!ValidateChild(&varChild) || !varChild.lVal)
        return(E_INVALIDARG);
    
    if (!GetSdmControl(varChild.lVal, &wctl, NULL) || !wctl.fList)
        return(S_FALSE);
    
    return(CreateSdmList(this, m_hwnd, wctl.wId, varChild.lVal, m_f16Bits,
        (wctl.wtp == wtpGeneralPicture), m_cbWtxiSize, 0, NULL, IID_IDispatch, 
        (void**)ppdispChild));
}



// --------------------------------------------------------------------------
//
//  CSdm32::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdm32::get_accName(VARIANT varChild, BSTR* pszName)
{
    LPTSTR  lpszTitle;
    
    //
    // The only way to get anything is to send WM_GETCONTROLS to get up to
    // and including the one we want.  Then we use the ID to get stuff.
    //
    InitPv(pszName);
    
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);
    
    if (!varChild.lVal)
        return(CClient::get_accName(varChild, pszName));
    
    //
    // This will allocate a string from the local heap
    //
    if (!GetSdmControlName(varChild.lVal, &lpszTitle))
        return(S_FALSE);
    
    StripMnemonic(lpszTitle);
    *pszName = TCharSysAllocString(lpszTitle);
    
    //
    // Free the allocated string now.
    //
    LocalFree(lpszTitle);
    
    return(*pszName ? S_OK : S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CSdm32::get_accKeyboardShortcut()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdm32::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszShortcut)
{
    LPTSTR  lpszShortcut;
    TCHAR   chMnemonic;
    
    InitPv(pszShortcut);
    
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);
    
    if (!varChild.lVal)
        return(CClient::get_accKeyboardShortcut(varChild, pszShortcut));
    
    //
    // This will allocate a string from the local heap
    //
    if (!GetSdmControlName(varChild.lVal, &lpszShortcut))
        return(S_FALSE);
    
    //
    // Get the mnemonic for it.
    //
    chMnemonic = StripMnemonic(lpszShortcut);
    
    //
    // Free the string.
    //
    LocalFree(lpszShortcut);
    
    //
    // Is there a shortcut?
    //
    if (chMnemonic)
    {
        TCHAR   szKey[2];
        
        *szKey = chMnemonic;
        *(szKey+1) = 0;
        
        return(HrMakeShortcut(szKey, pszShortcut));
    }
    else
        return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CSdm32::get_accValue()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdm32::get_accValue(VARIANT varChild, BSTR* pszValue)
{
    WCTL32      wc;
    LPWTXI32    lpwtxShared;
    LPVOID      lpvTx;
    LPSTR       lpszValueShared; // ANSI, not TCHAR
    HANDLE      hProcess;
    LPSTR       lpszValueLocal; // ANSI, not TCHAR
    WORD        wSomeWord;
    
    InitPv(pszValue);
    
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);
    
    if (!varChild.lVal)
        return(E_NOT_APPLICABLE);
    
    //
    // First, get the wID and value info of the control.
    //
    if (!GetSdmControl(varChild.lVal, &wc, NULL))
        return(S_FALSE);
    
    //
    // Is this a "value-like" dude?
    // Things with values are edits, listboxes, drop lists, and
    // tab controls (general picture with fList=TRUE).
    //
    switch (wc.wtp)
    {
    case wtpGeneralPicture:
        if (wc.fList)
        {
            // Include the '&' character.
            if (wc.cchText)
                wc.cchText++;
            break;
        }
        // FALL THROUGH
        
    default:
        return(E_NOT_APPLICABLE);
        
    case wtpEdit:
    case wtpListBox:
    case wtpDropList:
        break;
    }
    
    if (!wc.cchText)
        return(S_FALSE);
    
    //
    // cchText includes the NULL terminator.
    //
    
    //
    // Second, allocate a WTXI structure using the wID to get the text.
    //
    lpwtxShared = (LPWTXI32)SharedAlloc(m_cbWtxiSize + ((wc.cchText+1)*sizeof(TCHAR)),
        m_hwnd,&hProcess);
    if (!lpwtxShared)
        return(E_OUTOFMEMORY);
    
    lpszValueLocal = (LPSTR)LocalAlloc (LPTR,(wc.cchText+1)*sizeof(CHAR));
    if (!lpszValueLocal)
    {
        SharedFree (lpwtxShared,hProcess);
        return (E_OUTOFMEMORY);
    }

    // Map to 16-bits if we need to
    if (m_f16Bits)
    {
        lpvTx = MyMapLS(lpwtxShared);
        if (!lpvTx)
            goto End;
    }
    else
        lpvTx = lpwtxShared;
    
    lpszValueShared = (LPSTR)((LPBYTE)lpvTx + m_cbWtxiSize);
    
    // Hack!  Since we know we got back selector:0 for map, we can safely
    // add offset to get 16:16 address of string.
    if (m_f16Bits)
    {
		// We'll never get here under NT/UNICODE, since our MyMapLS (called above) returns NULL

        // since we know we are in 16 bit land, and 16 bit only
        // works on Win95 for now, just do regular memory access, 
        // instead of SharedRead. If we can figure out a way to 
        // get MapLS to work on NT, this is a BUGBUG!
        ((LPWTXI16)lpwtxShared)->lpszBuffer = (LPSTR) lpszValueShared;
        ((LPWTXI16)lpwtxShared)->cch = wc.cchText;
        ((LPWTXI16)lpwtxShared)->wId = wc.wId;
        ((LPWTXI16)lpwtxShared)->wIndex = static_cast<WORD>(wc.fHasState ? wc.wState : 0);
    }
    else
    {
        //lpwtxShared->lpszBuffer = lpszValueShared;
        SharedWrite(&lpszValueShared,&lpwtxShared->lpszBuffer,sizeof(LPSTR),hProcess);
        //lpwtxShared->cch = wc.cchText;
        SharedWrite(&wc.cchText,&lpwtxShared->cch,sizeof(int),hProcess);
        //lpwtxShared->wId = wc.wId;
        SharedWrite(&wc.wId,&lpwtxShared->wId,sizeof(WORD),hProcess);
        //lpwtxShared->wIndex = (wc.fHasState ? wc.wState : 0);
        if (wc.fHasState)
            wSomeWord = wc.wState;
        else
            wSomeWord = 0;
        SharedWrite (&wSomeWord,&lpwtxShared->wIndex,sizeof(WORD),hProcess);
    }
    
    SendMessage(m_hwnd, WM_GETCTLTEXT, wVerWord, (LPARAM)lpvTx);
    
    // UnMap pointers
    if (m_f16Bits)
    {
        // since we know we are in 16 bit land, and 16 bit only
        // works on Win95 for now, just do regular memory access, 
        // instead of SharedRead. If we can figure out a way to 
        // get MapLS to work on NT, this is a BUGBUG!
        // Recreate lpszValueShared as a 32-bit pointer
        lpszValueShared = (LPSTR)((LPBYTE)lpwtxShared + m_cbWtxiSize);
        MyUnMapLS(lpvTx);
    }
    
    // Was any text returned?
    SharedRead (lpszValueShared,lpszValueLocal,(wc.cchText+1)*sizeof(CHAR),hProcess);

    if (*lpszValueLocal)
    {
#ifdef UNICODE
        LPTSTR pszString = WCharLocalAllocString(lpszValueLocal);
        if (wc.wtp == wtpGeneralPicture)
            StripMnemonic(pszString);
        *pszValue = TCharSysAllocString(pszString);
        LocalFree(pszString);
#else
        if (wc.wtp == wtpGeneralPicture)
            StripMnemonic(lpszValueLocal);
        *pszValue = TCharSysAllocString(lpszValueLocal);
#endif
    }
    
End:
    SharedFree(lpwtxShared,hProcess);
    LocalFree (lpszValueLocal);
    
    return(*pszValue ? S_OK : S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CSdm32::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdm32::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    WCTL32  wc;
    
    InitPvar(pvarRole);
    
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);
    
    pvarRole->vt = VT_I4;
    
    if (!varChild.lVal)
    {
        if (GetWindowLong(m_hwnd, GWL_STYLE) & WS_CHILD)
            pvarRole->lVal = ROLE_SYSTEM_PROPERTYPAGE;
        else
            pvarRole->lVal = ROLE_SYSTEM_DIALOG;
    }
    else
    {
        if (!GetSdmControl(varChild.lVal, &wc, NULL))
        {
            pvarRole->vt = VT_EMPTY;
            return(S_FALSE);
        }
        
        switch (wc.wtp)
        {
        case wtpStaticText:
        case wtpFormattedText:
            pvarRole->lVal = ROLE_SYSTEM_STATICTEXT;
            break;
            
        case wtpPushButton:
            pvarRole->lVal = ROLE_SYSTEM_PUSHBUTTON;
            break;
            
        case wtpCheckBox:
            pvarRole->lVal = ROLE_SYSTEM_CHECKBUTTON;
            break;
            
        case wtpRadioButton:
            pvarRole->lVal = ROLE_SYSTEM_RADIOBUTTON;
            break;
            
        case wtpGroupBox:
            pvarRole->lVal = ROLE_SYSTEM_GROUPING;
            break;
            
        case wtpEdit:
            if (wc.fSpin)
                pvarRole->lVal = ROLE_SYSTEM_SPINBUTTON;
            else
                pvarRole->lVal = ROLE_SYSTEM_TEXT;
            break;
            
        case wtpListBox:
            pvarRole->lVal = ROLE_SYSTEM_LIST;
            break;
            
        case wtpDropList:
            pvarRole->lVal = ROLE_SYSTEM_COMBOBOX;
            break;
            
        case wtpGeneralPicture:
            if (wc.fList)
            {
                // This is a list of page tabs
                pvarRole->lVal = ROLE_SYSTEM_PAGETABLIST;
                break;
            }
            //
            // FALL THROUGH
            // Note:  A lot of GeneralPicture dudes are property pages,
            // but some are just graphics.  Like the Modify Selection
            // button in the Customize Commands property page.
            //
            
        case wtpBitmap:
        default:
            pvarRole->lVal = ROLE_SYSTEM_GRAPHIC;
            break;
            
        case wtpScroll:
            pvarRole->lVal = ROLE_SYSTEM_SCROLLBAR;
            break;
        }
    }
    
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CSdm32::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdm32::get_accState(VARIANT varChild, VARIANT* pvarState)
{
    WCTL32  wc;
    long    lFirstRadio;
    
    InitPvar(pvarState);
    
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);
    
    if (!varChild.lVal)
        return(CClient::get_accState(varChild, pvarState));
    
    if (!GetSdmControl(varChild.lVal, &wc, &lFirstRadio))
        return(S_FALSE);
    
    pvarState->vt = VT_I4;
    pvarState->lVal = 0;
    
    if (!wc.fEnabled)
        pvarState->lVal |= STATE_SYSTEM_UNAVAILABLE;
    if (!wc.fVisible)
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
    
    if (wc.fHasFocus && (MyGetFocus() == m_hwnd))
        pvarState->lVal |= STATE_SYSTEM_FOCUSED;
    if (wc.fCanFocus && (GetForegroundWindow() == MyGetAncestor(m_hwnd, GA_ROOT)))
        pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;
    
    if (wc.fHasState)
    {
        if (wc.wtp == wtpCheckBox)
        {
            // The state for a check button is the "checked" value.
            if (wc.wState)
                pvarState->lVal |= STATE_SYSTEM_CHECKED;
        }
        else if ((wc.wtp == wtpRadioButton) && (lFirstRadio))
        {
            // The state for a radio button is the offset of the one in the
            // group that is checked.
            //
            // The index of the first radio button in the group + the index ==
            // the index of the checked button.
            //
            if (lFirstRadio + wc.wState == varChild.lVal)
                pvarState->lVal |= STATE_SYSTEM_CHECKED;
        }
        
    }
    
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CSdm32::get_accFocus()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdm32::get_accFocus(VARIANT* pvarFocus)
{
    long    wID;
    long    cbSize;
    int     iControl;
    LPWCTL32  lpwcShared;
    LPVOID  lpvC;
    WCTL32  wcLocal;
    HANDLE  hProcess;
    
    InitPvar(pvarFocus);
    
    if (MyGetFocus() != m_hwnd)
        return(S_FALSE);
    
    pvarFocus->vt = VT_I4;
    pvarFocus->lVal = 0;
    
    // Make sure we know how many children we got.
    SetupChildren();
    
    // BOGUS!  We need to return an index-based object.  But we will get
    // back a wID from WM_GETCTLFOCUS.  So we need to ask for all the 
    // control data, and loop through looking for a wID match.
    
    wID = SendMessageINT(m_hwnd, WM_GETCTLFOCUS, wVerWord, 0);
    
    cbSize = m_cChildren * m_cbWctlSize;
    if (!cbSize)
        return(S_OK);
    
    lpwcShared = (LPWCTL32)SharedAlloc(cbSize,m_hwnd,&hProcess);
    if (!lpwcShared)
        return(E_OUTOFMEMORY);
    
    // Map pointer if 16-bits
    if (m_f16Bits)
    {
        lpvC = MyMapLS(lpwcShared);
        if (!lpvC)
            goto End;
        
        ((LPWCTL16)lpwcShared)->wtp = (WORD)cbSize;
    }
    else
    {
        lpvC = lpwcShared;
        //lpwcShared->wtp = (WORD)cbSize;
        SharedWrite (&cbSize,&lpwcShared->wtp,sizeof(WORD),hProcess);
    }
    
    if (SendMessage(m_hwnd, WM_GETCONTROLS, wVerWord, (LPARAM)lpvC)
        == m_cChildren)
    {
        WORD    wIdT;
        IDispatch* pdispFocus = NULL;
        
        for (iControl = 0; iControl < m_cChildren; iControl++)
        {
            if (m_f16Bits)
            {
                // since we know we are in 16 bit land, and 16 bit only
                // works on Win95 for now, just do regular memory access, 
                // instead of SharedRead. If we can figure out a way to 
                // get MapLS to work on NT, this is a BUGBUG!
                wIdT = ((LPWCTL16)lpwcShared)[iControl].wId;
                
                if (wIdT == wID)
                {
                    if (((LPWCTL16)lpwcShared)[iControl].fList)
                    {
                        CreateSdmList(this, m_hwnd, wID, iControl+1, TRUE,
                            (((LPWCTL16)lpwcShared)[iControl].wtp == wtpGeneralPicture),
                            m_cbWtxiSize, 0, NULL, IID_IDispatch, (void**)&pdispFocus);
                    }
                    
                    if (pdispFocus)
                    {
                        pvarFocus->vt = VT_DISPATCH;
                        pvarFocus->pdispVal = pdispFocus;
                    }
                    else
                        pvarFocus->lVal = iControl+1;
                    
                    break;
                }
            }
            else // 32 bits
            {
                SharedRead (&lpwcShared[iControl],&wcLocal,sizeof(WCTL32),hProcess);
                
                if (wcLocal.wId == wID)
                {
                    if (wcLocal.fList)
                    {
                        CreateSdmList(this, m_hwnd, wID, iControl+1, FALSE,
                            (wcLocal.wtp == wtpGeneralPicture),
                            m_cbWtxiSize, 0, NULL, IID_IDispatch, (void**)&pdispFocus);
                    }
                    
                    if (pdispFocus)
                    {
                        pvarFocus->vt = VT_DISPATCH;
                        pvarFocus->pdispVal = pdispFocus;
                    }
                    else
                        pvarFocus->lVal = iControl+1;
                    
                    break;
                }
            }
        }
    }
    
    // UnMap 16-bit pointer
    if (m_f16Bits)
        MyUnMapLS(lpvC);
    
End:
    SharedFree(lpwcShared,hProcess);
    
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CSdm32::accSelect()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdm32::accSelect(long lSelFlags, VARIANT varChild)
{
    WCTL32 wct;
    
    if (!ValidateChild(&varChild) || !ValidateSelFlags(lSelFlags))
        return(E_INVALIDARG);
    
    if (!varChild.lVal)
        return(CClient::accSelect(lSelFlags, varChild));
    
    // We only support changing the focus.
    if (lSelFlags != SELFLAG_TAKEFOCUS)
        return(E_NOT_APPLICABLE);
    
    // Get the wID of this item.
    if (!GetSdmControl(varChild.lVal, &wct, NULL))
        return(S_FALSE);
    
    // Change the focus
    SendMessage(m_hwnd, WM_SETCTLFOCUS, wVerWord, wct.wId);
    
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CSdm32::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdm32::accLocation(long* pxLeft, long* pyTop,
                                 long* pcxWidth, long* pcyHeight, VARIANT varChild)
{
    WCTL32 wct;
    
    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);
    
    if (!ValidateChild(&varChild))
        return(S_FALSE);
    
    if (!varChild.lVal)
        return(CClient::accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild));
    
    //
    // Get the item's rect, in client coords.
    //
    if (!GetSdmControl(varChild.lVal, &wct, NULL))
        return(S_FALSE);
    
    MapWindowPoints(m_hwnd, NULL, (LPPOINT)&wct.rect, 2);
    
    *pxLeft = wct.rect.left;
    *pyTop = wct.rect.top;
    *pcxWidth = wct.rect.right - wct.rect.left;
    *pcyHeight = wct.rect.bottom - wct.rect.top;
    
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CSdm32::accNavigate()
//
//  It's way too complicated to to up/left/right/down navigation.  We have
//  to use rects, just like with window positional stuff.  Once we have that
//  working well, we can maybe adapt it for this stuff too.
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdm32::accNavigate(long dwNavDir, VARIANT varStart, VARIANT* pvarEnd)
{
    long    lEnd;
    
    InitPvar(pvarEnd);
    
    if (!ValidateChild(&varStart) || !ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);
    
    if (dwNavDir == NAVDIR_FIRSTCHILD)
        dwNavDir = NAVDIR_NEXT;
    else if (dwNavDir == NAVDIR_LASTCHILD)
    {
        varStart.lVal = m_cChildren + 1;
        dwNavDir = NAVDIR_PREVIOUS;
    }
    else if (!varStart.lVal)
        return(CClient::accNavigate(dwNavDir, varStart, pvarEnd));
    
    switch (dwNavDir)
    {
    case NAVDIR_NEXT:
        lEnd = varStart.lVal + 1;
        if (lEnd > m_cChildren)
            lEnd = 0;
        break;
        
    case NAVDIR_PREVIOUS:
        lEnd = varStart.lVal - 1;
        break;
        
    default:
        lEnd = 0;
    }
    
    if (lEnd)
    {
        IDispatch*  pdispChild;
        VARIANT     varChild;
        
        pdispChild = NULL;
        
        VariantInit(&varChild);
        varChild.vt = VT_I4;
        varChild.lVal = lEnd;
        
        //
        // If the child is an object, return it directly.
        //
        get_accChild(varChild, &pdispChild);
        
        if (pdispChild)
        {
            pvarEnd->vt = VT_DISPATCH;
            pvarEnd->pdispVal = pdispChild;
        }
        else
        {
            pvarEnd->vt = VT_I4;
            pvarEnd->lVal = lEnd;
        }
        
        return(S_OK);
    }
    else
        return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CSdm32::accHitTest()
//
//  BOGUS!  We have no z-order info.  Best we can do is PtInRect() goo,
//  and try same thing for fake group boxes.
//
//	Params: x and y are the point to hit test. pvarhit is a pointer to 
//	a variant that gets filled in
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdm32::accHitTest(long x, long y, VARIANT* pvarHit)
{
    POINT			pt;
    long			cbSize;
    long			iControl;
    long			iControlMaybe;
    long			lChildId;
    LPWCTL32		rgwctShared;
    LPVOID			lpv;
    RECT			rc;
    HWND			hWndMaybe = NULL;	// Used to check real window at that point.
    IAccessible*	paccParent = NULL;
    IDispatch*		pdispChild = NULL;
    HANDLE          hProcess;
    WCTL32          wctLocal;
    
    InitPvar(pvarHit);
    
    //
    // NOTE:  In this case, since SDM wraps most real child windows in its
    // dialogs also, we don't defer to CClient::accHitTest() first.  That
    // will check right off the bat if the point is in a child window, and
    // if so, return a wrapper object for it.  We instead want to give SDM
    // a chance to handle it first.
    //
    // BUT we need to check if the point is actually in our client area
    // in the first place to avoid returning some object out of the client
    // range.
    //
    pt.x = x;
    pt.y = y;
    
    // Let's check if there is another window at this point.
    // If there is, it is either a property sheet, some other kind of
    // "real" window (RichEdit, toolbar, etc.).
    hWndMaybe = WindowFromPoint (pt);
    // if the window under the point is not the same as the window we were asked to check,
    // we should check what kind of window that thing is, and set the returned child VARIANT
    // so that when we are asked for the name/role/etc. of the thing, we can find it.
    //
    // We need to look at the window class of hwndMaybe to see if it is a property sheet
    // type thing. If so, we can just munge the m_hwnd to be the property sheet's hwnd
    // and everything will just work after that.
    
    if (hWndMaybe != m_hwnd)
    {
        if( CompareWindowClass( hWndMaybe, CreateSdmClientA ) )
        {
            m_hwnd = hWndMaybe;
        }
        else	// the window is not an SDM window
        {
            goto DeferToClient;
        }
    } // end if this window is not the SDM window we started talking to.
    
    
    ScreenToClient(m_hwnd, &pt);
    
    MyGetRect(m_hwnd, &rc, FALSE);
    if (!PtInRect(&rc, pt))
        return(S_FALSE);
    
    SetupChildren();	// calls CSdm32::SetupChildren, sets m_cChildren
    
    
    // If there are no children, we check if this is a dropdown list thing.
    if (m_cChildren == 0)
    {
        iControl = SendMessageINT(m_hwnd,WM_GETDROPDOWNID,wVerWord,0L);
        if (iControl)
        {
            // HACK - there is no association between a dropdown and the dialog. So
            // what I am going to do is find the window that is under the point 2 pixels
            // above and to the left of the dropdown's top left corner.
            pt.x = rc.left - 2;
            pt.y = rc.top - 2;
            ClientToScreen (m_hwnd,&pt);
            hWndMaybe = WindowFromPoint (pt);
            // make sure that is an SDM window...
            if( CompareWindowClass( hWndMaybe, CreateSdmClientA ) )
            {
                // create the parent dialog object
                CreateSdmClientA (hWndMaybe,0,IID_IAccessible,(void**)&paccParent);
                // need to find the child id (varchild.lval type) of the sdm list
                lChildId = ((CSdm32*)paccParent)->GetSdmChildIdFromSdmId (iControl);
                // use that in creating the child list object
                CreateSdmList((CSdm32*)paccParent,hWndMaybe,iControl,lChildId, m_f16Bits,
                    FALSE, m_cbWtxiSize, 0, m_hwnd, IID_IDispatch, (void**)&pdispChild);
                
                if (pdispChild)
                {
                    pvarHit->vt = VT_DISPATCH;
                    pvarHit->pdispVal = pdispChild;
                    return (S_OK);
                }
                
            } // end if parent is an SDM window
        } // end if this is a dropdown
        
        goto DeferToClient;
    }
    
    //
    // Allocate a buffer for the controls.
    //
    cbSize = m_cChildren * m_cbWctlSize;
    rgwctShared = (LPWCTL32)SharedAlloc(cbSize,m_hwnd,&hProcess);
    if (!rgwctShared)
        return(E_OUTOFMEMORY);
    
    // Map the pointer to 16-bits if needed
    if (m_f16Bits)
    {
        lpv = MyMapLS(rgwctShared);
        if (!lpv)
            goto AllDone;
        
        ((LPWCTL16)rgwctShared)->wtp = (WORD)cbSize;
    }
    else
    {
        lpv = rgwctShared;
        //rgwctShared->wtp = (WORD)cbSize;
        SharedWrite (&cbSize,&rgwctShared->wtp,sizeof(WORD),hProcess);
    }
    
    //
    // Double check we got back all the controls, like we asked for.
    //
    if (SendMessage(m_hwnd, WM_GETCONTROLS, wVerWord, (LPARAM)lpv) == m_cChildren)
    {
        //
        // Loop through all the controls, doing PtInRect tests.
        //
        iControlMaybe = m_cChildren;
        
        for (iControl = 0; iControl < m_cChildren; iControl++)
        {
            if (m_f16Bits)
            {
                // since we know we are in 16 bit land, and 16 bit only
                // works on Win95 for now, just do regular memory access, 
                // instead of SharedRead. If we can figure out a way to 
                // get MapLS to work on NT, this is a BUGBUG!
                if (!((LPWCTL16)rgwctShared)[iControl].fVisible)
                    continue;
                
                rc.left = ((LPWCTL16)rgwctShared)[iControl].left;
                rc.top = ((LPWCTL16)rgwctShared)[iControl].top;
                rc.right = ((LPWCTL16)rgwctShared)[iControl].right;
                rc.bottom = ((LPWCTL16)rgwctShared)[iControl].bottom;
                
                if (!PtInRect(&rc, pt))
                    continue;
                
                if ((((LPWCTL16)rgwctShared)[iControl].wtp == wtpGroupBox) ||
                    (((LPWCTL16)rgwctShared)[iControl].wtp == wtpGeneralPicture) && !(((LPWCTL16)rgwctShared)[iControl].fList))
                    iControlMaybe = iControl;
                else
                    break;
            }
            else
            {
                SharedRead (&rgwctShared[iControl],&wctLocal,sizeof(WCTL32),hProcess);
                if (!wctLocal.fVisible)
                    continue;
                
                if (!PtInRect(&wctLocal.rect, pt))
                    continue;
                
                //
                // Is this a "group box" or a general picture without a list?  
                // If so, remember this item, but see if this point is in 
                // another real control. A gerneral picture without a list
                // is a "property page".
                //
                if ((wctLocal.wtp == wtpGroupBox) ||
                    (wctLocal.wtp == wtpGeneralPicture && !wctLocal.fList))
                    iControlMaybe = iControl;
                else
                    break;
            }
        }
        
        if (iControl >= m_cChildren)
            iControl = iControlMaybe;
        
        if (iControl < m_cChildren)
        {
            //
            // If this is a real object, return it.
            //
            if (m_f16Bits)
            {
                // since we know we are in 16 bit land, and 16 bit only
                // works on Win95 for now, just do regular memory access, 
                // instead of SharedRead. If we can figure out a way to 
                // get MapLS to work on NT, this is a BUGBUG!
                if (((LPWCTL16)rgwctShared)[iControl].fList)
                    CreateSdmList(this, m_hwnd, ((LPWCTL16)rgwctShared)[iControl].wId,
                    iControl+1, TRUE, (((LPWCTL16)rgwctShared)[iControl].wtp == wtpGeneralPicture),
                    m_cbWtxiSize, 0, NULL, IID_IDispatch, (void**)&pdispChild);
            }
            else
            {
                if (wctLocal.fList)
                    CreateSdmList(this, m_hwnd, wctLocal.wId,
                    iControl+1, FALSE, (wctLocal.wtp == wtpGeneralPicture),
                    m_cbWtxiSize, 0, NULL, IID_IDispatch, (void**)&pdispChild);
            }
            
            if (pdispChild)
            {
                pvarHit->vt = VT_DISPATCH;
                pvarHit->pdispVal = pdispChild;
            }
            else
            {
                pvarHit->vt = VT_I4;
                pvarHit->lVal = iControl+1;
            }
        }
    }
    
    // UnMap pointer
    if (m_f16Bits)
        MyUnMapLS(lpv);
    
AllDone:
    SharedFree(rgwctShared,hProcess);
    
    //
    // If the point is over an SDM item, return it.
    //
    if (pvarHit->vt != VT_EMPTY)
        return(S_OK);
    else
    {
        //
        // Let default client handler take over
        //
DeferToClient:
    return(CClient::accHitTest(x, y, pvarHit));
    }
}





/////////////////////////////////////////////////////////////////////////////
//
//  SDM lists
//
//  BOGUS:
//  I can currently let you enumerate the children.  But I have no way,
//  with existing SDM messages, to do the hittesting/location stuff.
//  The Office97 folks are considering adding such a thing for O97.  We
//  should use them if so.  But we should work also fine with O95 which
//  doesn't have them.
//
/////////////////////////////////////////////////////////////////////////////


// --------------------------------------------------------------------------
//
//  CreateSdmList()
//
//  Parameters:
//		psdmParent	A pointer to the SDM dialog window OBJECT that owns this
//					list. This object should be able to answer questions
//					about the list.
//		hwnd		The window handle of the owning dialog window.
//		idSdm		The SDM child id for the list control.
//		idAccess	
//		f16bits		Is this a 16 bit SDM dialog?
//		fPageTabs	Is this a page tab list?
//		cbTxiSize	size of the wtxi structure???
//		idChildCur	id of the current child (1..n type)
//		hwndList	IFF the list is a dropdown, this is the window handle
//					of the dropdown. Otherwise, is NULL.
//		riid		What kind of interface pointer to the list object do
//					you want?
//		ppvSdmList	This will be filled in with the interface pointer to
//					the new sdm list object.
//
//	Returns:
//		Standard COM error codes?
//
// --------------------------------------------------------------------------
HRESULT CreateSdmList(CSdm32* psdmParent, HWND hwnd, long idSdm, long idAccess,
                      BOOL f16Bits, BOOL fPageTabs, UINT cbTxiSize, long idChildCur, HWND hwndList,
                      REFIID riid, void** ppvSdmList)
{
    CSdmList* psdmList;
    HRESULT hr;
    
    InitPv(ppvSdmList);
    
    psdmList = new CSdmList(psdmParent, hwnd, idSdm, idAccess, f16Bits,
        fPageTabs, cbTxiSize, idChildCur, hwndList);
    if (!psdmList)
        return(E_OUTOFMEMORY);
    
    hr = psdmList->QueryInterface(riid, ppvSdmList);
    if (!SUCCEEDED(hr))
        delete psdmList;
    
    return(hr);
}



// --------------------------------------------------------------------------
//
//  CSdmList()
//
// --------------------------------------------------------------------------
CSdmList::CSdmList(CSdm32* psdm, HWND hwndSdm, long idSdm, long idAccess,
                   BOOL f16Bits, BOOL fPageTabs, UINT cbTxiSize, long idChildCur,HWND hwndList)
{
    m_hwnd = hwndSdm;
    m_idSdm = idSdm;
    m_idAccess = idAccess;
    m_f16Bits = f16Bits;
    m_fPageTabs = fPageTabs;
    m_cbWtxi = cbTxiSize;
    m_hwndList = hwndList;
    m_idChildCur = idChildCur;
    
    m_psdmParent = psdm;
    psdm->AddRef();
}



// --------------------------------------------------------------------------
//
//  ~CSdmList()
//
// --------------------------------------------------------------------------
CSdmList::~CSdmList()
{
    m_psdmParent->Release();
}



// --------------------------------------------------------------------------
//
//  CSdmList::SetupChildren()
//
// --------------------------------------------------------------------------
void CSdmList::SetupChildren(void)
{
    m_cChildren = SendMessageINT(m_hwnd, WM_GETLISTCOUNT, wVerWord, m_idSdm);
}



// --------------------------------------------------------------------------
//
//  CSdmList::GetListItemName()
//
//  This gets the list item name, mnemonic and all.
//
// --------------------------------------------------------------------------
BOOL CSdmList::GetListItemName(long index, LPTSTR lpszName, UINT cchMax)
{
    LPWTXI32    lpwtxShared;
    LPVOID      lpvTx;
    LPSTR       lpszShared; // ANSI, not TCHAR
    HANDLE      hProcess;
    LPSTR       lpszLocal; // ANSI, not TCHAR
    WORD        wSomeWord;
    
    *lpszName = 0;
    
    //
    // We have no way of finding out ahead of time how big the text is
    // for each item.  So just use the number the caller passes in.
    //
    lpwtxShared = (LPWTXI32)SharedAlloc(m_cbWtxi + cchMax*sizeof(CHAR),m_hwnd,&hProcess);
    if (!lpwtxShared)
        return(FALSE);
    lpszLocal = (LPSTR)LocalAlloc(LPTR,cchMax*sizeof(CHAR));
    if (!lpszLocal)
    {
        SharedFree (lpwtxShared,hProcess);
        return(FALSE);
    }
    
    // Map to 16-bits if we need to
    if (m_f16Bits)
    {
        lpvTx = MyMapLS(lpwtxShared);
        if (!lpvTx)
            goto End;
    }
    else
        lpvTx = lpwtxShared;
    
    // This works for 16-bit and 32-bit pointers
    lpszShared = (LPSTR)((LPBYTE)lpvTx + m_cbWtxi);
    
    if (m_f16Bits)
    {
		// We'll never get here under NT/UNICODE, since our MyMapLS returns NULL

        // since we know we are in 16 bit land, and 16 bit only
        // works on Win95 for now, just do regular memory access, 
        // instead of SharedRead. If we can figure out a way to 
        // get MapLS to work on NT, this is a BUGBUG!
        ((LPWTXI16)lpwtxShared)->lpszBuffer = (LPSTR) lpszShared;
        ((LPWTXI16)lpwtxShared)->cch = static_cast<WORD>(cchMax-1);
        ((LPWTXI16)lpwtxShared)->wId = (WORD)m_idSdm;
        ((LPWTXI16)lpwtxShared)->wIndex = (WORD)index;
    }
    else
    {
        //lpwtxShared->lpszBuffer = lpszShared;
        SharedWrite (&lpszShared,&lpwtxShared->lpszBuffer,sizeof(LPSTR),hProcess);
        //lpwtxShared->cch = cchMax-1;
        wSomeWord = static_cast<WORD>(cchMax-1);
        SharedWrite (&wSomeWord,&lpwtxShared->cch,sizeof(WORD),hProcess);
        //lpwtxShared->wId = (WORD)m_idSdm;
        SharedWrite (&m_idSdm,&lpwtxShared->wId,sizeof(WORD),hProcess);
        //lpwtxShared->wIndex = (WORD)index;
        SharedWrite (&index,&lpwtxShared->wIndex,sizeof(WORD),hProcess);
    }
    
    //
    // Get the text
    //
    SendMessage(m_hwnd, WM_GETCTLTEXT, wVerWord, (LPARAM)lpvTx);
    
    //
    // Unmap the flat pointer
    //
    if (m_f16Bits)
        MyUnMapLS(lpvTx);
    
    //
    // Did we get a string?
    //
    lpszShared = (LPSTR)(lpwtxShared + 1);
    SharedRead (lpszShared,lpszLocal,cchMax*sizeof(CHAR),hProcess);
    if (*lpszLocal)
    {
#ifdef UNICODE
        MultiByteToWideChar( CP_ACP, 0, lpszLocal, -1, lpszName, cchMax );
#else
        lstrcpyn(lpszName, lpszLocal, cchMax);
#endif
    }
    
End:
    SharedFree(lpwtxShared,hProcess);
    
    return(*lpszName ? TRUE : FALSE);
}



// --------------------------------------------------------------------------
//
//  CSdmList::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdmList::get_accName(VARIANT varChild, BSTR* pszName)
{
    TCHAR   szItemName[256];
    
    InitPv(pszName);
    
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);
    
    //
    // Hand off to SDM dialog for ourself
    //
    if (!varChild.lVal)
    {
        varChild.lVal = m_idAccess;
        return(m_psdmParent->get_accName(varChild, pszName));
    }
    
    GetListItemName(varChild.lVal-1, szItemName, 256);
    StripMnemonic(szItemName);
    
    if (*szItemName)
        *pszName = TCharSysAllocString(szItemName);
    
    return(*pszName ? S_OK : S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CSdmList::get_accValue()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdmList::get_accValue(VARIANT varChild, BSTR* pszValue)
{
    InitPv(pszValue);
    
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);
    
    if (!varChild.lVal)
    {
        varChild.lVal = m_idAccess;
        return(m_psdmParent->get_accValue(varChild, pszValue));
    }
    
    return(E_NOT_APPLICABLE);
}



// --------------------------------------------------------------------------
//
//  CSdmList::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdmList::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    WCTL32  wc;
    
    InitPvar(pvarRole);
    
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);
    
    pvarRole->vt = VT_I4;
    
    if (!m_psdmParent->GetSdmControl(m_idAccess, &wc, NULL))
    {
        pvarRole->vt = VT_EMPTY;
        return(S_FALSE);
    }
    
    if (varChild.lVal)
        pvarRole->lVal = (m_fPageTabs ? ROLE_SYSTEM_PAGETAB : ROLE_SYSTEM_LISTITEM);
    else
    {
        if (m_fPageTabs)
            pvarRole->lVal = ROLE_SYSTEM_PAGETABLIST;
        else if (wc.wtp == wtpListBox)
            pvarRole->lVal = ROLE_SYSTEM_LIST;
        else if (wc.wtp == wtpDropList)
            pvarRole->lVal = ROLE_SYSTEM_COMBOBOX;
        else
            pvarRole->lVal = ROLE_SYSTEM_LIST;
        
    }
    
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CSdmList::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdmList::get_accState(VARIANT varChild, VARIANT* pvarState)
{
    WCTL32  wctl;
    
    InitPvar(pvarState);
    
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);
    
    if (!varChild.lVal)
    {
        varChild.lVal = m_idAccess;
        return(m_psdmParent->get_accState(varChild, pvarState));
    }
    
    pvarState->vt = VT_I4;
    pvarState->lVal = 0;
    
    if (m_psdmParent->GetSdmControl(m_idAccess, &wctl, NULL))
    {
        if (varChild.lVal == wctl.wState+1)
            pvarState->lVal |= STATE_SYSTEM_SELECTED;
        
        if (wctl.fHasFocus && (MyGetFocus() == m_hwnd))
            pvarState->lVal |= STATE_SYSTEM_FOCUSED;
    }
    
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CSdmList::get_accKeyboardShortcut()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdmList::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszKey)
{
    TCHAR   szMnemonic[2];
    TCHAR   szName[256];
    
    InitPv(pszKey);
    
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);
    
    if (!varChild.lVal)
    {
        varChild.lVal = m_idAccess;
        return(m_psdmParent->get_accKeyboardShortcut(varChild, pszKey));
    }
    
    if (!m_fPageTabs)
        return(E_NOT_APPLICABLE);
    
    GetListItemName(varChild.lVal-1, szName, 256);
    if (szMnemonic[0] = StripMnemonic(szName))
    {
        szMnemonic[1] = 0;
        
        return(HrMakeShortcut(szMnemonic, pszKey));
    }
    
    return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CSdmList::get_accFocus()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdmList::get_accFocus(VARIANT* pvarFocus)
{
    WCTL32 wctl;
    
    InitPvar(pvarFocus);
    
    //
    // Does the SDM dialog have the focus?
    //
    if (MyGetFocus() != m_hwnd)
        return(S_FALSE);
    
    //
    // Is the control in the dialog us?
    //
    if (!m_psdmParent->GetSdmControl(m_idAccess, &wctl, NULL))
        return(S_FALSE);
    if (!wctl.fHasFocus)
        return(S_FALSE);
    
    pvarFocus->vt = VT_I4;
    pvarFocus->lVal = wctl.wState+1;
    
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CSdmList::accSelect()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdmList::accSelect(long flagsSel, VARIANT varChild)
{
    if (!ValidateChild(&varChild) || !ValidateSelFlags(flagsSel))
        return(E_INVALIDARG);
    
    if (!varChild.lVal)
    {
        varChild.lVal = m_idAccess;
        return(m_psdmParent->accSelect(flagsSel, varChild));
    }
    
    return(E_NOT_APPLICABLE);
}



// --------------------------------------------------------------------------
//
//  CSdmList::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdmList::accLocation(long* pxLeft, long* pyTop, long* pcxWidth,
                                   long* pcyHeight, VARIANT varChild)
{
    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);
    
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);
    
    if (!varChild.lVal)
    {
        varChild.lVal = m_idAccess;
        return(m_psdmParent->accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild));
    }
    
    return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CSdmList::accNavigate()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdmList::accNavigate(long dwNavDir, VARIANT varStart, VARIANT* pvarEnd)
{
    long    lEnd;
    
    InitPvar(pvarEnd);
    
    if (!ValidateChild(&varStart) || !ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);
    
    lEnd = varStart.lVal;
    
    if (dwNavDir == NAVDIR_FIRSTCHILD)
    {
        lEnd = 0;
        dwNavDir = NAVDIR_NEXT;
    }
    else if (dwNavDir == NAVDIR_LASTCHILD)
    {
        lEnd = m_cChildren + 1;
        dwNavDir = NAVDIR_PREVIOUS;
    }
    else if (!varStart.lVal)
    {
        varStart.lVal = m_idAccess;
        return(m_psdmParent->accNavigate(dwNavDir, varStart, pvarEnd));
    }
    
    switch (dwNavDir)
    {
    case NAVDIR_NEXT:
NextListItem:
        ++lEnd;
        if (lEnd > m_cChildren)
            lEnd = 0;
        break;
        
    case NAVDIR_PREVIOUS:
PrevListItem:
        --lEnd;
        break;
        
    case NAVDIR_LEFT:
        if (m_fPageTabs)
            goto PrevListItem;
        else
            lEnd = 0;
        break;
        
    case NAVDIR_RIGHT:
        if (m_fPageTabs)
            goto NextListItem;
        else
            lEnd = 0;
        break;
        
    case NAVDIR_UP:
        if (!m_fPageTabs)
            goto PrevListItem;
        else
            lEnd = 0;
        break;
        
    case NAVDIR_DOWN:
        if (!m_fPageTabs)
            goto NextListItem;
        else
            lEnd = 0;
        break;
    }
    
    if (lEnd)
    {
        pvarEnd->vt = VT_I4;
        pvarEnd->lVal = lEnd;
        return(S_OK);
    }
    
    return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CSdmList::accHitTest()
//
//  BOGUS:
//  Currently in SDM there is no way to get the individual list item rects.
//  If the Office97 folks add some messages, use them.  They will do nothing
//  on earlier versions of Office (shouldn't collide anyway).
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdmList::accHitTest(long x, long y, VARIANT* pvarHit)
{
    RECT    rcLocation;
    VARIANT varList;
    POINT   pt;
    
    InitPvar(pvarHit);
    
    //
    // Get list's location
    //
    SetRectEmpty(&rcLocation);
    
    VariantInit(&varList);
    varList.vt = VT_I4;
    varList.lVal = m_idAccess;
    
    m_psdmParent->accLocation((long*)&rcLocation.left, (long*)&rcLocation.top,
        (long*)&rcLocation.right, (long*)&rcLocation.bottom, varList);
    
    rcLocation.right += rcLocation.left;
    rcLocation.bottom += rcLocation.top;
    
    pt.x = x;
    pt.y = y;
    
    if (PtInRect(&rcLocation, pt))
    {
        pvarHit->vt = VT_I4;
        pvarHit->lVal = 0;
        
        return(S_OK);
    }
    
    // need to check if this is a drop down list with it's own window.
    SetupChildren();
    if (m_hwndList != NULL)
    {
        ScreenToClient(m_hwndList, &pt);
        
        MyGetRect(m_hwndList, &rcLocation, FALSE);
        if (PtInRect(&rcLocation, pt))
        {
            pvarHit->vt = VT_I4;
            pvarHit->lVal = 0;
            
            return(S_OK);
        }
    }
    return(S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CSdmList::Clone()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSdmList::Clone(IEnumVARIANT** ppenum)
{
    return(CreateSdmList(m_psdmParent, m_hwnd, m_idSdm, m_idAccess,
        m_f16Bits, m_fPageTabs, m_cbWtxi, m_idChildCur,m_hwndList,
        IID_IEnumVARIANT, (void**)ppenum));
}
