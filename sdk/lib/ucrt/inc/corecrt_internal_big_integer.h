//
// corecrt_internal_big_integer.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// A lightweight high precision integer type for use by the binary floating
// point <=> decimal string conversion functions.
//
#include <corecrt_internal.h>
#include <float.h>
#include <stdint.h>

// CRT_REFACTOR TODO We should be building the whole CRT /O2 /GL.  For the moment,
// just ensure that everything using big_integer is optimized for maximum speed.
#ifndef _DEBUG
    #if !defined(_BEGIN_PRAGMA_OPTIMIZE_DISABLE)
    #define _BEGIN_PRAGMA_OPTIMIZE_DISABLE(flags, bug, reason) \
        __pragma(optimize(flags, off))
    #define _BEGIN_PRAGMA_OPTIMIZE_ENABLE(flags, bug, reason) \
        __pragma(optimize(flags, on))
    #define _END_PRAGMA_OPTIMIZE() \
        __pragma(optimize("", on))
    #endif
    _BEGIN_PRAGMA_OPTIMIZE_ENABLE("gt", MSFT:4499494, "Optimize for maximum speed")
#endif

namespace __crt_strtox {

// A lightweight, sufficiently functional high-precision integer type for use in
// the binary floating point <=> decimal string conversions.  We define only the
// operations (and in some cases parts of operations) that are actually used.
//
// We require sufficient precision to represent the reciprocal of the smallest
// representable value (the smallest denormal, 2^-1074).  During parsing, we may
// also consider up to 768 decimal digits.  For this, we require an additional
// log2(10^768) bits of precision.  Finally, we require 54 bits of space for
// pre-division numerator shifting, because double explicitly stores 52 bits,
// implicitly stores 1 bit, and we need 1 more bit for rounding.
//
// PERFORMANCE NOTE:  We intentionally do not initialize the _data array when a
// big_integer object is constructed.  Profiling showed that zero initialization
// caused a substantial performance hit.  Initialization of the _data array is
// not necessary:  all operations on the big_integer type are carefully written
// to only access elements at indices [0, _used], and all operations correctly
// update _used as the utilized size increases.
struct big_integer
{
    __forceinline big_integer() throw()
        : _used(0)
    {
        #ifdef _DEBUG
        memset(_data, 0xcc, sizeof(_data));
        #endif
    }

    __forceinline big_integer(big_integer const& other) throw()
        : _used(other._used)
    {
        memcpy_s(_data, sizeof(_data), other._data, other._used * sizeof(uint32_t));
    }

    __forceinline big_integer& operator=(big_integer const& other) throw()
    {
        _used = other._used;
        memcpy_s(_data, sizeof(_data), other._data, other._used * sizeof(uint32_t));
        return *this;
    }

    enum : uint32_t
    {
        maximum_bits  =
            1074 + // 1074 bits required to represent 2^1074
            2552 + // ceil(log2(10^768))
            54,    // shift space
            
        element_bits  = sizeof(uint32_t) * CHAR_BIT,

        element_count = (maximum_bits + element_bits - 1) / element_bits
    };

    uint32_t _used;                // The number of elements currently in use
    uint32_t _data[element_count]; // The number, stored in little endian form
};

__forceinline bool __cdecl operator==(big_integer const& lhs, big_integer const& rhs) throw()
{
    if (lhs._used != rhs._used)
        return false;

    for (uint32_t i = 0; i != lhs._used; ++i)
    {
        if (lhs._data[i] != rhs._data[i])
            return false;
    }

    return true;
}

__forceinline bool __cdecl operator!=(big_integer const& lhs, big_integer const& rhs) throw()
{
    return !(rhs == lhs);
}

__forceinline bool __cdecl operator<(big_integer const& lhs, big_integer const& rhs) throw()
{
    if (lhs._used > rhs._used)
        return false;

    if (lhs._used < rhs._used)
        return true;

    uint32_t i = lhs._used - 1;
    for (; i != static_cast<uint32_t>(-1) && lhs._data[i] == rhs._data[i]; --i)
    {
        // No-op
    }

    if (i == static_cast<uint32_t>(-1))
        return false;

    if (lhs._data[i] <= rhs._data[i])
        return true;

    return false;
}

__forceinline bool __cdecl operator>=(big_integer const& lhs, big_integer const& rhs) throw()
{
    return !(lhs < rhs);
}

__forceinline big_integer __cdecl make_big_integer(uint64_t const value) throw()
{
    big_integer x{};
    x._data[0] = value & 0xffffffff;
    x._data[1] = value >> 32;
    x._used    = x._data[1] == 0 ? 1 : 2;
    return x;
}

__forceinline big_integer __cdecl make_big_integer_power_of_two(uint32_t const power) throw()
{
    uint32_t const one = 1;

    big_integer x{};

    uint32_t const element_index = power / big_integer::element_bits;
    uint32_t const bit_index     = power % big_integer::element_bits;

    memset(x._data, 0, element_index * sizeof(uint32_t));
    x._data[element_index] = (one << bit_index);
    x._used = element_index + 1;

    return x;
}

__forceinline bool __cdecl is_zero(big_integer const& value) throw()
{
    return value._used == 0;
}

__forceinline uint32_t __cdecl bit_scan_reverse(uint32_t const value) throw()
{
    unsigned long index = 0;
    if (_BitScanReverse(&index, value))
        return index + 1;
    return 0;
}

__forceinline uint32_t __cdecl bit_scan_reverse(uint64_t const value) throw()
{
    if (value > UINT32_MAX)
    {
        return bit_scan_reverse(reinterpret_cast<uint32_t const*>(&value)[1]) + 32;
    }
    else
    {
        return bit_scan_reverse(reinterpret_cast<uint32_t const*>(&value)[0]);
    }
}

__forceinline uint32_t __cdecl bit_scan_reverse(big_integer const& x) throw()
{
    if (x._used == 0)
    {
        return 0;
    }

    return (x._used - 1) * big_integer::element_bits + bit_scan_reverse(x._data[x._used - 1]);
}

// Shifts the high precision integer x by n bits to the left.  Returns true if
// the left shift was successful; false if it overflowed.  When overflow occurs,
// the high precision integer is reset to zero.
__forceinline bool __cdecl shift_left(big_integer& x, uint32_t const n) throw()
{
    uint32_t const unit_shift = n / big_integer::element_bits;
    uint32_t const bit_shift  = n % big_integer::element_bits;

    uint64_t const one = 1;

    uint32_t const msb_bits = bit_shift;
    uint32_t const lsb_bits = big_integer::element_bits - msb_bits;

    uint32_t const lsb_mask = static_cast<uint32_t>((one << lsb_bits) - one);
    uint32_t const msb_mask = ~lsb_mask;

    bool const bit_shifts_into_next_unit = bit_shift > (big_integer::element_bits - bit_scan_reverse(x._data[x._used - 1]));

    bool const unit_shift_will_overflow = x._used + unit_shift > big_integer::element_count;

    if (unit_shift_will_overflow)
    {
        x = big_integer{};
        return false;
    }

    uint32_t const new_used =
        x._used + unit_shift + static_cast<uint32_t>(bit_shifts_into_next_unit);

    if (new_used > big_integer::element_count)
    {
        x = big_integer{};
        return false;
    }

    for (uint32_t destination_index = new_used - 1; destination_index != unit_shift - 1; --destination_index)
    {
        uint32_t const upper_source_index = destination_index - unit_shift;
        uint32_t const lower_source_index = destination_index - unit_shift - 1;

        uint32_t const upper_source = upper_source_index < x._used ? x._data[upper_source_index] : 0;
        uint32_t const lower_source = lower_source_index < x._used ? x._data[lower_source_index] : 0;

        uint32_t const shifted_upper_source = (upper_source & lsb_mask) << msb_bits;
        uint32_t const shifted_lower_source = (lower_source & msb_mask) >> lsb_bits;

        uint32_t const combined_shifted_source = shifted_upper_source | shifted_lower_source;

        x._data[destination_index] = combined_shifted_source;
    }

    for (uint32_t destination_index = 0; destination_index != unit_shift; ++destination_index)
    {
        x._data[destination_index] = 0;
    }

    x._used = new_used;

    return true;
}

// Adds a 32-bit value to the high-precision integer x.  Returns true if the
// addition was successful; false if it overflowed.  When overflow occurs, the
// high precision integer is reset to zero.
__forceinline bool __cdecl add(big_integer& x, uint32_t const value) throw()
{
    if (value == 0)
    {
        return true;
    }

    uint32_t carry = value;
    for (uint32_t i = 0; i != x._used; ++i)
    {
        uint64_t const result = static_cast<uint64_t>(x._data[i]) + carry;
        x._data[i]            = static_cast<uint32_t>(result);
        carry                 = static_cast<uint32_t>(result >> 32);
    }

    if (carry != 0)
    {
        if (x._used < big_integer::element_count)
        {
            x._data[x._used] = carry;
            ++x._used;
        }
        else
        {
            x = big_integer{};
            return false;
        }
    }

    return true;
}

__forceinline uint32_t __cdecl add_carry(
    uint32_t&      u1,
    uint32_t const u2,
    uint32_t const u_carry
    ) throw()
{
    uint64_t const uu = static_cast<uint64_t>(u1) + u2 + u_carry;
    u1   = static_cast<uint32_t>(uu);
    return static_cast<uint32_t>(uu >> 32);
}

__forceinline uint32_t __cdecl add_multiply_carry(
    uint32_t&      u_add,
    uint32_t const u_mul_1,
    uint32_t const u_mul_2,
    uint32_t const u_carry
    ) throw()
{
    uint64_t const uu_res = static_cast<uint64_t>(u_mul_1) * u_mul_2 + u_add + u_carry;
    u_add = static_cast<uint32_t>(uu_res);
    return  reinterpret_cast<unsigned const*>(&uu_res)[1];
}

__forceinline uint32_t __cdecl multiply_core(
    _Inout_updates_all_(multiplicand_count) uint32_t*   const multiplicand,
                                            uint32_t    const multiplicand_count,
                                            uint32_t    const multiplier
    ) throw()
{
    uint32_t carry = 0;
    for (uint32_t i = 0; i != multiplicand_count; ++i)
    {
        uint64_t const result = static_cast<uint64_t>(multiplicand[i]) * multiplier + carry;
        multiplicand[i]       = static_cast<uint32_t>(result);
        carry                 = static_cast<uint32_t>(result >> 32);
    }

    return carry;
}


// Multiplies the high precision multiplicand by a 32-bit multiplier.  Returns
// true if the multiplication was successful; false if it overflowed.  When
// overflow occurs, the multiplicand is reset to zero.
__forceinline bool __cdecl multiply(big_integer& multiplicand, uint32_t const multiplier) throw()
{
    if (multiplier == 0)
    {
        multiplicand = big_integer{};
        return true;
    }

    if (multiplier == 1)
    {
        return true;
    }

    if (multiplicand._used == 0)
    {
        return true;
    }

    uint32_t const carry = multiply_core(multiplicand._data, multiplicand._used, multiplier);
    if (carry != 0)
    {
        if (multiplicand._used < big_integer::element_count)
        {
            multiplicand._data[multiplicand._used] = carry;
            ++multiplicand._used;
        }
        else
        {
            multiplicand = big_integer{};
            return false;
        }
    }

    return true;
}

// This high precision integer division implementation was translated from the
// implementation of System.Numerics.BigIntegerBuilder.Mul in the .NET Framework
// sources.  It multiplies the multiplicand by the multiplier and returns true
// if the multiplication was successful; false if it overflowed.  When overflow
// occurs, the multiplicand is reset to zero.
__forceinline bool __cdecl multiply(big_integer& multiplicand, big_integer const& multiplier) throw()
{
    if (multiplier._used <= 1)
    {
        return multiply(multiplicand, multiplier._data[0]);
    }

    if (multiplicand._used <= 1)
    {
        uint32_t const small_multiplier = multiplicand._data[0];
        multiplicand = multiplier;
        return multiply(multiplicand, small_multiplier);
    }

    // We prefer more iterations on the inner loop and fewer on the outer:
    bool const multiplier_is_shorter = multiplier._used < multiplicand._used;
    uint32_t const* const rgu1 = multiplier_is_shorter ? multiplier._data : multiplicand._data;
    uint32_t const* const rgu2 = multiplier_is_shorter ? multiplicand._data : multiplier._data;

    uint32_t const cu1 = multiplier_is_shorter ? multiplier._used : multiplicand._used;
    uint32_t const cu2 = multiplier_is_shorter ? multiplicand._used : multiplier._used;

    big_integer result{};
    for (uint32_t iu1 = 0; iu1 != cu1; ++iu1)
    {
        uint32_t const u_cur = rgu1[iu1];
        if (u_cur == 0)
        {
            if (iu1 == result._used)
            {
                result._data[iu1] = 0;
                result._used = iu1 + 1;
            }

            continue;
        }

        uint32_t u_carry = 0;
        uint32_t iu_res = iu1;
        for (uint32_t iu2 = 0; iu2 != cu2 && iu_res != big_integer::element_count; ++iu2, ++iu_res)
        {
            if (iu_res == result._used)
            {
                result._data[iu_res] = 0;
                result._used = iu_res + 1;
            }

            u_carry = add_multiply_carry(result._data[iu_res], u_cur, rgu2[iu2], u_carry);
        }

        while (u_carry != 0 && iu_res != big_integer::element_count)
        {
            if (iu_res == result._used)
            {
                result._data[iu_res] = 0;
                result._used = iu_res + 1;
            }

            u_carry = add_carry(result._data[iu_res++], 0, u_carry);
        }

        if (iu_res == big_integer::element_count)
        {
            multiplicand = big_integer{};
            return false;
        }
    }

    // Store the result in the multiplicand and compute the actual number of
    // elements used:
    multiplicand = result;
    return true;
}

// Multiplies the high precision integer x by 10^power.  Returns true if the
// multiplication was successful; false if it overflowed.  When overflow occurs,
// the high precision integer is reset to zero.
__forceinline bool __cdecl multiply_by_power_of_ten(big_integer& x, uint32_t const power) throw()
{
    // To improve performance, we use a table of precomputed powers of ten, from
    // 10^10 through 10^380, in increments of ten.  In its unpacked form, as an
    // array of big_integer objects, this table consists mostly of zero elements.
    // Thus, we store the table in a packed form, trimming leading and trailing
    // zero elements.  We provide an index that is used to unpack powers from the
    // table, using the function that appears after this function in this file.
    //
    // The minimum value representable with double precision is 5E-324.  With
    // this table we can thus compute most multiplications with a single multiply.
    static uint32_t const large_power_data[] = 
    {
        0x540be400, 0x00000002, 0x63100000, 0x6bc75e2d, 0x00000005, 0x40000000, 0x4674edea, 0x9f2c9cd0, 
        0x0000000c, 0xb9f56100, 0x5ca4bfab, 0x6329f1c3, 0x0000001d, 0xb5640000, 0xc40534fd, 0x926687d2, 
        0x6c3b15f9, 0x00000044, 0x10000000, 0x946590d9, 0xd762422c, 0x9a224501, 0x4f272617, 0x0000009f, 
        0x07950240, 0x245689c1, 0xc5faa71c, 0x73c86d67, 0xebad6ddc, 0x00000172, 0xcec10000, 0x63a22764, 
        0xefa418ca, 0xcdd17b25, 0x6bdfef70, 0x9dea3e1f, 0x0000035f, 0xe4000000, 0xcdc3fe6e, 0x66bc0c6a, 
        0x2e391f32, 0x5a450203, 0x71d2f825, 0xc3c24a56, 0x000007da, 0xa82e8f10, 0xaab24308, 0x8e211a7c, 
        0xf38ace40, 0x84c4ce0b, 0x7ceb0b27, 0xad2594c3, 0x00001249, 0xdd1a4000, 0xcc9f54da, 0xdc5961bf, 
        0xc75cabab, 0xf505440c, 0xd1bc1667, 0xfbb7af52, 0x608f8d29, 0x00002a94, 0x21000000, 0x17bb8a0c, 
        0x56af8ea4, 0x06479fa9, 0x5d4bb236, 0x80dc5fe0, 0xf0feaa0a, 0xa88ed940, 0x6b1a80d0, 0x00006323, 
        0x324c3864, 0x8357c796, 0xe44a42d5, 0xd9a92261, 0xbd3c103d, 0x91e5f372, 0xc0591574, 0xec1da60d, 
        0x102ad96c, 0x0000e6d3, 0x1e851000, 0x6e4f615b, 0x187b2a69, 0x0450e21c, 0x2fdd342b, 0x635027ee, 
        0xa6c97199, 0x8e4ae916, 0x17082e28, 0x1a496e6f, 0x0002196e, 0x32400000, 0x04ad4026, 0xf91e7250, 
        0x2994d1d5, 0x665bcdbb, 0xa23b2e96, 0x65fa7ddb, 0x77de53ac, 0xb020a29b, 0xc6bff953, 0x4b9425ab, 
        0x0004e34d, 0xfbc32d81, 0x5222d0f4, 0xb70f2850, 0x5713f2f3, 0xdc421413, 0xd6395d7d, 0xf8591999, 
        0x0092381c, 0x86b314d6, 0x7aa577b9, 0x12b7fe61, 0x000b616a, 0x1d11e400, 0x56c3678d, 0x3a941f20, 
        0x9b09368b, 0xbd706908, 0x207665be, 0x9b26c4eb, 0x1567e89d, 0x9d15096e, 0x7132f22b, 0xbe485113, 
        0x45e5a2ce, 0x001a7f52, 0xbb100000, 0x02f79478, 0x8c1b74c0, 0xb0f05d00, 0xa9dbc675, 0xe2d9b914, 
        0x650f72df, 0x77284b4c, 0x6df6e016, 0x514391c2, 0x2795c9cf, 0xd6e2ab55, 0x9ca8e627, 0x003db1a6, 
        0x40000000, 0xf4ecd04a, 0x7f2388f0, 0x580a6dc5, 0x43bf046f, 0xf82d5dc3, 0xee110848, 0xfaa0591c, 
        0xcdf4f028, 0x192ea53f, 0xbcd671a0, 0x7d694487, 0x10f96e01, 0x791a569d, 0x008fa475, 0xb9b2e100, 
        0x8288753c, 0xcd3f1693, 0x89b43a6b, 0x089e87de, 0x684d4546, 0xfddba60c, 0xdf249391, 0x3068ec13, 
        0x99b44427, 0xb68141ee, 0x5802cac3, 0xd96851f1, 0x7d7625a2, 0x014e718d, 0xfb640000, 0xf25a83e6, 
        0x9457ad0f, 0x0080b511, 0x2029b566, 0xd7c5d2cf, 0xa53f6d7d, 0xcdb74d1c, 0xda9d70de, 0xb716413d, 
        0x71d0ca4e, 0xd7e41398, 0x4f403a90, 0xf9ab3fe2, 0x264d776f, 0x030aafe6, 0x10000000, 0x09ab5531, 
        0xa60c58d2, 0x566126cb, 0x6a1c8387, 0x7587f4c1, 0x2c44e876, 0x41a047cf, 0xc908059e, 0xa0ba063e, 
        0xe7cfc8e8, 0xe1fac055, 0xef0144b2, 0x24207eb0, 0xd1722573, 0xe4b8f981, 0x071505ae, 0x7a3b6240, 
        0xcea45d4f, 0x4fe24133, 0x210f6d6d, 0xe55633f2, 0x25c11356, 0x28ebd797, 0xd396eb84, 0x1e493b77, 
        0x471f2dae, 0x96ad3820, 0x8afaced1, 0x4edecddb, 0x5568c086, 0xb2695da1, 0x24123c89, 0x107d4571, 
        0x1c410000, 0x6e174a27, 0xec62ae57, 0xef2289aa, 0xb6a2fbdd, 0x17e1efe4, 0x3366bdf2, 0x37b48880, 
        0xbfb82c3e, 0x19acde91, 0xd4f46408, 0x35ff6a4e, 0x67566a0e, 0x40dbb914, 0x782a3bca, 0x6b329b68, 
        0xf5afc5d9, 0x266469bc, 0xe4000000, 0xfb805ff4, 0xed55d1af, 0x9b4a20a8, 0xab9757f8, 0x01aefe0a, 
        0x4a2ca67b, 0x1ebf9569, 0xc7c41c29, 0xd8d5d2aa, 0xd136c776, 0x93da550c, 0x9ac79d90, 0x254bcba8, 
        0x0df07618, 0xf7a88809, 0x3a1f1074, 0xe54811fc, 0x59638ead, 0x97cbe710, 0x26d769e8, 0xb4e4723e, 
        0x5b90aa86, 0x9c333922, 0x4b7a0775, 0x2d47e991, 0x9a6ef977, 0x160b40e7, 0x0c92f8c4, 0xf25ff010, 
        0x25c36c11, 0xc9f98b42, 0x730b919d, 0x05ff7caf, 0xb0432d85, 0x2d2b7569, 0xa657842c, 0xd01fef10, 
        0xc77a4000, 0xe8b862e5, 0x10d8886a, 0xc8cd98e5, 0x108955c5, 0xd059b655, 0x58fbbed4, 0x03b88231, 
        0x034c4519, 0x194dc939, 0x1fc500ac, 0x794cc0e2, 0x3bc980a1, 0xe9b12dd1, 0x5e6d22f8, 0x7b38899a, 
        0xce7919d8, 0x78c67672, 0x79e5b99f, 0xe494034e, 0x00000001, 0xa1000000, 0x6c5cd4e9, 0x9be47d6f, 
        0xf93bd9e7, 0x77626fa1, 0xc68b3451, 0xde2b59e8, 0xcf3cde58, 0x2246ff58, 0xa8577c15, 0x26e77559, 
        0x17776753, 0xebe6b763, 0xe3fd0a5f, 0x33e83969, 0xa805a035, 0xf631b987, 0x211f0f43, 0xd85a43db, 
        0xab1bf596, 0x683f19a2, 0x00000004, 0xbe7dfe64, 0x4bc9042f, 0xe1f5edb0, 0x8fa14eda, 0xe409db73, 
        0x674fee9c, 0xa9159f0d, 0xf6b5b5d6, 0x7338960e, 0xeb49c291, 0x5f2b97cc, 0x0f383f95, 0x2091b3f6, 
        0xd1783714, 0xc1d142df, 0x153e22de, 0x8aafdf57, 0x77f5e55f, 0xa3e7ca8b, 0x032f525b, 0x42e74f3d, 
        0x0000000a, 0xf4dd1000, 0x5d450952, 0xaeb442e1, 0xa3b3342e, 0x3fcda36f, 0xb4287a6e, 0x4bc177f7, 
        0x67d2c8d0, 0xaea8f8e0, 0xadc93b67, 0x6cc856b3, 0x959d9d0b, 0x5b48c100, 0x4abe8a3d, 0x52d936f4, 
        0x71dbe84d, 0xf91c21c5, 0x4a458109, 0xd7aad86a, 0x08e14c7c, 0x759ba59c, 0xe43c8800, 0x00000017, 
        0x92400000, 0x04f110d4, 0x186472be, 0x8736c10c, 0x1478abfb, 0xfc51af29, 0x25eb9739, 0x4c2b3015, 
        0xa1030e0b, 0x28fe3c3b, 0x7788fcba, 0xb89e4358, 0x733de4a4, 0x7c46f2c2, 0x8f746298, 0xdb19210f, 
        0x2ea3b6ae, 0xaa5014b2, 0xea39ab8d, 0x97963442, 0x01dfdfa9, 0xd2f3d3fe, 0xa0790280, 0x00000037, 
        0x509c9b01, 0xc7dcadf1, 0x383dad2c, 0x73c64d37, 0xea6d67d0, 0x519ba806, 0xc403f2f8, 0xa052e1a2, 
        0xd710233a, 0x448573a9, 0xcf12d9ba, 0x70871803, 0x52dc3a9b, 0xe5b252e8, 0x0717fb4e, 0xbe4da62f, 
        0x0aabd7e1, 0x8c62ed4f, 0xceb9ec7b, 0xd4664021, 0xa1158300, 0xcce375e6, 0x842f29f2, 0x00000081, 
        0x7717e400, 0xd3f5fb64, 0xa0763d71, 0x7d142fe9, 0x33f44c66, 0xf3b8f12e, 0x130f0d8e, 0x734c9469, 
        0x60260fa8, 0x3c011340, 0xcc71880a, 0x37a52d21, 0x8adac9ef, 0x42bb31b4, 0xd6f94c41, 0xc88b056c, 
        0xe20501b8, 0x5297ed7c, 0x62c361c4, 0x87dad8aa, 0xb833eade, 0x94f06861, 0x13cc9abd, 0x8dc1d56a, 
        0x0000012d, 0x13100000, 0xc67a36e8, 0xf416299e, 0xf3493f0a, 0x77a5a6cf, 0xa4be23a3, 0xcca25b82, 
        0x3510722f, 0xbe9d447f, 0xa8c213b8, 0xc94c324e, 0xbc9e33ad, 0x76acfeba, 0x2e4c2132, 0x3e13cd32, 
        0x70fe91b4, 0xbb5cd936, 0x42149785, 0x46cc1afd, 0xe638ddf8, 0x690787d2, 0x1a02d117, 0x3eb5f1fe, 
        0xc3b9abae, 0x1c08ee6f, 0x000002be, 0x40000000, 0x8140c2aa, 0x2cf877d9, 0x71e1d73d, 0xd5e72f98, 
        0x72516309, 0xafa819dd, 0xd62a5a46, 0x2a02dcce, 0xce46ddfe, 0x2713248d, 0xb723d2ad, 0xc404bb19, 
        0xb706cc2b, 0x47b1ebca, 0x9d094bdc, 0xc5dc02ca, 0x31e6518e, 0x8ec35680, 0x342f58a8, 0x8b041e42, 
        0xfebfe514, 0x05fffc13, 0x6763790f, 0x66d536fd, 0xb9e15076, 0x00000662, 0x67b06100, 0xd2010a1a, 
        0xd005e1c0, 0xdb12733b, 0xa39f2e3f, 0x61b29de2, 0x2a63dce2, 0x942604bc, 0x6170d59b, 0xc2e32596, 
        0x140b75b9, 0x1f1d2c21, 0xb8136a60, 0x89d23ba2, 0x60f17d73, 0xc6cad7df, 0x0669df2b, 0x24b88737, 
        0x669306ed, 0x19496eeb, 0x938ddb6f, 0x5e748275, 0xc56e9a36, 0x3690b731, 0xc82842c5, 0x24ae798e, 
        0x00000ede, 0x41640000, 0xd5889ac1, 0xd9432c99, 0xa280e71a, 0x6bf63d2e, 0x8249793d, 0x79e7a943, 
        0x22fde64a, 0xe0d6709a, 0x05cacfef, 0xbd8da4d7, 0xe364006c, 0xa54edcb3, 0xa1a8086e, 0x748f459e, 
        0xfc8e54c8, 0xcc74c657, 0x42b8c3d4, 0x57d9636e, 0x35b55bcc, 0x6c13fee9, 0x1ac45161, 0xb595badb, 
        0xa1f14e9d, 0xdcf9e750, 0x07637f71, 0xde2f9f2b, 0x0000229d, 0x10000000, 0x3c5ebd89, 0xe3773756, 
        0x3dcba338, 0x81d29e4f, 0xa4f79e2c, 0xc3f9c774, 0x6a1ce797, 0xac5fe438, 0x07f38b9c, 0xd588ecfa, 
        0x3e5ac1ac, 0x85afccce, 0x9d1f3f70, 0xe82d6dd3, 0x177d180c, 0x5e69946f, 0x648e2ce1, 0x95a13948, 
        0x340fe011, 0xb4173c58, 0x2748f694, 0x7c2657bd, 0x758bda2e, 0x3b8090a0, 0x2ddbb613, 0x6dcf4890, 
        0x24e4047e, 0x00005099, 
    };

    struct unpack_index
    {
        uint16_t _offset; // The offset of this power's initial byte in the array
        uint8_t  _zeroes; // The number of omitted leading zero elements
        uint8_t  _size;   // The number of elements present for this power
    };

    static unpack_index const large_power_indices[] = 
    {
        {    0,  0,  2 }, {    2,  0,  3 }, {    5,  0,  4 }, {    9,  1,  4 }, 
        {   13,  1,  5 }, {   18,  1,  6 }, {   24,  2,  6 }, {   30,  2,  7 }, 
        {   37,  2,  8 }, {   45,  3,  8 }, {   53,  3,  9 }, {   62,  3, 10 }, 
        {   72,  4, 10 }, {   82,  4, 11 }, {   93,  4, 12 }, {  105,  5, 12 }, 
        {  117,  5, 13 }, {  130,  5, 14 }, {  144,  5, 15 }, {  159,  6, 15 }, 
        {  174,  6, 16 }, {  190,  6, 17 }, {  207,  7, 17 }, {  224,  7, 18 }, 
        {  242,  7, 19 }, {  261,  8, 19 }, {  280,  8, 21 }, {  301,  8, 22 }, 
        {  323,  9, 22 }, {  345,  9, 23 }, {  368,  9, 24 }, {  392, 10, 24 }, 
        {  416, 10, 25 }, {  441, 10, 26 }, {  467, 10, 27 }, {  494, 11, 27 }, 
        {  521, 11, 28 }, {  549, 11, 29 },
    };

    uint32_t large_power = power / 10;
    while (large_power  != 0)
    {
        uint32_t const current_power = large_power > _countof(large_power_indices)
            ? _countof(large_power_indices)
            : large_power;

        unpack_index const& index = large_power_indices[current_power - 1];
        big_integer multiplier{};
        multiplier._used = index._size + index._zeroes;

        uint32_t const* const source = large_power_data + index._offset;
        
        memset(multiplier._data, 0, index._zeroes * sizeof(uint32_t));
        memcpy(multiplier._data + index._zeroes, source, index._size * sizeof(uint32_t));

        if (!multiply(x, multiplier))
        {
            x = big_integer{};
            return false;
        }

        large_power -= current_power;
    }

    static uint32_t const small_powers_of_ten[9] =
    {
          10,
         100,
        1000,
        1000 *   10,
        1000 *  100,
        1000 * 1000,
        1000 * 1000 *   10,
        1000 * 1000 *  100,
        1000 * 1000 * 1000
    };

    uint32_t const small_power = power % 10;
    if (small_power != 0)
    {
        if (!multiply(x, small_powers_of_ten[small_power - 1]))
        {
            return false;
        }
    }

    return true;
}

// The following non-compiled functions are the generators for the big powers of
// ten table found in multiply_by_power_of_ten().  This code is provided for
// future use if the table needs to be amended.  Do not remove this code.
/*
uint32_t count_leading_zeroes(big_integer const& x)
{
    for (uint32_t i = 0; i != x._used; ++i)
    {
        if (x._data[i] != 0)
            return i;
    }

    return 0;
}

void generate_table()
{
    std::vector<uint32_t>     elements;
    std::vector<unpack_index> indices;

    for (uint32_t i = 10; i != 390; i += 10)
    {
        big_integer x = make_big_integer(1);
        for (uint32_t j = 0; j != i; ++j)
        {
            multiply(x, 10);
        }

        unpack_index index{};
        index._offset = elements.size();
        index._zeroes = count_leading_zeroes(x);
        index._size   = x._used - index._zeroes;

        for (uint32_t j = index._zeroes; j != x._used; ++j)
        {
            elements.push_back(x._data[j]);
        }
        indices.push_back(index);
    }

    printf("static uint32_t const large_power_data[] = \n{");
    for (uint32_t i = 0; i != elements.size(); ++i)
    {
        printf("%s0x%08x, ", i % 8 == 0 ? "\n    " : "", elements[i]);
    }
    printf("\n};\n");
    
    printf("static unpack_index const large_power_indices[] = \n{\n");
    for (uint32_t i = 0; i != indices.size(); ++i)
    {
        printf("%s{ %4u, %2u, %2u }, ",
            i % 4 == 0 ? "\n    " : "",
            indices[i]._offset,
            indices[i]._zeroes,
            indices[i]._size);
    }
    printf("};\n");
}
*/

// Computes the number of zeroes higher than the most significant set bit in 'u'
__forceinline uint32_t __cdecl count_sequential_high_zeroes(uint32_t const u) throw()
{
    unsigned long result;
    return _BitScanReverse(&result, u) ? 31 - result : 32;
}

// PERFORMANCE NOTE:  On x86, for multiplication of a 64-bit unsigned integer by
// a 32-bit unsigned integer, the compiler will generate a call to _allmul.  For
// division-heavy conversions, the inline assembly version presented here gives a
// 10% overall performance improvement (not 10% faster division--10% faster total).
// This function [1] uses only two 32-bit multiplies instead of the three required
// for general 64-bit x 64-bit multiplication, and [2] is inlineable, allowing the
// compile to elide the extreme overhead of calling the _allmul function.
#if defined(_M_IX86) && !defined(_M_HYBRID_X86_ARM64)
    __forceinline uint64_t __cdecl multiply_64_32(
        uint64_t const multiplicand,
        uint32_t const multiplier
        ) throw()
    {
       #ifdef _MSC_VER
        __asm
        {
            mov eax, dword ptr [multiplicand + 4]
            mul multiplier

            mov ecx, eax

            mov eax, dword ptr [multiplicand]
            mul multiplier

            add edx, ecx
        }
        #else // ^^^ _MSC_VER ^^^ // vvv !_MSC_VER vvv //
	    uint64_t retval;
	    __asm__(
            "mull %[multiplier]\n"
            "movl %%eax, %%ecx\n"
            "movl %[multiplicand_lo], %%eax\n"
            "mull %[multiplier]\n"
            "addl %%ecx, %%edx\n"
            : "=A" (retval)
            : [multiplicand_hi] "a" ((uint32_t)((multiplicand >> 32) & 0xFFFFFFFF)),
              [multiplicand_lo] "rm" ((uint32_t)((multiplicand >>  0) & 0xFFFFFFFF)),
              [multiplier] "rm" (multiplier)
            : "ecx" );
        return retval;
        #endif // !_MSC_VER
    }
#else
    __forceinline uint64_t __cdecl multiply_64_32(
        uint64_t const multiplicand,
        uint32_t const multiplier
        ) throw()
    {
        return multiplicand * multiplier;
    }
#endif

// This high precision integer division implementation was translated from the
// implementation of System.Numerics.BigIntegerBuilder.ModDivCore in the .NET
// Framework sources.  It computes both quotient and remainder:  the remainder
// is stored in the numerator argument, and the least significant 32 bits of the
// quotient are returned from the function.
inline uint64_t __cdecl divide(
    big_integer      & numerator,
    big_integer const& denominator
    ) throw()
{
    // If the numerator is zero, then both the quotient and remainder are zero:
    if (numerator._used == 0)
    {
        return 0;
    }

    // If the denominator is zero, then uh oh. We can't divide by zero:
    if (denominator._used == 0)
    {
        _ASSERTE(("Division by zero", false));
        return 0;
    }

    uint32_t max_numerator_element_index   = numerator._used   - 1;
    uint32_t max_denominator_element_index = denominator._used - 1;

    // The numerator and denominator are both nonzero.  If the denominator is
    // only one element wide, we can take the fast route:
    if (max_denominator_element_index == 0)
    {
        uint32_t const small_denominator = denominator._data[0];

        if (small_denominator == 1)
        {
            uint32_t const quotient = numerator._data[0];
            numerator = big_integer{};
            return quotient;
        }

        if (max_numerator_element_index == 0)
        {
            uint32_t const small_numerator = numerator._data[0];

            numerator = big_integer{};
            numerator._data[0] = small_numerator % small_denominator;
            numerator._used = numerator._data[0] > 0 ? 1 : 0;
            return small_numerator / small_denominator;
        }

        // We count down in the next loop, so the last assignment to quotient
        // will be the correct one.
        uint64_t quotient = 0;

        uint64_t uu = 0;
        for (uint32_t iv = max_numerator_element_index; iv != static_cast<uint32_t>(-1); --iv)
        {
            uu = (uu << 32) | numerator._data[iv];
            quotient = (quotient << 32) + static_cast<uint32_t>(uu / small_denominator);
            uu %= small_denominator;
        }

        numerator = big_integer{};
        numerator._data[1] = static_cast<uint32_t>(uu >> 32);
        numerator._data[0] = static_cast<uint32_t>(uu      );
        numerator._used = numerator._data[1] > 0 ? 2 : 1;
        return quotient;
    }

    if (max_denominator_element_index > max_numerator_element_index)
    {
        return 0;
    }

    uint32_t cu_den  = max_denominator_element_index + 1;
    int32_t  cu_diff = max_numerator_element_index - max_denominator_element_index;

    // Determine whether the result will have cu_diff or cu_diff + 1 digits:
    int32_t cu_quo = cu_diff;
    for (int32_t iu = max_numerator_element_index; ; --iu)
    {
        if (iu < cu_diff)
        {
            ++cu_quo;
            break;
        }

        if (denominator._data[iu - cu_diff] != numerator._data[iu])
        {
            if (denominator._data[iu - cu_diff] < numerator._data[iu])
            {
                ++cu_quo;
            }

            break;
        }
    }

    if (cu_quo == 0)
    {
        return 0;
    }

    // Get the uint to use for the trial divisions.  We normalize so the
    // high bit is set:
    uint32_t u_den      = denominator._data[cu_den - 1];
    uint32_t u_den_next = denominator._data[cu_den - 2];

    uint32_t cbit_shift_left  = count_sequential_high_zeroes(u_den);
    uint32_t cbit_shift_right = 32 - cbit_shift_left;
    if (cbit_shift_left > 0)
    {
        u_den        = (u_den << cbit_shift_left) | (u_den_next >> cbit_shift_right);
        u_den_next <<= cbit_shift_left;

        if (cu_den > 2)
        {
            u_den_next |= denominator._data[cu_den - 3] >> cbit_shift_right;
        }
    }

    uint64_t quotient{};
    for (int32_t iu = cu_quo; --iu >= 0; )
    {
        // Get the high (normalized) bits of the numerator:
        uint32_t u_num_hi = (iu + cu_den <= max_numerator_element_index)
            ? numerator._data[iu + cu_den]
            : 0;

        uint64_t uu_num = numerator._data[iu + cu_den - 1];
        reinterpret_cast<uint32_t*>(&uu_num)[1] = u_num_hi;

        uint32_t u_num_next = numerator._data[iu + cu_den - 2];
        if (cbit_shift_left > 0)
        {
            uu_num = (uu_num << cbit_shift_left) | (u_num_next >> cbit_shift_right);
            u_num_next <<= cbit_shift_left;

            if (iu + cu_den >= 3)
            {
                u_num_next |= numerator._data[iu + cu_den - 3] >> cbit_shift_right;
            }
        }

        // Divide to get the quotient digit:
        uint64_t uu_quo = uu_num / u_den;
        uint64_t uu_rem = static_cast<uint32_t>(uu_num % u_den);

        if (uu_quo > UINT32_MAX)
        {
            uu_rem += u_den * (uu_quo - UINT32_MAX);
            uu_quo  = UINT32_MAX;
        }

        while (uu_rem <= UINT32_MAX && uu_quo * u_den_next > ((uu_rem << 32) | u_num_next))
        {
            --uu_quo;
            uu_rem += u_den;
        }

        // Multiply and subtract.  Note that uu_quo may be one too large.  If
        // we have a borrow at the end, we'll add the denominator back on and
        // decrement uu_quo.
        if (uu_quo > 0)
        {
            uint64_t uu_borrow = 0;

            for (uint32_t iu2 = 0; iu2 < cu_den; ++iu2)
            {
                uu_borrow += multiply_64_32(uu_quo, denominator._data[iu2]);

                uint32_t const u_sub = static_cast<uint32_t>(uu_borrow);
                uu_borrow >>= 32;
                if (numerator._data[iu + iu2] < u_sub)
                {
                    ++uu_borrow;
                }

                numerator._data[iu + iu2] -= u_sub;
            }

            if (u_num_hi < uu_borrow)
            {
                // Add, tracking carry:
                uint32_t u_carry = 0;
                for (uint32_t iu2 = 0; iu2 < cu_den; ++iu2)
                {
                    uint64_t const sum =
                        static_cast<uint64_t>(numerator._data[iu + iu2]) +
                        static_cast<uint64_t>(denominator._data[iu2])    +
                        u_carry;

                    numerator._data[iu + iu2] = static_cast<uint32_t>(sum);
                    u_carry = sum >> 32;
                }

                --uu_quo;
            }

            max_numerator_element_index = iu + cu_den - 1;
        }

        quotient = (quotient << 32) + static_cast<uint32_t>(uu_quo);
    }

    // Trim the remainder:
    for (uint32_t i = max_numerator_element_index + 1; i < numerator._used; ++i)
    {
        numerator._data[i] = 0;
    }

    numerator._used = max_numerator_element_index + 1;
    while (numerator._used != 0 && numerator._data[numerator._used - 1] == 0)
    {
        --numerator._used;
    }

    return quotient;
}

} // namespace __crt_strtox
