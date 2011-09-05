/*
 * $Id$
 */

#pragma once

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

