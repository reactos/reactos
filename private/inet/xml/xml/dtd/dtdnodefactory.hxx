/*
 * @(#)DTDNodeFactory.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _DTDNODEFACTORY_HXX
#define _DTDNODEFACTORY_HXX

typedef _reference<IXMLNodeFactory> RNodeFactory;
typedef _reference<IUnknown> RUnknown;

#include "dtd.hxx"
#include "elementdecl.hxx"
#include "attdef.hxx"
#include "entity.hxx"
#include "notation.hxx"
#include "attdef.hxx"
#include "namespacemgr.hxx"
#include <xmlparser.h>

DEFINE_CLASS(Enumeration);

class String;

// Helper functions (they throw exceptions on failure)
void _createDTDNode(IXMLNodeFactory* pFactory, IXMLNodeSource* pSource, PVOID parent, DWORD nodetype, bool beginChildren, bool terminal, const WCHAR* pwcText, ULONG ulLen, ULONG usNSLen, NameDef* name, PVOID* node);
void _endChildren(IXMLNodeFactory* pFactory, IXMLNodeSource* pSource, IUnknown* node, DWORD nodetype, BOOL empty, NameDef* name);

// Help that returns internal Parser interface from IXMLNodeSource.
// Throws an exception if it fails.  Caller must Release the parser when finished.
class XMLParser;
XMLParser* _getParser(IXMLNodeSource* pSource);

///////////////////////////////////////////////////////////////////////
// DTDNodeFactory class
//

class DTDNodeFactory : public _unknown<IXMLNodeFactory, &IID_IXMLNodeFactory>
{
protected:
    DTDNodeFactory() {};

public: 
    DTDNodeFactory(IXMLNodeFactory * fc, DTD* dtd, Document * document);
    virtual ~DTDNodeFactory();

	HRESULT STDMETHODCALLTYPE NotifyEvent( 
			/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
			/* [in] */ XML_NODEFACTORY_EVENT iEvt);

    HRESULT STDMETHODCALLTYPE BeginChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);

    HRESULT STDMETHODCALLTYPE EndChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ BOOL fEmptyNode,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);
    
    HRESULT STDMETHODCALLTYPE Error( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ HRESULT hrErrorCode,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo);
    
    HRESULT STDMETHODCALLTYPE CreateNode( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo);

protected:

    void            init();
    HRESULT         _parseEntities(IXMLNodeSource* pSource);
    HRESULT         _parseEntity(IXMLNodeSource* pSource);
    void            checkEntityRefLoop();
    HRESULT         HandlePERef(IXMLNodeSource* pSource, Name* name);
//    HRESULT         ExpandEntity(IXMLNodeSource* pSource, Name* name);
    String*         AddError(HRESULT hr, String* msg);
    void            FixNamesInDTD();
    HRESULT         FinishAttDefs();

    RNodeFactory _pFactory;         // IXMLNodeFactory to delegate calls
    RDTD         _pDTD;             // The DTD to build & validate against.
    RString      _pDTDHref;
    long         _lInDTD;
    bool         _fInDoctype;
    RAttDef      _pAttDef;          // attribute name
    REntity      _pEntity;
    RElementDecl _pElementDecl;
    bool         _fInElementDecl;
    RNotation    _pNotation;
    bool         _fInAttribute;
    bool         _fParsingEntity;
    RObject      _pDocTypeNode;
    int          _nAttDefState;
    REnumeration _pEntityEnumeration; 
    RObject      _pCurrentElement;
    long         _cDepth;
    long         _s_cDepth;
    bool         _fBuildDefNode;
    DWORD        _dwAttrType;
    bool         _fBuildNodes; // whether we are building nodes.
    RNameDef     _pDeclNamedef;
    WDocument    _pDocument;
    RNamespaceMgr _pNamespaceMgr;

    // These are the main states that the DTD node factory goes through.
    enum EState { eProlog,      // parsing prolog
                eDocType,       // parsing doctype 
                eNone,          // finished prolog and there's no DTD validation needed
                eDTD,           // parsing DTD's
                eEntities,      // parsing entities
                eValidating,    // doing validation
                eAttribute,     // validating an an attribute value.
    } _nState;

	typedef HRESULT (DTDNodeFactory:: *DTDBuilder)(
        /* [in] */ IXMLNodeSource* pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ DWORD dwType,
        /* [in] */ DWORD dwSubType,
        /* [in] */ String* str,
        /* [in] */ NameDef* namedef,
        /* [out] */ PVOID* ppNodeChild);


    DTDBuilder  _pfn;


    HRESULT BuildDecl( 
        /* [in] */ IXMLNodeSource* pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ DWORD dwType,
        /* [in] */ DWORD dwSubType,
        /* [in] */ String* str,
        /* [in] */ NameDef* namedef,
        /* [out] */ PVOID* ppNodeChild);

    HRESULT BuildEntity( 
        /* [in] */ IXMLNodeSource* pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ DWORD dwType,
        /* [in] */ DWORD dwSubType,
        /* [in] */ String* str,
        /* [in] */ NameDef* namedef,
        /* [out] */ PVOID* ppNodeChild);

    HRESULT BuildElementDecl( 
        /* [in] */ IXMLNodeSource* pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ DWORD dwType,
        /* [in] */ DWORD dwSubType,
        /* [in] */ String* str,
        /* [in] */ NameDef* namedef,
        /* [out] */ PVOID* ppNodeChild);

    HRESULT BuildNotation( 
        /* [in] */ IXMLNodeSource* pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ DWORD dwType,
        /* [in] */ DWORD dwSubType,
        /* [in] */ String* str,
        /* [in] */ NameDef* namedef,
        /* [out] */ PVOID* ppNodeChild);

    HRESULT BuildAttDefs( 
        /* [in] */ IXMLNodeSource* pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ DWORD dwType,
        /* [in] */ DWORD dwSubType,
        /* [in] */ String* str,
        /* [in] */ NameDef* namedef,
        /* [out] */ PVOID* ppNodeChild);
};

#endif _DTDNODEFACTORY_HXX
