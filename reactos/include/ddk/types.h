#ifndef __DDK_TYPES_H
#define __DDK_TYPES_H

// these should be moved to a file like ntdef.h

typedef const int CINT;


typedef ULONG KAFFINITY, *PKAFFINITY;

typedef LONG NTSTATUS, *PNTSTATUS;

typedef ULONG DEVICE_TYPE;





enum
{
   DIRECTORY_QUERY,
   DIRECTORY_TRAVERSE,
   DIRECTORY_CREATE_OBJECT,
   DIRECTORY_CREATE_SUBDIRECTORY,
   DIRECTORY_ALL_ACCESS,
};

typedef unsigned long long ULONGLONG;

/*
 * General type for status information
 */
//typedef LONG NTSTATUS;

typedef struct _UNICODE_STRING
{
   USHORT Length;
   USHORT MaximumLength;
   PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef enum _SECTION_INHERIT {
    ViewShare = 1,
    ViewUnmap = 2
} SECTION_INHERIT;

/*
 * Various other types (all quite pointless)
 */
typedef ULONG KPROCESSOR_MODE;
typedef UCHAR KIRQL;
typedef KIRQL* PKIRQL;
typedef ULONG IO_ALLOCATION_ACTION;
typedef ULONG POOL_TYPE;
typedef ULONG TIMER_TYPE;
typedef ULONG MM_SYSTEM_SIZE;
typedef ULONG LOCK_OPERATION;
typedef ULONG KEY_INFORMATION_CLASS;
typedef ULONG FILE_INFORMATION_CLASS;
typedef ULONG KEY_VALUE_INFORMATION_CLASS;
typedef LARGE_INTEGER PHYSICAL_ADDRESS;
typedef PHYSICAL_ADDRESS* PPHYSICAL_ADDRESS;
typedef ULONG WAIT_TYPE;
//typedef ULONG KINTERRUPT_MODE;
typedef USHORT CSHORT;


typedef struct _TIME {
	DWORD LowPart;
	LONG HighPart;
} TIME;

#endif
