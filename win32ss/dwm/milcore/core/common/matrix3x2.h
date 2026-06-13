// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  File:   matrix3x2.h
//
//  Class:  MILMatrix3x2
//
//  Light weight class for two-dimensional affine transformations.
//------------------------------------------------------------------------------

#pragma once


class MILMatrix3x2
{
public:
    float m_00, m_01, m_10, m_11, m_20, m_21;

    MILMatrix3x2() {}
    MILMatrix3x2(float m00, float m01, float m10, float m11, float m20, float m21)
    {
        m_00 = m00; m_01 = m01;
        m_10 = m10; m_11 = m11;
        m_20 = m20; m_21 = m21;
    }
    void Set(float m00, float m01, float m10, float m11, float m20, float m21)
    {
        m_00 = m00; m_01 = m01;
        m_10 = m10; m_11 = m11;
        m_20 = m20; m_21 = m21;
    }
    void SetIdentity()
    {
        m_00 = 1.f; m_01 = 0.f;
        m_10 = 0.f; m_11 = 1.f;
        m_20 = 0.f; m_21 = 0.f;
    }

    bool IsIdentity() const
    {
        return (   m_00 == 1.f && m_01 == 0.f
                && m_10 == 0.f && m_11 == 1.f
                && m_20 == 0.f && m_21 == 0.f
                   );
    }

    bool SetInverse(float m00, float m01, float m10, float m11, float m20, float m21)
    {
        float det = m00*m11 - m01*m10;
        if (det == 0.0f) return false;

        float rcp = 1.0f/det;
        if (!_finite(rcp)) return false;

        m_00 =  m11 * rcp;
        m_01 = -m01 * rcp;
        m_10 = -m10 * rcp;
        m_11 =  m00 * rcp;
        m_20 = (m10 * m21 - m20 * m11) * rcp;
        m_21 = (m20 * m01 - m00 * m21) * rcp;

        return true;
    }

    void SetScaling(float m00, float m11)
    {
        m_00 = m00; m_01 = 0.f;
        m_10 = 0.f; m_11 = m11;
        m_20 = 0.f; m_21 = 0.f;
    }

    bool IsDegenerated() const
    {
        // smallest such that 1.0+FLT_EPSILON != 1.0 
        static const float eps = FLT_EPSILON;
        return fabs(GetDeterminant()) < 10*eps;
    }

    float GetDeterminant() const
    {
        return m_00*m_11 - m_01*m_10;
    }

    void SetInverse(
        __in_ecount(1) const MILMatrix3x2& a
        )
    {
        Assert(&a != this);

        float invdet = 1.f/a.GetDeterminant();
        m_00 =  a.m_11 * invdet;
        m_01 = -a.m_01 * invdet;
        m_10 = -a.m_10 * invdet;
        m_11 =  a.m_00 * invdet;
        m_20 = (a.m_10 * a.m_21 - a.m_20 * a.m_11) * invdet;
        m_21 = (a.m_20 * a.m_01 - a.m_00 * a.m_21) * invdet;
    }

    void SetProduct(
        __in_ecount(1) const MILMatrix3x2& a,
        __in_ecount(1) const MILMatrix3x2& b
        )
    {
        Assert(&a != this);
        Assert(&b != this);

        m_00 = a.m_00 * b.m_00 + a.m_01 * b.m_10;
        m_01 = a.m_00 * b.m_01 + a.m_01 * b.m_11;
        m_10 = a.m_10 * b.m_00 + a.m_11 * b.m_10;
        m_11 = a.m_10 * b.m_01 + a.m_11 * b.m_11;
        m_20 = a.m_20 * b.m_00 + a.m_21 * b.m_10 + b.m_20;
        m_21 = a.m_20 * b.m_01 + a.m_21 * b.m_11 + b.m_21;
    }

    void TransformPoint(
        __out_ecount(1) MilPoint2F* p,
        const float x,
        const float y
        ) const
    {
        p->X = x * m_00 + y * m_10 + m_20;
        p->Y = x * m_01 + y * m_11 + m_21;
    }

    void TransformPoint(
        __out_ecount(1) MilPoint2F &dest,
        __in_ecount(1) const MilPoint2F &source
        ) const
    {
        TransformPoint(&dest, source.X, source.Y);
    }

    void TransformPoints(
        __in_ecount(count) const MilPoint2F *srcPoints,
        __out_ecount(count) MilPoint2F *destPoints,
        UINT count
        ) const;

    void Transform2DBounds(
        __in_ecount(1) const MilRectF &srcRect,
        __out_ecount(1) MilRectF &destRect
        ) const;
};

