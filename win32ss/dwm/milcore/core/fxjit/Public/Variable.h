// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Base class for prototype variables.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_Variable
//
//  Synopsis:
//      Representation of a variable in prototype program.
//
//      Prototype program is the program that serves as a prototype of a real
//      program. The execution of a proto program does not assume real actions
//      but instead building a description of algorithm to do it.
//
//      Example:
//          Suppose we need to generate binary code for following C++ operator:
//          int a = b + c;
//
//      In prototype program we should place this line:
//          C_Int a = b + c;
//
//      C_Int is an example derivative of C_Variable.
//      Executing of this line will result in following:
//        - reserve the identifier for variable "a";
//        - create the operator that requests adding "b" to "c" and storing
//          the result in "a";
//        - add this operator at the end of operators list.
//
//      Consequent call to CJitterSupport::Compile will unwind operators list, map
//      variables onto registers and memory and produce desired CPU instructions.
//
//------------------------------------------------------------------------------
class C_Variable
{
private:
    // Define private operators "new" and "delete" to protect
    // against using standard allocation.
    __bcount(cbSize) void * __cdecl operator new(size_t cbSize);
    __bcount(cbSize) void * __cdecl operator new[](size_t cbSize);
    void __cdecl operator delete(void * pv);
    void __cdecl operator delete[](void * pv);

public:
    C_Variable() { m_ID = 0; } // 0 means "undefined"
    UINT32 GetID() const {return m_ID;}

    bool IsInitialized() const;

protected:
    UINT32 m_ID;
#if DBG
    static void AssertSSE41();
#else
    static void AssertSSE41() {}
#endif
};

//
// C_Variable derivatives.
//
class C_u32;
class C_s32;

#if WPFGFX_FXJIT_X86
#else // _AMD64_
class C_u64;
#endif

#if WPFGFX_FXJIT_X86
class C_MmValue;
class C_u64x1;
class C_u32x2;
class C_u16x4;
class C_u8x8;
class C_s32x2;
class C_s16x4;
#endif

class C_XmmValue;
class C_u128x1;
class C_u64x2;
class C_u32x4;
class C_u16x8;
class C_u8x16;

class C_s32x4;
class C_s16x8;

class C_f32x4;
class C_f32x1;

class C_LazyVar;

//
// Pointer types
//
class C_pVoid;
class P_u8;
class P_u16;
class P_u32;

#if WPFGFX_FXJIT_X86
class P_u64x1;
class P_u32x2;
class P_u16x4;
class P_u8x8;
#endif //WPFGFX_FXJIT_X86

class P_u128x1;
class P_u64x2;
class P_u32x4;
class P_s32x4;
class P_u16x8;
class P_u8x16;
class P_f32x1;
class P_f32x4;

//
// Reference classes.
//
class R_u32;

#if WPFGFX_FXJIT_X86
class R_u64x1;
class R_u32x2;
class R_u16x4;
class R_u8x8;
#endif

class R_u128x1;
class R_u64x2;
class R_u32x4;
class R_s32x4;
class R_u16x8;
class R_u8x16;
class R_f64x2;
class R_f32x4;
class R_f32x1;


//+-----------------------------------------------------------------------------
//
//  Enumeration:
//      RefType
//
//  Synopsis:
//      Defines the meaning of last operand of an operator.
//      Default is RefType_Direct which assumes data to come in referenced variable.
//      Remaining types are indirect; they assume data are in memory while
//      references value is either base pointer or index.
//      All indirect types involve COperator::m_uDisplacement (synonym of
//      COperator::m_pData) which is always added to combined address.
//
//
//  Details:
//
//      RefType_Index_N:
//          Last operand is an index. Base pointer can also present if an operator
//          has a room for it. Base pointer is referred to as next to last operand.
//          The fact whether or not base pointer presents is detected implicitly
//          by a number of operands.
//
//      RefType_Base:
//          Last operand is base pointer.
//
//      RefType_Static:
//          Neither base pointer nor index are involved. m_uDisplacement is
//          a pointer to a temporary copy of data. During assembly pass these
//          data will be copied to a location attached to generated binary code.
//
//      RefType_Direct:
//          Default. Last operand refers the variable. Data are located
//          either in registed on in stack frame.
//
//------------------------------------------------------------------------------
enum RefType
{
    RefType_Index_1 = 0,    // == scale_1
    RefType_Index_2 = 1,    // == scale_2
    RefType_Index_4 = 2,    // == scale_4
    RefType_Index_8 = 3,    // == scale_8
    RefType_Base    = 4,
    RefType_Static  = 5,
    RefType_Direct  = 6,
};


