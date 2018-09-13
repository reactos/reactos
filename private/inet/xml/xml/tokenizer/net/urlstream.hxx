/*
 * @(#)URLStream.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _URLSTREM_HXX
#define _URLSTREM_HXX

#include "url.hxx"
#include "urlcallback.hxx"
typedef _reference<IStream> RStream;

class URLCallback;
class URLStream;

typedef _reference<URLCallback> RURLCallback;

//------------------------------------------------------------------------
// Some useful global functions that returns IStreams
HRESULT STDMETHODCALLTYPE CreateEncodingStream( 
            /* [in] */ IStream __RPC_FAR *pStm,
            /* [in] */ const WCHAR __RPC_FAR *pszEncoding,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppStm);

HRESULT STDMETHODCALLTYPE CreateURLStream(
            /* [in] */ const WCHAR* pszURL, 
            /* [in] */ const WCHAR* pszBaseURL, 
            /* [in] */ BOOL write, // TRUE for write, FALSE for read.
            /* [out] */ IStream** ppStm);

// The creator of the URLStream can also provide a
// URLDownloadTask object and HandleData will be called
// when more data becomes available.  There is no ownership
// semantics.  The caller manages the lifetimes of both the
// URLDownloadTask and the URLStream.  The HandleData
// method can return E_ABORT to abort the download.
class NOVTABLE URLDownloadTask
{
public:
    virtual HRESULT HandleData(URLStream* pStm, bool last) = 0;
    virtual void SetMimeType(URLStream* pStm, const WCHAR * pwszMimeType, int length) = 0;
    virtual void SetCharset(URLStream* pStm, const WCHAR * pwszCharset, int length) = 0;
};

//--------------------------------------------------------------------------------
class URLStream : public _unknown<IStream, &IID_IStream>
{

public: 

    URLStream(URLDownloadTask* task, bool fdtd = false);
    virtual ~URLStream();

    enum Mode
    {
        ASYNCREAD,
        SYNCREAD,
        WRITE,
        APPEND
    };
    
    // Opens a URL stream on the given URL.  It uses the file system
    // directly if this is a local URL, otherwise it uses URLMON.
    // For URLMON to work your application needs to pump the
    // message queue.    
    HRESULT Open(URL* url, Mode flag);
    HRESULT Open(IMoniker * pmk, LPBC lpbc, URL* url, Mode flag);

    // Must call this method to stop a download (this breaks
    // circular references so that objects are cleaned up).
    void Reset();
    void Abort(); // stop a download.

    ///////////////////////////////////////////////////////////
    // IStream Interface
    //
    HRESULT STDMETHODCALLTYPE Read(void * pv, ULONG cb, ULONG * pcbRead);

    HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG * pcbWritten);

    virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER * plibNewPosition)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream * pstm, ULARGE_INTEGER cb, ULARGE_INTEGER * pcbRead, ULARGE_INTEGER * pcbWritten)
    {
        return E_NOTIMPL;
    } 
 
    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags);
    
    virtual HRESULT STDMETHODCALLTYPE Revert(void)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE LockRegion( ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG * pstatstg, DWORD grfStatFlag)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Clone(IStream ** ppstm)
    {
        return E_NOTIMPL;
    }

    ///////////////////////////////////////////////////////////
    // Methods used by BindStatusCallback to set the real stream and
    // get URL info 
    void    SetStream(IStream * s);
    URL *   GetURL() {return _pUrl;}
    bool    IsLocalStream() { return _fIsLocal;}
    void    SetStatus(HRESULT hr);
    HRESULT GetStatus();
    int     GetMode() { return _nMode; }

    URLDownloadTask* GetTask() { return _pTask; }

    ///////////////////////////////////////////////////////////
    // Internet security
    HRESULT OpenAllowed(LPCWSTR pwszUrl);

private:
    HRESULT OpenFile(Mode flag);
    HRESULT OpenURL(IMoniker * pmk, LPBC lpbc, Mode flag);

private:
    CRITICAL_SECTION _cs;
    URL * _pUrl;
    RStream _pStream;
    RURLCallback _pCallback;
    bool _fIsLocal;
    bool _fIsOutput;
    HANDLE _hFile;
    HRESULT _ulStatus;
    URLDownloadTask* _pTask;
    int  _nMode;
    bool    _fDTD;
};

typedef _reference<URLStream> RURLStream;


#endif _URLSTREM_HXX