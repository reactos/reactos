/*
 * @(#)EnumWrapper.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

// use ABSTRACT because of no default constructor 
DEFINE_ABSTRACT_CLASS_MEMBERS(EnumWrapper, _T("EnumWrapper"), Base);

SREnumWrapper EnumWrapper::s_emptyEnumeration;

EnumWrapper * EnumWrapper::emptyEnumeration()
{
    return s_emptyEnumeration;
}

EnumWrapper * EnumWrapper::newEnumWrapper(Object * o)
{
	EnumWrapper * ew = new EnumWrapper();
	ew->object = o;
        ew->reset();
	return ew;
}

void EnumWrapper::classInit()
{
    if (!EnumWrapper::s_emptyEnumeration)
	{
		EnumWrapper::s_emptyEnumeration = new EnumWrapper();
        EnumWrapper::s_emptyEnumeration->object = null;
        EnumWrapper::s_emptyEnumeration->done = false;
	}
}
void
EnumWrapper::reset()
{
     done = false;
}
