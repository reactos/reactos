/*
 * @(#)Calc.hxx 1.0 97/5/5
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _CORE_UTIL_CALC
#define _CORE_UTIL_CALC


DEFINE_CLASS(Calc);

// public: virtual class Calc
class Calc : public Base
{
    DECLARE_CLASS_MEMBERS(Calc, Base);

    public: static int muldiv(
        int nNumber,    // 32-bit signed multiplicand  
        int nNumerator, // 32-bit signed multiplier 
        int nDenominator)   // 32-bit signed divisor 
    {
        return (int)((nNumber * (long)nNumerator) / nDenominator);
    }
};



#endif _CORE_UTIL_CALC
   


