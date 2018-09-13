//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       strbuf.hxx
//
//  Contents:   CStreamWriteBuff, CROStmOnBuffer
//
//----------------------------------------------------------------------------

#ifndef I_STRBUF_HXX_
#define I_STRBUF_HXX_
#pragma INCMSG("--- Beg 'strbuf.hxx'")

#ifndef X_ENCODE_HXX_
#define X_ENCODE_HXX_
#include "encode.hxx"
#endif

#ifndef X_ATOMTBL_HXX
#define X_ATOMTBL_HXX
#include "atomtbl.hxx"
#endif

const int WBUFF_SIZE  = 1024;

enum WB_FLAGS
{
    WBF_ENTITYREF           = 0x00000001,
    WBF_NO_WRAP             = 0x00000002,
    WBF_SAVE_PLAINTEXT      = 0x00000004,
    WBF_SAVE_VERBATIM       = 0x00000008,
    WBF_FORMATTED           = 0x00000010,
    WBF_DATABIND_MODE       = 0x00000020,
    WBF_KEEP_BREAKS         = 0x00000040,
    WBF_FORMATTED_PLAINTEXT = 0x00000080,
    WBF_NO_DATABIND_ATTRS   = 0x00000100,
    WBF_NO_TAG_FOR_CONTEXT  = 0x00000200,
    WBF_NO_NAMED_ENTITIES   = 0x00000400,
    WBF_NUMBER_LISTS        = 0x00000800,
    WBF_FOR_RTF_CONV        = 0x00001000,
    WBF_SAVE_FOR_PRINTDOC   = 0x00002000,
    WBF_NO_DQ_ENTITY        = 0x00004000,
    WBF_NO_AMP_ENTITY       = 0x00008000,
    WBF_NO_LT_ENTITY        = 0x00010000,
    WBF_NO_CHARSET_META     = 0x00020000,
    WBF_CRLF_ENTITY         = 0x00040000,
    WBF_NO_PRETTY_CRLF      = 0x00080000,
    WBF_SAVE_FOR_XML        = 0x00100000,
    WBF_Last_Enum
};

class CElement;
class CXmlNamespaceTable;

enum XMLNAMESPACETYPE;

MtExtern(CStreamWriteBuff)

//+---------------------------------------------------------------------------
//  Class:      CStreamWriteBuff
//  
//  Synopsis:   Intermediate stream write buffer which takes
//              care of formating issues and buffers the output
//              when saving a HTML tree to a file(IStream).
//
//----------------------------------------------------------------------------

class CStreamWriteBuff : public IStream, public CEncodeWriter
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CStreamWriteBuff))

    // destructor/constructor
    CStreamWriteBuff(IStream *pStm, CODEPAGE cp = g_cpDefault);
    ~CStreamWriteBuff();

     // IUnknown methods
    STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj) { return E_FAIL; }
    STDMETHOD_(ULONG,AddRef) (void)  { return 0; }
    STDMETHOD_(ULONG, Release) (void)  { return 0; }

    // IStream methods
    STDMETHOD(Read)(
         void HUGEP * pv,
         ULONG cb,
         ULONG * pcbRead) { return E_FAIL; }

    STDMETHOD(Write)(
         const void HUGEP * pv,
         ULONG cb,
         ULONG * pcbWritten) 
    {
        HRESULT hr = Write((const TCHAR *)pv, cb == -1 ? cb : cb / sizeof (TCHAR));
        if (pcbWritten)
        {
            // BUGBUG this is not correct, should be supported by our Write (istvanc)
            *pcbWritten = hr ? 0 : cb;
        }
        RRETURN(hr);
    }

    STDMETHOD(Seek)(
         LARGE_INTEGER dlibMove,
         DWORD dwOrigin,
         ULARGE_INTEGER * plibNewPosition)
    {
        ++_cPreFormat;    // Kludge to force NewLine to flush but not add CRLF
        NewLine();          // flush remaining wide chars to ansi buffer
        --_cPreFormat;

        HRESULT hr = Flush( );
        if( hr )
            goto Cleanup;

        hr = _pStm->Seek( dlibMove, dwOrigin, plibNewPosition);
        
    Cleanup:
        RRETURN(hr);
    }

    STDMETHOD(SetSize)(
         ULARGE_INTEGER libNewSize) { return E_FAIL; }

    STDMETHOD(CopyTo)(
         IStream * pstm,
         ULARGE_INTEGER cb,
         ULARGE_INTEGER * pcbRead,
         ULARGE_INTEGER * pcbWritten) { return E_FAIL; }

    STDMETHOD(Commit)(
         DWORD grfCommitFlags) { return E_FAIL; }

    STDMETHOD(Revert)( void) { return E_FAIL; }

    STDMETHOD(LockRegion)(
         ULARGE_INTEGER libOffset,
         ULARGE_INTEGER cb,
         DWORD dwLockType) { return E_FAIL; }

    STDMETHOD(UnlockRegion)(
         ULARGE_INTEGER libOffset,
         ULARGE_INTEGER cb,
         DWORD dwLockType) { return E_FAIL; }

    STDMETHOD(Stat)(
         STATSTG * pstatstg,
         DWORD grfStatFlag) { return E_FAIL; }

    STDMETHOD(Clone)(
         IStream ** ppstm) { return E_FAIL; }

    // public methods
    HRESULT Write(const TCHAR *pch, int cch = -1);

    // xml namespace handling

    HRESULT EnsureNamespaceSaved
        (CDoc * pDoc, LPTSTR pchNamespace, LPTSTR pchUrn, XMLNAMESPACETYPE namespaceType);

    HRESULT SaveNamespaceAttr(LPTSTR pchNamespace, LPTSTR pchUrn);
    HRESULT SaveNamespaceTag (LPTSTR pchNamespace, LPTSTR pchUrn);

private:
    // private methods
    HRESULT FlushMBBuffer();
    HRESULT FlushWBuffer(BOOL fIndent, BOOL fNewLine);
    HRESULT WriteDirectToMultibyte(CHAR ch, int iRepeat);

public:
    HRESULT NewLine() { return FlushWBuffer(!_cPreFormat, !_cPreFormat); }
    HRESULT WriteRule();
    
    void    BeginPre() { _cPreFormat++; }
    void    EndPre() { Assert (_cPreFormat); _cPreFormat--; }
    
    void    BeginIndent();
    void    EndIndent();

    void    BeginSuppress() { _cSuppress++; }
    void    EndSuppress()   { Assert(_cSuppress >= 0); _cSuppress--; }

    HRESULT Terminate();

    int     SetCurListItemIndex(int nItemIndex);
    int     GetNextListItemIndex() { return _nCurListItemIndex++; }
    DWORD   SetFlags(DWORD  dwFlags);
    DWORD   ClearFlags(DWORD  dwFlags);
    void    RestoreFlags(DWORD  dwFlags) { _dwFlags = dwFlags; }
    BOOL    TestFlag(WB_FLAGS wbFlag) { return _dwFlags & wbFlag; }
    HRESULT Flush();

    HRESULT WriteQuotedText( const TCHAR* pch, BOOL fAlwaysQuote = FALSE );

    CElement * GetElementContext();
    void       SetElementContext(CElement * pElementContext);

private:
    int                     _ichLastNewLine;    // index to the last carriage return
    int                     _iLastValidBreak;   // index to last valid line break
    int                     _cPreFormat;        // current indentation level of pre-format
    int                     _cSuppress;         // when larger than zero, suppress altogether
    int                     _cchIndent;         // count of characters to be indented
    int                     _nCurListItemIndex; // current index of a list item
    DWORD                   _dwFlags;           // state for CStreamWriteBuff
    IStream *               _pStm;              // pointer to IStream
    CElement *              _pElementContext;   // pointer to context element (or null)
    BOOL                    _fNeedIndent;       // keeps track of indent state
    CXmlNamespaceTable *    _pXmlNamespaceTable;  // keeps track of what Xml PIs we saved out 
};

inline DWORD
CStreamWriteBuff::SetFlags(DWORD dwFlags)
{
    DWORD dwOldFlags = _dwFlags;
    _dwFlags |= dwFlags;
    return dwOldFlags;
}

inline DWORD
CStreamWriteBuff::ClearFlags(DWORD dwFlags)
{
    DWORD dwOldFlags = _dwFlags;
    _dwFlags &= ~dwFlags;
    return dwOldFlags;
}

inline int
CStreamWriteBuff::SetCurListItemIndex(int nCurListItemIndex)
{
    int nOldListItemIndex = _nCurListItemIndex;
    _nCurListItemIndex = nCurListItemIndex;
    return nOldListItemIndex;
}

inline CElement *
CStreamWriteBuff::GetElementContext()
{
    return _pElementContext;
}

inline void
CStreamWriteBuff::SetElementContext(CElement * pElementContext)
{
    _pElementContext = pElementContext;
}

inline void    
CStreamWriteBuff::BeginIndent() 
{ 
    _cchIndent += 2; 
}

inline void
CStreamWriteBuff::EndIndent() 
{
    if (_cchIndent > 0)
        _cchIndent -= 2; 
    Assert (_cchIndent >= 0);
}

//+---------------------------------------------------------------------------
//  Class:      CStreamReadBuff
//  
//  Synopsis:   A class which can be instantiated from an IStream, and offers
//              a way to get chunks, lines, or simple tokens at a time.  It
//              is currently used by the HandlePasteHTMLToRange function to
//              parse the CF_HTML format
//
//----------------------------------------------------------------------------

class CStreamReadBuff : public IStream, public CEncodeReader {
public:
    CStreamReadBuff( IStream* pIStream, CODEPAGE cp = g_cpDefault);
    ~CStreamReadBuff( );

     // IUnknown methods
    STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj) { return E_FAIL; }
    STDMETHOD_(ULONG,AddRef) (void)  { return 0; }
    STDMETHOD_(ULONG, Release) (void)  { return 0; }

    // IStream methods
    STDMETHOD(Read)(
         void HUGEP * pv,
         ULONG cb,
         ULONG * pcbRead) { return Get( (TCHAR*)pv, (int)cb, (int*)pcbRead); }

    STDMETHOD(Write)(
         const void HUGEP * pv,
         ULONG cb,
         ULONG * pcbWritten) 
    {
        return E_FAIL;
    }

    STDMETHOD(Seek)(
         LARGE_INTEGER dlibMove,
         DWORD dwOrigin,
         ULARGE_INTEGER * plibNewPosition)
    {
#ifdef UNIX
        HRESULT hr = SetPosition( (LONG)QUAD_PART(dlibMove), dwOrigin );
#else
        HRESULT hr = SetPosition( (LONG)dlibMove.QuadPart, dwOrigin );
#endif
        if( plibNewPosition && hr == S_OK )
        {
            LONG lPosition;
            hr = GetPosition( &lPosition );
            if( hr == S_OK )
            {
#ifdef UNIX
                U_QUAD_PART(*plibNewPosition) = (LONGLONG) lPosition;
#else
                plibNewPosition->QuadPart = (LONGLONG) lPosition;
#endif
            }
        }
        RRETURN( hr );
    }

    STDMETHOD(SetSize)(
         ULARGE_INTEGER libNewSize) { return E_FAIL; }

    STDMETHOD(CopyTo)(
         IStream * pstm,
         ULARGE_INTEGER cb,
         ULARGE_INTEGER * pcbRead,
         ULARGE_INTEGER * pcbWritten) { return E_FAIL; }

    STDMETHOD(Commit)(
         DWORD grfCommitFlags) { return E_FAIL; }

    STDMETHOD(Revert)( void) { return E_FAIL; }

    STDMETHOD(LockRegion)(
         ULARGE_INTEGER libOffset,
         ULARGE_INTEGER cb,
         DWORD dwLockType) { return E_FAIL; }

    STDMETHOD(UnlockRegion)(
         ULARGE_INTEGER libOffset,
         ULARGE_INTEGER cb,
         DWORD dwLockType) { return E_FAIL; }

    STDMETHOD(Stat)(
         STATSTG * pstatstg,
         DWORD grfStatFlag) { return E_FAIL; }

    STDMETHOD(Clone)(
         IStream ** ppstm) { return E_FAIL; }

        
    HRESULT   Get( TCHAR* ppBuffer, int cchBuffer, int* pcbRead );
    HRESULT   GetLine( TCHAR* pBuffer, int cchBuffer );
    HRESULT   GetStringValue( TCHAR* pTag, TCHAR* pBuffer, int cchBuffer );
    HRESULT   GetLongValue( TCHAR* pTag, LONG* pLong );

    HRESULT   GetPosition( LONG* plPosition );
    HRESULT   SetPosition( LONG lPosition, DWORD dwOrigin = STREAM_SEEK_SET );  // absolute position

private:
    IStream* _pStm;                         // pointer to the stream
    TCHAR *  _achBuffer;                    // pointer to our wide character buffer
    BOOL     _eof;                          // true if we hit eof
    HRESULT  _lastGetCharError;             // last error seen by GetChar(), if any
    int      _cchChunk;                     // number of tchars in last chunk read
    int      _iIndex;                       // index within chunk buffer

    TCHAR    GetChar( );                    // get character at current position
    HRESULT  Advance( );                    // advance one position forward
    HRESULT  ReadChunk( );                  // read the next chunk into the buffer

    HRESULT  MakeRoomForChars( int cch );   // override for CEncodeReader class

};

#pragma INCMSG("--- End 'strbuf.hxx'")
#else
#pragma INCMSG("*** Dup 'strbuf.hxx'")
#endif
