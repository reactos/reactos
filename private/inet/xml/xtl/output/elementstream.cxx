/*
 * @(#)ElementStream.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of ElementStream object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "xtl/engine/xtlkeywords.hxx"
#include "xtl/engine/xtlprocessorevents.hxx"
#include "elementstream.hxx"

DEFINE_CLASS_MEMBERS(ElementStream, _T("ElementStream"), ElementStream);

DeclareTag(tagXTLOutput,  "XTLOutput", "Trace XTLProcessor output");

//
// BUGBUG - This file should be deleted once the new NodeFactory is ready.
// 

ElementStream *
ElementStream::newElementStream(OutputHelper * out)
{
    return new ElementStream(out);
}


ElementStream::ElementStream(OutputHelper * out)
{
    EnableTag(tagXTLOutput, TRUE);

    _out = out;
}


void
ElementStream::finalize()
{
    close();
}


void
ElementStream::beginElement(int type, String * name, String * text, bool fNoEntities)
{
    int c;
    TCHAR ch;
    const ATCHAR * chars;
    const TCHAR * pc;
    
    switch(type)
    {
    case Element::DOCTYPE: 
        _out->write(_T("<!DOCTYPE "));
        _fDontExpandEntities++;
        break;

    case Element::PI:
        _fDontExpandEntities++;
    	_out->write(_T("<?"));
        _out->write(name);
        _out->write(_T(" "));
        if (text)
        {
            _out->write(text);
        }
        break;

    case Element::ELEMENT:
    	_out->write(_T("<"));
        _out->write(name);
        break;

    case Element::ATTRIBUTE: 
        _out->write(_T(" "));
	    _out->write(name);
        // BUGBUG - Must figure out if attribute should be single or double quoted
        _out->write(_T("=\""));
        _fIsAttr++;
        break;

    case Element::PCDATA:
        if (text)
        {
            if (_fDontExpandEntities || fNoEntities)
            {
                _out->write(text);
            }
            else
            {
                _out->writeString(text, !_fIsAttr);
            }
        }
        break;

    case Element::CDATA:
        _fDontExpandEntities++;
        _out->write(_T("<![CDATA["));
        if (text)
        {
            _out->write(text);
        }
        break;

    case Element::ENTITYREF:
        _out->write(_T("&"));
        _out->write(name);
        break;

    case Element::COMMENT:
        _fDontExpandEntities++;
        _out->write(_T("<!--"));
        // If the element is a comment then the text is inside the comment.  If this
        // is an <xsl:comment> then the text will follow as a PCDATA child.
        if (text)
        {
            _out->write(text);
        }
        break;
    }
}


void
ElementStream::beginChildren(int parentType, bool hasWSInside)
{
    if (parentType == Element::ELEMENT)
    {
       	_out->write(_T(">"));

        if (hasWSInside /* && type != Element::PCDATA */)
        {
            _out->writeNewLine();
        }
    }
}


void
ElementStream::endElement(int type, String * name, bool hasChildren, bool hasWSAfter)
{
    switch(type)
    {
    case Element::DOCTYPE:
        _out->write(_T(">"));
        _fDontExpandEntities--;
        break;

    case Element::PI:
        _fDontExpandEntities--;        
    	_out->write(_T("?>"));
        break;

    case Element::ELEMENT:
        if (!hasChildren)
        {
        	_out->write(_T(" />"));
        }
        else
        {
            _out->write(_T("</"));
            _out->write(name);
            _out->write(_T(">"));
        }
        break;

    case Element::ATTRIBUTE: 
        _out->write(_T("\""));
        // BUGBUG - This will cause all attributes to be on the same line.  There is a problem with the <xsl:attribute> element when it is the 
        // last attribute and the first child element is on the next line.  It causes a LF before the end of the parent element.
        hasWSAfter = false;
        _fIsAttr--;
        break;

    case Element::COMMENT:
        _fDontExpandEntities--;        
        _out->write(_T("-->"));
        break;

    case Element::CDATA:
        _fDontExpandEntities--;        
        _out->write(_T("]]>"));
        break;

    case Element::ENTITYREF:
        _out->write(_T(";"));
        break;
    
    /* 
        This will cause all other types to never be followed by WS.
    default:
        hasWSAfter = false;
    */
    }

    if (hasWSAfter)
    {
        _out->writeNewLine();
    }
};

void
ElementStream::close()
{
    if (_out)
    {
        _out->close();
        _out = null;
    }
}        


#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
ElementStream::toString()
{
    return String::newString(_T("ElementStream"));
}
#endif

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
