// ===========================================================================
// File: JSETUP.CXX
//    implements CJavaSetup
//


#include <cdlpch.h>
#include <pkgguid.h>

// ---------------------------------------------------------------------------
// %%Function: CJavaSetup::CJavaSetup
// ---------------------------------------------------------------------------
CJavaSetup::CJavaSetup(
    CDownload *pdl,
    LPCWSTR szPackageName,
    LPCWSTR szNameSpace,
    IXMLElement *pPackage,
    DWORD dwVersionMS,
    DWORD dwVersionLS,
    DWORD flags,
    HRESULT *phr)
    :
    m_pdl(pdl),
    m_dwVersionMS(dwVersionMS),
    m_dwVersionLS(dwVersionLS),
    m_flags(flags),
    m_state(INSTALL_INIT)
{
    *phr = S_OK;

    if (szPackageName) {

        DWORD len = lstrlenW(szPackageName) +1;

        m_szPackageName = new WCHAR [len]; // make private copy

        if (m_szPackageName)
            StrCpyW(m_szPackageName, szPackageName);
        else
            *phr = E_OUTOFMEMORY;

    } else {
        m_szPackageName = NULL;
    }

    if (szNameSpace) {

        DWORD len = lstrlenW(szNameSpace) +1;

        m_szNameSpace = new WCHAR [len]; // make private copy

        if (m_szNameSpace)
            StrCpyW(m_szNameSpace, szNameSpace);
        else
            *phr = E_OUTOFMEMORY;

    } else {
        m_szNameSpace = NULL;
    }

    m_pPackage = pPackage;
    if (pPackage) {
        pPackage->AddRef(); 
    }

}  // CJavaSetup

// ---------------------------------------------------------------------------
// %%Function: CJavaSetup::~CJavaSetup
// ---------------------------------------------------------------------------
CJavaSetup::~CJavaSetup()
{
    SAFEDELETE(m_szPackageName);
    SAFEDELETE(m_szNameSpace);
    SAFERELEASE(m_pPackage);

}  // ~CJavaSetup

// ---------------------------------------------------------------------------
// %%Function: CJavaSetup::DoSetup
// ---------------------------------------------------------------------------
HRESULT
CJavaSetup::DoSetup()
{
    Assert(m_pdl);
    CCodeDownload *pcdl = m_pdl->GetCodeDownload();
    Assert(pcdl);
    HWND hWnd = pcdl->GetClientBinding()->GetHWND();
    HRESULT hr = S_OK;
    CList<CJavaSetup*,CJavaSetup*> *pjsList = m_pdl->GetJavaSetupList();
    PACKAGEINSTALLINFO *PackageInfo = NULL;
    LPCWSTR pwszNameSpace = NULL;
    int i;

    if (SUCCEEDED(hr)) {

        // call the pkg mgr to install the package.

        int nCntjs = pjsList->GetCount(), njs = 0;
        POSITION pos = pjsList->GetHeadPosition();
        BOOL bInstallReqd = FALSE;

        // find the first setup that needs to be installed and then record 
        // its namespace. We will then instal all packages in the CDownload
        // that macth that namespace and then return. On each call we will
        // install all pkgs in a namespace till all setups are done.
        for (i=0; i<nCntjs; i++) {

            CJavaSetup *pjs = pjsList->GetNext(pos);
            Assert(pjs != NULL);

            if (pjs->GetState() != INSTALL_DONE) {
                pwszNameSpace = pjs->GetNameSpace();
                bInstallReqd = TRUE;
                break;
            }
        }

        if (!bInstallReqd)
            return hr;


        PackageInfo = new PACKAGEINSTALLINFO[nCntjs];
        if (PackageInfo == NULL) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        PACKAGESECURITYINFO PkgSecInfo;
        ZeroMemory(PackageInfo,sizeof(PACKAGEINSTALLINFO)*nCntjs);
        ZeroMemory(&PkgSecInfo,sizeof(PkgSecInfo));

        pos = pjsList->GetHeadPosition();
        for (i=0; i<nCntjs; i++) {

            CJavaSetup *pjs = pjsList->GetNext(pos);
            Assert(pjs != NULL);

            // when installing multiple packages, if some are installed they should be skipped.
            if (pjs->GetPackageFlags() & CJS_FLAG_NOSETUP) {
                pjs->SetState(INSTALL_DONE);
                continue;
            }

            if  ((pjs->GetState() != INSTALL_DONE) &&
                 ((pjs->GetNameSpace() == pwszNameSpace) || 
                  ( pwszNameSpace && pjs->GetNameSpace() && 
                    (StrCmpIW(pjs->GetNameSpace(), pwszNameSpace) == 0)))) {

                pcdl->CodeDownloadDebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_JAVA_PKG_SETUP, pjs->GetPackageName(), (pwszNameSpace)?pwszNameSpace:L"Global");

                PACKAGEINSTALLINFO *CurPackageInfo = &PackageInfo[njs];

                CurPackageInfo->cbStruct = sizeof(PACKAGEINSTALLINFO);
                CurPackageInfo->pszPackageName = pjs->GetPackageName();
                pjs->GetPackageVersion(CurPackageInfo->dwVersionMS, CurPackageInfo->dwVersionLS);
    
                CurPackageInfo->pszDistributionUnit = pcdl->GetMainDistUnit();   // Should be same dist unit

                CurPackageInfo->pUnknown = (IUnknown *)(pjs->GetPackageXMLElement());

                if (pjs->GetPackageFlags() & CJS_FLAG_SYSTEM) {
                    CurPackageInfo->dwFlags |= JPMPII_SYSTEMCLASS;          // Already true by zeroing
                } else {
                    CurPackageInfo->dwFlags |= JPMPII_NONSYSTEMCLASS;
                }

                if (pjs->GetPackageFlags() & CJS_FLAG_NEEDSTRUSTEDSOURCE) {
                    CurPackageInfo->dwFlags |= JPMPII_NEEDSTRUSTEDSOURCE;
                }

                njs++;
                pjs->SetState(INSTALL_DONE);
            }
        }

        Assert(pos == NULL);                // Should be exact count.

        // if nothing actually needs to be installed, skip calling InstallPackage
        if (njs == 0)
            goto Exit;

        PkgSecInfo.cbStruct = sizeof(PkgSecInfo);
        PJAVA_TRUST pjt = m_pdl->GetJavaTrust();
        if (pjt) {
            PkgSecInfo.pCapabilities = pjt->pbJavaPermissions;
            PkgSecInfo.cbCapabilities = pjt->cbJavaPermissions;
            PkgSecInfo.pSigner = pjt->pbSigner;
            PkgSecInfo.cbSigner = pjt->cbSigner;
            PkgSecInfo.fAllPermissions = pjt->fAllPermissions;
        }

        WCHAR szCabName[MAX_PATH];

        if (!MultiByteToWideChar(CP_ACP, 0, m_pdl->GetFileName(), -1, szCabName, MAX_PATH)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        hr = pcdl->GetPackageManager()->InstallPackage(szCabName, pwszNameSpace,
                JPMINST_CAB,&PackageInfo[0],njs,0, &PkgSecInfo);
            
    }


Exit:
    SAFEDELETE(PackageInfo);

    if (FAILED(hr)) {
        pcdl->CodeDownloadDebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_JAVA_PKG_FAILED, hr);
    }

    return hr;
}
