/* Do not edit - Machine generated */

#pragma once

#define REACTOS_SOURCE_DIR "@REACTOS_SOURCE_DIR@"
#define REACTOS_BINARY_DIR "@REACTOS_BINARY_DIR@"

#if defined(__GNUC__) && defined(__OPTIMIZE__)
#define __RELFILE__ \
    (!__builtin_strncmp(__FILE__, REACTOS_SOURCE_DIR, sizeof(REACTOS_SOURCE_DIR) - 1) \
     ? __FILE__ + sizeof(REACTOS_SOURCE_DIR) : __FILE__)
#endif

/* EOF */
