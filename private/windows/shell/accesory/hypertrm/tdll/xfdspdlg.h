/*	C:\WACKER\TDLL\XFDSPDLG.H (Created: 10-Jan-1994)
 *	xfer_dsp.h -- outer include file for various transfer related operations
 *
 *	Copyright 1994 by Hilgraeve, Inc. - Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:21p $
 *
 */

#if !defined(XFER_DSP_H_DEFINED)
#define XFER_DSP_H_DEFINED

/*
 * This is a list of control IDs used in the display
 */

#define	XFR_DISPLAY_CLASS		"XfrDisplayClass"

extern INT_PTR CALLBACK XfrDisplayDlg(HWND, UINT, WPARAM, LPARAM);

// extern HWND xfrReceiveCreateDisplay(HSESSION);
// extern HWND xfrSendCreateDisplay(HSESSION);

// extern VOID xfrNewDisplayWindow(HWND, INT, LPSTR, DLGPROC);

/* internal message used in shutdown processing */
#define XFR_SHUTDOWN					200

/*
 * This is a list of user defined messages used internally by the transfer
 * display functions.
 */

#define	WM_DLG_TO_DISPLAY				WM_USER+161		/* wMsg value */

#define	XFR_BUTTON_PUSHED				1				/* wPar value */
														/* lPar is button ID */

#define	XFR_UPDATE_DLG					2				/* wPar value */
														/* a fake message */

#define	XFR_SINGLE_TO_DOUBLE			3				/* wPar value */




#define	WM_DISPLAY_TO_DLG				WM_USER+162		/* wMsg value */

#define	XFR_UPDATE_DATA					1				/* wPar value */
														/* lPar is pointer */

#define	XFR_FORCE_DATA					2				/* wPar value */
														/* lPar is pointer */

#define	XFR_GET_DATA					3				/* wPar value */

#endif

