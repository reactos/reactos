/*****************************************************************/
/**				  Microsoft Windows for Workgroups				**/
/**		      Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/* NPMSG.H -- Definition of MsgBox subroutine.
 *
 * History:
 *	05/06/93	gregj	Created
 *	10/07/93	gregj	Added DisplayGenericError.
 */

#ifndef _INC_NPMSG
#define _INC_NPMSG

class NLS_STR;			/* forward declaration */

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


#define	IDS_MSGTITLE	1024

extern int MsgBox( HWND hwndDlg, UINT idMsg, UINT wFlags, const NLS_STR **apnls = NULL );
extern UINT DisplayGenericError(HWND hwnd, UINT msg, UINT err, LPCSTR psz1, LPCSTR psz2, WORD wFlags, UINT nMsgBase);


#ifndef RC_INVOKED
#pragma pack()
#endif

#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif	/* _INC_NPMSG */
