// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent generic pointer variable.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_pVoid
//
//  Synopsis:
//      Represents variable of type "void*" in prototype program.
//
//------------------------------------------------------------------------------
class C_pVoid : public C_Variable
{
public:
    C_pVoid();
    C_pVoid(C_pVoid const & origin);
    C_pVoid& operator=(C_pVoid const& origin);
    C_pVoid(void const * pOrigin);

    operator P_u8&() {return *reinterpret_cast<P_u8*>(this);}
    operator P_u16&() {return *reinterpret_cast<P_u16*>(this);}
    operator P_u32&() {return *reinterpret_cast<P_u32*>(this);}
    operator P_u8x16&() {return *reinterpret_cast<P_u8x16*>(this);}
    operator P_u16x8&() {return *reinterpret_cast<P_u16x8*>(this);}
    operator P_u32x4&() {return *reinterpret_cast<P_u32x4*>(this);}
    operator P_s32x4&() {return *reinterpret_cast<P_s32x4*>(this);}
    operator P_u64x2&() {return *reinterpret_cast<P_u64x2*>(this);}
    operator P_u128x1&() {return *reinterpret_cast<P_u128x1*>(this);}
    operator P_f32x1&() {return *reinterpret_cast<P_f32x1*>(this);}
    operator P_f32x4&() {return *reinterpret_cast<P_f32x4*>(this);}

    P_u8     const & AsP_u8    () const {return *reinterpret_cast<P_u8     const *>(this);}
    P_u16    const & AsP_u16   () const {return *reinterpret_cast<P_u16    const *>(this);}
    P_u32    const & AsP_u32   () const {return *reinterpret_cast<P_u32    const *>(this);}
    P_u8x16  const & AsP_u8x16 () const {return *reinterpret_cast<P_u8x16  const *>(this);}
    P_u16x8  const & AsP_u16x8 () const {return *reinterpret_cast<P_u16x8  const *>(this);}
    P_u32x4  const & AsP_u32x4 () const {return *reinterpret_cast<P_u32x4  const *>(this);}
    P_s32x4  const & AsP_s32x4 () const {return *reinterpret_cast<P_s32x4  const *>(this);}
    P_u64x2  const & AsP_u64x2 () const {return *reinterpret_cast<P_u64x2  const *>(this);}
    P_u128x1 const & AsP_u128x1() const {return *reinterpret_cast<P_u128x1 const *>(this);}
    P_f32x1  const & AsP_f32x1 () const {return *reinterpret_cast<P_f32x1  const *>(this);}
    P_f32x4  const & AsP_f32x4 () const {return *reinterpret_cast<P_f32x4  const *>(this);}

    P_u8     & AsP_u8    () {return *reinterpret_cast<P_u8     *>(this);}
    P_u16    & AsP_u16   () {return *reinterpret_cast<P_u16    *>(this);}
    P_u32    & AsP_u32   () {return *reinterpret_cast<P_u32    *>(this);}
    P_u8x16  & AsP_u8x16 () {return *reinterpret_cast<P_u8x16  *>(this);}
    P_u16x8  & AsP_u16x8 () {return *reinterpret_cast<P_u16x8  *>(this);}
    P_u32x4  & AsP_u32x4 () {return *reinterpret_cast<P_u32x4  *>(this);}
    P_s32x4  & AsP_s32x4 () {return *reinterpret_cast<P_s32x4  *>(this);}
    P_u64x2  & AsP_u64x2 () {return *reinterpret_cast<P_u64x2  *>(this);}
    P_u128x1 & AsP_u128x1() {return *reinterpret_cast<P_u128x1 *>(this);}
    P_f32x1  & AsP_f32x1 () {return *reinterpret_cast<P_f32x1  *>(this);}
    P_f32x4  & AsP_f32x4 () {return *reinterpret_cast<P_f32x4  *>(this);}

    static C_pVoid GetpVoidArgument(INT32 nDisplacement);
    static C_u32 GetUINT32Argument(INT32 nDisplacement);

    C_pVoid GetMemberPtr(INT32 nDisplacement) const;
    C_pVoid GetMemberPtrIndexed(INT32 nDisplacement, C_u32 const & index) const;
    C_u32 GetMemberUINT32(INT32 nDisplacement) const;
    C_XmmValue GetMemberXmm(INT32 nDisplacement) const;
    C_f32x1 GetMemberFloat1(INT32 nDisplacement) const;

// Indexing support
    static void AddOperator(
        OpType ot,
        UINT32 vResult,
        UINT32 vOperand1,
        UINT32 vOperand2,
        UINT32 vOperand3,
        RefType refType,
        UINT_PTR uDisplacement
        );

    static UINT32 ScaleIdx(UINT32 uIndexVarID, UINT32 uShift);

    void ConstOffset(int nIndexDelta);
    void ScaledOffset(C_u32 const& IndexDelta, RefType IndexScale);
};

//+-----------------------------------------------------------------------------
//
//  Template class:
//      R_void
//
//  Synopsis:
//      Represents a reference to variable of type "TypedValue" in prototype
//      program. Serves as intermediate calculation type for operator[]
//      in corresponding pointer class (P_xxx).
//
//------------------------------------------------------------------------------
template <typename TypedRef, typename TypedValue>
class R_void
{
public:
    R_void(UINT32 uBaseVarID, UINT32 uIndexVarID, UINT_PTR uDisplacement)
        : m_uBaseVarID(uBaseVarID)
        , m_uIndexVarID(uIndexVarID)
        , m_uDisplacement(uDisplacement)
    {
        WarpAssert(uBaseVarID);
        // uIndexVarID might be zero; this means no indexing
    }

    TypedValue Load(OpType ot) const
    {
        TypedValue tmp;

        if (m_uIndexVarID == 0)
        {
            C_pVoid::AddOperator(ot, tmp.GetID(), m_uBaseVarID, 0, 0, RefType_Base, m_uDisplacement);
        }
        else
        {
            __if_exists(TypedRef::IndexScale)
            {
                // Short path: use CPU ability to scale the index by 1/2/4/8
                C_pVoid::AddOperator(ot, tmp.GetID(), m_uBaseVarID, m_uIndexVarID, 0, TypedRef::IndexScale, m_uDisplacement);
            }

            __if_not_exists(TypedRef::IndexScale)
            {
                // Longer path: index should be scaled by separate instruction
                UINT32 uScaledIdxVarID = C_pVoid::ScaleIdx(m_uIndexVarID, TypedRef::IndexShift);
                C_pVoid::AddOperator(ot, tmp.GetID(), m_uBaseVarID, uScaledIdxVarID, 0, RefType_Index_1, m_uDisplacement);
            }
        }
        return tmp;
    }

    TypedValue const& Store (TypedValue const& origin, OpType ot) const
    {
        if (m_uIndexVarID == 0)
        {
            C_pVoid::AddOperator(ot, 0, origin.GetID(), m_uBaseVarID, 0, RefType_Base, m_uDisplacement);
        }
        else
        {
            __if_exists(TypedRef::IndexScale)
            {
                C_pVoid::AddOperator(ot, 0, origin.GetID(), m_uBaseVarID, m_uIndexVarID, TypedRef::IndexScale, m_uDisplacement);
            }

            __if_not_exists(TypedRef::IndexScale)
            {
                // Longer path: index should be scaled by separate instruction
                UINT32 uScaledIdxVarID = C_pVoid::ScaleIdx(m_uIndexVarID, TypedRef::IndexShift);
                C_pVoid::AddOperator(ot, 0, origin.GetID(), m_uBaseVarID, uScaledIdxVarID, RefType_Index_1, m_uDisplacement);
            }
        }

        return origin;
    }

    TypedValue BinaryOperation(TypedValue const& src, OpType ot) const
    {
        TypedValue result;

        if (m_uIndexVarID == 0)
        {
            C_pVoid::AddOperator(ot, result.GetID(), src.GetID(), m_uBaseVarID, 0, RefType_Base, m_uDisplacement);
        }
        else
        {
            __if_exists(TypedRef::IndexScale)
            {
                C_pVoid::AddOperator(ot, result.GetID(), src.GetID(), m_uBaseVarID, m_uIndexVarID, TypedRef::IndexScale, m_uDisplacement);
            }

            __if_not_exists(TypedRef::IndexScale)
            {
                // Longer path: index should be scaled by separate instruction
                UINT32 uScaledIdxVarID = C_pVoid::ScaleIdx(m_uIndexVarID, TypedRef::IndexShift);
                C_pVoid::AddOperator(ot, result.GetID(), src.GetID(), m_uBaseVarID, uScaledIdxVarID, RefType_Index_1, m_uDisplacement);
            }
        }

        return result;
    }

    TypedValue UnaryOperation(OpType ot) const
    {
        TypedValue result;

        if (m_uIndexVarID == 0)
        {
            C_pVoid::AddOperator(ot, result.GetID(), m_uBaseVarID, 0, 0, RefType_Base, m_uDisplacement);
        }
        else
        {
            __if_exists(TypedRef::IndexScale)
            {
                C_pVoid::AddOperator(ot, result.GetID(), src.GetID(), m_uBaseVarID, m_uIndexVarID, TypedRef::IndexScale, m_uDisplacement);
            }

            __if_not_exists(TypedRef::IndexScale)
            {
                // Longer path: index should be scaled by separate instruction
                UINT32 uScaledIdxVarID = C_pVoid::ScaleIdx(m_uIndexVarID, TypedRef::IndexShift);
                C_pVoid::AddOperator(ot, result.GetID(), m_uBaseVarID, uScaledIdxVarID, 0, RefType_Index_1, m_uDisplacement);
            }
        }

        return result;
    }

public:
    UINT32 const m_uBaseVarID;
    UINT32 const m_uIndexVarID;
    UINT_PTR const m_uDisplacement;
};

//+-----------------------------------------------------------------------------
//
//  Template class:
//      TIndexer
//
//  Synopsis:
//      Base class for typed pointer class, e.g. P_u32.
//      Implements indexing operators, e.g. +, +=, ++, --.
//
//------------------------------------------------------------------------------
template<typename TypedPointer, typename TypedRef>
class TIndexer : public C_pVoid
{
public:
    TIndexer() {}
    TIndexer(void * pOrigin) : C_pVoid(pOrigin) {}

    // Copy this instance, apply scaled constant offset to it.
    TypedPointer operator+(int nIndexDelta) const
    {
        TypedPointer tmp = *(TypedPointer*)this;
        if (nIndexDelta)
        {
            tmp.ConstOffset(nIndexDelta << TypedRef::IndexShift);
        }
        return tmp;
    }

    // Add scaled constant offset to this pointer.
    TypedPointer& operator+=(int nIndexDelta)
    {
        if (nIndexDelta)
        {
            ConstOffset(nIndexDelta << TypedRef::IndexShift);
        }
        return *(TypedPointer*)this;
    }

    // Copy this instance, apply scaled constant offset to it.
    TypedPointer operator-(int nIndexDelta) const
    {
        TypedPointer tmp = *(TypedPointer*)this;
        if (nIndexDelta)
        {
            tmp.ConstOffset(-nIndexDelta << TypedRef::IndexShift);
        }
        return tmp;
    }

    // Subtract scaled constant offset to this pointer.
    TypedPointer& operator-=(int nIndexDelta)
    {
        if (nIndexDelta)
        {
            ConstOffset(-nIndexDelta << TypedRef::IndexShift);
        }
        return *(TypedPointer*)this;
    }

    // Preincrement.
    TypedPointer& operator++()
    {
        return *this += 1;
    }

    // Postincrement.
    TypedPointer operator++(int)
    {
        TypedPointer tmp = *(TypedPointer*)this;
        ++*this;
        return tmp;
    }

    // Predecrement.
    TypedPointer& operator--()
    {
        return *this -= 1;
    }

    // Postdecrement.
    TypedPointer operator--(int)
    {
        TypedPointer tmp = *(TypedPointer*)this;
        --*this;
        return tmp;
    }

    // Offset with variable index.
    TypedPointer operator+(C_u32 const& IndexDelta) const
    {
        TypedPointer tmp = *(TypedPointer*)this;

        __if_exists (TypedRef::IndexScale)
        {
            tmp.ScaledOffset(IndexDelta, TypedRef::IndexScale);
        }
        __if_not_exists (TypedRef::IndexScale)
        {
            tmp.ScaledOffset(IndexDelta << TypedRef::IndexShift, RefType_Index_1);
        }

        return tmp;
    }
     
    // Offset with variable index and assign.
    TypedPointer& operator+=(C_u32 const& IndexDelta)
    {
        __if_exists (TypedRef::IndexScale)
        {
            ScaledOffset(IndexDelta, TypedRef::IndexScale);
        }
        __if_not_exists (TypedRef::IndexScale)
        {
            ScaledOffset(IndexDelta << TypedRef::IndexShift, RefType_Index_1);
        }

        return *this;
    }

    TypedRef operator[](int nIndex) const
    {
        return TypedRef(GetID(), 0, nIndex << TypedRef::IndexShift);
    }

    TypedRef operator[] (C_u32 const& index) const
    {
        return TypedRef(GetID(), index.GetID(), 0);
    }

    TypedRef operator* () const { return (*this)[0]; }

};

