// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "../xmcommon.hpp"

namespace dxlayer
{
    // Describes color values in a manner compatible 
    // in size and layout with D3DCOLORVALUE struct and 
    // D3DXCOLOR extensions.
    struct basetypes<dxapi::xmath>::color_base_t : public D3DCOLORVALUE
    {
    public:

#pragma region constructors

        inline color_base_t(FLOAT r, FLOAT g, FLOAT b, FLOAT a)
            : D3DCOLORVALUE({r, g, b, a}) {}

        inline color_base_t()
            : color_base_t(0.0f, 0.0f, 0.0f, 0.0f) {}

        inline color_base_t(const color_base_t& c)
            : color_base_t(c.r, c.g, c.b, c.a) {}

        inline color_base_t(uint32_t argb)
        {
            const float f = 1.0f / 255.0f;

            b = f * static_cast<float>(static_cast<uint8_t>(argb >> 0));
            g = f * static_cast<float>(static_cast<uint8_t>(argb >> 8));
            r = f * static_cast<float>(static_cast<uint8_t>(argb >> 16));
            a = f * static_cast<float>(static_cast<uint8_t>(argb >> 24));
        }

#pragma endregion

#pragma region casting

        inline operator uint32_t() const
        {
            auto R = static_cast<uint32_t>(clamp(0.0f, 1.0f, r) * 255.0f + 0.5f);
            auto G = static_cast<uint32_t>(clamp(0.0f, 1.0f, g) * 255.0f + 0.5f);
            auto B = static_cast<uint32_t>(clamp(0.0f, 1.0f, b) * 255.0f + 0.5f);
            auto A = static_cast<uint32_t>(clamp(0.0f, 1.0f, a) * 255.0f + 0.5f);

            return (A << 24) | (R << 16) | (G << 8) | (B << 0);
        }

        inline operator float*()
        {
            return &r;
        }

        inline operator float const*() const
        {
            return &r;
        }

        inline operator D3DCOLORVALUE*()
        {
            return this;
        }

        inline operator D3DCOLORVALUE const*() const
        {
            return this;
        }

        inline operator D3DCOLORVALUE&()
        {
            return *this;
        }

        inline operator D3DCOLORVALUE const&() const
        {
            return *this;
        }

#pragma endregion

#pragma region assignment operators

        inline color_base_t& operator=(const color_base_t& c)
        {
            r = c.r;
            g = c.g;
            b = c.b;
            a = c.a;

            return *this;
        }

        inline color_base_t& operator+=(const color_base_t& c)
        {
            r += c.r;
            g += c.g;
            b += c.b;
            a += c.a;

            return *this;
        }

        inline color_base_t& operator-=(const color_base_t& c)
        {
            r -= c.r;
            g -= c.g;
            b -= c.b;
            a -= c.a;

            return *this;
        }

        inline color_base_t& operator*=(float f)
        {
            r *= f;
            g *= f;
            b *= f;
            a *= f;

            return *this;
        }

        inline color_base_t& operator/=(float f)
        {
            r /= f;
            g /= f;
            b /= f;
            a /= f;

            return *this;
        }

#pragma endregion

#pragma region unary operators

        inline color_base_t operator+() const
        {
            return{ r, g, b, a };
        }

        inline color_base_t operator-() const
        {
            return{ -r, -g, -b, -a };
        }

#pragma endregion

#pragma region binary operators

        inline color_base_t operator+(const color_base_t& c) const
        {
            return (color_base_t(*this) += c);
        }

        inline color_base_t operator-(const color_base_t& c) const
        {
            return (color_base_t(*this) -= c);
        }

        inline color_base_t operator*(float f) const
        {
            return (color_base_t(*this) * f);
        }

        inline color_base_t operator/(float f) const
        {
            return (color_base_t(*this) / f);
        }

        inline friend color_base_t operator*(float f, const color_base_t& c)
        {
            return c * f;
        }

        inline bool operator==(const color_base_t& c) const
        {
            return
                comparer<float>::is_essentially_equal_to(a, c.a) &&
                comparer<float>::is_essentially_equal_to(r, c.r) &&
                comparer<float>::is_essentially_equal_to(g, c.g) &&
                comparer<float>::is_essentially_equal_to(b, c.b);
        }

        inline bool operator !=(const color_base_t& c) const
        {
            return !(*this == c);
        }

#pragma endregion

    private:
        // std::clamp won't be available until C++17, so we need to roll our own
        // implementation. This is adapted from <algorithm> that ships with VS2015.
        template<typename T, typename LessThan = std::less<T>>
        inline static const T& clamp(const T& min, const T& max, const T& value)
        {
            // cache an instance of less_than so that new
            // instances aren't created for each call into clamp<T>(...)
            static const LessThan less_than;

#if defined (DEBUG) || defined(_DEBUG)
            if (less_than(max, min))
            {
                throw dxlayer::dxlayer_exception();
            }
#endif 

            return 
                (less_than(max, value) ? 
                    max : 
                    (less_than(value, min) ? 
                        min : value));
                    
        }
        
    };
}

