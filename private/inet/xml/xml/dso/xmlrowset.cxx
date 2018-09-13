/*
 * @(#)xmlrowset.cxx 1.0 6/16/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#include "ospwrap.hxx"
#include "xmlrowset.hxx"
#include "xmlnames.hxx"

DeclareTag(tagXMLrowset, "XML RowSet", "XMLROWSET");
extern TAG tagDSONotify;

DEFINE_CLASS_MEMBERS(XMLRowsetProvider, _T("XMLRowsetProvider"), GenericBase);

static NameDef *
GetChildName(Element *pElemShape)
{
    NameDef *pNameDef;

    pNameDef = (NameDef*) pElemShape->getAttribute(XMLNames::name(NAME_CHILDNAME));
    if (pNameDef == null)
    {
        pNameDef = (NameDef*) pElemShape->getAttribute(XMLNames::name(NAME_NAME));
    }

    return pNameDef;
}


XMLRowsetProvider::XMLRowsetProvider(Document* pDoc, Element* pElement, Document* pSchemaDoc, Element* pSchema, XMLRowsetProvider* pParent)
{
    Assert(pDoc);
    Assert(pElement);   // parent of the row nodes (the rowset node)
    Assert(pSchemaDoc);

//    EnableTag(tagXMLrowset, TRUE);              //tag for watching function calls

    _pDocument = pDoc;
    _pRoot = pElement;
    _pSchemaDoc = pSchemaDoc;
    _pSchema = pSchema;
    _pParent = pParent;
    if (_pDocument->getReadyStatus()==READYSTATE_COMPLETE)
        _doneReason=OSPXFER_COMPLETE;           //document is already loaded, store completion code

    // This provider iterates over nodes in root that match the
    // ROWSET name defined in schema.
    _pRowset = GetChildName(_pSchema);
    _pIter = ElementCollection::newElementCollection(_pRoot,_pRowset->getName());
    _pIter->Release();                          // born with refcount 1
    _pNewDataIter = ElementCollection::newElementCollection(_pRoot,_pRowset->getName());
    _pNewDataIter->Release();                   // born with refcount 1
    ResetIterator();                            //resets _pCurRow
    _pCurNewRow=null;
    findNewRows();                              //see if any children yet

    TraceTag((tagXMLrowset, "%p   XMLRowsetProvider::XMLRowsetProvider()  new!   Name: %s",this,
            (char *)AsciiText(_pRowset->getName()->toString())));
}


XMLRowsetProvider::~XMLRowsetProvider()
{
}


void
XMLRowsetProvider::finalize()
{
    _paryChildRecord = null;
    super::finalize();
}


long 
XMLRowsetProvider::getRowCount()
{
    TraceTag((tagXMLrowset, "%p   XMLRowsetProvider::getRowCount()   Name: %s    Rows: %ld",
            this,(char *)AsciiText(_pRowset->getName()->toString()), _lNumRows));
    return _lNumRows;
}
    

long
XMLRowsetProvider::getColumnCount()
{
    // Simply return the number of elements in the schema.
    return _pSchema->numElements();
}
    

OSPRW
XMLRowsetProvider::getRWStatus(long iRow, long iColumn)
{
    // BUGBUG - should row zero (the schema) be read only ?
    return OSPRW_READWRITE;
}


void
XMLRowsetProvider::getVariant(long iRow, long iColumn, OSPFORMAT format,
                            VARIANT *pVar)
{
    TraceTag((tagXMLrowset, "%p   XMLRowsetProvider::getVariant()   iRow=%ld   iColumn=%ld",this, iRow, iColumn));
    // BUGBUG - what do we do with 'format' ?
    // Do we do a VariantChanegTypeEx ?
    V_VT(pVar) = VT_NULL;
    V_BSTR(pVar) = NULL;

    if (iRow == 0)
    {
        // return the column names.
        long cColCount = getColumnCount();
        if (iColumn <= cColCount)
        {
            Element* e = GetChild(_pSchema, iColumn-1);
            NameDef* name = (NameDef*)e->getAttribute(XMLNames::name(NAME_NAME));
            if (name != null)
            {
                String *str = name->toString();
                
                if (e->getTagName() == XMLNames::name(NAME_ROWSET))
                {
                    // mark the column as rowset for the OSP...
                    str = String::add(getPrefix(), str,  getPrefix(), null);
                }
                
                V_VT(pVar) = VT_BSTR;
                V_BSTR(pVar) = str->getBSTR();                        
            }
        }
    }
    else
    {
        MoveToRow(iRow); // update current row.
        if (_pRow == null)
            Exception::throwE(E_FAIL); // non-existent row.
        GetColumn(_pRow, iColumn-1, pVar);

//            if (pVar->vt == VT_EMPTY)
//            {
//                pVar->vt = VT_BSTR;
//                V_BSTR(pVar) = getNBSP()->getBSTR();
//            }

        // return a BSTR if that's what was requested
        if (format == OSPFORMAT_FORMATTED && V_VT(pVar) != VT_BSTR)
        {
            VariantChangeTypeEx(pVar, pVar,
                                GetSystemDefaultLCID(),
                                VARIANT_NOVALUEPROP,
                                VT_BSTR);
        }
    }
}


void
XMLRowsetProvider::setVariant(long iRow, long iColumn, OSPFORMAT format,
                            VARIANT Var)
{
    TraceTag((tagXMLrowset, "%p   XMLRowsetProvider::setVariant()   iRow=%ld   iColumn=%ld    Var.bstrVal: %s",
            this,iRow, iColumn, (char *)(Var.bstrVal)));
    MoveToRow(iRow); // update current row.
    if (_pRow == null)
        Exception::throwE(E_FAIL); // non-existent row.

    Element* pSE = GetChild(_pSchema, iColumn-1);
    if (pSE->getTagName() == XMLNames::name(NAME_COLUMN))
    {
        NameDef* pColNameDef = GetChildName(pSE);
        
        if (pColNameDef != null) 
        {
            // if this is the $Text column, set the text
            if (pSE->getAttribute(XMLNames::name(NAME_TEXT)))       //if flag is set
            {
                VARIANT varText;
                HRESULT hr;

                VariantInit(&varText);
                hr = VariantChangeTypeEx(&varText, &Var,
                                        GetSystemDefaultLCID(),
                                        VARIANT_NOVALUEPROP,
                                        VT_BSTR);
                if (!hr)
                {
                    AssertPMATCH(_pRow, ElementNode);
                    ElementNode * penElem = CAST_TO(ElementNode *, _pRow);
                    TRY
                    {
                        penElem->getNodeData()->setInnerText(V_BSTR(&varText));
                        VariantClear(&varText);
                    }
                    CATCH
                    {
                        VariantClear(&varText);
                        Exception::throwAgain();
                    }
                    ENDTRY
                }
                else
                {
                    Exception::throwE(hr);
                }

                return;
            }
            
            // if this is an attribute column, set the attribute
            if (pSE->getAttribute(XMLNames::name(NAME_ATTR)))
            {
                if (format==OSPFORMAT_FORMATTED || Var.vt == VT_BSTR)        //then variant is string
                {
                    String *strValue = String::newString(V_BSTR(&Var));
                    _pRow->setAttribute(pColNameDef->getName(), strValue);
                }
                else if (format==OSPFORMAT_RAW)
                {
                    _pRow->setTypedAttribute(pColNameDef->getName(), &Var);
                }
                
                return;
            }

            // otherwise, find the relevant child
            Element* pChild;
            
            pChild=FindChild(_pRow, pColNameDef->getName());

            if (pChild == null)
            {
                // order doesn't actually matter because we always use
                // FindChild in getVariant.
                pChild = _pDocument->createElement(NULL, Element::ELEMENT, pColNameDef, NULL);
                _pRow->addChild(pChild,null);
            }

            // give the child the desired value
            if (pChild != null)
            {
                if (pChild->isTyped())
                {
                    pChild->setTypedValue(&Var);
                    return;
                }
                
                int cElements = pChild->numElements();

                if (cElements == 0) 
                {
                    Element* pText = _pDocument->createElement(NULL, Element::PCDATA, null, null);
                    pChild->addChild(pText,null);
                    cElements++;
                }

                if (cElements == 1)
                {
                    HANDLE h;
                    Element* pText = pChild->getFirstChild(&h);
                    if (pText->getType() == Element::PCDATA)
                    {
                        if (Var.vt == VT_BSTR)
                        {
                            String* newText = String::newString(V_BSTR(&Var));
                            pText->setText(newText);
                        }
                        else if (pText->isTyped())
                        {
                            pText->setTypedValue(&Var);
                        }
                        else
                        {
                            VARIANT varString;
                            HRESULT hr;
                            String *newText;
                            
                            VariantInit(&varString);
                            hr = VariantChangeTypeEx(&varString, &Var,
                                                    GetSystemDefaultLCID(),
                                                    VARIANT_NOVALUEPROP,
                                                    VT_BSTR);
                            if (!hr)
                            {
                                Assert(V_VT(&varString) == VT_BSTR);
                                TRY
                                {
                                    newText = String::newString(V_BSTR(&varString));
                                    VariantClear(&varString);
                                    pText->setText(newText);
                                }
                                CATCH
                                {
                                    VariantClear(&varString);
                                    Exception::throwAgain();
                                }
                                ENDTRY
                            }
                            else
                            {
                                Exception::throwE(hr);
                            }
                        }
                    }
                }
                // otherwise ignore the setVariant !
            }
        }
    }
}


long
XMLRowsetProvider::deleteRows(long iRow, long cRows)
{
    TraceTag((tagXMLrowset, "%p   XMLRowsetProvider::deleteRows()   iRow=%ld   cRows=%ld",this,iRow,cRows));
    int result = 0;

    for (int i = iRow; i < iRow+cRows; i++)     // BUGBUG (sambent) back-to-front?
    {
        Element* pRow = MoveToRow(iRow);
        if (pRow != null)
        {
            result++;
            _pRoot->removeChild(pRow);
            RemoveChildProvider(pRow);
        }
    }

    ResetIterator();        

    return result;
}
    

long
XMLRowsetProvider::insertRows(long iRow, long cRows)
{
    TraceTag((tagXMLrowset, "%p   XMLRowsetProvider::insertRows()   iRow=%ld   cRows=%ld",this,iRow,cRows));
    for (int i = iRow; i < iRow+cRows; i++)
    {
        Element* newRow = _pDocument->createElement(NULL, Element::ELEMENT,
                                                    _pRowset, NULL);
        _pRoot->addChildAt(newRow, i-1);            
    }
    ResetIterator();

    return cRows;
}
    

void
XMLRowsetProvider::addOLEDBSimpleProviderListener(
                        OLEDBSimpleProviderListener *pospIListener)
{
    _pListener = pospIListener;

                    //if we are at the top level rowset provider, and document is already loaded.
                    //(we need the getRoot() to see if the document has loaded, because it starts in READYSTATE_COMPLETE)
    if (_pDocument->getReadyStatus()==READYSTATE_COMPLETE && _pDocument->getRoot() && !_pParent)
        fireTransferComplete(_doneReason);                      //we must now tell listener transferComplete()
}
    

void
XMLRowsetProvider::removeOLEDBSimpleProviderListener(
                        OLEDBSimpleProviderListener *pospIListener)
{
    if (_pListener == (void*)pospIListener)
        _pListener = null;
}
    

BOOL
XMLRowsetProvider::isAsync()
{
    // BUGBUG -- need to fix this so we can return TRUE, but 
    // first need an event sink on Document.
//    return FALSE;
    return _pDocument->isAsync();
}
    
void
XMLRowsetProvider::fireTransferComplete(OSPXFER doneReason) //default doneReason is OSPXFER_COMPLETE
{
    _doneReason=doneReason;                                 //store doneReason in case we have no listener
    if (_pListener)
    {
        if (doneReason == OSPXFER_COMPLETE)
        {
            findNewRows();                                      //check if any new rows
        }
        TraceTag((tagXMLrowset, "%p   XMLRowsetProvider::fireTransferComplete()    reason=%d",this,doneReason));
        _pListener->transferComplete(_doneReason);          //tell it document is done loading
    }
}

long
XMLRowsetProvider::getEstimatedRows()
{
    return getRowCount();
}
    

void
XMLRowsetProvider::stopTransfer()
{
    TraceTag((tagXMLrowset, "%p   XMLRowsetProvider::stopTransfer()",this));
    _pDocument->abort(Exception::newException(XMLOM_USERABORT, XMLOM_USERABORT, null));
    fireTransferComplete(OSPXFER_ABORT);        //tell listener doc was aborted
}


/////////////////////////////////////////////////////////////
//      Notification firing

void 
XMLRowsetProvider::FireCellChange(LONG iRow, LONG iCol, BOOL fBefore)
{
    TraceTag((tagDSONotify, "%p FireCellChange(%d, %d, %d)",
                this, iRow, iCol, (LONG)fBefore));
    
    if (_pListener)
    {
        if (fBefore)
        {
            HRESULT hr = _pListener->aboutToChangeCell(iRow, iCol);
            if (hr)
                Exception::throwE(hr);
        }
        else
        {
            _pListener->cellChanged(iRow, iCol);
        }
    }
}

void 
XMLRowsetProvider::FireRowInsert(LONG iRow, LONG cRows, BOOL fBefore)
{
    TraceTag((tagDSONotify, "%p FireRowInsert(%d, %d, %d)",
                this, iRow, cRows, (LONG)fBefore));

    if (_pListener)
    {
        if (fBefore)
        {
            HRESULT hr = _pListener->aboutToInsertRows(iRow, cRows);
            if (hr)
                Exception::throwE(hr);
        }
        else
        {
            if (iRow <= _iRowIndex + 1)     // adjust index of cached row
                _iRowIndex += cRows;
            
            _lNumRows += cRows;
            _pListener->insertedRows(iRow, cRows);
        }
    }
}

void 
XMLRowsetProvider::FireRowDelete(LONG iRow, LONG cRows, BOOL fBefore)
{
    TraceTag((tagDSONotify, "%p FireRowDelete(%d, %d, %d)",
                this, iRow, cRows, (LONG)fBefore));

    if (_pListener)
    {
        if (fBefore)
        {
            HRESULT hr = _pListener->aboutToDeleteRows(iRow, cRows);
            if (hr)
                Exception::throwE(hr);
        }
        else
        {
            if (iRow <= _iRowIndex + 1)     // adjust index of cached row
            {
                if (iRow + cRows - 1 < _iRowIndex + 1)
                    _iRowIndex -= cRows;
                else
                    _iRowIndex = -10;
            }
            
            _lNumRows -= cRows;
            _pListener->deletedRows(iRow, cRows);
        }
    }
}


//================================= PRIVATE METHODS ===============================
void 
XMLRowsetProvider::ResetIterator()
{
    _pCurRow = null;
    do {
        _pRow = (Element*)_pIter->nextNode(&_pCurRow);
    } while (_pRow && _pRow->getType() != Node::ELEMENT);
    _iRowIndex = 0;
}

Element*
XMLRowsetProvider::GetChild(Element* pParent, int i)
{
    HANDLE h;
    Element* pElement = pParent->getFirstChild(&h);
    while (i-- > 0)
    {
        pElement = pParent->getNextChild(&h);
    }
    return pElement;
}

Element*
XMLRowsetProvider::MoveToRow(int iRow)
{
    Assert(_pIter != null);
    // The current row is cached for performance reasons.
    if (_iRowIndex != iRow - 1) {
        if (_iRowIndex == iRow - 2) {
            // next row in sequence.
            do {
                _pRow = (Element*)_pIter->nextNode(&_pCurRow);
            } while (_pRow && _pRow->getType() != Node::ELEMENT);
            _iRowIndex++;
        } else {
            // caller just did a random jump, so we need to resync
            // the iterator.
            ResetIterator();
            for (int i = 0; i < iRow - 1; i++) {
                do {
                    _pRow = (Element*)_pIter->nextNode(&_pCurRow);
                } while (_pRow && _pRow->getType() != Node::ELEMENT);
                _iRowIndex++;
            }
        }
    }
    return _pRow;
}

void 
XMLRowsetProvider::GetColumn(Element* pRow, int iCol, VARIANT* pVar)
{
    Element* pSE = GetChild(_pSchema, iCol);
    NameDef* pNameDef = GetChildName(pSE);

    if (pSE->getTagName() == XMLNames::name(NAME_COLUMN))
    {
        // if it's the !Text column, return the row's inner text
        if (pSE->getAttribute(XMLNames::name(NAME_TEXT)))
        {
            V_VT(pVar) = VT_BSTR;
            V_BSTR(pVar) = pRow->getText(false, true)->getBSTR();
        }

        // if it's an attribute column, get the attribute.
        else if (pSE->getAttribute(XMLNames::name(NAME_ATTR)))
        {
            if (pRow->getAttribute(pNameDef->getName()))
            {
                pRow->getTypedAttribute(pNameDef->getName(), pVar);
            }
        }

        // otherwise, it's a child element
        else
        {
            Element *pChild = FindChild(pRow, pNameDef->getName());
            if (pChild != null)
            {
                pChild->getTypedValue(pVar);
            }
        }

        return;
    }
    
    else
    {
        // Must be a rowset, so return a rowset provider.
        // First see if we've already created one for this row.
        OSPWrapper* pChildOSP = FindChildProvider(pRow, iCol);

        if (pChildOSP == null)
        {
            // Have to create a new rowset provider then.
            XMLRowsetProvider *pChildProvider;
            
            pChildProvider = new XMLRowsetProvider(_pDocument, _pRow, _pSchemaDoc, pSE, this);
            pChildOSP = new OSPWrapper(pChildProvider, _pDocument->getMutex());
            // The key is the the Element object itself.
            AddChildProvider(pChildProvider->_pRoot, iCol, pChildOSP);
            pChildOSP->Release();       // new sets refcount to 1
        }
        pVar->vt = VT_UNKNOWN;
        pChildOSP->AddRef(); // since we are going to return it.
        V_UNKNOWN(pVar) = pChildOSP;
        return;
    }
}

XMLRowsetProvider::ChildRecord *
XMLRowsetProvider::FindChildColumn(int iCol)
{
    const ChildRecord *pChildRecord = null;
    int i;

    if (_paryChildRecord)
    {
        // look up column index
        for (i=_paryChildRecord->length(), pChildRecord=_paryChildRecord->getData();
             i > 0;
             --i, ++pChildRecord)
        {
            if (pChildRecord->_iColIndex == iCol)
                break;
        }
        if (i == 0)
            pChildRecord = null;
    }

    return const_cast<ChildRecord*>(pChildRecord);
}

OSPWrapper*
XMLRowsetProvider::FindChildProvider(Element* pRow, int iCol)
{
    OSPWrapper *pOSPChild;
    ChildRecord *pChildRecord;

    // look up column index
    pChildRecord = FindChildColumn(iCol);
    
    if (pChildRecord)
    {
        // look up row in the column's hashtable
        pOSPChild = (OSPWrapper*)pChildRecord->_pChildProviders->get(pRow);
        
        // get returns a refcounted answer, but FindChildProvider does not
        if (pOSPChild)
            pOSPChild->Release();
    }
    else
        pOSPChild = null;

    return pOSPChild;
}

void 
XMLRowsetProvider::AddChildProvider(Element *pRow, int iCol, OSPWrapper* pChildOSP)
{
    ChildRecord *pChildRecord;

    // look up column index
    pChildRecord = FindChildColumn(iCol);

    // if there's no entry for the column yet, make one
    if (!pChildRecord)
    {
        int iNew;
        
        if (!_paryChildRecord)      // first make sure the array exists
        {
            iNew = 0;
            _paryChildRecord = new (1) AChildRecord;
        }
        else                        // or grow it to hold a new entry
        {
            iNew = _paryChildRecord->length();
            _paryChildRecord = _paryChildRecord->resize(iNew + 1);
        }

        pChildRecord = &((*_paryChildRecord)[iNew]);
        pChildRecord->_iColIndex = iCol;
        pChildRecord->_pChildProviders = new UHashtable(5);
    }

    // add the child OSP to the column's hashtable
    pChildRecord->_pChildProviders->put(pRow, pChildOSP);
}

void 
XMLRowsetProvider::RemoveChildProvider(Element* pRow)
{
    if (_paryChildRecord)
    {
        const ChildRecord *pChildRecord = null;
        int i;

        // remove the row from each column
        for (i=_paryChildRecord->length(), pChildRecord=_paryChildRecord->getData();
             i > 0;
             --i, ++pChildRecord)
        {
            pChildRecord->_pChildProviders->remove(pRow);
        }
    }
}

/**
 * Recursively search given row for first child or grand-child 
 * node with matching tag name.
 */
Element* 
XMLRowsetProvider::FindChild(Element* pRow, Name* pTag)
{
    HANDLE h;
    Element* pElement = pRow->getFirstChild(&h);
    while (pElement != NULL)
    {
        if (pElement->getType() == Element::ELEMENT && pElement->getTagName() == pTag) 
        {
            return pElement;
        } 
        else if (pElement->hasChildren()) 
        {
            pElement = FindChild(pElement,pTag);
            if (pElement != null)
                return pElement;
        }
        pElement = pRow->getNextChild(&h);
    }
    return null;
}

String*
XMLRowsetProvider::getPrefix()
{
    return String::newString(L"^");
}

String* 
XMLRowsetProvider::getNBSP()
{
    return String::newString(L"&nbsp;");
}


void XMLRowsetProvider::findNewRows()
{
    long newrows=0;
    void * pPrevRow=_pCurNewRow;                //trails behind nextrow, in case nextrow is null
    Node * nextRow=_pNewDataIter->nextNode(&_pCurNewRow);

    while (true)
    {
        if (nextRow && nextRow->isFinished())
        {                                       //if next row is finished
            if (nextRow->getType() == Node::ELEMENT)
            {
                newrows++;                      //increment number of new rows
            }
            pPrevRow=_pCurNewRow;
            nextRow=_pNewDataIter->nextNode(&_pCurNewRow);       //increment iterator
        }
        else                                    //otherwise
        {
            _pCurNewRow=pPrevRow;               //move current row pointer back one
            break;                              //stop checking
        }
    }

    if (newrows>0)
    {
        long startrow=_lNumRows+1;
        if (_pListener)
            _pListener->rowsAvailable(startrow, newrows);
        _lNumRows+=newrows;
        TraceTag((tagXMLrowset, "%p   XMLRowsetProvider::findNewRows()    rows=%ld   startrow=%ld   newrows=%ld",this,_lNumRows,startrow,newrows));
    }

    if (_paryChildRecord)
    {
        const ChildRecord *pChildRecord = null;
        int i;

        // find new rows in each OSP column
        for (i=_paryChildRecord->length(), pChildRecord=_paryChildRecord->getData();
             i > 0;
             --i, ++pChildRecord)
        {
            UHashtableIter * pChildIter = 
                UHashtableIter::newUHashtableIter(pChildRecord->_pChildProviders);
            Object * pKey=null;
            while (pChildIter->hasMoreElements())
            {
                OSPWrapper *pOSP = (OSPWrapper *)(pChildIter->nextElement(&pKey));
                TRY
                {
                    pOSP->getProvider()->findNewRows();
                }
                CATCH
                {
                }
                ENDTRY
                pOSP->Release();        // UHashtableIter addRefs its return value
            }
        }
    }
}


