/* File: C:\WACKER\xfer\hpr.c (Created: 25-Jan-1994)
 * created from HAWIN source file
 * hpr.c  --  Functions common to HyperSend and HyperSave routines.
 *
 *	Copyright 1989,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

#include <windows.h>
#include <setjmp.h>
#include <time.h>
#include <term\res.h>
#include <sys\types.h>
#include <sys\utime.h>

#include <tdll\stdtyp.h>
#include <tdll\mc.h>
#include <tdll\com.h>
#include <tdll\session.h>
#include <tdll\load_res.h>
#include <tdll\xfer_msc.h>
#include <tdll\globals.h>
#include <tdll\file_io.h>


#if !defined(BYTE)
#define	BYTE	unsigned char
#endif

#include "cmprs.h"

#include "xfr_dsp.h"
#include "xfr_todo.h"
#include "xfr_srvc.h"

#include "xfer.h"
#include "xfer.hh"
#include "xfer_tsc.h"

#include "hpr.h"
#include "hpr.hh"
#include "hpr_sd.hh"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *	Routines to handle building and sending of outgoing messages   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * omsg_init
 *
 * DESCRIPTION:
 *	Called before using any other omsg_ functions to provide the routines
 *	with resources.
 *
 * ARGUMENTS:
 *	bufr		-- A pointer to a memory buffer that the omsg routines can
 *					use to build messages. It must be large enough for the
 *					largest message to be sent plus a size byte and two
 *					check bytes.
 *	size		-- The size of bufr in bytes.
 *	fPrintable	 -- TRUE if the message should be sent in printable form. If
 *					this is TRUE, the only non-printable character sent as
 *					a part of outgoing messages will be the initial SOH
 *					character. The size and check bytes are converted to
 *					printable characters.
 *	sndfunc 	-- A pointer to a function that omsg_send can use to transmit
 *					a message after it has been formatted. The function should
 *					accepet a single character argument and return VOID.
 *
 * RETURNS:
 *	nothing
 */
void omsg_init(struct s_hc *hc, int fPrintable, int fEmbedMsg)
	{
	hc->omsg_printable = fPrintable;
	hc->omsg_embed = fEmbedMsg;
	omsg_setnum(hc, -1);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * omsg_new
 *
 * DESCRIPTION:
 *	Begins formatting a new message. Messages are built up in pieces and
 *	can be sent more than once. This function discards any old message
 *	and sets up a new one containing no fields.
 *
 * ARGUMENTS:
 *	type -- A single character type character to be used in the message
 *			type field of the the new message.
 *
 * RETURNS:
 *	nothing
 */
void omsg_new(struct s_hc *hc, BYTE type)
	{
	hc->omsg_bufr[0] = type;
	hc->omsg_bufr[1] = ' ';
	hc->omsg_bufr[2] = ' ';
	hc->omsg_bufr[3] = '\0';
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * omsg_add
 *
 * DESCRIPTION:
 *	Adds a field to a message being built. A prior call to omsg_new will have
 *	set up an empty message. One or more fields can then be added to the
 *	message using this funtion. A semi-colon will automatically be appended
 *	to the field.
 *
 * ARGUMENTS:
 *	newfield - A text string containing the field to be added.
 *
 * RETURNS:
 *	TRUE if field is added, FALSE if there is insufficient room in the
 *	message buffer.
 */
int omsg_add(struct s_hc *hc, BYTE *newfield)
	{
	if (strlen(hc->omsg_bufr) + strlen(newfield) > sizeof(hc->omsg_bufr) - 3)
		return(FALSE);
	strcat(hc->omsg_bufr, newfield);
	strcat(hc->omsg_bufr, ";");
	return(TRUE);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * omsg_setnum
 *
 * DESCRIPTION:
 *	Messages are numbered consecutively as they are sent. This function
 *	forces a the messages to start at a new number. Since message numbers
 *	are incremented just before a message is sent, this funtion changes the
 *	effective number of the last message sent. The next message out will have
 *	a number one greater than the number specified in this function.
 *
 * ARGUMENTS:
 *	n -- The new starting number for outgoing messages.
 *
 * RETURNS:
 *	Returns the new message number as a convenience.
 */
int omsg_setnum(struct s_hc *hc, int n)
	{
	return(hc->omsgn = n);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * omsg_send
 *
 * DESCRIPTION:
 *	Completes a message and transmits it. The size and check fields of the
 *	current message are computed and the message is transmitted according to
 *	instructions. The message number is automatically incremented just before
 *	transmission.
 *
 * ARGUMENTS:
 *	burstcnt  -- Number of identical copies of the message to send
 *	usecrc	  -- If TRUE, the CRC calculation is used to calculate the
 *					check bytes. If FALSE, a simple sum is used.
 *	backspace -- If TRUE, each character out is followed by a backspace to
 *					keep the messages from showing up on the remote computer
 *					screen if they haven't started their transfer yet.
 *
 * RETURNS:
 *	The number assigned to the message as it was transmitted.
 */
int omsg_send(struct s_hc *hc, int burstcnt, int usecrc, int backspace)
	{
	HCOM hCom;
	register unsigned checksum;
	unsigned hold_crc = hc->h_crc;	/* hold onto data crc & restore at end */
	int t;
	size_t sl;
	register size_t i;

	hCom = sessQueryComHdl(hc->hSession);

	hc->omsgn = (hc->omsgn + 1) % (hc->omsg_printable ? 94 : 256);
	sl = strlen(hc->omsg_bufr);

	/* len includes check bytes */
	hc->omsg_bufr[1] = (hc->omsg_printable ? tochar(sl) : (BYTE)sl);

	hc->omsg_bufr[2] = (hc->omsg_printable ?
			tochar(hc->omsgn) : (BYTE)hc->omsgn);
	hc->h_crc = checksum = 0;
	for (i = 0; i < sl; ++i)
		{
		checksum += hc->omsg_bufr[i];
		if (usecrc)
			h_crc_calc(hc, hc->omsg_bufr[i]);
		}
	if (hc->omsg_printable)
		{
		hc->omsg_bufr[sl] = (BYTE)tochar(checksum & 0x3F);
		hc->omsg_bufr[sl + 1] = (BYTE)tochar((checksum >> 6) & 0x3F);
		}
	else
		{
		hc->omsg_bufr[sl] = (BYTE)((usecrc ? hc->h_crc : checksum) % 256);
		hc->omsg_bufr[sl + 1] = (BYTE)((usecrc ? hc->h_crc : checksum) / 256);
		}

	for (t = 0; t < burstcnt; ++t)
		{
		if (hc->omsg_embed)
			{
			hs_xmit_(hc, H_MSGCHAR);
			if (backspace)
				hs_xmit_(hc, '\b');
			for(i = 0; i < sl + 2; ++i)
				{
				if (hc->omsg_bufr[i] == H_MSGCHAR)
					{
					hs_xmit_(hc, hc->omsg_bufr[i]);
					if (backspace)
						hs_xmit_(hc, '\b');
					}
				hs_xmit_(hc, hc->omsg_bufr[i]);
				if (backspace)
					hs_xmit_(hc, '\b');
				}
			}
		else
			{
			ComSendChar(hCom, H_MSGCHAR);

			if (backspace)
				ComSendChar(hCom, '\b');

			for(i = 0; i < sl + 2; ++i)
				{
				if (hc->omsg_bufr[i] == H_MSGCHAR)
					{
					ComSendChar(hCom, hc->omsg_bufr[i]);

					if (backspace)
						ComSendChar(hCom, '\b');
					}
				ComSendChar(hCom, hc->omsg_bufr[i]);

				if (backspace)
					ComSendChar(hCom, '\b');
				}
			}
		}


	if (hc->omsg_embed)
		{
		if (backspace)
			{
			hs_xmit_(hc, ' ');
			hs_xmit_(hc, '\b');
			}
		}
	else
		{
		if (backspace)
			{
			ComSendChar(hCom, ' ');
			ComSendChar(hCom, '\b');
			}
		ComSendWait(hCom);
		}

	hc->last_omsg = startinterval();
	hc->omsg_bufr[sl] = '\0';
	hc->h_crc = hold_crc;
	return(hc->omsgn);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * omsg_last
 *
 * DESCRIPTION:
 *	Returns the time that the last message was sent in a form suitable to
 *	use with interval(). Passing the returned value to interval() will give
 *	the time in tenths of a second since the last message was sent.
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	nothing
 */
long omsg_last(struct s_hc *hc)
	{
	return(hc->last_omsg);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * omsg_number
 *
 * DESCRIPTION:
 *	Returns the message number of the last message sent. The value will be
 *	-1 if no messages have been sent.
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	The number of the last message.
 */
int omsg_number(struct s_hc *hc)
	{
	return(hc->omsgn);
	}


#if FALSE	/* this is a 'C' version of the code in hpr_calc.asm */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * h_crc_calc
 *
 * DESCRIPTION:
 *	Does byte-by-byte CRC calcuation for HyperProtocol
 *
 * ARGUMENTS:
 *	cc -- Next character to include in CRC calculation. The global value
 *			h_crc is modified to include the effects of cc
 *
 * RETURNS:
 *	nothing
 */
void NEAR h_crc_calc(uchar cc)
	{
	register unsigned q;

	q = (h_crc ^ cc) & 0x0F;
	h_crc = (h_crc >> 4) ^ (q * 0x1081);
	q = (h_crc ^ (cc >> 4)) & 0x0F;
	h_crc = (h_crc >> 4) ^ (q * 0x1081);
	}

#endif

/********************* end of hpr.c ***********************/
