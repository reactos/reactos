// This file is included by COMMCTRL\PRSHT1.C and LIBRARY\PRSHT16.C
// Note that 32-bit COMCTL32.DLL has these functions, while 16-bit SHELL.DLL
// and COMMCTRL.DLL have them.

#include <prsht.h>
#ifndef IEWIN31
#include <commctrl.h>           // For the hotkey hack.
#endif
#include <prshtp.h>


#define FLAG_NOTID	0x80000000

#ifndef NearAlloc
// wrappers for private allocations, near in 16 bits

#define NearAlloc(cb)       ((void NEAR*)LocalAlloc(LPTR, (cb)))
#define NearReAlloc(pb, cb) ((void NEAR*)LocalReAlloc((HLOCAL)(pb), (cb), LMEM_MOVEABLE | LMEM_ZEROINIT))
#define NearFree(pb)        (LocalFree((HLOCAL)(pb)) ? FALSE : TRUE)
#define NearSize(pb)        LocalSize(pb)
#endif  // NearAlloc

// Thunk entries for 16-bit pages.
typedef LPARAM HPROPSHEETPAGE16;
extern BOOL WINAPI DestroyPropertySheetPage16(HPROPSHEETPAGE16 hpage);
extern HWND WINAPI CreatePage16(HPROPSHEETPAGE16 hpage, HWND hwndParent);
extern BOOL WINAPI GetPageInfo16(HPROPSHEETPAGE16 hpage, LPSTR pszCaption, int cbCaption, LPPOINT ppt, HICON FAR * phIcon);

#ifdef WIN32

#define _Rstrcpyn(psz, pszW, cchMax)  _SWstrcpyn(psz, (LPCWCH)pszW, cchMax)
#define _Rstrlen(pszW)                _Wstrlen((LPCWCH)pszW)
#define RESCHAR WCHAR

#else  // WIN32

#define _Rstrcpyn   lstrcpyn
#define _Rstrlen    lstrlen
#define RESCHAR char

#endif // WIN32

#ifdef WIN32

void _SWstrcpyn(LPSTR psz, LPCWCH pwsz, UINT cchMax)
{
    WideCharToMultiByte(CP_ACP, 0, pwsz, -1, psz, cchMax, NULL, NULL);
}

UINT _Wstrlen(LPCWCH pwsz)
{
    UINT cwch = 0;
    while (*pwsz++)
	cwch++;
    return cwch;
}

#endif



BOOL WINAPI DestroyPropertySheetPage(PSP FAR *hpage)
{
#ifdef WIN32
    // Check if this is a proxy page for 16-bit page object.
    if (hpage->psp.dwFlags & PSP_IS16)
    {
	// Yes, call 16-bit side of DestroyPropertySheetPage();
	DestroyPropertySheetPage16(hpage->psp.lParam);
	// Then, free the 16-bit DLL if we need to.
	if (hpage->psp.hInstance)
	{
	    FreeLibrary16(hpage->psp.hInstance);
	}
    }
    else
#endif
    {
	if ((hpage->psp.dwFlags & PSP_USEREFPARENT) && hpage->psp.pcRefParent)
	    (*hpage->psp.pcRefParent)--;

        if ((hpage->psp.dwFlags & PSP_USECALLBACK) && hpage->psp.pfnCallback)
            hpage->psp.pfnCallback(NULL, PSPCB_RELEASE, &hpage->psp);
        
    }

#ifdef WIN32
    Free(hpage);
#else
    GlobalFreePtr(hpage);
#endif
    return TRUE;
}


BOOL WINAPI GetPageInfo(PSP FAR *hpage, LPSTR pszCaption, int cbCaption,
			     LPPOINT ppt, HICON FAR *phIcon)
{
    HRSRC hRes;
    LPDLGTEMPLATE pDlgTemplate;
    BOOL bResult = FALSE;
#ifndef WIN31
    int cxIcon      = GetSystemMetrics(SM_CXSMICON);
    int cyIcon      = GetSystemMetrics(SM_CYSMICON);
#else
    int cxIcon      = GetSystemMetrics(SM_CXICON) / 2;
    int cyIcon      = GetSystemMetrics(SM_CYICON) / 2;
#endif // !WIN31

#ifdef WIN32
    // Check if this is a proxy page for 16-bit page object.
    if (hpage->psp.dwFlags & PSP_IS16)
    {
	// Yes, call 16-bit side of GetPageInfo
	return GetPageInfo16(hpage->psp.lParam, pszCaption, cbCaption, ppt, phIcon);
    }
#endif

    if (hpage->psp.dwFlags & PSP_USEHICON)
	*phIcon = hpage->psp.hIcon;
#ifndef WIN31
    else if (hpage->psp.dwFlags & PSP_USEICONID)
	*phIcon = LoadImage(hpage->psp.hInstance, hpage->psp.pszIcon, IMAGE_ICON, cxIcon, cyIcon, LR_DEFAULTCOLOR);
#endif // !WIN31
    else
	*phIcon = NULL;

    if (hpage->psp.dwFlags & PSP_DLGINDIRECT)
    {
	pDlgTemplate = (LPDLGTEMPLATE)hpage->psp.pResource;
	goto UseTemplate;
    }

    hRes = FindResource(hpage->psp.hInstance, hpage->psp.pszTemplate, RT_DIALOG);
    if (hRes)
    {
	HGLOBAL hDlgTemplate;
	hDlgTemplate = LoadResource(hpage->psp.hInstance, hRes);
	if (hDlgTemplate)
	{
	    // BUGBUG: need to support DIALOGEX template format
	    pDlgTemplate = (LPDLGTEMPLATE)LockResource(hDlgTemplate);
	    if (pDlgTemplate)
	    {
UseTemplate:
		//
		// Get the width and the height in dialog units.
		//
		ppt->x = pDlgTemplate->cx;
		ppt->y = pDlgTemplate->cy;

		bResult = TRUE;

		if (pszCaption)
		{
			
		    if (hpage->psp.dwFlags & PSP_USETITLE)
		    {
			if (HIWORD(hpage->psp.pszTitle) == 0)
			    LoadString(hpage->psp.hInstance, (UINT)LOWORD(hpage->psp.pszTitle), pszCaption, cbCaption);
			else
			{
			    // pszTitle is just an offset with FLAG_NOTID added in
			    lstrcpyn(pszCaption, ((LPSTR)hpage)
				+ ((DWORD)(hpage->psp.pszTitle)&~FLAG_NOTID), cbCaption);
			}
		    }
		    else
		    {
			// Get the caption string from the dialog template, only
			//
			// The menu name is either 0xff followed by a word, or a string.
			//
			LPCSTR pszT = (LPCSTR)pDlgTemplate + sizeof(DLGTEMPLATE);
			switch (*pszT) {
			case 0xff:
			    pszT += sizeof(WORD);
			    break;

			default:
			    pszT += (_Rstrlen(pszT) + 1) * sizeof(RESCHAR);
			    break;
			}
			//
			// Now we are pointing at the class name.
			//
			pszT += (_Rstrlen(pszT) + 1) * sizeof(RESCHAR);

			_Rstrcpyn(pszCaption, pszT, cbCaption);
		    }
		}

		if (hpage->psp.dwFlags & PSP_DLGINDIRECT)
		    return TRUE;

		UnlockResource(hDlgTemplate);
	    }
	    FreeResource(hDlgTemplate);
	}
    }
    else
    {
	DebugMsg(DM_ERROR, "GetPageInfo - ERROR: FindResource() failed");
    }
    return bResult;
}


//
//  This function creates a dialog box from the specified dialog template
// with appropriate style flags.
//
HWND NEAR PASCAL _CreatePageDialog(PSP FAR *hpage, HWND hwndParent, LPDLGTEMPLATE pDlgTemplate)
{
    HWND hwndPage;
    // BUGBUG: need to support DIALOGEX template format

    // We need to save the SETFONT, LOCALEDIT, and CLIPCHILDREN
    // flags.
    //
    DWORD lSaveStyle;

    lSaveStyle = pDlgTemplate->style;
    pDlgTemplate->style= (lSaveStyle & (DS_SETFONT | DS_LOCALEDIT | WS_CLIPCHILDREN))
			    | WS_CHILD | WS_TABSTOP | DS_3DLOOK | DS_CONTROL;

    hwndPage = CreateDialogIndirectParam(
		    hpage->psp.hInstance,
		    (LPCDLGTEMPLATE)pDlgTemplate,
		    hwndParent,
		    hpage->psp.pfnDlgProc, (LPARAM)(LPPROPSHEETPAGE)&hpage->psp);

    pDlgTemplate->style = lSaveStyle;
    return hwndPage;
}


HWND WINAPI CreatePage(PSP FAR *hpage, HWND hwndParent)
{
    HWND hwndPage = NULL; // NULL indicates an error
    
    if ((hpage->psp.dwFlags & PSP_USECALLBACK) && hpage->psp.pfnCallback)
        if (!hpage->psp.pfnCallback(NULL, PSPCB_CREATE, &hpage->psp))
            return NULL;

#ifdef WIN32
    // Check if this is a proxy page for 16-bit page object.
    if (hpage->psp.dwFlags & PSP_IS16)
    {
	// Yes, call 16-bit side of CreatePage();
	return CreatePage16(hpage->psp.lParam, hwndParent);
    }
#endif

    if (hpage->psp.dwFlags & PSP_DLGINDIRECT)
    {
	hwndPage=_CreatePageDialog(hpage, hwndParent, (LPDLGTEMPLATE)hpage->psp.pResource);
    }
    else
    {
	HRSRC hRes;
	hRes = FindResource(hpage->psp.hInstance, hpage->psp.pszTemplate, RT_DIALOG);
	if (hRes)
	{
	    HGLOBAL hDlgTemplate;
	    hDlgTemplate = LoadResource(hpage->psp.hInstance, hRes);
	    if (hDlgTemplate)
	    {
		const DLGTEMPLATE FAR * pDlgTemplate;
		pDlgTemplate = (LPDLGTEMPLATE)LockResource(hDlgTemplate);
		if (pDlgTemplate)
		{
		    ULONG cbTemplate=SizeofResource(hpage->psp.hInstance, hRes);
		    LPDLGTEMPLATE pdtCopy = (LPDLGTEMPLATE)Alloc(cbTemplate);

		    Assert(cbTemplate>=sizeof(DLGTEMPLATE));

		    if (pdtCopy)
		    {
			hmemcpy(pdtCopy, pDlgTemplate, cbTemplate);
			hwndPage=_CreatePageDialog(hpage, hwndParent, pdtCopy);
			Free(pdtCopy);
		    }

		    UnlockResource(hDlgTemplate);
		}
		FreeResource(hDlgTemplate);
	    }
	}
    }

    return hwndPage;
}

//
//

PSP FAR * WINAPI CreatePropertySheetPage(LPCPROPSHEETPAGE psp)
{
    PSP FAR *hpage;
    DWORD uHeaderLen, uTitleLen;

    if ((psp->dwSize < sizeof(PROPSHEETPAGE)) ||        // structure size wrong
	(psp->dwSize > 4096) ||                         // (unreasonable amout to ask for)
	(psp->dwFlags & ~PSP_ALL))                      // bogus flag used
	return NULL;

    uHeaderLen = psp->dwSize + sizeof(*hpage) - sizeof(hpage->psp);

    if ((psp->dwFlags & PSP_USETITLE) && HIWORD(psp->pszTitle))
    {
	uTitleLen = lstrlen(psp->pszTitle) + 1;
    }
    else
    {
	uTitleLen = 0;
    }

    // enough for the user data (dwSize) and our extra fields and the title
#ifdef WIN32
    hpage = Alloc(uHeaderLen + uTitleLen);
#else
    hpage = (PSP FAR *)GlobalAllocPtr(GPTR, uHeaderLen + uTitleLen);
#endif


    if (hpage) {
	hmemcpy(&hpage->psp, psp, psp->dwSize);
	if (uTitleLen)
	{
	    lstrcpy(((LPSTR)hpage) + uHeaderLen, psp->pszTitle);
	    // We need to OR in the NOTID flag so that the HIWORD is not 0
	    // (which would mean that it is a MAKEINTRESOURCE thing)
	    hpage->psp.pszTitle = (LPSTR)(uHeaderLen|FLAG_NOTID);
	}

	// Increment the reference count to the parent object.
	if ((hpage->psp.dwFlags & PSP_USEREFPARENT) && hpage->psp.pcRefParent)
	    (*hpage->psp.pcRefParent)++;

    }
    return hpage;
}
