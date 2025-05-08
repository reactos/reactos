// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "dxlcommon.hpp"
#include <array>

namespace dxlayer
{

    // Represents the interfaces present in a 3D vector
    // All method implementations are deferred to 
    // template specializations
    template<dxapi apiset>
    class vector3_t : public basetypes<apiset>::vector3_base_t
    {
        typedef typename basetypes<apiset>::vector3_base_t base_t;
    public:
        template<typename... Args>
        vector3_t(Args&&...args) 
            : base_t(std::forward<Args>(args)...)
        {
            // empty
        }

        // construct a vector from any other type that contains 
        // fields X, Y, Z
        template <typename T>
        inline static vector3_t<apiset> create(const T& source)
        {
            return vector3_t(source.X, source.Y, source.Z);
        }


        // Accessors & Setters for X, Y, Z values
        float& operator[](axis_3d axis);

        // Const accessors for X, Y, Z values
        float operator[](axis_3d axis) const;

        // Accessors and setters for X (index = 0), Y (index = 1), Z (index = 2) values 
        float& operator[](int index); 

        // Const accessors for X (index = 0), Y (index = 1), Z (index = 2) values 
        float operator[](int index) const;

        // D3DXVec3Normalize
        vector3_t<apiset> normalize() const;

        // D3DXVec3LengthSq
        float length_sq() const;

        // D3DXComputeBoundingBox
        static vector3pair_t<apiset> compute_bounding_box(
            const vector3_t<apiset>* points,
            size_t num_vertices,
            unsigned long stride);

        template<size_t size>
        static vector3pair_t<apiset> compute_bounding_box(
            std::array<vector3_t<apiset>, size> points,
            unsigned long stride);

        template<size_t size>
        static vector3pair_t<apiset> compute_bounding_box(
            const vector3_t<apiset>(&points)[size],
            unsigned long stride);


        // D3DXVec3Dot
        static float dot_product(const vector3_t<apiset>& v1, const vector3_t<apiset>& v2);

        // D3DXVec3Cross
        static vector3_t<apiset> cross_product(const vector3_t<apiset>& v1, const vector3_t<apiset>& v2);

        // Type-cast operator to convert to an array of floats
        operator const std::array<float, 3>() const;

    };
}

