/*
 * @(#)CopyAction.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XTL CopyAction object
 * 
 */


#ifndef XTL_ENGINE_COPYACTION
#define XTL_ENGINE_COPYACTION

#ifndef _XTL_ENGINE_ACTION
#include "action.hxx"
#endif

#ifndef _XTL_ENGINE_PROCESSOR
#include "processor.hxx"
#endif

DEFINE_CLASS(CopyAction);

/**
 * A simple action for copying elements.
 *
 * Hungarian: cpyact
 *
 */

class CopyAction : public Action
{

    DECLARE_CLASS_MEMBERS(CopyAction, Action);

    public:

        static CopyAction * newCopyAction(Element * e, Processor::ElementSource eSource, int type);
        static CopyAction * newCopyAction(Element * e, int type);

        virtual void execute(int state, Processor * xtl);
        virtual bool compile(Processor * xtl, String * language);

#if DBG == 1
        virtual String * toString();
#endif

    protected: 
        CopyAction(Element * e, Processor::ElementSource eSource, int type);
        virtual void finalize();


         // hide these (not implemented)

        CopyAction(){}
        CopyAction( const CopyAction &);
        void operator =( const CopyAction &);

    private:

        // BUGBUG - remove when Element::ANY is 0 instead of -1.
        void setType(int type) {_type = type + 1;}
        int getType() {return _type - 1;}

        RString  _name;
        Processor::ElementSource _eSource;
        unsigned    _type:5;
};


#endif _XTL_ENGINE_COPYACTION

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
