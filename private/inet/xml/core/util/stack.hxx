/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _CORE_UTIL_STACK
#define _CORE_UTIL_STACK

#include "vector.hxx"


DEFINE_CLASS(Stack);

#define FINAL
#define VIRTUAL virtual

class Stack : public Vector 
{
    DECLARE_CLASS_MEMBERS(Stack, Vector);
    DECLARE_CLASS_CLONING(Stack, Vector);

    //  Stack() 
    public: Stack();

    // cloning constructor, shouldn't do anything with data members...
    protected:  Stack(CloningEnum e) : super(e) {}

    //  empty() 
    // Tests if this stack is empty. 
    public: VIRTUAL bool empty();

    //  peek() 
    // Looks at the object at the top of this stack without removing it from the 
    // stack. 
    public: VIRTUAL Object * peek();

    //  pop() 
    // Removes the object at the top of this stack and returns that object as the 
    // value of this function. 
    public: VIRTUAL Object * pop();

    //  push(Object) 
    // Pushes an item onto the top of this stack. 
    public: VIRTUAL Object * push(Object *obj);

    //  search(Object) 
    // Returns where an object is on this stack. 
    public: VIRTUAL int search(Object *obj);

};


#undef FINAL
#undef VIRTUAL

#endif _CORE_UTIL_STACK


