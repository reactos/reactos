/*  
 * @(#)BaseOperand.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XQL BaseOperand object
 * 
 */


#include "core.hxx"
#pragma hdrstop

#include "operandvalue.hxx"

// BUGBUG - consider using the var parsing function from oleaut.h 

pfnComp 
OperandValue::s_aapfnComp[][LAST_TYPE + 1] = 
{
    // IS_EMPTY     IS_BOOL         IS_CY           IS_R8           IS_DATE         IS_STRING       IS_ELEMENT
    {compUNKNOWN,   compUNKOther,   compUNKOther,   compUNKOther,   compUNKOther,   compUNKOther,   compUNKOther},      // IS_EMPTY  
    {compOtherUNK,  compBOOLBOOL,   compUNKNOWN,    compUNKNOWN,    compUNKNOWN,    compBOOLString, compOpElement},     // IS_BOOL   
    {compOtherUNK,  compUNKNOWN,    compCYCY,       compCYR8,       compUNKNOWN,    compCYString,   compOpElement},     // IS_CY   
    {compOtherUNK,  compUNKNOWN,    compR8CY,       compR8R8,       compUNKNOWN,    compR8String,   compOpElement},     // IS_R8     
    {compOtherUNK,  compUNKNOWN,    compUNKNOWN,    compUNKNOWN,    compDATEDATE,   compDATEString, compOpElement},     // IS_DATE   
    {compOtherUNK,  compStringBOOL, compStringCY,   compStringR8,   compStringDATE, compStringString, compOpElement},   // IS_STRING 
    {compOtherUNK,  compElementOp,  compElementOp,  compElementOp,  compElementOp,  compElementOp,   compElementOp}   // IS_ELEMENT
};

 
TriState
OperandValue::s_aRelOpToTriState[GE_RELOP + 1][3] =
{
    // <               =               >
    {TRI_UNKNOWN,   TRI_UNKNOWN,    TRI_UNKNOWN},   // INVALID_RELOP
    {TRI_TRUE,      TRI_FALSE,      TRI_FALSE},     // LT_RELOP
    {TRI_FALSE,     TRI_TRUE,       TRI_FALSE},     // EQ_RELOP
    {TRI_TRUE,      TRI_TRUE,       TRI_FALSE},     // LE_RELOP
    {TRI_FALSE,     TRI_FALSE,      TRI_TRUE},      // GT_RELOP
    {TRI_TRUE,      TRI_FALSE,      TRI_TRUE},      // NE_RELOP
    {TRI_FALSE,     TRI_TRUE,       TRI_TRUE},      // GE_RELOP
};


TriState OperandValue::compare(RelOp op, OperandValue * popval)
{
    int result;
    TriState triResult;

    if (compare(isCaseInsensitive(op) ? NORM_IGNORECASE : 0, popval, &result))
    {
        Assert(op >= FIRST_RELOP && op <= LAST_RELOP);
    
        triResult = s_aRelOpToTriState[op & ~CASE_INSENSITIVE][result + 1];
    }
    else
    {
        triResult = TRI_UNKNOWN;
    }

    return triResult;

}


int OperandValue::compare(DWORD dwCmpFlags, OperandValue * popval, int * presult)
{
    return (*s_aapfnComp[_type][popval->_type])(dwCmpFlags, this, popval, presult);
}


int OperandValue::compUNKNOWN(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    *presult = 0;
    return false;
}


int OperandValue::compUNKOther(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    *presult = -1;
    return false;
}

int OperandValue::compOtherUNK(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    *presult = 1;
    return false;
}


int OperandValue::compOpElement(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    OperandValue opval;
    Element * e = pval2->_e;
    DataType dt = pval2->_dt;

    if (dt == DT_NONE)
    {
        dt = pval1->_dt;
    }

    e->getValue(dt, &opval);

    return (*s_aapfnComp[pval1->_type][opval._type])(dwCmpFlags, pval1, &opval, presult);
}


int OperandValue::compElementOp(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    int retval = compOpElement(dwCmpFlags, pval2, pval1, presult);
    *presult = -*presult;
    return retval;
}


int OperandValue::compBOOLBOOL(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    int i1 = pval1->_b;
    int i2 = pval2->_b;
    *presult =  CompI4I4(i1, i2);
    return true;
}


int OperandValue::compCYCY(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    CY cy1 = pval1->_cy;
    CY cy2 = pval2->_cy;

    *presult = CompCYCY(cy1, cy2);
    return true;
}


int OperandValue::compCYR8(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    CY cy1 = pval1->_cy;
    CY cy2;

    if (VarCyFromR8(pval2->_r8, &cy2) != S_OK)
    {
        return false;
    }
    *presult = CompCYCY(cy1, cy2);
    return true;
}


int OperandValue::compR8CY(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    CY cy1;
    CY cy2 = pval2->_cy;

    if (VarCyFromR8(pval1->_r8, &cy1) != S_OK)
    {
        return false;
    }
    *presult = CompCYCY(cy1, cy2);
    return true;
}


int OperandValue::compR8R8(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    double d1 = pval1->_r8;
    double d2 = pval2->_r8;

    *presult = CompR8R8(d1, d2);
    return true;
}


int OperandValue::compDATEDATE(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    DATE d1 = pval1->_date;
    DATE d2 = pval2->_date;

    *presult = CompDATEDATE(d1, d2);
    return true;
}


int OperandValue::compStringString(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    String * s1 = pval1->_s;
    String * s2 = pval2->_s;

    *presult = CompStringString(dwCmpFlags, s1, s2);
    return true;
}


int OperandValue::compStringBOOL(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    String * s = pval1->_s;
    bool  b;
    VARIANT var;
    
    V_VT(&var) = VT_EMPTY;
    if (FAILED(ParseDatatype(s->getWCHARPtr(), 
            s->length(), 
            DT_BOOLEAN, 
            &var, 
            null)))
    {
        *presult = 0;
        return false;
    }

    b = V_BOOL(&var) ? true : false;

    *presult = CompI4I4(b, pval2->_b);
    return true;
}


int OperandValue::compStringCY(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    String * s = pval1->_s;
    VARIANT var;

    V_VT(&var) = VT_EMPTY;
    if (FAILED(ParseNumeric(s->getWCHARPtr(), 
            s->length(),
            DT_FIXED_14_4, 
            &var, 
            null)))
    {
        *presult = 0;
        return false;
    }

    *presult = CompCYCY(V_CY(&var), pval2->_cy);
    return true;
}


int OperandValue::compStringR8(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    String * s = pval1->_s;
    VARIANT var;

    V_VT(&var) = VT_EMPTY;
    if (FAILED(ParseNumeric(s->getWCHARPtr(), 
            s->length(),
            DT_R8, 
            &var, 
            null)))
    {
        *presult = 0;
        return false;
    }

    *presult = CompR8R8(V_R8(&var), pval2->_r8);
    return true;
}


int OperandValue::compStringDATE(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    String * s = pval1->_s;
    VARIANT var;

    V_VT(&var) = VT_EMPTY;
    if (FAILED(ParseDatatype(s->getWCHARPtr(), 
            s->length(),
            DT_DATETIME_ISO8601TZ, 
            &var, 
            null)))
    {
        *presult = 0;
        return false;
    }

    *presult = CompDATEDATE(V_DATE(&var), pval2->_date);
    return true;
}


int OperandValue::compBOOLString(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    int retval = compStringBOOL(dwCmpFlags, pval2, pval1, presult);
    *presult = -*presult;
    return retval;
}



int OperandValue::compCYString(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    int retval = compStringCY(dwCmpFlags, pval2, pval1, presult);
    *presult = -*presult;
    return retval;

}


int OperandValue::compR8String(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    int retval = compStringR8(dwCmpFlags, pval2, pval1, presult);
    *presult = -*presult;
    return retval;

}


int OperandValue::compDATEString(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult)
{
    int retval = compStringDATE(dwCmpFlags, pval1, pval2, presult);
    *presult = -*presult;
    return retval;
}


//+--------------------------------------------------------------------------
//
//  Function:   IsNearlyEqual (local helper)
//
//  Synopsis:   Determines whether two floating-point numbers are "equal"
//              within a tolerance allowing for the usual floating-point
//              vagaries (drift, loss of precision, roundoff, etc)

int
OperandValue::IsNearlyEqual(double d1, double d2)
{
    if (d1 == 0.0)
    {
        return (d1 == d2);
    }
    else
    {
        double dRelError = (d1-d2)/d1;
        if (dRelError < 0)
            dRelError = -dRelError;
        return (dRelError < 1.0e-6);
    }
}


int OperandValue::CompI4I4(int i1, int i2)
{
    if (i1 == i2)
    {
        return 0;
    }

    if (i1 < i2)
    {
        return -1;
    }

    return 1;
}


int OperandValue::CompCYCY(CY cy1, CY cy2)
{
    if (cy1.Hi == cy2.Hi)
    {
        if (cy1.Lo == cy2.Lo)
        {
            return 0;
        }

        if (cy1.Lo < cy2.Lo)
        {
            return -1;
        }

        return 1;
    }

    if (cy1.Hi < cy2.Hi)
    {
        return -1;
    }

    return 1;
}


int OperandValue::CompR8R8(double d1, double d2)
{

    if (IsNearlyEqual(d1, d2))
    {
        return 0;
    }

    if (d1 < d2)
    {
        return -1;
    }

    return 1;
}


int OperandValue::CompDATEDATE(DATE d1, DATE d2)
{
    if (d1 == d2)
    {
        return 0;
    }

    if (d1 < d2)
    {
        return -1;
    }

    return 1;
}


int OperandValue::CompStringString(DWORD dwCmpFlags, String * s1, String * s2)
{
    return ::CompareString(
            GetThreadLocale(), //BUGBUG - perf - consider caching the lcid.  It speeds up the xsl 1%.
            dwCmpFlags, 
            s1->getWCHARPtr(), 
            s1->length(), 
            s2->getWCHARPtr(), 
            s2->length()) - 2;

}


/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
