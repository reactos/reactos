#ifndef __INCLUDE_NTOS_SHARED_DATA_H
#define __INCLUDE_NTOS_SHARED_DATA_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif


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
   volatile LARGE_INTEGER InterruptTime;
   volatile LARGE_INTEGER SystemTime;
   volatile LARGE_INTEGER TimeZoneBias;
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

/* Values for DosDeviceDriveType */
#define DOSDEVICE_DRIVE_UNKNOWN		0
#define DOSDEVICE_DRIVE_CALCULATE	1
#define DOSDEVICE_DRIVE_REMOVABLE	2
#define DOSDEVICE_DRIVE_FIXED		3
#define DOSDEVICE_DRIVE_REMOTE		4
#define DOSDEVICE_DRIVE_CDROM		5
#define DOSDEVICE_DRIVE_RAMDISK		6


#define KERNEL_SHARED_DATA	(0xFFDF0000)
#define USER_SHARED_DATA	(0x7FFE0000)

#if defined(_NTOSKRNL_) || defined(__NTDRIVER__) || defined(__NTHAL__)
#else
#define SharedUserData		((KUSER_SHARED_DATA * const)USER_SHARED_DATA)
#endif


#endif /* __INCLUDE_NTOS_SHARED_DATA_H */
