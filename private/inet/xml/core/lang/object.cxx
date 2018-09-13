/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/*
 */

#include "core.hxx"
#pragma hdrstop
#include "core/base/mpheap.hxx"

/**
 */

Object::Object()
{
/* 
// WIN64 Code Review - This doesn't compile under Win64, says not all paths return a value 
// which is unusual for a ctor! So I re-implimented below in a way that does compile. Please
// review carefully
*/
#ifdef FOSSIL_CODE
// This is the original code
#ifdef SPECIAL_OBJECT_ALLOCATION
    if (!isObjectRegion(this))
#endif
    {
        TRY
        {
            ::AddPointerToCache(this);
        }
        CATCH
        {
    #if NEVER
            // if we don't delete ourselves now, we will never get deleted.. since we never got in the pointer cache
            Base * pBase = this->getBase();
            delete pBase;
    #endif
            Exception::throwAgain();
        }
        ENDTRY
    }

#else	// !FOSSIL_CODE
	#ifdef SPECIAL_OBJECT_ALLOCATION
	if (!isObjectRegion(this))
	#endif
		::AddPointerToCache(this);
#endif // FOSSIL_CODE
}

Object::~Object()
{
#ifdef SPECIAL_OBJECT_ALLOCATION
    if (!isObjectRegion(this))
#endif
    {
        ::SafeRemovePointerFromCache(this);
    }
}

/**
 */
int Object::hashCode()
{
    return PtrToLong(this);
}

/**
 */
bool Object::equals(Object * obj) 
{
    return (this == obj);
}

/**
 */
Object * Object::clone() //throws CloneNotSupportedException;
{
    CREATEOBJECT cloneCreateObject = getClass()->_cloneCreateObject;
    if (cloneCreateObject != null)
    {
        return (*cloneCreateObject)();
    }
    else
    {
        Assert(String::add(String::newString(_T("Cloning is not supported on ")), String::newString(getClass()->getName()), null));
        Exception::throwE(E_NOTIMPL);
        return null;
    }
}

/**
 */
String * Object::toString() 
{
    return String::emptyString();
}

/**
 */
void Object::finalize() //throws Throwable 
{ 
#if DBG == 1
    TraceTag((tagRefCount, "Object %p : finalized", this));
#endif
}
