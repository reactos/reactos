/* @(#)TemplateFrame.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XTL TemplateFrame object
 * 
 */


#ifndef _XTL_ENGINE_TEMPLATEFRAME
#define _XTL_ENGINE_TEMPLATEFRAME

#ifndef _XTL_ENGINE_TEMPLATEACTION
#include "templateaction.hxx"
#endif

#ifndef _XQL_QUERY_CONTEXT
#include "xql/query/querycontext.hxx"
#endif

/**
 * An action that contains other actions and establishes a lexical scope for block lookup.
 *
 * Hungarian: tmplframe
 *
 */

class TemplateFrame;
typedef _array<TemplateFrame> ATemplateFrame;
typedef _reference<ATemplateFrame> RATemplateFrame;


class TemplateFrame
{
    friend class Processor;

    public:
        TemplateFrame(){}
        ~TemplateFrame();
        void operator =( const TemplateFrame &);


        TemplateAction *   getTemplate(QueryContext * inContext) {return _template->getTemplate(inContext, _eData);}
        Action *           getAction(Element * e);
        Element *          getData() {return _eData;}
        Query *            getQuery() {return _qy;};
        void               resetActions(); 

#if DBG == 1
        virtual String * toString();
#endif

    protected: 

        void init(QueryContext *inContext, TemplateAction * t, Element * e);

         // hide these (not implemented)

        TemplateFrame( const TemplateFrame &);

    private:

        /**
         * The current template
         */

        RTemplateAction  _template;

        /**
         * Instruction pointer to template's actions
         */

        int             _ip;

        /**
         * The next action to execute
         */

        RAction         _action;

        /**
         * The template's query
         */

        RQuery          _qy;

        /**
         * The current XML data (context)
         */

        // WAA - change RElement to Element *
        Element  *      _eData;
};


#endif _XTL_ENGINE_TEMPLATEFRAME

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
