////////////////////////////////////////////////////////////////////////////
// BufferedStream.hxx
//--------------------------------------------------------------------------
// Copyright (c) 1998 - 1999 Microsoft Corporation. All rights reserved.*///
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//--------------------------------------------------------------------------
//
// Content: A custom build read/write stream that buffers the writes so
// until the read returns the data.
//
////////////////////////////////////////////////////////////////////////////

#ifndef _BUFFEREDSTREAM_HXX
#define _BUFFEREDSTREAM_HXX

#include <urlmon.h>
#include <tchar.h>
#include <stdio.h>
//#include "unknown.hxx"

#include "mimedownload.hxx"

extern TAG tagMIME;

#include "callback.hxx"

class ViewerFactory;
class Viewer;

#if MIMEASYNC
#define XMN_FIRST 0
#define XMN_INTER 1
#define XMN_LAST  2
#endif

// once MBS_NOTIFYSIZE has been surpassed we will notify trident
// trident likes to read in 8K chunks, hence that appears like
// the best notify size

#define MBS_NOTIFYSIZE      0x2000L

// buffer allocation
// we first double the size of the buffer with each reallocation, 
// until we reach MBS_LINEAR_CLIFF.  At that point we grow the
// the buffer linearly by MBS_LINEAR.  We do this for really large files

// MBS_LINEAR_CLIFF is 1 MB.  Trident should have read something from us by now!
// Unless we have a big gob of consecutive XSL stuff in a single Execute call, and
// we never get the thread switch to the GUI thread.  Highly unlikely.
#define MBS_BUFSIZE_INITIAL (MBS_NOTIFYSIZE << 1)
#define MBS_LINEAR_CLIFF    0x100000L       
#define MBS_LINEAR          0x40000L        // linear growth by 256K

#define E_NOTRELEASED       0xC00CE4FF      // special internal error code.

//=======================================================================
class MIMEBufferedStream : public IStream
{
public:

    MIMEBufferedStream();
    virtual ~MIMEBufferedStream();

    void CleanUp()
    {
//        TraceTag((0, "Cleanup called"));
        if (_pbs)
        {
            _pbs->Release();
            _pbs = NULL;
        }
        if (_pBinding)
        {
            // NULL out the _pViewer pointer in the binding so that we 
            // don't try and dereference it later when Trident or WININET
            // finally releases the last reference on the CBinding object.
            // This fixes a reproducable crash !!
            _pBinding->SetAbortCB(NULL,NULL);
            _pBinding->Release();
            _pBinding = NULL;
        }
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID iid, //Identifier of the requested interface
        void ** ppvObject   //Receives an indirect pointer to the object
        ) 
    { 
        if (iid == IID_IUnknown || iid == IID_IStream)
        {
            *ppvObject = (IStream*)this;
            AddRef();
            return S_OK;
        } 
        else
       {
            return E_NOINTERFACE; 
        }
    }

    virtual ULONG STDMETHODCALLTYPE AddRef( void);
    
    virtual ULONG STDMETHODCALLTYPE Release( void);

#if MIMEASYNC
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read( 
        /* [out] */ void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbRead)
    {
        HRESULT hr = S_OK;

#if DBG == 1
        if (_fCanRead)
        {
            _xslReads++;
            TraceTag((tagMIME, "MIMEStream::Read call %d, # of bytes: %d", _xslReads, cb)); 
        }
#endif

        EnterCS();

        if ((_cRead >= _cWritten) || (!_fCanRead))
        {
            *pcbRead = 0;
            if (!_fCommit)
            {
                // There is still a writer !
                hr = E_PENDING;
                goto CleanUp;
            }
            else
            {
                SafeDelete(_pchBuffer);
                _cSize = _cWritten = _cRead = 0;
                hr = S_FALSE;
                // can no longer destroy window at end of read, because we need to post
                // to it to terminate the download on the main thread
//              if (_mdhAsync)
//              {
//                  Assert(g_pMimeDwnWndMgr);
//                  g_pMimeDwnWndMgr->ReleaseGUIWnd(_mdhAsync);
//                  LeaveCS();  // because we are about to set _mdhAsync to NULL
//                  _mdhAsync = NULL;
//                  return hr;
//              }
//              else
//                  goto CleanUp;

            }
        }        
        else 
        {
            if (cb > _cWritten - _cRead)
            {
                cb = _cWritten - _cRead;
            }
            ::memcpy(pv, &_pchBuffer[_cRead], cb);
            _cRead += cb;
            *pcbRead = cb;
        }
CleanUp:
        LeaveCS();
        return hr;
    }
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write( 
        /* [size_is][in] */ const void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbWritten)
    {
        if (_fStopAsync)
            return E_FAIL;
#if DBG == 1
        if (_fCanRead)
        {
            _xslWrites++;
            TraceTag((tagMIME, "MIMEStream::Write call %d, # of bytes: %d", _xslWrites, cb)); 
        }
#endif

        HRESULT hr = S_OK;

        EnterCS();

        if (_cWritten + cb > _cSize)
        {
            _cWritten -= _cRead;
            _cLastNotify -= _cRead;
            // Try to compact the buffer
            if (_cWritten + cb <= _cSize)
            {
                memmove(_pchBuffer, _pchBuffer + _cRead, _cWritten);
            }
            else 
            {
                ULONG cNewSize;
                ULONG ctBytes = cb + _cWritten;
                if (ctBytes >= MBS_LINEAR_CLIFF)
                    cNewSize = (ctBytes/MBS_LINEAR + 1) * MBS_LINEAR;
                else
                {
                    cNewSize = _cSize ? _cSize : MBS_BUFSIZE_INITIAL;
                    while (cNewSize < ctBytes)
                        cNewSize = cNewSize << 1;
                }
                TraceTag((tagMIME, "resizing buffer to size %d", cNewSize));
                char* newbuf = new_ne char[ cNewSize ];
                if (newbuf == NULL)
                {
                    LeaveCS();
                    return E_OUTOFMEMORY;
                }

                ::memcpy(newbuf, _pchBuffer + _cRead, _cWritten);
                delete _pchBuffer;
                _pchBuffer = newbuf;
                _cSize = cNewSize;
            }
            _cRead = 0;
        }
        ::memcpy(&_pchBuffer[_cWritten], pv, cb);
        *pcbWritten = cb;
        ULONG w = _cWritten;
        _cWritten += cb;

        LeaveCS();

        if (_pbs && _fCanRead)
        {
            if (_fFirstNotify)
            {
                hr = NotifyData(XMN_FIRST);
                _fFirstNotify = false;
            }
            else if (_cWritten - _cLastNotify >= MBS_NOTIFYSIZE)
            {
                _cLastNotify = _cWritten;
                hr = NotifyData(XMN_INTER);
            }      
        }

        return hr;
    }

    HRESULT NotifyData(WORD xmn)
    {   
        HRESULT hr;    

        if (_mdhAsync)
        {
#if DBG == 1
            _ntfData++;
            TraceTag((tagMIME, "MIMEStream::NotifyData call %d", _ntfData)); 
#endif
            Assert(g_pMimeDwnWndMgr);
            HWND hWnd = g_pMimeDwnWndMgr->GetGUIWnd(_mdhAsync);
            if (hWnd)
            {
                ::PostMessage(hWnd, WM_MIMECBDATA, xmn, (LPARAM)this);
                // send it to the other thread
                hr = S_OK;
            }
            else
            {
                hr = E_NOTRELEASED;
            }
        }
        else
        {
            // do it now!!
            hr = NotifyDataTrident(xmn);
        }
        return hr;
    }

    HRESULT NotifyDataTrident(WORD xmn)
    {
        FORMATETC fmtc;
        DWORD flags;
        DWORD size;
        HRESULT hr = S_OK;

        if (_pbs == NULL)
        {
            // don't do anything, we got a message after cleanup...
            TraceTag((0, "Message arrived when _pbs = null"));
            goto CleanUp;
        }
        EnterCS();
        size = _cWritten - _cRead;
        if (xmn == XMN_LAST)
            _fCommit = true;
        LeaveCS();

        // don't bother to notify trident it's a gigantic waste!!
        if (size == 0 && xmn==XMN_INTER)
            return S_OK;
        
        fmtc.cfFormat = getClipFormat();
        fmtc.ptd = NULL;
        fmtc.dwAspect = DVASPECT_CONTENT;
        fmtc.lindex = -1;
        fmtc.tymed = TYMED_NULL;
        STGMEDIUM stgm;
        stgm.tymed = TYMED_ISTREAM;
        stgm.pstm = this;       // trident addref's on first notify only, and will release it when done
        stgm.pUnkForRelease = NULL;
        flags = (xmn == XMN_FIRST) ? BSCF_FIRSTDATANOTIFICATION :
                (xmn == XMN_INTER) ? BSCF_INTERMEDIATEDATANOTIFICATION : BSCF_LASTDATANOTIFICATION;

#if DBG == 1
        if (_mdhAsync)
        {
            _ntfReceive++;
            TraceTag((tagMIME, "MIMEStream::NotifyReceive call %d, # of bytes: %d", _ntfReceive, size)); 
        }
#endif
        hr = _pbs->OnDataAvailable(flags, size, &fmtc, &stgm);
CleanUp:
        if (xmn == XMN_LAST) 
        {
            StopBind();
            Release();      // release the ref seized on the other thread
        }
        return hr;
    }

    void StopBind(void)
    {
        if (_pbs)
            _pbs->OnStopBinding(0, L"");
        CleanUp();
    }

#else
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read( 
        /* [out] */ void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbRead)
    {
#if DBG == 1
        if (_fCanRead)
        {
            _xslReads++;
            TraceTag((tagMIME, "MIMEStream::Read call %d, # of bytes: %d", _xslReads, cb)); 
        }
#endif
        if ((_cRead >= _cWritten) || (!_fCanRead))
        {
            *pcbRead = 0;
            if (!_fCommit)
            {
                // There is still a writer !
                return E_PENDING;
            }
            else
            {
                SafeDelete(_pchBuffer);
                _cSize = _cWritten = _cRead = 0;
                return S_FALSE; // reached end of file
            }
        }        
        else 
        {
            if (cb > _cWritten - _cRead)
            {
                cb = _cWritten - _cRead;
            }
            ::memcpy(pv, &_pchBuffer[_cRead], cb);
            _cRead += cb;
            *pcbRead = cb;
        }
        return S_OK;
    }
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write( 
        /* [size_is][in] */ const void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbWritten)
    {
        HRESULT hr = S_OK;

#if DBG == 1
        if (_fCanRead)
        {
            _xslWrites++;
            TraceTag((tagMIME, "MIMEStream::Write call %d, # of bytes: %d", _xslWrites, cb)); 
        }
#endif
        if (_cWritten + cb > _cSize)
        {
            _cWritten -= _cRead;
            _cLastNotify -= _cRead;
            // Try to compact the buffer
            if (_cWritten + cb <= _cSize)
            {
                memmove(_pchBuffer, _pchBuffer + _cRead, _cWritten);
            }
            else 
            {
                ULONG cNewSize = ((cb + _cWritten)/MBS_BUFSIZE_INITIAL + 1) * MBS_BUFSIZE_INITIAL;
                char* newbuf = new_ne char[ cNewSize ];
                if (newbuf == NULL)
                    return E_OUTOFMEMORY;
                ::memcpy(newbuf, _pchBuffer + _cRead, _cWritten);
                delete _pchBuffer;
                _pchBuffer = newbuf;
                _cSize = cNewSize;
            }
            _cRead = 0;
        }
        ::memcpy(&_pchBuffer[_cWritten], pv, cb);
        *pcbWritten = cb;
        ULONG w = _cWritten;
        _cWritten += cb;

        if (_pbs && _fCanRead)
        {
            if (_fFirstNotify)
            {
                NotifyData(BSCF_FIRSTDATANOTIFICATION);
                _fFirstNotify = false;
            }
            else if (_cWritten - _cLastNotify >= MBS_NOTIFYSIZE)
            {
                _cLastNotify = _cWritten;
                NotifyData(BSCF_INTERMEDIATEDATANOTIFICATION);
            }
        }
        return hr;
    }

    void NotifyData(DWORD flags)
    {
        FORMATETC fmtc;
        fmtc.cfFormat = getClipFormat();
        fmtc.ptd = NULL;
        fmtc.dwAspect = DVASPECT_CONTENT;
        fmtc.lindex = -1;
        fmtc.tymed = TYMED_NULL;
        STGMEDIUM stgm;
        stgm.tymed = TYMED_ISTREAM;
        stgm.pstm = this;
        stgm.pUnkForRelease = NULL;

#if DBG == 1
        if (_fCanRead)
        {
            _ntfData++;
            _ntfReceive++;
            TraceTag((tagMIME, "MIMEStream::NotifyDataSync call %d, # of bytes %d", _ntfData, _cWritten - _cRead));
        }
#endif
        if (_pbs)
            _pbs->OnDataAvailable(flags, _cWritten - _cRead, &fmtc, &stgm);

    }
#endif


    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Seek( 
        /* [in] */ LARGE_INTEGER dlibMove,
        /* [in] */ DWORD dwOrigin,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition) 
    { 
        return E_FAIL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE SetSize( 
        /* [in] */ ULARGE_INTEGER libNewSize) 
    { 
        return E_FAIL; 
    }
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo( 
        /* [unique][in] */ IStream __RPC_FAR *pstm,
        /* [in] */ ULARGE_INTEGER cb,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten) 
    { 
        return E_FAIL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE Commit( 
        /* [in] */ DWORD grfCommitFlags) 
    { 
        if (_pbs)
        {
#if MIMEASYNC
            // we need to keep this bufferedstream around until 
            // notify comes in on the GUI thread to stop
            // the binding.  Otherwise the destructor might be
            // called once we unwind on this thread 
            AddRef();
            if (E_NOTRELEASED == NotifyData(XMN_LAST))
                Release();
#else
            _fCommit = true;
            NotifyData(BSCF_LASTDATANOTIFICATION);
            _pbs->OnStopBinding(0, L"");
            CleanUp();
#endif        
        }
        return S_OK; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE Revert(void) 
    { 
        if (_cRead)
            return E_FAIL;
        _cWritten = _cRead = 0;
        return S_OK;
    }

    
    virtual HRESULT STDMETHODCALLTYPE LockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType) 
    { 
        return E_FAIL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType) 
    { 
        return E_FAIL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE Stat( 
        /* [out] */ STATSTG __RPC_FAR *pstatstg,
        /* [in] */ DWORD grfStatFlag) 
    { 
        return E_FAIL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE Clone( 
        /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm) 
    { 
        return E_FAIL; 
    }

    void SetCallback(IBindStatusCallback* pbs, CBinding* binding)
    {
        _pbs = pbs;
        _pbs->AddRef();
        _pBinding = binding;
        _pBinding->AddRef();
        pbs->OnStartBinding(0, _pBinding);
    }

    void EnableRead(void)
    {
        _fCanRead = TRUE;
    }
#if MIMEASYNC
    void TurnOnAsync(MDHANDLE mdh) { 
        _mdhAsync = mdh; 
        // now addref the window since we hold onto it.
        if (_mdhAsync && g_pMimeDwnWndMgr)
        {
            g_pMimeDwnWndMgr->AddRefGUIWnd(_mdhAsync);
        }
    }
    void StopAsync(void) { 
        _fStopAsync = true; 
        if (_mdhAsync && g_pMimeDwnQueue)
        {
            g_pMimeDwnQueue->StopAsync(_mdhAsync);
        }
    }
#endif

private:
    static CLIPFORMAT _cfHtml;
    
    static CLIPFORMAT getClipFormat(void)
    {
        if (_cfHtml == CF_NULL)
        {
            _cfHtml = (CLIPFORMAT)RegisterClipboardFormatA("text/html");
            if (_cfHtml == 0)
                _cfHtml = CF_TEXT;	// degenerate to text
        }
        return _cfHtml;
    }

#if MIMEASYNC
    void EnterCS(void)
    {
        if (_mdhAsync)
        {
#if DBG == 1
            if (_bCSActive)
                _nConflicts++;
#endif
            EnterCriticalSection(&_csRW);
#if DBG == 1
            _bCSActive = true;
#endif
        }
    }

    void LeaveCS(void)
    {
        if (_mdhAsync)
        {
#if DBG == 1
            _bCSActive = false;
#endif
            LeaveCriticalSection(&_csRW);
        }

    }
#endif

public:
    void SetAbortCB(Viewer *pViewer, ViewerFactory *pNodeFactory)
    {
        if (_pBinding)
            _pBinding->SetAbortCB(pViewer, pNodeFactory);
    }

    IUnknown* getTrident()
    {
        if (_pBinding)
            return _pBinding->getTrident();
        return null;
    }

private:
    ULONG   _cRefs;
    char*   _pchBuffer;
    ULONG   _cSize;
    ULONG   _cWritten;
    ULONG   _cRead;
    ULONG   _cLastNotify;
    bool    _fFirstNotify;
    bool    _fCanRead;
    bool    _fCommit;
    IBindStatusCallback* _pbs;
    CBinding *_pBinding;
#if MIMEASYNC
    MDHANDLE  _mdhAsync;
    bool      _fStopAsync;
    CRITICAL_SECTION _csRW;
#endif
#if DBG == 1
    ULONG _xslWrites;
    ULONG _xslReads;
    ULONG _ntfData;
    ULONG _ntfReceive;
    ULONG _nConflicts;
    bool  _bCSActive;
#endif
};


//=====================================================================
// This moniker wraps a BufferedStream. 

class BufferedStreamMoniker : public IMoniker
{
private:
    MIMEBufferedStream* _pStm;
    IMoniker *_pimkSrc;
    long _ulRefs;
public:
    BufferedStreamMoniker(MIMEBufferedStream* pStm, IMoniker *pimkSrc);
    ~BufferedStreamMoniker();

    // ============= IUnknown ============================
    
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // ============= IPersist ============================

    virtual HRESULT STDMETHODCALLTYPE GetClassID( 
        /* [out] */ CLSID __RPC_FAR *pClassID);

    // ============= IPersistStream ============================

    virtual HRESULT STDMETHODCALLTYPE IsDirty( void) 
    { 
        return S_FALSE; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE Load( 
        /* [unique][in] */ IStream __RPC_FAR *pStm);
    
    virtual HRESULT STDMETHODCALLTYPE Save( 
        /* [unique][in] */ IStream __RPC_FAR *pStm,
        /* [in] */ BOOL fClearDirty);
    
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax( 
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbSize) 
    { 
        return _pimkSrc->GetSizeMax(pcbSize);
    }

    // ============ IMoniker =======================

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE BindToObject( 
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
        /* [in] */ REFIID riidResult,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvResult) 
    { 
        return _pStm->QueryInterface(riidResult, ppvResult); 
    }
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE BindToStorage( 
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObj);
     
    virtual HRESULT STDMETHODCALLTYPE Reduce( 
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [in] */ DWORD dwReduceHowFar,
        /* [unique][out][in] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkToLeft,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkReduced) 
    { 
        return E_NOTIMPL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE ComposeWith( 
        /* [unique][in] */ IMoniker __RPC_FAR *pmkRight,
        /* [in] */ BOOL fOnlyIfNotGeneric,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkComposite) 
    { 
        return E_NOTIMPL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE Enum( 
        /* [in] */ BOOL fForward,
        /* [out] */ IEnumMoniker __RPC_FAR *__RPC_FAR *ppenumMoniker) 
    { 
        return E_NOTIMPL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE IsEqual( 
        /* [unique][in] */ IMoniker __RPC_FAR *pmkOtherMoniker) 
    { 
        return S_FALSE; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE Hash( 
        /* [out] */ DWORD __RPC_FAR *pdwHash) 
    {	// This is a 32 bit hash value, which never appears to be used so PtrToLong should be safe here
        *pdwHash = PtrToUlong(_pStm);
        return S_OK; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE IsRunning( 
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkNewlyRunning) 
    { 
        return E_NOTIMPL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE GetTimeOfLastChange( 
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
        /* [out] */ FILETIME __RPC_FAR *pFileTime) 
    { 
        return E_NOTIMPL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE Inverse( 
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmk) 
    { 
        return E_NOTIMPL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE CommonPrefixWith( 
        /* [unique][in] */ IMoniker __RPC_FAR *pmkOther,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkPrefix) 
    { 
        return E_NOTIMPL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE RelativePathTo( 
        /* [unique][in] */ IMoniker __RPC_FAR *pmkOther,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkRelPath) 
    { 
        return E_NOTIMPL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE GetDisplayName( 
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
        /* [out] */ LPOLESTR __RPC_FAR *ppszDisplayName) 
    { 
        _pimkSrc->GetDisplayName(pbc, pmkToLeft, ppszDisplayName);
//        *ppszDisplayName = CopyString(_pszName);
        return S_OK; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE ParseDisplayName( 
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
        /* [in] */ LPOLESTR pszDisplayName,
        /* [out] */ ULONG __RPC_FAR *pchEaten,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkOut) 
    { 
        return E_NOTIMPL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE IsSystemMoniker( 
        /* [out] */ DWORD __RPC_FAR *pdwMksys) 
    { 
        *pdwMksys = MKSYS_URLMONIKER;
        return S_OK; 
    }

};



#endif // _BUFFEREDSTREAM_HXX