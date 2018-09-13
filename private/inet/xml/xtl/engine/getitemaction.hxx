/*
 * @(#)GetItemAction.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XTL GetItemAction object
 * 
 */


#ifndef XTL_ENGINE_GETITEMACTION
#define XTL_ENGINE_GETITEMACTION

#ifndef _XTL_ENGINE_ACTION
#include "action.hxx"
#endif

#ifndef _XQL_QUERY_CHILDRENQUERY
#include "xql/query/childrenquery.hxx"
#endif

#ifndef _XQL_QUERY_CONDITION
#include "xql/query/condition.hxx"
#endif

#ifndef _XQL_PARSE_XQLPARSER
#include "xql/parser/xqlparser.hxx"
#endif

DEFINE_CLASS(GetItemAction);


/**
 * A simple action for copying elements.
 *
 * Hungarian: getitem
 *
 */
class GetItemAction : public Action 
{

    DECLARE_CLASS_MEMBERS(GetItemAction, Action);

    public:

        static GetItemAction * newGetItemAction(Element * e);

        Element * getData(Processor * xtl);

        virtual void execute(int state, Processor * xtl);
        virtual bool compile(Processor * xtl, String * language);

#if DBG == 1
        virtual String * toString();
#endif

    protected: 
        GetItemAction(Element * e);

        virtual void finalize();

         // hide these (not implemented)

        GetItemAction(){}
        GetItemAction( const GetItemAction &);
        void operator =( const GetItemAction &);

        /**
         * The XQL element for this action
         */
        RQuery _qy;
};


#endif _XTL_ENGINE_GETITEMACTION

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
