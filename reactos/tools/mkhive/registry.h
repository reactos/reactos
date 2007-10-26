/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/registry.h
 * PURPOSE:         Registry code
 */

#ifndef __REGISTRY_H__
#define __REGISTRY_H__

typedef struct _REG_VALUE
{
  LIST_ENTRY ValueList;

  /* value name */
  ULONG NameSize;
  PCHAR Name;

  /* value data */
  ULONG DataType;
  ULONG DataSize;
  PCHAR Data;
} VALUE, *PVALUE;

typedef struct _REG_KEY
{
  LIST_ENTRY KeyList;
  LIST_ENTRY SubKeyList;
  LIST_ENTRY ValueList;

  ULONG SubKeyCount;
  ULONG ValueCount;

  ULONG NameSize;
  PWCHAR Name;

  /* default data */
  ULONG DataType;
  ULONG DataSize;
  PCHAR Data;

  /* Information on hard disk structure */
  HCELL_INDEX KeyCellOffset;
  PCM_KEY_NODE KeyCell;
  PEREGISTRY_HIVE RegistryHive;

  /* Used when linking to another key */
  struct _REG_KEY* LinkedKey;
} KEY, *FRLDRHKEY, **PFRLDRHKEY, *MEMKEY, **PMEMKEY;

#define HKEY_TO_MEMKEY(hKey) ((MEMKEY)(hKey))
#define MEMKEY_TO_HKEY(memKey) ((HKEY)(memKey))

extern EREGISTRY_HIVE DefaultHive;  /* \Registry\User\.DEFAULT */
extern EREGISTRY_HIVE SamHive;      /* \Registry\Machine\SAM */
extern EREGISTRY_HIVE SecurityHive; /* \Registry\Machine\SECURITY */
extern EREGISTRY_HIVE SoftwareHive; /* \Registry\Machine\SOFTWARE */
extern EREGISTRY_HIVE SystemHive;   /* \Registry\Machine\SYSTEM */

#define ERROR_SUCCESS                    0L
#define ERROR_UNSUCCESSFUL               1L
#define ERROR_OUTOFMEMORY                14L
#define ERROR_INVALID_PARAMETER          87L
#define ERROR_MORE_DATA                  234L
#define ERROR_NO_MORE_ITEMS              259L

#define REG_NONE 0
#define REG_SZ 1
#define REG_EXPAND_SZ 2
#define REG_BINARY 3
#define REG_DWORD 4
#define REG_DWORD_BIG_ENDIAN 5
#define REG_DWORD_LITTLE_ENDIAN 4
#define REG_LINK 6
#define REG_MULTI_SZ 7
#define REG_RESOURCE_LIST 8
#define REG_FULL_RESOURCE_DESCRIPTOR 9
#define REG_RESOURCE_REQUIREMENTS_LIST 10

LONG WINAPI
RegCreateKeyA(
	IN HKEY hKey,
	IN LPCSTR lpSubKey,
	OUT PHKEY phkResult);

LONG WINAPI
RegOpenKeyA(
	IN HKEY hKey,
	IN LPCSTR lpSubKey,
	OUT PHKEY phkResult);

LONG WINAPI
RegQueryValueExA(HKEY Key,
	      LPCSTR ValueName,
	      PULONG Reserved,
	      PULONG Type,
	      PUCHAR Data,
	      PSIZE_T DataSize);

LONG WINAPI
RegSetValueExA(
	IN HKEY hKey,
	IN LPCSTR lpValueName OPTIONAL,
	ULONG Reserved,
	IN ULONG dwType,
	IN const UCHAR* lpData,
	IN ULONG cbData);

LONG WINAPI
RegDeleteValueA(HKEY Key,
	       LPCSTR ValueName);

LONG WINAPI
RegDeleteKeyA(HKEY Key,
	     LPCSTR Name);

USHORT
RegGetSubKeyCount (HKEY Key);

ULONG
RegGetValueCount (HKEY Key);

VOID
RegInitializeRegistry(VOID);

#endif /* __REGISTRY_H__ */

/* EOF */


