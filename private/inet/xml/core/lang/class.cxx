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

// use ABSTRACT because there is no default constructor !
DEFINE_ABSTRACT_CLASS_MEMBERS(Class, _T("Class"), Base);

Class * Base::newClass(const TCHAR * name, Class * parent, CREATEOBJECT createObject, CREATEOBJECT cloneCreateObject)
{
    Class * pClass;
#ifdef RENTAL_MODEL
    Model model(MultiThread);
#endif
    TRY
    {
        pClass = new Class(name, parent, createObject, cloneCreateObject);
    }
    CATCH
    {
#ifdef RENTAL_MODEL
        model.Release();
#endif
        Exception::throwAgain();
        
        // add this to avoid compiler warning (error)
        pClass = null;
    }
    ENDTRY
    return pClass;
}

/**
 */
/*
 */

Class::Class(const TCHAR * name, Class * parent, CREATEOBJECT createObject, CREATEOBJECT cloneCreateObject)
{
    this->parent = parent;
    this->name = name;
    this->_createObject = createObject;
    this->_cloneCreateObject = cloneCreateObject;
}


void Class::finalize()
{
    parent = null;
    super::finalize();
}


/**
 */
String * Class::toString() 
{
    return String::add(String::newString(_T("class ")), getName(), null);
}

/**
 */
Object * Class::newInstance() // // throws InstantiationException, IllegalAccessException;
{
    if (_createObject != null)
    {
        return (*_createObject)();
    }
    else
    {
        Assert(String::add(String::newString(_T("newInstance is not supported on ")), String::newString(name), null));
        Exception::throwE(E_NOTIMPL);
        return null;
    }
}

/**
 */
bool Class::isInstance(Object * obj)
{
    if (obj == null)
        return false;

    // BUGBUG check only classes for now...
    const Class * cl = obj->getClass();
    while (cl != null)
    {
        if (cl == this)
            return true;
        cl = cl->parent;
    }
    return false;
}

/**
 */
String * Class::getName()
{
    return String::newString(name);
}

/**
 */
Class * Class::getSuperclass()
{
    return parent;
}
