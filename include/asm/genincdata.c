#undef __MSVCRT__
#include <psdk/ntverp.h>

/* DDK/IFS/NDK Headers */
#define _NTSYSTEM_
#include <excpt.h>
#include <setjmp.h>
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

#ifdef _M_AMD64
enum
{
    P1Home = 1 * sizeof(PVOID),
    P2Home = 2 * sizeof(PVOID),
    P3Home = 3 * sizeof(PVOID),
    P4Home = 4 * sizeof(PVOID),
};
#endif

// FIXME: where to put this?
typedef struct _FIBER                                      /* Field offsets:  */
{                                                          /* 32 bit   64 bit */
    /* this must be the first field */
    PVOID Parameter;                                       /*   0x00     0x00 */
    PEXCEPTION_REGISTRATION_RECORD ExceptionList;          /*   0x04     0x08 */
    PVOID StackBase;                                       /*   0x08     0x10 */
    PVOID StackLimit;                                      /*   0x0C     0x18 */
    PVOID DeallocationStack;                               /*   0x10     0x20 */
    CONTEXT Context;                                       /*   0x14     0x28 */
    ULONG GuaranteedStackBytes;                            /*   0x2E0         */
    PVOID FlsData;                                         /*   0x2E4         */
    PVOID /* PACTIVATION_CONTEXT_STACK */ ActivationContextStack; /*   0x2E8         */
} FIBER, *PFIBER;

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
#define CONSTANT(name) {TYPE_CONSTANT, #name, (ULONG)name}
#define CONSTANT64(name) {TYPE_CONSTANT, #name, (ULONGLONG)name}
#define CONSTANTPTR(name) {TYPE_CONSTANT, #name, (ULONG_PTR)name}
#define CONSTANTX(name, value) {TYPE_CONSTANT, #name, value}
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
#if defined (_M_IX86) || defined (_M_AMD64)
/* PORTABLE CONSTANTS ********************************************************/
#include "ksx.template.h"
#endif

/* ARCHITECTURE SPECIFIC CONTSTANTS ******************************************/
#ifdef _M_IX86
#include "ks386.template.h"
#elif defined(_M_AMD64)
#include "ksamd64.template.h"
#elif defined(_M_ARM)
#include "ksarm.template.h"
#endif

    /* End of list */
    {TYPE_END, "", 0}
};

