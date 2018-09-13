/*
 * @(#)RefQuery.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XQL RefQuery object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "elementquery.hxx"
#include "refquery.hxx"


DEFINE_CLASS_MEMBERS_CLONING(RefQuery, _T("RefQuery"), BaseQuery);

#ifdef UNIX
#ifdef TraceTag
#undef TraceTag
#define TraceTag(x)
#endif // TraceTag
#endif UNIX

/*  ----------------------------------------------------------------------------
    newRefQuery()

    Public "static constructor"

    @param strName  -   query's node name
    @param enParent -   parent enumeration

    @return         -   Pointer to new RefQuery object
*/
RefQuery *
RefQuery::newRefQuery(String * s, Query * qyParent, Cardinality card, const bool addRef)
{
    Assert((s && !qyParent) || (!s && qyParent));
    return new RefQuery(s, qyParent, card, addRef);
}        

void
RefQuery::finalize()
{
    _d = null;
    _v = null;
    _s = null;
    _table = null;
}


void
RefQuery::setContext(QueryContext * inContext, Element * e)
{
    super::setContext(inContext,e);  
    if (e)
    {
        Document * d =  e->getDocument();
        if (!_s || _d != d)
        {
            _d = d;
            _v = null;
        }
        _table = Hashtable::newHashtable();
    }
    else
    {
        _d = null;
        _v = null;
        _table = null;
    }
    _i = 0;
}


/*  ----------------------------------------------------------------------------
    contains(Element * e)
*/
Element * 
RefQuery::contains(QueryContext * inContext, Element * eRoot, Element * e)
{
    Element * eNext;

    // To implement - evaluate the refquery.  If it returns e then we match.  This
    // only works if the refquery is a constant.

    if (_s)
    {
        if (!_v)
        {
            setContext(inContext, e);
        }
        else
        {
            if (_table->get(e))
                return e;
            //_i = 0;
        }

        while ((eNext = advance()) != null)
        {
            if (e == eNext)
            {
                return e;
            }
        }
    }

    return null;
}



DWORD
RefQuery::getFlags()
{
    if (_s)
    {
        return IS_ELEMENT | NOT_FROM_ROOT | SUPPORTS_CONTAINS;
    }

    return IS_ELEMENT | NOT_FROM_ROOT | WRAPS_INPUT;
}

/*  ----------------------------------------------------------------------------
    advance()

    Advances this query's enumeration to the next matching element

    @return         -   No return value
*/
Element * 
RefQuery::advance()
{
    String * str;
    Element * e;
    Query * qy = getInput();

   // BUGBUG - This is not effecient. The PCDATA is parsed using the namesapce manager to return a vector of
   // idrefs.  However, the validation factory already calls chekforwardRefs which could just as well patch the
   // document to create a graph.

    if (!_d)
    {
        return null;
    }

    while (true)
    {
        if (!_v)
        {
            if (qy)
            {
                e = peekInput();
                if (!e)
                {
                   break;
                }
            }

            if (!_s)
            {
                str = e->getText();
            }
            else
            {
                str = _s;
            }

            if (qy)
            {
                // OK to advance here because refquery does need to preserve input path.

                advanceInput();
            }

            if (str)
            {
 
                TRY
                {
                    //
                    // parseNames may throw an exception parsing the id name.
                    // 

                    NamespaceMgr * nsmgr = _d->getNamespaceMgr();

                    Object * o = nsmgr->parseNames(DT_AV_IDREFS, str);

                    Assert(Vector::_getClass()->isInstance(o));

                    _v = SAFE_CAST(Vector *, o);

                    _i = 0;
                }
                CATCH
                {
                    // If an XML error occurs then the query is empty.
                    // Any other exception should be passed up.

                    HRESULT hr = ERESULT;
                    if ((hr & XML_ERROR_MASK) != 0)
                    {
                        break;
                    } 

                    Exception::throwAgain();                     
                }
                ENDTRY                
            }
        }

        if (_v)
        {
            int size = _v->size();
            while (_i < size)
            {
                Name * n = SAFE_CAST(Name *, _v->elementAt(_i));
                Element * e = _d->nodeFromID(n);
                // Don't increment i until returning from nodeFromID.  In the
                // async case, nodeFromID can throw an E_PENDING exception.
                _i++;
                if (e && !_table->get(e))
                {
                    _table->put(e, e);
                    return e;
                }
            }

            if (_s)
            {
                break;
            }

            _v = null;
        }

    }

    return null;
}



Object *
RefQuery::clone()
{
    RefQuery * rqy = ICAST_TO(RefQuery *, super::clone());
    rqy->_s = _s;
    rqy->_v = _v;
    rqy->_d = _d;
    return (Query *) rqy;
}


#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
RefQuery::toString()
{
    StringBuffer * sb = StringBuffer::newStringBuffer();

    sb->append(_T("ref("));
    Query * qy = getInput();
    if (qy)
    {
        sb->append(qy->toString());
    }
    sb->append(_T(")"));

    return sb->toString();

}
#endif

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
