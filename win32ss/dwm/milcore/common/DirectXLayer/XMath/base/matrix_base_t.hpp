// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "../xmcommon.hpp"
#include "vector4_base_t.hpp"

namespace dxlayer
{
    // Describes a 4x4 matrix 
    // This is based on the D3DMATRIX type
    struct basetypes<dxapi::xmath>::matrix_base_t : public D3DMATRIX
    {
    public:

#pragma region constructors

        inline matrix_base_t() : D3DMATRIX() {}
        inline matrix_base_t(const D3DMATRIX& mat) 
            : D3DMATRIX(mat) {}
        inline matrix_base_t(const FLOAT* values) 
            : D3DMATRIX(
        {
            values[0], values[1], values[2], values[3],
            values[4], values[5], values[6], values[7],
            values[8], values[9], values[10], values[11],
            values[12], values[13], values[14], values[15]
        }) {}

        inline matrix_base_t(
            FLOAT f11, FLOAT f12, FLOAT f13, FLOAT f14,
            FLOAT f21, FLOAT f22, FLOAT f23, FLOAT f24,
            FLOAT f31, FLOAT f32, FLOAT f33, FLOAT f34,
            FLOAT f41, FLOAT f42, FLOAT f43, FLOAT f44)
            : D3DMATRIX(
        {
            f11, f12, f13, f14,
            f21, f22, f23, f24,
            f31, f32, f33, f34,
            f41, f42, f43, f44
        }) {}

        inline matrix_base_t(const matrix_base_t& mat) 
            : matrix_base_t(
            mat._11, mat._12, mat._13, mat._14,
            mat._21, mat._22, mat._23, mat._24,
            mat._31, mat._32, mat._33, mat._34,
            mat._41, mat._42, mat._43, mat._44
        ) {}

        inline matrix_base_t(const matrix_base_t* mat) 
            : matrix_base_t(*mat) {}

#pragma endregion

#pragma region access grants

        inline FLOAT operator()(UINT row, UINT col) const
        {
            return m[row][col];
        }

        inline FLOAT& operator()(UINT row, UINT col)
        {
            return m[row][col];
        }

        inline const vector4_base_t operator[](int x) const
        {
            switch (x)
            {
            case 0:
                return{ _11, _12, _13, _14 };
                break;
            case 1:
                return{ _21, _22, _23, _24 };
                break;
            case 2:
                return{ _31, _32, _33, _34 };
                break;
            case 3:
                return{ _41, _42, _43, _44 };
                break;
            default:
                invalid_index_assert::terminate();
            }
        }

#pragma endregion

#pragma region casting operators

        inline operator FLOAT*()
        {
            return &_11;
        }

        inline operator FLOAT const*() const
        {
            return &_11;
        }

#pragma endregion

#pragma region assignment operators

        // unified copy/move assignment operator
        inline matrix_base_t& operator=(matrix_base_t mat)
        {
            _11 = mat._11;
            _12 = mat._12;
            _13 = mat._13;
            _14 = mat._14;

            _21 = mat._21;
            _22 = mat._22;
            _23 = mat._23;
            _24 = mat._24;

            _31 = mat._31;
            _32 = mat._32;
            _33 = mat._33;
            _34 = mat._34;

            _41 = mat._41;
            _42 = mat._42;
            _43 = mat._43;
            _44 = mat._44;

            return *this;
        }

        inline matrix_base_t& operator+=(const matrix_base_t& mat)
        {
            _11 += mat._11;
            _12 += mat._12;
            _13 += mat._13;
            _14 += mat._14;

            _21 += mat._21;
            _22 += mat._22;
            _23 += mat._23;
            _24 += mat._24;

            _31 += mat._31;
            _32 += mat._32;
            _33 += mat._33;
            _34 += mat._34;

            _41 += mat._41;
            _42 += mat._42;
            _43 += mat._43;
            _44 += mat._44;

            return *this;
        }

        inline matrix_base_t& operator-=(const matrix_base_t& mat)
        {
            _11 -= mat._11;
            _12 -= mat._12;
            _13 -= mat._13;
            _14 -= mat._14;

            _21 -= mat._21;
            _22 -= mat._22;
            _23 -= mat._23;
            _24 -= mat._24;

            _31 -= mat._31;
            _32 -= mat._32;
            _33 -= mat._33;
            _34 -= mat._34;

            _41 -= mat._41;
            _42 -= mat._42;
            _43 -= mat._43;
            _44 -= mat._44;

            return *this;
        }


        inline matrix_base_t& operator*=(FLOAT f)
        {
            _11 *= f;
            _12 *= f;
            _13 *= f;
            _14 *= f;

            _21 *= f;
            _22 *= f;
            _23 *= f;
            _24 *= f;

            _31 *= f;
            _32 *= f;
            _33 *= f;
            _34 *= f;

            _41 *= f;
            _42 *= f;
            _43 *= f;
            _44 *= f;

            return *this;
        }

        inline matrix_base_t& operator/=(FLOAT f)
        {
            _11 /= f;
            _12 /= f;
            _13 /= f;
            _14 /= f;

            _21 /= f;
            _22 /= f;
            _23 /= f;
            _24 /= f;

            _31 /= f;
            _32 /= f;
            _33 /= f;
            _34 /= f;

            _41 /= f;
            _42 /= f;
            _43 /= f;
            _44 /= f;

            return *this;
        }

#pragma endregion

#pragma region unary operators

        inline matrix_base_t operator+() const
        {
            return
            {
                _11, _12, _13, _14,
                _21, _22, _23, _24,
                _31, _32, _33, _34,
                _41, _42, _43, _44
            };
        }


        inline matrix_base_t operator-() const
        {
            return
            {
                -_11, -_12, -_13, -_14,
                -_21, -_22, -_23, -_24,
                -_31, -_32, -_33, -_34,
                -_41, -_42, -_43, -_44
            };
        }

#pragma endregion

#pragma region binary operators

        // matrix multiplication is not defined here - that should be dealt with 
        // by derived types.

        inline matrix_base_t operator+(const matrix_base_t& rhs) const
        {
            return (matrix_base_t(*this) += rhs);
        }

        inline matrix_base_t operator-(const matrix_base_t& rhs) const
        {
            return (matrix_base_t(*this) -= rhs);
        }

        inline matrix_base_t operator*(FLOAT f) const
        {
            return (matrix_base_t(*this) *= f);
        }

        inline matrix_base_t operator/(FLOAT f) const
        {
            return (matrix_base_t(*this) /= f);
        }

        inline friend matrix_base_t operator*(FLOAT f, matrix_base_t mat)
        {
            return mat * f;
        }

        inline bool operator==(const matrix_base_t& m) const
        {
            return (
                comparer<float>::is_essentially_equal_to(_11, m._11) &&
                comparer<float>::is_essentially_equal_to(_12, m._12) &&
                comparer<float>::is_essentially_equal_to(_13, m._13) &&
                comparer<float>::is_essentially_equal_to(_14, m._14) &&
                comparer<float>::is_essentially_equal_to(_21, m._21) &&
                comparer<float>::is_essentially_equal_to(_22, m._22) &&
                comparer<float>::is_essentially_equal_to(_23, m._23) &&
                comparer<float>::is_essentially_equal_to(_24, m._24) &&
                comparer<float>::is_essentially_equal_to(_31, m._31) &&
                comparer<float>::is_essentially_equal_to(_32, m._32) &&
                comparer<float>::is_essentially_equal_to(_33, m._33) &&
                comparer<float>::is_essentially_equal_to(_34, m._34) &&
                comparer<float>::is_essentially_equal_to(_41, m._41) &&
                comparer<float>::is_essentially_equal_to(_42, m._42) &&
                comparer<float>::is_essentially_equal_to(_43, m._43) &&
                comparer<float>::is_essentially_equal_to(_44, m._44));
        }

        inline bool operator!=(const matrix_base_t& m) const
        {
            return !(*this == m);
        }

#pragma endregion

    protected:

        inline matrix_base_t& set(const matrix_base_t& mat)
        {
            *this = mat;
            return *this;
        }
    };
}

