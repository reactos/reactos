#ifndef COMPAT_UNDOC_H
#define COMPAT_UNDOC_H


typedef struct _ReactOS_ShimData
{
    DWORD dwReserved1[130];
    DWORD dwSize;
    DWORD dwMagic;
    DWORD dwReserved2[242];
    DWORD dwRosProcessCompatVersion;
} ReactOS_ShimData;


#define REACTOS_SHIMDATA_MAGIC  0xAC0DEDAB
#define REACTOS_COMPATVERSION_UNINITIALIZED 0xfffffffe
#define REACTOS_COMPATVERSION_IGNOREMANIFEST 0xffffffff

// Returns values in the form of _WIN32_WINNT_VISTA, _WIN32_WINNT_WIN7 etc
DWORD
NTAPI
RosGetProcessCompatVersion(VOID);

#endif // COMPAT_UNDOC_H
