/*
 * @(#)UseTemplatesAction.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XTL UseTemplatesAction object
 * 
 */


#ifndef _XTL_ENGINE_USETEMPLATESACTION
#define _XTL_ENGINE_USETEMPLATESACTION

#ifndef _XTL_ENGINE_REPEATEACTION
#include "repeataction.hxx"
#endif


DEFINE_CLASS(UseTemplatesAction);


/**
 * An action that contains other actions and establishes a lexical scope for template lookup.
 *
 * Hungarian: usetmpl
 *
 */

class UseTemplatesAction : public RepeatAction
{

    DECLARE_CLASS_MEMBERS(UseTemplatesAction, RepeatAction);

    public:

        static UseTemplatesAction * newUseTemplatesAction(TemplateAction * tmpl, Element * e, Query * qy = null);

#if DBG == 1
        virtual String * toString();
#endif

    protected: 
        UseTemplatesAction(TemplateAction * tmpl, Element * e, Query * qy);

        virtual void execute(int state, Processor * xtl);
        virtual bool    compile(Processor * xtl, String * language);

         // hide these (not implemented)

        UseTemplatesAction(){}
        UseTemplatesAction( const UseTemplatesAction &);
        void operator =( const UseTemplatesAction &);
};


#endif _XTL_ENGINE_USETEMPLATESACTION

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
