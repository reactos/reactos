/*
 * @(#)ElementEnumeration.cxx 1.0 7/11/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#ifndef _XML_OM_ELEMENTENUMERATION
#include "xml/om/elementenumeration.hxx"
#endif

// use ABSTRACT because of no default constructor 
DEFINE_ABSTRACT_CLASS_MEMBERS(ElementEnumeration, _T("ElementEnumeration"), Base);

/**
 * A simple Enumeration for iterating over Element declarations.
 * Returns the XML-DATA specification for each element.
 * This is DTD information represented in an XML Object Model.
 * See <a href="http://www.microsoft.com/standards/xml/xmldata.htm">
 * Specification for XML-Data</a> for details.
 *
 * @version 1.0, 7/11/97
 * @see Element
 * @see Name
 */

/**
 * Creates a new enumerator for enumerating over the immediate children
 * of the given root node that have matching tag names and/or types.
 * @param root The element whose children are going to be enumerated. 
 *
 * @param tag The name of the tag; this parameter can be null if the name is not important. 
 * @param type The element type. <code>Element.ELEMENT</code> is the most common. If the
 * element type is not important, pass -1.
 */

Enumeration * ElementEnumeration::newElementEnumeration(Element * root, Name * tag, bool fEnumAttributes)
{
	ElementEnumeration * en = new ElementEnumeration();
    en->root = root;
    en->tag = tag;
    en->fEnumAttributes = fEnumAttributes;
    en->reset();
	return en;
}

Element * ElementEnumeration::getFirstElement()
{
    Element * e;

    if (fEnumAttributes)
    {
        e = root->getFirstAttribute(&handle);
    }
    else
    {
        e = root->getFirstChild(&handle);
    }

    return e;
}


Element * ElementEnumeration::getNextElement()
{
    Element * e;

    if (fEnumAttributes)
    {
        e = root->getNextAttribute(&handle);
    }
    else
    {
        e = root->getNextChild(&handle);
    }

    return e;
}


Element * ElementEnumeration::getNext()
{
    Element * e;

    if (root != null) 
    {
        if (fGetFirst)
        {
            e = getFirstElement();
            fGetFirst = false;
        }
        else
        {
            e = getNextElement();
        }
        while (e) 
        {
            if (tag == null || e->getTagName() == tag)
                return e;

            e = getNextElement();
        }
    }

    return null;
}
