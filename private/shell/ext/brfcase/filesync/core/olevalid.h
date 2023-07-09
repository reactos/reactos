/*
 * olevalid.h - OLE validation functions description.
 */


/* Prototypes
 *************/

/* olevalid.c */

#if defined(DEBUG) || defined(VSTF)

extern BOOL IsValidPCGUID(PCGUID);
extern BOOL IsValidPCCLSID(PCCLSID);
extern BOOL IsValidPCIID(PCIID);
extern BOOL IsValidREFIID(REFIID);
extern BOOL IsValidREFCLSID(REFCLSID);
extern BOOL IsValidPCInterface(PCVOID);
extern BOOL IsValidPCIClassFactory(PCIClassFactory);
extern BOOL IsValidPCIDataObject(PCIDataObject);
extern BOOL IsValidPCIMalloc(PCIMalloc);
extern BOOL IsValidPCIMoniker(PCIMoniker);
extern BOOL IsValidPCIPersist(PCIPersist);
extern BOOL IsValidPCIPersistFile(PCIPersistFile);
extern BOOL IsValidPCIPersistStorage(PCIPersistStorage);
extern BOOL IsValidPCIPersistStream(PCIPersistStream);
extern BOOL IsValidPCIStorage(PCIStorage);
extern BOOL IsValidPCIStream(PCIStream);
extern BOOL IsValidPCIUnknown(PCIUnknown);

#endif

