#include "priv.h"
#include "nullstm.h"

#ifndef UNIX

// A static empty stream implementation, for back compat & no memory hit
//
// Macros copied from cfdefs.h
//
#define STDMETHODX(fn)      HRESULT STDMETHODCALLTYPE fn
#define STDMETHODX_(ret,fn) ret STDMETHODCALLTYPE fn
class CNullStream {
public:
    IStream *vtable;
    
    STDMETHODX (QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHODX_(ULONG, AddRef)();
    STDMETHODX_(ULONG, Release)();

    STDMETHODX (Read)(void *pv, ULONG cb, ULONG *pcbRead) { if (pcbRead) *pcbRead = 0; return E_NOTIMPL; }
    STDMETHODX (Write)(void const *pv, ULONG cb, ULONG *pcbWritten) { if (pcbWritten) *pcbWritten = 0; return E_NOTIMPL; }
    STDMETHODX (Seek)(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition) { plibNewPosition->HighPart = plibNewPosition->LowPart = 0; return E_NOTIMPL; }
    STDMETHODX (SetSize)(ULARGE_INTEGER libNewSize) { return E_NOTIMPL; }
    STDMETHODX (CopyTo)(IStream *, ULARGE_INTEGER, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten) { if (pcbRead) pcbRead->LowPart = pcbRead->HighPart = 0; if (pcbWritten) pcbWritten->LowPart = pcbWritten->HighPart = 0; return E_NOTIMPL; }
    STDMETHODX (Commit)(DWORD) { return E_NOTIMPL; }
    STDMETHODX (Revert)() { return E_NOTIMPL; }
    STDMETHODX (LockRegion)(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) { return E_NOTIMPL; }
    STDMETHODX (UnlockRegion)(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) { return E_NOTIMPL; }
    STDMETHODX (Stat)(STATSTG *, DWORD) { return E_NOTIMPL; }
    STDMETHODX (Clone)(IStream **ppstm) { *ppstm = NULL; return E_NOTIMPL; }
};

STDMETHODIMP CNullStream::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IStream) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj=this;
        // DllAddRef(); // if this dll supported CoCreateInstance
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}
STDMETHODIMP_(ULONG) CNullStream::AddRef()
{
    // DllAddRef(); // if this dll supported CoCreateInstance
    return 2;
}
STDMETHODIMP_(ULONG) CNullStream::Release()
{
    // DllRelease(); // if this dll supported CoCreateInstance
    return 1;
}

// We need the C vtable declaration, but this is .CPP.
// Simulate the vtable, swiped from objidl.h and touched up.
//
typedef struct IStreamVtbl
{
    HRESULT ( STDMETHODCALLTYPE CNullStream::*QueryInterface )(REFIID riid, void **ppvObject);
    ULONG ( STDMETHODCALLTYPE CNullStream::*AddRef )( );
    ULONG ( STDMETHODCALLTYPE CNullStream::*Release )( );
    
    HRESULT ( STDMETHODCALLTYPE CNullStream::*Read )( 
        void *pv,
        ULONG cb,
        ULONG *pcbRead);
    
    HRESULT ( STDMETHODCALLTYPE CNullStream::*Write )( 
        const void *pv,
        ULONG cb,
        ULONG *pcbWritten);
    
    HRESULT ( STDMETHODCALLTYPE CNullStream::*Seek )( 
        LARGE_INTEGER dlibMove,
        DWORD dwOrigin,
        ULARGE_INTEGER *plibNewPosition);
    
    HRESULT ( STDMETHODCALLTYPE CNullStream::*SetSize )( 
        ULARGE_INTEGER libNewSize);
    
    HRESULT ( STDMETHODCALLTYPE CNullStream::*CopyTo )( 
        IStream *pstm,
        ULARGE_INTEGER cb,
        ULARGE_INTEGER *pcbRead,
        ULARGE_INTEGER *pcbWritten);
    
    HRESULT ( STDMETHODCALLTYPE CNullStream::*Commit )( 
        DWORD grfCommitFlags);
    
    HRESULT ( STDMETHODCALLTYPE CNullStream::*Revert )( );
    
    HRESULT ( STDMETHODCALLTYPE CNullStream::*LockRegion )( 
        ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb,
        DWORD dwLockType);
    
    HRESULT ( STDMETHODCALLTYPE CNullStream::*UnlockRegion )( 
        ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb,
        DWORD dwLockType);
    
    HRESULT ( STDMETHODCALLTYPE CNullStream::*Stat )( 
        STATSTG *pstatstg,
        DWORD grfStatFlag);
    
    HRESULT ( STDMETHODCALLTYPE CNullStream::*Clone )( 
        IStream **ppstm);
    
} IStreamVtbl;


IStream* SHConstNullStream()
{
    static const IStreamVtbl c_NullStream = {
        CNullStream::QueryInterface, 
        CNullStream::AddRef,
        CNullStream::Release,
        CNullStream::Read,
        CNullStream::Write,
        CNullStream::Seek,
        CNullStream::SetSize,
        CNullStream::CopyTo,
        CNullStream::Commit,
        CNullStream::Revert,
        CNullStream::LockRegion,
        CNullStream::UnlockRegion,
        CNullStream::Stat,
        CNullStream::Clone,
    };
    static const void * vtable = (void*)&c_NullStream;

    return (IStream*)&vtable;
}

#endif

