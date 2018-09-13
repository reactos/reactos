/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include <core.hxx>
#pragma hdrstop

#include "stack.hxx"

DEFINE_CLASS_MEMBERS_CLONING(Stack, _T("Stack"), Vector);


Stack::Stack():
    Vector()
{
}


// Tests if this stack is empty. 
bool
Stack::empty()
{
    return (size() == 0);
}


// Looks at the object at the top of this stack without removing it from the 
// stack. 
Object *
Stack::peek()
{
    Object *result;
    
    if (size() > 0)
    {
        result = elementAt(size() - 1);
    }
    else
    {
        Exception::throwE(E_FAIL); // Exception::EmptyStackException);
        result = null;
    }

    return result;
}


// Removes the object at the top of this stack and returns that object as the 
// value of this function. 
Object *
Stack::pop()
{
    Object *result;
    
    if (size() > 0)
    {
        result = elementAt(size() - 1);
        removeElementAt(size() - 1);
    }
    else
    {
        Exception::throwE(E_FAIL); // Exception::EmptyStackException);
        result = null;
    }

    return result;
}


// Pushes an item onto the top of this stack. 
Object *
Stack::push(Object *obj)
{
    addElement(obj);
    return obj;
}


// Returns where an object is on this stack. 
int
Stack::search(Object *obj)
{
    return lastIndexOf(obj);
}

