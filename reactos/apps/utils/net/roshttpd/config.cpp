/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS HTTP Daemon
 * FILE:        config.cpp
 * PURPOSE:     Daemon configuration
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH  01/09/2000 Created
 */
#include <new>
#include <stdlib.h>
#include <string.h>
#include <config.h>

using namespace std;

LPCConfig pConfiguration;
LPCHttpDaemonThread pDaemonThread;

// Default constructor
CConfig::CConfig()
{
	Reset();
}

// Default destructor
CConfig::~CConfig()
{
	Clear();
}

// Clear configuration
void CConfig::Reset()
{
	MainBase = NULL;
	HttpBase = NULL;
	DefaultResources.RemoveAll();
}

// Create default configuration. Can throw bad_alloc
void CConfig::Default()
{
	Clear();
	MainBase = (LPWSTR)_wcsdup(dcfgMainBase);
	HttpBase = _strdup(dcfgHttpBase);

	LPSTR lpsStr;
	try {
		lpsStr = _strdup(dcfgDefaultResource);
		DefaultResources.Insert(lpsStr);
	} catch (bad_alloc e) {
		free((void *)lpsStr);
		Clear();
		throw;
	}

    Port = dcfgDefaultPort;
}

// Clear configuration
void CConfig::Clear()
{
	if (MainBase != NULL)
		free((void *)MainBase);
	if (HttpBase != NULL)
		free((void *)HttpBase);

	// Free memory for all strings
	CIterator<LPSTR> *i = DefaultResources.CreateIterator();
	for (i->First(); !i->IsDone(); i->Next())
		free((void *)i->CurrentItem());
	delete i;

	Reset();
}

// Load configuration
BOOL CConfig::Load()
{
    Default();
	return TRUE;
}

// Save configuration
BOOL CConfig::Save()
{
    return TRUE;
}

// Return MainBase
LPWSTR CConfig::GetMainBase()
{
	return MainBase;
}

// Set MainBase
void CConfig::SetMainBase(LPWSTR lpwsMainBase)
{
	MainBase = lpwsMainBase;
}

// Return HttpBase
LPSTR CConfig::GetHttpBase()
{
	return HttpBase;
}

// Set HttpBase
void CConfig::SetHttpBase(LPSTR lpsHttpBase)
{
	HttpBase = lpsHttpBase;
}

// Return DefaultResources
CList<LPSTR>* CConfig::GetDefaultResources()
{
	return &DefaultResources;
}

// Return bound port
USHORT CConfig::GetPort()
{
    return Port;
}

// Set port
VOID CConfig::SetPort(USHORT wPort)
{
    Port = wPort;
}
