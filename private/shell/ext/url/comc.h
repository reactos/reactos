/*
 * comc.h - Shared routines description.
 */


/* Types
 ********/

/* input flags to MyExecute() */

typedef enum myexecute_in_flags
{
   /*
    * Adds double quotes around the given argument string on the generated
    * command line if the argument string contains any white space.
    */

   ME_IFL_QUOTE_ARGS    = 0x0001,

   /* flag combinations */

   ALL_ME_IN_FLAGS      = ME_IFL_QUOTE_ARGS
}
MYEXECUTE_IN_FLAGS;


/* Global Variables
 *******************/

/* comc.c */

extern const char g_cszWhiteSpace[];
extern const char g_cszSlashes[];
extern const char g_cszPathSeparators[];
extern const char g_cszEditFlags[];


/* Prototypes
 *************/

/* comc.c */

extern BOOL DataCopy(PCBYTE pcbyteSrc, ULONG ulcbLen, PBYTE *ppbyteDest);
extern BOOL StringCopy(PCSTR pcszSrc, PSTR *ppszCopy);
extern BOOL GetMIMETypeSubKey(PCSTR pcszMIMEType, PSTR pszSubKeyBuf, UINT ucSubKeyBufLen);
extern BOOL GetMIMEValue(PCSTR pcszMIMEType, PCSTR pcszValue, PDWORD pdwValueType, PBYTE pbyteValueBuf, PDWORD pdwcbValueBufLen);
extern BOOL GetFileTypeValue(PCSTR pcszExtension, PCSTR pcszSubKey, PCSTR pcszValue, PDWORD pdwValueType, PBYTE pbyteValueBuf, PDWORD pdwcbValueBufLen);
extern BOOL GetMIMEFileTypeValue(PCSTR pcszMIMEType, PCSTR pcszSubKey, PCSTR pcszValue, PDWORD pdwValueType, PBYTE pbyteValueBuf, PDWORD pdwcbValueBufLen);
extern BOOL MIME_IsExternalHandlerRegistered(PCSTR pcszMIMEType);
extern BOOL MIME_GetExtension(PCSTR pcszMIMEType, PSTR pszExtensionBuf, UINT ucExtensionBufLen);
extern BOOL MIME_GetMIMETypeFromExtension(PCSTR pcszPath, PSTR pszMIMETypeBuf, UINT ucMIMETypeBufLen);
extern void CatPath(PSTR pszPath, PCSTR pcszSubPath);
extern void MyLStrCpyN(PSTR pszDest, PCSTR pcszSrc, int ncb);
extern COMPARISONRESULT MapIntToComparisonResult(int nResult);
extern void TrimWhiteSpace(PSTR pszTrimMe);
extern void TrimSlashes(PSTR pszTrimMe);
extern void TrimString(PSTR pszTrimMe, PCSTR pszTrimChars);
extern PCSTR ExtractFileName(PCSTR pcszPathName);
extern PCSTR ExtractExtension(PCSTR pcszName);
extern LONG SetRegKeyValue(HKEY hkeyParent, PCSTR pcszSubKey, PCSTR pcszValue, DWORD dwType, PCBYTE lpcbyte, DWORD dwcb);;
extern LONG GetRegKeyValue(HKEY hkeyParent, PCSTR pcszSubKey, PCSTR pcszValue, PDWORD pdwValueType, PBYTE pbyteBuf, PDWORD pdwcbBufLen);
extern LONG GetRegKeyStringValue(HKEY hkeyParent, PCSTR pcszSubKey, PCSTR pcszValue, PSTR pszBuf, PDWORD pdwcbBufLen);
extern LONG GetDefaultRegKeyValue(HKEY hkeyParent, PCSTR pcszSubKey, PSTR pszBuf, PDWORD pdwcbBufLen);
extern HRESULT FullyQualifyPath(PCSTR pcszPath, PSTR pszFullyQualifiedPath, UINT ucFullyQualifiedPathBufLen);
extern HRESULT MyExecute(PCSTR pcszApp, PCSTR pcszArgs, DWORD dwInFlags);
extern BOOL GetClassDefaultVerb(PCSTR pcszClass, PSTR pszDefaultVerbBuf, UINT ucDefaultVerbBufLen);
extern BOOL GetPathDefaultVerb(PCSTR pcszPath, PSTR pszDefaultVerbBuf, UINT ucDefaultVerbBufLen);
extern BOOL ClassIsSafeToOpen(PCSTR pcszClass);
extern BOOL SetClassEditFlags(PCSTR pcszClass, DWORD dwFlags, BOOL bSet);

#ifdef DEBUG

extern BOOL IsFullPath(PCSTR pcszPath);

#endif   /* DEBUG */
