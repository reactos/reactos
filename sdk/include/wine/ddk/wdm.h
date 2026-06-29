
#pragma once

typedef struct _KSYSTEM_TIME {
    ULONG LowPart;
    LONG High1Time;
    LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

#define PROCESSOR_FEATURE_MAX 64

typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE
{
   StandardDesign,
   NEC98x86,
   EndAlternatives
} ALTERNATIVE_ARCHITECTURE_TYPE;

typedef enum _NT_PRODUCT_TYPE {
    NtProductWinNt = 1,
    NtProductLanManNt,
    NtProductServer
} NT_PRODUCT_TYPE, *PNT_PRODUCT_TYPE;

typedef struct _KUSER_SHARED_DATA {
    ULONG TickCountLowDeprecated;                          /* 0x000 */
    ULONG TickCountMultiplier;                             /* 0x004 */
    volatile KSYSTEM_TIME InterruptTime;                   /* 0x008 */
    volatile KSYSTEM_TIME SystemTime;                      /* 0x014 */
    volatile KSYSTEM_TIME TimeZoneBias;                    /* 0x020 */
    USHORT ImageNumberLow;                                 /* 0x02c */
    USHORT ImageNumberHigh;                                /* 0x02e */
    WCHAR NtSystemRoot[260];                               /* 0x030 */
    ULONG MaxStackTraceDepth;                              /* 0x238 */
    ULONG CryptoExponent;                                  /* 0x23c */
    ULONG TimeZoneId;                                      /* 0x240 */
    ULONG LargePageMinimum;                                /* 0x244 */
    ULONG AitSamplingValue;                                /* 0x248 */
    ULONG AppCompatFlag;                                   /* 0x24c */
    ULONGLONG RNGSeedVersion;                              /* 0x250 */
    ULONG GlobalValidationRunLevel;                        /* 0x258 */
    volatile ULONG TimeZoneBiasStamp;                      /* 0x25c */
    ULONG NtBuildNumber;                                   /* 0x260 */
    NT_PRODUCT_TYPE NtProductType;                         /* 0x264 */
    BOOLEAN ProductTypeIsValid;                            /* 0x268 */
    USHORT NativeProcessorArchitecture;                    /* 0x26a */
    ULONG NtMajorVersion;                                  /* 0x26c */
    ULONG NtMinorVersion;                                  /* 0x270 */
    BOOLEAN ProcessorFeatures[PROCESSOR_FEATURE_MAX];      /* 0x274 */
    ULONG Reserved1;                                       /* 0x2b4 */
    ULONG Reserved3;                                       /* 0x2b8 */
    volatile ULONG TimeSlip;                               /* 0x2bc */
    ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture; /* 0x2c0 */
    ULONG BootId;                                          /* 0x2c4 */
    LARGE_INTEGER SystemExpirationDate;                    /* 0x2c8 */
    ULONG SuiteMask;                                       /* 0x2d0 */
    BOOLEAN KdDebuggerEnabled;                             /* 0x2d4 */
    UCHAR NXSupportPolicy;                                 /* 0x2d5 */
    USHORT CyclesPerYield;                                 /* 0x2d6 */
    volatile ULONG ActiveConsoleId;                        /* 0x2d8 */
    volatile ULONG DismountCount;                          /* 0x2dc */
    ULONG ComPlusPackage;                                  /* 0x2e0 */
    ULONG LastSystemRITEventTickCount;                     /* 0x2e4 */
    ULONG NumberOfPhysicalPages;                           /* 0x2e8 */
    BOOLEAN SafeBootMode;                                  /* 0x2ec */
    UCHAR VirtualizationFlags;                             /* 0x2ed */
    union {
        ULONG SharedDataFlags;                             /* 0x2f0 */
        struct {
            ULONG DbgErrorPortPresent       : 1;
            ULONG DbgElevationEnabed        : 1;
            ULONG DbgVirtEnabled            : 1;
            ULONG DbgInstallerDetectEnabled : 1;
            ULONG DbgLkgEnabled             : 1;
            ULONG DbgDynProcessorEnabled    : 1;
            ULONG DbgConsoleBrokerEnabled   : 1;
            ULONG DbgSecureBootEnabled      : 1;
            ULONG DbgMultiSessionSku        : 1;
            ULONG DbgMultiUsersInSessionSku : 1;
            ULONG DbgStateSeparationEnabled : 1;
            ULONG SpareBits                 : 21;
        } DUMMYSTRUCTNAME2;
    } DUMMYUNIONNAME2;
    ULONG DataFlagsPad[1];                                 /* 0x2f4 */
    ULONGLONG TestRetInstruction;                          /* 0x2f8 */
    LONGLONG QpcFrequency;                                 /* 0x300 */
    ULONG SystemCall;                                      /* 0x308 */
    union {
        ULONG AllFlags;                                    /* 0x30c */
        struct {
            ULONG Win32Process            : 1;
            ULONG Sgx2Enclave             : 1;
            ULONG VbsBasicEnclave         : 1;
            ULONG SpareBits               : 29;
        } DUMMYSTRUCTNAME;
    } UserCetAvailableEnvironments;
    ULONGLONG SystemCallPad[2];                            /* 0x310 */
    union {
        volatile KSYSTEM_TIME TickCount;                   /* 0x320 */
        volatile ULONG64 TickCountQuad;
    } DUMMYUNIONNAME;
    ULONG Cookie;                                          /* 0x330 */
    ULONG CookiePad[1];                                    /* 0x334 */
    LONGLONG ConsoleSessionForegroundProcessId;            /* 0x338 */
    ULONGLONG TimeUpdateLock;                              /* 0x340 */
    ULONGLONG BaselineSystemTimeQpc;                       /* 0x348 */
    ULONGLONG BaselineInterruptTimeQpc;                    /* 0x350 */
    ULONGLONG QpcSystemTimeIncrement;                      /* 0x358 */
    ULONGLONG QpcInterruptTimeIncrement;                   /* 0x360 */
    UCHAR QpcSystemTimeIncrementShift;                     /* 0x368 */
    UCHAR QpcInterruptTimeIncrementShift;                  /* 0x369 */
    USHORT UnparkedProcessorCount;                         /* 0x36a */
    ULONG EnclaveFeatureMask[4];                           /* 0x36c */
    ULONG TelemetryCoverageRound;                          /* 0x37c */
    USHORT UserModeGlobalLogger[16];                       /* 0x380 */
    ULONG ImageFileExecutionOptions;                       /* 0x3a0 */
    ULONG LangGenerationCount;                             /* 0x3a4 */
    ULONG ActiveProcessorAffinity;                         /* 0x3a8 */
    volatile ULONGLONG InterruptTimeBias;                  /* 0x3b0 */
    volatile ULONGLONG QpcBias;                            /* 0x3b8 */
    ULONG ActiveProcessorCount;                            /* 0x3c0 */
    volatile UCHAR ActiveGroupCount;                       /* 0x3c4 */
    union {
        USHORT QpcData;                                    /* 0x3c6 */
        struct {
            UCHAR volatile QpcBypassEnabled;
            UCHAR QpcShift;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME3;
    LARGE_INTEGER TimeZoneBiasEffectiveStart;              /* 0x3c8 */
    LARGE_INTEGER TimeZoneBiasEffectiveEnd;                /* 0x3d0 */
    XSTATE_CONFIGURATION XState;                           /* 0x3d8 */
} KSHARED_USER_DATA, *PKSHARED_USER_DATA;

#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_ENABLED 0x01
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_USE_HV_PAGE 0x02
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_DISABLE_32BIT 0x04
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_USE_MFENCE 0x10
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_USE_LFENCE 0x20
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_A73_ERRATA 0x40
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_USE_RDTSCP 0x80
