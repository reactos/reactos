/*
 * @(#)Entity.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _ENTITY_HXX
#define _ENTITY_HXX

#ifndef _CORE_UTIL_NAME
#include "core/util/name.hxx"
#endif

#ifndef _CORE_UTIL_ATOM
#include "core/util/atom.hxx"
#endif

#ifndef _XML_OUTPUTHELPER
#include "xml/util/outputhelper.hxx"
#endif

#ifndef _XML_OM_ELEMENT
#include "xml/om/element.hxx"
#endif

#ifndef _CORE_UTIL_ENUMWRAPPER
#include "core/util/enumwrapper.hxx"
#endif

#ifndef _CORE_UTIL_ENUMERATION
#include "core/util/enumeration.hxx"
#endif

#ifndef _CORE_UTIL_VECTOR
#include "core/util/vector.hxx"
#endif

DEFINE_CLASS(Entity);

/**
 * This class implements an <code>Entity</code> object representing an XML internal 
 * or external entity as defined in the XML Document Type Definition (DTD).
 *
 * @version 1.0, 6/3/97
 */
class Entity : public Base
{
friend class Context;
friend class ElementDecl;
friend class DTD;
friend class ContentModel;
friend class Entity;
friend class Notation;
friend class AttDef;

    DECLARE_CLASS_MEMBERS(Entity, Base);
    DECLARE_CLASS_INSTANCE(Entity, Base);

    public: Entity(Name * name, bool par);

    private: Entity(Name * name, bool par, String * text);
    
    private: Entity(Name * name, bool par, int c);
    
    private: set(Name * name, bool par);

    public: void setURL(String * url);
   
    public: String * getURL()
    {
        return url;
    }

    public: void setNDATA(Name * name)
    {
        ndata = name;
    }

    public: bool isParsed()
    {
        return _fParsed;
    }

    public: void setParsed(bool parsed)
    {
        _fParsed = parsed;
    }

    /**
    * Changes the text of entity.
    * @param text  The new text of the entity.
    */
    public: virtual void setText(String * text);

    public: String* getText() { return text; }
    
    public: void setPosition(ULONG line, ULONG column);

    private: int getLength();

    private: TCHAR getChar(int index);

   /**
    * Retrieves the name of the entity.
    * @return the <code>Name</code> object containing the entity name.
    */
    public: virtual Name * getName()
    {
        return name;
    }

    /**
     * Name * of entity
     */
    RName name;

    /**
     * Url for external entity
     */
    RString url;

    /**
     * Pubid for external entity
     */
    RString pubid;

    /**
     * Text for internal entity
     */
    RString text;

    /**
     * Char for internal entity
     */
    TCHAR cdata;

    /**
     * NDATA identifier
     */    
    RName ndata;

    /**
     * line number
     */    
    ULONG line;
    
    /**
     * character pos
     */    
    ULONG column;
    
    /**
     * set if paramater entity
     */    
    bool par;
    
    /**
     * set if external entity
     */    
    bool sys;

    /**
     * whether entity is being validated. (infinite recurrsion check)
     */
    bool validating;

    //
	// BUGBUG: HACKHACK: Need to break circular references, in notation.hxx, attdef.hxx, and entity.hxx
    // This used to be a smart pointer (RObject _pNode;)
	// Have to change it this way because currently Node::Addref() addref-s the document when 
	// the reference count goes to 2 (thinking we are handing it out). In the situation we addref it 
	// up to 2 internally, we have circular references 
	//
    Object* _pNode;

    bool _fParsed;

    protected: virtual void finalize()
    {
        name = null;
        url = null;
        pubid = null;
        text = null;
        ndata = null;
        _pNode = null;
        super::finalize();
    }
};


#endif _ENTITY_HXX

