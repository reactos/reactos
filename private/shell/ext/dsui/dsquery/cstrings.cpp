#include "pch.h"
#pragma hdrstop

TCHAR const c_szDsQuery[]               = TEXT("DsQuery");
TCHAR const c_szScope[]                 = TEXT("Scope");
TCHAR const c_szScopeSize[]             = TEXT("ScopeSize");
TCHAR const c_szViewMode[]              = TEXT("ViewMode");
TCHAR const c_szEnableFilter[]          = TEXT("EnableFilter");

TCHAR const c_szMsPeople[]              = TEXT("Microsoft.People");
TCHAR const c_szMsComputer[]            = TEXT("Microsoft.Computers");
TCHAR const c_szMsPrinters[]            = TEXT("Microsoft.Printers");
TCHAR const c_szMsPrintersMore[]        = TEXT("Microsoft.Printers.MoreChoices");
TCHAR const c_szMsVolume[]              = TEXT("Microsoft.Volume");
TCHAR const c_szMsContainers[]          = TEXT("Microsoft.Containers");
TCHAR const c_szMsObjects[]             = TEXT("Microsoft.Objects");
TCHAR const c_szMsPropertyWell[]        = TEXT("Microsoft.PropertyWell");

// BUGBUG: jonn removing
TCHAR const c_szMsDomainControllers[]   = TEXT("Microsoft.DomainControllers");
TCHAR const c_szMsFrsMembers[]          = TEXT("Microsoft.FRSMembers");

// These must be UNICODE as they are passed directly to ADSI

WCHAR c_szLDAP[]                        = L"LDAP:";

WCHAR c_szADsPath[]                     = L"ADsPath";
WCHAR c_szADsPathCH[]                   = L"ADsPath,{DE4874D1-FEEE-11D1-A0B0-00C04FA31A86}";

WCHAR c_szObjectClass[]                 = L"objectClass";
WCHAR c_szObjectClassCH[]               = L"objectClass,{DE4874D2-FEEE-11D1-A0B0-00C04FA31A86}";

WCHAR c_szShowInAdvancedViewOnly[]      = L"(!showInAdvancedViewOnly=TRUE)";

WCHAR c_szCN[]                          = L"cn";
WCHAR c_szName[]                        = L"name";
WCHAR c_szOwner[]                       = L"owner";
WCHAR c_szMachineRole[]                 = L"machineRole";
WCHAR c_szDescription[]                 = L"description";
WCHAR c_szUNCName[]                     = L"UNCName";
WCHAR c_szKeywords[]                    = L"Keywords";
WCHAR c_szContactName[]                 = L"contactName";
WCHAR c_szLocation[]                    = L"location";
WCHAR c_szDistinguishedName[]           = L"distinguishedName";
