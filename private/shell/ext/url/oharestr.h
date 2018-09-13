//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************

//
//	OHARESTR.H - string defines for O'Hare components
//			
//

//	HISTORY:
//	
//	3/10/95		jeremys		Created.
//


#ifndef _OHARESTR_H_
#define _OHARESTR_H_

// path to RNA values (under HKEY_CURRENT_USER)
#define REGSTR_PATH_REMOTEACCESS	"RemoteAccess"

// string value under HKCU\REGSTR_PATH_REMOTEACCESS that contains name of
// connectoid used to connect to internet
#define REGSTR_VAL_INTERNETPROFILE	"InternetProfile"
#define REGSTR_VAL_BKUPINTERNETPROFILE	"BackupInternetProfile"

// path to global internet settings (also under HKEY_CURRENT_USER)
#define REGSTR_PATH_INTERNET_SETTINGS REGSTR_PATH_SETUP "\\Internet Settings"

// values under HKCY\REGSTR_PATH_INTERNET_SETTINGS

// 4-byte REG_BINARY, autodialing is enabled if this value is present and
// non-zero, disabled otherwise
#define REGSTR_VAL_ENABLEAUTODIAL 		"EnableAutodial"
#define REGSTR_VAL_ENABLEAUTODISCONNECT	"EnableAutodisconnect"
#define REGSTR_VAL_ENABLESECURITYCHECK	"EnableSecurityCheck"

// 4-byte REG_BINARY containing number of minutes of idle time to allow
// before autodisconnect.  Autodisconnect is disabled if this value is zero
// or not present.
#define REGSTR_VAL_DISCONNECTIDLETIME	"DisconnectIdleTime"

// class name for window to receive Winsock activity messages
#define AUTODIAL_MONITOR_CLASS_NAME	"MS_AutodialMonitor"

// name of connectoid-specific autodial handler dll and function
#define REGSTR_VAL_AUTODIALDLLNAME		"AutodialDllName"
#define REGSTR_VAL_AUTODIALFCNNAME		"AutodialFcnName"

// proxy settings
#define REGSTR_VAL_PROXYENABLE          "ProxyEnable"
#define REGSTR_VAL_PROXYSERVER          "ProxyServer"
#define REGSTR_VAL_PROXYOVERRIDE        "ProxyOverride"

// access medium (modem, LAN, [etc?])
#define REGSTR_VAL_ACCESSMEDIUM			"AccessMedium"

// access type (MSN, other)
#define REGSTR_VAL_ACCESSTYPE			"AccessType"

#endif // _OHARESTR_H_
