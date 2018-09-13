/*
 * @(#)TemplateAction.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XTL TemplateAction object
 * 
 */


#ifndef _XTL_ENGINE_TEMPLATEACTION
#define _XTL_ENGINE_TEMPLATEACTION

#ifndef _XTL_ENGINE_GETITEMACTION
#include "getitemaction.hxx"
#endif

DEFINE_CLASS(TemplateAction);

// BUGBUG - optimizations
//
// 1. Don't use weak references
// 2. use shorts 
// 3. put templates array into actions array.
// 4. put _cExecuting into bit field so that flags are available
// 5. eliminate _cActions and _cTemplates 
// 6. use auxiliary structure to hold template information.  Most templateactions don't have
//    nested templates

/**
 * An action that contains other actions and establishes a lexical scope for template lookup.
 *
 * Hungarian: tmpl
 *
 */

class TemplateAction : public GetItemAction
{
    DECLARE_CLASS_MEMBERS(TemplateAction, GetItemAction);

    public:

        static TemplateAction * newTemplateAction(TemplateAction * tmplOuter, Element * e, Query * qy = null);
        static TemplateAction * newChooseAction(TemplateAction * tmplOuter, Element * e);

        // Action Methods

        virtual void    execute(int state, Processor * xtl);
        virtual bool    compile(Processor * xtl, String * language);


        // TemplateAction Methods
        virtual Query * getSelectQuery();

        TemplateAction *    getTemplate(QueryContext * inContext, Element * e);
        Action *            getAction(int i);
        void                compileTemplates(Processor * xtl, Element * e, String * language);
        void                setCopy(bool fCopy) {_fCopy = fCopy;}
        void                setForceEndTag(bool fForceEndTag) {_fForceEndTag = fForceEndTag;}
        void                setId(int id) {_id = id;}
        int                 getId() {return _id;}

#if DBG == 1
        virtual String * toString();
#endif

    protected: 
        TemplateAction(TemplateAction * tmplOuter, Element * e, Query * qy = null, bool fIsChoose = false);

        virtual void finalize();
        virtual void compileQuery(Processor * xtl, String * language);

        int isExecuting() {return _cExecuting > 0;}
        void push(Processor * xtl);
        void pop(Processor * xtl);

         // hide these (not implemented)

        TemplateAction(){}
        TemplateAction( const TemplateAction &);
        void operator =( const TemplateAction &);


    private:

        void    compileBody(Processor * xtl, Element * e, Name * nm, String * language);
        Action * compileTemplate(Processor * xtl, Element * e, Name * nm, String * language);
        Action * compileChoose(Processor * xtl, Element * e, Name * nm, String * language);

        void    addAction(Action * action);
        void    addTemplate(TemplateAction * tmpl);
        void    hashTemplates();

        enum
        {
            DEFAULT_ACTIONS_SIZE = 4,
            DEFAULT_TEMPLATES_SIZE = 4,
            MIN_TEMPLATES_FOR_HASHTABLE = 16
        };

        /**
         * Local templates in this block in hashtable
         */

        RHashtable       _table;

        /**
         * Local templates in this block 
         */

        RATemplateAction _templates;

        /**
         * Actions in this block
         */

        RAAction         _actions;


        /**
         * Outer lexical block for template lookup
         */

        WTemplateAction _tmplOuter;


        /**
         * The number of times this action is on 
         * the execution stack
         */
        int             _cExecuting;    // BUGBUG - Remove - add a call to Query inUse() - Actions must be read only!


        /**
         * The number of actions
         */

        int             _cActions;

        /**
         * The number of templates
         */

        int             _cTemplates;

        /**
         * The template's id - This uniquely identifies and orders the template within it's parent template
         */

        int             _id;

        unsigned        _fCopy:1;
        unsigned        _fIsChoose:1;
        unsigned        _fForceEndTag:1;
};


#endif _XTL_ENGINE_TEMPLATEACTION

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
