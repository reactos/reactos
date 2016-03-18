/* Do not edit - Machine generated */

#pragma once

#define REACTOS_SOURCE_DIR "@REACTOS_SOURCE_DIR@"
#define REACTOS_BINARY_DIR "@REACTOS_BINARY_DIR@"

#if defined(__GNUC__)
#define __RELFILE__ \
    (!__builtin_strncmp(__FILE__, REACTOS_SOURCE_DIR, sizeof(REACTOS_SOURCE_DIR) - 1) \
     ? __FILE__ + sizeof(REACTOS_SOURCE_DIR) : __FILE__)
#else
#define __RELFILE__ __FILE__
#endif

/* EOF */
