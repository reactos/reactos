/****************************************************************************\
 *
 *   RATINGS.CPP --Parses out the actual ratings from a site.
 *
 *   Created:   Ann McCurdy
 *     
\****************************************************************************/

/*Includes---------------------------------------------------------*/
#include "msrating.h"
#include "mslubase.h"
#include "ratings.h"
#include "parselbl.h"
#include "picsrule.h"
#include <convtime.h>
#include <contxids.h>
#include <shlwapip.h>

#include <wininet.h>

#include <mluisupp.h>

extern PICSRulesRatingSystem * g_pPRRS;
extern array<PICSRulesRatingSystem*> g_arrpPRRS;
extern PICSRulesRatingSystem * g_pApprovedPRRS;
extern PICSRulesRatingSystem * g_pApprovedPRRSPreApply;
extern array<PICSRulesRatingSystem*> g_arrpPICSRulesPRRSPreApply;

extern BOOL g_fPICSRulesEnforced,g_fApprovedSitesEnforced;

extern HMODULE g_hURLMON,g_hWININET;

extern char g_szLastURL[INTERNET_MAX_URL_LENGTH];

extern HINSTANCE hInstance;

HANDLE g_HandleGlobalCounter,g_ApprovedSitesHandleGlobalCounter;
long   g_lGlobalCounterValue,g_lApprovedSitesGlobalCounterValue;

DWORD g_dwDataSource;
BOOL  g_fInvalid;

PicsRatingSystemInfo *gPRSI = NULL;

//7c9c1e2a-4dcd-11d2-b972-0060b0c4834d
const GUID GUID_Ratings = { 0x7c9c1e2aL, 0x4dcd, 0x11d2, { 0xb9, 0x72, 0x00, 0x60, 0xb0, 0xc4, 0x83, 0x4d } };

//7c9c1e2b-4dcd-11d2-b972-0060b0c4834d
const GUID GUID_ApprovedSites = { 0x7c9c1e2bL, 0x4dcd, 0x11d2, { 0xb9, 0x72, 0x00, 0x60, 0xb0, 0xc4, 0x83, 0x4d } };

HRESULT WINAPI RatingInit()
{
    DWORD                   dwNumSystems,dwCounter;
    HRESULT                 hRes;
    PICSRulesRatingSystem   * pPRRS=NULL;

    g_hURLMON=LoadLibrary("URLMON.DLL");

    if(g_hURLMON==NULL)
    {
        g_pPRRS=NULL;                   //we couldn't load URLMON

        hRes=E_UNEXPECTED;
    }

    g_hWININET=LoadLibrary("WININET.DLL");

    if(g_hWININET==NULL)
    {
        g_pPRRS=NULL;                   //we couldn't load URLMON

        hRes=E_UNEXPECTED;
    }

    g_HandleGlobalCounter=SHGlobalCounterCreate(GUID_Ratings);
    g_lGlobalCounterValue=SHGlobalCounterGetValue(g_HandleGlobalCounter);

    g_ApprovedSitesHandleGlobalCounter=SHGlobalCounterCreate(GUID_ApprovedSites);
    g_lApprovedSitesGlobalCounterValue=SHGlobalCounterGetValue(g_ApprovedSitesHandleGlobalCounter);

    gPRSI = new PicsRatingSystemInfo;
    gPRSI->Init();

    hRes=PICSRulesReadFromRegistry(PICSRULES_APPROVEDSITES,&g_pApprovedPRRS);

    if(FAILED(hRes))
    {
        g_pApprovedPRRS=NULL;
    }

    hRes=PICSRulesGetNumSystems(&dwNumSystems);

    if(SUCCEEDED(hRes)) //we have PICSRules systems to inforce
    {
        for(dwCounter=PICSRULES_FIRSTSYSTEMINDEX;
            dwCounter<(dwNumSystems+PICSRULES_FIRSTSYSTEMINDEX);
            dwCounter++)
        {
            hRes=PICSRulesReadFromRegistry(dwCounter,&pPRRS);

            if(FAILED(hRes))
            {
                char    *lpszTitle,*lpszMessage;

                //we couldn't read in the systems, so don't inforce PICSRules,
                //and notify the user
                
                g_arrpPRRS.DeleteAll();

                lpszTitle=(char *) GlobalAlloc(GPTR,MAX_PATH);
                lpszMessage=(char *) GlobalAlloc(GPTR,MAX_PATH);

                MLLoadString(IDS_PICSRULES_TAMPEREDREADTITLE,(LPTSTR) lpszTitle,MAX_PATH);
                MLLoadString(IDS_PICSRULES_TAMPEREDREADMSG,(LPTSTR) lpszMessage,MAX_PATH);

                MessageBox(NULL,(LPCTSTR) lpszMessage,(LPCTSTR) lpszTitle,MB_OK|MB_ICONERROR);

                GlobalFree(lpszTitle);
                GlobalFree(lpszMessage);

                break;
            }
            else
            {
                g_arrpPRRS.Append(pPRRS);

                pPRRS=NULL;
            }
        }
    }

    return NOERROR; 
}

void RatingTerm()
{
    delete gPRSI;
    gPRSI = NULL;

    if(g_pApprovedPRRS != NULL)
    {
        delete g_pApprovedPRRS;
        g_pApprovedPRRS = NULL;
    }

    if(g_pApprovedPRRSPreApply != NULL)
    {
        delete g_pApprovedPRRSPreApply;
        g_pApprovedPRRSPreApply = NULL;
    }

    g_arrpPRRS.DeleteAll();
    g_arrpPICSRulesPRRSPreApply.DeleteAll();

    CloseHandle(g_HandleGlobalCounter);
    CloseHandle(g_ApprovedSitesHandleGlobalCounter);

    FreeLibrary(g_hURLMON);
    FreeLibrary(g_hWININET);
}


HRESULT WINAPI RatingEnabledQuery()
{
    CheckGlobalInfoRev();

    if (gPRSI && !gPRSI->fSettingsValid)
        return S_OK;

    if (gPRSI && gPRSI->fRatingInstalled) {
        PicsUser *pUser = ::GetUserObject();
        return (pUser && pUser->fEnabled) ? S_OK : S_FALSE;
    }
    else {
        return E_FAIL;
    }
}


HRESULT WINAPI RatingCheckUserAccess(LPCSTR pszUsername, LPCSTR pszURL,
                                     LPCSTR pszRatingInfo, LPBYTE pData,
                                     DWORD cbData, LPVOID *ppRatingDetails)
{
    HRESULT hRes;
    BOOL    fPassFail;

    g_fInvalid=FALSE;
    g_dwDataSource=cbData;
    g_fPICSRulesEnforced=FALSE;
    g_fApprovedSitesEnforced=FALSE;
    lstrcpy(g_szLastURL,pszURL);

    CheckGlobalInfoRev();

    if (ppRatingDetails != NULL)
        *ppRatingDetails = NULL;

    if (!gPRSI->fSettingsValid)
        return ResultFromScode(S_FALSE);

    if (!gPRSI->fRatingInstalled)
        return ResultFromScode(S_OK);

    PicsUser *pUser = GetUserObject(pszUsername);
    if (pUser == NULL) {
        return HRESULT_FROM_WIN32(ERROR_BAD_USERNAME);
    }

    if (!pUser->fEnabled)
        return ResultFromScode(S_OK);

    //check Approved Sites list
    hRes=PICSRulesCheckApprovedSitesAccess(pszURL,&fPassFail);

    if(SUCCEEDED(hRes)) //the list made a determination
    {
        g_fApprovedSitesEnforced=TRUE;

        if(fPassFail==PR_PASSFAIL_PASS)
        {
            return ResultFromScode(S_OK);
        }
        else
        {
            return ResultFromScode(S_FALSE);
        }
    }

    CParsedLabelList *pParsed=NULL;

    //check PICSRules systems
    hRes=PICSRulesCheckAccess(pszURL,pszRatingInfo,&fPassFail,&pParsed);

    if(SUCCEEDED(hRes)) //the list made a determination
    {
        g_fPICSRulesEnforced=TRUE;

        if (ppRatingDetails != NULL)
            *ppRatingDetails = pParsed;
        else
            FreeParsedLabelList(pParsed);

        if(fPassFail==PR_PASSFAIL_PASS)
        {
            return ResultFromScode(S_OK);
        }
        else
        {
            return ResultFromScode(S_FALSE);
        }
    }

    if (pszRatingInfo == NULL)
    {
        //Site is unrated.  Check if user can see unrated sites.
        if (pUser->fAllowUnknowns)
            return ResultFromScode(S_OK);
        else
            return ResultFromScode(S_FALSE);
    }    
    else
    {
        if(pParsed!=NULL)
        {
            hRes = S_OK;
        }
        else
        {
            hRes = ParseLabelList(pszRatingInfo, &pParsed);
        }
    }
    
    if (SUCCEEDED(hRes)) {
        BOOL fRated = FALSE;
        BOOL fDenied = FALSE;

        DWORD timeCurrent = GetCurrentNetDate();

        CParsedServiceInfo *psi = &pParsed->m_ServiceInfo;

        while (psi != NULL && !fDenied) {
            UserRatingSystem *pURS = pUser->FindRatingSystem(psi->m_pszServiceName);
            if (pURS != NULL && pURS->m_pPRS != NULL) {
                psi->m_fInstalled = TRUE;
                UINT cRatings = psi->aRatings.Length();
                for (UINT i=0; i<cRatings; i++) {
                    CParsedRating *pRating = &psi->aRatings[i];

                    if (!pRating->pOptions->CheckURL(pszURL)) {
                        pParsed->m_pszURL = new char[strlenf(pszURL) + 1];
                        if (pParsed->m_pszURL != NULL)
                            strcpyf(pParsed->m_pszURL, pszURL);

                        continue;    /* this rating has expired or is for
                                     * another URL, ignore it
                                     */
                    }

                    if (!pRating->pOptions->CheckUntil(timeCurrent))
                        continue;

                    UserRating *pUR = pURS->FindRating(pRating->pszTransmitName);
                    if (pUR != NULL) {
                        fRated = TRUE;
                        pRating->fFound = TRUE;
                        if ((*pUR).m_pPC!=NULL)
                        {
                            if ((pRating->nValue > (*((*pUR).m_pPC)).etnMax.Get())||
                                (pRating->nValue < (*((*pUR).m_pPC)).etnMin.Get()))
                            {
                                g_fInvalid = TRUE;
                                fDenied = TRUE;
                                pRating->fFailed = TRUE;
                            }
                        }
                        if (pRating->nValue > pUR->m_nValue) {
                            fDenied = TRUE;
                            pRating->fFailed = TRUE;
                        }
                        else
                            pRating->fFailed = FALSE;
                    }
                    else {
                        g_fInvalid = TRUE;
                        fDenied = TRUE;
                        pRating->fFailed = TRUE;
                    }
                }
            }
            else {
                psi->m_fInstalled = FALSE;
            }

            psi = psi->Next();
        }

        if (!fRated) {
            pParsed->m_fRated = FALSE;
            hRes = E_RATING_NOT_FOUND;
        }
        else {
            pParsed->m_fRated = TRUE;
            if (fDenied)
                hRes = ResultFromScode(S_FALSE);
        }
    }

    if (ppRatingDetails != NULL)
        *ppRatingDetails = pParsed;
    else
        FreeParsedLabelList(pParsed);

    return hRes;
}


void AppendString(HWND hwndEC, LPCSTR pszString)
{
    int cchEdit = ::GetWindowTextLength(hwndEC);
    ::SendMessage(hwndEC, EM_SETSEL, (WPARAM)cchEdit, (LPARAM)cchEdit);
    ::SendMessage(hwndEC, EM_REPLACESEL, 0, (LPARAM)pszString);
}


void AddSeparator(HWND hwndEC, BOOL fAppendToEnd)
{
    NLS_STR nlsTemp(MAX_RES_STR_LEN);
    if (nlsTemp.QueryError() != ERROR_SUCCESS)
        return;

    if (fAppendToEnd) {
        nlsTemp.LoadString(IDS_DESCRIPTION_SEPARATOR);
        AppendString(hwndEC, nlsTemp.QueryPch());
    }

    nlsTemp.LoadString(IDS_FRAME);
    if (fAppendToEnd)
        AppendString(hwndEC, nlsTemp.QueryPch());
    else {
        ::SendMessage(hwndEC, EM_SETSEL, 0, 0);
        ::SendMessage(hwndEC, EM_REPLACESEL, 0, (LPARAM)(LPCSTR)nlsTemp.QueryPch());
    }
}


const UINT MAX_CACHED_LABELS = 16;
struct PleaseDlgData
{
    LPCSTR pszUsername;
    LPCSTR pszContentDescription;
    PicsUser *pPU;
    CParsedLabelList *pLabelList;
    HWND hwndDlg;
    HWND hwndOwner;
    DWORD dwFlags;
    HWND hwndEC;
    UINT cLabels;
    LPSTR apLabelStrings[MAX_CACHED_LABELS];
};

const DWORD PDD_DONE = 0x1;
const DWORD PDD_ALLOW = 0x2;
const char szRatingsProp[] = "RatingsDialogHandleProp";


void InitPleaseDialog(HWND hDlg, PleaseDlgData *pdd)
{
/****
    rated:
        for each URS with m_fInstalled = TRUE:
            for each UR with m_fFailed = TRUE:
                add line to EC
        
    not rated:
        no label list? --> report not rated
        invalid string in label list? --> report invalid rating
        any URS's with invalid strings? --> report invalid rating
        any URS's with error strings? --> report label error
        no URS's marked installed? --> report unknown rating system
        for installed URS's:
            for each UR:
                options has invalid string? --> report invalid rating
                options expired? --> report expired
                not fFound? --> report unknown rating
****/

    if (pdd->hwndDlg == NULL) {
        pdd->hwndDlg = hDlg;

        /* Attach our data structure to the dialog so we can find it when the
         * dialog is dismissed, and on the owner window passed to the API so
         * we can find it on subsequent API calls.
         */
        SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pdd);
        SetProp(pdd->hwndOwner, szRatingsProp, (HANDLE)pdd);
    }

    CParsedLabelList *pLabelList = pdd->pLabelList;

    for (UINT i=0; i<pdd->cLabels && i<ARRAYSIZE(pdd->apLabelStrings); i++) {
        if (pdd->apLabelStrings[i] == NULL) {
            if (pLabelList == NULL || pLabelList->m_pszOriginalLabel == NULL) {
                return;
            }
        }
        else {
            if (pLabelList != NULL &&
                pLabelList->m_pszOriginalLabel != NULL &&
                !::strcmpf(pdd->apLabelStrings[i], pLabelList->m_pszOriginalLabel)) {
                return;
            }
        }
    }

    if (pdd->cLabels < ARRAYSIZE(pdd->apLabelStrings)) {
        if (pLabelList == NULL || pLabelList->m_pszOriginalLabel == NULL)
            pdd->apLabelStrings[pdd->cLabels] = NULL;
        else {
            pdd->apLabelStrings[pdd->cLabels] = new char[::strlenf(pLabelList->m_pszOriginalLabel)+1];
            if (pdd->apLabelStrings[pdd->cLabels] != NULL) {
                ::strcpyf(pdd->apLabelStrings[pdd->cLabels], pLabelList->m_pszOriginalLabel);
            }
        }
    }

    HWND hwndDescription = ::GetDlgItem(hDlg, IDC_CONTENTDESCRIPTION);
    HWND hwndError = ::GetDlgItem(hDlg, IDC_CONTENTERROR);
    HWND hwndPrevEC = pdd->hwndEC;

    /* There are two edit controls in the dialog.  One, the "description"
     * control, has a horizontal scrollbar, because we don't want word wrap
     * for the category names (cleaner presentation).  The other EC has no
     * scrollbar so that the lengthy error strings will wordwrap.
     *
     * If we've been using the description control and we add error-type
     * information, we need to copy the text out of the description control
     * into the error control and show it.
     */
    BOOL fRatedPage = (pLabelList != NULL && pLabelList->m_fRated);
    if (!fRatedPage && pdd->hwndEC == hwndDescription) {
        NLS_STR nlsTemp(::GetWindowTextLength(hwndDescription));
        if (nlsTemp.QueryError() == ERROR_SUCCESS) {
            GetWindowText(hwndDescription, nlsTemp.Party(), nlsTemp.QueryAllocSize());
            nlsTemp.DonePartying();
            SetWindowText(hwndError, nlsTemp.QueryPch());
        }
        pdd->hwndEC = hwndError;
    }
    else if (pdd->hwndEC == NULL)
        pdd->hwndEC = fRatedPage ? hwndDescription : hwndError;

    if (pdd->hwndEC != hwndPrevEC) {
        BOOL fShowErrorCtl = (pdd->hwndEC == hwndError);
        if (::GetFocus() == hwndPrevEC)
            SetFocus(pdd->hwndEC);
        ShowWindow(hwndError, fShowErrorCtl ? SW_SHOW : SW_HIDE);
        EnableWindow(hwndError, fShowErrorCtl);
        ShowWindow(hwndDescription, fShowErrorCtl ? SW_HIDE : SW_SHOW);
        EnableWindow(hwndDescription, !fShowErrorCtl);
    }

    /* If there's already just one label in the list, prefix it with a
     * label "Frame:" since there will now be two.
     */
    if (pdd->cLabels == 1)
        AddSeparator(pdd->hwndEC, FALSE);

    /* If this is not the first label we're adding, we need a full separator
     * appended before we add new descriptive text.
     */
    if (pdd->cLabels > 0)
        AddSeparator(pdd->hwndEC, TRUE);

    if (g_fInvalid)
    {
        char szSourceMessage[MAX_PATH];

        MLLoadString(IDS_TAMPEREDRATING1,(char *) szSourceMessage,MAX_PATH);

        AppendString(pdd->hwndEC, szSourceMessage);
        AppendString(pdd->hwndEC, "\x0D\x0A");

        MLLoadString(IDS_TAMPEREDRATING2,(char *) szSourceMessage,MAX_PATH);
        AppendString(pdd->hwndEC, szSourceMessage);
    }
    else if (fRatedPage) {
        NLS_STR nlsTemplate(MAX_RES_STR_LEN);
        nlsTemplate.LoadString(IDS_RATINGTEMPLATE);
        NLS_STR nlsTmp;
        if (nlsTemplate.QueryError() || nlsTmp.QueryError())
            return;

        for (CParsedServiceInfo *ppsi = &pLabelList->m_ServiceInfo;
             ppsi != NULL;
             ppsi = ppsi->Next()) {

            if (!ppsi->m_fInstalled)
                continue;

            UserRatingSystem *pURS = pdd->pPU->FindRatingSystem(ppsi->m_pszServiceName);
            if (pURS == NULL || pURS->m_pPRS == NULL)
                continue;
            NLS_STR nlsSystemName(STR_OWNERALLOC, pURS->m_pPRS->etstrName.Get());
            UINT cRatings = ppsi->aRatings.Length();
            for (UINT i=0; i<cRatings; i++) {
                CParsedRating *pRating = &ppsi->aRatings[i];
                if (pRating->fFailed) {
                    nlsTmp = nlsTemplate;
                    UserRating *pUR = pURS->FindRating(pRating->pszTransmitName);
                    if (pUR == NULL)
                        continue;
                    PicsCategory *pPC = pUR->m_pPC;
                    if (pPC == NULL)
                        continue;

                    LPCSTR pszCategory;
                    if (pPC->etstrName.fIsInit())
                        pszCategory = pPC->etstrName.Get();
                    else if (pPC->etstrDesc.fIsInit())
                        pszCategory = pPC->etstrDesc.Get();
                    else
                        pszCategory = pRating->pszTransmitName;

                    NLS_STR nlsCategoryName(STR_OWNERALLOC, (LPSTR)pszCategory);
                    UINT cValues = pPC->arrpPE.Length();
                    PicsEnum *pPE;
                    for (UINT iValue=0; iValue<cValues; iValue++) {
                        pPE = pPC->arrpPE[iValue];
                        if (pPE->etnValue.Get() == pRating->nValue)
                            break;
                    }

                    LPCSTR pszValue = szNULL;
                    char szNumBuf[20];
                    if (iValue < cValues) {
                        if (pPE->etstrName.fIsInit())
                            pszValue = pPE->etstrName.Get();
                        else if (pPE->etstrDesc.fIsInit())
                            pszValue = pPE->etstrDesc.Get();
                        else {
                            wsprintf(szNumBuf, "%d", pRating->nValue);
                            pszValue = szNumBuf;
                        }
                    }
                    NLS_STR nlsValueName(STR_OWNERALLOC, (LPSTR)pszValue);
                    const NLS_STR *apnls[] = { &nlsSystemName, &nlsCategoryName, &nlsValueName, NULL };
                    nlsTmp.InsertParams(apnls);
                    if (!nlsTmp.QueryError()) {
                        AppendString(pdd->hwndEC, nlsTmp.QueryPch());
                    }
                }
            }
        }

        if((g_fPICSRulesEnforced!=TRUE)&&(g_fApprovedSitesEnforced!=TRUE))
        {
            UINT idSourceMsg;
            char szSourceMessage[MAX_PATH];

            switch(g_dwDataSource)
            {
                case PICS_LABEL_FROM_HEADER:
                {
                    idSourceMsg=IDS_SOURCE_SERVER;
                    break;
                }
                case PICS_LABEL_FROM_PAGE:
                {
                    idSourceMsg=IDS_SOURCE_EMBEDDED;
                    break;
                }
                case PICS_LABEL_FROM_BUREAU:
                {
                    idSourceMsg=IDS_SOURCE_BUREAU;
                    break;
                }
            }

            MLLoadString(idSourceMsg,(char *) szSourceMessage,MAX_PATH);

            AppendString(pdd->hwndEC, "\x0D\x0A");
            AppendString(pdd->hwndEC, szSourceMessage);
        }
    }
    else {
        UINT idMsg = 0;
        LPCSTR psz1=szNULL, psz2=szNULL;
        if (pLabelList == NULL)
            idMsg = IDS_UNRATED;
        else if (pLabelList->m_pszInvalidString) {
            idMsg = IDS_INVALIDRATING;
            psz1 = pLabelList->m_pszInvalidString;
        }
        else {
            BOOL fErrorFound = FALSE;
            BOOL fAnyInstalled = FALSE;
            CParsedServiceInfo *ppsi = &pLabelList->m_ServiceInfo;
            while (ppsi != NULL) {
                if (ppsi->m_pszInvalidString) {
                    idMsg = IDS_INVALIDRATING;
                    psz1 = ppsi->m_pszInvalidString;
                    fErrorFound = TRUE;
                }
                else if (ppsi->m_pszErrorString) {
                    idMsg = IDS_LABELERROR;
                    psz1 = ppsi->m_pszErrorString;
                    fErrorFound = TRUE;
                }
                else if (ppsi->m_fInstalled)
                    fAnyInstalled = TRUE;
                ppsi = ppsi->Next();
            }
            if (!fErrorFound) {
                if (!fAnyInstalled) {
                    idMsg = IDS_UNKNOWNSYSTEM;
                    psz1 = pLabelList->m_ServiceInfo.m_pszServiceName;
                }
                else {
                    for (ppsi = &pLabelList->m_ServiceInfo;
                         ppsi != NULL;
                         ppsi = ppsi->Next()) {
                        if (!ppsi->m_fInstalled)
                            continue;
                        UINT cRatings = ppsi->aRatings.Length();
                        for (UINT i=0; i<cRatings; i++) {
                            CParsedRating *ppr = &ppsi->aRatings[i];
                            COptionsBase *pOpt = ppr->pOptions;
                            if (pOpt->m_pszInvalidString) {
                                idMsg = IDS_INVALIDRATING;
                                psz1 = pOpt->m_pszInvalidString;
                                break;
                            }
                            else if (pOpt->m_fdwFlags & LBLOPT_WRONGURL) {
                                idMsg = IDS_WRONGURL;
                                psz1 = pLabelList->m_pszURL;
                                psz2 = pOpt->m_pszURL;
                            }
                            else if (pOpt->m_fdwFlags & LBLOPT_EXPIRED) {
                                idMsg = IDS_EXPIRED;
                                break;
                            }
                            else if (!ppr->fFound) {
                                idMsg = IDS_UNKNOWNRATING;
                                psz1 = ppr->pszTransmitName;
                                UserRatingSystem *pURS = pdd->pPU->FindRatingSystem(ppsi->m_pszServiceName);
                                if (pURS != NULL && pURS->m_pPRS != NULL) {
                                    if (pURS->m_pPRS->etstrName.fIsInit()) {
                                        psz2 = pURS->m_pPRS->etstrName.Get();
                                    }
                                }
                                break;
                            }
                        }
                        if (idMsg != 0)
                            break;
                    }
                }
            }
        }

        if(g_fPICSRulesEnforced==TRUE)
        {
            idMsg=IDS_PICSRULES_ENFORCED;
        }
        else if (g_fApprovedSitesEnforced==TRUE)
        {
            idMsg=IDS_APPROVEDSITES_ENFORCED;
        }

        /* It's theoretically possible that we got through all that and
         * didn't find anything explicitly wrong, yet the site was considered
         * unrated (perhaps it was a valid label with no actual ratings in it).
         * So if we didn't decide what error to display, just don't stick
         * anything in the edit control, and the dialog will just say "Sorry"
         * at the top.
         */
        if (idMsg != 0) {
            NLS_STR nls1(STR_OWNERALLOC, (LPSTR)psz1);
            NLS_STR nls2(STR_OWNERALLOC, (LPSTR)psz2);
            const NLS_STR *apnls[] = { &nls1, &nls2, NULL };
            NLS_STR nlsMessage(MAX_RES_STR_LEN);
            nlsMessage.LoadString((USHORT)idMsg, apnls);
            AppendString(pdd->hwndEC, nlsMessage.QueryPch());
        }

        if (idMsg == IDS_UNKNOWNSYSTEM) {
            NLS_STR nlsTemplate(MAX_RES_STR_LEN);
            nlsTemplate.LoadString(IDS_UNKNOWNRATINGTEMPLATE);
            NLS_STR nlsTmp;
            if (nlsTemplate.QueryError() || nlsTmp.QueryError())
                return;
            
            UINT cRatings = pLabelList->m_ServiceInfo.aRatings.Length();
            for (UINT i=0; i<cRatings; i++) {
                CParsedRating *ppr = &pLabelList->m_ServiceInfo.aRatings[i];

                char szNumBuf[20];
                wsprintf(szNumBuf, "%d", ppr->nValue);

                NLS_STR nlsCategoryName(STR_OWNERALLOC, ppr->pszTransmitName);
                NLS_STR nlsValueName(STR_OWNERALLOC, szNumBuf);
                const NLS_STR *apnls[] = { &nlsCategoryName, &nlsValueName, NULL };
                nlsTmp = nlsTemplate;
                nlsTmp.InsertParams(apnls);
                if (!nlsTmp.QueryError()) {
                    AppendString(pdd->hwndEC, nlsTmp.QueryPch());
                }
            }
        }
    }

    SendDlgItemMessage(hDlg,
                       IDC_BLOCKING_ONCE,
                       BM_SETCHECK,
                       (WPARAM) BST_CHECKED,
                       (LPARAM) 0);

    SendDlgItemMessage(hDlg,
                       IDC_BLOCKING_PAGE,
                       BM_SETCHECK,
                       (WPARAM) BST_UNCHECKED,
                       (LPARAM) 0);

    SendDlgItemMessage(hDlg,
                       IDC_BLOCKING_SITE,
                       BM_SETCHECK,
                       (WPARAM) BST_UNCHECKED,
                       (LPARAM) 0);

    pdd->cLabels++;       /* now one more label description in the box */
}


const UINT WM_NEWDIALOG = WM_USER + 1000;

void EndPleaseDialog(HWND hDlg, BOOL fRet)
{
    PleaseDlgData *ppdd = (PleaseDlgData *)GetWindowLongPtr(hDlg, DWLP_USER);
    if (ppdd != NULL) {
        ppdd->dwFlags = PDD_DONE | (fRet ? PDD_ALLOW : 0);
        ppdd->hwndDlg = NULL;
        RemoveProp(ppdd->hwndOwner, szRatingsProp);
    }
    EndDialog(hDlg, fRet);
}

//The FN_INTERNETCRACKURL type describes the URLMON function InternetCrackUrl
typedef BOOL (*FN_INTERNETCRACKURL)(LPCTSTR lpszUrl,DWORD dwUrlLength,DWORD dwFlags,LPURL_COMPONENTS lpUrlComponents);

HRESULT AddToApprovedSites(BOOL fAlwaysNever,BOOL fSitePage)
{
    PICSRulesPolicy             * pPRPolicy;
    PICSRulesByURL              * pPRByURL;
    PICSRulesByURLExpression    * pPRByURLExpression;
    char                        * lpszSiteURL;
    HRESULT                     hRes;
    URL_COMPONENTS              URLComponents;
    FN_INTERNETCRACKURL         pfnInternetCrackUrl;
    INTERNET_SCHEME             INetScheme=INTERNET_SCHEME_DEFAULT;
    INTERNET_PORT               INetPort=INTERNET_INVALID_PORT_NUMBER;
    LPSTR                       lpszScheme,lpszHostName,lpszUserName,
                                lpszPassword,lpszUrlPath,lpszExtraInfo;
    BOOL                        fAddedScheme=FALSE;
    int                         iCounter,iLoopCounter;

    lpszSiteURL=new char[INTERNET_MAX_URL_LENGTH];

    if(lpszSiteURL==NULL)
    {
        return(E_OUTOFMEMORY);
    }

    strcpy(lpszSiteURL,g_szLastURL);

    if(g_pApprovedPRRS==NULL)
    {
        g_pApprovedPRRS=new PICSRulesRatingSystem;

        if(g_pApprovedPRRS==NULL)
        {
            return(E_OUTOFMEMORY);
        }
    }

    pPRPolicy=new PICSRulesPolicy;

    if(pPRPolicy==NULL)
    {
        return(E_OUTOFMEMORY);
    }

    pPRByURL=new PICSRulesByURL;

    if(pPRByURL==NULL)
    {
        return(E_OUTOFMEMORY);
    }

    if(fAlwaysNever==PICSRULES_NEVER)
    {
        pPRPolicy->m_PRPolicyAttribute=PR_POLICY_REJECTBYURL;
        pPRPolicy->AddItem(PROID_REJECTBYURL,pPRByURL);
    }
    else
    {
        pPRPolicy->m_PRPolicyAttribute=PR_POLICY_ACCEPTBYURL;
        pPRPolicy->AddItem(PROID_ACCEPTBYURL,pPRByURL);
    }

    pPRByURLExpression=new PICSRulesByURLExpression;
    
    if(pPRByURLExpression==NULL)
    {
        return(E_OUTOFMEMORY);
    }

    pPRByURL->m_arrpPRByURL.Append(pPRByURLExpression);

    //if we made it through all that, then we have a
    //PICSRulesByURLExpression to fill out, and need
    //to update the list box.

    lpszScheme=new char[INTERNET_MAX_SCHEME_LENGTH+1];
    lpszHostName=new char[INTERNET_MAX_PATH_LENGTH+1];
    lpszUserName=new char[INTERNET_MAX_PATH_LENGTH+1];
    lpszPassword=new char[INTERNET_MAX_PATH_LENGTH+1];
    lpszUrlPath=new char[INTERNET_MAX_PATH_LENGTH+1];
    lpszExtraInfo=new char[INTERNET_MAX_PATH_LENGTH+1];

    if(lpszScheme==NULL ||
       lpszHostName==NULL ||
       lpszUserName==NULL ||
       lpszPassword==NULL ||
       lpszUrlPath==NULL ||
       lpszExtraInfo==NULL)
    {
        return(E_OUTOFMEMORY);
    }

    URLComponents.dwStructSize=sizeof(URL_COMPONENTS);
    URLComponents.lpszScheme=lpszScheme;
    URLComponents.dwSchemeLength=INTERNET_MAX_SCHEME_LENGTH;
    URLComponents.nScheme=INetScheme;
    URLComponents.lpszHostName=lpszHostName;
    URLComponents.dwHostNameLength=INTERNET_MAX_PATH_LENGTH;
    URLComponents.nPort=INetPort;
    URLComponents.lpszUserName=lpszUserName;
    URLComponents.dwUserNameLength=INTERNET_MAX_PATH_LENGTH;
    URLComponents.lpszPassword=lpszPassword;
    URLComponents.dwPasswordLength=INTERNET_MAX_PATH_LENGTH;
    URLComponents.lpszUrlPath=lpszUrlPath;
    URLComponents.dwUrlPathLength=INTERNET_MAX_PATH_LENGTH;
    URLComponents.lpszExtraInfo=lpszExtraInfo;
    URLComponents.dwExtraInfoLength=INTERNET_MAX_PATH_LENGTH;

    pfnInternetCrackUrl=(FN_INTERNETCRACKURL) GetProcAddress(g_hWININET,"InternetCrackUrlA");

    if(pfnInternetCrackUrl==NULL)
    {
        return(E_UNEXPECTED);
    }

    pfnInternetCrackUrl(lpszSiteURL,0,ICU_DECODE,&URLComponents);

    delete lpszExtraInfo; //we don't do anything with this for now
    delete lpszPassword; //not supported by PICSRules

    if(g_fApprovedSitesEnforced==TRUE)
    {
        int             iCounter;
        PICSRulesPolicy * pPRFindPolicy;
        BOOL            fFound=FALSE,fDeleted=FALSE;
        
        //we've already got an Approved Sites setting enforcing this
        //so check for an exact match, and if it exists, change it
        //instead of adding another
        
        for(iCounter=0;iCounter<g_pApprovedPRRS->m_arrpPRPolicy.Length();iCounter++)
        {
            PICSRulesByURLExpression * pPRFindByURLExpression;
            PICSRulesByURL           * pPRFindByURL;
            char                     * lpszTest;

            pPRFindPolicy=g_pApprovedPRRS->m_arrpPRPolicy[iCounter];

            switch(pPRFindPolicy->m_PRPolicyAttribute)
            {
                case PR_POLICY_REJECTBYURL:
                {
                    pPRFindByURL=pPRFindPolicy->m_pPRRejectByURL;

                    break;
                }
                case PR_POLICY_ACCEPTBYURL:
                {
                    pPRFindByURL=pPRFindPolicy->m_pPRAcceptByURL;

                    break;
                }
            }

            pPRFindByURLExpression=pPRFindByURL->m_arrpPRByURL[0];

            if((pPRFindByURLExpression->m_bNonWild)&BYURL_SCHEME)
            {
                if(lpszScheme==NULL)
                {
                    fFound=FALSE;

                    continue;
                }

                lpszTest=pPRFindByURLExpression->m_etstrScheme.Get();

                if(lstrcmpi(lpszScheme,lpszTest)==0)
                {
                    fFound=TRUE;
                }
                else
                {
                    fFound=FALSE;

                    continue;
                }
            }
            else
            {
                fFound=TRUE;
            }

            if((pPRFindByURLExpression->m_bNonWild)&BYURL_USER)
            {
                if(lpszUserName==NULL)
                {
                    fFound=FALSE;

                    continue;
                }

                lpszTest=pPRFindByURLExpression->m_etstrUser.Get();

                if(lstrcmpi(lpszUserName,lpszTest)==0)
                {
                    fFound=TRUE;
                }
                else
                {
                    fFound=FALSE;

                    continue;
                }
            }
            else
            {
                fFound=TRUE;
            }

            if((pPRFindByURLExpression->m_bNonWild)&BYURL_HOST)
            {
                if(lpszHostName==NULL)
                {
                    fFound=FALSE;

                    continue;
                }

                lpszTest=pPRFindByURLExpression->m_etstrHost.Get();

                if(lstrcmp(lpszHostName,lpszTest)==0)
                {
                    fFound=TRUE;
                }
                else
                {
                    fFound=FALSE;

                    continue;
                }
            }
            else
            {
                fFound=TRUE;
            }

            if(fSitePage!=PICSRULES_SITE)
            {
                if((pPRFindByURLExpression->m_bNonWild)&BYURL_PATH)
                {
                    int iLen;

                    if(lpszUrlPath==NULL)
                    {
                        fFound=FALSE;

                        continue;
                    }

                    lpszTest=pPRFindByURLExpression->m_etstrPath.Get();

                    //kill trailing slashes
                    iLen=lstrlen(lpszTest);

                    if(lpszTest[iLen-1]=='/')
                    {
                        lpszTest[iLen-1]='\0';
                    }

                    iLen=lstrlen(lpszUrlPath);

                    if(lpszUrlPath[iLen-1]=='/')
                    {
                        lpszUrlPath[iLen-1]='\0';
                    }
                    
                    if(lstrcmp(lpszUrlPath,lpszTest)==0)
                    {
                        fFound=TRUE;
                    }
                    else
                    {
                        fFound=FALSE;

                        continue;
                    }
                }
                else
                {
                    fFound=FALSE;

                    continue;
                }
            }

            if(fFound==TRUE)
            {
                if(fSitePage==PICSRULES_PAGE)
                {
                    break;
                }
                else
                {
                    delete pPRFindPolicy;

                    g_pApprovedPRRS->m_arrpPRPolicy[iCounter]=NULL;

                    fDeleted=TRUE;
                }
            }
        }

        if(fDeleted==TRUE)
        {
            PICSRulesRatingSystem * pPRRSNew;

            pPRRSNew=new PICSRulesRatingSystem;

            if(pPRRSNew==NULL)
            {
                return(E_OUTOFMEMORY);
            }

            for(iCounter=0;iCounter<g_pApprovedPRRS->m_arrpPRPolicy.Length();iCounter++)
            {
                if((g_pApprovedPRRS->m_arrpPRPolicy[iCounter])!=NULL)
                {
                    pPRRSNew->m_arrpPRPolicy.Append((g_pApprovedPRRS->m_arrpPRPolicy[iCounter]));
                }
            }

            g_pApprovedPRRS->m_arrpPRPolicy.ClearAll();

            delete g_pApprovedPRRS;

            g_pApprovedPRRS=pPRRSNew;

            fFound=FALSE;
        }

        if(fFound==TRUE)
        {
            delete pPRFindPolicy;

            g_pApprovedPRRS->m_arrpPRPolicy[iCounter]=pPRPolicy;
        }
        else
        {
            hRes=g_pApprovedPRRS->AddItem(PROID_POLICY,pPRPolicy);

            if(FAILED(hRes))
            {
                return(hRes);
            }
        }
    }
    else
    {
        hRes=g_pApprovedPRRS->AddItem(PROID_POLICY,pPRPolicy);

        if(FAILED(hRes))
        {
            return(hRes);
        }
    }

    pPRByURLExpression->m_fInternetPattern=TRUE;

    if((*lpszScheme!=NULL)&&(fAddedScheme==FALSE))
    {
        pPRByURLExpression->m_bNonWild|=BYURL_SCHEME;
        pPRByURLExpression->m_etstrScheme.SetTo(lpszScheme);   
    }
    else
    {
        delete lpszScheme;
    }
    pPRByURLExpression->m_bSpecified|=BYURL_SCHEME;

    if(*lpszUserName!=NULL)
    {
        pPRByURLExpression->m_bNonWild|=BYURL_USER;           
        pPRByURLExpression->m_etstrUser.SetTo(lpszUserName);
    }
    else
    {
        delete lpszUserName;
    }
    pPRByURLExpression->m_bSpecified|=BYURL_USER;

    if(*lpszHostName!=NULL)
    {
        pPRByURLExpression->m_bNonWild|=BYURL_HOST;           
        pPRByURLExpression->m_etstrHost.SetTo(lpszHostName);
    }
    else
    {
        delete lpszHostName;
    }
    pPRByURLExpression->m_bSpecified|=BYURL_HOST;

    if(*lpszUrlPath!=NULL)
    {
        if(lstrcmp(lpszUrlPath,"/")!=0)
        {
            if(fSitePage==PICSRULES_PAGE)
            {
                pPRByURLExpression->m_bNonWild|=BYURL_PATH;           
                pPRByURLExpression->m_etstrPath.SetTo(lpszUrlPath);
            }
        }
    }
    else
    {
        delete lpszUrlPath;
    }
    pPRByURLExpression->m_bSpecified|=BYURL_PATH;

    if(URLComponents.nPort!=INTERNET_INVALID_PORT_NUMBER)
    {
        LPSTR lpszTemp;

        lpszTemp=new char[MAX_PATH];

        wsprintf(lpszTemp,"%d",URLComponents.nPort);

        pPRByURLExpression->m_bNonWild|=BYURL_PORT;           
        pPRByURLExpression->m_etstrPort.SetTo(lpszTemp);
    }
    pPRByURLExpression->m_bSpecified|=BYURL_PORT;

    if(fSitePage==PICSRULES_PAGE)
    {
        pPRByURLExpression->m_etstrURL.SetTo(lpszSiteURL);
    }
    else
    {
        pPRByURLExpression->m_etstrURL.Set(pPRByURLExpression->m_etstrHost.Get());
    }

    //copy master list to the PreApply list so we can reorder the entries
    //to obtain the appropriate logic.

    if(g_pApprovedPRRSPreApply!=NULL)
    {
        delete g_pApprovedPRRSPreApply;
    }

    g_pApprovedPRRSPreApply=new PICSRulesRatingSystem;

    if(g_pApprovedPRRSPreApply==NULL)
    {
        char    szTitle[MAX_PATH],szMessage[MAX_PATH];

        //out of memory, so we init on the stack

        MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
        MLLoadString(IDS_PICSRULES_OUTOFMEMORY,(LPTSTR) szMessage,MAX_PATH);

        MessageBox(NULL,(LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);

        return(E_OUTOFMEMORY);
    }

    for(iCounter=0;iCounter<g_pApprovedPRRS->m_arrpPRPolicy.Length();iCounter++)
    {
        PICSRulesPolicy             * pPRPolicy,* pPRPolicyToCopy;
        PICSRulesByURL              * pPRByURL,* pPRByURLToCopy;
        PICSRulesByURLExpression    * pPRByURLExpression,* pPRByURLExpressionToCopy;

        pPRPolicy=new PICSRulesPolicy;
        
        if(pPRPolicy==NULL)
        {
            char    szTitle[MAX_PATH],szMessage[MAX_PATH];

            //out of memory, so we init on the stack

            MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
            MLLoadString(IDS_PICSRULES_OUTOFMEMORY,(LPTSTR) szMessage,MAX_PATH);

            MessageBox(NULL,(LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);

            return(E_OUTOFMEMORY);
        }

        pPRPolicyToCopy=g_pApprovedPRRS->m_arrpPRPolicy[iCounter];
        
        pPRPolicy->m_PRPolicyAttribute=pPRPolicyToCopy->m_PRPolicyAttribute;

        pPRByURL=new PICSRulesByURL;
        
        if(pPRByURL==NULL)
        {
            char    szTitle[MAX_PATH],szMessage[MAX_PATH];

            //out of memory, so we init on the stack

            MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
            MLLoadString(IDS_PICSRULES_OUTOFMEMORY,(LPTSTR) szMessage,MAX_PATH);

            MessageBox(NULL,(LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);

            return(E_OUTOFMEMORY);
        }

        if(pPRPolicy->m_PRPolicyAttribute==PR_POLICY_ACCEPTBYURL)
        {
            pPRByURLToCopy=pPRPolicyToCopy->m_pPRAcceptByURL;
            
            pPRPolicy->m_pPRAcceptByURL=pPRByURL;
        }
        else
        {
            pPRByURLToCopy=pPRPolicyToCopy->m_pPRRejectByURL;

            pPRPolicy->m_pPRRejectByURL=pPRByURL;
        }

        pPRByURLExpression=new PICSRulesByURLExpression;

        if(pPRByURLExpression==NULL)
        {
            char    szTitle[MAX_PATH],szMessage[MAX_PATH];

            //out of memory, so we init on the stack

            MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
            MLLoadString(IDS_PICSRULES_OUTOFMEMORY,(LPTSTR) szMessage,MAX_PATH);

            MessageBox(NULL,(LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);

            return(E_OUTOFMEMORY);
        }

        pPRByURLExpressionToCopy=pPRByURLToCopy->m_arrpPRByURL[0];

        if(pPRByURLExpressionToCopy==NULL)
        {
            char    *lpszTitle,*lpszMessage;

            //we shouldn't ever get here

            lpszTitle=(char *) GlobalAlloc(GPTR,MAX_PATH);
            lpszMessage=(char *) GlobalAlloc(GPTR,MAX_PATH);

            MLLoadString(IDS_ERROR,(LPTSTR) lpszTitle,MAX_PATH);
            MLLoadString(IDS_PICSRULES_NOAPPROVEDSAVE,(LPTSTR) lpszMessage,MAX_PATH);

            MessageBox(NULL,(LPCTSTR) lpszMessage,(LPCTSTR) lpszTitle,MB_OK|MB_ICONERROR);

            GlobalFree(lpszTitle);
            GlobalFree(lpszMessage);

            delete pPRPolicy;

            return(E_UNEXPECTED);
        }

        pPRByURLExpression->m_fInternetPattern=pPRByURLExpressionToCopy->m_fInternetPattern;
        pPRByURLExpression->m_bNonWild=pPRByURLExpressionToCopy->m_bNonWild;
        pPRByURLExpression->m_bSpecified=pPRByURLExpressionToCopy->m_bSpecified;
        pPRByURLExpression->m_etstrScheme.Set(pPRByURLExpressionToCopy->m_etstrScheme.Get());
        pPRByURLExpression->m_etstrUser.Set(pPRByURLExpressionToCopy->m_etstrUser.Get());
        pPRByURLExpression->m_etstrHost.Set(pPRByURLExpressionToCopy->m_etstrHost.Get());
        pPRByURLExpression->m_etstrPort.Set(pPRByURLExpressionToCopy->m_etstrPort.Get());
        pPRByURLExpression->m_etstrPath.Set(pPRByURLExpressionToCopy->m_etstrPath.Get());
        pPRByURLExpression->m_etstrURL.Set(pPRByURLExpressionToCopy->m_etstrURL.Get());

        
        pPRByURL->m_arrpPRByURL.Append(pPRByURLExpression);

        g_pApprovedPRRSPreApply->m_arrpPRPolicy.Append(pPRPolicy);
    }

    if(g_pApprovedPRRS!=NULL)
    {
        delete g_pApprovedPRRS;
    }

    g_pApprovedPRRS=new PICSRulesRatingSystem;

    if(g_pApprovedPRRS==NULL)
    {
        char    szTitle[MAX_PATH],szMessage[MAX_PATH];

        //out of memory, so we init on the stack

        MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
        MLLoadString(IDS_PICSRULES_OUTOFMEMORY,(LPTSTR) szMessage,MAX_PATH);

        MessageBox(NULL,(LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);

        return(E_OUTOFMEMORY);
    }

    for(iLoopCounter=0;iLoopCounter<2;iLoopCounter++)
    {
        for(iCounter=0;iCounter<g_pApprovedPRRSPreApply->m_arrpPRPolicy.Length();iCounter++)
        {
            PICSRulesPolicy             * pPRPolicy,* pPRPolicyToCopy;
            PICSRulesByURL              * pPRByURL,* pPRByURLToCopy;
            PICSRulesByURLExpression    * pPRByURLExpression,* pPRByURLExpressionToCopy;

            pPRPolicyToCopy=g_pApprovedPRRSPreApply->m_arrpPRPolicy[iCounter];

            if(pPRPolicyToCopy->m_PRPolicyAttribute==PR_POLICY_ACCEPTBYURL)
            {
                pPRByURLToCopy=pPRPolicyToCopy->m_pPRAcceptByURL;
            }
            else
            {
                pPRByURLToCopy=pPRPolicyToCopy->m_pPRRejectByURL;
            }

            pPRByURLExpressionToCopy=pPRByURLToCopy->m_arrpPRByURL[0];

            if(pPRByURLExpressionToCopy==NULL)
            {
                char    *lpszTitle,*lpszMessage;

                //we shouldn't ever get here

                lpszTitle=(char *) GlobalAlloc(GPTR,MAX_PATH);
                lpszMessage=(char *) GlobalAlloc(GPTR,MAX_PATH);

                MLLoadString(IDS_ERROR,(LPTSTR) lpszTitle,MAX_PATH);
                MLLoadString(IDS_PICSRULES_NOAPPROVEDSAVE,(LPTSTR) lpszMessage,MAX_PATH);

                MessageBox(NULL,(LPCTSTR) lpszMessage,(LPCTSTR) lpszTitle,MB_OK|MB_ICONERROR);

                GlobalFree(lpszTitle);
                GlobalFree(lpszMessage);

                return(E_UNEXPECTED);
            }

            //we want to put all of the non-sitewide approved sites first
            //so that a user can specify, allow all of xyz.com except for
            //xyz.com/foo.htm
            switch(iLoopCounter)
            {
                case 0:
                {
                    if((pPRByURLExpressionToCopy->m_bNonWild)&BYURL_PATH)
                    {
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }
                case 1:
                {
                    if(!((pPRByURLExpressionToCopy->m_bNonWild)&BYURL_PATH))
                    {
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }
            }

            pPRPolicy=new PICSRulesPolicy;
    
            if(pPRPolicy==NULL)
            {
                char    szTitle[MAX_PATH],szMessage[MAX_PATH];

                //out of memory, so we init on the stack

                MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
                MLLoadString(IDS_PICSRULES_OUTOFMEMORY,(LPTSTR) szMessage,MAX_PATH);

                MessageBox(NULL,(LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);

                return(E_OUTOFMEMORY);
            }
   
            pPRPolicy->m_PRPolicyAttribute=pPRPolicyToCopy->m_PRPolicyAttribute;

            pPRByURL=new PICSRulesByURL;
    
            if(pPRByURL==NULL)
            {
                char    szTitle[MAX_PATH],szMessage[MAX_PATH];

                //out of memory, so we init on the stack

                MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
                MLLoadString(IDS_PICSRULES_OUTOFMEMORY,(LPTSTR) szMessage,MAX_PATH);

                MessageBox(NULL,(LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);

                return(E_OUTOFMEMORY);
            }

            if(pPRPolicy->m_PRPolicyAttribute==PR_POLICY_ACCEPTBYURL)
            {                       
                pPRPolicy->m_pPRAcceptByURL=pPRByURL;
            }
            else
            {
                pPRPolicy->m_pPRRejectByURL=pPRByURL;
            }

            pPRByURLExpression=new PICSRulesByURLExpression;

            if(pPRByURLExpression==NULL)
            {
                char    szTitle[MAX_PATH],szMessage[MAX_PATH];

                //out of memory, so we init on the stack

                MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
                MLLoadString(IDS_PICSRULES_OUTOFMEMORY,(LPTSTR) szMessage,MAX_PATH);

                MessageBox(NULL,(LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);

                return(E_OUTOFMEMORY);
            }

            pPRByURLExpression->m_fInternetPattern=pPRByURLExpressionToCopy->m_fInternetPattern;
            pPRByURLExpression->m_bNonWild=pPRByURLExpressionToCopy->m_bNonWild;
            pPRByURLExpression->m_bSpecified=pPRByURLExpressionToCopy->m_bSpecified;
            pPRByURLExpression->m_etstrScheme.Set(pPRByURLExpressionToCopy->m_etstrScheme.Get());
            pPRByURLExpression->m_etstrUser.Set(pPRByURLExpressionToCopy->m_etstrUser.Get());
            pPRByURLExpression->m_etstrHost.Set(pPRByURLExpressionToCopy->m_etstrHost.Get());
            pPRByURLExpression->m_etstrPort.Set(pPRByURLExpressionToCopy->m_etstrPort.Get());
            pPRByURLExpression->m_etstrPath.Set(pPRByURLExpressionToCopy->m_etstrPath.Get());
            pPRByURLExpression->m_etstrURL.Set(pPRByURLExpressionToCopy->m_etstrURL.Get());

    
            pPRByURL->m_arrpPRByURL.Append(pPRByURLExpression);

            g_pApprovedPRRS->m_arrpPRPolicy.Append(pPRPolicy);
        }           
    }

    PICSRulesDeleteSystem(PICSRULES_APPROVEDSITES);
    PICSRulesSaveToRegistry(PICSRULES_APPROVEDSITES,&g_pApprovedPRRS);

    return(NOERROR);
}

BOOL CALLBACK PleaseDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
    
    char    szPassword[MAXPATHLEN];
    HRESULT hRet;
    
    static DWORD aIds[] = {
        IDC_STATIC1,    IDH_RATINGS_SUPERVISOR_PASSWORD,
        IDC_PASSWORD,    IDH_RATINGS_SUPERVISOR_PASSWORD,
        0,0
    };

    switch (uMsg) {
        case WM_INITDIALOG:
            InitPleaseDialog(hDlg, (PleaseDlgData *)lParam);
            if(GetDlgItem(hDlg,IDC_PASSWORD)!=NULL)
            {
                SendDlgItemMessage(hDlg,IDC_PASSWORD,EM_SETLIMITTEXT,(WPARAM) RATINGS_MAX_PASSWORD_LENGTH,(LPARAM) 0);
            }

            // set focus to password field
            SetFocus(GetDlgItem(hDlg, IDC_PASSWORD));
            return FALSE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDCANCEL:
                    EndPleaseDialog(hDlg, FALSE);
                    break;

                case IDOK:
                    GetDlgItemText(hDlg, IDC_PASSWORD, szPassword, sizeof(szPassword));
                    hRet = VerifySupervisorPassword(szPassword);

                    if (hRet == ResultFromScode(S_OK))
                    {
                        if(SendDlgItemMessage(hDlg,
                                              IDC_BLOCKING_PAGE,
                                              BM_GETCHECK,
                                              (WPARAM) 0,
                                              (LPARAM) 0)==BST_CHECKED)
                        {
                            HRESULT hRes;

                            hRes=AddToApprovedSites(PICSRULES_ALWAYS,PICSRULES_PAGE);

                            if(FAILED(hRes))
                            {
                                char    szTitle[MAX_PATH],szMessage[MAX_PATH];

                                MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
                                MLLoadString(IDS_APPROVED_CANTSAVE,(LPTSTR) szMessage,MAX_PATH);

                                MessageBox(NULL,(LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);

                                return(E_OUTOFMEMORY);
                            }
                        }
                        else if(SendDlgItemMessage(hDlg,
                                                   IDC_BLOCKING_SITE,
                                                   BM_GETCHECK,
                                                   (WPARAM) 0,
                                                   (LPARAM) 0)==BST_CHECKED)
                        {
                            HRESULT hRes;

                            hRes=AddToApprovedSites(PICSRULES_ALWAYS,PICSRULES_SITE);

                            if(FAILED(hRes))
                            {
                                char    szTitle[MAX_PATH],szMessage[MAX_PATH];

                                MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
                                MLLoadString(IDS_APPROVED_CANTSAVE,(LPTSTR) szMessage,MAX_PATH);

                                MessageBox(NULL,(LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);

                                return(E_OUTOFMEMORY);
                            }
                        }

                        EndPleaseDialog(hDlg, TRUE);
                    }
                    else {
                        MyMessageBox(hDlg, IDS_BADPASSWORD, IDS_GENERIC, MB_OK|MB_ICONERROR);
                        SetFocus(GetDlgItem(hDlg, IDC_PASSWORD));
                        SetDlgItemText(hDlg, IDC_PASSWORD, szNULL);
                    }
                    break;
            }
            break;

        case WM_HELP:
            SHWinHelpOnDemandWrap((HWND)((LPHELPINFO)lParam)->hItemHandle, ::szHelpFile,
                    HELP_WM_HELP, (DWORD_PTR)(LPSTR)aIds);
            break;

        case WM_CONTEXTMENU:
            SHWinHelpOnDemandWrap((HWND)wParam, ::szHelpFile, HELP_CONTEXTMENU,
                    (DWORD_PTR)(LPVOID)aIds);
            break;

        case WM_NEWDIALOG:
            InitPleaseDialog(hDlg, (PleaseDlgData *)lParam);
            break;
    }

    return FALSE;
}


HRESULT WINAPI RatingAccessDeniedDialog(HWND hDlg, LPCSTR pszUsername, LPCSTR pszContentDescription, LPVOID pRatingDetails)
{
    PleaseDlgData pdd;

    pdd.pszUsername = pszUsername;
    pdd.pPU = GetUserObject(pszUsername);
    if (pdd.pPU == NULL)
        return HRESULT_FROM_WIN32(ERROR_BAD_USERNAME);

    pdd.pszContentDescription = pszContentDescription;
    pdd.pLabelList = (CParsedLabelList *)pRatingDetails;
    pdd.hwndEC = NULL;
    pdd.dwFlags = 0;
    pdd.hwndDlg = NULL;
    pdd.hwndOwner = hDlg;
    pdd.cLabels = 0;

    UINT idDlg = pdd.pPU->fPleaseMom ? IDD_PLEASE : IDD_DENY;

    HRESULT hres;
    if (DialogBoxParam(MLGetHinst(),
                       MAKEINTRESOURCE(idDlg),
                       hDlg,
                       (DLGPROC) PleaseDialogProc,
                       (LPARAM)(LPSTR)&pdd))
    {
        hres = ResultFromScode(S_OK);
    }
    else
    {
        hres = ResultFromScode(S_FALSE);
    }

    for (UINT i=0; i<pdd.cLabels; i++) {
        delete pdd.apLabelStrings[i];
    }

    return hres;
}

HRESULT WINAPI RatingAccessDeniedDialog2(HWND hwndParent, LPCSTR pszUsername, LPVOID pRatingDetails)
{
    PleaseDlgData *ppdd = (PleaseDlgData *)GetProp(hwndParent, szRatingsProp);
    if (ppdd == NULL)
        return RatingAccessDeniedDialog(hwndParent, pszUsername, NULL, pRatingDetails);

    ppdd->pLabelList = (CParsedLabelList *)pRatingDetails;
    SendMessage(ppdd->hwndDlg, WM_NEWDIALOG, 0, (LPARAM)ppdd);

    MSG msg;
    while (!ppdd->dwFlags) {
        if (GetMessage(&msg, NULL, 0, 0) &&
            !IsDialogMessage(ppdd->hwndDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (ppdd->dwFlags & PDD_ALLOW) ? S_OK : S_FALSE;
}

HRESULT WINAPI RatingFreeDetails(LPVOID pRatingDetails)
{
    if (pRatingDetails)
    {
        FreeParsedLabelList((CParsedLabelList *)pRatingDetails);
    }

    return NOERROR;
}
