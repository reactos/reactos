//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       substm.hxx
//
//  Contents:   Read-only cloned substream
//
//  History:    04-22-1997   DBau (David Bau)    Created
//
//----------------------------------------------------------------------------

#ifndef I_SUBSTM_HXX_
#define I_SUBSTM_HXX_
#pragma INCMSG("--- Beg 'substm.hxx'")

//+---------------------------------------------------------------------------
//
//  Class:      CSubstream
//
//              An IStream wrapper which restricts access to an interval
//              of bytes. Can be initialized to be read-only or writable.
//
//              In read-only mode, the class operates on a clone of the
//              source stream, so multiple read-only CSubstreams can
//              be active at arbitary positions within a single source
//              stream. In this mode, all write operations fail.
//
//              In writable mode, the class operates directly on the
//              source stream, so only one writable substream can be
//              used on single source stream. A writable substream must
//              be positioned at the end of the source stream. Operations
//              on bytes before the beginning of the substream fail, but
//              operations beyond the end of the substream are allowed;
//              they extend the length of both the substream and the
//              source stream. Writable mode is provided for symmetry
//              with read-only mode.
//
//----------------------------------------------------------------------------

MtExtern(CSubstream)

class CSubstream : public IStream
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSubstream))
    DECLARE_FORMS_STANDARD_IUNKNOWN(CSubstream)
    
    // ctor/dtor
    CSubstream();
    ~CSubstream();
    
    // Initialization
    HRESULT InitRead(IStream *pStream, ULARGE_INTEGER cb);
    HRESULT InitWrite(IStream *pStream);
    HRESULT InitClone(CSubstream *pOrigstream);
    void    Detach();

    // IStream methods
    STDMETHOD(Read)(void HUGEP *pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD(Write)(const void HUGEP *pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD(Seek)(LARGE_INTEGER ibMove, DWORD dwOrigin, ULARGE_INTEGER *pibNewPosition);
    STDMETHOD(SetSize)(ULARGE_INTEGER ibNewSize);
    STDMETHOD(CopyTo)(IStream *_pStream, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHOD(Commit)(DWORD dwFlags);
    STDMETHOD(Revert)();
    STDMETHOD(LockRegion)(ULARGE_INTEGER ibOffset, ULARGE_INTEGER cb, DWORD dwFlags);
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER ibOffset, ULARGE_INTEGER cb, DWORD dwFlags);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD dwFlags);
    STDMETHOD(Clone)(IStream **ppStream);

private:
    // Data members
    CSubstream         *_pOrig;             // this, or, if this is a clone, the original substream
    IStream            *_pStream;           // the underlying original stream (or a clone)

    ULARGE_INTEGER      _ibStart;           // Beginning of substream
    ULARGE_INTEGER      _ibEnd;             // End of the substream
    ULONG               _fWritable : 1;     // TRUE if writable
};


HRESULT CreateReadOnlySubstream(CSubstream **ppStreamOut, IStream *pStreamSource, ULARGE_INTEGER cb);
HRESULT CreateWritableSubstream(CSubstream **ppStreamOut, IStream *pStreamSource);

#pragma INCMSG("--- End 'substm.hxx'")
#else
#pragma INCMSG("*** Dup 'substm.hxx'")
#endif
