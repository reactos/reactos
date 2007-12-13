/* Defines for routines to implement a low-overhead timer for drivers */

 /*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

#ifndef	TIMER_H
#define TIMER_H

/* Ports for the 8254 timer chip */
#define	TIMER2_PORT	0x42
#define	TIMER_MODE_PORT	0x43

/* Meaning of the mode bits */
#define	TIMER0_SEL	0x00
#define	TIMER1_SEL	0x40
#define	TIMER2_SEL	0x80
#define	READBACK_SEL	0xC0

#define	LATCH_COUNT	0x00
#define	LOBYTE_ACCESS	0x10
#define	HIBYTE_ACCESS	0x20
#define	WORD_ACCESS	0x30

#define	MODE0		0x00
#define	MODE1		0x02
#define	MODE2		0x04
#define	MODE3		0x06
#define	MODE4		0x08
#define	MODE5		0x0A

#define	BINARY_COUNT	0x00
#define	BCD_COUNT	0x01

/* Timers tick over at this rate */
#define CLOCK_TICK_RATE	1193180U
#define	TICKS_PER_MS	(CLOCK_TICK_RATE/1000)

/* Parallel Peripheral Controller Port B */
#define	PPC_PORTB	0x61

/* Meaning of the port bits */
#define	PPCB_T2OUT	0x20	/* Bit 5 */
#define	PPCB_SPKR	0x02	/* Bit 1 */
#define	PPCB_T2GATE	0x01	/* Bit 0 */

/* Ticks must be between 0 and 65535 (0 == 65536)
   because it is a 16 bit counter */
extern void load_timer2(unsigned int ticks);
extern inline int timer2_running(void);
extern void waiton_timer2(unsigned int ticks);

extern void setup_timers(void);
extern void ndelay(unsigned int nsecs);
extern void udelay(unsigned int usecs);
extern void mdelay(unsigned int msecs);


#endif	/* TIMER_H */
