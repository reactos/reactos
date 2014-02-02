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
#include <arc/arc.h>
#include <ntdddisk.h>
#include <internal/hal.h>
