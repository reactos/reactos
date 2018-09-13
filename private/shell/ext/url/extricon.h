/*
 * extricon.h - IExtractIcon implementation description.
 */


#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */


/* Global Constants
 *******************/

/* extricon.cpp */

extern const char g_cszURLProtocolHandlersKey[];
extern const char g_cszURLDefaultIconKey[];
extern const HKEY g_hkeyURLSettings;


/* Prototypes
 *************/

/* extricon.cpp */

extern int StringToInt(PCSTR pcsz);
extern BOOL IsWhiteSpace(char ch);
extern BOOL AnyMeat(PCSTR pcsz);
extern HRESULT CopyURLProtocol(PCSTR pcszURL, PSTR *ppszProtocol);
extern HRESULT CopyURLSuffix(PCSTR pcszURL, PSTR *ppszSuffix);
extern HRESULT GetProtocolKey(PCSTR pcszProtocol, PCSTR pcszSubKey, PSTR *pszKey);
extern HRESULT GetURLKey(PCSTR pcszURL, PCSTR pcszSubKey, PSTR *pszKey);


#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */

