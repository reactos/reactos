/*
 * @(#)EvalAction.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XTL EvalAction object
 * 
 */


#ifndef XTL_ENGINE_EVALACTION
#define XTL_ENGINE_EVALACTION

#ifndef _XTL_ENGINE_ACTION
#include "action.hxx"
#endif

DEFINE_CLASS(Processor);
DEFINE_CLASS(ScriptEngine);
DEFINE_CLASS(EvalAction);


/**
 * The simplest XTL action
 *
 * Hungarian: evalact
 *
 */
class EvalAction : public Action
{
    DECLARE_CLASS_MEMBERS(EvalAction, Action);

    public:

        static EvalAction * newEvalAction(Element * e);

        virtual void execute(int state, Processor * xtl);

        virtual bool compile(Processor * xtl, String * language);

#if DBG == 1
        virtual String * toString();
#endif

    protected: 
        EvalAction(Element * e);

        virtual void finalize();

         // hide these (not implemented)

        EvalAction(){}
        EvalAction( const EvalAction &);
        void operator =( const EvalAction &);

    private:

        /**
         * The script parser to use for this eval
         */

    	RScriptEngine _se;

        bool          _fNoEntities;
};


#endif _XTL_ENGINE_EVALACTION

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
