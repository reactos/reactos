/****************************************************************************\
 *
 *   MSLUDLG.C
 *
 *   Updated:   Ann McCurdy
 *   Updated:   Mark Hammond (t-markh) 8/98
 *   
\****************************************************************************/

/*INCLUDES--------------------------------------------------------------------*/
#include "msrating.h"
#include "ratings.h"
#include "mslubase.h"
#include "commctrl.h"
#include "comctrlp.h"
#include "commdlg.h"
#include "buffer.h"
#include "picsrule.h"
#include <shlwapip.h>
#include <shellapi.h>
#include <wininet.h>
#include <contxids.h>

#include <mluisupp.h>

extern array<PICSRulesRatingSystem*>    g_arrpPRRS;
extern PICSRulesRatingSystem *          g_pPRRS;
extern PICSRulesRatingSystem *          g_pApprovedPRRS;

extern HMODULE                          g_hURLMON,g_hWININET;

HIMAGELIST                              g_hImageList;
int                                     g_iAllowAlways,g_iAllowNever;

extern HANDLE g_HandleGlobalCounter,g_ApprovedSitesHandleGlobalCounter;
extern long   g_lGlobalCounterValue,g_lApprovedSitesGlobalCounterValue;

PICSRulesRatingSystem * g_pApprovedPRRSPreApply;
array<PICSRulesRatingSystem*> g_arrpPICSRulesPRRSPreApply;

//The FN_INTERNETCRACKURL type describes the URLMON function InternetCrackUrl
typedef BOOL (*FN_INTERNETCRACKURL)(LPCTSTR lpszUrl,DWORD dwUrlLength,DWORD dwFlags,LPURL_COMPONENTS lpUrlComponents);

BOOL CALLBACK TurnOffDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* BUGBUG - use NLS_STRs instead of MAXPATHLEN stack buffers (gregj 03/12/96) */

// useful macro for getting rid of leaks.
#define SAFEDELETE(ptr)             \
            if(ptr)                 \
            {                       \
                delete ptr;         \
                ptr = NULL;         \
            }                       

void MarkChanged(HWND hDlg)
{
    SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);
}

/*Helpers---------------------------------------------------------------------*/
BOOL InitTreeViewImageLists(HWND hwndTV); 
int     g_nKeys, g_nLock; //         indexes of the images 
DWORD g_dwNumSystems=0;

/*Property Sheet Class--------------------------------------------------------*/
class PropSheet{
    private:
    public:
        PROPSHEETPAGE   psPage;
        PROPSHEETHEADER psHeader;

        PropSheet();
        ~PropSheet();
        BOOL Init(HWND hwnd, HINSTANCE hinst, int nPages, char *szCaption, BOOL fApplyNow);
        int Run();
};

PropSheet::PropSheet(){
    memset(&psPage,   0,sizeof(psPage));
    memset(&psHeader, 0,sizeof(psHeader));
    psHeader.dwSize = sizeof(psHeader);
    psPage.dwSize   = sizeof(psPage);
}

PropSheet::~PropSheet()
{
    delete (LPSTR)psHeader.pszCaption;
    delete psHeader.phpage;
}

BOOL PropSheet::Init(HWND hwnd, HINSTANCE hinst, int nPages, char *szCaption, BOOL fApplyNow){
    char *p;
    
    psHeader.hwndParent = hwnd;
    psHeader.hInstance  = MLGetHinst();
    p = new char [strlenf(szCaption)+1];
    if (p == NULL)
        return FALSE;
    strcpyf(p, szCaption);
    psHeader.pszCaption = p;
    
    psHeader.phpage = new HPROPSHEETPAGE [nPages];
    if (psHeader.phpage == NULL) {
        delete p;
        psHeader.pszCaption = NULL;
        return FALSE;
    }

    if (!fApplyNow) psHeader.dwFlags |= PSH_NOAPPLYNOW;

    psPage.hInstance = MLGetHinst();

    return (psHeader.pszCaption != NULL);
}

// We can safely cast down to (int) because we don't use modeless
// property sheets.
int PropSheet::Run(){
    return (int)PropertySheet(&psHeader);
}

/*Pics Tree Dialog Stuff------------------------------------------------------*/
struct PRSD{
    HINSTANCE             hInst;
    PicsRatingSystemInfo *pPRSI;
    PicsUser             *pPU;
    UserRatingSystem     *pTempRatings;
    HWND                  hwndBitmapCategory;
    HWND                  hwndBitmapLabel;
    BOOL                  fNewProviders;
};

void SelectRatingSystemNode(HWND hDlg, PRSD *pPRSD, PicsCategory *pPC);

/* GetTempRatingList returns the dialog's temporary copy of the user's rating
 * system list.  If we don't have any such temporary copy yet, we make one.
 */
UserRatingSystem *GetTempRatingList(PRSD *pPRSD)
{
    if (pPRSD->pTempRatings == NULL)
        pPRSD->pTempRatings = DuplicateRatingSystemList(pPRSD->pPU->m_pRatingSystems);

    return pPRSD->pTempRatings;
}


UserRating *GetTempRating(PRSD *pPRSD, PicsCategory *pPC)
{
    UserRating *pRating = NULL;

    /* save the selected value into the temporary ratings list */
    UserRatingSystem *pURS = GetTempRatingList(pPRSD);
    LPSTR pszRatingService = pPC->pPRS->etstrRatingService.Get();
    if (pURS != NULL) {
        pURS = FindRatingSystem(pURS, pszRatingService);
    }

    if (pURS == NULL) {
        pURS = new UserRatingSystem;
        if (pURS == NULL)
            return NULL;

        pURS->SetName(pszRatingService);
        pURS->m_pNext = pPRSD->pTempRatings;
        pURS->m_pPRS = pPC->pPRS;
        pPRSD->pTempRatings = pURS;
    }

    LPSTR pszRatingName = pPC->etstrTransmitAs.Get();

    pRating = pURS->FindRating(pszRatingName);
    if (pRating == NULL) {
        pRating = new UserRating;
        if (pRating == NULL)
            return NULL;

        pRating->SetName(pszRatingName);
        pRating->m_pPC = pPC;
        if (!pPC->etnMin.fIsInit() || (pPC->etfLabelled.fIsInit() && pPC->etfLabelled.Get()))
            pRating->m_nValue = 0;
        else
            pRating->m_nValue = pPC->etnMin.Get();

        pURS->AddRating(pRating);
    }

    return pRating;
}


void SelectRatingSystemInfo(HWND hDlg, PRSD *pPRSD, PicsRatingSystem *pPRS);

void DeleteBitmapWindow(HWND *phwnd){
    if (*phwnd){
        DeleteObject( (HGDIOBJ) SendMessage(*phwnd, STM_GETIMAGE, IMAGE_BITMAP, 0));
        DestroyWindow(*phwnd);
        *phwnd = 0;
    }
}



enum TreeNodeEnum{tneGeneral, tneAccessList, tneRatingSystemRoot, tneRatingSystemInfo, tneRatingSystemNode, tneNone};

struct TreeNode{
    TreeNodeEnum  tne;
    void         *pData;

    TreeNode(){}
    TreeNode(TreeNodeEnum tneInit, void* pDataInit){tne=tneInit;pData=pDataInit;}
};


void ShowHideWindow(HWND hCtrl, BOOL fEnable)
{
    EnableWindow(hCtrl, fEnable);
    ShowWindow(hCtrl, fEnable ? SW_SHOW : SW_HIDE);
}


void ControlsShow(HWND hDlg, PRSD *pPRSD, TreeNodeEnum tne)
{
    BOOL fEnable;

    /*Bitmap placeholders never need to be seen*/
    ShowHideWindow(GetDlgItem(hDlg, IDC_PT_T_BITMAP_CATEGORY), FALSE);
    ShowHideWindow(GetDlgItem(hDlg, IDC_PT_T_BITMAP_LABEL),    FALSE);

    /*Kill old graphic windows*/
    DeleteBitmapWindow(&pPRSD->hwndBitmapCategory);
    DeleteBitmapWindow(&pPRSD->hwndBitmapLabel);

    /*RatingSystemNode Controls*/
    fEnable = (tne == tneRatingSystemNode);

    ShowHideWindow(GetDlgItem(hDlg, IDC_PT_T_RSN_SDESC), fEnable);
    ShowHideWindow(GetDlgItem(hDlg, IDC_PT_TB_SELECT),   fEnable);
    ShowHideWindow(GetDlgItem(hDlg, IDC_RATING_LABEL),   fEnable);

    /*RatingSystemInfo Controls*/
    fEnable = (tne==tneRatingSystemInfo || tne==tneRatingSystemNode);

    ShowHideWindow(GetDlgItem(hDlg, IDC_PT_T_RSN_LDESC), fEnable);
    ShowHideWindow(GetDlgItem(hDlg, IDC_DETAILSBUTTON),  fEnable);
}

TreeNode* TreeView_GetSelectionLParam(HWND hwndTree){
    TV_ITEM tv;

    tv.mask  = TVIF_HANDLE | TVIF_PARAM;
    tv.hItem = TreeView_GetSelection(hwndTree);
    if (SendMessage(hwndTree, TVM_GETITEM, 0, (LPARAM) &tv)) return (TreeNode*) tv.lParam;
    else return 0;
}

HTREEITEM AddOneItem(HWND hwndTree, HTREEITEM hParent, LPSTR szText, HTREEITEM hInsAfter, LPARAM lpData, int iImage){
    HTREEITEM hItem;
    TV_ITEM tvI;
    TV_INSERTSTRUCT tvIns;

    // The .pszText is filled in.
    tvI.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvI.iSelectedImage = iImage;
    tvI.iImage = iImage;
    tvI.pszText = szText;
    tvI.cchTextMax = strlenf(szText);
    tvI.lParam = lpData;

    tvIns.item = tvI;
    tvIns.hInsertAfter = hInsAfter;
    tvIns.hParent = hParent;

    // Insert the item into the tree.
    hItem = (HTREEITEM)SendMessage(hwndTree, TVM_INSERTITEM, 0, (LPARAM)(LPTV_INSERTSTRUCT)&tvIns);

    return (hItem);
}

void AddCategory(PicsCategory *pPC, HWND hwndTree, HTREEITEM hParent){
    int        z;
    char      *pc;
    TreeNode  *pTN;

    /*if we have a real name, us it, else use transmission name*/
    if (pPC->etstrName.fIsInit()){
        pc = pPC->etstrName.Get();
    }
    else if (pPC->etstrDesc.fIsInit()){
        pc = pPC->etstrDesc.Get();
    }
    else{
        pc = (char*) pPC->etstrTransmitAs.Get();
    }

    /*Category Tab*/
    pTN  = new TreeNode(tneRatingSystemNode, pPC);
    ASSERT(pTN);    // BUGBUG

    /*insert self*/
    hParent = AddOneItem(hwndTree, hParent, pc, TVI_SORT, (LPARAM) pTN, g_nKeys);

    /*insert children*/
    int cChildren = pPC->arrpPC.Length();

    if (cChildren > 0) {
        for (z = 0; z < cChildren; ++z)
            AddCategory(pPC->arrpPC[z], hwndTree, hParent);
        TreeView_Expand(hwndTree, hParent, TVE_EXPAND);
    }
}


BOOL InstallDefaultProvider(HWND hDlg, PRSD *pPRSD)
{
    NLS_STR nlsFilename(MAXPATHLEN);
    BOOL fRet = FALSE;

    if (nlsFilename.QueryError() != ERROR_SUCCESS)
        return fRet;

    GetSystemDirectory(nlsFilename.Party(), nlsFilename.QueryAllocSize());
    nlsFilename.DonePartying();
    LPSTR pszBackslash = ::strrchrf(nlsFilename.QueryPch(), '\\');
    if (pszBackslash == NULL || *(pszBackslash+1) != '\0')
        nlsFilename.strcat(szBACKSLASH);
    nlsFilename.strcat(szDEFAULTRATFILE);

    PicsRatingSystem *pPRS;
    HRESULT hres = LoadRatingSystem(nlsFilename.QueryPch(), &pPRS);
    if (pPRS != NULL) {
        pPRSD->pPRSI->arrpPRS.Append(pPRS);
        fRet = TRUE;
    }

    pPRSD->pPRSI->fRatingInstalled = fRet;

    CheckUserSettings(pPRS);    /* give user default settings for all categories */

    return fRet;
}


void PicsDlgInit(HWND hDlg, PRSD *pPRSD){
    HTREEITEM  hTree;
    TreeNode  *pTN;
    HWND       hwndTree;
    int        x,z;

    hwndTree = GetDlgItem(hDlg, IDC_PT_TREE);

    /* Note, if there are installed providers but they all failed, there
     * will be dummy entries for them in the array.  So we will not attempt
     * to install RSACi automatically unless there really are no providers
     * installed at all.
     */
    if (!pPRSD->pPRSI->arrpPRS.Length())
    {
        // There are no providers.
        if (!InstallDefaultProvider(hDlg, pPRSD)) {
            MyMessageBox(hDlg, IDS_INSTALL_INFO, IDS_GENERIC, MB_OK);
            ControlsShow(hDlg, pPRSD, tneNone);
            return;
        }
    }
    /*make the tree listing*/
    /*Individual Rating Systems*/
    InitTreeViewImageLists(hwndTree);

    BOOL fAnyInvalid = FALSE;
    BOOL fAnyValid = FALSE;
    for (z = 0; z < pPRSD->pPRSI->arrpPRS.Length(); ++z)
    {
        PicsRatingSystem *pPRS = pPRSD->pPRSI->arrpPRS[z];

        if (!(pPRS->dwFlags & PRS_ISVALID)) {
            fAnyInvalid = TRUE;
            continue;
        }
        fAnyValid = TRUE;

        pTN  = new TreeNode(tneRatingSystemInfo, pPRS);
        ASSERT(pTN);    // BUGBUG
        hTree = AddOneItem(hwndTree, NULL, (char*) pPRS->etstrName.Get(), TVI_SORT, (LPARAM) pTN, g_nLock);
        for (x = 0; x < pPRS->arrpPC.Length(); ++x)
        {
            AddCategory(pPRS->arrpPC[x], hwndTree, hTree);
        }
        TreeView_Expand(hwndTree, hTree, TVE_EXPAND);
    }

    if (fAnyInvalid) {
        MyMessageBox(hDlg, IDS_INVALID_PROVIDERS, IDS_GENERIC, MB_OK | MB_ICONWARNING);
    }

    if (fAnyValid) {
        HTREEITEM hTreeItem;

        hTreeItem=TreeView_GetNextItem(hwndTree, TreeView_GetRoot(hwndTree),TVGN_CHILD);

        if(hTreeItem!=NULL)
        {
            TreeView_SelectItem(hwndTree, hTreeItem);       
            pTN   = TreeView_GetSelectionLParam(GetDlgItem(hDlg, IDC_PT_TREE));
            ControlsShow(hDlg, pPRSD, pTN->tne);
            switch(pTN->tne){
                case tneRatingSystemInfo:
                    SelectRatingSystemInfo(hDlg, pPRSD, (PicsRatingSystem*) pTN->pData);
                    break;
                case tneRatingSystemNode:
                    SelectRatingSystemNode(hDlg, pPRSD, (PicsCategory*) pTN->pData);
                    break;
            }
        }
        else
        {
            TreeView_SelectItem(hwndTree, TreeView_GetRoot(hwndTree));
            pTN   = TreeView_GetSelectionLParam(GetDlgItem(hDlg, IDC_PT_TREE));
            ControlsShow(hDlg, pPRSD, tneRatingSystemInfo);
            SelectRatingSystemInfo(hDlg, pPRSD, (PicsRatingSystem*) pTN->pData);
        }
    }
    else
        ControlsShow(hDlg, pPRSD, tneNone);
}


void KillTree(HWND hwnd, HTREEITEM hTree)
{
    while (hTree != NULL) {
        /* If this node has any items under it, delete them as well. */
        HTREEITEM hChild = TreeView_GetChild(hwnd, hTree);
        if (hChild != NULL)
            KillTree(hwnd, hChild);

        HTREEITEM hNext = TreeView_GetNextSibling(hwnd, hTree);

        TreeView_SelectItem(hwnd, hTree);
        delete TreeView_GetSelectionLParam(hwnd);
        TreeView_DeleteItem(hwnd, hTree);
        hTree = hNext;
    }
}


void PicsDlgUninit(HWND hDlg, PRSD *pPRSD)
{
    HWND hwnd;

    hwnd = GetDlgItem(hDlg, IDC_PT_TREE);
    KillTree(hwnd, TreeView_GetRoot(hwnd));

    /* If we have a temporary copy of the user's ratings list, destroy it. */
    if (pPRSD->pTempRatings != NULL) {
        DestroyRatingSystemList(pPRSD->pTempRatings);
        pPRSD->pTempRatings = NULL;
    }
    ControlsShow(hDlg, pPRSD, tneNone);
}


void PicsDlgSave(HWND hDlg, PRSD *pPRSD)
{
    /* To save changes, throw away the user's ratings list and steal the
     * temporary copy we're using in the dialog.  As an optimization, we
     * don't copy it here, because in the case of OK, we'd just be destroying
     * the original immediately after this.  If the user hit Apply, we'll
     * re-clone a new temp ratings list for the dialog's purpose the next
     * time we need one.
     *
     * If there is no temporary copy, then PicsDlgSave is a nop.
     */
    if (pPRSD->pTempRatings != NULL) {
        DestroyRatingSystemList(pPRSD->pPU->m_pRatingSystems);
        pPRSD->pPU->m_pRatingSystems = pPRSD->pTempRatings;
        pPRSD->pTempRatings = NULL;
    }
}

POINT BitmapWindowCoord(HWND hDlg, int nID){
    POINT pt;
    RECT  rD, rI;

    pt.x = GetWindowRect(GetDlgItem(hDlg, nID), &rI);
    pt.y = GetWindowRect(hDlg, &rD);
    pt.x = rI.left - rD.left;
    pt.y = rI.top  - rD.top;
    return pt;
}

#ifdef RATING_LOAD_GRAPHICS /* BUGBUG - loading icon out of msrating.dll?  completely bogus. */
void LoadGraphic(HWND hDlg, PRSD *pPRSD, HWND *phwnd, char *pIcon, POINT pt){
#if 0
    HBITMAP hBitmap;
    hBitmap = pIcon ? LoadBitmap(pPRSD->hInst, MAKEINTRESOURCE(atoi(pIcon))) : NULL;
    if (hBitmap){
        *phwnd = CreateWindow("Static",NULL,SS_BITMAP|WS_CHILD, pt.x, pt.y,0,0,hDlg, NULL,NULL,0);
        ShowWindow(*phwnd, SW_SHOW);
        DeleteObject( (HGDIOBJ) SendMessage(*phwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hBitmap));
    }
#else
    HICON hIcon;
    int i;

    MyAtoi(pIcon, &i);
    
    hIcon = pIcon ? LoadIcon(pPRSD->hInst, MAKEINTRESOURCE(i)) : NULL;
    if (hIcon){
        *phwnd = CreateWindow("Static",NULL,SS_ICON|WS_CHILD, pt.x, pt.y,0,0,hDlg, NULL,NULL,0);
        ShowWindow(*phwnd, SW_SHOW);
        DeleteObject( (HGDIOBJ) SendMessage(*phwnd, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon));
    }
#endif
}
#endif  /* RATING_LOAD_GRAPHICS */


PicsEnum* PosToEnum(PicsCategory *pPC, LPARAM lPos)
{
    int z, diff=-1, temp;
    PicsEnum *pPE=NULL;

    for (z=0;z<pPC->arrpPE.Length();++z){
        temp = (int) (lPos-pPC->arrpPE[z]->etnValue.Get());
        if (temp>=0){
            if (temp<diff || diff==-1){
                diff = temp;
                pPE  = pPC->arrpPE[z];
            }
        }
    }

    return pPE;
}

void NewTrackbarPosition(HWND hDlg, PRSD *pPRSD)
{
    signed long   lPos;
    TreeNode     *pTN;
    PicsEnum     *pPE;
    PicsCategory *pPC;                    

    DeleteBitmapWindow(&pPRSD->hwndBitmapLabel);

    pTN = TreeView_GetSelectionLParam(GetDlgItem(hDlg, IDC_PT_TREE));
    if (pTN == NULL)
        return;

    pPC = (PicsCategory*) pTN->pData;
    BOOL fLabelled = pPC->etfLabelled.fIsInit() && pPC->etfLabelled.Get();

    lPos = (long) SendMessage(GetDlgItem(hDlg, IDC_PT_TB_SELECT), TBM_GETPOS, 0, 0);
    pPE  = PosToEnum(pPC, fLabelled ? (LPARAM) pPC->arrpPE[lPos]->etnValue.Get() : lPos);
    if (pPE){
        //if (pPE->etstrIcon.fIsInit()){
        //    LoadGraphic(hDlg, pPRSD, &pPRSD->hwndBitmapLabel, pPE->etstrIcon.Get(),BitmapWindowCoord(hDlg, IDC_PT_T_BITMAP_LABEL));
        //}
        SetWindowText(GetDlgItem(hDlg, IDC_PT_T_RSN_SDESC), pPE->etstrName.Get());
        SetWindowText(GetDlgItem(hDlg, IDC_PT_T_RSN_LDESC), 
                      pPE->etstrDesc.fIsInit() ? pPE->etstrDesc.Get() : szNULL);

    }
    else{
        char pszBuf[MAXPATHLEN];
        char rgBuf[sizeof(pszBuf) + 12];    // big enough to insert a number

        MLLoadStringA(IDS_VALUE, pszBuf, sizeof(pszBuf));
        
        wsprintf(rgBuf, pszBuf, lPos);
        SetWindowText(GetDlgItem(hDlg, IDC_PT_T_RSN_SDESC), rgBuf);    
        SetWindowText(GetDlgItem(hDlg, IDC_PT_T_RSN_LDESC),
                      pPC->etstrDesc.fIsInit() ? pPC->etstrDesc.Get() : szNULL);
    }

    /* save the selected value into the temporary ratings list */
    UserRating *pRating = GetTempRating(pPRSD, pPC);
    if (pRating != NULL) {
        pRating->m_nValue = (int) (fLabelled ? pPC->arrpPE[lPos]->etnValue.Get() : lPos);
    }
}

void SelectRatingSystemNode(HWND hDlg, PRSD *pPRSD, PicsCategory *pPC)
{
    HWND      hwnd;
    BOOL      fLabelOnly;
    LPARAM    lValue;
    int       z;

#ifdef RATING_LOAD_GRAPHICS
    /*Category Icon*/
    if (pPC->etstrIcon.fIsInit()) LoadGraphic(hDlg, pPRSD, &pPRSD->hwndBitmapCategory, pPC->etstrIcon.Get(), BitmapWindowCoord(hDlg, IDC_PT_T_BITMAP_LABEL));
#endif

    /*Setup Trackbar*/
    if ((pPC->etnMax.fIsInit() && P_INFINITY==pPC->etnMax.Get())
        ||
        (pPC->etnMin.fIsInit() && N_INFINITY==pPC->etnMin.Get())
        ||
        (!(pPC->etnMin.fIsInit() && pPC->etnMax.fIsInit()))
    ){
        ShowHideWindow(GetDlgItem(hDlg, IDC_PT_T_RSN_SDESC), FALSE);
        ShowHideWindow(GetDlgItem(hDlg, IDC_PT_T_RSN_LDESC), FALSE);
        ShowHideWindow(GetDlgItem(hDlg, IDC_PT_TB_SELECT),   FALSE);
    }
    else{
        hwnd = GetDlgItem(hDlg, IDC_PT_TB_SELECT);
        SendMessage(hwnd, TBM_CLEARTICS, TRUE, 0);

        fLabelOnly = pPC->etfLabelled.fIsInit() && pPC->etfLabelled.Get();            
        /*Ranges*/
        if (pPC->etnMax.fIsInit()){
            lValue = (LPARAM) ( fLabelOnly ? pPC->arrpPE.Length()-1 : pPC->etnMax.Get() );
            SendMessage(hwnd, TBM_SETRANGEMAX, TRUE, lValue);
        }
        if (pPC->etnMin.fIsInit()){
            lValue = (LPARAM) ( fLabelOnly ? 0 : pPC->etnMin.Get() );
            SendMessage(hwnd, TBM_SETRANGEMIN, TRUE, lValue);
        }

        /*Ticks*/
        for (z=0;z<pPC->arrpPE.Length();++z){
            lValue = (LPARAM) ( fLabelOnly ? z : pPC->arrpPE[z]->etnValue.Get());
            SendMessage(hwnd, TBM_SETTIC, 0, lValue);
        }

        /*Initial Position of trackbar*/
        UserRating *pRating = GetTempRating(pPRSD, pPC);

        if (pRating != NULL) {
            if (fLabelOnly){
                for (z=0;z<pPC->arrpPE.Length();++z) if (pPC->arrpPE[z]->etnValue.Get() == pRating->m_nValue){
                    lValue=z;
                    break;
                }
            }
            else lValue = (LPARAM) pRating->m_nValue;
        }
        else{
            lValue = (LPARAM) ( fLabelOnly ? 0 : pPC->etnMin.Get());
        }
        SendMessage(hwnd, TBM_SETPOS, TRUE, lValue);
        ASSERT(lValue == SendMessage(hwnd, TBM_GETPOS, 0,0));
        NewTrackbarPosition(hDlg, pPRSD);
    }
}

void SelectRatingSystemInfo(HWND hDlg, PRSD *pPRSD, PicsRatingSystem *pPRS){
    SetWindowText(GetDlgItem(hDlg, IDC_PT_T_RSN_LDESC), pPRS->etstrDesc.Get());
#ifdef RATING_LOAD_GRAPHICS
    if (pPRS->etstrIcon.fIsInit()) LoadGraphic(hDlg, pPRSD, &pPRSD->hwndBitmapCategory, pPRS->etstrIcon.Get(), BitmapWindowCoord(hDlg, IDC_PT_T_BITMAP_LABEL));
#endif
}
// InitTreeViewImageLists - creates an image list, adds three bitmaps to 
// it, and associates the image list with a tree-view control. 
// Returns TRUE if successful or FALSE otherwise. 
// hwndTV - handle of the tree-view control 
//
#define NUM_BITMAPS  2
#define CX_BITMAP   16
#define CY_BITMAP   16
BOOL InitTreeViewImageLists(HWND hwndTV) 
{ 
    HIMAGELIST himl,oldHiml;  // handle of image list 
    HBITMAP hbmp;     // handle of bitmap 
 
    // Create the image list. 
    if ((himl = ImageList_Create(CX_BITMAP, CY_BITMAP, 
            FALSE, NUM_BITMAPS, 0)) == NULL) 
        return FALSE; 
    // Add the open file, closed file, and document bitmaps. 
    hbmp=(HBITMAP) LoadImage(hInstance,
                             MAKEINTRESOURCE(IDB_KEYS),
                             IMAGE_BITMAP,
                             0,
                             0,
                             LR_LOADTRANSPARENT|LR_DEFAULTCOLOR|LR_CREATEDIBSECTION);
    g_nKeys = ImageList_Add(himl, hbmp, (HBITMAP) NULL); 
    DeleteObject(hbmp); 

    hbmp=(HBITMAP) LoadImage(hInstance,
                             MAKEINTRESOURCE(IDB_LOCK),
                             IMAGE_BITMAP,
                             0,
                             0,
                             LR_LOADTRANSPARENT|LR_DEFAULTCOLOR|LR_CREATEDIBSECTION);
    g_nLock = ImageList_Add(himl, hbmp, (HBITMAP) NULL); 
    DeleteObject(hbmp); 

    // Fail if not all of the images were added. 
    if (ImageList_GetImageCount(himl) < NUM_BITMAPS) 
        return FALSE; 
 
    // Associate the image list with the tree-view control. 
    oldHiml=TreeView_SetImageList(hwndTV, himl, TVSIL_NORMAL); 
 
    if(oldHiml!=NULL)
    {
        ImageList_Destroy(oldHiml);
    }
    
    return TRUE; 
} 


typedef HINSTANCE (APIENTRY *PFNSHELLEXECUTE)(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);

void LaunchRatingSystemSite(HWND hDlg, PRSD *pPRSD)
{
    TreeNode *pTN = TreeView_GetSelectionLParam(GetDlgItem(hDlg, IDC_PT_TREE));
    if (pTN == NULL)
        return;

    PicsRatingSystem *pPRS = NULL;

    if (pTN->tne == tneRatingSystemInfo)
        pPRS = (PicsRatingSystem *)pTN->pData;
    else if (pTN->tne == tneRatingSystemNode) {
        if ((PicsCategory *)pTN->pData != NULL)
            pPRS = ((PicsCategory *)pTN->pData)->pPRS;
    }

    if (pPRS != NULL) {
        BOOL fSuccess = FALSE;
        HINSTANCE hShell32 = ::LoadLibrary(::szShell32);
        if (hShell32 != NULL) {
            PFNSHELLEXECUTE pfnShellExecute = (PFNSHELLEXECUTE)::GetProcAddress(hShell32, ::szShellExecute);
            if (pfnShellExecute != NULL) {
                fSuccess = (*pfnShellExecute)(hDlg, NULL, pPRS->etstrRatingService.Get(),
                                              NULL, NULL, SW_SHOW) != NULL;
            }
            ::FreeLibrary(hShell32);
        }
        if (!fSuccess) {
            NLS_STR nlsMessage(MAX_RES_STR_LEN);
            NLS_STR nlsTemp(STR_OWNERALLOC, pPRS->etstrRatingSystem.Get());
            const NLS_STR *apnls[] = { &nlsTemp, NULL };
            nlsMessage.LoadString(IDS_CANT_LAUNCH, apnls);
            MyMessageBox(hDlg, nlsMessage.QueryPch(), IDS_GENERIC, MB_OK | MB_ICONSTOP);
        }
    }
}
 
BOOL CALLBACK PicsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    PRSD *pPRSD = NULL;
    HWND  hwnd  = NULL;

    static DWORD aIds[] = {
        IDC_STATIC1,        IDH_RATINGS_CATEGORY_LABEL,
        IDC_PT_TREE,        IDH_RATINGS_CATEGORY_LIST,
        IDC_RATING_LABEL,   IDH_RATINGS_RATING_LABEL,
        IDC_PT_TB_SELECT,   IDH_RATINGS_RATING_LABEL,
        IDC_PT_T_RSN_SDESC, IDH_RATINGS_RATING_TEXT,
        IDC_STATIC2,        IDH_RATINGS_DESCRIPTION_LABEL,
        IDC_PT_T_RSN_LDESC, IDH_RATINGS_DESCRIPTION_TEXT,
        IDC_STATIC3,        IDH_RATINGS_VIEW_PROVIDER_PAGE,
        IDC_DETAILSBUTTON,  IDH_RATINGS_VIEW_PROVIDER_PAGE,
        0,0
    };

    switch (uMsg) {
        /*Set the initial state*/
        case WM_SYSCOLORCHANGE:
        {
            TV_ITEM  tvm;
            TreeNode *pTN;

            InitTreeViewImageLists(GetDlgItem(hDlg,IDC_PT_TREE));

            //force the trackbar to redraw its background with the new color
            pPRSD = (PRSD *) ((PROPSHEETPAGE*)GetWindowLongPtr (hDlg, DWLP_USER))->lParam;

            ZeroMemory(&tvm,sizeof(tvm));

            tvm.hItem=TreeView_GetSelection(GetDlgItem(hDlg,IDC_PT_TREE));
            tvm.mask=TVIF_PARAM;

            TreeView_GetItem(GetDlgItem(hDlg,IDC_PT_TREE),&tvm);

            pTN=(TreeNode *) tvm.lParam;

            ControlsShow(hDlg, pPRSD, pTN->tne);
            
            switch(pTN->tne)
            {
                case tneRatingSystemInfo:
                {
                    SelectRatingSystemInfo(hDlg, pPRSD, (PicsRatingSystem*) pTN->pData);
                    break;
                }
                case tneRatingSystemNode:
                {
                    SelectRatingSystemNode(hDlg, pPRSD, (PicsCategory*) pTN->pData);
                    break;
                }
            }

            break;
        }
        case WM_INITDIALOG:
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            pPRSD = (PRSD *) ((PROPSHEETPAGE*)lParam)->lParam;
            PicsDlgInit(hDlg, pPRSD);
            return TRUE;

        case WM_HSCROLL:
        case WM_VSCROLL:
            pPRSD = (PRSD *) ((PROPSHEETPAGE*)GetWindowLongPtr(hDlg, DWLP_USER))->lParam;
            switch (LOWORD(wParam)){
                case TB_THUMBTRACK:
                case TB_BOTTOM:
                case TB_ENDTRACK:
                case TB_LINEDOWN:
                case TB_LINEUP:
                case TB_PAGEDOWN:
                case TB_PAGEUP:
                case TB_THUMBPOSITION:
                case TB_TOP:
                    NewTrackbarPosition(hDlg, pPRSD);
                    MarkChanged(hDlg);
                    break;
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            case IDC_DETAILSBUTTON:
                LaunchRatingSystemSite(hDlg, pPRSD);
                break;
            }
            return TRUE;

        case WM_NOTIFY: {
            NMHDR *lpnm = (NMHDR *) lParam;
            switch (lpnm->code) {
                /*save us*/
                case PSN_SETACTIVE:
                    pPRSD = (PRSD *) ((PROPSHEETPAGE*)GetWindowLongPtr(hDlg, DWLP_USER))->lParam;
                    if (pPRSD->fNewProviders)
                    {
                        //Means that user changed list of provider files
                        hwnd = GetDlgItem(hDlg, IDC_PT_TREE);
                        KillTree(hwnd , TreeView_GetRoot(hwnd));
                        pPRSD->fNewProviders = FALSE;
                        PicsDlgInit(hDlg, pPRSD);
                    }
                    break;
                    
                case PSN_APPLY:
                    /*do apply stuff*/
                    pPRSD = (PRSD *) ((PROPSHEETPAGE*)GetWindowLongPtr (hDlg, DWLP_USER))->lParam;
                    PicsDlgSave(hDlg, pPRSD);
                    if (!((PSHNOTIFY*) lpnm)->lParam) break;
                    /*intentional fallthrough*/
                case PSN_RESET:
                    // Do this if hit OK or Cancel, not Apply
                    pPRSD = (PRSD *) ((PROPSHEETPAGE*)GetWindowLongPtr(hDlg, DWLP_USER))->lParam;
                    SendMessage(hDlg,WM_SETREDRAW, FALSE,0L);
                    PicsDlgUninit(hDlg, pPRSD);
                    SendMessage(hDlg,WM_SETREDRAW, TRUE,0L);
                    return TRUE;
                   
                case TVN_ITEMEXPANDING:{
                    NM_TREEVIEW *pNMT = (NM_TREEVIEW *) lParam;

                    if (pNMT->action == TVE_COLLAPSE)
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                        return TRUE; //Suppress expanding tree.
                    }
                    break;
                }    
                    
                case TVN_SELCHANGED:
                {
                    NM_TREEVIEW *pNMT = (NM_TREEVIEW *) lParam;
                    TreeNode    *pTN  = ((TreeNode*) pNMT->itemNew.lParam);
                    pPRSD = (PRSD *) ((PROPSHEETPAGE*)GetWindowLongPtr (hDlg, DWLP_USER))->lParam;
                    if (pPRSD->fNewProviders)
                        return TRUE;    /* tree is being cleaned up, ignore sel changes */

                    ControlsShow(hDlg, pPRSD, pTN->tne);
                    switch(pTN->tne){
                        case tneRatingSystemInfo:
                            SelectRatingSystemInfo(hDlg, pPRSD, (PicsRatingSystem*) pTN->pData);
                            break;
                        case tneRatingSystemNode:
                            SelectRatingSystemNode(hDlg, pPRSD, (PicsCategory*) pTN->pData);
                            break;
                    }
                    return TRUE;
                }
            }
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
    }
    return FALSE;
}

UINT FillBureauList(HWND hDlg, PicsRatingSystemInfo *pPRSI)
{
    int i;
    INT_PTR z;
    BOOL fHaveBureau = pPRSI->etstrRatingBureau.fIsInit();
    BOOL fSelectedOne = FALSE;

    HWND hwndCombo = GetDlgItem(hDlg, IDC_3RD_COMBO);
    LPCSTR pszCurrentBureau;
    BOOL fListBureau = FALSE;

    NLS_STR nlsURL;

    /* Save current selection if at all possible.  If an item in the listbox
     * is selected, get its item data, which is the rating bureau string.
     *
     * We remember if it was an item from the list that was selected before;
     * that indicates that the bureau belongs to a rating system, and if we
     * don't find it in the list after reinitializing, we know that rating
     * system has been removed and the bureau is probably useless now.
     */
    z = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
    if (z != CB_ERR) {
        pszCurrentBureau = (LPCSTR)SendMessage(hwndCombo, CB_GETITEMDATA, z, 0);
        fListBureau = TRUE;
    }
    else {
        /* No item selected.  If there is text in the edit control, preserve
         * it;  otherwise, try to select the current rating bureau if any.
         */
        UINT cch = GetWindowTextLength(hwndCombo);
        if (cch > 0 && nlsURL.realloc(cch + 1)) {
            GetWindowText(hwndCombo, nlsURL.Party(), nlsURL.QueryAllocSize());
            nlsURL.DonePartying();
            pszCurrentBureau = nlsURL.QueryPch();
        }
        else
            pszCurrentBureau = fHaveBureau ? pPRSI->etstrRatingBureau.Get() : szNULL;
    }

    SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);
   
    for (i = 0; i < pPRSI->arrpPRS.Length(); ++i)
    {
        PicsRatingSystem *prs = pPRSI->arrpPRS[i];
        if (prs->etstrRatingBureau.fIsInit())
        {
            z = SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)prs->etstrName.Get());
            SendMessage(hwndCombo, CB_SETITEMDATA, z,
                        (LPARAM)prs->etstrRatingBureau.Get());

            if (!fSelectedOne)
            {
                if (!strcmpf(pszCurrentBureau, prs->etstrRatingBureau.Get())) 
                {
                    SendMessage(hwndCombo, CB_SETCURSEL, z,0);
                    fSelectedOne = TRUE;
                }           
            }
        }
    }

    NLS_STR nlsNone(MAX_RES_STR_LEN);
    if (nlsNone.LoadString(IDS_NO_BUREAU) != ERROR_SUCCESS)
        nlsNone = NULL;

    z = SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)nlsNone.QueryPch());

    SendMessage(hwndCombo, CB_SETITEMDATA, z, 0L);
       
    if (!fSelectedOne)
    {
        if (!fListBureau && *pszCurrentBureau != '\0')
            SetWindowText(hwndCombo, pszCurrentBureau);
        else
            SendMessage(hwndCombo, CB_SETCURSEL, z, 0L);
    }
       
    return 0;
}


BOOL CALLBACK GeneralDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PRSD *pPRSD;
    
    static DWORD aIds[] = {
        IDC_STATIC1,            IDH_RATINGS_RATING_SYSTEM_TEXT,
        IDC_STATIC2,            IDH_RATINGS_RATING_SYSTEM_TEXT,
        IDC_STATIC3,            IDH_RATINGS_RATING_SYSTEM_TEXT,
        IDC_FINDRATINGS,        IDH_FIND_RATING_SYSTEM_BUTTON,
        IDC_PROVIDER,           IDH_RATINGS_RATING_SYSTEM_BUTTON,
        IDC_UNRATED,            IDH_RATINGS_UNRATED_CHECKBOX,
        IDC_PLEASE_MOMMY,       IDH_RATINGS_OVERRIDE_CHECKBOX,
        IDC_STATIC1,            IDH_RATINGS_CHANGE_PASSWORD_BUTTON,
        IDC_STATIC2,            IDH_RATINGS_CHANGE_PASSWORD_BUTTON,
        IDC_STATIC3,            IDH_RATINGS_CHANGE_PASSWORD_BUTTON,
        IDC_CHANGE_PASSWORD,    IDH_RATINGS_CHANGE_PASSWORD_BUTTON,
        0,0
    };

    switch (uMsg) {
        case WM_INITDIALOG:
            SetWindowLongPtr (hDlg, DWLP_USER, lParam);
            pPRSD = (PRSD *) ((PROPSHEETPAGE*)lParam)->lParam;

            if (pPRSD->pPU != NULL) {
                CheckDlgButton(hDlg, IDC_UNRATED, pPRSD->pPU->fAllowUnknowns?BST_CHECKED:BST_UNCHECKED);
                CheckDlgButton(hDlg, IDC_PLEASE_MOMMY, pPRSD->pPU->fPleaseMom?BST_CHECKED:BST_UNCHECKED);
#ifdef NASH
                CheckDlgButton(hDlg, IDC_MASTER_USER,  pPRSD->pPU->etfMaster.Get()?BST_CHECKED:BST_UNCHECKED);
                CheckDlgButton(hDlg, IDC_CONTROL_PANEL,  pPRSD->pPU->etfControlPanel.Get()?BST_CHECKED:BST_UNCHECKED);
                CheckDlgButton(hDlg, IDC_NEW_APPS, pPRSD->pPU->etfNewApps.Get()?BST_CHECKED:BST_UNCHECKED);
#endif
            }

            PostMessage(hDlg,WM_USER,(WPARAM) 0,(LPARAM) 0);

            break;

        case WM_USER:
            if (gPRSI->lpszFileName!=NULL)
            {
                SetFocus(GetDlgItem(hDlg,IDC_PROVIDER));

                DoProviderDialog(hDlg,gPRSI);
                gPRSI->lpszFileName=NULL;
            }

            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                /*edit controls/check boxes.  User updated these, highlight apply button*/
                case IDC_PROVIDER:
                    pPRSD = (PRSD *) ((PROPSHEETPAGE*)GetWindowLongPtr (hDlg, DWLP_USER))->lParam;
                    if (DoProviderDialog(hDlg, pPRSD->pPRSI))
                    {            
                        pPRSD->fNewProviders = TRUE;
                        MarkChanged(hDlg);
                        FillBureauList(hDlg, pPRSD->pPRSI);
                    }    
                    break;
                case IDC_FINDRATINGS:
                {
                    BOOL fSuccess=FALSE;
                    
                    HINSTANCE hShell32=::LoadLibrary(::szShell32);
                    
                    if(hShell32!=NULL)
                    {
                        PFNSHELLEXECUTE pfnShellExecute=(PFNSHELLEXECUTE)::GetProcAddress(hShell32,::szShellExecute);
                        
                        if(pfnShellExecute!=NULL)
                        {
                            fSuccess=(*pfnShellExecute)(hDlg,NULL,(char *) &szFINDSYSTEM,NULL,NULL,SW_SHOW)!=NULL;
                        }
                        ::FreeLibrary(hShell32);
                    }
                    if (!fSuccess)
                    {
                        NLS_STR nlsMessage(MAX_RES_STR_LEN);
                        NLS_STR nlsTemp(STR_OWNERALLOC,(char *) &szFINDSYSTEM);
                        const NLS_STR *apnls[] = { &nlsTemp, NULL };
                        nlsMessage.LoadString(IDS_CANT_LAUNCH, apnls);
                        MyMessageBox(hDlg,nlsMessage.QueryPch(),IDS_GENERIC,MB_OK|MB_ICONSTOP);
                    }

                    break;
                }
#ifdef NASH
                case IDC_MASTER_USER:
                case IDC_CONTROL_PANEL:
                case IDC_NEW_APPS:
#endif
                case IDC_PLEASE_MOMMY:
                case IDC_UNRATED:
                    MarkChanged(hDlg);
                    break;

               case IDC_CHANGE_PASSWORD:
                    if (DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_CHANGE_PASSWORD), hDlg, (DLGPROC) ChangePasswordDialogProc))
                    {
                        MyMessageBox(hDlg, IDS_PASSWORD_CHANGED, IDS_GENERIC, MB_OK | MB_ICONINFORMATION);
                        MarkChanged(hDlg);
                    }

                    break;
                   
            }

            return TRUE;

        case WM_NOTIFY: {
            NMHDR *lpnm = (NMHDR *) lParam;
            switch (lpnm->code) {
                /*save us*/
                case PSN_APPLY:
                    /*do apply stuff*/
                    pPRSD = (PRSD *) ((PROPSHEETPAGE*)GetWindowLongPtr (hDlg, DWLP_USER))->lParam;

                    if (pPRSD->pPU != NULL) {
                        pPRSD->pPU->fAllowUnknowns = (IsDlgButtonChecked(hDlg, IDC_UNRATED) & BST_CHECKED) ? TRUE: FALSE;
                        pPRSD->pPU->fPleaseMom = (IsDlgButtonChecked(hDlg, IDC_PLEASE_MOMMY) & BST_CHECKED) ? TRUE: FALSE;
#ifdef NASH
                        pPRSD->pPU->etfMaster.Set((IsDlgButtonChecked(hDlg, IDC_MASTER_USER)    & BST_CHECKED) ? TRUE: FALSE);
                        pPRSD->pPU->etfControlPanel.Set((IsDlgButtonChecked(hDlg, IDC_CONTROL_PANEL) & BST_CHECKED) ? TRUE: FALSE);
                        pPRSD->pPU->etfNewApps.Set((IsDlgButtonChecked(hDlg, IDC_NEW_APPS)       & BST_CHECKED) ? TRUE: FALSE);
#endif
                    }

                    if (!((PSHNOTIFY*) lpnm)->lParam) break;
                    /*intentional fallthrough*/
                case PSN_RESET:
                    return TRUE;
            }
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
    }

    return FALSE;
}

//
// ShowBadUrl
//
// Show some ui telling user URL is bad when adding an approved site
//
void ShowBadUrl(HWND hDlg)
{
    char    lpszTitle[MAX_PATH],lpszMessage[MAX_PATH];

    // show message box
    MLLoadString(IDS_PICSRULES_BADURLTITLE,(LPTSTR) lpszTitle,MAX_PATH);
    MLLoadString(IDS_PICSRULES_BADURLMSG,(LPTSTR) lpszMessage,MAX_PATH);
    MessageBox(hDlg,(LPCTSTR) lpszMessage,(LPCTSTR) lpszTitle,MB_OK|MB_ICONERROR);

    // set focus back to edit box and select all of it
    SetFocus(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDEDIT));
    SendDlgItemMessage(hDlg,
                    IDC_PICSRULESAPPROVEDEDIT,
                    EM_SETSEL,
                    (WPARAM) 0,
                    (LPARAM) -1);
}


//Processes adding sites to the Approved Sites list.
//note, users will type in URLs _NOT_ in the form required
//in the PICSRules spec.  Thus, we will use InternetCrackURL
//and fill in the appropriate fields for them.
HRESULT PICSRulesApprovedSites(HWND hDlg,BOOL fAlwaysNever)
{
    PICSRulesPolicy             * pPRPolicy;
    PICSRulesByURL              * pPRByURL;
    PICSRulesByURLExpression    * pPRByURLExpression;
    LPSTR                       lpszSiteURL;
    HRESULT                     hr = NOERROR;
    URL_COMPONENTS              URLComponents;
    FN_INTERNETCRACKURL         pfnInternetCrackUrl;
    INTERNET_SCHEME             INetScheme=INTERNET_SCHEME_DEFAULT;
    INTERNET_PORT               INetPort=INTERNET_INVALID_PORT_NUMBER;
    LPSTR                       lpszScheme,lpszHostName,lpszUserName,
                                lpszPassword,lpszUrlPath,lpszExtraInfo;
    BOOL                        fAddedScheme=FALSE;
    LV_ITEM                     lvItem;
    LV_FINDINFO                 lvFindInfo;

    lpszSiteURL=new char[INTERNET_MAX_URL_LENGTH+1];
    if(lpszSiteURL==NULL)
    {
        return(E_OUTOFMEMORY);
    }

    //have we already processed it?
    SendDlgItemMessage(hDlg,
                       IDC_PICSRULESAPPROVEDEDIT,
                       WM_GETTEXT,
                       (WPARAM) INTERNET_MAX_URL_LENGTH,
                       (LPARAM) lpszSiteURL);

    if(*lpszSiteURL=='\0')
    {
        //nothing to do
        delete lpszSiteURL;

        return(E_INVALIDARG);
    }

    ZeroMemory(&lvFindInfo,sizeof(lvFindInfo));

    lvFindInfo.flags=LVFI_STRING;
    lvFindInfo.psz=lpszSiteURL;

    if(SendDlgItemMessage(hDlg,
                          IDC_PICSRULESAPPROVEDLIST,
                          LVM_FINDITEM,
                          (WPARAM) -1,
                          (LPARAM) &lvFindInfo)!=-1)
    {
        char    *lpszTitle,*lpszMessage;

        //we already have settings for this URL

        lpszTitle=(char *) GlobalAlloc(GPTR,MAX_PATH);
        lpszMessage=(char *) GlobalAlloc(GPTR,MAX_PATH);

        MLLoadString(IDS_PICSRULES_DUPLICATETITLE,(LPTSTR) lpszTitle,MAX_PATH);
        MLLoadString(IDS_PICSRULES_DUPLICATEMSG,(LPTSTR) lpszMessage,MAX_PATH);

        MessageBox(NULL,(LPCTSTR) lpszMessage,(LPCTSTR) lpszTitle,MB_OK|MB_ICONERROR);

        GlobalFree(lpszTitle);
        GlobalFree(lpszMessage);

        delete lpszSiteURL;

        SetFocus(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDEDIT));

        SendDlgItemMessage(hDlg,
                           IDC_PICSRULESAPPROVEDEDIT,
                           EM_SETSEL,
                           (WPARAM) 0,
                           (LPARAM) -1);

        return(E_INVALIDARG);
    }


    //
    // Add a scheme if user didn't type one
    //
    LPSTR lpszTemp = new char[INTERNET_MAX_URL_LENGTH+1];
    DWORD cbBuffer = INTERNET_MAX_URL_LENGTH;
    hr = UrlApplySchemeA(lpszSiteURL, lpszTemp, &cbBuffer, 0);
    if(S_OK == hr)
    {
        // actually added a scheme - switch to new buffer
        delete lpszSiteURL;
        lpszSiteURL = lpszTemp;
        fAddedScheme = TRUE;
    }
    else
    {
        // delete temp buffer
        delete lpszTemp;
    }


    //
    // Allocate all the pics rules structures we need
    //
    if(g_pApprovedPRRSPreApply==NULL)
    {
        g_pApprovedPRRSPreApply=new PICSRulesRatingSystem;
        if(g_pApprovedPRRSPreApply==NULL)
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
        hr = E_OUTOFMEMORY;
        goto clean;
    }
    pPRByURLExpression=new PICSRulesByURLExpression;
    if(pPRByURLExpression==NULL)
    {
        hr = E_OUTOFMEMORY;
        goto clean;
    }

    //
    // Crack the URL
    //
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
        hr = E_OUTOFMEMORY;
        goto clean;
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
        hr = E_UNEXPECTED;
        goto clean;
    }

    if(FALSE == pfnInternetCrackUrl(lpszSiteURL,0,ICU_DECODE,&URLComponents))
    {
        // failed to crack url
        ShowBadUrl(hDlg);
        hr = E_INVALIDARG;
        goto clean;
    }


    //
    // Set up linkages of pics rules structures
    //
    hr=g_pApprovedPRRSPreApply->AddItem(PROID_POLICY,pPRPolicy);
    if(FAILED(hr))
    {
        goto clean;
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

    pPRByURL->m_arrpPRByURL.Append(pPRByURLExpression);


    //
    // Use cracked URL components to fill in pics structs
    //
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
        pPRByURLExpression->m_bNonWild|=BYURL_PATH;           
        pPRByURLExpression->m_etstrPath.SetTo(lpszUrlPath);
    }
    else
    {
        delete lpszUrlPath;
    }
    pPRByURLExpression->m_bSpecified|=BYURL_PATH;

    if(URLComponents.nPort!=INTERNET_INVALID_PORT_NUMBER)
    {
        LPSTR lpszTemp;

        lpszTemp=new char[sizeof("65536")];

        wsprintf(lpszTemp,"%d",URLComponents.nPort);

        pPRByURLExpression->m_bNonWild|=BYURL_PORT;           
        pPRByURLExpression->m_etstrPort.SetTo(lpszTemp);
    }
    pPRByURLExpression->m_bSpecified|=BYURL_PORT;

    //
    // need to make sure we echo exactly what they typed in
    //

    // just UI stuff left so assume success - we don't want to delete pics
    // structs now that they're linked into other pics structs
    hr = NOERROR;

    SendDlgItemMessage(hDlg,
                       IDC_PICSRULESAPPROVEDEDIT,
                       WM_GETTEXT,
                       (WPARAM) INTERNET_MAX_URL_LENGTH,
                       (LPARAM) lpszSiteURL);

    ZeroMemory(&lvItem,sizeof(lvItem));

    lvItem.mask=LVIF_TEXT|LVIF_IMAGE;
    lvItem.pszText=lpszSiteURL;

    if(fAlwaysNever==PICSRULES_NEVER)
    {
        lvItem.iImage=g_iAllowNever;
    }
    else
    {
        lvItem.iImage=g_iAllowAlways;
    }

    if(SendDlgItemMessage(hDlg,
                          IDC_PICSRULESAPPROVEDLIST,
                          LVM_INSERTITEM,
                          (WPARAM) 0,
                          (LPARAM) &lvItem)==-1)
    {
        goto clean;
    }

    ListView_SetColumnWidth(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDLIST),
                            0,
                            LVSCW_AUTOSIZE);

    SendDlgItemMessage(hDlg,
                       IDC_PICSRULESAPPROVEDEDIT,
                       WM_SETTEXT,
                       (WPARAM) 0,
                       (LPARAM) 0);

    MarkChanged(hDlg);

    pPRByURLExpression->m_etstrURL.SetTo(lpszSiteURL);

clean:
    SAFEDELETE(lpszPassword);
    SAFEDELETE(lpszExtraInfo);

    if(FAILED(hr))
    {
        // failed means we didn't get chance to save or delete these
        SAFEDELETE(lpszScheme);
        SAFEDELETE(lpszHostName);
        SAFEDELETE(lpszUserName);
        SAFEDELETE(lpszUrlPath);

        // a failed hr means we didn't link these guys together so they need
        // to be deleted
        SAFEDELETE(lpszSiteURL);
        SAFEDELETE(pPRPolicy);
        SAFEDELETE(pPRByURL);
        SAFEDELETE(pPRByURLExpression);
    }

    return hr;
}

BOOL CALLBACK ApprovedSitesDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PRSD *pPRSD;
    
    static DWORD aIds[] = {
        IDC_STATIC_ALLOW,               IDH_PICSRULES_APPROVEDEDIT,
        IDC_PICSRULESAPPROVEDEDIT,      IDH_PICSRULES_APPROVEDEDIT,
        IDC_PICSRULESAPPROVEDALWAYS,    IDH_PICSRULES_APPROVEDALWAYS,
        IDC_PICSRULESAPPROVEDNEVER,     IDH_PICSRULES_APPROVEDNEVER,
        IDC_STATIC_LIST,                IDH_PICSRULES_APPROVEDLIST,
        IDC_PICSRULESAPPROVEDLIST,      IDH_PICSRULES_APPROVEDLIST,
        IDC_PICSRULESAPPROVEDREMOVE,    IDH_PICSRULES_APPROVEDREMOVE,
        0,0
    };

    switch (uMsg) {
        case WM_SYSCOLORCHANGE:
        {
            HICON      hIcon;
            HIMAGELIST hOldImageList;
            UINT flags = 0;
            
            pPRSD=(PRSD *) ((PROPSHEETPAGE*)GetWindowLongPtr(hDlg,DWLP_USER))->lParam;

            ListView_SetBkColor(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDLIST),
                                GetSysColor(COLOR_WINDOW));
            if(IS_WINDOW_RTL_MIRRORED(hDlg))
            {
                flags |= ILC_MIRROR;
            }

            g_hImageList=ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                          GetSystemMetrics(SM_CYSMICON),
                                          flags,
                                          2,
                                          0);

            hIcon=(HICON) LoadImage(hInstance,
                                    MAKEINTRESOURCE(IDI_ACCEPTALWAYS),
                                    IMAGE_ICON,
                                    16,
                                    16,
                                    LR_LOADTRANSPARENT|LR_DEFAULTCOLOR|LR_CREATEDIBSECTION);

            g_iAllowAlways=ImageList_AddIcon(g_hImageList,hIcon); 
            DeleteObject(hIcon); 
 
            hIcon=(HICON) LoadImage(hInstance,
                                    MAKEINTRESOURCE(IDI_ACCEPTNEVER),
                                    IMAGE_ICON,
                                    16,
                                    16,
                                    LR_LOADTRANSPARENT|LR_DEFAULTCOLOR|LR_CREATEDIBSECTION);
            g_iAllowNever=ImageList_AddIcon(g_hImageList,hIcon); 
            DeleteObject(hIcon); 

            hOldImageList=ListView_SetImageList(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDLIST),g_hImageList,LVSIL_SMALL); 

            if(hOldImageList!=NULL)
            {
                ImageList_Destroy(hOldImageList);
            }
            
            break;
        }
        case WM_INITDIALOG:
        {
            RECT        Rect;
            HDC         hDC;
            LV_COLUMN   lvColumn;
            TEXTMETRIC  tm;
            HICON       hIcon;
            int         iCounter;
            UINT flags = 0;

            SetWindowLongPtr (hDlg, DWLP_USER, lParam);
            pPRSD = (PRSD *) ((PROPSHEETPAGE*)lParam)->lParam;

            if (pPRSD->pPU != NULL) {
                //set defaults for controls
            }

            GetWindowRect(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDLIST),&Rect);

            tm.tmAveCharWidth=0;
            if(hDC=GetDC(hDlg))
            {
                GetTextMetrics(hDC,&tm);
                ReleaseDC(hDlg,hDC);
            }

            lvColumn.mask=LVCF_FMT|LVCF_WIDTH;
            lvColumn.fmt=LVCFMT_LEFT;
            lvColumn.cx=Rect.right-Rect.left
                        -GetSystemMetrics(SM_CXVSCROLL)
                        -GetSystemMetrics(SM_CXSMICON)
                        -tm.tmAveCharWidth;

            SendDlgItemMessage(hDlg,
                               IDC_PICSRULESAPPROVEDLIST,
                               LVM_INSERTCOLUMN,
                               (WPARAM) 0,
                               (LPARAM) &lvColumn);

            if(IS_WINDOW_RTL_MIRRORED(hDlg))
            {
                flags |= ILC_MIRROR;
            }
            g_hImageList=ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                          GetSystemMetrics(SM_CYSMICON),
                                          flags,
                                          2,
                                          0);

            hIcon=(HICON) LoadImage(hInstance,
                                    MAKEINTRESOURCE(IDI_ACCEPTALWAYS),
                                    IMAGE_ICON,
                                    16,
                                    16,
                                    LR_LOADTRANSPARENT|LR_DEFAULTCOLOR|LR_CREATEDIBSECTION);

            g_iAllowAlways=ImageList_AddIcon(g_hImageList,hIcon); 
            DeleteObject(hIcon); 
 
            hIcon=(HICON) LoadImage(hInstance,
                                    MAKEINTRESOURCE(IDI_ACCEPTNEVER),
                                    IMAGE_ICON,
                                    16,
                                    16,
                                    LR_LOADTRANSPARENT|LR_DEFAULTCOLOR|LR_CREATEDIBSECTION);
            g_iAllowNever=ImageList_AddIcon(g_hImageList,hIcon); 
            DeleteObject(hIcon); 

            ListView_SetImageList(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDLIST),g_hImageList,LVSIL_SMALL); 

            //disable the remove button until someone selects something
            EnableWindow(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDREMOVE),FALSE);
            
            //disable the always and never buttons until someone types something
            EnableWindow(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDNEVER),FALSE);
            EnableWindow(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDALWAYS),FALSE);

            if(g_pApprovedPRRSPreApply!=NULL)
            {
                delete g_pApprovedPRRSPreApply;

                g_pApprovedPRRSPreApply=NULL;
            }

            if(g_lApprovedSitesGlobalCounterValue!=SHGlobalCounterGetValue(g_ApprovedSitesHandleGlobalCounter))
            {
                PICSRulesRatingSystem * pPRRS=NULL;
                HRESULT               hRes;

                hRes=PICSRulesReadFromRegistry(PICSRULES_APPROVEDSITES,&pPRRS);

                if(SUCCEEDED(hRes))
                {
                    if(g_pApprovedPRRS!=NULL)
                    {
                        delete g_pApprovedPRRS;
                    }

                    g_pApprovedPRRS=pPRRS;
                }

                g_lApprovedSitesGlobalCounterValue=SHGlobalCounterGetValue(g_ApprovedSitesHandleGlobalCounter);
            }

            if(g_pApprovedPRRS==NULL)
            {
                //nothing to do
                break;
            }
            
            //copy master list to the PreApply list
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

                break;
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

                    break;
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

                    break;
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

                    break;
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

                    break;
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

            //fill in the listview with known items
            for(iCounter=0;iCounter<g_pApprovedPRRSPreApply->m_arrpPRPolicy.Length();iCounter++)
            {
                BOOL                        fAcceptReject;
                PICSRulesPolicy             * pPRPolicy;
                PICSRulesByURLExpression    * pPRByURLExpression;
                LV_ITEM                     lvItem;

                pPRPolicy=g_pApprovedPRRSPreApply->m_arrpPRPolicy[iCounter];

                if(pPRPolicy->m_PRPolicyAttribute==PR_POLICY_ACCEPTBYURL)
                {
                    fAcceptReject=PICSRULES_ALWAYS;

                    pPRByURLExpression=pPRPolicy->m_pPRAcceptByURL->m_arrpPRByURL[0];
                }
                else
                {
                    fAcceptReject=PICSRULES_NEVER;

                    pPRByURLExpression=pPRPolicy->m_pPRRejectByURL->m_arrpPRByURL[0];
                }

                ZeroMemory(&lvItem,sizeof(lvItem));

                lvItem.mask=LVIF_TEXT|LVIF_IMAGE;
                lvItem.pszText=pPRByURLExpression->m_etstrURL.Get();

                if(fAcceptReject==PICSRULES_NEVER)
                {
                    lvItem.iImage=g_iAllowNever;
                }
                else
                {
                    lvItem.iImage=g_iAllowAlways;
                }

                if(SendDlgItemMessage(hDlg,
                                      IDC_PICSRULESAPPROVEDLIST,
                                      LVM_INSERTITEM,
                                      (WPARAM) 0,
                                      (LPARAM) &lvItem)==-1)
                {
                    break;
                }

            }

            // Set the column width to satisfy longest element
            ListView_SetColumnWidth(
                            GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDLIST),
                            0,
                            LVSCW_AUTOSIZE);

            // set focus to first item in list
            ListView_SetItemState(
                            GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDLIST),
                            0,
                            LVIS_FOCUSED,
                            LVIS_FOCUSED);
            break;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_PICSRULESAPPROVEDEDIT:
                {
                    if(HIWORD(wParam)==EN_UPDATE)
                    {
                        INT_PTR iCount;

                        iCount=SendDlgItemMessage(hDlg,
                                                  IDC_PICSRULESAPPROVEDEDIT,
                                                  WM_GETTEXTLENGTH,
                                                  (WPARAM) 0,
                                                  (LPARAM) 0);

                        if(iCount>0)
                        {
                            EnableWindow(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDNEVER),TRUE);
                            EnableWindow(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDALWAYS),TRUE);
                        }
                        else
                        {
                            SetFocus(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDEDIT));

                            SendDlgItemMessage(hDlg,
                                               IDC_PICSRULESAPPROVEDEDIT,
                                               EM_SETSEL,
                                               (WPARAM) 0,
                                               (LPARAM) -1);

                            SendDlgItemMessage(hDlg,
                                               IDC_PICSRULESAPPROVEDNEVER,
                                               BM_SETSTYLE,
                                               (WPARAM) BS_PUSHBUTTON,
                                               (LPARAM) MAKELPARAM(TRUE,0));

                            SendDlgItemMessage(hDlg,
                                               IDC_PICSRULESAPPROVEDALWAYS,
                                               BM_SETSTYLE,
                                               (WPARAM) BS_PUSHBUTTON,
                                               (LPARAM) MAKELPARAM(TRUE,0));

                            EnableWindow(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDNEVER),FALSE);
                            EnableWindow(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDALWAYS),FALSE);
                        }
                    }

                    break;
                }
                case IDC_PICSRULESAPPROVEDNEVER:
                {
                    HRESULT hRes;

                    hRes=PICSRulesApprovedSites(hDlg,PICSRULES_NEVER);

                    break;
                }
                case IDC_PICSRULESAPPROVEDALWAYS:
                {
                    HRESULT hRes;

                    hRes=PICSRulesApprovedSites(hDlg,PICSRULES_ALWAYS);

                    break;
                }
                case IDC_PICSRULESAPPROVEDREMOVE:
                {
                    int                     iNumApproved,iCounter,iNumSelected,
                                            iSubCounter,iItem=-1;
                    LPTSTR                  lpszRemoveURL;
                    LV_ITEM                 lvItem;
                    PICSRulesRatingSystem   * pNewApprovedPRRS;

                    if(g_pApprovedPRRSPreApply==NULL)
                    {
                        //nothing to do
                        break;
                    }

                    iNumApproved=g_pApprovedPRRSPreApply->m_arrpPRPolicy.Length();

                    iNumSelected=ListView_GetSelectedCount(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDLIST));

                    if(iNumSelected==0)
                    {
                        //nothing to do
                        break;
                    }

                    lpszRemoveURL=new char[INTERNET_MAX_URL_LENGTH+1];

                    if(lpszRemoveURL==NULL)
                    {
                        char    szTitle[MAX_PATH],szMessage[MAX_PATH];

                        //out of memory, so we init on the stack

                        MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
                        MLLoadString(IDS_PICSRULES_OUTOFMEMORY,(LPTSTR) szMessage,MAX_PATH);

                        MessageBox(NULL,(LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);

                        return(E_OUTOFMEMORY);
                    }

                    for(iCounter=0;iCounter<iNumSelected;iCounter++)
                    {
                        ZeroMemory(&lvItem,sizeof(lvItem));

                        iItem=ListView_GetNextItem(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDLIST),iItem,LVNI_SELECTED);
                        
                        lvItem.iItem=iItem;
                        lvItem.pszText=lpszRemoveURL;
                        lvItem.cchTextMax=INTERNET_MAX_URL_LENGTH;

                        SendDlgItemMessage(hDlg,IDC_PICSRULESAPPROVEDLIST,LVM_GETITEMTEXT,(WPARAM) iItem,(LPARAM) &lvItem);

                        for(iSubCounter=0;iSubCounter<iNumApproved;iSubCounter++)
                        {
                            PICSRulesPolicy *pPRPolicy;
                            PICSRulesByURLExpression * pPRByURLExpression;

                            pPRPolicy=g_pApprovedPRRSPreApply->m_arrpPRPolicy[iSubCounter];

                            if(pPRPolicy==NULL)
                            {
                                continue;
                            }

                            if(pPRPolicy->m_PRPolicyAttribute==PR_POLICY_REJECTBYURL)
                            {
                                if(pPRPolicy->m_pPRRejectByURL==NULL)
                                {
                                    continue;
                                }

                                pPRByURLExpression=pPRPolicy->m_pPRRejectByURL->m_arrpPRByURL[0];
                            }
                            else
                            {
                                if(pPRPolicy->m_pPRAcceptByURL==NULL)
                                {
                                    continue;
                                }

                                pPRByURLExpression=pPRPolicy->m_pPRAcceptByURL->m_arrpPRByURL[0];
                            }

                            if(pPRByURLExpression==NULL)
                            {
                                continue;
                            }
                            
                            if(pPRByURLExpression->m_etstrURL.Get()==NULL)
                            {
                                continue;
                            }

                            if(lstrcmp(pPRByURLExpression->m_etstrURL.Get(),lpszRemoveURL)==0)
                            {
                                //we found one to delete
                                if(pPRPolicy!=NULL)
                                {
                                    delete pPRPolicy;
                                }

                                g_pApprovedPRRSPreApply->m_arrpPRPolicy[iSubCounter]=0;
                            }
                        }
                    }

                    //delete them from the list view
                    for(iCounter=0;iCounter<iNumSelected;iCounter++)
                    {
                        iItem=ListView_GetNextItem(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDLIST),-1,LVNI_SELECTED);
                    
                        ListView_DeleteItem(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDLIST),iItem);
                    }

                    //rebuild the approved PICSRules structure
                    pNewApprovedPRRS=new PICSRulesRatingSystem;
                    
                    for(iCounter=0;iCounter<iNumApproved;iCounter++)
                    {
                        if(g_pApprovedPRRSPreApply->m_arrpPRPolicy[iCounter]!=NULL)
                        {
                            pNewApprovedPRRS->m_arrpPRPolicy.Append(g_pApprovedPRRSPreApply->m_arrpPRPolicy[iCounter]);
                        }
                    }

                    g_pApprovedPRRSPreApply->m_arrpPRPolicy.ClearAll();

                    delete g_pApprovedPRRSPreApply;

                    g_pApprovedPRRSPreApply=pNewApprovedPRRS;

                    MarkChanged(hDlg);

                    SetFocus(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDEDIT));

                    SendDlgItemMessage(hDlg,
                                       IDC_PICSRULESAPPROVEDEDIT,
                                       EM_SETSEL,
                                       (WPARAM) 0,
                                       (LPARAM) -1);

                    delete lpszRemoveURL;

                    break;
                }
            }

            return TRUE;

        case WM_NOTIFY: {
            NMHDR *lpnm = (NMHDR *) lParam;
            switch (lpnm->code) {
                case LVN_ITEMCHANGED:
                {
                    BOOL    fEnable = FALSE;

                    if(ListView_GetSelectedCount(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDLIST)) > 0)
                    {
                        fEnable = TRUE;
                    }
                    EnableWindow(GetDlgItem(hDlg,IDC_PICSRULESAPPROVEDREMOVE),fEnable);
                    break;
                }
                /*save us*/
                case PSN_APPLY:
                {
                    int iCounter,iLoopCounter;

                    if(g_pApprovedPRRSPreApply==NULL)
                    {
                        //we don't have anything to set

                        pPRSD = (PRSD *) ((PROPSHEETPAGE*)GetWindowLongPtr (hDlg, DWLP_USER))->lParam;

                        if (pPRSD->pPU != NULL) {
                            //set defaults
                        }

                        if (!((PSHNOTIFY*) lpnm)->lParam) break;

                        return TRUE;
                    }

                    //make the Approved Sites Global
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

                        break;
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

                                break;
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

                                break;
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

                                break;
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

                                break;
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

                    SHGlobalCounterIncrement(g_ApprovedSitesHandleGlobalCounter);

                    /*do apply stuff*/
                    pPRSD = (PRSD *) ((PROPSHEETPAGE*)GetWindowLongPtr (hDlg, DWLP_USER))->lParam;

                    if (pPRSD->pPU != NULL) {
                        //set defaults
                    }

                    if (!((PSHNOTIFY*) lpnm)->lParam) break;

                    return TRUE;
                }
                case PSN_RESET:
                    return TRUE;
            }
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
    }

    return FALSE;
}

void InstallRatingBureauHelper(void)
{
    HKEY hkey;

    if (RegCreateKey(HKEY_LOCAL_MACHINE, szRATINGHELPERS, &hkey) == ERROR_SUCCESS) {
        RegSetValueEx(hkey, szRORSGUID, 0, REG_SZ, (LPBYTE)"", 1);
        RegCloseKey(hkey);
    }
}


void DeinstallRatingBureauHelper(void)
{
    HKEY hkey;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, szRATINGHELPERS, &hkey) == ERROR_SUCCESS) {
        RegDeleteValue(hkey, szRORSGUID);
        RegCloseKey(hkey);
    }
}

HRESULT CopySubPolicyExpression(PICSRulesPolicyExpression * pPRSubPolicyExpression,
                                PICSRulesPolicyExpression * pPRSubPolicyExpressionToCopy,
                                PICSRulesRatingSystem * pPRRSLocal,
                                PICSRulesPolicy * pPRPolicy)
{
    BOOL fFlag;
    
    fFlag=pPRSubPolicyExpressionToCopy->m_prYesNoUseEmbedded.GetYesNo();
    pPRSubPolicyExpression->m_prYesNoUseEmbedded.Set(&fFlag);

    pPRSubPolicyExpression->m_etstrServiceName.Set(pPRSubPolicyExpressionToCopy->m_etstrServiceName.Get());
    pPRSubPolicyExpression->m_etstrCategoryName.Set(pPRSubPolicyExpressionToCopy->m_etstrCategoryName.Get());
    pPRSubPolicyExpression->m_etstrFullServiceName.Set(pPRSubPolicyExpressionToCopy->m_etstrFullServiceName.Get());
    pPRSubPolicyExpression->m_etnValue.Set(pPRSubPolicyExpressionToCopy->m_etnValue.Get());

    pPRSubPolicyExpression->m_PROPolicyOperator=pPRSubPolicyExpressionToCopy->m_PROPolicyOperator;
    pPRSubPolicyExpression->m_PRPEPolicyEmbedded=pPRSubPolicyExpressionToCopy->m_PRPEPolicyEmbedded;

    if(pPRSubPolicyExpressionToCopy->m_pPRPolicyExpressionLeft!=NULL)
    {
        PICSRulesPolicyExpression * pPRSubPolicyExpression2,* pPRSubPolicyExpressionToCopy2;

        pPRSubPolicyExpression2=new PICSRulesPolicyExpression;

        if(pPRSubPolicyExpression2==NULL)
        {
            PICSRulesOutOfMemory();

            delete pPRRSLocal;
            delete pPRPolicy;

            return(E_OUTOFMEMORY);
        }

        pPRSubPolicyExpressionToCopy2=pPRSubPolicyExpressionToCopy->m_pPRPolicyExpressionLeft;

        if(FAILED(CopySubPolicyExpression(pPRSubPolicyExpression2,pPRSubPolicyExpressionToCopy2,pPRRSLocal,pPRPolicy)))
        {
            return(E_OUTOFMEMORY);
        }

        pPRSubPolicyExpression->m_pPRPolicyExpressionLeft=pPRSubPolicyExpression2;
    }

    if(pPRSubPolicyExpressionToCopy->m_pPRPolicyExpressionRight!=NULL)
    {
        PICSRulesPolicyExpression * pPRSubPolicyExpression2,* pPRSubPolicyExpressionToCopy2;

        pPRSubPolicyExpression2=new PICSRulesPolicyExpression;

        if(pPRSubPolicyExpression2==NULL)
        {
            PICSRulesOutOfMemory();

            delete pPRRSLocal;
            delete pPRPolicy;

            return(E_OUTOFMEMORY);
        }

        pPRSubPolicyExpressionToCopy2=pPRSubPolicyExpressionToCopy->m_pPRPolicyExpressionRight;

        if(FAILED(CopySubPolicyExpression(pPRSubPolicyExpression2,pPRSubPolicyExpressionToCopy2,pPRRSLocal,pPRPolicy)))
        {
            return(E_OUTOFMEMORY);
        }

        pPRSubPolicyExpression->m_pPRPolicyExpressionRight=pPRSubPolicyExpression2;
    }

    return(NOERROR);
}

HRESULT CopyArrayPRRSStructures(array<PICSRulesRatingSystem*> * parrpPRRSDest,array<PICSRulesRatingSystem*> * parrpPRRSSource)
{
    DWORD                       dwCounter;
    PICSRulesRatingSystem       * pPRRSLocal,* pPRRSBeingCopied;
    PICSRulesPolicy             * pPRPolicy,* pPRPolicyBeingCopied;
    PICSRulesPolicyExpression   * pPRPolicyExpression,* pPRPolicyExpressionBeingCopied;
    PICSRulesServiceInfo        * pPRServiceInfo,* pPRServiceInfoBeingCopied;
    PICSRulesOptExtension       * pPROptExtension,* pPROptExtensionBeingCopied;
    PICSRulesReqExtension       * pPRReqExtension,* pPRReqExtensionBeingCopied;
    PICSRulesName               * pPRName,* pPRNameBeingCopied;
    PICSRulesSource             * pPRSource,* pPRSourceBeingCopied;
    PICSRulesByURL              * pPRByURL,* pPRByURLToCopy;
    PICSRulesByURLExpression    * pPRByURLExpression,* pPRByURLExpressionToCopy;

    if(parrpPRRSDest->Length()>0)
    {
        parrpPRRSDest->DeleteAll();
    }

    for(dwCounter=0;dwCounter<(DWORD) (parrpPRRSSource->Length());dwCounter++)
    {
        pPRRSLocal=new PICSRulesRatingSystem;

        if(pPRRSLocal==NULL)
        {
            PICSRulesOutOfMemory();

            return(E_OUTOFMEMORY);
        }

        pPRRSBeingCopied=(*parrpPRRSSource)[dwCounter];

        pPRRSLocal->m_etstrFile.Set(pPRRSBeingCopied->m_etstrFile.Get());
        pPRRSLocal->m_etnPRVerMajor.Set(pPRRSBeingCopied->m_etnPRVerMajor.Get());
        pPRRSLocal->m_etnPRVerMinor.Set(pPRRSBeingCopied->m_etnPRVerMinor.Get());
        pPRRSLocal->m_dwFlags=pPRRSBeingCopied->m_dwFlags;
        pPRRSLocal->m_nErrLine=pPRRSBeingCopied->m_nErrLine;

        if((pPRRSBeingCopied->m_pPRName)!=NULL)
        {
            pPRName=new PICSRulesName;

            if(pPRName==NULL)
            {
                PICSRulesOutOfMemory();

                delete pPRRSLocal;

                return(E_OUTOFMEMORY);
            }

            pPRNameBeingCopied=pPRRSBeingCopied->m_pPRName;

            pPRName->m_etstrRuleName.Set(pPRNameBeingCopied->m_etstrRuleName.Get());
            pPRName->m_etstrDescription.Set(pPRNameBeingCopied->m_etstrDescription.Get());
        
            pPRRSLocal->m_pPRName=pPRName;          
        }
        else
        {
            pPRRSLocal->m_pPRName=NULL;
        }

        if((pPRRSBeingCopied->m_pPRSource)!=NULL)
        {
            pPRSource=new PICSRulesSource;

            if(pPRSource==NULL)
            {
                PICSRulesOutOfMemory();

                delete pPRRSLocal;

                return(E_OUTOFMEMORY);
            }

            pPRSourceBeingCopied=pPRRSBeingCopied->m_pPRSource;

            pPRSource->m_prURLSourceURL.Set(pPRSourceBeingCopied->m_prURLSourceURL.Get());
            pPRSource->m_etstrCreationTool.Set(pPRSourceBeingCopied->m_etstrCreationTool.Get());
            pPRSource->m_prEmailAuthor.Set(pPRSourceBeingCopied->m_prEmailAuthor.Get());
            pPRSource->m_prDateLastModified.Set(pPRSourceBeingCopied->m_prDateLastModified.Get());

            pPRRSLocal->m_pPRSource=pPRSource;
        }
        else
        {
            pPRRSLocal->m_pPRSource=NULL;
        }

        if(pPRRSBeingCopied->m_arrpPRPolicy.Length()>0)
        {
            DWORD dwSubCounter;

            for(dwSubCounter=0;dwSubCounter<(DWORD) (pPRRSBeingCopied->m_arrpPRPolicy.Length());dwSubCounter++)
            {
                DWORD dwPolicyExpressionSubCounter;

                pPRPolicy=new PICSRulesPolicy;

                pPRPolicyBeingCopied=pPRRSBeingCopied->m_arrpPRPolicy[dwSubCounter];

                if(pPRPolicy==NULL)
                {
                    PICSRulesOutOfMemory();

                    delete pPRRSLocal;

                    return(E_OUTOFMEMORY);
                }

                pPRPolicy->m_etstrExplanation.Set(pPRPolicyBeingCopied->m_etstrExplanation.Get());
                pPRPolicy->m_PRPolicyAttribute=pPRPolicyBeingCopied->m_PRPolicyAttribute;

                pPRByURLToCopy=NULL;
                pPRPolicyExpressionBeingCopied=NULL;

                switch(pPRPolicy->m_PRPolicyAttribute)
                {
                    case PR_POLICY_ACCEPTBYURL:
                    {
                        pPRByURLToCopy=pPRPolicyBeingCopied->m_pPRAcceptByURL;
                    
                        pPRByURL=new PICSRulesByURL;
                
                        if(pPRByURL==NULL)
                        {
                            PICSRulesOutOfMemory();

                            delete pPRRSLocal;
                            delete pPRPolicy;

                            return(E_OUTOFMEMORY);
                        }

                        pPRPolicy->m_pPRAcceptByURL=pPRByURL;

                        break;
                    }
                    case PR_POLICY_REJECTBYURL:
                    {
                        pPRByURLToCopy=pPRPolicyBeingCopied->m_pPRRejectByURL;

                        pPRByURL=new PICSRulesByURL;
                
                        if(pPRByURL==NULL)
                        {
                            PICSRulesOutOfMemory();

                            delete pPRRSLocal;
                            delete pPRPolicy;

                            return(E_OUTOFMEMORY);
                        }

                        pPRPolicy->m_pPRRejectByURL=pPRByURL;

                        break;
                    }
                    case PR_POLICY_REJECTIF:
                    {
                        pPRPolicyExpressionBeingCopied=pPRPolicyBeingCopied->m_pPRRejectIf;

                        pPRPolicyExpression=new PICSRulesPolicyExpression;
                
                        if(pPRPolicyExpression==NULL)
                        {
                            PICSRulesOutOfMemory();

                            delete pPRRSLocal;
                            delete pPRPolicy;

                            return(E_OUTOFMEMORY);
                        }

                        pPRPolicy->m_pPRRejectIf=pPRPolicyExpression;

                        break;
                    }
                    case PR_POLICY_ACCEPTIF:
                    {
                        pPRPolicyExpressionBeingCopied=pPRPolicyBeingCopied->m_pPRAcceptIf;

                        pPRPolicyExpression=new PICSRulesPolicyExpression;
                
                        if(pPRPolicyExpression==NULL)
                        {
                            PICSRulesOutOfMemory();

                            delete pPRRSLocal;
                            delete pPRPolicy;

                            return(E_OUTOFMEMORY);
                        }

                        pPRPolicy->m_pPRAcceptIf=pPRPolicyExpression;

                        break;
                    }
                    case PR_POLICY_REJECTUNLESS:
                    {
                        pPRPolicyExpressionBeingCopied=pPRPolicyBeingCopied->m_pPRRejectUnless;

                        pPRPolicyExpression=new PICSRulesPolicyExpression;
                
                        if(pPRPolicyExpression==NULL)
                        {
                            PICSRulesOutOfMemory();

                            delete pPRRSLocal;
                            delete pPRPolicy;

                            return(E_OUTOFMEMORY);
                        }

                        pPRPolicy->m_pPRRejectUnless=pPRPolicyExpression;

                        break;
                    }
                    case PR_POLICY_ACCEPTUNLESS:
                    {
                        pPRPolicyExpressionBeingCopied=pPRPolicyBeingCopied->m_pPRAcceptUnless;

                        pPRPolicyExpression=new PICSRulesPolicyExpression;
                
                        if(pPRPolicyExpression==NULL)
                        {
                            PICSRulesOutOfMemory();

                            delete pPRRSLocal;
                            delete pPRPolicy;

                            return(E_OUTOFMEMORY);
                        }

                        pPRPolicy->m_pPRAcceptUnless=pPRPolicyExpression;

                        break;
                    }
                }

                if(pPRByURLToCopy!=NULL)
                {
                    for(dwPolicyExpressionSubCounter=0;
                        dwPolicyExpressionSubCounter<(DWORD) (pPRByURLToCopy->m_arrpPRByURL.Length());
                        dwPolicyExpressionSubCounter++)
                    {
                        pPRByURLExpression=new PICSRulesByURLExpression;

                        if(pPRByURLExpression==NULL)
                        {
                            PICSRulesOutOfMemory();

                            delete pPRRSLocal;
                            delete pPRPolicy;

                            return(E_OUTOFMEMORY);
                        }

                        pPRByURLExpressionToCopy=pPRByURLToCopy->m_arrpPRByURL[dwPolicyExpressionSubCounter];

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
                    }

                    pPRRSLocal->m_arrpPRPolicy.Append(pPRPolicy);
                }

                if(pPRPolicyExpressionBeingCopied!=NULL)
                {
                    BOOL fFlag;
                    
                    fFlag=pPRPolicyExpressionBeingCopied->m_prYesNoUseEmbedded.GetYesNo();
                    pPRPolicyExpression->m_prYesNoUseEmbedded.Set(&fFlag);

                    pPRPolicyExpression->m_etstrServiceName.Set(pPRPolicyExpressionBeingCopied->m_etstrServiceName.Get());
                    pPRPolicyExpression->m_etstrCategoryName.Set(pPRPolicyExpressionBeingCopied->m_etstrCategoryName.Get());
                    pPRPolicyExpression->m_etstrFullServiceName.Set(pPRPolicyExpressionBeingCopied->m_etstrFullServiceName.Get());
                    pPRPolicyExpression->m_etnValue.Set(pPRPolicyExpressionBeingCopied->m_etnValue.Get());

                    pPRPolicyExpression->m_PROPolicyOperator=pPRPolicyExpressionBeingCopied->m_PROPolicyOperator;
                    pPRPolicyExpression->m_PRPEPolicyEmbedded=pPRPolicyExpressionBeingCopied->m_PRPEPolicyEmbedded;

                    if(pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionLeft!=NULL)
                    {
                        PICSRulesPolicyExpression * pPRSubPolicyExpression,* pPRSubPolicyExpressionToCopy;

                        pPRSubPolicyExpression=new PICSRulesPolicyExpression;

                        if(pPRSubPolicyExpression==NULL)
                        {
                            PICSRulesOutOfMemory();

                            delete pPRRSLocal;
                            delete pPRPolicy;

                            return(E_OUTOFMEMORY);
                        }

                        pPRSubPolicyExpressionToCopy=pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionLeft;

                        if(FAILED(CopySubPolicyExpression(pPRSubPolicyExpression,pPRSubPolicyExpressionToCopy,pPRRSLocal,pPRPolicy)))
                        {
                            return(E_OUTOFMEMORY);
                        }

                        pPRPolicyExpression->m_pPRPolicyExpressionLeft=pPRSubPolicyExpression;
                    }

                    if(pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionRight!=NULL)
                    {
                        PICSRulesPolicyExpression * pPRSubPolicyExpression,* pPRSubPolicyExpressionToCopy;

                        pPRSubPolicyExpression=new PICSRulesPolicyExpression;

                        if(pPRSubPolicyExpression==NULL)
                        {
                            PICSRulesOutOfMemory();

                            delete pPRRSLocal;
                            delete pPRPolicy;

                            return(E_OUTOFMEMORY);
                        }

                        pPRSubPolicyExpressionToCopy=pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionRight;

                        if(FAILED(CopySubPolicyExpression(pPRSubPolicyExpression,pPRSubPolicyExpressionToCopy,pPRRSLocal,pPRPolicy)))
                        {
                            return(E_OUTOFMEMORY);
                        }

                        pPRPolicyExpression->m_pPRPolicyExpressionRight=pPRSubPolicyExpression;
                    }

                    pPRRSLocal->m_arrpPRPolicy.Append(pPRPolicy);
                }
            }
        }

        if(pPRRSBeingCopied->m_arrpPRServiceInfo.Length()>0)
        {
            DWORD dwSubCounter;

            for(dwSubCounter=0;dwSubCounter<(DWORD) (pPRRSBeingCopied->m_arrpPRServiceInfo.Length());dwSubCounter++)
            {
                BOOL fFlag;

                pPRServiceInfo=new PICSRulesServiceInfo;

                if(pPRServiceInfo==NULL)
                {
                    PICSRulesOutOfMemory();

                    delete pPRRSLocal;

                    return(E_OUTOFMEMORY);
                }

                pPRServiceInfoBeingCopied=pPRRSBeingCopied->m_arrpPRServiceInfo[dwSubCounter];

                pPRServiceInfo->m_prURLName.Set(pPRServiceInfoBeingCopied->m_prURLName.Get());
                pPRServiceInfo->m_prURLBureauURL.Set(pPRServiceInfoBeingCopied->m_prURLBureauURL.Get());
                pPRServiceInfo->m_etstrShortName.Set(pPRServiceInfoBeingCopied->m_etstrShortName.Get());
                pPRServiceInfo->m_etstrRatfile.Set(pPRServiceInfoBeingCopied->m_etstrRatfile.Get());

                fFlag=pPRServiceInfoBeingCopied->m_prYesNoUseEmbedded.GetYesNo();
                pPRServiceInfo->m_prYesNoUseEmbedded.Set(&fFlag);

                fFlag=pPRServiceInfoBeingCopied->m_prPassFailBureauUnavailable.GetPassFail();
                pPRServiceInfo->m_prPassFailBureauUnavailable.Set(&fFlag);

                pPRRSLocal->m_arrpPRServiceInfo.Append(pPRServiceInfo);
            }
        }

        if(pPRRSBeingCopied->m_arrpPROptExtension.Length()>0)
        {
            DWORD dwSubCounter;

            for(dwSubCounter=0;dwSubCounter<(DWORD) (pPRRSBeingCopied->m_arrpPROptExtension.Length());dwSubCounter++)
            {
                pPROptExtension=new PICSRulesOptExtension;

                if(pPROptExtension==NULL)
                {
                    PICSRulesOutOfMemory();

                    delete pPRRSLocal;

                    return(E_OUTOFMEMORY);
                }

                pPROptExtensionBeingCopied=pPRRSBeingCopied->m_arrpPROptExtension[dwSubCounter];

                pPROptExtension->m_prURLExtensionName.Set(pPROptExtensionBeingCopied->m_prURLExtensionName.Get());
                pPROptExtension->m_etstrShortName.Set(pPROptExtensionBeingCopied->m_etstrShortName.Get());

                pPRRSLocal->m_arrpPROptExtension.Append(pPROptExtension);
            }
        }

        if(pPRRSBeingCopied->m_arrpPRReqExtension.Length()>0)
        {
            DWORD dwSubCounter;

            for(dwSubCounter=0;dwSubCounter<(DWORD) (pPRRSBeingCopied->m_arrpPRReqExtension.Length());dwSubCounter++)
            {
                pPRReqExtension=new PICSRulesReqExtension;

                if(pPRReqExtension==NULL)
                {
                    PICSRulesOutOfMemory();

                    delete pPRRSLocal;

                    return(E_OUTOFMEMORY);
                }

                pPRReqExtensionBeingCopied=pPRRSBeingCopied->m_arrpPRReqExtension[dwSubCounter];

                pPRReqExtension->m_prURLExtensionName.Set(pPRReqExtensionBeingCopied->m_prURLExtensionName.Get());
                pPRReqExtension->m_etstrShortName.Set(pPRReqExtensionBeingCopied->m_etstrShortName.Get());

                pPRRSLocal->m_arrpPRReqExtension.Append(pPRReqExtension);
            }
        }
    
        parrpPRRSDest->Append(pPRRSLocal);
    }

    return(NOERROR);
}

BOOL CALLBACK AdvancedDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PRSD *pPRSD;
    int i;
    const char *p;
    
    static DWORD aIds[] = {
        IDC_TEXT1,          IDH_RATINGS_BUREAU,
        IDC_TEXT2,          IDH_RATINGS_BUREAU,
        IDC_TEXT3,          IDH_RATINGS_BUREAU,
        IDC_3RD_COMBO,      IDH_RATINGS_BUREAU,
        IDC_PICSRULESOPEN,  IDH_PICSRULES_OPEN,
        IDC_PICSRULESEDIT,  IDH_PICSRULES_EDIT,
        IDC_PICSRULES_UP,   IDH_ADVANCED_TAB_UP_ARROW_BUTTON,
        IDC_PICSRULES_DOWN, IDH_ADVANCED_TAB_DOWN_ARROW_BUTTON,
        IDC_PICSRULES_LIST, IDH_PICS_RULES_LIST,
        0,0
    };

    switch (uMsg) {
        case WM_SYSCOLORCHANGE:
        {
            HICON   hIcon, hOldIcon;

            hIcon = (HICON) LoadImage(hInstance,
                                    MAKEINTRESOURCE(IDI_PICSRULES_UP),
                                    IMAGE_ICON,
                                    16,
                                    16,
                                    LR_LOADTRANSPARENT|LR_DEFAULTCOLOR|LR_CREATEDIBSECTION);
            
            hOldIcon=(HICON) SendDlgItemMessage(hDlg,IDC_PICSRULES_UP,BM_SETIMAGE,(WPARAM) IMAGE_ICON,(LPARAM) hIcon);

            if(hOldIcon!=NULL)
            {
                DeleteObject(hOldIcon);
            }
            
            hIcon = (HICON) LoadImage(hInstance,
                                    MAKEINTRESOURCE(IDI_PICSRULES_DOWN),
                                    IMAGE_ICON,
                                    16,
                                    16,
                                    LR_LOADTRANSPARENT|LR_DEFAULTCOLOR|LR_CREATEDIBSECTION);
            
            hOldIcon=(HICON) SendDlgItemMessage(hDlg,IDC_PICSRULES_DOWN,BM_SETIMAGE,(WPARAM) IMAGE_ICON,(LPARAM) hIcon);

            if(hOldIcon!=NULL)
            {
                DeleteObject(hOldIcon);
            }
            break;
        }
        case WM_INITDIALOG:
        {
            HICON   hIcon;
            int     iCounter;

            SetWindowLongPtr (hDlg, DWLP_USER, lParam);
            pPRSD = (PRSD *) ((PROPSHEETPAGE*)lParam)->lParam;

            FillBureauList(hDlg, pPRSD->pPRSI);

            hIcon = (HICON) LoadImage(hInstance,
                                    MAKEINTRESOURCE(IDI_PICSRULES_UP),
                                    IMAGE_ICON,
                                    16,
                                    16,
                                    LR_LOADTRANSPARENT|LR_DEFAULTCOLOR|LR_CREATEDIBSECTION);
            
            SendDlgItemMessage(hDlg,IDC_PICSRULES_UP,BM_SETIMAGE,(WPARAM) IMAGE_ICON,(LPARAM) hIcon);

            hIcon = (HICON) LoadImage(hInstance,
                                    MAKEINTRESOURCE(IDI_PICSRULES_DOWN),
                                    IMAGE_ICON,
                                    16,
                                    16,
                                    LR_LOADTRANSPARENT|LR_DEFAULTCOLOR|LR_CREATEDIBSECTION);
            
            SendDlgItemMessage(hDlg,IDC_PICSRULES_DOWN,BM_SETIMAGE,(WPARAM) IMAGE_ICON,(LPARAM) hIcon);

            g_arrpPICSRulesPRRSPreApply.DeleteAll();

            if(g_lGlobalCounterValue!=SHGlobalCounterGetValue(g_HandleGlobalCounter))
            {
                HRESULT               hRes;
                DWORD                 dwNumSystems;
                PICSRulesRatingSystem * pPRRS=NULL;

                g_arrpPRRS.DeleteAll();

                //someone modified our settings, so we'd better reload them.
                hRes=PICSRulesGetNumSystems(&dwNumSystems);

                if(SUCCEEDED(hRes))
                {
                    DWORD dwCounter;

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

                g_lGlobalCounterValue=SHGlobalCounterGetValue(g_HandleGlobalCounter);
            }

            CopyArrayPRRSStructures(&g_arrpPICSRulesPRRSPreApply,&g_arrpPRRS);

            //fill in the listbox with installed PICSRules systems
            for(iCounter=0;iCounter<g_arrpPICSRulesPRRSPreApply.Length();iCounter++)
            {
                PICSRulesRatingSystem * pPRRSToList;
                char                  * lpszName=NULL;

                pPRRSToList=g_arrpPICSRulesPRRSPreApply[iCounter];

                if((pPRRSToList->m_pPRName)!=NULL)
                {
                    lpszName=pPRRSToList->m_pPRName->m_etstrRuleName.Get();
                }

                if(lpszName==NULL)
                {
                    lpszName=pPRRSToList->m_etstrFile.Get();
                }

                SendDlgItemMessage(hDlg,IDC_PICSRULES_LIST,LB_ADDSTRING,(WPARAM) 0,(LPARAM) lpszName);
            }

            SendDlgItemMessage(hDlg,IDC_PICSRULES_LIST,LB_SETCURSEL,(WPARAM) -1,(LPARAM) 0);
            EnableWindow(GetDlgItem(hDlg,IDC_PICSRULESEDIT),FALSE);

            if(SendDlgItemMessage(hDlg, IDC_PICSRULES_LIST, LB_GETCOUNT, 0, 0) < 2)
            {
                // less than 2 elements in box - disable up and down buttons
                EnableWindow(GetDlgItem(hDlg, IDC_PICSRULES_UP), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_PICSRULES_DOWN), FALSE);
            }

            PostMessage(hDlg,WM_USER,(WPARAM) 0,(LPARAM) 0);

            break;
        }
        case WM_USER:
        {
            if (gPRSI->lpszFileName!=NULL)
            {
                SetFocus(GetDlgItem(hDlg,IDC_PICSRULESOPEN));

                SendMessage(hDlg,WM_COMMAND,(WPARAM) IDC_PICSRULESOPEN,(LPARAM) 0);
                
                gPRSI->lpszFileName=NULL;
            }

            break;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                /*edit controls/check boxes.  User updated these, highlight apply button*/
                case IDC_3RD_COMBO:
                    switch(HIWORD(wParam)) {
                    case CBN_EDITCHANGE:
                    case CBN_SELENDOK:
                        MarkChanged(hDlg);
                        break;
                    }
                    break;

                case IDC_PICSRULES_LIST:
                {
                    switch(HIWORD(wParam))
                    {
                        case LBN_SELCHANGE:
                        {
                            EnableWindow(GetDlgItem(hDlg,IDC_PICSRULESEDIT),TRUE);

                            break;
                        }
                    }

                    break;
                }
                case IDC_PICSRULES_UP:
                {
                    int                     iSelected;
                    PICSRulesRatingSystem * pPRRSToMove;
                    char                  * lpszName=NULL;

                    iSelected= (int) SendDlgItemMessage(hDlg,
                                                        IDC_PICSRULES_LIST,
                                                        LB_GETCURSEL,
                                                        (WPARAM) 0,
                                                        (LPARAM) 0);

                    if(iSelected==LB_ERR)
                    {
                        break;
                    }

                    if(iSelected==0)
                    {
                        //already at the top

                        break;
                    }

                    pPRRSToMove=g_arrpPICSRulesPRRSPreApply[iSelected];

                    g_arrpPICSRulesPRRSPreApply[iSelected]=
                        g_arrpPICSRulesPRRSPreApply[iSelected-1];

                    g_arrpPICSRulesPRRSPreApply[iSelected-1]=pPRRSToMove;

                    //update the listbox
                    SendDlgItemMessage(hDlg,
                                       IDC_PICSRULES_LIST,
                                       LB_DELETESTRING,
                                       (WPARAM) iSelected,
                                       (LPARAM) 0);

                    SendDlgItemMessage(hDlg,
                                       IDC_PICSRULES_LIST,
                                       LB_DELETESTRING,
                                       (WPARAM) iSelected-1,
                                       (LPARAM) 0);

                    pPRRSToMove=g_arrpPICSRulesPRRSPreApply[iSelected-1];

                    if((pPRRSToMove->m_pPRName)!=NULL)
                    {
                        lpszName=pPRRSToMove->m_pPRName->m_etstrRuleName.Get();
                    }

                    if(lpszName==NULL)
                    {
                        lpszName=pPRRSToMove->m_etstrFile.Get();
                    }

                    SendDlgItemMessage(hDlg,
                                       IDC_PICSRULES_LIST,
                                       LB_INSERTSTRING,
                                       (WPARAM) iSelected-1,
                                       (LPARAM) lpszName);

                    pPRRSToMove=g_arrpPICSRulesPRRSPreApply[iSelected];

                    if((pPRRSToMove->m_pPRName)!=NULL)
                    {
                        lpszName=pPRRSToMove->m_pPRName->m_etstrRuleName.Get();
                    }

                    if(lpszName==NULL)
                    {
                        lpszName=pPRRSToMove->m_etstrFile.Get();
                    }

                    SendDlgItemMessage(hDlg,
                                       IDC_PICSRULES_LIST,
                                       LB_INSERTSTRING,
                                       (WPARAM) iSelected,
                                       (LPARAM) lpszName);

                    SendDlgItemMessage(hDlg,
                                       IDC_PICSRULES_LIST,
                                       LB_SETCURSEL,
                                       (WPARAM) iSelected-1,
                                       (LPARAM) 0);

                    MarkChanged(hDlg);

                    break;
                }
                case IDC_PICSRULES_DOWN:
                {
                    int                     iSelected, iNumItems;
                    PICSRulesRatingSystem * pPRRSToMove;
                    char                  * lpszName=NULL;

                    iSelected= (int) SendDlgItemMessage(hDlg,
                                                        IDC_PICSRULES_LIST,
                                                        LB_GETCURSEL,
                                                        (WPARAM) 0,
                                                        (LPARAM) 0);

                    if(iSelected==LB_ERR)
                    {
                        break;
                    }

                    iNumItems= (int) SendDlgItemMessage(hDlg,
                                                        IDC_PICSRULES_LIST,
                                                        LB_GETCOUNT,
                                                        (WPARAM) 0,
                                                        (LPARAM) 0);

                    if(iNumItems==LB_ERR)
                    {
                        break;
                    }

                    if(iSelected==(iNumItems-1))
                    {
                        //already at the bottom

                        break;
                    }

                    pPRRSToMove=g_arrpPICSRulesPRRSPreApply[iSelected];

                    g_arrpPICSRulesPRRSPreApply[iSelected]=
                        g_arrpPICSRulesPRRSPreApply[iSelected+1];

                    g_arrpPICSRulesPRRSPreApply[iSelected+1]=pPRRSToMove;

                    //update the listbox
                    SendDlgItemMessage(hDlg,
                                       IDC_PICSRULES_LIST,
                                       LB_DELETESTRING,
                                       (WPARAM) iSelected+1,
                                       (LPARAM) 0);

                    SendDlgItemMessage(hDlg,
                                       IDC_PICSRULES_LIST,
                                       LB_DELETESTRING,
                                       (WPARAM) iSelected,
                                       (LPARAM) 0);

                    pPRRSToMove=g_arrpPICSRulesPRRSPreApply[iSelected];

                    if((pPRRSToMove->m_pPRName)!=NULL)
                    {
                        lpszName=pPRRSToMove->m_pPRName->m_etstrRuleName.Get();
                    }

                    if(lpszName==NULL)
                    {
                        lpszName=pPRRSToMove->m_etstrFile.Get();
                    }

                    SendDlgItemMessage(hDlg,
                                       IDC_PICSRULES_LIST,
                                       LB_INSERTSTRING,
                                       (WPARAM) iSelected,
                                       (LPARAM) lpszName);

                    pPRRSToMove=g_arrpPICSRulesPRRSPreApply[iSelected+1];

                    if((pPRRSToMove->m_pPRName)!=NULL)
                    {
                        lpszName=pPRRSToMove->m_pPRName->m_etstrRuleName.Get();
                    }

                    if(lpszName==NULL)
                    {
                        lpszName=pPRRSToMove->m_etstrFile.Get();
                    }

                    SendDlgItemMessage(hDlg,
                                       IDC_PICSRULES_LIST,
                                       LB_INSERTSTRING,
                                       (WPARAM) iSelected+1,
                                       (LPARAM) lpszName);

                    SendDlgItemMessage(hDlg,
                                       IDC_PICSRULES_LIST,
                                       LB_SETCURSEL,
                                       (WPARAM) iSelected+1,
                                       (LPARAM) 0);

                    MarkChanged(hDlg);

                    break;
                }
                case IDC_PICSRULESEDIT:
                {
                    int     iSelected, iCounter;
                    array<PICSRulesRatingSystem*> g_arrpPICSRulesPRRSNew;


                    iSelected= (int) SendDlgItemMessage(hDlg,
                                                        IDC_PICSRULES_LIST,
                                                        LB_GETCURSEL,
                                                        (WPARAM) 0,
                                                        (LPARAM) 0);

                    if(iSelected==LB_ERR)
                    {
                        break;
                    }

                    SendDlgItemMessage(hDlg,
                                       IDC_PICSRULES_LIST,
                                       LB_DELETESTRING,
                                       (WPARAM) iSelected,
                                       (LPARAM) 0);

                    // If there's less than two left, turn off up and down buttons
                    if(SendDlgItemMessage(hDlg, IDC_PICSRULES_LIST, LB_GETCOUNT, 0, 0) < 2)
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_PICSRULES_UP), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_PICSRULES_DOWN), FALSE);
                    }

                    delete (g_arrpPICSRulesPRRSPreApply[iSelected]);

                    g_arrpPICSRulesPRRSPreApply[iSelected]=NULL;

                    for(iCounter=0;iCounter<g_arrpPICSRulesPRRSPreApply.Length();iCounter++)
                    {
                        PICSRulesRatingSystem * pPRRSToMove;

                        pPRRSToMove=g_arrpPICSRulesPRRSPreApply[iCounter];

                        if(pPRRSToMove!=NULL)
                        {
                            g_arrpPICSRulesPRRSNew.Append(pPRRSToMove);
                        }
                    }

                    g_arrpPICSRulesPRRSPreApply.ClearAll();

                    for(iCounter=0;iCounter<g_arrpPICSRulesPRRSNew.Length();iCounter++)
                    {
                        PICSRulesRatingSystem * pPRRSToMove;

                        pPRRSToMove=g_arrpPICSRulesPRRSNew[iCounter];

                        g_arrpPICSRulesPRRSPreApply.Append(pPRRSToMove);
                    }

                    g_arrpPICSRulesPRRSNew.ClearAll();

                    EnableWindow(GetDlgItem(hDlg,IDC_PICSRULESEDIT),FALSE);
                    MarkChanged(hDlg);
                    SetFocus(GetDlgItem(hDlg,IDC_PICSRULESOPEN));

                    SendDlgItemMessage(hDlg,
                                       IDC_PICSRULESOPEN,
                                       BM_SETSTYLE,
                                       (WPARAM) BS_DEFPUSHBUTTON,
                                       (LPARAM) MAKELPARAM(TRUE,0));

                    SendDlgItemMessage(hDlg,
                                       IDC_PICSRULESEDIT,
                                       BM_SETSTYLE,
                                       (WPARAM) BS_PUSHBUTTON,
                                       (LPARAM) MAKELPARAM(TRUE,0));
                    break;
                }
                case IDC_PICSRULESOPEN:
                {
                    OPENFILENAME            OpenFileName;
                    char                    *szFile,*szFileTitle,*szDlgTitle,*szDlgFilter,
                                            *szDlgFilterPtr;
                    BOOL                    bExists=FALSE,fPICSRulesSaved=FALSE,fHaveFile=FALSE;
                    int                     iReplaceInstalled=IDYES;
                    PICSRulesRatingSystem   *pPRRS;

                    pPRSD=(PRSD *)((PROPSHEETPAGE*)GetWindowLongPtr(hDlg,DWLP_USER))->lParam;

                    if(!pPRSD)
                    {
                        WCHAR   szErrorMessage[MAX_PATH],szErrorTitle[MAX_PATH];

                        MLLoadString(IDS_ERROR,(LPTSTR) szErrorTitle,MAX_PATH);
                        MLLoadString(IDS_PICSRULES_SYSTEMERROR,(LPTSTR) szErrorMessage,MAX_PATH);
                        MessageBox(NULL,(LPCTSTR) szErrorMessage,(LPCTSTR) szErrorTitle,MB_OK|MB_ICONERROR);

                        break;
                    }

                    szFile=(char *) GlobalAlloc(GPTR,MAX_PATH);
                    szFileTitle=(char *) GlobalAlloc(GPTR,MAX_PATH);
                    szDlgTitle=(char *) GlobalAlloc(GPTR,MAX_PATH);
                    szDlgFilter=(char *) GlobalAlloc(GPTR,MAX_PATH);

                    if(gPRSI->lpszFileName!=NULL)
                    {
                        fHaveFile=TRUE;
                        lstrcpy(szFile,gPRSI->lpszFileName);
                    }
                    else
                    {
                        MLLoadString(IDS_OPENDIALOGFILTER,(LPTSTR) szDlgFilter,MAX_PATH);
                        MLLoadString(IDS_OPENDIALOGTITLE,(LPTSTR) szDlgTitle,MAX_PATH);

                        szDlgFilterPtr=&szDlgFilter[lstrlen(szDlgFilter)+1];

                        lstrcpy(szDlgFilterPtr,"*.prf");

                        OpenFileName.lStructSize       =sizeof(OPENFILENAME); 
                        OpenFileName.hwndOwner         =hDlg; 
                        OpenFileName.hInstance         =pPRSD->hInst; 
                        OpenFileName.lpstrFilter       =szDlgFilter; 
                        OpenFileName.lpstrCustomFilter =(LPTSTR) NULL; 
                        OpenFileName.nMaxCustFilter    =0L; 
                        OpenFileName.nFilterIndex      =1L; 
                        OpenFileName.lpstrFile         =szFile; 
                        OpenFileName.nMaxFile          =MAX_PATH; 
                        OpenFileName.lpstrFileTitle    =szFileTitle; 
                        OpenFileName.nMaxFileTitle     =MAX_PATH; 
                        OpenFileName.lpstrInitialDir   =NULL; 
                        OpenFileName.lpstrTitle        =szDlgTitle; 
                        OpenFileName.nFileOffset       =0; 
                        OpenFileName.nFileExtension    =0; 
                        OpenFileName.lpstrDefExt       =NULL; 
                        OpenFileName.lCustData         =0; 
                        OpenFileName.Flags             =OFN_FILEMUSTEXIST
                                                        |OFN_PATHMUSTEXIST
                                                        |OFN_HIDEREADONLY;
                        
                        fHaveFile=GetOpenFileName(&OpenFileName);
                    }    
                    
                    if(fHaveFile==TRUE) 
                    {
                        HRESULT hRes;

                        //create PICSRulesRatingSystem class, and pass to the
                        //import procedure with file name

                        hRes=PICSRulesImport(szFile,&pPRRS);

                        if(SUCCEEDED(hRes))
                        {
                            LPSTR   lpszPICSRulesSystemName,lpszNewSystemName;
                            DWORD   dwSystemToSave,dwNumSystemsInstalled,dwCounter;
                            BOOL    fPromptToOverwrite=FALSE;

                            lpszNewSystemName=pPRRS->m_pPRName->m_etstrRuleName.Get();

                            dwNumSystemsInstalled=g_arrpPICSRulesPRRSPreApply.Length();

                            //Systems below PICSRULES_FIRSTSYSTEMINDEX are
                            //reserved for the Approved Sites Rules, and
                            //future expansion.
                            for(dwCounter=0;dwCounter<dwNumSystemsInstalled;dwCounter++)
                            {
                                //check to see if the system we just processed
                                //is already installed
                                lpszPICSRulesSystemName=(g_arrpPICSRulesPRRSPreApply[dwCounter])->m_pPRName->m_etstrRuleName.Get();

                                if(lstrcmp(lpszPICSRulesSystemName,lpszNewSystemName)==0)
                                {
                                    //We've found an identical system, so set dwSystemToSave
                                    //to dwCounter and set fPromptToOverwrite to TRUE
                                    fPromptToOverwrite=TRUE;
                                    
                                    break;
                                }
                            }

                            dwSystemToSave=dwCounter+PICSRULES_FIRSTSYSTEMINDEX;

                            if(fPromptToOverwrite)
                            {
                                char    *lpszTitle,*lpszMessage;

                                //We found a system which already exists, so lets
                                //prompt the user to replace those settings.

                                lpszTitle=(char *) GlobalAlloc(GPTR,MAX_PATH);
                                lpszMessage=(char *) GlobalAlloc(GPTR,MAX_PATH);

                                MLLoadString(IDS_PICSRULES_EXISTSTITLE,(LPTSTR) lpszTitle,MAX_PATH);
                                MLLoadString(IDS_PICSRULES_EXISTSMESSAGE,(LPTSTR) lpszMessage,MAX_PATH);

                                iReplaceInstalled=MessageBox(NULL,(LPCTSTR) lpszMessage,(LPCTSTR) lpszTitle,MB_YESNO|MB_ICONERROR);

                                GlobalFree(lpszTitle);
                                GlobalFree(lpszMessage);
                            }

                            if(iReplaceInstalled==IDNO)
                            {
                                GlobalFree(szFileTitle);
                                GlobalFree(szFile);
                                GlobalFree(szDlgTitle);
                                GlobalFree(szDlgFilter);

                                delete pPRRS;

                                pPRRS=NULL;
                                g_pPRRS=NULL;

                                break;
                            }               

                            if((dwSystemToSave-PICSRULES_FIRSTSYSTEMINDEX)<(DWORD) (g_arrpPICSRulesPRRSPreApply.Length()))
                            {
                                char * lpszName=NULL;

                                delete g_arrpPICSRulesPRRSPreApply[dwSystemToSave-PICSRULES_FIRSTSYSTEMINDEX];
                                g_arrpPICSRulesPRRSPreApply[dwSystemToSave-PICSRULES_FIRSTSYSTEMINDEX]=pPRRS;

                                //update the listbox
                                if((pPRRS->m_pPRName)!=NULL)
                                {
                                    lpszName=pPRRS->m_pPRName->m_etstrRuleName.Get();
                                }

                                if(lpszName==NULL)
                                {
                                    lpszName=pPRRS->m_etstrFile.Get();
                                }

                                SendDlgItemMessage(hDlg,
                                                   IDC_PICSRULES_LIST,
                                                   LB_DELETESTRING,
                                                   (WPARAM) dwSystemToSave-PICSRULES_FIRSTSYSTEMINDEX,
                                                   (LPARAM) 0);

                                SendDlgItemMessage(hDlg,
                                                   IDC_PICSRULES_LIST,
                                                   LB_INSERTSTRING,
                                                   (WPARAM) dwSystemToSave-PICSRULES_FIRSTSYSTEMINDEX,
                                                   (LPARAM) lpszName);
                            }
                            else
                            {
                                char * lpszName=NULL;

                                g_arrpPICSRulesPRRSPreApply.Append(pPRRS);

                                //update the listbox
                                if((pPRRS->m_pPRName)!=NULL)
                                {
                                    lpszName=pPRRS->m_pPRName->m_etstrRuleName.Get();
                                }

                                if(lpszName==NULL)
                                {
                                    lpszName=pPRRS->m_etstrFile.Get();
                                }

                                SendDlgItemMessage(hDlg,
                                                   IDC_PICSRULES_LIST,
                                                   LB_ADDSTRING,
                                                   (WPARAM) 0,
                                                   (LPARAM) lpszName);

                                // if there's now more than one element,
                                // turn on up and down buttons
                                if(SendDlgItemMessage(hDlg, IDC_PICSRULES_LIST,
                                                    LB_GETCOUNT, 0, 0) > 1)
                                {
                                    EnableWindow(GetDlgItem(hDlg, IDC_PICSRULES_UP), TRUE);
                                    EnableWindow(GetDlgItem(hDlg, IDC_PICSRULES_DOWN), TRUE);
                                }
                            }

                            fPICSRulesSaved=TRUE;

                            g_dwNumSystems=g_arrpPICSRulesPRRSPreApply.Length();

                            MarkChanged(hDlg);
                        }
                        else
                        {
                            pPRRS->ReportError(hRes);

                            delete pPRRS;

                            pPRRS=NULL;
                        }

                        if(fPICSRulesSaved==FALSE)
                        {
                            if(pPRRS!=NULL)
                            {
                                char    *lpszTitle,*lpszMessage;

                                //we successfully processed the PICSRules, but couldn't
                                //save them

                                lpszTitle=(char *) GlobalAlloc(GPTR,MAX_PATH);
                                lpszMessage=(char *) GlobalAlloc(GPTR,MAX_PATH);

                                MLLoadString(IDS_PICSRULES_ERRORSAVINGTITLE,(LPTSTR) lpszTitle,MAX_PATH);
                                MLLoadString(IDS_PICSRULES_ERRORSAVINGMSG,(LPTSTR) lpszMessage,MAX_PATH);

                                MessageBox(NULL,(LPCTSTR) lpszMessage,(LPCTSTR) lpszTitle,MB_OK|MB_ICONERROR);

                                GlobalFree(lpszTitle);
                                GlobalFree(lpszMessage);

                                delete pPRRS;

                                pPRRS=NULL;
                            }
                        }
                        else
                        {
                            char    *lpszTitle,*lpszMessage;

                            //Success!  Notify the user.

                            lpszTitle=(char *) GlobalAlloc(GPTR,MAX_PATH);
                            lpszMessage=(char *) GlobalAlloc(GPTR,MAX_PATH);

                            MLLoadString(IDS_PICSRULES_SUCCESSTITLE,(LPTSTR) lpszTitle,MAX_PATH);
                            MLLoadString(IDS_PICSRULES_SUCCESSMESSAGE,(LPTSTR) lpszMessage,MAX_PATH);

                            MessageBox(NULL,(LPCTSTR) lpszMessage,(LPCTSTR) lpszTitle,MB_OK);

                            GlobalFree(lpszTitle);
                            GlobalFree(lpszMessage);
                        }
                    }

                    GlobalFree(szFileTitle);
                    GlobalFree(szFile);
                    GlobalFree(szDlgTitle);
                    GlobalFree(szDlgFilter);

                    g_pPRRS=NULL; //done processing the current system, so make sure it
                                  //doesn't point to anything

                    break;
                }
            }

            return TRUE;

        case WM_NOTIFY: {
            NMHDR *lpnm = (NMHDR *) lParam;
            switch (lpnm->code) {
                /*save us*/
                case PSN_SETACTIVE:
                {
                    pPRSD = (PRSD *) ((PROPSHEETPAGE*)GetWindowLongPtr (hDlg, DWLP_USER))->lParam;

                    if(pPRSD->fNewProviders==TRUE)
                    {
                        FillBureauList(hDlg, pPRSD->pPRSI);
                    }

                    break;
                }
                case PSN_APPLY:
                {
                    DWORD dwNumExistingSystems,dwCounter;

                    /*do apply stuff*/
                    pPRSD = (PRSD *) ((PROPSHEETPAGE*)GetWindowLongPtr (hDlg, DWLP_USER))->lParam;

                    /* Get the text from the rating bureau combo box. */
                    /* Default for etstrRatingBureau (the URL) will be this text. */
                    NLS_STR nlsURL;

                    if (nlsURL.realloc(GetWindowTextLength(GetDlgItem(hDlg, IDC_3RD_COMBO)) + 1)) {
                        GetDlgItemText(hDlg, IDC_3RD_COMBO, nlsURL.Party(), nlsURL.QueryAllocSize());
                        nlsURL.DonePartying();
                        p = (char *)nlsURL.QueryPch();

                        INT_PTR temp = SendDlgItemMessage(hDlg, IDC_3RD_COMBO, CB_GETCURSEL, 0, 0L);
                        if (temp != CB_ERR) {
                            /* Get the text of the selected item in the list. */
                            UINT cbName = (UINT)SendDlgItemMessage(hDlg, IDC_3RD_COMBO, CB_GETLBTEXTLEN, temp, 0);
                            NLS_STR nlsName(cbName+1);
                            if (nlsName.QueryError())
                                p = NULL;
                            else {
                                /* If the text of the selected item in the list
                                 * is what's in the edit field, then the user
                                 * has selected one of those rating system names.
                                 * The itemdata for the item is the URL.
                                 */
                                SendDlgItemMessage(hDlg, IDC_3RD_COMBO, CB_GETLBTEXT, temp, (LPARAM)(LPSTR)nlsName.Party());
                                nlsName.DonePartying();
                                if (nlsName == nlsURL)
                                    p = (char *)SendDlgItemMessage(hDlg, IDC_3RD_COMBO, CB_GETITEMDATA, temp, 0L);
                            }
                        }
                    }
                    else
                        p = NULL;

                    if (pPRSD->pPRSI->etstrRatingBureau.fIsInit() && (!p ||
                        strcmpf(pPRSD->pPRSI->etstrRatingBureau.Get(),p)))
                    {
                        // check if old bureau is required
                        for (i = 0; i <pPRSD->pPRSI->arrpPRS.Length(); ++i)
                        {
                            if (pPRSD->pPRSI->arrpPRS[i]->etstrRatingBureau.fIsInit() &&
                                (!strcmpf(pPRSD->pPRSI->etstrRatingBureau.Get(),
                                          pPRSD->pPRSI->arrpPRS[i]->etstrRatingBureau.Get())))
                            {
                                if (!(pPRSD->pPRSI->arrpPRS[i]->etbBureauRequired.Get()))
                                {
                                    break; // Not required.  We're done.
                                }      
                                      
                                //We're removing a bureau that's required.  Warn user.
                                char pszBuf[MAXPATHLEN];
                                char szTemp[MAXPATHLEN];

                                MLLoadStringA(IDS_NEEDBUREAU, pszBuf, sizeof(pszBuf));
                                wsprintf(szTemp, pszBuf, pPRSD->pPRSI->arrpPRS[i]->etstrName.Get());
                                if (MessageBox(hDlg, szTemp, NULL, MB_YESNO) == IDNO)
                                    return FALSE;
                                else
                                    break;   
                            }
                        }
                    }

                    if (!p || !*p)
                    {
                        pPRSD->pPRSI->etstrRatingBureau.Set(NULL);
                        DeinstallRatingBureauHelper();
                    }
                    else
                    {
                        pPRSD->pPRSI->etstrRatingBureau.Set((LPSTR)p);
                        InstallRatingBureauHelper();
                    }    

                    /* Update the rating helper list to include or not include
                     * the rating bureau helper.
                     */
                    CleanupRatingHelpers();
                    InitRatingHelpers();

                    //process PICSRules

                    PICSRulesGetNumSystems(&dwNumExistingSystems);

                    for(dwCounter=0;dwCounter<dwNumExistingSystems;dwCounter++)
                    {
                        //delete all existing systems from the registry
                        PICSRulesDeleteSystem(dwCounter+PICSRULES_FIRSTSYSTEMINDEX);
                    }

                    g_arrpPRRS.DeleteAll();

                    CopyArrayPRRSStructures(&g_arrpPRRS,&g_arrpPICSRulesPRRSPreApply);

                    for(dwCounter=0;dwCounter<(DWORD) (g_arrpPRRS.Length());dwCounter++)
                    {
                        g_pPRRS=g_arrpPRRS[dwCounter];

                        PICSRulesSaveToRegistry(dwCounter+PICSRULES_FIRSTSYSTEMINDEX,&g_arrpPRRS[dwCounter]);
                    }

                    PICSRulesSetNumSystems(g_arrpPICSRulesPRRSPreApply.Length());

                    SHGlobalCounterIncrement(g_HandleGlobalCounter);

                    if (!((PSHNOTIFY*) lpnm)->lParam) break;

                    return TRUE;
                }
                case PSN_RESET:
                {
                    return TRUE;
                }
            }
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
    }

    return FALSE;
}

#define NUM_PAGES 4

void MakePropPage(PropSheet *ps, PRSD *pPRSD, DLGPROC pProc, UINT idd)
{
   ps->psPage.lParam      = (LPARAM)pPRSD;
   ps->psPage.pfnDlgProc  = pProc;
   ps->psPage.pszTemplate = MAKEINTRESOURCE(idd);

   ps->psHeader.phpage[ps->psHeader.nPages] = CreatePropertySheetPage(&ps->psPage);
   if (ps->psHeader.phpage[ps->psHeader.nPages])
       ps->psHeader.nPages++;
}


BOOL PicsOptionsDialog(HWND hwnd, HINSTANCE hinst, PicsRatingSystemInfo *pPRSI, PicsUser *pPU)
{
    PropSheet            ps;
    PRSD                 *pPRSD;
    char                 pszBuf[MAXPATHLEN];
    BOOL                 fRet = FALSE;
    INITCOMMONCONTROLSEX ex;

    ex.dwSize = sizeof(ex);
    ex.dwICC = ICC_NATIVEFNTCTL_CLASS;
    InitCommonControlsEx(&ex);

    MLLoadStringA(IDS_GENERIC, pszBuf, sizeof(pszBuf));
    ps.Init(hwnd, hinst, NUM_PAGES, pszBuf, TRUE);

    pPRSD = new PRSD;
    if (!pPRSD) return FALSE;

    pPRSD->pPU                = pPU;
    pPRSD->pTempRatings       = NULL;
    pPRSD->hwndBitmapCategory = NULL;
    pPRSD->hwndBitmapLabel    = NULL;
    pPRSD->hInst              = hinst;
    pPRSD->pPRSI              = pPRSI;
    pPRSD->fNewProviders      = FALSE;

    MakePropPage(&ps, pPRSD, (DLGPROC)PicsDlgProc, IDD_RATINGS);
    MakePropPage(&ps, pPRSD, (DLGPROC)ApprovedSitesDlgProc, IDD_APPROVEDSITES);
    MakePropPage(&ps, pPRSD, (DLGPROC)GeneralDlgProc, IDD_GENERAL);
    MakePropPage(&ps, pPRSD, (DLGPROC)AdvancedDlgProc, IDD_ADVANCED);
    
    if (ps.psHeader.nPages == NUM_PAGES)
        fRet = ps.Run();

    ImageList_Destroy(g_hImageList);

    delete pPRSD;

    return fRet;
}

void EnableDlgItems(HWND hDlg)
{
    CHAR pszBuf[MAXPATHLEN];
    PicsUser *pUser = ::GetUserObject();

        if (!gPRSI->fRatingInstalled)
        {
            MLLoadStringA(IDS_RATING_NEW, pszBuf, sizeof(pszBuf));
            SetDlgItemText(hDlg, IDC_INTRO_TEXT, pszBuf);

            MLLoadStringA(IDS_TURN_ONOFF, pszBuf, sizeof(pszBuf));
            SetDlgItemText(hDlg, IDC_TURN_ONOFF, pszBuf);
            EnableWindow(GetDlgItem(hDlg, IDC_TURN_ONOFF), FALSE);
        }
        else if (pUser && pUser->fEnabled)
        {
            MLLoadStringA(IDS_RATING_ON, pszBuf, sizeof(pszBuf));
            SetDlgItemText(hDlg, IDC_INTRO_TEXT, pszBuf);

            MLLoadStringA(IDS_TURN_OFF, pszBuf, sizeof(pszBuf));
            SetDlgItemText(hDlg, IDC_TURN_ONOFF, pszBuf);
            EnableWindow(GetDlgItem(hDlg, IDC_TURN_ONOFF), TRUE);
        }
        else
        {
            MLLoadStringA(IDS_RATING_OFF, pszBuf, sizeof(pszBuf));
            SetDlgItemText(hDlg, IDC_INTRO_TEXT, pszBuf);

            MLLoadStringA(IDS_TURN_ON, pszBuf, sizeof(pszBuf));
            SetDlgItemText(hDlg, IDC_TURN_ONOFF, pszBuf);
            EnableWindow(GetDlgItem(hDlg, IDC_TURN_ONOFF), TRUE);
        }
}


const UINT PASSCONFIRM_FAIL = 0;
const UINT PASSCONFIRM_OK = 1;
const UINT PASSCONFIRM_NEW = 2;

INT_PTR DoPasswordConfirm(HWND hDlg)
{
    if (SUCCEEDED(VerifySupervisorPassword(szNULL))) {
        return DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_PASSWORD), hDlg, PasswordDialogProc);
    }
    else {
        return DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_CREATE_PASSWORD), hDlg, ChangePasswordDialogProc)
                ? PASSCONFIRM_NEW : PASSCONFIRM_FAIL;
    }
}

#define NO_EXISTING_PASSWORD PASSCONFIRM_NEW

UINT_PTR DoExistingPasswordConfirm(HWND hDlg,BOOL * fExistingPassword)
{
    if (SUCCEEDED(VerifySupervisorPassword(szNULL)))
    {
        *fExistingPassword=TRUE;
    
        return DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_PASSWORD), hDlg, (DLGPROC) PasswordDialogProc);
    }
    else
    {
        *fExistingPassword=FALSE;

        return(NO_EXISTING_PASSWORD);
    }
}


INT_PTR CALLBACK IntroDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static DWORD aIds[] = {
        IDC_SET_RATINGS,    IDH_RATINGS_SET_RATINGS_BUTTON,
        IDC_TURN_ONOFF,     IDH_RATINGS_TURNON_BUTTON,
        0,0
    };

    switch (uMsg) {
        case WM_INITDIALOG:
            CheckGlobalInfoRev();
            EnableDlgItems(hDlg);
            break;

        case WM_NOTIFY:
            {
                NMHDR *lpnm = (NMHDR *) lParam;
                switch (lpnm->code) {
                case PSN_APPLY:
                    gPRSI->fSettingsValid = TRUE;
                    gPRSI->SaveRatingSystemInfo();
                    return TRUE;
                }
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
#if 0   /* no OK button in prop sheet page */
                case IDOK:
                    EndDialog(hDlg, TRUE);
                    break;
#endif

                case IDC_SET_RATINGS:
                {
                    UINT_PTR passConfirm = DoPasswordConfirm(hDlg);
                    if (passConfirm == PASSCONFIRM_FAIL)
                        break;

                    if (!gPRSI->fRatingInstalled)
                    {
                        gPRSI->FreshInstall();
                        if (!PicsOptionsDialog(hDlg, hInstance, gPRSI, GetUserObject())) {
                            if (passConfirm == PASSCONFIRM_NEW) {
                                ::RemoveSupervisorPassword();
                                gPRSI->fRatingInstalled = FALSE;
                            }
                        }
                        MarkChanged(hDlg);
                    }
                    else
                    {
                        if (PicsOptionsDialog(hDlg, hInstance, gPRSI, GetUserObject()))
                            MarkChanged(hDlg);
                    }
                    EnableDlgItems(hDlg);
                    
                    break;
                }

                case IDC_TURN_ONOFF:
                    if (DoPasswordConfirm(hDlg))
                    {
                        PicsUser *pUser = ::GetUserObject();
                        if (pUser != NULL) {
                            pUser->fEnabled = !pUser->fEnabled;
                            if (pUser->fEnabled)
                                MyMessageBox(hDlg, IDS_NOW_ON, IDS_ENABLE_WARNING,
                                             IDS_GENERIC, MB_OK);
                            else
                            {
                                // Delete the supervisor key so that we don't load
                                // ratings by other components and we "quick" know it is off.
                                HKEY hkeyRatings = NULL;
                                if (RegOpenKey(HKEY_LOCAL_MACHINE, szRATINGS, &hkeyRatings) == ERROR_SUCCESS)
                                {
                                    unsigned long lType;
                                    DWORD         dwFlag,dwSizeOfFlag=sizeof(dwFlag);

                                    RegDeleteValue(hkeyRatings, szRatingsSupervisorKeyName);

                                    if(RegQueryValueEx(hkeyRatings,(char *) &szTURNOFF,NULL,&lType,(LPBYTE) &dwFlag,&dwSizeOfFlag)==ERROR_SUCCESS)
                                    {
                                        if(dwFlag!=1)
                                        {
                                            DialogBox(MLGetHinst(),
                                                            MAKEINTRESOURCE(IDD_TURNOFF),
                                                            hDlg,
                                                            (DLGPROC) TurnOffDialogProc);
                                        }
                                    }
                                    else
                                    {
                                        DialogBox(MLGetHinst(),
                                                        MAKEINTRESOURCE(IDD_TURNOFF),
                                                        hDlg,
                                                        (DLGPROC) TurnOffDialogProc);
                                    }

                                    RegCloseKey(hkeyRatings);
                                }
                            }
                            EnableDlgItems(hDlg);
                        }
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
    }
    return FALSE;
}

VOID SetErrorFocus(HWND hDlg, UINT idCtrl)
{
    HWND hCtrl = ::GetDlgItem(hDlg, idCtrl);
    ::SetFocus(hCtrl);
    ::SendMessage(hCtrl, EM_SETSEL, 0, -1);
}

INT_PTR CALLBACK PasswordDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static DWORD aIds[] = {
        IDC_STATIC1,    IDH_RATINGS_SUPERVISOR_PASSWORD,
        IDC_STATIC2,    IDH_RATINGS_SUPERVISOR_PASSWORD,
        IDC_PASSWORD,   IDH_RATINGS_SUPERVISOR_PASSWORD,
        0,0
    };

    CHAR pszPassword[MAXPATHLEN];
    HRESULT hRet;

    switch (uMsg) {
        case WM_INITDIALOG:
        {
            if(GetDlgItem(hDlg,IDC_PASSWORD)!=NULL)
            {
                SendDlgItemMessage(hDlg,IDC_PASSWORD,EM_SETLIMITTEXT,(WPARAM) RATINGS_MAX_PASSWORD_LENGTH,(LPARAM) 0);
            }

            return(TRUE);
        }
        case WM_COMMAND:
            switch (LOWORD(wParam))  {
            case IDCANCEL:
                    EndDialog(hDlg, PASSCONFIRM_FAIL);
                    break;

                case IDOK:
                    GetDlgItemText(hDlg, IDC_PASSWORD, pszPassword, sizeof(pszPassword));

                    hRet = VerifySupervisorPassword(pszPassword);

                    if (hRet == (NOERROR)) {
                        /* In case we got the users-CPL-supervisor password,
                         * make sure we write it to the ratings location in
                         * the registry too.
                         */
                        if (ChangeSupervisorPassword(pszPassword, pszPassword) == S_FALSE)
                            EndDialog(hDlg, PASSCONFIRM_NEW);
                        else
                            EndDialog(hDlg, PASSCONFIRM_OK);
                    }
                    else
                    {
                        MyMessageBox(hDlg, IDS_BADPASSWORD, IDS_GENERIC, MB_OK|MB_ICONERROR);    
                        SetErrorFocus(hDlg, IDC_PASSWORD);
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
    }
    return FALSE;
}

BOOL CALLBACK TurnOffDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_INITDIALOG:
        {
            SendDlgItemMessage(hDlg,IDC_ADVISOR_OFF_CHECK,BM_SETCHECK,(WPARAM) BST_UNCHECKED,(LPARAM) 0);

            return(TRUE);
        }
        case WM_COMMAND:
            switch (LOWORD(wParam))  {
                case IDOK:
                {
                    if(BST_CHECKED==SendDlgItemMessage(hDlg,
                                                       IDC_ADVISOR_OFF_CHECK,
                                                       BM_GETCHECK,
                                                       (WPARAM) 0,
                                                       (LPARAM) 0))
                    {
                        HKEY  hKey;
                        DWORD dwTurnOff=1;

                        RegCreateKey(HKEY_LOCAL_MACHINE,szRATINGS,&hKey);

                        RegSetValueEx(hKey,(char *) &szTURNOFF,0,REG_DWORD,(LPBYTE)&dwTurnOff,sizeof(dwTurnOff));
                    }
                    
                    EndDialog(hDlg,TRUE);
                }
            }
            break;
    }
    return FALSE;
}

INT_PTR CALLBACK ChangePasswordDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static DWORD aIds[] = {
        IDC_STATIC1,            IDH_RATINGS_CHANGE_PASSWORD_OLD,
        IDC_OLD_PASSWORD,       IDH_RATINGS_CHANGE_PASSWORD_OLD,
        IDC_STATIC2,            IDH_RATINGS_CHANGE_PASSWORD_NEW,
        IDC_PASSWORD,           IDH_RATINGS_CHANGE_PASSWORD_NEW,
        IDC_STATIC4,            IDH_RATINGS_SUPERVISOR_CREATE_PASSWORD,
        IDC_CREATE_PASSWORD,    IDH_RATINGS_SUPERVISOR_CREATE_PASSWORD,
        IDC_STATIC3,            IDH_RATINGS_CHANGE_PASSWORD_CONFIRM,
        IDC_CONFIRM_PASSWORD,   IDH_RATINGS_CHANGE_PASSWORD_CONFIRM,
        0,0
    };

    CHAR pszPassword[MAXPATHLEN];
    CHAR pszTempPassword[MAXPATHLEN];
    CHAR *p = NULL;
    HRESULT hRet;
    HWND hwndPassword;

    switch (uMsg) {
        case WM_INITDIALOG:
        {
            if(GetDlgItem(hDlg,IDC_OLD_PASSWORD)!=NULL)
            {
                SendDlgItemMessage(hDlg,IDC_OLD_PASSWORD,EM_SETLIMITTEXT,(WPARAM) RATINGS_MAX_PASSWORD_LENGTH,(LPARAM) 0);
            }
            if(GetDlgItem(hDlg,IDC_PASSWORD)!=NULL)
            {
                SendDlgItemMessage(hDlg,IDC_PASSWORD,EM_SETLIMITTEXT,(WPARAM) RATINGS_MAX_PASSWORD_LENGTH,(LPARAM) 0);
            }
            if(GetDlgItem(hDlg,IDC_CONFIRM_PASSWORD)!=NULL)
            {
                SendDlgItemMessage(hDlg,IDC_CONFIRM_PASSWORD,EM_SETLIMITTEXT,(WPARAM) RATINGS_MAX_PASSWORD_LENGTH,(LPARAM) 0);
            }
            if(GetDlgItem(hDlg,IDC_CREATE_PASSWORD)!=NULL)
            {
                SendDlgItemMessage(hDlg,IDC_CREATE_PASSWORD,EM_SETLIMITTEXT,(WPARAM) RATINGS_MAX_PASSWORD_LENGTH,(LPARAM) 0);
            }
            return(TRUE);
        }
        case WM_COMMAND:
            switch (LOWORD(wParam))  {
                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                case IDOK:
                    hwndPassword = ::GetDlgItem(hDlg, IDC_PASSWORD);
                    if (hwndPassword == NULL)
                        hwndPassword = ::GetDlgItem(hDlg, IDC_CREATE_PASSWORD);
                    GetWindowText(hwndPassword, pszPassword, sizeof(pszPassword));
                    GetDlgItemText(hDlg, IDC_CONFIRM_PASSWORD, pszTempPassword, sizeof(pszTempPassword));

                    /* if they've typed just the first password but not the
                     * second, let Enter take them to the second field
                     */
                    if (*pszPassword && !*pszTempPassword && GetFocus() == hwndPassword) {
                        SetErrorFocus(hDlg, IDC_CONFIRM_PASSWORD);
                        break;
                    }

                    if (strcmpf(pszPassword, pszTempPassword))
                    {
                        MyMessageBox(hDlg, IDS_NO_MATCH, IDS_GENERIC, MB_OK);
                        SetErrorFocus(hDlg, IDC_CONFIRM_PASSWORD);
                        break;
                    }

                    if (*pszPassword=='\0')
                    {
                        MyMessageBox(hDlg, IDS_NO_NULL_PASSWORD, IDS_GENERIC, MB_OK);
                        SetErrorFocus(hDlg, IDC_PASSWORD);
                        break;
                    }

                    if (SUCCEEDED(VerifySupervisorPassword(szNULL)))
                    {
                        GetDlgItemText(hDlg, IDC_OLD_PASSWORD, pszTempPassword, sizeof(pszTempPassword));
                        p = pszTempPassword;
                    }
                    
                    hRet = ChangeSupervisorPassword(p, pszPassword);
                    
                    if (SUCCEEDED(hRet))
                        EndDialog(hDlg, TRUE);
                    else
                    {
                        MyMessageBox(hDlg, IDS_BADPASSWORD, IDS_GENERIC, MB_OK|MB_ICONERROR);
                        SetErrorFocus(hDlg, IDC_OLD_PASSWORD);
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
    }
    return FALSE;
}


STDAPI RatingSetupUI(HWND hDlg, LPCSTR pszUsername)
{
#if 0
    PropSheet ps;
    char      pszBuf[MAXPATHLEN];

    MLLoadStringA(IDS_GENERIC, pszBuf, sizeof(pszBuf));
    ps.Init(hDlg, hInstance, 1, pszBuf, TRUE);

    ps.psPage.lParam      = NULL;
    ps.psPage.pfnDlgProc  = (DLGPROC) IntroDialogProc;
    ps.psPage.pszTemplate = MAKEINTRESOURCE(IDD_INTRO);

    ps.psHeader.phpage[ps.psHeader.nPages] = CreatePropertySheetPage(&ps.psPage);
    if (ps.psHeader.phpage[ps.psHeader.nPages])
        ps.psHeader.nPages++;

    ps.Run();
    return (NOERROR);
#else

    BOOL fExistingPassword;

    UINT_PTR passConfirm = DoExistingPasswordConfirm(hDlg,&fExistingPassword);

    if (passConfirm == PASSCONFIRM_FAIL)
        return E_ACCESSDENIED;

    HRESULT hres = NOERROR;

    BOOL fFreshInstall = FALSE;
    if (!gPRSI->fRatingInstalled)
    {
        gPRSI->FreshInstall();
        fFreshInstall = TRUE;
    }

    if (!PicsOptionsDialog(hDlg, hInstance, gPRSI, GetUserObject(pszUsername))) {
        /* If we have no saved settings and they cancelled the settings UI, and
         * they just entered a new supervisor password, then we need to remove
         * the supervisor password too, otherwise it looks like there's been
         * tampering.  The other option would be to actually commit the
         * settings in that case but disable enforcement, but the case we're
         * looking to treat here is the casual exploring user who goes past
         * entering the password but decides he doesn't want ratings after all.
         * If we leave the password and ratings settings there, then he's not
         * going to remember what the password was when he decides he does want
         * ratings a year from now.  Best to just remove the password and let
         * him enter and confirm a new one next time.
         */
        if (fFreshInstall) {
            if (passConfirm == PASSCONFIRM_NEW) {
                RemoveSupervisorPassword();
                gPRSI->fRatingInstalled = FALSE;
            }
        }
        return E_FAIL;
    }

    if(fExistingPassword==FALSE)
    {
        passConfirm = DoPasswordConfirm(hDlg);

        if(passConfirm==PASSCONFIRM_FAIL)
        {
            gPRSI->fRatingInstalled = FALSE;
            return E_FAIL;
        }
    }

    gPRSI->fSettingsValid = TRUE;
    gPRSI->SaveRatingSystemInfo();

    return NOERROR;

#endif
}


STDAPI RatingAddPropertyPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lparam, DWORD dwPageFlags)
{
    HRESULT hr = NOERROR;

    PROPSHEETPAGE psPage;

    psPage.dwSize = sizeof(psPage);
    psPage.hInstance = MLGetHinst();
    psPage.dwFlags = PSP_DEFAULT;
    psPage.lParam = NULL;
    psPage.pfnDlgProc  = (DLGPROC) IntroDialogProc;
    psPage.pszTemplate = MAKEINTRESOURCE(IDD_INTRO);
    // set a pointer to the PAGEINFO struct as the private data for this page
    // psPage.lParam = (LPARAM)nPageIndex;

    HPROPSHEETPAGE hpage = CreatePropertySheetPage(&psPage);

    if (hpage)
    {
        if (!pfnAddPage(hpage, lparam))
        {
            DestroyPropertySheetPage(hpage);
            hr = E_FAIL;
        }
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}


STDAPI RatingEnable(HWND hwndParent, LPCSTR pszUsername, BOOL fEnable)
{
    if (!gPRSI || !gPRSI->fRatingInstalled || !gPRSI->fSettingsValid) {
        if (!fEnable)
            return NOERROR;         /* ratings are disabled by not being installed */
        HRESULT hres = RatingSetupUI(hwndParent, pszUsername);

        /* User clicked "Turn On" but we installed and let him choose his
         * settings, so give him friendly confirmation that we set things
         * up for him and he can click Settings later to change things
         * (therefore implying that he doesn't need to click Settings now).
         */
        if (SUCCEEDED(hres)) {
            MyMessageBox(hwndParent, IDS_NOWENABLED, IDS_ENABLE_WARNING,
                         IDS_GENERIC, MB_ICONINFORMATION | MB_OK);
        }
        return hres;
    }

    PicsUser *pUser = ::GetUserObject(pszUsername);
    if (pUser == NULL)
        return HRESULT_FROM_WIN32(ERROR_BAD_USERNAME);

    /* !a == !b to normalize non-zero values */
    if (!fEnable == !pUser->fEnabled)
        return NOERROR;             /* already in state caller wants */

    if (DoPasswordConfirm(hwndParent))
    {
        PicsUser *pUser = ::GetUserObject();
        if (pUser != NULL) {
            pUser->fEnabled = !pUser->fEnabled;
            gPRSI->SaveRatingSystemInfo();
            if (pUser->fEnabled)
                MyMessageBox(hwndParent, IDS_NOW_ON, IDS_ENABLE_WARNING,
                             IDS_GENERIC, MB_OK);
            else
            {
                HKEY hkeyRatings;

                if (RegOpenKey(HKEY_LOCAL_MACHINE, szRATINGS, &hkeyRatings) == ERROR_SUCCESS)
                {
                    unsigned long lType;
                    DWORD         dwFlag,dwSizeOfFlag=sizeof(dwFlag);

                    if(RegQueryValueEx(hkeyRatings,(char *) &szTURNOFF,NULL,&lType,(LPBYTE) &dwFlag,&dwSizeOfFlag)==ERROR_SUCCESS)
                    {
                        if(dwFlag!=1)
                        {
                            DialogBox(MLGetHinst(),
                                            MAKEINTRESOURCE(IDD_TURNOFF),
                                            hwndParent,
                                            (DLGPROC) TurnOffDialogProc);
                        }
                    }
                    else
                    {
                        DialogBox(MLGetHinst(),
                                        MAKEINTRESOURCE(IDD_TURNOFF),
                                        hwndParent,
                                        (DLGPROC) TurnOffDialogProc);
                    }

                    RegCloseKey(hkeyRatings);
                }
            }
        }
        return NOERROR;
    }
    else
        return E_ACCESSDENIED;
}

INT_PTR CALLBACK PRFPasswordDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static DWORD aIds[] = {
        IDC_STATIC1,    IDH_RATINGS_SUPERVISOR_PASSWORD,
        IDC_STATIC2,    IDH_RATINGS_SUPERVISOR_PASSWORD,
        IDC_PRFPASSWORD,IDH_RATINGS_SUPERVISOR_PASSWORD,
        0,0
    };

    CHAR pszPassword[MAXPATHLEN];
    HRESULT hRet;

    switch (uMsg) {
        case WM_INITDIALOG:
        {
            if(GetDlgItem(hDlg,IDC_PRFPASSWORD)!=NULL)
            {
                SendDlgItemMessage(hDlg,IDC_PRFPASSWORD,EM_SETLIMITTEXT,(WPARAM) RATINGS_MAX_PASSWORD_LENGTH,(LPARAM) 0);
            }

            return(TRUE);
        }
        case WM_COMMAND:
            switch (LOWORD(wParam))  {
            case IDCANCEL:
                    EndDialog(hDlg, PASSCONFIRM_FAIL);
                    break;

                case IDOK:
                    if(GetDlgItem(hDlg,IDC_PRFPASSWORD)!=NULL)
                    {
                        GetDlgItemText(hDlg, IDC_PRFPASSWORD, pszPassword, sizeof(pszPassword));

                        hRet = VerifySupervisorPassword(pszPassword);

                        if (hRet == (NOERROR)) {
                            /* In case we got the users-CPL-supervisor password,
                             * make sure we write it to the ratings location in
                             * the registry too.
                             */
                            if (ChangeSupervisorPassword(pszPassword, pszPassword) == S_FALSE)
                                EndDialog(hDlg, PASSCONFIRM_NEW);
                            else
                                EndDialog(hDlg, PASSCONFIRM_OK);
                        }
                        else
                        {
                            MyMessageBox(hDlg, IDS_BADPASSWORD, IDS_GENERIC, MB_OK|MB_ICONERROR);    
                            SetErrorFocus(hDlg, IDC_PRFPASSWORD);
                        }
                        break;
                    }
                    else
                    {
                        EndDialog(hDlg, PASSCONFIRM_NEW);
                        break;
                    }
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
    }
    return FALSE;
}

int WINAPI ClickedOnPRF(HWND hWndOwner,HINSTANCE hInstance,PSTR lpszFileName,int nShow)
{
    BOOL                    bExists=FALSE,fPICSRulesSaved=FALSE,fExistingPassword;
    int                     iReplaceInstalled=IDYES;
    char                    szTitle[MAX_PATH],szMessage[MAX_PATH];
    PropSheet               ps;
    PRSD                    *pPRSD;
    char                    pszBuf[MAXPATHLEN];
    BOOL                    fRet=FALSE;
    UINT_PTR                passConfirm;

    //Make sure the user wants to do this
    if(SUCCEEDED(VerifySupervisorPassword(szNULL)))
    {
        fExistingPassword=TRUE;
    
        passConfirm = DialogBox(MLGetHinst(),MAKEINTRESOURCE(IDD_PRFPASSWORDEXISTS),hWndOwner,(DLGPROC) PRFPasswordDialogProc);
    }
    else
    {
        fExistingPassword=FALSE;

        passConfirm = DialogBox(MLGetHinst(),MAKEINTRESOURCE(IDD_PRFPASSWORDNOEXIST),hWndOwner,(DLGPROC) PRFPasswordDialogProc);
    }

    if(passConfirm==PASSCONFIRM_FAIL)
    {
        return(E_ACCESSDENIED);
    }

    if(fExistingPassword==FALSE)
    {
        passConfirm=NO_EXISTING_PASSWORD;
    }

    BOOL fFreshInstall=FALSE;

    if(!gPRSI->fRatingInstalled)
    {
        gPRSI->FreshInstall();
        fFreshInstall=TRUE;
    }

    gPRSI->lpszFileName=lpszFileName;

    MLLoadStringA(IDS_GENERIC,pszBuf,sizeof(pszBuf));
    ps.Init(hWndOwner,hInstance,NUM_PAGES,pszBuf,TRUE);

    pPRSD=new PRSD;
    if (!pPRSD) return FALSE;

    pPRSD->pPU                =GetUserObject((LPCTSTR) NULL);
    pPRSD->pTempRatings       =NULL;
    pPRSD->hwndBitmapCategory =NULL;
    pPRSD->hwndBitmapLabel    =NULL;
    pPRSD->hInst              =hInstance;
    pPRSD->pPRSI              =gPRSI;
    pPRSD->fNewProviders      =FALSE;

    MakePropPage(&ps,pPRSD,(DLGPROC) PicsDlgProc,IDD_RATINGS);
    MakePropPage(&ps,pPRSD,(DLGPROC) ApprovedSitesDlgProc,IDD_APPROVEDSITES);
    MakePropPage(&ps,pPRSD,(DLGPROC) GeneralDlgProc,IDD_GENERAL);
    MakePropPage(&ps,pPRSD,(DLGPROC) AdvancedDlgProc,IDD_ADVANCED);
 
    if(ps.psHeader.nPages==NUM_PAGES)
    {
        if(fExistingPassword==FALSE)
        {
            InstallDefaultProvider(NULL,pPRSD);
            PicsDlgSave(NULL,pPRSD);
        }

        ps.psHeader.nStartPage=ps.psHeader.nPages-1;
        fRet=ps.Run();
    }

    ImageList_Destroy(g_hImageList);

    delete pPRSD;

    if(!fRet)
    {
        // If we have no saved settings and they cancelled the settings UI, and
        // they just entered a new supervisor password, then we need to remove
        // the supervisor password too, otherwise it looks like there's been
        // tampering.  The other option would be to actually commit the
        // settings in that case but disable enforcement, but the case we're
        // looking to treat here is the casual exploring user who goes past
        // entering the password but decides he doesn't want ratings after all.
        // If we leave the password and ratings settings there, then he's not
        // going to remember what the password was when he decides he does want
        // ratings a year from now.  Best to just remove the password and let
        // him enter and confirm a new one next time.

        if(fFreshInstall)
        {
            if(passConfirm==PASSCONFIRM_NEW)
            {
                RemoveSupervisorPassword();
                gPRSI->fRatingInstalled=FALSE;
            }
        }

        return(FALSE);
    }

    if(fExistingPassword==FALSE)
    {
        passConfirm=DoPasswordConfirm(hWndOwner);

        if(passConfirm==PASSCONFIRM_FAIL)
        {
            gPRSI->fRatingInstalled=FALSE;
            return(FALSE);
        }
    }

    gPRSI->fSettingsValid=TRUE;
    gPRSI->SaveRatingSystemInfo();

    MLLoadString(IDS_PICSRULES_CLICKIMPORTTITLE,(LPTSTR) szTitle,MAX_PATH);
    MLLoadString(IDS_PICSRULES_CLICKFINISHED,(LPTSTR) szMessage,MAX_PATH);

    MessageBox(hWndOwner,(LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK);

    return(TRUE);
}

BOOL CALLBACK RATPasswordDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static DWORD aIds[] = {
        IDC_STATIC1,    IDH_RATINGS_SUPERVISOR_PASSWORD,
        IDC_STATIC2,    IDH_RATINGS_SUPERVISOR_PASSWORD,
        IDC_RATPASSWORD,IDH_RATINGS_SUPERVISOR_PASSWORD,
        0,0
    };

    CHAR pszPassword[MAXPATHLEN];
    HRESULT hRet;

    switch (uMsg) {
        case WM_INITDIALOG:
        {
            if(GetDlgItem(hDlg,IDC_RATPASSWORD)!=NULL)
            {
                SendDlgItemMessage(hDlg,IDC_RATPASSWORD,EM_SETLIMITTEXT,(WPARAM) RATINGS_MAX_PASSWORD_LENGTH,(LPARAM) 0);
            }

            return(TRUE);
        }
        case WM_COMMAND:
            switch (LOWORD(wParam))  {
            case IDCANCEL:
                    EndDialog(hDlg, PASSCONFIRM_FAIL);
                    break;

                case IDOK:
                    if(GetDlgItem(hDlg,IDC_RATPASSWORD)!=NULL)
                    {
                        GetDlgItemText(hDlg, IDC_RATPASSWORD, pszPassword, sizeof(pszPassword));

                        hRet = VerifySupervisorPassword(pszPassword);

                        if (hRet == (NOERROR)) {
                            /* In case we got the users-CPL-supervisor password,
                             * make sure we write it to the ratings location in
                             * the registry too.
                             */
                            if (ChangeSupervisorPassword(pszPassword, pszPassword) == S_FALSE)
                                EndDialog(hDlg, PASSCONFIRM_NEW);
                            else
                                EndDialog(hDlg, PASSCONFIRM_OK);
                        }
                        else
                        {
                            MyMessageBox(hDlg, IDS_BADPASSWORD, IDS_GENERIC, MB_OK|MB_ICONERROR);    
                            SetErrorFocus(hDlg, IDC_RATPASSWORD);
                        }
                        break;
                    }
                    else
                    {
                        EndDialog(hDlg, PASSCONFIRM_NEW);
                        break;
                    }
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
    }
    return FALSE;
}

int WINAPI ClickedOnRAT(HWND hWndOwner,HINSTANCE hInstance,PSTR lpszFileName,int nShow)
{
    BOOL                    bExists=FALSE,fPICSRulesSaved=FALSE,fExistingPassword;
    int                     iReplaceInstalled=IDYES;
    char                    szTitle[MAX_PATH],szMessage[MAX_PATH],szNewFile[MAX_PATH];
    char                    *lpszFile,*lpszTemp;
    PropSheet               ps;
    PRSD                    *pPRSD;
    char                    pszBuf[MAXPATHLEN];
    BOOL                    fRet=FALSE;
    UINT_PTR                passConfirm;

    //Make sure the user wants to do this
    if(SUCCEEDED(VerifySupervisorPassword(szNULL)))
    {
        fExistingPassword=TRUE;
    
        passConfirm = DialogBox(MLGetHinst(),MAKEINTRESOURCE(IDD_RATPASSWORDEXISTS),hWndOwner,(DLGPROC) RATPasswordDialogProc);
    }
    else
    {
        fExistingPassword=FALSE;

        passConfirm = DialogBox(MLGetHinst(),MAKEINTRESOURCE(IDD_RATPASSWORDNOEXIST),hWndOwner,(DLGPROC) RATPasswordDialogProc);
    }

    if(passConfirm==PASSCONFIRM_FAIL)
    {
        return(E_ACCESSDENIED);
    }

    if(fExistingPassword==FALSE)
    {
        passConfirm=NO_EXISTING_PASSWORD;
    }

    //Copy the file to the windows system directory
    GetSystemDirectory(szNewFile,MAX_PATH);
    
    lpszTemp=lpszFileName;

    do{
        lpszFile=lpszTemp;
    }
    while((lpszTemp=strchrf(lpszTemp+1,'\\'))!=NULL);
    
    lstrcat(szNewFile,lpszFile);
    
    CopyFile(lpszFileName,szNewFile,FALSE);

    BOOL fFreshInstall = FALSE;
    if (!gPRSI->fRatingInstalled)
    {
        gPRSI->FreshInstall();
        fFreshInstall = TRUE;
    }

    gPRSI->lpszFileName=szNewFile;

    MLLoadStringA(IDS_GENERIC,pszBuf,sizeof(pszBuf));
    ps.Init(hWndOwner,hInstance,NUM_PAGES,pszBuf,TRUE);

    pPRSD=new PRSD;
    if (!pPRSD) return FALSE;

    pPRSD->fNewProviders      =TRUE;
    pPRSD->pPU                =GetUserObject((LPCTSTR) NULL);
    pPRSD->pTempRatings       =NULL;
    pPRSD->hwndBitmapCategory =NULL;
    pPRSD->hwndBitmapLabel    =NULL;
    pPRSD->hInst              =hInstance;
    pPRSD->pPRSI              =gPRSI;
    pPRSD->fNewProviders      =FALSE;

    MakePropPage(&ps,pPRSD,(DLGPROC) PicsDlgProc,IDD_RATINGS);
    MakePropPage(&ps,pPRSD,(DLGPROC) ApprovedSitesDlgProc,IDD_APPROVEDSITES);
    MakePropPage(&ps,pPRSD,(DLGPROC) GeneralDlgProc,IDD_GENERAL);
    MakePropPage(&ps,pPRSD,(DLGPROC) AdvancedDlgProc,IDD_ADVANCED);
 
    if(ps.psHeader.nPages==NUM_PAGES)
    {
        if(fExistingPassword==FALSE)
        {
            InstallDefaultProvider(NULL,pPRSD);
            PicsDlgSave(NULL,pPRSD);
        }

        ps.psHeader.nStartPage=ps.psHeader.nPages-2;
        fRet=ps.Run();
    }

    ImageList_Destroy(g_hImageList);

    delete pPRSD;

    if(!fRet)
    {
        // If we have no saved settings and they cancelled the settings UI, and
        // they just entered a new supervisor password, then we need to remove
        // the supervisor password too, otherwise it looks like there's been
        // tampering.  The other option would be to actually commit the
        // settings in that case but disable enforcement, but the case we're
        // looking to treat here is the casual exploring user who goes past
        // entering the password but decides he doesn't want ratings after all.
        // If we leave the password and ratings settings there, then he's not
        // going to remember what the password was when he decides he does want
        // ratings a year from now.  Best to just remove the password and let
        // him enter and confirm a new one next time.

        if(fFreshInstall)
        {
            if(passConfirm==PASSCONFIRM_NEW)
            {
                RemoveSupervisorPassword();
                gPRSI->fRatingInstalled=FALSE;
            }
        }

        return(FALSE);
    }

    if(fExistingPassword==FALSE)
    {
        passConfirm=DoPasswordConfirm(hWndOwner);

        if(passConfirm==PASSCONFIRM_FAIL)
        {
            gPRSI->fRatingInstalled=FALSE;
            return(FALSE);
        }
    }

    gPRSI->fSettingsValid=TRUE;
    gPRSI->SaveRatingSystemInfo();

    MLLoadString(IDS_PICSRULES_CLICKIMPORTTITLE,(LPTSTR) szTitle,MAX_PATH);
    MLLoadString(IDS_PICSRULES_CLICKFINISHED,(LPTSTR) szMessage,MAX_PATH);

    MessageBox(hWndOwner,(LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK);

    return(TRUE);
}
