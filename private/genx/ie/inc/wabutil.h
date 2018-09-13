/*
 *	WABUTIL.H
 *
 *  Definitions and prototypes for utility functions provided by MAPI
 *  in MAPI[xx].DLL.
 *
 *  Copyright 1986-1998 Microsoft Corporation. All Rights Reserved.
 */

#if !defined(_MAPIUTIL_H) && !defined(_WABUTIL_H)
#define _WABUTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BEGIN_INTERFACE
#define BEGIN_INTERFACE
#endif


/* IMAPITable in memory */

/* ITableData Interface ---------------------------------------------------- */

DECLARE_MAPI_INTERFACE_PTR(ITableData, LPTABLEDATA);

typedef void (STDAPICALLTYPE CALLERRELEASE)(
	ULONG		ulCallerData,
	LPTABLEDATA	lpTblData,
	LPMAPITABLE	lpVue
);

#define MAPI_ITABLEDATA_METHODS(IPURE)									\
	MAPIMETHOD(HrGetView)												\
		(THIS_	LPSSortOrderSet				lpSSortOrderSet,			\
				CALLERRELEASE FAR *			lpfCallerRelease,			\
				ULONG						ulCallerData,				\
				LPMAPITABLE FAR *			lppMAPITable) IPURE;		\
	MAPIMETHOD(HrModifyRow)												\
		(THIS_	LPSRow) IPURE;											\
	MAPIMETHOD(HrDeleteRow)												\
		(THIS_	LPSPropValue				lpSPropValue) IPURE;		\
	MAPIMETHOD(HrQueryRow)												\
		(THIS_	LPSPropValue				lpsPropValue,				\
				LPSRow FAR *				lppSRow,					\
				ULONG FAR *					lpuliRow) IPURE;			\
	MAPIMETHOD(HrEnumRow)												\
		(THIS_	ULONG						ulRowNumber,				\
				LPSRow FAR *				lppSRow) IPURE;				\
	MAPIMETHOD(HrNotify)												\
		(THIS_	ULONG						ulFlags,					\
				ULONG						cValues,					\
				LPSPropValue				lpSPropValue) IPURE;		\
	MAPIMETHOD(HrInsertRow)												\
		(THIS_	ULONG						uliRow,						\
				LPSRow						lpSRow) IPURE;				\
	MAPIMETHOD(HrModifyRows)											\
		(THIS_	ULONG						ulFlags,					\
				LPSRowSet					lpSRowSet) IPURE;			\
	MAPIMETHOD(HrDeleteRows)											\
		(THIS_	ULONG						ulFlags,					\
				LPSRowSet					lprowsetToDelete,			\
				ULONG FAR *					cRowsDeleted) IPURE;		\

#undef		 INTERFACE
#define		 INTERFACE	ITableData
DECLARE_MAPI_INTERFACE_(ITableData, IUnknown)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(PURE)
	MAPI_ITABLEDATA_METHODS(PURE)
};


/* Entry Point for in memory ITable */


/*	CreateTable()
 *		Creates the internal memory structures and object handle
 *		to bring a new table into existence.
 *
 *	lpInterface
 *		Interface ID of the TableData object (IID_IMAPITableData)
 *
 *	lpAllocateBuffer, lpAllocateMore, and lpFreeBuffer
 *		Function addresses are provided by the caller so that
 *		this DLL allocates/frees memory appropriately.
 *	lpvReserved
 *		Reserved.  Should be NULL.
 *	ulTableType
 *		TBLTYPE_DYNAMIC, etc.  Visible to the calling application
 *		as part of the GetStatus return data on its views
 *	ulPropTagIndexColumn
 *		Index column for use when changing the data
 *	lpSPropTagArrayColumns
 *		Column proptags for the minimum set of columns in the table
 *	lppTableData
 *		Address of the pointer which will receive the TableData object
 */

STDAPI_(SCODE)
CreateTable( LPCIID					lpInterface,
			 ALLOCATEBUFFER FAR *	lpAllocateBuffer,
			 ALLOCATEMORE FAR *		lpAllocateMore,
			 FREEBUFFER FAR *		lpFreeBuffer,
			 LPVOID					lpvReserved,
			 ULONG					ulTableType,
			 ULONG					ulPropTagIndexColumn,
			 LPSPropTagArray		lpSPropTagArrayColumns,
			 LPTABLEDATA FAR *		lppTableData );


/*	HrGetView()
 *		This function obtains a new view on the underlying data
 *		which supports the IMAPITable interface.  All rows and columns
 *		of the underlying table data are initially visible
 *	lpSSortOrderSet
 *		if specified, results in the view being sorted
 *	lpfCallerRelease
 *		pointer to a routine to be called when the view is released, or
 *		NULL.
 *	ulCallerData
 *		arbitrary data the caller wants saved with this view and returned in
 *		the Release callback.
 */

/*	HrModifyRows()
 *		Add or modify a set of rows in the table data
 *	ulFlags
 *		Must be zero
 *	lpSRowSet
 *		Each row in the row set contains all the properties for one row
 *		in the table.  One of the properties must be the index column.  Any
 *		row in the table with the same value for its index column is
 *		replaced, or if there is no current row with that value the
 *		row is added.
 *		Each row in LPSRowSet MUST have a unique Index column!
 *		If any views are open, the view is updated as well.
 *		The properties do not have to be in the same order as the
 *		columns in the current table
 */

/*	HrModifyRow()
 *		Add or modify one row in the table
 *	lpSRow
 *		This row contains all the properties for one row in the table.
 *		One of the properties must be the index column.	 Any row in
 *		the table with the same value for its index column is
 *		replaced, or if there is no current row with that value the
 *		row is added
 *		If any views are open, the view is updated as well.
 *		The properties do not have to be in the same order as the
 *		columns in the current table
 */

/*	HrDeleteRows()
 *		Delete a row in the table.
 *	ulFlags
 *		TAD_ALL_ROWS - Causes all rows in the table to be deleted
 *					   lpSRowSet is ignored in this case.
 *	lpSRowSet
 *		Each row in the row set contains all the properties for one row
 *		in the table.  One of the properties must be the index column.  Any
 *		row in the table with the same value for its index column is
 *		deleted.
 *		Each row in LPSRowSet MUST have a unique Index column!
 *		If any views are open, the view is updated as well.
 *		The properties do not have to be in the same order as the
 *		columns in the current table
 */
#define	TAD_ALL_ROWS	1

/*	HrDeleteRow()
 *		Delete a row in the table.
 *	lpSPropValue
 *		This property value specifies the row which has this value
 *		for its index column
 */

/*	HrQueryRow()
 *		Returns the values of a specified row in the table
 *	lpSPropValue
 *		This property value specifies the row which has this value
 *		for its index column
 *	lppSRow
 *		Address of where to return a pointer to an SRow
 *	lpuliRow
 *	  Address of where to return the row number. This can be NULL
 *	  if the row number is not required.
 *
 */

/*	HrEnumRow()
 *		Returns the values of a specific (numbered) row in the table
 *	ulRowNumber
 *		Indicates row number 0 to n-1
 *	lppSRow
 *		Address of where to return a pointer to a SRow
 */

/*	HrInsertRow()
 *		Inserts a row into the table.
 *	uliRow
 *		The row number before which this row will be inserted into the table.
 *		Row numbers can be from 0 to n where o to n-1 result in row insertion
 *	  a row number of n results in the row being appended to the table.
 *	lpSRow
 *		This row contains all the properties for one row in the table.
 *		One of the properties must be the index column.	 Any row in
 *		the table with the same value for its index column is
 *		replaced, or if there is no current row with that value the
 *		row is added
 *		If any views are open, the view is updated as well.
 *		The properties do not have to be in the same order as the
 *		columns in the current table
 */


/* IMAPIProp in memory */

/* IPropData Interface ---------------------------------------------------- */


#define MAPI_IPROPDATA_METHODS(IPURE)									\
	MAPIMETHOD(HrSetObjAccess)											\
		(THIS_	ULONG						ulAccess) IPURE;			\
	MAPIMETHOD(HrSetPropAccess)											\
		(THIS_	LPSPropTagArray				lpPropTagArray,				\
				ULONG FAR *					rgulAccess) IPURE;			\
	MAPIMETHOD(HrGetPropAccess)											\
		(THIS_	LPSPropTagArray FAR *		lppPropTagArray,			\
				ULONG FAR * FAR *			lprgulAccess) IPURE;		\
	MAPIMETHOD(HrAddObjProps)											\
		(THIS_	LPSPropTagArray				lppPropTagArray,			\
				LPSPropProblemArray FAR *	lprgulAccess) IPURE;


#undef		 INTERFACE
#define		 INTERFACE	IPropData
DECLARE_MAPI_INTERFACE_(IPropData, IMAPIProp)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(PURE)
	MAPI_IMAPIPROP_METHODS(PURE)
	MAPI_IPROPDATA_METHODS(PURE)
};

DECLARE_MAPI_INTERFACE_PTR(IPropData, LPPROPDATA);


/* Entry Point for in memory IMAPIProp */


/*	CreateIProp()
 *		Creates the internal memory structures and object handle
 *		to bring a new property interface into existance.
 *
 *	lpInterface
 *		Interface ID of the TableData object (IID_IMAPIPropData)
 *
 *	lpAllocateBuffer, lpAllocateMore, and lpFreeBuffer
 *		Function addresses are provided by the caller so that
 *		this DLL allocates/frees memory appropriately.
 *	lppPropData
 *		Address of the pointer which will receive the IPropData object
 *	lpvReserved
 *		Reserved.  Should be NULL.
 */

// If MAPI isn't included, use WABCreateIProp instead
#ifndef CreateIProp
STDAPI_(SCODE)
CreateIProp( LPCIID					lpInterface,
			 ALLOCATEBUFFER FAR *	lpAllocateBuffer,
			 ALLOCATEMORE FAR *		lpAllocateMore,
			 FREEBUFFER FAR *		lpFreeBuffer,
			 LPVOID					lpvReserved,
			 LPPROPDATA FAR *		lppPropData );
#endif

STDAPI_(SCODE)
WABCreateIProp( LPCIID					lpInterface,
			 ALLOCATEBUFFER FAR *	lpAllocateBuffer,
			 ALLOCATEMORE FAR *		lpAllocateMore,
			 FREEBUFFER FAR *		lpFreeBuffer,
			 LPVOID					lpvReserved,
			 LPPROPDATA FAR *		lppPropData );

/*
 *	Defines for prop/obj access
 */
#define IPROP_READONLY		((ULONG) 0x00000001)
#define IPROP_READWRITE		((ULONG) 0x00000002)
#define IPROP_CLEAN			((ULONG) 0x00010000)
#define IPROP_DIRTY			((ULONG) 0x00020000)

/*
 -	HrSetPropAccess
 -
 *	Sets access right attributes on a per-property basis.  By default,
 *	all properties are read/write.
 *
 */

/*
 -	HrSetObjAccess
 -
 *	Sets access rights for the object itself.  By default, the object has
 *	read/write access.
 *
 */

#ifndef NOIDLEENGINE

/* Idle time scheduler */

/*
 *	PRI
 *
 *	Priority of an idle task.
 *	The idle engine sorts tasks by priority, and the one with the higher
 *	value runs first. Within a priority level, the functions are called
 *	round-robin.
 */

#define PRILOWEST	-32768
#define PRIHIGHEST	32767
#define PRIUSER		0

/*
 *	IRO
 *
 *	Idle routine options.  This is a combined bit mask consisting of
 *	individual firo's.	Listed below are the possible bit flags.
 *
 *		FIROWAIT and FIROINTERVAL are mutually exclusive.
 *		If neither of the flags are specified, the default action
 *		is to ignore the time parameter of the idle function and
 *		call it as often as possible if firoPerBlock is not set;
 *		otherwise call it one time only during the idle block
 *		once the time constraint has been set. FIROINTERVAL
 *		is also incompatible with FIROPERBLOCK.
 *
 *		FIROWAIT		- time given is minimum idle time before calling
 *						  for the first time in the block of idle time,
 *						  afterwhich call as often as possible.
 *		FIROINTERVAL	- time given is minimum interval between each
 *						  successive call
 *		FIROPERBLOCK	- called only once per contiguous block of idle
 *						  time
 *		FIRODISABLED	- initially disabled when registered, the
 *						  default is to enable the function when registered.
 *		FIROONCEONLY	- called only one time by the scheduler and then
 *						  deregistered automatically.
 */

#define IRONULL			((USHORT) 0x0000)
#define FIROWAIT		((USHORT) 0x0001)
#define FIROINTERVAL	((USHORT) 0x0002)
#define FIROPERBLOCK	((USHORT) 0x0004)
#define FIRODISABLED	((USHORT) 0x0020)
#define FIROONCEONLY	((USHORT) 0x0040)

/*
 *	IRC
 *
 *	Idle routine change options. This is a combined bit mask consisting
 *	of individual firc's; each one identifies an aspect of the idle task
 *	that can be changed.
 *
 */

#define IRCNULL			((USHORT) 0x0000)
#define FIRCPFN			((USHORT) 0x0001)	/* change function pointer */
#define FIRCPV			((USHORT) 0x0002)	/* change parameter block  */
#define FIRCPRI			((USHORT) 0x0004)	/* change priority		   */
#define FIRCCSEC		((USHORT) 0x0008)	/* change time			   */
#define FIRCIRO			((USHORT) 0x0010)	/* change routine options  */

/*
 *	Type definition for idle functions.	 An idle function takes one
 *	parameter, an PV, and returns a BOOL value.
 */

typedef BOOL (STDAPICALLTYPE FNIDLE) (LPVOID);
typedef FNIDLE FAR *PFNIDLE;

/*
 *	FTG
 *
 *	Function Tag.  Used to identify a registered idle function.
 *
 */

typedef void FAR *FTG;
typedef FTG  FAR *PFTG;
#define FTGNULL			((FTG) NULL)

/*
 -	MAPIInitIdle/MAPIDeinitIdle
 -
 *	Purpose:
 *		Initialises the idle engine
 *		If the initialisation succeded, returns 0, else returns -1
 *
 *	Arguments:
 *		lpvReserved		Reserved, must be NULL.
 */

STDAPI_(LONG)
MAPIInitIdle (LPVOID lpvReserved);

STDAPI_(VOID)
MAPIDeinitIdle (VOID);


/*
 *	FtgRegisterIdleRoutine
 *
 *		Registers the function pfn of type PFNIDLE, i.e., (BOOL (*)(LPVOID))
 *		as an idle function.
 *
 *		The idle function will be called with the parameter pv by the
 *		idle engine. The function has initial priority priIdle,
 *		associated time csecIdle, and options iroIdle.
 */

STDAPI_(FTG)
FtgRegisterIdleRoutine (PFNIDLE lpfnIdle, LPVOID lpvIdleParam,
	short priIdle, ULONG csecIdle, USHORT iroIdle);

/*
 *	DeregisterIdleRoutine
 *
 *		Removes the given routine from the list of idle routines.
 *		The routine will not be called again.  It is the responsibility
 *		of the caller to clean up any data structures pointed to by the
 *		pvIdleParam parameter; this routine does not free the block.
 */

STDAPI_(void)
DeregisterIdleRoutine (FTG ftg);

/*
 *	EnableIdleRoutine
 *
 *		Enables or disables an idle routine.
 */

STDAPI_(void)
EnableIdleRoutine (FTG ftg, BOOL fEnable);

/*
 *	ChangeIdleRoutine
 *
 *		Changes some or all of the characteristics of the given idle
 *		function. The changes to make are indicated with flags in the
 *		ircIdle parameter.
 */

STDAPI_(void)
ChangeIdleRoutine (FTG ftg, PFNIDLE lpfnIdle, LPVOID lpvIdleParam,
	short priIdle, ULONG csecIdle, USHORT iroIdle, USHORT ircIdle);


#endif	/* ! NOIDLEENGINE */


/* IMalloc Utilities */

STDAPI_(LPMALLOC) MAPIGetDefaultMalloc(VOID);


/* StreamOnFile (SOF) */

/*
 *	Methods and #define's for implementing an OLE 2.0 storage stream
 *	(as defined in the OLE 2.0 specs) on top of a system file.
 */

#define SOF_UNIQUEFILENAME	((ULONG) 0x80000000)

STDMETHODIMP OpenStreamOnFile(
	LPALLOCATEBUFFER	lpAllocateBuffer,
	LPFREEBUFFER		lpFreeBuffer,
	ULONG				ulFlags,
	LPTSTR				lpszFileName,
	LPTSTR				lpszPrefix,
	LPSTREAM FAR *		lppStream);

typedef HRESULT (STDMETHODCALLTYPE FAR * LPOPENSTREAMONFILE) (
	LPALLOCATEBUFFER	lpAllocateBuffer,
	LPFREEBUFFER		lpFreeBuffer,
	ULONG				ulFlags,
	LPTSTR				lpszFileName,
	LPTSTR				lpszPrefix,
	LPSTREAM FAR *		lppStream);

#ifdef	_WIN32
#define OPENSTREAMONFILE "OpenStreamOnFile"
#endif
#ifdef	WIN16
#define OPENSTREAMONFILE "_OPENSTREAMONFILE"
#endif


/* Property interface utilities */

/*
 *	Copies a single SPropValue from Src to Dest.  Handles all the various
 *	types of properties and will link its allocations given the master
 *	allocation object and an allocate more function.
 */
STDAPI_(SCODE)
PropCopyMore( LPSPropValue		lpSPropValueDest,
			  LPSPropValue		lpSPropValueSrc,
			  ALLOCATEMORE *	lpfAllocMore,
			  LPVOID			lpvObject );

/*
 *	Returns the size in bytes of structure at lpSPropValue, including the
 *	Value.
 */
STDAPI_(ULONG)
UlPropSize(	LPSPropValue	lpSPropValue );


STDAPI_(BOOL)
FEqualNames( LPMAPINAMEID lpName1, LPMAPINAMEID lpName2 );

#if defined(_WIN32) && !defined(_WINNT) && !defined(_WIN95) && !defined(_MAC)
#define _WINNT
#endif

STDAPI_(void)
GetInstance(LPSPropValue lpPropMv, LPSPropValue lpPropSv, ULONG uliInst);

extern unsigned char rgchCsds[];
extern unsigned char rgchCids[];
extern unsigned char rgchCsdi[];
extern unsigned char rgchCidi[];

STDAPI_(BOOL)
FPropContainsProp( LPSPropValue	lpSPropValueDst,
				   LPSPropValue	lpSPropValueSrc,
				   ULONG		ulFuzzyLevel );

STDAPI_(BOOL)
FPropCompareProp( LPSPropValue	lpSPropValue1,
				  ULONG			ulRelOp,
				  LPSPropValue	lpSPropValue2 );

STDAPI_(LONG)
LPropCompareProp( LPSPropValue	lpSPropValueA,
				  LPSPropValue	lpSPropValueB );

STDAPI_(HRESULT)
HrAddColumns(	LPMAPITABLE			lptbl,
				LPSPropTagArray		lpproptagColumnsNew,
				LPALLOCATEBUFFER	lpAllocateBuffer,
				LPFREEBUFFER		lpFreeBuffer);

STDAPI_(HRESULT)
HrAddColumnsEx(	LPMAPITABLE			lptbl,
				LPSPropTagArray		lpproptagColumnsNew,
				LPALLOCATEBUFFER	lpAllocateBuffer,
				LPFREEBUFFER		lpFreeBuffer,
				void				(FAR *lpfnFilterColumns)(LPSPropTagArray ptaga));


/* Notification utilities */

/*
 *	Function that creates an advise sink object given a notification
 *	callback function and context.
 */

STDAPI
HrAllocAdviseSink( LPNOTIFCALLBACK lpfnCallback,
				   LPVOID lpvContext,
				   LPMAPIADVISESINK FAR *lppAdviseSink );


/*
 *	Wraps an existing advise sink with another one which guarantees
 *	that the original advise sink will be called in the thread on
 *	which it was created.
 */

STDAPI
HrThisThreadAdviseSink( LPMAPIADVISESINK lpAdviseSink,
						LPMAPIADVISESINK FAR *lppAdviseSink);



/*
 *	Allows a client and/or provider to force notifications
 *	which are currently queued in the MAPI notification engine
 *	to be dispatched without doing a message dispatch.
 */

STDAPI HrDispatchNotifications (ULONG ulFlags);


/* Service Provider Utilities */

/*
 *	Structures and utility function for building a display table
 *	from resources.
 */

typedef struct {
	ULONG			ulCtlType;			/* DTCT_LABEL, etc. */
	ULONG			ulCtlFlags;			/* DT_REQUIRED, etc. */
	LPBYTE			lpbNotif;			/*	pointer to notification data */
	ULONG			cbNotif;			/* count of bytes of notification data */
	LPTSTR			lpszFilter;			/* character filter for edit/combobox */
	ULONG			ulItemID;			/* to validate parallel dlg template entry */
	union {								/* ulCtlType discriminates */
		LPVOID			lpv;			/* Initialize this to avoid warnings */
		LPDTBLLABEL		lplabel;
		LPDTBLEDIT		lpedit;
		LPDTBLLBX		lplbx;
		LPDTBLCOMBOBOX	lpcombobox;
		LPDTBLDDLBX		lpddlbx;
		LPDTBLCHECKBOX	lpcheckbox;
		LPDTBLGROUPBOX	lpgroupbox;
		LPDTBLBUTTON	lpbutton;
		LPDTBLRADIOBUTTON lpradiobutton;
		LPDTBLMVLISTBOX	lpmvlbx;
		LPDTBLMVDDLBX	lpmvddlbx;
		LPDTBLPAGE		lppage;
	} ctl;
} DTCTL, FAR *LPDTCTL;

typedef struct {
	ULONG			cctl;
	LPTSTR			lpszResourceName;	/* as usual, may be an integer ID */
	union {								/* as usual, may be an integer ID */
		LPTSTR			lpszComponent;
		ULONG			ulItemID;
	};
	LPDTCTL			lpctl;
} DTPAGE, FAR *LPDTPAGE;



STDAPI
BuildDisplayTable(	LPALLOCATEBUFFER	lpAllocateBuffer,
					LPALLOCATEMORE		lpAllocateMore,
					LPFREEBUFFER		lpFreeBuffer,
					LPMALLOC			lpMalloc,
					HINSTANCE			hInstance,
					UINT				cPages,
					LPDTPAGE			lpPage,
					ULONG				ulFlags,
					LPMAPITABLE *		lppTable,
					LPTABLEDATA	*		lppTblData );


/* MAPI structure validation/copy utilities */

/*
 *	Validate, copy, and adjust pointers in MAPI structures:
 *		notification
 *		property value array
 *		option data
 */

STDAPI_(SCODE)
ScCountNotifications(int cNotifications, LPNOTIFICATION lpNotifications,
		ULONG FAR *lpcb);

STDAPI_(SCODE)
ScCopyNotifications(int cNotification, LPNOTIFICATION lpNotifications,
		LPVOID lpvDst, ULONG FAR *lpcb);

STDAPI_(SCODE)
ScRelocNotifications(int cNotification, LPNOTIFICATION lpNotifications,
		LPVOID lpvBaseOld, LPVOID lpvBaseNew, ULONG FAR *lpcb);


STDAPI_(SCODE)
ScCountProps(int cValues, LPSPropValue lpPropArray, ULONG FAR *lpcb);

STDAPI_(LPSPropValue)
LpValFindProp(ULONG ulPropTag, ULONG cValues, LPSPropValue lpPropArray);

STDAPI_(SCODE)
ScCopyProps(int cValues, LPSPropValue lpPropArray, LPVOID lpvDst,
		ULONG FAR *lpcb);

STDAPI_(SCODE)
ScRelocProps(int cValues, LPSPropValue lpPropArray,
		LPVOID lpvBaseOld, LPVOID lpvBaseNew, ULONG FAR *lpcb);

STDAPI_(SCODE)
ScDupPropset(int cValues, LPSPropValue lpPropArray,
		LPALLOCATEBUFFER lpAllocateBuffer, LPSPropValue FAR *lppPropArray);


/* General utility functions */

/* Related to the OLE Component object model */

STDAPI_(ULONG)			UlAddRef(LPVOID lpunk);
STDAPI_(ULONG)			UlRelease(LPVOID lpunk);

/* Related to the MAPI interface */

STDAPI					HrGetOneProp(LPMAPIPROP lpMapiProp, ULONG ulPropTag,
						LPSPropValue FAR *lppProp);
STDAPI					HrSetOneProp(LPMAPIPROP lpMapiProp,
						LPSPropValue lpProp);
STDAPI_(BOOL)			FPropExists(LPMAPIPROP lpMapiProp, ULONG ulPropTag);
STDAPI_(LPSPropValue)	PpropFindProp(LPSPropValue lpPropArray, ULONG cValues,
						ULONG ulPropTag);
STDAPI_(void)			FreePadrlist(LPADRLIST lpAdrlist);
STDAPI_(void)			FreeProws(LPSRowSet lpRows);
STDAPI					HrQueryAllRows(LPMAPITABLE lpTable,
						LPSPropTagArray lpPropTags,
						LPSRestriction lpRestriction,
						LPSSortOrderSet lpSortOrderSet,
						LONG crowsMax,
						LPSRowSet FAR *lppRows);




/* C runtime substitutes */


STDAPI_(LPTSTR)			SzFindCh(LPCTSTR lpsz, USHORT ch);		/* strchr */
STDAPI_(LPTSTR)			SzFindLastCh(LPCTSTR lpsz, USHORT ch);	/* strrchr */
STDAPI_(LPTSTR)			SzFindSz(LPCTSTR lpsz, LPCTSTR lpszKey); /*strstr */
STDAPI_(unsigned int)	UFromSz(LPCTSTR lpsz);					/* atoi */

STDAPI_(SCODE)			ScUNCFromLocalPath(LPSTR lpszLocal, LPSTR lpszUNC,
						UINT cchUNC);
STDAPI_(SCODE)			ScLocalPathFromUNC(LPSTR lpszUNC, LPSTR lpszLocal,
						UINT cchLocal);

/* 64-bit arithmetic with times */

STDAPI_(FILETIME)		FtAddFt(FILETIME ftAddend1, FILETIME ftAddend2);
STDAPI_(FILETIME)		FtMulDwDw(DWORD ftMultiplicand, DWORD ftMultiplier);
STDAPI_(FILETIME)		FtMulDw(DWORD ftMultiplier, FILETIME ftMultiplicand);
STDAPI_(FILETIME)		FtSubFt(FILETIME ftMinuend, FILETIME ftSubtrahend);
STDAPI_(FILETIME)		FtNegFt(FILETIME ft);

/* Message composition */

STDAPI_(SCODE)			ScCreateConversationIndex (ULONG cbParent,
							LPBYTE lpbParent,
							ULONG FAR *	lpcbConvIndex,
							LPBYTE FAR * lppbConvIndex);

/* Store support */

STDAPI WrapStoreEntryID (ULONG ulFlags, LPTSTR lpszDLLName, ULONG cbOrigEntry,
	LPENTRYID lpOrigEntry, ULONG *lpcbWrappedEntry, LPENTRYID *lppWrappedEntry);

/* RTF Sync Utilities */

#define RTF_SYNC_RTF_CHANGED	((ULONG) 0x00000001)
#define RTF_SYNC_BODY_CHANGED	((ULONG) 0x00000002)

STDAPI_(HRESULT)
RTFSync (LPMESSAGE lpMessage, ULONG ulFlags, BOOL FAR * lpfMessageUpdated);


/* Flags for WrapCompressedRTFStream() */

/****** MAPI_MODIFY				((ULONG) 0x00000001) mapidefs.h */
/****** STORE_UNCOMPRESSED_RTF	((ULONG) 0x00008000) mapidefs.h */

STDAPI_(HRESULT)
WrapCompressedRTFStream (LPSTREAM lpCompressedRTFStream,
		ULONG ulFlags, LPSTREAM FAR * lpUncompressedRTFStream);

/* Storage on Stream */

#if defined(_WIN32) || defined(WIN16)
STDAPI_(HRESULT)
HrIStorageFromStream (LPUNKNOWN lpUnkIn,
	LPCIID lpInterface, ULONG ulFlags, LPSTORAGE FAR * lppStorageOut);
#endif


/*
 * Setup and cleanup.
 *
 * Providers never need to make these calls.
 *
 * Test applications and the like which do not call MAPIInitialize
 * may want to call them, so that the few utility functions which
 * need MAPI allocators (and do not ask for them explicitly)
 * will work.
 */

/* All flags are reserved for ScInitMapiUtil. */

STDAPI_(SCODE)			ScInitMapiUtil(ULONG ulFlags);
STDAPI_(VOID)			DeinitMapiUtil(VOID);


/*
 *	Entry point names.
 *	
 *	These are for new entry points defined since MAPI first shipped
 *	in Windows 95. Using these names in a GetProcAddress call makes
 *	it easier to write code which uses them optionally.
 */

#if defined (WIN16)
#define szHrDispatchNotifications "HrDispatchNotifications"
#elif defined (_WIN32) && defined (_X86_)
#define szHrDispatchNotifications "_HrDispatchNotifications@4"
#elif defined (_ALPHA_) || defined (_MIPS_) || defined (_PPC_)
#define szHrDispatchNotifications "HrDispatchNotifications"
#endif

typedef HRESULT (STDAPICALLTYPE DISPATCHNOTIFICATIONS)(ULONG ulFlags);
typedef DISPATCHNOTIFICATIONS FAR * LPDISPATCHNOTIFICATIONS;

#if defined (WIN16)
#define szScCreateConversationIndex "ScCreateConversationIndex"
#elif defined (_WIN32) && defined (_X86_)
#define szScCreateConversationIndex "_ScCreateConversationIndex@16"
#elif defined (_ALPHA_) || defined (_MIPS_) || defined (_PPC_)
#define szScCreateConversationIndex "ScCreateConversationIndex"
#endif

typedef SCODE (STDAPICALLTYPE CREATECONVERSATIONINDEX)(ULONG cbParent,
	LPBYTE lpbParent, ULONG FAR *lpcbConvIndex, LPBYTE FAR *lppbConvIndex);
typedef CREATECONVERSATIONINDEX FAR *LPCREATECONVERSATIONINDEX;

#ifdef __cplusplus
}
#endif

#endif /* _WABUTIL_H_ */
