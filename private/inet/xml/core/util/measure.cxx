/*
 * @(#)Measure.cxx 1.0 3/24/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#include "core/util/measure.hxx"

DEFINE_CLASS_MEMBERS_NEWINSTANCE(Measure, _T("Measure"), Base);

/**
 * This is a general purpose Measure * object to allow store
 * pixel or inch, absolute or relative (%) measurements.
 *
 * @author  Istvan Cseri
 * @version 1.0, 3/24/97
 */

Measure * Measure::parseMeasure(String * s)
{
    byte type;
    int value;
    int l = s->length();
    if (l > 0 && s->charAt(l - 1) == '%')
    {
        type = RELATIVE;
        TRY
        {
            value = Integer::parseInt(s->substring(0, l - 1));           
        }
        CATCHE
        {
            value = 0;
        }
        ENDTRY
    }
    else
    {
        type = ABSOLUTE;
        TRY
        {
            value = Integer::parseInt(s);           
        }
        CATCHE
        {
            value = 0;
        }
        ENDTRY
    }
    return new Measure(value, type);
}

bool Measure::equals(Object * obj)
{
    if (Measure::_getClass()->isInstance(obj))
    {
        Measure * m = (Measure *)obj;
        return value == m->value && type == m->type;
    }
    else if (Integer::_getClass()->isInstance(obj))
    {
        int i = ((Integer *)obj)->intValue();
        return value == i && type == ABSOLUTE;
    }
    return false;
}

String * Measure::toString()
{
    return String::add(String::newString(value),
        (type == ABSOLUTE ? String::emptyString() : String::newString(_T("%"))), 
        null);
}

