#ifndef _LINUX_LP_H
#define _LINUX_LP_H

/*
 * usr/include/linux/lp.h c.1991-1992 James Wiegand
 * many modifications copyright (C) 1992 Michael K. Johnson
 * Interrupt support added 1993 Nigel Gamble
 */

/*
 * Per POSIX guidelines, this module reserves the LP and lp prefixes
 * These are the lp_table[minor].flags flags...
 */
#define LP_EXIST 0x0001
#define LP_SELEC 0x0002
#define LP_BUSY	 0x0004
#define LP_OFFL	 0x0008
#define LP_NOPA  0x0010
#define LP_ERR   0x0020
#define LP_ABORT 0x0040
#define LP_CAREFUL 0x0080
#define LP_ABORTOPEN 0x0100

/* timeout for each character.  This is relative to bus cycles -- it
 * is the count in a busy loop.  THIS IS THE VALUE TO CHANGE if you
 * have extremely slow printing, or if the machine seems to slow down
 * a lot when you print.  If you have slow printing, increase this
 * number and recompile, and if your system gets bogged down, decrease
 * this number.  This can be changed with the tunelp(8) command as well.
 */

#define LP_INIT_CHAR 1000

/* The parallel port specs apparently say that there needs to be
 * a .5usec wait before and after the strobe.  Since there are wildly
 * different computers running linux, I can't come up with a perfect
 * value, but since it worked well on most printers before without,
 * I'll initialize it to 0.
 */

#define LP_INIT_WAIT 0

/* This is the amount of time that the driver waits for the printer to
 * catch up when the printer's buffer appears to be filled.  If you
 * want to tune this and have a fast printer (i.e. HPIIIP), decrease
 * this number, and if you have a slow printer, increase this number.
 * This is in hundredths of a second, the default 2 being .05 second.
 * Or use the tunelp(8) command, which is especially nice if you want
 * change back and forth between character and graphics printing, which
 * are wildly different...
 */

#define LP_INIT_TIME 2

/* IOCTL numbers */
#define LPCHAR   0x0601  /* corresponds to LP_INIT_CHAR */
#define LPTIME   0x0602  /* corresponds to LP_INIT_TIME */
#define LPABORT  0x0604  /* call with TRUE arg to abort on error,
			    FALSE to retry.  Default is retry.  */
#define LPSETIRQ 0x0605  /* call with new IRQ number,
			    or 0 for polling (no IRQ) */
#define LPGETIRQ 0x0606  /* get the current IRQ number */
#define LPWAIT   0x0608  /* corresponds to LP_INIT_WAIT */
#define LPCAREFUL   0x0609  /* call with TRUE arg to require out-of-paper, off-
			    line, and error indicators good on all writes,
			    FALSE to ignore them.  Default is ignore. */
#define LPABORTOPEN 0x060a  /* call with TRUE arg to abort open() on error,
			    FALSE to ignore error.  Default is ignore.  */
#define LPGETSTATUS 0x060b  /* return LP_S(minor) */
#define LPRESET     0x060c  /* reset printer */

/* timeout for printk'ing a timeout, in jiffies (100ths of a second).
   This is also used for re-checking error conditions if LP_ABORT is
   not set.  This is the default behavior. */

#define LP_TIMEOUT_INTERRUPT	(60 * HZ)
#define LP_TIMEOUT_POLLED	(10 * HZ)

#if 0
#define LP_B(minor)	lp_table[(minor)].base		/* IO address */
#define LP_F(minor)	lp_table[(minor)].flags		/* flags for busy, etc. */
#define LP_S(minor)	inb_p(LP_B((minor)) + 1)	/* status port */
#define LP_C(minor)	(lp_table[(minor)].base + 2)	/* control port */
#define LP_CHAR(minor)	lp_table[(minor)].chars		/* busy timeout */
#define LP_TIME(minor)	lp_table[(minor)].time		/* wait time */
#define LP_WAIT(minor)	lp_table[(minor)].wait		/* strobe wait */
#define LP_IRQ(minor)	lp_table[(minor)].irq		/* interrupt # */
							/* 0 means polled */
#endif

#define LP_BUFFER_SIZE 256


/*
 * The following constants describe the various signals of the printer port
 * hardware.  Note that the hardware inverts some signals and that some
 * signals are active low.  An example is LP_STROBE, which must be programmed
 * with 1 for being active and 0 for being inactive, because the strobe signal
 * gets inverted, but it is also active low.
 */

/* 
 * bit defines for 8255 status port
 * base + 1
 * accessed with LP_S(minor), which gets the byte...
 */
#define LP_PBUSY	0x80  /* inverted input, active high */
#define LP_PACK		0x40  /* unchanged input, active low */
#define LP_POUTPA	0x20  /* unchanged input, active high */
#define LP_PSELECD	0x10  /* unchanged input, active high */
#define LP_PERRORP	0x08  /* unchanged input, active low */

/* 
 * defines for 8255 control port
 * base + 2 
 * accessed with LP_C(minor)
 */
#define LP_PINTEN	0x10
#define LP_PSELECP	0x08  /* inverted output, active low */
#define LP_PINITP	0x04  /* unchanged output, active low */
#define LP_PAUTOLF	0x02  /* inverted output, active low */
#define LP_PSTROBE	0x01  /* inverted output, active low */

/* 
 * the value written to ports to test existence. PC-style ports will 
 * return the value written. AT-style ports will return 0. so why not
 * make them the same ? 
 */
#define LP_DUMMY	0x00

/*
 * This is the port delay time.  Your mileage may vary.
 * It is used only in the lp_init() routine.
 */
#define LP_DELAY 	150000

#endif
