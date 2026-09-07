// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "vector3_t.hpp"
#include "dx9layercommon.hpp"

#include <memory>
#include <exception>

#include <d3dx9math.h>

namespace dxlayer
{
    // Represents a 3D vector based on DX9
    template<>
    class vector3_t<dxapi::d3dx9>
        : public basetypes<dxapi::d3dx9>::vector3_base_t
    {
        typedef basetypes<dxapi::d3dx9>::vector3_base_t base_t;
    public:
        template<typename... Args>
        inline vector3_t(Args&&...args) : base_t(std::forward<Args>(args)...)
        {
            // empty
        }

        inline vector3_t& operator=(const vector3_t& v)
        {
            base_t::operator=(v);
            return *this;
        }

        // construct a vector from any other type that contains 
        // fields X, Y, Z
        template <typename T>
        inline static vector3_t<dxapi::d3dx9>create(const T& source)
        {
            return vector3_t(source.X, source.Y, source.Z);
        }


        // Accessors & Setters for X, Y, Z values
        inline float& operator[](axis_3d axis)
        {
            switch (axis)
            {
            case axis_3d::X:
                return x;
                break;
            case axis_3d::Y:
                return y;
                break;
            case axis_3d::Z:
                return z;
                break;
            default:
                invalid_index_assert::terminate();
            }
        }

        // Const accessors for X, Y, Z values
        inline float operator[](axis_3d axis) const
        {
            switch (axis)
            {
            case axis_3d::X:
                return x;
                break;
            case axis_3d::Y:
                return y;
                break;
            case axis_3d::Z:
                return z;
                break;
            default:
                invalid_index_assert::terminate();
            }
        }

        // Accessors and setters for X (index = 0), Y (index = 1), Z (index = 2) values 
        inline float& operator[](int index)
        {
            return (*this)[static_cast<axis_3d>(index)];
        }

        // Const accessors for X (index = 0), Y (index = 1), Z (index = 2) values 
        float operator[](int index) const
        {
            auto retVal = (*this)[static_cast<axis_3d>(index)];

            return retVal;
        }

        // D3DXVec3Normalize
        inline vector3_t<dxapi::d3dx9>normalize() const
        {
            vector3_t out;
            D3DXVec3Normalize(&out, this);
            
            return out;
        }

        // D3DXVec3LengthSq
        inline float length_sq() const
        {
            return static_cast<float>(D3DXVec3LengthSq(this));
        }

#pragma region compute_bounding_box

        // D3DXComputeBoundingBox
        inline static vector3pair_t<dxapi::d3dx9> compute_bounding_box(
            const vector3_t<dxapi::d3dx9>* points,
            size_t num_vertices,
            unsigned long stride)
        {
            vector3_t<dxapi::d3dx9> vec_min, vec_max;

            HRESULT hResult 
                = D3DXComputeBoundingBox(points, static_cast<DWORD>(num_vertices), stride, &vec_min, &vec_max);
            if (D3D_OK != hResult)
            {
                throw dxlayer_exception(hresult(hResult));
            }

            return std::make_pair(vec_min, vec_max);
        }

        template<size_t size>
        inline static vector3pair_t<dxapi::d3dx9> compute_bounding_box(
            std::array<vector3_t<dxapi::d3dx9>, size> points,
            unsigned long stride)
        {
            return compute_bounding_box(points.data(), size, stride);
        }

        template<size_t size>
        inline static vector3pair_t<dxapi::d3dx9> compute_bounding_box(
            const vector3_t<dxapi::d3dx9>(&points)[size],
            unsigned long stride)
        {
            return compute_bounding_box(points, size, stride);
        }

#pragma endregion // compute_bounding_box

        // D3DXVec3Dot
        inline static float dot_product(const vector3_t<dxapi::d3dx9>& v1, const vector3_t<dxapi::d3dx9>& v2)
        {
            return D3DXVec3Dot(&v1, &v2);
        }

        // D3DXVec3Cross
        inline static vector3_t<dxapi::d3dx9> cross_product(const vector3_t<dxapi::d3dx9>& v1, const vector3_t<dxapi::d3dx9>& v2)
        {
            vector3_t<dxapi::d3dx9> product;
            D3DXVec3Cross(&product, &v1, &v2);

            return product;
        }

        // Type-cast operator to convert to an array of floats
        inline operator const std::array<float, 3>() const
        {
            return {x, y, z};
        }
    };
}
