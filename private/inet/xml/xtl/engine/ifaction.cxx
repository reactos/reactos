/*
 * @(#)IfAction.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XTL IfAction object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "xtlkeywords.hxx"
#include "scriptengine.hxx"
#include "processor.hxx"
#include "ifaction.hxx"


DEFINE_CLASS_MEMBERS(IfAction, _T("TemplateAction"), TemplateAction);

/*  ----------------------------------------------------------------------------
    newIfAction()

    Public "static constructor"

    @param strName  -   query's node name
    @param enParent -   parent enumeration

    @return         -   Pointer to new IfAction object
*/
IfAction *
IfAction::newIfAction(TemplateAction * tmpl, Element * e, bool fIsWhen)
{
    return new IfAction(tmpl, e, fIsWhen);
}   


/*  ----------------------------------------------------------------------------
    finalize()

    Release all references to other objects.

*/

void 
IfAction::finalize()
{
    _se = null;
    _script = null;
    super::finalize();
}     


/*  ----------------------------------------------------------------------------
    execute()
*/

void
IfAction::execute(int state, Processor * xtl)
{
    VARIANT var;

    switch (state)
    {
    case 0:
        if (_qy)
        {
            Element * e = xtl->getDataElement();
            if (_fIsTest)
            {
                _qy->setContext(xtl, e);
                if (!_qy->hasMoreElements())
                {
                    break;
                }
            }
            else if (!_qy->contains(xtl, null, e))
            {
                break;
            }
        }

        if (_script)
        {
            // Don't eval any script until the document is fully downloaded - perf cache download status in xsl
            xtl->checkDocLoaded();

            V_VT(&var) = VT_EMPTY;
            _se->parseScriptText(xtl, _script, VT_BOOL, &var);
            if (V_BOOL(&var) == VARIANT_FALSE)
            {
                break;
            }
        }

        push(xtl);

        // Push an execution frame for the children

        if (xtl->pushAction(Processor::ENUMELEMENTS, Processor::CURRENTDATA) == Processor::ACTION_OK)
        {
            break;
        }

        // fall through

    default:
        pop(xtl);

        if (_fIsWhen)
        {
            xtl->exitTemplate();
        }
    }
}


/*  ----------------------------------------------------------------------------
    compile()

    Compile the children elements of this action into actions.
*/

bool
IfAction::compile(Processor * xtl, String * defaultLang)
{
    Element * e = getElement();
    String * s = (String *) e->getAttribute(XTLKeywords::s_nmExpr);
    if (s)
    {
        _se = xtl->getScriptEngine(getLanguage(defaultLang));
        _script = s;
    }

    return super::compile(xtl, defaultLang);
}


void
IfAction::compileQuery(Processor * xtl, String * language)
{
    // BUGBUG - throw an exception if "test" and "match" are both specified

    _qy = xtl->compileQuery(getElement(), XTLKeywords::s_nmTest);

    if (_qy)
    {
        _fIsTest = true;
    }

    super::compileQuery(xtl, language);
}


/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
