/*
 * @(#)Operand.hxx 1.0 4/22/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. *
 * definition of XQL Operand Interface
 * 
 */

#ifndef _XQL_QUERY_OPERANDVALUE
#define _XQL_QUERY_OPERANDVALUE

#ifndef _CORE_DATATYPE_HXX
#include "core/util/datatype.hxx"
#endif

#include "limits.h"

DEFINE_CLASS(Element);

class OperandValue;
typedef _array<OperandValue> AOperandValue;
typedef _reference<AOperandValue> RAOperandValue;


enum TriState   // hungarian: tri
{
    TRI_TRUE = 1,
    TRI_FALSE = 0,
    TRI_UNKNOWN = -1,
};


typedef int (*pfnComp)(DWORD dwCmpFlags, OperandValue *, OperandValue *, int * presult);

class OperandValue // hungarian: opval 
{
public:
    enum Type
    {
        IS_EMPTY = 0,
        IS_BOOL,
        IS_CY,
        IS_R8,
        IS_DATE,
        IS_STRING,
        IS_ELEMENT,
        LAST_TYPE = IS_ELEMENT
    };

    enum RelOp  // hungarian: relop
    {
        INVALID_RELOP = 0,
        FIRST_RELOP = 1,
        LT_RELOP = FIRST_RELOP,
        EQ_RELOP = 2,
        GT_RELOP = 4,
        LE_RELOP = LT_RELOP + EQ_RELOP, // 3
        NE_RELOP = LT_RELOP + GT_RELOP, // 5
        GE_RELOP = GT_RELOP + EQ_RELOP, // 6
        CASE_INSENSITIVE = 8,
        ILT_RELOP = CASE_INSENSITIVE + LT_RELOP, // 9
        IEQ_RELOP = CASE_INSENSITIVE + EQ_RELOP, // 10
        ILE_RELOP = CASE_INSENSITIVE + LE_RELOP, // 11
        IGT_RELOP = CASE_INSENSITIVE + GT_RELOP, // 12
        INE_RELOP = CASE_INSENSITIVE + NE_RELOP, // 13
        IGE_RELOP = CASE_INSENSITIVE + GE_RELOP, // 14
        LAST_RELOP = IGE_RELOP
    };

    OperandValue() : _type(IS_EMPTY), _isRef(false) {}

    int  empty() {_type = IS_EMPTY; return 0;}
    void initBOOL(bool b) { init(DT_BOOLEAN, IS_BOOL); _b = b; }
    void initCY(CY cy) { init(DT_FIXED_14_4, IS_CY); _cy = cy; }
    void initR8(double r8) { init(DT_R8, IS_R8); _r8 = r8; }
    void initDATE(DATE date) { init(DT_DATETIME_ISO8601TZ, IS_DATE); _date = date; }
    void initString(String * s) { init(DT_STRING, IS_STRING); _s = s; if (_isRef) {_s->AddRef();}}
    void init(DataType dt, Element * e) { init(dt, IS_ELEMENT); _e = e; }
    void makeRef() {_isRef = true; }
    void clear() {if (_isRef && isString()) { _s->Release(); } }

    bool isEmpty() { return _type == IS_EMPTY; }
    bool isR8() {return _type == IS_R8; }
    bool isI4() 
#if defined(_M_ALPHA) || defined(_M_AXP64) || defined(_M_IA64)
        {if (isR8()) { return ((_r8 > (double)INT_MIN) && (_r8 < (double)INT_MAX)); } return false;}
#else // Intel CPU
        {int i; if (isR8()) {i = _r8; return i == _r8;} return false;} 
#endif

    bool isString() {return _type == IS_STRING;} 

    TriState compare(RelOp op, OperandValue * popval);
    int compare(DWORD dwCmpFlags, OperandValue * popval, int * presult);

    static int compUNKNOWN(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);
    static int compUNKOther(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);
    static int compOtherUNK(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);
    static int compOpElement(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);
    static int compElementOp(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);
    static int compBOOLBOOL(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);
    static int compCYCY(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);
    static int compCYR8(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);
    static int compR8CY(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);
    static int compR8R8(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);
    static int compDATEDATE(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);
    static int compStringString(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);
    static int compStringBOOL(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);
    static int compStringCY(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);
    static int compStringR8(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);
    static int compStringDATE(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);
    static int compBOOLString(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);
    static int compCYString(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);
    static int compR8String(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);
    static int compDATEString(DWORD dwCmpFlags, OperandValue * pval1, OperandValue * pval2, int * presult);

    static int IsNearlyEqual(double d1, double d2);
    static int CompI4I4(int i1, int i2);
    static int CompCYCY(CY cy1, CY cy2);
    static int CompR8R8(double d1, double d2);
    static int CompDATEDATE(DATE d1, DATE d2);
    static int CompStringString(DWORD dwCmpFlags, String * s1, String * s2);
    static int isValidRelop(RelOp op) { return (op &  ~CASE_INSENSITIVE) > INVALID_RELOP && (op & ~CASE_INSENSITIVE) <= GE_RELOP;}
    static int isCaseInsensitive(RelOp op) {return op & CASE_INSENSITIVE;}

    static pfnComp s_aapfnComp[LAST_TYPE+1][LAST_TYPE + 1];
    static TriState s_aRelOpToTriState[GE_RELOP + 1][3];

    // BUGBUG - consider using bitfields to make this class smaller
    // This is important for sorting!

    Type _type;  // Type
    DataType _dt;    // DataType 
    bool _isRef; 

    union
    {
        bool        _b;
        double      _r8;
        CY          _cy;
        DATE        _date;
        String *    _s;
        Element *   _e;
    };

private:
    void init(DataType dt, Type type) { _dt = dt, _type = type;}
};

inline
TriState
TriStateFromBool(int b)
{
    Assert(TRI_TRUE == true);
    Assert(TRI_FALSE == false);
    return (TriState) b;
}

#endif _XQL_QUERY_OPERANDVALUE

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
