/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    pip.h

Abstract:

    This file is the include file for the Program Information
    Provider stuff.

Author:

    Dave Hastings (daveh) creation-date-01-May-1997

Notes:

    This should move to a more sensible place.  It doesn't
    really belong here.

Revision History:


--*/

typedef struct _PipApplicationProperties {
    ULONG ProductNameSize;
    PWSTR ProductName;
    ULONG ProductVersionSize;
    PWSTR ProductVersion;
    ULONG PublisherSize;
    PWSTR Publisher;
    ULONG ProductIDSize;
    PWSTR ProductID;
    ULONG RegisteredOwnerSize;
    PWSTR RegisteredOwner;
    ULONG RegisteredCompanySize;
    PWSTR RegisteredCompany;
    ULONG LanguageSize;
    PWSTR Language;
    ULONG SupportUrlSize;
    PWSTR SupportUrl;
    ULONG SupportTelephoneSize;
    PWSTR SupportTelephone;
    ULONG HelpFileSize;
    PWSTR HelpFile;
    ULONG InstallLocationSize;
    PWSTR InstallLocation;
    ULONG InstallSourceSize;
    PWSTR InstallSource;
    ULONG RequiredByPolicySize;
    PWSTR RequiredByPolicy;
    ULONG AdministrativeContactSize;
    PWSTR AdministrativeContact;
} PIPAPPLICATIONPROPERTIES, *PPIPAPPLICATIONPROPERTIES;



typedef BOOL (*PIPUNINSTALLFUNCTION)(HANDLE);
typedef BOOL (*PIPMODIFYFUNCTION)(HANDLE);
typedef BOOL (*PIPREPAIRFUNCTION)(HANDLE, DWORD);
typedef BOOL (*PIPUPGRADEFUNCTION)(HANDLE);
typedef VOID (*PIPFREEHANDLEFUNCTION)(HANDLE);
typedef VOID (*PIPPORPERTIESFUNCTION)(HANDLE, PPIPAPPLICATIONPROPERTIES);

typedef struct _PipActions {
    PIPUNINSTALLFUNCTION Uninstall;
    PIPMODIFYFUNCTION Modify;
    PIPREPAIRFUNCTION Repair;
    PIPUPGRADEFUNCTION Upgrade;
    PIPFREEHANDLEFUNCTION FreeHandle;
    PIPPORPERTIESFUNCTION GetProperties;
} PIPACTIONS, *PPIPACTIONS;


typedef struct _PipApplicationInformation {
	ULONG Size;
	ULONG ProductNameSize;
	PWSTR ProductName;
	ULONG LogoLevelSize;
	PWSTR LogoLevel;
	ULONG InstalledVersionSize;
	PWSTR InstalledVersion;
    HANDLE h;
} PIPAPPLICATIONINFORMATION, *PPIPAPPLICATIONINFORMATION;

HANDLE
PipInitialize(
    VOID
    );

ULONG
PipFindNextProduct(
	HANDLE IN Handle,
	PULONG OUT Capabilities,
	PPIPACTIONS OUT Actions,
	PPIPAPPLICATIONINFORMATION IN OUT ApplicationInformation
	);

BOOL
PipUninitialize(
	HANDLE IN Handle
	);

#define PIP_UNINSTALL   0x00000001
#define PIP_MODIFY      0x00000002
#define PIP_REPAIR      0x00000004
#define PIP_UPGRADE     0x00000008
#define PIP_PROPERTIES  0x00000010

#define PIP_REPAIR_REPAIR       0x00000001
#define PIP_REPAIR_REINSTALL    0x00000002

HANDLE
PipDarwinInitialize(
    VOID
    );

ULONG
PipDarwinFindNextProduct(
	HANDLE IN Handle,
	PULONG OUT Capabilities,
	PPIPACTIONS OUT Actions,
	PPIPAPPLICATIONINFORMATION IN OUT ApplicationInformation
	);

BOOL
PipDarwinUninitialize(
	HANDLE IN Handle
	);
