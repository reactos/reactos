/*----------------------------------------------------------------------------
/ Title;
/   viewcb.cpp
/
/ Authors;
/   Rick Turner (ricktu)
/
/ Notes;
/   Implements IShellFolderViewCB for My Documents code.
/----------------------------------------------------------------------------*/
#include "precomp.hxx"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ CMyDocsViewCB
/   This is the My Documents IShellFolderViewCB implementation.
/----------------------------------------------------------------------------*/

CMyDocsViewCB::CMyDocsViewCB( IShellFolderView * psfv, LPITEMIDLIST pidl )
{
    MDTraceEnter(TRACE_CALLBACKS, "CMyDocsViewCB::CMyDocsViewCB");

    m_psfv = psfv;
    m_pidlReal = ILClone(pidl);
    m_psfvcb = NULL;

    MDTraceLeave();
}

CMyDocsViewCB::~CMyDocsViewCB()
{
    MDTraceEnter(TRACE_CALLBACKS, "CMyDocsViewCB::~CMyDocsViewCB");

    DoILFree( m_pidlReal );
    // DoRelease( m_psfv );  // Not currently Addrefing this...
    DoRelease( m_psfvcb );

    MDTraceLeave();
}


STDMETHODIMP
CMyDocsViewCB::SetRealCB(IShellFolderViewCB * psfvcb)
{
    HRESULT hr = S_OK;

    MDTraceEnter( TRACE_CALLBACKS, "CMyDocsViewCB::SetRealCB" );

    DoRelease( m_psfvcb );
    m_psfvcb = psfvcb;

    MDTraceLeaveResult(hr);
}



#undef CLASS_NAME
#define CLASS_NAME CMyDocsViewCB
#include "unknown.inc"

STDMETHODIMP
CMyDocsViewCB::QueryInterface(REFIID riid, LPVOID* ppvObject)
{

    MDTraceEnter( TRACE_QI, "CMyDocsViewCB::QueryInterface" );
    MDTraceGUID("Interface requested", riid);

    HRESULT hr;

    INTERFACES iface[] =
    {
        &IID_IShellFolderViewCB, (IShellFolderViewCB *)this,
    };

    if (IsEqualIID(riid, IID_IShellFolderViewCB) || IsEqualIID(riid, IID_IUnknown))
    {
        hr = HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
    }
    else
    {
        MDTrace(TEXT("Getting interface from m_psfvcb->QI directly..."));
        // This is totally bogus, but we may need to allow the query to go through to
        // the one we are holding onto as the compatability one juses this to extract
        // additional information...
        hr = m_psfvcb->QueryInterface(riid, ppvObject);
    }

    MDTraceLeaveResult(hr);
}


STDMETHODIMP
CMyDocsViewCB::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = E_FAIL;

    MDTraceEnter( TRACE_CALLBACKS, "CMyDocsViewCB::MessageSFVCB" );
    MDTraceViewMsg( uMsg, wParam, lParam );

    switch (uMsg)
    {
        case SFVM_GETHELPTEXT:
        {
            UINT id = LOWORD(wParam);
            UINT cchBuf = HIWORD(wParam);
            LPTSTR pszBuf = (LPTSTR)lParam;

            if (LoadString(g_hInstance, id+IDS_SORT_FIRST, pszBuf, cchBuf))
            {
                hr = S_OK;
            }

        }
            break;

        case SFVM_GETNOTIFY:
        {
            LPITEMIDLIST *  ppidl   = (LPITEMIDLIST *)wParam;
            LONG *          pEvents = (LONG *)lParam;

            hr = m_psfvcb->MessageSFVCB( uMsg, wParam, lParam );

            *ppidl = m_pidlReal;
            if (FAILED(hr))
            {
                *pEvents = SHCNE_DELETE | SHCNE_CREATE | SHCNE_RENAMEITEM |
                           SHCNE_UPDATEDIR | SHCNE_DISKEVENTS;
                hr = S_OK;
            }
        }
            break;

        case SFVM_INSERTITEM:
        {
            LPITEMIDLIST pidl = (LPITEMIDLIST) lParam;
            TCHAR szPath[ MAX_PATH ];
            LPTSTR pszName;

            if (MDGetPathFromIDL(pidl, szPath, NULL))
            {
                // Always hide the desktop.ini file.
                pszName = PathFindFileName( szPath );
                if ( 0 == lstrcmpi(pszName, TEXT("Desktop.ini")) )
                {
                    hr = S_FALSE;
                }
            }
        }
            break;

        case SFVM_INVOKECOMMAND:
            MDTraceAssert( m_psfv );
            if (m_psfv)
                hr = m_psfv->Rearrange( (UINT)wParam );
            break;

        case SFVM_MERGEMENU:
            hr = _MergeArrangeMenu( (LPQCMINFO)lParam );
            break;

        case SFVM_GETHELPTOPIC:
        {
            SFVM_HELPTOPIC_DATA * phtd = (SFVM_HELPTOPIC_DATA *)lParam;
            StrCpyW( phtd->wszHelpFile, L"mydoc.chm" );
            hr = S_OK;
        }
            break;

        case DVM_DEFVIEWMODE:
            *(FOLDERVIEWMODE *)lParam = FVM_ICON;
            hr = S_OK;
            break;

    }

    if (FAILED(hr))
    {
        MDTrace(TEXT("Calling m_psfvcb..."));
        hr = m_psfvcb->MessageSFVCB( uMsg, wParam, lParam );
    }

    MDTraceLeaveResult(hr);

}

