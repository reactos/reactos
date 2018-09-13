/*
 * @(#)ContentModel.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _CONTENTMODEL_HXX
#define _CONTENTMODEL_HXX

#ifndef _XML_OM_ELEMENT
#include "xml/om/element.hxx"
#endif

#ifndef _CORE_UTIL_NAME
#include "core/util/name.hxx"
#endif

#ifndef _CORE_UTIL_ATOM
#include "core/util/atom.hxx"
#endif

#ifndef _CORE_LANG_STRING
#include "core/lang/string.hxx"
#endif

#ifndef _CORE_UTIL_VECTOR
#include "core/util/vector.hxx"
#endif

#ifndef _CORE_UTIL_HASHTABLE
#include "core/util/hashtable.hxx"
#endif

#ifndef _CORE_UTIL_ENUMERATION
#include "core/util/enumeration.hxx"
#endif

#ifndef _XML_OUTPUTHELPER
#include "xml/util/outputhelper.hxx"
#endif

#ifndef _CORE_UTIL_STACK
#include "core/util/stack.hxx"
#endif

class DTDState;

DEFINE_CLASS(ContentModel);
DEFINE_CLASS(ContentNode);
DEFINE_CLASS(Terminal);

/** 
 * This class represents the content model definition for a given
 * XML element. The content model is defined in the element
 * declaration in the Document Type Definition (DTD); for example, 
 * (a,(b|c)*,d). The
 * content model is stored in an expression tree of <code>ContentNode</code> objects
 * for use by the XML parser during validation.
 */
class ContentModel: public Base
{
friend class ElementDecl;
friend class DTD;
friend class ContentModel;
friend class Entity;
friend class Notation;
friend class AttDef;
friend class Terminal;
friend class DTDNodeFactory;

    DECLARE_CLASS_MEMBERS(ContentModel, Base);

    public: enum
    {
        EMPTY       = 1,
        ANY         = 2,
        ELEMENTS    = 4,
    };

public:

    void initContent(DTDState* context);
    void checkContent(DTDState* context, Name* name, DWORD type);
    bool allowAny() { return _nType == ANY; }

public:
    ContentModel()
    {
        _fOpen = false;
        _fMixed = false;
    }

    inline void setOpen(bool flag) { _fOpen = flag; }
    inline bool isOpen() { return _fOpen; }
    inline bool isMixed() const { return _fMixed; }
    inline byte getType() const { return _nType; }
    inline void setType(byte type) { _nType = type; }
    inline bool isEmpty() const { return _nType == EMPTY; }

    Enumeration* getSymbols(); // return enumeration of Name objects for all symbols in content model.

    String * toString();

    void    start();
    void    finish();

    /**
     * is true if n is repeatable
     */
    bool isRepeatable(Name * n);
    
    //--------------------------------------------------------------------------------
    // methods for building a new content model            
    void    openGroup();
    void    addTerminal(Name* n);
    void    addChoice();
    void    addSequence();
    void    closeGroup();
    void    star();
    void    plus();
    void    questionMark();
    void    closure(int type);

    /**
     * this parses through content tree, sees if any instance of n is contained in PLUS or STAR
     */
    private: bool isRepeatable(ContentNode * pCN,Name * n);
                            
    /**
     * check whether the content model allows empty content
     */
    private: bool acceptEmpty();

    /**
     *  returns names of all legal elements following the specified state
     */
    private: Vector * expectedElements(int state);

private:

    bool _fOpen; 
    bool _fMixed;

    RHashtable _pTable; // table for checking duplicates in mixed content model.

    /**
     * content model
     * points to syntax tree 
     * @see ContentNode
     */    
    RContentNode _pContent;

    /**
     * end node
     */
    RTerminal _pEnd;

    /** 
     * terminal nodes
     */
    RVector _pTerminalNodes;

    /**
     * unique terminal names
     */
    RHashtable _pSymbolTable;

    /**
     * symbol array
    */
    RVector _pSymbols;

    /**
     * transition table
     */
    RVector _pDtrans;

    /**
     * content type
     */    
    byte _nType;

    bool _fPartial;  // whether the closure applies to partial or the whole node that is on top of the stack

    RStack _pStack;  // parsing context

    protected: virtual void finalize()
    {
        _pContent = null;
        _pEnd = null;
        _pTerminalNodes = null;
        _pSymbolTable = null;
        _pSymbols = null;
        _pDtrans = null;
        _pStack = null;
        _pTable = null;
        super::finalize();
    }
};


#endif _CONTENTMODEL_HXX
