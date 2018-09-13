/* IAddrBook Interface ----------------------------------------------------- */

/*  CreateOneOff */
/****** MAPI_UNICODE			((ULONG) 0x80000000) */
/****** MAPI_SEND_NO_RICH_INFO		((ULONG) 0x00010000) */

/*  RecipOptions */
/****** MAPI_UNICODE			((ULONG) 0x80000000) */

/*  QueryDefaultRecipOpt */
/****** MAPI_UNICODE			((ULONG) 0x80000000) */

/*  GetSearchPath */
/****** MAPI_UNICODE			((ULONG) 0x80000000) */


#ifndef WABIAB_H
#define WABIAB_H

// These are WAB only flags for IAdrBook::ResolveName
//      MAPI_UNICODE                        ((ULONG) 0x80000000)
#define WAB_RESOLVE_LOCAL_ONLY              ((ULONG) 0x80000000) 
#define WAB_RESOLVE_ALL_EMAILS              ((ULONG) 0x40000000)
#define WAB_RESOLVE_NO_ONE_OFFS             ((ULONG) 0x20000000)
#define WAB_RESOLVE_NEED_CERT               ((ULONG) 0x10000000)
#define WAB_RESOLVE_NO_NOT_FOUND_UI         ((ULONG) 0x08000000)
#define WAB_RESOLVE_USE_CURRENT_PROFILE     ((ULONG) 0x04000000)
#define WAB_RESOLVE_FIRST_MATCH             ((ULONG) 0x02000000)
#define WAB_RESOLVE_UNICODE                 ((ULONG) 0x01000000)
//      MAPI_DIALOG                         ((ULONG) 0x00000008)

#ifndef MAPIX_H

#define MAPI_IADDRBOOK_METHODS(IPURE)									\
	MAPIMETHOD(OpenEntry)												\
		(THIS_	ULONG						cbEntryID,					\
				LPENTRYID					lpEntryID,					\
				LPCIID						lpInterface,				\
				ULONG						ulFlags,					\
				ULONG FAR *					lpulObjType,				\
				LPUNKNOWN FAR *				lppUnk) IPURE;	\
	MAPIMETHOD(CompareEntryIDs)											\
		(THIS_	ULONG						cbEntryID1,					\
				LPENTRYID					lpEntryID1,					\
				ULONG						cbEntryID2,					\
				LPENTRYID					lpEntryID2,					\
				ULONG						ulFlags,					\
				ULONG FAR *					lpulResult) IPURE;			\
	MAPIMETHOD(Advise)													\
		(THIS_	ULONG						cbEntryID,					\
				LPENTRYID					lpEntryID,					\
				ULONG						ulEventMask,				\
				LPMAPIADVISESINK			lpAdviseSink,				\
				ULONG FAR *					lpulConnection) IPURE;		\
	MAPIMETHOD(Unadvise)												\
		(THIS_	ULONG						ulConnection) IPURE;		\
	MAPIMETHOD(CreateOneOff)											\
		(THIS_	LPTSTR						lpszName,					\
				LPTSTR						lpszAdrType,				\
				LPTSTR						lpszAddress,				\
				ULONG						ulFlags,					\
				ULONG FAR *					lpcbEntryID,				\
				LPENTRYID FAR *				lppEntryID) IPURE;			\
	MAPIMETHOD(NewEntry)												\
		(THIS_	ULONG						ulUIParam,					\
				ULONG						ulFlags,					\
				ULONG						cbEIDContainer,				\
				LPENTRYID					lpEIDContainer,				\
				ULONG						cbEIDNewEntryTpl,			\
				LPENTRYID					lpEIDNewEntryTpl,			\
				ULONG FAR *					lpcbEIDNewEntry,			\
				LPENTRYID FAR *				lppEIDNewEntry) IPURE;		\
	MAPIMETHOD(ResolveName)												\
		(THIS_	ULONG_PTR						ulUIParam,					\
				ULONG						ulFlags,					\
				LPTSTR						lpszNewEntryTitle,			\
				LPADRLIST					lpAdrList) IPURE;			\
	MAPIMETHOD(Address)													\
		(THIS_	ULONG FAR *					lpulUIParam,				\
				LPADRPARM					lpAdrParms,					\
				LPADRLIST FAR *				lppAdrList) IPURE;			\
	MAPIMETHOD(Details)													\
		(THIS_	ULONG FAR *					lpulUIParam,				\
				LPFNDISMISS					lpfnDismiss,				\
				LPVOID						lpvDismissContext,			\
				ULONG						cbEntryID,					\
				LPENTRYID					lpEntryID,					\
				LPFNBUTTON					lpfButtonCallback,			\
				LPVOID						lpvButtonContext,			\
				LPTSTR						lpszButtonText,				\
				ULONG						ulFlags) IPURE;				\
	MAPIMETHOD(RecipOptions)											\
		(THIS_	ULONG						ulUIParam,					\
				ULONG						ulFlags,					\
				LPADRENTRY					lpRecip) IPURE;				\
	MAPIMETHOD(QueryDefaultRecipOpt)									\
		(THIS_	LPTSTR						lpszAdrType,				\
				ULONG						ulFlags,					\
				ULONG FAR *					lpcValues,					\
				LPSPropValue FAR *			lppOptions) IPURE;			\
	MAPIMETHOD(GetPAB)													\
		(THIS_	ULONG FAR *					lpcbEntryID,				\
				LPENTRYID FAR *				lppEntryID) IPURE;			\
	MAPIMETHOD(SetPAB)													\
		(THIS_	ULONG						cbEntryID,					\
				LPENTRYID					lpEntryID) IPURE;			\
	MAPIMETHOD(GetDefaultDir)											\
		(THIS_	ULONG FAR *					lpcbEntryID,				\
				LPENTRYID FAR *				lppEntryID) IPURE;			\
	MAPIMETHOD(SetDefaultDir)											\
		(THIS_	ULONG						cbEntryID,					\
				LPENTRYID					lpEntryID) IPURE;			\
	MAPIMETHOD(GetSearchPath)											\
		(THIS_	ULONG						ulFlags,					\
				LPSRowSet FAR *				lppSearchPath) IPURE;		\
	MAPIMETHOD(SetSearchPath)											\
		(THIS_	ULONG						ulFlags,					\
				LPSRowSet					lpSearchPath) IPURE;		\
	MAPIMETHOD(PrepareRecips)											\
		(THIS_	ULONG						ulFlags,					\
				LPSPropTagArray				lpPropTagArray,				\
				LPADRLIST					lpRecipList) IPURE;			\

#undef		 INTERFACE
#define		 INTERFACE  IAddrBook
DECLARE_MAPI_INTERFACE_(IAddrBook, IMAPIProp)
{
	BEGIN_INTERFACE	
	MAPI_IUNKNOWN_METHODS(PURE)
	MAPI_IMAPIPROP_METHODS(PURE)
	MAPI_IADDRBOOK_METHODS(PURE)
};

DECLARE_MAPI_INTERFACE_PTR(IAddrBook, LPADRBOOK);
#endif  // MAPIX_H
#endif  // WABIAB_H

