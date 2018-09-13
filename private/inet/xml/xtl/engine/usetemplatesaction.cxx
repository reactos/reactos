/*
 * @(#)UseTemplatesAction.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XTL UseTemplatesAction object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "processor.hxx"
#include "usetemplatesaction.hxx"


DEFINE_CLASS_MEMBERS(UseTemplatesAction, _T("UseTemplatesAction"), RepeatAction);


/*  ----------------------------------------------------------------------------
    newUseTemplatesAction()

    Public "static constructor"

    @param strName  -   query's node name
    @param enParent -   parent enumeration

    @return         -   Pointer to new UseTemplatesAction object
*/
UseTemplatesAction *
UseTemplatesAction::newUseTemplatesAction(TemplateAction * tmpl, Element * e, Query * qy)
{
    return new UseTemplatesAction(tmpl, e, qy);
}        


/*  ----------------------------------------------------------------------------
    UseTemplatesAction()

    Protected constructor

    @param strName  -   query's node name
    @param enParent -   parent enumeration

*/
UseTemplatesAction::UseTemplatesAction(TemplateAction * tmpl, Element * e, Query * qy) : RepeatAction(tmpl, e, qy)
{
}


/*  ----------------------------------------------------------------------------
    execute()
*/

void
UseTemplatesAction::execute(int state, Processor * xtl)
{
    // BUGBUG - Consider merging UseTemplateAction class with RepeatAction class
    // The only difference is the processor actions passed to pushAction.

    if (state == 0)
    {
        push(xtl);
    }

    // Push an execution frame for the children

    if (xtl->pushAction(Processor::FINDTEMPLATE, Processor::NEXTDATA) != Processor::ACTION_OK)
    {
        pop(xtl);
    }
}



/*  ----------------------------------------------------------------------------
    compile()

    Compiles a TemplateAction by walking XTL element tree and creating the necessary
    action objects.

    @return             -   none
*/

bool
UseTemplatesAction::compile(Processor * xtl, String * defaultLang)
{
    Element * e = getElement();  
    String *    language = getLanguage(defaultLang);

    compileQuery(xtl, language);

    // The only elements allowed in xsl:apply-templates is xsl:template
    // Call compileTemplates directly to avoid compiling other xsl:template keywords.

    super:: compileTemplates(xtl, e, language);

    return true;
}


#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
UseTemplatesAction::toString()
{
    return super::toString();
}
#endif

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
