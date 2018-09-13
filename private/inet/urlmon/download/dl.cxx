// ===========================================================================
// File: DL.CXX
//    implements CDownload, CBindStatusCallback classes
//

#include <cdlpch.h>

// SILENT MODE
#include <winineti.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <comcat.h>

#include <initguid.h>       // office antivirus goo
#define AVVENDOR            // don't look at unwanted office defs
#include <msoav.h>

extern char g_szOCXTempDir[MAX_PATH];
extern IEnumFORMATETC *g_pEFmtETC;
extern FORMATETC g_rgfmtetc[];

// ---------------------------------------------------------------------------
// %%Function: CDownload::CDownload
// CDownload is the basic download obj.
// ---------------------------------------------------------------------------

CDownload::CDownload(LPCWSTR szURL, FILEXTN extn, HRESULT *phr)
{
    DllAddRef();
    DWORD len = lstrlenW(szURL); // make private copy

    m_url = new WCHAR [len + 1];

    if (m_url)
        StrCpyW(m_url, szURL);
    else
        *phr = E_OUTOFMEMORY;

    m_pmk = 0;
    m_pbc = 0;
    m_pbsc = 0;

    m_pdlnext= NULL;

    m_ParentCDL.RemoveAll();

    m_extn = extn;
    m_pFileName = NULL; // we don't know the dest filename
                        // till we create a temp file in the first
                        // notification of OnDataAvailable
                        // This is guaranteed to get set before
                        // OnStopBinding

    m_ulProgress = 0;
    m_ulProgressMax = 0;

    m_state = DLSTATE_INIT;

    m_psess = NULL;
    m_pFilesToExtract = NULL;

    m_pSetuphead = NULL;

    m_hPostData = NULL;
    m_cbPostData = 0;

    m_hrOSB = S_OK;
    m_hrStatus = S_OK;
    m_hrResponseHdr = S_OK;

    m_bCompleteSignalled = FALSE;

    m_flags = DL_FLAGS_INIT;

    m_SetupHooks.RemoveAll();

    m_JavaSetupList.RemoveAll();

    m_pUnkForCacheFileRelease = NULL;

    m_pbJavaTrust = NULL;

    m_wszDistUnit = NULL;

    m_pcbhList = NULL;
    m_ppmkContext = NULL;
    m_grfBINDF = 0;

#ifdef _ZEROIMPACT
    m_bIsZeroImpact = FALSE;
#endif

    m_bExactVersion = FALSE;
}  // CDownload

// ---------------------------------------------------------------------------
// %%Function: CDownload::~CDownload
// ---------------------------------------------------------------------------

CDownload::~CDownload()
{
    int               i;
    CCodeBaseHold    *pcbh = NULL;
    LISTPOSITION      lpos = 0;

    CleanUp();
    LISTPOSITION pos = m_ParentCDL.GetHeadPosition();
    int iNum = m_ParentCDL.GetCount();

    Assert(iNum == 0);
    for (i=0; i < iNum; i++) {
        CParentCDL *pParentCDL = m_ParentCDL.GetNext(pos); // pass ref!
        delete pParentCDL;
    }
    m_ParentCDL.RemoveAll();

    if (m_pcbhList != NULL) {
        lpos = m_pcbhList->GetHeadPosition();
        while (lpos) {
            pcbh = m_pcbhList->GetNext(lpos);
            delete pcbh;
        }
        m_pcbhList->RemoveAll();
    }
    SAFEDELETE(m_pcbhList);
    SAFEDELETE(m_wszDistUnit);

    pos = m_SetupHooks.GetHeadPosition();
    iNum = m_SetupHooks.GetCount();
    for (i=0; i < iNum; i++) {
        CSetupHook *pSetupHook = m_SetupHooks.GetNext(pos); // pass ref!
        delete pSetupHook;
    }
    m_SetupHooks.RemoveAll();

    pos = m_JavaSetupList.GetHeadPosition();
    iNum = m_JavaSetupList.GetCount();
    for (i=0; i < iNum; i++) {
        CJavaSetup *pJavaSetup = m_JavaSetupList.GetNext(pos); // pass ref!
        delete pJavaSetup;
    }
    m_JavaSetupList.RemoveAll();
    DllRelease();
}  // ~CDownload

// ---------------------------------------------------------------------------
// %%Function: CDownload::HasJavaPermissions
// ---------------------------------------------------------------------------
BOOL
CDownload::HasJavaPermissions()
{
    if (m_pbJavaTrust) {
        // new jaavcypt > 2151 succeeds even if one of activex/java
        // is allowed

        return  (m_pbJavaTrust->pbJavaPermissions != NULL);
    }

    return FALSE;
}
// ---------------------------------------------------------------------------
// %%Function: CDownload::HasAllActiveXPermissions
// ---------------------------------------------------------------------------
BOOL
CDownload::HasAllActiveXPermissions()
{
    PJAVA_TRUST              pbJavaTrust = NULL;

    if (m_pbJavaTrust) {
        // new jaavcypt > 2151 succeeds even if one of activex/java
        // is allowed

        return  m_pbJavaTrust->fAllActiveXPermissions;
    }
    else {
        pbJavaTrust = GetCodeDownload()->GetJavaTrust();
        if (pbJavaTrust) {
            return pbJavaTrust->fAllActiveXPermissions;
        }
    }

    return FALSE;
}

// ---------------------------------------------------------------------------
// %%Function: CDownload::CompleteSignal
// ---------------------------------------------------------------------------
HRESULT
CDownload::CompleteSignal(HRESULT hrOSB, HRESULT hrStatus, HRESULT hrResponseHdr, LPCWSTR szError)
{

    int i, iNum;
    LISTPOSITION pos;

    m_hrOSB = hrOSB;
    m_hrStatus = hrStatus;
    m_hrResponseHdr = hrResponseHdr;

    m_bCompleteSignalled = TRUE;

restart:

    iNum = m_ParentCDL.GetCount();
    Assert(iNum);

    pos = m_ParentCDL.GetHeadPosition();

    for (i=0; i < iNum; i++) {
        CParentCDL *pParentCDL = m_ParentCDL.GetNext(pos); // pass ref!

        if (!pParentCDL->m_bCompleteSignalled) {

            // unsignalled code download
            pParentCDL->m_bCompleteSignalled = TRUE;
            pParentCDL->m_pcdl->CompleteOne( this ,hrOSB, hrStatus, hrResponseHdr, szError);

            if (iNum > 1) {
                // failed complete reports could cause CodeDownloads to release
                // us and thus change the list
                goto restart;
            }
        }

    }

    return S_OK;
}


// ---------------------------------------------------------------------------
// %%Function: CDownload::AddParent(CCodeDownload *pcdl)
// ---------------------------------------------------------------------------
HRESULT
CDownload::AddParent(CCodeDownload *pcdl)
{
    HRESULT hr = S_OK;

    CParentCDL *pParentCDL = new CParentCDL(pcdl);

    if (pParentCDL)
        m_ParentCDL.AddTail(pParentCDL);
    else
        hr = E_OUTOFMEMORY;


    if (SUCCEEDED(hr)) {
        if (m_bCompleteSignalled) {
            pParentCDL->m_bCompleteSignalled = TRUE;
            pcdl->CompleteOne(this ,m_hrOSB, m_hrStatus, m_hrResponseHdr, NULL);
        } else {

            hr = FAILED(m_hrOSB)?m_hrOSB:(FAILED(m_hrStatus)?m_hrStatus:(FAILED(m_hrResponseHdr)?m_hrResponseHdr:S_OK));

        }
    }

    return hr;

}

// ---------------------------------------------------------------------------
// %%Function: CDownload::IsSignalled(CCodeDownload *pcdl)
// ---------------------------------------------------------------------------
BOOL
CDownload::IsSignalled(CCodeDownload *pcdl)
{
    CParentCDL            *pParentCDL = NULL;
    BOOL                   bRet = FALSE;
    LISTPOSITION           pos = 0;
    DLSTATE                dls;
    int                    iNum = 0;
    int                    i;

    dls = GetDLState();
    if (dls == DLSTATE_DONE || dls == DLSTATE_READY_TO_SETUP) {
        bRet = TRUE;
        goto Exit;
    }

    iNum = m_ParentCDL.GetCount();
    Assert(iNum);
    pos = m_ParentCDL.GetHeadPosition();

    for (i=0; i < iNum; i++) {
        pParentCDL = m_ParentCDL.GetNext(pos); // pass ref!
        if (pParentCDL->m_pcdl == pcdl && pParentCDL->m_bCompleteSignalled) {
            bRet = TRUE;
            break;
        }
    }

Exit:

    return bRet;
}

// ---------------------------------------------------------------------------
// %%Function: CDownload::Abort(CCodeDownload *pcdl)
// ---------------------------------------------------------------------------
HRESULT
CDownload::Abort(CCodeDownload *pcdl)
{
    int i;
    CParentCDL *pThisParentCDL = NULL;
    BOOL bDelinkParent = FALSE;
    BOOL fWaitForAbortCompletion = FALSE;
    HRESULT hr = S_OK;

    int iNum = m_ParentCDL.GetCount();
    Assert(iNum);
    LISTPOSITION pos = m_ParentCDL.GetHeadPosition();

    for (i=0; i < iNum; i++) {
        CParentCDL *pParentCDL = m_ParentCDL.GetNext(pos); // pass ref!

        if (pParentCDL->m_pcdl == pcdl)
            pThisParentCDL = pParentCDL;
        else if ( !pParentCDL->m_bCompleteSignalled )
            bDelinkParent = TRUE;
    }

    Assert(pThisParentCDL);

    if (!pThisParentCDL)
        return E_FAIL;


    if (bDelinkParent) {

        // multiple code downloads interested in this
        // delink this parent, by marking as complete signalled

        pThisParentCDL->m_bCompleteSignalled = TRUE;

        return S_OK;
    }

    switch ( GetDLState())  {

    case DLSTATE_BINDING:

        GetBSC()->GetBinding()->Abort();

        break;

    case DLSTATE_ABORT:

        // have aborted this but the OSB has not been recieved yet
        // so wait for that to come to us before we complteall
        // or post the setup packet to completeall

        fWaitForAbortCompletion = TRUE;
        break;

    case DLSTATE_DONE:
    case DLSTATE_READY_TO_SETUP:

        break;

    default:

        // packet processing pending for this state. we will check for
        // DLSTATE_ABORT in each packet processing state and if true
        // it will call CompleteOne(us), which marks each piece DLSTATE_DONE

        SetDLState(DLSTATE_ABORT);
        fWaitForAbortCompletion = TRUE;

    }

    if (fWaitForAbortCompletion) {

        hr = S_FALSE;
    }

    return hr;

}


// ---------------------------------------------------------------------------
// %%Function: CDownload::ReleaseParent(CCodeDownload *pcdl)
// ---------------------------------------------------------------------------
HRESULT
CDownload::ReleaseParent(CCodeDownload *pcdl)
{
    int iNum = m_ParentCDL.GetCount();

    Assert(iNum);
    LISTPOSITION pos = m_ParentCDL.GetHeadPosition();

    for (int i=0; i < iNum; i++) {
        CParentCDL *pParentCDL = m_ParentCDL.GetNext(pos); // pass ref!

        if (pParentCDL->m_pcdl == pcdl) {

            // found the item
            // getnext would have stepped past the position
            pos = m_ParentCDL.Find(pParentCDL);
            m_ParentCDL.RemoveAt(pos);
            iNum = m_ParentCDL.GetCount();
            if (iNum == 0) {

                CleanupFiles();

                delete this;
            }

                        return S_OK;
        }
    }

    // not found in list
    Assert(TRUE);

    return E_FAIL;
}


// ---------------------------------------------------------------------------
// HRESULT CDownload::IsDownloadedVersionRequired()
// returns S_OK if downloaded version is required
// error if local version is OK and new verison is not required
// ---------------------------------------------------------------------------
HRESULT CDownload::IsDownloadedVersionRequired()
{
    HRESULT hr = S_OK;
    char szFullURL[INTERNET_MAX_URL_LENGTH];
    DWORD dwLen = INTERNET_MAX_URL_LENGTH;
    FILETIME *pftLastMod = GetCodeDownload()->GetLastModifiedTime();
    HANDLE hf = INVALID_HANDLE_VALUE;

    if (!GetCodeDownload()->LocalVersionPresent()) {
        // if no prev version always download
        return hr;
    } else {
        // if prev version exists, but we are not doing Get Latest
        // then accept the download.
        if (!GetCodeDownload()->NeedLatestVersion())
            return hr;
    }

    if (GetMoniker() != GetCodeDownload()->GetContextMoniker()){
        // if we are not the context (or the main moniker then
        // -1 does not apply (to secondary CABs)

        return hr;
    }

    dwLen = WideCharToMultiByte(CP_ACP, 0, GetURL(), -1,
                    szFullURL, INTERNET_MAX_URL_LENGTH, NULL, NULL);

    Assert(dwLen);

    if (!dwLen) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    if (StrCmpNI(szFullURL, "file:", 5) == 0) {

        WIN32_FIND_DATA fd;

        HANDLE hf = FindFirstFile(GetFileName(), &fd);

        if (hf == INVALID_HANDLE_VALUE) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
        
        // BUGBUG: Defend against Bug#40696 - Vatsan should check to see if this is the _right_ defense.
        if ( pftLastMod != NULL &&
             CompareFileTime(pftLastMod, &(fd.ftLastWriteTime)) >= 0) {

            // if the file needs no upgrade then fail!
            // if we succeed then an update will take place.
            hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
            goto Exit;
        }


    }

Exit:

    if ( hf != INVALID_HANDLE_VALUE)
        FindClose(hf);

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CDownload::GetFriendlyName
// ---------------------------------------------------------------------------
HRESULT CDownload::GetFriendlyName(LPSTR szUrlPath, LPSTR *ppBaseFileName)
{
    HRESULT hr = S_OK;
    char szFullURL[INTERNET_MAX_URL_LENGTH];
    DWORD dwLen = INTERNET_MAX_URL_LENGTH;
    URL_COMPONENTS UrlComponents;
    char *pBaseFileName = NULL;

    dwLen = WideCharToMultiByte(CP_ACP, 0, GetURL(), -1,
                    szFullURL, INTERNET_MAX_URL_LENGTH, NULL, NULL);


    memset(&UrlComponents, 0, sizeof(URL_COMPONENTS));
    UrlComponents.dwStructSize = sizeof(URL_COMPONENTS);

    UrlComponents.lpszUrlPath = szUrlPath;
    UrlComponents.dwUrlPathLength = INTERNET_MAX_URL_LENGTH;

    if (!InternetCrackUrl( szFullURL, 0,
        ICU_DECODE, &UrlComponents)) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    Assert(UrlComponents.lpszUrlPath);
    Assert(UrlComponents.dwUrlPathLength);

    if ( !UrlComponents.dwUrlPathLength ||
        !UrlComponents.lpszUrlPath ) {

        hr = E_UNEXPECTED;
        goto Exit;
    }

    if (ppBaseFileName)
        GetExtnAndBaseFileName(szUrlPath, ppBaseFileName);

Exit:
    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CDownload::SniffType
// ---------------------------------------------------------------------------
HRESULT CDownload::SniffType()
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HRESULT hr = S_OK;
    DWORD dwSignature;
    DWORD dwBytesRead = 0;

#define CAB_SIG 0x4643534d

    if (GetExtn() != FILEXTN_CAB) {


        if ( (hFile = CreateFile(GetFileName(), GENERIC_READ, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE) {

            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        if ((ReadFile(hFile, &dwSignature, sizeof(DWORD), &dwBytesRead, NULL))
            && (dwSignature == CAB_SIG)) {

            SetURLAndExtn(NULL, FILEXTN_CAB);
        } else {

            // here if its not a CAB
            // check if of compatible type

            hr = IsCompatibleFile(GetFileName());
        }

    }

Exit:

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CDownload::VerifyTrust
// ---------------------------------------------------------------------------
VOID CDownload::VerifyTrust()
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HRESULT hr = S_OK;
    HWND hWnd = GetCodeDownload()->GetClientBinding()->GetHWND();
    WCHAR szDisplayUrl[INTERNET_MAX_URL_LENGTH];
    DWORD cchDisplayUrl = INTERNET_MAX_URL_LENGTH;
    LPSTR szCatalogFile = NULL;

    CUrlMkTls tls(hr); // hr passed by reference!
    if (FAILED(hr))     // if tls ctor failed above
        goto Exit;

    if ( GetDLState() == DLSTATE_ABORT) {
        hr = E_ABORT;
        goto Exit;
    }

    // sniff file for detecting CAB extensions
    // and if not CAB, assume PE and check if of compatible type
    // before calling trust on it. The reason we presniff for
    // compat is because it will make for better user experience to
    // fail if not of correct binary before we present trust dialogs

    hr = SniffType();

    if (FAILED(hr))
        goto Exit;

    Assert(tls->pTrustCookie);

    // need to serialize all trust verification on this thread
    // grab the trust cookie

    hr = tls->pTrustCookie->Acquire(this);
    if (hr != S_OK) {

        Assert(!tls->pTrustCookie->IsFree());
        Assert(!tls->pTrustCookie->IsOwner(this));

        return; // wait till we get posted a message when the current owner
                // relinquishes the cookie
    }

    // have the cookie
    Assert(tls->pTrustCookie->IsOwner(this));

    if ( (hFile = CreateFile(GetFileName(), GENERIC_READ, FILE_SHARE_READ,
                    NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // form friendly displayable URL (ie. decode) in browser mode
    hr = UrlUnescapeW((LPWSTR)GetURL(), szDisplayUrl, &cchDisplayUrl, 0);

    Assert(!GetCodeDownload()->IsSilentMode() || hWnd == INVALID_HANDLE_VALUE);

    // Try to get a catalog file
    szCatalogFile = GetCodeDownload()->GetCatalogFile();

    hr = m_wvt.VerifyTrust(hFile, hWnd, &m_pbJavaTrust, szDisplayUrl, 
                           GetCodeDownload()->GetClientBinding()->GetHostSecurityManager(),
                           (char *)GetFileName(), szCatalogFile, this);

    if(SUCCEEDED(hr)) {

        SetTrustVerified();

    } else {

        // trust failed on this file. Delete it from the cache for
        // added safety. 

        // remove entry from cache only if we're not in silent mode.
        // or we are in silent mode and the hr != TRUST_E_SUBJECT_NOT_TRUSTED
        // when ui choice is NONE, WVT reurns the special error code to
        // mean that all was OK but could not trust because we did not
        // allow them to put up confirmation UI.

        if (!GetCodeDownload()->IsSilentMode())
        {
            CHAR szURL[INTERNET_MAX_URL_LENGTH];
            DWORD cchURL = INTERNET_MAX_URL_LENGTH;

            WideCharToMultiByte(CP_ACP, 0, GetURL(), -1, szURL,
                        INTERNET_MAX_URL_LENGTH, 0,0);

            // If we still have the file open when we call DeleteUrlCacheEntry, then
            // WinInet won't be able to delete it. Having untrusted bits in the cache
            // is dangerous.
            if ( hFile != INVALID_HANDLE_VALUE)
            {
                CloseHandle(hFile);
                hFile = INVALID_HANDLE_VALUE;
            }

            DeleteUrlCacheEntry(szURL);
        }
        else
            GetCodeDownload()->SetTrustSomeFailed();
    }

Exit:

    // reset status to resume the rest of download if we're in
    // silent mode
    if (GetCodeDownload()->IsSilentMode() && FAILED(hr) && (hr != E_ABORT))
        hr = S_OK;

    if (tls->pTrustCookie->IsOwner(this)) {
        tls->pTrustCookie->Relinquish(this);
    }

    if (SUCCEEDED(hr)) {

        CCDLPacket *pPkt= new CCDLPacket(CODE_DOWNLOAD_PROCESS_PIECE, this, 0);

        if (pPkt) {
            hr = pPkt->Post();
        } else {
            hr = E_OUTOFMEMORY;
        }

    }

    if (FAILED(hr)) {

        // does all the master state analysis
        CompleteSignal(hr, S_OK, S_OK, NULL);
    }

    if ( hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return;
}

// ---------------------------------------------------------------------------
// %%Function: CDownload::SetCdlProtocol
// ---------------------------------------------------------------------------
HRESULT
CDownload::SetUsingCdlProtocol(LPWSTR wszDistUnit)
{
    HRESULT hr = S_OK;

    m_wszDistUnit = new WCHAR [lstrlenW(wszDistUnit) + 1];

    if (m_wszDistUnit)
        StrCpyW(m_wszDistUnit, wszDistUnit);
    else
        hr = E_OUTOFMEMORY;

    m_flags |= DL_FLAGS_CDL_PROTOCOL;

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CDownload::ExtractManifest()
// ---------------------------------------------------------------------------
HRESULT
CDownload::ExtractManifest(FILEXTN extn, LPSTR szFileName, LPSTR& pBaseFileName)
{
    CCodeDownload *pcdl = GetCodeDownload();
    char * pFileName;
    PSESSION psess = NULL;
    PFNAME pf = NULL;
    HRESULT hr = S_FALSE;   // assume not found

    Assert(m_psess);

    for (pf = m_psess->pFileList; pf != NULL; pf = pf->pNextName) {

        FILEXTN curextn = ::GetExtnAndBaseFileName(pf->pszFilename,
                                                &pBaseFileName);

        if (( curextn == extn) && 
            ((szFileName[0] == '\0') || 
                (lstrcmpi(szFileName, pBaseFileName) == 0))) {

            FNAME fne;
            memset(&fne, 0, sizeof(FNAME));
            fne.pszFilename = pf->pszFilename;

            // INF present in CAB, extract it and process it
            m_psess->pFilesToExtract = &fne;
            m_psess->flags &= ~SESSION_FLAG_ENUMERATE; // already enumerated

            if (FAILED((hr=::Extract(m_psess, GetFileName()))))
                goto Exit;

            // side effect!
            // if extract succeeds we also have set the return hr to S_OK.
            // hr = S_OK;

            m_psess->pFilesToExtract = NULL;

            if (!catDirAndFile(szFileName, MAX_PATH, m_psess->achLocation,
                               pf->pszFilename)) {
                hr = E_UNEXPECTED;
            }

            goto Exit;

        }
    }

Exit:

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CDownload::ProcessPiece
//      CBSC::OnStopBinding calls us as soon as a piece gets downloaded
//      and trust is verified.
//      ie. this CDownload obj has completed binding
//      Depending on the Content type we will process further
// This triggers a change state in our state machine. Depending on the
// obj we have downloaded (a CAB or INF or DLL/OCX/EXE) we:
//
// OCX:
//    Csetup for this download is usually previously created
//      mark this download as done and
//    call into main CodeDownload::CompleteOne (state analyser)
//
// CAB:
//    if we don't have an INF already we look for one in the CAB
//           if INF in CAB
//               process INF (may trigger further extractions/downloads/Csetup)
//           else
//              look for primary OCX in CAB and create CSetup or it.
//
// INF:
//      Process INF
//
// ---------------------------------------------------------------------------
VOID
CDownload::ProcessPiece()
{
    CCodeDownload *pcdl = GetCodeDownload();
    char * pFileName;
    char *pBaseFileName;
    PSESSION psess = NULL;
    PFNAME pf = NULL;
    HRESULT hr = S_OK; // assume all OK
    CSetup *pSetup;
    char szBuf[INTERNET_MAX_URL_LENGTH];
    char szCatalogBuf[INTERNET_MAX_URL_LENGTH];
    FILEXTN extn = m_extn;

    if ( GetDLState() == DLSTATE_ABORT) {
        hr = E_ABORT;
        goto Exit;
    }

    //REVIEW: Virus scanning a CAB or INF file may not be a bright thing to do since
    //        they are not executable.

    if (FAILED(hr = PerformVirusScan((char *)GetFileName()))) {
        goto Exit;
    }

    switch (extn) {

    case FILEXTN_EXE:
    case FILEXTN_OCX:
    case FILEXTN_DLL:
    case FILEXTN_NONE:
    case FILEXTN_UNKNOWN:

        pSetup = GetSetupHead();

        if (pSetup) {
            // If a CSetup exists for this m_pdl then initialize its
            // m_pSrcFileName using the m_pFileName

            Assert(pSetup->GetNext() == NULL);

            pSetup->SetSrcFileName(GetFileName());

        } else {

            if (!HasAllActiveXPermissions()) {

                if (GetCodeDownload()->IsSilentMode())
                {
                    GetCodeDownload()->SetBitsInCache();
                }

                hr = TRUST_E_FAIL;
                goto Exit;
            }

            // If no CSetup exists then make one and attach it to the m_pdl
            // initiated at top level

            hr = GetFriendlyName(szBuf, &pBaseFileName);
            if (pBaseFileName[0] == '\0') {
                hr = E_UNEXPECTED; // No filename to setup!
                goto Exit;
            }

            if (FAILED(hr)) {
                goto Exit;
            }

            pSetup = new CSetup(GetFileName(), pBaseFileName, extn,
                                                pcdl->GetDestDirHint(), &hr);

            if (!pSetup) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            } else if (FAILED(hr)) {
                delete pSetup;
                goto Exit;
            }

            AddSetupToList(pSetup);

        }

        break;

    case FILEXTN_CAB:
        // if CAB then make SESSION for this CDownload
        Assert(!(GetSession()));

        psess = new SESSION;
        if (!psess) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        SetSession(psess);

        // Initialize the structure
        psess->pFileList        = NULL;
        psess->cFiles           = 0;
        psess->cbCabSize        = 0;
        psess->flags = SESSION_FLAG_ENUMERATE;

        // extract files in a CAB into a unique dir so that
        // parallel downloads of CABs containing same name
        // components can go on without conflict.
        // By serailizing the setup phase we make sure the right
        // latest version is finally left on client machine.

        hr =  MakeUniqueTempDirectory(g_szOCXTempDir, psess->achLocation);

        if (FAILED(hr)) {
            goto Exit;
        }

        psess->pFilesToExtract = NULL; // just enumerate first

        if (!pcdl->HaveManifest() || NeedToExtractAllFiles()) {
            psess->flags |= SESSION_FLAG_EXTRACT_ALL;
        }

        // enumerate the files of the CAB
        if (FAILED((hr = ::Extract(psess, GetFileName()))))
            goto Exit;

        if (!pcdl->HaveManifest() || NeedToExtractAllFiles()) {
            psess->flags |= SESSION_FLAG_EXTRACTED_ALL;
        }

        // if we don't have INF already look for one
        if (!pcdl->HaveManifest()) {

            // Extract catalog file
            szCatalogBuf[0] = '\0';
            if (ExtractManifest(FILEXTN_CAT, szCatalogBuf, pBaseFileName) == S_OK) {
                if (FAILED(pcdl->SetCatalogFile(szCatalogBuf))) {
                    goto Exit;
                }
            } 

            szBuf[0] = '\0';
            hr = ExtractManifest(FILEXTN_OSD , szBuf, pBaseFileName);
            if (FAILED(hr))
                goto Exit;

            if (hr == S_FALSE) {
                szBuf[0] = '\0';

                // if no dist unit profile, process old INF
                hr = ExtractManifest(FILEXTN_INF , szBuf, pBaseFileName);

                if (FAILED(hr))
                    goto Exit;

                if (hr == S_OK) {
                    hr=pcdl->SetupInf(szBuf, pBaseFileName, this);
                    goto Exit;
                }

            } else {
                // process dist unit profile

                hr=pcdl->ParseOSD(szBuf, pBaseFileName, this);
                goto Exit;
            }

            if (!pcdl->HaveManifest()) { // still don't have an INF?


                if (!HasAllActiveXPermissions()) {

                    if (GetCodeDownload()->IsSilentMode())
                    {
                        GetCodeDownload()->SetBitsInCache();
                    }

                    hr = TRUST_E_FAIL;
                    goto Exit;
                }

                // only valid case at this point is
                // case where we have a CAB file with ONE file in it
                if (psess->cFiles != 1) {
                    hr = E_INVALIDARG;
                    goto Exit;
                }

                pf = psess->pFilesToExtract = psess->pFileList;
                psess->flags &= ~SESSION_FLAG_ENUMERATE; // already enumerated

                if (FAILED((hr=::Extract(psess, GetFileName()))))
                    goto Exit;


                extn = GetExtnAndBaseFileName(pf->pszFilename, &pBaseFileName);

                if (!catDirAndFile(szBuf, MAX_PATH, psess->achLocation,
                                   pf->pszFilename)) {
                    hr = E_UNEXPECTED;
                    goto Exit;
                }

                psess->pFilesToExtract = NULL;

                pSetup = new CSetup(szBuf, pBaseFileName, extn,
                                                pcdl->GetDestDirHint(), &hr);
                if (!pSetup) {
                    hr = E_OUTOFMEMORY;
                    goto Exit;
                } else if (FAILED(hr)) {
                    delete pSetup;
                    goto Exit;
                }

                AddSetupToList(pSetup);

            }

        } else { /* have inf */

            // INF processor would have made Csetup already

            psess->pFilesToExtract = GetFilesToExtract();

            if (!psess->pFilesToExtract) {
                // no files to extract, means there was only a hook
                // in this CAB and no other particular files
                // the general code downloader is looking for
                // so no setup work either.

                Assert(NeedToExtractAllFiles());

                break;
            }

            psess->flags &= ~SESSION_FLAG_ENUMERATE; // already enumerated

            CSetup *pSetupCur = m_pSetuphead;
            Assert(m_pSetuphead);

            // set fully qual names for all of these
            for (; pSetupCur; pSetupCur = pSetupCur->GetNext()) {

                if (!catDirAndFile(szBuf, MAX_PATH, m_psess->achLocation,
                    (LPSTR)pSetupCur->GetSrcFileName())) {

                    hr = E_UNEXPECTED;
                    goto Exit;
                }

                pSetupCur->SetSrcFileName(szBuf);
            }

            if (FAILED((hr=::Extract(psess, GetFileName()))))
                goto Exit;
        }

        break;

    case FILEXTN_INF:

        if(pcdl->HaveManifest()) {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            goto Exit;
        }

        hr = GetFriendlyName(szBuf, &pBaseFileName);

        if (FAILED(hr)) {
            goto Exit;
        }

        // get friendly name for INF from URL
        hr=pcdl->SetupInf(GetFileName(), pBaseFileName, this);

        if (FAILED(hr)) {
            SetDLState(DLSTATE_DONE);
        }

        goto Exit;

    case FILEXTN_OSD:

        if(pcdl->HaveManifest()) {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            goto Exit;
        }

        hr = GetFriendlyName(szBuf, &pBaseFileName);

        if (FAILED(hr)) {
            goto Exit;
        }

        // get friendly name for OSD from URL
        hr=pcdl->ParseOSD(GetFileName(), pBaseFileName, this);
        goto Exit;

    } /* end switch (extn) */

    // done with this CDownload. Mark it ready for setup
    SetDLState(DLSTATE_READY_TO_SETUP);

    Assert(SUCCEEDED(hr));

Exit:

    if ( (FAILED(hr)) || (GetDLState() == DLSTATE_READY_TO_SETUP)) {
        CompleteSignal(hr, S_OK, S_OK, NULL);
    }

    // if success setupInf would have dispatched a msg for
    // INF processing and only when that completes this
    // CDownload is deemed completed/ready_to_setup

    return;
}


// ---------------------------------------------------------------------------
// %%Function: CDownload::SetURLAndExnt(LPCWSTR szURL, FILEXTN extn);
// ---------------------------------------------------------------------------
HRESULT
CDownload::SetURLAndExtn(LPCWSTR szURL, FILEXTN extn)
{
    m_extn = extn;

    if (!szURL)
        return S_OK;

    DWORD len = lstrlenW(szURL); // make private copy
    LPWSTR lpch = new WCHAR [len + 1];

    if (!lpch)
        return E_OUTOFMEMORY;

    StrCpyW(lpch, szURL);

    if (m_url)
        SAFEDELETE(m_url);

    m_url = lpch;

    return S_OK;
}

// ---------------------------------------------------------------------------
// %%Function: CDownload::CheckForNameCollision(LPCSTR szCacheDir);
// for each in list CSetup::CheckForNameCollision
// ---------------------------------------------------------------------------
HRESULT
CDownload::CheckForNameCollision(LPCSTR szCacheDir)
{
    CSetup *pSetupCur = m_pSetuphead;
    HRESULT hr = S_OK;

    for (pSetupCur = m_pSetuphead; pSetupCur; pSetupCur =pSetupCur->GetNext()) {
        if ((hr=pSetupCur->CheckForNameCollision(GetCodeDownload(), szCacheDir)) == S_FALSE)
            break;
    }

    return hr;

}


// ---------------------------------------------------------------------------
// %%Function: CDownload::FindJavaSetup
// ---------------------------------------------------------------------------
CJavaSetup*
CDownload::FindJavaSetup(LPCWSTR szPackageName)
{
    HRESULT hr = S_OK;
    CJavaSetup *pjs = NULL;

    if (!szPackageName)
        return NULL;


    int iNumJavaSetup = m_JavaSetupList.GetCount();
    LISTPOSITION curpos = m_JavaSetupList.GetHeadPosition();
    for (int i=0; i < iNumJavaSetup; i++) {

        pjs = m_JavaSetupList.GetNext(curpos);

        if (pjs->GetPackageName() && (StrCmpIW(szPackageName, pjs->GetPackageName()) == 0)) {
            return pjs;
        }
    }

    return NULL;
}


// ---------------------------------------------------------------------------
// %%Function: CDownload::FindHook
// ---------------------------------------------------------------------------
CSetupHook*
CDownload::FindHook(LPCSTR szHook)
{
    HRESULT hr = S_OK;
    CSetupHook *psh = NULL;

    if (!szHook)
        return NULL;


    int iNumHooks = m_SetupHooks.GetCount();
    LISTPOSITION curpos = m_SetupHooks.GetHeadPosition();
    for (int i=0; i < iNumHooks; i++) {

        psh = m_SetupHooks.GetNext(curpos);

        if (psh->GetHookName() && (lstrcmpi(szHook, psh->GetHookName()) == 0)) {
            return psh;
        }
    }

    return NULL;
}

// ---------------------------------------------------------------------------
// %%Function: CDownload::DoSetup
// ---------------------------------------------------------------------------
HRESULT
CDownload::DoSetup()
{
    CSetup *pSetupCur = m_pSetuphead;
    HRESULT hr = S_OK;
    int nSetupsPerCall = 0;
    int iNumHooks,i;
    POSITION curpos;

    SetDLState(DLSTATE_SETUP);

    // SILENT MODE
    // determine if we're in silent mode
    if (GetCodeDownload()->IsSilentMode() && !GetCodeDownload()->IsAllTrusted())
    {
        SetDLState(DLSTATE_DONE);
        return hr;
    }

    // process all Java Setups
#ifdef _ZEROIMPACT
    // if this is a zero impact install, do not process the java setups or the hooks
    // we have no way of guaranteeing that their behavior is zero-impact
    if(! this->IsZeroImpact())
    {
#endif
        if (m_JavaSetupList.GetCount() != 0) {

            CJavaSetup *pjs = m_JavaSetupList.GetHead();
            curpos = m_JavaSetupList.GetHeadPosition();
            BOOL bInstallReqd = FALSE;

            if (pjs != NULL) {

                for (int i=0; i< m_JavaSetupList.GetCount(); i++) {

                    CJavaSetup *pjs = m_JavaSetupList.GetNext(curpos);
                    Assert(pjs != NULL);

                    if (pjs->GetState() != INSTALL_DONE) {
                        bInstallReqd = TRUE;
                        break;
                    }
                }


                if (bInstallReqd) {
                    Assert(HasJavaPermissions());
                    // the below check is our final security test
                    // we should never need to test this in retail
                    // but, we do anyway
                    if (HasJavaPermissions())
                        hr = pjs->DoSetup();
                    else 
                        hr = TRUST_E_FAIL;

                    if (FAILED(hr))
                        goto Exit;
                    else
                        return S_OK;
                }
            }

        }

        // done processing Java Setups
        // process all hooks
        iNumHooks = m_SetupHooks.GetCount();
        curpos = m_SetupHooks.GetHeadPosition();
        for (i=0; i < iNumHooks; i++) {

            CSetupHook *psh = m_SetupHooks.GetNext(curpos);

            if (psh->GetState() == INSTALL_DONE)
                continue;

            if (nSetupsPerCall++) {
                // here if we have already done 1 hook and there's more to
                // do in this CDownload

                // we don't set DLState to DONE and just return
                return S_OK;
            }

            Assert(HasAllActiveXPermissions());
            // the below check is our final security test
            // we should never need to test this in retail
            // but, we do anyway
            if (HasAllActiveXPermissions())
                hr=psh->Run();
            else 
                hr = TRUST_E_FAIL;

            if (FAILED(hr))
                goto Exit;

            if (psh->GetState() != INSTALL_DONE) {

                // more work left in this setup hook
                // wait for next msg, don't mark ourselves done yet.

                return S_OK;
            }

        }
#ifdef _ZEROIMPACT
    } // if(! this->IsZeroImpact())
#endif

    // processed all Java Setups, hooks, now run setups
    for (pSetupCur = m_pSetuphead; pSetupCur; pSetupCur = pSetupCur->GetNext()) {

        if (pSetupCur->GetState() == INSTALL_DONE)
            continue;

        if (nSetupsPerCall++) {
            // here if we have already done 1 setup and there's more to
            // do in this CDownload

            // we don't set DLState to DONE and just return
            return S_OK;
        }

#ifdef _ZEROIMPACT
        // mark the CSetup as ZeroImpact if this is a zeroimpact CDownload
        if(this->IsZeroImpact())
            pSetupCur->SetZeroImpact(TRUE);
#endif

        if (m_bExactVersion) {
            pSetupCur->SetExactVersion(TRUE);
        }

        if (pSetupCur->GetExtn() == FILEXTN_OSD) {
            hr=pSetupCur->DoSetup(GetCodeDownload(), this);
        } else {

            Assert(HasAllActiveXPermissions());
            // the below check is our final security test
            // we should never need to test this in retail
            // but, we do anyway
            if (HasAllActiveXPermissions())
                hr=pSetupCur->DoSetup(GetCodeDownload(), this);
            else 
                hr = TRUST_E_FAIL;
        }

        if (FAILED(hr))
            break;

        if (pSetupCur->GetState() != INSTALL_DONE) {

            // more work left in this CSetup (pSetupCur)
            // wait for next msg, don't mark ourselves done yet.

            return S_OK;
        }

    } /* for each CSetup */


Exit:
    SetDLState(DLSTATE_DONE);

    return hr;

}

// ---------------------------------------------------------------------------
// %%Function: CDownload::AddJavaSetup
//
//  create and add a new JavaSetup to the list of setup hooks in this cab
// ---------------------------------------------------------------------------
HRESULT
CDownload::AddJavaSetup(
    LPCWSTR szPackageName,
    LPCWSTR szNameSpace,
    IXMLElement *pPackage,
    DWORD dwVersionMS,
    DWORD dwVersionLS,
    DWORD flags)
{
    HRESULT hr = S_OK;
    CJavaSetup *pJavaSetup = NULL;

    if (GetCodeDownload()->IsDuplicateJavaSetup(szPackageName) == S_OK) {

        goto Exit;
    }

    // create a CJavaSetup OBJ and add it to the CDownload obj
    pJavaSetup = new CJavaSetup(this, szPackageName, szNameSpace, pPackage, dwVersionMS, dwVersionLS, flags, &hr);
    if(!pJavaSetup) {
        hr = E_OUTOFMEMORY;
    }
    if (FAILED(hr)) {
        SAFEDELETE(pJavaSetup);
        goto Exit;
    }

    m_JavaSetupList.AddTail(pJavaSetup);

Exit:
    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CDownload::AddHook
//
//  create and add a new hook to the list of setup hooks in this cab
// ---------------------------------------------------------------------------
HRESULT
CDownload::AddHook(
    LPCSTR szHook,
    LPCSTR szInf,
    LPCSTR szInfSection,
    DWORD flags)
{

    HRESULT hr = S_OK;
    CSetupHook *psh;

    Assert(m_state != DLSTATE_EXTRACTING);


    if (GetCodeDownload()->IsDuplicateHook(szHook) == S_OK) {

        goto Exit;
    }


    if (m_extn == FILEXTN_CAB) { // if a CAB

        if (m_state > DLSTATE_DOWNLOADED) {

            // this CAB is ready, extract this code first
            // BUGBUG: multi-threading issue: we are relying on
            // not being re-enterant in our extraction

            Assert(m_psess);

            if (!m_psess) {
                hr = E_UNEXPECTED;
                goto Exit;
            }

            if (!(m_psess->flags & SESSION_FLAG_EXTRACTED_ALL)) {

                m_psess->pFilesToExtract = NULL;
                m_psess->flags &= ~SESSION_FLAG_ENUMERATE; // already enumerated
                m_psess->flags |= SESSION_FLAG_EXTRACT_ALL;

                if (FAILED((hr = ::Extract(m_psess, m_pFileName)))) {
                    goto Exit;
                }

                m_psess->flags |= SESSION_FLAG_EXTRACTED_ALL;
            }


        } else {

            // newly initiated download, mark CDownload as extract all.
            SetNeedToExtractAll();

        }

    }


    psh = new CSetupHook(this, szHook, szInf, szInfSection, flags, &hr);

    if (psh && SUCCEEDED(hr)) {

        m_SetupHooks.AddTail(psh);

    } else {

        if (psh)
            delete psh;

        hr = E_OUTOFMEMORY;
    }

Exit:
    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CDownload::AddSetupToExistingCAB
//        if CAB is already downloaded
//            extract file; create CSetup to install it (piggy back to pdl)
//        else if some other CAB that has been set for download
//                attach file to be extracted to pFilesToExtract
//                attach a CSetup for this file
//        else
// ---------------------------------------------------------------------------
HRESULT
CDownload::AddSetupToExistingCAB(char * lpCode, const char * szDestDir, DESTINATION_DIR dest, DWORD dwRegisterServer, DWORD dwCopyFlags)
{

    char *pBaseFileName =  lpCode;
    FILEXTN extn;
    HRESULT hr = NO_ERROR;
    CSetup* pSetup = NULL;
    char szBuf[MAX_PATH];

    Assert(lpCode);

    if (!lpCode) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (IsDuplicateSetup(lpCode))
        goto Exit;

    // assumes that both CAB extraction and download
    // are into same temp dir
    // make a name for extraction ie: tempdir\curcode

    extn = GetExtnAndBaseFileName( lpCode, &pBaseFileName);

    // this check is totally legit : ie no race condition here
    // we are on the main wininet thread and all onstopbindgs get
    // posted on this thread. So a newly initialted download could not
    // have completed, and even if so CAB extraction could not have started

    Assert(m_state != DLSTATE_EXTRACTING);
    Assert(m_state != DLSTATE_SETUP);
    Assert(m_state != DLSTATE_DONE);

    if (m_state > DLSTATE_DOWNLOADED) {

        // part of CAB that the INF is in,
        // or part of a CAB of some other code download that matches our spec.
        // extract this code first

        Assert(m_psess);

        if (!m_psess) {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        FNAME fname;

        fname.pszFilename = pBaseFileName;
        fname.pNextName = NULL;
        fname.status = SFNAME_INIT;

        m_psess->pFilesToExtract = &fname;
        m_psess->flags &= ~SESSION_FLAG_ENUMERATE; // already enumerated

        if (FAILED((hr = ::Extract(m_psess, m_pFileName)))) {
            goto Exit;
        }

        m_psess->pFilesToExtract = NULL;

    } else {


        // newly initiated download, piggy back to end of extraction list

        PFNAME pf = new FNAME;
        if (!pf) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        pf->pszFilename = new char [lstrlen(pBaseFileName)+1];

        if (!pf->pszFilename) {
            delete pf;
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        lstrcpy(pf->pszFilename, pBaseFileName);
        pf->status = SFNAME_INIT;

        pf->pNextName = m_pFilesToExtract; // add to list
        m_pFilesToExtract = pf;
    }

    if (!catDirAndFile(szBuf, MAX_PATH,
        (m_psess)?m_psess->achLocation:NULL, pBaseFileName)) {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    // create a CSetup OBJ and add it to us
    pSetup = new CSetup(szBuf, pBaseFileName, extn, szDestDir, &hr, dest);
    if (!pSetup) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    } else if (FAILED(hr)) {
        delete pSetup;
        goto Exit;
    }

    AddSetupToList(pSetup);

    pSetup->SetCopyFlags (dwCopyFlags);
    if (dwRegisterServer) {
        pSetup->SetUserOverrideRegisterServer(dwRegisterServer&CST_FLAG_REGISTERSERVER);
    }

Exit:
    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CDownload::IsDuplicateSetup
// ---------------------------------------------------------------------------
BOOL
CDownload::IsDuplicateSetup(LPCSTR pBaseFileName)
{
    CSetup *pSetupCur = m_pSetuphead;

    for (pSetupCur = m_pSetuphead; pSetupCur; pSetupCur=pSetupCur->GetNext()) {

        if (lstrcmpi(pBaseFileName, pSetupCur->GetBaseFileName()) == 0)
            return TRUE;
    }

    return FALSE;
}

// ---------------------------------------------------------------------------
// %%Function: CDownload::AddSetupToList
// ---------------------------------------------------------------------------
VOID
CDownload::AddSetupToList(CSetup *pSetup)
{
    pSetup->SetNext(m_pSetuphead);
    m_pSetuphead = pSetup;

}

// ---------------------------------------------------------------------------
// %%Function: CDownload::RemoveSetupFromList
// ---------------------------------------------------------------------------
HRESULT
CDownload::RemoveSetupFromList(CSetup *pSetup)
{
    CSetup *pSetupCur = m_pSetuphead;
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);

    Assert(pSetup);
    Assert(pSetupCur);             // empty list?

    if (pSetupCur == pSetup) {
        m_pSetuphead = pSetup->GetNext();
        hr = S_OK;
        goto Exit;
    }

    do {
        if (pSetupCur->GetNext() == pSetup) {
            pSetupCur->SetNext(pSetup->GetNext());
            hr = S_OK;
            goto Exit;
        }
    } while ( (pSetupCur = pSetupCur->GetNext()));

Exit:
    return hr;                // not found in list!

}


// ---------------------------------------------------------------------------
// %%Function: CDownload::CleanupFiles
// ---------------------------------------------------------------------------
 HRESULT
CDownload::CleanupFiles()
{

    if (m_psess) { // CAB?

        DeleteExtractedFiles(m_psess);
        RemoveDirectoryAndChildren(m_psess->achLocation);
        SAFEDELETE(m_psess);

    }

    if (!m_pSetuphead) {

        if (m_pFileName) {
            delete (LPSTR)m_pFileName;
            m_pFileName = NULL;
        }

    } else {

        CSetup *pSetupCur = m_pSetuphead;
        CSetup *pSetupNext;

        for (pSetupCur = m_pSetuphead; pSetupCur;
                                    pSetupCur = pSetupNext) {
                pSetupNext = pSetupCur->GetNext();
                SAFEDELETE(pSetupCur);
        }

    }

    if (m_pUnkForCacheFileRelease)
        SAFERELEASE(m_pUnkForCacheFileRelease);

    return S_OK;
}

// ---------------------------------------------------------------------------
// %%Function: CDownload::DoDownload
// CDownload is the basic download obj. It's action entry point is DoDownload
// Here it creates a URL moniker for the given m_url and a bind ctx to go
// with it and then calls pmk->BindToStorage to get the bits. Note how we
// use URL mon's services to get the bits even as URLmon is our client for
// the Code Download. We are its client for individual downloads. CDownload
// has a BSC implementation to track progress and completion. This BSC is
// where the magic of taking us from one state to next occurs.
//
// ---------------------------------------------------------------------------
 HRESULT
CDownload::DoDownload(LPMONIKER *ppmkContext, DWORD grfBINDF,
                      CList<CCodeBaseHold *, CCodeBaseHold *> *pcbhList)
{
    HRESULT        hr =  NOERROR;
    IBindHost     *pBindHost = NULL;

    m_pcbhList = pcbhList;

    m_ppmkContext = ppmkContext;

    m_grfBINDF = grfBINDF;

    pBindHost = GetCodeDownload()->GetClientBinding()->GetIBindHost();

    hr = CreateBindCtx(0, &m_pbc);

    if (FAILED(hr)) {
        goto Exit;
    }

    // register the format enumerator with the bind ctx if one exists

    if (g_pEFmtETC) {
        hr = RegisterFormatEnumerator(m_pbc, g_pEFmtETC, 0);
    }

    if( SUCCEEDED(hr) ) {

        m_pbsc = new CBindStatusCallback(this, grfBINDF);

        if (m_pbsc == NULL)
            hr = E_OUTOFMEMORY;

        if (!pBindHost)
            if (SUCCEEDED(hr))
                hr = RegisterBindStatusCallback(m_pbc, m_pbsc, 0, 0);
    }

    if (FAILED(hr)) {
        goto Exit;
    }



    if (pBindHost) {

        IMoniker *pmk;

        hr = pBindHost->CreateMoniker(m_url, m_pbc, &pmk, 0);

        if (FAILED(hr)) {
            goto Exit;
        }

        if (*ppmkContext == NULL) { // no context moniker yet?

            m_pmk = pmk;
            m_ppmkContext = &pmk;

        } else {

            hr = (*ppmkContext)->ComposeWith(pmk, FALSE, &m_pmk);

            pmk->Release();

        }

    } else {

        hr =  CreateURLMoniker(*ppmkContext, m_url, &m_pmk);


    }


    if( SUCCEEDED(hr) ) {

        // store away the full URL
        SAFEDELETE(m_url);
        hr = m_pmk->GetDisplayName(m_pbc, NULL, &m_url);

        if (FAILED(hr))
            goto Exit;

        // everything succeeded
        if (*ppmkContext == NULL) { // no context moniker yet?

            // make this the context moniker
            *ppmkContext = m_pmk;
        }

        IUnknown *pUnk = NULL;

        if (pBindHost) {
            hr = pBindHost->MonikerBindToStorage(m_pmk, m_pbc, m_pbsc,
                IID_IUnknown, (void **)&pUnk);
        } else {
            hr = m_pmk->BindToStorage(m_pbc, 0, IID_IUnknown, (void**)&pUnk);
        }
        // m_pbc will get the onstopbinding, ondatavailable, and onprogress
        // messages and pass them on to m_pbsc; wait asynchronously


        if (pUnk) {
            pUnk->Release();
        }

    }


Exit:

    if (FAILED(hr) && hr != E_PENDING) {

        // real failure!
        m_hrOSB = hr;
        SetDLState(DLSTATE_DONE);

        if (*ppmkContext == m_pmk)
            *ppmkContext = NULL;

    }
    else {

/*
        // everything succeeded
        if (*ppmkContext == NULL) { // no context moniker yet?

            // make this the context moniker
            *ppmkContext = m_pmk;
        }
*/
        hr = MK_S_ASYNCHRONOUS;
    }

    return hr;
}  // CDownload::DoDownload

// ---------------------------------------------------------------------------
// %%Function: CDownload::PerformVirusScan
//   S_OK : continue with operation
//   S_FALSE : cancel operation.
// ---------------------------------------------------------------------------
HRESULT CDownload::PerformVirusScan(LPSTR szFileName)
{
    HRESULT             hr = S_OK, hrReturn = S_OK;
    ICatInformation *   pci = NULL;             // category manager
    IEnumCLSID *        peclsid = NULL;         // enum of av objects
    IOfficeAntiVirus *  poav = NULL;            // current av interface
    CLSID               clsidCurrent;           // current av clsid
    ULONG               pcFetched;
    MSOAVINFO           msavi;                  // antivirus struct
    BOOL                fInitStruct = FALSE;

    //
    // Get COM category manager and get an enumerator for our virus
    // scanner category
    //
    // If something goes wrong finding AV objects, proceed as normal.
    //

    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL,
                CLSCTX_INPROC_SERVER, IID_ICatInformation, (void **)&pci);
    if(FAILED(hr))
    {
        return NOERROR;
    }

    hr = pci->EnumClassesOfCategories(1, (GUID *)&CATID_MSOfficeAntiVirus, 0, NULL, &peclsid);
    pci->Release();
    if(FAILED(hr))
    {
        return NOERROR;
    }

    //
    // Call all scanners.  If any fail, return E_FAIL.
    //

    hr = peclsid->Next(1, &clsidCurrent, &pcFetched);
    while(SUCCEEDED(hr) && pcFetched > 0)
    {
        if(FALSE == fInitStruct)
        {
            if (FAILED(Ansi2Unicode(szFileName,&msavi.u.pwzFullPath)))
            {
                break;
            }
            msavi.cbsize = sizeof(msavi);
            msavi.fPath = TRUE;
            msavi.fHttpDownload = TRUE;
            msavi.fReadOnlyRequest = FALSE;
            msavi.fInstalled = FALSE;
            msavi.hwnd = GetCodeDownload()->GetClientBinding()->GetHWND();
            msavi.pwzOrigURL = (LPWSTR)GetURL();

            // per office spec, this is only meant as a method for the scanner
            // to differentiate the caller.  Not localized.
            msavi.pwzHostName = L"Urlmon";

            fInitStruct = TRUE;
        }

        // have clsid of av component
        hr = CoCreateInstance(clsidCurrent, NULL, CLSCTX_INPROC_SERVER,
                IID_IOfficeAntiVirus, (void **)&poav);
        if(SUCCEEDED(hr))
        {
            // call scan method
            hr = poav->Scan(&msavi);
            poav->Release();

            if(hr == E_FAIL)
            {
                // file could not be cleaned
                hrReturn = E_FAIL;
            }
        }

        hr = peclsid->Next(1, &clsidCurrent, &pcFetched);
    }

    //
    // clean up
    //
    peclsid->Release();

    if(fInitStruct)
    {
        SAFEDELETE(msavi.u.pwzFullPath);
    }

    return hrReturn;
}

// ---------------------------------------------------------------------------
// %%Function: CDownload::DownloadRedundantCodeBase()
//    Returns S_OK if starting next download, or S_FALSE if no redundant
//    codebases remaining to try.
// ---------------------------------------------------------------------------

STDMETHODIMP CDownload::DownloadRedundantCodeBase()
{
    HRESULT                      hr = S_FALSE;
    LISTPOSITION                 lpos = 0;
    CCodeBaseHold               *pcbh = NULL;

    if (m_pcbhList == NULL) {
        goto Exit;
    }

    lpos = m_pcbhList->GetHeadPosition();
    while (lpos) {
        pcbh = m_pcbhList->GetNext(lpos);
        if (!(pcbh->dwFlags & CBH_FLAGS_DOWNLOADED)) {
            RevokeBindStatusCallback(GetBindCtx(), GetBSC());
            CleanUp();
            m_url = new WCHAR[lstrlenW(pcbh->wszCodeBase) + 1];
            if (m_url == NULL) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            StrCpyW(m_url, pcbh->wszCodeBase);
            SetResponseHeaderStatus(S_OK);
            pcbh->dwFlags |= CBH_FLAGS_DOWNLOADED;

            // Try another download
            hr = DoDownload(m_ppmkContext, m_grfBINDF, m_pcbhList);
            break;
        }
    }

Exit:
    if (hr == S_FALSE) {
        GetCodeDownload()->CodeDownloadDebugOut(DEB_CODEDL, FALSE, ID_CDLDBG_DL_REDUNDANT_FAILED);
    }
    else {
        LPSTR szUrl = NULL;
        Unicode2Ansi(m_url, &szUrl);
        GetCodeDownload()->CodeDownloadDebugOut(DEB_CODEDL, FALSE, ID_CDLDBG_DL_REDUNDANT, (szUrl == NULL) ? ("") : (szUrl), hr);
        delete szUrl;
    }



    return hr;
}

void CDownload::CleanUp()
{
    LISTPOSITION               pos = 0;
    int                        i, iNum;

    SAFERELEASE(m_pmk);
    SAFERELEASE(m_pbc);
    SAFERELEASE(m_pbsc);
    SAFEDELETE(m_url);

    if (m_pFilesToExtract) {
        PFNAME pf = m_pFilesToExtract;
        PFNAME pfnext;

        for (;pf != NULL; pf = pfnext) {
            delete pf->pszFilename;
            pfnext = pf->pNextName;
            delete pf;
        }
    }
    m_pFilesToExtract = NULL;

    if (m_hPostData)
        GlobalFree(m_hPostData);

    SAFERELEASE(m_pUnkForCacheFileRelease);

    SAFEDELETE(m_pbJavaTrust);
}

HRESULT CDownload::SetMainCABJavaTrustPermissions(PJAVA_TRUST pbJavaTrust)
{
    return GetCodeDownload()->SetMainCABJavaTrustPermissions(pbJavaTrust);
}

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::CBindStatusCallback
//    The BSC implementation for CDownload to track progress of indiv dwlds
// ---------------------------------------------------------------------------
CBindStatusCallback::CBindStatusCallback(CDownload *pdl, DWORD grfBINDF)
{
    DllAddRef();
    m_pbinding = NULL;
    m_cRef = 1; // equ of internal addref
    m_pdl = pdl;

    m_grfBINDF = grfBINDF;
}  // CBindStatusCallback

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::~CBindStatusCallback
// ---------------------------------------------------------------------------
CBindStatusCallback::~CBindStatusCallback()
{
    SAFERELEASE(m_pbinding);
    DllRelease();
}  // ~CBindStatusCallback

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CBindStatusCallback::AddRef()
{
    return m_cRef++;
}

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CBindStatusCallback::Release()
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::QueryInterface
// ---------------------------------------------------------------------------
 STDMETHODIMP
CBindStatusCallback::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if (riid==IID_IUnknown || riid==IID_IBindStatusCallback)
        *ppv = (IBindStatusCallback *)this;

    if (riid==IID_IHttpNegotiate)
        *ppv = (IHttpNegotiate *)this;

    if (riid==IID_IWindowForBindingUI)
        *ppv = (IWindowForBindingUI*)this;

    if (riid==IID_IServiceProvider)
        *ppv = (IServiceProvider *)this;

    if (riid==IID_ICatalogFileInfo)
        *ppv = (ICatalogFileInfo *)this;

    if (*ppv == NULL)
        return E_NOINTERFACE;

    AddRef();

    return S_OK;

}  // CBindStatusCallback::QueryInterface

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::GetWindow
// ---------------------------------------------------------------------------
 STDMETHODIMP
CBindStatusCallback::GetWindow(REFGUID rguidreason, HWND *phWnd)
{
    HRESULT hr = S_OK;
    CCodeDownload *pcdl = m_pdl->GetCodeDownload();
    HWND hWnd = pcdl->GetClientBinding()->GetHWND(rguidreason);

    if (hWnd == INVALID_HANDLE_VALUE)
        hr = S_FALSE;

    *phWnd = hWnd;

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::QueryService
// ---------------------------------------------------------------------------
 STDMETHODIMP
CBindStatusCallback::QueryService(REFGUID guidService, REFIID riid, LPVOID *ppv)
{
    IBindStatusCallback *pbsc = m_pdl->GetCodeDownload()->GetClientBSC();
    IServiceProvider *psp = NULL;
    HRESULT hr = E_NOINTERFACE;

    ASSERT(pbsc);

    if (pbsc && SUCCEEDED(pbsc->QueryInterface(IID_IServiceProvider, (void **)&psp)) && psp) {
        hr = psp->QueryService(guidService, riid, ppv);
        SAFERELEASE(psp);
    }
    
    // Since this is QueryService we can QI on our client's BSC object too.
    if (FAILED(hr)) {
    
        // This is special case we handle so we can bind to client's ultimate IBindHost
        // if one exists.  BUG BUG: Support other interfaces here, in general?
        // BUG BUG: Rearrange order of comparisons for performance.

        if (IsEqualGUID(guidService, riid) &&
            (IsEqualGUID(riid, IID_IBindHost) ||
             IsEqualGUID(riid, IID_IWindowForBindingUI) ||
             IsEqualGUID(riid, IID_ICodeInstall) ||
             IsEqualGUID(riid, IID_ICatalogFileInfo) ||
             IsEqualGUID(riid, IID_IInternetHostSecurityManager))) {

            hr = pbsc->QueryInterface(riid, (void **)ppv);
            
        }

    }

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::GetBindInfo
// ---------------------------------------------------------------------------
 STDMETHODIMP
CBindStatusCallback::GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindInfo)
{
    if ((pgrfBINDF == NULL) || (pbindInfo == NULL) || (pbindInfo->cbSize == 0))
        return E_INVALIDARG;

    *pgrfBINDF = m_grfBINDF;

    // clear BINDINFO but keep its size
    DWORD cbSize = pbindInfo->cbSize;
    ZeroMemory( pbindInfo, cbSize );
    pbindInfo->cbSize = cbSize;

    // use IE5's utf-8 policy
    pbindInfo->dwOptions |= BINDINFO_OPTIONS_USE_IE_ENCODING;


    if (m_pdl->DoPost()) {

        pbindInfo->dwBindVerb = BINDVERB_POST;

        pbindInfo->stgmedData.tymed = TYMED_HGLOBAL;
        pbindInfo->stgmedData.hGlobal = m_pdl->GetPostData(&(pbindInfo->cbstgmedData));
        pbindInfo->stgmedData.pUnkForRelease = (IUnknown *) (IBindStatusCallback *) this;
        AddRef();  // AddRef ourselves so we stick around; caller must release!

    }

    DWORD grfBINDF = 0;
    BINDINFO bindInfo;
    memset(&bindInfo, 0, sizeof(BINDINFO));
    bindInfo.cbSize = sizeof(BINDINFO);

    CCodeDownload *pcdl = m_pdl->GetCodeDownload();
    pcdl->GetClientBSC()->GetBindInfo(&grfBINDF, &bindInfo);

    if (grfBINDF & BINDF_SILENTOPERATION)
    {
        *pgrfBINDF |= BINDF_SILENTOPERATION;
        pcdl->SetSilentMode();
    }

    if (grfBINDF & BINDF_OFFLINEOPERATION)
        *pgrfBINDF |= BINDF_OFFLINEOPERATION;

    if (grfBINDF & BINDF_GETNEWESTVERSION)
        *pgrfBINDF |= BINDF_GETNEWESTVERSION;

    if (grfBINDF & BINDF_RESYNCHRONIZE)
        *pgrfBINDF |= BINDF_RESYNCHRONIZE;

    // To make sure the file winds up on disk even for SSL connections, we need to add
    *pgrfBINDF |= BINDF_NEEDFILE;

    //  BINDINFO_FIX(LaszloG 8/15/97)
    ReleaseBindInfo(&bindInfo);

    return S_OK;
}  // CBindStatusCallback::GetBindInfo

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::OnStartBinding
// ---------------------------------------------------------------------------
 STDMETHODIMP
CBindStatusCallback::OnStartBinding(DWORD grfBSCOPTION,IBinding* pbinding)
{
    CCodeDownload *pcdl = m_pdl->GetCodeDownload();

    Assert(pbinding);

    if (m_pbinding != NULL)
        SAFERELEASE(m_pbinding);
    m_pbinding = pbinding;
    if (m_pbinding != NULL)
        m_pbinding->AddRef();

    m_pdl->SetDLState(DLSTATE_BINDING);

    // call the client BSC::OnStartBinding if not already done

    CClBinding *pClientBinding = pcdl->GetClientBinding();

    if(pClientBinding->GetState() == CDL_NoOperation){

        Assert(pClientBinding->GetAssBSC() == pcdl->GetClientBSC());

        pClientBinding->SetState(CDL_Downloading);
        pcdl->AddRef();
        pcdl->GetClientBSC()->OnStartBinding(grfBSCOPTION, pClientBinding);
    }

    return S_OK;
}  // CBindStatusCallback::OnStartBinding

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::GetPriority
// ---------------------------------------------------------------------------
 STDMETHODIMP
CBindStatusCallback::GetPriority(LONG* pnPriority)
{
    return E_NOTIMPL;
}  // CBindStatusCallback::GetPriority

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::OnProgress
// Here we get the master CodeDownload obj to collate progress and report
// cumulative code download progress to client BSC::OnProgress.
// ---------------------------------------------------------------------------
 STDMETHODIMP
CBindStatusCallback::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    IBindStatusCallback *pClientBSC = m_pdl->GetCodeDownload()->GetClientBSC();
    char szURL[INTERNET_MAX_URL_LENGTH];
    FILEXTN extn;
    char *pBaseFileName;
    HRESULT hr = S_OK;
    IMoniker *pmk = NULL;
    CCodeDownload *pcdl = m_pdl->GetCodeDownload();

    // if this a redirect set the context appropriately
    // also use this URL to get the extension and base dest name for this
    // component, if its a POST (Search Path)

    if (m_pdl->DoPost() && (ulStatusCode == BINDSTATUS_REDIRECTING)) {

        WideCharToMultiByte(CP_ACP, 0, szStatusText, -1, szURL,
                    INTERNET_MAX_URL_LENGTH, 0,0);


        // BUGBUG: use mime type in response header to determine extn
        extn = GetExtnAndBaseFileName( szURL, &pBaseFileName);

        hr = m_pdl->SetURLAndExtn( szStatusText, extn);

        if (SUCCEEDED(hr)) {

            IBindHost *pBH = pcdl->GetClientBinding()->GetIBindHost();
            if (pBH) {
                hr = pBH->CreateMoniker((LPOLESTR)szStatusText, m_pdl->GetBindCtx(), &pmk, 0);
            } else {
                hr =  CreateURLMoniker(NULL, szStatusText, &pmk);
            }


            if (SUCCEEDED(hr)) {
                pcdl->SetContextMoniker(pmk);
                pcdl->MarkNewContextMoniker();
            }
        }

        if (FAILED(hr))
            m_pdl->SetResponseHeaderStatus( hr );
    }

    // we are only interested in cumulative numbers for "downloading" status
    // for all others progress is usually: "connecting: 0 of 0", so we
    // pass these as is to our client

    if ((ulStatusCode != BINDSTATUS_DOWNLOADINGDATA ) &&
        (ulStatusCode != BINDSTATUS_ENDDOWNLOADDATA )) {

        // pass on progress as is to our client
        pClientBSC->OnProgress(ulProgress, ulProgressMax, ulStatusCode,
                                                                szStatusText);
        return S_OK;
    }

    // here if Downloading Data progress
    m_pdl->SetProgress(ulProgress, ulProgressMax); // update my dl-object's prog

    // now summate stats and report to client
    CDownload *pdl = m_pdl->GetCodeDownload()->GetDownloadHead();
    ULONG ulSum = 0;
    ULONG ulSumMax = 0;

    // walk each dl object and make a sum of all ulProgress and ulProgressMax
    do {
        pdl->SumProgress(&ulSum, &ulSumMax);
    } while ((pdl = pdl->GetNext()) != NULL);


    // pass on cumulative downloading progress to our client
    pClientBSC->OnProgress(ulSum, ulSumMax, BINDSTATUS_DOWNLOADINGDATA,
                    m_pdl->GetCodeDownload()->GetMainURL());

    if (ulStatusCode == BINDSTATUS_ENDDOWNLOADDATA  ) {
        // pass on progress as is to our client
        pClientBSC->OnProgress(ulProgress, ulProgressMax, ulStatusCode,
                                                                szStatusText);
    }

    return(NOERROR);
}  // CBindStatusCallback


// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::OnDataAvailable
// At the last notification we get the filename URLmon has downloaded the
// m_url data to and rename it to a file in the temp dir.
// ---------------------------------------------------------------------------
 STDMETHODIMP
CBindStatusCallback::OnDataAvailable(DWORD grfBSC, DWORD dwSize, FORMATETC *pFmtetc, STGMEDIUM  __RPC_FAR *pstgmed)
{
    HRESULT hr = NO_ERROR;

    // never forward OnDataAvailable to code download's client BSC

    if (grfBSC & BSCF_LASTDATANOTIFICATION)
    {
        // if this is the final notification then get the data and display it

        // we asked for IUnknown, we should get back a filename
        Assert((pFmtetc->tymed & TYMED_FILE));

        if (pFmtetc->tymed & TYMED_FILE) {

            char szFile[MAX_PATH];
            DWORD dwLen = 0;

            if (!(dwLen = WideCharToMultiByte(CP_ACP, 0 , pstgmed->lpszFileName , -1 , szFile, MAX_PATH, NULL, NULL))) {

                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;

            } else {

                LPSTR lpFileName = new char[dwLen + 1];

                if (!lpFileName) {
                    hr = E_OUTOFMEMORY;
                    goto Exit;
                } else {

                    lstrcpy(lpFileName, szFile);
                    m_pdl->SetFileName(lpFileName);
                }
            }

            // check last modified date for file: URLs
            // maybe we don't need the file
            HRESULT hr1 = m_pdl->IsDownloadedVersionRequired();

            if (FAILED(hr1)) {
                m_pdl->SetResponseHeaderStatus(hr1);
                goto Exit;
            }


            // ref count on the cache
            // file.

            pstgmed->pUnkForRelease->AddRef();

            m_pdl->SetUnkForCacheFileRelease(pstgmed->pUnkForRelease);

        }


    }


Exit:
    return hr;

}  // CBindStatusCallback::OnDataAvailable

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::OnObjectAvailable
// ---------------------------------------------------------------------------
 STDMETHODIMP
CBindStatusCallback::OnObjectAvailable( REFIID riid, IUnknown* punk)
{
    // Not applicable: we call pmk->BTS not BTO
    return E_NOTIMPL;
}  // CBindStatusCallback::OnObjectAvailable

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::OnLowResource
// ---------------------------------------------------------------------------
 STDMETHODIMP
CBindStatusCallback::OnLowResource(DWORD dwReserved)
{
    return E_NOTIMPL;
}  // CBindStatusCallback::OnLoadResource

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::OnStopBinding
//
// we get here when we have fully downloaded 'this'.
// ---------------------------------------------------------------------------
 STDMETHODIMP
CBindStatusCallback::OnStopBinding(HRESULT hrStatus, LPCWSTR szError)
{
    CCodeDownload *pcdl = m_pdl->GetCodeDownload();
    HRESULT hrResponseHdr = m_pdl->GetResponseHeaderStatus();
    IBindHost *pBindHost = NULL;
    HRESULT hr = S_OK; // assume all OK

    if (pcdl) {
        pcdl->CodeDownloadDebugOut(DEB_CODEDL, FALSE, ID_CDLDBG_DL_ON_STOP_BINDING, hrStatus, hrResponseHdr);
    }
    if ((FAILED(hrStatus) && (SCODE_FACILITY(hrStatus) == FACILITY_INTERNET)) ||
        FAILED(hrResponseHdr) || SCODE_CODE(hrStatus) == ERROR_MOD_NOT_FOUND) {
        hr = m_pdl->DownloadRedundantCodeBase();
        if (hr == E_PENDING || hr == MK_S_ASYNCHRONOUS) {
            goto Exit;
        }
    }

    m_pdl->SetDLState(DLSTATE_DOWNLOADED);

    pBindHost = pcdl->GetClientBinding()->GetIBindHost();

    SAFERELEASE(m_pbinding);

    if (!pBindHost) {
        hr = RevokeBindStatusCallback(m_pdl->GetBindCtx(), m_pdl->GetBSC());

    }

    if (FAILED(hr)) {
        goto OSB_Complete;
    }

    // if URLMON failed the download or if the response hdr indicated
    // a failure that URLMON failed to detect properly
    // pass the problem to pcdl->CompleteOne(). This will determine if it
    // will query for the clsid with more urls in the CodeSearchPath
    // in the registry.
    if (FAILED(hrStatus) || FAILED(hrResponseHdr)) {
        goto OSB_Complete;
    }

    // BUGBUG: also check here for Last Modified Date on the Cache Entry
    // versus Last Modified if a previous version exists and we are doung 
    // GetLatest. If data is in the cache then wininet ignores our
    // if-modified-since and so we will end up re-installing even though
    // there is no version change.

    if (m_pdl->GetFileName() != NULL) { // should be set by OnDataAvailable

        // This takes us to the next state. VerifyTrust moves us when
        // complete to the next state of processing the ProcessPiece.

        CCDLPacket *pPkt= new CCDLPacket(CODE_DOWNLOAD_TRUST_PIECE, m_pdl, 0);

        if (pPkt) {
            hr = pPkt->Post();
        } else {
            hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
            goto Exit;
        // else fall thru to OSB_Complete

    } else if (!m_pdl->UsingCdlProtocol()) {

        // In case of CDL protocol handler we don't need OnDataAvailable or 
        // Trust Verification done here.

        // BindToStorage may have been a bozo and not detected the error
        if (m_pdl->DoPost())
            hrResponseHdr = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
        else
            hr = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
    }

OSB_Complete:

    // does all the master state analysis
    m_pdl->CompleteSignal(hr, hrStatus, hrResponseHdr, szError);

    // This very BSC may already have been deleted if all done.
    // Don't access any members. Just return !!!

Exit:
    return S_OK; // always succeed to url mon.

}  // CBindStatusCallback::OnStopBinding


// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::BeginningTransaction
// ---------------------------------------------------------------------------
STDMETHODIMP
CBindStatusCallback::BeginningTransaction(
    LPCWSTR szURL,
    LPCWSTR szHeaders,
    DWORD dwReserved,
    LPWSTR *pszAdditionalHeaders)
{
    HRESULT hr = S_OK;
    char szHttpDate[INTERNET_RFC1123_BUFSIZE+1];
    DWORD dwLen = 0;
    LPWSTR szAHdrs = NULL;
    static const char cszHeaderFmt[] = "%s %s\r\n";
    static const char szIfMod[] = "If-Modified-Since:";
    static const char szNONEMATCH[] = "If-None-Match:";
    static const WCHAR szFORM[] = L"Content-Type: application/x-www-form-urlencoded\r\n";
    static const char szAcceptLanguageFmt[] = "Accept-Language: %s\r\n";
    char szBuf[MAX_PATH];
    WCHAR szAcceptLanguage[MAX_PATH];
    char szLangBuf[10];
    char *pszNoneMatch = NULL;

    CCodeDownload *pcdl = m_pdl->GetCodeDownload();
    LCID lcid = pcdl->GetLCID();

    // BUGBUG: we currently only support primary lang or default
    // it should really be "en-us, en", instead of just "en"
    // waiting for note from TonyCi about some servers like Apache
    // broken by this
    lcid = MAKELCID(MAKELANGID(PRIMARYLANGID(LANGIDFROMLCID(lcid)), SUBLANG_DEFAULT), SORT_DEFAULT);

    pcdl->GetLangInfo()->GetAcceptLanguageString(lcid, szLangBuf);
    wsprintf(szBuf, szAcceptLanguageFmt, szLangBuf);
    dwLen = MultiByteToWideChar(CP_ACP, 0, szBuf, -1, szAcceptLanguage, MAX_PATH);

    Assert((pszAdditionalHeaders != NULL));

    FILETIME *pftLastMod = pcdl->GetLastModifiedTime();
    SYSTEMTIME  sSysTime;

    BOOL bSendNoneMatch = !pcdl->ForceDownload() && ( pcdl->LocalVersionPresent() && (pcdl->GetLocalVersionEtag()) ) && pcdl->NeedLatestVersion();

    BOOL bSendLastMod = !bSendNoneMatch && (!pcdl->ForceDownload() && ( pcdl->LocalVersionPresent() && (pftLastMod) ) && pcdl->NeedLatestVersion());


    if ( bSendLastMod) {
        Assert( (pftLastMod != NULL) ); // Check for bug#40696

        // need to send If-Modified-Since

        if (!FileTimeToSystemTime(pftLastMod, &sSysTime)) {
            m_pdl->SetResponseHeaderStatus( HRESULT_FROM_WIN32(GetLastError()));
            goto Exit;
        }

        if (!InternetTimeFromSystemTime(&sSysTime, INTERNET_RFC1123_FORMAT,
            szHttpDate, INTERNET_RFC1123_BUFSIZE)) {

            m_pdl->SetResponseHeaderStatus( HRESULT_FROM_WIN32(GetLastError()));
            goto Exit;
        }


       dwLen += (INTERNET_RFC1123_BUFSIZE + 1 + sizeof(szIfMod) +
            sizeof(cszHeaderFmt));

    }

    if (bSendNoneMatch) {

        DWORD dwNoneMatch = lstrlen(pcdl->GetLocalVersionEtag()) + sizeof(szNONEMATCH) + sizeof(cszHeaderFmt);
        pszNoneMatch = new char [dwNoneMatch+1];
        wsprintf(pszNoneMatch, cszHeaderFmt, szNONEMATCH, pcdl->GetLocalVersionEtag());

        dwLen += dwNoneMatch;
    }

    if (m_pdl->DoPost()) {
        dwLen += sizeof(szFORM);
    }

    if (dwLen) {

        szAHdrs = new WCHAR [dwLen + 1];

        if (!szAHdrs) {
            m_pdl->SetResponseHeaderStatus( E_OUTOFMEMORY );

            // BUGBUG: Clean all this up to never return right away, and
            // goto exit to cleanup
            SAFEDELETE(pszNoneMatch);
            return hr;
        }

        szAHdrs[0] = '\0';
    }

    if (bSendLastMod) {
        char *szTemp = new char [dwLen + 1];

        if (!szTemp) {
            hr = E_OUTOFMEMORY;
            delete szAHdrs;
            goto Exit;
        }

        wsprintf(szTemp, cszHeaderFmt, szIfMod, szHttpDate);
        MultiByteToWideChar(CP_ACP, 0, szTemp, -1, szAHdrs, dwLen);

        delete szTemp;

    }

    if (bSendNoneMatch) {
        MultiByteToWideChar(CP_ACP, 0, pszNoneMatch, -1, szAHdrs, dwLen);
    }

    if (m_pdl->DoPost()) {
        StrCatW(szAHdrs, szFORM);
    }

    StrCatW(szAHdrs, szAcceptLanguage);


Exit:

    SAFEDELETE(pszNoneMatch);

    *pszAdditionalHeaders = szAHdrs;

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::OnResponse
// ---------------------------------------------------------------------------
STDMETHODIMP
CBindStatusCallback::OnResponse(
    DWORD dwResponseCode,
    LPCWSTR szResponseHeaders,
    LPCWSTR szRequestHeaders,
    LPWSTR *pszAdditionalRequestHeaders)
{
    HRESULT hr = S_OK;

    // propogate errors here to CSBC::OnStopBinding
    // we need this as urlmon might just convert any error returned here
    // as user_cancelled
    if (dwResponseCode != HTTP_STATUS_OK) {
        if (dwResponseCode == HTTP_STATUS_NOT_MODIFIED) {
            hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        } else {
            hr = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
        }
        m_pdl->SetResponseHeaderStatus( hr );
    }

    if (m_pdl->DoPost() || 
        (m_pdl->GetMoniker() == m_pdl->GetCodeDownload()->GetContextMoniker())){

        // Get the HttpQueryInfo wrapper object.
        IWinInetHttpInfo *pHttpInfo = NULL;
        HRESULT hr = GetBinding()->QueryInterface
            (IID_IWinInetHttpInfo, (void **) &pHttpInfo);


        if (SUCCEEDED(hr)) {
            DWORD cbLen = INTERNET_RFC1123_BUFSIZE + 1;
            char szHttpDate[INTERNET_RFC1123_BUFSIZE+1];


            if ((pHttpInfo->QueryInfo (HTTP_QUERY_LAST_MODIFIED,
                (LPVOID)szHttpDate, &cbLen, NULL, 0) == S_OK) && cbLen)
                 m_pdl->GetCodeDownload()->SetLastModifiedTime(szHttpDate);

            cbLen = 0; // reset

            if ( (pHttpInfo->QueryInfo (HTTP_QUERY_ETAG,
                (LPVOID)NULL, &cbLen, NULL, 0) == S_OK) && cbLen) {

                char *pbEtag = new char [cbLen  +1];

                *pbEtag = '\0'; // clr

                pHttpInfo->QueryInfo (HTTP_QUERY_ETAG,
                    (LPVOID)pbEtag, &cbLen, NULL, 0);

                if (*pbEtag)
                    m_pdl->GetCodeDownload()->SetEtag(pbEtag);
            }
        
            pHttpInfo->Release();
        }
    }

    return S_OK;
}

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::GetCatalogFile
// ---------------------------------------------------------------------------
STDMETHODIMP CBindStatusCallback::GetCatalogFile(LPSTR *ppszCatalogFile)
{
    HRESULT                        hr = S_OK;
    LPSTR                          pszCatFile = NULL;

    if (ppszCatalogFile) {
        pszCatFile = m_pdl->GetCodeDownload()->GetCatalogFile();
        if (pszCatFile) {
            *ppszCatalogFile = new char[lstrlen(pszCatFile) + 1];
            if (*ppszCatalogFile == NULL) {
                hr = E_OUTOFMEMORY;
            }
            else {
                lstrcpy(*ppszCatalogFile, pszCatFile);
            }
        }
        else {
            *ppszCatalogFile = NULL;
        }
    }
    else {
        hr = E_INVALIDARG;
    }

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::GetJavaTrust
// ---------------------------------------------------------------------------
STDMETHODIMP CBindStatusCallback::GetJavaTrust(void **ppJavaTrust)
{
    HRESULT                   hr = S_OK;

    if (ppJavaTrust) {
        *ppJavaTrust = (void *)m_pdl->GetCodeDownload()->GetJavaTrust();
    }
    else {
        hr = E_INVALIDARG;
    }

    return hr;
}
   
