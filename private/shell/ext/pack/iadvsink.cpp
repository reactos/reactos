#include "priv.h"
#include "privcpp.h"


CPackage_IAdviseSink::CPackage_IAdviseSink(CPackage *pPackage) : 
    _pPackage(pPackage)
{
    ASSERT(_cRef == 0);
}

CPackage_IAdviseSink::~CPackage_IAdviseSink()
{
    DebugMsg(DM_TRACE,"CPackage_IAdviseSink destroyed with ref count %d",_cRef);
}


//////////////////////////////////
//
// IUnknown Methods...
//
HRESULT CPackage_IAdviseSink::QueryInterface(REFIID iid, void ** ppv)
{
    return _pPackage->QueryInterface(iid,ppv);
}

ULONG CPackage_IAdviseSink::AddRef(void) 
{
    _cRef++;    // interface ref count for debugging
    return _pPackage->AddRef();
}

ULONG CPackage_IAdviseSink::Release(void)
{
    _cRef--;    // interface ref count for debugging
    return _pPackage->Release();
}

//////////////////////////////////
//
// IAdviseSink Methods...
//

void CPackage_IAdviseSink::OnDataChange(LPFORMATETC, LPSTGMEDIUM)
{
    // NOTE: currently, we never set up a data advise connection with
    // anyone, but if we ever do, we'll want to set our dirty flag
    // when we get a datachange notificaiton.
    
    DebugMsg(DM_TRACE, "pack as - OnDataChange() called.");
    // when we get a data change notification, set our dirty flag
    _pPackage->_fIsDirty = TRUE;
    return;
}


void CPackage_IAdviseSink::OnViewChange(DWORD, LONG) 
{
    DebugMsg(DM_TRACE, "pack as - OnViewChange() called.");
    //
    // there's nothing to do here....we don't care about view changes.
    // we are always viewed as an icon and that can't be changed by the server
    // which is run when the contents are activated.  the icon can
    // only be changed through the edit package verb
    //
    return;
}

void CPackage_IAdviseSink::OnRename(LPMONIKER)
{
    DebugMsg(DM_TRACE, "pack as - OnRename() called.");
    //
    // once again, nothing to do here...if the user for some unknown reason
    // tries to save the packaged file by a different name when he's done
    // editing the contents then we'll just give not receive those changes.
    // why would anyone want to rename a temporary file, anyway?
    //
    return;
}

void CPackage_IAdviseSink::OnSave(void)
{
    DebugMsg(DM_TRACE, "pack as - OnSave() called.");
    // if the contents have been saved, then our storage is out of date,
    // so set our dirty flag, then the container can choose to save us or not
    _pPackage->_fIsDirty;
    
    // we just notifiy our own container that we've been saved and it 
    // can do whatever it wants to.
    _pPackage->_pIOleAdviseHolder->SendOnSave();
    
}

void CPackage_IAdviseSink::OnClose(void) 
{
    DebugMsg(DM_TRACE, "pack as - OnClose() called.");

    switch(_pPackage->_panetype)
    {
    case PEMBED:
	// get rid of advsiory connnection
	_pPackage->_pEmbed->poo->Unadvise(_pPackage->_dwCookie);
	_pPackage->_pEmbed->poo->Release();
	_pPackage->_pEmbed->poo = NULL;

    // this updates the size of the packaged file in out _pPackage->_pEmbed
    if (FAILED(_pPackage->EmbedInitFromFile(_pPackage->_pEmbed->pszTempName, FALSE)))
    {
        ShellMessageBox(g_hinst,
                        NULL,
                        MAKEINTRESOURCE(IDS_UPDATE_ERROR),
                        MAKEINTRESOURCE(IDS_APP_TITLE),
                        MB_TASKMODAL | MB_ICONERROR | MB_OK);
    }

    if (FAILED(_pPackage->_pIOleClientSite->SaveObject()))
    {
        ShellMessageBox(g_hinst,
                        NULL,
                        MAKEINTRESOURCE(IDS_UPDATE_ERROR),
                        MAKEINTRESOURCE(IDS_APP_TITLE),
                        MB_TASKMODAL | MB_ICONERROR | MB_OK);
    }

	_pPackage->_pIOleAdviseHolder->SendOnSave();

	_pPackage->_pIOleClientSite->OnShowWindow(FALSE);
    
	// we just notify out own container that we've been closed and let
	// it do whatever it wants to.
	_pPackage->_pIOleAdviseHolder->SendOnClose();

	break;

    case CMDLINK:
	// there shouldn't be anything to do here, since a CMDLINK is always
	// executed using ShellExecute and never through OLE, so who would be
	// setting up an advisory connection with the package?
	break;
    }
    
}
    
