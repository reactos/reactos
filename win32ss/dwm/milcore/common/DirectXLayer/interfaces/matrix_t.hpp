// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "dxlcommon.hpp"
#include "vector3_t.hpp"
#include "vector4_t.hpp"
#include "quaternion_t.hpp"

#include <vector>
#include <tuple>

namespace dxlayer
{
    // Represents the interfaces present in a 4x4 matrix
    // All method implementations are deferred to 
    // template specializations
    template<dxapi apiset>
    class matrix_t: public basetypes<apiset>::matrix_base_t
    {
        typedef typename basetypes<apiset>::matrix_base_t base_t;
    public:
        template<typename... Args>
        inline matrix_t(Args&&...args) : base_t(std::forward<Args>(args)...)
        {
            // empty
        }

        // Returns the Vector4 corresponding to row x
        const vector4_t<apiset> operator[](int x) const;

        // D3DXMatrixDecompose
        inline std::tuple<vector3_t<apiset>, quaternion_t<apiset>, vector3_t<apiset>> decompose() const;

        // D3DXMatrixDeterminant
        float determinant() const;

        // D3DXMatrixInverse
        matrix_t<apiset> inverse(__out float& determinant) const;
        matrix_t<apiset> inverse() const;

        // D3DXMatrixMultiply
        matrix_t<apiset> multiply_by(const matrix_t<apiset>& rhs) const;
        matrix_t<apiset> operator*(const matrix_t<apiset>& rhs) const;

        // D3DXMatrixMultiplyTranspose
        matrix_t<apiset> multiply_transpose(const matrix_t<apiset>& rhs) const;

        // D3DXMatrixTranspose
        matrix_t<apiset> transpose() const;

        // D3DXMatrixRotationAxis
        static matrix_t<apiset> rotation_axis(const vector3_t<apiset>& vector, float angle);

        // D3DXMatrixRotationX
        static matrix_t<apiset> get_rotation_x(float angle);

        // D3DXMatrixRotationY
        static matrix_t<apiset> get_rotation_y(float angle);

        // D3DXMatrixRotationZ
        static matrix_t<apiset> get_rotation_z(float angle);

        // D3DXMatrixScaling
        static matrix_t<apiset> get_scaling(float sx, float sy, float sz);

        // D3DXMatrixTranslation
        static matrix_t<apiset> get_translation(float x, float y, float z);

        // D3DXMatrixOrthoRH
        static matrix_t<apiset> get_ortho_rh(float w, float h, float zn, float zf);

        // D3DXMatrixLookAtRH
        static matrix_t<apiset> get_lookat_rh(
            const vector3_t<apiset>& eye,
            const vector3_t<apiset>& at,
            const vector3_t<apiset>& up);
        
        // D3DXMatrixLookAtLH
        static matrix_t<apiset> get_lookat_lh(
            const vector3_t<apiset>& eye,
            const vector3_t<apiset>& at,
            const vector3_t<apiset>& up
        );

        // D3DXMatrixPerspectiveFovLH
        static matrix_t<apiset> get_perspective_fov_lh(float fov_y, float aspect, float zn, float zf);

        // D3DXMatrixIdentity
        static matrix_t<apiset> get_identity();

        // D3DXMatrixRotationQuaternion
        static matrix_t<apiset> make_rotation(const quaternion_t<apiset>& q);

        // Reset value to an identity matrix
        void reset_to_identity();

        // Test whether the matrix is an identity-matrix
        bool is_identity() const;

    protected:
        // helper for copy-constructor
        matrix_t<apiset>& set(matrix_t<apiset> rhs);

        // swap two matrices
        static void swap(matrix_t<apiset>& a, matrix_t<apiset>& b);
    };
}


