/*
 * @(#)NodeNameAction.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XTL NodeNameAction object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "xtlkeywords.hxx"
#include "processor.hxx"
#include "nodenameaction.hxx"


DEFINE_CLASS_MEMBERS(NodeNameAction, _T("GetItemNodeNameAction"), NodeNameAction);


/*  ----------------------------------------------------------------------------
    newNodeNameAction()

    Public "static constructor"

    @param strName  -   query's node name
    @param enParent -   parent enumeration

    @return         -   Pointer to new NodeNameAction object
*/
NodeNameAction *
NodeNameAction::newNodeNameAction(Element * e)
{
    return new NodeNameAction(e);
}        


/*  ----------------------------------------------------------------------------
    execute()
*/

void
NodeNameAction::execute(int state, Processor * xtl)
{
    // BUGBUG - perf - Once item is fetched, remember it because getText can throw an E_PENDING
    // exception.  This code refetches the data until e->getText() succeeds.

    Element * e = getData(xtl);

    if (e)
    {
        NameDef * nm = e->getNameDef();
        if (nm)
        {
            xtl->beginElement(Element::PCDATA, null, nm->toString());
            xtl->endElement();
        }
    }
}


/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
