// gfldset.cpp
//
//  A class that manages global folder settings.

#include "priv.h"
#include "sccls.h"

class CGlobalFolderSettings : public IGlobalFolderSettings
{
    public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG,AddRef)(THIS);
    STDMETHOD_(ULONG,Release)(THIS);

    // *** IGlobalFolderSettings meTHODs ***
    STDMETHOD(Get)(THIS_ DEFFOLDERSETTINGS *pdfs, int cbDfs);
    STDMETHOD(Set)(THIS_ const DEFFOLDERSETTINGS *pdfs, int cbDfs, UINT flags);

    protected:
        friend HRESULT CGlobalFolderSettings_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

        CGlobalFolderSettings();
        ~CGlobalFolderSettings();

        LONG            m_cRef;
};

STDAPI CGlobalFolderSettings_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    // class factory will take care of denying aggregation and giving a null
    // back on failure
    ASSERT(pUnkOuter == NULL);

    CGlobalFolderSettings* pid = new CGlobalFolderSettings();

    if (pid)
    {
        *ppunk = SAFECAST(pid, IGlobalFolderSettings*);
        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
CGlobalFolderSettings::CGlobalFolderSettings() : m_cRef(1)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
CGlobalFolderSettings::~CGlobalFolderSettings()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CGlobalFolderSettings::QueryInterface ( REFIID riid, LPVOID * ppvObj )
{
    if ( riid == IID_IUnknown || riid == IID_IGlobalFolderSettings)
    {
        *ppvObj = SAFECAST( this, IGlobalFolderSettings *);
        AddRef();
    }
    else
    {
        return E_NOINTERFACE;
    }

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CGlobalFolderSettings:: AddRef ()
{
    InterlockedIncrement( &m_cRef );
    return m_cRef;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CGlobalFolderSettings:: Release ()
{
    if ( InterlockedDecrement( &m_cRef ) == 0 )
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CGlobalFolderSettings::Get(DEFFOLDERSETTINGS *pdfs, int cbDfs)
{
    if (cbDfs < sizeof(DEFFOLDERSETTINGS))
    {
        return E_INVALIDARG;
    }
    else if (cbDfs > sizeof(DEFFOLDERSETTINGS))
    {
        ZeroMemory(pdfs, cbDfs);
    }

    *pdfs = g_dfs;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CGlobalFolderSettings::Set(const DEFFOLDERSETTINGS *pdfs, int cbDfs, UINT flags)
{
    if (flags & ~GFSS_VALID) {
        ASSERT(!"Invalid flags passed to CGlobalFolderSettings::Set");
        return E_INVALIDARG;
    }

    //
    //  Special hack:  If you pass (NULL, 0) then it means "reset to
    //  default".
    //
    if (pdfs == NULL && cbDfs == 0)
    {
        static DEFFOLDERSETTINGS dfs = INIT_DEFFOLDERSETTINGS;
        dfs.vid = g_bRunOnNT5 ? VID_LargeIcons : DFS_VID_Default;
        pdfs = &dfs;
    }
    else if (cbDfs < sizeof(DEFFOLDERSETTINGS))
    {
        ASSERT(!"Invalid cbDfs passed to CGlobalFolderSettings::Set");
        return E_INVALIDARG;
    }

    // Preserve the dwDefRevCount, otherwise we'll never be able
    // to tell if the structure has been revised!
    DWORD dwDefRevCount = g_dfs.dwDefRevCount;
    g_dfs = *pdfs;
    g_dfs.dwDefRevCount = dwDefRevCount;
    SaveDefaultFolderSettings(flags);
    return S_OK;
}

