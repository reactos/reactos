//
// SymCrypt library pre-compiled header file
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#ifdef __cplusplus
#error C++
#endif

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winnt.h>

#include "symcrypt.h"
#include "sc_lib.h"

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
#include <wmmintrin.h>
#include <immintrin.h>

    #if SYMCRYPT_GNUC
        #include <x86intrin.h>  // required for definition of _rdseed64_step for GCC 8 and earlier
        #include <xsaveintrin.h>
        #define _XCR_XFEATURE_ENABLED_MASK 0
    #endif
#endif
