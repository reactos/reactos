/*
 * @(#)SortedQuery.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XQL SortedQuery object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "sortedquery.hxx"

extern "C" void __cdecl qsortex (
    void *context, 
    void *base,
    unsigned num,
    unsigned width,
    int (__cdecl *comp)(void * , const void *, const void *)
    );
 
DEFINE_CLASS_MEMBERS_CLONING(SortedQuery, _T("SortedQuery"), BaseQuery);

DeclareTag(tagSortedQuery, "SortedQuery", "Trace SortedQuery actions");

#define SQY_SIGNATURE this, String::toCharZA(_qyKey ? _qyKey->toString() : String::nullString())

/*  ----------------------------------------------------------------------------
    newSortedQuery()

    Public "static constructor"

    @param qyParent -   parent query
    @param cond     -   query's condition
    @param op       -   query op (SCALAR, ANY, ALL)

    @return         -   Pointer to new SortedQuery object
*/
SortedQuery *
SortedQuery::newSortedQuery(Query * qyParent, Cardinality card, const bool addRef)
{
    return new SortedQuery(qyParent, card, addRef);
}        


/*  ----------------------------------------------------------------------------
    SortedQuery()

    Protected constructor

    @param strName  -   query's node name
    @param cond     -   query's condition
    @param enParent -   parent enumeration

*/
SortedQuery::SortedQuery(Query * qyParent, Cardinality card, const bool addRef)
    : BaseQuery(qyParent, card, addRef)
{
}


void 
SortedQuery::finalize()
{
    _elements = null;
    _aindex = null;
    _ki = null;
    clearKeyValues();
    super::finalize();
}


/*  ----------------------------------------------------------------------------
    setContext(QueryContext * inContext, Element * e)
*/
void
SortedQuery::setContext(QueryContext * inContext, Element * e)
{
    _state = GETTING_INPUT;
    _cElements = 0;
    // Reuse _elements and _aindex. Note: this could leave some big arrays in memory.
    clearKeyValues();
    super::setContext(inContext, e);
}


/*  ----------------------------------------------------------------------------
    advance()

    Advances this query's enumeration to the next matching element

    @return         -   No return value
*/
Element * 
SortedQuery::advance()
{
    Element * eNext;
    unsigned i;

    if (_state != SORTED)
    {
        // Cache and sort the results from the input Query

        sortElements();
    }

    i = getIndex(_qctxt, null);

    if (i < _cElements)
    {
        if (_cki)
        {
            i = (*_aindex)[i];
        }
        eNext = (*_elements)[i];
    }
    else
    {
        eNext = null;
    }

    return eNext;
}


/*  ----------------------------------------------------------------------------
    advance(QueryContext *inContext)

    Advances this query's enumeration to the next matching element

    @return         -   No return value
*/
void 
SortedQuery::sortElements()
{
    Element * eNext, * eKey;
    unsigned i, len;
    unsigned cKeys;
    Query * qyKey;
    KeyInfo * ki;
    OperandValue * popval;


    // BUGBUG - Perf - Avoid using array index operator []

    if (_state == GETTING_INPUT)
    {
        if (!_elements)
        {
            // Cache the results from the input Query

            _elements = new(DEFAULT_RESULT_SIZE) AElement;
        }

        len = _elements->length();
        while ((eNext = peekInput()) != null)
        {
            if (_cElements >= len)
            {
                len = 2 * len;
                _elements = _elements->resize(len);
            }

            (*_elements)[_cElements++] = eNext;

            // OK to advance here because sorted query can't be used with union so
            // path doesn't need to be preserved.

            advanceInput();
        }

        _state = GETTING_KEYS;
    }

    if (_state == GETTING_KEYS)
    {
        // Cache the results from the key queries

        if (_cki)
        {
            cKeys = _cElements * _cki;

            if (!_keys)
            {
                _keys = new(cKeys) AOperandValue;
                _i = 0;
                _j = 0;
                _k = 0;
            }

            while (_i < _cElements)
            {
                // Get the elements to use as keys.  Cache the key values
                // so the values don't need to be converted with each compare.

                eNext = (*_elements)[_i];

                while (_j < _cki)
                {
                    ki = &(*_ki)[_j];
                    qyKey = ki->_qy;
                    qyKey->setContext(_qctxt, eNext);
                    eKey = (Element *) qyKey->nextElement();
                    if (eKey != NULL)
                    {
                        if (ki->_dt == DT_NONE)
                        {
                            ki->_dt = eKey->getDataType();
                        }

                        popval = &(*_keys)[_k];
                        Assert(popval->isEmpty());
                        popval->makeRef();
                        eKey->getValue(ki->_dt, popval);
                    }
                    _j++;
                    _k++;
                }
                _i++;
                _j = 0;
            }

            if (_cki)
            {
                // Create index array
                _aindex = new (_cElements) auint;

                for (i = 0; i < _cElements; i++)
                {
                    (*_aindex)[i] = i;
                }                    

                // Sort the elements
                qsortex(this, (void *) _aindex->getData(), _cElements, sizeof(int), compare);

                // Release the keys

                clearKeyValues();
            }
        }

        _state = SORTED;
    }
}


int
SortedQuery::compare(void * context, const void * e1, const void * e2)
{
    unsigned i;
    int result;
    KeyInfo * ki;
    int * pi1 = (int *) e1;
    int * pi2 = (int *) e2;
    OperandValue *pKey1;
    OperandValue *pKey2;
    SortedQuery * sqy = reinterpret_cast<SortedQuery *>(context);

    pKey1 = const_cast<OperandValue *>(sqy->_keys->getData()) + (*pi1 * sqy->_cki);
    pKey2 = const_cast<OperandValue *>(sqy->_keys->getData()) + (*pi2 * sqy->_cki);

    for (i = 0; i < sqy->_cki; i++, pKey1++, pKey2++)
    {
        ki = &(*sqy->_ki)[i];

        if (!pKey1 || !pKey2)
        {
            if (pKey2)
            {
                result = -1;
                break;
            }

            if (pKey1)
            {
                result = 1;
                break;
            }

            continue;
        }

        if (pKey1->compare(0, pKey2, &result) && result)
        {
            break;
        }
    }

    if (!result)
    {
        // If keys are equal preserve document order.
        
        if (*pi1 < *pi2)
        {
            result = -1;
        }
        else
        {
            result = 1;
        }
    }

    if (ki->_fDescending)
    {
        result = -result;
    }

    return result;
}


void    
SortedQuery::addKey(Query * qyKey, bool fDescending, DataType dt)
{
    KeyInfo * ki;

    if (!_ki)
    {
        _ki = new(DEFAULT_NUM_KEYS) AKeyInfo;
    }

    if ((int) _cki >= _ki->length())
    {
        _ki = _ki->resize(2 * _ki->length());
    }

    ki = &(*_ki)[_cki++];
    ki->_qy = qyKey;
    ki->_dt = dt;
    ki->_fDescending = fDescending;
}

void
SortedQuery::clearKeyValues()
{
    // Release the keys

    if (_keys)
    {
        int i;
        int cKeys = _cElements * _cki;

        for (i = 0; i < cKeys; i++)
        {
            OperandValue * popval = &(*_keys)[i];
            popval->clear();
        }

        _keys = null;
    }
}


Object *
SortedQuery::clone()
{
    SortedQuery * sqy = ICAST_TO(SortedQuery *, super::clone());
    return (Query *) sqy;
}


#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
SortedQuery::toString()
{
    unsigned i;
    StringBuffer * sb = StringBuffer::newStringBuffer();

    sb->append(_T("SortedQuery["));
    for (i = 0; i < _cki; i++)
    {
        sb->append((*_ki)[i]._qy->toString());
    }
    sb->append(_T("]"));
    return sb->toString();
}
#endif



/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
