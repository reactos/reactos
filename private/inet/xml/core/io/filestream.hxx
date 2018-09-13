////////////////////////////////////////////////////////////////////////////
// FileStream.hxx
//--------------------------------------------------------------------------
// Copyright (c) 1998 - 1999 Microsoft Corporation. All rights reserved.*///
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//--------------------------------------------------------------------------
//
// Content: A custom build read/write FileStream
//
////////////////////////////////////////////////////////////////////////////

#ifndef _FILESTREAM_HXX
#define _FILESTREAM_HXX

class FileStream : public IStream
{

public:

    FileStream( char* name, bool write) 
    { 
        refcount = 0; 
        hFile = ::CreateFileA(
            name,
            write ? GENERIC_WRITE : GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            write ? CREATE_ALWAYS : OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    }

    ~FileStream() 
    { 
        ::CloseHandle(hFile);
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

    virtual ULONG STDMETHODCALLTYPE AddRef( void) 
    { 
        return refcount++; 
    }
    
    virtual ULONG STDMETHODCALLTYPE Release( void) 
    { 
        refcount--; 
        if (refcount ==0) delete this;
        return S_OK;
    }

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read( 
        /* [out] */ void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbRead)
    {   
        BOOL rc = ReadFile(
            hFile,  // handle of file to read 
            pv, // address of buffer that receives data  
            cb, // number of bytes to read 
            pcbRead,    // address of number of bytes read 
            NULL    // address of structure for data 
           );
        return (rc) ? S_OK : E_FAIL;
    }
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write( 
        /* [size_is][in] */ const void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbWritten)
    {
        DWORD written;
        BOOL rc = WriteFile(
            hFile,  // handle of file to write
            pv, // address of buffer that receives data  
            cb, // number of bytes to read 
            &written,   // address of number of bytes read 
            NULL    // address of structure for data 
           );
        if (pcbWritten)
            *pcbWritten = written;

        return rc ? S_OK : E_FAIL;
    }

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Seek( 
        /* [in] */ LARGE_INTEGER dlibMove,
        /* [in] */ DWORD dwOrigin,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition) { return E_FAIL; }
    
    virtual HRESULT STDMETHODCALLTYPE SetSize( 
        /* [in] */ ULARGE_INTEGER libNewSize) { return E_FAIL; }
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo( 
        /* [unique][in] */ IStream __RPC_FAR *pstm,
        /* [in] */ ULARGE_INTEGER cb,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten) { return E_FAIL; }
    
    virtual HRESULT STDMETHODCALLTYPE Commit( 
        /* [in] */ DWORD grfCommitFlags) { return E_FAIL; }
    
    virtual HRESULT STDMETHODCALLTYPE Revert( void) { return E_FAIL; }
    
    virtual HRESULT STDMETHODCALLTYPE LockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType) { return E_FAIL; }
    
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType) { return E_FAIL; }
    
    virtual HRESULT STDMETHODCALLTYPE Stat( 
        /* [out] */ STATSTG __RPC_FAR *pstatstg,
        /* [in] */ DWORD grfStatFlag) { return E_FAIL; }
    
    virtual HRESULT STDMETHODCALLTYPE Clone( 
        /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm) { return E_FAIL; }

private:

    HANDLE hFile;
    ULONG refcount;
};

#endif // _FILESTREAM_HXX