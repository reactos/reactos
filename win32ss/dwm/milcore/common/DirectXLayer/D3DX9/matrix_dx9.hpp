// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "matrix_t.hpp"
#include "dx9layercommon.hpp"
#include "factories/factory.hpp"

#include <d3dx9math.h>

#include <array>
#include <functional>

namespace dxlayer
{
    // Represents a 4x4 matrix based on DX9
    template <>
    class matrix_t<dxapi::d3dx9> 
        : public basetypes<dxapi::d3dx9>::matrix_base_t
    {
        typedef basetypes<dxapi::d3dx9>::matrix_base_t base_t;
    public: 

        template<typename... Args>
        inline matrix_t(Args&&...args) : base_t(std::forward<Args>(args)...)
        {
            // empty
        }

        // copy constructor
        inline matrix_t(const matrix_t<dxapi::d3dx9>& source)
        {
            this->_11 = source._11;
            this->_12 = source._12;
            this->_13 = source._13;
            this->_14 = source._14;

            this->_21 = source._21;
            this->_22 = source._22;
            this->_23 = source._23;
            this->_24 = source._24;

            this->_31 = source._31;
            this->_32 = source._32;
            this->_33 = source._33;
            this->_34 = source._34;

            this->_41 = source._41;
            this->_42 = source._42;
            this->_43 = source._43;
            this->_44 = source._44;
        }

        // assignment operator 
        inline matrix_t<dxapi::d3dx9> operator=(const matrix_t<dxapi::d3dx9>& rhs)
        {
            return set(rhs);
        }

        // Returns the Vector4 corresponding to row x
        inline const vector4_t<dxapi::d3dx9> operator[](int x) const
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

        // D3DXMatrixDecompose
        inline std::tuple<vector3_t<dxapi::d3dx9>, quaternion_t<dxapi::d3dx9>, vector3_t<dxapi::d3dx9>> decompose() const
        {
            vector3_t<dxapi::d3dx9> scale;
            quaternion_t<dxapi::d3dx9> rotation;
            vector3_t<dxapi::d3dx9> translation;

            HRESULT hResult = S_OK;
            if (S_OK  != (hResult = D3DXMatrixDecompose(&scale, &rotation, &translation, this)))
            {
                throw dxlayer_exception(hresult(hResult));
            }

            return std::make_tuple(scale, rotation, translation);
        }

        // D3DXMatrixDeterminant
        inline float determinant() const
        {
            return D3DXMatrixDeterminant(this);
        }

        // D3DXMatrixInverse
        inline matrix_t<dxapi::d3dx9> inverse(__out float& determinant) const
        {
            matrix_t<dxapi::d3dx9> result;
            if (D3DXMatrixInverse(&result, &determinant, this) == nullptr)
            {
                // generic DX error
                throw dxlayer_exception(dxerror());
            }

            return result;
        }

        // D3DXMatrixInverse
        inline matrix_t<dxapi::d3dx9> inverse() const
        {
            matrix_t<dxapi::d3dx9> result;
            if (D3DXMatrixInverse(&result, nullptr, this) == nullptr)
            {
                // generic DX error
                throw dxlayer_exception(dxerror());
            }

            return result;
        }

        // D3DXMatrixMultiply
        inline matrix_t<dxapi::d3dx9> multiply_by(const matrix_t<dxapi::d3dx9>& rhs) const
        {
            matrix_t<dxapi::d3dx9> out;
            D3DXMatrixMultiply(&out, this, &rhs);

            return out;
        }

        // D3DXMatrixMultiply
        inline matrix_t<dxapi::d3dx9> operator*(const matrix_t<dxapi::d3dx9>& rhs) const
        {
            return this->multiply_by(rhs);
        }

        // Overload other versions of operator* and forward it to the base class version 
        inline matrix_t<dxapi::d3dx9> operator*(float f) const
        {
            return base_t::operator*(f);
        }

        inline friend matrix_t<dxapi::d3dx9> operator*(float f, const matrix_t<dxapi::d3dx9>& m)
        {
            return m * f;
        }


        // D3DXMatrixMultiplyTranspose
        inline matrix_t<dxapi::d3dx9> multiply_transpose(const matrix_t<dxapi::d3dx9>& rhs) const
        {
            matrix_t<dxapi::d3dx9> out;
            D3DXMatrixMultiplyTranspose(&out, this, &rhs);
            
            return out;
        }

        // D3DXMatrixTranspose
        inline matrix_t<dxapi::d3dx9> transpose() const
        {
            matrix_t<dxapi::d3dx9> out;
            D3DXMatrixTranspose(&out, this);

            return out;
        }

        // D3DXMatrixRotationAxis
        inline static matrix_t<dxapi::d3dx9> rotation_axis(const vector3_t<dxapi::d3dx9>& vector, float angle)
        {
            matrix_t<dxapi::d3dx9> result;
            D3DXMatrixRotationAxis(&result, &vector, angle);

            return result;
        }

        // D3DXMatrixRotationX
        inline static matrix_t<dxapi::d3dx9> get_rotation_x(float angle)
        {
            matrix_t<dxapi::d3dx9> result;
            D3DXMatrixRotationX(&result, angle);

            return result;
        }

        // D3DXMatrixRotationY
        inline static matrix_t<dxapi::d3dx9> get_rotation_y(float angle)
        {
            matrix_t<dxapi::d3dx9> result;
            D3DXMatrixRotationY(&result, angle);

            return result;
        }

        // D3DXMatrixRotationZ
        inline static matrix_t<dxapi::d3dx9> get_rotation_z(float angle)
        {
            matrix_t<dxapi::d3dx9> result;
            D3DXMatrixRotationZ(&result, angle);

            return result;
        }

        // D3DXMatrixScaling
        inline static matrix_t<dxapi::d3dx9> get_scaling(float sx, float sy, float sz)
        {
            matrix_t<dxapi::d3dx9> result;
            D3DXMatrixScaling(&result, sx, sy, sz);

            return result;
        }

        // D3DXMatrixTranslation
        inline static matrix_t<dxapi::d3dx9> get_translation(float x, float y, float z)
        {
            matrix_t<dxapi::d3dx9> result;
            D3DXMatrixTranslation(&result, x, y, z);

            return result;
        }

        // D3DXMatrixOrthoRH
        inline static matrix_t<dxapi::d3dx9> get_ortho_rh(float w, float h, float zn, float zf)
        {
            matrix_t<dxapi::d3dx9> result;
            D3DXMatrixOrthoRH(&result, w, h, zn, zf);

            return result;
        }

        // D3DXMatrixLookAtRH
        inline static matrix_t<dxapi::d3dx9> get_lookat_rh(
            const vector3_t<dxapi::d3dx9>& eye,
            const vector3_t<dxapi::d3dx9>& at,
            const vector3_t<dxapi::d3dx9>& up)
        {
            matrix_t<dxapi::d3dx9> result;
            D3DXMatrixLookAtRH(&result, &eye, &at, &up);

            return result;
        }

        // D3DXMatrixLookAtLH
        inline static matrix_t<dxapi::d3dx9> get_lookat_lh(
            const vector3_t<dxapi::d3dx9>& eye,
            const vector3_t<dxapi::d3dx9>& at,
            const vector3_t<dxapi::d3dx9>& up)
        {
            matrix_t<dxapi::d3dx9> result;
            D3DXMatrixLookAtLH(&result, &eye, &at, &up);

            return result;
        }

        // D3DXMatrixPerspectiveFovLH
        inline static matrix_t<dxapi::d3dx9> get_perspective_fov_lh(float fov_y, float aspect, float zn, float zf)
        {
            matrix_t<dxapi::d3dx9> result;
            D3DXMatrixPerspectiveFovLH(&result, fov_y, aspect, zn, zf);

            return result;
        }

        // D3DXMatrixIdentity
        inline static matrix_t<dxapi::d3dx9> get_identity()
        {
            matrix_t<dxapi::d3dx9> result;
            D3DXMatrixIdentity(&result);

            return result;
        }

        // Reset value to an identity matrix
        inline void reset_to_identity()
        {
            set(get_identity());
        }

        // Test whether the matrix is an identity-matrix
        inline bool is_identity() const
        {
            return D3DXMatrixIsIdentity(this) ? true : false;
        }

        // D3DXMatrixRotationQuaternion
        inline static matrix_t<dxapi::d3dx9> make_rotation(const quaternion_t<dxapi::d3dx9>& q)
        {
            matrix_t<dxapi::d3dx9> m;
            D3DXMatrixRotationQuaternion(&m, &q);
            return m;
        }


    protected:
        // helper for copy-constructor
        // argument is intentionally passed by value rather than 
        //   being passed by const-reference
        inline matrix_t<dxapi::d3dx9>& set(matrix_t<dxapi::d3dx9> rhs)
        {
            matrix_t<dxapi::d3dx9>::swap(*this, rhs);
            return *this;
        }

        // swap two matrices
        inline static void swap(matrix_t<dxapi::d3dx9>& a, matrix_t<dxapi::d3dx9>& b)
        {
            std::swap(a._11, b._11);
            std::swap(a._12, b._12);
            std::swap(a._13, b._13);
            std::swap(a._14, b._14);

            std::swap(a._21, b._21);
            std::swap(a._22, b._22);
            std::swap(a._23, b._23);
            std::swap(a._24, b._24);

            std::swap(a._31, b._31);
            std::swap(a._32, b._32);
            std::swap(a._33, b._33);
            std::swap(a._34, b._34);

            std::swap(a._41, b._41);
            std::swap(a._42, b._42);
            std::swap(a._43, b._43);
            std::swap(a._44, b._44);
        }

    };

}
