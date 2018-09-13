/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#define WORKINGSET

#if PRODUCT_PROF
#include "icapexp.h"
#endif

#include "core/com/bstr.hxx"
#include "core/io/cstdio.hxx"

#ifndef _XML_OM_DOCUMENT
#include "xml/om/document.hxx"
#endif

#ifndef _XML_OM_ELEMENT
#include "xml/om/element.hxx"
#endif

#ifndef _XML_OUTPUTHELPER
#include "xml/util/outputhelper.hxx"
#endif

#include "resource.h"

#ifndef _XML_OM_NODEDATANODEFACTORY
#include "xml/om/nodedatanodefactory.hxx"
#endif

#ifdef PRFDATA
#include "core/prfdata/msxmlprfcounters.h"
#include "core/prfdata/msxmlprfdatamap.hxx"
#endif

#include "core/io/stringbufferoutputstream.hxx"

extern bool g_fClassInitCalled; // whether class init functions such as Name::classInit() have been called

#ifndef _XQL_PARSE_XQLPARSER
#include "xql/parser/xqlparser.hxx"
#endif

#ifndef _XTL_ENGINE_PROCESSOR
#include "xtl/engine/processor.hxx"
#endif

#include "core/io/filestream.hxx"

#ifndef _RAWSTACK_HXX
#include "core/util/_rawstack.hxx" 
#endif

extern HRESULT
CreateStreamOnFile(LPCTSTR pchFile, DWORD dwSTGM, LPSTREAM * ppstrm);

bool _MSXMLEXE_ = true;    // not used by new parser
bool fStreamTest = false;
bool fCreateTest = false;
bool fEncode = false;
bool fUnicode = false;
bool fDecode = false;
bool fOutput = false;
bool fDecodeEncode = false;
bool fPreserveWhiteSpace = true;
bool fAtomValues = false;
bool fFreeThreaded = false;

//BUGBUG: a hack to enforce different error message in getCurrentContext()
extern bool _MSXMLEXE_;

#if DBG == 1
BOOL g_fMemoryLeak;
#endif

extern void xqlMain(AString * args);


void LoadXMLDocument(BSTR b, IXMLDocument2 ** pp)
{
     Document * d = new Document();
     d->setCaseInsensitive(true);
     checkhr(d->QueryInterface(IID_IXMLDocument2, (void **) pp));
     (*pp)->put_URL(b);
}


#ifndef UNIX
void __cdecl hprintf(HANDLE hfile, char * pchFmt, ...)
{
    char    ach[2048];
    UINT    cb;
    DWORD   dw;
    va_list vl;

    va_start(vl, pchFmt);
    cb = wvsprintfA(ach, pchFmt, vl);
    va_end(vl);

    WriteFile(hfile, ach, cb, &dw, NULL);
}
#endif // UNIX

#ifdef WORKINGSET

extern void WsExit();

extern LONG_PTR WsPages();

extern LONG_PTR WsStart(LONG_PTR row);

extern bool WsPageInfo(LONG_PTR page, MEMORY_BASIC_INFORMATION *);
                       
void DumpWorkingSet(bool append = false)
{
    LONG_PTR c, l;

    if (WsTakeSnapshot(GetCurrentProcess()) != S_OK)
        return;

#ifdef UNIX
    CHAR szHeapDumpFile[] = "wkset.txt";
#else
    CHAR szHeapDumpFile[] = "\\wkset.txt";
#endif

    
    HANDLE hfile = CreateFileA(szHeapDumpFile, GENERIC_WRITE,
        FILE_SHARE_READ, NULL, append ? OPEN_ALWAYS : CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
 
    if (hfile == INVALID_HANDLE_VALUE)
        return;

    if (append)
        SetFilePointer (hfile, 0, 0, FILE_END) ; 

    c = WsCount();

    hprintf(hfile,"\n\n");
    hprintf(hfile, "%20s %20s %8s %10s Total=%8d\n",
        "Module", "Section", "Size", "Start", WsTotal());
    for (l = 0; l < c; l++)
    {
        hprintf(hfile, "%20S %20S %8d %10X\n",
            WsGetModule(l),
            WsGetSection(l),
            WsSize(l),
            WsStart(l));
    }

    hprintf(hfile,"\n\n");
    hprintf(hfile, "%10s %10s %10s\n",
        "AllocationBase", "RegionSize", "BaseAddress");

    c = WsPages();
    MEMORY_BASIC_INFORMATION mbi;

    for (l = 0; l < c; l++)
    {
        if (WsPageInfo(l, &mbi))
            if (mbi.State == MEM_COMMIT)
                hprintf(hfile, "%10X %10X %10X\n", mbi.AllocationBase, mbi.RegionSize, mbi.BaseAddress);
    }

    hprintf(hfile,"\n\n");
    hprintf(hfile, "%10s %10s %10s\n",
        "AllocationBase", "RegionSize", "BaseAddress");

    for (l = 0; l < c; l++)
    {
        if (WsPageInfo(l, &mbi))
            if (mbi.State == MEM_COMMIT)
                hprintf(hfile, "%10d %10d %10d\n", mbi.AllocationBase, mbi.RegionSize, mbi.BaseAddress);
    }

    CloseHandle(hfile);
}
#endif

//-------------------------------------------------------------------
class TestNodeFactory : public _unknown<IXMLNodeFactory, &IID_IXMLNodeFactory>
{
    _reference<IXMLNodeFactory> _pFactory;
public:
    TestNodeFactory(IXMLNodeFactory* other)
    {
        _pFactory = other;
    }

    ~TestNodeFactory()
    {
        _pFactory = null;
    }

    virtual HRESULT STDMETHODCALLTYPE NotifyEvent( 
			/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
			/* [in] */ XML_NODEFACTORY_EVENT iEvt)
    {
        if (_pFactory != NULL) 
            return _pFactory->NotifyEvent(pSource, iEvt);
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE BeginChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo)
    {
        if (_pFactory != NULL) 
            return _pFactory->BeginChildren(pSource, pNodeInfo);
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE EndChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ BOOL fEmptyNode,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo)
    {
        if (_pFactory != NULL) 
            return _pFactory->EndChildren(pSource, fEmptyNode, pNodeInfo);
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Error( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ HRESULT hrErrorCode,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
    {
        if (_pFactory != NULL) 
            return _pFactory->Error(pSource, hrErrorCode, cNumRecs, apNodeInfo);
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateNode( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
    {
        if (_pFactory != NULL) 
            return _pFactory->CreateNode(pSource, pNodeParent, cNumRecs, apNodeInfo);
        return S_OK;
    }

};    

static TCHAR s_achDTDSubSet[] = _T("[");
#define CHAR_DTDSUBSET _T('[')
static TCHAR s_achComment[] = _T("-");
#define CHAR_COMMENT _T('-')
static TCHAR s_achEntityRef[] = _T("&");
#define CHAR_ENTITYREF _T('&')
static TCHAR s_achPI[] = _T("?");
#define CHAR_PI _T('?')
static TCHAR s_achXMLPI[] = _T("^");
#define CHAR_XMLPI _T('^')
static TCHAR s_achDocType[] = _T("!");
#define CHAR_DOCTYPE _T('!')
static TCHAR s_achCDATA[] = _T("<");
#define CHAR_CDATA _T('<')
static TCHAR s_achWHITESPACE[] = _T("\r\n");

#if DBG==1
extern char * 
nodeTypeDebug( DWORD dwType);
#endif

#define XML_MASK    0x1f
#define XML_SUBTYPE 0x20
#define XML_EMPTY   0x40
#define XML_END     0x80

DeclareTag(tagEncode, "Encode", "Encode/Decode");
DeclareTag(tagDecode, "Decode", "Encode/Decode");
DeclareTag(tagLeaks, "Memory", "Leak reporting");


/**
 * Encoding:
 *
 * <start element>[tag]<attribute>[tag]"text"<attribute>[tag]"text"...<start element>...<end element>
 *
 * <> : 8 bits element type
 *
 * 255 = end element
 *
 * [] : 8 bits
 * 0 means introduce a new tag followed by text
 * new tag increments every tag 1 up, 255th tag will be discarded
 *
 * "" : UTF8
 * 1 byte length followed by text
 * or byte 255 + 2 byte length
 */
#include "../tokenizer/encoder/charencoder.hxx"

class EncodeNodeFactory : public _unknown<IXMLNodeFactory, &IID_IXMLNodeFactory>
{
    _reference<IXMLNodeFactory> _pFactory;

    int _iTagIndex;             // points to next tag to be added
    RAString _aStringTags;      // array of tag strings
    raint _aiHashTags;          // hashed values for tags
    int _iValueIndex;
    RAString _aStringValues;      // array of value strings
    raint _aiHashValues;          // hashed values for values
    _reference<IStream> _pOut;  // output
//    DWORD _dwBuffer;            // buffer for output bits
//    ULONG _ulBits;              // number of bits in buffer
    DWORD _dwLastType;          // last type from CreateNode
    DWORD _dwLastElementType;   // last element type
    rachar _achar;              // char buffer
#if DBG == 1
    unsigned _uNodes;
    DWORD dwTypes[XML_MASK];
    DWORD dwTextSize;
    DWORD dwWhiteSpaceSize;
    DWORD dwTagSize;
    DWORD dwTags;
#endif
    bool _fUnicode;
    DWORD _dwMode;

public: void * operator new(size_t size)
        {
            void * p = ::operator new(size);
            memset(p, 0, size);
            return p;
        }

public:
    EncodeNodeFactory(IXMLNodeFactory * pOther, IStream * pOut, bool fUnicode)
    {
        _fUnicode = fUnicode;
        _pFactory = pOther;
        _iTagIndex = 0;
        _aStringTags = new (255) AString;
        _aiHashTags = new (255) aint;
        _iValueIndex = 0;
        _aStringValues = new (255) AString;
        _aiHashValues = new (255) aint;
        _pOut = pOut;
//        _dwBuffer = 0;
//        _ulBits = 0;
        _dwLastType = 0;
	_dwMode = 0;
        _dwLastElementType = 0;
        _achar = new (4096 * 3) achar;
        Assert(XML_LASTNODETYPE < XML_MASK);
#if DBG == 1
        _uNodes = 0;
        EnableTag(tagEncode, TRUE);
        SetDiskFlag (tagEncode, TRUE);
#endif
    }

    ~EncodeNodeFactory()
    {
    }

    int addTag(const TCHAR * pch, ULONG ulLen)
    {
        int iHash = String::hashCode(pch, ulLen);
        for (int i = 0; i < 254; i++)
        {
            if ((*_aiHashTags)[i] == iHash && (*_aStringTags)[i]->equals(pch, ulLen))
            {
                int iTag = _iTagIndex - i;
                if (iTag <= 0)
                    iTag += 255;
                TraceTag((tagEncode, "Found tag \"%s\" at %d, returned %d", (char *)AsciiText((*_aStringTags)[i]), i, iTag));
                return iTag;
            }
        }
        // have to add it
        (*_aiHashTags)[_iTagIndex] = iHash;
        (*_aStringTags)[_iTagIndex] = String::newString(pch, 0, ulLen);
        TraceTag((tagEncode, "Added new tag \"%s\" at %d", (char *)AsciiText((*_aStringTags)[_iTagIndex]), _iTagIndex));
        if (++_iTagIndex > 254)
            _iTagIndex = 0;
        return 0;
    }

    int addValue(const TCHAR * pch, ULONG ulLen)
    {
        int iHash = String::hashCode(pch, ulLen);
        for (int i = 0; i < 254; i++)
        {
            if ((*_aiHashValues)[i] == iHash && (*_aStringValues)[i]->equals(pch, ulLen))
            {
                int iValue = _iValueIndex - i;
                if (iValue <= 0)
                    iValue += 255;
                TraceTag((tagEncode, "Found value \"%s\" at %d, returned %d", (char *)AsciiText((*_aStringValues)[i]), i, iValue));
                return iValue;
            }
        }
        // have to add it
        (*_aiHashValues)[_iValueIndex] = iHash;
        (*_aStringValues)[_iValueIndex] = String::newString(pch, 0, ulLen);
        TraceTag((tagEncode, "Added new tag \"%s\" at %d", (char *)AsciiText((*_aStringValues)[_iValueIndex]), _iValueIndex));
        if (++_iValueIndex > 254)
            _iValueIndex = 0;
        return 0;
    }

    /*
    void writeBits(DWORD dw, ULONG ulLen)
    {
        Assert((_ulBits & 1) == 0);
        ULONG ul, ulBits;
        
//        TraceTag((tagEncode, "Writing %d bits [%d]", ulLen, dw));
        Assert(ulLen < sizeof(_dwBuffer) * 8);
        ulBits = 0;
        while(ulLen)
        {
            ul = (sizeof(_dwBuffer) * 8) - _ulBits;
            if (ul > ulLen)
                ul = ulLen;
            ulLen -= ul;
            _dwBuffer |= (dw << _ulBits);
            dw >>= ul;
            _ulBits += ul;
            if (_ulBits == sizeof(_dwBuffer) * 8)
            {
                ULONG ulWritten;

                checkhr(_pOut->Write(&_dwBuffer, sizeof(_dwBuffer), &ulWritten));
                Assert(ulWritten = sizeof(_dwBuffer));
                _dwBuffer = 0;
                _ulBits = 0;
            }
        }
        Assert(dw == 0);
    }
    */
    void writeByte(byte b)
    {
        ULONG ulWritten;

//        TraceTag((tagEncode, "writeByte %d", (unsigned)b)); 
        checkhr(_pOut->Write(&b, sizeof(b), &ulWritten));
        Assert(ulWritten = sizeof(b));
    }

    void writeWord(WORD w)
    {
        ULONG ulWritten;

//        TraceTag((tagEncode, "writeWord %d", (unsigned)w)); 
        checkhr(_pOut->Write(&w, sizeof(w), &ulWritten));
        Assert(ulWritten = sizeof(w));
    }

    void writeType(int iType, int iSubType)
    {
        TraceTag((tagEncode, "writeType %s (%x)", nodeTypeDebug((DWORD)(iType & XML_MASK)), iType)); 
        Assert(iType < 256);
        if (iSubType)
        {
            writeByte((byte)iType | XML_SUBTYPE);
            TraceTag((tagEncode, "writeSubType %d", iSubType)); 
            Assert(iSubType < 256);
            writeByte((byte)iSubType);
        }
        else
            writeByte((byte)iType);
    }

    void writeTag(const TCHAR * pch, ULONG ulLen)
    {
#if DBG == 1
        dwTagSize += ulLen;
        dwTags++;
#endif
        TraceTag((tagEncode, "writeTag %d \"%.*S\"", ulLen, ulLen, pch));
        DWORD dw;
        int iTag = addTag(pch, ulLen);
        if (iTag == 0)
        {
            // new tag, write out text as well
            writeByte(0);
            writeText(pch, ulLen);
        }
        else
        {
            writeByte((byte)iTag);
        }
    }

    void writeValue(const TCHAR * pch, ULONG ulLen)
    {
        TraceTag((tagEncode, "writeValue %d \"%.*S\"", ulLen, ulLen, pch));
        DWORD dw;
        int iValue = addValue(pch, ulLen);
        if (iValue == 0)
        {
            // new value, write out text as well
            writeByte(0);
            writeText(pch, ulLen);
        }
        else
        {
            writeByte((byte)iValue);
        }
    }

    void writeText(const TCHAR * pch, ULONG ulLen)
    {
        ULONG ulWritten;
        char buf[4];
        UINT ul;
        UINT cb;
#if DBG == 1
        UINT cbWritten = 0;
        const TCHAR * pchWrite = pch;
        ULONG ulWrite = ulLen;
#endif

        if (!ulLen)
        {
            writeByte((byte)ulLen);
        }
        else
        {
            while (ulLen)
            {
                if (_fUnicode)
                {
                    ul = 65535;
                    if (ul > ulLen)
                        ul = ulLen;
                    if (ul < 255)
                    {
                        writeByte((byte)ul);
                    }
                    else 
                    {
                        writeByte(255);
                        writeWord((WORD)ul);
                    }
                    checkhr(_pOut->Write(pch, ul * sizeof(TCHAR), &ulWritten));
    #if DBG == 1
                    for (unsigned u = 0; u < ul; u++)
                        TraceTag((tagEncode, "char %d", (unsigned)pch[u]));
                    cbWritten += ulWritten;
    #endif
                    ulLen -= ul;
                    pch += ul;
                }
                else
                {
                    ul = 65536 / 3;
                    if (ul > ulLen)
                        ul = ulLen;
                    if (_achar->length() < (int)ul * 3)
                        _achar = new (ul * 3) achar;
                    cb = _achar->length();

                    checkhr(CharEncoder::wideCharToUtf8(&_dwMode, CP_UNDEFINED, (TCHAR *)pch, &ul, (BYTE *)_achar->getData(), &cb));
                    ulLen -= ul;
                    pch += ul;
                    Assert(cb < 65536);
                    if (cb < 255)
                    {
                        writeByte((byte)cb);
                    }
                    else 
                    {
                        writeByte(255);
                        writeWord((WORD)cb);
                    }
                    checkhr(_pOut->Write((char *)_achar->getData(), cb, &ulWritten));
    #if DBG == 1
                    cbWritten += ulWritten;
                    for (unsigned u = 0; u < cb; u++)
                        TraceTag((tagEncode, "char %d", (unsigned)(*_achar)[u]));
    #endif
                }
            }
        }
        TraceTag((tagEncode, "writeText %d \"%.*S\", written %d bytes", ulWrite, ulWrite, pchWrite, cbWritten));
    }

    virtual HRESULT STDMETHODCALLTYPE NotifyEvent( 
			/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
			/* [in] */ XML_NODEFACTORY_EVENT iEvt)
    {
        ULONG ulWritten;

        TraceTag((tagEncode, "NotifyEvent %d", iEvt));
        if (iEvt == XMLNF_STARTDOCUMENT)
        {
//            _pOut->Write("\xff\xfe", 2, &ulWritten);
//            _pOut->Write(_T("<?xml version='1.0' encoding=''?>"), 66, &ulWritten);
        }
        else if (iEvt == XMLNF_ENDDOCUMENT)
        {
            writeType(XML_END, 0);
#if DBG == 1
            PrintStream * out = StdIO::getOut();
            for (int i = 1; i < XML_LASTNODETYPE; i++)
            {
                out->print(i);
                out->print(_T(": "));
                out->println((int)dwTypes[i]);
            }
            out->print(_T("tags: "));
            out->println((int)dwTags);
            out->print(_T("textSize: "));
            out->println((int)dwTextSize);
            out->print(_T("tagSize: "));
            out->println((int)dwTagSize);
            out->print(_T("whiteSpaceSize: "));
            out->println((int)dwWhiteSpaceSize);
#endif
        }
        if (_pFactory != NULL) 
            return _pFactory->NotifyEvent(pSource, iEvt);
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE BeginChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo)
    {
        TraceTag((tagEncode, "BeginChildren %s %d \"%.*S\"", nodeTypeDebug(pNodeInfo->dwType), pNodeInfo->ulLen, pNodeInfo->ulLen, pNodeInfo->pwcText));
        writeType(XML_END | pNodeInfo->dwType, 0);
        if (_pFactory != NULL) 
            return _pFactory->BeginChildren(pSource, pNodeInfo);
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE EndChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ BOOL fEmptyNode,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo)
    {
        TraceTag((tagEncode, "EndChildren %s %d \"%.*S\"", nodeTypeDebug(pNodeInfo->dwType), pNodeInfo->ulLen, pNodeInfo->ulLen, pNodeInfo->pwcText));
#if DBG == 1
        _uNodes++;
        if (_uNodes > 1000)
            EnableTag(tagEncode, FALSE);
#endif
        TraceTag((tagEncode, "%d XML_END", _uNodes));
        writeType(XML_END | pNodeInfo->dwType | (fEmptyNode ? XML_EMPTY : 0), 0);
        if (_pFactory != NULL) 
            return _pFactory->EndChildren(pSource, fEmptyNode, pNodeInfo);
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Error( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ HRESULT hrErrorCode,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
    {
        if (_pFactory != NULL) 
            return _pFactory->Error(pSource, hrErrorCode, cNumRecs, apNodeInfo);
        return S_OK;
    }


    virtual HRESULT STDMETHODCALLTYPE CreateNode( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
    {
        TraceTag((tagEncode, "CreateNode %d records", cNumRecs));
        while (cNumRecs--)
        {
            XML_NODE_INFO * pNodeInfo = *apNodeInfo++;
            DWORD dwType = pNodeInfo->dwType;
            const WCHAR * pwcText = pNodeInfo->pwcText;
            ULONG ulLen = pNodeInfo->ulLen;
            DWORD dwSubType = pNodeInfo->dwSubType;
    #if DBG == 1
            _uNodes++;
            if (_uNodes > 1000)
                EnableTag(tagEncode, FALSE);
            dwTypes[dwType]++;
    #endif
            TraceTag((tagEncode, "%d Node %s:%d %d \"%.*S\" [%d]", _uNodes, nodeTypeDebug(dwType), pNodeInfo->dwSubType, ulLen, ulLen, pwcText, pNodeInfo->fTerminal));
            switch (dwType)
            {
            case XML_ATTRIBUTE:
                writeType(dwType, dwSubType);
                writeTag(pwcText, ulLen);
                break;

            case XML_ELEMENT:
            case XML_PI:
            case XML_XMLDECL:
            case XML_DOCTYPE:
                _dwLastElementType = dwType;
                writeType(dwType, dwSubType);
                writeTag(pwcText, ulLen);
                break;

            case XML_PCDATA:
#if DBG == 1
                dwTextSize += ulLen;
#endif
                if (_dwLastType != XML_ATTRIBUTE)
                    writeType(dwType, dwSubType);
                if (fAtomValues)
                    writeValue(pwcText, ulLen);
                else
                    writeText(pwcText, ulLen);
                break;

            case XML_WHITESPACE:
#if DBG == 1
                dwWhiteSpaceSize += ulLen;
#endif
                if (fPreserveWhiteSpace)
                {
                    Assert(_dwLastType != XML_ATTRIBUTE);
                    writeType(dwType, dwSubType);
                    if (fAtomValues)
                        writeValue(pwcText, ulLen);
                    else
                        writeText(pwcText, ulLen);
                }
                break;

            case XML_DTDSUBSET:
            case XML_CDATA:
            case XML_COMMENT:
#if DBG == 1
                dwTextSize += ulLen;
#endif
                writeType(dwType, dwSubType);
                if (fAtomValues)
                    writeValue(pwcText, ulLen);
                else
                    writeText(pwcText, ulLen);
                break;

            case XML_ENTITYREF:
                writeType(dwType, dwSubType);
                writeTag(pwcText, ulLen);
                break;

            case XML_ENTITYDECL:
            case XML_ELEMENTDECL:
            case XML_ATTLISTDECL:
            case XML_NOTATION:
            case XML_GROUP:
            case XML_INCLUDESECT:
            case XML_IGNORESECT:
            case XML_PEREF:
            case XML_STRING:
            case XML_MODEL:
            case XML_ATTDEF:
            case XML_ATTTYPE:
            case XML_ATTPRESENCE:    
            case XML_NAME:
            case XML_NMTOKEN:
            default:
                Assert( 0 && "Unrecognized node type!");
                break;
            }

            _dwLastType = dwType;
        }
        if (_pFactory != NULL) 
            return _pFactory->CreateNode(pSource, pNodeParent, cNumRecs, apNodeInfo);
        return S_OK;
    }

};

    
BOOL s_afTerminal[] =
{
    FALSE,

    // -------------- Container Nodes -----------------
    FALSE, // XML_ELEMENT = 1,        // <foo ... >
    FALSE, // XML_ATTRIBUTE,      // <foo bar=...> 
    FALSE, // XML_PI,             // <?foo ...?>   
    FALSE, // XML_XMLDECL,        // <?xml version=...
    FALSE, // XML_DOCTYPE,        // <!DOCTYPE          
	FALSE, // XML_DTDATTRIBUTE	= XML_DOCTYPE + 1,
    FALSE, // XML_ENTITYDECL,     // <!ENTITY ...       
    FALSE, // XML_ELEMENTDECL,    // <!ELEMENT ...      
    FALSE, // XML_ATTLISTDECL,    // <!ATTLIST ...        
    FALSE, // XML_NOTATION,       // <!NOTATION ...
    FALSE, // XML_GROUP,          // The ( ... ) grouping in content models.
    FALSE, // XML_INCLUDESECT,    // <![ INCLUDE [... ]]>  

    // -------------- Terminal Nodes -------------------
    TRUE, // XML_PCDATA,  // text inside a node or an attribute.
    TRUE, // XML_CDATA,          // <![CDATA[...]]>   
    TRUE, // XML_IGNORESECT,     // <![ IGNORE [...]]>
    TRUE, // XML_COMMENT,        // <!--' and '-->
    TRUE, // XML_ENTITYREF,      // &foo;
    TRUE, // XML_WHITESPACE,     // white space between elements 
    TRUE, // XML_NAME,           // general NAME token for typed attribute values or DTD declarations
    TRUE, // XML_NMTOKEN,        // general NMTOKEN for typed attribute values or DTD declarations
    TRUE, // XML_STRING,         // general quoted string literal in DTD declarations.
    TRUE, // XML_PEREF,          // %foo;        
	TRUE, // XML_MODEL,			// EMPTY, ANY of MIXED.
    TRUE, // XML_ATTDEF,         // Name of attribute being defined.
	TRUE, // XML_ATTTYPE,
	TRUE, // XML_ATTPRESENCE,

    TRUE, // XML_DTDSUBSET,      // entire DTD subset as a string. 
};

typedef _array<unsigned long> aulong;
typedef _reference<auint> raulong;

class DecodeNodeSource : public _unknown<IXMLNodeSource, &IID_IXMLNodeSource>
{
    _reference<IXMLNodeFactory> _pFactory;
/*
    struct ElementInfo
    {
        bool    _fEmpty;        // set to false when see children
        DWORD   _dwType;        // type
        IUnknown * _pUnkNode;   // node returned by factory
        String * _pTag;         // tag name

        void reset()
        {
            _fEmpty = true;
            _dwType = 0;
            _pUnkNode = NULL;
            _pTag = null;
        }
    };
    ElementInfo _elementinfo;   // info for current element
*/
    struct ElementInfo : public XML_NODE_INFO
    {
        bool    _fEmpty;        // set to false when see children
        String * _pTag;         // tag name
        USHORT  _cNumRecs;      // number of records

        void init()
        {
            memset(this, 0, sizeof(*this));
            dwSize = sizeof(XML_NODE_INFO);
            _fEmpty = true;
        }

        void fixup(INT_PTR offset)
        {
            if (dwType == XML_ELEMENT || dwType == XML_ATTRIBUTE)
                ;
            else if (pwcText)
            {
                pwcText = (const TCHAR *)((INT_PTR)pwcText + offset);
            }
        }
    };

    int    _cbNodeInfos;            // number of infos
    ElementInfo * _pNodeInfos;      // array of node infos
    ElementInfo * _pNodePos;        // point next node in nodeinfos
    ElementInfo ** _apNodeInfos;    // pointer array to node infos
    ElementInfo * _pElementInfo;    // last element info still pending
    bool    _fInAttributes;         // true when processing attributes

    int _iTagIndex;             // points to next tag to be added
    RAString _aStringTags;      // array of tag strings
    int _iValueIndex;           // points to next value to be added
    RAString _aStringValues;      // array of tag strings
    rauint _auns;               // array of namespace lengths
    _reference<IStream> _pIn;   // input
//    DWORD _dwBuffer;            // buffer for input bits
//    ULONG _ulBits;              // number of bits in buffer
    DWORD _dwLastType;          // last type from CreateNode
    unsigned char * _pchBuf;    // input buffer
    unsigned _uBuf;             // size of buffer
    unsigned char * _pchPos;    // position in buffer
    unsigned char * _pchEnd;    // end of data
//    RATCHAR _aTCHAR;            // unicode buffer
//    unsigned _uTCHAR;           // length of data
//    unsigned _unsTCHAR;         // length of namespace
    TCHAR * _pchWBuf;    // input buffer
    unsigned _uWBuf;             // size of buffer
    TCHAR * _pchWPos;    // position in buffer
    TCHAR * _pchWEnd;    // end of data
    RDocument _pDoc;            // document
#if DBG == 1
    unsigned _uNodes;
#endif
    bool _fUnicode;
    DWORD _dwMode;

public:
    DecodeNodeSource(Document * d, IXMLNodeFactory * pOther, IStream * pIn, bool fUnicode)
    {
        _fUnicode = fUnicode;
        _pFactory = pOther;
        _iTagIndex = 0;
        _aStringTags = new (255) AString;
        _iValueIndex = 0;
        _aStringValues = new (255) AString;
        _auns = new (255) auint;
        _pIn = pIn;
	_dwMode = 0;
//        _dwBuffer = 0;
//        _ulBits = 0;
        _pchBuf = _pchPos = _pchEnd = null;
//        _aTCHAR = null;
//        _uTCHAR = 0;
        _pDoc = d;
#if DBG == 1
        _uNodes = 0;
        EnableTag(tagDecode, TRUE);
        SetDiskFlag(tagDecode, TRUE);
#endif
        _cbNodeInfos = 1024;
        _pNodeInfos = new ElementInfo[_cbNodeInfos];
        _apNodeInfos = new ElementInfo * [_cbNodeInfos];
        ElementInfo * pNodeInfo = _pNodeInfos;
        ElementInfo ** ppInfo = _apNodeInfos;
        for (int i = 0; i < _cbNodeInfos; i++)
        {
            *ppInfo++ = pNodeInfo++;
        }
        _pNodePos = _pNodeInfos;
    }

    ~DecodeNodeSource()
    {
        delete [] _pchBuf;
        delete [] _pchWBuf;
        delete [] _pNodeInfos;
        delete [] _apNodeInfos;
    }

    void push()
    {
        TraceTag((tagDecode, " %s * pushed %.*S", nodeTypeDebug(_pNodePos->dwType), _pNodePos->ulLen, _pNodePos->pwcText));
        _pNodePos++;
        if (_pNodePos - _pNodeInfos == _cbNodeInfos)
        {
            // bugbug put resize logic here
        }
        _pNodePos->init();
    }

    void pop(int cbNodes)
    {
        Assert(_pNodePos - _pNodeInfos >= cbNodes);
        while (cbNodes)
        {
            _pNodePos--;
            TraceTag((tagDecode, " %s * popped %.*S", nodeTypeDebug(_pNodePos->dwType), _pNodePos->ulLen, _pNodePos->pwcText));
            cbNodes--;
        }
    }

    void fixupInfos(ElementInfo * pNodeInfo, INT_PTR offset)
    {
        while (pNodeInfo < _pNodePos)
        {
            pNodeInfo->fixup(offset);
            pNodeInfo++;
        }
    }

    void addTag(const TCHAR * pch, ULONG ulLen, ULONG ulnsLen)
    {
        (*_aStringTags)[_iTagIndex] = String::newString(pch, 0, ulLen);
        (*_auns)[_iTagIndex] = ulnsLen;
        TraceTag((tagDecode, "Added tag \"%s\" at %d", (char *)AsciiText((*_aStringTags)[_iTagIndex]), _iTagIndex));
        if (++_iTagIndex > 254)
            _iTagIndex = 0;
    }

    void addValue(const TCHAR * pch, ULONG ulLen)
    {
        (*_aStringValues)[_iValueIndex] = String::newString(pch, 0, ulLen);
        TraceTag((tagDecode, "Added Value \"%s\" at %d", (char *)AsciiText((*_aStringValues)[_iValueIndex]), _iValueIndex));
        if (++_iValueIndex > 254)
            _iValueIndex = 0;
    }

    /*
    DWORD readBits(ULONG ulLen)
    {
#if DBG == 1
        ULONG ulCached = ulLen;
#endif
        Assert((_ulBits & 1) == 0);
        ULONG ul, ulBits;
        DWORD dw;

        Assert(ulLen < sizeof(_dwBuffer) * 8);
        ulBits = 0;
        dw = 0;
        while(ulLen)
        {
            if (_ulBits == 0)
            {
                DWORD dwRead;

                HRESULT hr = _pIn->Read(&_dwBuffer, sizeof(_dwBuffer), &dwRead);
                if (hr || dwRead == 0)
                    Exception::throwE((HRESULT)E_FAIL);
                _ulBits = dwRead * 8;
            }
            ul = _ulBits;
            if (ul > ulLen)
                ul = ulLen;
            ulLen -= ul;
            dw |= (_dwBuffer << ulBits);
            _ulBits -= ul;
            _dwBuffer >>= ul;
            ulBits += ul;
            // mask out bits on top
            ul = (sizeof(DWORD) * 8) - ulBits;
            dw = ((dw << ul) >> ul);
        }
        TraceTag((tagDecode, "Read %d bits [%d]", ulCached, dw));
        return dw;
    }
    */

    void ensureBufferSize(unsigned uLen)
    {
        if (uLen > _uBuf)
        {
            unsigned char * p = new unsigned char [uLen];
            memcpy(p, _pchBuf, (ULONG)(_pchEnd - _pchBuf));
            fixupInfos(_pNodeInfos, (INT_PTR)p - (INT_PTR)_pchBuf);
            _pchEnd = p + (_pchEnd - _pchBuf);
            _pchPos = p + (_pchPos - _pchBuf);
            _pchBuf = p;
            _uBuf = uLen;
//            _aTCHAR = new (uLen + 1) ATCHAR;
        }
        ensureSize(uLen);
    }

    void ensureData(unsigned uLen);

    inline void ensureSize(unsigned uLen)
    {
        Assert(uLen <= _uBuf);
        if (_pchPos + uLen > _pchEnd)
        {
            ensureData(uLen);
        }
    }

    inline unsigned readByte()
    {
        ensureSize(1);
        Assert(sizeof(BYTE) == 1);
        BYTE b = *(BYTE *)_pchPos;
        _pchPos += 1;
//        TraceTag((tagDecode, "readByte %d", (unsigned)b)); 
        return b;
    }

    inline unsigned readWord()
    {
        ensureSize(2);
        Assert(sizeof(WORD) == 2);
        WORD w = *(WORD *)_pchPos;
        _pchPos += 2;
//        TraceTag((tagDecode, "readWord %d", (unsigned)w)); 
        return w;
    }

    inline int readType(ElementInfo * pNodeInfo)
    {
        int iType = readByte(); // readBits(2);
        TraceTag((tagDecode, "readType %s (%x)", nodeTypeDebug((DWORD)(iType & XML_MASK)), iType)); 
        _pNodePos->dwType = iType & XML_MASK;
        if (iType & XML_SUBTYPE)
        {
            int iSubType = readByte(); // readBits(8);
            TraceTag((tagDecode, "readSubType %d", iSubType)); 
            _pNodePos->dwSubType = iSubType;
        }
        return iType;
    }

    void readTag(ElementInfo * pNodeInfo)
    {
        int iTag = readByte(); // readBits(8);
        int iIndex;
        if (iTag == 0)
        {
            // new tag
            readText(pNodeInfo, TRUE);
            addTag(pNodeInfo->pwcText, pNodeInfo->ulLen, pNodeInfo->ulNsPrefixLen);
            iTag = 1;
        }
        iIndex = _iTagIndex - iTag;
        if (iIndex < 0)
            iIndex += 255;
        TraceTag((tagDecode, "Read tag \"%s\"", (char *)AsciiText((*_aStringTags)[iIndex])));
        pNodeInfo->_pTag = (*_aStringTags)[iIndex];
        pNodeInfo->pwcText = pNodeInfo->_pTag->getData();
        pNodeInfo->ulLen = pNodeInfo->_pTag->length();
        pNodeInfo->ulNsPrefixLen = (*_auns)[iIndex];
    }

    void readValue(ElementInfo * pNodeInfo)
    {
        int iValue = readByte(); // readBits(8);
        int iIndex;
        if (iValue == 0)
        {
            // new Value
            readText(pNodeInfo, FALSE);
            addValue(pNodeInfo->pwcText, pNodeInfo->ulLen);
            iValue = 1;
        }
        iIndex = _iValueIndex - iValue;
        if (iIndex < 0)
            iIndex += 255;
        TraceTag((tagDecode, "Read Value \"%s\"", (char *)AsciiText((*_aStringValues)[iIndex])));
        String * pS = (*_aStringValues)[iIndex];
        pNodeInfo->pwcText = pS->getData();
        pNodeInfo->ulLen = pS->length();
        pNodeInfo->ulNsPrefixLen = 0;
    }

    void readText(ElementInfo * pNodeInfo, BOOL fTag)
    {
        DWORD dw;

        unsigned uchar = readByte(); // readBits(8);
        if (uchar == 255)
        {
            uchar = readWord(); // readBits(16);
        }
        if (uchar)
        {
#if 1
            if (_fUnicode)
            {
                ensureBufferSize(uchar * sizeof(TCHAR));
    #if DBG == 1
                for (unsigned i = 0; i < uchar; i++)
                    TraceTag((tagDecode, "char %d", (unsigned)((TCHAR *)_pchPos)[i])); 
    #endif
                if (fTag)
                {
                    TCHAR c = ((TCHAR *)_pchPos)[uchar];
                    ((TCHAR *)_pchPos)[uchar] = 0;
                    TCHAR * p = _tcsrchr((TCHAR *)_pchPos, _T(':'));
                    if (p)
                        pNodeInfo->ulNsPrefixLen = (ULONG)(p - (TCHAR *)_pchPos);
                    ((TCHAR *)_pchPos)[uchar] = c;
                }
                pNodeInfo->pwcText = (const TCHAR *)_pchPos;
                pNodeInfo->ulLen = uchar;
                _pchPos += uchar * sizeof(TCHAR);
                TraceTag((tagDecode, "readText %d \"%.*S\"", uchar, pNodeInfo->ulLen, pNodeInfo->pwcText));
            }
            else
            {
                ensureBufferSize(uchar);
    #if DBG == 1
                for (unsigned i = 0; i < uchar; i++)
                    TraceTag((tagDecode, "char %d", (unsigned)(_pchPos)[i])); 
    #endif
                // BUGBUG we assume here that the unicode buffer is always big enough...
                unsigned utchar = uchar;
                checkhr(CharEncoder::wideCharFromUtf8(&_dwMode, CP_UNDEFINED, (BYTE *)_pchPos, &uchar, 
                    _pchWPos, &utchar));
                if (fTag)
                {
                    TCHAR c = (_pchWPos)[utchar];
                    (_pchWPos)[uchar] = 0;
                    TCHAR * p = _tcsrchr(_pchWPos, _T(':'));
                    if (p)
                        pNodeInfo->ulNsPrefixLen = (ULONG)(p - _pchWPos);
                    (_pchWPos)[uchar] = c;
                }
                pNodeInfo->pwcText = _pchWPos;
                pNodeInfo->ulLen = utchar;
                _pchPos += uchar;
                _pchWPos += utchar;
                TraceTag((tagDecode, "readText %d \"%.*S\"", uchar, pNodeInfo->ulLen, pNodeInfo->pwcText));
            }
#else
            TCHAR * ptch = (TCHAR *)_aTCHAR->getData();
            _uTCHAR = 0;
            _unsTCHAR = 0;
            // UTF-8 multi-byte encoding.  See Appendix A.2 of the Unicode book for more info.
            //
            // Unicode value    1st byte    2nd byte    3rd byte
            // 000000000xxxxxxx 0xxxxxxx
            // 00000yyyyyxxxxxx 110yyyyy    10xxxxxx
            // zzzzyyyyyyxxxxxx 1110zzzz    10yyyyyy    10xxxxxx

            WCHAR c;
            while (uchar)
            {
                // This is an optimization for straight runs of 7-bit ascii 
                // inside the UTF-8 data.
                c = *_pchPos;
                if (c & 0x80)   // check 8th-bit and get out of here
                    break;      // so we can do proper UTF-8 decoding.
                if (fTag && c == _T(':'))
                    _unsTCHAR = ptch - _aTCHAR->getData();
                *ptch++ = c;
                _pchPos++;
                _uTCHAR++;
                uchar--;
            }

            if (uchar)
            {
                bool valid = true;

                while (uchar)
                {
                    UINT bytes = 0;
                    for (c = *_pchPos; c & 0x80; c <<= 1)
                        bytes++;

                    if (bytes == 0) 
                        bytes = 1;

                    if (uchar < bytes)
                    {
                        break;
                    }
         
                    c = 0;
                    switch ( bytes )
                    {
                        case 6: _pchPos++;                  // we don't handle
                        case 5: _pchPos++;                 // UCS-4.
                        case 4: _pchPos++;                
                                valid = false;
                                // fall through
                        case 3: c  = WCHAR(*_pchPos++ & 0x0f) << 12;    // 0x0800 - 0xffff
                                if ((*_pchPos & 0xc0) != 0x80)
                                    valid = false;
                                // fall through
                        case 2: c |= WCHAR(*_pchPos++ & 0x3f) << 6;     // 0x0080 - 0x07ff
                                if ((*_pchPos & 0xc0) != 0x80)
                                    valid = false;
                                c |= WCHAR(*_pchPos++ & 0x3f);
                                break;
                    
                        case 1:
                            c = TCHAR(*_pchPos++);                      // 0x0000 - 0x007f
                            break;

                        default:
                            valid = false; // not a valid UTF-8 character.
                            break;
                    }

                    // If the multibyte sequence was illegal, store a FFFF character code.
                    // The Unicode spec says this value may be used as a signal like this.
                    // This will be detected later by the parser and an error generated.
                    // We don't throw an exception here because the parser would not yet know
                    // the line and character where the error occurred and couldn't produce a
                    // detailed error message.

                    if (! valid)
                    {
                        c = 0xffff;
                        valid = true;
                    }

                    if (fTag && c == _T(':'))
                        _unsTCHAR = ptch - _aTCHAR->getData();
                    *ptch++ = c;
                    _uTCHAR++;
                    uchar -= bytes;
                }
            }
#endif
#if DBG == 1
//            (*_aTCHAR)[_uTCHAR] = 0;
#endif
        }
        else
        {
//            _uTCHAR = 0;
#if DBG == 1
//            (*_aTCHAR)[_uTCHAR] = 0;
#endif
        }
//        TraceTag((tagDecode, "readText %d \"%S\"", _uTCHAR, (TCHAR *)_aTCHAR->getData()));
    }

    HRESULT decode();

    virtual HRESULT STDMETHODCALLTYPE SetFactory( 
        /* [in] */ IXMLNodeFactory __RPC_FAR *pNodeFactory)
    {
        return E_NOTIMPL;
    }
    
    virtual HRESULT STDMETHODCALLTYPE GetFactory( 
        /* [out] */ IXMLNodeFactory __RPC_FAR *__RPC_FAR *ppNodeFactory)
    {
        *ppNodeFactory = _pFactory;
        _pFactory->AddRef();
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE Abort( 
        /* [in] */ BSTR bstrErrorInfo)
    {
        return E_NOTIMPL;
    }
    
    virtual ULONG STDMETHODCALLTYPE GetLineNumber( void)
    {
        return 0;
    }
    
    virtual ULONG STDMETHODCALLTYPE GetLinePosition( void)
    {
        return 0;
    }
    
    virtual ULONG STDMETHODCALLTYPE GetAbsolutePosition( void)
    {
        return 0;
    }
    
    virtual HRESULT STDMETHODCALLTYPE GetLineBuffer( 
        /* [out] */ const WCHAR __RPC_FAR *__RPC_FAR *ppwcBuf,
        /* [out] */ ULONG __RPC_FAR *pulLen,
        /* [out] */ ULONG __RPC_FAR *pulStartPos)
    {
        return E_NOTIMPL;
    }
    
    virtual HRESULT STDMETHODCALLTYPE GetLastError( void)
    {
        return E_NOTIMPL;
    }
    
    virtual HRESULT STDMETHODCALLTYPE GetErrorInfo( 
        /* [out] */ BSTR __RPC_FAR *pbstrErrorInfo)
    {
        return E_NOTIMPL;
    }
    
    virtual ULONG STDMETHODCALLTYPE GetFlags( void)
    {
        return 0;
    }
    
    virtual HRESULT STDMETHODCALLTYPE GetURL( 
        /* [out] */ const WCHAR __RPC_FAR *__RPC_FAR *ppwcBuf)
    {
        return E_NOTIMPL;
    }
};    


void DecodeNodeSource::ensureData(unsigned uLen)
{
    unsigned u = (ULONG)(_pchEnd - _pchPos);
    if (u)
    {
        memmove(_pchBuf, _pchPos, u);
        fixupInfos(_pNodeInfos, (INT_PTR)_pchPos - (INT_PTR)_pchBuf);
    }
    _pchEnd = _pchBuf + u;
    _pchPos = _pchBuf;
    DWORD dwRead;
    HRESULT hr = _pIn->Read(_pchEnd, _uBuf - u, &dwRead);
    if (hr || dwRead == 0)
        Exception::throwE((HRESULT)E_FAIL);
    _pchEnd += dwRead;
    if (_pchEnd - _pchBuf < (int)uLen)
        Exception::throwE((HRESULT)E_FAIL);
}


HRESULT DecodeNodeSource::decode()
{
    HRESULT hr = S_OK;
    IUnknown * pUnkNode = null;

    TRY
    {
        _uBuf = 320 * 1024;
        _pchBuf = new unsigned char [_uBuf];
        _pchPos = _pchEnd = _pchBuf;
        _uWBuf = 320 * 1024;
        _pchWBuf = new TCHAR [_uBuf];
        _pchWPos = _pchWEnd = _pchWBuf;
//        _aTCHAR = new (_uBuf + 1) ATCHAR;
        checkhr(_pFactory->NotifyEvent(this, XMLNF_STARTDOCUMENT));
        checkhr(_pFactory->NotifyEvent(this, XMLNF_ENDPROLOG));
        Assert(_pNodePos == _pNodeInfos);
        _pNodePos->init();
        checkhr(_pDoc->QueryInterface(IID_Element, &_pNodePos->pNode));
        push();
        _fInAttributes = false;
        for (;;)
        {
            _pNodePos->init();
            DWORD dwType = readType(_pNodePos);
#if DBG == 1
            _uNodes++;
            if (_uNodes > 1000)
                EnableTag(tagDecode, FALSE);
#endif
            if (dwType & XML_END)
            {
                if (_fInAttributes)
                {
                    checkhr(_pFactory->CreateNode(this,
                                (_pElementInfo - 1)->pNode,
                                _pElementInfo->_cNumRecs,
                                (XML_NODE_INFO **)(_apNodeInfos + (_pElementInfo - _pNodeInfos))));
                    if (dwType & XML_EMPTY)
                    {
                        pop(1);
                        checkhr(_pFactory->EndChildren(this,
                            _pElementInfo->_fEmpty,
                            _pElementInfo));
                    }
                    else if (_pElementInfo->dwType == XML_ELEMENT) 
                        checkhr(_pFactory->BeginChildren(this,
                            _pElementInfo));
                    else
                    {
                        pop(1);
                        checkhr(_pFactory->EndChildren(this,
                            _pElementInfo->_fEmpty, 
                            _pElementInfo));
                    }
                    pop(_pElementInfo->_cNumRecs - 1);
                    _fInAttributes = false;
                    _pElementInfo = null;
                }
                else
                {
                    pop(1);
                    if (_pNodePos->dwType == XML_ELEMENT)
                    {
                        checkhr(_pFactory->EndChildren(this,
                            _pNodePos->_fEmpty, 
                            _pNodePos));
                    }
                }
                if (_pNodePos == _pNodeInfos)
                {
                    // final end, stop processing
                    goto Cleanup;
                }
            }
            else
            {
                switch(dwType & XML_MASK)
                {
                case XML_PI:
                case XML_DOCTYPE:
                    _pNodePos->_fEmpty = false;
                    // fall thru
                case XML_XMLDECL:
                case XML_ELEMENT:
                    Assert(_fInAttributes == false);
                    _fInAttributes = true;
                    (_pNodePos - 1)->_fEmpty = false;
                    readTag(_pNodePos);
                    Assert(s_afTerminal[dwType] == FALSE);
                    _pNodePos->fTerminal = FALSE;
                    _pElementInfo = _pNodePos;
                    _pElementInfo->_cNumRecs = 1;
                    push();
                    break;
                case XML_DTDSUBSET:
                case XML_COMMENT:
                case XML_ENTITYREF:
                case XML_CDATA:
                    if (fAtomValues)
                        readValue(_pNodePos);
                    else
                        readText(_pNodePos, FALSE);
                    Assert(s_afTerminal[_pNodePos->dwType] == TRUE);
                    _pNodePos->fTerminal = TRUE;
                    if (_fInAttributes)
                    {
                        _pElementInfo->_cNumRecs++;
                        push();
                    }
                    else
                        checkhr(_pFactory->CreateNode(this,
                                    (_pNodePos - 1)->pNode,
                                    1,
                                    (XML_NODE_INFO **)(_apNodeInfos + (_pNodePos - _pNodeInfos))));
                    break;
                case XML_ATTRIBUTE:
                    if (_pNodePos->dwSubType == XML_NS)
                    {
                        // ugly hack to tell nodefactory that there is an ns attribute !
                        _pElementInfo->pReserved = (void *)0x1;
                    }
                    readTag(_pNodePos);
                    Assert(s_afTerminal[_pNodePos->dwType] == FALSE);
                    _pNodePos->fTerminal = FALSE;
    #if DBG == 1
                    _uNodes++;
    #endif
                    push();
                    _pElementInfo->_cNumRecs++;
                    TraceTag((tagDecode, "%d PCDATA", _uNodes));
                    // fall thru
                    _pNodePos->dwType = XML_PCDATA;
                case XML_PCDATA:
                case XML_WHITESPACE:
                    Assert(s_afTerminal[_pNodePos->dwType] == TRUE);
                    _pNodePos->fTerminal = TRUE;
                    if (fAtomValues)
                        readValue(_pNodePos);
                    else
                        readText(_pNodePos, FALSE);
                    if (_fInAttributes)
                    {
                        push();
                        _pElementInfo->_cNumRecs++;
                    }
                    else
                    {
                        (_pNodePos - 1)->_fEmpty = false;
                        checkhr(_pFactory->CreateNode(this,
                                    (_pNodePos - 1)->pNode,
                                    1,
                                    (XML_NODE_INFO **)(_apNodeInfos + (_pNodePos - _pNodeInfos))));
                    }
                    break;
                default:
                    Assert(0 && "Unknown encoding type");
                    Exception::throwE(E_FAIL);
                    }
            }
        }
    }
    CATCHE
    {
        hr = ERESULT;
        if (hr == S_FALSE)  // end of stream...
            hr = S_OK;
    }
    ENDTRY

Cleanup:

    checkhr(_pFactory->NotifyEvent(this, XMLNF_ENDDOCUMENT));

    return hr;
}


class EmptyStream : public _unknown<IStream, &IID_IStream>
{
public:
    EmptyStream() {}

    virtual HRESULT STDMETHODCALLTYPE Read(void * pv, ULONG cb, ULONG * pcbRead)
    {
        return E_NOTIMPL;
    } 

    virtual HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG * pcbWritten)
    {
        if (pcbWritten)
            *pcbWritten = cb;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER * plibNewPosition)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream * pstm, ULARGE_INTEGER cb, ULARGE_INTEGER * pcbRead, ULARGE_INTEGER * pcbWritten)
    {
        return E_NOTIMPL;
    } 
 
    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags)
    {
        return E_NOTIMPL;
    }
    
    virtual HRESULT STDMETHODCALLTYPE Revert(void)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE LockRegion( ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG * pstatstg, DWORD grfStatFlag)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Clone(IStream ** ppstm)
    {
        return E_NOTIMPL;
    }
};


//
// This is a stream to test async behavior.  
//  stm is a real stream
//  c is the maximum number of characters to return before an E_PENDING
//
 
class AsyncStream : public _unknown<IStream, &IID_IStream>
{
public:
    AsyncStream(IStream * stm, int c) {_stm = stm; _c = c; _hr = S_OK;}

    virtual HRESULT STDMETHODCALLTYPE Read(void * pv, ULONG cb, ULONG * pcbRead)
    {
        HRESULT hr;

        if (_hr != E_PENDING)
        {
            if (cb > _c)
            {
                cb = _c;
                _hr = E_PENDING;
            }

            hr = _stm->Read(pv, cb, pcbRead);
        }
        else
        {
            hr = _hr;
            _hr = S_OK;
            *pcbRead = 0;
        }
        return hr;
    } 

    virtual HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG * pcbWritten)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER * plibNewPosition)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream * pstm, ULARGE_INTEGER cb, ULARGE_INTEGER * pcbRead, ULARGE_INTEGER * pcbWritten)
    {
        return E_NOTIMPL;
    } 
 
    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags)
    {
        return E_NOTIMPL;
    }
    
    virtual HRESULT STDMETHODCALLTYPE Revert(void)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE LockRegion( ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG * pstatstg, DWORD grfStatFlag)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Clone(IStream ** ppstm)
    {
        return E_NOTIMPL;
    }

private:
    _reference<IStream>     _stm;
    ULONG                   _c;
    HRESULT                 _hr;
};


char s_c;

//-------------------------------------------------------------------

typedef DWORD (WINAPI *PFNTHREADPROC)(void *);
typedef int (*PFNELEMENTWALK)(Element *);
typedef int (*PFNNODEWALK)(IXMLDOMNode *);

class ThreadData
{
public:

    ThreadData()
    {
        lTime = 0;
    }

    long lTime;
};

class ElementThreadData : public ThreadData
{
public:
    ElementThreadData(String * url, PFNELEMENTWALK pfn, int n)
    {
        bUrl = url;
        pfnElementWalk = pfn;
        loops = n;
//        LoadElementDoc(bUrl, &ie4doc);
    }

    static void LoadElementDoc(WCHAR * url, IXMLDocument2 ** pp)
    {
        IXMLDocument2 * ie4doc;

        Document * d = new Document();

        checkhr(d->QueryInterface(IID_IXMLDocument2, (void **) &ie4doc));

        checkhr(ie4doc->put_URL(url));
        *pp = ie4doc;
    }

    bstr        bUrl;
    PFNELEMENTWALK   pfnElementWalk;
    static _reference<IXMLDocument2> ie4doc;
    int loops;
    int cElements;
};

class NodeThreadData : public ThreadData
{
public:
    NodeThreadData(String * url, PFNNODEWALK pfn, int n)
    {
        bUrl = url;
        pfnNodeWalk = pfn;
        loops = n;
//        LoadNodeDoc(bUrl, &domdoc);
    }

    static void LoadNodeDoc(WCHAR *url, IXMLDOMDocument ** pp)
    {
        IXMLDOMDocument * domdoc = null;
        VARIANT_BOOL varSucceeded;
        VARIANT vURL;

        Document * d = new Document();
        d->setDOM(true);
        checkhr(d->QueryInterface(IID_IXMLDOMDocument, (void **) &domdoc));

        vURL.vt = VT_BSTR;
        V_BSTR(&vURL) = url;
        checkhr(domdoc->load(vURL, &varSucceeded));
        *pp = domdoc;
    }

    bstr        bUrl;
    PFNNODEWALK     pfnNodeWalk;
    static _reference<IXMLDOMDocument> domdoc;
    int loops;
    int cNodes;
};


int WalkElements(Element * e)
{
    int n = 0;
    HANDLE  h;
    Element * c;

    if (e != null)
    {
        n++;

        c = e->getFirstChild(&h);
        while (c)
        {
            n += WalkElements(c);
            c = e->getNextChild(&h);
        }
    }

    return n;
}


int WalkNodes(IXMLDOMNode * inode)
{
#if 1
    int n = 1;

    IXMLDOMNode * child;
    IXMLDOMNode * next;

    HRESULT hr = inode->get_firstChild(&child);
    while (child != NULL && hr == S_OK)
    {
        n += WalkNodes(child);
        hr = child->get_nextSibling(&next);
        child->Release();
        child = next;
    }

    if (child)
        child->Release();
#else
    HRESULT hr;
    _reference<IXMLDOMNodeList> inodelist;
    _reference<IXMLDOMNode> inodeNext;
    int n = 0;
    long l = 0;

    if (inode != null)
    {
        n++;
        hr = inode->get_childNodes(&inodelist);
        if (hr != 0 || inodelist == null)
            goto Cleanup;

        while ((hr = inodelist->get_item(l, &inodeNext)) == S_OK)
        {
            n += WalkNodes(inodeNext);
            inodeNext = null;
            l++;
        }
    }
#endif

Cleanup:
    return n;
}


int WalkElementNames(Element * e)
{
    int n = 0;
    HANDLE h;
    Element * c;

    if (e != null)
    {
        n++;
        NameDef * nm = e->getNameDef();
        if (nm != null)
        {
            c = e->getFirstChild(&h);
            while (c)
            {
                n += WalkElementNames(c);
                c = e->getNextChild(&h);
            }
        }
    }

    return n;
}


int WalkNodeNames(IXMLDOMNode * inode)
{
    HRESULT hr;
    _reference<IXMLDOMNodeList> inodelist;
    _reference<IXMLDOMNode> inodeNext;
    int n = 0;
    long l = 0;

    if (inode != null)
    {
        n++;
        bstr b;

        hr = inode->get_nodeName(&b);
        if (hr != 0 && b != null)
            goto Cleanup;
        
        hr = inode->get_childNodes(&inodelist);
        if (hr != 0 || inodelist == null)
            goto Cleanup;

        if (b != null)
        {
            while ((hr = inodelist->get_item(l, &inodeNext)) == S_OK)
            {
                n += WalkNodeNames(inodeNext);
                inodeNext = null;
                l++;
            }   
        }
    }

Cleanup:
    return n;
}


DWORD WINAPI TestElements(void * pv)
{
    STACK_ENTRY;
    ElementThreadData * etd = (ElementThreadData *) pv;
    _reference<IXMLElement2> ixmlElement;
    Element * root;
    __int64 start, end, freq;

    QueryPerformanceCounter((LARGE_INTEGER *)&start);
#if PRODUCT_PROF
    AllowCAP();
    StartCAP();
#endif

    TRY
    {
        checkhr(etd->ie4doc->get_root(&ixmlElement));
        checkhr(ixmlElement->QueryInterface(IID_Element, (void **) &root));

        for (int i = 0; i < etd->loops; i++)
            etd->cElements = (*etd->pfnElementWalk)(root);
    }
    CATCH
    {
        Exception * e = GETEXCEPTION();
        StdIO::getOut()->println(e->toString());
    }
    ENDTRY

#if PRODUCT_PROF
    StopCAP();
#endif
    QueryPerformanceCounter((LARGE_INTEGER *)&end);
    QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
    etd->lTime = (long)((end - start) * 1000 / freq);

    return 0;
}


DWORD WINAPI TestNodes(void * pv)
{
    STACK_ENTRY;
    NodeThreadData * ntd = (NodeThreadData *) pv;
    _reference<IXMLDOMElement> inode;
    _reference<IPersistStreamInit> ipstream;
    _reference<IStream> istream;
    __int64 start, end, freq;

    QueryPerformanceCounter((LARGE_INTEGER *)&start);
#if PRODUCT_PROF
    AllowCAP();
    StartCAP();
#endif
    TRY
    {
        if (fCreateTest)
        {
            ntd->cNodes = 0;
            Document * d = new Document();
            TCHAR buf[4];
            buf[0] = _T('T');
            buf[1] = _T('A');
            buf[2] = _T('G');
            buf[3] = _T('0');
            NameDef * pName = null;
            NameSpace * ns = null;
            bool scoping;
            NamespaceMgr * pNamespaceMgr = d->getNamespaceMgr();
            pName = pNamespaceMgr->createNameDef(buf, 3, 0, false);
            Node * pRoot = Node::newNode( Node::ELEMENT, pName, d, d->getNodeMgr());
            ntd->cNodes++;
            ((Node *)d->getDocElem())->_insert(pRoot, null);
            for (int i = 0; i < ntd->loops; i++)
            {
                buf[3] = _T('0') + (i % 10);
                pName = pNamespaceMgr->createNameDef(buf, 4, 0, false);
                Node * pNode = Node::newNode( Node::ELEMENT, pName, d, d->getNodeMgr());
                ntd->cNodes++;
                pRoot->_insert(pNode, null);
                for (int j = _T('a'); j <= _T('z'); j++)
                {
                    buf[3] = (TCHAR)j;
                    pName = pNamespaceMgr->createNameDef(buf, 4, 0, false);
                    Node * pChild = Node::newNode( Node::ELEMENT, pName, d, d->getNodeMgr());
                    ntd->cNodes++;
                    pNode->_insert(pChild, null);
                    Node * pText = Node::newNode( Node::PCDATA, null, d, d->getNodeMgr());
                    ntd->cNodes++;
                    pText->setInnerText(_T("TEXT"), 4, false);
                    pChild->_insert(pText, null);

                }
            }
            if (fOutput)
            {
                _reference<IStream> pIStream;
                FileOutputStream* out = FileOutputStream::newFileOutputStream(String::newString(_T("test.xml")));
                out->getIStream(&pIStream);
                OutputHelper* hout = d->createOutput(pIStream); // BUGBUG ?, encoding);
                d->save(hout);
                hout->close();
            }
        }
        else if (fStreamTest)
        {
            checkhr(ntd->domdoc->QueryInterface(IID_IPersistStreamInit, (void **)&ipstream));
            EmptyStream * p = new EmptyStream();
            checkhr(p->QueryInterface(IID_IStream, (void **)&istream));
            p->Release();
            if (!istream)
                Exception::throwEOutOfMemory();
            checkhr(ipstream->Save(istream, FALSE));
        }
        else
        {
            checkhr(ntd->domdoc->get_documentElement(&inode));
            for (int i = 0; i < ntd->loops; i++)
                ntd->cNodes = (*ntd->pfnNodeWalk)(inode);
        }
    }
    CATCH
    {
        Exception * e = GETEXCEPTION();
        StdIO::getOut()->println(e->toString());
    }
    ENDTRY

#if PRODUCT_PROF
    StopCAP();
#endif
    QueryPerformanceCounter((LARGE_INTEGER *)&end);
    QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
    ntd->lTime = (long)((end - start) * 1000 / freq);

    return 0;
}


class TestApp : public Base
{
public:
    DECLARE_CLASS_MEMBERS(TestApp, Base);

    TestApp()
    {
        init();
    }


    void MultiThreadTest(int n, String * msg, PFNTHREADPROC pfntest, void ** ppv)
    {
        DWORD   * pid;
        HANDLE  * phandle;
        int i;

        TRY
        {
            pid = new DWORD [n];
            phandle = new HANDLE [n];

            StdIO::getOut()->print(msg);
            GarbageCollect();

            Start();

            for (i = 0; i < n; i++)
            {
                phandle[i] = CreateThread(NULL, 0, pfntest, ppv[i], 0, pid + i);
            }

            WaitForMultipleObjects(n, phandle, TRUE, INFINITE);

            Stop();

            for (i = 0; i < n; i++)
            {
                CloseHandle(phandle[i]);
                sum1 += ((ThreadData *)ppv[i])->lTime;
            }

            PrintStream* out1 = StdIO::getOut();

            out1->print(_T(" time1 = "));
            out1->print(sum1/n );


#ifdef WORKINGSET
            if (dumpws)
            {
                DumpWorkingSet(true);
                GarbageCollect();
                DumpWorkingSet(true);
            }

#endif
        }
        CATCH
        {
            Exception * e = GETEXCEPTION();
            StdIO::getOut()->println(e->toString());
        }
        ENDTRY

        delete [] pid;
        delete [] phandle;
    }

    void ElementMThreadTest(String * s)
    {
        TRY
        {
            ElementThreadData ** ppv = new ElementThreadData * [threads];
            for (int i = 0; i < threads; i++)
                ppv[i] = new ElementThreadData(s, WalkElements, loops);
            bstr        bUrl = s;
            ElementThreadData::LoadElementDoc(bUrl, &ElementThreadData::ie4doc);

            MultiThreadTest(threads, String::newString(_T("WalkElements")), TestElements, (void **)ppv);
            StdIO::getOut()->print(String::newString(_T("Total Elements = ")));
            StdIO::getOut()->println(((ElementThreadData *)ppv[0])->cElements);
            /*
            for (i = 0; i < threads; i++)
                ((ElementThreadData *)ppv[i])->pfnElementWalk = WalkElementNames;
            MultiThreadTest(threads, String::newString(_T("WalkElementNames")), TestElements, (void **)ppv);
            */
            ElementThreadData::ie4doc = null;
            for (i = 0; i < threads; i++)
                delete ppv[i];
            delete ppv;
        }
        CATCH
        {
            Exception * e = GETEXCEPTION();
            StdIO::getOut()->println(e->toString());
        }
        ENDTRY

    }

    void NodeMThreadTest(String * s)
    {
        TRY
        {
            NodeThreadData ** ppv = new NodeThreadData * [threads];
            for (int i = 0; i < threads; i++)
                ppv[i] = new NodeThreadData(s, WalkNodes, loops);
            bstr        bUrl = s;
            NodeThreadData::LoadNodeDoc(bUrl, &NodeThreadData::domdoc);

            MultiThreadTest(threads, String::newString(_T("WalkNodes")), TestNodes, (void **)ppv);
            StdIO::getOut()->print(String::newString(_T("Total Nodes = ")));
            StdIO::getOut()->println(((NodeThreadData *)ppv[0])->cNodes);
            /*
            for (i = 0; i < threads; i++)
                ((NodeThreadData *)ppv[i])->pfnNodeWalk = WalkNodeNames;
            MultiThreadTest(threads, String::newString(_T("WalkNodeNames")), TestNodes, (void **)ppv);
            */
            NodeThreadData::domdoc = null;
            for (i = 0; i < threads; i++)
                delete ppv[i];
            delete ppv;
        }
        CATCH
        {
            Exception * e = GETEXCEPTION();
            StdIO::getOut()->println(e->toString());
        }
        ENDTRY

    }

    Document* TestLoad(String * url)
    {
        Document* d = new Document(); 

        d->setIe4Compatibility(ie4compat);
        d->setShortEndTags(shortEndTags);

        d->setDOM(! ie4compat);

        if (ie4compat)
        {
            // MSXML version 1 compatibility
            d->setCaseInsensitive(true);
            d->setIgnoreDTD(true);
            d->setOmitWhiteSpaceElements(true);
            d->setParseNamespaces(false);
        }
        if (ignoreDTD)
            d->setIgnoreDTD(ignoreDTD);
        if (omitWhitespace)
            d->setOmitWhiteSpaceElements(true);

        if (fEncode)
        {
            IStream * pStream = NULL;
            out->getIStream(&pStream);
            EncodeNodeFactory * pEncodeFac = new EncodeNodeFactory(null, pStream, fUnicode);
            d->setFactory(pEncodeFac);
            pEncodeFac->Release();
            release(&pStream);
        }
        else if (fast)
        {
            TestNodeFactory* factory = new TestNodeFactory(null);
            d->setFactory(factory);
            factory->Release();
        }

        IStream * instream = null;
      
        if (fDecode)
        {
            TRY 
            {
#if 1
                CreateStreamOnFile(url->toCharArrayZ()->getData(), 0, &instream);
#else
                RIMoniker moniker;
                _reference<IBindCtx> pbc;

                checkhr(CreateURLMoniker(NULL, url->toCharArrayZ()->getData(), (IMoniker **)&moniker));
                checkhr(CreateBindCtx(0, (IBindCtx **)&pbc));
                checkhr(moniker->BindToStorage(pbc, NULL, IID_IStream, (void **)&instream));
#endif
            } 
            CATCH
            {
                StdIO::getOut()->println(Resources::FormatMessage(XML_IOERROR,
                    url->toString(), null));
            }
            ENDTRY
            Start();
#if PRODUCT_PROF
            AllowCAP();
            StartCAP();
#endif
            IXMLParser * pParser = NULL;
            checkhr(d->QueryInterface(IID_IXMLParser, (void **)&pParser));
            IXMLNodeFactory * pFactory = null;
            if (fDecodeEncode)
            {
                IStream * pStream = NULL;
                EmptyStream * p = new EmptyStream();
                checkhr(p->QueryInterface(IID_IStream, (void **)&pStream));
                release(&p);
                pFactory = new EncodeNodeFactory(null, pStream, fUnicode);
                release(&pStream);
            }
            else
            {
                checkhr(pParser->GetFactory(&pFactory));
            }
            DecodeNodeSource * pd  = new DecodeNodeSource(d, pFactory, instream, fUnicode);
            if (pd)
            {
                pd->decode();
            }
            release(&pd);
            release(&pFactory);
            release(&pParser);
        }
        else if (stream) 
        {
            TRY 
            {
#if 1
                CreateStreamOnFile(url->toCharArrayZ()->getData(), STGM_SHARE_DENY_NONE, &instream);
#else
                RIMoniker moniker;
                _reference<IBindCtx> pbc;

                checkhr(CreateURLMoniker(NULL, url->toCharArrayZ()->getData(), (IMoniker **)&moniker));
                checkhr(CreateBindCtx(0, (IBindCtx **)&pbc));
                checkhr(moniker->BindToStorage(pbc, NULL, IID_IStream, (void **)&instream));
#endif
            } 
            CATCH
            {
                StdIO::getOut()->println(Resources::FormatMessage(XML_IOERROR,
                    url->toString(), null));
                Exception::throwAgain();
            }
            ENDTRY
            Start();
#if PRODUCT_PROF
            AllowCAP();
            StartCAP();
#endif
            d->Load(instream);
        } 
        else 
        {
            Start();
#if PRODUCT_PROF
            AllowCAP();
            StartCAP();
#endif
            d->load(url->toString(), asynchronous);
            long fReady = d->getReadyStatus();

            if (asynchronous && fReady != READYSTATE_COMPLETE)
            {
                // Pump the message queue to keep the download happening.
                MSG msg;
                fReady = d->getReadyStatus();

                while (fReady != READYSTATE_COMPLETE && fReady != READYSTATE_UNINITIALIZED)
                {
                    GetMessage(&msg, null, 0, 0);
                    DispatchMessage(&msg);
                    fReady = d->getReadyStatus();
                }

                while (PeekMessage(&msg, null, 0, 0, PM_NOREMOVE))
                {
                    GetMessage(&msg, null, 0, 0);
                    DispatchMessage(&msg);
                }

            }

            if (d->getErrorMsg() != null)
            {
                Exception * e = d->getErrorMsg();
                String * s = null;
                if (e)
                    s = e->getMessage();
                if (s)
                {
                    StdIO::getOut()->println(s);
                }
                else
                {
                    StdIO::getOut()->println(String::newString(_T("Unspecified error.")));  
                }

                d = null;
            }
        }
        release(&instream);
#if PRODUCT_PROF
        StopCAP();
#endif
        Stop();
        return d;
    }

    void TestDOMDocument()
    {
        PrintStream * sout = StdIO::getOut();

        IXMLDOMDocument* pDoc = NULL;
        HRESULT hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
                                        IID_IXMLDOMDocument, (void**)&pDoc);
        if (hr != 0 || pDoc == NULL)
        {
            sout->println(String::newString(_T("Error creating DOM document")));
            return;
        }

        IXMLParser* pParser = NULL;
        hr = pDoc->QueryInterface(IID_IXMLParser, (void **)&pParser);
        if (hr != 0 || pParser == NULL)
        {
            sout->println(String::newString(_T("Error getting IXMLParser from document.")));
            pDoc->Release();
            return;
        }

        IXMLNodeFactory* pFactory = NULL;
        hr = pParser->GetFactory(&pFactory);

        if (hr != 0 || pFactory == NULL)
        {
            sout->println(String::newString(_T("Error getting IXMLNodeFactory from parser.")));
            pParser->Release();
            pDoc->Release();
            return;
        }

        TestNodeFactory* factory = new TestNodeFactory(pFactory);
        pParser->SetFactory(factory);
        factory->Release();

        pFactory->Release();
        static const char* s_TestData = "<?xml version='1.0'?>"
                                        "<root>"
                                            "<item id='123'>This is a test</item>"
                                        "</root>"
                                        "<!-- THE END -->";


#ifdef UNIX
        pParser->PushData((const unsigned char*)s_TestData, strlen(s_TestData), TRUE);
#else
        pParser->PushData(s_TestData, strlen(s_TestData), TRUE);
#endif
        hr = pParser->Run(-1);
        if (hr != 0)
        {
            sout->println(String::newString(_T("Error parsing test string")));
        }

        IXMLDOMNodeList* pList = NULL;
        hr = pDoc->get_childNodes(&pList);

        if (hr != 0 || pList == NULL)
        {
            sout->println(String::newString(_T("Error getting child nodes.")));
        }
        else
        {
            long length = 0;
            hr = pList->get_length(&length);

            sout->print(String::newString(_T("Document has ")));
#ifdef UNIX
            sout->print((int)length);
#else
            sout->print(length);
#endif
            sout->println(String::newString(_T(" nodes.")));

            pList->Release();
        }

        pParser->Release();
        pDoc->Release();
    }

    void GarbageCollect()
    {
        PrintStream* sout = StdIO::getOut();
        if (timeit)
        {
//            sout->print(_T("------- Garbage Collecting..."));
        }
//        Base::testForGC(GC_FORCE | GC_FULL);
        if (timeit)
        {
//            sout->print(_T(" collected="));
        }
    }

    void TestXSL(String * urlXSL, String *urlDoc)        
    {
        Document * dXSL = new Document();
        Document * dXML = new Document();
        _reference<IXMLDOMDocument> docXSL;
        _reference<IXMLDOMElement> rootXSL;
        _reference<IXMLDOMDocument> docXML;
        bstr bXSL = urlXSL;
        bstr bXML = urlDoc;
        int i;
        VARIANT_BOOL  varSucceeded;
       OutputHelper * o;
        AsciiText   pchDoc(urlDoc);   
        _reference<IStream> pstm;
        FileStream * fstm = new FileStream(pchDoc, false);
        AsyncStream * pastm = new AsyncStream(fstm, 16);
        _reference<AsyncStream> astm = pastm;
        pastm->Release();
        VARIANT vURL;

        dXSL->setDOM(true);
        dXML->setDOM(true);

        TRY
        {

            Processor * xsl = Processor::newProcessor();

            checkhr(dXSL->QueryInterface(IID_IXMLDOMDocument, (void **) &docXSL));
            vURL.vt = VT_BSTR;
            V_BSTR(&vURL) = bXSL;
            docXSL->load(vURL, &varSucceeded);
            bXSL = (BSTR) null;
            Exception * exc = dXSL->getErrorMsg();
            if (exc)
                exc->throwE();

            checkhr(docXSL->get_documentElement(&rootXSL));

            checkhr(dXML->QueryInterface(IID_IXMLDOMDocument, (void **) &docXML));
            if (asynchronous)
            {
                docXML->put_async(VARIANT_TRUE);
                dXML->Load(astm);
            }
            else
            {
                vURL.vt = VT_BSTR;
                V_BSTR(&vURL) = bXML;
                docXML->load(vURL, &varSucceeded);
                bXML = (BSTR) null;
            }
            exc = dXML->getErrorMsg();
            if (exc)
                exc->throwE();

            for (int i = 0; i < loops; i++)
            {
                if (out)
                {
                    if (outputFile->equals(_T("none")))
                    {
                        o = null;
                    }
                    else
                    {
                        out->getIStream(&pstm);
                        o = dXML->createOutput(pstm);
                        o->setOutputStyle(OutputHelper::PRETTY);
                    }
                }
                else
                {
                    StringBuffer * strbuf = StringBuffer::newStringBuffer();
                    StringStream * strstm = StringStream::newStringStream(strbuf);
                    strstm->getIStream(&pstm);
                    o = dXML->createOutput(pstm);
                    o->setOutputStyle(OutputHelper::PRETTY);
                }

                Start();

                xsl->init(GetElement(useXSLDoc ? (IXMLDOMNode *) docXSL : (IXMLDOMNode *) rootXSL), GetElement(docXML), o);

                while (true)
                {
                    TRY
                    {
                        if (asynchronous)
                        {
                            dXML->run();
                        }
                        xsl->execute();
                        pstm = null;
                        break;
                    }
                    CATCH
                    {
                        HRESULT hr = ERESULT;
                        if (hr != E_PENDING)
                        {
                            if (o)
                            {
                                o->close();
                            }
                            pstm = null;
                            Exception::throwAgain();
                        }                    
                    }
                    ENDTRY
                }

                Stop();
            }

            if (timeit)
            {                
                reportTimes(StdIO::getOut());
            }


            timeit = 0;
            loops = 0;
        }
        CATCH
        {
            Exception * e = GETEXCEPTION();
            PrintStream* stdout = StdIO::getOut();
            if (e->getUrl())
            {
                stdout->println(e->getUrl());
            }
            stdout->println(e->toString());
            stdout->println(_T("\n"));
        }
        ENDTRY
    }

    void Run(A(String) * args)
    {
#if DBG == 1
       // DbgRegisterMallocSpy();
#endif
        // if we are running an XQL test, vector to that code then bail out
        if ((args->length() > 0) && (*args)[0]->equals(_T("-xql")))
        {
            xqlMain(args);
            return;
        } 

        //testURL();  return;
        outputFile = String::newString(_T("StdIO::getOut()"));
        outputEncoding = null;
        parseArgs(args);

        PrintStream* sout = StdIO::getOut();

        if (fileName == null && omtest == false && domtest == false) 
        {
            printUsage(StdIO::getOut());
        } 
        else 
        {
#ifdef RENTAL_MODEL
            Model model(fFreeThreaded ? MultiThread : Rental);
#endif
            _MSXMLEXE_ = true;
            if (omtest) omTest();
            if (domtest) TestDOMDocument();
            if (xslFilename) TestXSL(xslFilename, fileName);
            if (fileName == null) return;

            String * url = createURL(fileName);
            Document* d = null;

            if (threads)
            {
#ifdef WORKINGSET
                if (dumpws)
                    DumpWorkingSet();
#endif
                if (mte)
                    ElementMThreadTest(url);

                if (mtn)
                    NodeMThreadTest(url);
                loops = 0;
                timeit = false;
            }

            TRY 
            {
                for (long i = 0; i < loops; i++) 
                {
                    d = TestLoad(url);

                    if (prompt)
                    {
                        sout->print(String::newString(_T("Press any key to continue...")));
                        int ch = StdIO::getIn()->read();
                    }
                    if (i < loops - 1)
                    {
                        d = null;
                        GarbageCollect();
                    }
#ifdef WORKINGSET
                    if (dumpws)
                        DumpWorkingSet(true);
#endif
                }
                if (loops > 1)
                {
                    reportTimes(sout);
                }
            }
            CATCH
            {
                Exception * e = GETEXCEPTION();
                sout->println(e->toString());
            }
            ENDTRY

            timeit = false;
            if (d != null) 
            {
#if DBG == 1
//                DbgDumpMemory();
#endif
                if (tree) 
                {
                    TRY 
                    {
                        dumpTree(d->getDocElem(), PrintStream::newPrintStream(out, true), String::emptyString());
                    } 
                    CATCH
                    {
                        Exception * e = GETEXCEPTION();
                        sout->print(_T("Error saving output '"));
                        sout->print(outputFile);
                        sout->println(_T("'"));
                        Exception::throwAgain();
                    }
                    ENDTRY
                } 
                else if (fOutput ) 
                {
                    if( outputEncoding != null )                    
                        d->setEncoding( outputEncoding );
//                    d->setOutputStyle(style);
                    d->setOutputStyle(OutputHelper::PRETTY);
                    IStream * pIStream = NULL;
                    TRY 
                    {
                        if (! outputFile->equals(_T("StdIO::getOut()")))
                        {
//                            URL * url = createURL(outputFile);
//                            UrlStream * out = UrlStream::newUrlStream(url, Stream::WRITE);
                        }
                        out->getIStream(&pIStream);

                        OutputHelper* xout = d->createOutput(pIStream, d->getEncoding());

                        d->save(xout);
                        xout->close();
                    } 
                    CATCH
                    {
                        Exception * e = GETEXCEPTION();
                        sout->print(_T("Error saving output '"));
                        sout->print(outputFile);
                        sout->println(_T("'"));
                        Exception::throwAgain();
                        return;
                    }
                    ENDTRY
                    release(&pIStream);
                }

                d = null;
                GarbageCollect();
            }
        }   
        if (count && timeit)
        {
            StdIO::getOut()->print(_T("avg="));
            StdIO::getOut()->println(sum/count);
            StdIO::getOut()->print(_T("avg1="));
            StdIO::getOut()->println(sum1/count);
        }
    }

    void printUsage(PrintStream * o)
    {
        Resources::VerInfo info;
        bool rc = Resources::GetVersion(String::newString(_T("MSXML.EXE")),info);
        String* ver = null;
        if (rc)
        {
            String* dot = String::newString(_T("."));
            ver = String::add(String::valueOf(info[0]),
                dot, String::valueOf(info[1]), 
                dot, String::valueOf(info[2]), 
                dot, String::valueOf(info[3]), null);
        } 
        else
        {
            ver = String::newString(_T("1.0"));
        }
        o->println(_T("Microsoft C++ XML Parser Command Line Tester"));
        o->print(_T("Version: ")); o->println(ver);
        o->println(_T(""));
        o->println(_T("Usage: msxml [options] filename"));
        o->println(_T(""));
        o->println(_T("This program parses the given XML file and optionally generates "));
        o->println(_T("output from the resulting data structures."));
        o->println(_T(""));
        o->println(_T("Possible options are:"));
#ifdef UNIX
        o->println(_T("-x   Write parsed XML in original XML format."));
        o->println(_T("-x1  Write parsed XML in tree format."));
#else
        o->println(_T("-d   Write parsed XML in original XML format."));
        o->println(_T("-d1  Write parsed XML in tree format."));
#endif
        o->println(_T("-f   Fast parsing that bypasses tree building."));
//        o->println(_T("-fc  case insensitive mode"));
        o->println(_T("-fd  ignore DTD's"));
        o->println(_T("-ft  use free threaded document"));
//        o->println(_T("-fe  omit built in HTML entities"));
//        o->println(_T("-fw  no whitespace nodes"));
//        o->println(_T("-fn  no namespace handling"));
        o->println(_T("-f4  ie4 compatibility"));
        o->println(_T("-fs  short end tags"));
        o->println(_T("-t n Specifies how many iterations to time the parser."));
        o->println(_T("-o n Provides a filename for dumping output."));
        o->println(_T("     (This is needed for unicode XML)."));
        o->println(_T("-e n Specifies a different character encoding for output."));
        o->println(_T("     (Default is same as input)."));
        o->println(_T("-c   Output in compact mode (no newlines or tabs)."));
        o->println(_T("-p   Output in pretty mode (insert newlines & tabs)."));
        o->println(_T("-i   interactive mode - prompt after document is loaded."));
        o->println(_T("-m   Perform Object Model test, which creates file test.xml."));
        o->println(_T("-w   Test IXMLDOMDocument."));
        o->println(_T("-a   Parse asynchronously."));
        o->println(_T("-mte n Runs element tests on n threads.  Can be combined with -t"));
        o->println(_T("-mtn n Runs node tests on n threads. Can be combined with -t and -stream"));
        o->println(_T("-gc n Run gc every n allocations"));
   #ifdef WORKINGSET
        o->println(_T("-ws  Dump working set."));
   #endif
        o->println(_T("-s   XSL stylesheet."));
        o->println(_T(""));
        o->println(_T("-xql Run an XQL test."));
        o->println(_T("-stream stream save test"));
        o->println(_T("-x n encode"));
        o->println(_T("-xu n encode unicode"));
        o->println(_T("-y n decode"));
        o->println(_T("-yu n decode"));
    }
    
    void parseArgs(AString * args)
    {
         for (int i = 0; i < args->length(); i++)
        {
#ifdef UNIX
            if ((*args)[i]->equals(_T("-x"))) 
#else
            if ((*args)[i]->equals(_T("-d"))) 
#endif
            {
                fOutput = true;
                out = FileOutputStream::newFileOutputStream(STD_OUTPUT_HANDLE);
            } 
#ifdef UNIX
            else if ((*args)[i]->equals(_T("-x1"))) 
#else
            else if ((*args)[i]->equals(_T("-d1"))) 
#endif
            {
                tree = true;
                fOutput = true;
                out = FileOutputStream::newFileOutputStream(STD_OUTPUT_HANDLE);
            } 
            else if ((*args)[i]->equals(_T("-t"))) 
            {
                i++;
                loops = Integer::parseInt((*args)[i]);
                timeit = true;
            } 
            else if ((*args)[i]->equals(_T("-mte"))) 
            {
                i++;
                threads = Integer::parseInt((*args)[i]);
                timeit = true;
                mte = true;
            } 
            else if ((*args)[i]->equals(_T("-mtn"))) 
            {
                i++;
                threads = Integer::parseInt((*args)[i]);
                timeit = true;
                mtn = true;
            } 
            else if ((*args)[i]->equals(_T("-i"))) 
            {
                prompt = true;
            } 
            else if ((*args)[i]->equals(_T("-fc"))) 
            {
                caseInsensitive = true;
            } 
            else if ((*args)[i]->equals(_T("-fd"))) 
            {
                ignoreDTD = true;
            } 
            else if ((*args)[i]->equals(_T("-ft"))) 
            {
                fFreeThreaded = true;
            } 
            else if ((*args)[i]->equals(_T("-fw"))) 
            {
                omitWhitespace = true;
            } 
            else if ((*args)[i]->equals(_T("-fn"))) 
            {
                parseNamespaces = false;
            }
            else if ((*args)[i]->equals(_T("-f4"))) 
            {
                ie4compat = true;
            }
            else if ((*args)[i]->equals(_T("-fs"))) 
            {
                shortEndTags = true;
            }
            else if ((*args)[i]->equals(_T("-a"))) 
            {
                asynchronous = true;
            } 
            else if ((*args)[i]->equals(_T("-o"))) 
            {
                i++;
                TRY 
                {
                    outputFile = (*args)[i] ;
                    out = FileOutputStream::newFileOutputStream(outputFile);
                } 
                CATCH
                {
                    Exception * e = GETEXCEPTION();
                    PrintStream* sout = StdIO::getOut();
                    sout->print(_T("Error saving output '"));
                    sout->print(outputFile);
                    sout->println(_T("'"));
                    Exception::throwAgain();
                }
                ENDTRY
            } 
            else if ((*args)[i]->equals(_T("-e"))) 
            {
                i++;
                outputEncoding = (*args)[i];
            } 
            else if ((*args)[i]->equals(_T("-c"))) 
            {
                style = OutputHelper::COMPACT;
            } 
            else if ((*args)[i]->equals(_T("-p"))) 
            {
                style = OutputHelper::PRETTY;
            } 
            else if ((*args)[i]->equals(_T("-f"))) 
            {
                fast = true;
            } 
            else if ((*args)[i]->equals(_T("-S"))) 
            {
                stream = true;
            } 
            else if ((*args)[i]->equals(_T("-ws"))) 
            {
                dumpws = true;
            } 
            else if ((*args)[i]->equals(_T("-w"))) 
            {
                domtest = true;
            } 
            else if ((*args)[i]->equals(_T("-ne"))) 
            {
                nodeelem = true;
            } 
            else if ((*args)[i]->equals(_T("-m"))) 
            {
                omtest = true;
            } 
            else if ((*args)[i]->equals(_T("-s")))
            {
                i++;
                if (i < args->length())
                    xslFilename = (*args)[i];

            }
            else if ((*args)[i]->equals(_T("-sd")))
            {
                useXSLDoc = true;
                i++;
                if (i < args->length())
                    xslFilename = (*args)[i];

            }
            else if ((*args)[i]->equals(_T("-stream")))
            {
                fStreamTest = true;
            }
            else if ((*args)[i]->equals(_T("-create")))
            {
                fCreateTest = true;
            }
            else if ((*args)[i]->equals(_T("-xy")))
            {
                fDecodeEncode = true;
            }
            else if ((*args)[i]->equals(_T("-xa")))
            {
                fAtomValues = true;
            }
            else if ((*args)[i]->equals(_T("-xw")))
            {
                fPreserveWhiteSpace = false;
            }
#if DBG == 1
            else if ((*args)[i]->equals(_T("-log"))) 
            {
                OpenLogFile("\\log.");
            } 
#endif
            else if ((*args)[i]->equals(_T("-x")) || (*args)[i]->equals(_T("-xu"))) 
            {
                fEncode = true;
                fUnicode = (*args)[i]->equals(_T("-xu"));
                i++;
                TRY 
                {
                    outputFile = (*args)[i] ;
                    out = FileOutputStream::newFileOutputStream(outputFile);
                } 
                CATCH
                {
                    Exception * e = GETEXCEPTION();
                    PrintStream* stdout = StdIO::getOut();
                    stdout->print(_T("Error saving output '"));
                    stdout->print(outputFile);
                    stdout->println(_T("'"));
                    Exception::throwAgain();
                }
                ENDTRY
            } 
            else if ((*args)[i]->equals(_T("-y")) || (*args)[i]->equals(_T("-yu"))) 
            {
                fDecode = true;
                fUnicode = (*args)[i]->equals(_T("-yu"));
            }
            else 
            {
                fileName = (*args)[i];
            }
        }        
    }
    
    void Start() 
    { 
        QueryPerformanceCounter((LARGE_INTEGER *)&start);
    }

    void Stop() 
    { 
        __int64 end;
        __int64 freq;
        QueryPerformanceCounter((LARGE_INTEGER *)&end);
        QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
        long time = (long)((end - start) * 1000 / freq);
        if (timeit)
        {
            PrintStream* out1 = StdIO::getOut();
#if DBG == 1
            out1->print(_T("time ="));
#ifdef UNIX
            out1->print((int)time);
#else
            out1->print(time);
#endif
#endif
#if DBG == 1
            out1->print(_T(" created="));
#ifdef UNIX
            out1->print((int)Name::created);
#else
            out1->print(Name::created);
#endif
            out1->print(_T(" reused="));
#ifdef UNIX
            out1->println((int)Name::reused);
#else
            out1->println(Name::reused);
#endif
#endif
        }
#if DBG == 1
        Name::created = 0;
        Name::reused = 0;
#endif
        if (min == 0 || time < min) min = time;
        if (max == 0 || time > max) max = time;
        sum += time;
        count++;
    }

    void reportTimes(PrintStream * o) 
    {
        o->print(fileName);
        o->print(_T("\t"));
#ifdef UNIX
        o->print((int)min);
#else
        o->print(min);
        o->print(_T("\t"));
#endif
        o->println(_T(" milliseconds."));
#if DBG == 1
        o->print(_T("\tMax="));
#ifdef UNIX
        o->print((int)max);
#else
        o->print(max);
#endif
        o->print(_T("\t"));
        o->println((sum/count));
#endif
    }

    String * createURL(String * fileName) 
    {
        String * url = null;
        TRY 
        {
            url = String::newString(fileName);
        } 
        CATCH
        {
            PrintStream* sout = StdIO::getOut();
            sout->print(_T("Cannot create url from input argument '"));
            sout->print(fileName);
            sout->println(_T("'"));
            Exception::throwAgain();
        }
        ENDTRY
        return url;
    }

    // dumpTree dumps a tree out in the following format:
    // 
    // root
    // |---node
    // |---node2
    // |   |---foo
    // |   +---bar.
    // +---lastnode
    //     |---apple
    //     +---orange.
    //

    void dumpTree(Element * e, PrintStream * o, String * indent) //throws IOException
    {
        Element * eChild;
        HANDLE hChild;

        if (indent->length() > 0)
        {
            o->print(indent);
            o->print(_T("---"));
        }
        // Once we've printed the '+', from then on we are
        // to print a blank space, since the '+' means we've
        // reached the end of that branch.
        String * lines = indent->replace('+',' ');
        bool dumpText = false;
        bool dumpTagName = true;
        bool dumpAttributes = false;

        switch (e->getType()) {
        case Element::CDATA:
            o->print(_T("CDATA"));
            dumpText = true;
            break;
        case Element::COMMENT:
            o->print(_T("COMMENT"));
            dumpTagName = false;
            break;
        case Element::DOCUMENT:
            o->print(_T("DOCUMENT"));
            break;
        case Element::DOCTYPE:
            o->print(_T("DOCTYPE"));
            dumpAttributes = true;
            dumpTagName = false;            
            break;
        case Element::ELEMENT:
            o->print(_T("ELEMENT"));
            dumpAttributes = true;
            break;
        case Element::ENTITY:
            o->print(e->getTagName());
            dumpTagName = false;
            if (e->numElements() == 0) dumpText = true;
            break;
        case Element::ENTITYREF:
            o->print(_T("ENTITYREF"));
            dumpText = true;
            break;
        case Element::NOTATION:
            o->print(_T("NOTATION"));
            dumpText = true;
            break;
        case Element::ELEMENTDECL:
            o->print(_T("ELEMENTDECL"));
            break;
        case Element::PCDATA:
            o->print(_T("PCDATA"));
            dumpText = true;
            break;
        case Element::PI:
            if (e->getTagName()->toString()->equalsIgnoreCase(_T("xml")))
            {
                o->print(_T("XMLDECL"));
                dumpAttributes = true;
                dumpTagName = false;
            } else {
                o->print(_T("PI"));
                dumpTagName = true;
            }
            break;
        }
            
        if (dumpTagName) {
            NameDef * n = e->getNameDef();
            if (n != null) 
            {
                o->print(_T(" "));
                o->print(n->toString());
            }
        }
        if (e->getType() == Element::ENTITY) 
        {
            Entity * en = (Entity *)e;
            o->print(_T(" "));
            o->print(en->getName());
        } 
        else if (e->getType() == Element::ELEMENTDECL)
        {
            ElementDecl * ed = (ElementDecl *)e;
            o->print(_T(" "));
        }
        if (dumpAttributes)
        {
            HANDLE h;
            Element * a = e->getFirstAttribute(&h);
            while (a)
            {
                Element * aNext;

                o->println(String::emptyString());
                o->print(lines);

                aNext = e->getNextAttribute(&h);

                if (! aNext)
                {
                    o->print(_T("   |---"));
                }
                else
                {
                    o->print(_T("   +---"));
                }
                o->print(_T("ATTRIBUTE "));
                o->print(a->getNameDef()->toString());
                o->print(_T(" \""));
                o->print(a->getValue()->toString());
                o->print(_T("\""));

                a = aNext;

            }
        }

        if (dumpText && e->getText() != null) 
        {
            o->print(_T(" \""));
            o->print(e->getText());
            o->print(_T("\""));
        }
        o->println(String::emptyString());

        String * newLines = String::emptyString();
        if (lines->length() > 0) 
        {
            newLines = String::add(lines, String::newString(_T("   |")), null);
        } 
        else 
        {
            newLines = String::newString(_T("|"));
        }

        eChild = e->getFirstChild(&hChild);
        while (eChild)
        {
            Element * eNext = e->getNextChild(&hChild);
            if (! eNext ) 
            {
                if (lines->length() > 0) 
                {
                    newLines = String::add(lines, String::newString(_T("   +")), null);
                } 
                else 
                {
                    newLines = String::newString(_T("+"));
                }
            }
            dumpTree(eChild,o,newLines);
            eChild = eNext;
        }
        o->flush();
    }

    void omTest()
    {
        IStream * pIStream = NULL;
        TRY {
            Document* d = new Document();
            NamespaceMgr * mgr = d->getNamespaceMgr();
            String* encoding = String::newString(_T("UTF-8"));
            d->setEncoding(encoding);
            d->createElement(d->getDocElem(),Element::PCDATA,null,String::newString(_T("\r\n")));
            Element* root = d->createElement(d->getDocElem(),Element::ELEMENT, mgr->createNameDef(String::newString(_T("ROOT"))),null);

            d->createElement(root,Element::PCDATA,null,String::newString(_T("\r\n")));
            Element* child = d->createElement(root,Element::ELEMENT,mgr->createNameDef(String::newString(_T("CHILD"))),null);
            d->createElement(child,Element::PCDATA,null,String::newString(_T("This is a test")));
            d->createElement(child,Element::PCDATA,null,String::newString(_T("\r\n")));
            d->createElement(child,Element::COMMENT,null,String::newString(_T("comment goes here")));
            d->createElement(child,Element::PCDATA,null,String::newString(_T("\r\n")));
            d->createElement(child,Element::PI,mgr->createNameDef(String::newString(_T("PI"))),String::newString(_T("processing instruction")));
            d->createElement(child,Element::PCDATA,null,String::newString(_T("\r\n")));

            FileOutputStream* out = FileOutputStream::newFileOutputStream(String::newString(_T("test.xml")));
            out->getIStream(&pIStream);
            OutputHelper* hout = d->createOutput(pIStream); // BUGBUG ?, encoding);
            d->save(hout);
            hout->close();
        } CATCH 
        {
            Exception * e = GETEXCEPTION();
            StdIO::getOut()->print(_T("Exception writing text.xml: "));
            StdIO::getOut()->println(e->toString());
        }
        ENDTRY 
        release(&pIStream);
    }

    RString fileName;    
    RString xslFilename;
    bool tree;
    int loops;
    int threads;
    bool mte;
    bool mtn;
    bool timeit;
    bool caseInsensitive;
    bool shortEndTags;
    bool omitWhitespace;
    bool ignoreDTD;
    bool asynchronous;
    bool ie4compat;
    bool stream;
    __int64 start;
    long min;
    long max;
#ifdef UNIX
    int count;
    int sum;
#else
    long count;
    long sum;
#endif
    long sum1;
    int style;
    RFileOutputStream out;
    RString outputEncoding;
    bool fast;
    bool slot;
    bool nodeelem;
    bool dumpws;
    bool omtest;
    RString outputFile;
    bool prompt;
    bool parseNamespaces;
    bool domtest;
    bool useXSLDoc;

    void init()
    {
        fOutput = false;
        tree = false;
        loops = 1;
        threads = 0;
        mte = false;
        mtn = false;
        timeit = false;
        caseInsensitive = false;
        shortEndTags = false;
        omitWhitespace = false;
        ignoreDTD = false;
        asynchronous = false;
        ie4compat = false;
        stream =false;
        start = 0;
        min =0;
        max = 0;
        count = 0;
        sum = 0;
        sum1 = 0;
        style = 0;
        fast = false;
        slot = false;
        nodeelem = false;
        dumpws = false;
        omtest = false;
        prompt = false;
        parseNamespaces = true;
        domtest = false;
    }

    void finalize()
    {
        fileName = null;
        out = null;
        outputEncoding = null;
        outputFile = null;
    }
};
DEFINE_CLASS_MEMBERS(TestApp, _T("TestApp"), Base);

//===================================================================================
// fake instance for _root.cxx
HINSTANCE g_hInstance = NULL;

#include "core/io/printstream.hxx"

#ifdef _DEBUG
extern TAG tagPointerCache;
extern TAG tagNode;
extern TAG tagDOMNode;
#endif

#ifdef unix
#define main prog_main
 
int main(int argc, char **argv);
 
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pszCmdLine, int
nCmdShow)  {
    extern int __argc;
    extern char **__argv;
    return main(__argc, __argv);
}
#endif


void __cdecl 
CPlusMain(int argc, char * argv[])
{
    EnsureTls _EnsureTls;
    if (!_EnsureTls.getTlsData())
        return;
    EnableTag(tagNode, FALSE);
    EnableTag(tagRefCount, FALSE);
    EnableTag(tagPointerCache, FALSE);

    TRY
    {
        A(String) * args = new (argc-1) A(String);
        for (int i = 1; i < argc; i++)
        {
            (*args)[i-1] = String::newString(argv[i]);
        }
        // hardcoded for now
        TestApp * testapp = new TestApp();
        testapp->Run(args);
    }
    CATCH
    {
        Exception * e = GETEXCEPTION();
        StdIO::getOut()->println(String::add(String::newString(_T("Exception: ")), e->toString(), null));
    }
    ENDTRY
}

int __cdecl main(int argc, char * argv[])
{
    CoInitialize(NULL);

    CPlusMain(argc, argv);
#ifdef WORKINGSET
//    DumpWorkingSet();
#endif

#if DBG == 1
//    DbgDumpMemory();
//    DumpWorkingSet();
//    DbgDumpProcessHeaps();
#endif
    // Can't call this, because if we do the static destructors that use
    // COM will crash.
    // CoUninitialize();

    return 0;
}

extern BOOL MTInit();
extern void OMInit();

EXTERN_C BOOL 
Runtime_init()
{
    EnableTag(11, TRUE);   // !SYMBOLS

    // make sure TLS is allocated for this thread
    EnsureTlsData();

    // init multi thread support
    if (MTInit() == FALSE)
        return FALSE;

    // EnsureTlsData();
    STACK_ENTRY;


    TRY
    {
        g_fClassInitCalled = true;

        Exception::classInit();

//       BaseInit();
        Name::classInit();
        EnumWrapper::classInit();
        Document::classInit();
        OMInit();

#ifdef PRFDATA
        PrfInitCounters();
#endif

    }
    CATCH
    {
        g_fClassInitCalled = false;
        return FALSE;
    }
    ENDTRY
    return TRUE;
}

extern void ClearReferences();
extern void MTExit();

EXTERN_C void
Runtime_exit()
{
#ifdef WORKINGSET
    WsExit();
#endif

    EnableTag(tagAssertPop, FALSE);

    TRY
    {
        // free objects on 0 count list
        Base::StartFreeObjects();

        Document::classExit();
        // free global references
        ClearReferences();

        Exception::classExit();

        // free objects on 0 count list
        Base::FinishFreeObjects();

        // release global page manager
        SlotAllocator::classExit();
        VMManager::classExit();

#ifdef PRFDATA
        PrfCleanupCounters();
#endif

    }
    CATCH
    {
        Assert(0 && "Should never have an Exception thrown in *::classExit()");
    }
    ENDTRY

#ifdef DEBUG_MEMORY
    DbgDumpMemory();
#endif

    // clear multi thread support;
    MTExit();
}
#if 0
void _EnterCriticalSection(LPCRITICAL_SECTION lpcs)
{
    int i = lpcs->LockCount;
    lpcs->LockCount++;
}

void _LeaveCriticalSection(LPCRITICAL_SECTION lpcs)
{
    int i = lpcs->LockCount;
    lpcs->LockCount--;
}
#endif


_reference<IXMLDOMDocument> NodeThreadData::domdoc;
_reference<IXMLDocument2> ElementThreadData::ie4doc;
