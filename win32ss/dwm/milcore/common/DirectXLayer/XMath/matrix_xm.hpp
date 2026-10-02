// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "matrix_t.hpp"
#include "xmcommon.hpp"
#include "factories/factory.hpp"

#include <DirectXMath.h>

namespace dxlayer
{
    // Represents a 4x4 matrix that is compatible with D3DX9 D3DXMATRIX type
    // and uses DirectX math for its internals
    template<>
    class matrix_t<dxapi::xmath> : public basetypes<dxapi::xmath>::matrix_base_t
    {
    private:
        typedef basetypes<dxapi::xmath>::matrix_base_t base_t;

    public:
#pragma region constructors

        inline matrix_t() : base_t() {}
        inline matrix_t(const base_t& mat) : base_t(mat){}
        inline matrix_t(const D3DMATRIX& mat): base_t(mat) {}
        inline matrix_t(const FLOAT* values) : base_t(values) {}
        inline matrix_t(
            FLOAT _11, FLOAT _12, FLOAT _13, FLOAT _14,
            FLOAT _21, FLOAT _22, FLOAT _23, FLOAT _24,
            FLOAT _31, FLOAT _32, FLOAT _33, FLOAT _34,
            FLOAT _41, FLOAT _42, FLOAT _43, FLOAT _44)
            : base_t(
                _11, _12, _13, _14,
                _21, _22, _23, _24,
                _31, _32, _33, _34,
                _41, _42, _43, _44
            ) {}
        inline matrix_t(const matrix_t& mat) : base_t(mat) {}
        inline matrix_t(const matrix_t* mat) : base_t(*mat) {}

        inline matrix_t(const DirectX::XMMATRIX& mat)
            : matrix_t()
        {
            DirectX::XMFLOAT4X4 xmFloat4x4;
            DirectX::XMStoreFloat4x4(&xmFloat4x4, mat);

            _11 = xmFloat4x4._11;
            _12 = xmFloat4x4._12;
            _13 = xmFloat4x4._13;
            _14 = xmFloat4x4._14;

            _21 = xmFloat4x4._21;
            _22 = xmFloat4x4._22;
            _23 = xmFloat4x4._23;
            _24 = xmFloat4x4._24;

            _31 = xmFloat4x4._31;
            _32 = xmFloat4x4._32;
            _33 = xmFloat4x4._33;
            _34 = xmFloat4x4._34;

            _41 = xmFloat4x4._41;
            _42 = xmFloat4x4._42;
            _43 = xmFloat4x4._43;
            _44 = xmFloat4x4._44;
        }

#pragma endregion

#pragma region casting operators

        inline operator DirectX::XMMATRIX() const
        {
            auto xmFloat4x4 = static_cast<DirectX::XMFLOAT4X4>(*this);

            return DirectX::XMLoadFloat4x4(&xmFloat4x4);
        }

        inline operator const DirectX::XMFLOAT4X4() const
        {
            return DirectX::XMFLOAT4X4
            (
                _11, _12, _13, _14,
                _21, _22, _23, _24,
                _31, _32, _33, _34,
                _41, _42, _43, _44
            );
        }

#pragma endregion

#pragma region assignment operators

        inline matrix_t& operator*=(const base_t& mat)
        {
            *this = *this * mat;
            return *this;
        }

#pragma endregion

#pragma region binary operators

        inline matrix_t operator*(const base_t& rhs) const
        {
            return this->multiply_by(rhs);
        }

        // Since matrix multiplication is defined by overloading
        // operator*, it hides other versions of operator* exposed
        // by the base class. We need to add them back and forward 
        // the calls to the base type.
        inline matrix_t operator*(FLOAT f) const
        {
            // make a copy, then multiply that copy by f
            return matrix_t<dxapi::xmath>(*this).base_t::operator*=(f);
        }

        inline friend matrix_t operator*(FLOAT f, const matrix_t& mat)
        {
            return mat * f;
        }

#pragma endregion

#pragma region APIs


        // D3DXMatrixDecompose ~= DirectX::XMMatrixDecompose
        inline std::tuple<vector3_t<dxapi::xmath>, quaternion_t<dxapi::xmath>, vector3_t<dxapi::xmath>> decompose() const
        {
            DirectX::XMVECTOR scale;
            DirectX::XMVECTOR rotation;
            DirectX::XMVECTOR translation;

            DirectX::XMMatrixDecompose(&scale, &rotation, &translation, *this);

            return std::make_tuple<vector3_t<dxapi::xmath>, quaternion_t<dxapi::xmath>, vector3_t<dxapi::xmath>>(scale, rotation, translation);
        }

        // D3DMatrixDeterminant ~= XMMatrixDeterminant
        inline float determinant() const
        {
            // The determinant is replicated into each component
            return DirectX::XMVectorGetX(DirectX::XMMatrixDeterminant(*this));
        }

#pragma region inverse

        // D3DXMatrixInverse ~= XMMatrixInverse
        inline matrix_t<dxapi::xmath> inverse(__out float& determinant) const
        {
            DirectX::XMVECTOR det;
            auto inv = DirectX::XMMatrixInverse(&det, *this);

            determinant = DirectX::XMVectorGetX(det);
            if (determinant == 0) // inverse is not possible
            {
                // generic DX layer error
                throw dxlayer_exception(dxerror());
            }

            return inv;
        }

        inline matrix_t<dxapi::xmath> inverse() const
        {
            auto inv = DirectX::XMMatrixInverse(nullptr, *this);
            if (DirectX::XMMatrixIsInfinite(inv))
            {
                // generic DX layer error
                throw dxlayer_exception(dxerror());
            }

            return inv;
        }

#pragma endregion

        // D3DXMatrixMultiply ~= XMMatrixMultiply
        inline matrix_t<dxapi::xmath> multiply_by(const matrix_t<dxapi::xmath>& rhs) const
        {
            return DirectX::XMMatrixMultiply(*this, rhs);
        }


        // D3DXMatrixMultiplyTranspose ~= XMMatrixMultiplyTranspose
        inline matrix_t<dxapi::xmath> multiply_transpose(const matrix_t<dxapi::xmath>& rhs) const
        {
            return DirectX::XMMatrixMultiplyTranspose(*this, rhs);
        }

        // D3DXMatrixTranspose ~= XMMatrixTranspose
        inline matrix_t<dxapi::xmath> transpose() const
        {
            return DirectX::XMMatrixTranspose(*this);
        }

        // D3DMatrixRotationAxis ~= XMMatrixRotationAxis
        inline static matrix_t<dxapi::xmath> rotation_axis(const vector3_t<dxapi::xmath>& vector, float angle)
        {
            return DirectX::XMMatrixRotationAxis(vector, angle);
        }

        // D3DXMatrixRotationX ~= XMMatrixRotationX
        inline static matrix_t<dxapi::xmath> get_rotation_x(float angle)
        {
            return DirectX::XMMatrixRotationX(angle);
        }

        // D3DXMatrixRotationY ~= XMMatrixRotationY
        inline static matrix_t<dxapi::xmath> get_rotation_y(float angle)
        {
            return DirectX::XMMatrixRotationY(angle);
        }

        // D3DXMatrixRotationZ ~= XMMatrixRotationZ
        inline static matrix_t<dxapi::xmath> get_rotation_z(float angle)
        {
            return DirectX::XMMatrixRotationZ(angle);
        }

        // D3DXMatrixScaling ~= XMMatrixScaling
        inline static matrix_t<dxapi::xmath> get_scaling(float sx, float sy, float sz)
        {
            return DirectX::XMMatrixScaling(sx, sy, sz);
        }

        // D3DXMatrixTranslation ~= XMMatrixTranslation
        inline static matrix_t<dxapi::xmath> get_translation(float x, float y, float z)
        {
            return DirectX::XMMatrixTranslation(x, y, z);
        }

        // D3DXMatrixOrthoRH ~= XMMatrixOrthographicRH
        inline static matrix_t<dxapi::xmath> get_ortho_rh(float viewWidth, float viewHeight, float nearZ, float farZ)
        {
            return DirectX::XMMatrixOrthographicRH(viewWidth, viewHeight, nearZ, farZ);
        }

        // D3DXMatrixLookAtRH ~= XMMatrixLookAtRH
        inline static matrix_t<dxapi::xmath> get_lookat_rh(
            const vector3_t<dxapi::xmath>& eye,
            const vector3_t<dxapi::xmath>& at,
            const vector3_t<dxapi::xmath>& up)
        {
            return DirectX::XMMatrixLookAtRH(eye, at, up);
        }

        // D3DXMatrixLookAtLH ~= XMMatrixLookAtLH
        inline static matrix_t<dxapi::xmath> get_lookat_lh(
            const vector3_t<dxapi::xmath>& eye,
            const vector3_t<dxapi::xmath>& at,
            const vector3_t<dxapi::xmath>& up)
        {
            return DirectX::XMMatrixLookAtLH(eye, at, up);
        }

        // D3DXMatrixPerspectiveFovLH ~= XMMatrixPerspectiveFovLH
        inline static matrix_t<dxapi::xmath> get_perspective_fov_lh(float fov_y, float aspect, float zn, float zf)
        {
            return DirectX::XMMatrixPerspectiveFovLH(fov_y, aspect, zn, zf);
        }

#pragma region identity

        // D3DXMatrixIdentity ~= XMMatrixIdentity
        inline static matrix_t<dxapi::xmath> get_identity()
        {
            return DirectX::XMMatrixIdentity();
        }

        // Reset value to an identity matrix
        inline void reset_to_identity()
        {
            set(get_identity());
        }

        // Test whether this matrix is an identity-matrix
        // D3DXMatrixIsIdentity ~= XMMatrixIsIdentity
        inline bool is_identity() const
        {
            return DirectX::XMMatrixIsIdentity(*this);
        }

#pragma endregion

        // D3DXMatrixRotationQuaternion ~= XMMatrixRotationQuaternion
        inline static matrix_t<dxapi::xmath> make_rotation(const quaternion_t<dxapi::xmath>& q)
        {
            return DirectX::XMMatrixRotationQuaternion(q);
        }

#pragma endregion
    };
}
