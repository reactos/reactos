/*
 * Registry Association Management
 *
 * HTREGMNG.H
 *
 * Copyright (c) 1995 Microsoft Inc.
 *
 */

#ifndef HTREGMNG_H
#define HTREGMNG_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Registry Management Structures
 *
 * We need a way to specify a set of registry entries to
 * represent an association.   We can then test and
 * set the registry appropriately to restore associations
 * as needed
 *
 */

typedef enum 
{ 
    RC_ADD, 
    RC_RUNDLL,
    RC_DEL,              // Remove key
    RC_CALLBACK
} REGCMD;


// Flags for RegEntry

#define REF_NORMAL      0x00000000      // Required and forcefully set
#define REF_NOTNEEDED   0x00000001      // Ignored during checks
#define REF_IFEMPTY     0x00000002      // Set only if value/key is empty
#define REF_DONTINTRUDE 0x00000004      // Don't intrude at setup time
#define REF_NUKE        0x00000008      // Remove a key, regardless of the subkeys/values
#define REF_PRUNE       0x00000010      // Walk up this path and remove empty keys
#define REF_EDITFLAGS   0x00000020      // Remove edit flags only if the rest of the tree is empty


// NOTE: these structures are deliberately CHAR, not TCHAR, so we don't
// have to mess with the TEXT macro in all the tables.

typedef struct _RegEntry {
    REGCMD  regcmd;         // Special Handling
    DWORD   dwFlags;        // REF_* 
    HKEY    hkeyRoot;       // Root key
    LPCSTR  pszKey;         // Key Name
    LPCSTR  pszValName;     // Value Name
    DWORD   dwType;         // Value Type
    union 
    {
        LPARAM  lParam;     // lParam
        DWORD   dwSize;     // Value Size (in bytes)
    }DUMMYUNIONNAME;
    VOID const * pvValue;   // Value
} RegEntry;

typedef RegEntry RegList[];

typedef struct _RegSet {
    DWORD       cre;       // Count of entries
    const RegEntry * pre;
} RegSet;


#define IEA_NORMAL          0x00000001 // Only install IE assoc. if IE is currently owner.
#define IEA_FORCEIE         0x00000002 // Force IE to take over associations

HRESULT InstallIEAssociations(DWORD dwFlags);   // IEA_* flags

HRESULT UninstallPlatformRegItems(BOOL bIntegrated);
void    UninstallCurrentPlatformRegItems();
BOOL    IsCheckAssociationsOn();
void    SetCheckAssociations( BOOL );
BOOL    GetIEPath(LPSTR szPath, DWORD cch);
BOOL    IsIEDefaultBrowser(void);
BOOL IsIEDefaultBrowserQuick(void);
HRESULT ResetWebSettings(HWND hwnd, BOOL *pfChangedHomePage);

extern const TCHAR c_szCLSID[];

#ifdef __cplusplus
};
#endif

#endif /* HTREGMNG_H */
