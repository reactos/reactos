#ifndef _SHALIAS_H_
#define _SHALIAS_H_

// Editing modes
#define ADD_ALIAS   0x01
#define EDIT_ALIAS  0x02

#define LocalRealloc(a, b) LocalReAlloc(a, b, LMEM_MOVEABLE)

class CAlias;

typedef struct tagALIASINFO
{
    HWND hDlg;
    int  mode;
    HDPA aliasList;
    HDPA aliasDelList;
    BOOL fInternalChange;
    BOOL fChanged;
} ALIASINFO, * LPALIASINFO;

#define ALIASLIST_COLUMNS 2  

typedef struct tagALIASITEM 
{ 
	LPSTR aCols[ALIASLIST_COLUMNS]; 
} ALIASITEM; 

typedef struct tagALIASEDITINFO
{
    HDPA aliasList;
    CAlias * alias;
    HWND   hWnd;
    DWORD  dwFlags;
} ALIASEDITINFO, *LPALIASEDITINFO;

#define ALIASINFO_FROM_HDLG( hDlg ) \
    ((LPALIASINFO)GetWindowLong(hDlg, DWL_USER)) \

TCHAR *  EatSpaces( TCHAR * str );
TCHAR *  ChopSpaces( TCHAR * str );
TCHAR *  DuplicateString( TCHAR * str );

// CAlias - object representing one alias.
class CAlias
{
public:
    LPTSTR  m_alias;
    LPTSTR  m_szurl;
    BOOL    m_fDirty;
    CAlias( LPTSTR name );
   ~CAlias();
    
    // Operations defined for Asscociation

    Load();
    Save(); 
    Delete();

#ifdef DEBUG
    Print();
#endif
};

// Some Helper Function Prototypes
BOOL     FAR PASCAL InitAliasDialog(HWND hDlg, CAlias * current, BOOL fFullInit );
CAlias * GetCurrentAlias( HWND hDlg );

#define MAX_ALIAS_LENGTH 256

STDAPI_(BOOL) LoadAliases( HDPA aliasListIn );
STDAPI_(BOOL) SaveAliases( HDPA aliasListIn );
STDAPI_(BOOL) FreeAliases( HDPA aliasListIn );

STDAPI_(LPCTSTR) GetAliasName( CAlias * ptr );
STDAPI_(LPCTSTR) GetAliasUrl( CAlias * ptr );
STDAPI_(LPVOID)  CreateAlias( LPTSTR str );
STDAPI_(VOID)    DestroyAlias( CAlias * ptr );
STDAPI_(BOOL)    SetAliasInfo( CAlias * ptr, TCHAR * alias, TCHAR * url );

#ifdef UNICODE
// TODO :
#define FindAliasIndex FindAliasIndexW
#define FindAliasByURL FindAliasByURLW
#define AddAliasToList AddAliasToListW
#define GetURLForAlias GetURLForAliasW
#else
#define FindAliasIndex FindAliasIndexA
#define FindAliasByURL FindAliasByURLA
#define AddAliasToList AddAliasToListA
#define GetURLForAlias GetURLForAliasA
#endif

STDAPI_(BOOL)  GetURLForAliasW(HDPA  aliasListIn, LPWSTR alias, LPWSTR szurl, int cchUrl );
STDAPI_(BOOL)  AddAliasToListW(HDPA  aliasListIn, LPWSTR alias, LPWSTR szurl, HWND hwnd);
STDAPI_(BOOL)  FindAliasByURLW(HDPA  aliasListIn, LPWSTR szurl, LPWSTR alias, INT cchAlias);
STDAPI_(INT)   FindAliasIndexW(HDPA  aliasListIn, LPWSTR alias);

STDAPI_(BOOL)  GetURLForAliasA(HDPA  aliasListIn, LPSTR alias, LPSTR szurl, int cchUrl );
STDAPI_(BOOL)  AddAliasToListA(HDPA  aliasListIn, LPSTR alias, LPSTR szurl, HWND hwnd);
STDAPI_(BOOL)  FindAliasByURLA(HDPA  aliasListIn, LPSTR szurl, LPSTR alias, INT cchAlias);
STDAPI_(INT)   FindAliasIndexA(HDPA  aliasListIn, LPSTR alias);

#endif 

