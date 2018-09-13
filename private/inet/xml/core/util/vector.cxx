/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include "core/util/vector.hxx"

#ifndef _CORE_BASE_VMM
#include "core/base/vmm.hxx"
#endif


Enumeration * VectorEnumerator::newVectorEnumerator(Vector *vec)
{
    VectorEnumerator * ve = new VectorEnumerator();
    ve->_vector = vec;
    ve->_position = 0;    
    return ve;
}

///////////////////////////////////////////////////////////////////////////////
//////////////////    Vector    ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_MEMBERS_CLONING(Vector, _T("Vector"), Base);


//////////////   Java constructors   //////////////////////////////////////////

// Constructs an empty vector with the specified initial capacity and capacity 
// increment. 
Vector::Vector(int initialCapacity, int capacityIncr):
    capacityIncrement(capacityIncr),
    elementCount(0),
    elementData(new (initialCapacity) AObject)
{
}


//////////////   Java methods   ///////////////////////////////////////////////

// Adds the specified component to the end of this vector, increasing its size 
// by one. 
void
Vector::addElement(Object *obj)
{
    ensureCapacity(elementCount + 1);

    (*elementData)[elementCount] = obj;
    ++ elementCount;
}


// Returns a clone of this vector. 
Object *
Vector::clone()
{
    Vector *vec = CAST_TO(Vector*, super::clone());
    AObject *newData = new (elementCount) AObject;

    copyObjects(0, elementCount, newData, 0, Forward);

    vec->capacityIncrement = capacityIncrement;
    vec->elementCount = elementCount;
    vec->elementData = newData;
    
    return vec;
}


// Returns the component at the specified index. 
Object *
Vector::elementAt(int index)
{
    return (*elementData)[index];
}


// Returns an enumeration of the components of this vector. 
Enumeration *
Vector::elements()
{
    return VectorEnumerator::newVectorEnumerator(this);
}


// Increases the capacity of this vector, if necessary, to ensure that it can 
// hold at least the number of components specified by the minimum capacity 
// argument. 
void
Vector::ensureCapacity(int minCapacity)
{
    int myCapacity = elementData->length();
    if (minCapacity > myCapacity)
    {
        AObject *newData;
        int newCapacity = (capacityIncrement == 0) ? 2 * (myCapacity + 1)
                                                   : myCapacity + capacityIncrement;
        if (minCapacity > newCapacity)
            newCapacity = minCapacity;

        newData = new (newCapacity) AObject;
        copyObjects(0, elementCount, newData, 0, Forward);
        
        elementData = newData;
    }
}


// Searches for the first occurence of the given argument, testing for equality 
// using the equals method. 
int
Vector::indexOf(Object *obj)
{
    for (int i=0; i<elementCount; ++i)
    {
        if (obj->equals((*elementData)[i]))
            return i;
    }

    return -1;
}


// Inserts the specified object as a component in this vector at the specified 
// index. 
void
Vector::insertElementAt(Object *obj, int index)
{
    ensureCapacity(elementCount + 1);

    copyObjects(index, elementCount, elementData, index+1, Backward);

    (*elementData)[index] = obj;
    ++ elementCount;
}


// Searches backwards for the specified object, starting from the specified index,
// and returns an index to it. 
int
Vector::lastIndexOf(Object *obj, int index)
{
    for (int i=index; i>=0; --i)
    {
        if (obj->equals((*elementData)[i]))
            return i;
    }

    return -1;
}


// Removes all components from this vector and sets its size to zero. 
void
Vector::removeAllElements()
{
    for (int i=0; i<elementCount; ++i)
    {
        (*elementData)[i] = null;
    }

    elementCount = 0;
}


// Removes the first occurrence of the argument from this vector. 
bool
Vector::removeElement(Object *obj)
{
    int index = indexOf(obj);
    bool objInVector = (index >= 0);

    if (objInVector)
    {
        removeElementAt(index);
    }

    return objInVector;
}


// Deletes the component at the specified index. 
void
Vector::removeElementAt(int index)
{
    if (0<=index && index<elementCount)
    {
        (*elementData)[index] = null;
        copyObjects(index+1, elementCount, elementData, index, Forward);
        -- elementCount;
    }
    else
    {
        Exception::throwE(E_INVALIDARG); // Exception::ArrayIndexOutOfBoundsException);
    }
}


// Sets the component at the specified index of this vector to be the specified 
// object. 
void
Vector::setElementAt(Object *obj, int index)
{
    (*elementData)[index] = obj;
}


// Sets the size of this vector. 
void
Vector::setSize(int newSize)
{
    int i;

    Assert(newSize >= 0);
    
    if (newSize > elementCount)
    {
        ensureCapacity(newSize);
        for (int i=elementCount; i<newSize; ++i)
        {
            (*elementData)[i] = null;
        }
        elementCount = newSize;
    }
    else
    {
        for (int i=elementCount-1; i>=newSize; --i)
        {
            (*elementData)[i] = null;
        }
        elementCount = newSize;
    }
}


// Compares this vector to the specified object. 
bool
Vector::equals(Object * anObject)
{
    if (this == anObject)
        return true;

    if (anObject && Vector::_getClass()->isInstance(anObject))
    {
        Vector * pVect = SAFE_CAST(Vector *, anObject);
        if (pVect->size() == size())
        {
            for (int i = size() - 1; i >= 0; i--)
            {
                if (pVect->elementAt(i) != elementAt(i))
                    return false;
            }
            return true;
        }
    }
    return false;
}


// Returns a string representation of this vector. 
String *
Vector::toString()
{
    StringBuffer *buffer = StringBuffer::newStringBuffer();

    for (int i=0; i<elementCount; ++i)
    {
        buffer->append((*elementData)[i]->toString());
        if (i+1 < elementCount)
        {
            buffer->append(_T(", "));
        }
    }

    return buffer->toString();
}


////////////////   Helper methods   ///////////////////////////////////

void
Vector::copyObjects(int srcStart, int srcFinish,
                    AObject *dst, int dstStart, CopyDirection dir)
{
    Assert(0<=srcStart && srcStart<=srcFinish && srcFinish<=elementCount &&
           0<=dstStart && dstStart+srcFinish-srcStart <= dst->length() );
    int i;
    
    switch (dir)
    {
    case Forward:
        for (i=srcStart; i<srcFinish; ++i)
        {
            (*dst)[dstStart - srcStart + i] = (*elementData)[i];
        }
        break;

    case Backward:
        for (i=srcFinish-1; i>=srcStart; --i)
        {
            (*dst)[dstStart - srcStart + i] = (*elementData)[i];
        }
        break;
    }
}



///////////////////////////////////////////////////////////////////////////////
//////////////////    VectorEnumerator    /////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// use ABSTRACT because of no default constructor 
DEFINE_ABSTRACT_CLASS_MEMBERS(VectorEnumerator, _T("VectorEnumerator"), Base);

// Tests if this enumeration contains more elements. 
bool
VectorEnumerator::hasMoreElements()
{
    return (_position < _vector->size());
}


// Returns the next element of this enumeration without advancing. 
Object *
VectorEnumerator::peekElement()
{
    Object *result;
    
    if (hasMoreElements())
    {
        result = _vector->elementAt(_position);
    }
    else
    {
        Exception::throwE(E_FAIL); // Exception::NoSuchElementException);
        result = null;
    }

    return result;
}


// Returns the next element of this enumeration. 
Object *
VectorEnumerator::nextElement()
{
    Object *result = peekElement();
    _position++;
    return result;
}


// Resets the enumeration

void VectorEnumerator::reset()
{
    _position = 0;
}


// Called by the garbage collector on an object when garbage collection 
// determines that there are no more references to the object. 
void
VectorEnumerator::finalize()
{
    _vector = null;

    super::finalize();
}
