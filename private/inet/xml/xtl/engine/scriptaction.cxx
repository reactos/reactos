/*
 * @(#)ScriptAction.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XTL ScriptAction object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "xtl/script/iactivescriptparse.hxx"
#include "xtlkeywords.hxx"
#include "scriptengine.hxx"
#include "processor.hxx"
#include "scriptaction.hxx"


DEFINE_CLASS_MEMBERS(ScriptAction, _T("ScriptAction"), Action);


/*  ----------------------------------------------------------------------------
    newScriptAction()

    Public "static constructor"

    @param Element  -   The XTL element for this action.

    @return         -   Pointer to new ScriptAction object
*/
ScriptAction *
ScriptAction::newScriptAction(Element * e)
{
    return new ScriptAction(e);
}        


/*  ----------------------------------------------------------------------------
    ScriptAction()

    Protected constructor

    @param Element  -   The XTL element for this action.

*/
ScriptAction::ScriptAction(Element * e) : Action(e)
{
}


/*  ----------------------------------------------------------------------------
    compile()

    Compile the children elements of this action into actions.
*/

bool
ScriptAction::compile(Processor * xtl, String * defaultLang)
{
    ScriptEngine * se = xtl->getScriptEngine(getLanguage(defaultLang));
    se->addScriptText(getElement()->getInnerText(true, true, true));
    return checkOnlyText();
}


#if DBG == 1
/**
 * Retrieves the string representation of this action.
 * @return a string representing the action.
 */


String * 
ScriptAction::toString()
{
    return super::toString();
}
#endif

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
