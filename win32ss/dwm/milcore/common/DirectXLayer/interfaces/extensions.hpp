// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "vector3_t.hpp"
#include "vector4_t.hpp"
#include "quaternion_t.hpp"
#include "matrix_t.hpp"

#include <vector>
#include <type_traits>

namespace dxlayer
{
    // Miscellaneous methods that either do not fit well as a member of a vector or matrix 
    // types, or cannot be implemented in one of those types directly due to circular 
    // (header file) inclusion problems arising form the fact that full definitions of 
    // some of these types would be needed at points where they wouldn't yet be available. 
    // These problems could well have been avoided by using pointers instead of objects, but 
    // we choose to take the approach of having a small utility class (this class) 
    // to work around the header-inclusion and definition-availability problems. 
    template<dxapi apiset>
    class math_extensions_t
    {
    public:
        static quaternion_t<apiset> make_quaternion_from_rotaion_matrix(const matrix_t<apiset>& matrix);
        static vector3_t<apiset> transform_coord(const vector3_t<apiset>& vector, const matrix_t<apiset>& matrix);
        static vector3_t<apiset> transform_normal(const vector3_t<apiset>& vector, const matrix_t<apiset>& matrix);
        static std::vector<vector4_t<apiset>> transform_array(
            unsigned int out_stride,
            const std::vector<vector4_t<apiset>>& in,
            unsigned int in_stride,
            const matrix_t<apiset>& transformation,
            unsigned int n);

        template<typename T> static T to_radian(T degree);
        static float get_pi();
    };
}

