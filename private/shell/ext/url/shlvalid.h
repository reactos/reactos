/*
 * shlvalid.h - Shell validation functions description.
 */


#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */


/* Prototypes
 *************/

/* shlvalid.c */

#ifdef DEBUG

/* interfaces */

extern BOOL IsValidPCIExtractIcon(PCIExtractIcon pciei);
extern BOOL IsValidPCINewShortcutHook(PCINewShortcutHook pcinshhk);
extern BOOL IsValidPCIShellExecuteHook(PCIShellExecuteHook pciseh);
extern BOOL IsValidPCIShellExtInit(PCIShellExtInit pcisei);
extern BOOL IsValidPCIShellLink(PCIShellLink pcisl);
extern BOOL IsValidPCIShellPropSheetExt(PCIShellPropSheetExt pcispse);

/* structures */

extern BOOL IsValidPCITEMIDLIST(PCITEMIDLIST pcidl);
extern BOOL IsValidPCPROPSHEETPAGE(PCPROPSHEETPAGE pcpsp);
extern BOOL IsValidPCSHELLEXECUTEINFO(PCSHELLEXECUTEINFO pcei);

#endif   /* DEBUG */


#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */

