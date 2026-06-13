// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "quaternion_t.hpp"
#include "vector4_t.hpp"
#include "xmcommon.hpp"

#include "base/quaternion_base_t.hpp"

namespace dxlayer
{
    // Represents a quaternion that is compatible with D3DX9 D3DXQUATERNION 
    // type, and uses DirectXMath for its internals.
    template<>
    class quaternion_t<dxapi::xmath> : public basetypes<dxapi::xmath>::quaternion_base_t
    {
    private:
        typedef basetypes<dxapi::xmath>::quaternion_base_t base_t;

    public:
        inline quaternion_t() : base_t() {}
        inline quaternion_t(const FLOAT* values) : base_t(values) {}
        inline quaternion_t(FLOAT x, FLOAT y, FLOAT z, FLOAT w)
            : base_t(x,y,z,w) {}

        inline quaternion_t(const vector4_t<dxapi::xmath>& vec4)
            : quaternion_t(vec4.x, vec4.y, vec4.z, vec4.w){}

        inline quaternion_t(const DirectX::XMFLOAT4 xmFloat4)
            : quaternion_t(xmFloat4.x, xmFloat4.y, xmFloat4.z, xmFloat4.w){}

        inline quaternion_t(const DirectX::XMVECTOR& xmVector)
            : quaternion_t(make_xmfloat4(xmVector)) {}

        inline quaternion_t& operator=(const quaternion_t& q)
        {
            base_t::operator=(q);
            return *this;
        }

        // Type-cast operator to convert to a 4D vector
        inline operator basetypes<dxapi::xmath>::vector4_base_t() const
        {
            return{ x,y,z,w };
        }

        // Accessors and setters for x, y, z, w
        inline virtual float& operator[](axis_4d axis)
        {
            switch (axis)
            {
            case axis_4d::X:
                return x;
                break;
            case axis_4d::Y:
                return y;
                break;
            case axis_4d::Z:
                return z;
                break;
            case axis_4d::W:
                return w;
                break;
            default:
                invalid_index_assert::terminate();
            }
        }

        // Const accessors for x, y, z, w
        inline virtual float operator[](axis_4d axis) const
        {
            return (*this)[axis];
        }

        inline operator DirectX::XMVECTOR() const
        {
            auto xmFloat4 = static_cast<DirectX::XMFLOAT4>(*this);
            return DirectX::XMLoadFloat4(&xmFloat4);
        }

    private:
        inline static DirectX::XMFLOAT4 make_xmfloat4(const DirectX::XMVECTOR& v)
        {
            DirectX::XMFLOAT4 xmFloat4;
            DirectX::XMStoreFloat4(&xmFloat4, v);

            return xmFloat4;
        }

        inline operator const DirectX::XMFLOAT4() const
        {
            return DirectX::XMFLOAT4(x, y, z, w);
        }
    };
}
