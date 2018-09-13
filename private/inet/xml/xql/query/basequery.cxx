/*
 * @(#)BaseQuery.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XQL BaseQuery object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "elementquery.hxx"
#include "basequery.hxx"


DEFINE_CLASS_MEMBERS_CLONING(BaseQuery, _T("BaseQuery"), BaseOperand);


/*  ----------------------------------------------------------------------------
    BaseQuery()

    Protected constructor

    @param qyInput -   parent query
    @param op       -   query op (SCALAR, ANY, ALL)

*/
BaseQuery::BaseQuery(Query * qyInput, Cardinality card, const bool shouldAddRef) 
    : 
    _qyInput(REF_NOINIT),
    _eLookahead(shouldAddRef),
    _eNext(shouldAddRef)
{
    init(qyInput, card, shouldAddRef);
}


BaseQuery::BaseQuery() 
    : _eLookahead(false),
    _eNext(false)
{
}

BaseQuery::BaseQuery(const bool shouldAddRef) 
    : _eLookahead(shouldAddRef),
    _eNext(shouldAddRef)
{
}

BaseQuery::BaseQuery(CloningEnum e)
    : super(e),
    _eLookahead(false),
    _eNext(false)
{
}

void
BaseQuery::init(Query * qyInput, Cardinality card, const bool shouldAddRef)
{
    _qyInput = qyInput;
    _card = card;
    _fLookahead = false;
    _eLookahead.setAddRef(shouldAddRef);
    _eNext.setAddRef(shouldAddRef);
}


void 
BaseQuery::finalize()
{
    _qyInput = null;
    _eNext = null;
    _qctxt = null;
    _path = null;

    // NOTE BaseOperand has no finalize()
    //super::finalize();
}


/*  ----------------------------------------------------------------------------
    hasMoreElements()
*/
bool 
BaseQuery::hasMoreElements()
{
    if (_eNext == null)
    {
        _eNext = advance();
    }

    return (_eNext != null);
}


/*  ----------------------------------------------------------------------------
    peekElement()
*/
Object *
BaseQuery::peekElement()
{
    if (_eNext == null)
    {
        _eNext = advance();
    }

    return _eNext;
}


/*  ----------------------------------------------------------------------------
    nextElement()
*/
Object *
BaseQuery::nextElement()
{
    Element * e;

    if (_eNext == null)
    {
        e = advance();
    }
    else
    {        
        e = _eNext;
        if (_fLookahead)
        {
            _eNext = _eLookahead;
            _index = _iLookahead;
            _eLookahead = null;
            _fLookahead = false;
            return e;
        }
        else
        {
            _eNext = null;
        }
    }

    if (e != null)
    {
        _index++;
    }

    return e;
}


/*  ----------------------------------------------------------------------------
    reset()
*/
void
BaseQuery::reset()
{
    // BUGBUG - delete reset.  It isn't used by the queries.

    // CONSIDER could share this null-out code with setContext()
    _index = 0;
    _eNext = null;
    _eLookahead = null;
    _fLookahead = false;
    _fAdvancedInput = false;

    if (_qyInput)
    {
        _qyInput->reset();
    }
}


/*  ----------------------------------------------------------------------------
    setContext(QueryContext *inContext, Element * e)
*/
void
BaseQuery::setContext(QueryContext *inContext, Element * e)
{
    _index = 0;
    _eNext = null;
    _eLookahead = null;
    _qctxt = inContext;
    _fLookahead = false;
    _fAdvancedInput = false;

    if (!_qyInput)
    {
        if (!e)
        {
            return;
        }

        // Create an ElementQuery only if caller passes in
        // a real context.

        _qyInput = ElementQuery::newElementQuery(e,shouldAddRef());
    }

    _qyInput->setContext(inContext, e);
}


/*  ----------------------------------------------------------------------------
    contains(Element * e)
  
*/
Element *
BaseQuery::contains(QueryContext *inContext, Element * eRoot, Element * e)
{
    if (_qyInput)
    {
        return _qyInput->contains(inContext, eRoot, e);
    }

    return e;
}


/*  ----------------------------------------------------------------------------
    target()
*/
void
BaseQuery::target(Vector * v)
{
    if (_qyInput)
    {
        _qyInput->target(v);
    }
}


/*  ----------------------------------------------------------------------------
    getIndex()
*/
int
BaseQuery::getIndex(QueryContext *inContext, Element * e)
{
    if (e && e != _eNext)
    {
        Element * eContext = contains(inContext, null, e);
        if (eContext)
        {
            Element * eNext;

            setContext(inContext, eContext);
            while ((eNext = (Element *) peekElement()) != null)
            {
                if (eNext == e)
                {
                    return _index;
                }

                nextElement();
            }

            // Element not found return index -1.
            return -1;
        }
    }

    return _index;
}


/*  ----------------------------------------------------------------------------
    isEnd()
*/
bool
BaseQuery::isEnd(QueryContext *inContext, Element * e)
{
    Element * eNext;
    unsigned index;
    bool isEnd;

    if (!_fInIsEnd)
    {
        getIndex(inContext, e);

        _path = path(null);

        // When looking a head, the element, index and the path must be cached.
        eNext = _eNext;
        index = _index;

        // Consume the current element
        nextElement();

        _fInIsEnd = true;
    }
    else
    {
        eNext = _eNext;
        index = _index;
        _index = _iLookahead;
        _eNext = null;
    }

    // See if any more elements are there

    TRY
    {
        isEnd = !hasMoreElements() || (_index == 0 && _eNext != eNext);
    }
    CATCH
    {
        _iLookahead = _index;
        _eNext = eNext;
        _index = index;
        Exception::throwAgain();
    }
    ENDTRY

    // Restore the query state
    _fLookahead = true;
    _eLookahead = _eNext;
    _iLookahead = _index;
    _eNext = eNext;
    _index = index;
    _fInIsEnd = false;

    return isEnd;
}


Operand * 
BaseQuery::toOperand()
{
    return this;
}


DWORD
BaseQuery::getFlags()
{
    if (_qyInput)
    {
        return _qyInput->getFlags();
    }        

    return IS_ELEMENT | STAYS_IN_SUBTREE | NOT_FROM_ROOT | SUPPORTS_CONTAINS;
}


aint *
BaseQuery::path(aint *p)
{
    if (_fLookahead)
    {
        p = Path::append(p, _path);
    }
    else 
    {
        if (p == null)
        {
            // If there's a left over path reuse it.
            p = _path;
            _path = null;
            Path::clear(p);
        }

        if (_qyInput)
        {
            p = _qyInput->path(p);
        }

        p = appendPath(p);
    }
    return p;
}


aint *
BaseQuery::appendPath(aint *p)
{
    return p;
}


Element *
BaseQuery::getNext(ElementFrame * frame)
{
    Element * e;
    HANDLE * ph = &frame->_h;
    Element * eParent = frame->_eParent;

    if (!frame->_fNext)
    {
        if (frame->_fIsAttribute)
        {
            e = eParent->getFirstAttribute(ph);
        }
        else
        {
            // never walk into doctype node's childnodes
            if (Element::DOCTYPE != eParent->getType())
                e = eParent->getFirstChild(ph);
            else
                e = null;
            frame->_index |= 0x40000000;
        }

        frame->_fNext = true;
    }
    else
    {
        if (frame->_fIsAttribute)
        {
            e = eParent->getNextAttribute(ph);
        }
        else
        {
            e = eParent->getNextChild(ph);
        }
    }

    frame->_index++;
    return e;
}



Element *
BaseQuery::advance()
{
    return null;
}



Element *
BaseQuery::peekInput()
{
    Element * e;
    _index = 0;
    if (_qyInput)
    {
        e = (Element *) _qyInput->peekElement();
    }
    else
    {   
        e = null;
    }

    _fAdvancedInput = false;
    return e;
}


bool 
BaseQuery::isScalar()
{
    return _card == SCALAR;
}


Query * 
BaseQuery::toQuery()
{
    return this;
}


TriState 
BaseQuery::isTrue(QueryContext *inContext, Query * qyContext, Element * eContext)
{
    // set context 
    setContext(inContext, eContext);

    return TriStateFromBool(hasMoreElements());
}


void 
BaseQuery::getValue(QueryContext *inContext, Query * qyContext, Element * eContext, OperandValue * popval)
{
    // set context 
    setContext(inContext, eContext);

    Element * e = (Element *) peekElement();
    if (e)
    {
        popval->init(getDT(), e);
    }
}


Object *
BaseQuery::clone()
{
    BaseQuery * bqy = ICAST_TO(BaseQuery *, super::clone());
    Query * qyInput = _qyInput ? (Query *) _qyInput->clone() : null;
    bqy->init(qyInput, getCard(),shouldAddRef());

    return (Query *) bqy;
}


/*  ----------------------------------------------------------------------------
    compare()
*/
TriState 
BaseQuery::compare(QueryContext * inContext, OperandValue::RelOp op, Query * qyContext, Element * eContext, Operand * opnd)
{
    TriState triResult = TRI_UNKNOWN; 
    OperandValue opval;
    Element * e;
    DataType dt;
    aint * p;

    setContext(inContext, eContext);

    opnd->getValue(inContext, qyContext, eContext, &opval);

    if (opval.isEmpty())
        goto Exit;

    dt = getDT();
    switch(_card)
    {
        case SCALAR:
        {
            // SCALAR comparison - simply compare first element
            e = (Element * ) peekElement();
            if (!e)
            {
                break;
            }

            triResult = e->compare(op, dt, &opval);

            if (triResult == TRI_TRUE)
            {
                break;
            }

            // If the path is greater than 1 than do an ANY compare

            p = path(null);
            if (!p || (*p)[0] <= 1)
            {
                break;
            }

            _card = ANY;
            nextElement();

            // fall through;
        }

        case ANY:
        {
            // ANY comparison - if any element succeeds, return success
            while ((e = (Element *) peekElement()) != null)
            {
                triResult = e->compare(op, dt, &opval);

                if (TRI_TRUE == triResult)
                {
                    goto Exit;
                }

                nextElement();
            }
            break;
        }

        case ALL:
        {
            // ALL comparison - if any element fails, return failure
            while ((e = (Element *) peekElement()) != null)
            {
                triResult = e->compare( op, dt, &opval);

                if (TRI_TRUE != triResult)
                {
                    goto Exit;
                }

                nextElement();
            }
            break;
        }
    }

Exit:
    return triResult;
}


aint* 
Path::append(aint *p, int i)
{
    if (!p)
    {
        p = new(DEFAULT_PATH_LENGTH) aint;
    }
    int next = (*p)[0] + 1;
    int len = p->length();
    if (next >= len)
    {
        p = p->resize(2 * len);
    }
    (*p)[next] = i;
    (*p)[0] = next;
    return p;
}


int
Path::compare(aint *ai1, aint * ai2)
{
    int result;

    if (!ai1)
    {
        return ai2 ? -1 : 0;
    }

    if (!ai2)
    {
        return 1;
    }

    int *pi1 = &(*ai1)[0];
    int *pi2 = &(*ai2)[0];
    int c1 = *pi1++;
    int c2 = *pi2++;

    while (c1 && c2)
    {
        result = *pi1++ - *pi2++;
        if (result)
        {
            return result;
        }

        c1--;
        c2--;
    }

    result = c1 - c2;

    return result;
}


aint *
Path::append(aint * pdest, aint * psrc)
{
    if (psrc)
    {
        Assert(!pdest || pdest != psrc);

        int * pi = &(*psrc)[0];
        int c = *pi++;
        while (c-- > 0)
        {
            pdest = Path::append(pdest, *pi++);
        }
    }
    return pdest;
}


#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
BaseQuery::toString()
{
    return String::add(String::newString(_T("BaseQuery")),
        String::newString(_T("[_card=")), (Integer::newInteger(_card))->toString(),
        String::newString(_T(",")),
        String::newString(_T("_qyInput=")), _qyInput ? _qyInput->toString() : String::nullString(),
        String::newString(_T("]")), null);
}
#endif



/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
