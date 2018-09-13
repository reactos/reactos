/*
 * @(#)ContentModel->cxx 1->0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

/** 
 * This class represents the content model definition for a given
 * XML element. The content model is defined in the element
 * declaration in the Document Type Definition (DTD); for example, 
 * (a,(b|c)*,d). The
 * content model is stored in an expression tree of <code>ContentNode</code> objects
 * for use by the XML parser during validation.
 * 
 */

#include "core.hxx"
#pragma hdrstop

#ifndef _CORE_UTIL_BITSET
#include "core/util/bitset.hxx"
#endif

#ifndef _XML_ELEMENTDECL
#include "elementdecl.hxx"
#endif

#include "entity.hxx"
#include "contentmodel.hxx"
#include "xmlnames.hxx"
#include "dtdstate.hxx"
#include "xml/om/node.hxx"

DEFINE_CLASS_MEMBERS(ContentModel, _T("ContentModel"), Base);

/**
 * This is the ContentNode * object on the syntax tree describing element 
 * content model
 */
class NOVTABLE ContentNode : public Base
{
    DECLARE_CLASS_MEMBERS(ContentNode, Base);
    DECLARE_CLASS_INSTANCE(ContentNode, Base);

public:
    //
    // Internal node types
    //
    enum
    {
        SEQUENCE = 0,
        CHOICE = 1,
        QMARK = 2,
        STAR = 3,
        PLUS = 4,
        TERMINAL = 5,
    };

    /**
     * Node type
     */
    byte _nType;

    /**
     * firstpos
     */    
    RBitSet _pFirst;
    
    /**
     * lastpos
     */    
    RBitSet _pLast;

    virtual bool nullable() = 0;
    virtual ContentNode * clone(ContentModel * cm) = 0;
    virtual BitSet * firstpos(int positions) = 0;
    virtual BitSet * lastpos(int positions) = 0;
    virtual void calcfollowpos(ABitSet * followpos) = 0;

protected:

    virtual void finalize()
    {
        _pFirst = null;
        _pLast = null;
    }
};

DEFINE_CLASS_MEMBERS_CLASS(ContentNode, _T("ContentNode"), Base);

class Terminal : public ContentNode
{
    DECLARE_CLASS_MEMBERS(Terminal, ContentNode);
    DECLARE_CLASS_INSTANCE(Terminal, ContentNode);

public:

    /**
     * numbering the node
     */
    int _nPos;
    
    /**
     * name it refers to
     */    
    RName _pName;

    
    protected: Terminal(ContentModel * cm, Name * name)
    {
        this->_pName = name;
        this->_nPos = cm->_pTerminalNodes->size();
        
        cm->_pTerminalNodes->addElement(this);
        if (name != null && cm->_pSymbolTable->get(name) == null)
        {
            cm->_pSymbolTable->put(name, Integer::newInteger(cm->_pSymbols->size()));
            cm->_pSymbols->addElement(name);
        }
        _nType = TERMINAL;
    }

    public: static Terminal * newTerminal(ContentModel * cm, Name * name)
            {
                return new Terminal(cm, name);
            }
    
    virtual ContentNode * clone(ContentModel * cm)
    {
        return Terminal::newTerminal(cm, _pName);
    }

    virtual bool nullable() 
    {
        if (_pName == null)
            return true;
        else return false;
    }
    
    virtual BitSet * firstpos(int positions)
    {
        if (_pFirst == null)
        {
            _pFirst = new BitSet(positions);
            _pFirst->set(_nPos);
        }

        return _pFirst;
    }
    
    virtual BitSet * lastpos(int positions)
    {
        if (_pLast == null)
        {
            _pLast = new BitSet(positions);
            _pLast->set(_nPos);
        }

        return _pLast;
    }
    
    virtual void calcfollowpos(ABitSet * followpos)
    {
    }
 
protected:

    virtual void finalize()
    {
        _pName = null;
        super::finalize();
    }
};

DEFINE_CLASS_MEMBERS_CLASS(Terminal, _T("Terminal"), ContentNode);

class InternalNode : public ContentNode
{
    DECLARE_CLASS_MEMBERS(InternalNode, ContentNode);
    DECLARE_CLASS_INSTANCE(InternalNode, ContentNode);

public:

    /**
     * left node
     */    
    RContentNode _pLeft;
    
    /**
     * right node, if node type is QMark, Closure, or PLUS, right node is NULL
     */    
    RContentNode _pRight;

    protected: InternalNode(ContentNode * l, ContentNode * r, int t)
    {
        _pLeft = l;
        _pRight = r;
        Assert(0 <= t && t <= 255);
        _nType = (BYTE)t;
    }

    public: static InternalNode * newInternalNode(ContentNode * l, ContentNode * r, int t)
            {
                return new InternalNode(l, r, t);
            }
    
    virtual ContentNode * clone(ContentModel * cm)
    {
        if (_pRight == null)
            return InternalNode::newInternalNode(_pLeft->clone(cm), null, _nType);
        return InternalNode::newInternalNode(_pLeft->clone(cm), _pRight->clone(cm), _nType);
    }

    virtual bool nullable() 
    {
        switch (_nType)
        {
            case SEQUENCE:    return _pLeft->nullable() && _pRight->nullable();
            case CHOICE:      return _pLeft->nullable() || _pRight->nullable();
            case PLUS: return _pLeft->nullable();
            default:          return true;  // QMARK, or STAR                              
        }
    }
    
    virtual BitSet * firstpos(int positions)
    {
        if (_pFirst == null)
        {
            if (_nType == SEQUENCE && !_pLeft->nullable())
            {
                _pFirst = _pLeft->firstpos(positions);
            }
            else if (_nType == SEQUENCE || _nType == CHOICE)
            {
                _pFirst = (BitSet *)_pLeft->firstpos(positions)->clone();
                _pFirst->or(_pRight->firstpos(positions));
            }
            else
            {
                _pFirst = _pLeft->firstpos(positions);      
            }
        }

        return _pFirst;
    }
    
    virtual BitSet * lastpos(int positions)
    {
        if (_pLast == null)
        {
            if (_nType == SEQUENCE && !_pRight->nullable())
            {
                _pLast = _pRight->lastpos(positions);
            }
            else if (_nType == CHOICE || _nType == SEQUENCE)
            {
                _pLast = (BitSet *)_pLeft->lastpos(positions)->clone();
                _pLast->or(_pRight->lastpos(positions));
            }
            else // QMARK, STAR, or PLUS
            {
                _pLast = _pLeft->lastpos(positions);
            }
        }

        return _pLast;
    }

    
    virtual void calcfollowpos(ABitSet * followpos)
    {
        int i, l;
        BitSet *lp, *fp;

        switch (_nType)
        {
            case SEQUENCE:
                _pLeft->calcfollowpos(followpos);
                _pRight->calcfollowpos(followpos);
        
                l = followpos->length();        
                lp = _pLeft->lastpos(l);
                fp = _pRight->firstpos(l);        
                for (i = followpos->length() - 1; i >= 0; i--)
                {
                    if (lp->get(i))
                    {
                        (*followpos)[i]->or(fp);
                    }
                }
                break;
            case CHOICE:
                _pLeft->calcfollowpos(followpos);
                _pRight->calcfollowpos(followpos);
                break;
            case QMARK:
                _pLeft->calcfollowpos(followpos);
                break;
            default: // STAR, or PLUS
                _pLeft->calcfollowpos(followpos);
        
                l = followpos->length();        
                lastpos(l);
                firstpos(l);        

                for (i = followpos->length() - 1; i >= 0; i--)
                {
                    if (_pLast->get(i))
                    {
                        (*followpos)[i]->or(_pFirst);
                    }
                }
        }
    }


protected:

    virtual void finalize()
    {
        _pLeft = null;
        _pRight = null;
        super::finalize();
    }
};

DEFINE_CLASS_MEMBERS_CLASS(InternalNode, _T("InternalNode"), ContentNode);


/////////////////////////////////////////////////////////////////////
// For verification
//
void ContentModel::initContent(DTDState* context)
{
    context->state = 0;
    
    if (_pDtrans != null && _pDtrans->size() > 0)
    {
        context->matched = (*(aint *)_pDtrans->elementAt(context->state))[_pSymbols->size()] > 0;
    }
    else
    {
        context->matched = true;
    }
}

void ContentModel::checkContent(DTDState* context, Name* name, DWORD dwType)
{
    if (_nType == ANY)
    {
        context->matched = true;
        return;
    }

    // This is a special test for EMPTY content model.  EMPTY means it
    // cannot have whitespace either.
//    if (_pCurrent->ed->_pContent->getType() == ContentModel::EMPTY)
//    {
//        return XML_ILLEGAL_TEXT;
//    }

    Name * n;
    if (name != null)
    {
        n = name;
    }
    else
    {
        n = XMLNames::name(NAME_PCDATA);
    }

    Integer * lookup = (Integer *)_pSymbolTable->get(n);
    if (lookup != null)
    {
        int sym = lookup->intValue();
        int state = (*(aint *)_pDtrans->elementAt(context->state))[sym];
        if (state != -1)
        {
            context->state = state;
            context->matched = (*(aint *)_pDtrans->elementAt(context->state))[_pSymbols->size()] > 0;
            return;
        }
    }

    if (context->matched && isOpen())
        return;

    // FAILED to match the content model - so generate
    // a nice informative message about why it failed.
    {
        HRESULT hr = (dwType == XML_PCDATA || dwType == XML_CDATA) ? XML_ILLEGAL_TEXT : XML_INVALID_CONTENT;
        String* msg = Resources::FormatMessage(hr, null);
        if (_pDtrans)
        {
            String* str = expectedElements(context->state)->toString();
            if (str->length() > 0)
            {
                String* expected = Resources::FormatMessage(XML_DTD_EXPECTING, str, null);
                msg = String::add(msg, expected, null);
            }
        }
        Exception::throwE(msg, hr);
    }
}

/**
 * Retrieves a string representation of the content type.
 * @return a <A>String</A>. 
 */ 
String * ContentModel::toString()
{
    TCHAR * s;
    switch(_nType)
    {
        case EMPTY:
            s = (TCHAR*)XMLNames::pszEMPTY;
            break;
        case ANY:
            s = (TCHAR*)XMLNames::pszANY;
            break;
        case ELEMENTS:
            s = (TCHAR*)XMLNames::pszELEMENTS;
            break;
        default:
            s = (TCHAR*)XMLNames::pszUnknown;
    }
    // BUGBUG - FormatMessage.
    return String::add(String::newString((TCHAR*)XMLNames::pszContentType), String::newString(s), null);
}

void  ContentModel::start()
{
    _pTerminalNodes = Vector::newVector();
    _pSymbolTable = Hashtable::newHashtable();
    _pSymbols = Vector::newVector();
    _pStack = Stack::newStack();
}

void  ContentModel::finish()
{
    // add end node
    if (_pContent == null)
        return;
    _pTable = null;

    _pEnd = Terminal::newTerminal(this, null);
    _pContent = InternalNode::newInternalNode(_pContent, _pEnd, ContentNode::SEQUENCE);

    // calculate followpos for terminal nodes
    int terminals = _pTerminalNodes->size();
    ABitSet * followpos = new (terminals) ABitSet;
    for (int i = 0; i < terminals; i++)
    {
        (*followpos)[i] = new BitSet(terminals);
    }
    _pContent->calcfollowpos(followpos);

    // state table
    Vector * Dstates = Vector::newVector();
    // transition table
    _pDtrans = Vector::newVector();
    // lists unmarked states
    Vector * unmarked = Vector::newVector();
    // state lookup table
    Hashtable * statetable = Hashtable::newHashtable();

    BitSet * empty = new BitSet(terminals);
    statetable->put(empty, Integer::newInteger(-1));

    // current state processed
    int state = 0;                

    // start with firstpos at the root                
    BitSet * set = _pContent->firstpos(terminals);
    statetable->put(set, Integer::newInteger(Dstates->size()));
    unmarked->addElement(set);
    Dstates->addElement(set);
    aint * a = new (_pSymbols->size() + 1) aint;
    _pDtrans->addElement(a);
    if (set->get(_pEnd->_nPos))
    {
        (*a)[_pSymbols->size()] = 1;   // accepting
    }

    // check all unmarked states
    while (unmarked->size() > 0)
    {
        aint * t = (aint *)_pDtrans->elementAt(state);
    
        set = (BitSet *)unmarked->elementAt(0);
        unmarked->removeElementAt(0);

        // check all input _pSymbols
        for (int sym = 0; sym < _pSymbols->size(); sym++)
        {
            Name * n = (Name *)_pSymbols->elementAt(sym);

            BitSet * newset = new BitSet(terminals);
            // if symbol is in the set add followpos to new set
            for (int i = 0; i < terminals; i++)
            {
                if (set->get(i) && ((Terminal *)_pTerminalNodes->elementAt(i))->_pName == n)
                {
                    newset->or((*followpos)[i]);
                }
            }

            Integer * lookup = (Integer *)statetable->get(newset);                        
            // this state will transition to
            int transitionTo;
            // if new set is not in states add it                        
            if (lookup == null)
            {
                transitionTo = Dstates->size();            
                statetable->put(newset, Integer::newInteger(transitionTo));
                unmarked->addElement(newset);
                Dstates->addElement(newset);
                a = new (_pSymbols->size() + 1) aint;
                _pDtrans->addElement(a);
                if (newset->get(_pEnd->_nPos))
                {
                    (*a)[_pSymbols->size()] = 1;   // accepting
                }
            }
            else
            {
                transitionTo = lookup->intValue();                            
            }
            // set the transition for the symbol
            (*t)[sym] = transitionTo;
        }
        state++;
    }
}

void    ContentModel::openGroup()
{
    _pStack->push(null);
}

void    ContentModel::closeGroup()
{
    ContentNode* n = (ContentNode*)_pStack->pop();
    if (_pStack->size() == 0)
    {
        if (n->_nType == ContentNode::TERMINAL)
        {
            Terminal* t = (Terminal*)n;
            if (t->_pName == XMLNames::name(NAME_PCDATA))
            {
                // we had a lone (#PCDATA) which needs to be
                // wrapped in a STAR node.
                n = InternalNode::newInternalNode(n, null, ContentNode::STAR);
            }           
        }
        _pContent = n;
        _fPartial = false;
    }
    else
    {
        // some collapsing to do...
        InternalNode* in = (InternalNode*)_pStack->pop();
        if (in != null)
        {
            Assert(in->_pRight == null);
            in->_pRight = n;
            n = in;
            _fPartial = true;
        }
        else
        {
            _fPartial = false;
        }

        _pStack->push(n);
    }
}

void    ContentModel::addTerminal(Name* name)
{
    Assert(name != null);       

    if (name == XMLNames::name(NAME_PCDATA))
    {
        _fMixed = true;
        _pTable = Hashtable::newHashtable();
    }
    else if (_pTable)
    {
        if (_pTable->containsKey(name))
        {
            Exception::throwE(XML_E_MIXEDCONTENT_DUP_NAME, XML_E_MIXEDCONTENT_DUP_NAME,
                name->toString(), null);
        }
        else
        {
            _pTable->put(name, name);
        }
    }

    ContentNode* n = Terminal::newTerminal(this, name);
    if (_pStack->size() > 0)
    {
        InternalNode* in = (InternalNode*)_pStack->pop();
        if (in != null)
        {
            Assert(in->_pRight == null);
            in->_pRight = n;
            n = in;
        }
    }
    _pStack->push( n );
    _fPartial = true;
}

void    ContentModel::addChoice()
{
    ContentNode* n = (ContentNode*)_pStack->pop();
    _pStack->push(InternalNode::newInternalNode( n, null, ContentNode::CHOICE));
}

void    ContentModel::addSequence()
{
    ContentNode* n = (ContentNode*)_pStack->pop();
    _pStack->push(InternalNode::newInternalNode( n, null, ContentNode::SEQUENCE));
}

void    ContentModel::star()
{
    closure(ContentNode::STAR);
}

void    ContentModel::plus()
{
    closure(ContentNode::PLUS);
}

void    ContentModel::questionMark()
{
    closure(ContentNode::QMARK);
}

void    ContentModel::closure(int type)
{
    if (_pStack->size() > 0)
    {
        ContentNode* n = (ContentNode*)_pStack->pop();
        if (_fPartial && n->_nType != ContentNode::TERMINAL)
        {
            // need to reach in and wrap _pRight hand side of element.
            // and n remains the same.
            InternalNode* in = (InternalNode*)n;
            in->_pRight = InternalNode::newInternalNode(in->_pRight, null, type);
        }
        else
        {
            // wrap terminal node
            n = InternalNode::newInternalNode(n, null, type);
        }
        _pStack->push(n);
    }
    else
    {
        // wrap whole content
        _pContent = InternalNode::newInternalNode(_pContent, null, type);
    }
}

/**
 * Checks whether the content model allows empty content
 */
bool ContentModel::acceptEmpty()
{
    if (_nType == ANY || _nType == EMPTY)
        return true;

    return (*(aint *)_pDtrans->elementAt(0))[_pSymbols->size()] > 0;
}

//  Returns names of all legal elements following the specified state
Vector * ContentModel::expectedElements(int state)
{
    aint * t = (aint *)_pDtrans->elementAt(state);
    if (t==null)
        return null;
    Vector * names = Vector::newVector();

    for (Enumeration * e = _pTerminalNodes->elements(); e->hasMoreElements();)
    {
        Name * n = ((Terminal *)e->nextElement())->_pName;
        if (n != null && !names->contains(n))
        {
            Integer * lookup = (Integer *)_pSymbolTable->get(n);
            if (lookup != null && (*t)[lookup->intValue()] != -1)
            {
                names->addElement(n);
            }
        }
    }
    return names;
}

bool ContentModel::isRepeatable(Name * n)       //this is public function, call recursive private function (below)
{
    if (n==null || _pSymbolTable->get(n)==null)
        return false;                           //n is not a child of this element

    return isRepeatable(_pContent,n);
}

bool ContentModel::isRepeatable(ContentNode * pCN, Name * n)     //this is recursive private function, recurses thru tree
{
    if (pCN==null || n==null || pCN->_nType==ContentNode::TERMINAL)
        return false;

    InternalNode * pIN = (InternalNode *)pCN;       //we know pCN is internal now

    if (pCN->_nType==ContentNode::PLUS || pCN->_nType==ContentNode::STAR)       //if this is a node indicating repeatability, 
    {                                                           //check to see if children are terminal nodes of same name
        if (pIN->_pLeft!=null && pIN->_pLeft->_nType==ContentNode::TERMINAL && n->equals(((Terminal *)(ContentNode *)pIN->_pLeft)->_pName))
            return true;
        if (pIN->_pRight!=null && pIN->_pRight->_nType==ContentNode::TERMINAL && n->equals(((Terminal *)(ContentNode *)pIN->_pRight)->_pName))
            return true;
    }

    if (isRepeatable(pIN->_pLeft,n))      //if pIN->_pLeft is a terminal node or null, isRepeatable will return false
        return true;
    if (isRepeatable(pIN->_pRight,n))     //if pIN->_pRight is a terminal node or null, isRepeatable will return false
        return true;

    return false;
}

Enumeration* 
ContentModel::getSymbols()
{
    return _pSymbolTable->keys();
}


