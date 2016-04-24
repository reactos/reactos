
#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include <ntdef.h>

/* SHA Context Structure Declaration */
typedef struct
{
   ULONG Unknown[6];
   ULONG State[5];
   ULONG Count[2];
   UCHAR Buffer[64];
} SHA_CTX, *PSHA_CTX;

VOID NTAPI
A_SHAInit(PSHA_CTX Context);

VOID NTAPI
A_SHAUpdate(PSHA_CTX Context, const unsigned char *Buffer, ULONG BufferSize);

VOID NTAPI
A_SHAFinal(PSHA_CTX Context, PULONG Result);

#ifdef __cplusplus
}
#endif

