#include <ntddk.h>

#ifndef _FXDYNAMICS_H_
#define _FXDYNAMICS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _WDFFUNCTIONS {
} WDFFUNCTIONS, *PWDFFUNCTIONS;

typedef struct _WDFSTRUCTURES {
} WDFSTRUCTURES, *PWDFSTRUCTURES;

typedef struct _WDFVERSION {

    ULONG         Size;
    ULONG         FuncCount;
    WDFFUNCTIONS  Functions;
    ULONG         StructCount;
    WDFSTRUCTURES Structures;

} WDFVERSION, *PWDFVERSION;

extern WDFVERSION WdfVersion;

#ifdef __cplusplus
}
#endif

#endif //_FXDYNAMICS_H_
