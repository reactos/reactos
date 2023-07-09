#ifndef _STREAM_H_
#define _STREAM_H_

class CMemStream : public IStream {
public:
        STDMETHOD (QueryInterface)(REFIID riid, void **ppvObj);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        STDMETHOD (Read)(void *pv, ULONG cb, ULONG *pcbRead);
        STDMETHOD (Write)(void const *pv, ULONG cb, ULONG *pcbWritten);
        STDMETHOD (Seek)(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
        STDMETHOD (SetSize)(ULARGE_INTEGER libNewSize);
        STDMETHOD (CopyTo)(IStream *, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *);
        STDMETHOD (Commit)(DWORD);
        STDMETHOD (Revert)();
        STDMETHOD (LockRegion)(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);
        STDMETHOD (UnlockRegion)(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);
        STDMETHOD (Stat)(STATSTG *, DWORD);
        STDMETHOD (Clone)(IStream **);

        LPBYTE  GrowBuffer(ULONG);
private:
        BOOL    WriteToReg();

public:
    UINT        cRef;           // Reference count
    LPBYTE      pBuf;           // Buffer pointer
    UINT        cbAlloc;        // The allocated size of the buffer
    UINT        cbData;         // The used size of the buffer
    UINT        iSeek;          // Where we are in the buffer.
    DWORD       grfMode;        // mode used at creation (for Stat, and to enforce)
    // Extra variables that are used for loading and saving to ini files.
    HKEY        hkey;           // Key for writing to registry.
    BITBOOL     fDontCloseKey;  // if caller passes in a key
    TCHAR       szValue[1];     // for reg stream
};

#endif /* _STREAM_H_ */

