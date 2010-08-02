#include <ntdef.h>
#undef _NTHAL_
//#undef DECLSPEC_IMPORT
//#define DECLSPEC_IMPORT
#undef NTSYSAPI
#define NTSYSAPI

/* Windows Device Driver Kit */
#include <ntddk.h>
#include <ndk/haltypes.h>

//typedef GUID UUID;

/* Disk stuff */
typedef PVOID PLOADER_PARAMETER_BLOCK;
#include <ntdddisk.h>
#include <internal/hal.h>
