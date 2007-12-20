#ifndef _WINGLUE_H
#define _WINGLUE_H

#include <host/typedefs.h>

#define DLL_PROCESS_ATTACH 1
#define DLL_PROCESS_DETACH 0

#define IMAGE_FILE_EXECUTABLE_IMAGE	2
#define IMAGE_FILE_LARGE_ADDRESS_AWARE	32
#define IMAGE_FILE_32BIT_MACHINE	256
#define IMAGE_FILE_DLL	8192
#define IMAGE_SUBSYSTEM_NATIVE		1
#define IMAGE_SUBSYSTEM_WINDOWS_GUI	2
#define IMAGE_SUBSYSTEM_WINDOWS_CUI	3
#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16
#define IMAGE_FILE_MACHINE_UNKNOWN      0
#define IMAGE_FILE_MACHINE_I386	        0x014c
#define IMAGE_FILE_MACHINE_ALPHA        0x0184
#define IMAGE_FILE_MACHINE_POWERPC      0x01f0
#define IMAGE_FILE_MACHINE_AMD64        0x8664
#define IMAGE_NT_SIGNATURE 0x00004550
#define IMAGE_DLLCHARACTERISTICS_NX_COMPAT 0x0100
#define IMAGE_SIZEOF_NT_OPTIONAL32_HEADER 224
#define IMAGE_SIZEOF_NT_OPTIONAL64_HEADER 240
#define IMAGE_NT_OPTIONAL_HDR32_MAGIC 0x10b
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x20b
#if defined(_WIN64)
#define IMAGE_SIZEOF_NT_OPTIONAL_HEADER IMAGE_SIZEOF_NT_OPTIONAL64_HEADER
#define IMAGE_NT_OPTIONAL_HDR_MAGIC IMAGE_NT_OPTIONAL_HDR64_MAGIC
#else
#define IMAGE_SIZEOF_NT_OPTIONAL_HEADER IMAGE_SIZEOF_NT_OPTIONAL32_HEADER
#define IMAGE_NT_OPTIONAL_HDR_MAGIC IMAGE_NT_OPTIONAL_HDR32_MAGIC
#endif

#ifdef __GNUC__
#ifndef NONAMELESSUNION
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define _ANONYMOUS_UNION __extension__
#define _ANONYMOUS_STRUCT __extension__
#else
#if defined(__cplusplus)
#define _ANONYMOUS_UNION __extension__
#endif /* __cplusplus */
#endif /* __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95) */
#endif /* NONAMELESSUNION */
#elif defined(__WATCOMC__) || defined(_MSC_VER)
#define _ANONYMOUS_UNION
#define _ANONYMOUS_STRUCT
#endif /* __GNUC__/__WATCOMC__ */
#ifndef _ANONYMOUS_UNION
#define _ANONYMOUS_UNION
#define _UNION_NAME(x) x
#define DUMMYUNIONNAME  u
#define DUMMYUNIONNAME2 u2
#define DUMMYUNIONNAME3 u3
#define DUMMYUNIONNAME4 u4
#define DUMMYUNIONNAME5 u5
#define DUMMYUNIONNAME6 u6
#define DUMMYUNIONNAME7 u7
#define DUMMYUNIONNAME8 u8
#else
#define _UNION_NAME(x)
#define DUMMYUNIONNAME
#define DUMMYUNIONNAME2
#define DUMMYUNIONNAME3
#define DUMMYUNIONNAME4
#define DUMMYUNIONNAME5
#define DUMMYUNIONNAME6
#define DUMMYUNIONNAME7
#define DUMMYUNIONNAME8
#endif
#ifndef _ANONYMOUS_STRUCT
#define _ANONYMOUS_STRUCT
#define _STRUCT_NAME(x) x
#define DUMMYSTRUCTNAME s
#define DUMMYSTRUCTNAME2 s2
#define DUMMYSTRUCTNAME3 s3
#else
#define _STRUCT_NAME(x)
#define DUMMYSTRUCTNAME
#define DUMMYSTRUCTNAME2
#define DUMMYSTRUCTNAME3
#endif

typedef struct _IMAGE_RESOURCE_DIRECTORY {
        DWORD Characteristics;
        DWORD TimeDateStamp;
        WORD MajorVersion;
        WORD MinorVersion;
        WORD NumberOfNamedEntries;
        WORD NumberOfIdEntries;
} IMAGE_RESOURCE_DIRECTORY,*PIMAGE_RESOURCE_DIRECTORY;
_ANONYMOUS_STRUCT typedef struct _IMAGE_RESOURCE_DIRECTORY_ENTRY {
        _ANONYMOUS_UNION union {
                _ANONYMOUS_STRUCT struct {
                        DWORD NameOffset:31;
                        DWORD NameIsString:1;
                }DUMMYSTRUCTNAME;
                DWORD Name;
                WORD Id;
        } DUMMYUNIONNAME;
        _ANONYMOUS_UNION union {
                DWORD OffsetToData;
                _ANONYMOUS_STRUCT struct {
                        DWORD OffsetToDirectory:31;
                        DWORD DataIsDirectory:1;
                } DUMMYSTRUCTNAME2;
        } DUMMYUNIONNAME2;
} IMAGE_RESOURCE_DIRECTORY_ENTRY,*PIMAGE_RESOURCE_DIRECTORY_ENTRY;
typedef struct _IMAGE_RESOURCE_DATA_ENTRY {
        DWORD OffsetToData;
        DWORD Size;
        DWORD CodePage;
        DWORD Reserved;
} IMAGE_RESOURCE_DATA_ENTRY,*PIMAGE_RESOURCE_DATA_ENTRY;

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#endif /* _WINGLUE_H */
