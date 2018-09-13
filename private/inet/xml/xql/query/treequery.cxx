/*
 * @(#)TreeQuery.cxx 1.0 6/14/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#include "treequery.hxx"


DEFINE_CLASS_MEMBERS_CLONING(TreeQuery, _T("TreeQuery"), BaseQuery);

/*  ----------------------------------------------------------------------------
    newTreeQuery()

    Public "static constructor"

    @param strName  -   query's node name
    @param enParent -   parent enumeration

    @return         -   Pointer to new TreeQuery object
*/
TreeQuery *
TreeQuery::newTreeQuery(Query * qyParent, Query * qyCond, Cardinality card, const bool shouldAddRef)
{
    return new TreeQuery(qyParent, qyCond, card, shouldAddRef);
}        


TreeQuery::TreeQuery()
    : _eRoot(false), _ePending(false)
{
}

/*  ----------------------------------------------------------------------------
    TreeQuery()

    Protected constructor

    @param strName  -   query's node name
    @param enParent -   parent enumeration

*/
TreeQuery::TreeQuery(Query * qyParent, Query * qyCond, Cardinality card, const bool shouldAddRef)
    : BaseQuery(qyParent, card == SCALAR ? ANY : card, shouldAddRef),
    _eRoot(shouldAddRef), _ePending(shouldAddRef)
{
    init(qyCond, shouldAddRef);
}


void 
TreeQuery::init(Query * qyCond, const bool shouldAddRef) 
{
    _qyCond = qyCond;
    if (qyCond)
    {
        _fIsAttribute = (qyCond->getFlags() & IS_ELEMENT) == 0;
    }
    _eRoot.setAddRef(shouldAddRef);
    _ePending.setAddRef(shouldAddRef);
}


void 
TreeQuery::finalize()
{
    _qyCond = null;
    _eRoot = null;
    _ePending = null;
    super::finalize();
}

/*  ----------------------------------------------------------------------------
    reset()
*/

void
TreeQuery::reset()
{
    _stk.reset();
    super::reset();
}


/*  ----------------------------------------------------------------------------
    setContext(QueryContext * inContext, Element * e)
*/
void
TreeQuery::setContext(QueryContext * inContext, Element * e)
{
    _stk.reset();
    if (!getInput())
    {
        _eRoot = e;
        super::setContext(inContext, null);
    }
    else
    {
        super::setContext(inContext, e);
        _eRoot = peekInput();
    }

    if (_eRoot)
    {
        _stk.push(_eRoot, false, shouldAddRef());
    }
}



/*  ----------------------------------------------------------------------------
    contains(Element * e)
*/
Element *
TreeQuery::contains(QueryContext *inContext, Element * eRoot, Element * e)
{
    Element * eNext;

    if (e && e != eRoot)
    {
        //
        // Check that e is in _qyCond, the query on the right side of the '//'
        //

        if (_qyCond)
        {
            e = _qyCond->contains(inContext, eRoot, e);
        }

        //
        // Now check that is in the query on the left side of the '//'
        //

        if (e)
        {
            while (e && e != eRoot)
            {
                eNext = super::contains(inContext, eRoot, e);
                if (eNext)
                    return eNext;

                e = e->getParent();
            }
        }
    }

    return null;
}


/*  ----------------------------------------------------------------------------
    target()
*/
void
TreeQuery::target(Vector * v)
{
    if (_qyCond)
    {
        _qyCond->target(v);
    }
    else if (!v->contains(null))
    {
        v->addElement(null);
    }
}


DWORD
TreeQuery::getFlags()
{
    DWORD dwFlags = super::getFlags();

    if (_fIsAttribute)
    {
        dwFlags &= ~IS_ELEMENT;
    }        

    return dwFlags;
}

aint *
TreeQuery::appendPath(aint *p)
{
    int tos = _stk.sp() - 2;
    for (int i = 0; i <= tos; i++)
    {
        ElementFrame * frame = _stk.item(i);
        p = Path::append(p, frame->_index);
    }        
    return p;
}


/*  ----------------------------------------------------------------------------
    advance(QueryContext *inContext)
*/
Element * 
TreeQuery::advance()
{
    ElementFrame * pframe;
    Element * eNext;
    bool fAddRef = shouldAddRef();

    // BUGBUG optimize to traverse only as necessary.  Each query should have a length.  
    // The tree query doesn't need to check if the element is in the _qyCond if the length of
    // _qyCond is greater than the element's  depth from the root.
    // 

    while(true)
    {
        if (!_stk.empty())
        {
            // If the stack isn't empty then revisit the
            // element at the top of stack.

            pframe = _stk.tos();
        }
        else 
        {
            // Get the next element from the input
            advanceInput();
            _eRoot = peekInput();
            if (_eRoot)
            {
                pframe = _stk.push(_eRoot, false, fAddRef);
            }
            else
            {
                return null;
            }
        }

        if (!_ePending)
        {
            eNext = getNext(pframe);
        }
        else
        {
            eNext = _ePending;
        }

        while (eNext)
        {
            // Visit the children next time advance
            // is called.

            if (!_ePending)
            {
                pframe = _stk.push(eNext, false, fAddRef);

                if (_fIsAttribute)
                {
                    // Visit the attributes next time advance
                    // is called.

                    pframe = _stk.push(eNext, _fIsAttribute, fAddRef);
                }

                _ePending = eNext;
            }

            // Check this node

            if (!_qyCond || _qyCond->contains(_qctxt, _eRoot, eNext))
            {
                _ePending = null;
                return eNext;
            }

            _ePending = null;

            eNext = getNext(pframe);
        }           

        _stk.pop();
    }
    return null; // Required by win64 compiler
}


Object *
TreeQuery::clone()
{
    TreeQuery * tqy = ICAST_TO(TreeQuery *, super::clone());
    tqy->init(_qyCond, shouldAddRef());

    return (Query *) tqy;
}


#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
TreeQuery::toString()
{
    StringBuffer * sb = StringBuffer::newStringBuffer();
    Query * qy = getInput();
    if (qy)
    {
        sb->append(qy->toString());
    }
    sb->append(_T("//"));
    sb->append(_qyCond ? _qyCond->toString() : String::emptyString());
    return sb->toString();
}
#endif
