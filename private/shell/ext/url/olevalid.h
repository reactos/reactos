/*
 * olevalid.h - OLE validation functions description.
 */


#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */


/* Macros
 *********/

/* method validation macro */

#define IS_VALID_METHOD(piface, mthd) \
   IS_VALID_CODE_PTR((piface)->lpVtbl->mthd, mthd)


/* Prototypes
 *************/

/* olevalid.c */

#ifdef DEBUG

extern BOOL IsValidPCGUID(PCGUID);
extern BOOL IsValidPCCLSID(PCCLSID);
extern BOOL IsValidPCIID(PCIID);
extern BOOL IsValidPCDVTARGETDEVICE(PCDVTARGETDEVICE);
extern BOOL IsValidPCFORMATETC(PCFORMATETC);
extern BOOL IsValidStgMediumType(DWORD);
extern BOOL IsValidPCSTGMEDIUM(PCSTGMEDIUM);
extern BOOL IsValidREFIID(REFIID);
extern BOOL IsValidREFCLSID(REFCLSID);
extern BOOL IsValidPCINTERFACE(PCVOID);
extern BOOL IsValidPCIAdviseSink(PCIAdviseSink);
extern BOOL IsValidPCIClassFactory(PCIClassFactory);
extern BOOL IsValidPCIDataObject(PCIDataObject);
extern BOOL IsValidPCIDropSource(PCIDropSource);
extern BOOL IsValidPCIDropTarget(PCIDropTarget);
extern BOOL IsValidPCIEnumFORMATETC(PCIEnumFORMATETC);
extern BOOL IsValidPCIEnumSTATDATA(PCIEnumSTATDATA);
extern BOOL IsValidPCIMalloc(PCIMalloc);
extern BOOL IsValidPCIMoniker(PCIMoniker);
extern BOOL IsValidPCIPersist(PCIPersist);
extern BOOL IsValidPCIPersistFile(PCIPersistFile);
extern BOOL IsValidPCIPersistStorage(PCIPersistStorage);
extern BOOL IsValidPCIPersistStream(PCIPersistStream);
extern BOOL IsValidPCIStorage(PCIStorage);
extern BOOL IsValidPCIStream(PCIStream);
extern BOOL IsValidPCIUnknown(PCIUnknown);

#ifdef __INTSHCUT_H__

extern BOOL IsValidPCIUniformResourceLocator(PCIUniformResourceLocator);

#endif   /* __INTSHCUT_H__ */

#endif   /* DEBUG */


#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */

