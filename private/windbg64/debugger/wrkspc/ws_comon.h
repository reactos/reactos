#ifndef __WS_COMMON_H__
#define __WS_COMMON_H__



//
//
// RTTI: Used for sanity checks, to be removed from the retail version.
// Used to verify the type of object we are referencing.
//
// ei:
//    AssertType(pPointer, ClassFoo *);
//      Verify that the pointer we have is of that type.
//
//    AssertType(*pPointer, ClassFoo);
//      Verify that what we are pointing to, is of that object.
//
//    AssertChildOf(*pPointer, ClassFoo);
//      Verify that pPointer points to a class derived from ClassFoo.
//
#ifdef _CPPRTTI
#define AssertType(Obj1, Obj2) Assert( RttiTypesEqual(typeid(Obj1), typeid(Obj2)) )
#define AssertNotType(Obj1, Obj2) Assert( !RttiTypesEqual(typeid(Obj1), typeid(Obj2)) )
#define AssertChildOf(pObj, ___Parent_Class) Assert(dynamic_cast<___Parent_Class *> (pObj))
#define AssertNotChildOf(pObj, ___Parent_Class) Assert(NULL == dynamic_cast<___Parent_Class *> (pObj))
#else
#define AssertType(Obj1, Obj2) ((void)0)
#define AssertNotType(Obj1, Obj2) ((void)0)
#define AssertChildOf(pObj, ___Parent_Class) ((void)0)
#define AssertNotChildOf(pObj, ___Parent_Class) ((void)0)
#endif



//
//
//
// Implementation of the data members.
//
// class Foo {
// public:
//    int m_nCounter;
// };
//
// pFooData is a constant pointer to a class Foo.
//    Foo * const pFooData;
//    pFooData->m_nCounter = 2;  // valid
//    pFooData = NULL;           // error
//
// WorkAround:
//    Foo **p = (Foo **) &pFooData;
//    *p = NULL;
//
// Use the following macro:
#define SetConstPointer(type, constptr, newvalue)   \
{                                                   \
    type **p = (type **) &constptr;                 \
    *p = newvalue;                                  \
}





#endif
