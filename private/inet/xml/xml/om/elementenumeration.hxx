/*
 * @(#)ElementEnumeration.hxx 1.0 7/11/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _XML_OM_ELEMENTENUMERATION
#define _XML_OM_ELEMENTENUMERATION

#ifndef _CORE_UTIL_NAME
#include "core/util/name.hxx"
#endif

#ifndef _CORE_UTIL_ENUMERATION
#include "core/util/enumeration.hxx"
#endif

#ifndef _XML_OM_ELEMENT
#include "xml/om/element.hxx"
#endif



DEFINE_CLASS(ElementEnumeration);

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

class ElementEnumeration: public Base, public Enumeration
{
    DECLARE_CLASS_MEMBERS_I1(ElementEnumeration, Base, Enumeration);

    private: ElementEnumeration() {};

    /**
     * Creates a new enumerator for enumerating over the immediate children
     * of the given root node that have matching tag names and/or types.
     *
     * @param root The element whose children are going to be enumerated. 
     * @param tag The name of the tag; this parameter can be null if the name is not important. 
     * @param type The element type. <code>Element.ELEMENT</code> is the most common. If the
     * element type is not important, pass -1.
     */
    public: DLLEXPORT static Enumeration * newElementEnumeration(Element * root, Name * tag = null, bool fEnumAttributes = false);

    /**
     * Determines if there are any more matching elements.
     * @return true if the next call to <code>nextElement</code> will return
     * a non-null result.
     */
    public: virtual bool hasMoreElements()
    {
        if (next == null) 
        {
            next = getNext();
        }
        return (next != null) ? true : false;
    }

    /**
     * Retrieves the next matching element.
     * @return <code>Element</code> or null if there are no more matching elements.
     */
    public: virtual Object * peekElement()
    {
        if (next == null) 
        {
            next = getNext();
        }
        return next;
    }


    /**
     * Retrieves the next matching element.
     * @return <code>Element</code> or null if there are no more matching elements.
     */
    public: virtual Object * nextElement()
    {
        if (next == null) 
        {
            return getNext();
        }

        Element * result = next;
        next = null;
        return result;
    }

    /**
     * Resets the iterator so that you can iterate through the elements again.
     * @return No return value.
     */
    public: virtual void reset()
    {
        fGetFirst = true;
        next = null;
    }

    /**
     * Internal method for getting next element.
     */
    Element * getNext();
    Element * getFirstElement();
    Element * getNextElement();

    RElement root;
    RName tag;
    RElement next;
    HANDLE  handle;
    unsigned    fGetFirst:1;
    unsigned    fEnumAttributes:1;

    protected: virtual void finalize()
    {
        root = null;
        tag = null;
        next = null;
        super::finalize();
    }
};


#endif _XML_OM_ELEMENTENUMERATION

