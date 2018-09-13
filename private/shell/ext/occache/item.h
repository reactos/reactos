#ifndef __CONTROL_ITEM__
#define __CONTROL_ITEM__

#include "folder.h"
#include "cdlbsc.hpp"

class CControlItem : public IDataObject,
                     public IExtractIcon,
                     public IContextMenu
{
    // CControlItem interfaces
    friend HRESULT ControlFolderView_DidDragDrop(
                                            HWND hwnd, 
                                            IDataObject *pdo, 
                                            DWORD dwEffect);

public:
    CControlItem();
    HRESULT Initialize(
                   CControlFolder *pCFolder, 
                   UINT cidl, 
                   LPCITEMIDLIST *ppidl);

    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IContextMenu Methods
    STDMETHODIMP QueryContextMenu(
                            HMENU hmenu, 
                            UINT indexMenu, 
                            UINT idCmdFirst,
                            UINT idCmdLast, 
                            UINT uFlags);

    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);

    STDMETHODIMP GetCommandString(
                             UINT_PTR idCmd, 
                             UINT uType,
                             UINT *pwReserved,
                             LPTSTR pszName, 
                             UINT cchMax);

    // IDataObject Methods...
    STDMETHODIMP GetData(LPFORMATETC pFEIn, LPSTGMEDIUM pSTM);
    STDMETHODIMP GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pSTM);
    STDMETHODIMP QueryGetData(LPFORMATETC pFE);
    STDMETHODIMP GetCanonicalFormatEtc(LPFORMATETC pFEIn, LPFORMATETC pFEOut);
    STDMETHODIMP SetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM, BOOL fRelease);
    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC *ppEnum);
    STDMETHODIMP DAdvise(LPFORMATETC pFE, DWORD grfAdv, LPADVISESINK pAdvSink,
                            LPDWORD pdwConnection);
    STDMETHODIMP DUnadvise(DWORD dwConnection);
    STDMETHODIMP EnumDAdvise(LPENUMSTATDATA *ppEnum);

    // IDataObject helper functions
    HRESULT CreatePrefDropEffect(STGMEDIUM *pSTM);
    HRESULT Remove(HWND hwnd);
/*
    HRESULT _CreateHDROP(STGMEDIUM *pmedium);
    HRESULT _CreateNameMap(STGMEDIUM *pmedium);
    HRESULT _CreateFileDescriptor(STGMEDIUM *pSTM);
    HRESULT _CreateFileContents(STGMEDIUM *pSTM, LONG lindex);
    HRESULT _CreateURL(STGMEDIUM *pSTM);
    HRESULT _CreatePrefDropEffect(STGMEDIUM *pSTM);
*/

    // IExtractIcon Methods
    STDMETHODIMP GetIconLocation(
                            UINT uFlags,
                            LPSTR szIconFile,
                            UINT cchMax,
                            int *piIndex,
                            UINT *pwFlags);
    STDMETHODIMP Extract(
                    LPCSTR pszFile,
                    UINT nIconIndex,
                    HICON *phiconLarge,
                    HICON *phiconSmall,
                    UINT nIconSize);

    // Support for our progress UI 
    static INT_PTR DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

    // Misc helper function

    static BOOL IsGlobalOffline();

protected:

    ~CControlItem();

    HRESULT Update(LPCMINVOKECOMMANDINFO pici, LPCONTROLPIDL pcpidl);

    UINT                 m_cRef;            // reference count
    UINT                 m_cItems;          // number of items we represent
    CControlFolder*  m_pCFolder;    // back pointer to our shell folder
    LPCONTROLPIDL*       m_ppcei;           // variable size array of items
    LPCMINVOKECOMMANDINFO m_piciUpdate;
    LPCONTROLPIDL         m_pcpidlUpdate;
    CodeDownloadBSC      *m_pcdlbsc;
};

#endif
