/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     IShellFolderViewCB implementation
 * COPYRIGHT:   Copyright 2017 David Quintana (gigaherz@gmail.com)
 *              Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

class CFolderViewCB :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellFolderViewCB
{
public:

    virtual ~CFolderViewCB()
    {
    }

    // *** IShellFolderViewCB methods ***
    STDMETHODIMP MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        /* TODO: Handle SFVM_GET_WEBVIEW_CONTENT to add tasks */
        switch (uMsg)
        {
        case SFVM_DEFVIEWMODE:
        {
            FOLDERVIEWMODE* pViewMode = (FOLDERVIEWMODE*)lParam;
            *pViewMode = FVM_DETAILS;
            return S_OK;
        }
        case SFVM_COLUMNCLICK:
            return S_FALSE;
        case SFVM_BACKGROUNDENUM:
            return S_OK;
        }

        return E_NOTIMPL;
    }

public:
    DECLARE_NOT_AGGREGATABLE(CFolderViewCB)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CFolderViewCB)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolderViewCB, IShellFolderViewCB)
    END_COM_MAP()
};

HRESULT _CFolderViewCB_CreateInstance(REFIID riid, LPVOID * ppvOut)
{
    return ShellObjectCreator<CFolderViewCB>(riid, ppvOut);
}
