/*
 * valid.h - Validation functions description.
 */


/* Prototypes
 *************/

/* valid.c */

extern BOOL IsValidHWND(HWND);

#ifdef DEBUG

extern BOOL IsValidFileCreationMode(DWORD);
extern BOOL IsValidHANDLE(HANDLE);
extern BOOL IsValidHEVENT(HANDLE);
extern BOOL IsValidHFILE(HANDLE);
extern BOOL IsValidHGLOBAL(HGLOBAL);
extern BOOL IsValidHMENU(HMENU);
extern BOOL IsValidHICON(HICON);
extern BOOL IsValidHINSTANCE(HINSTANCE);
extern BOOL IsValidHKEY(HKEY);
extern BOOL IsValidHMODULE(HMODULE);
extern BOOL IsValidHPROCESS(HANDLE);
extern BOOL IsValidHTEMPLATEFILE(HANDLE);
extern BOOL IsValidIconIndex(HRESULT, PCSTR, UINT, int);
extern BOOL IsValidPCFILETIME(PCFILETIME);
extern BOOL IsValidPCPOINT(PCPOINT);
extern BOOL IsValidPCPOINTL(PCPOINTL);
extern BOOL IsValidPCSECURITY_ATTRIBUTES(PCSECURITY_ATTRIBUTES);
extern BOOL IsValidPCWIN32_FIND_DATA(PCWIN32_FIND_DATA);
extern BOOL IsValidPath(PCSTR);
extern BOOL IsValidPathResult(HRESULT, PCSTR, UINT);
extern BOOL IsValidExtension(PCSTR);
extern BOOL IsValidRegistryValueType(DWORD);
extern BOOL IsValidShowCmd(int);
extern BOOL IsValidHotkey(WORD);

#ifdef _COMPARISONRESULT_DEFINED_

extern BOOL IsValidCOMPARISONRESULT(COMPARISONRESULT);

#endif   /* _COMPARISONRESULT_DEFINED_ */

#endif   /* DEBUG */

