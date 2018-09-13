/*
 * @(#)Stream.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _CORE_IO_STREAM
#define _CORE_IO_STREAM

//
// An abstract I/O stream class for all I/O stream classes used in XML/XSL
// This class enhances IStream by adding two new methods get() and put()
// This class also provides empty body implementations for most methods of IStream
// so that subclasses of Stream only need to implement methods when needed
//

class NOVTABLE Stream : public Object
{
friend class IStreamWrapper;

public: void getIStream(IStream ** ppStream)
        {
            AddRef();
            *ppStream = &_istream;
        }

public: class IStreamWrapper : public IStream
    {
    private:
        Stream * outer()
        {
            return (Stream *)((BYTE *)this - (BYTE *)&((Stream *)0)->_istream);
        }

        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject)
        {
            if (iid == IID_IStream)
            {
                *ppvObject = this;
                AddRef();
                return S_OK;
            }
            else
            {
                return outer()->QueryInterface(iid, ppvObject);
            }
        }

        virtual ULONG STDMETHODCALLTYPE AddRef()
        {
            return outer()->AddRef();
        }

        virtual ULONG STDMETHODCALLTYPE Release()
        {
            return outer()->Release();
        }
           
        virtual HRESULT STDMETHODCALLTYPE Read(void * pv, ULONG cb, ULONG * pcbRead)
        {
            return outer()->Read(pv, cb, pcbRead);
        } 

        virtual HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG * pcbWritten)
        {
            return outer()->Write(pv, cb, pcbWritten);
        }

        virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER * plibNewPosition)
        {
            return outer()->Seek(dlibMove, dwOrigin, plibNewPosition);
        }

        virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize)
        {
            return outer()->SetSize(libNewSize);
        }

        virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream * pstm, ULARGE_INTEGER cb, ULARGE_INTEGER * pcbRead, ULARGE_INTEGER * pcbWritten)
        {
            return outer()->CopyTo(pstm, cb, pcbRead, pcbWritten);
        } 
 
        virtual HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags)
        {
            return outer()->Commit(grfCommitFlags);
        }
    
        virtual HRESULT STDMETHODCALLTYPE Revert(void)
        {
            return outer()->Revert();
        }

        virtual HRESULT STDMETHODCALLTYPE LockRegion( ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
        {
            return outer()->LockRegion(libOffset, cb, dwLockType);
        }

        virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
        {
            return outer()->UnlockRegion(libOffset, cb, dwLockType);
        }

        virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG * pstatstg, DWORD grfStatFlag)
        {
            return outer()->Stat(pstatstg, grfStatFlag);
        }

        virtual HRESULT STDMETHODCALLTYPE Clone(IStream ** ppstm)
        {
            return outer()->Clone(ppstm);
        }
    } _istream;

public:

    enum
    {
        SYNCREAD,
        ASYNCREAD,
        WRITE,
        APPEND
    };

    virtual int get()
    {
        return -1;
    }

    virtual HRESULT put(TCHAR c)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Read(void * pv, ULONG cb, ULONG * pcbRead)
    {
        return E_NOTIMPL;
    } 

    virtual HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG * pcbWritten)
    {
        return E_NOTIMPL;
    }

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
 
    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags)
    {
        return E_NOTIMPL;
    }
    
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
 
}; 

#endif _CORE_IO_STREAM