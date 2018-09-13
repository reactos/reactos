/*
 * persist.hpp - IPersist, IPersistFile, and IPersistStream implementation
 *               description.
 */


/* Global Constants
 *******************/

// persist.cpp

extern const UINT g_ucMaxURLLen;
extern const char g_cszURLLeftDelimiter[];
extern const char g_cszURLRightDelimiter[];
extern const char g_cszURLPrefix[];
extern const UINT g_ucbURLPrefixLen;
extern const char g_cszURLExt[];
extern const char g_cszURLDefaultFileNamePrompt[];
extern const char g_cszCRLF[];


/* Prototypes
 *************/

// persist.cpp

extern HRESULT UnicodeToANSI(LPCOLESTR pcwszUnicode, PSTR *ppszANSI);
extern HRESULT ANSIToUnicode(PCSTR pcszANSI, LPOLESTR *ppwszUnicode);

