#ifndef __AWGLOBAL_H__
#define __AWGLOBAL_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

#include "rsa.h"

BYTE TransByte(BYTE in);
BYTE UnTransByte(BYTE in);
WORD Conv11to5(BYTE FAR *src, BYTE FAR *dest, WORD group);
void Conv5to11(BYTE FAR *src, BYTE FAR *dest, WORD group);

void RestoreKeyFromData (BSAFE_KEY BSAFE_PTR, LPWORD);
DWORD RandDWord(void);

#ifdef __cplusplus
}
#endif

#endif
