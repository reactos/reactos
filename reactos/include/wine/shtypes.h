#include <rpc.h>
#include <rpcndr.h>

#include_next <shtypes.h>

#ifndef __WIDL_SHTYPES_H
#define __WIDL_SHTYPES_H
#ifdef __cplusplus
extern "C" {
#endif
#include <wtypes.h>

typedef enum tagSTRRET_TYPE {
    STRRET_WSTR = 0,
    STRRET_OFFSET = 1,
    STRRET_CSTR = 2
} STRRET_TYPE;

#ifdef __cplusplus
}
#endif
#endif /* __WIDL_SHTYPES_H */
