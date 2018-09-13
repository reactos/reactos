//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       fprint.cxx
//
//  Contents:   Contains the printing code of doc
//
//  Classes:    CDoc (partial)
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

#ifndef X_FPRINT_HXX_
#define X_FPRINT_HXX_
#include "fprint.hxx"
#endif

#ifndef X_DOCPRINT_HXX_
#define X_DOCPRINT_HXX_
#include "docprint.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_PUTIL_HXX_
#define X_PUTIL_HXX_
#include "putil.hxx"
#endif

#ifndef X_PSPOOLER_HXX_
#define X_PSPOOLER_HXX_
#include "pspooler.hxx"
#endif

#ifndef X_HEADFOOT_HXX_
#define X_HEADFOOT_HXX_
#include "headfoot.hxx"
#endif

#ifndef X_SHELLAPI_H_
#define X_SHELLAPI_H_
#include <shellapi.h>
#endif

#ifndef X_PRINTWRP_HXX_
#define X_PRINTWRP_HXX_
#include "printwrp.hxx"
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include "shell.h"
#endif

#ifndef X_DLGS_H_
#define X_DLGS_H_
#include "dlgs.h"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"      // PrintHeaderFooter()
#endif

#ifndef X_MSRATING_HXX_
#define X_MSRATING_HXX_
#include "msrating.hxx" // AreRatingsEnabled()
#endif

#ifndef X_ELINK_HXX_
#define X_ELINK_HXX_
#include "elink.hxx"    // GetAlternatePrintDoc()
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_PROGSINK_HXX_
#define X_PROGSINK_HXX_
#include "progsink.hxx" // CProgSink::WaitingForNothingButControls()
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"    // for frame focus rect
#endif

#ifndef X_CODEPAGE_H_
#define X_CODEPAGE_H_
#include "codepage.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

DeclareTag(tagPrintToTempFoo, "Print", "Print to c:\\temp\\foo.ps")
DeclareTag(tagPrintToTempFoo2, "Print", "Print to d:\\temp\\foo.ps")
DeclareTag(tagPrintBackground, "Print", "Print background")
DeclareTag(tagPrintBringUpDialog, "Print", "Print Always Bring Up Print Dialog")
DeclareTag(tagPrintKeepTempFile, "Print", "Save a copy of the temp file to c:\\ee.htm")

ExternTag(tagPaginate);

MtDefine(CPrintDoc, Printing, "CPrintDoc")
MtDefine(CPrintDoc_paryPrintPage, CPrintDoc, "CPrintDoc::_paryPrintPage")
MtDefine(CPrintDoc_paryPrintPage_pv, CPrintDoc, "CPrintDoc::_paryPrintPage::_pv")
MtDefine(CPrintDocReadNewStyleHeaderOrFooter_pHeaderFooter, Locals, "CPrintDoc::ReadNewStyleHeaderOrFooter pHeaderFooter")
MtDefine(CPrintDocReadOldStyleHeaderOrFooter_ppszHeaderFooter, Locals, "CPrintDoc::ReadOldStyleHeaderOrFooter *ppszHeaderFooter")
MtDefine(CPrintDocReadOldStyleHeaderOrFooter_pWork, Locals, "CPrintDoc::ReadOldStyleHeaderOrFooter pWork")

#define PRINTDRT_FILE _T("c:\\printdrt.ps")

BOOL g_fFoundOutIfATMIsInstalled;
BOOL g_fATMIsInstalled;

//
// NOTE(SujalP + OliverSe): This code is safe in a multithreaded environment because in the
// worst case multiple threads will end up calling GetPrivateProfileStringA, but they will
// all drive the same information -- either it is installed or not.
//
static void
FindIfATMIsInstalled()
{
    if (!g_fFoundOutIfATMIsInstalled)
    {
        if (!g_fUnicodePlatform)
        {
            char szReturned[2];
            GetPrivateProfileStringA("Boot",
                                     "atm.system.drv",
                                     "*",
                                     szReturned,
                                     sizeof(szReturned),
                                     "system.ini"
                                    );
            g_fATMIsInstalled = szReturned[0] != '*';
        }
        else
        {
            g_fATMIsInstalled = FALSE;
        }
        g_fFoundOutIfATMIsInstalled = TRUE;
    }
}

BOOL
IsATMInstalled()
{
    FindIfATMIsInstalled();
    return g_fATMIsInstalled;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::UpdatePrinter, public
//
//  Synopsis:   Updates the default printer as a reaction to  WM_WININICHANGE
//
//  Returns:    void
//
//----------------------------------------------------------------------------

void
CDoc::UpdateDefaultPrinter(void)
{
    if (IsSpooler()==S_OK)
    {
        CSpooler    *pSpooler;
        HRESULT hr = THR(GetSpooler(&pSpooler));
        if (!hr)
        {
            Assert(pSpooler);

            pSpooler->UpdateDefaultPrinter();
        }
    }
    return;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::DoPrint, public
//
//  Synopsis:   Prints the document
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CDoc::DoPrint(const TCHAR *pchInput, long lRecursive, DWORD dwFlags, SAFEARRAY *psaHeaderFooter)
{
#ifndef NO_PRINT
    HRESULT         hr;
    ULONG           ulDummy;
    HWND            hwnd;
    CSpooler    *   pSpooler;
    PRINTINFOBAG    PrintInfoBag;
    PRINTINFO *     ppiCurrentPrintInfo;
    unsigned        fExtJobsComplete = FALSE;
    BOOL            fWaitForPluginsToPrint = FALSE;
    EVENT_HANDLE    hEvent=0;
    DWORD           dwResult;
    TCHAR           achTempPath[MAX_PATH];
    TCHAR           achTempFileName[MAX_PATH];
    TCHAR           achOverridePrintUrl[pdlUrlLen];
    CDoc *          pDocToPrint = pchInput ? NULL : this;
    CDoc *          pActiveFrameDoc = NULL;
    CMarkup *       pActiveFramePrimaryMarkup = NULL;
    IStream *       pStream = NULL;
    IPrint *        pPrint = NULL;
    TCHAR *         pchPath = 0;
    const TCHAR *   pchURL = 0;
    HGLOBAL         hDevNames = 0, hDevMode = 0;
    HDC             hdc = 0;
    CODEPAGE        codepageFamily;

    // Initialize override print url to url or blank page.
    if (pDocToPrint && pDocToPrint->_cstrUrl.Length())
        _tcscpy(achOverridePrintUrl, pDocToPrint->_cstrUrl);
    else
        _tcscpy(achOverridePrintUrl, _T("about:blank"));

    if (!pchInput && S_OK == PrintPlugInSite())
    {
        // If the control of in the plugin site printed successfully, we are done.
        hr = S_OK;
        goto Cleanup;
    }

#if DBG==1
        if (IsTagEnabled(tagPrintBringUpDialog) && (dwFlags & PRINT_DONTBOTHERUSER))
        {
            dwFlags = dwFlags ^ PRINT_DONTBOTHERUSER;
        }
#endif // DBG == 1

    if (dwFlags & PRINT_DRTPRINT)
    {
        dwFlags |= PRINT_WAITFORCOMPLETION | PRINT_HTMLONLY;
    }

    // parse the pchURL to figure out if the string
    // contains a printer name, which can happen
    // if the method was called from the shell API
    // printHMTL

    if (dwFlags & PRINT_PARSECMD)
    {
        hr = ParseCMDLine(pchInput, &pchPath, &hDevNames, (dwFlags & PRINT_DRTPRINT)?&hDevMode:NULL, (dwFlags & PRINT_DRTPRINT)?&hdc:NULL);

        if (hr)
        {
            goto Cleanup;
        }
    }

    // work with pchUrl in the rest of the method
    pchURL = pchPath ? pchPath : pchInput;

    // If ratings are enabled, refuse to print anything we don't already know about.
    if (pchURL && (S_OK == AreRatingsEnabled()))
    {
        // BUGBUG: If the ratings people ever convince us to bring up some UI, this is the
        // place.  That UI can potentially also provide the option to continue as normal
        // after typing in a password.
        hr = E_FAIL;
        goto Cleanup;
    }

    // Fire up the print spooler
    // Get the spooler
    hr = THR(GetSpooler(&pSpooler));
    if (hr)
        goto Cleanup;

    Assert(pSpooler);

    hr = pSpooler->GetPrintInfo(&PrintInfoBag);
    if (hr)
        goto Cleanup;

    // we are clearing this to force to get the values from the registry, unless
    // psaHeaderFooter has been set (by Athena)
    PrintInfoBag.strHeader.Free();
    PrintInfoBag.strFooter.Free();

    if (psaHeaderFooter)
    {
        // Read print parameters
        // 1. header string
        // 2. footer string
        // 3. Outlook Express header document IStream *
        // 4. alternate URL string (used for MHTML in OE)
        // 5. dwFlags to be ORed in

        // so we got a safearray passed in to replace the standard header/footer strings
        // if one of the strings is 0, we will still get the other one out of the registry
        VARIANT varstr;
        VARIANT varStream;
        VARIANT varFlags;
        long    lArg;

        if (SafeArrayGetDim(psaHeaderFooter) != 1
         || psaHeaderFooter->rgsabound[0].cElements < 2
         || psaHeaderFooter->rgsabound[0].cElements > 5)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        VariantInit(&varstr);

        for (lArg = 0; lArg < 2; ++lArg)
        {
            VariantInit(&varstr);

            hr = SafeArrayGetElement(psaHeaderFooter, &lArg, &varstr);

            if (hr == S_OK)
            {
                hr = VariantChangeType(&varstr, &varstr, NULL, VT_BSTR);

                if (hr == S_OK && V_BSTR(&varstr))
                {
                    if (lArg == 0)
                    {
                        PrintInfoBag.strHeader.Set(V_BSTR(&varstr));
                    }
                    else
                    {
                        PrintInfoBag.strFooter.Set(V_BSTR(&varstr));
                    }
                }
                VariantClear(&varstr);
            }
        }

        if (psaHeaderFooter->rgsabound[0].cElements > 2)
        {
            // Obtain OE Header stream.
            VariantInit(&varStream);
            lArg = 2;
            hr = SafeArrayGetElement(psaHeaderFooter, &lArg, &varStream);

            if (hr == S_OK && V_UNKNOWN(&varStream))
            {
                pStream = (IStream *)V_UNKNOWN(&varStream);
            }
        }

        if (psaHeaderFooter->rgsabound[0].cElements > 3)
        {
            // Obtain OE MHTML Url.
            VariantInit(&varstr);
            lArg = 3;
            hr = SafeArrayGetElement(psaHeaderFooter, &lArg, &varstr);

            if (hr == S_OK && V_BSTR(&varstr) && _tcslen(V_BSTR(&varstr)))
            {
                _tcscpy(achOverridePrintUrl, V_BSTR(&varstr));
                pchURL = achOverridePrintUrl;
                pDocToPrint = NULL;
            }
        }

        if (psaHeaderFooter->rgsabound[0].cElements > 4)
        {
            // Obtain dwFlags and OR them in.
            VariantInit(&varFlags);
            lArg = 4;
            hr = SafeArrayGetElement(psaHeaderFooter, &lArg, &varFlags);

            if (hr == S_OK && V_VT(&varFlags) == VT_I4 && V_I4(&varFlags) != 0)
            {
                dwFlags = dwFlags | ((DWORD) V_I4(&varFlags));
            }
        }
    }

    if (dwFlags & PRINT_WAITFORCOMPLETION)
    {
        hEvent = CreateEvent(0, TRUE, FALSE, 0);
        Assert(hEvent);
    }

    //  need to snapshot the registry-based options right now at enqueue time...

    if (PrintInfoBag.strHeader.Length()==0)
    {
        CStr* pstrHeaderFooter;
        pstrHeaderFooter = GetRegPrintOptionString(PRINTOPT_HEADER);
        if (pstrHeaderFooter)
        {
            PrintInfoBag.strHeader.Set(*pstrHeaderFooter);
            delete pstrHeaderFooter;
        }
    }

    if (PrintInfoBag.strFooter.Length()==0)
    {
        CStr* pstrHeaderFooter;
        pstrHeaderFooter = GetRegPrintOptionString(PRINTOPT_FOOTER);
        if (pstrHeaderFooter)
        {
            PrintInfoBag.strFooter.Set(*pstrHeaderFooter);
            delete pstrHeaderFooter;
        }
    }

    // Set print info bag defaults needed by print dialog.
    PrintInfoBag.fShortcutTable  = GetRegPrintOptionBool(PRINTOPT_SHORTCUT_TABLE);
    PrintInfoBag.fBackground     = GetRegPrintOptionBool(PRINTOPT_PRINT_BACKGROUND);
    PrintInfoBag.fAllPages       = !(PrintInfoBag.fPrintSelection || PrintInfoBag.fPagesSelected);
    PrintInfoBag.fDrtPrint       = !!(dwFlags & PRINT_DRTPRINT);
    PrintInfoBag.fSpoolerEmptyBeforeEnqueue = pSpooler->IsEmpty();
    PrintInfoBag.fRootDocumentHasFrameset = _fFrameSet && !pchURL;
    PrintInfoBag.fPrintActiveFrame = (THR(GetActiveFrame(achOverridePrintUrl, ARRAY_SIZE(achOverridePrintUrl), &pActiveFrameDoc, &pPrint)) == S_OK) && (this != pActiveFrameDoc);
    PrintInfoBag.fPrintSelection = !pchURL && (HasTextSelection() || (PrintInfoBag.fPrintActiveFrame && pActiveFrameDoc && pActiveFrameDoc->HasTextSelection()));
    if (hDevNames) PrintInfoBag.hDevNames = hDevNames;
    if (hDevMode)  PrintInfoBag.hDevMode  = hDevMode;
    if (hdc)       PrintInfoBag.hdc       = hdc;

    // figure out the current fontscaling setting to preinit the combobox..
    PrintInfoBag.iFontScaling = _sBaselineFont;
    // always init this to false
    PrintInfoBag.fPrintLinked = FALSE;

    // get the active CBodyElement so the dialog can show a focus
    // rect around active frame when necessary
    if(pActiveFrameDoc &&
       pActiveFrameDoc->_pPrimaryMarkup &&
       pActiveFrameDoc->GetPrimaryElementClient() &&
       pActiveFrameDoc->GetPrimaryElementClient()->Tag() == ETAG_BODY)
    {
        PrintInfoBag.pBodyActive = DYNCAST(CBodyElement, pActiveFrameDoc->GetPrimaryElementClient());
    }
    else
    {
        PrintInfoBag.pBodyActive = NULL;
    }
    pActiveFramePrimaryMarkup = pActiveFrameDoc ? pActiveFrameDoc->_pPrimaryMarkup : NULL;

    // we always want the UI while printing the DRT...
    if (dwFlags & PRINT_DONTBOTHERUSER && !(dwFlags & PRINT_DRTPRINT))
    {
        // reset certain members who should always be in their initial state and not be
        // remembered between printjobs...if we do not bring up the UI
        PrintInfoBag.fPrintSelection=FALSE;

        if (PrintInfoBag.hDevNames && PrintInfoBag.hDevMode)
        {
            // If we are  not to bother the user,
            // we assume that the printer passed in (identified by hDevNames)
            // or the printer we had the last time is the
            // one to use instead of the default printer.
            // if we have dont have a DC, need to create one...
            if (!PrintInfoBag.hdc)
            {
                PrintInfoBag.hdc = CreateDCFromDevNames(PrintInfoBag.hDevNames, PrintInfoBag.hDevMode);
            }

        }
        // now we should have a DC, if not FALL back to use the default (better than not printing at all)

        // Use the default printer.
        if (!PrintInfoBag.hdc)
        {
            hr = FormsPrint(&PrintInfoBag, 0, this, TRUE);
            if (hr)
            {
                // if that fails you want to bring up the dialog,
                // because most likely this fails due to some printer setup problem
                // like no default printer exists
                goto BotherUser;
            }
        }
    }
    else // May bother user.
    {
BotherUser:
        {
            CDoEnableModeless   dem(this);

            hwnd = dem._hwnd;
            
            // Show Print common dialog.  S_FALSE if user cancels.
            hr = E_FAIL;
            if (hwnd || (dwFlags & PRINT_PARSECMD))
            {
                hr = FormsPrint(&PrintInfoBag, hwnd, this, FALSE);
            }
        }
        
        if (hr)
        {
            if (hr == S_FALSE)
            {
                // Either the print dialog or the print-to-file dialog was cancelled.  In case the second is true, we do the following
                // before bailing out.

                // Throw away device context if we got one (e.g. if print-to-file dialog was cancelled).
                if (PrintInfoBag.hdc)
                {
                    DeleteDC(PrintInfoBag.hdc);
                    PrintInfoBag.hdc = 0;
                }

                // Take snapshot of print info bag.
                pSpooler->SetPrintInfo(PrintInfoBag);
            }
            goto Cleanup;
        }
    }

#ifdef WIN16
    // Hack: If we are not loaded, the backup plan of binding
    // to mailnews:// fails on win16.  So we pump some messages to
    // help the load complete.  Yuck!  A better solution would be
    // to get urlmon to accept the mailnews moniker.
    //
    // (bug 3005)
    //

    // BUGBUG: This code would GPF mercilessly whenever we don't have a pDocToPrint,
    // which happens when doc to be printed is specified by pchURL (1% of all
    // print operations).
    if (pDocToPrint)  // This line added to prevent GPF and notified vreddy
    {
        const char * cszMailNews = "mailnews:";
        if ((pDocToPrint->LoadStatus() < LOADSTATUS_DONE) &&
            (pDocToPrint->_cstrUrl && strncmp(pDocToPrint->_cstrUrl, cszMailNews, sizeof(cszMailNews)) == 0))
        {
            // Let through WM_TIMER and WM_METHODCALL messages.
            // BUGWIN16: Could fail if timer is not ready to go off!
            MSG msg;
            const WORD WM_METHODCALL = WM_APP+2; // see gwnd.cxx
            while (PeekMessage(&msg, NULL, WM_METHODCALL, WM_METHODCALL, PM_REMOVE) ||
                   PeekMessage(&msg, NULL, WM_TIMER, WM_TIMER, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
#endif //WIN16

    // Print Outlook express mail header?
    PrintInfoBag.fPrintOEHeader = (pStream != NULL);

    if (PrintInfoBag.fPrintOEHeader)
    {
        // Print info bag takes ownership of stream.
        PrintInfoBag.pstmOEHeader = pStream;
        PrintInfoBag.pPrintDocOEHeader = NULL;

        // Addref the stream, so we can party on it as long as we want.
        pStream->AddRef();
    }

    // Are we an xml document? 
    PrintInfoBag.fXML = _pPrimaryMarkup && _pPrimaryMarkup->IsXML();

    codepageFamily = (PrintInfoBag.fPrintActiveFrame && pActiveFrameDoc) ? pActiveFrameDoc->_codepageFamily : _codepageFamily;
    PrintInfoBag.fConsiderBitmapFonts = !g_fUnicodePlatform && (codepageFamily == CP_JPN_SJ || codepageFamily == CP_CHN_GB || codepageFamily == CP_KOR_5601 || codepageFamily == CP_TWN);
    if (PrintInfoBag.fConsiderBitmapFonts)
    {
        CPINFO cpinfo;
        if (GetCPInfo(g_cpDefault, &cpinfo))
            PrintInfoBag.fConsiderBitmapFonts = cpinfo.MaxCharSize == 1;
    }

    // Give the spooler a new print info bag snapshot for defaults of future print jobs.
    // NO MODIFICATIONS TO THE PRINTINFOBAG ALLOWED BEYONG THIS POINT BECAUSE THEY HAVE NO
    // EFFECT.
    Assert(PrintInfoBag.hDevNames && PrintInfoBag.hDevMode);
    pSpooler->SetPrintInfo(PrintInfoBag);

    // Find out which doc we are printing.
    if (PrintInfoBag.fPrintActiveFrame)
    {
        // We are printing the active frame.
        if (pActiveFrameDoc)
        {
            // The active frame has a doc.  We print that.
            pDocToPrint = pActiveFrameDoc;

            // The active frame has since changed.  Don't use the document, but instead use
            // its original url.
            if (!pDocToPrint->_pPrimaryMarkup || pDocToPrint->_pPrimaryMarkup != pActiveFramePrimaryMarkup)
            {
                pDocToPrint = NULL;
                pchURL = achOverridePrintUrl;
            }
        }
        else
        {
            // The active frame has no doc.  Print the content as a URL pass-in.
            pchURL = achOverridePrintUrl;
            pDocToPrint = NULL;

            // If we are printing an IPrint object, print it synchronously to avoid
            // RPC thread problems.  (RPC_E_WRONG_THREAD)
            if (pPrint && S_OK == PrintExternalIPrintObject(&PrintInfoBag, pPrint))
            {
                // If the IPrint document printed successfully, we are done.
                hr = S_OK;
                goto Cleanup;
            }
        }
    }
    else if (PrintInfoBag.fRootDocumentHasFrameset)
    {
        // BUGBUG:  ShDocVw doesn't let us print Office docobjects via IViewObject::Draw()
        // and Word97 has a bug where their docs disappear from hosts if another host
        // requests an instance, so consider those IFrames to be blank to avoid this
        // nasty side-effect.  (52158)
        // Instead of not printing embedded Office docs at all we will print them as
        // separate documents instead.  Once we can print Office docs inplace, we can
        // add to following condition: !PrintInfoBag.fPrintAsShown

        // If we are printing all frames in a frameset individually, do IPrint frames
        // synchronously and let the spooler print the remaining frames asynchronously.
        if ( S_OK == PrintExternalIPrintObject(&PrintInfoBag) )
        {
            // Remember not to print IPrint jobs again.
            fExtJobsComplete = TRUE;
        }
    }

    if (PrintInfoBag.fPrintLinked)
    {
        lRecursive = (lRecursive==0) ? 1 : lRecursive;
    }

    // Create a print info structure
    hr = THR(CreatePrintInfo(&ppiCurrentPrintInfo));
    if (hr || !ppiCurrentPrintInfo)
    {
        goto Cleanup;
    }

    Assert((pDocToPrint || pchURL) && !(pDocToPrint && pchURL)
        && "At this point we either have a pDocToPrint or a pchURL, but not both.");

    // If the print document is in memory, ask for an alternate print document.
    if (pDocToPrint && !pchURL
        && (S_OK != AreRatingsEnabled())
        && !PrintInfoBag.fPrintSelection
        && S_OK == pDocToPrint->GetAlternatePrintDoc(achOverridePrintUrl, pdlUrlLen)
        && (_tcslen(achOverridePrintUrl) > 0))
    {
        // IMPORTANT:  We never print alternate documents if ratings are enabled to avoid
        // ratings security holes.

        // We mark this job as alternate, so that we don't fall into an
        // infinite "print alternate doc" loop trap.
        ppiCurrentPrintInfo->fAlternateDoc = TRUE;
        pchURL = achOverridePrintUrl;
        pDocToPrint = NULL;

        // Make sure we print alternate office documents.  (58665)
        fExtJobsComplete = FALSE;
    }

    Assert((pDocToPrint || pchURL) && !(pDocToPrint && pchURL)
        && "At this point we either have a pDocToPrint or a pchURL, but not both.");

    // Store event info, set the document's print depth, and pass on
    // information about the document type.
    ppiCurrentPrintInfo->nDepth = lRecursive;
    ppiCurrentPrintInfo->fExtJobsComplete = fExtJobsComplete;
    ppiCurrentPrintInfo->fPrintHtmlOnly = !pchURL || !!(dwFlags & PRINT_HTMLONLY);
    ppiCurrentPrintInfo->fTempFile = PrintInfoBag.fPrintSelection
        || (!pchURL && pDocToPrint && pDocToPrint->LoadStatus() >= LOADSTATUS_DONE);

    // Let's see if we want to print this document or something else?
    // If we do not have a URL and the current print doc is completely loaded,
    // we save to a tempfile (otherwise we go back to the original URL).
    if (ppiCurrentPrintInfo->fTempFile)
    {
        unsigned fSaveTempfileForPrinting = pDocToPrint->_fSaveTempfileForPrinting;
        CODEPAGE codepage = pDocToPrint->_codepage;

        // Obtain a temporary file name
        if (!GetTempPath( MAX_PATH, achTempPath ))
            goto Cleanup;
        if (!GetHTMLTempFileName( achTempPath, _T("tri"), 0, achTempFileName ))
            goto Cleanup;
        ppiCurrentPrintInfo->cstrTempFileName.Set(achTempFileName);

        // Also remember its URL base.
        Assert(!!pDocToPrint->_cstrUrl);

        ppiCurrentPrintInfo->cstrBaseUrl.Set(pDocToPrint->_cstrUrl);

        pDocToPrint->_fSaveTempfileForPrinting = TRUE;

        // HACK (cthrash) We must ensure that our encoding roundtrips -- the
        // only known codepage for which this isn't guaranteed is ISO-2022-JP.
        // In this codepage, half-width katakana will be converted to full-
        // width katakana, which can lead to potentially disasterous results.
        // Swap in a more desirable codepage if we have ISO-2022-JP.
        if (codepage == CP_ISO_2022_JP)
        {
            pDocToPrint->_codepage = CP_ISO_2022_JP_ESC1;
        }

        // HACK (cthrash/sumitc) we can't identify UTF-7 pages as such, and so
        // we can't load them in correctly.  So (for bug 46925), we save UTF-7
        // as Unicode instead.
        if (codepage == CP_UTF_7)
        {
            pDocToPrint->_codepage = CP_UCS_2;
        }

        if (pDocToPrint->_pOmWindow)
        {
            pDocToPrint->_fPrintEvent = TRUE;
            pDocToPrint->_pOmWindow->Fire_onbeforeprint();
            pDocToPrint->_fPrintEvent = FALSE;
        }

        if (PrintInfoBag.fPrintSelection)
        {
            if (pDocToPrint->HasTextSelection())
            {
                // now we have to figure out if just the selection box was checked
                // note: if the box was checked that implies already that there is
                // a selection, because otherwise the selection box should not have
                // been selectable

                // general algorithm: if selection box was checked, save selection to
                // a tempfile and enque this file
                hr = pDocToPrint->SaveSelection(achTempFileName);
            }
            else
            {
                // something is wrong
                Assert("There should be a selection at this point");
                hr = E_FAIL;
            }
        }
        else
        {
            // Save the whole doc to the temporary file
            hr = pDocToPrint->Save(achTempFileName, FALSE);
        }

        fWaitForPluginsToPrint = (!(dwFlags & PRINT_WAITFORCOMPLETION)) && pDocToPrint->GetRootDoc()->_fPrintedDocSavedPlugins;
        if (fWaitForPluginsToPrint)
            pDocToPrint->GetRootDoc()->_fPrintedDocSavedPlugins = FALSE;

        if (pDocToPrint->_pOmWindow)
        {
            pDocToPrint->_fPrintEvent = TRUE;
            pDocToPrint->_pOmWindow->Fire_onafterprint();
            pDocToPrint->_fPrintEvent = FALSE;
        }

        pDocToPrint->_fSaveTempfileForPrinting = fSaveTempfileForPrinting;
        pDocToPrint->_codepage = codepage;

        if (hr)
        {
            // If saving to a tempfile failed or there is something wrong with
            // the selection, go with the original URL.  This is better than
            // failing here and not printing anything without user feedback.
            ppiCurrentPrintInfo->fTempFile = FALSE;
        }
#if DBG == 1
        else if (IsTagEnabled(tagPrintKeepTempFile))
        {
            Assert(hr == S_OK);
            CopyFile(achTempFileName, _T("c:\\ee.htm"), FALSE);
        }
#endif
    }
    
    // Set the "don't run scripts" flag once we have a definite plan
    // regarding the tempfile.
    ppiCurrentPrintInfo->fDontRunScripts = ppiCurrentPrintInfo->fTempFile;

    if (!ppiCurrentPrintInfo->fTempFile)
    {
        if (!pchURL)
        {
            // if we wanted to print the current, but it was
            // not fully loaded treat it as a URL pass in.
            pchURL = pDocToPrint->_cstrUrl;
        }
        // now we should have a BSTR passed in...

        ppiCurrentPrintInfo->cstrBaseUrl.Set(pchURL);
    }

    if (fWaitForPluginsToPrint && !hEvent)
    {
        Assert(!(dwFlags & PRINT_WAITFORCOMPLETION));
        hEvent = CreateEvent(0, TRUE, FALSE, 0);
        fWaitForPluginsToPrint = hEvent != 0;
    }

    ppiCurrentPrintInfo->hEvent = hEvent;

    // Finally enqueue the job.
    hr = pSpooler->EnqueuePrintJob(ppiCurrentPrintInfo, &ulDummy);
    if (hr)
        goto Cleanup;

    UpdatePrintNotifyWindow(GetHWND());

    if ((dwFlags & PRINT_WAITFORCOMPLETION) && hEvent)
    {
        dwResult = WaitForSingleObject(hEvent, INFINITE);
        Assert(dwResult == WAIT_OBJECT_0);
    }
    else if (fWaitForPluginsToPrint)
    {
        // Perform a modal msg loop until the event is signalled
        CDoEnableModeless   dem(this);
        DWORD               dwResult = WAIT_OBJECT_0 + 1;
        MSG                 msg;

        Assert(hEvent);

        do
        {
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            dwResult = ::MsgWaitForMultipleObjects(1, &hEvent, FALSE, INFINITE, QS_ALLINPUT);
        }
        while (dwResult > WAIT_OBJECT_0);
    }

    if (dwFlags & PRINT_RELEASESPOOLER)
    {
        // BUGBUG (cthrash) Who does the addref such that we need to do a release here?
        
        ULONG ulRefs = pSpooler->GetRefs();

        // happens e.g. from PRINTHTML, means we are called on a seperate
        // THREADID and this guy will go away soon....
        pSpooler->Release();

        if (ulRefs == 1)
        {
            THREADSTATE *pts = GetThreadState();

            if (pts->pSpooler == pSpooler)
            {
                pts->pSpooler = NULL;
            }
        }
    }

Cleanup:

    // 53416: Help (hh.exe) loads documents to be printed, but doesn't wait until they finish loading.
    // Thus the document (and its hwnd) is destroyed before its posted OnMethodCall arrives thus wedging
    // the thread's method calls.
    if (pDocToPrint)
        pDocToPrint->_fUnwedgeFromPrinting = OK(hr) && pDocToPrint->LoadStatus() < LOADSTATUS_PARSE_DONE;

    if (hEvent)
        CloseEvent(hEvent);

    delete pchPath;
    ReleaseInterface(pPrint);
    RRETURN1(hr, S_FALSE);
#else
    return S_OK;
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::PageSetup, public
//
//  Synopsis:   Set parameters of the page for printing.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CDoc::PageSetup()
{
#ifndef NO_PRINT
    HRESULT hr;
    CSpooler        *pSpooler;
    PRINTINFOBAG    PrintInfoBag;
    HWND            hwnd;
    hr = THR(GetSpooler(&pSpooler));
    if (hr)
    {
        goto Cleanup;
    }

    Assert(pSpooler);

    hr = pSpooler->GetPrintInfo(&PrintInfoBag);
    if (hr)
    {
        goto Cleanup;
    }

    {
        CDoEnableModeless   dem(this);

        hwnd = dem._hwnd;
        if (hwnd)
        {
            hr = FormsPageSetup( &PrintInfoBag, hwnd, this);
        }
    }
    
    if (hr || !hwnd)
    {
        goto Cleanup;
    }

    // everything is fine, set the info bag...
    Assert(PrintInfoBag.hDevNames && PrintInfoBag.hDevMode);
    pSpooler->SetPrintInfo(PrintInfoBag);


Cleanup:    
     // We always want to substitute S_FALSE
     // because our primary caller (Exec::) does translate S_FALSE
     // correctly, where E_FAIL would cause a a ShowLastErrorInfo
     // which we do not want to supply, because the printing subsystem
     // in windows WILL in effect take care of that by informing the user
     // (frankman, bug 45912)    
     hr = (hr <= S_FALSE) ? S_OK : S_FALSE;
    RRETURN(hr);
#else
    return S_OK;
#endif
}



#ifndef NO_PRINT
CPrintDoc::CPrintDoc() : CDoc(NULL)
{
    RECT rcInit = {0, 0, 1, 1};
    SIZEL sizelInit = {1, 1};

    _paryPrintPage = NULL;
    _ptd    = NULL;
    _hdc    = NULL;
    _hic    = NULL;
    _pHInfo = NULL;
    _pFInfo = NULL;
    _pSpoolerToNotify = NULL;

    // to avoid division by zero
    _dci.CTransform::Init(&rcInit, sizelInit);
}


CPrintDoc::~CPrintDoc()
{
    if (_paryPrintPage)
    {
        delete _paryPrintPage;
    }

    if (_pHInfo)
    {
        delete _pHInfo;
        _pHInfo = NULL;
    }

    if (_pFInfo)
    {
        delete _pFInfo;
        _pFInfo = NULL;
    }
}




//+------------------------------------------------------------------------
//
//  Member:     CPrintDoc::DoInit
//
//  Synopsis:   Override init so that we can, once and for all, get all the
//              values that have been set in the bag into the CPrintDoc
//
//-------------------------------------------------------------------------

HRESULT
CPrintDoc::DoInit(PRINTINFO * pPICurrent, CSpooler *pSpoolerToNotify)
{
    HRESULT hr;

    Assert(pPICurrent && pPICurrent->ppibagRootDocument && pSpoolerToNotify);

    hr = THR( Init() );
    if (hr)
        goto Cleanup;

    // Copy print info bag from spooler.
    _PrintInfoBag = *(pPICurrent->ppibagRootDocument);
    _fRootDoc = pPICurrent->fRootDocument;


    // Copy selected information from the spooler's print info to the print doc.

    _fDontRunScripts = pPICurrent->fDontRunScripts;
    _fTempFile = pPICurrent->fTempFile;
    _fLaidOut = FALSE;
    if (pPICurrent->fUsePrettyUrl) _cstrPrettyUrl.Set(pPICurrent->cstrPrettyUrl);
    if (!_cstrPrettyUrl.Length())  _cstrPrettyUrl.Set(pPICurrent->cstrBaseUrl);
    _ptd = pPICurrent->ppibagRootDocument->ptd;
    _hdc = pPICurrent->ppibagRootDocument->hdc;
    _hic = pPICurrent->ppibagRootDocument->hic;

    Assert(_ptd && _hdc && _hic);

    hr = InitForPrint();
    if (hr)
    {
        Assert(!"Initializing print doc has failed");
        goto Cleanup;
    }

    // Copy pointer to spooler to notify when loading finishes.
    _pSpoolerToNotify = pSpoolerToNotify;

Cleanup:

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CPrintDoc::OnLoadStatus
//
//  Synopsis:   The document is now loaded, format the pages and print them.
//
//  Arguments:  LoadStatus    The current type of feedback to give
//
//-------------------------------------------------------------------------

void
CPrintDoc::OnLoadStatus(LOADSTATUS LoadStatus)
{
    if (LoadStatus == LOADSTATUS_DONE)
    {
        IGNORE_HR(EnsureOmWindow());
    }

    super::OnLoadStatus(LoadStatus);

    if (LoadStatus == LOADSTATUS_DONE)
    {
        // Now freeze events
        IGNORE_HR(FreezeEvents(TRUE));

        // Notify the spooler that we finished loading, so it can initiate
        // printing this document.

        if ( _pSpoolerToNotify )
        {
            AfterLoadComplete();

            _pSpoolerToNotify->OnLoadComplete(this, SP_FINISHED_LOADING_HTMLDOCUMENT);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPrintDoc::PrintOnePage, private
//
//  Synopsis:   Prints 1 page of the document
//
//  Parameters  iPage   page number for header-footer
//              rc      prints into this rectangle
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CPrintDoc::PrintOnePage(int cPage, RECT rc)
{
    BOOL    fLastOEPageFull = _PrintInfoBag.fPrintOEHeader
                          && ((*(_PrintInfoBag.pPrintDocOEHeader->_paryPrintPage))[_PrintInfoBag.pPrintDocOEHeader->_paryPrintPage->Size() - 1]).yPageHeight == _dci._sizeDst.cy;
    HRGN    hrgnClip = 0;
    HRESULT hr = S_OK;

    if (StartPage(_hdc) <= 0)
    {
        WHEN_DBG(DWORD dwError = GetLastError(); Assert(!g_Zero.ab[0] || dwError);)
        hr = E_FAIL;
        goto Cleanup;
    }

    // Print header and footer.
    IGNORE_HR(PrintHeaderFooter(cPage));

    // 62057: Prevent rendering into margins and header/footer.
    hrgnClip = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
    if (hrgnClip)
        SelectClipRgn(_hdc, hrgnClip);

    if (!_PrintInfoBag.fPrintOEHeader || (cPage >= _PrintInfoBag.pPrintDocOEHeader->_paryPrintPage->Size()))
    {
        // Main doc only page.
        POINT ptOffset;
        int cPageCurrent = cPage - (_PrintInfoBag.fPrintOEHeader ?
                                                    (_PrintInfoBag.pPrintDocOEHeader->_paryPrintPage->Size() - (fLastOEPageFull?0:1))
                                                    : 0);
        

        ptOffset.x = 0;
        ptOffset.y = 0;

        // See if we need to print a repeated table header.
        if ((*_paryPrintPage)[cPageCurrent].fReprintTableHeader)
        {
            ptOffset.y = (*_paryPrintPage)[cPageCurrent].rcTableHeader.bottom - (*_paryPrintPage)[cPageCurrent].rcTableHeader.top;
            IGNORE_HR( DoDraw(&((*_paryPrintPage)[cPageCurrent].rcTableHeader)) );
        }

        IGNORE_HR( DoDraw(&rc, &ptOffset) );

        // See if we need to print a repeated table footer.
        if ((*_paryPrintPage)[cPageCurrent].fReprintTableFooter)
        {
            ptOffset.y += rc.bottom - rc.top;
            IGNORE_HR( DoDraw(&((*_paryPrintPage)[cPageCurrent].rcTableFooter), &ptOffset) );
        }
    }
    else if (cPage < _PrintInfoBag.pPrintDocOEHeader->_paryPrintPage->Size() - 1
        || (cPage == _PrintInfoBag.pPrintDocOEHeader->_paryPrintPage->Size() - 1
            && fLastOEPageFull))
    {
        // Outlook Express header only page.
        IGNORE_HR( _PrintInfoBag.pPrintDocOEHeader->DoDraw(&rc) );
    }
    else
    {
        // Mixed page.
        POINT  ptOffset;
        RECT rcOEHeader = rc, rcMain = rc;
        LONG yHeightOEHeader = ((*(_PrintInfoBag.pPrintDocOEHeader->_paryPrintPage))[cPage]).yPageHeight;

        Assert(cPage == _PrintInfoBag.pPrintDocOEHeader->_paryPrintPage->Size() - 1);

        // Set up both rects relative to (left,top) corner (second part will be shifted by DC).
        rcOEHeader.bottom = rc.top + yHeightOEHeader;
        rcMain.bottom = rc.bottom;

        // Print Outlook Express header part.
        IGNORE_HR( _PrintInfoBag.pPrintDocOEHeader->DoDraw(&rcOEHeader) );

        // Pass in the header height to offset the viewport
        ptOffset.y = yHeightOEHeader;
        ptOffset.x = 0;
        // Print Main part.
        IGNORE_HR( DoDraw(&rcMain, &ptOffset) );

        // See if we need to print a repeated table footer.
        if (_paryPrintPage->Size() && (*_paryPrintPage)[0].fReprintTableFooter)
        {
            ptOffset.y += rcMain.bottom - rcMain.top;
            IGNORE_HR( DoDraw(&((*_paryPrintPage)[0].rcTableFooter), &ptOffset) );
        }
    }

    SelectClipRgn(_hdc, NULL);

    if (EndPage(_hdc) <= 0)
    {
        WHEN_DBG(DWORD dwError = GetLastError(); Assert(!g_Zero.ab[0] || dwError);)
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:

    if (hrgnClip)
        DeleteObject(hrgnClip);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPrintDoc::GetPrintPageRange, private
//
//  Synopsis:   Obtains the printed page boundaries.
//
//  Parameters  cPages     [in]     number of pages in doc
//              nStartPage [out]
//              nEndPage   [out]
//
//----------------------------------------------------------------------------

void
CPrintDoc::GetPrintPageRange(int cPages, int *pnStartPage, int *pnEndPage)
{
    Assert(pnStartPage && pnEndPage);

    if (_PrintInfoBag.fAllPages == 0 && !(_PrintInfoBag.fPrintSelection))
    {
        *pnStartPage = (_PrintInfoBag.nFromPage >= 1) ? (_PrintInfoBag.nFromPage-1) : 0;        // start page >= 0
        *pnEndPage = ((_PrintInfoBag.nToPage <= cPages) ? _PrintInfoBag.nToPage : cPages) - 1;  // end page <= cPages - 1
    }
    else
    {
        *pnStartPage = 0;
        *pnEndPage = cPages-1;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPrintDoc::PrintCollated, private
//
//  Synopsis:   Prints the document collated, i.e. the order of the printed
//              pages is 1,2,3  1,2,3  1,2,3
//
//  Parameters  rc      prints into this rectangle
//              cCopies number of copies
//              cPages  number of pages
//
//  Returns:    none
//
//----------------------------------------------------------------------------

void
CPrintDoc::PrintCollated(RECT rcClip, int cCopies, int cPages)
{
    int nCopy;

    // Convert collated print into "cCopies" number of non-collated prints.
    for (nCopy = 0; nCopy < cCopies && !_fCancelled; nCopy++)
    {
        PrintNonCollated(rcClip, 1, cPages);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPrintDoc::PrintNotCollated, private
//
//  Synopsis:   Prints the document not collated, i.e. the order of the printed
//              pages is 1,1,1  2,2,2  3,3,3
//
//  Parameters  rc      prints into this rectangle
//              cCopies number of copies
//              cPages  number of pages
//
//  Returns:    none
//
//----------------------------------------------------------------------------

void
CPrintDoc::PrintNonCollated(RECT rcClip, int cCopies, int cPages)
{
    LONG        yScrollMainDoc = 0;
    int         nPage;
    int         nCopy;
    int         nStartPage;
    int         nEndPage;
    int         nOEHeaderPages = 0;
    int         nStartPageMainDoc = 0;
    CRect        rcScroll(0,0,0,0);

    GetPrintPageRange(cPages, &nStartPage, &nEndPage);


    //BUGBUG (gideons) the logic of OE headers needs more testing. A better understanding of
    // OE needs is needed inorder to complete this feature.
    if (_PrintInfoBag.fPrintOEHeader)
    {
        nOEHeaderPages = _PrintInfoBag.pPrintDocOEHeader->_paryPrintPage->Size();
        nStartPageMainDoc = nOEHeaderPages - 1;

        // If last OE header page fills the page entirely, the main doc starts yet another page later.
        if ( ((*(_PrintInfoBag.pPrintDocOEHeader->_paryPrintPage))[nOEHeaderPages-1]).yPageHeight == _dci._sizeDst.cy )
        {
            nStartPageMainDoc++;
        }
    }

    for (nPage = 0; nPage <= nEndPage && !_fCancelled ; nPage++)
    {
        LONG yScrollDeltaMainDoc = 0/*, yScrollDeltaOEHeader = 0*/;

        // Set up rect and scroll distance for both main doc and OE header doc.
        
#ifdef NEVER
        // If printing OE header get it's height
        if (nPage <= nOEHeaderPages - 1 && !_PrintInfoBag.pPrintDocOEHeader->_fFrameSet)
        {
            // Outlook Express header only page.
            yScrollDeltaOEHeader = ((*(_PrintInfoBag.pPrintDocOEHeader->_paryPrintPage))[nPage]).yPageHeight;
        }
        else
        {
            yScrollDeltaOEHeader = 0;
        }
#endif

        if (nPage >= nStartPageMainDoc)
        {
            // Main doc only page. // BUGBUG ???? (gideons) what does that mean ????
            yScrollDeltaMainDoc = (*_paryPrintPage)[nPage-nStartPageMainDoc].yPageHeight;

            // Set scroll position for the page.
            rcScroll.SetRect(0, yScrollMainDoc, _rcClip.right - _rcClip.left, yScrollMainDoc + yScrollDeltaMainDoc);
            
            // Update yScrollMainDoc, exactly as set above.
            yScrollMainDoc += yScrollDeltaMainDoc;
        }

        // Print page if in range.
        if (nPage >= nStartPage)
        {
            // if the content of the page is wider then the physical page we are 
            // going to "scroll" horizontaly until we cover the whole content.
            LONG LViewWidth, lViewWidthLeft;
            LViewWidth = lViewWidthLeft = (*_paryPrintPage)[nPage-nStartPageMainDoc].xPageWidth;

            for (nCopy = 0; nCopy < cCopies  && !_fCancelled; nCopy++)
            {
#ifdef NEVER // DBG == 1  //BUGBUG  (gideons) need to put back this debug feature
                if (IsTagEnabled(tagPaginate))
                {
                    HBRUSH  hbr;
                    HPEN    hpen;
                    hbr = (HBRUSH) SelectObject(_hdc, GetStockObject(LTGRAY_BRUSH));
                    hpen = (HPEN) SelectObject(_hdc, GetStockObject(BLACK_PEN));
                    Rectangle(_hdc, rc.left, rc.bottom + 1, rc.right, rcClip.bottom);
                    SelectObject(_hdc, hbr);
                    SelectObject(_hdc, hpen);
                }
#endif // DBG == 1

                if (PrintOnePage(nPage, rcScroll) != S_OK)
                    return;

                // BUGBUG: RELAYOUT DISABLED.  Besides, change this to a do-while loop.  Also make sure to
                // resolve this with collated printing.
                /*
                if ((nCopy + 1 == cCopies) && (lViewWidthLeft > lPageWidth) && fDoHorizontalScrolling)
                {
                     nCopy = -1; // one more page to print

                     // Move the rect one page to the right
                     rcScroll.left = rcScroll.right;
                     rcScroll.right += lPageWidth; 
                     // Count this page's width
                     lViewWidthLeft -= lPageWidth;
                     continue;
                }
                */
            }
        }
        if (_pSpoolerToNotify)
        {
            _fCancelled |= _pSpoolerToNotify->ShuttingDown();
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPrintDoc::Print
//
//  Synopsis:   Prints the print document.
//
//  Arguments:  None, CPrintDoc and device context already configured.
//
//----------------------------------------------------------------------------

HRESULT
CPrintDoc::Print()
{
    int         cPages = 1, cPagesOEHeader = 0;
    LONG        lViewWidth = 0;
    LONG        yInitialPageHeight = 0;
    int         cCopies=1;
    DOCINFO     docinfo;
    TCHAR       achDocName[MAX_PATH];
    CSize       sizeView;

    if (!_fTempFile && _pOmWindow)
    {
        _fPrintEvent = TRUE;
        _pOmWindow->Fire_onbeforeprint();
        _fPrintEvent = FALSE;
    }

    HRESULT     hr = TransitionTo(OS_RUNNING);
    if (hr)
        goto Cleanup;

    Assert(_pSpoolerToNotify);
    if (!_pSpoolerToNotify)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // Set the document's title.
    if (_pPrimaryMarkup && _pPrimaryMarkup->GetTitleElement() && _pPrimaryMarkup->GetTitleElement()->Length() > 0)
    {
        _tcsncpy(achDocName, _pPrimaryMarkup->GetTitleElement()->GetTitle(), ARRAY_SIZE(achDocName));
        achDocName[ARRAY_SIZE(achDocName)-1] = _T('\0');
    }
    else if (_cstrPrettyUrl.Length() > 0)
    {
        _tcsncpy(achDocName, _cstrPrettyUrl, ARRAY_SIZE(achDocName));
        achDocName[ARRAY_SIZE(achDocName)-1] = _T('\0');
    }
    else if (_cstrUrl.Length() > 0)
    {
        if (GetUrlScheme(_cstrUrl) == URL_SCHEME_FILE)
        {
            ULONG cchDocName = ARRAY_SIZE(achDocName);
            PathCreateFromUrl(_cstrUrl, achDocName, &cchDocName, 0);
        }
        else
        {
            _tcsncpy(achDocName, _cstrUrl, ARRAY_SIZE(achDocName));
            achDocName[ARRAY_SIZE(achDocName)-1] = _T('\0');
        }
    }
    else
    {
        Assert(!"No title exists for HTML document");
        _tcscpy(achDocName, L'\0');
    }

    // Fill in the DOCINFO structure used by StartDoc()
    docinfo.cbSize = sizeof(DOCINFO);
    docinfo.lpszDocName = achDocName;
    docinfo.lpszOutput = NULL;
#ifndef WIN16
    docinfo.lpszDatatype = NULL;
    docinfo.fwType = 0;
#endif //!WIN16

    if (_PrintInfoBag.fPrintToFile != 1)
    {
        if (_PrintInfoBag.fDrtPrint)
        {
            docinfo.lpszOutput = PRINTDRT_FILE;
        }
    #if DBG==1
        else
        {
            // When debugging, it is easiest to print to a file with a postscript
            // driver.
            if (IsTagEnabled(tagPrintToTempFoo))
                docinfo.lpszOutput = _T("c:\\temp\\foo.ps");

            if (IsTagEnabled(tagPrintToTempFoo2))
                docinfo.lpszOutput = _T("d:\\temp\\foo.ps");
        }

        if ((IsTagEnabled(tagPrintToTempFoo) || IsTagEnabled(tagPrintToTempFoo2))
            && _PrintInfoBag.fPrintLinked)
        {
            static TCHAR   c = _T('0');
            static TCHAR   achTempFile[MAX_PATH];
            TCHAR   achTempPath[MAX_PATH];

            Assert(GetTempPath(MAX_PATH, achTempPath));
            wsprintf(achTempFile, _T("%s%s%c%s"), achTempPath, _T("\\foo"), c, _T(".ps"));

            // increment c from 0 to 9, and then a to z.  Stop at z.
            if (_istdigit(c))
            {
                c = (_istdigit(c + 1) ? (c + 1) : 'a');
            }
            else
            {
                c = (((c + 1) >= _T('a') && (c + 1) <= _T('z')) ? (c + 1) : c);
            }

            docinfo.lpszOutput = achTempFile;
        }
    #endif
    }
    else
    {
        if (!_fRootDoc)
        {
            VerifyPrintFileName(_PrintInfoBag.achPrintToFileName);
        }
        docinfo.lpszOutput = _PrintInfoBag.achPrintToFileName;
    }

    if (StartDoc( _hdc, &docinfo ) <= 0)
    {
        WHEN_DBG(DWORD dwError = GetLastError(); Assert(!g_Zero.ab[0] || dwError);)
        hr = E_FAIL;
        goto Cleanup;
    }

    // Initialize clip / draw rectangle to the whole page minus margins.
    _rcClip.left  = _dci._ptDst.x;
    _rcClip.right = _rcClip.left + _dci._sizeDst.cx;
    _rcClip.top    = _dci._ptDst.y;
    _rcClip.bottom = _rcClip.top + _dci._sizeDst.cy;

    // Layout Outlook Express mail header print doc.
    if (_PrintInfoBag.fPrintOEHeader && _PrintInfoBag.pPrintDocOEHeader)
    {
        LONG yLastPageHeight;
        RECT rcClip =  _PrintInfoBag.pPrintDocOEHeader->_rcClip;

        CView * pOEHView = &_PrintInfoBag.pPrintDocOEHeader->_view;

        // Set up the view to the paper size, so we can layout and paginate.
        pOEHView->Activate();

        sizeView.SetSize(rcClip.right - rcClip.left, rcClip.bottom - rcClip.top);
        pOEHView->SetViewSize(sizeView);



        if (S_OK == THR(_PrintInfoBag.pPrintDocOEHeader->TransitionTo(OS_RUNNING))
            && S_OK == THR(_PrintInfoBag.pPrintDocOEHeader->DoLayout(
                     &cPagesOEHeader, &lViewWidth, &yLastPageHeight, 0, FALSE))
            && cPagesOEHeader)
        {
            CPrintPage *pPP = &((*(_PrintInfoBag.pPrintDocOEHeader->_paryPrintPage))[cPagesOEHeader-1]);

            // If the main doc is a frameset (wysiwyg) and the header takes up more than
            // half the page, then print the frameset on its own page.
            if (_fFrameSet && (yLastPageHeight > (_dci._sizeDst.cy >> 1)))
            {
                pPP->yPageHeight = _dci._sizeDst.cy;
            }

            // Compute remaining space for main print doc's first page.
            yInitialPageHeight = _dci._sizeDst.cy - pPP->yPageHeight;

            if (!yInitialPageHeight)
            {
                cPagesOEHeader++;
            }
        }
    }

    if (!cPagesOEHeader)
    {
        // Don't accept framesets as headers.
        _PrintInfoBag.fPrintOEHeader = FALSE;
    }
    

    // Set up the view to the paper size, so we can layout and paginate.
    _view.Activate();

    sizeView.SetSize(_rcClip.right - _rcClip.left, _rcClip.bottom - _rcClip.top);
    _view.SetViewSize(sizeView);

    // Lay out and paginate the print doc.
    hr = THR(DoLayout(&cPages, &lViewWidth, NULL, yInitialPageHeight));
    if (hr)
        goto Cleanup;

    if (_PrintInfoBag.fPrintOEHeader)
    {
        // Combining the number of pages involves adding them and subtracting 1
        // for the shared page.  (If there is no shared page, cPagesOEHeader is
        // has been incremented.)
        cPages += cPagesOEHeader - 1;
    }

    _pHInfo = new CHeaderFooterInfo(this);
    Assert(_pHInfo);
    _pFInfo = new CHeaderFooterInfo(this);
    Assert(_pFInfo);

    if (_pHInfo && _pFInfo)
    {
        cCopies = _PrintInfoBag.nCopies;
        _pHInfo->SetHeaderFooter(LPTSTR(_PrintInfoBag.strHeader));
        _pHInfo->SetTotalPages(cPages);
        SetPrintHeaderFooterURL(_cstrPrettyUrl);
        _pFInfo->SetHeaderFooter(LPTSTR(_PrintInfoBag.strFooter));
        _pFInfo->SetTotalPages(cPages);
    }

    // Print document.

    if (_PrintInfoBag.fCollate)
        PrintCollated(_rcClip, cCopies, cPages);
    else
        PrintNonCollated(_rcClip, cCopies, cPages);

Cleanup:
    // End of document
    _view.Deactivate();

    EndDoc(_hdc);
    Close(OLECLOSE_NOSAVE);

    if (!_fTempFile && _pOmWindow)
    {
        _fPrintEvent = TRUE;
        _pOmWindow->Fire_onafterprint();
        _fPrintEvent = FALSE;
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPrintDoc::DoLayout
//
//  Synopsis:   Lays out and paginates the document
//
//  Arguments:  pcPages [out]           number of pages needed
//              pyLastPageHeight [out]  height of last page in device units
//              yInitialPageHeight [in] remaining space on which the first
//                                      page of the doc is to be printed
//
//----------------------------------------------------------------------------

HRESULT
CPrintDoc::DoLayout(int *pcPages, LONG * plViewWidth, LONG *pyLastPageHeight, LONG yInitialPageHeight, BOOL fFillLastPage)
{
    HRESULT hr = S_OK;
    LONG yFullSizeDst = _dci._sizeDst.cy;
    CElement * pElementClient = _pPrimaryMarkup->GetElementClient();

    // dequeue any potentially pending peer tasks - the result may affect layout
    PeerDequeueTasks(0);

    Assert(pElementClient);

    if (!_fLaidOut)
    {
        // Create a pagination array for the print doc.
        _paryPrintPage = new(Mt(CPrintDoc_paryPrintPage)) CDataAry<CPrintPage>(Mt(CPrintDoc_paryPrintPage_pv));
        if (!_paryPrintPage)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        if (_fFrameSet && yInitialPageHeight)
        {
            // Force frameset to squeeze into smaller space.
            _dci._sizeDst.cy = yInitialPageHeight;
        }

        // Layout the print doc.
        pElementClient->ResizeElement(NFLAGS_FORCE);
        _view.EnsureView(LAYOUT_SYNCHRONOUS | LAYOUT_FORCE | LAYOUT_DEFERPAINT);

        // Paginate the print doc.

        if (ETAG_BODY == pElementClient->Tag())
        {
            CFlowLayout * pLayoutBody = DYNCAST(CFlowLayout, pElementClient->GetUpdatedLayout());

            IGNORE_HR( pLayoutBody->Paginate(plViewWidth, yInitialPageHeight, fFillLastPage) );
        }
        else if (_fFrameSet)
        {
            CPrintPage PP;

            Assert(ETAG_FRAMESET == pElementClient->Tag());

            if (yInitialPageHeight)
            {
                PP.yPageHeight = yInitialPageHeight;

                // Set _dci._sizeDst.cy to full page height again.
                _dci._sizeDst.cy = yFullSizeDst;
            }
            else
            {
                PP.yPageHeight = _dci._sizeDst.cy;
            }

            *plViewWidth = _dci._sizeDst.cx;
            IGNORE_HR(_paryPrintPage->AppendIndirect(&PP));
        }
    }
    else if (pElementClient && !_fFrameSet)
    {
        // If already laid out, a DoLayout means reset the scroll position to the top.
        pElementClient->GetUpdatedLayout()->OnScroll(1, SB_TOP, 0);
    }

    // Return number of pages.
    if (pcPages)
    {
        *pcPages = _fFrameSet ? 1 : _paryPrintPage->Size();
    }

    // Return height of last page.
    if (pyLastPageHeight)
    {
        if (_fFrameSet)
            *pyLastPageHeight = _dci._ptDst.y;
        else if (_paryPrintPage->Size())
            *pyLastPageHeight = (*_paryPrintPage)[_paryPrintPage->Size()-1].yPageHeight;
        else
            *pyLastPageHeight = 0;
    }

    _fLaidOut = TRUE;

Cleanup:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPrintDoc::DoDraw
//
//  Synopsis:   Lays out and paginates the document
//
//  Arguments:  prc [in]  rectangle of page to print into
//
//----------------------------------------------------------------------------

HRESULT
CPrintDoc::DoDraw(RECT * prcDraw, POINT * pptPageOffset)
{
    CFormDrawInfo   DI;
    CPoint          ptOrg(pptPageOffset ? *pptPageOffset : g_Zero.pt);

    Assert(prcDraw);
    Assert(_view.IsActive());

    DI.Init(PrimaryRoot());

    //
    // Set up the page rect, i.e. the rect relative to the page origin adjusted
    // for page margins and pptPageOffset passed in (OE header and repeated table footers).
    //

    ptOrg += ((CRect&)_rcClip).TopLeft().AsSize();
    _view.SetViewPosition(ptOrg);

    // Set cliprect to printable area
    DI._rcClipSet.SetRect(ptOrg, ((CRect*)prcDraw)->Size());

    //
    // Set up document offset.
    //

    // if this method returns FALSE, we can only print one page!
    WHEN_DBG(BOOL fOffsetSet =) _view.SetViewOffset(((CRect*)prcDraw)->TopLeft().AsSize());
    Assert(fOffsetSet || ((CRect*)prcDraw)->TopLeft() == g_Zero.pt);

    //
    // Finally ensure the view and render.
    //

    _view.EnsureView(LAYOUT_SYNCHRONOUS | LAYOUT_DEFERPAINT);
    _view.RenderView(&DI, &DI._rcClipSet);

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPrintDoc::ReadOldStyleHeaderOrFooter
//
//  Synopsis:   Get the value belonging to either header_... or footer_...
//              Allocate neccessary memory into ppHeaderFooter. It will hold the value from
//              the registry
//
//  Arguments:  hfKey           handle of PageSetup key in the registry
//              pValuePrefis    either "header" or "footer"
//              pHF             header or footer Info
//              ppHeaderFooter  the value string
//
//  Returns :   S_OK or E_FAIL (if not enough memory, or the registry cannot be read)
//
//----------------------------------------------------------------------------

HRESULT
CPrintDoc::ReadOldStyleHeaderOrFooter(
        HKEY hfKey,
        const TCHAR * pValuePrefix,
        CHeaderFooterInfo* pHF,
        TCHAR ** ppszHeaderFooter)
{
#define MAXVALUENAME 256

    Assert(pValuePrefix);
    Assert(pHF);

    DWORD   dwLeftLength  = 0;
    DWORD   dwRightLength = 0;
    TCHAR * pWork         = NULL;
    TCHAR   szLeftValueName[MAXVALUENAME];
    TCHAR   szRightValueName[MAXVALUENAME];
    HRESULT hr = E_FAIL;
    HRESULT hr2;

    *ppszHeaderFooter = NULL;

    Assert((_tcslen(pValuePrefix) + _tcslen(_T("_right"))) < MAXVALUENAME);
    if ((_tcslen(pValuePrefix) + _tcslen(_T("_right"))) >= MAXVALUENAME)
    {
        goto Cleanup;
    }

    _tcscpy((TCHAR*)&szLeftValueName,pValuePrefix);
    _tcscat((TCHAR*)&szLeftValueName,_T("_left"));

    _tcscpy((TCHAR*)&szRightValueName,pValuePrefix);
    _tcscat((TCHAR*)&szRightValueName,_T("_right"));

    hr2 = RegQueryValueEx(hfKey,(TCHAR*)&szLeftValueName,NULL,NULL,
                      NULL,&dwLeftLength);
    if (!SUCCEEDED(hr2))
        goto Cleanup;
    hr2 = RegQueryValueEx(hfKey,(TCHAR*)&szRightValueName,NULL,NULL,
                     NULL,&dwRightLength);
    if (!SUCCEEDED(hr2))
        goto Cleanup;
    if (dwLeftLength+dwRightLength > 0)
    {
        // +2 because we concat them with &b
        // -1 because both left and right contains a terminating
        // zero, but we need only one

        *ppszHeaderFooter = new(Mt(CPrintDocReadOldStyleHeaderOrFooter_ppszHeaderFooter)) TCHAR[dwLeftLength+dwRightLength+2-1];
        Assert(*ppszHeaderFooter);
        if (*ppszHeaderFooter)
        {
            RegQueryValueEx(hfKey,(TCHAR*)&szLeftValueName,NULL,NULL,
                            (LPBYTE)*ppszHeaderFooter,&dwLeftLength);
            _tcscat(*ppszHeaderFooter,_T("&b"));
            if (dwRightLength > 0)
            {
                pWork = new(Mt(CPrintDocReadOldStyleHeaderOrFooter_pWork)) TCHAR[dwRightLength];
                if (pWork != NULL)
                {
                    RegQueryValueEx(hfKey,(TCHAR*)&szRightValueName,NULL,NULL,
                                    (LPBYTE)pWork,&dwRightLength);
                    _tcscat(*ppszHeaderFooter,pWork);
                    delete [] pWork;
                }
            }
            if (pHF)
                pHF->SetHeaderFooter(*ppszHeaderFooter);
            hr = S_OK;
        }
    }
    else
    {
        *ppszHeaderFooter = new(Mt(CPrintDocReadOldStyleHeaderOrFooter_ppszHeaderFooter)) TCHAR[1];
        Assert(*ppszHeaderFooter);
        if (*ppszHeaderFooter)
        {
            _tcscpy(*ppszHeaderFooter,_T(""));
            if (pHF)
                pHF->SetHeaderFooter(*ppszHeaderFooter);
            hr = S_OK;
        }
    }
Cleanup:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPrintDoc::ReadNewStyleHeaderOrFooter
//
//  Synopsis:   Get the value belonging to either header or footer
//
//  Arguments:  hfKey       handle of PageSetup key in the registry
//              pValueName  either "header" or "footer"
//              pHF         header or footer Info
//
//  Returns :   S_OK or E_FAIL (if not enough memory, or the registry cannot be read)
//
//----------------------------------------------------------------------------

HRESULT
CPrintDoc::ReadNewStyleHeaderOrFooter(HKEY hfKey,const TCHAR* pValueName,CHeaderFooterInfo* pHF)
{
    DWORD   dwLength        = 0;
    TCHAR*  pHeaderFooter   = NULL;
    HRESULT hr              = E_FAIL;

    Assert(pValueName);
    Assert(pHF);

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
            pHeaderFooter = new(Mt(CPrintDocReadNewStyleHeaderOrFooter_pHeaderFooter)) TCHAR[dwLength];
            Assert(pHeaderFooter);
            if (pHeaderFooter)
            {
                RegQueryValueEx(hfKey,pValueName,NULL,NULL,
                                (LPBYTE)pHeaderFooter,&dwLength);
                if (pHF)
                    pHF->SetHeaderFooter(pHeaderFooter);
            }
        }
        else
        {
            if (pHF)
               pHF->SetHeaderFooter(_T(""));
        }
        hr = S_OK;
    }
    delete [] pHeaderFooter;
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPrintDoc::ReadOldStyleHeaderAndFooter
//
//  Synopsis:   Get the values belonging to the valuenames "header_..." and "footer_..." from the registry
//
//  Arguments:  keyPS       handle to PageSetup key in the registry
//              ppHeader    address of pointer to the header string
//              ppFooter    address of pointer to the footer string
//
//  Note:       ppHeader and ppFooter are allocated in the ReadOldStyleHeaderOrFooter procedure
//
//  Returns :   S_OK or E_FAIL (if not enough memory, or the registry cannot be read)
//
//----------------------------------------------------------------------------

inline HRESULT
CPrintDoc::ReadOldStyleHeaderAndFooter(HKEY keyPS, TCHAR ** ppHeader, TCHAR ** ppFooter)
{
    HRESULT hr;

    hr = ReadOldStyleHeaderOrFooter(keyPS, _T("header"), _pHInfo, ppHeader);
    if (hr)
        RRETURN(E_FAIL);

    hr = ReadOldStyleHeaderOrFooter(keyPS, _T("footer"), _pFInfo, ppFooter);
    if (hr)
        RRETURN(E_FAIL);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPrintDoc::ReadNewStyleHeaderAndFooter
//
//  Synopsis:   Get the values belonging to the valuenames "header" and "footer" from the registry
//
//  Arguments:  keyPS   handle to PageSetup key in the registry
//
//  Returns :   S_OK or E_FAIL (if not enough memory, or the registry cannot be read)
//
//----------------------------------------------------------------------------

inline HRESULT
CPrintDoc::ReadNewStyleHeaderAndFooter(HKEY keyPS)
{
    HRESULT hr;

    hr = ReadNewStyleHeaderOrFooter(keyPS, _T("header"), _pHInfo);
    if (hr)
        RRETURN(E_FAIL);

    hr = ReadNewStyleHeaderOrFooter(keyPS, _T("footer"), _pFInfo);
    if (hr)
        RRETURN(E_FAIL);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPrintDoc::CreateNewHeaderFooter
//
//  Synopsis:   Create the new style value entries : header and footer
//
//  Arguments:  keyPS       handle for the PageSetup key
//              pHeader     value of header
//              pFooter     value of footer
//
//----------------------------------------------------------------------------

inline void
CPrintDoc::CreateNewHeaderFooter(HKEY keyPS,TCHAR* pHeader,TCHAR* pFooter)
{
    int     iLen;
    HRESULT hr;

    Assert(pHeader);
    Assert(pFooter);

    if (!pHeader || !pFooter)
        return;

    iLen = _tcslen(pHeader) * sizeof(TCHAR);

    hr = RegSetValueEx(keyPS,_T("header"),0,REG_SZ,(const byte*)pHeader,iLen);
    if (SUCCEEDED(hr))
    {
        iLen = _tcslen(pFooter) * sizeof(TCHAR);

        hr = RegSetValueEx(keyPS,_T("footer"),0,REG_SZ,(const byte*)pFooter,iLen);
        if (!SUCCEEDED(hr))
        {
            RegDeleteValue(keyPS,_T("header"));
        }
    }
}
//+---------------------------------------------------------------------------
//
//  Member:     CPrintDoc::ReadHeaderAndFooter
//
//  Synopsis:   Read the header and footer strings from the registry
//              There is two kind of value names :
//              Old style : header_left,header_right,footer_left,footer_right
//              New style : header, footer
//              if we can read the new style return
//              if not we try to read the old style, and then create the new
//              style with either the old style values or the default.
//
//  Arguments:  None
//
//  Returns :   S_OK if the header and footer could be set, E_FAIL otherwise
//              (can fail if there is not enough memory to allocate the header
//               and footer or the registry couldn't be read)
//
//
//----------------------------------------------------------------------------

HRESULT
CPrintDoc::ReadHeaderAndFooter(void)
{
    HKEY     keyPageSetup;
    HRESULT  hr = E_FAIL;
    TCHAR*   pHeader = NULL;
    TCHAR*   pFooter = NULL;

    Assert(_pHInfo);
    Assert(_pFInfo);
    if (!_pHInfo || !_pFInfo)
        return S_OK;

    if (GetRegPrintOptionsKey(PRINTOPTSUBKEY_PAGESETUP, &keyPageSetup) == S_OK)
    {
        // Successfully set the keys to \HKLM\Software\Microsoft\Internet Explorer\PageSetup

        // try to read the new style values ( header, footer)
        hr = ReadNewStyleHeaderAndFooter(keyPageSetup);
        if (hr != S_OK)
        {
            // There are no such values, try to read header_left,header_right, etc. old style values
            hr = ReadOldStyleHeaderAndFooter(keyPageSetup, &pHeader, &pFooter);
            // Now create the new values (Header and Footer)
            if (hr == S_OK)
            {
                // we could read the old values, create the new values
                CreateNewHeaderFooter(keyPageSetup, pHeader, pFooter);
            }
            else
            {
                // we couldn't read the old values, create the new values with
                // default
                TCHAR*  pDefaultHeader = _T("&w&bPage &p of &P");
                TCHAR*  pDefaultFooter = _T("&u&b&d");
                CreateNewHeaderFooter(keyPageSetup,pDefaultHeader,pDefaultFooter);
            }
            delete [] pHeader;
            delete [] pFooter;
        }
        RegCloseKey(keyPageSetup);
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPrintDoc::SetPrintHeaderFooterURL
//
//  Synopsis:   Set the URL in the printer header and footer
//
//  Arguments:  pURL    pointer to the URL address string
//
//
//----------------------------------------------------------------------------

void
CPrintDoc::SetPrintHeaderFooterURL(TCHAR * pURL)
{
    if (_pHInfo)
        _pHInfo->SetHeaderFooterURL(pURL);
    if (_pFInfo)
        _pFInfo->SetHeaderFooterURL(pURL);
}



//+---------------------------------------------------------------------------
//
//  Member:     CPrintDoc::AfterLoadComplete
//
//  Synopsis:   Used to add decorations just before actually printing the
//              document (such as the table of URLs)
//
//  Returns:    void
//
//----------------------------------------------------------------------------

void
CPrintDoc::AfterLoadComplete(void)
{
    HRESULT         hr;
    CStr            cstrTable;
    TCHAR           szTblHeader[128], szRow1Header[128], szRow2Header[128];

    if (_PrintInfoBag.fShortcutTable)
    {
        CURLAry         aryURL;
        CURLAry         aryString;
        int             cURLs;
        int             i;

        hr = EnumContainedURLs(&aryURL, &aryString);
        if (hr)
            goto Cleanup;

        cURLs = aryURL.Size();
        Assert(cURLs == aryString.Size() && "bug in EnumContainedURLs, sizes of returned arrays do not match");

        // load up the strings from resources

        LoadString(GetResourceHInst(), IDS_PRINT_URLTITLE, szTblHeader, ARRAY_SIZE(szTblHeader));
        LoadString(GetResourceHInst(), IDS_PRINT_URLCOL1HEAD, szRow1Header, ARRAY_SIZE(szRow1Header));
        LoadString(GetResourceHInst(), IDS_PRINT_URLCOL2HEAD, szRow2Header, ARRAY_SIZE(szRow2Header));

        if (cURLs > 0)
        {
            cstrTable.Append(TEXT("<HR style=\"clear:all;\"><CENTER>"));
            cstrTable.Append(szTblHeader);
            cstrTable.Append(TEXT("<TABLE border=1><TR><TH>"));
            cstrTable.Append(szRow1Header);
            cstrTable.Append(TEXT("</TH><TH>"));
            cstrTable.Append(szRow2Header);
            cstrTable.Append(TEXT("</TH></TR>"));
        }

        for (i = 0; i < cURLs; ++i)
        {
            BOOL fDuplicate = FALSE;
            int iPrevious;

            // Duplicate?
            for (iPrevious = 0; iPrevious < i && !fDuplicate; ++iPrevious)
            {
                // IMPORTANT (27739): we are case-sensitive.  Same story in
                // CSpooler::DuplicatePrintJob().
                fDuplicate = StrCmpC(aryURL[i], aryURL[iPrevious]) == 0;
            }

            // Ignore duplicates.
            if (!fDuplicate)
            {
                cstrTable.Append(TEXT("<TR><TD>"));
                cstrTable.Append(aryString[i]);
                cstrTable.Append(TEXT("</TD><TD>"));
                cstrTable.Append(aryURL[i]);
                cstrTable.Append(TEXT("</TD></TR>"));
            }
        }

        cstrTable.Append(TEXT("</TABLE></CENTER>"));

Cleanup:

        // all the cleanup happens as the variables go out of scope


        // and now add this code into the HTML stream

        if (_pPrimaryMarkup->GetElementClient()->Tag() == ETAG_BODY)
        {
            IHTMLTxtRange * prange;
            BSTR            bstr = SysAllocString(cstrTable);

            if (bstr)
            {
                // Create and move range to end before paste operation
                _pPrimaryMarkup->createTextRange(
                    & prange, _pPrimaryMarkup->GetElementClient() );
                prange->collapse(FALSE);
                prange->pasteHTML(bstr);
                ClearInterface(&prange);

                SysFreeString(bstr);
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CPrintDoc::InitForPrint
//
//  Synopsis:   Initialize paint info for printing
//
//----------------------------------------------------------------------------

HRESULT
CPrintDoc::InitForPrint()
{
    int     width, height, xOffset, yOffset;
    SIZE    sizePage, sizeInch;
    RECT    rcDst;
    SIZEL   sizelSrc;
    HRESULT hr = S_OK;

#ifdef WIN16
    //BUGWIN16: incomplete. need to understand/fillup on whats happening below -
    // vamshi.
#else

    if (!_hic)
    {
        Assert(!"No information context (HIC) created");
        hr = E_FAIL;
        goto Cleanup;
    }

    // The top-level bounding rectangle
    // rtMargin is in himetrics convert to pixels.

    sizeInch.cx = GetDeviceCaps( _hic, LOGPIXELSX );
    sizeInch.cy = GetDeviceCaps( _hic, LOGPIXELSY );

    width  = GetDeviceCaps( _hic, PHYSICALWIDTH );
    height = GetDeviceCaps( _hic, PHYSICALHEIGHT );

    xOffset = GetDeviceCaps( _hic, PHYSICALOFFSETX );
    yOffset = GetDeviceCaps( _hic, PHYSICALOFFSETY );
#endif //!WIN16

    // We nix any margins the ped itself wants and use the margins as
    // specified in the PageSetup dialog (or the default).

    // _rtMargin is in 1/1000 inch units
    sizePage.cx  = width - MulDivQuick(_PrintInfoBag.rtMargin.right +
                                       _PrintInfoBag.rtMargin.left,
                                       sizeInch.cx, 1000 );
    sizePage.cy = height - MulDivQuick(_PrintInfoBag.rtMargin.bottom +
                                       _PrintInfoBag.rtMargin.top,
                                       sizeInch.cy, 1000 );

    // sizelSrc is in Himetrics

    sizelSrc.cx = MulDivQuick(sizePage.cx, 2540, sizeInch.cx);
    sizelSrc.cy = MulDivQuick(sizePage.cy, 2540, sizeInch.cy);

    SIZEL   sizelMarginDelta;

    sizelMarginDelta.cx = MulDivQuick(_PrintInfoBag.rtMargin.left, sizeInch.cx, 1000);
    sizelMarginDelta.cy = MulDivQuick(_PrintInfoBag.rtMargin.top, sizeInch.cy, 1000);

    if (sizelMarginDelta.cx < xOffset)
    {
        _PrintInfoBag.rtMargin.left = 1000*xOffset / sizeInch.cx;
    }
    if (sizelMarginDelta.cy < yOffset)
    {
        _PrintInfoBag.rtMargin.top = 1000*yOffset / sizeInch.cy;
    }

    rcDst.left   = max(0L, (LONG)(sizelMarginDelta.cx - xOffset));
    rcDst.right  = rcDst.left + sizePage.cx;
    rcDst.top    = max(0L, (LONG)(sizelMarginDelta.cy - yOffset));
    rcDst.bottom = rcDst.top + sizePage.cy;

    _rcClip = rcDst;

    _dci.CTransform::Init( &rcDst, sizelSrc, &sizeInch );

Cleanup:
    RRETURN(hr);
}
#endif // NO_PRINT




//+---------------------------------------------------------------------------
//
//  Member:     CDoc::IsPrintDoc
//
//  Synopsis:   Returns whether the root document of the document hierarchy
//              if of type CPrintDoc rather than CDoc.
//
//              Uses the two flags
//
//                  _fIdentifiedRootDocAsCDoc
//                  _fIdentifiedRootDocAsCPrintDoc
//
//              of which at most one can be set at any given point in time
//              to cache the information.
//
//----------------------------------------------------------------------------

BOOL CDoc::IsPrintDoc()
{
    // Make sure that not both _fIdentified flags are set.
    Assert(!_fIdentifiedRootDocAsCDoc || !_fIdentifiedRootDocAsCPrintDoc
           || !"Root document cannot be both CDoc and CPrintDoc.");

    // IMPORTANT:  Optimize speed for plain CDoc's.
    if (_fIdentifiedRootDocAsCDoc || !_pDocParent)
    {
        return FALSE;
    }

    if (_fIdentifiedRootDocAsCPrintDoc)
    {
        return TRUE;
    }

    // At this point neither of the two _fIdentified flags is set.
    Assert(!_fIdentifiedRootDocAsCDoc && !_fIdentifiedRootDocAsCPrintDoc
           && "If root document identified, don't do the work.");

    // Find out what our root doc is.
    if (_pDocParent->IsPrintDoc())
    {
        // This flag is final because we never run into false positives.
        _fIdentifiedRootDocAsCPrintDoc = TRUE;
        return TRUE;
    }
    else
    {
        // This flag is cleared by CDoc::SetParentDoc() whenever a new
        // parent doc is set.
        _fIdentifiedRootDocAsCDoc = TRUE;
        return FALSE;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::PrintBackgroundColorOrBitmap
//
//  Synopsis:   Returns cached flag indicating whether printing of backgrounds
//              is on or off.
//
//----------------------------------------------------------------------------

BOOL CDoc::PaintBackground()
{
    if (!IsPrintDoc())
        return TRUE;

#if DBG==1
    if (IsTagEnabled(tagPrintBackground))
        return TRUE;
#endif // DBG == 1

    // IsPrintDoc() returns TRUE if the rootdoc of this document is a printdoc,
    // so the rootdoc must be the printdoc.
    return (DYNCAST(CPrintDoc, GetRootDoc()))->_PrintInfoBag.fBackground;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::DontRunScripts
//
//  Synopsis:   Returns FALSE for CDoc's and TRUE for CPrintDoc's that were
//              are marked not to run scripts because they were saved out to
//              tempfiles.
//
//----------------------------------------------------------------------------

BOOL CDoc::DontRunScripts()
{
    if (!IsPrintDoc())
        return FALSE;

    // IsPrintDoc() returns TRUE if the rootdoc of this document is a printdoc,
    // so the rootdoc must be the printdoc.
    return (DYNCAST(CPrintDoc, GetRootDoc()))->_fDontRunScripts;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::TrustSecurityUI
//
//  Synopsis:   Returns FALSE for CDoc's and TRUE for CPrintDoc's that were
//              saved out to tempfiles.
//
//----------------------------------------------------------------------------

BOOL CDoc::TrustSecurityUI()
{
    if (!IsPrintDoc())
        return FALSE;

    // IsPrintDoc() returns TRUE if the rootdoc of this document is a printdoc,
    // so the rootdoc must be the printdoc.
    return (DYNCAST(CPrintDoc, GetRootDoc()))->_fTempFile;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::PrintJobsPending
//
//  Synopsis:   Returns TRUE if a spooler has a non-empty queue.
//
//----------------------------------------------------------------------------

BOOL CDoc::PrintJobsPending()
{
    CSpooler *  pSpooler = NULL;
    BOOL        fRet = FALSE;

    // Conditions for pending print jobs:
    // 1. There needs to be an active spooler (we don't want to create a spooler for this).
    // 2. We need to successfully get the spooler (hosts should go on with life if we can't get the spooler).
    // 3. The spooler has to be non-empty.
    if (S_OK == ::IsSpooler()
     && S_OK == THR(GetSpooler(&pSpooler)) && pSpooler
     && !pSpooler->IsEmpty())
    {
        // Return TRUE if there are jobs in the spooler.
        fRet = TRUE;
    }

    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetHDC
//
//  Synopsis:   Returns cached flag indicating whether printing of backgrounds
//              is on or off.
//
//----------------------------------------------------------------------------

HDC CDoc::GetHDC()
{
    if (!IsPrintDoc())
        return TLS(hdcDesktop);

    // IsPrintDoc() returns TRUE if the rootdoc of this document is a printdoc,
    // so the rootdoc must be the printdoc.
    return (DYNCAST(CPrintDoc, GetRootDoc()))->_hdc;
}

//+----------------------------------------------------------------
//
//  Member:    CDoc::GetAlternatePrintDoc
//
//  Arguments: pstrUrl (out): pointer to Url of active frame
//                            needs to be at least pdlUrlLen
//                            characters long.
//             cchUrl (in):   length of url array passed in.
//
//  Synopsis : Returns S_OK and the Url of alternate print doc if it exists.
//             Otherwise returns S_FALSE.
//
//-----------------------------------------------------------------

HRESULT CDoc::GetAlternatePrintDoc(TCHAR *pchUrl, DWORD cchUrl)
{
    CTreeNode *     pNode;
    CLinkElement *  pLink;
    HRESULT         hr = S_FALSE;

    if (!pchUrl || cchUrl < pdlUrlLen)
        return E_POINTER;

    if (!_pPrimaryMarkup->GetHeadElement())
        return S_FALSE;

    CChildIterator ci ( _pPrimaryMarkup->GetHeadElement() );

    while ( (pNode = ci.NextChild()) != NULL )
    {
        if ( pNode->Tag() == ETAG_LINK )
        {
            LPCTSTR pstrRel = NULL, pstrMedia = NULL, pstrUrl = NULL;

            // Found a link element.  Examine it.
            pLink = DYNCAST(CLinkElement, pNode->Element());

            pstrRel = pLink->GetAArel();

            if (!pstrRel || _tcsicmp(pstrRel, _T("alternate")))
                continue;

            pstrMedia = pLink->GetAAmedia();

            if (!pstrMedia || _tcsicmp(pstrMedia, _T("print")))
                continue;

            // We found a REL=alternate MEDIA=print candidate.  Lets
            // get the url.

            pstrUrl = pLink->GetAAhref();

            if (pstrUrl && (_tcslen(pstrUrl) > 0))
            {
                TCHAR achUrl[pdlUrlLen];
                DWORD cchDummy;

                // Obtain absolute Url.
                if (FAILED(CoInternetCombineUrl(_cstrUrl,
                                pstrUrl,
                                URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE,
                                achUrl,
                                ARRAY_SIZE(achUrl),
                                &cchDummy,
                                0))
                  || (_tcslen(achUrl) == 0))
                {
                    // Skip problematic Urls.
                    continue;
                }

                _tcsncpy(pchUrl, achUrl, cchUrl);
                hr = S_OK;
                break;
            }
        }
    }

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::PrintPlugInSite
//
//  Synopsis:   Prints plugin site directly.  Only used for printing Adobe
//              Acrobat plugin controls which only print when inplace-active.
//
//  Returns:    S_FALSE in most cases.  S_OK if the plugin is asked to print
//              directly.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::PrintPlugInSite()
{
    CLayout *       pChildLayout = NULL;
    COleSite *      pPlugInSite = NULL;
    HRESULT         hr = S_FALSE;
    DWORD_PTR       dwEnumCookie = 0;
    static OLECHAR * oszPrint = _T("Print");
    DISPID          dispidPrint = 0;
    unsigned int    nParam = 0;
    CElement *      pElementClient = _pPrimaryMarkup
                        ? _pPrimaryMarkup->GetElementClient()
                        : NULL;
    CLayout  *      pLayoutClient = NULL;
    INSTANTCLASSINFO * pici;

    // No plugin site unless the doc's rootsite's clientsite is a
    // body element.
    if (!pElementClient || ETAG_BODY != pElementClient->Tag())
        goto Cleanup;

    pLayoutClient = pElementClient->GetUpdatedLayout();

    // Get the first child site.
    pChildLayout = pLayoutClient->GetFirstLayout(&dwEnumCookie);
    if (!pChildLayout || ETAG_EMBED != pChildLayout->Tag())
        goto Cleanup;

    // We have a plugin site.
    pPlugInSite = DYNCAST(COleSite, pChildLayout->ElementOwner());

    // Insist on the plugin site being the only site.
    pChildLayout = pLayoutClient->GetNextLayout(&dwEnumCookie);
    if (pChildLayout)
        goto Cleanup;

    // Do we print this as a plugin?
    pici = pPlugInSite->GetInstantClassInfo();
    
    if (!pici || !(pici->dwCompatFlags & COMPAT_PRINTPLUGINSITE))
        goto Cleanup;

    // Make sure we have the IDispatch pointer
    if (!pPlugInSite->_pDisp)
        goto Cleanup;

    // Find out what the "Print" method's dispid is.  Should be 2 for Adobe Acrobat.
    hr = THR(pPlugInSite->_pDisp->GetIDsOfNames(IID_NULL, &oszPrint, 1, g_lcidUserDefault, &dispidPrint));
    if (hr)
        goto Cleanup;

    Assert(dispidPrint == 2);

    // Finally invoke the print method.  For Adobe this should return immediately after
    // posting a window message to the out-of-proc server.
    {
        DISPPARAMS      DispParams;
        VARIANT         varReturn;
        EXCEPINFO       excepinfo;

        VariantInit(&varReturn);
        DispParams.cNamedArgs         = 0;
        DispParams.rgdispidNamedArgs  = NULL;
        DispParams.cArgs = 0;

        hr = THR(pPlugInSite->_pDisp->Invoke(dispidPrint,
                                            IID_NULL,
                                            g_lcidUserDefault,
                                            DISPATCH_METHOD,
                                            &DispParams,
                                            &varReturn,
                                            &excepinfo,
                                            &nParam));
    }

Cleanup:
    if (pLayoutClient)
        pLayoutClient->ClearLayoutIterator(dwEnumCookie, FALSE);

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::PrintExternalIPrintObject
//
//  Synopsis:   Prints IPrint doc directly.  Only used for Office documents
//              when they are already in memory, hosted inside a frame or iframe,
//              and can be asked to print directly.
//
//  Returns:    S_OK if the IPrint document printed successfully.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::PrintExternalIPrintObject(struct PRINTINFOBAG *pPrintInfoBag, IPrint *pPrint)
{
    HRESULT hr = S_OK;
    LONG lFirstPage, lLastPage, lPages, iInterfaceCount;
    PAGESET PageSet, *pPageSet = &PageSet;
    DVTARGETDEVICE  *ptdOld = NULL, *ptd = NULL;
    CIPrintAry aryIPrint;

    Assert(_fFrameSet || pPrint);

    // 1. Obtain prerequisites.

    if (!pPrintInfoBag)
    {
        Assert(!"Null PrintInfoBag pointer");
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!(pPrintInfoBag->ptd))
    {
        hr = THR(::InitPrintHandles(pPrintInfoBag->hDevMode,
                                    pPrintInfoBag->hDevNames,
                                    &ptd,
                                    NULL,
                                    NULL));
        Assert(ptd);
        if (hr)
            goto Cleanup;
    }

    // If an IPrint pointer was passed in, that's the only one to print.
    if (pPrint)
    {
        IPrint **ppPrintAppend = NULL;

        hr = aryIPrint.AppendIndirect(NULL, &ppPrintAppend);
        if (!hr && ppPrintAppend)
        {
            *ppPrintAppend = pPrint;

            // Array wants ownership of IPrint refcount, so addref the guy.
            pPrint->AddRef();
        }
    }
    else if (_fFrameSet)
    {
        // Otherwise get all IPrint docs from frameset.
        hr = EnumFrameIPrintObjects(&aryIPrint);
        if (hr)
            goto Cleanup;
    }

    lFirstPage = pPrintInfoBag->fAllPages ? 1 : pPrintInfoBag->nFromPage;
    lLastPage  = pPrintInfoBag->fAllPages ? PAGESET_TOLASTPAGE : pPrintInfoBag->nToPage;
    pPageSet->cbStruct = sizeof(PAGESET);
    pPageSet->fOddPages = TRUE;
    pPageSet->fEvenPages = TRUE;
    pPageSet->cPageRange = 1;
    pPageSet->rgPages[0].nFromPage = lFirstPage;
    pPageSet->rgPages[0].nToPage = lLastPage;

#ifndef WIN16
    if (!g_fUnicodePlatform)
    {
        // IMPORTANT: We are not internally converting the DEVMODE structure back and forth
        // from ASCII to Unicode on non-Unicode platforms anymore because we are not touching
        // the two strings or any other member.  Converting the DEVMODE structure can
        // be tricky because of potential and common discrepancies between the
        // value of the dmSize member and sizeof(DEVMODE).  (25155)

        // As a result, we pass around DEVMODEW structures on unicode platforms and DEVMODEA
        // structures on non-unicode platforms.  Members of IPrint, however, always
        // expect DEVMODEW pointers.  Here we manually create a new target device structure
        // containing a DEVMODEW copy of the DEVMODEA original currently in ptd.

        ptdOld = ptd;
        ptd = TargetDeviceWFromTargetDeviceA(ptdOld);
    }
#endif // ndef WIN16

    // 2. Walk through the array and print all IPrint docs.

    for (iInterfaceCount = aryIPrint.Size()-1 ; iInterfaceCount >= 0 ; iInterfaceCount--)
    {
        IPrint *pPrintCurrent = aryIPrint[iInterfaceCount];

        Assert(pPrintCurrent && "Found NULL IPrint pointer in IPrint pointer array");

        if (pPrintCurrent)
        {
            hr = THR(pPrintCurrent->Print(
                        PRINTFLAG_MAYBOTHERUSER,
                        &ptd,
                        &pPageSet,
                        NULL,
                        NULL,
                        lFirstPage,
                        &lPages,        // out
                        &lLastPage      // out
                    ));
        }
    }

#ifndef WIN16
    if (!g_fUnicodePlatform)
#endif // ndef WIN16
    {
        // Free the temporary DEVMODEW copy of ptd.
        if (ptd)
            CoTaskMemFree(ptd);

        ptd = ptdOld;
    }

Cleanup:

    if (ptd)
    {
        DeinitPrintHandles(ptd, 0);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// UpdatePrintStatusBar() - result of a postmessage from the spooler...
//
//----------------------------------------------------------------------------
void
CDoc::UpdatePrintStatusBar(BOOL fPrinting)
{

    IOleCommandTarget *pCommandTarget=0;

    VARIANT varIn;
    HRESULT hr;

    if (_pClientSite)
    {
        hr = _pClientSite->QueryInterface(IID_IOleCommandTarget,
                                          (void**)&pCommandTarget);
        if (!OK(hr) || !pCommandTarget)
            goto Cleanup;

        VariantInit(&varIn);

        V_VT(&varIn) = VT_BOOL;
        V_BOOL(&varIn) = !!fPrinting;

        hr = pCommandTarget->Exec(&CGID_ShellDocView,
                                   SHDVID_SETPRINTSTATUS,
                                   0,
                                   &varIn,
                                   0);
    }

Cleanup:
    ReleaseInterface(pCommandTarget);
}




//+---------------------------------------------------------------------------
//
// UpdatePrintNotifyWindow() - tells the spooler about a new window to notify
//
//----------------------------------------------------------------------------
void
CDoc::UpdatePrintNotifyWindow(HWND hwnd)
{

    if (IsSpooler()==S_OK)
    {
        CSpooler    *pSpooler;
        HRESULT hr = THR(GetSpooler(&pSpooler));
        if (!hr)
        {
            Assert(pSpooler);
            pSpooler->SetNotifyWindow(hwnd);
        }
    }
    else
    {
        // no spooler, just turn it off
        UpdatePrintStatusBar(FALSE);
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::WaitingForNothingButControls
//
//  Synopsis:   Returns FALSE while we are loading stuff besides controls.
//
//----------------------------------------------------------------------------

BOOL
CDoc::WaitingForNothingButControls()
{
    UINT cLoadTasks = 0;
    CProgSink * pProgSink = GetProgSinkC();

    // If our state is less than LOADSTATUS_PARSE_DONE, we are doing more than
    // just waiting for controls.
    if (LoadStatus() < LOADSTATUS_PARSE_DONE)
    {
        return FALSE;
    }

    if (pProgSink)
    {
        // The load tasks we are interested in waiting for are
        // ALL LOAD TASKS - CONTROL LOAD TASKS - CONTROL READYSTATE TASKS - FRAME LOAD TASKS.
        // We have to subtract for the current level (FALSE) as well as for frame levels
        // "below" (TRUE).  We subtract frame load tasks because their specific load jobs are
        // also listed separately such that control problems inside frames are caught (53695)
        // (OliverSe).
        cLoadTasks = pProgSink->GetClassCounter((DWORD) -1)
                   - pProgSink->GetClassCounter(PROGSINK_CLASS_CONTROL, FALSE)
                   - pProgSink->GetClassCounter(PROGSINK_CLASS_CONTROL, TRUE)
                   - pProgSink->GetClassCounter(PROGSINK_CLASS_OTHER, FALSE)
                   - pProgSink->GetClassCounter(PROGSINK_CLASS_OTHER, TRUE)
                   - pProgSink->GetClassCounter(PROGSINK_CLASS_NOREMAIN, FALSE)
                   - pProgSink->GetClassCounter(PROGSINK_CLASS_NOREMAIN, TRUE);
    }

    return (cLoadTasks <= 0);
}


//+---------------------------------------------------------------------------
//+---------------------------------------------------------------------------
//
//  Member:     CDoc::UnwedgeFromPrinting
//
//  Synopsis:   Unwedges OnMethodCall mechanism after help app prints us (53416).
//
//----------------------------------------------------------------------------

void
CDoc::UnwedgeFromPrinting()
{
    // 53416: Help (hh.exe) loads documents to be printed, but doesn't wait until they finish loading.
    // Thus the document (and its hwnd) is destroyed before its posted OnMethodCall arrives thus wedging
    // the thread's method calls.
    if (_fUnwedgeFromPrinting)
    {
        THREADSTATE * pts = GetThreadState();
        if (pts && pts->gwnd.fMethodCallPosted)
        {
            EnterCriticalSection(&pts->gwnd.cs);
            pts->gwnd.fMethodCallPosted = FALSE;
            LeaveCriticalSection(&pts->gwnd.cs);
        }

        _fUnwedgeFromPrinting = FALSE;
    }
}


//
//  Member:     CPrintDoc::DrawHeaderFooter
//
//  Synopsis:   Draws Header and Footer only if it is printing and
//              this is a RootSite
//
//----------------------------------------------------------------------------

extern class CFontCache & fc();

HRESULT
CPrintDoc::PrintHeaderFooter(int cPage)
{
    HFONT           hFont = 0;
    IFont *         pFont = NULL;
    CFormDrawInfo   DI;
    HRESULT         hr;
    POINT           ptViewPortOriginOld;
    CCcs *          pccs = NULL;

    DI.Init(PrimaryRoot());

    SetViewportOrgEx(_hdc, 0, 0, (POINT *)&ptViewPortOriginOld);

    //  Cook up our own font, consider the style sheet stuff

    // This code segment was copied from eselect.cxx: AcquireFont()
    const CCharFormat * pcf = PrimaryRoot()->GetFirstBranch()->GetCharFormat();
    if ( pcf )
    {
        pccs = fc().GetCcs(_hdc, &DI, pcf);
    }

    if (pccs)
    {
        hFont = pccs->GetBaseCcs()->_hfont;
    }
    else
    {
        FONTDESC fd;

        memset(&fd, 0, sizeof(fd));
        fd.cbSizeofstruct = sizeof(fd);

        // BUGBUG (cthrash) The fact that OleCreateFontIndirect fails
        // miserably when you try and create a non-sbcs (when charset
        // is not ANSI_CHARSET, DEFAULT_CHARSET, or SYMBOL_CHARSET) is
        // a real problem.  We obviously don't want MS Sans Serif
        // everywhere now do we?

        fd.lpstrName = TEXT("MS Sans Serif");
        fd.cySize.Lo = 8;
        fd.sWeight = FW_NORMAL;
        hr = THR(OleCreateFontIndirect(&fd, IID_IFont, (void**) &pFont));
        if (SUCCEEDED(hr))
        {
            hr = THR(pFont->get_hFont(&hFont));
        }
    }

    if (Header())
    {
        Header()->UpdatePageNumber(cPage+1);
        Header()->Draw(&DI, pcf, TRUE, hFont);
    }

    if (Footer())
    {
        Footer()->UpdatePageNumber(cPage+1);
        Footer()->Draw(&DI, pcf, FALSE, hFont);
    }

    SetViewportOrgEx(_hdc, ptViewPortOriginOld.x, ptViewPortOriginOld.y, (POINT *)NULL);

    if (pccs)
    {
        pccs->Release();
    }

    return S_OK;
}
