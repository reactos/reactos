/*
 * DDEML library
 *
 * Copyright 1997 Alexandre Julliard
 * Copyright 1997 Len White
 * Copyright 1999 Keith Matthews
 * Copyright 2000 Corel
 * Copyright 2001 Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_DDEML_PRIVATE_H
#define __WINE_DDEML_PRIVATE_H

/* defined in atom.c file.
 */
#define MAX_ATOM_LEN              255

/* Maximum buffer size ( including the '\0' ).
 */
#define MAX_BUFFER_LEN            (MAX_ATOM_LEN + 1)

/* The internal structures (prefixed by WDML) are used as follows:
 * + a WDML_INSTANCE is created for each instance creation (DdeInitialize)
 *      - a popup window (InstanceClass) is created for each instance.
 *      - this window is used to receive all the DDEML events (server registration,
 *	  conversation confirmation...). See the WM_WDML_???? messages for details
 * + when registering a server (DdeNameService) a WDML_SERVER is created
 *	- a popup window (ServerNameClass) is created
 * + a conversation is represented by two WDML_CONV structures:
 *	- one on the client side, the other one on the server side
 *	- this is needed because the address spaces may be different
 *	- therefore, two lists of links are kept for each instance
 *	- two windows are created for a conversation:
 *		o a popup window on client side (ClientConvClass)
 *		o a child window (of the ServerName) on the server side
 *		  (ServerConvClass)
 *	- all the exchanges then take place between those two windows
 *	- windows for the conversation exist in two forms (Ansi & Unicode). This
 *	  is only needed when a partner in a conv is not handled by DDEML. The
 *	  type (A/W) of the window is used to handle the ansi <=> unicode
 *        transformations
 *	- two handles are created for a conversation (on each side). Each handle
 *	  is linked to a structure. To help differentiate those handles, the
 *	  local one has an even value, whereas the remote one has an odd value.
 * + a (warm or link) is represented by two WDML_LINK structures:
 *	- one on client side, the other one on server side
 *	- therefore, two lists of links are kept for each instance
 *
 * To help getting back to data, WDML windows store information:
 *	- offset 0: the DDE instance
 *	- offset 4: the current conversation (for ClientConv and ServerConv only)
 *
 * All the implementation (client & server) makes the assumption that the other side
 * is not always a DDEML partner. However, if it's the case, supplementary services
 * are available (most notably the REGISTER/UNREGISTER and CONNECT_CONFIRM messages
 * to the callback function). To be correct in every situation, all the basic
 * exchanges are made using the 'pure' DDE protocol. A (future !) enhancement would
 * be to provide a new protocol in the case were both partners are handled by DDEML.
 *
 * The StringHandles are in fact stored as local atoms. So an HSZ and a (local) atom
 * can be used interchangably. However, in order to keep track of the allocated HSZ,
 * and to free them upon instance termination, all HSZ are stored in a link list.
 * When the HSZ need to be passed thru DDE messages, we need to convert them back and
 * forth to global atoms.
 */

/* this struct has the same mapping as all the DDE??? structures */
typedef struct {
    unsigned short unused:12,
		   fResponse:1,
                   fRelease:1,
		   fDeferUpd:1,
                   fAckReq:1;
    short    cfFormat;
} WINE_DDEHEAD;

typedef struct tagHSZNode
{
    struct tagHSZNode*		next;
    HSZ 			hsz;
    unsigned			refCount;
} HSZNode;

typedef struct tagWDML_SERVER
{
    struct tagWDML_SERVER*	next;
    HSZ				hszService;
    HSZ				hszServiceSpec;
    ATOM			atomService;
    ATOM			atomServiceSpec;
    BOOL			filterOn;
    HWND			hwndServer;
} WDML_SERVER;

typedef struct tagWDML_XACT {
    struct tagWDML_XACT*	next;		/* list of transactions in conversation */
    DWORD			xActID;
    UINT			ddeMsg;
    HDDEDATA			hDdeData;
    DWORD			dwTimeout;
    DWORD			hUser;
    UINT			wType;
    UINT			wFmt;
    HSZ				hszItem;
    ATOM			atom;		/* as converted from or to hszItem */
    HGLOBAL			hMem;
    LPARAM			lParam; 	/* useful for reusing */
} WDML_XACT;

typedef struct tagWDML_CONV
{
    struct tagWDML_CONV*	next;		/* to link all the conversations */
    struct tagWDML_INSTANCE*	instance;
    HSZ				hszService;	/* pmt used for connection */
    HSZ				hszTopic;	/* pmt used for connection */
    UINT			magic;		/* magic number to check validity */
    UINT			afCmd;		/* service name flag */
    CONVCONTEXT			convContext;
    HWND			hwndClient;	/* source of conversation (ClientConvClass) */
    HWND			hwndServer;	/* destination of conversation (ServerConvClass) */
    WDML_XACT*			transactions;	/* pending transactions */
    DWORD			hUser;		/* user defined value */
    DWORD			wStatus;	/* same bits as convinfo.wStatus */
    DWORD			wConvst;	/* same values as convinfo.wConvst */
} WDML_CONV;

#define WDML_CONV_MAGIC  0xbabe1234

/* DDE_LINK struct defines hot, warm, and cold links */
typedef struct tagWDML_LINK {
    struct tagWDML_LINK*	next;		/* to link all the active links */
    HCONV			hConv;		/* to get back to the converstaion */
    UINT			transactionType;/* 0 for no link */
    HSZ				hszItem;	/* item targetted for (hot/warm) link */
    UINT			uFmt;		/* format for data */
} WDML_LINK;

typedef struct tagWDML_INSTANCE
{
    struct tagWDML_INSTANCE*	next;
    DWORD           		instanceID;	/* needed to track monitor usage */
    DWORD			threadID;	/* needed to keep instance linked to a unique thread */
    BOOL			monitor;        /* have these two as full Booleans cos they'll be tested frequently */
    BOOL			clientOnly;	/* bit wasteful of space but it will be faster */
    BOOL			unicode;	/* Flag to indicate Win32 API used to initialise */
    BOOL			win16;          /* flag to indicate Win16 API used to initialize */
    HSZNode*			nodeList;	/* for cleaning upon exit */
    PFNCALLBACK     		callback;
    DWORD           		CBFflags;
    DWORD           		monitorFlags;
    DWORD			lastError;
    HWND			hwndEvent;
    DWORD			wStatus;	/* global instance status */
    WDML_SERVER*		servers;	/* list of registered servers */
    WDML_CONV*			convs[2];	/* active conversations for this instance (client and server) */
    WDML_LINK*			links[2];	/* active links for this instance (client and server) */
} WDML_INSTANCE;

extern CRITICAL_SECTION WDML_CritSect;		/* protection for instance list */

/* header for the DDE Data objects */
typedef struct tagDDE_DATAHANDLE_HEAD
{
    WORD	cfFormat;
    WORD        bAppOwned;
} DDE_DATAHANDLE_HEAD;

typedef enum tagWDML_SIDE
{
    WDML_CLIENT_SIDE = 0, WDML_SERVER_SIDE = 1
} WDML_SIDE;

typedef enum {
    WDML_QS_ERROR, WDML_QS_HANDLED, WDML_QS_PASS, WDML_QS_SWALLOWED, WDML_QS_BLOCK,
} WDML_QUEUE_STATE;

extern	HDDEDATA 	WDML_InvokeCallback(WDML_INSTANCE* pInst, UINT uType, UINT uFmt, HCONV hConv,
					    HSZ hsz1, HSZ hsz2, HDDEDATA hdata,
					    ULONG_PTR dwData1, ULONG_PTR dwData2);
extern	HDDEDATA 	WDML_InvokeCallback16(PFNCALLBACK pfn, UINT uType, UINT uFmt, HCONV hConv,
					      HSZ hsz1, HSZ hsz2, HDDEDATA hdata,
					      DWORD dwData1, DWORD dwData2);
extern	WDML_SERVER*	WDML_AddServer(WDML_INSTANCE* pInstance, HSZ hszService, HSZ hszTopic);
extern	void		WDML_RemoveServer(WDML_INSTANCE* pInstance, HSZ hszService, HSZ hszTopic);
extern	WDML_SERVER*	WDML_FindServer(WDML_INSTANCE* pInstance, HSZ hszService, HSZ hszTopic);
/* transaction handler on the server side */
extern WDML_QUEUE_STATE WDML_ServerHandle(WDML_CONV* pConv, WDML_XACT* pXAct);
/* transaction handler on the client side */
HDDEDATA WDML_ClientHandle(WDML_CONV *pConv, WDML_XACT *pXAct, DWORD dwTimeout, LPDWORD pdwResult) DECLSPEC_HIDDEN;
/* called both in DdeClientTransaction and server side. */
extern	UINT		WDML_Initialize(LPDWORD pidInst, PFNCALLBACK pfnCallback,
					DWORD afCmd, DWORD ulRes, BOOL bUnicode, BOOL b16);
extern	WDML_CONV* 	WDML_AddConv(WDML_INSTANCE* pInstance, WDML_SIDE side,
				     HSZ hszService, HSZ hszTopic, HWND hwndClient, HWND hwndServer);
extern	void		WDML_RemoveConv(WDML_CONV* pConv, WDML_SIDE side);
extern	WDML_CONV*	WDML_GetConv(HCONV hConv, BOOL checkConnected);
extern	WDML_CONV*	WDML_GetConvFromWnd(HWND hWnd);
extern	WDML_CONV*	WDML_FindConv(WDML_INSTANCE* pInstance, WDML_SIDE side,
				      HSZ hszService, HSZ hszTopic);
extern  BOOL		WDML_PostAck(WDML_CONV* pConv, WDML_SIDE side, WORD appRetCode,
				     BOOL fBusy, BOOL fAck, UINT pmt, LPARAM lParam, UINT oldMsg);
extern	void		WDML_AddLink(WDML_INSTANCE* pInstance, HCONV hConv, WDML_SIDE side,
				     UINT wType, HSZ hszItem, UINT wFmt);
extern	WDML_LINK*	WDML_FindLink(WDML_INSTANCE* pInstance, HCONV hConv, WDML_SIDE side,
				      HSZ hszItem, BOOL use_fmt, UINT uFmt);
extern	void 		WDML_RemoveLink(WDML_INSTANCE* pInstance, HCONV hConv, WDML_SIDE side,
					HSZ hszItem, UINT wFmt);
extern	void		WDML_RemoveAllLinks(WDML_INSTANCE* pInstance, WDML_CONV* pConv, WDML_SIDE side);
/* string internals */
extern	void 		WDML_FreeAllHSZ(WDML_INSTANCE* pInstance);
extern	BOOL		WDML_DecHSZ(WDML_INSTANCE* pInstance, HSZ hsz);
extern	BOOL		WDML_IncHSZ(WDML_INSTANCE* pInstance, HSZ hsz);
extern	ATOM		WDML_MakeAtomFromHsz(HSZ hsz);
extern	HSZ	        WDML_MakeHszFromAtom(const WDML_INSTANCE* pInstance, ATOM atom);
/* client calls these */
extern	WDML_XACT*	WDML_AllocTransaction(WDML_INSTANCE* pInstance, UINT ddeMsg, UINT wFmt, HSZ hszItem);
extern	void		WDML_QueueTransaction(WDML_CONV* pConv, WDML_XACT* pXAct);
extern	BOOL		WDML_UnQueueTransaction(WDML_CONV* pConv, WDML_XACT*  pXAct);
extern	void		WDML_FreeTransaction(WDML_INSTANCE* pInstance, WDML_XACT* pXAct, BOOL doFreePmt);
extern	WDML_XACT*	WDML_FindTransaction(WDML_CONV* pConv, DWORD tid);
extern  HGLOBAL     WDML_DataHandle2Global(HDDEDATA hDdeData, BOOL fResponse, BOOL fRelease,
                           BOOL fDeferUpd, BOOL fAckReq);
extern  HDDEDATA    WDML_Global2DataHandle(WDML_CONV* pConv, HGLOBAL hMem, WINE_DDEHEAD* p);
extern  BOOL            WDML_IsAppOwned(HDDEDATA hDdeData);
extern	WDML_INSTANCE*	WDML_GetInstance(DWORD InstId);
extern	WDML_INSTANCE*	WDML_GetInstanceFromWnd(HWND hWnd);
/* broadcasting to DDE windows */
extern	void		WDML_BroadcastDDEWindows(LPCWSTR clsName, UINT uMsg,
						 WPARAM wParam, LPARAM lParam);
extern	void		WDML_NotifyThreadExit(DWORD tid);

static __inline void WDML_ExtractAck(WORD status, DDEACK* da)
{
    *da = *((DDEACK*)&status);
}

extern const WCHAR WDML_szEventClass[]; /* class of window for events (aka instance) */
extern const char WDML_szServerConvClassA[]; /* ANSI class of window for server side conv */
extern const WCHAR WDML_szServerConvClassW[]; /* unicode class of window for server side conv */
extern const char WDML_szClientConvClassA[]; /* ANSI class of window for client side conv */
extern const WCHAR WDML_szClientConvClassW[]; /* unicode class of window for client side conv */

#define WM_WDML_REGISTER	(WM_USER + 0x200)
#define WM_WDML_UNREGISTER	(WM_USER + 0x201)
#define WM_WDML_CONNECT_CONFIRM	(WM_USER + 0x202)

/* parameters for messages:
 *			wParam				lParam
 * Register		atom for service name		atom for service spec
 * Unregister		atom for service name		atom for service spec
 * ConnectConfirm	client window handle		server window handle
 */

#define	GWL_WDML_INSTANCE	(0)
#define	GWL_WDML_CONVERSATION	(sizeof(ULONG_PTR))
#define	GWL_WDML_SERVER		(sizeof(ULONG_PTR))

#endif  /* __WINE_DDEML_PRIVATE_H */
