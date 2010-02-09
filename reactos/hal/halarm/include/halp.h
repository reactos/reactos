#ifndef __INTERNAL_HAL_HAL_H
#define __INTERNAL_HAL_HAL_H

//
// ARM Headers
//
#include <internal/arm/ke.h>
#include <internal/arm/intrin_i.h>

//
// Versatile Peripherals
//
#include <peripherals/pl011.h>
#include <peripherals/pl190.h>
#include <peripherals/sp804.h>

#define PRIMARY_VECTOR_BASE     0x00

/* Usage flags */
#define IDT_REGISTERED          0x01
#define IDT_LATCHED             0x02
#define IDT_INTERNAL            0x11
#define IDT_DEVICE              0x21

typedef struct _IDTUsageFlags
{
    UCHAR Flags;
} IDTUsageFlags;

typedef struct
{
    KIRQL Irql;
    UCHAR BusReleativeVector;
} IDTUsage;

VOID
NTAPI
HalpRegisterVector(IN UCHAR Flags,
                   IN ULONG BusVector,
                   IN ULONG SystemVector,
                   IN KIRQL Irql);

VOID
NTAPI
HalpEnableInterruptHandler(IN UCHAR Flags,
                           IN ULONG BusVector,
                           IN ULONG SystemVector,
                           IN KIRQL Irql,
                           IN PVOID Handler,
                           IN KINTERRUPT_MODE Mode);

VOID HalpInitPhase0 (PLOADER_PARAMETER_BLOCK LoaderBlock);
VOID HalpInitPhase1(VOID);

VOID HalpInitializeInterrupts(VOID);
VOID HalpInitializeClock(VOID);
VOID HalpClockInterrupt(VOID);
VOID HalpProfileInterrupt(VOID);

extern ULONG HalpCurrentTimeIncrement, HalpNextTimeIncrement, HalpNextIntervalCount;

#endif /* __INTERNAL_HAL_HAL_H */
