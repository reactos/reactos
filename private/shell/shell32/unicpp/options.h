#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <iethread.h>
#include <browseui.h>


#include "cowsite.h"

void DoGlobalFolderOptions(void);

class CFolderOptionsPsx :
        public
               IShellPropSheetExt,
               IShellExtInit,
               CObjectWithSite
{
public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IShellPropSheetExt
    STDMETHOD(AddPages)(THIS_ LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    STDMETHOD(ReplacePage)(THIS_ UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);

    // IShellExtInit
    STDMETHOD(Initialize)(THIS_ LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);

    void SetNeedRefresh(BOOL fNeedRefresh) { m_fNeedRefresh = fNeedRefresh; }
    BOOL NeedRefresh() { return m_fNeedRefresh; }

    BOOL HasBrowserService() { return m_pbs2 != NULL; }

    void SetAsDefFolderSettings()
    { if (HasBrowserService()) m_pbs2->SetAsDefFolderSettings(); }

    HRESULT GetDefFolderSettings(DEFFOLDERSETTINGS *pdfs, int cbDfs)
                { return m_pgfs->Get(pdfs, cbDfs); }
    HRESULT SetDefFolderSettings(const DEFFOLDERSETTINGS *pdfs, int cbDfs, UINT flags)
                { return m_pgfs->Set(pdfs, cbDfs, flags); }
    HRESULT ResetDefFolderSettings()
                { IUnknown_Exec(m_pbs2, &CGID_DefView, DVID_RESETDEFAULT, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
                  return m_pgfs->Set(NULL, 0, GFSS_SETASDEFAULT); }

private:
    CFolderOptionsPsx();
    ~CFolderOptionsPsx();

    LONG m_cRef;
    BOOL m_fNeedRefresh;
    IBrowserService2 *m_pbs2;
    IGlobalFolderSettings *m_pgfs;

    friend HRESULT CFolderOptionsPsx_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut);
    static UINT CALLBACK PropCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
};

BOOL_PTR CALLBACK FolderOptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif // _OPTIONS_H_
