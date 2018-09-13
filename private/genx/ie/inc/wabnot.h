/*
 *	WABNOT.H
 *
 * Defines Notification structures.  These are also defined in mapispi.h.
 *
 * Copyright 1986-1998 Microsoft Corporation. All Rights Reserved.
 */

#if !defined(MAPISPI_H) && !defined(WABSPI_H)
#define WABSPI_H
/* Include common MAPI header files if they haven't been already. */


#ifndef BEGIN_INTERFACE
#define BEGIN_INTERFACE
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Notification key structure for the MAPI notification engine */

typedef struct
{
	ULONG		cb;				/* How big the key is */
	BYTE		ab[MAPI_DIM];	/* Key contents */
} NOTIFKEY, FAR * LPNOTIFKEY;

#define CbNewNOTIFKEY(_cb)		(offsetof(NOTIFKEY,ab) + (_cb))
#define CbNOTIFKEY(_lpkey)		(offsetof(NOTIFKEY,ab) + (_lpkey)->cb)
#define SizedNOTIFKEY(_cb, _name) \
	struct _NOTIFKEY_ ## _name \
{ \
	ULONG		cb; \
	BYTE		ab[_cb]; \
} _name


/* For Subscribe() */

#define NOTIFY_SYNC				((ULONG) 0x40000000)

/* For Notify() */

#define NOTIFY_CANCELED			((ULONG) 0x80000000)


/* From the Notification Callback function (well, this is really a ulResult) */

#define CALLBACK_DISCONTINUE	((ULONG) 0x80000000)

/* For Transport's SpoolerNotify() */

#define NOTIFY_NEWMAIL			((ULONG) 0x00000001)
#define NOTIFY_READYTOSEND		((ULONG) 0x00000002)
#define NOTIFY_SENTDEFERRED		((ULONG) 0x00000004)
#define NOTIFY_CRITSEC			((ULONG) 0x00001000)
#define NOTIFY_NONCRIT			((ULONG) 0x00002000)
#define NOTIFY_CONFIG_CHANGE	((ULONG) 0x00004000)
#define NOTIFY_CRITICAL_ERROR	((ULONG) 0x10000000)

/* For Message Store's SpoolerNotify() */

#define NOTIFY_NEWMAIL_RECEIVED	((ULONG) 0x20000000)

/* For ModifyStatusRow() */

#define	STATUSROW_UPDATE		((ULONG) 0x10000000)

/* For IStorageFromStream() */

#define STGSTRM_RESET			((ULONG) 0x00000000)
#define STGSTRM_CURRENT			((ULONG) 0x10000000)
#define STGSTRM_MODIFY			((ULONG) 0x00000002)
#define STGSTRM_CREATE			((ULONG) 0x00001000)

/* For GetOneOffTable() */
/****** MAPI_UNICODE			((ULONG) 0x80000000) */

/* For CreateOneOff() */
/****** MAPI_UNICODE			((ULONG) 0x80000000) */
/****** MAPI_SEND_NO_RICH_INFO	((ULONG) 0x00010000) */

/* For ReadReceipt() */
#define MAPI_NON_READ			((ULONG) 0x00000001)

/* For DoConfigPropSheet() */
/****** MAPI_UNICODE			((ULONG) 0x80000000) */

#ifdef __cplusplus
}
#endif

#endif /* MAPISPI_H */
