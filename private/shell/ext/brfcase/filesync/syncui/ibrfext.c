//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: ibrfext.c
//
//  This files contains the IShellExtInit, IShellPropSheetExt and
//  IContextMenu interfaces.
//
// History:
//  02-02-94 ScottH     Moved from iface.c; added new shell interface support
//
//---------------------------------------------------------------------------


#include "brfprv.h"         // common headers
#include <brfcasep.h>

#include "res.h"
#include "recact.h"


// Briefcase extension structure.  This is used for IContextMenu
// and PropertySheet binding.
//
typedef struct _BriefExt
{
    // We use the sxi also as our IUnknown interface
    IShellExtInit       sxi;            // 1st base class
    IContextMenu        ctm;            // 2nd base class
    IShellPropSheetExt  spx;            // 3rd base class
    UINT                cRef;           // reference count
    LPDATAOBJECT        pdtobj;         // data object
    HKEY                hkeyProgID;     // reg. database key to ProgID
} BriefExt, * PBRIEFEXT;


//---------------------------------------------------------------------------
// IDataObject extraction functions
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: Return TRUE if the IDataObject knows the special
         briefcase file-system object format

Returns: see above
Cond:    --
*/
BOOL PUBLIC DataObj_KnowsBriefObj(
    LPDATAOBJECT pdtobj)
{
    HRESULT hres;
    FORMATETC fmte = {(CLIPFORMAT)g_cfBriefObj, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    // Does this dataobject support briefcase object format?
    //
    hres = pdtobj->lpVtbl->QueryGetData(pdtobj, &fmte);
    return (hres == ResultFromScode(S_OK));
}


/*----------------------------------------------------------
Purpose: Gets the briefcase path from an IDataObject.

Returns: standard
Cond:    --
*/
HRESULT PUBLIC DataObj_QueryBriefPath(
    LPDATAOBJECT pdtobj,
    LPTSTR pszBriefPath)         // Must be size MAX_PATH
{
    HRESULT hres = ResultFromScode(E_FAIL);
    FORMATETC fmte = {(CLIPFORMAT)g_cfBriefObj, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium;

    ASSERT(pdtobj);
    ASSERT(pszBriefPath);

    // Does this dataobject support briefcase object format?
    //
    hres = pdtobj->lpVtbl->GetData(pdtobj, &fmte, &medium);
    if (SUCCEEDED(hres))
    {
        PBRIEFOBJ pbo = (PBRIEFOBJ)GlobalLock(medium.hGlobal);
        LPTSTR psz = BOBriefcasePath(pbo);

        lstrcpy(pszBriefPath, psz);

        GlobalUnlock(medium.hGlobal);
        MyReleaseStgMedium(&medium);
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: Gets a single path from an IDataObject.

Returns: standard
         S_OK if the object is inside a briefcase
         S_FALSE if not
Cond:    --
*/
HRESULT PUBLIC DataObj_QueryPath(
    LPDATAOBJECT pdtobj,
    LPTSTR pszPath)          // Must be size MAX_PATH
{
    HRESULT hres = E_FAIL;
    FORMATETC fmteBrief = {(CLIPFORMAT)g_cfBriefObj, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    FORMATETC fmteHdrop = {(CLIPFORMAT)CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium;

    ASSERT(pdtobj);
    ASSERT(pszPath);

    // Does this dataobject support briefcase object format?
    //
    hres = pdtobj->lpVtbl->GetData(pdtobj, &fmteBrief, &medium);
    if (SUCCEEDED(hres))
    {
        // Yup

        PBRIEFOBJ pbo = (PBRIEFOBJ)GlobalLock(medium.hGlobal);
        LPTSTR psz = BOFileList(pbo);

        // Only get first path in list
        lstrcpy(pszPath, psz);
        GlobalUnlock(medium.hGlobal);
        MyReleaseStgMedium(&medium);
        hres = S_OK;
    }
    else
    {
        // Or does it support hdrops?
        hres = pdtobj->lpVtbl->GetData(pdtobj, &fmteHdrop, &medium);
        if (SUCCEEDED(hres))
        {
            // Yup
            HDROP hdrop = medium.hGlobal;

            // Only get first path in the file list
            DragQueryFile(hdrop, 0, pszPath, MAX_PATH);

            MyReleaseStgMedium(&medium);
            hres = S_FALSE;
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: Gets a file list from an IDataObject.  Allocates
         ppszList to appropriate size and fills it with
         a null-terminated list of paths.  It is double-null
         terminated.

         If ppszList is NULL, then simply get the count of files.

         Call DataObj_FreeList to free the ppszList.

Returns: standard
         S_OK if the objects are inside a briefcase
         S_FALSE if not
Cond:    --
*/
HRESULT PUBLIC DataObj_QueryFileList(
    LPDATAOBJECT pdtobj,
    LPTSTR * ppszList,       // List of files (may be NULL)
    LPUINT puCount)         // Count of files
{
    HRESULT hres = ResultFromScode(E_FAIL);
    FORMATETC fmteBrief = {(CLIPFORMAT)g_cfBriefObj, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    FORMATETC fmteHdrop = {(CLIPFORMAT)CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium;

    ASSERT(pdtobj);
    ASSERT(puCount);

    // Does this dataobject support briefcase object format?
    //
    hres = pdtobj->lpVtbl->GetData(pdtobj, &fmteBrief, &medium);
    if (SUCCEEDED(hres))
    {
        // Yup
        PBRIEFOBJ pbo = (PBRIEFOBJ)GlobalLock(medium.hGlobal);

        *puCount = BOFileCount(pbo);
        hres = ResultFromScode(S_OK);

        if (ppszList)
        {
            *ppszList = GAlloc(BOFileListSize(pbo));
            if (*ppszList)
            {
                BltByte(*ppszList, BOFileList(pbo), BOFileListSize(pbo));
            }
            else
            {
                hres = ResultFromScode(E_OUTOFMEMORY);
            }
        }

        GlobalUnlock(medium.hGlobal);
        MyReleaseStgMedium(&medium);
        goto Leave;
    }

    // Or does it support hdrops?
    //
    hres = pdtobj->lpVtbl->GetData(pdtobj, &fmteHdrop, &medium);
    if (SUCCEEDED(hres))
    {
        // Yup
        HDROP hdrop = medium.hGlobal;
        UINT cFiles = DragQueryFile(hdrop, (UINT)-1, NULL, 0);
        UINT cchSize = 0;
        UINT i;

        *puCount = cFiles;
        hres = ResultFromScode(S_FALSE);

        if (ppszList)
        {
            // Determine size we need to allocate
            for (i = 0; i < cFiles; i++)
            {
                cchSize += DragQueryFile(hdrop, i, NULL, 0) + 1;
            }
            cchSize++;      // for extra null

            *ppszList = GAlloc(CbFromCch(cchSize));
            if (*ppszList)
            {
                LPTSTR psz = *ppszList;
                UINT cch;

                // Translate the hdrop into our file list format.
                // We know that they really are the same format,
                // but to maintain the abstraction layer, we
                // pretend we don't.
                for (i = 0; i < cFiles; i++)
                {
                    cch = DragQueryFile(hdrop, i, psz, cchSize) + 1;
                    psz += cch;
                    cchSize -= cch;
                }
                *psz = TEXT('\0');    // extra null
            }
            else
            {
                hres = ResultFromScode(E_OUTOFMEMORY);
            }
        }
        MyReleaseStgMedium(&medium);
        goto Leave;
    }

    // BUGBUG: do we need to query for CF_TEXT?

Leave:
    return hres;
}


/*----------------------------------------------------------
Purpose: Frees a file list that was allocated by DataObj_QueryFileList.
Returns: --
Cond:    --
*/
void PUBLIC DataObj_FreeList(
    LPTSTR pszList)
{
    GFree(pszList);
}


//---------------------------------------------------------------------------
// BriefExt IUnknown base member functions
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: IUnknown::QueryInterface

Returns: standard
Cond:    --
*/
STDMETHODIMP BriefExt_QueryInterface(
    LPUNKNOWN punk, 
    REFIID riid, 
    LPVOID * ppvOut)
{
    PBRIEFEXT this = IToClass(BriefExt, sxi, punk);
    HRESULT hres;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IShellExtInit))
    {
        // We use the sxi field as our IUnknown as well
        *ppvOut = &this->sxi;
        this->cRef++;
        hres = NOERROR;
    }
    else if (IsEqualIID(riid, &IID_IContextMenu))
    {
        (LPCONTEXTMENU)*ppvOut = &this->ctm;
        this->cRef++;
        hres = NOERROR;
    }
    else if (IsEqualIID(riid, &IID_IShellPropSheetExt))
    {
        (LPSHELLPROPSHEETEXT)*ppvOut = &this->spx;
        this->cRef++;
        hres = NOERROR;
    }
    else
    {
        *ppvOut = NULL;
        hres = ResultFromScode(E_NOINTERFACE);
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IUnknown::AddRef

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) BriefExt_AddRef(
    LPUNKNOWN punk)
{
    PBRIEFEXT this = IToClass(BriefExt, sxi, punk);

    return ++this->cRef;
}


/*----------------------------------------------------------
Purpose: IUnknown::Release

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) BriefExt_Release(
    LPUNKNOWN punk)
{
    PBRIEFEXT this = IToClass(BriefExt, sxi, punk);

    if (--this->cRef)
    {
        return this->cRef;
    }

    if (this->pdtobj)
    {
        this->pdtobj->lpVtbl->Release(this->pdtobj);
    }

    if (this->hkeyProgID)
    {
        RegCloseKey(this->hkeyProgID);
    }

    GFree(this);
    ENTEREXCLUSIVE()
    {
        DecBusySemaphore();     // Decrement the reference count to the DLL
    }
    LEAVEEXCLUSIVE()

    return 0;
}


//---------------------------------------------------------------------------
// BriefExt IShellExtInit member functions
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: IShellExtInit::QueryInterface

Returns: standard
Cond:    --
*/
STDMETHODIMP BriefExt_SXI_QueryInterface(
    LPSHELLEXTINIT psxi,
    REFIID riid, 
    LPVOID * ppvOut)
{
    return BriefExt_QueryInterface((LPUNKNOWN)psxi, riid, ppvOut);
}


/*----------------------------------------------------------
Purpose: IShellExtInit::AddRef

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) BriefExt_SXI_AddRef(
    LPSHELLEXTINIT psxi)
{
    return BriefExt_AddRef((LPUNKNOWN)psxi);
}


/*----------------------------------------------------------
Purpose: IShellExtInit::Release

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) BriefExt_SXI_Release(
    LPSHELLEXTINIT psxi)
{
    return BriefExt_Release((LPUNKNOWN)psxi);
}


/*----------------------------------------------------------
Purpose: IShellExtInit::Initialize

Returns: standard
Cond:    --
*/
STDMETHODIMP BriefExt_SXI_Initialize(
    LPSHELLEXTINIT psxi,
    LPCITEMIDLIST pidlFolder,
    LPDATAOBJECT pdtobj,
    HKEY hkeyProgID)
{
    PBRIEFEXT this = IToClass(BriefExt, sxi, psxi);

    // Initialize can be called more than once.
    //
    if (this->pdtobj)
    {
        this->pdtobj->lpVtbl->Release(this->pdtobj);
    }

    if (this->hkeyProgID)
    {
        RegCloseKey(this->hkeyProgID);
    }

    // Duplicate the pdtobj pointer
    if (pdtobj)
    {
        this->pdtobj = pdtobj;
        pdtobj->lpVtbl->AddRef(pdtobj);
    }

    // Duplicate the handle
    if (hkeyProgID)
    {
        RegOpenKeyEx(hkeyProgID, NULL, 0L, MAXIMUM_ALLOWED, &this->hkeyProgID);
    }

    return NOERROR;
}


//---------------------------------------------------------------------------
// BriefExt IContextMenu member functions
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: IContextMenu::QueryInterface

Returns: standard
Cond:    --
*/
STDMETHODIMP BriefExt_CM_QueryInterface(
    LPCONTEXTMENU pctm,
    REFIID riid, 
    LPVOID * ppvOut)
{
    PBRIEFEXT this = IToClass(BriefExt, ctm, pctm);
    return BriefExt_QueryInterface((LPUNKNOWN)&this->sxi, riid, ppvOut);
}


/*----------------------------------------------------------
Purpose: IContextMenu::AddRef

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) BriefExt_CM_AddRef(
    LPCONTEXTMENU pctm)
{
    PBRIEFEXT this = IToClass(BriefExt, ctm, pctm);
    return BriefExt_AddRef((LPUNKNOWN)&this->sxi);
}


/*----------------------------------------------------------
Purpose: IContextMenu::Release

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) BriefExt_CM_Release(
    LPCONTEXTMENU pctm)
{
    PBRIEFEXT this = IToClass(BriefExt, ctm, pctm);
    return BriefExt_Release((LPUNKNOWN)&this->sxi);
}


/*----------------------------------------------------------
Purpose: IContextMenu::QueryContextMenu

Returns: standard
Cond:    --
*/
#define IDCM_UPDATEALL  0
#define IDCM_UPDATE     1
STDMETHODIMP BriefExt_CM_QueryContextMenu(
    LPCONTEXTMENU pctm,
    HMENU hmenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags)
{
    PBRIEFEXT this = IToClass(BriefExt, ctm, pctm);
    USHORT cItems = 0;
    // We only want to add items to the context menu if:
    //  1) That's what the caller is asking for; and
    //  2) The object is a briefcase or an object inside
    //     a briefcase
    //
    if (IsFlagClear(uFlags, CMF_DEFAULTONLY))   // check for (1)
    {
        TCHAR szIDS[MAXSHORTLEN];

        // Is the object inside a briefcase?  We know it is if
        // the object understands our special format.
        //
        if (DataObj_KnowsBriefObj(this->pdtobj))
        {
            // Yes
            InsertMenu(hmenu, indexMenu++, MF_BYPOSITION | MF_STRING,
                idCmdFirst+IDCM_UPDATE, SzFromIDS(IDS_MENU_UPDATE, szIDS, ARRAYSIZE(szIDS)));

            // NOTE: We should actually be using idCmdFirst+0 above since we are only adding
            // one item to the menu.  But since this code relies on using idCmdFirst+1 then
            // we need to lie and say that we added two items to the menu.  Otherwise the next
            // context menu handler to get called might use the same menu ID that we are using.
            cItems = 2;
        }
        else
        {
            // No
            TCHAR szPath[MAX_PATH];

            // Is the object a briefcase root?
            if (SUCCEEDED(DataObj_QueryPath(this->pdtobj, szPath)) &&
                PathIsBriefcase(szPath))
            {
                // Yup
                InsertMenu(hmenu, indexMenu++, MF_BYPOSITION | MF_STRING,
                    idCmdFirst+IDCM_UPDATEALL, SzFromIDS(IDS_MENU_UPDATEALL, szIDS, ARRAYSIZE(szIDS)));
                cItems++;
            }
        }
    }

    return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_NULL, (USHORT)cItems));
}


/*----------------------------------------------------------
Purpose: IContextMenu::InvokeCommand

Returns: standard
Cond:    --
*/
STDMETHODIMP BriefExt_CM_InvokeCommand(
    LPCONTEXTMENU pctm,
    LPCMINVOKECOMMANDINFO pici)
{
    HWND hwnd = pici->hwnd;
        //LPCSTR pszWorkingDir = pici->lpDirectory;
        //LPCSTR pszCmd = pici->lpVerb;
        //LPCSTR pszParam = pici->lpParameters;
        //int iShowCmd = pici->nShow;
    PBRIEFEXT this = IToClass(BriefExt, ctm, pctm);
    LPBRIEFCASESTG pbrfstg;
    HRESULT hres;

    // The only command we have is to update the selection(s).  Create
    // an instance of IBriefcaseStg so we can call its Update
    // member function.
    //
    if (SUCCEEDED(BriefStg_CreateInstance(NULL, &IID_IBriefcaseStg, &pbrfstg)))
    {
        TCHAR szPath[MAX_PATH];

        if (SUCCEEDED(DataObj_QueryPath(this->pdtobj, szPath)))
        {
            hres = pbrfstg->lpVtbl->Initialize(pbrfstg, szPath, hwnd);
            if (SUCCEEDED(hres))
            {
                hres = pbrfstg->lpVtbl->UpdateObject(pbrfstg, this->pdtobj, hwnd);
            }
            pbrfstg->lpVtbl->Release(pbrfstg);
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IContextMenu::GetCommandString

Returns: standard
Cond:    --
*/
STDMETHODIMP BriefExt_CM_GetCommandString(
    LPCONTEXTMENU pctm,
    UINT_PTR    idCmd,
    UINT        wReserved,
    UINT  *  pwReserved,
    LPSTR       pszName,
    UINT        cchMax)
{
    switch (wReserved)
    {
        case GCS_VERB:
            switch (idCmd)
            {
                case IDCM_UPDATE:
                    lstrcpyn((LPTSTR)pszName, TEXT("update"), cchMax);
                    return NOERROR;
                case IDCM_UPDATEALL:
                    lstrcpyn((LPTSTR)pszName, TEXT("update all"), cchMax);
                    return NOERROR;
            }
    }
    return E_NOTIMPL;
}


//---------------------------------------------------------------------------
// PageData functions
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: Allocates a pagedata.

Returns: TRUE if the allocation/increment was successful

Cond:    --
*/
BOOL PRIVATE PageData_Alloc(
    PPAGEDATA * pppd,
    int atomPath)
    {
    PPAGEDATA this;

    ASSERT(pppd);

    this = GAlloc(sizeof(*this));
    if (this)
        {
        HRESULT hres;
        LPCTSTR pszPath = Atom_GetName(atomPath);
        int  atomBrf;

        // Create an instance of IBriefcaseStg.
        hres = BriefStg_CreateInstance(NULL, &IID_IBriefcaseStg, &this->pbrfstg);
        if (SUCCEEDED(hres))
            {
            hres = this->pbrfstg->lpVtbl->Initialize(this->pbrfstg, pszPath, NULL);
            if (SUCCEEDED(hres))
                {
                TCHAR szBrfPath[MAX_PATH];

                // Request the root path of the briefcase storage
                this->pbrfstg->lpVtbl->GetExtraInfo(this->pbrfstg, NULL, GEI_ROOT,
                    (WPARAM)ARRAYSIZE(szBrfPath), (LPARAM)szBrfPath);

                atomBrf = Atom_Add(szBrfPath);
                hres = (ATOM_ERR != atomBrf) ? NOERROR : E_OUTOFMEMORY;
                }
            }

        if (SUCCEEDED(hres))
            {
            this->pcbs = CBS_Get(atomBrf);
            ASSERT(this->pcbs);

            Atom_AddRef(atomPath);
            this->atomPath = atomPath;

            this->cRef = 1;

            this->bFolder = (FALSE != PathIsDirectory(pszPath));

            Atom_Delete(atomBrf);
            }
        else
            {
            // Failed
            if (this->pbrfstg)
                this->pbrfstg->lpVtbl->Release(this->pbrfstg);

            GFree(this);
            }
        }
    *pppd = this;
    return NULL != this;
    }


/*----------------------------------------------------------
Purpose: Increments the reference count of a pagedata

Returns: Current count
Cond:    --
*/
UINT PRIVATE PageData_AddRef(
    PPAGEDATA this)
    {
    ASSERT(this);

    return ++(this->cRef);
    }


/*----------------------------------------------------------
Purpose: Releases a pagedata struct

Returns: the next reference count
         0 if the struct was freed

Cond:    --
*/
UINT PRIVATE PageData_Release(
    PPAGEDATA this)
    {
    UINT cRef;

    ASSERT(this);
    ASSERT(0 < this->cRef);

    cRef = this->cRef;
    if (0 < this->cRef)
        {
        this->cRef--;
        if (0 == this->cRef)
            {
            if (this->pftl)
                {
                Sync_DestroyFolderList(this->pftl);
                }
            if (this->prl)
                {
                Sync_DestroyRecList(this->prl);
                }

            CBS_Delete(this->pcbs->atomBrf, NULL);

            Atom_Delete(this->atomPath);

            this->pbrfstg->lpVtbl->Release(this->pbrfstg);
            GFree(this);
            return 0;
            }
        }
    return this->cRef;
    }


/*----------------------------------------------------------
Purpose: Sets the data in the pagedata struct to indicate this
         is an orphan.  This function makes no change to the
         database--the caller must do that.

Returns: --
Cond:    --
*/
void PUBLIC PageData_Orphanize(
    PPAGEDATA this)
    {
    this->bOrphan = TRUE;
    if (this->pftl)
        {
        Sync_DestroyFolderList(this->pftl);
        this->pftl = NULL;
        }
    if (this->prl)
        {
        Sync_DestroyRecList(this->prl);
        this->prl = NULL;
        }
    }


/*----------------------------------------------------------
Purpose: Initializes the common page data struct shared between
         the property pages.  Keep in mind that this function may
         be called multiple times, so it must behave properly
         under these conditions (ie, don't blow anything away).

         This function will return S_OK if it is.  S_FALSE means
         the data in question has been invalidated.  This means
         the twin has become an orphan.

Returns: standard result
Cond:    --
*/
HRESULT PUBLIC PageData_Init(
    PPAGEDATA this,
    HWND hwndOwner)
    {
    HRESULT hres;
    HBRFCASE hbrf = PageData_GetHbrf(this);
    LPCTSTR pszPath = Atom_GetName(this->atomPath);

    // ** Note: this structure is not serialized because it is
    // assumed that of the pages that are sharing it, only one
    // can access it at a time.

    ASSERT(pszPath);

    // Has this been explicitly marked as an orphan?
    if (FALSE == this->bOrphan)
        {
        // No; is it (still) a twin?
        if (S_OK == Sync_IsTwin(hbrf, pszPath, 0))
            {
            // Yes; has the folder twinlist or reclist been created yet?
            if (NULL == this->prl ||
                (this->bFolder && NULL == this->pftl))
                {
                // No; create it/them
                HTWINLIST htl;
                PFOLDERTWINLIST pftl = NULL;
                PRECLIST prl = NULL;
                HWND hwndProgress;
                TWINRESULT tr;

                ASSERT(NULL == this->prl);
                ASSERT( !this->bFolder || NULL == this->pftl);

                hwndProgress = UpdBar_Show(hwndOwner, UB_CHECKING | UB_NOCANCEL, DELAY_UPDBAR);

                tr = Sync_CreateTwinList(hbrf, &htl);
                hres = HRESULT_FROM_TR(tr);

                if (SUCCEEDED(hres))
                    {
                    // Add to the twinlist.  Create folder twinlist if
                    // necessary.
                    if (Sync_AddPathToTwinList(hbrf, htl, pszPath, &pftl))
                        {
                        // Does the reclist need creating?
                        if (NULL == this->prl)
                            {
                            // Yes
                            hres = Sync_CreateRecListEx(htl, UpdBar_GetAbortEvt(hwndProgress), &prl);

                            if (SUCCEEDED(hres))
                                {
                                // The object may have been implicitly
                                // deleted in CreateRecList.  Check again.
                                hres = Sync_IsTwin(hbrf, pszPath, 0);
                                }
                            }
                        }
                    else
                        hres = E_FAIL;

                    // Fill in proper fields
                    if (NULL == this->prl && prl)
                        {
                        this->prl = prl;
                        }
                    if (NULL == this->pftl && pftl)
                        {
                        this->pftl = pftl;
                        }

                    // Clean up twinlist
                    Sync_DestroyTwinList(htl);
                    }

                UpdBar_Kill(hwndProgress);

                // Did the above succeed?
                if (FAILED(hres) || S_FALSE == hres)
                    {
                    // No
                    PageData_Orphanize(this);
                    }
                }
            else
                {
                // Yes; do nothing
                hres = S_OK;
                }
            }
        else
            {
            // No; say the thing is an orphan
            PageData_Orphanize(this);
            hres = S_FALSE;
            }
        }
    else
        {
        // Yes
        hres = S_FALSE;
        }

#ifdef DEBUG
    if (S_OK == hres)
        {
        ASSERT( !this->bFolder || this->pftl );
        ASSERT(this->prl);
        }
    else
        {
        ASSERT(NULL == this->pftl);
        ASSERT(NULL == this->prl);
        }
#endif

    return hres;
    }


/*----------------------------------------------------------
Purpose: Verifies whether the page data shared by the property
         pages is still valid.  This function will return S_OK if
         it is.  S_FALSE means the data in question has been
         invalidated.  This means the twin has become an orphan.

         This function assumes PageData_Init has been previously
         called.

Returns: standard result
Cond:    --
*/
HRESULT PUBLIC PageData_Query(
    PPAGEDATA this,
    HWND hwndOwner,
    PRECLIST * pprl,            // May be NULL
    PFOLDERTWINLIST * ppftl)    // May be NULL
    {
    HRESULT hres;
    LPCTSTR pszPath = Atom_GetName(this->atomPath);

    // ** Note: this structure is not serialized because it is
    // assumed that of the pages that are sharing it, only one
    // can access it at a time.

    ASSERT(pszPath);

    // Is a recalc called for?
    if (this->bRecalc)
        {
        // Yes; clear the fields and do again
        PageData_Orphanize(this);       // only temporary
        this->bOrphan = FALSE;          // undo the orphan state
        this->bRecalc = FALSE;

        // Reinit
        hres = PageData_Init(this, hwndOwner);
        if (pprl)
            *pprl = this->prl;
        if (ppftl)
            *ppftl = this->pftl;
        }

    // Are the fields valid?
    else if ( this->prl && (!this->bFolder || this->pftl) )
        {
        // Yes; is it (still) a twin?
        ASSERT(FALSE == this->bOrphan);

        hres = Sync_IsTwin(this->pcbs->hbrf, pszPath, 0);
        if (S_OK == hres)
            {
            // Yes
            if (pprl)
                *pprl = this->prl;
            if (ppftl)
                *ppftl = this->pftl;
            }
        else if (S_FALSE == hres)
            {
            // No; update struct fields
            PageData_Orphanize(this);
            goto OrphanTime;
            }
        }
    else
        {
        // No; say it is an orphan
OrphanTime:
        ASSERT(this->bOrphan);

        if (pprl)
            *pprl = NULL;
        if (ppftl)
            *ppftl = NULL;
        hres = S_FALSE;
        }

    return hres;
    }


//---------------------------------------------------------------------------
// BriefExt IShellPropSheetExt member functions
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: IShellPropSheetExt::QueryInterface

Returns: standard
Cond:    --
*/
STDMETHODIMP BriefExt_SPX_QueryInterface(
    LPSHELLPROPSHEETEXT pspx,
    REFIID riid, 
    LPVOID * ppvOut)
    {
    PBRIEFEXT this = IToClass(BriefExt, spx, pspx);
    return BriefExt_QueryInterface((LPUNKNOWN)&this->sxi, riid, ppvOut);
    }


/*----------------------------------------------------------
Purpose: IShellPropSheetExt::AddRef

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) BriefExt_SPX_AddRef(
    LPSHELLPROPSHEETEXT pspx)
    {
    PBRIEFEXT this = IToClass(BriefExt, spx, pspx);
    return BriefExt_AddRef((LPUNKNOWN)&this->sxi);
    }


/*----------------------------------------------------------
Purpose: IShellPropSheetExt::Release

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) BriefExt_SPX_Release(
    LPSHELLPROPSHEETEXT pspx)
    {
    PBRIEFEXT this = IToClass(BriefExt, spx, pspx);
    return BriefExt_Release((LPUNKNOWN)&this->sxi);
    }


/*----------------------------------------------------------
Purpose: Callback when Status property page is done
Returns: --
Cond:    --
*/
UINT CALLBACK StatusPageCallback(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp)
    {
    if (PSPCB_RELEASE == uMsg)
        {
        PPAGEDATA ppagedata = (PPAGEDATA)ppsp->lParam;

        DEBUG_CODE( TRACE_MSG(TF_GENERAL, TEXT("Releasing status page")); )

        PageData_Release(ppagedata);
        }
    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Callback when Info property sheet is done
Returns: --
Cond:    --
*/
UINT CALLBACK InfoPageCallback(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp)
    {
    if (PSPCB_RELEASE == uMsg)
        {
        PPAGEDATA ppagedata = (PPAGEDATA)ppsp->lParam;
        PINFODATA pinfodata = (PINFODATA)ppagedata->lParam;

        DEBUG_CODE( TRACE_MSG(TF_GENERAL, TEXT("Releasing info page")); )

        if (pinfodata->hdpaTwins)
            {
            int iItem;
            int cItems = DPA_GetPtrCount(pinfodata->hdpaTwins);
            HTWIN htwin;

            for (iItem = 0; iItem < cItems; iItem++)
                {
                htwin = DPA_FastGetPtr(pinfodata->hdpaTwins, iItem);

                Sync_ReleaseTwin(htwin);
                }
            DPA_Destroy(pinfodata->hdpaTwins);
            }
        GFree(pinfodata);

        PageData_Release(ppagedata);
        }
    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Add the status property page
Returns: TRUE on success
         FALSE if out of memory
Cond:    --
*/
BOOL PRIVATE AddStatusPage(
    PPAGEDATA ppd,
    LPFNADDPROPSHEETPAGE pfnAddPage,
    LPARAM lParam)
    {
    BOOL bRet = FALSE;
    HPROPSHEETPAGE hpsp;
    PROPSHEETPAGE psp = {
        sizeof(PROPSHEETPAGE),          // size
        PSP_USECALLBACK,                // PSP_ flags
        g_hinst,                        // hinstance
        MAKEINTRESOURCE(IDD_STATUS),    // pszTemplate
        NULL,                           // icon
        NULL,                           // pszTitle
        Stat_WrapperProc,               // pfnDlgProc
        (LPARAM)ppd,                    // lParam
        StatusPageCallback,             // pfnCallback
        0 };                            // ref count

    ASSERT(ppd);
    ASSERT(pfnAddPage);

    DEBUG_CODE( TRACE_MSG(TF_GENERAL, TEXT("Adding status page")); )

    // Add the status property sheet
    hpsp = CreatePropertySheetPage(&psp);
    if (hpsp)
        {
        bRet = (*pfnAddPage)(hpsp, lParam);
        if (FALSE == bRet)
            {
            // Cleanup on failure
            DestroyPropertySheetPage(hpsp);
            }
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Add the info property page.
Returns: TRUE on success
         FALSE if out of memory
Cond:    --
*/
BOOL PRIVATE AddInfoPage(
    PPAGEDATA ppd,
    LPFNADDPROPSHEETPAGE lpfnAddPage,
    LPARAM lParam)
    {
    BOOL bRet = FALSE;
    HPROPSHEETPAGE hpsp;
    PINFODATA pinfodata;

    ASSERT(lpfnAddPage);

    DEBUG_CODE( TRACE_MSG(TF_GENERAL, TEXT("Adding info page")); )

    pinfodata = GAlloc(sizeof(*pinfodata));
    if (pinfodata)
        {
        PROPSHEETPAGE psp = {
            sizeof(PROPSHEETPAGE),          // size
            PSP_USECALLBACK,                // PSP_ flags
            g_hinst,                        // hinstance
            MAKEINTRESOURCE(IDD_INFO),      // pszTemplate
            NULL,                           // icon
            NULL,                           // pszTitle
            Info_WrapperProc,               // pfnDlgProc
            (LPARAM)ppd,                    // lParam
            InfoPageCallback,               // pfnCallback
            0 };                            // ref count

        ppd->lParam = (LPARAM)pinfodata;

        pinfodata->atomTo = ATOM_ERR;       // Not needed for page
        pinfodata->bStandAlone = FALSE;

        if (NULL != (pinfodata->hdpaTwins = DPA_Create(8)))
            {
            hpsp = CreatePropertySheetPage(&psp);
            if (hpsp)
                {
                bRet = (*lpfnAddPage)(hpsp, lParam);
                if (FALSE == bRet)
                    {
                    // Cleanup on failure
                    DestroyPropertySheetPage(hpsp);
                    }
                }
            if (FALSE == bRet)
                {
                // Cleanup on failure
                DPA_Destroy(pinfodata->hdpaTwins);
                }
            }
        if (FALSE == bRet)
            {
            // Cleanup on failure
            GFree(pinfodata);
            }
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Does the real work to add the briefcase pages to
         the property sheet.
Returns: --
Cond:    --
*/
void PRIVATE BriefExt_AddPagesPrivate(
    LPSHELLPROPSHEETEXT pspx,
    LPCTSTR pszPath,
    LPFNADDPROPSHEETPAGE lpfnAddPage,
    LPARAM lParam)
    {
    PBRIEFEXT this = IToClass(BriefExt, spx, pspx);
    HRESULT hres = NOERROR;
    TCHAR szCanonPath[MAX_PATH];
    int atomPath;

    BrfPathCanonicalize(pszPath, szCanonPath);
    atomPath = Atom_Add(szCanonPath);
    if (atomPath != ATOM_ERR)
        {
        PPAGEDATA ppagedata;
        BOOL bVal;

        // Allocate the pagedata
        if (PageData_Alloc(&ppagedata, atomPath))
            {
            // Always add the status page (even for orphans).
            // Add the info page if the object is a folder.
            bVal = AddStatusPage(ppagedata, lpfnAddPage, lParam);
            if (bVal && ppagedata->bFolder)
                {
                PageData_AddRef(ppagedata);
                AddInfoPage(ppagedata, lpfnAddPage, lParam);
                }
            else if (FALSE == bVal)
                {
                // (Cleanup on failure)
                PageData_Release(ppagedata);
                }
            }
        Atom_Delete(atomPath);
        }
    }


/*----------------------------------------------------------
Purpose: IShellPropSheetExt::AddPages

         The shell calls this member function when it is
         time to add pages to a property sheet.

         As the briefcase storage, we only add pages for
         entities inside a briefcase.  Anything outside
         a briefcase is not touched.

         We can quickly determine if the object is inside
         the briefcase by querying the data object that
         we have.  If it knows our special "briefcase object"
         format, then it must be inside a briefcase.  We
         purposely do not add pages for the root folder itself.

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP BriefExt_SPX_AddPages(
    LPSHELLPROPSHEETEXT pspx,
    LPFNADDPROPSHEETPAGE lpfnAddPage,
    LPARAM lParam)
    {
    PBRIEFEXT this = IToClass(BriefExt, spx, pspx);
    LPTSTR pszList;
    UINT cFiles;

    // Questions:
    //  1) Does this know the briefcase object format?
    //  2) Is there only a single object selected?
    //
    if (DataObj_KnowsBriefObj(this->pdtobj) &&      /* (1) */
        SUCCEEDED(DataObj_QueryFileList(this->pdtobj, &pszList, &cFiles)) &&
        cFiles == 1)                                /* (2) */
        {
        // Yes; add the pages
        BriefExt_AddPagesPrivate(pspx, pszList, lpfnAddPage, lParam);

        DataObj_FreeList(pszList);
        }
    return NOERROR;     // Always allow property sheet to appear
    }


//---------------------------------------------------------------------------
// BriefExtMenu class : Vtables
//---------------------------------------------------------------------------


#pragma data_seg(DATASEG_READONLY)

IShellExtInitVtbl c_BriefExt_SXIVtbl =
    {
    BriefExt_SXI_QueryInterface,
    BriefExt_SXI_AddRef,
    BriefExt_SXI_Release,
    BriefExt_SXI_Initialize
    };

IContextMenuVtbl c_BriefExt_CTMVtbl =
    {
    BriefExt_CM_QueryInterface,
    BriefExt_CM_AddRef,
    BriefExt_CM_Release,
    BriefExt_CM_QueryContextMenu,
    BriefExt_CM_InvokeCommand,
    BriefExt_CM_GetCommandString,
    };

IShellPropSheetExtVtbl c_BriefExt_SPXVtbl = {
    BriefExt_SPX_QueryInterface,
    BriefExt_SPX_AddRef,
    BriefExt_SPX_Release,
    BriefExt_SPX_AddPages
    };

#pragma data_seg()


/*----------------------------------------------------------
Purpose: This function is called back from within
         IClassFactory::CreateInstance() of the default class
         factory object, which is created by SHCreateClassObject.

Returns: standard
Cond:    --
*/
HRESULT CALLBACK BriefExt_CreateInstance(
    LPUNKNOWN punkOuter,
    REFIID riid, 
    LPVOID * ppvOut)
    {
    HRESULT hres;
    PBRIEFEXT this;

    DBG_ENTER_RIID(TEXT("BriefExt_CreateInstance"), riid);

    // Shell extentions typically do not support aggregation.
    //
    if (punkOuter)
        {
        hres = ResultFromScode(CLASS_E_NOAGGREGATION);
        *ppvOut = NULL;
        goto Leave;
        }

    this = GAlloc(sizeof(*this));
    if (!this)
        {
        hres = ResultFromScode(E_OUTOFMEMORY);
        *ppvOut = NULL;
        goto Leave;
        }
    this->sxi.lpVtbl = &c_BriefExt_SXIVtbl;
    this->ctm.lpVtbl = &c_BriefExt_CTMVtbl;
    this->spx.lpVtbl = &c_BriefExt_SPXVtbl;
    this->cRef = 1;
    this->pdtobj = NULL;
    this->hkeyProgID = NULL;

    ENTEREXCLUSIVE()
        {
        // The decrement is in BriefExt_Release()
        IncBusySemaphore();
        }
    LEAVEEXCLUSIVE()

    // Note that the Release member will free the object, if QueryInterface
    // failed.
    //
    hres = c_BriefExt_SXIVtbl.QueryInterface(&this->sxi, riid, ppvOut);
    c_BriefExt_SXIVtbl.Release(&this->sxi);

Leave:
    DBG_EXIT_HRES(TEXT("BriefExt_CreateInstance"), hres);

    return hres;        // S_OK or E_NOINTERFACE
    }
