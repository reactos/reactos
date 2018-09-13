#ifndef I_ENCODE_HXX_
#define I_ENCODE_HXX_
#pragma INCMSG("--- Beg 'encode.hxx'")

#define DECLARE_ENCODING_FUNCTION( fn ) HRESULT fn( BOOL, int * )

#define ENCODE_BLOCK_SIZE           4096    // assumed to be power of 2
#define ENCODE_DBCS_THRESHOLD       5

#ifndef X_CODEPAGE_H_
#define X_CODEPAGE_H_
#include "codepage.h"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

MtExtern(CEncodeReader)
MtExtern(CEncodeWriter)
MtExtern(CToUnicodeConverterPchBuf);

//
// class CEncode -- template for the encoding classes
//
class CEncode
{
protected:
    size_t          _nBlockSize;        // blocksize
    CODEPAGE        _cp;                // In case we must switch codepages
    DWORD           _dwState;           // Encoding state for MLANG

public:

                    CEncode( size_t nBlockSize );
                    ~CEncode( );

    inline size_t   BlockSize()   { return _nBlockSize; }
    inline CODEPAGE GetCodePage() { return _cp; }
    
    virtual HRESULT PrepareToEncode( ) = 0;
    virtual HRESULT MakeRoomForChars( int cch ) = 0;
    virtual BOOL    SwitchCodePage( CODEPAGE cp, BOOL *pfDifferentEncoding = NULL, BOOL fNeedRestart = FALSE ) = 0;
};

//
// class CEncodeReader -- To unicode.
//
class CEncodeReader : public CEncode
{

private:
    // for signature detection
    unsigned    _fCheckedForUnicode:1;
    unsigned    _fDiscardUtf7BOM:1;
    
    // for general codepage detection
    unsigned    _fDetectionFailed:1;
    
    size_t      _cbScanStart;           // relative to _pbBufferPtr

    UINT        _uMaxCharSize;

    DECLARE_ENCODING_FUNCTION( (CEncodeReader::*_pfnWideCharFromMultiByte) );
    DECLARE_ENCODING_FUNCTION( WideCharFromUcs2 );
    DECLARE_ENCODING_FUNCTION( WideCharFromUcs2BigEndian );
    DECLARE_ENCODING_FUNCTION( WideCharFromUtf8 );
    DECLARE_ENCODING_FUNCTION( WideCharFromJapaneseAuto );
    DECLARE_ENCODING_FUNCTION( WideCharFromJapaneseEuc );
    DECLARE_ENCODING_FUNCTION( WideCharFromHz );
    DECLARE_ENCODING_FUNCTION( WideCharFromMultiByteGeneric );
    DECLARE_ENCODING_FUNCTION( WideCharFromMultiByteMlang );

public:

    // NB (cthrash) CEncodeReader originated as part of CHtmPre.

    TCHAR *         _pchBuffer;         // pointer to buffer
    TCHAR *         _pchEnd;            // last char in buffer
    int             _cchBuffer;         // current size

public:
    
    unsigned char * _pbBuffer;          // multibyte buffer
    unsigned char * _pbBufferPtr;       // pointer to last processed char
    size_t          _cbBuffer;          // valid bytes in buffer
    size_t          _cbBufferMax;       // buffer total size

    // NB (cthrash) We cache these values in case we need to reencode the
    // buffer.  These two pointers hold the corresponding values prior to
    // the last WideCharFromMultiByte call.

    unsigned char * _pbBufferPtrLast;   // pointer to last processed char
    TCHAR *         _pchEndLast;        // last char in buffer

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CEncodeReader))

                CEncodeReader( CODEPAGE cp, size_t nBlockSize );
                ~CEncodeReader();
    HRESULT     PrepareToEncode();
    HRESULT     WideCharFromMultiByte( BOOL fReadEof, int * pcch );

    BOOL        SwitchCodePage( CODEPAGE cp, BOOL *pfDifferentEncoding = NULL, BOOL fNeedRestart = FALSE );
    BOOL        ForceSwitchCodePage( CODEPAGE cp, BOOL *pfDifferentEncoding = NULL );
    BOOL        Exhausted();
    BOOL        AutoDetectionFailed() { Assert( !_fDetectionFailed || IsAutodetectCodePage(_cp) ); return _cp == CP_AUTO && _fDetectionFailed; }


public:
    virtual HRESULT MakeRoomForChars( int cch );

    inline BOOL     IsSBCS() { return _uMaxCharSize == 1; }
};

//
// class CEncodeWriter -- From unicode
//
class CEncodeWriter : public CEncode
{

public:
    DECLARE_ENCODING_FUNCTION( (CEncodeWriter::*_pfnMultiByteFromWideChar) );
    DECLARE_ENCODING_FUNCTION( UnicodeFromWideChar );
    DECLARE_ENCODING_FUNCTION( Utf8FromWideChar );
    DECLARE_ENCODING_FUNCTION( MultiByteFromWideCharGeneric );
    DECLARE_ENCODING_FUNCTION( MultiByteFromWideCharMlang );
    DECLARE_ENCODING_FUNCTION( MultiByteFromWideCharMlang2 );

    char     _cDefaultChar;                 // character to use when we cannot map properly

    UINT     _uiWinCodepage;                // Cached call to WindowsCodePageFromCodePage(_cp)

    unsigned _fEntitizeUnknownChars : 1;


public:

    unsigned char * _pbBuffer;         // pointer to buffer
    int             _cbBuffer;         // number of bytes currently in buffer
    int             _cbBufferMax;      // maximum bytes that can go into _pbBuffer

public:
    
    TCHAR *         _pchBuffer;         // unicode buffer
    size_t          _cchBuffer;         // valid bytes in buffer
    size_t          _cchBufferMax;      // buffer total size

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CEncodeWriter))

                CEncodeWriter( CODEPAGE cp, size_t nBlockSize );
                ~CEncodeWriter();
    HRESULT     PrepareToEncode();
    HRESULT     MultiByteFromWideChar( BOOL fChanPending, int * pcch );

    BOOL        SwitchCodePage( CODEPAGE cp, BOOL *pfDifferentEncoding = NULL, BOOL fNeedRestart = FALSE );
    void        SetEntitizeUnknownChars( BOOL fEntitize )
                        { _fEntitizeUnknownChars = fEntitize; }

public:

    virtual HRESULT MakeRoomForChars( int cch );
};

class CToUnicodeConverter : public CEncodeReader
{
public:

#ifdef PERFMETER
    CToUnicodeConverter(CODEPAGE cp) : CEncodeReader(cp,0) { _mt = Mt(CToUnicodeConverterPchBuf); }
    void SetMt(PERFMETERTAG mt) { _mt = mt; }
#else
    CToUnicodeConverter(CODEPAGE cp) : CEncodeReader(cp,0) { }
#endif
    ~CToUnicodeConverter();

    HRESULT Convert( const char *pbBuffer, const int cbBuffer, TCHAR ** ppchBuffer, int *pcch );


private:

    HRESULT MakeRoomForChars( int cch );

private:

    BOOL _fMakeRoomForNUL;

#ifdef PERFMETER
    PERFMETERTAG _mt;
#endif
};

#pragma INCMSG("--- End 'encode.hxx'")
#else
#pragma INCMSG("*** Dup 'encode.hxx'")
#endif
