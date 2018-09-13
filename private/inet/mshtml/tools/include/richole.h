#ifndef _RICHOLE_
#define _RICHOLE_

/*
 *	RICHOLE.H
 *
 *	Purpose:
 *		OLE Extensions to the Rich Text Editor
 *
 *	Copyright (c) 1985-1996, Microsoft Corporation
 */

// Structure passed to GetObject and InsertObject
typedef struct _reobject
{
	DWORD			cbStruct;			// Size of structure
	LONG			cp;					// Character position of object
	CLSID			clsid;				// Class ID of object
	LPOLEOBJECT		poleobj;			// OLE object interface
	LPSTORAGE		pstg;				// Associated storage interface
	LPOLECLIENTSITE	polesite;			// Associated client site interface
	SIZEL			sizel;				// Size of object (may be 0,0)
	DWORD			dvaspect;			// Display aspect to use
	DWORD			dwFlags;			// Object status flags
	DWORD			dwUser;				// Dword for user's use
} REOBJECT;

// Flags to specify which interfaces should be returned in the structure above
#define REO_GETOBJ_NO_INTERFACES	(0x00000000L)
#define REO_GETOBJ_POLEOBJ			(0x00000001L)
#define REO_GETOBJ_PSTG				(0x00000002L)
#define REO_GETOBJ_POLESITE			(0x00000004L)
#define REO_GETOBJ_ALL_INTERFACES	(0x00000007L)

// Place object at selection
#define REO_CP_SELECTION ((ULONG) -1L)

// Use character position to specify object instead of index
#define REO_IOB_SELECTION ((ULONG) -1L)
#define REO_IOB_USE_CP ((ULONG) -2L)

// Object flags
#define REO_NULL			(0x00000000L)	// No flags
#define REO_READWRITEMASK	(0x0000003FL)	// Mask out RO bits
#define REO_DONTNEEDPALETTE	(0x00000020L)	// Object doesn't need palette
#define REO_BLANK			(0x00000010L)	// Object is blank
#define REO_DYNAMICSIZE		(0x00000008L)	// Object defines size always
#define REO_INVERTEDSELECT	(0x00000004L)	// Object drawn all inverted if sel
#define REO_BELOWBASELINE	(0x00000002L)	// Object sits below the baseline
#define REO_RESIZABLE		(0x00000001L)	// Object may be resized
#define REO_LINK			(0x80000000L)	// Object is a link (RO)
#define REO_STATIC			(0x40000000L)	// Object is static (RO)
#define REO_SELECTED		(0x08000000L)	// Object selected (RO)
#define REO_OPEN			(0x04000000L)	// Object open in its server (RO)
#define REO_INPLACEACTIVE	(0x02000000L)	// Object in place active (RO)
#define REO_HILITED			(0x01000000L)	// Object is to be hilited (RO)
#define REO_LINKAVAILABLE	(0x00800000L)	// Link believed available (RO)
#define REO_GETMETAFILE		(0x00400000L)	// Object requires metafile (RO)

// flags for IRichEditOle::GetClipboardData(),
// IRichEditOleCallback::GetClipboardData() and
// IRichEditOleCallback::QueryAcceptData()
#define RECO_PASTE			(0x00000000L)	// paste from clipboard
#define RECO_DROP			(0x00000001L)	// drop
#define RECO_COPY			(0x00000002L)	// copy to the clipboard
#define RECO_CUT			(0x00000003L)	// cut to the clipboard
#define RECO_DRAG			(0x00000004L)	// drag

/*
 *	IRichEditOle
 *
 *	Purpose:
 *		Interface used by the client of RichEdit to perform OLE-related
 *		operations.
 *
 *	//$ REVIEW:
 *		The methods herein may just want to be regular Windows messages.
 */
#undef INTERFACE
#define INTERFACE   IRichEditOle

DECLARE_INTERFACE_(IRichEditOle, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * lplpObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IRichEditOle methods ***
    STDMETHOD(GetClientSite) (THIS_ LPOLECLIENTSITE FAR * lplpolesite) PURE;
	STDMETHOD_(LONG,GetObjectCount) (THIS) PURE;
	STDMETHOD_(LONG,GetLinkCount) (THIS) PURE;
	STDMETHOD(GetObject) (THIS_ LONG iob, REOBJECT FAR * lpreobject,
						  DWORD dwFlags) PURE;
    STDMETHOD(InsertObject) (THIS_ REOBJECT FAR * lpreobject) PURE;
	STDMETHOD(ConvertObject) (THIS_ LONG iob, REFCLSID rclsidNew,
							  LPCSTR lpstrUserTypeNew) PURE;
	STDMETHOD(ActivateAs) (THIS_ REFCLSID rclsid, REFCLSID rclsidAs) PURE;
	STDMETHOD(SetHostNames) (THIS_ LPCSTR lpstrContainerApp,
							 LPCSTR lpstrContainerObj) PURE;
	STDMETHOD(SetLinkAvailable) (THIS_ LONG iob, BOOL fAvailable) PURE;
	STDMETHOD(SetDvaspect) (THIS_ LONG iob, DWORD dvaspect) PURE;
	STDMETHOD(HandsOffStorage) (THIS_ LONG iob) PURE;
	STDMETHOD(SaveCompleted) (THIS_ LONG iob, LPSTORAGE lpstg) PURE;
	STDMETHOD(InPlaceDeactivate) (THIS) PURE;
	STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;
	STDMETHOD(GetClipboardData) (THIS_ CHARRANGE FAR * lpchrg, DWORD reco,
									LPDATAOBJECT FAR * lplpdataobj) PURE;
	STDMETHOD(ImportDataObject) (THIS_ LPDATAOBJECT lpdataobj,
									CLIPFORMAT cf, HGLOBAL hMetaPict) PURE;
};
typedef         IRichEditOle FAR * LPRICHEDITOLE;

/*
 *	IRichEditOleCallback
 *
 *	Purpose:
 *		Interface used by the RichEdit to get OLE-related stuff from the
 *		application using RichEdit.
 */
#undef INTERFACE
#define INTERFACE   IRichEditOleCallback

DECLARE_INTERFACE_(IRichEditOleCallback, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * lplpObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IRichEditOleCallback methods ***
	STDMETHOD(GetNewStorage) (THIS_ LPSTORAGE FAR * lplpstg) PURE;
    STDMETHOD(GetInPlaceContext) (THIS_ LPOLEINPLACEFRAME FAR * lplpFrame,
								  LPOLEINPLACEUIWINDOW FAR * lplpDoc,
								  LPOLEINPLACEFRAMEINFO lpFrameInfo) PURE;
	STDMETHOD(ShowContainerUI) (THIS_ BOOL fShow) PURE;
	STDMETHOD(QueryInsertObject) (THIS_ LPCLSID lpclsid, LPSTORAGE lpstg,
									LONG cp) PURE;
	STDMETHOD(DeleteObject) (THIS_ LPOLEOBJECT lpoleobj) PURE;
	STDMETHOD(QueryAcceptData) (THIS_ LPDATAOBJECT lpdataobj,
								CLIPFORMAT FAR * lpcfFormat, DWORD reco,
								BOOL fReally, HGLOBAL hMetaPict) PURE;
	STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;
	STDMETHOD(GetClipboardData) (THIS_ CHARRANGE FAR * lpchrg, DWORD reco,
									LPDATAOBJECT FAR * lplpdataobj) PURE;
	STDMETHOD(GetDragDropEffect) (THIS_ BOOL fDrag, DWORD grfKeyState,
									LPDWORD pdwEffect) PURE;
	STDMETHOD(GetContextMenu) (THIS_ WORD seltype, LPOLEOBJECT lpoleobj,
									CHARRANGE FAR * lpchrg,
									HMENU FAR * lphmenu) PURE;
};
typedef         IRichEditOleCallback FAR * LPRICHEDITOLECALLBACK;

#ifndef MAC
// RichEdit interface GUIDs
DEFINE_GUID(IID_IRichEditOle,         0x00020D00, 0, 0, 0xC0,0,0,0,0,0,0,0x46);
DEFINE_GUID(IID_IRichEditOleCallback, 0x00020D03, 0, 0, 0xC0,0,0,0,0,0,0,0x46);
#endif // !MAC

#endif // _RICHOLE_
