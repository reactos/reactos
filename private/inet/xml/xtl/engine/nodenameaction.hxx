/*
 * @(#)NodeNameAction.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XTL NodeNameAction object
 * 
 */


#ifndef XTL_ENGINE_NODENAMEACTION
#define XTL_ENGINE_NODENAMEACTION

#ifndef _XTL_ENGINE_ACTION
#include "getitemaction.hxx"
#endif

DEFINE_CLASS(NodeNameAction);


/**
 * A simple action for copying elements.
 *
 * Hungarian: nodename
 *
 */
class NodeNameAction : public GetItemAction 
{
    DECLARE_CLASS_MEMBERS(NodeNameAction, GetItemAction);

    public:

        static NodeNameAction * newNodeNameAction(Element * e);

        virtual void execute(int state, Processor * xtl);

    protected: 
        NodeNameAction(Element * e) : GetItemAction(e) {}

         // hide these (not implemented)

        NodeNameAction(){}
        NodeNameAction( const NodeNameAction &);
        void operator =( const NodeNameAction &);
};


#endif _XTL_ENGINE_NODENAMEACTION

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
