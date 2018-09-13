/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _XMLPARSER_HXX
#define _XMLPARSER_HXX

#include <xmlparser.h>
class XMLStream;

typedef _reference<IXMLParser> RXMLParser;
typedef _reference<IXMLNodeFactory> RNodeFactory;
typedef _reference<IUnknown> RUnknown;
typedef _reference<IBindStatusCallback> RIBindStatusCallback;

#include "../net/urlstream.hxx"
#include "../encoder/encodingstream.hxx"

#include "_rawstack.hxx"

//------------------------------------------------------------------------
// An internal Parser IID so that DTDNodeFactory can call internal methods.
const IID IID_Parser = {0xa79b04fe,0x8b3c,0x11d2,{0x9c, 0xd3,0x00,0x60,0xb0,0xec,0x3d,0x30}};

class XMLParser : public _unknown<IXMLParser, &IID_IXMLParser>,
                    public URLDownloadTask
{
public:
#ifdef UNIX
#ifndef POSSIBLY_FOSSIL_CODE
// BUGBUG
// This was a hack to fix this calling the wrong delete
        void operator delete(void *pv);
#endif // FOSSIL_CODE
#endif // UNIX
        XMLParser();
        ~XMLParser();
#ifdef RENTAL_MODEL
        XMLParser(RentalEnum);
#endif
		// ======= IUnknown override ============================
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject);

        virtual ULONG STDMETHODCALLTYPE AddRef( void);
   
        virtual ULONG STDMETHODCALLTYPE Release( void);

		// ====== IXMLNodeSource methods ========================
        virtual HRESULT STDMETHODCALLTYPE SetFactory( 
            /* [in] */ IXMLNodeFactory __RPC_FAR *pNodeFactory);
        
        virtual HRESULT STDMETHODCALLTYPE GetFactory(
            /* [out] */ IXMLNodeFactory** ppNodeFactory);

        virtual HRESULT STDMETHODCALLTYPE Abort( 
            /* [in] */ BSTR bstrErrorInfo);

        virtual ULONG STDMETHODCALLTYPE GetLineNumber( void);
        
        virtual ULONG STDMETHODCALLTYPE GetLinePosition( void);
        
        virtual ULONG STDMETHODCALLTYPE GetAbsolutePosition( void);

        virtual HRESULT STDMETHODCALLTYPE GetLineBuffer( 
            /* [out] */ const WCHAR __RPC_FAR *__RPC_FAR *ppwcBuf,
            /* [out] */ ULONG __RPC_FAR *pulLen,
            /* [out] */ ULONG __RPC_FAR *pulStartPos);
        
        virtual HRESULT STDMETHODCALLTYPE GetLastError( void);
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorInfo( 
            /* [out] */ BSTR __RPC_FAR *pbstrErrorInfo);

        virtual ULONG STDMETHODCALLTYPE GetFlags();

        virtual HRESULT STDMETHODCALLTYPE GetURL( 
            /* [out] */ const WCHAR __RPC_FAR *__RPC_FAR *ppwcBuf);

		// ====== IXMLParser methods ==============================

        virtual HRESULT STDMETHODCALLTYPE SetURL( 
            /* [in] */ const WCHAR* pszBaseUrl,
            /* [in] */ const WCHAR* pszRelativeUrl,
            /* [in] */ BOOL async);

        virtual HRESULT STDMETHODCALLTYPE Load( 
            /* [in] */ BOOL fFullyAvailable,
            /* [in] */ IMoniker __RPC_FAR *pimkName,
            /* [in] */ LPBC pibc,
            /* [in] */ DWORD grfMode);

        virtual HRESULT STDMETHODCALLTYPE SetInput( 
            /* [in] */ IUnknown __RPC_FAR *pStm);
        
        virtual HRESULT STDMETHODCALLTYPE PushData( 
#ifdef UNIX
            //
            // The IDL compiler under Unix is outputting a 
            // /* [in] */ const unsigned char *pData,
            // instead of
            // /* [in] */ const char *pData,
            // See also xmlparser.cxx
            //
            /* [in] */ const unsigned char *pData,
#else
            /* [in] */ const char __RPC_FAR *pData,
#endif
            /* [in] */ ULONG ulChars,
            /* [in] */ BOOL bLastBuffer);
        
        virtual HRESULT STDMETHODCALLTYPE LoadEntity(
            /* [in] */ const WCHAR* pszBaseUrl,
            /* [in] */ const WCHAR* pszRelativeUrl,
            /* [in] */ BOOL fpe);

        virtual HRESULT STDMETHODCALLTYPE ParseEntity(
            /* [in] */ const WCHAR* pwcText, 
            /* [in] */ ULONG ulLen,
            /* [in] */ BOOL fpe);

	    virtual HRESULT STDMETHODCALLTYPE ExpandEntity(
            /* [in] */ const WCHAR* pwcText, 
            /* [in] */ ULONG ulLen);

        virtual HRESULT STDMETHODCALLTYPE SetRoot( 
            /* [in] */ PVOID pRoot);
        
        virtual HRESULT STDMETHODCALLTYPE GetRoot( 
            /* [in] */ PVOID __RPC_FAR *ppRoot);

        virtual HRESULT STDMETHODCALLTYPE Run( 
            /* [in] */ long lChars);
        
        virtual HRESULT STDMETHODCALLTYPE GetParserState( void);
                
        virtual HRESULT STDMETHODCALLTYPE Suspend( void);
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void);
        
        virtual HRESULT STDMETHODCALLTYPE SetFlags( 
            /* [in] */ ULONG iFlags);
        
        virtual HRESULT STDMETHODCALLTYPE LoadDTD(
            /* [in] */ const WCHAR* pszBaseUrl,
            /* [in] */ const WCHAR* pszRelativeUrl);
    
        virtual HRESULT STDMETHODCALLTYPE SetSecureBaseURL( 
            /* [in] */ const WCHAR* pszBaseUrl);

        virtual HRESULT STDMETHODCALLTYPE GetSecureBaseURL( 
            /* [out] */ const WCHAR __RPC_FAR *__RPC_FAR *ppwcBuf);

        // ======= URLDownloadTask method =============================
        virtual HRESULT HandleData(URLStream* pStm, bool last);
        virtual void SetMimeType(URLStream *pStm, const WCHAR * pwszMimeType, int length);
        virtual void SetCharset(URLStream *pStm, const WCHAR * pwszCharset, int length);

        // ======= internal only methods for Parser 
        HRESULT  SetCurrentURL( 
            /* [in] */ const WCHAR* pszCurrentUrl); // SRC attribute on <SCRIPT island>

        HRESULT SetBaseURL( 
            /* [in] */ const WCHAR* pszBaseUrl); // in case PushData is called.

        void setIgnoreEncodingAttr(bool flag) { _fIgnoreEncodingAttr = flag; }

        void setSafetyOptions(DWORD options) { _dwSafetyOptions = options; }

        HRESULT ErrorCallback(HRESULT hr);

private:
        void ctorInit();

        HRESULT PushURL( 
            /* [in] */ const WCHAR* pszBaseUrl,
            /* [in] */ const WCHAR* pszRelativeUrl,
            /* [in] */ bool async,
            /* [in] */ bool tokenizer,
            /* [in] */ bool dtd,
            /* [in] */ bool fentity,
            /* [in] */ bool fpe );


        HRESULT PushTokenizer(URLStream* stream);
        HRESULT PushDownload(URLStream* stream, XMLStream* tokenizer);
        HRESULT PopDownload();

        XMLStream*  _pTokenizer;
        PVOID       _pRoot;
        HRESULT     _fLastError;
        BSTR        _bstrError;
        bool        _fWaiting;
        bool        _fSuspended;
        bool        _fStopped;
        bool        _fStarted;
        bool        _fInXmlDecl;
        bool        _fFoundEncoding;
        USHORT      _usFlags;
        bool        _fCaseInsensitive;
        bool        _fSettingUp;
        bool        _fTokenizerChanged;
        bool        _fGotVersion;
        long        _fRunEntryCount;
        int         _cDTD;
        bool        _fInLoading;
        bool        _fInsideRun;
        bool        _fFoundRoot;
        bool        _fFoundDTDAttribute;
        bool        _fSeenDocType;
        bool        _fRootLevel; // whether we are at the root level in document.
        bool        _fFoundNonWS;
        bool        _fPendingBeginChildren;
        bool        _fPendingEndChildren;
        bool        _fIE4Mode;
        BSTR        _fAttemptedURL;

        struct Download
        {
            XMLStream*      _pTokenizer;
            RURLStream      _pURLStream;
            REncodingStream _pEncodingStream;
            bool            _fAsync;
            bool            _fDTD;
            bool            _fEntity;
            bool            _fPEReference;
            bool            _fFoundNonWS;
            bool            _fFoundRoot;    // saved values in case we're downloading a schema
            bool            _fSeenDocType;
            bool            _fRootLevel; // whether we are at the root level in document.
            int             _fDepth;    // current depth of stack.
        };
        _rawstack<Download> _pDownloads;
        Download*       _pdc;   // current download.


        // the Context struct contains members that map to the XML_NODE_INFO struct
        // defined in xmlparser.idl so that we can pass the contents of the Context
        // as a XML_NODE_INFO* pointer in BeginChildren, EndChildren and Error.

        typedef struct _MY_XML_NODE_INFO : public XML_NODE_INFO
        {
//            DWORD           dwSize;             // size of this struct
//            DWORD           dwType;             // node type (XML_NODE_TYPE)
//            DWORD           dwSubType;          // node sub type (XML_NODE_SUBTYPE)
//            BOOL            fTerminal;          // whether this node can have any children
//            WCHAR*          pwcText;            // element names, or text node contents.
//            ULONG           ulLen;              // length of pwcText
//            ULONG           ulNsPrefixLen;      // if element name, this is namespace prefix length.
//            PVOID           pNode;              // optionally created by & returned from node factory
//            PVOID           pReserved;          // reserved for factories to use between themselves.
            WCHAR*          _pwcTagName;        // saved tag name
            ULONG           _ulBufLen; 
        } MY_XML_NODE_INFO;
        typedef MY_XML_NODE_INFO* PMY_XML_NODE_INFO;

        _rawstack<MY_XML_NODE_INFO> _pStack;

        long            _lCurrentElement;
        PMY_XML_NODE_INFO _pCurrent; 
        USHORT          _cAttributes; // count of attributes on stack.

        // And we need a contiguous array of pointers to the XML_NODE_INFO 
        // structs for CreateNode.
        PMY_XML_NODE_INFO* _paNodeInfo;
        USHORT             _cNodeInfoAllocated;
        USHORT             _cNodeInfoCurrent;
        
        PVOID   _pNode; // current node (== pCurrent->pNode OR _pRoot).

        // Push saves this factory in the context and pop restores it
        // from the context.
        RNodeFactory _pFactory; // current factory (!= pCurrent->pParentFactory).

        HRESULT push(XML_NODE_INFO& info);
        HRESULT pushAttribute(XML_NODE_INFO& info);
        HRESULT pushAttributeValue(XML_NODE_INFO& info);
        HRESULT pushDTDAttribute(XML_NODE_INFO& info);

        HRESULT pop(const WCHAR* tag, ULONG len);
        HRESULT pop();
        HRESULT popAttributes();
        void    popAttribute();
        HRESULT popDTDAttribute();

        HRESULT CopyContext();
        HRESULT CopyText(PMY_XML_NODE_INFO pNodeInfo);
        HRESULT ReportUnclosedTags(int index);
        HRESULT GrowBuffer(PMY_XML_NODE_INFO pNodeInfo, long newlen);
        HRESULT GrowNodeInfo();
        
        CRITICAL_SECTION _cs;

#ifdef RENTAL_MODEL
        RentalEnum _reThreadModel;
#endif
        HRESULT init();
        HRESULT ExtractURL(IMoniker* pmk, LPBC pbc, URL* pURL);
        HRESULT PushStream(IStream* pStm, bool fpe);
        Download* FindDownload(URLStream* pStream);
        WCHAR*   getSecureBaseURL() 
                {
                    if (_pszSecureBaseURL)
                        return _pszSecureBaseURL;
                    else if (_dwSafetyOptions)
                        return _pszBaseURL;
                    return NULL;
                 }


        WCHAR*  _pszSecureBaseURL;
        WCHAR*  _pszCurrentURL;
        WCHAR*  _pszBaseURL;
        bool    _fIgnoreEncodingAttr;
        DWORD   _dwSafetyOptions;
};


#endif // _XMLPARSER_HXX
