/*
 * @(#)BaseQuery.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XQL BaseQuery object
 * 
 */


#ifndef _XQL_QUERY_BASEQUERY
#define _XQL_QUERY_BASEQUERY

#ifndef _XQL_QUERY_QUERY
#include "xql/query/query.hxx"
#endif

#ifndef _XQL_QUERY_BASEOPERAND
#include "xql/query/baseoperand.hxx"
#endif

#ifndef _XQL_QUERY_ELEMENTSTACK
#include "xql/query/elementstack.hxx"
#endif

DEFINE_CLASS(BaseQuery);


/**
 * A simple Enumeration for iterating over nodes that satisfy an XQL query.
 *
 * Hungarian: bqy
 *
 */
class BaseQuery : public BaseOperand, public Query
{

    DECLARE_CLASS_MEMBERS_I1(BaseQuery, BaseOperand, Query);
    DECLARE_CLASS_CLONING(BaseQuery, Base);

    public:
        enum Cardinality    // hungarian: card
        {
            SCALAR = 0,
            ANY = 1,
            ALL = 2
        };


        // Query Interface Methods

        virtual void setContext(QueryContext *inContext, Element * e);

        virtual Element * contains(QueryContext *inContext, Element * eRoot, Element * e);

        virtual void target(Vector * v);

        virtual int getIndex(QueryContext *inContext, Element * e);

        virtual bool isEnd(QueryContext *inContext, Element * e);

        virtual Operand * toOperand();

        virtual DWORD getFlags();

        virtual aint * path(aint * p);


        // Methods from Enumeration
        virtual bool hasMoreElements();

        virtual Object * peekElement();

        virtual Object * nextElement();

        virtual void reset();

        // Operand Interace Methods

        virtual bool isScalar();

        virtual Query * toQuery();

        virtual TriState isTrue(QueryContext *inContext, Query * qyContext, Element * eContext);

        virtual void getValue(QueryContext *inContext, Query * qyContext, Element * eContext, OperandValue * opval);

        virtual TriState compare(QueryContext * inContext, OperandValue::RelOp op, Query * qyContext, Element * eContext, Operand * opnd);


        // Object Methods
        virtual Object * clone();

#if DBG == 1
        virtual String * toString();
#endif
        // BaseQuery Methods

        static Element * getNext(ElementFrame * frame);
        
        bool    shouldAddRef() {return _eNext.isAddRef();};

    protected: 
        BaseQuery();
        BaseQuery(Query * qyInput , Cardinality card, const bool shouldAddRef);

        BaseQuery(const bool shouldAddRef); 

        // cloning constructor, shouldn't do anything with data members...
        BaseQuery(CloningEnum e);

        Query * getInput() {return _qyInput;}

        void setInput(Query * qy) {_qyInput = qy;}

        virtual Element * peekInput();

        void advanceInput() {if (_qyInput && !_fAdvancedInput) {_qyInput->nextElement(); _fAdvancedInput = true;}}

        virtual Element * advance();

        virtual aint * appendPath(aint * p);

        virtual void finalize();

        /**
         * Input Query
         */

        RQuery _qyInput;

        // BUGBUG - Doesn't _qctxt need to be a RQueryContext???
        QueryContext  * _qctxt;

        /**
         * Next element
         */

        // WAA - replacing RElement
        ROElement  _eNext;

        /**
         * Lookahead element
         */
        // WAA - replacing RElement
        ROElement  _eLookahead;

        /**
         * Lookahead index
         */

        unsigned    _iLookahead;

        /**
         * Cached path to the current element
         */

        raint       _path;

        unsigned    _index;          // Index of the current element
 
        unsigned    _card:2;            // SCALAR, ANY, or ALL
        unsigned    _fLookahead:1;      // Did we lookahead?
        unsigned    _fAdvancedInput:1;  // Did we advance the input?
        unsigned    _fInIsEnd:1;        // In the isEnd function?

private:

        void    init(Query * qyParent, Cardinality card, const bool shouldAddRef);

        Cardinality getCard() {return (Cardinality) _card;}


};


class Path 
{
public:
    static aint * append(aint *p, int i);
    static aint * append(aint * pdest, aint * psrc);
    static int compare(aint *p1, aint *p2);
    static void clear(aint * p) {if (p) {(*p)[0] = 0;}}

private:
    enum {DEFAULT_PATH_LENGTH = 8};
};


#endif _XQL_QUERY_BASEQUERY

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
