/* mdmx_snd.c -- Routines to handle xmodem sending for HA5G
 *
 *	Copyright 1989 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */
#include <windows.h>
#include <stdlib.h>

#pragma hdrstop
// #include <setjmp.h>

#define	BYTE	unsigned char

#include <tdll\mc.h>
#include <tdll\stdtyp.h>
#include <tdll\com.h>
#include <tdll\assert.h>
#include <tdll\session.h>
#include <tdll\load_res.h>
#include <tdll\xfer_msc.h>
#include <tdll\file_io.h>
#include <tdll\tchar.h>
#include "xfr_srvc.h"
#include "xfr_todo.h"
#include "xfr_dsp.h"
#include "xfer_tsc.h"
#include "foo.h"

#include "cmprs.h"

#include "xfer.h"
#include "xfer.hh"

#include "mdmx.h"
#include "mdmx.hh"

#if !defined(STATIC_FUNC)
#define	STATIC_FUNC
#endif

#if !defined(CMPRS_MINSIZE)
#define	CMPRS_MINSIZE	4000L
#endif

/* * * * * * * * * * * * * * * *
 *	local function prototypes  *
 * * * * * * * * * * * * * * * */

STATIC_FUNC	int xsend_start(ST_MDMX *xc, BYTE *start_chars, int *start_char);

STATIC_FUNC	int getresponse(ST_MDMX *xc, int time);

STATIC_FUNC	void make_file_pckt(ST_MDMX *xc,
								struct s_mdmx_pckt *p,
								char *fname,
								long size);

/* * * * * * * *
 *	Functions  *
 * * * * * * * */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * mdmx_snd
 *
 * DESCRIPTION:
 *	Sends a file using XMODEM or YMODEM protocol. Support 1k packets, batch
 *	transfers and 'G' option streaming.
 *
 * ARGUMENTS:
 *	attended -- TRUE if user is probably in attendance. Controls the display
 *				of some messages.
 *
 * RETURNS:
 *	True if transfer completes successfully, FALSE otherwise.
 */
int mdmx_snd(HSESSION hSession, int attended, int method, unsigned nfiles, long nbytes)
	{
	ST_MDMX *xc;
	struct s_mdmx_pckt * this_pckt = NULL;
	struct s_mdmx_pckt * next_pckt = NULL;
	struct s_mdmx_pckt * tpckt;

	/* column values for display box */
	char	 sfname[FNAME_LEN];// file name of file being sent
	char	 xname[FNAME_LEN]; // transmitted file name
	int	 still_trying;		// controls exit from main transfer loop
	int	 got_file;			// controls when to complete batch op
	int	 got_response;		// controls loop to get valid response
	int 	 tries = 0; 		// number of retries for each packet
	unsigned total_tries;		// number of retries for entire transfer
	int 	 response;			// response char. received from other end
	BYTE	 start_chars[3];	// acceptable start chars. from receiver
	int 	 xstatus;			// winds up with overall status of transfer
	int 	 check_type;		// type of error checking in use
	unsigned pcktn; 			// number of packet currently being sent
	int	 override = FALSE;	// set TRUE if comm. details changed to
	unsigned int uiOldOptions;
	int	 batch; 			// TRUE if YMODEM batch transfers used
	int	 big_pckts; 		// TRUE if 1K packets are allowed
	int	 streaming; 		// TRUE if no packet responses expected

	if (xfer_set_comport(hSession, TRUE, &uiOldOptions) != TRUE)
		goto done;
	else
		override = TRUE;

	/* set up options based on method used */
	big_pckts = (method != XF_XMODEM);
	batch = (method == XF_YMODEM || method == XF_YMODEM_G);
	streaming = FALSE;	/* will be turned on if receiver starts with 'G' */
	// assert(nfiles == 1 || batch);

	this_pckt = NULL;
	next_pckt = NULL;

	xc = malloc(sizeof(struct s_mdmx_cntrl));
	if (xc == NULL)
		goto done;
	memset(xc, 0, sizeof(struct s_mdmx_cntrl));

	xc->hSession = hSession;
	xc->hCom     = sessQueryComHdl(hSession);

	// RemoteClear(hSession);
	ComRcvBufrClear(xc->hCom);

	this_pckt = malloc(sizeof(struct s_mdmx_pckt) +
					   (big_pckts ? LARGE_PACKET : SMALL_PACKET) + 2);
	if (this_pckt == NULL)
		goto done;
	memset(this_pckt, 0, sizeof(struct s_mdmx_pckt) +
						   (big_pckts ? LARGE_PACKET : SMALL_PACKET) + 2);

	next_pckt = malloc(sizeof(struct s_mdmx_pckt) +
					   (big_pckts ? LARGE_PACKET : SMALL_PACKET) + 2);
	if (next_pckt == NULL)
		goto done;
	memset(next_pckt, 0, sizeof(struct s_mdmx_pckt) +
						   (big_pckts ? LARGE_PACKET : SMALL_PACKET) + 2);

	mdmxXferInit(xc, method);  /* Could be smaller but this is easier */
	if (xc->p_crc_tbl == NULL)
		goto done;

	// hp_report_xtime(0);	   /* make invalid in case transfer bombs */
	xc->file_bytes = 0L;
	xc->total_bytes = 0L;
	xc->fh = NULL;
	xc->xfertimer = -1L;
	xc->nfiles = nfiles;	/* make these available to display routines */
	xc->filen = 0;
	xc->filesize = -1L;
	xc->nbytes = nbytes;

	mdmxdspTotalsize(xc, nbytes);
	mdmxdspFilecnt(xc, nfiles);

	xc->mdmx_byte_cnt = 0;
	StrCharCopy(start_chars, (batch ? "CG" : "C\x15"));  /* \x15 is NAK */
	check_type = CRC;
	mdmxdspChecktype(xc, (check_type == CRC) ? 0 : 1);
	total_tries = 0;
	mdmxdspErrorcnt(xc, total_tries);
	tries = 0;
	mdmxdspPacketErrorcnt(xc, tries);
	got_file = TRUE;
	while (got_file)
		{
		if ((got_file = xfer_nextfile(hSession, sfname)) == TRUE)
			{
			xc->total_bytes += xc->file_bytes;
			xc->file_bytes = xc->mdmx_byte_cnt = 0L;

			mdmxdspNewfile(xc,
						   xc->filen + 1,
						   sfname,
						   sfname);

			++xc->filen;

			if (xfer_opensendfile(hSession,
								  &xc->fh,
								  sfname,
								  &xc->filesize,
								  xname,
								  NULL) != 0)
				{
				xstatus = TSC_CANT_OPEN;
				goto done;
				}
			mdmxdspFilesize(xc, xc->filesize);
			}
		else
			{
			// strblank(xname);
			xname[0] = '\0';
			}


		pcktn = 0;
		if (batch)
			make_file_pckt(xc, this_pckt, xname, xc->filesize);

		if ((xstatus = xsend_start(xc, start_chars, &response)) != TSC_OK)
			break;

		if (xc->filen <= 1)
			{
			xc->xfertimer = startinterval();	  /* start the clock  */
			if (response == NAK)
				check_type = CHECKSUM;
			mdmxdspChecktype(xc, (check_type == CRC) ? 0 : 1);
			if (response == 'G')
				{
				streaming = TRUE;
				mdmxdspChecktype(xc, 2);
				}

			/* once we've received the first start_char,
			 * subsequent ones must match
			 */
			start_chars[0] = (BYTE)response;
			start_chars[1] = '\0';
			}


		if (got_file)
			{
			xc->p_getc = xm_getc;
			tries = 0;
			if (!batch &&
					!load_pckt(xc, this_pckt, pcktn = 1, big_pckts, check_type))
				{
				xstatus = TSC_DISK_ERROR;
				goto done;
				}
			}


		/* get the first pckt on its way while we prepare the second pckt */
		ComSndBufrSend(xc->hCom,
					&this_pckt->start_char,
					(unsigned)this_pckt->pcktsize,
					50);

		mdmxdspPacketnumber(xc, pcktn);

		/* load next pckt*/
		if (got_file && !load_pckt(xc, next_pckt, ++pcktn, big_pckts, check_type))
			{
			xstatus = TSC_DISK_ERROR;
			goto done;
			}

		still_trying = TRUE;
		while (still_trying)
			{
			if (streaming)
				{
				/* these things are done in getresponse() if not streaming */

				if (xfer_carrier_lost(hSession))
					{
					xstatus = TSC_LOST_CARRIER;
					break;
					}

				if (xfer_user_interrupt(hSession))
					{
					xstatus = TSC_USER_CANNED;
					break;
					}
				}

			/* wait until last packet is out before watching for response */
			ComSndBufrWait(xc->hCom,
							this_pckt->pcktsize >= LARGE_PACKET ? LARGE_WAIT :
																  SMALL_WAIT);

			/* get response from receiver */
			got_response = FALSE;
			while (!got_response)
				{
				response = (streaming && this_pckt->start_char != EOT) ?
						ACK : getresponse(xc, 60);

				got_response = TRUE;

				switch(response)
					{
				case ACK:

					if (this_pckt->start_char == EOT)
						{
						/* successful */
						mdmx_progress(xc, FILE_DONE);
						xc->xfertime = interval(xc->xfertimer);
						fio_close(xc->fh);

						xfer_log_xfer(hSession,
									  TRUE,
									  sfname,
									  NULL,
									  TSC_OK);

						xc->fh = NULL;
						xstatus = TSC_OK;
						still_trying = FALSE;
						}
					else
						{
						xc->file_bytes = this_pckt->byte_count;
						tpckt = this_pckt;
						this_pckt = next_pckt;
						next_pckt = tpckt;

						/* pcktn will only be <= 1 when batch is on and
						 *	we've just sent the filename packet (packet 0)
						 */
						if (pcktn <= 1 && (!got_file ||
								(xstatus = xsend_start(xc, start_chars, &response))
								!= TSC_OK))
							{
							still_trying = FALSE;
							break;
							}

						/* send packet */

						ComSndBufrSend(xc->hCom,
									&this_pckt->start_char,
									(unsigned)this_pckt->pcktsize,
									50);

						mdmxdspPacketnumber(xc, pcktn);

						mdmx_progress(xc, 0);
						if (tries != 0)
							{
							mdmxdspPacketErrorcnt(xc, 0);

							tries = 0;
							}
						if (this_pckt->start_char != EOT)
							{
							if (!load_pckt(xc, next_pckt, ++pcktn, big_pckts, check_type))
								{
								xstatus = TSC_DISK_ERROR;
								still_trying = FALSE;
								}
							}
						}
					break;

				case NO_RESPONSE:
					mdmxdspLastError(xc, 12);

					still_trying = FALSE;
					break;

				case ABORTED:
					xstatus = TSC_USER_CANNED;
					still_trying = FALSE;
					break;

				case CARR_LOST:
					xstatus = TSC_LOST_CARRIER;
					still_trying = FALSE;
					break;

				case 'C':
				case 'G':
					/* these act as NAKs for packets first packets */
					if (pcktn > 2)
						{
						got_response = FALSE;
						break;
						}
					/* else fall through */
				case NAK:
					if (++tries >= xc->mdmx_tries)
						{
						xstatus = TSC_ERROR_LIMIT;
						goto done;
						}
					else	/* send packet */
						{
						ComSndBufrSend(xc->hCom,
									&this_pckt->start_char,
									(unsigned)this_pckt->pcktsize,
									50);

						}
					if (this_pckt->start_char == EOT && tries == 1)
						break;	/* don't print first retransmission on final EOT */
					mdmxdspPacketErrorcnt(xc, tries);

					mdmxdspErrorcnt(xc, ++total_tries);

					mdmxdspLastError(xc,
									 (response == NAK) ? 13 : 14);

					break;

				case CAN:
					if (getresponse(xc, 1) == CAN)  /* two consecutive CANs? */
						{
						xstatus = TSC_RMT_CANNED;
						still_trying = FALSE;
						break;
						}
					/* fall through */

				default:
					got_response = FALSE;
					}

				} /* end while (!got_response) */

			}	/* end while(still_trying) */

		if (!batch || xstatus != TSC_OK)
			break;

		}	/* end while(got_file) */

	done:

	mdmx_progress(xc, TRANSFER_DONE);

	mdmxdspCloseDisplay(xc);

	// ComSendSetCharDelay(hld_send_cdelay, COMSEND_SETDELAY);
	if (override)
		{
#if FALSE
		cnfg.bits_per_char = hld_bits_per_char;
		cnfg.parity_type = hld_parity_type;
		(void)(*ComResetPort)();
#endif
		xfer_restore_comport(hSession, uiOldOptions);
		}
	// hp_report_xtime((unsigned)xc->xfertime);
	if (xc->fh)
		fio_close(xc->fh);

	if (xstatus != TSC_OK)
		{
		if (xstatus != TSC_RMT_CANNED)
			{
			for (tries = 5 + 1; --tries > 0; )
				ComSendChar(xc->hCom, &xc->stP, CAN);
			ComSendPush(xc->hCom, &xc->stP);
			}
		xfer_log_xfer(hSession,
					  TRUE,
					  sfname,
					  NULL,
					  xstatus);
		}

	if (attended && xstatus != TSC_USER_CANNED)
		{
#if FALSE
		menu_bottom_line (BL_ESC, 0L);
		DosBeep(beepfreq, beeplen);
		menu_replybox((int)xc->msgrow, ENTER_RESP, 0, (int)transfer_status_msg((unsigned short)xstatus));
#endif
		}

	if (xc->p_crc_tbl != NULL)
		resFreeDataBlock(xc->hSession, xc->p_crc_tbl);
	if (xc)
		{
		free(xc);
		xc = NULL;
		}
	if (this_pckt)
		{
		free(this_pckt);
		this_pckt = NULL;
		}
	if (next_pckt)
		{
		free(next_pckt);
		next_pckt = NULL;
		}

	return((unsigned)xstatus);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * xsend_start
 *
 * DESCRIPTION:
 *	Waits up to one minute for a start request from the receiver at the
 *	other end of the line.
 *
 * ARGUMENTS:
 *	chktype -- Pointer to a variable to be set to the requested error
 *				correction checking method, CRC or Checksum
 *
 * RETURNS:
 *	Status code indicating result. Can be one of
 *		TSC_USER_CANNED if user interrupted by hitting the ESC key.
 *		TSC_NO_RESPONSE if 60 seconds elapses without receiving a start char.
 *		TSC_RMT_CANNED	if remote sends a control-C
 *		TSC_OK			if remote sends proper start char.
 *
 */
STATIC_FUNC int xsend_start(ST_MDMX *xc, BYTE *start_chars, int *start_char)
	{
	for ( ; ; )
		{
		switch(*start_char = getresponse(xc, 60))
			{
		case ABORTED:
			return(TSC_USER_CANNED);

		case NO_RESPONSE:
			return(TSC_NO_RESPONSE);

		case CARR_LOST:
			return(TSC_LOST_CARRIER);

		case ESC:
		case '\003':						/* control-C */
			return(TSC_RMT_CANNED);

		default:
			if (strchr(start_chars, *start_char))
				{
				return(TSC_OK);
				}

			/* ignore any other char. */
			break;
			}
		}
	return TSC_OK;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * getresponse
 *
 * DESCRIPTION:
 *	Waits a specified time for a response from the receiver. Can be forced
 *	to terminate early if user intervention is detected. No effort is made
 *	here to interpret the meaning of a response character. Any character
 *	received within the time limit will be returned.
 *
 * ARGUMENTS:
 *	time -- Number of seconds to wait for a response character.
 *
 * RETURNS:
 *	The response character if one was received or ABORTED or NO_RESPONSE or
 *	CARR_LOST.
 */
STATIC_FUNC int getresponse(ST_MDMX *xc, int time)
	{
	TCHAR rc = 0;
	long timer;

	DbgOutStr("getresponse ", 0,0,0,0,0);

#if FALSE
	if (kbd_check_flagkey(xc->flagkey, TRUE) > 0)
		{
		kbd_flush();
		return ABORTED;
		}
#endif
	if (xfer_user_interrupt(xc->hSession))
		{
		DbgOutStr("aborted\r\n", 0,0,0,0,0);
		return ABORTED;
		}

	// if ((rc = RemoteGet(xc->hSession)) != -1)
	if (mComRcvChar(xc->hCom, &rc) != 0)
		{
		DbgOutStr("returned %d\r\n", rc, 0,0,0,0);
		return(rc & 0x7F);
		}

	time *= 10;
	timer = startinterval();
	while ((long)interval(timer) < (long)time)
		{
#if FALSE
		if (kbd_check_flagkey(xc->flagkey, TRUE) > 0)
			{
			kbd_flush();
			return ABORTED;
			}
#endif

		if (xfer_carrier_lost(xc->hSession))
			{
			DbgOutStr(" lost\r\n", 0,0,0,0,0);
			return CARR_LOST;
			}

		if (xfer_user_interrupt(xc->hSession))
			{
			DbgOutStr("aborted\r\n", 0,0,0,0,0);
			return ABORTED;
			}

		mdmx_progress(xc, 0);

		if (mComRcvChar(xc->hCom, &rc) != 0)
			{
			DbgOutStr("returned %d\r\n", rc, 0,0,0,0);
			return(rc & 0x7F);
			}

		xfer_idle(xc->hSession, XFER_IDLE_IO);

		}
	DbgOutStr(" none\r\n", 0,0,0,0,0);
	return(NO_RESPONSE);
	}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * make_file_pckt
 *
 * DESCRIPTION: sets up the initial filename pckt for Ymodem (only)
 *
 * ARGUMENTS:
 *	p		-- Pointer to the packet structure which receives the filename
 *				packet.
 *	fname	-- The file name as it should be placed in the packet.
 *	size	-- The size of the file as it should be placed in the packet.
 *
 * RETURNS:
 *	nothing
 */
STATIC_FUNC void make_file_pckt(ST_MDMX *xc,
								struct s_mdmx_pckt *p,
								char *fname,
								long size)
	{
	BYTE sizestr[20];
	BYTE *ptr;
	BYTE *cp;
	unsigned int crc;

	p->start_char = SOH;			   /* set start char to SOH   */
	p->pcktnum = 0; 				   /* set pcktnumber to 0	  */
	p->npcktnum = 0xff; 			   /* set npcktnumber to 0xff */
	ptr = p->bdata;

	/* initialize data area with zeros */
	memset(ptr, 0, SMALL_PACKET + 2);

	if (*fname)
		{
		StrCharCopy(ptr, fname);			 /* copy filename into buffer*/

		/* replace all back slashes with slashes */
		//while (strreplace(ptr, FstrBslashBslash(), "/"))
			// ;
		for (cp = ptr; *cp != '\0'; cp += 1)
			if (*cp == '\\') *cp = '/';

		// StrFmt(sizestr, "%ld", size);		  /* format the file size */
		wsprintf(sizestr, "%ld", (LONG)size);

		StrCharCopy(&ptr[StrCharGetByteCount(ptr)+1], sizestr);
		/* add it to buffer   */
		}

	/* calculate CRC value */
	ptr = &p->bdata[SMALL_PACKET];	/* set ptr to char after buffer */
									/* calculate CRC			*/
	crc = calc_crc(xc, (unsigned)0, p->bdata, SMALL_PACKET+2);
									/* set the CRC				*/
	*ptr++ = (BYTE)(crc / 0x100);
	*ptr = (BYTE)(crc % 0x100);
									/* set the packetsize		*/
	p->pcktsize = SMALL_PACKET + 5;
	p->byte_count = xc->mdmx_byte_cnt;
	}


/*********************** end of mdmx_snd.c **************************/
