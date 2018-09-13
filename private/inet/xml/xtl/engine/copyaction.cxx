/*
 * @(#)CopyAction.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XTL CopyAction object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "xtlkeywords.hxx"
#include "processor.hxx"
#include "copyaction.hxx"

extern Element::NodeType aXMLType2NT[];

DEFINE_CLASS_MEMBERS(CopyAction, _T("CopyAction"), Action);


/*  ----------------------------------------------------------------------------
    newCopyAction()

    Public "static constructor"

    @param strName  -   query's node name
    @param enParent -   parent enumeration

    @return         -   Pointer to new CopyAction object
*/
CopyAction *
CopyAction::newCopyAction(Element * e, Processor::ElementSource eSource, int type)
{
    return new CopyAction(e, eSource, type);
}        


CopyAction *
CopyAction::newCopyAction(Element * e, int type)
{
    return new CopyAction(e, Processor::DATAELEMENT, type);
}        


/*  ----------------------------------------------------------------------------
    CopyAction()

    Protected constructor

    @param strName  -   query's node name
    @param enParent -   parent enumeration

*/
CopyAction::CopyAction(Element * e, Processor::ElementSource eSource, int type) : Action(e)
{
    _eSource = eSource;
    setType(type);
}


void 
CopyAction::finalize()
{
    _name = null;
    super::finalize();
}


/*  ----------------------------------------------------------------------------
    execute()
*/


// BUGBUG - TemplateAction can now perform a copy - Consider removing CopyAction.   Downside is that a templateaction is
// larger than a copyaction and it may execute a little slower.  

void
CopyAction::execute(int state, Processor * xtl)
{
    switch(state)
    {
    case 0:        
        // Copy the current input element
        xtl->beginElement(_eSource, getType(), _name);

        // fall through

    case Processor::PROCESS_ATTRIBUTES:
        // Push a frame that copies the attributes

        if (_eSource == Processor::CODEELEMENT && 
            xtl->pushAction(Processor::ENUMATTRIBUTES, Processor::CURRENTDATA, _eSource) == Processor::ACTION_OK)
        {
            break;
        }
        else
        {
            // Move to next state

            xtl->setState(Processor::PROCESS_CHILDREN);
        }

        // fall through
    
    case Processor::PROCESS_CHILDREN:
        // Push a frame that copies the children

        if (xtl->pushAction(Processor::ENUMELEMENTS, Processor::CURRENTDATA, _eSource) == Processor::ACTION_OK)
        {
            break;
        }

        // fall through 

    default:
        xtl->endElement();
    }
}

/*  ----------------------------------------------------------------------------
    compile()

    Compile the children elements of this action into actions.
*/

bool
CopyAction::compile(Processor * xtl, String * language)
{
    _name = CAST_TO(String *, getElement()->getAttribute(XTLKeywords::s_nmName));
    return false;
}



#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
CopyAction::toString()
{
    return String::newString(_T("CopyAction"));
}
#endif

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
