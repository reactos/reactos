/*
 * @(#)Action.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XTL Action object
 * 
 */


#ifndef XTL_ENGINE_ACTION
#define XTL_ENGINE_ACTION


DEFINE_CLASS(Processor);
DEFINE_CLASS(Action);


/**
 * The simplest XTL action
 *
 * Hungarian: act
 *
 */
class Action : public GenericBase
{
    DECLARE_CLASS_MEMBERS(Action, GenericBase);

    public:

        static Action * newAction(Element * e);

        virtual void execute(int state, Processor * xtl);

        // BUGBUG - if more attributes are inherited in XSL then language should become inherited attributes
        virtual bool compile(Processor * xtl, String * language);

        bool matches(Element * e) {return e == _e;}
        Element * getElement() {return _e;}
        String * getLanguage(String * defaultLang);
        bool checkEmpty();
        bool checkOnlyText();

#if DBG == 1
        virtual String * toString();
#endif

    protected: 
        Action(Element * e);

        virtual void finalize();

         // hide these (not implemented)

        Action(){}
        Action( const Action &);
        void operator =( const Action &);

    private:

        /**
         * The XTL element for this action
         */

        // WAA - change RElement to Element *
        Element *   _e;
};

#if DBG == 1
String * getElementString(Element * e);
#endif

#endif _XTL_ENGINE_ACTION

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
