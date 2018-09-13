/*
 * @(#)MethodOperand.cxx 1.0 6/26/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XQL MethodOperand object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "methodoperand.hxx"

// BUGBUG - Add this to header file
int
_NodeType2DOMNodeType(Element::NodeType eNodeType);


DEFINE_CLASS_MEMBERS(MethodOperand, _T("MethodOperand"), BaseOperand);

/*  ----------------------------------------------------------------------------
    newMethodOperand()

    Public "static constructor"

    @param opnd -   l-value

    @return      -   Pointer to new MethodOperand object
*/
MethodOperand *
MethodOperand::newMethodOperand(Query * qy, MethodType mt)
{
    return new MethodOperand(qy, mt);
}        


/*  ----------------------------------------------------------------------------
    MethodOperand()

    Protected constructor

    @param opnd -   l-value

*/
MethodOperand::MethodOperand(Query * qy, MethodType mt)
{
    _mt = mt;
    _qy = qy;
}


void
MethodOperand::finalize()
{
    _qy = null;
    super::finalize();
}

   
TriState 
MethodOperand::isTrue(QueryContext *inContext, Query * qyContext, Element * eContext)
{
    TriState triResult;
    OperandValue opval;

    getValue(inContext, qyContext, eContext, &opval);

    // BUGBUG - Should get the value?  For example, if the method is DATE(value()) then
    // the result is true only if the value exists and it is a DATE.

    triResult = TriStateFromBool(!opval.isEmpty());

    return triResult;
}

/*  ----------------------------------------------------------------------------
    getValue()

*/
void
MethodOperand::getValue(QueryContext *inContext, Query * qyContext, Element * eContext, OperandValue * popval)
{

    NameDef * namedef;
    bool isEnd;

    if (_qy)
    {
        _qy->setContext(inContext, eContext);
        eContext = (Element *) _qy->peekElement();
        if (eContext == null)
        {
            return;
        }
        qyContext = _qy;
    }

    if (null != eContext)
    {
        switch (_mt)
        {
	    case TEXT:
            // Datatype is STRING
            popval->init(DT_STRING, eContext);
            break;

        case VALUE:
            // Datatype is data type of element
            popval->init(getDT(), eContext);
            break;

        case NODETYPE:
            // BUGBUG should be an int, but only R8's are supported
            // BUGBUG - stop mapping NodeType to ElementType to DOMNodeType
            popval->initR8(_NodeType2DOMNodeType((Element::NodeType) eContext->getType()));
            break;

        case NODENAME:
            // BUGBUG - This returns prefix:gi. It is not namespace safe.
            // BUGBUG - perf - This allocates a String 

            namedef = eContext->getNameDef();
            if (!namedef)
            {
                break;
            }

            popval->initString(namedef->toString());
            break;

        case INDEX:
            if (qyContext)
            {
                popval->initR8(qyContext->getIndex(inContext, eContext));
            }
            break;

        case END:
            // NOTE we signal boolean false by setting variant type to empty
            // any other type represents true

            if (qyContext)
            {
                isEnd = qyContext->isEnd(inContext, eContext);
                if (isEnd)
                {
                    popval->initBOOL(isEnd);
                }
            }
            break;
        }
    }

    return;
}


#if DBG == 1
/**
 * Retrieves the string representation of this object.
 */
String * 
MethodOperand::toString()
{

    switch (_mt)
    {
    case TEXT:
        return String::newString(_T("text"));

    case VALUE:
        return String::newString(_T("value"));

    case NODETYPE:
        return String::newString(_T("nodeType"));

    case NODENAME:
        return String::newString(_T("nodeName"));

    case INDEX:
        return String::newString(_T("index"));

    case END:
        return String::newString(_T("end"));
    }

    return String::newString(_T("Invalid MethodOperand!!!"));
}
#endif


/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
