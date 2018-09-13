/* File: C:\WACKER\xfer\mdmx_rcv.c (Created: 18-Jan-1994)
 * created from HAWIN source file
 *
 * mdmx_rcv.c -- XMODEM compatible file receiving routines for HA5G
 *
 *	Copyright 1987,88,89,90,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

#include <windows.h>
#pragma hdrstop

#include <stdlib.h>
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

/* * * * * * * * * * * * *
 *	Function Prototypes  *
 * * * * * * * * * * * * */

STATIC_FUNC void 	  start_receive(ST_MDMX *xc, unsigned expect);

STATIC_FUNC int	   wait_receive(ST_MDMX *xc);

STATIC_FUNC	int 	  receivepckt(ST_MDMX *xc,
								HSESSION hSession,
								unsigned expect,
								struct s_mdmx_pckt *pckt);

STATIC_FUNC	void	  respond(HSESSION hSession, ST_MDMX *xc, char code);

STATIC_FUNC	void	  xm_clear_input(HSESSION hSession);

STATIC_FUNC void	 xm_rcheck(ST_MDMX *xc, HSESSION hSession, int before);

STATIC_FUNC void	 xm_check_input(HSESSION hSession, int suspend);

extern	int	  xr_collect(ST_MDMX *, int, long, unsigned char **,
								  unsigned char *, unsigned *);


/*lint -e502*/				/* lint seems to want the ~ operator applied
							 *	only to unsigned, wer'e using uchar
							 */
#define ESC_MSG_COL 	1


/* * * * * * * * * * * *
 *	Shared Variables   *
 * * * * * * * * * * * */

// int (NEAR *p_putc)(metachar c) = xm_putc;
// static struct s_mdmx_pckt *next_pckt;
// static unsigned this_pckt;

// static tiny check_type;
// static int batch;
// static int streaming;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * mdmx_rcv
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int mdmx_rcv(HSESSION hSession, int attended, int method, int single_file)
	{
	ST_MDMX *xc;
	struct 	s_mdmx_pckt *last_pckt = NULL;
	struct 	s_mdmx_pckt *swap_pckt = NULL;
	struct 	st_rcv_open stRcv;
	char 	fname[FNAME_LEN];
	char 	our_fname[FNAME_LEN];
	long 	basesize;
	int 	still_trying = TRUE;
	int 	xpckt_size;
	int 	xstatus = TSC_OK;
	int 	override = FALSE;
	unsigned int uiOldOptions;
	unsigned tries, retries;
	unsigned char 	*cp;
	int 	blk_result = UNDEFINED, result = 0;
	char 	start_char;
	char 	nak_char;

	*fname = '\0';

	/* allocate space for large packets since we don't necessarily know what
	 *	we'll be getting.
	 */
	xc = NULL;
	last_pckt = NULL;
	xc = malloc(sizeof(struct s_mdmx_cntrl));
	if (xc == NULL)
		goto done;
	memset(xc, 0, sizeof(struct s_mdmx_cntrl));
	DbgOutStr("xc = 0x%x\r\n", xc, 0,0,0,0);

	last_pckt = malloc(sizeof(struct s_mdmx_pckt) + LARGE_PACKET + 2);
	if (last_pckt == NULL)
		goto done;
	memset(last_pckt, 0, sizeof(struct s_mdmx_pckt));

	xc->next_pckt = malloc(sizeof(struct s_mdmx_pckt) + LARGE_PACKET + 2);
	if (xc->next_pckt == NULL)
		goto done;
	memset(xc->next_pckt, 0, sizeof(struct s_mdmx_pckt));

	xc->hSession = hSession;
	xc->hCom     = sessQueryComHdl(hSession);

	DbgOutStr("hs = 0x%x\r\n", hSession, 0,0,0,0);

	mdmxXferInit(xc, method);

	if (xfer_set_comport(hSession, FALSE, &uiOldOptions) != TRUE)
		goto done;
	else
		override = TRUE;

	xc->file_bytes = 0L;
	xc->total_bytes = 0L;
#if FALSE
	xc->flagkey = kbd_register_flagkey(ESC_KEY, NULL);
#endif
	xc->fh = NULL;
	xc->xfertimer = -1L;
	xc->xfertime = 0L;
	xc->nfiles = 0;
	xc->filen = 0;
	xc->filesize = -1L;
	xc->nbytes = -1L;

	still_trying = TRUE;
	blk_result = UNDEFINED;
	xc->batch = TRUE;
	xc->streaming = FALSE;
	start_char = 'C';
	xc->check_type = CRC;
	mdmxdspChecktype(xc, (xc->check_type == CRC) ? 0 : 1);
	if (method == XF_YMODEM_G)
		{
		xc->streaming = TRUE;
		start_char = 'G';
		}
	else if ((method == XF_XMODEM || method == XF_XMODEM_1K) &&
			xc->mdmx_chkt == CHECKSUM)
		{
		start_char = NAK;
		xc->check_type = CHECKSUM;
		}

	nak_char = start_char;
	xc->this_pckt = 0;

	tries = 0;
	mdmxdspPacketErrorcnt(xc, tries);

	retries = 0;
	mdmxdspErrorcnt(xc, retries);

	xc->mdmx_byte_cnt = 0L;

	mdmxdspChecktype(xc, (xc->check_type == CRC) ? 0 : 1);

	start_receive(xc, xc->this_pckt);	   /* setup to receive first pckt */
	if (attended)
		respond(hSession, xc, start_char);

	while (still_trying)
		{
		blk_result = wait_receive(xc);
		switch(blk_result)
			{
		case NOBATCH_PCKT:
			/* Received pckt 1 while waiting for pckt 0.
			 * This must be an XMODEM as opposed to a YMODEM transfer
			 */
			if (!single_file)
				{
				xstatus = TSC_BAD_FORMAT;
				still_trying = FALSE;
				break;
				}
			xc->this_pckt = 1;
			xc->batch = FALSE;
			xc->filesize = -1L;
			nak_char = NAK;

			stRcv.pszSuggestedName = "";
			stRcv.pszActualName = our_fname;
			stRcv.lFileTime = 0;

			result = xfer_open_rcv_file(hSession, &stRcv, 0L);
			if (result != 0)
				{
				switch (result)
				{
				case -6:
					xstatus = TSC_REFUSE;
					break;
				case -5:
					xstatus = TSC_CANT_OPEN;
					break;
				case -4:
					xstatus = TSC_NO_FILETIME;
					break;
				case -3:
					xstatus = TSC_CANT_OPEN;
					break;
				case -2:
					xstatus = TSC_OLDER_FILE;
					break;
				case -1:
				default:
					xstatus = TSC_CANT_OPEN;
					break;
				}
				still_trying = FALSE;
				break;
				}

			xc->fh = stRcv.bfHdl;
			xc->basesize = stRcv.lInitialSize;

			basesize = stRcv.lInitialSize;

			mdmxdspNewfile(xc,
						   0,
						   our_fname,
						   our_fname);

			/* fall through */
		case ALT_PCKT:
		case GOOD_PCKT:
			/* swap pckt pointers so we can work on this one while receiving
			 *	the next
			 */
			swap_pckt = last_pckt;
			last_pckt = xc->next_pckt;
			xc->next_pckt = swap_pckt;
			if (xc->this_pckt != 0)
				start_receive(xc, xc->this_pckt + 1);

			if (!xc->streaming)
				respond(hSession, xc, ACK);  /* send ACK as soon as possible */
										/* then send burst for Xmodem pckt 1 */
			if (xc->this_pckt == 0)
				{
				if (!*(last_pckt->bdata)) /* no more files? */
					{
					xc->xfertime = interval(xc->xfertimer);
					still_trying = FALSE;
					break;
					}
				else if (xc->filen > 0 && single_file) /* getting too many files? */
					{
					xstatus = TSC_TOO_MANY;
					still_trying = FALSE;
					break;
					}

				start_receive(xc, 1);
				respond(hSession, xc, start_char);

				/* get info out of packet 0 and open file */
				StrCharCopy(fname, last_pckt->bdata);
				for (cp = fname; *cp != '\0'; cp++)
					if (*cp == '/')
						*cp = '\\';

				stRcv.pszSuggestedName = fname;
				stRcv.pszActualName = our_fname;
				stRcv.lFileTime = 0;
				xfer_build_rcv_name(hSession, &stRcv);

				result = xfer_open_rcv_file(hSession, &stRcv, 0L);
				if (result != 0)
					{
					switch (result)
					{
					case -6:
						xstatus = TSC_REFUSE;
						break;
					case -5:
						xstatus = TSC_CANT_OPEN;
						break;
					case -4:
						xstatus = TSC_NO_FILETIME;
						break;
					case -3:
						xstatus = TSC_CANT_OPEN;
						break;
					case -2:
						xstatus = TSC_OLDER_FILE;
						break;
					case -1:
					default:
						xstatus = TSC_CANT_OPEN;
						break;
					}
					still_trying = FALSE;
					break;
					}

				xc->fh = stRcv.bfHdl;
				xc->basesize = stRcv.lInitialSize;

				basesize = stRcv.lInitialSize;

				mdmxdspNewfile(xc,
							   ++xc->filen,
							   fname,
							   our_fname);

				/* accumlate last transfer total and start new counter */
				xc->total_bytes += xc->file_bytes;
				xc->mdmx_byte_cnt = xc->file_bytes = 0L;
				xc->filesize = -1L;
				cp = last_pckt->bdata +
						StrCharGetByteCount(last_pckt->bdata) + 1;
				if (*cp)
					{
					xc->filesize = atol(cp);

					mdmxdspFilesize(xc, xc->filesize);
					}
				nak_char = NAK;
				}
			else
				{
				/* unload packet data */
				cp = last_pckt->bdata;
				xpckt_size = (last_pckt->start_char == STX ?
						LARGE_PACKET : SMALL_PACKET);

				if (xs_unload(xc, cp, xpckt_size) == (-1) /* ERROR */ )
					{
					xm_clear_input(hSession);
					respond(hSession, xc, CAN);
					xstatus = TSC_DISK_ERROR;
					still_trying = FALSE;
					break;
					}

				// if (xc->filesize != -1L) jmh 03-08-96 to match HAWin
				if (xc->filesize > 0)
					xc->file_bytes = min(xc->filesize, xc->mdmx_byte_cnt);
				else
					xc->file_bytes = xc->mdmx_byte_cnt;
				}

			mdmxdspPacketnumber(xc, (long)++xc->this_pckt);

			if (tries)
				mdmxdspPacketErrorcnt(xc, tries = 0);

			mdmx_progress(xc, 0);


			break;

		case END_PCKT:
			/* the special EOT handling was removed from version 3.20
			 *	due to problems with RBBS-PC. It should be modified and
			 *	reenabled after experimentation.
			 */


			respond(hSession, xc, ACK); 			  /* ACK the EOT */
			mdmx_progress(xc, FILE_DONE);

			/* It's possible to get an unwanted EOT (if the ACK from the
			 *	first EOT is lost) so we should treat it like a repeated
			 *	packet.
			 */

			if (xc->fh)    /* if file was open */
				{
				if (!xfer_close_rcv_file(hSession,
										 xc->fh,
										 xstatus,
										 fname,
										 our_fname,
										 xfer_save_partial(hSession),
										 xc->basesize + xc->file_bytes /*xc->filesize jmh 03-08-96*/,
										 0))
					{
					xstatus = TSC_DISK_ERROR;
					still_trying = FALSE;
					break;
					}

				xc->fh = NULL;
				}

			if (!xc->batch)
				{
				xc->xfertime = interval(xc->xfertimer);
				still_trying = FALSE;
				}
			else
				{
				start_receive(xc, xc->this_pckt = 0);
				respond(hSession, xc, nak_char = start_char);
				}

			if (tries)
				// VidWrtStrF(xc->toprow + XR_DR_RETRIES, xc->dc_retries,
						// "^H%-2d", tries = 0);
				mdmxdspPacketErrorcnt(xc, tries = 0);

			break;

		case REPEAT_PCKT:
			start_receive(xc, xc->this_pckt);
			respond(hSession, xc, ACK);
			++tries;
			break;

		case WRONG_PCKT:
			xm_clear_input(hSession);
			respond(hSession, xc, CAN);
			++tries;		/* to get packet error on screen */
			still_trying = FALSE;
			xstatus = TSC_OUT_OF_SEQ;
			break;

		case SHORT_PCKT:
		case BAD_FORMAT:
		case BAD_CHECK:
		case NO_PCKT:
			++tries;
			if (xc->mdmx_chkt == UNDETERMINED && xc->this_pckt == 0 && tries == 3
					&& method == XF_XMODEM)
				{
				xc->check_type = CHECKSUM;
				start_char = nak_char = NAK;
				// VidWrtStr(xc->toprow + XR_DR_ERR_CHK, xc->dc_err_chk,
						// strld(TM_CHECKSUM));

				mdmxdspChecktype(xc, 1);
				}
			xm_clear_input(hSession);
			respond(hSession, xc, nak_char);
			start_receive(xc, xc->this_pckt);
			break;

		case BLK_ABORTED:
			xm_clear_input(hSession);
			respond(hSession, xc, CAN);
			xstatus = TSC_USER_CANNED;
			still_trying = FALSE;
			break;

		case CARRIER_LOST:
			xm_clear_input(hSession);
			xstatus = TSC_LOST_CARRIER;
			still_trying = FALSE;
			break;

		case CANNED:
			xm_clear_input(hSession);
			xstatus = TSC_RMT_CANNED;
			still_trying = FALSE;
			break;

		default:
			// assert(FALSE);
			break;
			}

		if (tries)
			{
			mdmxdspPacketErrorcnt(xc, tries);

			mdmxdspErrorcnt(xc, ++retries);

			mdmxdspLastError(xc, blk_result);

			if ((tries >= (unsigned)xc->mdmx_tries) || (xc->streaming && xc->this_pckt > 0))
				{
				xm_clear_input(hSession);
				respond(hSession, xc, CAN);
				xstatus = TSC_ERROR_LIMIT;
				still_trying = FALSE;
				}
			}
		}	/* while (still_trying) */



	done:

	if (xc->xfertime == 0L)
		xc->xfertime = interval(xc->xfertimer);

	mdmx_progress(xc, TRANSFER_DONE);

	if (override)
		{
#if FALSE
		cnfg.send_xprot = hld_send_xprot;
		cnfg.save_xprot = hld_save_xprot;
		cnfg.bits_per_char = hld_bits_per_char;
		cnfg.parity_type = hld_parity_type;
		(void)(*ComResetPort)();
#endif
		xfer_restore_comport(hSession, uiOldOptions);
		}

	if (xstatus != TSC_OK)
		{
		if (xc->fh)
			xfer_close_rcv_file(hSession,
								xc->fh,
								xstatus,
								fname,
								our_fname,
								xfer_save_partial(hSession),
								xc->basesize + xc->file_bytes /*xc->filesize jmh 03-08-96*/,
								0);
		}

#if FALSE
	kbd_deregister_flagkey(xc->flagkey);
#endif
	mdmxdspCloseDisplay(xc);

	if (xc->p_crc_tbl != NULL)
		resFreeDataBlock(xc->hSession, xc->p_crc_tbl);
	if (xc->next_pckt)
		{
		free(xc->next_pckt);
		xc->next_pckt = NULL;
		}
	if (xc)
		{
		free(xc);
		xc = NULL;
		}
	if (last_pckt)
		{
		free(last_pckt);
		last_pckt = NULL;
		}

	return((int)xstatus);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * start_receive
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
STATIC_FUNC void  start_receive(ST_MDMX *xc, unsigned expect)
	{
	xc->next_pckt->result = UNDEFINED;
	xc->next_pckt->expected = expect;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * wait_receive
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
STATIC_FUNC int wait_receive(ST_MDMX *xc)
	{
	if (xc->next_pckt->result == UNDEFINED)
		{
		xc->next_pckt->result = receivepckt(xc,
											xc->hSession,
											xc->next_pckt->expected,
											xc->next_pckt);
		}
	return xc->next_pckt->result;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * receivepckt
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
STATIC_FUNC int  receivepckt(ST_MDMX *xc,
							HSESSION hSession,
							unsigned expect,
							struct s_mdmx_pckt *pckt)
	{
	long timer, timeout;
	int gothdr = FALSE;
	int started = FALSE;
	TCHAR cc;
	unsigned char *cp;
	int gotCAN = FALSE;
	unsigned char checksum;
	unsigned crc;
	int count;

	// DbgOutStr("pckt = 0x%x\r\n", pckt, 0,0,0,0);

	timer = startinterval();

	/* wait for valid pckt-start character */
	timeout = (long)(xc->mdmx_pckttime * 10);

	while (!started)
		{
#if FALSE
		if (kbd_check_flagkey(xc->flagkey, TRUE))
			{
			kbd_flush();
			return(BLK_ABORTED);
			}
#endif

		if (xfer_carrier_lost(hSession))
			return CARRIER_LOST;

		if (xfer_user_interrupt(hSession))
			{
			mdmxdspLastError(xc, BLK_ABORTED);
			return(BLK_ABORTED);
			}

		mdmx_progress(xc, 0);

		if ((long)interval(timer) > timeout)
			return(NO_PCKT);

		// if ((cc = RemoteGet(hSession)) != -1)
		if (mComRcvChar(xc->hCom, &cc) != 0)
			{
			DbgOutStr("pckt = 0x%x\r\n", pckt, 0,0,0,0);
			switch(pckt->start_char = (unsigned char)cc)
				{
			case EOT:
				if (xc->xfertimer == -1L)
					xc->xfertimer = startinterval();
				return(END_PCKT);
				/*lint -unreachable*/
				break;

			case SOH:
			case STX:
				started = TRUE;
				if (xc->xfertimer == -1L)
					xc->xfertimer = startinterval();
				break;

			case CAN:
				/* if two consecutive CANs are received, drop out */
				if (gotCAN)
					return(CANNED);
				gotCAN = TRUE;
				break;

			default:
				/* ignore */
				gotCAN = FALSE; /* two CANs must be consecutive */
				break;
				}
			}
		else
			{
			xfer_idle(hSession, XFER_IDLE_IO);
			}
		}
	/* got valid start character, get packet numbers, data, & error codes */
	timeout = xc->mdmx_chartime * 10;
	cp = &pckt->pcktnum;
	count = 2;
	for (;;)
		{
		if (!xr_collect(xc, count, timeout, &cp, &checksum, &crc))
			return(SHORT_PCKT);
		if (!gothdr)
			{
			/* got pckt numbers, now get data and check code(s) */
			gothdr = TRUE;
			count = (pckt->start_char == STX ? LARGE_PACKET : SMALL_PACKET);
			count += (xc->check_type == CRC ? 2 : 1);
			checksum = 0;
			crc = 0;
			cp = pckt->bdata;
			}
		else
			break;
		}

	/* all bytes have been collected, check for valid packet */
	if (xc->check_type == CHECKSUM)
		{
		/* at this point we've included the checksum itself in the checksum
		 *	calculation. We need to back up, subtract the last char. from
		 *	the computation and use it for comparison instead.
		 */
		--cp;						/* point to received checksum */
		checksum = (unsigned char)(checksum - *cp);	/* we added one too many */
		if (checksum != *cp)
			return(BAD_CHECK);
		}
	else if (crc != 0)
			return(BAD_CHECK);

	if (pckt->pcktnum != (unsigned char)((~pckt->npcktnum) & 0xFF))
		{
		return(BAD_FORMAT);
		}

	if (pckt->pcktnum != (unsigned char)(expect % 256))
		{
		/* we always start out expecting ymodem batch, on an xmodem
		 *	 transfer, this code will detect the situation.
		 */
		if (!xc->filen && expect == 0 && pckt->pcktnum == 1)
			return NOBATCH_PCKT;
		else if (pckt->pcktnum == (unsigned char)((expect % 256) - 1))
			return REPEAT_PCKT; 	/* repeated packets are harmless */
		else
			return WRONG_PCKT;
		}

	/* if we got this far, the pckt is good */
	return(GOOD_PCKT);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * respond
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
STATIC_FUNC void  respond(HSESSION hSession, ST_MDMX *xc, char code)
	/* wait for line to clear, then send code */
	{
	int i;

	ComSendChar(xc->hCom, &xc->stP, code);
	if (code == CAN)
		{
		for (i = 4 + 1; --i > 0; )
			ComSendChar(xc->hCom, &xc->stP, CAN);
		}
	ComSendWait(xc->hCom, &xc->stP);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * xm_clear_input
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
STATIC_FUNC void  xm_clear_input(HSESSION hSession)
	{
	// RemoteClear(hSession); /* make sure no junk is left sitting in it */
	ComRcvBufrClear(sessQueryComHdl(hSession));
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * xm_rcheck
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
STATIC_FUNC void xm_rcheck(ST_MDMX *xc, HSESSION hSession, int before)
	{
	if (xc->streaming)
		{
		/* Do it different for YMODEM-G, since the sender won't wait for ACK */
#if FALSE
		if (before)
			suspendinput(FLG_DISK_ACTIVE, 5);
		else
			allowinput(FLG_DISK_ACTIVE);
#endif
		}
	else
		{
		if (before)
			{
			/* wait till next packet is in before writing to disk */
			if (xc->next_pckt->result == UNDEFINED)
				xc->next_pckt->result = wait_receive(xc);
			}
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * xm_check_input
 *
 * DESCRIPTION:
 *
 * ARGUEMENTS:
 *
 * RETURNS:
 *
 */
STATIC_FUNC void xm_check_input(HSESSION hSession, int suspend)
	{
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * xr_collect
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
int  xr_collect(ST_MDMX *xc, int count, long timeout,
			 unsigned char **ptr,
			 unsigned char *checksum, unsigned *crc)
	{
	unsigned char lchecksum;
	unsigned char *cp, *head;
	TCHAR rchar;
	int cnt;
	long timer;

	head = cp = *ptr;
	lchecksum = *checksum;
	cnt = count;

	while (cnt--)
		{
		// if ((rchar = RemoteGet(xc->hSession)) == -1)
		if (mComRcvChar(xc->hCom, &rchar) == 0)
			{
			xfer_idle(xc->hSession, XFER_IDLE_IO);
			/* driver hasn't put any new chars in rmt_bufr */
			timer = startinterval();
			// while ((rchar = RemoteGet(xc->hSession)) == -1)
			while (mComRcvChar(xc->hCom, &rchar) == 0)
				{
				/* check for char timeout */
				xfer_idle(xc->hSession, XFER_IDLE_IO);
				if ((long)interval(timer) > timeout)
					return(FALSE);
				}
			}
		*cp = (unsigned char)rchar;
		lchecksum += *cp;
		++cp;
		}
	*ptr = cp;
	*checksum = lchecksum;
	if (count > 100)
		*crc = calc_crc(xc, (unsigned)0, head, count);
	return(TRUE);
	}

/***************************** end of mdmx_rcv.c **************************/
