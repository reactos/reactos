/* File: C:\WACKER\xfer\mdmx.h (Created: 17-Jan-1994)
 * created from HAWIN source file
 * mdmx.h -- Exported definitions for xmodem file transfer routines
 *
 *	Copyright 1989,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

#if !defined(EOF)
#define EOF (-1)
#endif

/* error checking method values */
#define UNDETERMINED	0
#define CHECKSUM		1
#define CRC 			2

/* XMODEM and YMODEM user settable control values. These are exported to
 *	  the configuration routines.
 */

extern int mdmx_rcv(HSESSION hSession,
					   int attended,
					   int method,
					   int single_file);

extern int mdmx_snd(HSESSION hSession,
					   int attended,
					   int method,
					   unsigned nfiles,
					   long nbytes);

/* end of mdmx.h */
