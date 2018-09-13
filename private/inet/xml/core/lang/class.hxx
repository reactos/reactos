/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/*
 */

#ifndef _CORE_LANG_CLASS
#define _CORE_LANG_CLASS

DEFINE_CLASS(URL);

DEFINE_CLASS(InputStream);

DEFINE_CLASS(Class);

/**
 */

class DLLEXPORT Class : public Base
{
friend class Object;
friend class Base;

    DECLARE_CLASS_MEMBERS(Class, Base);

    /*
     */
    private:    RClass    parent;
    private:    const TCHAR * name;
    private:    CREATEOBJECT _createObject;
    private:    CREATEOBJECT _cloneCreateObject;

    /*
     */
    protected: Class(const TCHAR * name, Class * parent, CREATEOBJECT createObject, CREATEOBJECT cloneCreateObject = null);
    
    /**
     */
    public: virtual String * toString();

    /**
     */
    public: virtual Object * newInstance(); // throws InstantiationException, IllegalAccessException;

    /**
     */
    public: virtual bool isInstance(Object * obj);

    /**
     */
    public: virtual String * getName();

    /**
     */
    public: virtual Class * getSuperclass();

    public: virtual void finalize();
};


#endif _CORE_LANG_CLASS
