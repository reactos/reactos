/*
 * objsel.h
 *
 * Object Picker Dialog
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef __OBJSEL_H_
#define __OBJSEL_H_

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_GUID(CLSID_DsObjectPicker, 0x17d6ccd8, 0x3b7b, 0x11d2, 0x00b9, 0xe0,0x00,0xc0,0x4f,0xd8,0xdb,0xf7);
DEFINE_GUID(IID_IDsObjectPicker, 0x0c87e64e, 0x3b7a, 0x11d2, 0x00b9, 0xe0,0x00,0xc0,0x4f,0xd8,0xdb,0xf7);

#define CFSTR_DSOP_DS_SELECTION_LIST    TEXT("CFSTR_DSOP_DS_SELECTION_LIST")

/* up-level scope filters in the DSOP_UPLEVEL_FILTER_FLAGS structure */
#define DSOP_FILTER_INCLUDE_ADVANCED_VIEW       (0x1)
#define DSOP_FILTER_USERS       (0x2)
#define DSOP_FILTER_BUILTIN_GROUPS      (0x4)
#define DSOP_FILTER_WELL_KNOWN_PRINCIPALS       (0x8)
#define DSOP_FILTER_UNIVERSAL_GROUPS_DL (0x10)
#define DSOP_FILTER_UNIVERSAL_GROUPS_SE (0x20)
#define DSOP_FILTER_GLOBAL_GROUPS_DL    (0x40)
#define DSOP_FILTER_GLOBAL_GROUPS_SE    (0x80)
#define DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL      (0x100)
#define DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE      (0x200)
#define DSOP_FILTER_CONTACTS    (0x400)
#define DSOP_FILTER_COMPUTERS   (0x800)

typedef struct _DSOP_UPLEVEL_FILTER_FLAGS
{
    ULONG flBothModes;
    ULONG flMixedModeOnly;
    ULONG flNativeModeOnly;
} DSOP_UPLEVEL_FILTER_FLAGS, *PDSOP_UPLEVEL_FILTER_FLAGS;

/* down-level scope filters in the DSOP_FILTER_FLAGS structure */
#define DSOP_DOWNLEVEL_FILTER_USERS     (0x80000001)
#define DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS      (0x80000002)
#define DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS     (0x80000004)
#define DSOP_DOWNLEVEL_FILTER_COMPUTERS (0x80000008)
#define DSOP_DOWNLEVEL_FILTER_WORLD     (0x80000010)
#define DSOP_DOWNLEVEL_FILTER_AUTHENTICATED_USER        (0x80000020)
#define DSOP_DOWNLEVEL_FILTER_ANONYMOUS (0x80000040)
#define DSOP_DOWNLEVEL_FILTER_BATCH     (0x80000080)
#define DSOP_DOWNLEVEL_FILTER_CREATOR_OWNER     (0x80000100)
#define DSOP_DOWNLEVEL_FILTER_CREATOR_GROUP     (0x80000200)
#define DSOP_DOWNLEVEL_FILTER_DIALUP    (0x80000400)
#define DSOP_DOWNLEVEL_FILTER_INTERACTIVE       (0x80000800)
#define DSOP_DOWNLEVEL_FILTER_NETWORK   (0x80001000)
#define DSOP_DOWNLEVEL_FILTER_SERVICE   (0x80002000)
#define DSOP_DOWNLEVEL_FILTER_SYSTEM    (0x80004000)
#define DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS    (0x80008000)
#define DSOP_DOWNLEVEL_FILTER_TERMINAL_SERVER   (0x80010000)
#define DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS        (0x80020000)
#define DSOP_DOWNLEVEL_FILTER_LOCAL_SERVICE     (0x80040000)
#define DSOP_DOWNLEVEL_FILTER_NETWORK_SERVICE   (0x80080000)
#define DSOP_DOWNLEVEL_FILTER_REMOTE_LOGON      (0x80100000)

typedef struct _DSOP_FILTER_FLAGS
{
    DSOP_UPLEVEL_FILTER_FLAGS Uplevel;
    ULONG flDownlevel;
} DSOP_FILTER_FLAGS, *PDSOP_FILTER_FLAGS;

/* ADsPath format flags in the DSOP_SCOPE_INIT_INFO structure */
#define DSOP_SCOPE_FLAG_STARTING_SCOPE  (0x1)
#define DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT     (0x2)
#define DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP      (0x4)
#define DSOP_SCOPE_FLAG_WANT_PROVIDER_GC        (0x8)
#define DSOP_SCOPE_FLAG_WANT_SID_PATH   (0x10)
#define DSOP_SCOPE_FLAG_WANT_DOWNLEVEL_BUILTIN_PATH     (0x20)
#define DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS    (0x40)
#define DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS   (0x80)
#define DSOP_SCOPE_FLAG_DEFAULT_FILTER_COMPUTERS        (0x100)
#define DSOP_SCOPE_FLAG_DEFAULT_FILTER_CONTACTS (0x200)

typedef struct _DSOP_SCOPE_INIT_INFO
{
    ULONG cbSize;
    ULONG flType;
    ULONG flScope;
    DSOP_FILTER_FLAGS FilterFlags;
    PCWSTR pwzDcName;
    PCWSTR pwzADsPath;
    HRESULT hr;
} DSOP_SCOPE_INIT_INFO, *PDSOP_SCOPE_INIT_INFO;
typedef const DSOP_SCOPE_INIT_INFO *PCDSOP_SCOPE_INIT_INFO;

/* object picker options in the DSOP_INIT_INFO structure */
#define DSOP_FLAG_MULTISELECT   (0x1)
#define DSOP_FLAG_SKIP_TARGET_COMPUTER_DC_CHECK (0x2)

typedef struct _DSOP_INIT_INFO
{
    ULONG cbSize;
    PCWSTR pwzTargetComputer;
    ULONG cDsScopeInfos;
    PDSOP_SCOPE_INIT_INFO aDsScopeInfos;
    ULONG flOptions;
    ULONG cAttributesToFetch;
    PCWSTR *apwzAttributeNames;
} DSOP_INIT_INFO, *PDSOP_INIT_INFO;

typedef const DSOP_INIT_INFO *PCDSOP_INIT_INFO;

/* selection scope types in the DS_SELECTION structure */
#define DSOP_SCOPE_TYPE_TARGET_COMPUTER (0x1)
#define DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN   (0x2)
#define DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN (0x4)
#define DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN       (0x8)
#define DSOP_SCOPE_TYPE_GLOBAL_CATALOG  (0x10)
#define DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN (0x20)
#define DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN       (0x40)
#define DSOP_SCOPE_TYPE_WORKGROUP       (0x80)
#define DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE      (0x100)
#define DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE    (0x200)

typedef struct _DS_SELECTION
{
    PWSTR pwzName;
    PWSTR pwzADsPath;
    PWSTR pwzClass;
    PWSTR pwzUPN;
    VARIANT *pvarFetchedAttributes;
    ULONG flScopeType;
} DS_SELECTION, *PDS_SELECTION;

typedef struct _DS_SELECTION_LIST
{
    ULONG cItems;
    ULONG cFetchedAttributes;
    DS_SELECTION aDsSelection[ANYSIZE_ARRAY];
} DS_SELECTION_LIST, *PDS_SELECTION_LIST;

/*****************************************************************************
 * IDsObjectPicker interface
 */
#define INTERFACE   IDsObjectPicker
DECLARE_INTERFACE_(IDsObjectPicker,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDsObjectPicker methods ***/
    STDMETHOD(Initialize)(THIS_ PDSOP_INIT_INFO pInitInfo) PURE;
    STDMETHOD(InvokeDialog)(THIS_ HWND hwndParent, IDataObject** ppdoSelections) PURE;
};
#undef INTERFACE


#ifdef __cplusplus
}
#endif
#endif /* __OBJSEL_H_ */
