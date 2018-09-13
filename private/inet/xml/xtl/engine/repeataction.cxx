/*
 * @(#)RepeatAction.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XTL RepeatAction object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "xql/query/childrenquery.hxx"
#include "xtlkeywords.hxx"
#include "processor.hxx"
#include "repeataction.hxx"


DEFINE_CLASS_MEMBERS(RepeatAction, _T("RepeatAction"), TemplateAction);


/*  ----------------------------------------------------------------------------
    newRepeatAction()

    Public "static constructor"

    @param strName  -   query's node name
    @param enParent -   parent enumeration

    @return         -   Pointer to new RepeatAction object
*/
RepeatAction *
RepeatAction::newRepeatAction(TemplateAction * tmpl, Element * e, Query * qy)
{
    return new RepeatAction(tmpl, e, qy);
}        


/*  ----------------------------------------------------------------------------
    RepeatAction()

    Protected constructor

    @param strName  -   query's node name
    @param enParent -   parent enumeration

*/
RepeatAction::RepeatAction(TemplateAction * tmpl, Element * e, Query * qy) : TemplateAction(tmpl, e, qy)
{
}


/*  ----------------------------------------------------------------------------
    execute()
*/

void
RepeatAction::execute(int state, Processor * xtl)
{
    if (state == 0)
    {
        push(xtl);
    }

    // Push an execution frame for the children

    if (xtl->pushAction(Processor::ENUMELEMENTS, Processor::NEXTDATA) != Processor::ACTION_OK)
    {
        pop(xtl);
    }
}


Query *         
RepeatAction::getSelectQuery()
{
    Query * qy;

    if (isExecuting() && _qy)
    {
        qy = (Query *) _qy->clone();
    }
    else
    {
        qy = _qy;
    }

    return qy;
}


void
RepeatAction::compileQuery(Processor * xtl, String * language)
{
    Query * qy = _qy;
    Element * e = getElement();

    if (!qy)
    {
        qy = xtl->compileQuery(e, XTLKeywords::s_nmSelect);

        if (!qy)
        {
            qy = ChildrenQuery::newChildrenQuery(null, null, null, null, BaseQuery::SCALAR, Element::ANY, false);
        }
    }

    qy = xtl->compileOrderBy(qy, e);

    if ((qy->getFlags() & Query::IS_ELEMENT))
    {
        setForceEndTag(true);
    }

    _qy = qy;
}


#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
RepeatAction::toString()
{
    return super::toString();
}
#endif

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
