/*
 * PROJECT:     ReactOS UCRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ARM64 exports for the rotate functions clang renames
 * COPYRIGHT:   Copyright 2026 Jonathan McAvin <mcavinjohn@gmail.com>
 */

#include <kxarm64.h>

// the ucrt CMakeLists renames the C versions to ___name, clang treats the real names as builtins

    IMPORT ___lrotl
    IMPORT ___lrotr
    IMPORT ___rotl
    IMPORT ___rotl64
    IMPORT ___rotr
    IMPORT ___rotr64

    TEXTAREA

#undef _lrotl
#undef _lrotr
#undef _rotl
#undef _rotl64
#undef _rotr
#undef _rotr64

    LEAF_ENTRY _lrotl
    b       ___lrotl
    LEAF_END _lrotl

    LEAF_ENTRY _lrotr
    b       ___lrotr
    LEAF_END _lrotr

    LEAF_ENTRY _rotl
    b       ___rotl
    LEAF_END _rotl

    LEAF_ENTRY _rotl64
    b       ___rotl64
    LEAF_END _rotl64

    LEAF_ENTRY _rotr
    b       ___rotr
    LEAF_END _rotr

    LEAF_ENTRY _rotr64
    b       ___rotr64
    LEAF_END _rotr64

    END
