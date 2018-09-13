/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _XMLROWSET_HXX
#define _XMLROWSET_HXX

#include <simpdata.h>
#include <msdatsrc.h>

#include "uhashtable.hxx" // a hashtable of IUnknowns.
#include "document.hxx"
#include "msxmlcom.hxx"
#include "osp.hxx"

DEFINE_CLASS(XMLRowsetProvider);
class OSPWrapper;

typedef _reference<Document> RDocument;
//typedef _gitpointer<OLEDBSimpleProviderListener, &IID_OLEDBSimpleProviderListener> POLEDBSimpleProviderListener;
typedef _tsreference<OLEDBSimpleProviderListener> POLEDBSimpleProviderListener;
typedef _reference<UHashtable> RUHashtable;
typedef _reference<ElementCollection> RElementCollection;

class XMLRowsetProvider : public GenericBase, public IOLEDBSimpleProvider
{
    DECLARE_CLASS_MEMBERS_I1(XMLRowsetProvider, GenericBase, IOLEDBSimpleProvider);
    
public:
    XMLRowsetProvider() {}
    XMLRowsetProvider(Document* pDoc, Element* pElement, Document* pSchemaDoc,
                        Element* pSchema, XMLRowsetProvider* pParent);

    virtual ~XMLRowsetProvider();
    virtual void finalize();

    ////////////////////////////////////////////////////////////////
    // IOLEDBSimpleProvider interface

    virtual long    getRowCount();
    virtual long    getColumnCount();
    virtual OSPRW   getRWStatus(long iRow, long iColumn);
    virtual void    getVariant(long iRow, long iColumn, OSPFORMAT format,
                                VARIANT *pVar);
    virtual void    setVariant(long iRow, long iColumn, OSPFORMAT format,
                                VARIANT var);
// NOTIMPL virtual BSTR    getLocale();
    virtual long    deleteRows(long iRow, long cRows);
    virtual long    insertRows(long iRow, long cRows);
// NOTIMPL virtual long    find(long iRowStart, long iColumn, VARIANT val,
// NOTIMPL                             OSPFIND findFlags, OSPCOMP compType);
    virtual void    addOLEDBSimpleProviderListener(
                            OLEDBSimpleProviderListener *pospIListener);
    virtual void    removeOLEDBSimpleProviderListener(
                            OLEDBSimpleProviderListener *pospIListener);
    virtual BOOL    isAsync();
    virtual long    getEstimatedRows();
    virtual void    stopTransfer();

    void fireTransferComplete(OSPXFER doneReason=OSPXFER_COMPLETE);
    void findNewRows();

    ////////////////////////////////////////////////////////////////
    // Notification firing

    void FireCellChange(LONG iRow, LONG iCol, BOOL fBefore);
    void FireRowInsert(LONG iRow, LONG cRows, BOOL fBefore);
    void FireRowDelete(LONG iRow, LONG cRows, BOOL fBefore);

    OSPWrapper*         FindChildProvider(Element* pRow, int iCol);
    void                AddChildProvider(Element *pRow, int iCol, OSPWrapper* pChildOSP);
    void                RemoveChildProvider(Element* pRow);
private:
    struct ChildRecord
    {
        int             _iColIndex;         // index of OSP-valued column
        RUHashtable     _pChildProviders;   // OSPs for that colum, hashed by row

        ~ChildRecord() { _pChildProviders = null; }
        ChildRecord& operator=(ChildRecord& cr)
            {
                _iColIndex = cr._iColIndex;
                _pChildProviders = cr._pChildProviders;
                return *this;
            }
    };
    typedef _array<ChildRecord> AChildRecord;
    typedef _reference<AChildRecord> RAChildRecord;

    void                ResetIterator();
    Element*            GetChild(Element* pParent, int i);
    Element*            MoveToRow(int iRow);
    void                GetColumn(Element* pRow, int iCol, VARIANT* pVar);
    Element*            FindChild(Element* pRow, Name* pTag);
    ChildRecord *       FindChildColumn(int iCol);
    
    RDocument           _pDocument;
    RElement            _pRoot; // root for this level of the hierarchy
    RDocument           _pSchemaDoc; // keep whole doc alive.
    RElement            _pSchema;  // root of schema for this level.
    RElement            _pRow;
    RElementCollection  _pIter;
    void*               _pCurRow;            //used with _pIter
    RElementCollection  _pNewDataIter;      //only used by find new data method!
    void*               _pCurNewRow;         //used with _pNewDataIter
    OSPXFER             _doneReason;        //reason transfer is complete
    long                _lNumRows;          //number of rows available
    int                 _iRowIndex;
    RNameDef            _pRowset;
    WXMLRowsetProvider  _pParent;
    POLEDBSimpleProviderListener _pListener;
    RAChildRecord       _paryChildRecord;    // access to child OSPs

    static String*      getPrefix();
    static String*      getNBSP();
};

#endif
