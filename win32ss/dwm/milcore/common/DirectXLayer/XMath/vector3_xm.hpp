// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "vector3_t.hpp"
#include "xmcommon.hpp"
#include <vector>

#include <DirectXMath.h>
#include <DirectXCollision.h>

namespace dxlayer
{
    // Represents a 3D vector that is size and layout compatible with the D3DX9 
    // D3DXVECTOR3 type and uses DirectXMath for its internals
    template<>
    class vector3_t<dxapi::xmath>
        : public basetypes<dxapi::xmath>::vector3_base_t
    {
    private: 
        typedef basetypes<dxapi::xmath>::vector3_base_t base_t;
    public:
        vector3_t() : base_t(){}
        vector3_t(const base_t& vec): base_t(vec){}
        vector3_t(const vector3_t& v) :base_t(v.x, v.y, v.z){}
        vector3_t(FLOAT* values) : base_t(values) {}
        vector3_t(FLOAT x, FLOAT y, FLOAT z): base_t(x,y,z) {}

        vector3_t(const DirectX::XMFLOAT3& xmFloat3)
            : vector3_t(xmFloat3.x, xmFloat3.y, xmFloat3.z)
        {}

        vector3_t(const DirectX::XMVECTOR& xmVector)
            : vector3_t(make_xmfloat3(xmVector))
        {}

        inline vector3_t& operator=(const vector3_t& v)
        {
            base_t::operator=(v);
            return *this;
        }

        // construct a vector from any other type that contains
        // fields X, Y, Z
        template<typename T>
        inline static vector3_t<dxapi::xmath>create(const T& source)
        {
            return vector3_t(source.X, source.Y, source.Z);
        }

        // Accessors & Setters for X, Y, Z values
        inline float& operator[](axis_3d axis)
        {
            switch (axis)
            {
            case dxlayer::axis_3d::X:
                return x;
                break;
            case dxlayer::axis_3d::Y:
                return y;
                break;
            case dxlayer::axis_3d::Z:
                return z;
                break;
            default:
                invalid_index_assert::terminate();
                break;
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

        vector3_t operator+(const vector3_t& v) const
        {
            return{ x + v.x, y + v.y, z + v.z };
        }

        vector3_t operator-(const vector3_t& v) const
        {
            return{ x - v.x, y - v.y, z - v.z };
        }

        vector3_t& operator+=(const vector3_t& v)
        {
            *this = *this + v;
            return *this;
        }

        vector3_t operator*(float d) const
        {
            return{ x*d, y*d, z*d };
        }

        vector3_t& operator*=(float d)
        {
            *this = *this * d;
            return *this;
        }

        friend vector3_t operator*(float d, const vector3_t& v)
        {
            return v*d;
        }


        // D3DXVec3Normalize ~= XMVector3Normalize
        inline vector3_t<dxapi::xmath> normalize() const
        {
            // implicitly converts *this to XMVECTOR before call
            // implicitly constructs vector3_t<xmath> before return
            return DirectX::XMVector3Normalize(*this);
        }

        // D3DXVec3LengthSq ~= XMVector3LengthSq
        inline float length_sq() const
        {
            // XMVector3LenghtSq replicates the square of the length of 
            // the vector into each component
            return DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(*this));
        }

        // D3DXComputeBoundingBox ~= BoundingBox::CreateFromPoints
        inline static vector3pair_t<dxapi::xmath> compute_bounding_box(
            const vector3_t<dxapi::xmath>* points,
            size_t num_vertices,
            unsigned long stride)
        {
            std::vector<DirectX::XMFLOAT3> pts;
            pts.assign(points, points + num_vertices);

            DirectX::BoundingBox box;
            DirectX::BoundingBox::CreateFromPoints(box, num_vertices,pts.data() , static_cast<size_t>(stride));

            DirectX::XMFLOAT3 vec_min(box.Center.x - box.Extents.x, box.Center.y - box.Extents.y, box.Center.z - box.Extents.z);
            DirectX::XMFLOAT3 vec_max(box.Center.x + box.Extents.x, box.Center.y + box.Extents.y, box.Center.z + box.Extents.z);

            return std::make_pair<vector3_t<dxapi::xmath>, vector3_t<dxapi::xmath>>(vec_min, vec_max);
        }

        template<size_t size>
        inline static vector3pair_t<dxapi::xmath> compute_bounding_box(
            std::array<vector3_t<dxapi::xmath>, size> points,
            unsigned long stride)
        {
            return compute_bounding_box(points.data(), size, stride);
        }

        template<size_t size>
        inline static vector3pair_t<dxapi::xmath> compute_bounding_box(
            const vector3_t<dxapi::xmath>(&points)[size],
            unsigned long stride)
        {
            return compute_bounding_box(points, size, stride);
        }

        // D3DXVec3Dot ~= DirectX::XMVector3Dot
        inline static float dot_product(const vector3_t<dxapi::xmath>& v1, const vector3_t<dxapi::xmath>& v2)
        {
            // The dot product between v1 and v2 is replicated into each component
            // returning any one component would suffice.
            return DirectX::XMVectorGetX(DirectX::XMVector3Dot(v1, v2));
        }

        // D3DXVec3Cross ~= DirectX::XMVector3Cross
        inline static vector3_t<dxapi::xmath> cross_product(const vector3_t<dxapi::xmath>& v1, const vector3_t<dxapi::xmath>& v2)
        {
            return DirectX::XMVector3Cross(v1, v2);
        }

        // Type-cast operator to convert to an array of floats
        inline operator const std::array<float, 3>() const
        {
            return{ x,y,z };
        }

        inline operator DirectX::XMVECTOR() const
        {
            auto xmFloat3 = static_cast<DirectX::XMFLOAT3>(*this);

            return DirectX::XMLoadFloat3(&xmFloat3);
        }

    private:

        inline static DirectX::XMFLOAT3 make_xmfloat3(const DirectX::XMVECTOR& v)
        {
            DirectX::XMFLOAT3 xmFloat3;
            DirectX::XMStoreFloat3(&xmFloat3, v);

            return xmFloat3;
        }

    public:
        inline operator const DirectX::XMFLOAT3() const
        {
            return DirectX::XMFLOAT3(x, y, z);
        }
    };
}
