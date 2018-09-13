#include "shellprv.h"
#include "sfviewp.h"
#include "views.h"
#include "ids.h"
#include "printer.h"

//
// Printers stuff
//


#define PRINTERS_EVENTS \
    SHCNE_UPDATEITEM | \
    SHCNE_DELETE | \
    SHCNE_RENAMEITEM | \
    SHCNE_ATTRIBUTES | \
    SHCNE_CREATE


class CPrinterFolderViewCB : public CBaseShellFolderViewCB
{
public:
    CPrinterFolderViewCB(IShellFolder* psf, CPrinterFolder *pPrinterFolder, LPCITEMIDLIST pidl)
        : CBaseShellFolderViewCB(psf, pidl, PRINTERS_EVENTS), 
          m_pPrinterFolder(pPrinterFolder)
        { }

    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    HRESULT OnMERGEMENU(DWORD pv, QCMINFO*lP)
    {
        CDefFolderMenu_MergeMenu(HINST_THISDLL, 0, POPUP_PRINTERS_POPUPMERGE, lP);
        return S_OK;
    }

    HRESULT OnINVOKECOMMAND(DWORD pv, UINT wP)
    {
        return CPrinters_DFMCallBackBG(m_pshf, m_hwndMain, NULL, DFM_INVOKECOMMAND, wP, 0);
    }

    HRESULT OnGETHELPTEXT(DWORD pv, UINT id, UINT cch, LPTSTR lP)
    {
#ifdef UNICODE
        return CPrinters_DFMCallBackBG(m_pshf, m_hwndMain, NULL, DFM_GETHELPTEXTW, MAKEWPARAM(id, cch), (LPARAM)lP);
#else
        return CPrinters_DFMCallBackBG(m_pshf, m_hwndMain, NULL, DFM_GETHELPTEXT, MAKEWPARAM(id, cch), (LPARAM)lP);
#endif
    }


    HRESULT OnDEFITEMCOUNT(DWORD pv, UINT*lP)
    {
        // if we time out enuming items make a guess at how many items there
        // will be to make sure we get large icon mode and a reasonable window size

        *lP = 20;
        return S_OK;
    }

    HRESULT OnBACKGROUNDENUM(DWORD pv)
    {
#ifdef WINNT
        return m_pPrinterFolder->pszServer ? S_OK : E_FAIL;
#else
        return E_FAIL;
#endif
    }

    HRESULT OnREFRESH(DWORD pv, UINT wP)
    {
#ifdef WINNT
        if( wP )
        {
            // if wP == TRUE this means that the defview is about to be 
            // refreshed, so invalidate the print folder cache here.
            if (m_pPrinterFolder->hFolder)
            {
                bFolderRefresh(m_pPrinterFolder->hFolder, &m_pPrinterFolder->bShowAddPrinter);
                m_pPrinterFolder->bRefreshed = TRUE;
                return S_OK;
            }
        }
        return E_FAIL;
#else
        return E_FAIL;
#endif
    }

    HRESULT OnGETHELPTOPIC(DWORD pv, SFVM_HELPTOPIC_DATA * phtd)
    {
        StrCpyW( phtd->wszHelpFile, L"printing.chm" );
        return S_OK;
    }

    CPrinterFolder *m_pPrinterFolder;
};


STDMETHODIMP CPrinterFolderViewCB::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_MERGEMENU, OnMERGEMENU);
    HANDLE_MSG(0, SFVM_INVOKECOMMAND, OnINVOKECOMMAND);
    HANDLE_MSG(0, SFVM_GETHELPTEXT, OnGETHELPTEXT);
    HANDLE_MSG(0, SFVM_DEFITEMCOUNT, OnDEFITEMCOUNT);
    HANDLE_MSG(0, SFVM_BACKGROUNDENUM, OnBACKGROUNDENUM);
    HANDLE_MSG(0, SFVM_GETHELPTOPIC, OnGETHELPTOPIC);
    HANDLE_MSG(0, SFVM_REFRESH, OnREFRESH);

    default:
        return E_FAIL;
    }

    return NOERROR;
}


STDAPI_(IShellFolderViewCB *) Printer_CreateSFVCB(IShellFolder2* psf,
                                        CPrinterFolder *pPrinterFolder,
                                        LPCITEMIDLIST pidl)
{
    return new CPrinterFolderViewCB(psf, pPrinterFolder, pidl);
}

