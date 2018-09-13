/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _CORE_UTIL_VECTOR
#define _CORE_UTIL_VECTOR

#include "enumeration.hxx"

DEFINE_CLASS(Vector);
DEFINE_CLASS(VectorEnumerator);

class SlotAllocator;

#define FINAL
#define VIRTUAL virtual


class VectorEnumerator: public Base, public Enumeration
{
    DECLARE_CLASS_MEMBERS_I1(VectorEnumerator, Base, Enumeration);

    private: VectorEnumerator() {}

    public: DLLEXPORT static Enumeration * newVectorEnumerator(Vector *vec);

    //  hasMoreElements() [Enumeration]
    // Tests if this enumeration contains more elements. 
    public: VIRTUAL bool hasMoreElements();

    //  peekElement() [Enumeration]
    // Peeks the next element of this enumeration but does not advance. 
    public: VIRTUAL Object * peekElement();

    //  nextElement() [Enumeration]
    // Returns the next element of this enumeration. 
    public: VIRTUAL Object * nextElement();

    //  reset() [Enumeration]
    // Resets this enumeration. 
    public: VIRTUAL void  reset();

    //  finalize() [Object]
    // Called by the garbage collector on an object when garbage collection 
    // determines that there are no more references to the object. 
    public: VIRTUAL void finalize();

    /////////////   Representation    /////////////////////////////////////
private:
    RVector _vector;
    int     _position;
};


class DLLEXPORT Vector: public Base // implements Cloneable
{
    DECLARE_CLASS_MEMBERS(Vector, Base);
    DECLARE_CLASS_CLONING(Vector, Base);

    ////////////   Protected instance variables   /////////////////////////////
    
    //  capacityIncrement 
    // The amount by which the capacity of the vector is automatically incremented 
    // when its size becomes greater than its capacity. 
    private: int capacityIncrement;

    //  elementCount 
    // The number of valid components in the vector. 
    private: int elementCount;

    //  elementData 
    // The array buffer into which the components of the vector are stored. 
    private: RAObject elementData;
    

    //////////////   Java constructors   //////////////////////////////////////////
    
    //  Vector() 
    // Constructs an empty vector. 

    //  Vector(int) 
    // Constructs an empty vector with the specified initial capacity. 

    //  Vector(int, int) 
    // Constructs an empty vector with the specified initial capacity and capacity 
    // increment. 
    public: Vector(int initialCapacity=16, int capacityIncrement=0);

    // cloning constructor, shouldn't do anything with data members...
    protected:  Vector(CloningEnum e) : super(e) {}

    /////////////   Java methods   ////////////////////////////////////////////
    
    //  addElement(Object) 
    // Adds the specified component to the end of this vector, increasing its size 
    // by one. 
    public: FINAL void addElement(Object *obj);

    //  capacity() 
    // Returns the current capacity of this vector. 
    public: FINAL int capacity() const { return elementData->length(); }

    //  clone() 
    // Returns a clone of this vector. 
    public: VIRTUAL Object * clone();

    //  contains(Object) 
    // Tests if the specified object is a component in this vector. 
    public: FINAL bool contains(Object *obj) { return (indexOf(obj) >= 0); }
    
    //  copyInto(Object[]) 
    // Copies the components of this vector into the specified array. 

    //  elementAt(int) 
    // Returns the component at the specified index. 
    public: FINAL Object * elementAt(int index);

    //  elements() 
    // Returns an enumeration of the components of this vector. 
    public: FINAL Enumeration * elements();

    //  ensureCapacity(int) 
    // Increases the capacity of this vector, if necessary, to ensure that it can 
    // hold at least the number of components specified by the minimum capacity 
    // argument. 
    public: FINAL void ensureCapacity(int minCapacity);

    //  firstElement() 
    // Returns the first component of this vector. 

    //  indexOf(Object) 
    // Searches for the first occurence of the given argument, testing for equality 
    // using the equals method. 
    public: FINAL int indexOf(Object *obj);

    //  indexOf(Object, int) 
    // Searches for the first occurence of the given argument, beginning the search 
    // at index, and testing for equality using the equals method. 

    //  insertElementAt(Object, int) 
    // Inserts the specified object as a component in this vector at the specified 
    // index. 
    public: FINAL void insertElementAt(Object *obj, int index);

    //  isEmpty() 
    // Tests if this vector has no components. 
    public: FINAL bool isEmpty() const { return elementCount == 0; }

    //  lastElement() 
    // Returns the last component of the vector. 

    //  lastIndexOf(Object) 
    // Returns the index of the last occurrence of the specified object in this 
    // vector. 
    public: FINAL int lastIndexOf(Object *obj) { return lastIndexOf(obj, elementCount-1); }

    //  lastIndexOf(Object, int) 
    // Searches backwards for the specified object, starting from the specified index,
    // and returns an index to it. 
    public: FINAL int lastIndexOf(Object *obj, int index);

    //  removeAllElements() 
    // Removes all components from this vector and sets its size to zero. 
    public: FINAL void removeAllElements();

    //  removeElement(Object) 
    // Removes the first occurrence of the argument from this vector. 
    public: FINAL bool removeElement(Object *obj);

    //  removeElementAt(int) 
    // Deletes the component at the specified index. 
    public: FINAL void removeElementAt(int index);

    //  setElementAt(Object, int) 
    // Sets the component at the specified index of this vector to be the specified 
    // object. 
    public: FINAL void setElementAt(Object *obj, int index);

    //  setSize(int) 
    // Sets the size of this vector. 
    public: FINAL void setSize(int newSize);

    //  size() 
    // Returns the number of components in this vector. 
    public: FINAL int size() const { return elementCount; }

    //  toString() [Object]
    // Returns a string representation of this vector. 
    public: VIRTUAL String * toString();

    public: VIRTUAL bool equals(Object * anObject);

    //  trimToSize() 
    // Trims the capacity of this vector to be the vector's current size. 

    // finalize() [Object]
    protected: VIRTUAL void finalize() { elementData = null; super::finalize(); }
    
    ////////////////   Helper methods   ///////////////////////////////////
private:
    enum CopyDirection { Forward, Backward };
    void    copyObjects(int srcStart, int srcFinish,
                        AObject *dst, int dstStart, CopyDirection dir);
};


#undef VIRTUAL
#undef FINAL

#endif _CORE_UTIL_VECTOR


