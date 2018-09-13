/*
 * @(#)NameSpaceContext.cxx 1.0 6/10/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */ 
#include "core.hxx"
#pragma hdrstop

DEFINE_CLASS_MEMBERS(NameSpaceContext, _T("NameSpaceContext"), Base);

NameSpaceContext::NameSpaceContext()
{
    current = Hashtable::newHashtable();
    contexts = Stack::newStack();
}

/**
 *  add name space. long name is the key
 */
void NameSpaceContext::addNameSpace(Atom * url, Atom * n)
{
    current->put(url, n);
}

/**
 * find name space, long name is the key
 */
Atom * NameSpaceContext::findNameSpace(Atom * n)
{
    return (Atom *)current->get(n);
}

void NameSpaceContext::push()
{
    contexts->push(current);
    current = (Hashtable *)current->clone();
}

void NameSpaceContext::pop()
{
    current = (Hashtable *)contexts->pop();
}

