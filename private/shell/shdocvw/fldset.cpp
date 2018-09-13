#include "priv.h"

#include "fldset.h"

#define IShellView_CreateViewWindow(_pi, _piPrev, _pfs, _psb, _prc, _phw) \
    (_pi)->CreateViewWindow(_piPrev, _pfs, _psb, _prc, _phw)

#define IShellView2_GetView(_pi, _pv, _flg) \
    (_pi)->GetView(_pv, _flg)
#define IShellView2_CreateViewWindow2(_pi, _cParams) \
    (_pi)->CreateViewWindow2(_cParams)

#define IUnknown_QueryInterface(_pu, _riid, _pi) \
        (_pu)->QueryInterface(_riid, (LPVOID*)_pi)
#define IUnknown_AddRef(_pu)    (_pu)->AddRef()
#define IUnknown_Release(_pu)   (_pu)->Release()

typedef struct CViewSet
{
    HDSA _dsaViews;
} CViewSet;


CViewSet* CViewSet_New()
{
    CViewSet* pThis = (CViewSet*)LocalAlloc(LPTR, SIZEOF(CViewSet));
    if (!pThis)
    {
        return(NULL);
    }

    pThis->_dsaViews = DSA_Create(SIZEOF(SHELLVIEWID), 8);
    if (!pThis->_dsaViews)
    {
        LocalFree(pThis);
        return(NULL);
    }

    return(pThis);
}


int CViewSet_Add(CViewSet* that, SHELLVIEWID const* pvid)
{
    return(DSA_AppendItem(that->_dsaViews, (LPVOID)pvid));
}


void CViewSet_Delete(CViewSet* that)
{
    DSA_Destroy(that->_dsaViews);
    LocalFree((HLOCAL)that);
}


void CViewSet_GetDefaultView(CViewSet* that, SHELLVIEWID* pvid)
{
    DSA_GetItem(that->_dsaViews, 0, (LPVOID)pvid);
}


void CViewSet_SetDefaultView(CViewSet* that, SHELLVIEWID const* pvid)
{
    DSA_SetItem(that->_dsaViews, 0, (LPVOID)pvid);
}


// BUGBUG: A linear search for the view
BOOL CViewSet_IsViewSupported(CViewSet* that, SHELLVIEWID const* pvid)
{
    int i;

    // Only go down to 1 since item 0 is the default view
    for (i=DSA_GetItemCount(that->_dsaViews)-1; i>=1; --i)
    {
        if (0 == memcmp(pvid, DSA_GetItemPtr(that->_dsaViews, i),
            SIZEOF(SHELLVIEWID)))
        {
            return(TRUE);
        }
    }

    return(FALSE);
}


// BUGBUG: a linear check
BOOL CViewSet_IsSame(CViewSet* that, CViewSet* pThatView)
{
    int iView = DSA_GetItemCount(pThatView->_dsaViews);

    if (DSA_GetItemCount(that->_dsaViews) != iView)
    {
        return(FALSE);
    }

    for (--iView; iView>=1; --iView)
    {
        if (!CViewSet_IsViewSupported(that,
            (SHELLVIEWID const*)DSA_GetItemPtr(pThatView->_dsaViews, iView)))
        {
            return(FALSE);
        }
    }

    return(TRUE);
}


BOOL CShellViews_Init(CShellViews* that)
{
    if (that->_dpaViews)
    {
        return(TRUE);
    }

    {
        HDPA dpaViews = DPA_Create(4);
        if (!dpaViews)
        {
            return(FALSE);
        }

        {
            CViewSet* pCommViews = CViewSet_New();
            if (!pCommViews)
            {
                DPA_Destroy(dpaViews);
                return(FALSE);
            }

            // The first one is the last known view for that set
            CViewSet_Add(pCommViews, &VID_LargeIcons);
            CViewSet_Add(pCommViews, &VID_LargeIcons);
            CViewSet_Add(pCommViews, &VID_SmallIcons);
            CViewSet_Add(pCommViews, &VID_List      );
            CViewSet_Add(pCommViews, &VID_Details   );

            if (0 != DPA_InsertPtr(dpaViews, 0, pCommViews))
            {
                CViewSet_Delete(pCommViews);
                DPA_Destroy(dpaViews);
                return(FALSE);
            }

            that->_dpaViews = dpaViews;
            return(TRUE);
        }
    }
}


void CShellViews_GetDefaultView(CShellViews* that, UINT uViewSet,
    SHELLVIEWID* pvid)
{
    CViewSet* pViewSet = (CViewSet*)DPA_GetPtr(that->_dpaViews, uViewSet);
    if (!pViewSet)
    {
        pViewSet = (CViewSet*)DPA_GetPtr(that->_dpaViews, 0);
        if (!pViewSet)
        {
            *pvid = VID_LargeIcons;
            return;
        }
    }

    CViewSet_GetDefaultView(pViewSet, pvid);
}


void CShellViews_SetDefaultView(CShellViews* that, UINT uViewSet,
    SHELLVIEWID const* pvid)
{
    CViewSet* pViewSet = (CViewSet*)DPA_GetPtr(that->_dpaViews, uViewSet);
    if (!pViewSet)
    {
        return;
    }

    CViewSet_SetDefaultView(pViewSet, pvid);
}


// BUGBUG: a linear search for the view set
int CShellViews_Add(CShellViews* that, CViewSet* pThisView, BOOL *pbNew)
{
    int iViewSet;

    *pbNew = FALSE;

    for (iViewSet=0; ; ++iViewSet)
    {
        CViewSet* pThatView = (CViewSet*)DPA_GetPtr(that->_dpaViews, iViewSet);
        if (!pThatView)
        {
            break;
        }

        if (CViewSet_IsSame(pThatView, pThisView))
        {
            // Found the same set; delete the one passed in and hand back the
            // existing one
            CViewSet_Delete(pThisView);
            return(iViewSet);
        }
    }

    // I guess we didn't find it
    iViewSet = DPA_AppendPtr(that->_dpaViews, (LPVOID)pThisView);
    if (iViewSet < 0)
    {
        CViewSet_Delete(pThisView);
        return(0);
    }

    *pbNew = TRUE;
    return(iViewSet);
}


BOOL CShellViews_IsViewSupported(CShellViews* that, UINT uViewSet,
    SHELLVIEWID  const*pvid)
{
    CViewSet* pViewSet = (CViewSet*)DPA_GetPtr(that->_dpaViews, uViewSet);
    if (!pViewSet)
    {
        return(FALSE);
    }

    return(CViewSet_IsViewSupported(pViewSet, pvid));
}


int DPA_CViewSet_DeleteCallback(LPVOID p, LPVOID d)
{
    if (p)
        CViewSet_Delete((CViewSet*)p);
    return 1;
}

void CShellViews_Delete(CShellViews* that)
{
    if (that->_dpaViews)
    {
        DPA_DestroyCallback(that->_dpaViews, DPA_CViewSet_DeleteCallback, 0);
        that->_dpaViews = NULL;
    }
}


BOOL FileCabinet_GetDefaultViewID2(FOLDERSETDATABASE* that, SHELLVIEWID* pvid)
{
    if (CShellViews_Init(&that->_cViews))
    {
        CShellViews_GetDefaultView(&that->_cViews, that->_iViewSet, pvid);
        return(TRUE);
    }

    return(FALSE);
}


HRESULT FileCabinet_CreateViewWindow2(IShellBrowser* psb, FOLDERSETDATABASE* that, IShellView *psvNew,
    IShellView *psvOld, RECT *prcView, HWND *phWnd)
{
    SHELLVIEWID vid, vidOld, vidRestore;
    IShellView2 *psv2New;
    CViewSet *pThisView;
    DWORD dwViewPriority;
    BOOL bCalledSV2 = FALSE;
    HRESULT hres = S_OK;  // init to avoid a bogus C4701 warning

    if (!CShellViews_Init(&that->_cViews))
    {
        // Can't do anything with view sets; just do the old thing
        goto OldStyle;
    }

    // Default to whatever the last "old-style" view is
    CShellViews_GetDefaultView(&that->_cViews, 0, &vidOld);

    if (psvOld)
    {
        IShellView2 *psv2Old;

        if (SUCCEEDED(IUnknown_QueryInterface(psvOld, IID_IShellView2,
                                              &psv2Old)))
        {
            // Try to get the current view
            if (NOERROR == IShellView2_GetView(psv2Old, &vidOld, SV2GV_CURRENTVIEW))
            {
                CShellViews_SetDefaultView(&that->_cViews, that->_iViewSet, &vidOld);
            }

            IUnknown_Release(psv2Old);
        }
        else
        {
            // Get the view ID from the folder settings
            ViewIDFromViewMode(that->_fld._fs.ViewMode, &vidOld);
            CShellViews_SetDefaultView(&that->_cViews, 0, &vidOld);
        }
    }

    pThisView = CViewSet_New();
    if (!pThisView)
    {
        goto OldStyle;
    }

    if (SUCCEEDED(IUnknown_QueryInterface(psvNew, IID_IShellView2, &psv2New)))
    {
        SHELLVIEWID vidFolderDefault;
        if (NOERROR == IShellView2_GetView(psv2New, &vidFolderDefault, SV2GV_DEFAULTVIEW))
        {
            // we can now make up a view set for that folder
            if (CViewSet_Add(pThisView, &vidFolderDefault) >= 0)
            {
                int iViewSet;
                UINT uView;
                BOOL bNew;

                for (uView=0; NOERROR==IShellView2_GetView(psv2New, &vid, uView);
                    ++uView)
                {
                    CViewSet_Add(pThisView, &vid);
                }

                // Add that view set.  we will get an existing view set if it is
                // a duplicate
                iViewSet = CShellViews_Add(&that->_cViews, pThisView, &bNew);
                // This is now owned by CShellViews
                pThisView = NULL;

                
                //
                // Here is where we decide which view we want to use.
                //

                // Start with what came from the FOLDERSETDATABASE, then see if
                // anyone else has a higher VIEW_PRIORITY_XXX that would override this one.
                vidRestore = that->_fld._vidRestore;
                dwViewPriority = that->_fld._dwViewPriority;
                that->_fld._dwViewPriority = VIEW_PRIORITY_NONE;


                // Make sure that what we got is a supported view
                if (!CShellViews_IsViewSupported(&that->_cViews, iViewSet, &vidRestore))
                {
                    // Oops, that view isn't supported by this shell ext.
                    // Set the priority to NONE so that one of the others will override it.
                    dwViewPriority = VIEW_PRIORITY_NONE;
                }

                // Let the shell ext select the view if it has higher priority than
                // what we already have, and it is supported as well.
                if (dwViewPriority <= VIEW_PRIORITY_SHELLEXT &&
                    vidFolderDefault != VID_LargeIcons &&
                    CShellViews_IsViewSupported(&that->_cViews, iViewSet, &vidFolderDefault))
                {
                    // shell extension is more important
                    vidRestore = vidFolderDefault;
                    dwViewPriority = VIEW_PRIORITY_SHELLEXT;
                }

                // Maybe we can inherit it from the previous view...
                if (dwViewPriority <= VIEW_PRIORITY_INHERIT &&
                    psvOld &&
                    bNew &&
                    CShellViews_IsViewSupported(&that->_cViews, iViewSet, &vidOld))
                {
                    // We just navigated from another shell view. Use the same view as the last
                    // folder.
                    vidRestore = vidOld;
                    dwViewPriority = VIEW_PRIORITY_INHERIT;
                }

                // We're getting really desperate now...
                if (dwViewPriority <= VIEW_PRIORITY_DESPERATE)
                {
                    // Try the last view for the folders current viewset.
                    CShellViews_GetDefaultView(&that->_cViews, iViewSet, &vidRestore);
                    dwViewPriority = VIEW_PRIORITY_DESPERATE;
                }
                  
                // All finished trying to figure out what view to use
                ASSERT(dwViewPriority > VIEW_PRIORITY_NONE);

                // assure webview no in vid, it is persisted in shellstate now.
                {
                    SV2CVW2_PARAMS cParams =
                    {
                        SIZEOF(SV2CVW2_PARAMS),

                        psvOld,
                        &that->_fld._fs,
                        psb,
                        prcView,
                        &vidRestore,

                        NULL,
                    } ;

                    hres = IShellView2_CreateViewWindow2(psv2New, &cParams);
                    bCalledSV2 = TRUE;
                    *phWnd = cParams.hwndView;
                }

                if (SUCCEEDED(hres))
                {
                    that->_iViewSet = iViewSet;
                }
            }
        }

        IUnknown_Release(psv2New);
    }

    if (pThisView)
    {
        CViewSet_Delete(pThisView);
    }

    if (bCalledSV2)
    {
        return(hres);
    }

OldStyle:
    that->_iViewSet = 0;
    return IShellView_CreateViewWindow(psvNew, psvOld, &that->_fld._fs, (IShellBrowser*)psb, prcView, phWnd);
}
