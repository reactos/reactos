#ifndef __INCLUDE_NAPI_SHARED_DATA_H
#define __INCLUDE_NAPI_SHARED_DATA_H

#define PROCESSOR_FEATURES_MAX 64

typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE
{
   StandardDesign,
   NEC98x86,
   EndAlternatives
} ALTERNATIVE_ARCHITECTURE_TYPE;


typedef struct _KUSER_SHARED_DATA
{
   volatile ULONG TickCountLow;
   ULONG TickCountMultiplier;
   volatile KSYSTEM_TIME InterruptTime;
   volatile KSYSTEM_TIME SystemTime;
   volatile KSYSTEM_TIME TimeZoneBias;
   USHORT ImageNumberLow;
   USHORT ImageNumberHigh;
   WCHAR NtSystemRoot[260];
   ULONG DosDeviceMap;
   ULONG CryptoExponent;
   ULONG TimeZoneId;
   UCHAR DosDeviceDriveType[32];
   NT_PRODUCT_TYPE NtProductType;
   BOOLEAN ProductTypeIsValid;
   ULONG NtMajorVersion;
   ULONG NtMinorVersion;
   BOOLEAN ProcessorFeatures[PROCESSOR_FEATURES_MAX];

   // NT5 / Win2k specific ??
   ULONG Reserved1;
   ULONG Reserved3;
   volatile ULONG TimeSlip;
   ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;
   ULONG SuiteMask;
#ifdef REMOTE_BOOT
   ULONG SystemFlags;
   UCHAR RemoteBootServerPath[260];
#endif
   BOOLEAN KdDebuggerEnabled;
} KUSER_SHARED_DATA, *PKUSER_SHARED_DATA;

#define KERNEL_SHARED_DATA	(0xFFDF0000)
#define USER_SHARED_DATA	(0x7FFE0000)

#if defined(__NTOSKRNL__) || defined(__NTDRIVER__) || defined(__NTHAL__)
#define KI_USER_SHARED_DATA	(0xFFDF0000)
#define SharedUserData		((KUSER_SHARED_DATA * const)KI_USER_SHARED_DATA)
#else
#define SharedUserData		((KUSER_SHARED_DATA * const)USER_SHARED_DATA)
#endif


#endif /* __INCLUDE_NAPI_SHARED_DATA_H */
