/*
 * @(#)GetItemAction.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XTL GetItemAction object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "xtlkeywords.hxx"
#include "processor.hxx"
#include "getitemaction.hxx"


DEFINE_CLASS_MEMBERS(GetItemAction, _T("GetItemGetItemAction"), Action);


/*  ----------------------------------------------------------------------------
    newGetItemAction()

    Public "static constructor"

    @param strName  -   query's node name
    @param enParent -   parent enumeration

    @return         -   Pointer to new GetItemAction object
*/
GetItemAction *
GetItemAction::newGetItemAction(Element * e)
{
    return new GetItemAction(e);
}        


/*  ----------------------------------------------------------------------------
    GetItemAction()

    Protected constructor

    @param strName  -   query's node name
    @param enParent -   parent enumeration

*/
GetItemAction::GetItemAction(Element * e) : Action(e)
{
}


void 
GetItemAction::finalize()
{
    _qy = null;
    super::finalize();
}


/*  ----------------------------------------------------------------------------
    execute()
*/

void
GetItemAction::execute(int state, Processor * xtl)
{
    // BUGBUG - perf - Once item is fetched, remember it because getText can throw an E_PENDING
    // exception.  This code refetches the data until e->getText() succeeds.

    Element * e = getData(xtl);

    xtl->beginElement(Element::PCDATA, null, e ? e->getText() : String::emptyString());
    xtl->endElement();
}


Element * 
GetItemAction::getData(Processor * xtl)
{
    Element * e = xtl->getDataElement();

    if (_qy)
    {
        _qy->setContext(xtl, e);
        e = (Element *) _qy->peekElement();
    }

    return e;
}


bool 
GetItemAction::compile(Processor * xtl, String * language)
{
    _qy = xtl->compileQuery(getElement(), XTLKeywords::s_nmSelect);
    return checkEmpty();
}


#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
GetItemAction::toString()
{
    return super::toString();
}
#endif

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
