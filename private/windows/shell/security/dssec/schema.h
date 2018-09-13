//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       schema.h
//
//--------------------------------------------------------------------------

#ifndef _SCHEMA_CACHE_H_
#define _SCHEMA_CACHE_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// Generic Mapping for DS, adapted from \nt\private\ds\src\inc\permit.h
//
#define DS_GENERIC_READ         ((STANDARD_RIGHTS_READ)     | \
                                 (ACTRL_DS_LIST)            | \
                                 (ACTRL_DS_READ_PROP)       | \
                                 (ACTRL_DS_LIST_OBJECT))

#define DS_GENERIC_EXECUTE      ((STANDARD_RIGHTS_EXECUTE)  | \
                                 (ACTRL_DS_LIST))

// Note, STANDARD_RIGHTS_WRITE is specifically NOT included here
#define DS_GENERIC_WRITE        ((ACTRL_DS_SELF)            | \
                                 (ACTRL_DS_WRITE_PROP))

#define DS_GENERIC_ALL          ((STANDARD_RIGHTS_REQUIRED) | \
                                 (ACTRL_DS_CREATE_CHILD)    | \
                                 (ACTRL_DS_DELETE_CHILD)    | \
                                 (ACTRL_DS_DELETE_TREE)     | \
                                 (ACTRL_DS_READ_PROP)       | \
                                 (ACTRL_DS_WRITE_PROP)      | \
                                 (ACTRL_DS_LIST)            | \
                                 (ACTRL_DS_LIST_OBJECT)     | \
                                 (ACTRL_DS_CONTROL_ACCESS)  | \
                                 (ACTRL_DS_SELF))

//
// Flags for SchemaCache_Get****ID
//
#define IDC_CLASS_NO_CREATE     0x00000001
#define IDC_CLASS_NO_DELETE     0x00000002
#define IDC_CLASS_NO_INHERIT    0x00000004
#define IDC_PROP_NO_READ        IDC_CLASS_NO_CREATE
#define IDC_PROP_NO_WRITE       IDC_CLASS_NO_DELETE

#define IDC_CLASS_NONE          (IDC_CLASS_NO_CREATE | IDC_CLASS_NO_DELETE | IDC_CLASS_NO_INHERIT)
#define IDC_PROP_NONE           (IDC_PROP_NO_READ | IDC_PROP_NO_WRITE)

#define SCHEMA_COMMON_PERM      0x80000000
#define SCHEMA_NO_FILTER        0x40000000
#define SCHEMA_CLASS            0x20000000


HRESULT SchemaCache_Create(LPCTSTR pszServer);
void SchemaCache_Destroy(void);

HRESULT SchemaCache_GetInheritTypes(LPCGUID pguidObjectType,
                                    DWORD dwFlags,
                                    PSI_INHERIT_TYPE *ppInheritTypes,
                                    ULONG *pcInheritTypes);
HRESULT SchemaCache_GetAccessRights(LPCGUID pguidObjectType,
                                    LPCTSTR pszClassName,   // optional (faster if provided)
                                    LPCTSTR pszSchemaPath,
                                    DWORD dwFlags,  // 0, SI_ADVANCED, or SI_ADVANCED | SI_EDIT_PROPERTIES
                                    PSI_ACCESS *ppAccesses,
                                    ULONG *pcAccesses,
                                    ULONG *piDefaultAccess);

HRESULT Schema_BindToObject(LPCTSTR pszSchemaPath,
                            LPCTSTR pszName,
                            REFIID riid,
                            LPVOID *ppv);
HRESULT Schema_GetObjectID(IADs *pObj, LPGUID pGUID);


#ifdef __cplusplus
}
#endif

#endif  // _SCHEMA_CACHE_H_
