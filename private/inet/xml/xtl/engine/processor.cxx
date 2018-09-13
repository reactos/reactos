/*
 * @(#)Processor.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of Processor object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "xql/query/absolutequery.hxx"
#include "xtl/output/elementstream.hxx"

#include "xtlkeywords.hxx"
#include "scriptengine.hxx"
#include "usetemplatesaction.hxx"
#include "processor.hxx"


DEFINE_CLASS_MEMBERS(Processor, _T("Processor"), GenericBase);

DeclareTag(tagXTLProcessor, "XSL", "Trace XTLProcessor execution");

extern TAG tagDOMOM;
extern TAG tagIE4OM;
extern TAG tagDOMNode;


void 
TransformNode(IXMLDOMNode *pStyleSheet, IXMLDOMNode *pNode, IStream *pstm)
{
    Processor * xtl = Processor::newProcessor();
    Document * pDoc;

    TRY
    {
        Element * pStyleElement = ::GetElement(pStyleSheet);
        Element * pDataElement = ::GetElement(pNode);
        // check that the models are compatible, if the target is a document
        // the check happens in TransformNodeToObject
        if (pStyleElement->model() != pDataElement->model())
            Exception::throwE(E_FAIL, XMLOM_INVALID_MODEL, null);
        Assert(pStyleElement);
        pDoc = pStyleElement->getDocument();
        xtl->init(pStyleElement, pDataElement, pstm);
        xtl->execute();
    }
    CATCH
    {
        xtl->close();
        Exception::throwAgain();
    }
    ENDTRY

    pDoc->removeNonReentrant();
    return;
}


HRESULT CreateXTLProcessor(IXTLProcessor ** ppxtl)
{
    HRESULT hr;

    TRY
    {
        *ppxtl = Processor::newProcessor();
        (*ppxtl)->AddRef();
        hr = S_OK;
    }
    CATCH
    {
        *ppxtl = null;
        hr = ERESULTINFO;
    }
    ENDTRY
    return hr;
}


SRString Processor::s_strDefaultLanguage;
SRAction Processor::s_actDefault;


HRESULT Processor::Init(IXMLDOMNode *pStyleSheet, IXMLDOMNode *pNode, IStream * pstm)
{
    HRESULT hr;
    TRY
    {
        init(::GetElement(pStyleSheet), ::GetElement(pNode), pstm);
        hr = S_OK;
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY
    return hr;
}


HRESULT Processor::Execute(BSTR *errMsg)
{
    HRESULT hr;
    *errMsg = NULL;
   
    TRY
    {
        execute();
        return S_OK;
    }
    CATCH
    {
        hr = ERESULT;

    }
    ENDTRY

    if (E_PENDING != hr)
    {

        TRY
        {
            String* msg = GETEXCEPTION()->getMessage();
            if (msg != null)
                *errMsg = ::SysAllocString(msg->getBSTR());
            close();
        }
        CATCH
        {
        }
        ENDTRY
    }

    return hr;
}


HRESULT Processor::Close()
{
    HRESULT hr;

    TRY
    {
        close();
        hr = S_OK;
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

void 
Processor::finalize()
{
    _stkActions = null;
    _stkTemplates = null;
    _xtlevents = null;
    _scriptEngines = null;
    _XRT = null;
    _xql = null;
    _eStyle = null;
    _dispOnTransformNode = null;
    _nsmgr = null;
    super::finalize();
}


/*  ----------------------------------------------------------------------------
    compile()

    Compile an XTL document

    @param Element  -   The XTL root element 

    @return         -   Pointer to new TemplateAction object
*/

TemplateAction *
Processor::compile(Element * eXTL)
{
    TemplateAction * tmpl = null;
    Name * nm;
    Atom * ns;
    Atom * gi;

    EnableTag(tagXTLProcessor, FALSE);
    EnableTag(tagDOMOM, FALSE);
    EnableTag(tagDOMNode, FALSE);
    EnableTag(tagIE4OM, FALSE);

    if (!s_strDefaultLanguage)
    {
        XTLKeywords::classInit();
    }

    _xql = XQLParser::newXQLParser(false);
    _nsmgr = NamespaceMgr::newNamespaceMgr(false);

    if (!eXTL)
    {
        goto Cleanup;
    }

    if (eXTL->getType() == Element::DOCUMENT)
    {
        eXTL = eXTL->getDocument()->getRoot();
    }

    if (!eXTL)
    {
        goto Cleanup;
    }

    pushNamespace(eXTL);

    nm = eXTL->getTagName();
    if (nm)
    {
        tmpl = TemplateAction::newTemplateAction(null, eXTL);

        ns = nm->getNameSpace();
        if (XTLKeywords::IsXSLNS(ns))
        {
            gi = nm->getName();
            if (gi == XTLKeywords::s_atomStylesheet)
            {
                // BUGBUG - Don't pass query in constructor - Add a setQuery method to GetItemAction

                tmpl = UseTemplatesAction::newUseTemplatesAction(null, eXTL, ElementQuery::newElementQuery());
            }
            else if (gi != XTLKeywords::s_atomTemplate)
            {
                Error(XSL_PROCESSOR_UNEXPECTEDKEYWORD, eXTL->getNameDef());
            }
        }
        else
        {
            tmpl->setCopy(true);
        }
    }

    if (tmpl)
    {
        tmpl->compile(this, s_strDefaultLanguage);
    }
    else
    {
        Exception::throwE(E_FAIL, XSL_PROCESSOR_BADROOTELEMENT, null);
    }

    popNamespace(eXTL);
Cleanup:
    return tmpl;
}


void
Processor::Error(ResourceID resid, Object * o1, Object * o2)
{
    Exception::throwE(E_FAIL, resid, o1 ? o1->toString() : String::emptyString(), o2 ? o2->toString() : String::emptyString(), null);
}


/*  ----------------------------------------------------------------------------
    execute()

    Execute a TemplateAction

    @param tmpl     -   The XTL TemplateAction 

    @param eData    -   The XML data source

    @return         -   none
*/

void
Processor::init(TemplateAction * tmpl, Element * eData, OutputHelper * out)
{
    if (out)
    {
        _xtlevents = ElementStream::newElementStream(out);
    }
    else
    {
        _xtlevents = null;
    }

    if (!_stkActions)
    {
        _stkActions = new (DEFAULT_STACK_SIZE) AActionFrame;
    }

    if (!_stkTemplates)
    {
        _stkTemplates = new (DEFAULT_STACK_SIZE) ATemplateFrame;
    }

    _spActions = _spTemplates = 0;

    if (tmpl && eData)
    {
        pushTemplate(TemplateAction::newTemplateAction(null, null), eData);
        pushAction(tmpl, null, ENUMELEMENTS, NOELEMENT);
        parseScriptText();
    }

    _dispOnTransformNode = eData->getDocument()->getTransformNodeSink();
}


/*  ----------------------------------------------------------------------------
    execute()

    Execute a TemplateAction

    @param eXTL     -   The XTL document

    @param eData    -   The XML document

    @return         -   none
*/

void
Processor::init(Element * eXTL, Element * eData, OutputHelper * out)
{
    _eStyle = eXTL;
    _pDocSrc = eData->getDocument();
    init(compile(eXTL), eData, out);
}


void
Processor::init(Element * eXTL, Element * eData, IStream * pstm)
{
    OutputHelper * out;

    if (pstm)
    {
        out = OutputHelper::newOutputHelper(pstm, OutputHelper::PRETTY);
        out->_fEncoding = false; // just write unicode string...
    }
    else
    {
        out = null;
    }

    init(eXTL, eData, out);
}


void
Processor::close()
{
    Enumeration * en;
    ScriptEngine * se;

    if (_xtlevents)
    {
        _xtlevents->close();
        _xtlevents = 0;
    }
    
    if (_scriptEngines)
    {
        en = _scriptEngines->elements();
    
        while (en->hasMoreElements())
        {
            se = (ScriptEngine *) en->nextElement();
            se->close();
        }

        _scriptEngines = 0;
    }
}


/*  ----------------------------------------------------------------------------
    execute()
*/

void
Processor::execute()
{
    int length;
    ActionFrame * actframe;

    if (_spActions == 0)
    {
        goto Cleanup;
    }        

    while (true)
    {
        // If we have a sink registered for debug events, fire the event
        if (_dispOnTransformNode)
        {
            // If the user returns FALSE from the event handler, stop running XSL
            if (!fireOnTransform())
                break;
        }

        if (_actframe->execute(this))
        {

            // Stop executing when there aren't anymore ActionFrames

            _spActions--;

            if (_spActions == 0)
            {
                break;
            }

            actframe = _actframe;

            // Get the next ActionFrame

            _actframe = getActionFrame();

            if (actframe->_source == PARENTELEMENT)
            {
                _actframe->_fBeginChildren = actframe->_fBeginChildren;
            }
            else
            {
                _actframe->_fHasChildren = actframe->_fBeginChildren;
            }

        }
        else
        {
            //
            // Ensure the stack is large enough and grow it if necessary!!
            // The stack can't be grown if an actionframe is running on the
            // the stack. Otherwise, after the stack is copied the code
            // can return to a dead actionframe and a execution state is lost.
            // 

            length = _stkActions->length();
            if (_spActions >= length)
            {
                length = 2 * length;
                if (length > MAX_STACK_SIZE)
                {
                    // BUGBUG - add context information
                    Exception::throwE(E_FAIL, XSL_PROCESSOR_STACKOVERFLOW, null);
                }
                _stkActions = _stkActions->resize(2 * length);
                _actframe = getActionFrame();
            }
        }
    }

Cleanup:
    unmarkReadOnly();
    close();
}


Processor::ActionResult
Processor::pushAction(EnumAction eAction, QueryAction qAction, ElementSource eSource)
{
    TemplateAction * tmpl;

    if (qAction == NEXTDATA && !nextData())
    {
        // If there isn't anymore data in the query then there's nothing more to do.

        return ACTION_NO_DATA;
    }

    if (eAction == FINDTEMPLATE)
    {
        tmpl = findTemplate();

        if (!tmpl)
        {
            // If there isn't a matching template then there's nothing more to do.

            return ACTION_OK;
        }

        // Push the new template

        return pushAction(tmpl, null, eAction, eSource);
    }

    // Push an action to iterate over this element's children or attributes.

    return pushAction(null, getCodeElement(), eAction, eSource);
}


Processor::ActionResult
Processor::pushAction(Action * action, Element * eXTLParent, EnumAction eAction, ElementSource eSource)
{
    ActionFrame * actframe;
    ActionResult  result;  
    int type;

    // BUGBUG - _actframe is null first time.
    // perf - Remove both if's.  _actframe should always be non-null
    //        Unify enum actions and processor states.
    if (_actframe)
    {
        if (eAction == Processor::ENUMATTRIBUTES)
        {
            setState(Processor::PROCESS_CHILDREN);
        }
        else if (eAction == Processor::ENUMELEMENTS)
        {
            setState(Processor::END_ELEMENT);
        }
    }

    // Get a new action frame

    actframe = getActionFrame(_spActions);

    result = actframe->init(this, action, eXTLParent, eAction);

    // BUGBUG - move this to init??

    if (result == ACTION_OK)
    {
        actframe->_source = eSource;
        switch(eSource)
        {
        case CODEELEMENT:
        case DATAELEMENT:
            actframe->setParentType(_actframe->getType());
            actframe->_fBeginChildren = false;
            actframe->_fParentHasWSInside = _actframe->_fHasWSInside;
            break;

        // BUGBUG - Consider renaming PARENTELEMENT to PROXYELEMENT?
        case PARENTELEMENT:
            actframe->setParentType(_actframe->getParentType());
            actframe->_fBeginChildren = _actframe->_fBeginChildren;
            actframe->_fHasWSInside = _actframe->_fHasWSInside;
            actframe->_fParentHasWSInside = _actframe->_fParentHasWSInside;
            break;

        // This is only used for the startup case
        default:
            actframe->setParentType(Element::ANY);
            actframe->_fBeginChildren = true;
            actframe->_fHasWSInside = false;
            actframe->_fParentHasWSInside = false;
            break;
        }

        _spActions++;
        _actframe = actframe;
    }

    return result;
}


void
Processor::pushTemplate(TemplateAction * block)
{
    pushTemplate(block, getDataElement());
}


void 
Processor::pushTemplate(TemplateAction * block, Element * eData)
{
    int length = _stkTemplates->length();
    if (_spTemplates >= length)
    {
        length = 2 * length;
        if (length > MAX_STACK_SIZE)
        {
            Exception::throwE(E_FAIL, XSL_PROCESSOR_STACKOVERFLOW, null);
        }
        _stkTemplates = _stkTemplates->resize(2 * length);
    }

    // Don't update the _spTemplates until tmplframe->init returns.
    // It may throw and E_PENDING exception.

    TemplateFrame * tmplframe = &(*_stkTemplates)[_spTemplates];
    tmplframe->init(this, block, eData);
    _tmplframe = tmplframe;
    _spTemplates++;

    _fNextData = true;

    // BUGBUG - Can't put setState here because initial push doesn't have an actionframe yet!
}

//
//  Method: getTemplateFrame
//
//  Get a specified frame from the stack of template frames.
//  n >= 0   ==> traverse down from top context as many as specified.
//      ie.  0 is top, 1 is next down etc.
//  n < 0   ==> traverse up from current context as many as specified
//      ie.  -1 is the one above us, -2 is a couple up, etc.
//

TemplateFrame * 
Processor::getTemplateFrame(int idx) 
{
    TemplateFrame * tmplStart =const_cast<TemplateFrame *>(_stkTemplates->getData());
    TemplateFrame * tmplEnd;
    TemplateFrame * tmplFrame, *tmplStop;
    int counter, delta;

    if (idx == 0)
    {
        return tmplStart;
    }        

    tmplEnd = tmplStart + _spTemplates - 1;

    // If _eData is null it means a new frame has been pushed but it
    // isn't fully initialized.
  
    if (!tmplEnd->_eData)
    {
        tmplEnd--;
    }

    if (idx < 0)
    {
        // For this case, we want to start at the end of the stack frames and
        // work ourself towards the first one.  -1 would be the parent of our
        // current context. 

        delta = -1;
        idx *= -1;
        tmplFrame = tmplEnd;
        tmplStop = tmplStart;
    }
    else
    {
        // In this case, we want to start from the top context
        // and walk down as many times as indicated to get the 
        // one desired.  

        delta = 1;
        tmplFrame = tmplStart + 1;
        tmplStop = tmplEnd;
    }

    counter = idx - 1;

    while (true)
    {
        // If the template has a query, then we want to see if we've skipped
        // down the desired number of frames yet.  If we have, then return the
        // frame.  If we haven't, then bump the number that we've skipped and
        // move on to the next one.

        if (tmplFrame->getQuery())
        {
            if (counter == 0)
            {
                return tmplFrame;
            }

            counter--;
        }

        if (tmplFrame == tmplStop)
        {
            break;
        }

        tmplFrame += delta;

        // If the template doesn't have a query, then we just want
        // to ignore it and move on to the next frame.
        // In the end, if we haven't found the specified frame, then we'll
        // return null.
    }


    // This extra check is necessary because the topmost frame does not have 
    // a query.

    if (tmplFrame == tmplStart && counter == 0)
    {
       return tmplStart;
    }

    return null;
}


Element * 
Processor::getDataElement(int idx) 
{
    TemplateFrame * aFrame = getTemplateFrame(idx);
    if (!aFrame)
        return null;

    return aFrame->getData(); 
}


Query * 
Processor::getQuery(int idx) 
{
    TemplateFrame * aFrame = getTemplateFrame(idx);
    if (!aFrame)
        return null;

    return aFrame->getQuery(); 
}


Element * 
Processor::nextData()
{
    TemplateFrame * tmplframe = getTemplateFrame();
    Query * qy = tmplframe->_qy;
    Element * e;

    if (qy)
    {
        if (!_fNextData)
        {
            qy->nextElement();
            _fNextData = true;
        }
        tmplframe->_eData = null;
        e = (Element *) qy->peekElement();
        tmplframe->_eData = e;
        tmplframe->resetActions();
    }        
    else
    {
        e = null;
    }

    return e;
}


void 
Processor::beginElement(ElementSource eSource, int toType, String * name) 
{
    Element * e = eSource == CODEELEMENT ? getCodeElement() : getDataElement(); 
    NameDef * nm;
    String * text = null;
    int type = e->getType();

    if (toType == Element::ANY)
    {
        // If the toType is ANY then this is a straight copy
        // from the source to the destination.  The following types
        // are terminal nodes to we need to get the text from them.

        toType = type;

        switch(type)
        {
            case Element::CDATA:
                if (eSource == CODEELEMENT)
                {
                    // CDATA in an XSL stylesheet is converted to PCDATA in the result
                    toType = Element::PCDATA;
                }
                // fall through 

            case Element::PI:
                // BUGBUG - Element exposes the XML Decl as a PI, but we need to differentiate
                if (Element::XMLDECL == ((ElementNode *) e)->getNodeType())
                    break;
            case Element::PCDATA:
            case Element::COMMENT:
                text = e->getText();
                break;
        }

        // BUGBUG - Need to add a method to Element to get this flag. - Consider a single function to
        // return all flags instead of multiple functions.

        // BUGBUG - Should the _fForceEndTag always come from the code element like the ws processing?

        _actframe->_fForceEndTag = ((ElementNode *) e)->getNotQuiteEmpty();
    }
    else
    {
        _actframe->_fForceEndTag = ((ElementNode *) getCodeElement())->getNotQuiteEmpty();
    }

    if (!name)
    {
        nm = e->getNameDef();
        name = nm ? nm->toString() : String::nullString();
    }

    beginElement(toType, name, text);
}


void
Processor::beginElement(int type, String * name, String * text, bool fNoEntities)
{
    Element * eCode;

    _actframe->setType(type);
    _actframe->_name = name;
    _actframe->_fHasChildren = false;

     eCode = getCodeElement();

    _actframe->_fHasWSInside = eCode->hasWSInside();
    _actframe->_fHasWSAfter = eCode->hasWSAfter();

    // Do BeginChildren notification if necessary

    if (!_actframe->_fBeginChildren && type != Element::ATTRIBUTE)
    {
        if (_xtlevents)
        {
            _xtlevents->beginChildren(_actframe->getParentType(), _actframe->_fParentHasWSInside);
        }
        _actframe->_fBeginChildren = true;
    }

    if (_xtlevents)
    {
        _xtlevents->beginElement(type, name, text, fNoEntities);
    }

    setState(PROCESS_ATTRIBUTES);
}


void 
Processor::endElement() 
{
    if (_xtlevents)
    {
        if (!_actframe->_fHasChildren && _actframe->_fForceEndTag)
        {
            _xtlevents->beginChildren(_actframe->getType(), _actframe->_fHasWSInside);
            _actframe->_fHasChildren = true;
        }
        _xtlevents->endElement(_actframe->getType(), _actframe->_name, _actframe->_fHasChildren, _actframe->_fHasWSAfter);
    }

    setState(0);
}


// Debugging helper.  This allows writing messages to the output
void
Processor::print(String * text)
{    
    if (_xtlevents)
    {
        _xtlevents->beginElement(Element::PCDATA, null, text, true);
        _xtlevents->endElement(Element::PCDATA, null, false, false);
    }
}



IUnknown *
Processor::getRuntimeObject()
{
    if (!_XRT)
    {
        _XRT = new CXTLRuntimeObject();
        // Runtime object starts with a refcount of 1 and _XRT is a smart pointer so
        // extra refcount has to be released.
        _XRT->Release();
    }

    return (IXTLRuntime *)_XRT;
}

void 
Processor::initRuntimeObject()
{
    // BUGBUG - perf - Call initRuntime once for each repeat loop instead once for each eval

    Element *pElem = getDataElement();
    getRuntimeObject();
    _XRT->_rXMLDOMNode = null;
    checkhr(pElem->QueryInterface(IID_IXMLDOMNode, (LPVOID *)&(_XRT->_rXMLDOMNode)));
}


void        
CLSIDfromProgID(const AWCHAR * progID, CLSID * pclsid)
{
    checkhr(CLSIDFromProgID(progID->getData(), pclsid));
}


ScriptEngine *
Processor::getScriptEngine(String * language)
{
    ScriptEngine * se;
    CLSID clsid; 
    String * strCLSID;

    if (!_scriptEngines)
    {
        _scriptEngines = Hashtable::newHashtable(4);
    }

    if (!language)
    {
        language = s_strDefaultLanguage;
    }

    // Get the class id for this progid
    // BUGBUG - CLSIDfromProgID has a buffer overrun security hole - truncate to max of 248 chars per Jason Taylor.
    
    CLSIDfromProgID(language->toCharArrayZ(), &clsid);

    // Store it in a String
    strCLSID = String::newString((TCHAR *) &clsid, 0, sizeof(clsid)/sizeof(TCHAR));

    // Use the class is the key because script engines can have more than one progid.
    se = (ScriptEngine *) _scriptEngines->get(strCLSID);
    if (!se)
    {
        se = ScriptEngine::newScriptEngine(this, clsid);
        _scriptEngines->put(strCLSID, se);
    }

    return se;
}


void
Processor::parseScriptText()
{
    Enumeration * en;
    ScriptEngine * se;
    
    if (_scriptEngines)
    {
        initRuntimeObject();
        en = _scriptEngines->elements();
    
        while (en->hasMoreElements())
        {
            se = (ScriptEngine *) en->nextElement();
            se->parseScriptText();
        }
    }
}

bool
Processor::fireOnTransform()
{
    VARIANT_BOOL fRet = VARIANT_TRUE;

    if (_actframe->getCurrentAction() != s_actDefault)
    {
        IXMLDOMNode *pdispData;
        IXMLDOMNode *pdispCode;
        VARIANT vaRes, vaBool;

        Element *pData = getDataElement();
                    
        Assert(pData && "Non-NULL data element expected");
        checkhr(pData->QueryInterface(IID_IXMLDOMNode, (LPVOID *)&pdispData));

        pData = getCodeElement();
        Assert(pData && "Non-NULL data element expected");
        checkhr(pData->QueryInterface(IID_IXMLDOMNode, (LPVOID *)&pdispCode));

        if (pdispCode && pdispData)
        {
            VariantInit(&vaRes);
            FireEventThroughInvoke0(
                &vaRes,
                _dispOnTransformNode, NULL,
                VT_DISPATCH, (IDispatch *)pdispCode,
                VT_DISPATCH, (IDispatch *)pdispData,
                NULL);

            // Consider VT_EMPTY to be true so that an ontransformnode event handler that returns no value doesn't
            // abort the xsl processing.

            if (V_VT(&vaRes) != VT_EMPTY)
            {
                // If we didn't get back a VT_BOOL, convert to VT_BOOL.  Then check success

                if (V_VT(&vaRes) == VT_BOOL)
                {
                    fRet = V_BOOL(&vaRes);
                }
                else 
                {
                    VariantInit(&vaBool);
                    if (SUCCEEDED(VariantChangeTypeEx(&vaBool, &vaRes, GetThreadLocale(), VARIANT_NOVALUEPROP, VT_BOOL)))
                    {
                        fRet = V_BOOL(&vaBool);
                    }
                    VariantClear(&vaRes);
                }
            }             
        }

        if (pdispCode)
            pdispCode->Release();

        if (pdispData)
            pdispData->Release();

    }

    return fRet == VARIANT_TRUE;
}


Query *
Processor::compileQuery(Element * e, Name * nm)
{
    Query * qy;
    String * str = (String *) e->getAttribute(nm);
    if (str)
    {
        qy = _xql->parse(str, _nsmgr);
    }
    else
    {
        qy = null;
    }

    return qy;
}


Query *
Processor::compileOrderBy(Query * qy, Element * e)
{
    String * str = (String *) e->getAttribute(XTLKeywords::s_nmOrderBy);
    if (str)
    {
        qy = _xql->parseOrderBy(qy, str);
    }

    return qy;
}



void
Processor::checkDocLoaded()
{
    // Don't eval any script until the document is fully downloaded
    if (!_fDocLoaded)
    {
        // BUGBUG - add isFinished to Element.
        _fDocLoaded = getDataElement()->getDocument()->getDocNode()->isFinished();
        if (!_fDocLoaded)
        {
            Exception::throwE(E_PENDING);
        }
    }
}


void
Processor::markReadOnly()
{
    if (!_fScriptReadOnlyMarked)
    {
        _pDocSrc->registerNonReentrant();
        _eStyle->getDocument()->registerNonReentrant();
        _fScriptReadOnlyMarked = true;
    }
}


void
Processor::unmarkReadOnly()
{
    if (_fScriptReadOnlyMarked)
    {
        _pDocSrc->removeNonReentrant();
        _eStyle->getDocument()->removeNonReentrant();
        _fScriptReadOnlyMarked = false;
    }
}


#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
Processor::toString()
{
    return String::newString(_T("Processor"));
}
#endif

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

