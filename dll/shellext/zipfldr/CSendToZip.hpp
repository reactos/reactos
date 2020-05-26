/*
 * PROJECT:   ReactOS Zip Shell Extension
 * LICENSE:   GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:   SendTo handler
 * COPYRIGHT: Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 *            Copyright 2019 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#ifndef CSENDTOZIP_HPP_
#define CSENDTOZIP_HPP_

class CSendToZip :
    public CComCoClass<CSendToZip, &CLSID_ZipFolderSendTo>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDropTarget,
    public IPersistFile
{
    CComPtr<IDataObject> m_pDataObject;
    BOOL m_fCanDragDrop;

public:
    CSendToZip() : m_fCanDragDrop(FALSE)
    {
        InterlockedIncrement(&g_ModuleRefCnt);
    }

    virtual ~CSendToZip()
    {
        InterlockedDecrement(&g_ModuleRefCnt);
    }

    // *** IShellFolder2 methods ***
    STDMETHODIMP DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave();
    STDMETHODIMP Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // *** IPersistFile methods ***
    STDMETHODIMP IsDirty()
    {
        return S_FALSE;
    }
    STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode)
    {
        return S_OK;
    }
    STDMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember)
    {
        return E_NOTIMPL;
    }
    STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName)
    {
        return E_NOTIMPL;
    }
    STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName)
    {
        return E_NOTIMPL;
    }

    // *** IPersist methods ***
    STDMETHODIMP GetClassID(CLSID *pclsid)
    {
        *pclsid = CLSID_ZipFolderSendTo;
        return S_OK;
    }

public:
    DECLARE_NO_REGISTRY()   // Handled manually
    DECLARE_NOT_AGGREGATABLE(CSendToZip)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSendToZip)
        COM_INTERFACE_ENTRY_IID(IID_IDropTarget, IDropTarget)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFile, IPersistFile)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
    END_COM_MAP()
};

#endif
