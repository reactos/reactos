/* File: C:\WACKER\xfer\hpr_sd.hh (Created: 24-Jan-1994)
 * created from HAWIN source file:
 * hpr_sd.hh  --  Header file containing system dependent declarations for
 *					Hyperprotocol
 *
 *	Copyright 1989,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

/* progress display codes */
#define FILE_DONE		1
#define TRANSFER_DONE	2


/* for HyperACCESS, convert references to file routines to bfile routines */
// #define FILETYPE  BFILE
// #define FileClose nb_close
// #define FileError nb_error
// #define FileSeek  nb_seek
// #define FilePutc  nb_putc
// #define FileGetc  nb_getc
// #define RemoteQueryBitRate() cnfg.bit_rate

// #define RemoteSendChar(c)	ComSendChar(c)
// #define RemoteSendDone() 	ComSendWait()

extern int hr_setup(struct s_hc *hc);
extern int hr_wrapup(struct s_hc *hc, int attended, int status);
extern void hpr_id_get(struct s_hc *hc, BYTE *dst);
extern int hpr_id_check(struct s_hc *hc, int rev, BYTE *name);


/* These routines are used to display the ongoing status of a transfer.
 * They may be implemented as macros or functions as needed. If no
 * display of a particular item is desired, a macro can be defined to
 * disable it.
 *	i.e.  #define hrdsp_errorcnt(cnt)
 */

/* During receiving:
 *
 *	hrdsp_filecnt(cnt)		  if sender transmits number of files coming
 *	hrdsp_totalsize(bytes)	  if sender transmits total bytes being sent
 *	hrdsp_newfile(theirname, ourname, filen)  upon start of new file
 *	hrdsp_filesize(size)	  if size of current file is transmitted
 *	hrdsp_progress(filebytes) at intervals during transfer
 *	hrdsp_errorcnt(cnt) 	  whenever an error is encountered
 *	hrdsp_event(event_code)   when significant events occur
 *	hrdsp_status(status_code) when status of transfer changes
 */

extern void hrdsp_compress(struct s_hc *hc, int cnt);
extern void hrdsp_errorcnt(struct s_hc *hc, int cnt);
extern void hrdsp_filecnt(struct s_hc *hc, int cnt);
extern void hrdsp_totalsize(struct s_hc *hc, long bytes);
extern void hrdsp_progress(struct s_hc *hc, int status);
extern void hrdsp_status(struct s_hc *hc, int status);
extern void hrdsp_event(struct s_hc *hc, int event);

extern void hrdsp_newfile(struct s_hc *hc,
							int filen,
							char FAR *theirname,
							char FAR *ourname);

extern void hrdsp_filesize(struct s_hc *hc, long fsize);


// extern void hpr_idle(struct s_hc *hc);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *								 SENDING								 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

extern void hsdsp_compress(struct s_hc *hc, int tf);
extern void hsdsp_retries(struct s_hc *hc, int t);
extern void hsdsp_status(struct s_hc *hc, int s);
extern void hsdsp_event(struct s_hc *hc, int e);


extern int hs_setup(struct s_hc *hc, int nfiles, long nbytes);
extern void hs_wrapup(struct s_hc *hc, int attended, int bailout_status);
extern void hs_fxmit(struct s_hc *, BYTE);
extern BYTE hs_xmit_switch(struct s_hc *, BYTE);
extern void hs_xbswitch(struct s_hc *);
extern void hs_xbclear(struct s_hc *);

extern void  hsdsp_progress(struct s_hc *hc, int status);
extern void  hsdsp_newfile(struct s_hc *hc,
							int filen,
							TCHAR *fname,
							long flength);

