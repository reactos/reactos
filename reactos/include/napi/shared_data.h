#ifndef __INCLUDE_NAPI_SHARED_DATA_H
#define __INCLUDE_NAPI_SHARED_DATA_H

#define PF_FLOATING_POINT_PRECISION_ERRATA  0
#define PF_FLOATING_POINT_EMULATED          1
#define PF_COMPARE_EXCHANGE_DOUBLE          2
#define PF_MMX_INSTRUCTIONS_AVAILABLE       3
#define PF_PPC_MOVEMEM_64BIT_OK             4
#define PF_ALPHA_BYTE_INSTRUCTIONS          5
#define PF_XMMI_INSTRUCTIONS_AVAILABLE      6
#define PF_3DNOW_INSTRUCTIONS_AVAILABLE     7
#define PF_RDTSC_INSTRUCTION_AVAILABLE      8
#define PF_PAE_ENABLED                      9
#define PF_XMMI64_INSTRUCTIONS_AVAILABLE   10

typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE
{
   StandardDesign,
   NEC98x86,
   EndAlternatives
} ALTERNATIVE_ARCHITECTURE_TYPE;

typedef struct _KSYSTEM_TIME
{
   ULONG LowPart;
   LONG High1Time;
   LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

#define PROCESSOR_FEATURE_MAX 64
typedef struct _KUSER_SHARED_DATA {
    ULONG TickCountLowDeprecated;
    ULONG TickCountMultiplier;
    volatile KSYSTEM_TIME InterruptTime;
    volatile KSYSTEM_TIME SystemTime;
    volatile KSYSTEM_TIME TimeZoneBias;
    USHORT ImageNumberLow;
    USHORT ImageNumberHigh;
    WCHAR NtSystemRoot[260];
    ULONG MaxStackTraceDepth;
    ULONG CryptoExponent;
    ULONG TimeZoneId;
    ULONG LargePageMinimum;
    ULONG Reserved2[7];
    NT_PRODUCT_TYPE NtProductType;
    BOOLEAN ProductTypeIsValid;
    ULONG NtMajorVersion;
    ULONG NtMinorVersion;
    BOOLEAN ProcessorFeatures[PROCESSOR_FEATURE_MAX];
    ULONG Reserved1;
    ULONG Reserved3;
    volatile ULONG TimeSlip;
    ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;
    LARGE_INTEGER SystemExpirationDate;
    ULONG SuiteMask;
    BOOLEAN KdDebuggerEnabled;
    volatile ULONG ActiveConsoleId;
    volatile ULONG DismountCount;
    ULONG ComPlusPackage;
    ULONG LastSystemRITEventTickCount;
    ULONG NumberOfPhysicalPages;
    BOOLEAN SafeBootMode;
    ULONG TraceLogging;
    ULONGLONG Fill0;
    ULONGLONG SystemCall[4];
    union {
        volatile KSYSTEM_TIME TickCount;
        volatile ULONG64 TickCountQuad;
    };
} KUSER_SHARED_DATA, *PKUSER_SHARED_DATA;

/* Values for DosDeviceDriveType */
#define DOSDEVICE_DRIVE_UNKNOWN		0
#define DOSDEVICE_DRIVE_CALCULATE	1
#define DOSDEVICE_DRIVE_REMOVABLE	2
#define DOSDEVICE_DRIVE_FIXED		3
#define DOSDEVICE_DRIVE_REMOTE		4
#define DOSDEVICE_DRIVE_CDROM		5
#define DOSDEVICE_DRIVE_RAMDISK		6

/* Values for TimeZoneId */
#ifndef __USE_W32API
#define TIME_ZONE_ID_INVALID		((ULONG) -1)
#define TIME_ZONE_ID_UNKNOWN		(0)
#define TIME_ZONE_ID_STANDARD		(1)
#define TIME_ZONE_ID_DAYLIGHT		(2)
#endif

#define KERNEL_SHARED_DATA	(0xFFDF0000)
#define USER_SHARED_DATA	(0x7FFE0000)

#if defined(__NTOSKRNL__) || defined(__NTDRIVER__) || defined(__NTHAL__)
#ifndef __USE_W32API
#define KI_USER_SHARED_DATA (0xFFDF0000)
#endif
#ifdef SharedUserData
#undef SharedUserData
#endif
#define SharedUserData		((KUSER_SHARED_DATA * const)KI_USER_SHARED_DATA)
#else
#undef SharedUserData
#define SharedUserData		((KUSER_SHARED_DATA * const)USER_SHARED_DATA)
#endif


#endif /* __INCLUDE_NAPI_SHARED_DATA_H */
