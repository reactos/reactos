/*
 * @(#)NamespaceMgr.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#include "namespacemgr.hxx"
#include "xmlnames.hxx"
#include "document.hxx"

#ifndef _CORE_UTIL_CHARTYPE_HXX
#include "core/util/chartype.hxx"
#endif

#ifdef UNIX
// Needed for _alloca
#include <malloc.h>
#endif // UNIX


//////////////////////////////////////////////////////////////////////////
// This class contains information about a Scope
//////////////////////////////////////////////////////////////////////////
class Scope : public Base
{
    DECLARE_CLASS_MEMBERS(Scope, Base);

friend class NamespaceMgr;

public:
    Atom * getURN() const     { return _pAtomURN; }
    Atom * getSrcURN() const  { return _pSrcURN; }

protected:
    
    Scope(Atom* pPrefix, Atom* pURN, Atom * pSrcURN, PVOID pContext);

protected:

    /**
     * The following information is known about a Scope...
     */
    RAtom _pAtomURN;
    RAtom _pSrcURN;
    RAtom _pAtomPrefix;
    PVOID _pContext;

    RScope _pPrevDefault; // pointing to the previous default scope,if this scope itself is a default scope 
                          // otherwise (when prefix is not null), this pointer is null;

    protected: virtual void finalize()
    {
        _pAtomURN = null;
        _pSrcURN = null;
        _pAtomPrefix = null;
        _pPrevDefault = null;
        super::finalize();
    }
};


DEFINE_ABSTRACT_CLASS_MEMBERS(Scope, _T("Scope"), Base);

Scope::Scope(Atom* pPrefix, Atom* pURN, Atom* pSrcURN, PVOID pContext)
{
    _pAtomPrefix = pPrefix;
    _pAtomURN = pURN;
    _pSrcURN = pSrcURN;
    _pContext = pContext;
    _pPrevDefault = null;
}


///////////////////////////////////////////////////////////////////////
// NameDef implementation
///////////////////////////////////////////////////////////////////////

DEFINE_CLASS_MEMBERS_CLASS(NameDef, _T("NameDef"), Base);

NameDef::NameDef(Name * pName, Atom * pSrcURN, Atom * pPrefix)
{   
    _pName = pName;
    _pSrcURN = pSrcURN ? pSrcURN : _pName->getNameSpace();
    _pAtomPrefix = pPrefix;
}

NameDef * 
NameDef::newNameDef(String * pstrName, Atom * pURN, Atom * pSrcURN, Atom * pPrefix)
{
    return newNameDef(Name::create(pstrName, pURN), pSrcURN, pPrefix);
}

NameDef * 
NameDef::newNameDef(const TCHAR * pch, ULONG ulLen, Atom * pURN, Atom * pSrcURN, Atom * pPrefix)
{
    return newNameDef(Name::create(pch, ulLen, pURN), pSrcURN, pPrefix);
}

NameDef * 
NameDef::newNameDef(Name * pName, Atom * pSrcURN, Atom * pPrefix)
{
    return new NameDef(pName, pSrcURN, pPrefix);
}

bool 
NameDef::equals(Object * that)
{
    bool fResult;
    if (this == that) 
    {
        fResult = true;
    }
    else if (that || getClass() != that->getClass()) 
    {
        fResult = false;
    }
    else
    {
        NameDef * pND = (NameDef *)that;
        fResult = ((_pName == pND->_pName) && (_pAtomPrefix == pND->_pAtomPrefix));
    }
    return fResult;
}


String * 
NameDef::toString()
{
    String *p;
    if (_pAtomPrefix)
    {
        if (_pName->toString()->length() > 0)
        {
            p = String::add(_pAtomPrefix->toString(), String::newString(_T(":")), _pName->toString(), null);
        }
        else
        {
            p = _pAtomPrefix->toString(); // must be xmlns="foo" (bug 44453).
        }
    }
    else if (_pName)
    {
        p = _pName->toString();
    }
    else
    {
        p = String::emptyString();
    }
    return p;
}


int 
NameDef::hashCode()
{
    int cCode =0;
    if (_pName)
        cCode = _pName->hashCode();
    if (_pSrcURN)
        cCode += _pSrcURN->hashCode();
    if (_pAtomPrefix)
        cCode += _pAtomPrefix->hashCode();
    return cCode;
}


///////////////////////////////////////////////////////////////////////////////
// Class NamespaceMgr Implementation
//

// use ABSTRACT because of no default constructor
DEFINE_ABSTRACT_CLASS_MEMBERS(NamespaceMgr, _T("NamespaceMgr"), Base);

DeclareTag(tagNameDef, "NameDef", "NameDef management");

NamespaceMgr::NamespaceMgr(bool fHash)
{
    _cSnum = 0;
    _ulLen = 0;
    _pwchBuffer = null;
#if DBG == 1
    created = 0;
    reused = 0;
#endif
    _fHash = fHash;
    reset();
}

NamespaceMgr* NamespaceMgr::newNamespaceMgr(bool fHash)
{
    return new NamespaceMgr(fHash);
}

void 
NamespaceMgr::pushScope(Atom * pPrefix, Atom * pURN, Atom * pSrcURN, PVOID pContext)
{
    if (!pURN && pSrcURN) pURN = CanonicalURN(pSrcURN);
    if (!pSrcURN && pURN) pSrcURN = pURN;
    Scope * pScope = new Scope(pPrefix, pURN, pSrcURN, pContext);
    _pScopes->addElement(pScope);
    _cSnum++;

    //
    // link default scopes
    //
    if (!pPrefix)
    {
        pScope->_pPrevDefault = _pScopeDefault;
        _pScopeDefault = pScope;
    }
}


void 
NamespaceMgr::pushScope(Node * pNode)
{
    Atom * pBaseName;
    NameDef * pNameDef;
    bool fScope;
    HANDLE  h;
    Node * a = pNode->getNodeFirstAttribute(&h);
    while (a)
    {
        pNameDef = a->getNameDef();
        fScope = (pNameDef->getPrefix() == XMLNames::atomXMLNS);
        pBaseName = pNameDef->getName()->getName();
        if (pBaseName->toString()->length() == 0)
            pBaseName = null;
        if (fScope)
        {
            Atom * pSrcURN = Atom::create(a->getInnerText());
            pushScope(pBaseName, 
                      null,
                      pSrcURN, 
                      (void *)pNode);
        }
        a = pNode->getNodeNextAttribute(&h);
    }
}


void
NamespaceMgr::popScope(PVOID pContext)
{
    while (_cSnum > 0)
    {
        Scope * pScope = (Scope *)_pScopes->elementAt(_cSnum - 1);
        if (pScope->_pContext != pContext)
            break;
        deleteScope(pScope);
    }

#if DBG == 1
    // sanity checking
    int s = _cSnum;
    while (s > 0)
    {
        Scope * pScope = (Scope *)_pScopes->elementAt(s - 1);
        Assert(pScope->_pContext != pContext);            
        s--;
    }
#endif
}

void 
NamespaceMgr::deleteScope(Scope * pScope)
{
    if (!pScope->_pAtomPrefix)
    {
        _pScopeDefault = pScope->_pPrevDefault;
    }

    // sanity checking
    Assert ((Scope *)_pScopes->elementAt(_cSnum - 1) == pScope);

    _pScopes->removeElementAt(--_cSnum);
}

Atom *
NamespaceMgr::findURN(const TCHAR* pwcText, ULONG ulLen, bool* pboolReserved, Atom ** ppSrcURN)
{
    if ((*pwcText == 'x' || *pwcText == 'X') && ulLen > 2)
    {
        bool fXMLNS = false;
        if ((ulLen == 3 && !StrCmpN(XMLNames::pszXML, pwcText, 3)) ||
            (fXMLNS = (ulLen == 5 && !StrCmpN(XMLNames::pszXMLNS, pwcText, 5))) )
        {
            Atom * pURN;
            if (fXMLNS)
                pURN = XMLNames::atomURNXMLNS;
            else
                pURN = XMLNames::atomURNXML;

            if (pboolReserved != null)
                *pboolReserved = true;
            if (ppSrcURN)
                *ppSrcURN = pURN;
            return pURN;
        }

        if (pboolReserved != null && isReservedNameSpace(pwcText, ulLen))
        {
            *pboolReserved = true;
        }
    }

    if (_cSnum > 0)
    {
        for (int i = _cSnum - 1; i >= 0; i--)
        {
            Scope * pScope = (Scope *)_pScopes->elementAt(i);
            Atom * pScopePrefix = pScope->_pAtomPrefix;
            if (pScopePrefix && pScopePrefix->equals(pwcText, (long)ulLen))
            {
                if (ppSrcURN)
                    *ppSrcURN = pScope->getSrcURN();
                return pScope->_pAtomURN;
            }
        }
    }

    return null;
}

Atom *
NamespaceMgr::findGlobalURN(Atom * pPrefix)
{
    if ((Atom * )XMLNames::atomXML == pPrefix)
        return XMLNames::atomURNXML;
    if ((Atom * )XMLNames::atomXMLNS == pPrefix)
        return XMLNames::atomURNXMLNS;
    return null;
}

Atom *
NamespaceMgr::findURN(Atom * pPrefix, PVOID * ppContext, Atom ** ppSrcURN)
{
    Scope * pScope = null;

    Atom * pURN = findGlobalURN(pPrefix);
    if (pURN)
        goto Done;

    if (!pPrefix)
    {
        pScope = _pScopeDefault;
        goto Done;
    }

    if (_cSnum > 0)
    {
        for (int i = _cSnum - 1; i >= 0; i--)
        {
            pScope = (Scope *)_pScopes->elementAt(i);
            if (pScope->_pAtomPrefix == pPrefix)
                goto Done;
        }
        pScope = null;
    }

Done:
    if (ppContext)
        *ppContext = pScope ? pScope->_pContext : null;
    if (ppSrcURN)
        *ppSrcURN = (pScope ? (Atom*)pScope->_pSrcURN : pURN);
    return (pScope ? (Atom*)pScope->_pAtomURN : pURN);
}

bool
NamespaceMgr::findPrefix(Atom * pAtomURN, Atom **ppAtomPrefix)
{
    if (_cSnum > 0)
    {
        for (int i = _cSnum - 1; i >= 0; i--)
        {
            Scope * pScope = (Scope *)_pScopes->elementAt(i);
            if (pScope->_pAtomURN == pAtomURN)
            {
                *ppAtomPrefix = pScope->_pAtomPrefix;
                return true;
            }
        }
    }

    return false;
}

void
NamespaceMgr::reset()
{
    _pScopes = Vector::newVector();
    _cSnum = 0;
    _pScopeDefault = null;
    if (_fHash)
        _pNameDefs = StringHashtable::newStringHashtable(100);
}


void 
NamespaceMgr::changeContext(PVOID pOldContext, PVOID pNewContext)
{
    if (_cSnum > 0)
    {
        for (int i = _cSnum - 1; i >= 0; i--)
        {
            Scope * pScope = (Scope *)_pScopes->elementAt(i);
            if (pScope->_pContext == pOldContext)
            {
                pScope->_pContext = pNewContext;
            }
            else
            {
                return;
            }
        }
    }
}
 

static const bool g_MuliValued[] = 
{
    false, // AV_CDATA      = 1,
    false, // AV_ID         = 2,
    false, // AV_IDREF      = 3,
    true,  // AV_IDREFS     = 4,
    false, // AV_ENTITY     = 5,
    true,  // AV_ENTITIES   = 6,
    false, // AV_NMTOKEN    = 7,
    true,  // AV_NMTOKENS   = 8,
    false, // AV_NOTATION   = 9,
    false, // AV_ENUMERATION= 10,
    false, // AV_NAMEDEF    = 11,
};

static const bool g_IsName[] = 
{
    false, // AV_CDATA      = 1,
    true,  // AV_ID         = 2,
    true,  // AV_IDREF      = 3,
    true,  // AV_IDREFS     = 4,
    true,  // AV_ENTITY     = 5,
    true,  // AV_ENTITIES   = 6,
    false, // AV_NMTOKEN    = 7,
    false, // AV_NMTOKENS   = 8,
    true,  // AV_NOTATION   = 9,
    false, // AV_ENUMERATION= 10,
    true,  // AV_NAMEDEF    = 11,
};


Object* 
NamespaceMgr::parseNames(DataType type, String * s)
{
    HRESULT hr = S_OK;
    Assert(type >= DT_AV_CDATA && type <= DT_AV_NAMEDEF);

    if (type <= DT_AV_CDATA)
    {
        return s->trim();        
    }
    // BUGBUG GC Danger on s
    const WCHAR* text = s->getWCHARPtr();
    int length = s->length();

    int attrtypeoffset = (int)type - 1; // CDATA == 0
    int count = 0;
    int pos = 0;
    int start;
    int colonPos;

    TCHAR ch;
    const WCHAR* token;
    int toklen;

    Vector * vec = null;
    Object * obj = null;

    // Now parse the value into a series of Names or NmTokens.
    // for each name found
    // MUST go through this loop at least once in order to 
    // guarentee we return a non-null object (String, NameDef, or Vector).
    do
    {
        // skip whitespace.
        if (pos < length)
        {
            ch = text[pos];
            while (isWhiteSpace(ch) && pos < length)
                ch = text[++pos];
        }
        if (pos >= length)
        {
            if (count == 0)
            {
                hr = XML_E_EXPECTING_NAME;
                goto Error;                   // we trimed, so there shouldn't be any spaces left at the end.
            }
            else
                break; // we're done !
        }

        start = pos;
        colonPos = -1;

        // make sure we are starting with a name char if we
        // are parsing a Name token.
        if (pos < length && g_IsName[attrtypeoffset] && ! isStartNameChar(ch))
        {
            hr = XML_E_BADSTARTNAMECHAR;
            goto Error;
        }

        while (pos < length)
        {
            ch = text[pos];
            if (isWhiteSpace(ch))
            {
                if (colonPos == pos - 1)
                {
                    // Name had a prefix but no GI: "foo:"
                    hr = XMLOM_UNEXPECTED_NS;
                    goto Error;
                }
                break;
            }
            else if (ch == ':')
            {
                if (pos == start)
                {
                    hr = XML_E_BADSTARTNAMECHAR;
                    goto Error;
                }
                if (colonPos == -1)
                {
                    colonPos = pos;
                }
                else
                {
                    hr = XML_E_MULTIPLE_COLONS;
                    goto Error;
                }
            }
            else if (! isNameChar(ch))
            {
                hr = XML_E_BADNAMECHAR;
                goto Error;
            }
            pos++;
        }

        count++;

        if (count > 1 && ! g_MuliValued[attrtypeoffset])
        {
            hr = XML_E_MULTI_ATTR_VALUE;
            goto Error;
        }

        toklen = pos - start;
        token = &text[start];

        switch(type)
        {
        case DT_AV_ENTITY:
        case DT_AV_ENTITIES:
            if (colonPos > 0)
            {
                hr = XML_NAME_COLON;
                goto Error;
            }
            // fall through
        case DT_AV_IDREF:
        case DT_AV_IDREFS:
        case DT_AV_NMTOKENS:
            // assume that there is no name space in dt:values
            if (vec == null)
            {
                vec = Vector::newVector();            
                obj = vec;
            }
            vec->addElement(Name::create(token, toklen));
            break;
        case DT_AV_NAMEDEF:
            obj = createNameDef(token, toklen, 
                        (colonPos > 0) ? colonPos - start : 0, false);
            break;
        default:
            obj = Name::create(token, toklen);
            break;
        }
    } 
    while (pos < length);
    
    return obj;

Error:
    Exception::throwE(hr, hr, null);
    return null;

}


NameDef *
NamespaceMgr::createNameDef(const WCHAR* bstrTagName, const WCHAR* bstrURN)
{
    Assert(bstrTagName);

    Atom * pAtomURN, * pSrcURN;
    bool fNSSpecified = true;
    ULONG ulNamespaceLen, ulLen = _tcslen(bstrTagName);

    if (!isValidName(bstrTagName, &ulNamespaceLen))
        Exception::throwE(E_FAIL, 
                          MSG_E_BADNAMECHAR, null);

    if (bstrURN && !*bstrURN)
        bstrURN = null;

    if (ulNamespaceLen && isReservedNameSpace(bstrTagName, ulNamespaceLen))
        pAtomURN = pSrcURN = findGlobalURN(Atom::create(bstrTagName, ulNamespaceLen));
    else if (bstrURN)
    {
        pSrcURN = Atom::create(bstrURN);
        pAtomURN = CanonicalURN(pSrcURN);
    }
    else 
        pAtomURN = pSrcURN = null;

    if (ulNamespaceLen && !pAtomURN)
        Exception::throwE(XML_XMLNS_UNDEFINED, 
                          XML_XMLNS_UNDEFINED, String::newString(bstrTagName, 0, ulNamespaceLen), null);
    
    return createNameDef(bstrTagName, ulLen, ulNamespaceLen, false, pAtomURN, pSrcURN, true);

Error:

    Exception::throwE(E_INVALIDARG);

    // add this to avoid compiler warning (error)
    return null;
}


NameDef *
NamespaceMgr::createNameDef(const TCHAR* pwcText, ULONG ulLen, ULONG ulNamespaceLen, 
                            bool fDefaultNS, Atom * pAtomURN, Atom * pSrcURN, bool fURNSpecified)
{
    //
    // We use a composite key as the key of the hashtable for NameDef-s
    // The composite key is consisted of two parts: [Atom *pAtomURN][WCHAR * prefix:gi]
    // We store the composite key in a WCHAR buffer
    //

    // 
    // allocate a new buffer if necessary
    //
    if (ulLen + sizeof(void *) > _ulLen)
    {
        if (_ulLen > 0)
            delete[] _pwchBuffer;
        if (0 == _ulLen)
            _ulLen = 100;
        _ulLen = ulLen + sizeof(void *);
        _pwchBuffer = new WCHAR[_ulLen];
    }

    //
    // make the composite key
    //
    if (!pAtomURN && !fURNSpecified)
    {
        if (ulNamespaceLen) // check explicit namespace
        {
            bool fReserved = false;

            pAtomURN = findURN(pwcText, ulNamespaceLen, &fReserved, &pSrcURN);

            // Declared?
            if (!pAtomURN)
            {
                Exception::throwE(XML_XMLNS_UNDEFINED, 
                    XML_XMLNS_UNDEFINED, String::newString(pwcText, 0, ulNamespaceLen), null);
            }

            if (fReserved && fDefaultNS) // reserved namespace on an element, error 
            {
                Exception::throwE(XML_E_RESERVEDNAMESPACE, 
                    XML_E_RESERVEDNAMESPACE, String::newString(pwcText, 0, ulNamespaceLen), null);          
            }
        }
        else if (fDefaultNS && _pScopeDefault) // check default namespace
        {
            pAtomURN = _pScopeDefault->_pAtomURN;
            pSrcURN = _pScopeDefault->_pSrcURN;
        }
    }
    Assert((pAtomURN && pSrcURN) || (!pAtomURN && !pSrcURN));
    *((DWORD_PTR *)_pwchBuffer) = (DWORD_PTR)pSrcURN;

    ::memcpy(_pwchBuffer + sizeof(DWORD_PTR) / sizeof(TCHAR), pwcText, ulLen * sizeof(TCHAR));

    //
    // get namedef
    //
    NameDef * namedef = (NameDef*)_pNameDefs->get(_pwchBuffer, ulLen + sizeof(void *) / sizeof(TCHAR));

    if (!namedef)
    {
        if (ulNamespaceLen)
        {
            namedef = NameDef::newNameDef(pwcText + ulNamespaceLen + 1, ulLen - ulNamespaceLen - 1,
                                          pAtomURN, pSrcURN, Atom::create(pwcText, ulNamespaceLen));
        }
        else
        {
            namedef = NameDef::newNameDef(pwcText, ulLen, pAtomURN, pSrcURN, null);
        }

        _pNameDefs->put(String::newString(_pwchBuffer, 0, ulLen + sizeof(void *) / sizeof(TCHAR)), namedef);

#if DBG == 1
        created++;
    }
    else
    {
        reused++;
    }
    TraceTag((tagNameDef, "Return %x h = %d n = %s ns = %s",
        namedef, namedef->hashCode(), (char*)AsciiText(namedef->getName()->toString()), 
        namedef->getPrefix() ? (char*)AsciiText(namedef->getPrefix()->toString()) : null));
#else
    }
#endif

    return namedef;
}

///////////////////////////////////////////////////////////////////////////////
// Warning: This method is used in IE4 Object model only
//
NameDef * 
NamespaceMgr::createNameDef(const WCHAR* pwcText, ULONG ulLen)
{
    WCHAR* ptr = (WCHAR*)pwcText;
    WCHAR wDist = L'A' - L'a';

    //
    // In place upper case
    //
    for (ULONG i = 0; i < ulLen; i++)
    {
        WCHAR ch = *ptr;
        if (ch < 128)
        {
            if (ch >= L'a' && ch <= L'z') 
                *ptr = wDist + ch;
        }
        else if (! IsCharUpper(ch))
            *ptr = (WCHAR)CharUpper((WCHAR*)ch);
        ptr++;
    }

    //
    // Get namedef
    //
    NameDef * namedef = (NameDef*)_pNameDefs->get(pwcText, ulLen);
    if (!namedef)
    {
        namedef = NameDef::newNameDef(pwcText, ulLen, null, null, null);
        _pNameDefs->put(String::newString(pwcText, 0, ulLen), namedef);
    }

    return namedef;
}

NameDef * 
NamespaceMgr::createNameDef(String* strGI, Atom * pAtomURN, Atom * pSrcURN, Atom * pAtomPrefix)
{
    int iLen = 0, iPreLen = 0, i = 0;
    if (pAtomPrefix)
    {
        iPreLen = pAtomPrefix->toString()->length();
        i = iPreLen + 1;
    }
    iLen = strGI->length();
    i += iLen;
    TCHAR * pBuf =  reinterpret_cast<TCHAR *>(_alloca(i * sizeof(TCHAR) + sizeof(Atom *)));
    Assert((pAtomURN && pSrcURN) || (!pAtomURN && !pSrcURN));
    *(Atom **)pBuf = pSrcURN;
    if (iPreLen)
    {
        pAtomPrefix->toString()->copyData(pBuf + sizeof(Atom*)/sizeof(TCHAR), iPreLen);
        *(pBuf + sizeof(Atom *)/sizeof(TCHAR) + iPreLen) = _T(':');
        strGI->copyData(pBuf + sizeof(Atom *)/sizeof(TCHAR) + iPreLen + 1, iLen);
    }
    else
    {
        strGI->copyData(pBuf + sizeof(Atom*)/sizeof(TCHAR), iLen);
    }

    NameDef * pNamedef = (NameDef*)_pNameDefs->get(pBuf, i + sizeof(Atom*) / sizeof(TCHAR));
    if (!pNamedef)
    {
        pNamedef = NameDef::newNameDef(Name::create(strGI, pAtomURN), pSrcURN, pAtomPrefix);
        _pNameDefs->put(String::newString(pBuf, 0, i + sizeof(Atom *) / sizeof(TCHAR)), pNamedef);

#if DBG == 1
        created++;
    }
    else
    {
        reused++;
    }
    TraceTag((tagNameDef, "Return %x h = %d n = %s ns = %s",
        pNamedef, pNamedef->hashCode(), (char*)AsciiText(pNamedef->getName()->toString()), 
        pNamedef->getPrefix() ? (char*)AsciiText(pNamedef->getPrefix()->toString()) : null));
#else
    }
#endif

    return pNamedef;
}


NameDef * 
NamespaceMgr::createNameDefOM(const WCHAR* bstrTagName, bool fUsePrefix)
{
    Atom * pURN;
    // parse name into prefix and baseName
    ULONG nPos;

    if (!isValidName(bstrTagName, &nPos))
        Exception::throwE(E_INVALIDARG, 
                          MSG_E_BADNAMECHAR,
                          null);
    if (!fUsePrefix)
        nPos = 0;

    if (nPos && isReservedNameSpace(bstrTagName, nPos))
        pURN = findGlobalURN(Atom::create(bstrTagName, nPos));
    else
        pURN = null;
    return createNameDef(bstrTagName, lstrlenW(bstrTagName), nPos, false, pURN, pURN, true);
}


Object * 
NamespaceMgr::clone()
{
    NamespaceMgr * pClonedMgr = NamespaceMgr::newNamespaceMgr(_fHash);
    pClonedMgr->_pNameDefs = _pNameDefs; // share the namedef hashtable, it is safe as long as 
                                         // the cloned document belongs to the same thread

#if DBG == 1
    pClonedMgr->created = created;
    pClonedMgr->reused = reused;
#endif

    // Do not need to clone scopes
    return pClonedMgr;
}


Atom * 
NamespaceMgr::CanonicalURN(Atom * pSrcURN)
{
    Atom * pURN;
    String * pstrSrcURN = pSrcURN->toString();
    int URNLen = pstrSrcURN->length();
    int offset = 5;
    if ((URNLen > 5 && 0==memcmp(pstrSrcURN->getWCHARPtr(), L"uuid:", 5*sizeof(TCHAR))) ||
        (URNLen > 9 && 0==memcmp(pstrSrcURN->getWCHARPtr(), L"urn:uuid:", 9*sizeof(TCHAR)) && 0!=(offset=9)) )
    {
        String * pGUID = pstrSrcURN->substring(offset)->toUpperCase();
        pURN = Atom::create(String::add(pstrSrcURN->substring(0, offset), pGUID, null));
        //pURN = Atom::create(pstrSrcURN->toUpperCase());
    }
    else
        pURN = pSrcURN;

    if ((Atom *)XMLNames::atomDTTYPENSAlias == pURN)
        pURN = XMLNames::atomDTTYPENS;
    else if ((Atom *)XMLNames::atomDTTYPENSOld == pURN)
        pURN =XMLNames::atomDTTYPENS;
    if ((Atom *)XMLNames::atomSCHEMAAlias == pURN)
        pURN = XMLNames::atomSCHEMA;

    return pURN;
}
