/*
 * @(#)ScriptAction.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XTL ScriptAction object
 * 
 */


#ifndef XTL_ENGINE_SCRIPTACTION
#define XTL_ENGINE_SCRIPTACTION

#ifndef _XTL_ENGINE_ACTION
#include "action.hxx"
#endif

DEFINE_CLASS(Processor);
DEFINE_CLASS(ScriptAction);


/**
 * The simplest XTL action
 *
 * Hungarian: scriptact
 *
 */
class ScriptAction : public Action
{
    DECLARE_CLASS_MEMBERS(ScriptAction, Action);

    public:

        static ScriptAction * newScriptAction(Element * e);

        virtual bool compile(Processor * xtl, String * language);

#if DBG == 1
        virtual String * toString();
#endif

    protected: 
        ScriptAction(Element * e);

         // hide these (not implemented)

        ScriptAction(){}
        ScriptAction( const ScriptAction &);
        void operator =( const ScriptAction &);
};


#endif _XTL_ENGINE_SCRIPTACTION

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
