#include "priv.h"
#include "privcpp.h"

// Constructor
CPackage_IPersistStorage::CPackage_IPersistStorage(CPackage *pPackage) : 
    _pPackage(pPackage)
{
    ASSERT(_cRef == 0); 
    ASSERT(_psState == PSSTATE_UNINIT);
}

CPackage_IPersistStorage::~CPackage_IPersistStorage()
{
    DebugMsg(DM_TRACE,"CPackage_IPersistStorage destroyed with ref count %d",_cRef);
}

//////////////////////////////////
//
// IUnknown Methods...
//
HRESULT CPackage_IPersistStorage::QueryInterface(REFIID iid, void ** ppv)
{
    return _pPackage->QueryInterface(iid,ppv);  // delegate to CPackage
}

ULONG CPackage_IPersistStorage::AddRef(void) 
{
    _cRef++;                                    // interface ref count for debug
    return _pPackage->AddRef();                 // delegate to CPackage
}

ULONG CPackage_IPersistStorage::Release(void)
{
    _cRef--;                                    // interface ref count for debug
    return _pPackage->Release();                // delegate to CPackage
}

//////////////////////////////////
//
// IPersistStorage Methods...
//
HRESULT CPackage_IPersistStorage::GetClassID(LPCLSID pClassID)
{
    DebugMsg(DM_TRACE, "pack ps - GetClassID() called.");
    
    if (_psState == PSSTATE_UNINIT)  
        return E_UNEXPECTED;
    
    if (pClassID == NULL)
        return E_INVALIDARG;
    
    *pClassID = CLSID_CPackage;
    return S_OK;
}

HRESULT CPackage_IPersistStorage::IsDirty(void)
{
    DebugMsg(DM_TRACE,
	"pack ps - IsDirty() called. _fIsDirty==%d",
	(INT)_pPackage->_fIsDirty);
    if (_psState == PSSTATE_UNINIT)  
        return E_UNEXPECTED;
    
    return (_pPackage->_fIsDirty ? S_OK : S_FALSE);
}

HRESULT CPackage_IPersistStorage::InitNew(IStorage *pstg)
{
    HRESULT hr;

    DebugMsg(DM_TRACE, "pack ps - InitNew() called.");
    
    if (_psState != PSSTATE_UNINIT)  
        return E_UNEXPECTED;
    
    if (!pstg)  
        return E_POINTER;
            
    // Create a stream to save the package and cache the pointer.  By doing 
    // this now we ensure being able to save in low memory conditions.
    //
    hr = pstg->CreateStream(SZCONTENTS,STGM_DIRECT | STGM_CREATE | 
                            STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, 
                            &_pPackage->_pstm);
    if (FAILED(hr)) 
        goto ErrRet;
    
    hr = WriteFmtUserTypeStg(pstg, _pPackage->_cf,SZUSERTYPE);
    if (FAILED(hr)) {
        goto ErrRet;
    }
     
    _pPackage->_fIsDirty = TRUE;
    _psState = PSSTATE_SCRIBBLE;
    
    _pPackage->_pIStorage = pstg;       // cache the IStorage pointer
    _pPackage->_pIStorage->AddRef();    // but don't forget to addref it!

    DebugMsg(DM_TRACE, "            leaving InitNew()");
    return S_OK;
    
ErrRet:
    if (_pPackage->_pstm) {
        _pPackage->_pstm->Release();
        _pPackage->_pstm = NULL;
    }
    return hr;
}

    
HRESULT CPackage_IPersistStorage::Load(IStorage *pstg)
{
    HRESULT     hr;
    LPSTREAM    pstm = NULL;         // package contents
    CLSID       clsid;

    DebugMsg(DM_TRACE, "pack ps - Load() called.");

    if (_psState != PSSTATE_UNINIT) {
        DebugMsg(DM_TRACE,"            wrong state!!");
        return E_UNEXPECTED;
    }
    
    if (!pstg) {
        DebugMsg(DM_TRACE,"            bad pointer!!");
        return E_POINTER;
    }
    
    
    // check to make sure this is one of our storages
    hr = ReadClassStg(pstg, &clsid);
    if (SUCCEEDED(hr) &&
	(clsid != CLSID_CPackage && clsid != CLSID_OldPackage) || FAILED(hr))
    {
        DebugMsg(DM_TRACE,"            bad storage type!!");
        return E_UNEXPECTED;
    }
    
    hr = pstg->OpenStream(SZCONTENTS,0, STGM_DIRECT | STGM_READWRITE | 
                          STGM_SHARE_EXCLUSIVE, 0, &pstm);
    if (FAILED(hr)) {
        DebugMsg(DM_TRACE,"            couldn't open contents stream!!");
        DebugMsg(DM_TRACE,"            hr==%Xh",hr);
        goto ErrRet;
    }

    hr = _pPackage->PackageReadFromStream(pstm);
    if (FAILED(hr))
        goto ErrRet;
    
    _pPackage->_pstm      = pstm;
    _pPackage->_pIStorage = pstg;
    pstg->AddRef();
    
    _psState = PSSTATE_SCRIBBLE;
    _pPackage->_fIsDirty = FALSE;
    _pPackage->_fLoaded  = TRUE;
    
    DebugMsg(DM_TRACE, "            leaving Load()");
    return S_OK;
    
ErrRet:
    if (pstm)
        pstm->Release();
    return hr;
}

    
HRESULT CPackage_IPersistStorage::Save(IStorage *pstg, BOOL fSameAsLoad)
{
    HRESULT     hr;
    LPSTREAM    pstm=NULL;

    DebugMsg(DM_TRACE, "pack ps - Save() called.");
    
    // must come here from scribble state
    if ((_psState != PSSTATE_SCRIBBLE) && fSameAsLoad) {
        DebugMsg(DM_TRACE,"            bad state!!");
        return E_UNEXPECTED;
    }
    
    // must have an IStorage if not SameAsLoad
    if (!pstg && !fSameAsLoad) {
        DebugMsg(DM_TRACE,"            bad pointer!!");
        return E_POINTER;
    }
    
    // hopefully, the container calls WriteClassStg with our CLSID before
    // we get here, that way we can overwrite that and write out the old
    // packager's CLSID so that the old packager can read new packages.
    //
    if (FAILED(WriteClassStg(pstg,CLSID_OldPackage))) {
        DebugMsg(DM_TRACE,
	    "            couldn't write CLSID to storage!!");
        return E_FAIL;
    }
    
    // 
    // ok, we have four possible ways we could be calling Save:
    //          1. we're creating a new package and saving to the same
    //             storage we received in InitNew
    //          2. We're creating a new package and saving to a different
    //             storage than we received in InitNew
    //          3. We were loaded by a container and we're saving to the
    //             same stream we received in Load
    //          4. We were loaded by a container and we're saving to a
    //             different stream than we received in Load
    //
    

    //////////////////////////////////////////////////////////////////
    //
    // Same Storage as Load
    //
    //////////////////////////////////////////////////////////////////
    
    if (fSameAsLoad) {          

	DebugMsg(DM_TRACE,"            Same as load.");
        
        LARGE_INTEGER   li = {0,0};
        pstm = _pPackage->_pstm;

	// If we're not dirty, so there's nothing new to save.
	
        if (FALSE == _pPackage->_fIsDirty) {
            DebugMsg(DM_TRACE, "            not saving cause we're not dirty!!");
	    return S_OK;
        }
        
        // if we are dirty, set the seek pointer to the beginning and write
        // the package information to the stream
        hr = pstm->Seek(li, STREAM_SEEK_SET, NULL);
        if (FAILED(hr)) {
            DebugMsg(DM_TRACE, "            couldn't set contents pointer!!");
            return hr;
        }
        
        // case 1: new package
        if (!_pPackage->_fLoaded) {
	    switch(_pPackage->_panetype) {
		LPTSTR temp;
		case PEMBED:
		    // if haven't created a temp file yet, then use the the
		    // file to be packaged to get our file contents from,
		    // otherwise we just use the temp file, because if we
		    // have a temp file, it contains the most recent info.
		    //
		    temp = _pPackage->_pEmbed->pszTempName;
            
		    if (!_pPackage->_pEmbed->pszTempName) {
			DebugMsg(DM_TRACE, "      case 1a:not loaded, using initFile.");
			_pPackage->_pEmbed->pszTempName = _pPackage->_pEmbed->fd.cFileName;
		    }
		    else {
			DebugMsg(DM_TRACE, "      case 1b:not loaded, using tempfile.");
		    }
	    
		    hr = _pPackage->PackageWriteToStream(pstm);
		    
		    // reset our temp name back, since we might have changed it
		    // basically, this just sets it to NULL if it was before
		    _pPackage->_pEmbed->pszTempName = temp;
		    break;

		case CMDLINK:
		    // nothing screwy to do here...just write out the info
		    // which we already have in memory.
		    hr = _pPackage->PackageWriteToStream(pstm);
		    break;
	    }
            
            if (FAILED(hr))
                return hr;
	}
    
        // case 3: loaded package
        else {
	    hr = _pPackage->PackageWriteToStream(pstm);
	}
    }
    
    //////////////////////////////////////////////////////////////////
    //
    // NEW Storage  
    //
    //////////////////////////////////////////////////////////////////
    
    else {

        DebugMsg(DM_TRACE,"            NOT same as load.");
        hr = pstg->CreateStream(SZCONTENTS,STGM_DIRECT | STGM_CREATE | 
                                STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, 
                                &pstm);
        if (FAILED(hr)) {
            DebugMsg(DM_TRACE, "            couldn't create contents stream!!");
            return hr;
        }
        WriteFmtUserTypeStg(pstg, _pPackage->_cf,SZUSERTYPE);
        
        // case 2:
        if (!_pPackage->_fLoaded) {
	    switch(_pPackage->_panetype) {
		LPTSTR temp;
		case PEMBED:
		    temp = _pPackage->_pEmbed->pszTempName;
            
		    if (!_pPackage->_pEmbed->pszTempName) {
			DebugMsg(DM_TRACE, "      case 2a:not loaded, using initFile.");
			_pPackage->_pEmbed->pszTempName = _pPackage->_pEmbed->fd.cFileName;
		    }            
		    else {
			DebugMsg(DM_TRACE, "      case 2b:not loaded, using tempfile.");
		    }
            
		    hr = _pPackage->PackageWriteToStream(pstm);
            
		    // reset our temp name back, since we might have changed it
		    // basically, this just sets it to NULL if it was before
		    _pPackage->_pEmbed->pszTempName = temp;
		    break;

		case CMDLINK:
		    // nothing interesting to do here, other than write out
		    // the package.
		    hr = _pPackage->PackageWriteToStream(pstm);
		    break;
	    }

	    if (FAILED(hr)) 
		goto ErrRet;
            
        }
        
        // case 4:
        else {
            if (_pPackage->_panetype == PEMBED && _pPackage->_pEmbed->pszTempName ) {
                DebugMsg(DM_TRACE,"    case 4a:loaded, using tempfile.");
                hr = _pPackage->PackageWriteToStream(pstm);
                if (FAILED(hr))
                    goto ErrRet;
            }
            else {
                DebugMsg(DM_TRACE,"    case 4b:loaded, using stream.");
                ULARGE_INTEGER uli = { 0xFFFFFFFF, 0xFFFFFFFF };
                LARGE_INTEGER li = {0,0};
                
                hr = _pPackage->_pstm->Seek(li,STREAM_SEEK_SET,NULL);
                if (FAILED(hr))
                    goto ErrRet;
                
                hr = _pPackage->_pstm->CopyTo(pstm,uli,NULL,NULL);
                if (FAILED(hr))
                    goto ErrRet;
            }
        }
    ErrRet:
        UINT u = pstm->Release();
        DebugMsg(DM_TRACE,"          new stream released.  cRef==%d.",u);
        if (FAILED(hr))
            return hr;
    }
    
    _psState = PSSTATE_ZOMBIE;
    
    DebugMsg(DM_TRACE, "            leaving Save()");
    return S_OK;
}

    
HRESULT CPackage_IPersistStorage::SaveCompleted(IStorage *pstg)
{
    HRESULT     hr;
    LPSTREAM    pstm;
    
    DebugMsg(DM_TRACE, "pack ps - SaveCompleted() called.");

    // must be called from no-scribble or hands-off state
    if (!(_psState == PSSTATE_ZOMBIE || _psState == PSSTATE_HANDSOFF))
        return E_UNEXPECTED;
    
    // if we're hands-off, we'd better get a storage to re-init from
    if (!pstg && _psState == PSSTATE_HANDSOFF)
        return E_UNEXPECTED;
    
    // if pstg is NULL then we have everything we need, otherwise we must
    // release our held pointers and reinitialize.
    //
    if (pstg != NULL) {
        DebugMsg(DM_TRACE, "            getting new storage.");
        hr = pstg->OpenStream(SZCONTENTS, 0, STGM_DIRECT | STGM_READWRITE | 
                              STGM_SHARE_EXCLUSIVE, 0, &pstm);
        if (FAILED(hr))
            return hr;

        if (_pPackage->_pstm != NULL)
            _pPackage->_pstm->Release();
        
        // this will be reinitialized when it's needed
        if (_pPackage->_pstmFileContents != NULL) {
            _pPackage->_pstmFileContents->Release();
            _pPackage->_pstmFileContents = NULL;
        }
        
        if (_pPackage->_pIStorage != NULL)
            _pPackage->_pIStorage->Release();

        _pPackage->_pstm = pstm;
        _pPackage->_pIStorage = pstg;
        _pPackage->_pIStorage->AddRef();
    }
    
    _psState = PSSTATE_SCRIBBLE;
    _pPackage->_fIsDirty = FALSE;
    DebugMsg(DM_TRACE, "            leaving SaveCompleted()");
    return S_OK;
}

    
HRESULT CPackage_IPersistStorage::HandsOffStorage(void)
{
    DebugMsg(DM_TRACE, "pack ps - HandsOffStorage() called.");

    // must come from scribble or no-scribble.  a repeated call to 
    // HandsOffStorage is an unexpected error (bug in client).
    //
    if (_psState == PSSTATE_UNINIT || _psState == PSSTATE_HANDSOFF)
        return E_UNEXPECTED;
    
    // release our held pointers
    //
    if (_pPackage->_pstmFileContents != NULL) {
        _pPackage->_pstmFileContents->Release();
        _pPackage->_pstmFileContents = NULL;
    }
    
    if (_pPackage->_pstm != NULL) {
        _pPackage->_pstm->Release();
        _pPackage->_pstm = NULL;
    }
    
    if (_pPackage->_pIStorage != NULL) {
        _pPackage->_pIStorage->Release();
        _pPackage->_pIStorage = NULL;
    }
    
    _psState = PSSTATE_HANDSOFF;
    DebugMsg(DM_TRACE, "            leaving HandsOffStorage()");
    return S_OK;
}
