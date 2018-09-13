#pragma once

class CFileStream : public IStream
{

public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IStream methods ***
    STDMETHOD(Read) (THIS_ VOID *pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD(Write) (THIS_ VOID const *pv, ULONG cb, ULONG *pcbWritten);
    STDMETHOD(Seek) (THIS_ LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHOD(SetSize) (THIS_ ULARGE_INTEGER libNewSize);
    STDMETHOD(CopyTo) (THIS_ IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHOD(Commit) (THIS_ DWORD grfCommitFlags);
    STDMETHOD(Revert) (THIS);
    STDMETHOD(LockRegion) (THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(UnlockRegion) (THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(Stat) (THIS_ STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHOD(Clone)(THIS_ IStream **ppstm);

public:
    CFileStream(HANDLE hf, DWORD grfMode);
    ~CFileStream();

private:
    LONG        cRef;           // Reference count
    HANDLE      hFile;          // the file.
    BOOL        fWrite;         // TRUE if writing.

    ULONG       ib;
    ULONG       cbBufLen;       // length of buffer if reading
    BYTE        ab[4096];       // buffer
};
