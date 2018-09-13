/*
 * @(#)RepeatAction.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XTL RepeatAction object
 * 
 */


#ifndef _XTL_ENGINE_REPEATACTION
#define _XTL_ENGINE_REPEATACTION

#ifndef _XTL_ENGINE_TEMPLATEACTION
#include "templateaction.hxx"
#endif


DEFINE_CLASS(RepeatAction);


/**
 * An action that contains other actions and establishes a lexical scope for template lookup.
 *
 * Hungarian: repeat
 *
 */

class RepeatAction : public TemplateAction
{

    DECLARE_CLASS_MEMBERS(RepeatAction, TemplateAction);

    public:

        static RepeatAction * newRepeatAction(TemplateAction * tmpl, Element * e, Query * qy = null);
        virtual Query * getSelectQuery();

#if DBG == 1
        virtual String * toString();
#endif

    protected: 
        RepeatAction(TemplateAction * tmpl, Element * e, Query * qy);

        virtual void execute(int state, Processor * xtl);
        virtual void compileQuery(Processor * xtl, String * language);

         // hide these (not implemented)

        RepeatAction(){}
        RepeatAction( const RepeatAction &);
        void operator =( const RepeatAction &);
};


#endif _XTL_ENGINE_REPEATACTION

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
