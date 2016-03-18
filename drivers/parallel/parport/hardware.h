/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Parallel Port Function Driver
 * FILE:            drivers/parallel/parport/hardware.h
 * PURPOSE:         Hardware definitions
 */

#ifndef _HARDWARE_H_
#define _HARDWARE_H_

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
#define LP_PBUSY    0x80  /* inverted input, active high */
#define LP_PACK     0x40  /* unchanged input, active low */
#define LP_POUTPA   0x20  /* unchanged input, active high */
#define LP_PSELECD  0x10  /* unchanged input, active high */
#define LP_PERRORP  0x08  /* unchanged input, active low */

/*
 * defines for 8255 control port
 * base + 2
 * accessed with LP_C(minor)
 */
#define LP_PINTEN   0x10
#define LP_PSELECP  0x08  /* inverted output, active low */
#define LP_PINITP   0x04  /* unchanged output, active low */
#define LP_PAUTOLF  0x02  /* inverted output, active low */
#define LP_PSTROBE  0x01  /* inverted output, active low */

#endif /* _HARDWARE_H_ */
