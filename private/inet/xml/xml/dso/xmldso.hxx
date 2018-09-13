/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _XMLDSO_HXX
#define _XMLDSO_HXX

#include <msdatsrc.h>
#include <msxml.h>
#include <simpdata.h>
#include <xmldocnf.h>

#include "xmlrowset.hxx"

#ifndef _DATASRC_HXX
#include "datasrc.hxx"
#endif

typedef _reference<IHTMLElement> RIHTMLElement;
//typedef _gitpointer<DataSourceListener, &IID_DataSourceListener> PDataSourceListener;
typedef _tsreference<DataSourceListener> PDataSourceListener;
typedef _reference<OSPWrapper> ROSPWrapper;
typedef _reference<IXMLDocumentNotify> RXMLDocNotify;

class ShapeNode;
class Node;

DEFINE_CLASS(DSODocument);
DEFINE_CLASS(XMLDSO);
DEFINE_CLASS(XMLListener);

//---------------------------------------------------------------
// This is the controlling IUnknown that creates the Control
// wrapper interface and that overrides the event callbacks on
// the document so that it can generate the appropriate OSP
// events when the object model changes occur.

class DSODocument : public Document
{
    friend class CXMLIslandPeer;     // BUGBUG QI(DataSource)
    DECLARE_CLASS_MEMBERS(DSODocument, Document);

public:
    DSODocument();

    // overrides from Document
    virtual HRESULT QIHelper(DOMDocumentWrapper * pDOMDoc, IDocumentWrapper * pIE4Doc, REFIID iid, void **ppv);
    virtual void setReadyStatus(int state);
    virtual void NotifyListener(XMLNotifyReason, XMLNotifyPhase,
                                Node *pNode, Node *pNodeParent,
                                Node *pNodeBefore);

    // internal use
    void    SetDSO(XMLDSO *pDSO) { _pDSO = pDSO; }
    virtual void onEndProlog();
    virtual void onDataAvailable();

    void    AddListener(IXMLDocumentNotify *pListener);
    void    RemoveListener(IXMLDocumentNotify *pListener);
    
private:
    WXMLDSO         _pDSO;      // my controlling DSO
    RXMLDocNotify   _pListener; // my listener (if any)

    virtual void finalize()
    {
        _pDSO = null;
        _pListener = null;
        super::finalize();
    }
};



class XMLDSO :  public CSafeControl,
                public CDataSource
{
    DECLARE_CLASS_MEMBERS_NOQI_INTERFACE(XMLDSO, CSafeControl);
    DECLARE_CLASS_INSTANCE(XMLDSO, CSafeControl);
    
public:
    XMLDSO();
    virtual ~XMLDSO();
    virtual void finalize();

    //---------------------------------------------------------------
    // IUnknown override.
    //---------------------------------------------------------------
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject);
        

    ///////////////////////////////////////////////////////////////////
    // ObjectWithSite interface
    
    virtual void setSite(IUnknown *u);
    virtual void setInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions);


    ///////////////////////////////////////////////////////////////////
    // CDataSource interface
    
    virtual IUnknown*   getDataMember(DataMember bstrDM, REFIID riid);
// NOTIMPL    virtual DataMember  getDataMemberName(long lIndex);
// NOTIMPL    virtual long        getDataMemberCount();
    virtual void        addDataSourceListener(DataSourceListener *pDSL);
    virtual void        removeDataSourceListener(DataSourceListener *pDSL);


    // Internal use only...
    Mutex * getMutex()
    {
        Assert(!!_pMutex);
        return _pMutex;
    }

    void                GetAttributesFromElement(IHTMLElement *pElement);
    void                AddDocument(DSODocument *pDoc);
    DSODocument *       getDoc() { return _pDoc; }
    
    void setJavaDSOCompatible(bool JavaDSOCompatible) { _fJavaDSOCompatible=JavaDSOCompatible; }
    bool getJavaDSOCompatible() { return _fJavaDSOCompatible; }
    XMLRowsetProvider * getProvider();

    void                makeShape();
    void                DiscardShape();
    void                FireDataMemberChanged();
    
private:
    bool                makeShapeFromDTD(DTD* pDTD);        //called to check for dtds and build shape
    void                makeShapeFromData();                //checks to see if dtds exist and otherwise generate shape
    void                GenerateShape(Element* pSource, ShapeNode* pSN);
    void                OnNodeChange(XMLNotifyReason eReason,
                                     XMLNotifyPhase ePhase,
                                     Node *pNode,
                                     Node *pNodeParent,
                                     Node *pNodeBefore);

    // XMLDSO contains a listener subobject, so it can get notifications from
    // the document.  It's a subobject to avoid circular references.
    class XMLListener: public IXMLDocumentNotify
    {
        friend class XMLDSO;
    public:
        XMLDSO *        MyDSO() { return CONTAINING_RECORD(this, XMLDSO, _Listener); }

        // IUnknown methods
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject);
        ULONG STDMETHODCALLTYPE   AddRef()  { MyDSO()->weakAddRef(); return 1; }
        ULONG STDMETHODCALLTYPE   Release() { MyDSO()->weakRelease(); return 1; }
        
        // IXMLDocumentNotify methods
        HRESULT STDMETHODCALLTYPE OnNodeChange(XMLNotifyReason eReason,
                                               XMLNotifyPhase ePhase,
                                               IUnknown *punkNode,
                                               IUnknown *punkNodeParent,
                                               IUnknown *punkNodeBefore);
    };
    friend class XMLListener;
    
private:
    RDSODocument        _pDoc;
    PDataSourceListener _pDSL;
    ROSPWrapper         _pOSP;
    RDocument           _pShape;                //used to be called pSchema, this is the Shape tree - structure of rowsets
    RMutex              _pMutex;
    XMLListener         _Listener;          // listener subobject
    
    unsigned            _fJavaDSOCompatible:1;
    unsigned            _fShapeFromFilter:1;    // flag is true if we have generated the shape from filter schema
    unsigned            _fDrillIn:1;            // elide the root rowset
};


#endif
