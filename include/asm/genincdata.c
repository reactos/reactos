#undef __MSVCRT__
#include <psdk/ntverp.h>

/* DDK/IFS/NDK Headers */
#include <excpt.h>
#include <ntdef.h>
#include <ntifs.h>
#include <arc/arc.h>
#include <ntndk.h>
#include <bugcodes.h>

/* KD Support */
#define NOEXTAPI
#include <windbgkd.h>
#include <wdbgexts.h>
#include <kddll.h>

typedef struct
{
    char Type;
    char Name[55];
    ULONGLONG Value;
} ASMGENDATA;

#define TYPE_END 0
#define TYPE_RAW 1
#define TYPE_CONSTANT 2
#define TYPE_HEADER 3

#define RAW(x) {TYPE_RAW, x, 0}
#define CONSTANT(name) {TYPE_CONSTANT, #name, name > (ULONG)-1 ? (ULONGLONG)name : (ULONG)name}
#define OFFSET(name, struct, member) {TYPE_CONSTANT, #name, FIELD_OFFSET(struct, member)}
#define RELOFFSET(name, struct, member, to) {TYPE_CONSTANT, #name, FIELD_OFFSET(struct, member) - FIELD_OFFSET(struct, to)}
#define SIZE(name, struct) {TYPE_CONSTANT, #name, sizeof(struct)}
#define HEADER(x) {TYPE_HEADER, x, 0}

#if defined(_MSC_VER)
#pragma section(".asmdef")
__declspec(allocate(".asmdef"))
#elif defined(__GNUC__)
__attribute__ ((section(".asmdef")))
#else
#error Your compiler is not supported.
#endif
ASMGENDATA Table[] =
{

/* PORTABLE CONSTANTS ********************************************************/
#include "ksx.template.h"

/* ARCHITECTURE SPECIFIC CONTSTANTS ******************************************/
#ifdef _M_IX86
#include "ks386.template.h"
#elif defined(_M_AMD64)
#include "ksamd64.template.h"
#endif

    /* End of list */
    {TYPE_END, "", 0}
};

LONG NTAPI DummyEntry(HANDLE hinstDLL, ULONG fdwReason, PVOID lpvReserved)
{
    return 0;
}
