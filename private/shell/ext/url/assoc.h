/*
 * assoc.h - Type association routines description.
 */


#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */


/* Global Constants
 *******************/

/* assoc.c */

extern const HKEY g_hkeyURLProtocols;
extern const HKEY g_hkeyMIMESettings;
extern CCHAR g_cszURLProtocol[];
extern CCHAR g_cszContentType[];
extern CCHAR g_cszExtension[];


/* Prototypes
 *************/

/* assoc.c */

extern BOOL RegisterMIMETypeForExtension(PCSTR pcszExtension, PCSTR pcszMIMEContentType);
extern BOOL UnregisterMIMETypeForExtension(PCSTR pcszExtension);
extern BOOL RegisterExtensionForMIMEType(PCSTR pcszExtension, PCSTR pcszMIMEContentType);
extern BOOL UnregisterExtensionForMIMEType(PCSTR pcszMIMEContentType);
extern BOOL RegisterMIMEAssociation(PCSTR pcszFile, PCSTR pcszMIMEContentType);
extern BOOL RegisterURLAssociation(PCSTR pcszProtocol, PCSTR pcszApp);
extern HRESULT MyMIMEAssociationDialog(HWND hwndParent, DWORD dwInFlags, PCSTR pcszFile, PCSTR pcszMIMEContentType, PSTR pszAppBuf, UINT ucAppBufLen);
extern HRESULT MyURLAssociationDialog(HWND hwndParent, DWORD dwInFlags, PCSTR pcszFile, PCSTR pcszURL, PSTR pszAppBuf, UINT ucAppBufLen);


#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */

