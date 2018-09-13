/*
 * @(#)EnumWrapper.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _CORE_UTIL_ENUMWRAPPER
#define _CORE_UTIL_ENUMWRAPPER

#ifndef _CORE_UTIL_ENUMERATION
#include "core/util/enumeration.hxx"
#endif

DEFINE_CLASS(EnumWrapper);

class EnumWrapper: public Base, public Enumeration
{
private: 
	EnumWrapper() {};

public:

    DECLARE_CLASS_MEMBERS_I1(EnumWrapper, Base, Enumeration);

    DLLEXPORT static EnumWrapper * emptyEnumeration();

	DLLEXPORT static EnumWrapper * newEnumWrapper(Object * o);

    DLLEXPORT static void classInit();

    virtual bool hasMoreElements()
    {
        return (! done && object != null);
    }

    virtual Object * peekElement()
    {
        if (! done) {
            return object;
        }
        return null;
    }

    virtual Object * nextElement()
    {
        if (! done) {
            done = true;
            return object;
        }
        return null;
    }

    virtual void reset();

    void reset(Object * o)
    {
        done = false;
        object = o;
    }

    static SREnumWrapper s_emptyEnumeration;

private:

    bool done;
    RObject object;

protected: 
	
	virtual void finalize()
    {
        object = null;
        super::finalize();
    }
};

#endif _CORE_UTIL_ENUMWRAPPER

