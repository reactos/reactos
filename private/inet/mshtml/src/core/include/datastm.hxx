//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       datastm.hxx
//
//  Contents:   CDataStream IStream wrapper
//
//  History:    04-22-1997   DBau (David Bau)    Created
//
//----------------------------------------------------------------------------

#ifndef I_DATASTM_HXX_
#define I_DATASTM_HXX_
#pragma INCMSG("--- Beg 'datastm.hxx'")

class CSubstream;

//+---------------------------------------------------------------------------
//
//  Class:      CDataStream
//
//              Useful for robust reading and writing of various data types
//              to a stream. (Unlike IStream, we don't succeed on partial
//              reads.)
//
//              Makes no attempt to be particularly efficient.
//
//----------------------------------------------------------------------------

MtExtern(CDataStream)
MtExtern(CDataStream_aryLocations_pv)

class CDataStream
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDataStream))
    CDataStream() : _aryLocations(Mt(CDataStream_aryLocations_pv)) { memset(this, 0, sizeof(*this)); }
    CDataStream(IStream *pStream) : _aryLocations(Mt(CDataStream_aryLocations_pv)) { memset(this, 0, sizeof(*this)); Init(pStream); } // for convenience
    ~CDataStream() { ReleaseInterface(_pStream); }

    void Init(IStream *pStream)
    {
        Assert(pStream && !_pStream);
        pStream->AddRef();
        _pStream = pStream;
    }
    
    // Saving basic data types
    HRESULT SaveData(void *pv, ULONG cb);
    HRESULT SaveDataLater(DWORD *pdwCookie, ULONG cb);
    HRESULT SaveDataNow(DWORD dwCookie, void *pv, ULONG cb);
    HRESULT LoadData(void *pv, ULONG cb);
    
    HRESULT SaveDword(DWORD dw)   { RRETURN(THR(SaveData(&dw, sizeof(DWORD)))); }
    HRESULT LoadDword(DWORD *pdw) { RRETURN(THR(LoadData(pdw, sizeof(DWORD)))); }
    HRESULT SaveString(TCHAR *pch);
    HRESULT LoadString(TCHAR **ppch);
    HRESULT SaveCStr(const CStr *pcstr);
    HRESULT LoadCStr(CStr *pcstr);

    // Saving substreams
    HRESULT BeginSaveSubstream(IStream **ppSubstream);
    HRESULT EndSaveSubstream();
    HRESULT LoadSubstream(IStream **ppSubstream);

#if (DBG == 1)
    // Debugging Streams
    void DumpStreamInfo();
#endif

    // Direct access to underlying stream
    IStream *_pStream;
    
private:
    CSubstream *_pSubstream;     // The substream being saved (if any)
    DWORD       _dwLengthCookie; // Cookie to the position reserved for the substream length
    
    class CLocation
    {
    public:
        LARGEINT       _ib;
        ULONG          _cb;
        ULONG          _dwCookie;
    };

    CDataAry<CLocation> _aryLocations;
};

#pragma INCMSG("--- End 'datastm.hxx'")
#else
#pragma INCMSG("*** Dup 'datastm.hxx'")
#endif
