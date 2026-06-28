/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     IExplorerCommand implementation
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

class CExplorerCommand :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IExplorerCommand
{
private:
    CComPtr<IContextMenu> m_pZipObject;

public:

    STDMETHODIMP Initialize(IContextMenu* zipObject)
    {
        m_pZipObject = zipObject;
        return S_OK;
    }

    // *** IExplorerCommand methods ***
    STDMETHODIMP GetTitle(IShellItemArray *psiItemArray, PWSTR *ppszName) override
    {
        CStringW Title(MAKEINTRESOURCEW(IDS_MENUITEM));
        return SHStrDup(Title, ppszName);
    }
    STDMETHODIMP GetIcon(IShellItemArray *psiItemArray, PWSTR *ppszIcon) override
    {
        CStringW IconName = L"zipfldr.dll,-1";
        return SHStrDup(IconName, ppszIcon);
    }
    STDMETHODIMP GetToolTip(IShellItemArray *psiItemArray, PWSTR *ppszInfotip) override
    {
        CStringW HelpText(MAKEINTRESOURCEW(IDS_HELPTEXT));
        return SHStrDup(HelpText, ppszInfotip);
    }
    STDMETHODIMP GetCanonicalName(GUID *pguidCommandName) override
    {
        *pguidCommandName = CLSID_ZipFolderExtractAllCommand;
        return S_OK;
    }
    STDMETHODIMP GetState(IShellItemArray *psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE *pCmdState) override
    {
        *pCmdState = ECS_ENABLED;
        return S_OK;
    }
    STDMETHODIMP Invoke(IShellItemArray *psiItemArray, IBindCtx *pbc) override
    {
        CMINVOKECOMMANDINFO cm = { sizeof(cm), 0 };
        cm.lpVerb = EXTRACT_VERBA;
        cm.nShow = SW_SHOW;
        return m_pZipObject->InvokeCommand(&cm);
    }
    STDMETHODIMP GetFlags(EXPCMDFLAGS *pFlags) override
    {
        *pFlags = ECF_DEFAULT;
        return S_OK;
    }
    STDMETHODIMP EnumSubCommands(IEnumExplorerCommand **ppEnum) override
    {
        DbgPrint("%s\n", __FUNCTION__);
        return E_NOTIMPL;
    }

public:
    DECLARE_NOT_AGGREGATABLE(CExplorerCommand)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CExplorerCommand)
        COM_INTERFACE_ENTRY_IID(IID_IExplorerCommand, IExplorerCommand)
    END_COM_MAP()
};


class CEnumExplorerCommand :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumExplorerCommand
{
private:
    bool m_bFirst;
    CComPtr<IContextMenu> m_pZipObject;

public:

    CEnumExplorerCommand()
        :m_bFirst(true)
    {
    }

    STDMETHODIMP Initialize(IContextMenu* zipObject)
    {
        m_pZipObject = zipObject;
        return S_OK;
    }

    // *** IEnumExplorerCommand methods ***
    STDMETHODIMP Next(ULONG celt, IExplorerCommand **pUICommand, ULONG *pceltFetched) override
    {
        if (!pUICommand)
            return E_POINTER;

        if (pceltFetched)
            *pceltFetched = 0;
        if (m_bFirst && celt)
        {
            m_bFirst = false;
            celt--;
            HRESULT hr = ShellObjectCreatorInit<CExplorerCommand>(m_pZipObject, IID_PPV_ARG(IExplorerCommand, pUICommand));
            if (SUCCEEDED(hr))
            {
                if (pceltFetched)
                    *pceltFetched = 1;
            }
            return hr;
        }
        return S_FALSE;
    }
    STDMETHODIMP Skip(ULONG celt) override
    {
        if (m_bFirst)
        {
            m_bFirst = false;
            return S_OK;
        }
        return S_FALSE;
    }
    STDMETHODIMP Reset() override
    {
        m_bFirst = true;
        return S_OK;
    }
    STDMETHODIMP Clone(IEnumExplorerCommand **ppenum) override
    {
        return E_NOTIMPL;
    }

public:
    DECLARE_NOT_AGGREGATABLE(CEnumExplorerCommand)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CEnumExplorerCommand)
        COM_INTERFACE_ENTRY_IID(IID_IEnumExplorerCommand, IEnumExplorerCommand)
    END_COM_MAP()
};

class CExplorerCommandProvider :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IExplorerCommandProvider
{
private:
    CComPtr<IContextMenu> m_pZipObject;

public:
    STDMETHODIMP Initialize(IContextMenu* zipObject)
    {
        m_pZipObject = zipObject;
        return S_OK;
    }

    // *** IExplorerCommandProvider methods ***
    STDMETHODIMP GetCommands(IUnknown *punkSite, REFIID riid, void **ppv) override
    {
        return ShellObjectCreatorInit<CEnumExplorerCommand>(m_pZipObject, riid, ppv);
    }
    STDMETHODIMP GetCommand(REFGUID rguidCommandId, REFIID riid, void **ppv) override
    {
        UNIMPLEMENTED;
        *ppv = NULL;
        return E_NOTIMPL;
    }

public:
    DECLARE_NOT_AGGREGATABLE(CExplorerCommandProvider)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CExplorerCommandProvider)
        COM_INTERFACE_ENTRY_IID(IID_IExplorerCommandProvider, IExplorerCommandProvider)
    END_COM_MAP()
};


HRESULT _CExplorerCommandProvider_CreateInstance(IContextMenu* zipObject, REFIID riid, LPVOID * ppvOut)
{
    return ShellObjectCreatorInit<CExplorerCommandProvider>(zipObject, riid, ppvOut);
}
