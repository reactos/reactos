/*
 * @(#)EvalAction.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XTL EvalAction object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "xtlkeywords.hxx"
#include "scriptengine.hxx"
#include "processor.hxx"
#include "evalaction.hxx"


DEFINE_CLASS_MEMBERS(EvalAction, _T("EvalAction"), Action);


/*  ----------------------------------------------------------------------------
    newEvalAction()

    Public "static constructor"

    @param Element  -   The XTL element for this action.

    @return         -   Pointer to new EvalAction object
*/
EvalAction *
EvalAction::newEvalAction(Element * e)
{
    return new EvalAction(e);
}        


/*  ----------------------------------------------------------------------------
    EvalAction()

    Protected constructor

    @param Element  -   The XTL element for this action.

*/
EvalAction::EvalAction(Element * e) : Action(e)
{
}


/*  ----------------------------------------------------------------------------
    finalize()

    Release all references to other objects.

*/

void 
EvalAction::finalize()
{
    _se = null;
    super::finalize();
}


/*  ----------------------------------------------------------------------------
    execute()

    Execute the action. The base action does nothing.

    @return         -   0 means the action is complete.

*/

void
EvalAction::execute(int state, Processor * xtl)
{
    VARIANT var;
    HRESULT hr;
    String * text;
    Element * e;

    // Don't eval any script until the document is fully downloaded
    xtl->checkDocLoaded();
    
    text = getElement()->getText();

    var.vt = VT_EMPTY;

    _se->parseScriptText(xtl, text, VT_BSTR, &var);

    // BUGBUG - Perf problem - avoid allocating this String! We've just allocated a BSTR!!!
    xtl->beginElement(Element::PCDATA, null, String::newString(V_BSTR(&var)), _fNoEntities);
    xtl->endElement();

Cleanup:
    VariantClear(&var);
}


/*  ----------------------------------------------------------------------------
    compile()

    Compile the children elements of this action into actions.
*/

bool
EvalAction::compile(Processor * xtl, String * defaultLang)
{
    _se = xtl->getScriptEngine(getLanguage(defaultLang));
    Element * e = getElement();
    _fNoEntities = e->getAttribute(XTLKeywords::s_nmNoEntities) != null;
    return checkOnlyText();
}


#if DBG == 1
/**
 * Retrieves the string representation of this action.
 * @return a string representing the action.
 */


String * 
EvalAction::toString()
{
    return super::toString();
}
#endif

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
