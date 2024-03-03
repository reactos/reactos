/*++ NDK Version: 0099

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    section_attribs.h

Abstract:

    Preprocessor definitions to put code and data into specific sections.

Author:

    Timo Kreuzer (timo.kreuzer@reactos.org)

--*/

#pragma once

#if defined(__GNUC__) || defined(__clang__)

#define DATA_SEG(segment) __attribute__((section(segment)))
#define CODE_SEG(segment) __attribute__((section(segment)))

#elif defined(_MSC_VER)

#define DATA_SEG(segment) __declspec(allocate(segment))
#define CODE_SEG(segment) __declspec(code_seg(segment))

#else

#error Invalid compiler!

#endif

/*
 * This attribute should be applied to a function that
 * called from pageable code and raises the IRQL at DISPATCH_LEVEL or higher,
 * and restores the IRQL before it returns. We must make sure that function is not inlined.
 */
#define DECLSPEC_NOINLINE_FROM_PAGED           DECLSPEC_NOINLINE

/*
 * This attribute should be applied to a function that
 * called from non-pageable code at IRQL lower than DISPATCH_LEVEL.
 * We should make sure that function is not inlined. Some compilers (GCC)
 * can do inlining even if the function is in another section.
 * See the discussion https://gcc.gnu.org/bugzilla/show_bug.cgi?id=31362
 * for more details.
 */
#define DECLSPEC_NOINLINE_FROM_NOT_PAGED       DECLSPEC_NOINLINE
