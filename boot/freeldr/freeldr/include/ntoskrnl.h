#include <ntdef.h>
#undef _NTHAL_
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#undef NTSYSAPI
#define NTSYSAPI

#include <wdm.h>

typedef GUID UUID;

/* Windows Device Driver Kit */
#include <winddk.h>
#include <ndk/haltypes.h>

/* Disk stuff */
typedef PVOID PLOADER_PARAMETER_BLOCK;
#include <ntdddisk.h>
#include <internal/hal.h>
