#ifndef _SERVICE_INFO_H
#define _SERVICE_INFO_H

typedef struct _SERVICE_ADDRESS {
	DWORD dwAddressType;
	DWORD dwAddressFlags;
	DWORD dwAddressLength;
	DWORD dwPrincipalLength;
	BYTE *lpAddress;
	BYTE *lpPrincipal;
} SERVICE_ADDRESS;
typedef struct _SERVICE_ADDRESSES {
	DWORD dwAddressCount;
	SERVICE_ADDRESS Addresses[1];
} SERVICE_ADDRESSES, *PSERVICE_ADDRESSES, *LPSERVICE_ADDRESSES;
typedef struct _SERVICE_INFOA {
	LPGUID lpServiceType;
	LPSTR lpServiceName;
	LPSTR lpComment;
	LPSTR lpLocale;
	DWORD dwDisplayHint;
	DWORD dwVersion;
	DWORD dwTime;
	LPSTR lpMachineName;
	LPSERVICE_ADDRESSES lpServiceAddress;
	BLOB ServiceSpecificInfo;
} SERVICE_INFOA, *LPSERVICE_INFOA;
typedef struct _SERVICE_INFOW {
	LPGUID lpServiceType;
	LPWSTR lpServiceName;
	LPWSTR lpComment;
	LPWSTR lpLocale;
	DWORD dwDisplayHint;
	DWORD dwVersion;
	DWORD dwTime;
	LPWSTR lpMachineName;
	LPSERVICE_ADDRESSES lpServiceAddress;
	BLOB ServiceSpecificInfo;
} SERVICE_INFOW, *LPSERVICE_INFOW;
#endif/*SERVICE_INFO_H*/
