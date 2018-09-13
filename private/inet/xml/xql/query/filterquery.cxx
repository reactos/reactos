/*
 * @(#)FilterQuery.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XQL FilterQuery object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "condition.hxx"
#include "elementquery.hxx"
#include "filterquery.hxx"

DEFINE_CLASS_MEMBERS_CLONING(FilterQuery, _T("FilterQuery"), BaseQuery);

/*  ----------------------------------------------------------------------------
    newFilterQuery()

    Public "static constructor"

    @param qyParent -   parent query
    @param cond     -   query's condition
    @param op       -   query op (SCALAR, ANY, ALL)

    @return         -   Pointer to new FilterQuery object
*/
FilterQuery *
FilterQuery::newFilterQuery(Query * qyParent, Operand * opnd, Cardinality card, const bool addRef)
{
    return new FilterQuery(qyParent, opnd, card, addRef);
}        


/*  ----------------------------------------------------------------------------
    FilterQuery()

    Protected constructor

    @param strName  -   query's node name
    @param cond     -   query's condition
    @param enParent -   parent enumeration

*/
FilterQuery::FilterQuery(Query * qyParent, Operand * opnd, Cardinality card, const bool addRef)
: BaseQuery(qyParent, card, addRef)
{
    init(opnd);
    Assert(_opnd != null);
    Assert(qyParent);
}


void 
FilterQuery::finalize()
{
    _opnd = null;
    super::finalize();
}


/*  ----------------------------------------------------------------------------
    nextElement()
*/
Object *
FilterQuery::nextElement()
{
    bool fAdvance = !_fLookahead;
    Element * e = (Element *) super::nextElement();
    if (fAdvance)
    {
        advanceInput();
    }
    return e;
}



/*  ----------------------------------------------------------------------------
    contains(Element * e)
*/
Element *
FilterQuery::contains(QueryContext * inContext, Element * eRoot, Element * e)
{
    Element * eResult = null;

    if (e && e != eRoot)
    {
        //
        // It is better to check for containment before calling match because
        // expression evaluation is more expensive than walking up the tree.
        //
        
        eResult = super::contains(inContext, eRoot, e);
        if (eResult && !matches(inContext, getInput(), e))
        {
            eResult = null;
        }
    }

    return eResult;
}


Element *
FilterQuery::peekInput()
{
    Element * e = (Element *)_qyInput->peekElement();
    unsigned index = _qyInput->getIndex(_qctxt, 0);

    if (index < _index)
    {
        _index = 0;
    }

    _fAdvancedInput = false;

    return e;
}


/*  ----------------------------------------------------------------------------
    advance()

    Advances this query's enumeration to the next matching element

    @return         -   No return value
*/
Element * 
FilterQuery::advance() 
{
    Element * eNext;
    Query * qy = getInput();

    while ((eNext = peekInput()) != null)
    {
        bool fMatches = matches(_qctxt, qy, eNext);
        if (fMatches)
        {
            break;
        }

        advanceInput();
    }

    return eNext;
}

bool FilterQuery::matches(QueryContext *inContext, Query * qy, Element * e)
{
    return _opnd->isTrue(inContext, qy, e) == TRI_TRUE;
}


Object *
FilterQuery::clone()
{
    FilterQuery * fqy = ICAST_TO(FilterQuery *, super::clone());
    fqy->init(_opnd);
    return (Query *) fqy;
}


#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
FilterQuery::toString()
{
    StringBuffer * sb = StringBuffer::newStringBuffer();

    Query * qy = getInput();
    if (qy)
    {
        sb->append(qy->toString());
    }

    sb->append(_T("["));
    sb->append(_opnd ? _opnd->toString() : String::emptyString());
    sb->append(_T("]"));
    return sb->toString();
}
#endif



/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
