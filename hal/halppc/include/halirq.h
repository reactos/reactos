/*
 * $Id: halirq.h 23669 2006-08-23 16:58:43Z ion $
 */

#ifndef __INCLUDE_HAL_HALIRQ
#define __INCLUDE_HAL_HALIRQ

#ifdef CONFIG_SMP

#define FIRST_DEVICE_VECTOR	(0x30)
#define FIRST_SYSTEM_VECTOR	(0xef)

#define IRQ_BASE		FIRST_DEVICE_VECTOR
#define	NR_IRQS			(FIRST_SYSTEM_VECTOR - FIRST_DEVICE_VECTOR)

/*
 * FIXME:
 *   This does not work if we have more than 24 IRQs (ie. more than one I/O APIC)
 */
#define VECTOR2IRQ(vector)	(23 - (vector - IRQ_BASE) / 8)
#define VECTOR2IRQL(vector)	(PROFILE_LEVEL - VECTOR2IRQ(vector))
#define IRQ2VECTOR(irq)		(((23 - (irq)) * 8) + FIRST_DEVICE_VECTOR)

#else

#define IRQ_BASE		(0x30)
#define	NR_IRQS			(16)

#define VECTOR2IRQ(vector)	((vector) - IRQ_BASE)
#define VECTOR2IRQL(vector)	(PROFILE_LEVEL - VECTOR2IRQ(vector))
#define IRQ2VECTOR(irq)		((irq) + IRQ_BASE)

#endif

#endif /* __INCLUDE_HAL_HALIRQ */
