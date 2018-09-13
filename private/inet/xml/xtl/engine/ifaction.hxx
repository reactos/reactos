/*
 * @(#)IfAction.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XTL IfAction object
 * 
 */


#ifndef XTL_ENGINE_IFACTION
#define XTL_ENGINE_IFACTION

#ifndef _XTL_ENGINE_ACTION
#include "templateaction.hxx"
#endif

DEFINE_CLASS(ScriptEngine);
DEFINE_CLASS(IfAction);


/**
 * A simple action for copying elements.
 *
 * Hungarian: nodename
 *
 */
class IfAction : public TemplateAction 
{
    DECLARE_CLASS_MEMBERS(IfAction, TemplateAction);

    public:

        static IfAction * newIfAction(TemplateAction * tmpl, Element * e, bool fIsWhen = false);

        virtual void execute(int state, Processor * xtl);

        virtual bool compile(Processor * xtl, String * language);

    protected: 
        IfAction(TemplateAction * tmpl, Element * e, bool fIsWhen) : TemplateAction(tmpl, e), _fIsWhen(fIsWhen) {}

        virtual void finalize();

        virtual void compileQuery(Processor * xtl, String * language);

         // hide these (not implemented)

        IfAction(){}
        IfAction( const IfAction &);
        void operator =( const IfAction &);

    private:
        /**
         * The script parser to use for this eval
         */

    	RScriptEngine _se;

        RString       _script;  

        unsigned      _fIsTest:1;
        unsigned      _fIsWhen:1;  
};


#endif _XTL_ENGINE_IFACTION

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
