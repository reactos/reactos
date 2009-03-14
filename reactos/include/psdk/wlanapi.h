#ifndef _WLANAPI_H
#define _WLANAPI_H

#ifdef __cplusplus
extern "C" {
#endif

PVOID WINAPI WlanAllocateMemory(DWORD dwSize);
VOID WINAPI WlanFreeMemory(PVOID pMemory);

#ifdef __cplusplus
}
#endif


#endif  // _WLANAPI_H
