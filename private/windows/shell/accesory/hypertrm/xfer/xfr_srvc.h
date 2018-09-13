/* xfr_srvc.h -- include file for transfer service routines
 *
 *	Copyright 1990 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

extern void xfer_set_pointer(HSESSION hSession, void *pV);

extern void *xfer_get_pointer(HSESSION hSession);

/* These are flags that can be passed to xfer_idle to indicate why it */
/* has been called and what should be done, if anything. */
#define	XFER_IDLE_IO				0x00000001 
#define	XFER_IDLE_DISPLAY			0x00000002
extern void xfer_idle(HSESSION h, int nMode);

#if !defined(XFER_ABORT)
#define	XFER_ABORT		1
#endif
#if !defined(XFER_SKIP)
#define	XFER_SKIP		2
#endif
extern int	xfer_user_interrupt(HSESSION hSession);

extern int  xfer_user_abort(HSESSION hSession, int p);

extern int  xfer_carrier_lost(HSESSION hSession);

extern void xfer_purgefile(HSESSION hSession, TCHAR *fname);

extern int xfer_open_rcv_file(HSESSION hSession,
							 struct st_rcv_open *pstRcv,
							 unsigned long ulOverRide);

extern void xfer_build_rcv_name(HSESSION hSession,
							  struct st_rcv_open *pstRcv);

extern int xfer_close_rcv_file(HSESSION Hsession,
							  void *vhdl,
							  int nReason,
							  TCHAR *pszRemoteName,
							  TCHAR *pszOurName,
							  int nSave,
							  unsigned long lFilesize,
							  unsigned long lTime);

extern void *xfer_get_params(HSESSION hSession, int nProtocol);

extern int xfer_set_comport(HSESSION hSession, int fSending, unsigned FAR *puiOldOptions);

extern int xfer_restore_comport(HSESSION hSession, unsigned uiOldOptions);

extern int	xfer_save_partial(HSESSION hSession);

extern int	xfer_nextfile(HSESSION hSession, TCHAR *filename);

extern void xfer_log_xfer(HSESSION hSession,
						  int sending,
						  TCHAR *theirname,
						  TCHAR *ourname,
						  int result);

extern int xfer_opensendfile(HSESSION hSession,
							 HANDLE *fp,
							 TCHAR *file_to_open,
							 long *size,
							 TCHAR *name_to_send,
							 void *ft);
							 // struct s_filetime FAR *ft);

extern void xfer_name_to_send(HSESSION hSession,
							  TCHAR *local_name,
							  TCHAR *name_to_send);

