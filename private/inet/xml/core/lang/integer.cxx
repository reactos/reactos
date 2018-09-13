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

#include "core/lang/integer.hxx"

DEFINE_CLASS_MEMBERS_CLONING(Integer, _T("Integer"), Base);

Integer *
Integer::newInteger(int i)
{
    return new Integer(i);
}

int Integer::parseInt(String * s)
{
    // BUGBUG unicode
    // BUGBUG - Should throw and exception if conversion fails.
    RATCHAR chars = s->toCharArrayZ();
    return _ttoi(chars->getData());
}

String * Integer::toString()
{
    return String::newString(value);
}

void
IntToStr(int i, TCHAR * buf, int radix)
{
    TCHAR tbuf[32];

    Assert(radix == 10 || radix == 16);
    bool fNegative = i < 0;

    unsigned u;
    if (radix == 16)
    {
        u = i;
    }
    else if (fNegative)
    {
        u = -i;
    }
    else
    {
        u = i;
    }
    unsigned d;
    TCHAR * p = tbuf;
    do
    {
        d = u % radix;
        *p++ = d < 10 ? '0' + d : 'A' + d;
        u /= radix;
    }
    while (u != 0);
    if (radix == 16)
    {
        *p++ = _T('x');
        *p++ = _T('0');
    }
    else if (fNegative)
    {
        *p++ = _T('-');
    }
    while (p > tbuf)
        *buf++ = *--p;
    *buf++ = 0;
}
