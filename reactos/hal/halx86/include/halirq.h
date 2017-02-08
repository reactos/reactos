
#pragma once

#ifdef _MINIHAL_
#define VECTOR2IRQ(vector)	((vector) - PRIMARY_VECTOR_BASE)
#define VECTOR2IRQL(vector)	(PROFILE_LEVEL - VECTOR2IRQ(vector))
#define IRQ2VECTOR(irq)		((irq) + PRIMARY_VECTOR_BASE)
#define HalpVectorToIrq(vector)	((vector) - PRIMARY_VECTOR_BASE)
#define HalpVectorToIrql(vector)	(PROFILE_LEVEL - VECTOR2IRQ(vector))
#define HalpIrqToVector(irq)		((irq) + PRIMARY_VECTOR_BASE)
#else

UCHAR
FASTCALL
HalpIrqToVector(UCHAR Irq);

KIRQL
FASTCALL
HalpVectorToIrql(UCHAR Vector);

UCHAR
FASTCALL
HalpVectorToIrq(UCHAR Vector);

#define VECTOR2IRQ(vector)	HalpVectorToIrq(vector)
#define VECTOR2IRQL(vector)	HalpVectorToIrql(vector)
#define IRQ2VECTOR(irq)		HalpIrqToVector(irq)

#endif

