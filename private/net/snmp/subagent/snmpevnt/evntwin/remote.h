
#ifndef _remote_h
#define _remote_h

//===============================================================================
// Class: CEnvCache
//
// This class caches the system environment variables for remote systems.  This
// cache is used to expand environment variables in the context of a remote system.
// 
// The values of the remote system environment variables are loaded from the
// remote system's registry.
//
//==============================================================================
class CEnvCache
{
public:
	
	CEnvCache();
	SCODE Lookup(LPCTSTR pszMachine, LPCTSTR pszName, CString& sResult);
	SCODE AddMachine(LPCTSTR pszMachine);


private:
	CMapStringToOb m_mapMachine;
	SCODE GetEnvironmentVars(LPCTSTR pszMachine, CMapStringToString* pmapVars);

};

SCODE RemoteExpandEnvStrings(LPCTSTR pszComputerName, CEnvCache& cache, CString& sValue);
SCODE MapPathToUNC(LPCTSTR pszMachineName, CString& sPath);




#endif //_remote_h
