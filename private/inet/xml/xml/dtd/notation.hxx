/*
 * @(#)Notation.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _NOTATION_HXX
#define _NOTATION_HXX

#ifndef _XML_OM_ELEMENT
#include "xml/om/element.hxx"
#endif

#ifndef _CORE_UTIL_NAME
#include "core/util/name.hxx"
#endif

#ifndef _XML_OUTPUTHELPER
#include "xml/util/outputhelper.hxx"
#endif

typedef _reference<IUnknown> RUnknown;

DEFINE_CLASS(Notation);

/**
 * This class implements an entity object representing an XML notation.
 *
 * @version 1.0, 6/3/97
 */
class Notation : public Base
{
friend class Context;
friend class ElementDecl;
friend class DTD;
friend class ContentModel;
friend class Entity;
friend class Notation;
friend class AttDef;

    DECLARE_CLASS_MEMBERS(Notation, Base);

    public: Notation(Name * name)
    {
        this->name = name;
    }

    private: void setURL(String * url)
    {
        this->url = url;
    }

    /**
     * Name * of Notation
     */
    public: RName name;

    /**
     * Url 
     */
    public: RString url;

    /**
     * pubid
     */
    public: RString pubid; 

    /**
     * Type of notation (Parser.SYSTEM or Parser.PUBLIC)
     */
    public: int type;


	//
	// BUGBUG: HACKHACK: Need to break circular references, in notation.hxx, attdef.hxx, and entity.hxx
    // This used to be a smart pointer (RObject _pNode;)
	// Have to change it this way because currently Node::Addref() addref-s the document when 
	// the reference count goes to 2 (thinking we are handing it out). In the situation we addref it 
	// up to 2 internally, we have circular references 
	//
    Object* _pNode;

    protected: virtual void finalize()
    {
        name = null;
        url = null;
        pubid = null;
        _pNode = null;
        super::finalize();
    }
};

#endif _NOTATION_HXX
