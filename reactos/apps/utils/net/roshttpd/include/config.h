/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS HTTP Daemon
 * FILE:        include/config.h
 */
#ifndef __CONFIG_H
#define __CONFIG_H

#include <list.h>
#include <httpd.h>

// General constants
#define APP_DESCRIPTION _T("ReactOS HTTP Daemon")

// Default configuration
#define dcfgDescription     _T("Default configuration")
#define dcfgMainBase        _T("C:\\roshttpd\\")
#define dcfgHttpBase        "C:\\roshttpd\\HttpBase\\"
#define dcfgDefaultResource "index.html"
#define dcfgDefaultPort     80

class CConfig {
public:
	CConfig();
	~CConfig();
	VOID Default();
	VOID Clear();
	BOOL Load();
	BOOL Save();
	LPWSTR GetMainBase();
	VOID SetMainBase(LPWSTR lpwsMainBase);
	LPSTR GetHttpBase();
	VOID SetHttpBase(LPSTR lpsHttpBase);
	CList<LPSTR>* GetDefaultResources();
    USHORT GetPort();
    VOID SetPort(USHORT wPort);
private:
	VOID Reset();
	LPWSTR MainBase;
	LPSTR HttpBase;
	CList<LPSTR> DefaultResources;
    USHORT Port;
};
typedef CConfig* LPCConfig;

extern LPCConfig pConfiguration;
extern LPCHttpDaemonThread pDaemonThread;

#endif /* __CONFIG_H */
