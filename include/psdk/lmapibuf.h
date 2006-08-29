#ifndef _LMAPIBUF_H
#define _LMAPIBUF_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif
NET_API_STATUS WINAPI NetApiBufferAllocate(DWORD,PVOID*);
NET_API_STATUS WINAPI NetApiBufferFree(PVOID);
NET_API_STATUS WINAPI NetApiBufferReallocate(PVOID,DWORD,PVOID*);
NET_API_STATUS WINAPI NetApiBufferSize(PVOID,PDWORD);
NET_API_STATUS WINAPI NetapipBufferAllocate(DWORD,PVOID*);
#ifdef __cplusplus
}
#endif
#endif
