////////////////////////////////////////////////////////////////
// 1998 Microsoft Systems Journal
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
// CModuleVersion provides an easy way to get version info
// for a module.(DLL or EXE).
//
#include "StdAfx.h"
#include "UIModulVer.h"

CModuleVersion::CModuleVersion()
{
	m_pVersionInfo = NULL;				// raw version info data 
}

//////////////////
// Destroy: delete version info
//
CModuleVersion::~CModuleVersion()
{
	delete [] m_pVersionInfo;
}

//////////////////
// Get file version info for a given module
// Allocates storage for all info, fills "this" with
// VS_FIXEDFILEINFO, and sets codepage.
//
BOOL CModuleVersion::GetFileVersionInfo(LPCTSTR modulename)
{
	m_translation.charset = 1252;		// default = ANSI code page
	memset((VS_FIXEDFILEINFO*)this, 0, sizeof(VS_FIXEDFILEINFO));

	CLoadLibrary lib(modulename);

	// get module handle
	TCHAR filename[_MAX_PATH];
	HMODULE hModule = ::GetModuleHandle(modulename);
	if (hModule==NULL && modulename!=NULL) 
		return FALSE;

	// get module file name
	DWORD len = GetModuleFileName(hModule, filename,
		sizeof(filename)/sizeof(filename[0]));
	if (len <= 0)
		return FALSE;

	// read file version info
	DWORD dwDummyHandle; // will always be set to zero
	len = GetFileVersionInfoSize(filename, &dwDummyHandle);
	if (len <= 0)
		return FALSE;

	if (m_pVersionInfo)
		delete m_pVersionInfo;
	m_pVersionInfo = new BYTE[len]; // allocate version info
	if (!::GetFileVersionInfo(filename, 0, len, m_pVersionInfo))
		return FALSE;

	LPVOID lpvi;
	UINT iLen;
	if (!VerQueryValue(m_pVersionInfo, _T("\\"), &lpvi, &iLen))
		return FALSE;

	// copy fixed info to myself, which am derived from VS_FIXEDFILEINFO
	*(VS_FIXEDFILEINFO*)this = *(VS_FIXEDFILEINFO*)lpvi;

	// Get translation info
	// Note: VerQueryValue could return a value > 4, in which case
	// mulitple languages are supported and VerQueryValue returns an
	// array of langID/codepage pairs and you have to decide which to use.
	if (VerQueryValue(m_pVersionInfo,
		_T("\\VarFileInfo\\Translation"), &lpvi, &iLen) && iLen >= 4) {
		m_translation = *(TRANSLATION*)lpvi;
		TRACE1("code page = %d\n", m_translation.charset);
	}

	return dwSignature == VS_FFI_SIGNATURE;
}

//////////////////
// Get string file info.
// Key name is something like "CompanyName".
// returns the value as a CString.
//
CString CModuleVersion::GetValue(LPCTSTR lpKeyName)
{
	CString sVal;
	if (m_pVersionInfo) {

		// To get a string value must pass query in the form
		//
		//    "\StringFileInfo\<langID><codepage>\keyname"
		//
		// where <lang-codepage> is the languageID concatenated with the
		// code page, in hex. Wow.
		//
		CString query;
		query.Format(_T("\\StringFileInfo\\%04x%04x\\%s"),
			m_translation.langID,
			m_translation.charset,
			lpKeyName);

		LPCTSTR pVal;
		UINT iLenVal;
		if (VerQueryValue(m_pVersionInfo, (LPTSTR)(LPCTSTR)query,
				(LPVOID*)&pVal, &iLenVal)) {

			sVal = pVal;
		}
	}
	return sVal;
}

// typedef for DllGetVersion proc
typedef HRESULT (CALLBACK* DLLGETVERSIONPROC)(DLLVERSIONINFO *);

/////////////////
// Get DLL Version by calling DLL's DllGetVersion proc
//
BOOL CModuleVersion::DllGetVersion(LPCTSTR modulename, DLLVERSIONINFO& dvi)
{
	CLoadLibrary lib(modulename);
	if (!lib)
		return FALSE;

	// Must use GetProcAddress because the DLL might not implement 
	// DllGetVersion. Depending upon the DLL, the lack of implementation of the 
	// function may be a version marker in itself.
	//
	DLLGETVERSIONPROC pDllGetVersion =
		(DLLGETVERSIONPROC)GetProcAddress(lib, "DllGetVersion");
	if (!pDllGetVersion)
		return FALSE;

	memset(&dvi, 0, sizeof(dvi));			 // clear
	dvi.cbSize = sizeof(dvi);				 // set size for Windows

	return SUCCEEDED((*pDllGetVersion)(&dvi));
}
