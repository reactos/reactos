#ifndef __CRT_INTERNAL_POPEN_H
#define __CRT_INTERNAL_POPEN_H

#ifndef _CRT_PRECOMP_H
#error DO NOT INCLUDE THIS HEADER DIRECTLY
#endif

struct popen_handle {
    FILE *f;
    HANDLE proc;
};
extern struct popen_handle *popen_handles;
extern DWORD popen_handles_size;

#endif
