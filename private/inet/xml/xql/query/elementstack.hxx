/*
 * @(#)ElementStack.hxx 1.0 6/14/97
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _XQL_QUERY_ELEMENTSTACK
#define _XQL_QUERY_ELEMENTSTACK

class ElementFrame
{
public:
                ElementFrame() : _eParent(false){};
                ElementFrame(const bool shouldAddRef) : _eParent(shouldAddRef) {};

    void        init(Element * eParent, bool fIsAttribute = false, const bool shouldAddRef=false) 
                {
                    _eParent.assign(eParent, shouldAddRef);
                    _fNext = false; 
                    _fIsAttribute = fIsAttribute; 
                    _index = 0;
                }
    Element *   getParent() {return _eParent;}

    // WAA - replacing RElement
    ROElement   _eParent;
    HANDLE      _h;
    unsigned    _fNext:1;
    unsigned    _fIsAttribute:1;
    int         _index;
};

typedef _array<ElementFrame>      AElementFrame;
typedef _reference<AElementFrame> RAElementFrame;


class ElementStack
{
public:

    ElementFrame * push(Element * e, bool fIsAttribute, bool fAddRef);

    void pop();

    ElementFrame * tos();

    bool    empty() {return _sp == 0;}

    void    reset() {_sp = 0;}

    int     sp() {return _sp;}

    ElementFrame * item(int i) {return &(*_astkframe)[i];}

private:

    enum 
    {
        INITIAL_STACK_SIZE = 8
    };

    /**
    * Stack for elements to remember the path when
    * walking the tree.
    */

    RAElementFrame    _astkframe;

    /**
    * Stack pointer
    */

    int             _sp;
};

#endif