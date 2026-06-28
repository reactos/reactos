/*
 * Copyright (c) 2026 Terascale Functionalists
 * SPDX-License-Identifier: MIT
 *
 * C++ standard-library feature fixups for ReactOS msvcrt builds.
 */

#pragma once

#if defined(__cplusplus) && defined(PAL_STDCPP_COMPAT)
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#if defined(__has_include)
#if __has_include(<bits/c++config.h>)
#include <bits/c++config.h>
#endif
#else
#include <bits/c++config.h>
#endif

#undef _GLIBCXX_HAVE_AT_QUICK_EXIT
#undef _GLIBCXX_HAVE_QUICK_EXIT
#endif
