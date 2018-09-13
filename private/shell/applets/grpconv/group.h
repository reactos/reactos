//---------------------------------------------------------------------------
// 
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

#define GC_PROMPTBEFORECONVERT	0x0001
#define GC_REPORTERROR		0x0002
#define GC_OPENGROUP		0x0004
#define GC_BUILDLIST		0x0008

typedef void (CALLBACK *PFNGRPCALLBACK)(LPCTSTR lpszGroup);

BOOL Group_Convert(HWND hwnd, LPCTSTR lpszOldGroup, UINT options);
DWORD Group_ReadLastModDateTime(LPCTSTR lpszGroupFile);
void Group_WriteLastModDateTime(LPCTSTR lpszGroupFile,DWORD dwLowDateTime);
int Group_Enum(PFNGRPCALLBACK pfncb, BOOL fProgress, BOOL fModifiedOnly);
int Group_EnumNT(PFNGRPCALLBACK pfncb, BOOL fProgress, BOOL fModifiedOnly, HKEY hKeyRoot, LPCTSTR lpKey);
void Group_EnumOldGroups(PFNGRPCALLBACK pfncb, BOOL fProgress);
void AppList_WriteFile(void);
BOOL AppList_Create(void);
void AppList_Destroy(void);
void AppList_AddCurrentStuff(void);
int Group_GetLastConversionCount(void);

#define PRICF_NORMAL            0x0000
#define PRICF_ALLOWSLASH        0x0001

void PathRemoveIllegalChars(LPTSTR pszPath, int iGroupName, UINT flags);
