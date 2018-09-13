/*	File: D:\WACKER\xfer\mdmx_crc.c (Created: 18-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */
#include <windows.h>
#pragma hdrstop

#include <tdll\stdtyp.h>
#include <tdll\assert.h>
#include <tdll\file_io.h>

#include "foo.h"

#include "mdmx.h"
#include "mdmx.hh"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-==-=
 * FUNCTION:
 *	calc_crc
 *
 * DESCRIPTION:
 *	This is a C language replacement for the following assembly language code.
 *
 *			TITLE	mdmx_crc
 *
 *			cdecl=1
 *
 *		IFDEF cdecl
 *			.model medium,C
 *		ELSE
 *			.model medium,PASCAL
 *		ENDIF
 *			.CODE	XFER_TEXT
 *
 *		; unsigned cdecl calc_crc(unsigned startval, uchar *pntr, int count)
 *		calc_crc	PROC USES SI, startval:WORD, pntr:PTR BYTE, count:WORD
 *			mov	ax,startval	; get starting value
 *			mov	si,pntr 	; get pointer to data to check
 *			mov	bx,count	; get count of characters
 *
 *		cnt:	mov	dl,[si] 	; get next character
 *			inc	si		; ++pntr
 *			mov	cx,8		; cycle through 8 bits
 *		ccycle: rol	dl,1		; rotate bit into CRC
 *			rcl	ax,1
 *			jnc	noxor
 *			xor	ax,1021h	; perform XOR for 1 bits
 *		noxor:	loop	ccycle
 *			dec	bx		; keep count of characters checked
 *			jnz	cnt
 *			ret
 *
 *		calc_crc	ENDP
 *
 *			END
 *
 *
 * PARAMETERS:
 *
 * RETURNS:
 */

unsigned short calc_crc(ST_MDMX *pX,
						unsigned short startval,
						LPSTR pntr,
						int cnt)
	{
	int i;
	unsigned short crc_16;

	/*
	 * This code was taken from "crctst.c" in HA5.  If it doesn't work, check
	 * the code that is used in ZMODEM.HH in the "updcrc" macro.  It might
	 * work better.
	 *
	 * Yes, it did work better.
	 */
	assert(pX->p_crc_tbl);

	crc_16 = startval;

	for (i = 0; i < cnt; ++i)
		crc_16 = pX->p_crc_tbl[((crc_16 >> 8) & 255)] ^ (crc_16 << 8) ^ *pntr++;

	return crc_16;
	}
