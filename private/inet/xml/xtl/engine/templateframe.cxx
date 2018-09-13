/*
 * @(#)TemplateFrame.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XTL TemplateFrame object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "processor.hxx"
#include "copyaction.hxx"
#include "templateaction.hxx"
#include "templateframe.hxx"


void 
TemplateFrame:: operator=( const TemplateFrame & tmplframe)
{
    _template = tmplframe._template;
    _ip = tmplframe._ip;
    _action = tmplframe._action;
    _qy = tmplframe._qy;
    _eData = tmplframe._eData;
}



TemplateFrame::~TemplateFrame()
{
    _template = null;
    _action = null;
    _qy = null;
    _eData = null;
}


void
TemplateFrame::init(QueryContext * inContext, TemplateAction * tmpl, Element * eData)
{
    _template = tmpl;
    resetActions();
    _qy = tmpl->getSelectQuery();

    if (_qy)
    {
        _qy->setContext(inContext, eData);
        _eData = null;
    }
    else
    {
        _eData = eData;
    }
}


void
TemplateFrame::resetActions()
{
    _ip = 0;
    _action = _template->getAction(0);
}


/*  ----------------------------------------------------------------------------
    getAction()
*/

Action *
TemplateFrame::getAction(Element * e)
{
    Action * action;

    if (_action && _action->matches(e))
    {
        action = _action;
        _ip++;
        _action = _template->getAction(_ip);
    }
    else
    {
        action = Processor::GetDefaultAction();
    }

    return action;
}


#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
TemplateFrame::toString()
{
    return String::newString(_T("TemplateFrame"));
}
#endif

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
