//**********************************************************************
// File name: cdataobj.cxx
//
// Definition of CImpIDataObject
// Implements the IDataObject interface required for Data transfer
//
// History :
//       Dec 23, 1997   [v-nirnay]    wrote it.
//       Modified RenderRTFText output procedure for charset
//
// Copyright (c) 1997-1999 Microsoft Corporation.
//**********************************************************************

#include <windows.h>
#include <ole2.h>

#include "olecomon.h"
#include "enumfetc.h"
#include "cdataobj.h"
#include "uce.h"

// Chargrid supports three formats Rich Text, Plain text and Unicode text
// Of which the last two are standard and are already registered
CLIPFORMAT g_cfRichText = 0;

// List of FormatEtcs which are supported by Chargrid
// The first format is filled up in CImpIDataObject constructor
static FORMATETC g_FormatEtc[] =
{
    { 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
    { CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
    { CF_TEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}
};

//**********************************************************************
// CImpIDataObject::CImpIDataObject
//
// Purpose:
//      Constructor
//
// Parameters:
//      HWND hWnd           -  Dialog handle which has to be cached
//
// Return Value:
//      None
//**********************************************************************
CImpIDataObject::CImpIDataObject(HWND hWnd)
{
    // Initialise reference count to 0
    m_cRef = 1;
    hWndDlg = hWnd;
    m_lpszText[0] = 0;

    // Register Rich Text Clipboard format if necessary
    if (g_cfRichText == 0) {
        TCHAR szRTF[80];
        int iRet = LoadString(hInst, IDS_RTF, szRTF, 80);
        g_cfRichText = (CLIPFORMAT)RegisterClipboardFormat(szRTF);
//should hardcode clipboard format. we will do it after Win2K
//        g_cfRichText = (CLIPFORMAT)RegisterClipboardFormat(RTFFMT);
        g_FormatEtc[0].cfFormat = g_cfRichText;
    }
}

//**********************************************************************
// CImpIDataObject::QueryInterface
//
// Purpose:
//      Return a pointer to a requested interface
//
// Parameters:
//      REFIID riid         -   ID of interface to be returned
//      PPVOID ppv          -   Location to return the interface
//
// Return Value:
//      NOERROR             -   Interface supported
//      E_NOINTERFACE       -   Interface NOT supported
//**********************************************************************
STDMETHODIMP CImpIDataObject::QueryInterface(REFIID riid,
                                             PPVOID ppv)
{
    // Initialise interface pointer to NULL
    *ppv = NULL;

    // If the interface asked for is what we have set ppv
    if (riid == IID_IUnknown || riid == IID_IDataObject) {
        *ppv = this;
    }

    // Increment reference count if the interface asked for is correct
    // and return no error
    if (*ppv != NULL) {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    // Else return no such interface
    return ResultFromScode(E_NOINTERFACE);
}

//**********************************************************************
// CImpIDataObject::AddRef
//
// Purpose:
//      Increments the reference count for an interface on an object
//
// Parameters:
//      None
//
// Return Value:
//      int                 -   Value of the new reference count
//**********************************************************************
ULONG STDMETHODCALLTYPE CImpIDataObject::AddRef()
{
    TRACE(TEXT("In AddRef\n"));

    // Increment reference count
    return InterlockedIncrement((LONG*)&m_cRef);
}

//**********************************************************************
// CImpIDataObject::Release
//
// Purpose:
//      Decrements the reference count for the interface on an object
//
// Parameters:
//      None
//
// Return Value:
//      int                 -   Value of the new reference count
//**********************************************************************
ULONG STDMETHODCALLTYPE CImpIDataObject::Release()
{
    TRACE(TEXT("In Release\n"));

    // Decrement reference count
    InterlockedDecrement((LONG*)&m_cRef);

    // If there are no references then free current copy
    if (m_cRef == 0L) {
        delete this;
        return 0;
    }

    return m_cRef;
}

//**********************************************************************
// CImpIDataObject::GetData
//
// Purpose:
//      Called by a data consumer to obtain data from a source data object
//
// Parameters:
//      FORMATETC* pFormatetc  -   Pointer to the FORMATETC structure
//      STGMEDIUM* pMedium     -   Pointer to the STGMEDIUM structure
//
// Return Value:
//      S_OK                   -   Data was successfully retrieved and
//                                 placed in the storage medium provided
//      DATA_E_FORMATETC       -   Invalid value for pFormatetc
//**********************************************************************
STDMETHODIMP CImpIDataObject::GetData(FORMATETC* pFormatetc,
                                      STGMEDIUM* pMedium)
{
    SCODE   sc = DATA_E_FORMATETC; // Deflt return value data format not supported

    // Initialise the storage medium structure
    pMedium->tymed = NULL;
    pMedium->pUnkForRelease = NULL;
    pMedium->hGlobal = NULL;

    TRACE(TEXT("In GetData\n"));

    // Our CharGrid only supports drag and drop of text through global memory
    // Client wants plain text through global memory
    if (pFormatetc->cfFormat == CF_TEXT &&
        pFormatetc->dwAspect == DVASPECT_CONTENT &&
        pFormatetc->tymed == TYMED_HGLOBAL) {

        sc = RenderPlainAnsiText(pMedium);
    }

    // Client wants Unicode text through global memory
    if (pFormatetc->cfFormat == CF_UNICODETEXT &&
        pFormatetc->dwAspect == DVASPECT_CONTENT &&
        pFormatetc->tymed == TYMED_HGLOBAL) {

        sc = RenderPlainUnicodeText(pMedium);
    }

    // Client wants Rich Text through global memory
    if (pFormatetc->cfFormat == g_cfRichText &&
        pFormatetc->dwAspect == DVASPECT_CONTENT &&
        pFormatetc->tymed == TYMED_HGLOBAL) {

        sc = RenderRTFText(pMedium);
    }

    return ResultFromScode(sc);
}

//**********************************************************************
// CImpIDataObject::GetDataHere
//
// Purpose:
//      Called by a consumer to obtain data with caller allocing memory
//
// Parameters:
//      FORMATETC* pFormatetc  -   Pointer to the FORMATETC structure
//      STGMEDIUM* pMedium     -   Pointer to the STGMEDIUM structure
//
// Return Value:
//      DATA_E_FORMATETC       -   Invalid value for pFormatetc
//**********************************************************************
STDMETHODIMP CImpIDataObject::GetDataHere(FORMATETC* pFormatetc,
                                          STGMEDIUM* pMedium)
{
    // This method says that the caller allocates and frees memory,
    // we dont support this method
    return ResultFromScode(DATA_E_FORMATETC);
}

//**********************************************************************
// CImpIDataObject::GetDataHere
//
// Purpose:
//      Determines whether the data object is capable of rendering the
//      data described in the FORMATETC structure
//
// Parameters:
//      FORMATETC* pFormatetc  -   Pointer to the FORMATETC structure
//
// Return Value:
//      DV_E_FORMATETC         -   Invalid value for pFormatetc
//      S_OK                   -   Next call to GetData will be success
//**********************************************************************
STDMETHODIMP CImpIDataObject::QueryGetData(FORMATETC* pFormatetc)
{
    SCODE  sc = DV_E_FORMATETC; // dflt return value is invalid formatetc

    TRACE(TEXT("In QueryGetData\n"));

    // Our CharGrid only supports drag and drop of text through global memory

    // If the client wants text, unicode text or rich text through global memory
    // then return OK
    if (pFormatetc->cfFormat == CF_TEXT
        && pFormatetc->dwAspect == DVASPECT_CONTENT
        && pFormatetc->tymed & TYMED_HGLOBAL) {
        sc = S_OK;
    }
    else if (pFormatetc->cfFormat == CF_UNICODETEXT
        && pFormatetc->dwAspect == DVASPECT_CONTENT
        && pFormatetc->tymed & TYMED_HGLOBAL) {
        sc = S_OK;
    }
    else if (pFormatetc->cfFormat == g_cfRichText
        && pFormatetc->dwAspect == DVASPECT_CONTENT
        && pFormatetc->tymed & TYMED_HGLOBAL) {
        sc = S_OK;
    }

    return ResultFromScode(sc);
}

//**********************************************************************
// CImpIDataObject::GetCanonicalFormatEtc
//
// Purpose:
//      Provides a standard FORMATETC structure that is logically
//      equivalent to one that is more complex.
//
// Parameters:
//      FORMATETC* pFormatetcIn  -   Pointer to the FORMATETC structure
//      FORMATETC* pFormatetcOut -   Pointer to canonical equivalent
//
// Return Value:
//      E_NOTIMPL              -   This call is not implemented
//**********************************************************************

STDMETHODIMP CImpIDataObject::GetCanonicalFormatEtc(FORMATETC* pFormatetcIn,
                                                    FORMATETC* pFormatetcOut)
{
    // Chargrid does not support conversion of FORMATETC
    // Check for invalid input
    if (pFormatetcIn == NULL) {
        return ResultFromScode(E_INVALIDARG);
    }

    pFormatetcOut->ptd = NULL;

    return ResultFromScode(E_NOTIMPL);
}

//**********************************************************************
// CImpIDataObject::SetData
//
// Purpose:
//      Called by an object containing a data source to transfer data to
//      the object that implements this method
//
// Parameters:
//      FORMATETC* pFormatetc  -   Pointer to the FORMATETC structure
//      STGMEDIUM* pMedium     -   Pointer to the STGMEDIUM structure
//      BOOL       fRelease    -   Indicates who owns storage medium
//
// Return Value:
//      E_NOTIMPL              -   This call is not implemented
//**********************************************************************

STDMETHODIMP CImpIDataObject::SetData(FORMATETC* pFormatetc,
                                      STGMEDIUM* pMedium,
                                      BOOL       fRelease)
{
    // We do not allow another object to send data to us
    // So return error not implemented
    return ResultFromScode(E_NOTIMPL);
}

//**********************************************************************
// CImpIDataObject::EnumFormatEtc
//
// Purpose:
//      Creates an object for enumerating the FORMATETC structures for a data object
//
// Parameters:
//      DWORD            dwDirection     -   Pointer to the FORMATETC structure
//      IEnumFORMATETC** ppEnumFormatetc -   Pointer to canonical equivalent
//
// Return Value:
//      S_OK                             -   Enumerator object was successfully created.
//      E_NOTIMPL                        -   The direction specified by dwDirection is not supported.
//**********************************************************************
STDMETHODIMP CImpIDataObject::EnumFormatEtc(DWORD            dwDirection,
                                            IEnumFORMATETC** ppEnumFormatetc)
{
    SCODE   sc = S_OK;

    *ppEnumFormatetc = NULL;

    TRACE(TEXT("In EnumFormatEtc\n"));

    // If the direction of enumeration is Get then create
    // an instance of the standard implementation of IEnumFmtEtc
    if (dwDirection == DATADIR_GET) {
        *ppEnumFormatetc = OleStdEnumFmtEtc_Create(
            sizeof(g_FormatEtc)/sizeof(g_FormatEtc[0]),
            g_FormatEtc);

        if (*ppEnumFormatetc == NULL) {
            sc = E_OUTOFMEMORY;
        }
    } else if (dwDirection == DATADIR_SET) {
        // Chargrid does not accept SetData
        sc = E_NOTIMPL;
    } else {
        // Anything else provided as direction is an error
        sc = E_INVALIDARG;
    }

    return ResultFromScode(sc);
}

//**********************************************************************
// CImpIDataObject::DAdvise
//
// Return Value:
//      OLE_E_ADVISENOTSUPPORTED - Data object does not support change
//                                 notification
//**********************************************************************
STDMETHODIMP CImpIDataObject::DAdvise(FORMATETC*,
                                      DWORD,
                                      IAdviseSink*,
                                      DWORD*)
{
    // Chargrid does not support notifications (AdviseSink) on change of data
    // So return error advise not supported
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

//**********************************************************************
// CImpIDataObject::DUnadvise
//
// Return Value:
//      OLE_E_ADVISENOTSUPPORTED - Data object does not support change
//                                 notification
//**********************************************************************
STDMETHODIMP CImpIDataObject::DUnadvise(DWORD dwConnection)
{
    // Chargrid does not support notifications (AdviseSink) on change of data
    // So return error advise not supported
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

//**********************************************************************
// CImpIDataObject::EnumDAdvise
//
// Return Value:
//      OLE_E_ADVISENOTSUPPORTED - Data object does not support change
//                                 notification
//**********************************************************************
STDMETHODIMP CImpIDataObject::EnumDAdvise(IEnumSTATDATA**)
{
    // Chargrid does not support notifications (AdviseSink) on change of data
    // So return error advise not supported
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

//**********************************************************************
// CImpIDataObject::RenderRTFText
//
// Purpose:
//      Copies the character from currently selected grid cell to global
//      memory in Rich Text format
//
// Parameters:
//      STGMEDIUM* pMedium  -   Pointer to the STGMEDIUM structure
//
// Return Value:
//      S_OK                - Data was transferred successfully
//**********************************************************************
HRESULT CImpIDataObject::RenderRTFText(STGMEDIUM* pMedium)
{
    INT iCurrFont, iCharset, nIndex;
    FONTINFO *pFontInfo;
    TCHAR szFaceName[LF_EUDCFACESIZE];
    HANDLE hmemRTF;
    LPTSTR lpstrClipString;
    TCHAR achHeaderTmpl[] = TEXT("{\\rtf1\\ansi\\ansicpg%d {\\fonttbl{\\f0\\");
    TCHAR achHeader[sizeof(achHeaderTmpl) / sizeof(TCHAR) + 20];
    TCHAR achMiddle[] = TEXT(";}}\\sectd\\pard\\plain\\f0 ");
    INT cchUC;
    LPTSTR lpstrText;

    TRACE(TEXT("Rendering Rich Text\n"));

    lpstrText = m_lpszText;

#ifndef UNICODE_RTF
    LPWSTR pszRTFW;
#endif

#define MAXLENGTHFONTFAMILY 8
#define ALITTLEEXTRA 10    // covers extra characters + length of font size

    iCurrFont = (INT)SendDlgItemMessage(hWndDlg, ID_FONT, CB_GETCURSEL, 0, 0L);
    nIndex = (INT)SendDlgItemMessage(hWndDlg, ID_FONT, CB_GETITEMDATA, iCurrFont, 0L);
    pFontInfo = Font_pList+nIndex;
    lstrcpy(szFaceName, pFontInfo->szFaceName);
    iCharset = GetCurFontCharSet(hWndDlg);

    wsprintf(achHeader, achHeaderTmpl, (INT)(SHORT)GetACP());

    //
    //  16 times in case they're all > 7 bits (each chr -> \uc1\uddddddd\'xx)
    //  and room for the second byte of DBCS.
    //
    hmemRTF = GlobalAlloc( 0,
        CTOB(lstrlen((LPTSTR)achHeader) +
        MAXLENGTHFONTFAMILY +
        lstrlen(szFaceName) +
        lstrlen((LPTSTR)achMiddle) +
        2 * 16 * lstrlen(lpstrText) +
        ALITTLEEXTRA) );
    if (hmemRTF == NULL)
    {
        return 1;
    }

    lpstrClipString = (LPTSTR)GlobalLock(hmemRTF);

#ifndef UNICODE_RTF
    pszRTFW = lpstrClipString;
#endif

    lstrcpy(lpstrClipString, achHeader);

    // Add the correct charset string
    if (iCharset == SYMBOL_CHARSET)
    {
        lstrcat(lpstrClipString, (LPTSTR)TEXT("fnil\\fcharset2 "));
    }
    else
    {
        //
        //  Top four bits specify family.
        //
        switch (pFontInfo->PitchAndFamily & 0xf0)
        {
            case ( FF_DECORATIVE ) :
            {
                lstrcat(lpstrClipString, (LPTSTR)TEXT("fdecor "));
                break;
            }
            case ( FF_MODERN ) :
            {
                lstrcat(lpstrClipString, (LPTSTR)TEXT("fmodern "));
                break;
            }
            case ( FF_ROMAN ) :
            {
                lstrcat(lpstrClipString, (LPTSTR)TEXT("froman "));
                break;
            }
            case ( FF_SCRIPT ) :
            {
                lstrcat(lpstrClipString, (LPTSTR)TEXT("fscript "));
                break;
            }
            case ( FF_SWISS ) :
            {
                lstrcat(lpstrClipString, (LPTSTR)TEXT("fswiss "));
                break;
            }
            default :
            {
                TCHAR pchar[30];
                wsprintf(pchar, TEXT("fnil\\fcharset%d "), iCharset); 
                lstrcat(lpstrClipString, pchar);
                break;
            }
        }
    }

    lstrcat(lpstrClipString, szFaceName);

    lstrcat(lpstrClipString, (LPTSTR)achMiddle);

    //
    //  We need to do the text character by character, making sure
    //  that we output a special sequence \'hh for characters bigger
    //  than 7 bits long!
    //
    lpstrClipString = (LPTSTR)(lpstrClipString + lstrlen(lpstrClipString));

    cchUC = 0;
    char achTmp[2];
    char *pTmp = achTmp;
    int cch;

    while (*lpstrText)
    {
      cch = ConvertUnicodeToAnsiFont(hWndDlg, lpstrText, pTmp);
            //
            // Put in a \uc# to tell Unicode reader how many bytes to skip
            // and the \uN code to indicate the real unicode value.
            //
            if (cch != cchUC )
            {
                cchUC = cch;
                lpstrClipString += wsprintf( lpstrClipString,
                    TEXT("\\uc%d"),
                    (INT)(SHORT)cchUC );
            }

            lpstrClipString += wsprintf( lpstrClipString,
                TEXT("\\u%d"),
                (INT)(SHORT)*lpstrText );

            //
            //  Now put the \'xx string in to indicate the actual character.
            //
            lpstrText++;
            while (cch--)
            {
                *lpstrClipString++ = TEXT('\\');
                *lpstrClipString++ = TEXT('\'');
                wsprintf(achMiddle, TEXT("%x"), (INT)*pTmp++);
                *lpstrClipString++ = achMiddle[0];
                *lpstrClipString++ = achMiddle[1];
            }
    }
    *lpstrClipString++ = TEXT('}');
    *lpstrClipString++ = TEXT('\0');

#ifndef UNICODE_RTF
    {
        //
        //  RTF is only defined for ANSI, not for Unicode, therefore
        //  we need to convert the buffer before we put it on the
        //  clipboard.  Eventually, we should add autoconversion code
        //  to USER to handle this for us.
        //
        int cch;
        HANDLE hmemRTFA;
        LPSTR pszRTFA;

        cch = WideCharToMultiByte( CP_ACP,
            0,
            pszRTFW,
            (int)(lpstrClipString - pszRTFW),
            NULL,
            0,
            NULL,
            NULL );

        if (cch != 0 &&
            (hmemRTFA = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,cch)) != NULL)
        {
            pszRTFA = (char*)GlobalLock(hmemRTFA);

            WideCharToMultiByte( CP_ACP,
                0,
                pszRTFW,
                (int)(lpstrClipString - pszRTFW),
                pszRTFA,
                cch,
                NULL,
                NULL );

            GlobalUnlock(hmemRTFA);
            GlobalUnlock(hmemRTF);
            GlobalFree(hmemRTF);

            hmemRTF = hmemRTFA;
        }
    }
#endif

    pMedium->tymed = TYMED_HGLOBAL;

    // Set memory handle by which the data is being transferred
    pMedium->hGlobal = hmemRTF;

    return S_OK;
}

//**********************************************************************
// CImpIDataObject::RenderPlainAnsiText
//
// Purpose:
//      Copies the character from currently selected grid cell to global
//      memory as Ansi Text
//
// Parameters:
//      STGMEDIUM* pMedium  -   Pointer to the STGMEDIUM structure
//
// Return Value:
//      S_OK                - Data was transferred successfully
//**********************************************************************
HRESULT CImpIDataObject::RenderPlainAnsiText(STGMEDIUM* pMedium)
{
    HGLOBAL hText;
    LPSTR   pszText;
    int     nChars;

    TRACE(TEXT("Rendering plain ANSI text\n"));

    // Convert currently selected character from Wide Char to Multibyte
    nChars = WideCharToMultiByte(CP_ACP,
        0,
        m_lpszText,  // the currently selected character
        wcslen(m_lpszText),
        NULL,
        0,
        NULL,
        NULL );

    hText = GlobalAlloc(GMEM_SHARE | GMEM_ZEROINIT, nChars+1);

    if (!hText) {
        return ResultFromScode(E_OUTOFMEMORY);
    }

    // Copy the text to the global memory
    pszText = (LPSTR)GlobalLock(hText);
    nChars = WideCharToMultiByte(CP_ACP,
        0,
        m_lpszText,  // the currently selected character
        wcslen(m_lpszText),
        pszText,
        nChars+1,
        NULL,
        NULL);

    GlobalUnlock(hText);

    // Set the storage medium by which data is transferred as global memory
    pMedium->tymed = TYMED_HGLOBAL;

    // Set memory handle by which the data is being transferred
    pMedium->hGlobal = hText;

    return S_OK;
}

//**********************************************************************
// CImpIDataObject::RenderPlainUnicodeText
//
// Purpose:
//      Copies the character from currently selected grid cell to global
//      memory as Unicode characters
//
// Parameters:
//      STGMEDIUM* pMedium  -   Pointer to the STGMEDIUM structure
//
// Return Value:
//      S_OK                - Data was transferred successfully
//**********************************************************************
HRESULT CImpIDataObject::RenderPlainUnicodeText(STGMEDIUM* pMedium)
{
    HGLOBAL hText;
    LPTSTR  pszText;

    TRACE(TEXT("Rendering plain unicode text\n"));

    hText = GlobalAlloc(GMEM_SHARE | GMEM_ZEROINIT,
        sizeof(WCHAR) * (wcslen(m_lpszText)+1));

    if (!hText) {
        TRACE(TEXT("Unable to alloc global memory\n"));
        return ResultFromScode(E_OUTOFMEMORY);
    }

    // Copy the text to the global memory
    pszText = (LPTSTR)GlobalLock(hText);
    wcscpy(pszText, m_lpszText);
    GlobalUnlock(hText);

    // Set the storage medium by which data is transferred as global memory
    pMedium->tymed = TYMED_HGLOBAL;

    // Set memory handle by which the data is being transferred
    pMedium->hGlobal = hText;

    return S_OK;
}


//**********************************************************************
// CImpIDataObject::SetText
//
// Purpose:
//      Sets the local pointer for text to be copied
//
// Parameters:
//      LPTSTR  lpszText    -   Pointer to text
//
// Return Value:
//      0                   -   Success
//**********************************************************************
int CImpIDataObject::SetText(LPTSTR lpszText)
{
    lstrcpy(m_lpszText, lpszText);
    return 0;
}

