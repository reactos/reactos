//+--------------------------------------------------------------------------
//
//  File:       print.cxx
//
//  Contents:   Print/PageSetup dialog helpers
//
//---------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_PSPOOLER_HXX_
#define X_PSPOOLER_HXX_
#include "pspooler.hxx"
#endif

#ifndef X_PRINTWRP_HXX_
#define X_PRINTWRP_HXX_
#include "printwrp.hxx"
#endif

#ifndef X_MSRATING_HXX_
#define X_MSRATING_HXX_
#include "msrating.hxx" // areratingsenabled()
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_STRING_H_
#define X_STRING_H_
#include "string.h"
#endif

#ifndef X_WINSPOOL_H_
#define X_WINSPOOL_H_
#include "winspool.h"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef WIN16
#ifndef X_HELP_H_
#define X_HELP_H_
#include "help.h"
#endif
#endif // ndef WIN16

#ifndef X_DLGS_H_
#define X_DLGS_H_
#include "dlgs.h"
#endif

#ifndef X_UNICWRAP_HXX_
#define X_UNICWRAP_HXX_
#include "unicwrap.hxx"
#endif

#ifndef X_FCNTL_H_
#define X_FCNTL_H_
#include <fcntl.h>
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"        // cbodyelement::lockfocusrect
#endif

#ifndef X_LOCALE_H_
#define X_LOCALE_H_
#include <locale.h>
#endif

#ifndef X_SHELLAPI_H_
#define X_SHELLAPI_H_
#include <shellapi.h>
#endif

#ifndef X_STDIO_H_
#define X_STDIO_H_
#include <stdio.h>
#endif

#ifndef X_BODYLYT_HXX_
#define X_BODYLYT_HXX_
#include "bodylyt.hxx"
#endif

#ifdef UNIX
#ifndef X_MAINWIN_H_
#define X_MAINWIN_H_
#  include <mainwin.h>
#endif
#endif // UNIX

#ifdef WIN16
#ifndef X_DLGS_H_
#define X_DLGS_H_
#include <dlgs.h>
#endif

#ifndef X_PRINT_H_
#define X_PRINT_H_
#include <print.h>
#endif

#ifndef X_PGSTUP16_HXX_
#define X_PGSTUP16_HXX_
#include <pgstup16.hxx>
#endif

#ifndef X_PAGE_H_
#define X_PAGE_H_
#include <page.h>
#endif

#define BST_UNCHECKED 0x0000
#endif // WIN16

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include <intl.hxx>
#endif

#include "eventobj.h"

DeclareTag(tagContextHelp, "Help", "Context help for dialogs");
MtDefine(PRINTINFOBAG, Printing, "PRINTINFOBAG")
MtDefine(DUMMYJOB, Printing, "DUMMYJOB")
MtDefine(PRINTINFO, Printing, "PRINTINFO")
MtDefine(GetNextToken_ppchOut, Printing, "GetNextToken *ppchOut")

extern DWORD g_dwPlatformID;

static int GetNextToken(const TCHAR *pchIn, TCHAR **ppchOut);
#ifndef WIN16
static HRESULT ReadURLFromFile(TCHAR *pchFileName, TCHAR **ppchURL);
#endif

// helpers for the string manipulation stuff in PrintHTML

static const TCHAR g_achURLDelimiter[] = TEXT("");
static const TCHAR g_achURLPrefix[]    = TEXT("url:");
static const UINT  g_uiURLPrefixLen    = sizeof(g_achURLPrefix) - 1;

#ifdef WIN16
static HRESULT GetPrintFileName(HWND hwnd, WCHAR *pchFileName);
#endif // WIN16

extern CCriticalSection    g_csFile;
extern TCHAR               g_achSavePath[];

//----------------------------------------------------------------------------
//
//  Array mapping driver names to print modes
//
//----------------------------------------------------------------------------

static const DRIVERPRINTMODE s_aPrintDriverPrintModes[] =
{ { "WinFax"               , PRINTMODE_NO_TRANSPARENCY },
  { "OLFAXDRV"             , PRINTMODE_NO_TRANSPARENCY },
  { "NEC  SuperScript 860" , PRINTMODE_NO_TRANSPARENCY },
  { "NEC  SuperScript 1260", PRINTMODE_NO_TRANSPARENCY }
};


void
WriteHeaderFooter(
    HKEY keyPS,
    TCHAR * pszHeader,
    TCHAR * pszFooter)
{
    TCHAR   achEmpty[2] = _T("");
    int     iLen;
    HRESULT hr;

    if (!pszHeader)
        pszHeader = achEmpty;
    if (!pszFooter)
        pszFooter = achEmpty;

    Assert(pszHeader);
    Assert(pszFooter);

    iLen = _tcslen(pszHeader);
#ifdef _UNICODE
        iLen *= sizeof(TCHAR);
#endif
    hr = RegSetValueEx(keyPS,_T("header"),0,REG_SZ,(const byte*)pszHeader,iLen);

    iLen = _tcslen(pszFooter);
#ifdef _UNICODE
        iLen *= sizeof(TCHAR);
#endif
    hr = RegSetValueEx(keyPS,_T("footer"),0,REG_SZ,(const byte*)pszFooter,iLen);
}


void
ConvDoubleToTCHAR(double value,TCHAR* pString)
{
    Assert(pString);
    if (pString == NULL)
        return;
    long   lint,lfract;
    lint = value;
    lfract = (value-lint)*100000.0;
    wsprintf(pString,_T("%lu.%5lu"),lint,lfract);
}


void
ConvTCHARToDouble(double* pValue,TCHAR* pString)
{
    Assert(pString);
    double d = 0.0;
    double dFract;

    if (pString == NULL)
        return;
    int    iLen = _tcslen(pString);
    TCHAR* p = pString;
    int    i;
    int    iChar = 0;

    for (i=0;i<iLen;i++,p++)
        if (*p != _T(' '))
            break;
    for (;i<iLen;i++,p++)
    {
        iChar = *p;
        if ((iChar < _T('0')) || (iChar > _T('9')))
            break;
        d = d*10.0 + (iChar - _T('0'));
    }
    if (iChar == _T('.'))
    {
        p++;
        i++;
        dFract = 10.0;
        for (;i<iLen;i++,p++)
        {
            iChar = *p;
            if ((iChar < _T('0')) || (iChar > _T('9')))
                break;
            iChar -= _T('0');
            d += iChar / dFract;
            dFract *= 10.0;
        }
    }
    *pValue = d;
}


HRESULT
WriteMargin(HKEY keyPS,TCHAR* pValueName,LONG margin)
{
#define MARGINLENGTH 32
    TCHAR   achMargin[MARGINLENGTH];
    double  dwork = margin;

    dwork /= 1000.0;    // in inch
    ConvDoubleToTCHAR(dwork,achMargin);
    return RegSetValueEx(keyPS,pValueName,0,REG_SZ,(const byte*)achMargin,(_tcslen(achMargin)+1)*sizeof(TCHAR));
}


void
WriteMargins(
    HKEY keyPS,
    RECT* prtMargins)
{
    WriteMargin(keyPS,_T("margin_top"),prtMargins->top);
    WriteMargin(keyPS,_T("margin_bottom"),prtMargins->bottom);
    WriteMargin(keyPS,_T("margin_left"),prtMargins->left);
    WriteMargin(keyPS,_T("margin_right"),prtMargins->right);
}


void
ReadHeaderOrFooter(HKEY hfKey,const TCHAR* pValueName,TCHAR* pHeaderFooter)
{
    DWORD   dwLength        = 0;

    Assert(pValueName);
    _tcscpy(pHeaderFooter,_T(""));
    // first try the new style : "header", "footer"
    if (RegQueryValueEx(
                hfKey,
                pValueName,
                NULL,
                NULL,
                NULL,
                &dwLength) == ERROR_SUCCESS)
    {
        if (dwLength > 0)
        {
                RegQueryValueEx(hfKey,pValueName,NULL,NULL,
                                (LPBYTE)pHeaderFooter,&dwLength);
        }
    }
}

void
ReadMargin(HKEY hfKey,TCHAR* pValueName,LONG* pmargin)
{
    Assert(pmargin);
    if (pmargin == NULL)
        return;
#define MARGINLENGTH    32
    DWORD   dwLength        = 0;
    TCHAR   achMargin[MARGINLENGTH];
    double  dwork;

    if (RegQueryValueEx(
                hfKey,
                pValueName,
                NULL,
                NULL,
                NULL,
                &dwLength) == ERROR_SUCCESS)
    {
        if (dwLength > 0)
        {
            if (dwLength > MARGINLENGTH)
                dwLength = MARGINLENGTH;
            RegQueryValueEx(hfKey,pValueName,NULL,NULL,(LPBYTE)achMargin,&dwLength);
            ConvTCHARToDouble(&dwork,achMargin);

            *pmargin = 1000 * dwork;    // in inch
        }
    }
}

void
ReadMargins(HKEY hfKey,RECT* prtMargins)
{
    Assert(prtMargins);
    if (prtMargins == NULL)
        return;
    prtMargins->top = 750;
    prtMargins->bottom = 750;
    prtMargins->left = 750;
    prtMargins->right = 750;
    ReadMargin(hfKey,_T("margin_top"),&(prtMargins->top));
    ReadMargin(hfKey,_T("margin_bottom"),&(prtMargins->bottom));
    ReadMargin(hfKey,_T("margin_left"),&(prtMargins->left));
    ReadMargin(hfKey,_T("margin_right"),&(prtMargins->right));
}

HRESULT
GetParamFromEvent(BSTR pchDispName, VARIANT *pvar, VARTYPE vartype, IHTMLEventObj* pEventObj)
{
    HRESULT         hr;
    DISPID          dispid;
    DISPPARAMS      dispparams = {NULL, NULL, 0, 0};
    IDispatchEx   * pDispatchEx = NULL;
                                       
    Assert(pchDispName && pvar && pEventObj);
    if (!pchDispName || !pvar || !pEventObj)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    VariantInit(pvar);

    hr = pEventObj->QueryInterface(IID_IDispatchEx, (void**)&pDispatchEx);
    if (hr)
        goto Cleanup;

    hr = pDispatchEx->GetDispID(              
            pchDispName,                 
            fdexNameCaseSensitive,           
            &dispid);
    if (hr)
        goto Cleanup;

    hr = pDispatchEx->InvokeEx(
            dispid,            
            LOCALE_USER_DEFAULT,
            DISPATCH_PROPERTYGET,
            &dispparams,
            pvar,
            NULL,
            NULL);
    if (hr)
        goto Cleanup;          

    Assert(V_VT(pvar) == vartype);
    if (V_VT(pvar) != vartype)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pDispatchEx);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:   FixupHwndOwner, static
//
//  Purpose:    comdlg32!PrintDlgExX will fail if the hwndOwner is NULL.
//              PrintDlgExX is called from shdocvw on NT5.
//
//------------------------------------------------------------------------------

HWND
FixupHwndOwner(HWND hwndOwner)
{
    return (   hwndOwner
            || g_dwPlatformID != VER_PLATFORM_WIN32_NT
            || g_dwPlatformVersion < 0x50000)
           ? hwndOwner
           : GetDesktopWindow();
}

//+---------------------------------------------------------------------------
//
//  Function:   FormsPageSetup
//
//  Synopsis:   Set the page up for printing
//
//  Arguments:  hwndOwner    - parent window
//              phDevMode    - pointer to device mode global handle
//              phDevNames   - pointer to device name global handle
//              pptPaperSize - pointer to paper size (in 1/1000 inches)
//              prtMargin    - pointer to margin
//
//  Remarks:    All but the first argument will contain return values,
//              set according to user input in the common dialog.
//
//              If pptPaperSize->Left is -1, the default paper size is
//              used.  Similarly, if prtMargin->x is -1, the default
//              margins are used.
//
//  Returns:    HRESULT.
//
//----------------------------------------------------------------------------

// BUGBUG (cthrash) we currently cannot set headers/footers.

#ifdef WIN16
BOOL DlgPage_RunDialog(PAGESETUPDLG *ppsd);
#endif // WIN16

HRESULT
FormsPageSetup(PRINTINFOBAG *pPrintInfoBag, HWND hwndOwner, CDoc* pDoc)
{
    HRESULT         hr = E_FAIL;
    PAGESETUPDLG    pagesetupdlg;
    HKEY            keyPageSetup = NULL;
    TCHAR           achLocale[32];
    int             iLocale = 32;
    BOOL            fMetricUnits = FALSE;

#ifndef WIN16        
    IHTMLEventObj * pEventObj = NULL; 
    IHTMLWindow2  * pOmWindow = NULL;         
    EVENTPARAM      param(pDoc, TRUE);
    TCHAR           achHeader[1024];
    TCHAR           achFooter[1024];
    VARIANT         varIn;  

    if (!pDoc)
        goto Cleanup;
   
    _tcscpy(achHeader,_T(""));
    _tcscpy(achFooter,_T(""));

    // Set up expandos
    param.SetType(_T("pagesetup"));
    param.pagesetupParams.pchPagesetupHeader    = achHeader;
    param.pagesetupParams.pchPagesetupFooter    = achFooter;       
    param.pagesetupParams.lPagesetupDlg         = (LONG_PTR)&pagesetupdlg;    
#endif // !WIN16

    hwndOwner = FixupHwndOwner(hwndOwner);

    // Fill out PAGESETUPDLG structure    
    ZeroMemory(&pagesetupdlg, sizeof (pagesetupdlg));
    pagesetupdlg.lStructSize = sizeof(pagesetupdlg);
    pagesetupdlg.hwndOwner = hwndOwner;
    pagesetupdlg.hDevMode = pPrintInfoBag->hDevMode;
    pagesetupdlg.hDevNames = pPrintInfoBag->hDevNames;   

    if (pPrintInfoBag->ptPaperSize.x     != -1)
    {
        pagesetupdlg.ptPaperSize = pPrintInfoBag->ptPaperSize;
    }

    if (pPrintInfoBag->rtMargin.left != -1)
    {
#ifndef WIN16
        pagesetupdlg.Flags |= PSD_MARGINS;
#endif // !WIN16
        pagesetupdlg.rtMargin = pPrintInfoBag->rtMargin;
    }
        
    if (GetRegPrintOptionsKey(PRINTOPTSUBKEY_PAGESETUP, &keyPageSetup) 
            == ERROR_SUCCESS)
    {            
#ifndef WIN16
        ReadHeaderOrFooter(keyPageSetup,_T("header"),achHeader);
        ReadHeaderOrFooter(keyPageSetup,_T("footer"),achFooter);                
#endif // !WIN16
        ReadMargins(keyPageSetup,&(pagesetupdlg.rtMargin));
        RegCloseKey(keyPageSetup);
    }          

#ifdef WIN16
    pagesetupdlg.lCustData = (LPARAM)pPrintInfoBag;   
    pagesetupdlg.hInstance = g_hInstResource;
    if (!DlgPage_RunDialog(&pagesetupdlg))
    {
        hr = E_FAIL;
        goto Cleanup;
    }
#else

    Verify( CommCtrlNativeFontSupport() );

#ifdef UNIX
    pagesetupdlg.Flags |= PD_SHOWHELP;
#endif // UNIX

    fMetricUnits = (GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_IMEASURE, achLocale, iLocale)
                   && achLocale[0] == TCHAR('0'));
    if (fMetricUnits)
    {
        // Margins from PrintInfoBag are in 1/1000 inches and need to be converted
        // to 1/100 mm.
        pagesetupdlg.rtMargin.left   = MulDivQuick(pagesetupdlg.rtMargin.left  , 2540, 1000);
        pagesetupdlg.rtMargin.right  = MulDivQuick(pagesetupdlg.rtMargin.right , 2540, 1000);
        pagesetupdlg.rtMargin.top    = MulDivQuick(pagesetupdlg.rtMargin.top   , 2540, 1000);
        pagesetupdlg.rtMargin.bottom = MulDivQuick(pagesetupdlg.rtMargin.bottom, 2540, 1000);
    }              

    // Pass event object pointer to UI Handler
    if (pDoc->get_parentWindow(&pOmWindow))            
        goto Cleanup;      
    if (pOmWindow->get_event(&pEventObj))           
        goto Cleanup;

    V_VT(&varIn) = VT_UNKNOWN;
    V_UNKNOWN(&varIn) = pEventObj;     

    // Query host to show dialog
    if (pDoc->_pHostUICommandHandler && !pDoc->_fOutlook98)
    {         
        hr = pDoc->_pHostUICommandHandler->Exec(        
            &CGID_DocHostCommandHandler,            
            OLECMDID_SHOWPAGESETUP,            
            0,                                
            &varIn,                                
            NULL);

        if (hr != OLECMDERR_E_NOTSUPPORTED && hr != OLECMDERR_E_UNKNOWNGROUP)
            goto UIHandled;
    }
        
    // Otherwise show it ourselves.
    pDoc->EnsureBackupUIHandler();                
    if (pDoc->_pBackupHostUIHandler)                        
    {                
        IOleCommandTarget * pBackupHostUICommandHandler;                                        
        hr = pDoc->_pBackupHostUIHandler->QueryInterface(IID_IOleCommandTarget,                        
            (void **) &pBackupHostUICommandHandler);                                
        if (hr)                                                     
            goto Cleanup;                
                       
        hr = pBackupHostUICommandHandler->Exec(                        
            &CGID_DocHostCommandHandler,                                
            OLECMDID_SHOWPAGESETUP,
            0,                                
            &varIn,                                
            NULL);                                
        ReleaseInterface(pBackupHostUICommandHandler);                        
    }
         
UIHandled:
    if (hr)
        goto Cleanup;
#endif // !WIN16

    pPrintInfoBag->hDevMode = pagesetupdlg.hDevMode;
    pPrintInfoBag->hDevNames = pagesetupdlg.hDevNames;
    pPrintInfoBag->ptPaperSize = pagesetupdlg.ptPaperSize;

    if (fMetricUnits)
    {
        // Margins from Page Setup dialog are in 1/100 mm and need to be converted
        // to 1/1000 inches.
        pagesetupdlg.rtMargin.left   = MulDivQuick(pagesetupdlg.rtMargin.left  , 1000, 2540);
        pagesetupdlg.rtMargin.right  = MulDivQuick(pagesetupdlg.rtMargin.right , 1000, 2540);
        pagesetupdlg.rtMargin.top    = MulDivQuick(pagesetupdlg.rtMargin.top   , 1000, 2540);
        pagesetupdlg.rtMargin.bottom = MulDivQuick(pagesetupdlg.rtMargin.bottom, 1000, 2540);
    }

    if (GetRegPrintOptionsKey(PRINTOPTSUBKEY_PAGESETUP,&keyPageSetup) == ERROR_SUCCESS)
    {

#ifndef WIN16        
        // Attempt to read the header/footer from the event object
        VARIANT         varheader;
        VARIANT         varfooter;                           
                
        if (!GetParamFromEvent(L"pagesetupHeader", &varheader, VT_BSTR, pEventObj) &&            
            !GetParamFromEvent(L"pagesetupFooter", &varfooter, VT_BSTR, pEventObj))
        {                                        
            WriteHeaderFooter(keyPageSetup,V_BSTR(&varheader),V_BSTR(&varfooter));                  
        }

        VariantClear(&varheader);
        VariantClear(&varfooter);         
#endif // !WIN16

        WriteMargins(keyPageSetup,&(pagesetupdlg.rtMargin));
        RegCloseKey(keyPageSetup);
    }
    pPrintInfoBag->rtMargin = pagesetupdlg.rtMargin;        


Cleanup:        

#ifndef WIN16
    ReleaseInterface(pEventObj);    
    ReleaseInterface(pOmWindow);    
#endif // !WIN16

    RRETURN1(hr, S_FALSE);
}


#ifdef WIN16
HGLOBAL Init_DevMode();
BOOL DlgPrnt_RunDialog(PRINTDLG* lppd, BOOL bSetup);
#endif // WIN16

//+---------------------------------------------------------------------------
//
//  Function:   FormsPrint
//
//  Synopsis:   Print a document
//
//  Arguments:  none
//
//  Modifies:   nothing
//
//  NOTE: For WIN16, we are using the IE3.0 print dialog (default
//        Windows print dialog). This is because the printer drop=-down
//        control in the IE40 template is not working. If we need
//        to switch back to the IE40 code, remove the #ifndef WIN16-s
//        below, and remove the call to DlgPrnt_RunDialog().
//----------------------------------------------------------------------------
HRESULT
FormsPrint( 
    PRINTINFOBAG * pPrintInfoBag,    
    HWND hwndOwner,        
    CDoc* pDoc,
    BOOL  fInitDefaults)
{
    HRESULT     hr = E_FAIL;    
    PRINTDLG    printdlg;

#ifdef WIN16
    BOOL WINAPI (*fnPD)(LPPRINTDLG) = UNICODE_FN(PrintDlg);
#else          
    IHTMLEventObj * pEventObj = NULL; 
    IHTMLWindow2  * pOmWindow = NULL;     
    EVENTPARAM      param(pDoc, TRUE);       
    TCHAR           achPrintToFileName[MAX_PATH];
    VARIANT         varIn;              

    if (!pDoc)
        goto Cleanup;

    // Set up expandos
    param.SetType(_T("print"));
    param.printParams.fPrintRootDocumentHasFrameset = pPrintInfoBag->fRootDocumentHasFrameset;
    param.printParams.fPrintAreRatingsEnabled       = (AreRatingsEnabled()== S_OK);           
    param.printParams.fPrintActiveFrame             = pPrintInfoBag->fPrintActiveFrame;            
    param.printParams.fPrintLinked                  = pPrintInfoBag->fPrintLinked;              
    param.printParams.fPrintSelection               = pPrintInfoBag->fPrintSelection;            
    param.printParams.fPrintAsShown                 = pPrintInfoBag->fPrintAsShown;                
    param.printParams.fPrintShortcutTable           = pPrintInfoBag->fShortcutTable;               
    param.printParams.iPrintFontScaling             = pPrintInfoBag->iFontScaling;           
    param.printParams.lPrintDlg                     = (LONG_PTR) &printdlg;  

    // BUGBUG - peterlee 8/6/98
    // need way to access CBodyLayout::SetFocusRect from shdocvw
    // this is just a placehold until that glorious day   
    param.printParams.pPrintBodyActiveTarget        = NULL;   
   
    // Get previous save path
    {
        LOCK_SECTION(g_csFile);
        if (g_achSavePath)         
            _tcscpy(achPrintToFileName, g_achSavePath);
        else    
            _tcscpy(achPrintToFileName, TEXT(""));
    }
    param.printParams.fPrintToFileOk = FALSE;
    param.printParams.pchPrintToFileName = achPrintToFileName;
           
#endif // WIN16

    hwndOwner = FixupHwndOwner(hwndOwner);

    // Fill out PRINTDLG structure
    ZeroMemory( &printdlg, sizeof(printdlg) );
    printdlg.lStructSize = sizeof(printdlg);
    printdlg.hwndOwner = hwndOwner;

        
       
    //  Check if we have a selection
    printdlg.Flags                |= (!pPrintInfoBag->fPrintSelection ? PD_NOSELECTION : 0);
    printdlg.Flags                |= (pPrintInfoBag->fCollate ? PD_COLLATE : 0);
    printdlg.Flags                |= (pPrintInfoBag->fPagesSelected? PD_PAGENUMS : 0);
    printdlg.Flags                |= (pPrintInfoBag->fPrintToFile ? PD_PRINTTOFILE : 0);

    printdlg.nFromPage            = pPrintInfoBag->nFromPage;
    printdlg.nToPage              = pPrintInfoBag->nToPage;
    if (fInitDefaults)
    {
        // this indicates we only want to retrieve the defaults,
        // not to bring up the dialog

        printdlg.hDevMode     = 0;
        printdlg.hDevNames    = 0;
        printdlg.Flags        |= PD_RETURNDEFAULT | PD_RETURNDC;
    }
    else
    {
        printdlg.hDevMode = pPrintInfoBag->hDevMode;
        printdlg.hDevNames = pPrintInfoBag->hDevNames;
        printdlg.hDC = pPrintInfoBag->hdc;
        if (!pPrintInfoBag->hdc)
            printdlg.Flags |= PD_RETURNDC;
    }

    printdlg.nMinPage = 1;
    printdlg.nMaxPage = USHRT_MAX;
    printdlg.nCopies = pPrintInfoBag->nCopies;
    if (pPrintInfoBag->fCollate && printdlg.hDevMode)
    {
        void * p = GlobalLock(printdlg.hDevMode);
        if (p)
        {
#ifndef WIN16
            if (g_fUnicodePlatform)
                ((DEVMODEW*)p)->dmCopies = pPrintInfoBag->nCopies;
            else
                ((DEVMODEA*)p)->dmCopies = pPrintInfoBag->nCopies;
#else
            //bugwin16: Fix g_fUnicodePlatform.
            ((DEVMODE*)p)->dmCopies = pPrintInfoBag->nCopies;
#endif // !WIN16
        }
        GlobalUnlock(printdlg.hDevMode);
    }

#ifdef WIN16
    printdlg.hInstance = g_hInstResource;
    hr = DlgPrnt_RunDialog(&printdlg, FALSE) ? S_OK : E_FAIL;
#else
    Verify( CommCtrlNativeFontSupport() );

    // Pass event object pointer to UI handler
    if (pDoc->get_parentWindow(&pOmWindow))            
        goto Cleanup;      
    if (pOmWindow->get_event(&pEventObj))           
        goto Cleanup;

    V_VT(&varIn) = VT_UNKNOWN;
    V_UNKNOWN(&varIn) = pEventObj;

    // Query host to show dialog
    if (pDoc->_pHostUICommandHandler && !pDoc->_fOutlook98)
    {
        hr = pDoc->_pHostUICommandHandler->Exec(
            &CGID_DocHostCommandHandler,
            OLECMDID_SHOWPRINT,
            0,
            &varIn,
            NULL);

        if (   hr != OLECMDERR_E_NOTSUPPORTED 
            && hr != OLECMDERR_E_UNKNOWNGROUP
            && hr != E_NOTIMPL)
            goto UIHandled;
    }
        
    pDoc->EnsureBackupUIHandler();                
    if (pDoc->_pBackupHostUIHandler)                        
    {                
        IOleCommandTarget * pBackupHostUICommandHandler;                                        
        hr = pDoc->_pBackupHostUIHandler->QueryInterface(IID_IOleCommandTarget,                        
            (void **) &pBackupHostUICommandHandler);                                
        if (hr)                                                     
            goto Cleanup;                
                       
        hr = pBackupHostUICommandHandler->Exec(                        
            &CGID_DocHostCommandHandler,                                
            OLECMDID_SHOWPRINT,                                
            0,                                
            &varIn,                                
            NULL);                                
        ReleaseInterface(pBackupHostUICommandHandler);                        
    }
         
UIHandled:
    if (hr)
        goto Cleanup;    
                     	
    // Read changed values from event object
    VARIANT var;
    if (!GetParamFromEvent(L"printfLinked", &var, VT_BOOL, pEventObj))
        pPrintInfoBag->fPrintLinked = V_BOOL(&var);

    if (!GetParamFromEvent(L"printfActiveFrame", &var, VT_BOOL, pEventObj))
        pPrintInfoBag->fPrintActiveFrame = V_BOOL(&var);

    if (!GetParamFromEvent(L"printfAsShown", &var, VT_BOOL, pEventObj))
        pPrintInfoBag->fPrintAsShown = V_BOOL(&var);

    if (!GetParamFromEvent(L"printfShortcutTable", &var, VT_BOOL, pEventObj))
        pPrintInfoBag->fShortcutTable = V_BOOL(&var);

#ifdef FONTSIZE_BOX       
    if (GetParamFromEvent(L"printiFontScaling", &var, VT_INT, pEventObj))
        pPrintInfoBag->iFontScaling = V_INT(&var);   
#endif

#ifdef UNIX
    if (GetParamFromEvent(L"printdmOrientation", &var, VT_INT, pEventObj))
    {
         LPDEVMODE pDM;             
         pDM= NULL;

         if ((pPrintInfoBag->hDevMode) &&
             (pDM = (LPDEVMODE)GlobalLock(pPrintInfoBag->hDevMode))) 
         {                             
             pDM->dmOrientation = V_INT(&var);                        
             GlobalUnlock(pPrintInfoBag->hDevMode);
         }
    }
#endif // UNIX
    
#endif // WIN16

    pPrintInfoBag->hDevMode = printdlg.hDevMode;
    pPrintInfoBag->hDevNames = printdlg.hDevNames;
    pPrintInfoBag->nCopies = printdlg.nCopies;

    // bugbug       when execute the print script ncopies is sometimes 0
    if (pPrintInfoBag->nCopies < 1)
         pPrintInfoBag->nCopies = 1;

    pPrintInfoBag->fCollate = (!!(printdlg.Flags & PD_COLLATE));
    pPrintInfoBag->fPagesSelected = (!!(printdlg.Flags & PD_PAGENUMS));
    pPrintInfoBag->fPrintSelection = (!!(printdlg.Flags & PD_SELECTION));
    pPrintInfoBag->fAllPages = !(pPrintInfoBag->fPagesSelected || pPrintInfoBag->fPrintSelection);
    pPrintInfoBag->fPrintToFile = (!!(printdlg.Flags & PD_PRINTTOFILE));
    pPrintInfoBag->nFromPage = printdlg.nFromPage;
    pPrintInfoBag->nToPage = printdlg.nToPage;
    pPrintInfoBag->hdc = printdlg.hDC;

    if (pPrintInfoBag->fPrintToFile)
    {
#ifndef WIN16       
        // assume failure, treating as canceling
        hr = S_FALSE;
    
        BOOL ptfOk = FALSE; 
        if (!GetParamFromEvent(L"printToFileOk", &var, VT_BOOL, pEventObj))
            ptfOk = V_BOOL(&var);

        if ((ptfOk) && 
            !GetParamFromEvent(L"printToFileName", &var, VT_BSTR, pEventObj) &&
            V_BSTR(&var))
        {                                              
            TCHAR * pchFullPath = V_BSTR(&var);
            _tcscpy(pPrintInfoBag->achPrintToFileName, pchFullPath);

            // get path only
            WCHAR * pchShortName = wcsrchr(pchFullPath, L'\\');
            if (pchShortName)
            {
                *(pchShortName + 1) = 0;
            }
            else
            {
                *pchFullPath = 0;
            }

            // save path to global
            {       
                LOCK_SECTION(g_csFile);
                _tcscpy(g_achSavePath, pchFullPath);
            }
            hr = S_OK;
        }
        VariantClear(&var);
        
#else
        // now pop up the fileselection dialog and save the filename...
        // this is the only place where we can make this modal
        // we then store this in the printinfo struct
        hr = GetPrintFileName(hwndOwner, pPrintInfoBag->achPrintToFileName);
        if (hr!=S_OK)
            hr = S_FALSE;       // treat as cancelling
#endif
    }


Cleanup:      

#ifndef WIN16
    ReleaseInterface(param.printParams.pPrintBodyActiveTarget);
    ReleaseInterface(pEventObj);    
    ReleaseInterface(pOmWindow);    
#endif // !WIN16

    RRETURN1(hr, S_FALSE);
}


//+----------------------------------------------------------------------
//
//  Function:   InitPrintHandles
//
//  Purpose:    Allocate a DVTARGETDEVICE structure, and initialize
//              it according to the hDevMode and hDevNames.
//              Also allocated an HIC.
//
//  Note:       IMPORTANT: Note that the DEVMODE structure is not wrapped
//              on non-unicode platforms.  (See comments below for details.)
//
//  Returns:    HRESULT
//
//-----------------------------------------------------------------------

HRESULT
InitPrintHandles(
    HGLOBAL hDevMode,
    HGLOBAL hDevNames,
    DVTARGETDEVICE ** pptd,
    HDC * phic,
    DWORD * pdwPrintMode,
    HDC * phdc)
{
    HRESULT hr = S_OK;
    LPDEVMODE pDM = (LPDEVMODE)GlobalLock( hDevMode );  // See 2 "IMPORTANT" comments below.
#ifndef WIN16
    LPDEVMODEA pDMA = (LPDEVMODEA) pDM;
#endif // !WIN16
    LPDEVNAMES pDN = (LPDEVNAMES)GlobalLock( hDevNames );
    DVTARGETDEVICE * ptd = NULL;
    WORD nMaxOffset;
    DWORD dwDevNamesSize, dwDevModeSize, dwPtdSize;
    int nNameLength;
    BOOL fReplaceDC = FALSE;

    Assert(pptd && "Null Target Device pointer passed to InitPrintHandles");

    // IMPORTANT: We have painstakingly
    // converted only the hDevNames parameter and NOT hDevMode (NOT!!!) to have TCHAR
    // members.

    if (!pDM || !pDN)
    {
        hr = E_FAIL;
        goto CleanupTargetDevice;
    }

    nMaxOffset = max( pDN->wDriverOffset, pDN->wDeviceOffset );
    nMaxOffset = max( nMaxOffset, pDN->wOutputOffset );

    nNameLength = _tcslen( (TCHAR *)pDN + nMaxOffset );

    // dw* are in bytes, not TCHARS

    dwDevNamesSize = sizeof(TCHAR) * ((DWORD)nMaxOffset + nNameLength + 1);
#ifndef WIN16
    dwDevModeSize = g_fUnicodePlatform ? ((DWORD)pDM->dmSize + pDM->dmDriverExtra)
                                       : ((DWORD)pDMA->dmSize + pDMA->dmDriverExtra);
#else
    dwDevModeSize = (DWORD)pDM->dmSize + pDM->dmDriverExtra;
#endif // !WIN16

    dwPtdSize = sizeof(DWORD) + dwDevNamesSize + dwDevModeSize;

    ptd = (DVTARGETDEVICE *)CoTaskMemAlloc( dwPtdSize );
    if (!ptd)
    {
        hr = E_OUTOFMEMORY;
        goto CleanupTargetDevice;
    }
    else
    {
        ptd->tdSize = dwPtdSize;

        // This is an ugly trick.  ptd->tdDriverNameOffset and pDN happen
        // to match up, so we just copy that plus the data in one big chunk.
        // Remember, I didn't write this -- this code is based on the OLE2 SDK.

        // Offsets are in characters, not bytes.
        memcpy( &ptd->tdDriverNameOffset, pDN, dwDevNamesSize );
        ptd->tdDriverNameOffset *= sizeof(TCHAR);
        ptd->tdDriverNameOffset += sizeof(DWORD);
        ptd->tdDeviceNameOffset *= sizeof(TCHAR);
        ptd->tdDeviceNameOffset += sizeof(DWORD);
        ptd->tdPortNameOffset *= sizeof(TCHAR);
        ptd->tdPortNameOffset += sizeof(DWORD);

        // IMPORTANT: We are not converting the DEVMODE structure back and forth
        // from ASCII to Unicode on Win95 anymore because we are not touching the
        // two strings or any other member.  Converting the DEVMODE structure can
        // be tricky because of potential and common discrepancies between the
        // value of the dmSize member and sizeof(DEVMODE).  (25155)

        memcpy( (BYTE *)&ptd->tdDriverNameOffset + dwDevNamesSize,
                pDM, dwDevModeSize );

        ptd->tdExtDevmodeOffset = sizeof(DWORD) + dwDevNamesSize;

        *pptd = ptd;
    }

    // 25436: Do we need to have the driver download fonts as bitmaps in order to
    // avoid HP PCL driver bug on Win9x.
    if (   !g_fUnicodePlatform && ptd && phdc && *phdc && pDMA
        && ((DWORD)pDMA->dmSize >= offsetof(DEVMODEA, dmTTOption) + sizeof(short)) // Make sure devmode structure is long enough.  This might be the case on some very old drivers or hp deskjets.
        && (pDMA->dmTTOption == DMTT_DOWNLOAD || pDMA->dmTTOption == DMTT_DOWNLOAD_OUTLINE) ) // Not in DMTT_BITMAP mode, rather in font download mode (PCL driver unlike postscript).
    {
        LPWSTR pchDeviceName = (TCHAR*) (((BYTE*)ptd) + ptd->tdDeviceNameOffset);
        HANDLE hPrinter = 0;

        OpenPrinter(pchDeviceName, &hPrinter, NULL);
        if (hPrinter)
        {
            DWORD  cbBuf = sizeof(DRIVER_INFO_1) + MAX_PATH, cbNeeded = 0;
            LPBYTE pDriverInfo = (LPBYTE) CoTaskMemAlloc(sizeof(DRIVER_INFO_1) + MAX_PATH);

            if (pDriverInfo)
            {
                if (GetPrinterDriverA(hPrinter, NULL, 1, pDriverInfo, cbBuf, &cbNeeded))
                {
                    // HP Printers: First two letters of driver name have to be HP					
                    LPSTR pchDriverName = ((DRIVER_INFO_1A *) pDriverInfo)->pName;
                    if (!strncmp("HP", pchDriverName, 2))
                    {
                        pDMA->dmTTOption = DMTT_BITMAP;
                        LONG lOutput = DocumentProperties(0, hPrinter, pchDeviceName, NULL, (PDEVMODEW) pDMA, DM_IN_BUFFER);
                        if (lOutput)
                            fReplaceDC = TRUE;
                    }
                }

                CoTaskMemFree(pDriverInfo);
            }

            ClosePrinter(hPrinter);
        }
    }

CleanupTargetDevice:

    if (pDM) GlobalUnlock( hDevMode );
    if (pDN) GlobalUnlock( hDevNames );

    if (fReplaceDC && phdc)
    {
        HDC hdc = CreateDCFromDevNames(hDevNames, hDevMode);
        if (hdc)
        {
            DeleteDC(*phdc);
            *phdc = hdc;
        }
    }

#ifndef WIN16
    if (!hr && ptd && phic)
    {
        // Compute the IC, bail if we fail.
        *phic = ::CreateICForPrintingInternal( (LPCTSTR)((char *)ptd + ptd->tdDriverNameOffset),
                         (LPCTSTR)((char *)ptd + ptd->tdDeviceNameOffset),
                         (LPCTSTR)((char *)ptd + ptd->tdPortNameOffset),
                         (DEVMODE *)((char *)ptd + ptd->tdExtDevmodeOffset) );
        if (!(*phic))
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }
#endif // ndef WIN16

    if (pdwPrintMode && ptd)
    {
#ifdef UNIX // IEUNIX: We don't need this. It's used for winfax.
        *pdwPrintMode = 0;
#else
        HANDLE   hPrinter = 0;
        BOOL     fRet = FALSE;
        DWORD    cbBuf = sizeof(DRIVER_INFO_1) + MAX_PATH, cbNeeded = 0;
        LPBYTE   pDriverInfo = (LPBYTE) CoTaskMemAlloc(sizeof(DRIVER_INFO_1) + MAX_PATH);
        int      cDriverCount;
        char   * pchDriverName = NULL;

        *pdwPrintMode = 0;

        if (pDriverInfo)
        {
            fRet = OpenPrinter((TCHAR*) (((BYTE*)ptd) + ptd->tdDeviceNameOffset), &hPrinter, NULL);

            if (fRet && hPrinter)
            {
                fRet = GetPrinterDriverA(hPrinter,
                                         NULL,
                                         1,
                                         pDriverInfo,
                                         cbBuf,
                                         &cbNeeded);
                ClosePrinter(hPrinter);
                if (fRet) 
                {
                    pchDriverName = ((DRIVER_INFO_1A *) pDriverInfo)->pName;
                  
                    for (cDriverCount = ARRAY_SIZE(s_aPrintDriverPrintModes) - 1 ; cDriverCount >= 0 ; cDriverCount--)
                    {
                        if (!strcmp(s_aPrintDriverPrintModes[cDriverCount].achDriverName,pchDriverName))
                        {
                            *pdwPrintMode = s_aPrintDriverPrintModes[cDriverCount].dwPrintMode;
                            break;
                        }
                    }
                }
            }

            CoTaskMemFree(pDriverInfo);
        }
#endif // UNIX
    }


Cleanup:

    RRETURN(hr);
}

//+----------------------------------------------------------------------
//
//  Function:   DeinitPrintHandles
//
//  Purpose:    Free DVTARGETDEVICE structure and HIC allocated by
//              InitPrintHandles.
//
//-----------------------------------------------------------------------

void
DeinitPrintHandles(DVTARGETDEVICE * ptd, HDC hic)
{
    if (ptd)
        CoTaskMemFree( ptd );

    if (hic)
        DeleteDC( hic );
}

//+----------------------------------------------------------------------
//
//  Function:   GetDefaultMargin
//
//  Purpose:    If keyOldValues is not NULL read it from the registry
//              If keyOldValues is NULL, or the read was unsuccessful
//              tries the resource file, if no value in therer
//              uses the hard coded "0.750000" value
//
//  Parameters  keyOldValues    registry key for the margins or NULL
//              pMarginName     "header" or "footer"
//              pMarginValue    where the value goes
//              pcbMargin       length of the value in bytes
//              dwMarginConst   const for getting the margin from the resource file
//
//  Returns:    nothing
//
//-----------------------------------------------------------------------

void
GetDefaultMargin(HKEY keyOldValues,TCHAR* pMarginName,TCHAR* pMarginValue,DWORD* pcbMargin,DWORD dwMarginConst)
{
    DWORD   dwValueType;
    HRESULT result = E_FAIL;

    if (keyOldValues != NULL)
    {
        result = RegQueryValueEx(keyOldValues,pMarginName,NULL,&dwValueType,
                                 (BYTE*)pMarginValue,pcbMargin);

    }

    if (result != ERROR_SUCCESS)
    {
        // try the resource file
        *pcbMargin = LoadString(GetResourceHInst(),dwMarginConst,pMarginValue,*pcbMargin);
        if (*pcbMargin > 0)
            result = ERROR_SUCCESS;
    }

    if (result != ERROR_SUCCESS)
    {
        _tcscpy(pMarginValue,_T("0.750000"));
        *pcbMargin = sizeof(_T("0.750000"));
    }
}
//+---------------------------------------------------------------------------
//
//  Member:     GetDefaultHeaderFooterFromResource
//
//  Synopsis:   Get the default header or footer from resources
//
//  Arguments:  HeaderFooterConst  resource id for header or footer
//              pDefaultHeaderFooter    array to hold the header or footer
//              pcbDefaultHeader        as input it holds the size of the array
//                                      as output it holds the size of the footer or header
//                                      (if ANSI in bytes, if Unicode in chars).
//
//  Returns :   nonzero if successful, zero if the string is not in the resource
//
//----------------------------------------------------------------------------
int
GetDefaultHeaderFooterFromResource(const DWORD HeaderFooterConst,
                                   TCHAR* pDefaultHeaderFooter,
                                   DWORD* pcbDefaultHeaderFooter)
{
    int cbLen;

    cbLen = LoadString(GetResourceHInst(),HeaderFooterConst,pDefaultHeaderFooter,*pcbDefaultHeaderFooter);
    if (cbLen > 0)
    {
#ifdef UNICODE
        cbLen *= sizeof(TCHAR);
        *pcbDefaultHeaderFooter = cbLen;
#endif //UNICODE
    };
    return cbLen;
};

//+---------------------------------------------------------------------------
//
//  Member:     GetDefaultHeaderFooter
//
//  Synopsis:   Get the default header or footer from resources, if it is not exists then : "&w&bPage &p of &P"
//
//  Arguments:  keyOldValues       If Not Null try to read from the IE3 defaults, If NULL or the read
//                                 was not successfull, get it from the resources
//              pValueName         "header" or "footer"
//              pDefault           ptr to the default header or footer
//              pcbDefault         at input it is the size of the array to hold the header-footer,
//                                 at output it is size of default header-footer in bytes
//              pDefaultLiteral    default value if there is no def. in resources
//
//  Returns :   None
//
//----------------------------------------------------------------------------

void
GetDefaultHeaderFooter(HKEY keyOldValues,
                       TCHAR* pValueName,
                       TCHAR* pDefault,
                       DWORD* pcbDefault,
                       DWORD  dwResourceID,
                       TCHAR* pDefaultLiteral)
{
#define MAXDEFAULTLENGTH 80
    DWORD   dwValueType;
    HRESULT result = E_FAIL;
    HRESULT result2 = E_FAIL;
    TCHAR   Left[MAXDEFAULTLENGTH] = _T("");
    TCHAR   Right[MAXDEFAULTLENGTH] = _T("");
    DWORD   cbLeft = MAXDEFAULTLENGTH;
    DWORD   cbRight = MAXDEFAULTLENGTH;
    TCHAR   Name[MAXDEFAULTLENGTH];

    if (keyOldValues != NULL)
    {
        _tcscpy(Name,pValueName);
        _tcscat(Name,_T("_left"));
        result = RegQueryValueEx(keyOldValues,Name,NULL,&dwValueType,
                                 (BYTE*)&Left,&cbLeft);
        if (result != ERROR_SUCCESS)
            cbLeft = 0;
        _tcscpy(Name,pValueName);
        _tcscat(Name,_T("_right"));
        result2 = RegQueryValueEx(keyOldValues,Name,NULL,&dwValueType,
                                (BYTE*)&Right,&cbRight);
        if (result2 != ERROR_SUCCESS)
            cbRight = 0;
    }
    if ((result == ERROR_SUCCESS) || (result2 == ERROR_SUCCESS))
    {
            _tcscpy(pDefault,Left);
            _tcscat(pDefault,_T("&b"));
            _tcscat(pDefault,Right);
#ifndef UNIX
            // cbleft and cbright both includes zero, we need only one
            *pcbDefault = cbLeft + cbRight + sizeof(_T("&b")) - 1;
#else
            *pcbDefault = (_tcslen(pDefault)+1) * sizeof(TCHAR);
#endif
    }
    else
    {
        if (!GetDefaultHeaderFooterFromResource(dwResourceID,pDefault,pcbDefault))
        {
                _tcscpy(pDefault,pDefaultLiteral);
                *pcbDefault     = (_tcslen(pDefaultLiteral) + 1) * sizeof(TCHAR);
        };
    }
}

void
GetDefaultHeader(HKEY keyOldValues,TCHAR* pDefaultHeader,DWORD* pcbDefaultHeader)
{
    GetDefaultHeaderFooter(keyOldValues,_T("header"),pDefaultHeader,pcbDefaultHeader,IDS_DEFAULTHEADER,_T("&w&bPage &p of &P"));
};

//+---------------------------------------------------------------------------
//
//  Member:     GetDefaultFooter
//
//  Synopsis:   Get the default footer from resources, if it is not exists then : "&u&b&d"
//
//  Arguments:  ppDefaultFooter    ptr to the default footer
//              pcbDefaultFooter   size of default footer in bytes
//
//  Returns :   None
//
//----------------------------------------------------------------------------

void
GetDefaultFooter(HKEY keyOldValues,TCHAR* pDefaultFooter, DWORD* pcbDefaultFooter)
{
    GetDefaultHeaderFooter(keyOldValues,_T("footer"),pDefaultFooter,pcbDefaultFooter,IDS_DEFAULTFOOTER,_T("&u&b&d"));
};
//+---------------------------------------------------------------------------
//
//  Member:     GetOldPageSetupValues
//
//  Synopsis:   Try to get the old page setup values from HKEY_LOCAL_MACHINE. If found copies them into
//              HKEY_CURRENT_USER, if not, copies the default values
//
//  Arguments:  None
//
//  Returns :   S_OK or E_FAIL
//
//  Summary :   ---
//
//----------------------------------------------------------------------------
HRESULT
GetOldPageSetupValues(HKEY keyExplorer,HKEY * pkeyPrintOptions)
{
    #define MAXHEADERFOOTERLEN 80
    DWORD   dwDisposition;
    HKEY    keyOldValues;
    TCHAR   DefaultHeader[MAXHEADERFOOTERLEN];
    DWORD   cbHeader = MAXHEADERFOOTERLEN;
    TCHAR   DefaultFooter[MAXHEADERFOOTERLEN];
    DWORD   cbFooter = MAXHEADERFOOTERLEN;
    TCHAR   DefaultMarginTop[MAXHEADERFOOTERLEN];
    DWORD   cbMarginTop = MAXHEADERFOOTERLEN;
    TCHAR   DefaultMarginBottom[MAXHEADERFOOTERLEN];
    DWORD   cbMarginBottom = MAXHEADERFOOTERLEN;
    TCHAR   DefaultMarginLeft[MAXHEADERFOOTERLEN];
    DWORD   cbMarginLeft = MAXHEADERFOOTERLEN;
    TCHAR   DefaultMarginRight[MAXHEADERFOOTERLEN];
    DWORD   cbMarginRight = MAXHEADERFOOTERLEN;
    HRESULT hr=S_OK;

    // Check the IE30 values
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        _T("Software\\Microsoft\\Internet Explorer\\PageSetup"),0,
         KEY_ALL_ACCESS,
        &keyOldValues) != ERROR_SUCCESS)
    {
        keyOldValues = NULL;
    }

    GetDefaultHeader(keyOldValues,(TCHAR*)&DefaultHeader,&cbHeader);
    GetDefaultFooter(keyOldValues,(TCHAR*)&DefaultFooter,&cbFooter);
    GetDefaultMargin(keyOldValues,
                 _T("margin_bottom"),
                 (TCHAR*)&DefaultMarginBottom,
                 &cbMarginBottom,
                 IDS_DEFAULTMARGINBOTTOM);
    GetDefaultMargin(keyOldValues,
                _T("margin_top"),
                (TCHAR*)&DefaultMarginTop,
                &cbMarginTop,
                IDS_DEFAULTMARGINTOP);
    GetDefaultMargin(keyOldValues,
                 _T("margin_left"),
                 (TCHAR*)&DefaultMarginLeft,
                 &cbMarginLeft,
                 IDS_DEFAULTMARGINLEFT);
    GetDefaultMargin(keyOldValues,
                 _T("margin_right"),
                 (TCHAR*)&DefaultMarginRight,
                 &cbMarginRight,
                 IDS_DEFAULTMARGINRIGHT);

    RegCloseKey(keyOldValues);
    keyOldValues = NULL;

    if (RegCreateKeyEx(keyExplorer,
                   _T("PageSetup"),
                   0,
                   NULL,
                   REG_OPTION_NON_VOLATILE,
                   KEY_ALL_ACCESS,
                   NULL,
                   pkeyPrintOptions,
                   &dwDisposition) == ERROR_SUCCESS)
    {
    // create header,footer
    hr = RegSetValueEx(*pkeyPrintOptions,
             _T("header"),
             0,
             REG_SZ,
             (CONST BYTE*)&DefaultHeader,
             cbHeader);
    hr = RegSetValueEx(*pkeyPrintOptions,
             _T("footer"),
             0,
             REG_SZ,
             (CONST BYTE*)&DefaultFooter,
             cbFooter);

    // create margins
    hr = RegSetValueEx(*pkeyPrintOptions,
             _T("margin_bottom"),
             0,
             REG_SZ,
             (CONST BYTE*)DefaultMarginBottom,
             cbMarginBottom);
    hr = RegSetValueEx(*pkeyPrintOptions,
             _T("margin_left"),
             0,
             REG_SZ,
             (CONST BYTE*)DefaultMarginLeft,
             cbMarginLeft);
    hr = RegSetValueEx(*pkeyPrintOptions,
             _T("margin_right"),
             0,
             REG_SZ,
             (CONST BYTE*)DefaultMarginRight,
             cbMarginRight);
    hr = RegSetValueEx(*pkeyPrintOptions,
             _T("margin_top"),
             0,
             REG_SZ,
             (CONST BYTE*)DefaultMarginTop,
             cbMarginTop);

    };
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     GetRegPrintOptionsKey
//
//  Synopsis:   Get handle of requested key under \HKCU\Software\Microsoft\Internet Explorer
//
//  Arguments:  PrintSubKey      - subkey of printoptions root to return key for
//              pkeyPrintOptions - ptr to handle of requested key in registry
//
//  Returns :   S_OK or E_FAIL
//
//  Summary :   First it tries to get the values from "new place"
//              HKEY_CURRENT_USER\Software\Microsoft\Internet Explorer\PageSetup
//              If there is no such a key, it creates it and tries to get the values from "old place"
//              HKEY_LOCAL_MACHINE\Software\Microsoft\Internet Explorer\PageSetup
//              If successful it copies the values into the "new place"
//              If not, it tries to get the values from the registry,
//              If no luck, it uses the hardcoded strings
//              NOTE : If the procedure returns with S_OK, it guaranties that they will be a
//              "new place" with values.
//
//----------------------------------------------------------------------------

HRESULT
GetRegPrintOptionsKey(PRINTOPTIONS_SUBKEY PrintSubKey, HKEY * pkeyPrintOptions)
{
    HKEY     keyExplorer;
    HRESULT  hr = E_FAIL;

    if (RegOpenKeyEx(
                HKEY_CURRENT_USER,
                _T("Software\\Microsoft\\Internet Explorer"),
                0,
                KEY_ALL_ACCESS,
                &keyExplorer) == ERROR_SUCCESS)
    {
        LPTSTR szSubKey = (PrintSubKey == PRINTOPTSUBKEY_MAIN ?
                                        _T("Main") :
                                        _T("PageSetup"));

        if (RegOpenKeyEx(keyExplorer,
                         szSubKey,
                         0,
                         KEY_ALL_ACCESS,
                         pkeyPrintOptions) == ERROR_SUCCESS)
        {
            if (StrCmpIC(szSubKey, _T("PageSetup")) == 0)
            {
                //
                //  For the PageSetup key, we do some additional checks to make
                //  sure that (at least) the header and footer keys exist.
                //

                DWORD dwT;

                if ((RegQueryValueEx(*pkeyPrintOptions, _T("Header"), 0, NULL, NULL, &dwT) == ERROR_SUCCESS) &&
                    (RegQueryValueEx(*pkeyPrintOptions, _T("Footer"), 0, NULL, NULL, &dwT) == ERROR_SUCCESS))
                {
                    // the header and footer keys exist, we're fine
                    hr = S_OK;
                }
                else
                {
                    // whoops.  fall back...
                    hr = GetOldPageSetupValues(keyExplorer, pkeyPrintOptions);
                }
            }
            else
                hr = S_OK;
        }
        else
        {
            if (StrCmpIC(szSubKey, _T("PageSetup")) == 0)
            {
                hr = GetOldPageSetupValues(keyExplorer, pkeyPrintOptions);
            }
        }

        RegCloseKey(keyExplorer);
    }
    return hr;
}


LPTSTR
OptionKey(PRINTOPTION PrintOption)
{
    static LPTSTR s_szTable[] =
    {
        _T("Header"),
        _T("Footer"),
        _T("Print_Background"),
        _T("Print_Shortcuts"),
    };

    Assert(PrintOption >= 0 && PrintOption < ARRAY_SIZE(s_szTable));

    return s_szTable[PrintOption];
};


PRINTOPTIONS_SUBKEY
OptionRoot(PRINTOPTION PrintOption)
{
    static PRINTOPTIONS_SUBKEY s_poptTable[] =
    {
        PRINTOPTSUBKEY_PAGESETUP,   // header
        PRINTOPTSUBKEY_PAGESETUP,   // footer
        PRINTOPTSUBKEY_MAIN,        // print background
        PRINTOPTSUBKEY_MAIN,        // shortcut table
    };

    Assert(PrintOption >= 0 && PrintOption < ARRAY_SIZE(s_poptTable));

    return s_poptTable[PrintOption];
}



CStr *
GetRegPrintOptionString(PRINTOPTION PrintOption)
{
    HKEY    hkeyRoot;
    DWORD   dwLength = 0;
    CStr *  pstr = NULL;

    if (GetRegPrintOptionsKey(OptionRoot(PrintOption), &hkeyRoot) != S_OK)
        goto Cleanup;

    if (RegQueryValueEx(
                hkeyRoot,
                OptionKey(PrintOption),
                0,
                NULL,
                NULL,
                &dwLength) == ERROR_SUCCESS)
    {
        if (dwLength > 0)
        {
            pstr = new CStr;

            if (SUCCEEDED(pstr->ReAlloc(dwLength)))
            {
                        RegQueryValueEx(
                                hkeyRoot,
                                OptionKey(PrintOption),
                                NULL,
                                NULL,
                                (LPBYTE) LPTSTR(*pstr),
                                &dwLength);

                        pstr->SetLengthNoAlloc(dwLength);
            }
        }
    }

    RegCloseKey(hkeyRoot);

Cleanup:
    return pstr;
}


HRESULT
SetRegPrintOptionString(PRINTOPTION PrintOption, TCHAR * psz)
{
    HRESULT hr = E_FAIL;
    HKEY    hkeyRoot;

    if (GetRegPrintOptionsKey(OptionRoot(PrintOption), &hkeyRoot) != S_OK)
        goto Cleanup;

    if (RegSetValueEx(
                hkeyRoot,
                OptionKey(PrintOption),
                0,
                REG_SZ,
                (const BYTE *) psz,
                _tcslen(psz)) == ERROR_SUCCESS)
    {
        hr = S_OK;
    }

    RegCloseKey(hkeyRoot);

Cleanup:
    RRETURN(hr);
}


BOOL
GetRegPrintOptionBool(PRINTOPTION PrintOption)
{
    HKEY    hkeyRoot;
    TCHAR   ach[4];
    DWORD   dwLength = 4 * sizeof(TCHAR);   // where 4 = char count of ach
    BOOL    fRet = FALSE;

    // CONSIDER: another way is to use (pDoc->_pOptionSettings->fShortcutTable)

    if (GetRegPrintOptionsKey(OptionRoot(PrintOption), &hkeyRoot) != S_OK)
        goto Cleanup;

    if (RegQueryValueEx(
                hkeyRoot,
                OptionKey(PrintOption),
                0,
                NULL,
                (LPBYTE) ach,
                &dwLength) == ERROR_SUCCESS)
    {
        if (dwLength)
        {
            fRet = !_tcsicmp(ach, _T("yes"));
        }
    }

    RegCloseKey(hkeyRoot);

Cleanup:
    return fRet;
}


HRESULT
SetRegPrintOptionBool(PRINTOPTION PrintOption, BOOL f)
{
    HRESULT hr = E_FAIL;
    HKEY    hkeyRoot;

    if (GetRegPrintOptionsKey(OptionRoot(PrintOption), &hkeyRoot) != S_OK)
        goto Cleanup;

    if (RegSetValueEx(
                hkeyRoot,
                OptionKey(PrintOption),
                0,
                REG_SZ,
                (const BYTE *)(f ? _T("yes") :  _T("no")),
                f ? 3 : 2) == ERROR_SUCCESS)
    {
        hr = S_OK;
    }

    RegCloseKey(hkeyRoot);

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------
//
//  Function:   GetNextToken
//
//  Purpose:    get a "..." token from a command line
//                  allocates the outstring
//
//  Returns:    index in stringnothing
//
//-----------------------------------------------------------------------
int GetNextToken(const TCHAR *pchIn, TCHAR **ppchOut)
{
    int i = 0, j = 0, len ;

    *ppchOut = 0 ;

    if (!pchIn)
    {
        return 0;
    }

    while ( pchIn[i] && pchIn[i] != '"' )
        i++ ;

    if ( !pchIn[i] )
        return 0 ;

    j=i+1 ;
    while ( pchIn[j] && pchIn[j] != '"' )
        j++ ;

    if ( j > i + 1 )
    {
        len = j - i - 1 ;
        *ppchOut = (TCHAR*) new(Mt(GetNextToken_ppchOut)) TCHAR[(len+1)];
        if ( !(*ppchOut) )
            return 0 ;

        _tcsncpy(*ppchOut,&pchIn[i+1],len);
        (*ppchOut)[len] = '\0' ;
    }
    else
        return 0 ;

    return j ;
}







//+----------------------------------------------------------------------
//
//  Function:   ParseCMDLine
//
//  Purpose:    takes a string, checks if there is a printer
//              driver info in the string, returns
//              URL, DEVNAMES, DEVMODE, and HDC as needed.
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  Arguments:  pchIn [IN]       The command line string to be parsed.
//                               Format: '"<URL>" "<Printer>" "<Driver>" "<Port"'
//              ppchURL [OUT]    The URL found.
//              pHDevNames [OUT] A handle to a DEVNAMES structure based on the
//                               other command line string parameters.
//                               Cannot pass in NULL for this.
//              pHDevMode [OUT]  A handle to a DEVMODE structure based on the
//                               printer and driver in the command line.
//                               Can pass in NULL to prevent generating a
//                               DEVMODE handle and an HDC.
//              pHDC [OUT]       A handle to a DEVICE CONTEXT structure based
//                               on the DEVMODE structure.
//                               Can pass in NULL to prevent generating an HDC.
//
//-----------------------------------------------------------------------
HRESULT
ParseCMDLine(const TCHAR *pchIn, TCHAR **ppchURL, HGLOBAL *pHDevNames, HGLOBAL *pHDevMode, HDC *pHDC)
{
    HRESULT hr = S_OK;
    TCHAR   *pchPrinter;
    TCHAR   *pchPort;
    TCHAR   *pchDriver;
    int     index;
    LPDEVNAMES  pDevNames;

    *ppchURL        = 0;
    *pHDevNames     = 0;
    pchPrinter      = 0;
    pchDriver       = 0;
    pchPort         = 0;

    if (!pchIn)
    {
        goto Cleanup;
    }

    index = GetNextToken(pchIn, ppchURL);

    if (index == 0)
    {
        // this was not a cmd line string, get out
        goto Cleanup;
    }

    // so we should have a filename now, but it maybe
    // an URL file, so get the data out if that is the case

    if (_tcsstr(*ppchURL, TEXT(".url"))
        || _tcsstr(*ppchURL, TEXT(".URL")))
    {
#ifndef WIN16
        // we need to get the string out of the file....
        hr = ReadURLFromFile(*ppchURL, ppchURL);
        if (hr)
#endif
            goto Cleanup;
    }


    if (pchIn[++index])
    {
        // wait...there is more
        index += GetNextToken(&pchIn[index], &pchPrinter);

        if (pchIn[++index])
        {
            // wait...there is more
            index += GetNextToken(&pchIn[index], &pchDriver);

            if (pchIn[++index])
            {
                // wait...there is more
                index += GetNextToken(&pchIn[index], &pchPort);
            }
        }
    }


    if (pchPrinter)
    {
        // allocate a devnames structure....
        index = 3 + _tcslen(pchPrinter) +
            (pchDriver ? _tcslen(pchDriver) : 0) +
            (pchPort ? _tcslen(pchPort) :0 );

        index *= sizeof(TCHAR);

        // this global alloc call does a ZEROINIT

        *pHDevNames = GlobalAlloc(GHND, sizeof(DEVNAMES) + index);
        if (*pHDevNames)
        {
            pDevNames = (LPDEVNAMES) GlobalLock(*pHDevNames);
            if (pDevNames)
            {
                pDevNames->wDriverOffset = sizeof(DEVNAMES) / sizeof(TCHAR);
                _tcscpy(((TCHAR*)pDevNames+pDevNames->wDriverOffset), pchDriver);
                if (pchPrinter)
                {
                    pDevNames->wDeviceOffset = pDevNames->wDriverOffset + _tcslen(pchDriver) + 1;
                    _tcscpy(((TCHAR*)pDevNames+pDevNames->wDeviceOffset), pchPrinter);
                }

                if (pchPort)
                {
                    pDevNames->wOutputOffset= pDevNames->wDeviceOffset ?
                                                  pDevNames->wDeviceOffset + _tcslen(pchPrinter)+1 :
                                                  pDevNames->wDriverOffset + _tcslen(pchDriver)+1;

                    _tcscpy(((TCHAR*)pDevNames+pDevNames->wOutputOffset), pchPort);
                }

                GlobalUnlock(*pHDevNames);

#ifndef WIN16
                // Do we have to retrieve a devmode handle?
                if (pHDevMode)
                {
                    // BUGBUG: Do we need the code in this if-body?  If we don't end up needing this code,
                    // delete it as well as the wrapper of DocumentProperties in src\core\wrappers\winspool.cxx .
                    // We don't need this code iff pHDevMode is always NULL.

                    LPDEVMODE   pDevMode;
                    HANDLE  hPrinter;

                    // Obtain hDevMode.
                    if (OpenPrinter(pchPrinter, &hPrinter, NULL))
                    {
                        LONG lSize = DocumentProperties(0, hPrinter, pchPrinter, NULL, NULL, 0);

                        if (lSize < sizeof(DEVMODE))
                        {
                            Assert(!"Memory size suggested by DocumentProperties is smaller than DEVMODE");

                            lSize = sizeof(DEVMODE);
                        }

                        *pHDevMode = GlobalAlloc(GHND, lSize);

                        if (*pHDevMode)
                        {
                            pDevMode = (LPDEVMODE) GlobalLock(*pHDevMode);

                            if (pDevMode)
                            {
                                DocumentProperties(0, hPrinter, pchPrinter, pDevMode, NULL, DM_OUT_BUFFER);

                                // Do we have to retrieve a device context handle?
                                if (pHDC)
                                {
                                    // Obtain hdc.
                                    *pHDC = CreateDCForPrintingInternal(pchDriver, pchPrinter, NULL, pDevMode);
                                }

                                GlobalUnlock(*pHDevMode);
                            }
                            else
                            {
                                // error case
                                GlobalFree(*pHDevMode);
                                *pHDevMode = 0;
                            }
                        }

                        ClosePrinter(hPrinter);
                    }
                }
#endif // !WIN16
            }
            else
            {
                // error case
                GlobalFree(*pHDevNames);
                *pHDevNames = 0;
            }
        }
    }


Cleanup:

    delete pchPort;
    delete pchDriver;
    delete pchPrinter;

    return hr;

}


#ifdef WIN16
HDC WINAPI
CreateDCForPrintingInternal(
        LPCWSTR             lpszDriver,
        LPCWSTR             lpszDevice,
        LPCWSTR             lpszOutput,
        CONST DEVMODE *    lpInitData);
#endif // WIN16


//+---------------------------------------------------------------------------
//
//  Member:     CreateDCFromDevNames, public
//
//  Synopsis:   creates a printer DC based on a DevMode structure
//
//  Returns:
//
//----------------------------------------------------------------------------
HDC CreateDCFromDevNames(HGLOBAL hDevNames, HGLOBAL hDevMode)
{
    HDC hdc=0;
    LPDEVNAMES  pDevNames=0;
    LPDEVMODE   pDevMode=0;


    if (!hDevNames)
    {
        goto Cleanup;
    }

    pDevNames = (LPDEVNAMES) GlobalLock(hDevNames);
    if (!pDevNames)
        goto Cleanup;

    if (hDevMode)
    {
        pDevMode = (LPDEVMODE) GlobalLock(hDevMode);
    }

    hdc = CreateDCForPrintingInternal((LPCTSTR)pDevNames + pDevNames->wDriverOffset,
                     (LPCTSTR)pDevNames + pDevNames->wDeviceOffset,
                     (LPCTSTR)pDevNames + pDevNames->wOutputOffset,
                     pDevMode);


Cleanup:
    if (pDevNames)
    {
        GlobalUnlock(hDevNames);
    }
    if (pDevMode)
    {
        GlobalUnlock(hDevMode);
    }
    return hdc;
}





//+---------------------------------------------------------------------------
//
//  Member:     PrintMSHTML, public
//
//  Synopsis:   Prints the passed in URL
//              called as a helper from the Shell
//
//  Returns:    like WinMain
//
//----------------------------------------------------------------------------
#ifdef UNIX //UNIX Shell32 uses WCHAR.
extern "C"
int WINAPI PrintHTML(HINSTANCE hInst, HINSTANCE hPrevInst, LPTSTR lpURLToPrint, int nNotUsed)
#else
int WINAPI PrintHTML(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpURLToPrint, int nNotUsed)
#endif
{

    // we can ignore all the stuff that is passed in beside the lpURLToPrint...
    // which can be either
    //  : a URL file spec of the form "<URL>"
    //  : or a string containing "<URL>" "<PRINTER>" "<DRIVER>" "<PORT>"

    HRESULT             hr = E_FAIL;
    Assert(lpURLToPrint);

    CoInitialize(NULL);

    // Open scope for thread state manager, cets.
    {

        CEnsureThreadState  cets;
        TCHAR               achBuff[pdlUrlLen];
        CDoc                *pTempDoc=0;

        if (FAILED(cets._hr))
        {
            goto Cleanup;
        }

        if (!lpURLToPrint)
        {
            goto Cleanup;
        }


        hr = InitDocClass();
        if (hr)
            goto Cleanup;

        // the easiest and most consistent way to do this is to:
        // create a new doc..
        // call DoPrint on it...

        pTempDoc = new CDoc(0);

        if (!pTempDoc)
        {
            goto Cleanup;
        }

        if (FAILED(pTempDoc->Init()))
        {
            goto Cleanup;
        }
#ifndef UNIX
                // if we got that far, all that's left to do is to convert the string...
        memset(achBuff, 0, pdlUrlLen*sizeof(TCHAR));
        if (MultiByteToWideChar(CP_ACP, 0, (const char*)lpURLToPrint, -1,achBuff, pdlUrlLen-1))
        {
            hr = pTempDoc->DoPrint(achBuff,0,PRINT_WAITFORCOMPLETION | PRINT_RELEASESPOOLER | PRINT_PARSECMD | PRINT_NOGLOBALWINDOW);
        }
#else
        hr = pTempDoc->DoPrint(lpURLToPrint,0,PRINT_WAITFORCOMPLETION | PRINT_RELEASESPOOLER | PRINT_PARSECMD | PRINT_NOGLOBALWINDOW);
#endif // UNIX

    Cleanup:
        if (pTempDoc)
        {
            pTempDoc->Release();
        }

    }

    CoUninitialize();

    return (hr==S_OK);
}





#ifndef WIN16
//+---------------------------------------------------------------------------
//
//  Member:     ReadURLFromFile
//
//  Synopsis:   helper to read the URL out of a shortcut file
//
//  Returns:
//
//----------------------------------------------------------------------------
HRESULT ReadURLFromFile(TCHAR *pchFileName, TCHAR **ppchURL)
{
    HRESULT  hr = E_FAIL;
    IPersistFile *pPF = 0;
    IUniformResourceLocator * pUR = 0;
    TCHAR *pchNew = 0;

    if (!*ppchURL)
    {
        goto Cleanup;
    }

    hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                           IID_IPersistFile, (void **)&pPF);
    if (hr)
        goto Cleanup;

    hr = pPF->Load(pchFileName,0);
    if (hr)
        goto Cleanup;

    hr = pPF->QueryInterface(IID_IUniformResourceLocator, (void**)&pUR);
    if (hr)
        goto Cleanup;

    hr = pUR->GetURL(&pchNew);
    if (!hr)
    {
        // If pre-allocated buffer is too small, re-alloc it.
        size_t  ilen  =  _tcslen(pchNew); 
        if  (_tcslen(*ppchURL)  <  ilen)
        {
            delete  *ppchURL;
            *ppchURL  =  new  TCHAR[ilen  +  1];
        }
        _tcscpy(*ppchURL, pchNew);
    }

Cleanup:
    ReleaseInterface(pPF);
    ReleaseInterface(pUR);
    //delete pchNew; 
    CoTaskMemFree(pchNew);
    RRETURN(hr);
}
#endif // ndef WIN16




#ifdef WIN16
UINT APIENTRY PrintToFileHookProc(HWND hdlg,
                              UINT uiMsg,
                              WPARAM wParam,
                              LPARAM lParam)
{
    switch (uiMsg)
    {
        case WM_INITDIALOG:
        {
            int      cbLen;
            TCHAR    achOK[MAX_PATH];

            // change "save" to "ok"
            cbLen = LoadString(GetResourceHInst(),IDS_PRINTTOFILE_OK,achOK,MAX_PATH);
            if (cbLen < 1)
                _tcscpy(achOK,_T("OK"));

    //        SetDlgItemText(hdlg, IDOK, _T("OK"));
            SetDlgItemText(hdlg, IDOK, achOK);

            // ...and, finally force us into foreground (needed for Win95, Bug : 13368)
            ::SetForegroundWindow(hdlg);
            break;
        }
    }
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     GetPrintFileName
//
//  Synopsis:   Opens up the customized save file dialog and gets
//              a filename for the printoutput
//  Returns:
//
//----------------------------------------------------------------------------
HRESULT GetPrintFileName(HWND hwnd, TCHAR *pchFileName)
{
    OPENFILENAME    openfilename;
    int             cbLen;
    TCHAR           achTitlePrintInto[MAX_PATH];
    TCHAR           achFilePrintInto[MAX_PATH];
    TCHAR           achFilter[MAX_PATH];
    TCHAR           achPath[MAX_PATH];

    HRESULT         hr = E_FAIL;

    memset(&openfilename,0,sizeof(openfilename));
    openfilename.lStructSize = sizeof(openfilename);
    openfilename.hwndOwner = hwnd;

    cbLen = LoadString(GetResourceHInst(),IDS_PRINTTOFILE_TITLE,achTitlePrintInto,MAX_PATH);
    Assert(cbLen && "could not load the resource");

    if (cbLen > 0)
        openfilename.lpstrTitle = achTitlePrintInto;

    // guarantee trailing 0 to terminate the filter string
    memset(achFilter, 0, sizeof(TCHAR)*MAX_PATH);
    cbLen = LoadString(GetResourceHInst(),IDS_PRINTTOFILE_SPEC,achFilter,MAX_PATH-2);
    Assert(cbLen && "could not load the resource");

    if (cbLen>0)
    {
        for (; cbLen >= 0; cbLen--)
        {
            if (achFilter[cbLen]==_T(','))
            {
                achFilter[cbLen] = 0;
            }
        }
    }


    openfilename.nMaxFileTitle = _tcslen(openfilename.lpstrTitle);
    _tcscpy(achFilePrintInto,_T(""));
    openfilename.lpstrFile = achFilePrintInto;
    openfilename.nMaxFile = MAX_PATH;
    openfilename.Flags = OFN_NOREADONLYRETURN | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT |
                        OFN_ENABLEHOOK | OFN_NOCHANGEDIR;
    openfilename.lpfnHook = PrintToFileHookProc;
    openfilename.lpstrFilter = achFilter;
    openfilename.nFilterIndex = 1;

    {
        LOCK_SECTION(g_csFile);

        _tcscpy(achPath, g_achSavePath);
        openfilename.lpstrInitialDir = *achPath ? achPath : NULL;
    }

    if (GetSaveFileName(&openfilename))
    {
        LOCK_SECTION(g_csFile);

        _tcscpy(g_achSavePath, openfilename.lpstrFile);

        TCHAR * pchShortName =_tcsrchr(g_achSavePath, _T('\\'));

        if (pchShortName)
        {
            *(pchShortName + 1) = 0;
        }
        else
        {
            *g_achSavePath = 0;
        }

        _tcscpy(pchFileName, achFilePrintInto);
        hr = S_OK;
    }
    else
    {
        WHEN_DBG(DWORD dwError = CommDlgExtendedError();)
    }

    return hr;
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     VerifyPrintFileName
//
//  Synopsis:   Verifies that the given filename does not yet exists
//              If it does it modifes the passed in name
//              like: print.ps will be print.1 to print.xxx
//  Returns:    S_OK if everything is fine
//
//----------------------------------------------------------------------------
HRESULT VerifyPrintFileName(TCHAR *pchFileName)
{
    int         iCounter=1;
    HRESULT     hr = E_FAIL;
    TCHAR       achNr[MAX_PATH];
    TCHAR       achFileName[MAX_PATH];
    char        achTemp[MAX_PATH];
    HFILE       hFile;
    OFSTRUCT    ofstruct;
    TCHAR       *pchFound;
    TCHAR       *pchTmp;

    _tcscpy(achFileName, pchFileName);
    while (hr == E_FAIL)
    {
        if (MbcsFromUnicode(achTemp, MAX_PATH, achFileName)==0)
            return S_FALSE;

        hFile = OpenFile(achTemp, &ofstruct, OF_EXIST);
        if (hFile == -1)
        {
            hr = S_OK;
        }
        else if (iCounter < 9999)
        {
            // try to find the extension dot...
            pchFound = achFileName;
            while (pchFound && (pchTmp = _tcschr(pchFound, '.')) != NULL)
            {
                pchFound = ++pchTmp;
            }

            // now we are either at the last dot, or the first character (if there was no dot)
            if (pchFound != achFileName)
            {
                // slam a null in the dot position
                *(--pchFound)= 0;
            }
            // append a dot
            _tcscat(achFileName, TEXT("."));

            // now append a number

            _itot(iCounter, achNr, 10);
            _tcscat(achFileName, achNr);
            iCounter++;
        }
        else
        {
            hr = S_FALSE;
        }
    }
    if (hr==S_OK)
    {
        _tcscpy(pchFileName, achFileName);
    }

    return hr;
}


UINT
GetHTMLTempFileName(const TCHAR *pchPathName, const TCHAR *pchPrefixString, UINT uUnique, TCHAR *pchTempFileName, UINT uAttempts)
{
    unsigned int nTempFileLength = 0;
    TCHAR        achTMPFileName[MAX_PATH];
    DWORD        dwHTMFileExists = 0;
    UINT         dwRet = 0;
    LPTSTR       pDummy = NULL;

    Assert(pchPathName && pchPrefixString && pchTempFileName);

    dwRet = GetTempFileName( pchPathName, pchPrefixString, uUnique, achTMPFileName );
    if (!dwRet)
        goto Cleanup;

    _tcscpy(pchTempFileName, achTMPFileName);
    nTempFileLength = _tcslen(pchTempFileName);

    if ((nTempFileLength >= 4)
        && !_tcsicmp(pchTempFileName + nTempFileLength - 4, _T(".TMP")))
    {
        // Replace the extension.
        _tcscpy(pchTempFileName + nTempFileLength - 3, _T("HTM"));

        dwHTMFileExists = SearchPath(pchPathName, pchTempFileName + _tcslen(pchPathName), NULL, 0, NULL, &pDummy);

        if (dwHTMFileExists <= 0)
        {
            // We are going to use the HTM file because no file with this name exists.
            // Delete the .TMP file because we will be using the .HTM filename instead.
            DeleteFile(achTMPFileName);
        }
        else
        {
            // The same filename with an HTM extension already exists.
            if (uAttempts > 0)
            {
                dwRet = GetHTMLTempFileName( pchPathName, pchPrefixString, uUnique, pchTempFileName, uAttempts-1 );

                // If this isn't our first attempt, make sure we delete the previous TMP-file
                // now.  (We kept this file to make sure we just picked a different one above.)
                DeleteFile(achTMPFileName);
            }
            else
            {
                // Use the TMP file because the same filename with an HTM extension
                // already exists and we have exhausted our attempt quota.
                _tcscpy(pchTempFileName, achTMPFileName);
            }
        }
    }

Cleanup:

    return dwRet;
}

PRINTINFOBAG::PRINTINFOBAG(void)
{
    HKEY    keyPageSetup = NULL;
    if (GetRegPrintOptionsKey(PRINTOPTSUBKEY_PAGESETUP, &keyPageSetup) == ERROR_SUCCESS)
    {
        ReadMargins(keyPageSetup,&(rtMargin));
        RegCloseKey(keyPageSetup);
    }
    else
    {
        rtMargin.left = rtMargin.top = rtMargin.right = rtMargin.bottom = 750; // in 1/1000 inch (3/4'')
    }
}
