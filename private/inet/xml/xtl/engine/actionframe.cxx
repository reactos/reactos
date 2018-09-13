/*
 * @(#)ActionFrame.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XTL ActionFrame object
 * 
 */

#include "core.hxx"
#pragma hdrstop


#include "processor.hxx"
#include "action.hxx"

/*  ----------------------------------------------------------------------------
    ActionFrame()

    Protected constructor

    @param strName  -   query's node name
    @param enParent -   parent enumeration

*/

Processor::ActionResult
ActionFrame::init(Processor * xtl, Action * action, Element * eXTLParent, Processor::EnumAction enumAction)
{
    Assert((action && !eXTLParent) || (!action && eXTLParent));

    _state = 0;
    _action = action;
    _eXTLParent = eXTLParent;
    _enumAction = enumAction;

    if (_action)
    {
        _eXTL = action->getElement();
    }
    else
    {
        _eXTL = firstElement();
        if (!_eXTL)
        {
            return Processor::ACTION_NO_ELEMENTS;
        }

        _action = xtl->getAction(_eXTL);
    }

    return Processor::ACTION_OK;
}


void 
ActionFrame:: operator=( const ActionFrame & actframe)
{
    _eXTLParent = actframe._eXTLParent;
    _eXTL = actframe._eXTL;
    _hXTL = actframe._hXTL;
    _action = actframe._action;
    _state = actframe._state;
    _flags = actframe._flags;
    _name = actframe._name;
}

/*  ----------------------------------------------------------------------------
    ~ActionFrame()

    Release all references to other objects.

*/

ActionFrame::~ActionFrame()
{
    _eXTLParent = null;
    _eXTL = null;
    _action = null;
    _name = null;
}

/*  ----------------------------------------------------------------------------
    execute()
*/

bool
ActionFrame::execute(Processor * xtl)
{
    if (IsTagEnabled(tagXTLProcessor))
    {
        // This throws E_PENDING assertion in debug build.
      TraceTag((tagXTLProcessor, "state = %d code<%x> = %s  data = %s", 
        _state, 
        _eXTL,
        (char *) AsciiText(getElementString(_eXTL)),
        (char *) AsciiText(getElementString(xtl->getDataElement()))));
    }

    _action->execute(_state, xtl);

    // BUGBUG - can this be done in processor instead of here?
    // BUGBUG - isn't it enough to null out the current data element?

    // Reset processor flag to fetch data on next instruction

    xtl->setNextData(false);

    if (_state == 0)
    {
        if (!_eXTLParent)
        {
            return true;
        }

        _eXTL = nextElement();

        if (!_eXTL)
        {
            return true;
        }

        _action = xtl->getAction(_eXTL);
    }

    return false;
}


Element * 
ActionFrame::firstElement() 
{ 
    Element * eXTL = null;

    switch (_enumAction)
    {
        case Processor::ENUMELEMENTS:
            eXTL = _eXTLParent->getFirstChild(&_hXTL);
            if (eXTL && eXTL->getType() == Element::COMMENT)
            {
                eXTL = nextElement();
            }
            break;

        case Processor::ENUMATTRIBUTES:
            eXTL = _eXTLParent->getFirstAttribute(&_hXTL);
            break;
    }

    return eXTL;
}


Element * 
ActionFrame::nextElement() 
{ 
    Element * eXTL = null;

    switch (_enumAction)
    {
        case Processor::ENUMELEMENTS:
            while (true)
            {
                eXTL = _eXTLParent->getNextChild(&_hXTL);
                if (!eXTL || eXTL->getType() != Element::COMMENT)
                {
                    break;
                }
            }
            break;

        case Processor::ENUMATTRIBUTES:
            eXTL = _eXTLParent->getNextAttribute(&_hXTL);
            break;
    }

    return eXTL;
}


#if DBG == 1
/**
 * Retrieves the string representation of this object.
 * @return a string representing the actionframe.
 */

String * 
ActionFrame::toString()
{
    return String::newString(_T("ActionFrame"));
}
#endif

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
