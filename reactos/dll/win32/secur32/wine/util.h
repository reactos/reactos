
#ifndef __WINE_UTIL_H__
#define __WINE_UTIL_H__

/* Functions from util.c */
ULONG ComputeCrc32(const BYTE *pData, INT iLen, ULONG initial_crc) DECLSPEC_HIDDEN;
SECURITY_STATUS SECUR32_CreateNTLM1SessionKey(PBYTE password, int len, PBYTE session_key) DECLSPEC_HIDDEN;
SECURITY_STATUS SECUR32_CreateNTLM2SubKeys(PNegoHelper helper) DECLSPEC_HIDDEN;
arc4_info *SECUR32_arc4Alloc(void) DECLSPEC_HIDDEN;
void SECUR32_arc4Init(arc4_info *a4i, const BYTE *key, unsigned int keyLen) DECLSPEC_HIDDEN;
void SECUR32_arc4Process(arc4_info *a4i, BYTE *inoutString, unsigned int length) DECLSPEC_HIDDEN;
void SECUR32_arc4Cleanup(arc4_info *a4i) DECLSPEC_HIDDEN;

#endif /* __WINE_UTIL_H__ */
