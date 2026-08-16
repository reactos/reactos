#pragma once

#include <uacpi/platform/compiler.h>

#define UACPI_BUILD_BUG_ON_WITH_MSG(expr, msg) UACPI_STATIC_ASSERT(!(expr), msg)

#define UACPI_BUILD_BUG_ON(expr) \
    UACPI_BUILD_BUG_ON_WITH_MSG(expr, "BUILD BUG: " #expr " evaluated to true")

#define UACPI_EXPECT_SIZEOF(type, size)                        \
    UACPI_BUILD_BUG_ON_WITH_MSG(sizeof(type) != size,          \
                                "BUILD BUG: invalid type size")
