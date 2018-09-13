/*
 * @(#)Action.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XTL Action object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "xtlkeywords.hxx"
#include "processor.hxx"
#include "action.hxx"


DEFINE_CLASS_MEMBERS(Action, _T("Action"), GenericBase);


/*  ----------------------------------------------------------------------------
    newAction()

    Public "static constructor"

    @param Element  -   The XTL element for this action.

    @return         -   Pointer to new Action object
*/
Action *
Action::newAction(Element * e)
{
    return new Action(e);
}        


/*  ----------------------------------------------------------------------------
    Action()

    Protected constructor

    @param Element  -   The XTL element for this action.

*/
Action::Action(Element * e) : _e(e)
{
}


/*  ----------------------------------------------------------------------------
    finalize()

    Release all references to other objects.

*/

void 
Action::finalize()
{
    _e = null;
    super::finalize();
}


/*  ----------------------------------------------------------------------------
    execute()

    Execute the action. The base action does nothing.

*/

void
Action::execute(int state, Processor * xtl)
{
}


/*  ----------------------------------------------------------------------------
    compile()

    Compile the children elements of this action into actions.
*/

bool
Action::compile(Processor * xtl, String * language)
{
    return checkEmpty();
}


String *
Action::getLanguage(String * defaultLang)
{
    String * language = (String *) getElement()->getAttribute(XTLKeywords::s_nmLanguage);
    if (!language)
    {
        language = defaultLang;
    }

    return language;
}


bool
Action::checkEmpty()
{
    HANDLE hNext;
    Element * eFirst = _e->getFirstChild(&hNext);
    if (eFirst)
    {
        // e must be empty
        Object * o = eFirst->getNameDef();
        if (!o)
        {
            o = eFirst->getText();
        }
        Processor::Error(XSL_KEYWORD_MAYNOTCONTAIN, _e->getNameDef(), o);
    }

    return true;
}


bool
Action::checkOnlyText()
{
    HANDLE hNext;
    Element * eNext = _e->getFirstChild(&hNext);
    while (eNext)
    {
        int type = eNext->getType();

        if (type != Element::PCDATA && type != Element::CDATA)
        {
            // e may only contain text
            Object * o = eNext->getNameDef();
            Processor::Error(XSL_KEYWORD_MAYNOTCONTAIN, _e->getNameDef(), o);
        }

        eNext = _e->getNextChild(&hNext);
    }

    return true;
}


#if DBG == 1
/**
 * Retrieves the string representation of this action.
 * @return a string representing the action.
 */
String * getElementString(Element * e)
{ 
    NameDef * nm;
    String * s;

    if (e)
    {
        nm = e->getNameDef();

        if (nm)
        {
            s = nm->toString();
        }
        else
        {
            s = e->getText();
        }
    }

    if (!s)
    {
        s = String::emptyString();
    }

    return s;
}


String * 
Action::toString()
{
    StringBuffer * sb = StringBuffer::newStringBuffer();
    sb->append(_T("Action["));
    sb->append(getElementString(_e));
    sb->append(_T("]"));
    return sb->toString();
}
#endif

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
