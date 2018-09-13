//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       urlmon.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#ifndef _URLMON_HXX_
#define _URLMON_HXX_

#include <urlint.h>
#include <wininet.h>
#include <sem.hxx>
#include <debug.h>

#define SZPROTOCOLROOT  "PROTOCOLS\\Handler\\"
#define SZCLASS         "CLSID"
#define SZALL           "*"
#define SZNAMESPACEROOT "PROTOCOLS\\Name-Space Handler\\"
#define SZHANDLER       "HANDLER"
#define ULPROTOCOLLEN    32


//extern char szMimeKey[]     = "MIME\\Database\\Content Type\\";
//const ULONG ulMimeKeyLen    = ((sizeof(szMimeKey)/sizeof(char))-1);

#define SZMIMEKEY       "MIME\\Database\\Content Type\\"
#define ULMIMEKEYLEN    32

// move this to public header file
#define S_NEEDMOREDATA                                ((HRESULT)0x00000002L)
#define BSCF_ASYNCDATANOTIFICATION  0x00010000
#define BSCF_DATAFULLYAVAILABLE     0x00020000

#define BINDF_NEEDFILE      ((BINDF) 0x00000040)


#define BINDSTATUS_ERROR            ((BINDSTATUS)0xf0000000)
#define BINDSTATUS_INTERNAL         ((BINDSTATUS)0xf1000000)
#define BINDSTATUS_INTERNALASYNC    ((BINDSTATUS)0xf2000000)


// BUGBUG: find maximum mime size
#define SZMIMESIZE_MAX 128

// download buffer per request, can this
#define  DNLD_BUFFER_SIZE           8192
#define  SNIFFDATA_SIZE             2048
#define  MAX_URL_SIZE    INTERNET_MAX_URL_LENGTH


#define CU_NO_CANONICALIZE  0x00000000
#define CU_CANONICALIZE     0x00000001

STDAPI ConstructURL(LPBC pBC, LPMONIKER pmkContext, LPMONIKER pmkToLeft,
            LPWSTR pwzURLRelative, LPWSTR pwzURLFull, DWORD cURLSize,
            DWORD dwFlags);

// global variable
extern HINTERNET g_hSession;

LPCSTR GetUserAgentString();


//+---------------------------------------------------------------------------
//
//  Class:      CRefCount ()
//
//  Purpose:    Safe class for refcounting
//
//  Interface:  CRefCount()
//              CRefCount(LONG lValue)
//              LONG operator++()       prefix
//              LONG operator--()       prefix
//              LONG operator++(int)    postfix
//              LONG operator--(int)    postfix
//              LONG operator++(int)    cast operator
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class CRefCount
{
public:
    CRefCount()
    {
        _cRefs = 1;
    }
    CRefCount(LONG lVal)
    {
        _cRefs = lVal;
    }

    // prefix operators
    LONG operator++()
    {
        InterlockedIncrement(&_cRefs);
        return _cRefs;
    }
    LONG operator--()
    {
        InterlockedDecrement(&_cRefs);
        return _cRefs;
    }
    // postfix operators
    LONG operator++(int)
    {
        return InterlockedIncrement(&_cRefs);
    }
    LONG operator--(int)
    {
        return InterlockedDecrement(&_cRefs);
    }
    // LONG cast operator
    operator LONG()
    {
        return _cRefs;
    }

private:
    LONG _cRefs;
};

#define DLD_PROTOCOL_NONE   0
#define DLD_PROTOCOL_HTTP   1
#define DLD_PROTOCOL_FTP    2
#define DLD_PROTOCOL_GOPHER 3
#define DLD_PROTOCOL_FILE   4
#define DLD_PROTOCOL_LOCAL  5
#define DLD_PROTOCOL_HTTPS  6
#define DLD_PROTOCOL_STREAM 7

// # of internal Protocols
#define INTERNAL_PROTOCOL_MAX 8


//+---------------------------------------------------------------------------
//
//  Class:      CEnumFmtEtc ()
//
//  Purpose:    Class for formatetc enumerator
//
//  Interface:  Create --
//              QueryInterface --
//              AddRef --
//              Release --
//              Next --
//              Skip --
//              Reset --
//              Clone --
//              Initialize --
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class CEnumFmtEtc : public IEnumFORMATETC
{
public:

    // Create and return a IEnumFormatEtc object.
    // Return the object if successful, NULL otherwise.
    static CEnumFmtEtc * Create(UINT cfmtetc, FORMATETC* rgfmtetc);

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // *** IEnumFormatEtc methods ***
    STDMETHODIMP Next(ULONG celt, FORMATETC * rgelt, ULONG * pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset(void);
    STDMETHODIMP Clone(IEnumFORMATETC ** ppenum);

private:
    CEnumFmtEtc() : _CRefs()
    {
        _iNext = 0;
        _cElements = 0;
        _pFmtEtc = NULL;
    }

    ~CEnumFmtEtc()
    {
        if (_pFmtEtc)
        {
            delete _pFmtEtc;
        }
    }

    BOOL Initialize(UINT cfmtetc, FORMATETC* rgfmtetc,  UINT iPos);

private:
    CRefCount   _CRefs;         // refcount
    UINT        _iNext;         // zero-based item that will next be enumerated
    UINT        _cElements;
    FORMATETC  *_pFmtEtc;       // has at least one item
};

class CMediaType;
class CMediaTypeHolder;
class CMediaTypeNode;

typedef enum _tagDATAFORMAT
{
     DATAFORMAT_UNKNOWN         = 0
    ,DATAFORMAT_AMBIGUOUS
    ,DATAFORMAT_KNOWN
    ,DATAFORMAT_TEXT
    ,DATAFORMAT_BINARY
    ,DATAFORMAT_TEXTORBINARY
} DATAFORMAT;

typedef struct _tagMediaInfo
{
    LPSTR       _pszType;
    CLIPFORMAT  _cfFormat;
    DWORD       _dwDataFormat;
    CLSID       _clsID;
}   MediaInfo;

class CMediaType : public  MediaInfo
{
public:
    CMediaType()
    {
        _pszType = 0;
        _cfFormat = 0;
        _clsID = CLSID_NULL;
        _dwDataFormat = 0;
    }
    void Initialize(LPSTR szType, CLIPFORMAT cfFormat);
    void Initialize(CLIPFORMAT cfFormat, CLSID *pClsID);

    LPSTR GetTypeString(void)
    {
        return _pszType;
    }
    CLIPFORMAT GetClipFormat(void)
    {
        return _cfFormat;
    }
    void SetClipFormat(CLIPFORMAT cf)
    {
        _cfFormat = cf;
    }
    DWORD GetDataFormat()
    {
        return _dwDataFormat;
    }
    void SetClsID(CLSID *pClsID)
    {
        _clsID = *pClsID;
    }
    void GetClsID(CLSID *pClsID)
    {
        *pClsID = _clsID;
    }

private:
//    LPSTR       _pszType;
//    CLIPFORMAT  _cfFormat;
//    CLSID       _clsID;
//    DWORD       _dwDataFormat;
};

class CMediaTypeHolder : public IMediaHolder
{
public:
    CMediaTypeHolder();
    ~CMediaTypeHolder();

    // unknown methods
    STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);

    // mediahoder methods

    STDMETHOD(RegisterClassMapping)(
        DWORD    ctypes,
        LPCSTR   rgszNames[],
        CLSID    rgClsIDs[],
        DWORD    dwReserved);

    STDMETHOD(FindClassMapping)(
        LPCSTR szMime,
        CLSID    *pClassID,
        DWORD    dwReserved);

    HRESULT RegisterW(UINT ctypes, const LPCWSTR* rgszTypes, CLIPFORMAT* rgcfTypes);
    HRESULT Register(UINT ctypes, const LPCSTR* rgszTypes, CLIPFORMAT* rgcfTypes);
    HRESULT RegisterMediaInfo(UINT ctypes, MediaInfo *pMediaInfo, BOOL fFree);
    HRESULT RegisterClass(UINT ctypes, const LPCSTR* rgszTypes, CLSID *rgclsID);
    HRESULT FindCMediaType(LPCSTR pszMimeStr, CMediaType **ppCMType);
    HRESULT FindCMediaType(CLIPFORMAT cfFormat, CMediaType **ppCMType);


private: // methods

private:    // data
    CRefCount        _CRefs;         // refcount class
    CMediaTypeNode  *_pCMTNode;
};

class CMediaTypeNode
{
public:
    CMediaTypeNode(CMediaType *pCMType, LPSTR pszTextBuffer, UINT cElements,
                    CMediaTypeNode *pNext, BOOL fFree = TRUE)
    {
        _pCMType = pCMType;
        _pszTextBuffer = pszTextBuffer;
        _cElements = cElements;
        _pNext = pNext;
        _fFree = fFree;
    }
    ~CMediaTypeNode()
    {

        if (_pszTextBuffer)
        {
            delete _pszTextBuffer;
        }
        if (_fFree)
        {
            delete _pCMType;
        }

    }

    CMediaType *GetMediaTypeArray(void)
    {
        return _pCMType;
    }
    LPSTR GetTextBuffer(void)
    {
        return _pszTextBuffer;
    }
    UINT GetElementCount(void)
    {
        return _cElements;
    }
    CMediaTypeNode *GetNextNode()
    {
        return _pNext;
    }
    void SetNextNode(CMediaTypeNode *pNode)
    {
        _pNext = pNode;
    }


private:    // data
    CMediaType     *_pCMType;
    LPSTR           _pszTextBuffer;
    UINT            _cElements;
    BOOL            _fFree;
    CMediaTypeNode *_pNext;

};

CMediaTypeHolder *GetMediaTypeHolder();

extern CMediaTypeHolder *g_pCMHolder;
extern CMutexSem g_mxsMedia;            // single access to media holder

HRESULT FindMediaString(CLIPFORMAT cfFormat, LPSTR *ppStr);
HRESULT FindMediaType(LPSTR pszType, CLIPFORMAT *cfType);
HRESULT RegisterDefaultMediaType();
inline HRESULT InternalRegisterDefaultMediaType();

LPSTR   FindFileExtension(LPSTR pszFileName);
HRESULT GetMimeFromExt(LPSTR pszExt, LPSTR pszMime, DWORD *pcbMime);

// helper routines
LPSTR StringAFromCLSID(CLSID *pclsid);
HRESULT CLSIDFromStringA(LPSTR pszClsid, CLSID *pclsid);


#define GUIDSTR_MAX (1+ 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)
#define CLSIDSTR_MAX (GUIDSTR_MAX)


#endif //_URLMON_HXX_

