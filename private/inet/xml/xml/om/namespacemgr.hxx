/*
 * @(#)NamespaceMgr.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _XML_DOM_NAMESPACEMGR
#define _XML_DOM_NAMESPACEMGR

DEFINE_CLASS(Scope);
DEFINE_CLASS(NamespaceMgr);
DEFINE_CLASS(Hashtable);
DEFINE_CLASS(Vector);
DEFINE_CLASS(Document);

//////////////////////////////////////////////////////////////////////////
// The name, as sepecified in the src document
//////////////////////////////////////////////////////////////////////////
class NameDef : public Base
{
friend class NamespaceMgr;

    DECLARE_CLASS_MEMBERS(NameDef, Base);
    DECLARE_CLASS_INSTANCE(NameDef, Base);

    // pURN defaults to the URN from the Name
    NameDef(Name * pName, Atom * pPrefix, Atom * pSrcURN = null);

public:

    //
    // Create NameDef objects
    //
    static NameDef * newNameDef(String * pstrName, 
                                Atom * pURN = null, Atom * pSrcURN = null, 
                                Atom * pPrefix = null);

    static NameDef * newNameDef(const TCHAR * pch, ULONG ulLen, 
                                Atom * pURN = null, Atom * pSrcURN = null, 
                                Atom * pPrefix = null);

    static NameDef * newNameDef(Name * pName, 
                                Atom * pSrcURN = null,
                                Atom * pPrefix = null);

    Name * getName()
    {
        return (Name*)_pName;
    }

    Atom * getPrefix()
    {
        return _pAtomPrefix;
    }

    Atom * getSrcURN()
    {
        return _pSrcURN;
    }

    bool equals(Object *);
    String * toString();
    int hashCode();

protected:

#if DBG == 1
    static long created;
    static long reused;
#endif

    RName       _pName;             // Reference to name
    RAtom       _pAtomPrefix;       // prefix
    RAtom       _pSrcURN;           // the src format of the URN

protected:

    virtual void finalize()
    {
        _pName = NULL;
        _pAtomPrefix = NULL;
        _pSrcURN = NULL;
        super::finalize();
    }
};


//////////////////////////////////////////////////////////////////////////
// "Manages" namespaces for a specific document
//////////////////////////////////////////////////////////////////////////
class NamespaceMgr : public Base
{
    DECLARE_CLASS_MEMBERS(NamespaceMgr, Base);

protected:
    NamespaceMgr(bool fHash = true);
    ~NamespaceMgr() {}

public:
    static NamespaceMgr* newNamespaceMgr(bool fHash = true);

public:
    // methods used by the NameSpaceNodeFactory during parsing 
    // to keep track of prefix -> URN mappings

    // push a new scope
    void pushScope(Node * pNode);

    // push a new scope
    void pushScope(Atom * pPrefix, Atom * pURN, Atom * pSrcURN, PVOID pContext);

    void changeContext(PVOID pOldContext, PVOID pNewContext);

    // pop all scopes with the given context (
    void popScope(PVOID pContext);

    // pop the top scope
    void popScope()
    {
        if (_cSnum > 0)
        {
            deleteScope((Scope *)_pScopes->elementAt(_cSnum - 1));
        }
    }

public:
    // look up URN for predefined prefixes (xml:* and xmlns:*)
    static Atom * findGlobalURN(Atom * pPrefix);

    // given an prefix, find the URN which applies
    Atom * findURN(const WCHAR* pwcText, ULONG ulLen, bool * pboolReserved = null, Atom ** ppSrcURN = null);
    Atom * findURN(Atom * pPrefix, PVOID * ppContext, Atom ** ppSrcURN = null);

    // given a URN, find the prefix
    bool findPrefix(Atom * pURI, Atom ** ppPrefix);

public:
    // parse the given string as given attribute type 
    // (with extended type for normal name)
    // !!! note: this doesn't handle DataTypes > DT_AV_NAMEDEF !!!
    Object * parseNames(DataType type, String* s);

public:
    NameDef * createNameDef(String* name, Atom * pURN = null, Atom * pSrcURN = null, Atom * pPrefix = null);

    NameDef * createNameDef(Name * name, Atom * pSrcURN = null, Atom * pPrefix = null)
    {
        Assert(name);
        return createNameDef(name->getName()->toString(), 
                             name->getNameSpace(), pSrcURN ? pSrcURN : name->getNameSpace(), 
                             pPrefix);
    }

    // create a nameDef (associated with this NSMgr) from the given namedef
    NameDef * createNameDef(NameDef* namedef, Atom * pAtomURN, Atom * pSrcURN)
    {
        Assert(namedef);
        return createNameDef(namedef->getName()->getName()->toString(), pAtomURN, pSrcURN, namedef->getPrefix());
    }

    NameDef * createNameDef(const WCHAR* pwcText, ULONG ulLen, ULONG ulNamespaceLen, bool fDefaultNS, Atom * pAtomURN = null, Atom * pSrcURN = null, bool fURNSpecified = false);
    NameDef * createNameDef(const WCHAR* bstrTagName, const WCHAR* bstrURN);

    NameDef * createNameDefOM(const WCHAR* bstrTagName, bool fUsePrefix = true);

    // This method is used by IE4 object model only
    NameDef * createNameDef(const WCHAR* pwcText, ULONG ulLen);

    void reset();

    Object * clone();

    // used to check for "reserved" namespace prefixes
    static
    bool isReservedNameSpace(const TCHAR *pwcText, int iLen)
    {
        return (iLen >= 3) && (0 == StrCmpNI(_T("xml"), pwcText, 3));
    }

    // used to check for "reserved" namespace prefixes
    static
    bool isReservedNameSpace(Atom * pNS)
    {
        return pNS->hasPrefixI(_T("xml"), 3);
    }

    static Atom * CanonicalURN(Atom *);

protected:
    void deleteScope(Scope * pScope);

protected:

    RStringHashtable _pNameDefs;  // nameDef hashtable
	WCHAR * _pwchBuffer;          // buffer containing chars for creating namedef 
    ULONG _ulLen;                 // length of _pwchBuffer

    RScope _pScopeDefault;        // pointer to default scope link 
    RVector _pScopes;             // scope stack
    int _cSnum;                   // number of scopes
    bool _fHash;

#if DBG == 1
    long created;
    long reused;
#endif

protected:

    virtual void finalize()
    {
        _pNameDefs = null;
        _pScopes = null;
        _pScopeDefault = null;
        if (_pwchBuffer)
            delete [] _pwchBuffer;
        super::finalize();
    }
};

#endif _XML_DOM_NAMESPACEMGR