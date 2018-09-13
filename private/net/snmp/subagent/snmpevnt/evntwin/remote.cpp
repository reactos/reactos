
//***********************************************************************
// remote.cpp
//
// This file contains code required to expand environment variables in the
// context of a remote machine. It does this by reading the environment variables
// from the remote machine's registry and caching them here.
//
// This file also contains code required to map local paths read out of the
// remote machine's registry into UNC paths such that c:\foo will be mapped
// to \\machine\c$\foo
//
// Author: Larry A. French
//
// History:
//      19-April-1996     Larry A. French
//          Wrote it.
//
// Copyright (C) 1995, 1996 Microsoft Corporation.  All rights reserved.
//
//************************************************************************


#include "stdafx.h"
#include "remote.h"
#include "trapreg.h"
#include "regkey.h"




CEnvCache::CEnvCache()
{
}


//*****************************************************************
// CEnvCache::GetEnvironmentVars
//
// Read the system environment variables for the remote machine out
// of its registry.
//
// Parameters:
//    LPCTSTR pszMachine
//          Pointer to the remote machine's name.
//
//    CMapStringToString* pmapVars
//          This string to string map is where the environment variables
//          for the machine are returned.
//
// Returns:
//    SCODE
//          S_OK if everything was successful, otherwise E_FAIL.
//
//****************************************************************
SCODE CEnvCache::GetEnvironmentVars(LPCTSTR pszMachine, CMapStringToString* pmapVars)
{
    CRegistryKey regkey;        // SYSTEM\CurrentControlSet\Services\EventLogs
    CRegistryValue regval;

    static TCHAR* apszNames1[] = {
        _T("SourcePath"),
        _T("SystemRoot")
    };

    if (regkey.Connect(pszMachine) != ERROR_SUCCESS) {
        goto CONNECT_FAILURE;
    }


    // First pick up the values for the SourcePath and SystemRoot environment variables and anything elese in
    // apszNames1.
    LONG nEntries;
    LONG iEntry;
    if (regkey.Open(_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), KEY_READ ) == ERROR_SUCCESS) {
        nEntries = sizeof(apszNames1) / sizeof(TCHAR*);
        for (iEntry=0; iEntry<nEntries; ++iEntry) {
            if (regkey.GetValue(apszNames1[iEntry], regval)) {
                pmapVars->SetAt(apszNames1[iEntry], (LPCTSTR) regval.m_pData);
            }
        }
        regkey.Close();
    }

    // Now get the rest of the environment variables.
    if (regkey.Open(_T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), KEY_READ ) == ERROR_SUCCESS) {
        CStringArray* pasValues = regkey.EnumValues();
        if (pasValues != NULL) {
            nEntries = (LONG)pasValues->GetSize();
            for (iEntry=0; iEntry< nEntries; ++iEntry) {
                CString sValueName =  pasValues->GetAt(iEntry);
                if (regkey.GetValue(sValueName, regval)) {
                    pmapVars->SetAt(sValueName, (LPCTSTR) regval.m_pData);
                }
            }
        }
        regkey.Close();
    }
    return S_OK;

CONNECT_FAILURE:
	return E_FAIL;
}


SCODE CEnvCache::AddMachine(LPCTSTR pszMachine)
{
	CMapStringToString* pmapVars;
	if (m_mapMachine.Lookup(pszMachine, (CObject*&) pmapVars)) {
		// The machine already has an entry, so don't add another
		return E_FAIL;
	}

	pmapVars = new CMapStringToString;
	m_mapMachine.SetAt(pszMachine, pmapVars);

	SCODE sc = GetEnvironmentVars(pszMachine, pmapVars);

	return sc;
}


//******************************************************************
// CEnvCache::Lookup
//
// Lookup an environment variable on the specified machine.
//
// Parameters:
//		LPCTSTR pszMachineName
//			Pointer to the machine name string.
//
//		LPCTSTR pszName
//			Pointer to the name of the environment variable to lookup.
//
//		CString& sValue
//			This is a reference to the place where the environment varaible's
//			value is returned.
//
//
// Returns:
//		SCODE
//			S_OK if the environment variable was found.
//			E_FAIL if the environment varaible was not found.
//
//*******************************************************************
SCODE CEnvCache::Lookup(LPCTSTR pszMachineName, LPCTSTR pszName, CString& sResult)
{
	SCODE sc;
	CMapStringToString* pmapVars;
	// Get a pointer to the machine's cached map of environment variable values.
	// If the map hasn't been loaded yet, do so now and try to get its map again.
	if (!m_mapMachine.Lookup(pszMachineName, (CObject*&) pmapVars)) {
		sc = AddMachine(pszMachineName);
		if (FAILED(sc)) {
			return sc;
		}
		if (!m_mapMachine.Lookup(pszMachineName, (CObject*&) pmapVars)) {
			ASSERT(FALSE);
		}
	}

	// Look for the variable name in the environment name map
	if (pmapVars->Lookup(pszName, sResult)) {
		return S_OK;
	}
	else {
		return E_FAIL;
	}
}



//****************************************************************
// RemoteExpandEnvStrings
//
// Epand a string that may contain environment variables in the
// context of a remote machine.
//
// Parameters:
//		LPCTSTR pszComputerName
//			A pointer to the name of the remote machine.
//
//		CEnvCache& cache
//			The environment variable cache for all machines.  Note: the
//			cached values for a particular machine are loaded when there
//			is a reference to the machine.
//
//		CString& sValue
//			The string to expand.  This string is expanded in-place such
//			that on return, the string will contain the expanded values.
//
// Returns:
//		SCODE
//			S_OK if all strings were expanded
//
//******************************************************************
SCODE RemoteExpandEnvStrings(LPCTSTR pszComputerName, CEnvCache& cache, CString& sValue)
{
	SCODE sc = S_OK;
    LPCTSTR psz = sValue;
	TCHAR ch;
	CString sEnvVarName;
	CString sEnvVarValue;
	CString sResult;
	LPCTSTR pszPercent = NULL;
    while (ch = *psz++) {
        if (ch == _T('%')) {
			pszPercent = psz - 1;

			sEnvVarName = _T("");
            while (ch = *psz) {
				++psz;
                if (ch == _T('%')) {
					SCODE sc;
					sc = cache.Lookup(pszComputerName, sEnvVarName, sEnvVarValue);
					if (SUCCEEDED(sc)) {
						sResult += sEnvVarValue;
						pszPercent = NULL;
					}
					else {
						// If any environment variable is not found, then fail.
						sc = E_FAIL;
					}
					break;
                }
                if (iswspace(ch) || ch==_T(';')) {
                    break;
                }
                sEnvVarName += ch;
            }

			if (pszPercent != NULL) {
				// Control comes here if the opening percent was not matched by a closing
				// percent.
				while(pszPercent < psz) {
					sResult += *pszPercent++;
				}
			}

        }
		else {
			sResult += ch;
		}
    }

	sValue = sResult;		
	return sc;
	
}


//************************************************************
// SplitComplexPath
//
// Split a complex path consisting of several semicolon separated
// paths into separate paths and return them in a string array.
//
// Parameters:
//		LPCTSTR pszComplexPath
//			Pointer to the path that may or may not be composed of
//			several semicolon separated paths.
//
//		CStringArray& saPath
//			The place to return the split paths.
//
// Returns:
//		The individual paths are returned via saPath
//
//*************************************************************
void SplitComplexPath(LPCTSTR pszComplexPath, CStringArray& saPath)
{
	CString sPath;
	while (*pszComplexPath) {
		sPath.Empty();
		while (isspace(*pszComplexPath))  {
			++pszComplexPath;
		}

		while (*pszComplexPath &&
			   (*pszComplexPath != _T(';'))) {
			sPath += *pszComplexPath++;
		}

		if (!sPath.IsEmpty()) {
			saPath.Add(sPath);
		}

		if (*pszComplexPath==_T(';')) {
			++pszComplexPath;
		}
	}
}



//**************************************************************************
// MapPathToUNC
//
// Map a path to the UNC equivallent. Note that this method assumes that
// for each path containing a drive letter that the target machine will have
// the path shared out.  For example, if the path contains a "c:\foodir" prefix, then
// then you can get to "foodir" bygenerating the "\\machine\c$\foodir" path.
//
// Parameters:
//		LPCTSTR pszMachineName
//			Pointer to the machine name.
//
//		CString& sPath
//			Pointer to the path to map.  Upon return, this string will contain
//			the mapped path.
//
// Returns:
//		SCODE
//			S_OK if successful
//			E_FAIL if something went wrong.
//
//**************************************************************************
SCODE MapPathToUNC(LPCTSTR pszMachineName, CString& sPath)
{
	CStringArray saPaths;
	SplitComplexPath(sPath, saPaths);
	sPath.Empty();
	
	
	LPCTSTR pszPath = sPath.GetBuffer(sPath.GetLength() + 1);
	LONG nPaths = (LONG)saPaths.GetSize();
	SCODE sc = S_OK;
	for (LONG iPath=0; iPath < nPaths; ++iPath) {
		pszPath = saPaths[iPath];

		if (isalpha(pszPath[0]) && pszPath[1]==_T(':')) {
			CString sResult;
			sResult += _T("\\\\");
			sResult += pszMachineName;
			sResult += _T('\\');
			sResult += pszPath[0];		// Drive letter
			sResult += _T("$\\");		
			pszPath += 2;
			if (pszPath[0]==_T('\\')) {
				++pszPath;
			}
			sResult += pszPath;
			saPaths[iPath] = sResult;
		}
		else {
			sc = E_FAIL;
		}
		sPath += saPaths[iPath];
		if (iPath < nPaths - 1) {
			sPath += _T("; ");
		}
	}
	return sc;
}

















