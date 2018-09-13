/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _XMLSTREAM_HXX
#define _XMLSTREAM_HXX

#include "bufferedstream.hxx"
#include "encoder/encodingstream.hxx"
#include "_rawstack.hxx"

class XMLParser;

// the XMLStream class uses the error code and token types defined in the xmlparser
#include <xmlparser.h>

//==============================================================================
// This enum and StateEntry struct are used in table driven parsing for DTD
// stuff - so that the parser isn't bloated by this stuff.  This is about 15%
// slower than a hand written parser.

typedef enum {
    OP_OWS,     // optional whitespace
    OP_WS,      // required whitespace
    OP_CHAR,    // char comparison, _pch[0] is char, _sArg1 is else goto state or error code
    OP_CHAR2,   // same os OP_CHAR - except it doesn't do _pInput->Mark.
    OP_PEEK,    // same as OP_CHAR - except it doesn't advance.
    OP_NAME,    // scan name
    OP_TOKEN,   // return token, _sArg1 = token
    OP_STRING,  // scan a string
    OP_EXTID,   // scan an external id.
    OP_STRCMP,  // string comparison.
    OP_POP,     // pop state
    OP_NWS,     // not whitespace conditional
    OP_SUBSET,  // skip an internal subset
    OP_PUBIDOPTION, // conditional for _fShortPubIdOption
    OP_NMTOKEN,
    OP_TABLE,   // push a new table. (pointer in _pch field).
    OP_STABLE,   // switch to new table. (pointer in _pch field).
    OP_COMMENT,
    OP_CONDSECT,
    OP_SNCHAR,  // conditional 'is start name char'
    OP_EQUALS,  // scan ' = '
    OP_ENCODING, // switch encoding.
    OP_CHARWS,   // match char or must be white space.
    OP_ATTRVAL, //parse attribute values.(_sArg1 = return PCDATA token or not)
    OP_PETEST,
    OP_ATTEXPAND,
    OP_NMSTRING, // unqualified name within quote 
    OP_FAKESYSTEM,
} OPCODE;

typedef struct {
    OPCODE _sOp;
    const WCHAR* _pch;
    DWORD  _sGoto;
    DWORD  _sArg1;
    long   _lDelta; // for when we do a Mark(), or Token if OP_CHAR
} StateEntry;

//================================================================================
class XMLStream
{   
public:
    XMLStream(XMLParser * pXMLParser);
    ~XMLStream();

    //------------------------------------------------------------------------
    // These are some more tokens that the XMLStream returns.
    typedef enum 
    {
        // ADDITIONAL TOKENS THAT THE PARSER SUCKS UP
        XML_PENDING = 0,    // still parsing.
        XML_NUMENTITYREF = XML_LASTSUBNODETYPE,   // &23;                 
        XML_HEXENTITYREF,   // &x0cf7;
        XML_BUILTINENTITYREF, //&gt;
        XML_TAGEND,       // >
        XML_EMPTYTAGEND,// /> (text = tag name)
        XML_ENDTAG,     // </ (text = tag name)
        XML_ENDPI,      // text = pi body minus '?>'
        XML_ENDXMLDECL, // end of xml declaration
        XML_ENDDECL,     // '>'
        XML_CLOSEPAREN,
        XML_ENDCONDSECT,    // ']]>'
        XML_STARTDTDSUBSET,
        XML_ENDPROLOG,
        XML_DATAAVAILABLE,
        XML_DATAREALLOCATE,
    } XMLToken;

    HRESULT PushStream( 
        /* [in] */ EncodingStream  *pStm,
        /* [in] */ bool fExternalPE);

    HRESULT InsertData(
        /* [in] */ const WCHAR *buffer,
        /* [in] */ long length,
        /* [in] */ bool pedata);

    HRESULT AppendData( 
        /* [in] */ const BYTE  *buffer,
        /* [in] */ long length,
        /* [in] */ BOOL lastBuffer);

    HRESULT Reset( void);

    HRESULT GetNextToken( 
        /* [out] */ DWORD  *token,
        /* [out] */ const WCHAR  **text,
        /* [out] */ long  *length,
        /* [out] */ long  *nslen);
        
    ULONG GetLine();

    ULONG GetLinePosition();
    
    ULONG GetInputPosition();

    HRESULT GetLineBuffer( 
        /* [out] */ const WCHAR  * *buf,
        /* [out] */ ULONG* len,
        /* [out] */ ULONG* startpos);

    void SetFlags( 
        /* [in] */ unsigned short usFlags);
    
    unsigned short GetFlags();
    
    // This method changes how the attribute value is parsed.
    void SetType(DWORD type);      

    WCHAR*  GetEncoding();

    HRESULT ErrorCallback(HRESULT hr);
    
    // For converting builtin entity references to unicode characters.

    void    SetDTD(bool flag) { _fDTD = flag; }
    bool    IsDTD() { return _fDTD; }

    WCHAR   getAttrValueQuoteChar() { return _chTerminator; }

private:
    HRESULT init();
    void _init();

    HRESULT GetNextTokenInDTD( 
        /* [out] */ DWORD  *token,
        /* [out] */ const WCHAR  **text,
        /* [out] */ long  *length,
        /* [out] */ long  *nslen);

    // XML Tokenization
    HRESULT firstAdvance();
    HRESULT parseContent();
    HRESULT parseElement();
    HRESULT parseEndTag();
    HRESULT parsePI();
    HRESULT parseComment();
    HRESULT parseName();
    HRESULT parseAttributes();
    HRESULT parseAttrValue();
    HRESULT processAttrValue();
    HRESULT expandAttrValue();
    HRESULT parsePCData();
    HRESULT parseString();
    HRESULT parseNMString();
    HRESULT parseEntityRef();
    HRESULT parseNmToken();

    // DTD tokenization.
    HRESULT parseCondSect();
    HRESULT parseCData();
    HRESULT parseIncludeSect();
    HRESULT parseIgnoreSect();
    HRESULT parseDTDContent();
    HRESULT parseEquals();
    HRESULT parseNames();
    HRESULT parsePEDecl();
    HRESULT parsePERef();
    HRESULT expandEntity();

    HRESULT parseTable();

    HRESULT skipWhiteSpace();
    HRESULT skipInternalSubset();
    inline void mark(long back = 0) { _pInput->Mark(back); }

    typedef HRESULT (XMLStream::* StateFunc)();

    // The state machine consists of functions where each
    // function can determine for itself its own substates 
    // so that when it is reactivated by a pop() it can pick 
    // up where it left off.  The current substate is set
    // to zero on a push() and at pop() time it is restored 
    // to whatever it was told to be in the push().
    HRESULT push(StateFunc f, short substate = 0);
    HRESULT pushTable(short substate = 0, const StateEntry* table = NULL, DWORD le = 0);
    HRESULT pop(bool boundary = true);
    HRESULT switchTo(StateFunc f); // pop & push
    HRESULT switchToTable(const StateEntry* table = NULL, DWORD le = 0); // pop & push

    // Advance and jump to state
    HRESULT AdvanceTo(short substate);

    HRESULT PopStream();

    HRESULT DTDAdvance();
    HRESULT ContinueDTDAdvance();

    HRESULT ScanHexDigits();
    HRESULT ScanDecimalDigits();

    bool    PreEntityText();

    // Always use this function instead of calling _pInput->getToken
    inline void getToken(const WCHAR** ppText, long* pLen) { _pInput->getToken(ppText,pLen); }

    BufferedStream* getCurrentStream();

    StateFunc   _fnState; // current function.
    short       _sSubState; // current substate.
    short       _sSavedState;

    struct StateInfo
    {
        StateFunc _fnState;
        short     _sSubState;
        const StateEntry* _pTable;
        DWORD     _lEOFError;
        int       _cStreamDepth;
    };
    _rawstack<StateInfo> _pStack;

    struct InputInfo
    {
        BufferedStream* _pInput;
        WCHAR           _chLookahead;
        bool            _fPE;
        bool            _fExternalPE;
        bool            _fInternalSubset; // remember that we were in internal subset.
        StateFunc       _fnState;         // remember the state function when pushstream
                                          // it is used to check parameter entity replacement text 
                                          // is properly nested with markup declarations.
    };
    _rawstack<InputInfo> _pStreams;

    // Cache the current value of _pStreams.used() which is used to making sure
    // a parameter entity doesn't pop out of the scope in which it was entered.
    int         _cStreamDepth; 

    BufferedStream* _pInput;    // current input stream.

    WCHAR       _chNextLookahead;
    bool        _fWasUsingBuffer;
    long        _lParseStringLevel;
    
    DWORD       _nPreToken;
    DWORD       _nToken;
    long        _lLengthDelta; // amount to adjust token length by
    long        _lMarkDelta; // amount to adjust mark position by
    bool        _fDelayMark;
    bool        _fFoundFirstElement; // special trick for EndProlog.

    WCHAR       _chLookahead;
    bool        _fWhitespace; // found whitespace while parsing PCDATA
    WCHAR       _chTerminator;
    WCHAR       _chBreakChar; // for parseString
    WCHAR       _chEndChar; // for parseAttributes.
    bool        _fEOF;  // reached end of file.
    bool        _fEOPE; // reach end of parameter entity.

    long        _lNslen; // namespace length
    long        _lNssep; // namespace separator length ':' or '::'.

    bool        _fShortPubIdOption; // whether system literal is required on PUBLIC id.

    long        _lEntityPos; // for parsing entity references.
    bool        _fPCDataPending; // whether pcdata is pending during parseEntityRef.
    const WCHAR* _pchCDataState;
    DWORD       _nAttrType;
    int         _cAttrCount;
    int         _nEntityNSLen; // saved namespace info for entity references.

    // Switches.
    unsigned short _usFlags;
    bool         _fFloatingAmp;
    bool         _fShortEndTags;
    bool         _fCaseInsensitive;
    bool         _fNoNamespaces;
    bool         _fNoWhitespaceNodes;
    bool         _fIE4Quirks;
    bool         _fNoDTDNodes;
    bool         _fHandlePE;   // This flag is used to turn on and off parameter entity handling in DTD

    // for table driven parsing.
    const StateEntry*     _pTable;
    DWORD           _lEOFError;

    // buffer used during whitespace normalization
    WCHAR*      _pchBuffer;
    long        _lBufLen;
    long        _lBufSize;
    bool        _fFoundWhitespace;
    bool        _fUsingBuffer;
    bool        _fFoundNonWhitespace;
    bool        _fCheckAttribute; // need to check the attribute name

    bool        _fDTD;
    bool        _fInternalSubset;
    int         _cConditionalSection;
    bool        _fFoundPEREf;
    bool        _fWasDTD;
//    bool        _fParsingNames;
    bool        _fParsingAttDef;
    int         _cIgnoreSectLevel;
    bool        _fResolved;
    bool        _fReturnAttributeValue;
    int         _cStreams;  // used to identify if PushStream was called.
    WCHAR       _wcEntityValue;
    XMLParser * _pXMLParser;  // regular pointer pointing back to the parser

    inline HRESULT     PushChar(WCHAR ch) 
    { 
        if (_lBufLen < _lBufSize) 
        { 
            _pchBuffer[_lBufLen++] = ch; return S_OK; 
        }
        else return _PushChar(ch); 
    }
    HRESULT     _PushChar(WCHAR ch); // grow the buffer.
};

#endif // _XML_STREAM_HXX
