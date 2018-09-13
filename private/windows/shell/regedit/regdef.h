
/* Do not include this before Windows.h */

/* ASM
; DO NOT INCLUDE THIS BEFORE WINDOWS.INC
*/

#define Dereference(x)  x=x;

/*XLATOFF*/
#pragma warning (disable:4209)      // turn off redefinition warning (with vmm.h)
/*XLATON*/

#ifndef _WINREG_
//  WINREG.H uses DECLARE_HANDLE(HKEY) giving incompatible types.
typedef DWORD       HKEY;
#endif

/*XLATOFF*/
#pragma warning (default:4209)      // turn on redefinition warning (with vmm.h)
/*XLATON*/

#define MAXKEYNAME      256
        // Max length of a key name string
#define MAXVALUENAME_LENGTH MAXKEYNAME
        // Max length of a value name string
#define MAXDATA_LENGTH      16L*1024L
        // Max length of a value data item
        

#ifndef REG_SZ
#define REG_SZ      0x0001
#endif

#ifndef REG_BINARY
#define REG_BINARY  0x0003
#endif

#ifndef REG_DWORD
#define REG_DWORD       0x0004
#endif

#ifndef FALSE
#define FALSE   0
#endif
#ifndef TRUE
#define TRUE    ~FALSE
#endif

/* following equates are also defined in Windows.h. To avoid warnings
 *  we should make these equates  conditional
 */


#ifndef ERROR_SUCCESS           
#define ERROR_SUCCESS           0L
#endif

#ifndef ERROR_FILE_NOT_FOUND
#define ERROR_FILE_NOT_FOUND        2L
#endif

#ifndef ERROR_ACCESS_DENIED
#define ERROR_ACCESS_DENIED              5L
#endif

#ifndef ERROR_BADDB
#define ERROR_BADDB                      1009L
#endif

#ifndef ERROR_MORE_DATA
#define ERROR_MORE_DATA                  234L
#endif

#ifndef ERROR_BADKEY
#define ERROR_BADKEY             1010L
#endif

#ifndef ERROR_CANTOPEN
#define ERROR_CANTOPEN                   1011L
#endif

#ifndef ERROR_CANTREAD
#define ERROR_CANTREAD                   1012L
#define ERROR_CANTWRITE                  1013L
#endif

#ifndef ERROR_REGISTRY_CORRUPT
#define ERROR_REGISTRY_CORRUPT           1015L
#define ERROR_REGISTRY_IO_FAILED         1016L
#endif

#ifndef ERROR_KEY_DELETED
#define ERROR_KEY_DELETED                1018L
#endif

#ifndef ERROR_OUTOFMEMORY
#define ERROR_OUTOFMEMORY          14L
#endif

#ifndef ERROR_INVALID_PARAMETER
#define ERROR_INVALID_PARAMETER        87L
#endif

#ifndef ERROR_LOCK_FAILED
#define ERROR_LOCK_FAILED                167L
#endif

#ifndef ERROR_NO_MORE_ITEMS
#define ERROR_NO_MORE_ITEMS       259L
#endif  

// INTERNAL

#ifndef ERROR_CANTOPEN16_FILENOTFOUND32
#define ERROR_CANTOPEN16_FILENOTFOUND32 0xffff0000
#define ERROR_CANTREAD16_FILENOTFOUND32 0xffff0001
#endif

#ifndef HKEY_CLASSES_ROOT
#define HKEY_CLASSES_ROOT          ((HKEY)0x80000000)
#endif

#ifndef HKEY_CURRENT_USER
#define HKEY_CURRENT_USER              ((HKEY)0x80000001)
#endif

#ifndef HKEY_LOCAL_MACHINE
#define HKEY_LOCAL_MACHINE             ((HKEY)0x80000002)
#endif

#ifndef HKEY_USERS
#define HKEY_USERS                     ((HKEY)0x80000003)
#endif

#ifndef HKEY_PERFORMANCE_DATA
#define HKEY_PERFORMANCE_DATA          ((HKEY)0x80000004)
#endif

#ifndef HKEY_CURRENT_CONFIG
#define HKEY_CURRENT_CONFIG            ((HKEY)0x80000005)
#endif

#ifndef HKEY_DYN_DATA
#define HKEY_DYN_DATA                  ((HKEY)0x80000006)
#endif

// INTERNAL

#ifndef HKEY_PREDEF_KEYS
#define HKEY_PREDEF_KEYS    7
#endif

#define MAXREGFILES     HKEY_PREDEF_KEYS    

// sub function indices for Registry services in VMM for 16 bit callers

#define RegOpenKey_Idx      0x100
#define RegCreateKey_Idx    0x101
#define RegCloseKey_Idx     0x102
#define RegDeleteKey_Idx    0x103
#define RegSetValue_Idx     0x104
#define RegQueryValue_Idx   0x105
#define RegEnumKey_Idx      0x106
#define RegDeleteValue_Idx  0x107
#define RegEnumValue_Idx    0x108
#define RegQueryValueEx_Idx 0x109
#define RegSetValueEx_Idx   0x10A
#define RegFlushKey_Idx     0x10B
#define RegLoadKey_Idx      0x10C
#define RegUnLoadKey_Idx    0x10D
#define RegSaveKey_Idx      0x10E
#define RegRestore_Idx      0x10F
#define RegRemapPreDefKey_Idx   0x110

// Data structure passed to SYSDM.CPL DMRegistryError function
//  After UI, the function is to call
//  RegRestore(DWORD iLevel, LPREGQRSTR lpRgRstr)
//

struct Reg_Query_Restore_s {
DWORD   dwRQR_Err;      // Error code
DWORD   hRQR_RootKey;       // Root key for file
DWORD   dwRQR_Reference;    // Reference data for RegRestore
TCHAR   szRQR_SubKey[MAXKEYNAME]; // Subkey (for hives) or NULL string
TCHAR   szRQR_FileName[MAX_PATH]; // File name of bad file
};
typedef struct Reg_Query_Restore_s REGQRSTR;
typedef REGQRSTR FAR * LPREGQRSTR;


// END INTERNAL
