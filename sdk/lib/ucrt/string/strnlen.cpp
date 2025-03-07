//
// strnlen.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines strnlen and wcsnlen, which return the length of a null-terminated
// string, not including the null terminator itself, up to the specified maximum
// number of characters.
//
#include <corecrt_internal.h>
#include <corecrt_internal_simd.h>
#include <stdlib.h>
#include <string.h>



// Disable "warning C4752: found Intel(R) Advanced Vector Extensions; consider
// using /arch:AVX."  We verify that we can use AVX2 before we execute those
// instructions.
#pragma warning(disable: 4752)



namespace
{
    enum strnlen_mode
    {
        bounded,  // strnlen mode; maximum_count is respected
        unbounded, // strlen mode; maximum_count is ignored
    };
}

// This function returns true if we have reached the end of the range to be
// searched for a terminator.  For the bounded strnlen functions, we must
// test to see whether
template <strnlen_mode Mode>
static __forceinline bool __cdecl last_reached(
    void const* const it,
    void const* const last
    ) throw()
{
    return it == last;
}

template <>
__forceinline bool __cdecl last_reached<unbounded>(
    void const* const it,
    void const* const last
    ) throw()
{
    UNREFERENCED_PARAMETER(it);
    UNREFERENCED_PARAMETER(last);

    return false;
}



// An implementation of strnlen using plain C, suitable for any architecture:
template <strnlen_mode Mode, typename Element>
_Check_return_
_When_(maximum_count > _String_length_(string), _Post_satisfies_(return == _String_length_(string)))
_When_(maximum_count <= _String_length_(string), _Post_satisfies_(return == maximum_count))
static __forceinline size_t __cdecl common_strnlen_c(
    Element const* const string,
    size_t         const maximum_count
    ) throw()
{
    Element const* const last = string + maximum_count;
    Element const*       it   = string;

    for (; !last_reached<Mode>(it, last) && *it != '\0'; ++it)
    {
    }

    return static_cast<size_t>(it - string);
}

#ifdef _CRT_SIMD_SUPPORT_AVAILABLE

_UCRT_ENABLE_EXTENDED_ISA

    template <strnlen_mode Mode, __crt_simd_isa Isa, typename Element>
    _Check_return_
    _When_(maximum_count > _String_length_(string), _Post_satisfies_(return == _String_length_(string)))
    _When_(maximum_count <= _String_length_(string), _Post_satisfies_(return == maximum_count))
    static __inline size_t __cdecl common_strnlen_simd(
        Element const* const string,
        size_t         const maximum_count
        ) throw()
    {
        using traits = __crt_simd_traits<Isa, Element>;

        // For efficient SIMD processing of the string, we will use a typical three-
        // -phase computation:
        //
        // [1] We compute the number of bytes from the start of the string to the
        //     next element_size boundary and process these bytes individually.  If
        //     we find a \0 we return immediately.
        //
        // [2] At this point, we now have a pointer to an aligned block of bytes.
        //     We process bytes in element_size chunks until there are fewer than
        //     element_size bytes remaining to be examined.  If we find a chunk
        //     that contains a \0 we break out of the loop without advancing to
        //     the next chunk, to let the phase 3 loop reexamine the chunk.
        //
        // [3] We process the remaining bytes individually.  If we find a \0 we
        //     return immediately.
        //
        // Note that in phase [2] we may read bytes beyond the terminator (and thus
        // beyond the end of the string).  This is okay, because we are reading
        // aligned chunks, so a chunk will never straddle a page boundary and if we
        // can read any byte from the chunk we can read all bytes from the chunk.
        //
        // Here we go...
        uintptr_t const string_integer = reinterpret_cast<uintptr_t>(string);
        if (string_integer % traits::element_size != 0)
        {
            // If the input string is itself unaligned (e.g. if it is a wchar_t*
            // with an odd address), we can't align for vector processing.  Switch
            // back to the slow implementation:
            return common_strnlen_c<Mode>(string, maximum_count);
        }

        // [1] Alignment Loop (Prefix)
        uintptr_t const prefix_forward_offset = string_integer % traits::pack_size;
        uintptr_t const prefix_reverse_offset = prefix_forward_offset == 0
            ? 0
            : traits::pack_size - prefix_forward_offset;

        size_t const prefix_count  = __min(maximum_count, prefix_reverse_offset / traits::element_size);
        size_t const prefix_result = common_strnlen_c<bounded>(string, prefix_count);
        if (prefix_result != prefix_count)
        {
            return prefix_result;
        }

        Element const* it = string + prefix_result;

        // [2] Aligned Vector Loop (Middle)
        __crt_simd_cleanup_guard<Isa> const simd_cleanup;

        typename traits::pack_type const zero = traits::get_zero_pack();

        size_t const middle_and_suffix_count = maximum_count - prefix_count;
        size_t const suffix_count            = middle_and_suffix_count % traits::pack_size;
        size_t const middle_count            = middle_and_suffix_count - suffix_count;

        Element const* const middle_last = it + middle_count;
        while (!last_reached<Mode>(it, middle_last))
        {
            auto const element_it = reinterpret_cast<typename traits::pack_type const*>(it);

            bool const element_has_terminator = traits::compute_byte_mask(traits::compare_equals(*element_it, zero)) != 0;
            if (element_has_terminator)
            {
                break;
            }

            it += traits::elements_per_pack;
        }

        // [3] Remainder Loop (Suffix)
        Element const* const suffix_last = string + maximum_count;
        for (; !last_reached<Mode>(it, suffix_last) && *it != '\0'; ++it)
        {
        }

        // Either we have exhausted the buffer or we have found the terminator:
        return static_cast<size_t>(it - string);
    }

_UCRT_RESTORE_DEFAULT_ISA

#endif // _CRT_SIMD_SUPPORT_AVAILABLE

template <strnlen_mode Mode, typename Element>
_Check_return_
_When_(maximum_count > _String_length_(string), _Post_satisfies_(return == _String_length_(string)))
_When_(maximum_count <= _String_length_(string), _Post_satisfies_(return == maximum_count))
static __forceinline size_t __cdecl common_strnlen(
    Element const* const string,
    size_t         const maximum_count
    ) throw()
{
    #ifdef _CRT_SIMD_SUPPORT_AVAILABLE
    if (__isa_available >= __ISA_AVAILABLE_AVX2)
    {
        return common_strnlen_simd<Mode, __crt_simd_isa::avx2>(string, maximum_count);
    }
    else if (__isa_available >= __ISA_AVAILABLE_SSE2)
    {
        return common_strnlen_simd<Mode, __crt_simd_isa::sse2>(string, maximum_count);
    }
    #endif

    return common_strnlen_c<Mode>(string, maximum_count);
}

#if !defined(_M_ARM64) && !defined(_M_ARM64EC)

extern "C" size_t __cdecl strnlen(
    char const* const string,
    size_t      const maximum_count
    )
{
    return common_strnlen<bounded>(reinterpret_cast<uint8_t const*>(string), maximum_count);
}

extern "C" size_t __cdecl wcsnlen(
    wchar_t const* const string,
    size_t         const maximum_count
    )
{
    return common_strnlen<bounded>(reinterpret_cast<uint16_t const*>(string), maximum_count);
}

#pragma function(wcslen)

extern "C" size_t __cdecl wcslen(
    wchar_t const* const string
    )
{
    return common_strnlen<unbounded>(reinterpret_cast<uint16_t const*>(string), _CRT_UNBOUNDED_BUFFER_SIZE);
}

#endif // _M_ARM64
