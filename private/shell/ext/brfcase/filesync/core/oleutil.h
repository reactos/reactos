/*
 * oleutil.h - OLE utility functions description.
 */


/* Macros
 *********/

/* interface pointer to class pointer conversion macros */

#define IfaceOffset(class, iface)            ((UINT_PTR)&(((class *)0)->iface))
#define ClassFromIface(class, iface, piface) ((class *)(((PBYTE)piface) - IfaceOffset(class, iface)))

/* macro wrappers for CompareGUIDs() */

#define CompareClassIDs(pcclsid1, pcclsid2)  CompareGUIDs(pcclsid1, pcclsid2)
#define CompareInterfaceIDs(pciid1, pciid2)  CompareGUIDs(pciid1, pciid2)


/* Types
 ********/

/* interfaces */

DECLARE_STANDARD_TYPES(INotifyReplica);
DECLARE_STANDARD_TYPES(IReconcileInitiator);
DECLARE_STANDARD_TYPES(IReconcilableObject);
DECLARE_STANDARD_TYPES(IBriefcaseInitiator);


/* Prototypes
 *************/

/* oleutil.c */

extern HRESULT GetClassFileByExtension(LPCTSTR, PCLSID);
extern HRESULT GetReconcilerClassID(LPCTSTR, PCLSID);
extern HRESULT GetCopyHandlerClassID(LPCTSTR, PCLSID);
extern HRESULT GetReplicaNotificationClassID(LPCTSTR, PCLSID);
extern COMPARISONRESULT CompareGUIDs(PCGUID, PCGUID);
extern TWINRESULT TranslateHRESULTToTWINRESULT(HRESULT);

#ifdef DEBUG

extern BOOL IsValidPCINotifyReplica(PCINotifyReplica);
extern BOOL IsValidPCIReconcileInitiator(PCIReconcileInitiator);

#endif
