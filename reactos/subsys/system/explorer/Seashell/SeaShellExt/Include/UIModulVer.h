////////////////////////////////////////////////////////////////
// 1998 Microsoft Systems Journal
//
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
#ifndef __MODULEVER_H
#define __MODULEVER_H

//#undef _INC_SHLWAPI
//#undef NOSHLWAPI
#include <shlwapi.h>

// tell linker to link with version.lib for VerQueryValue, etc.
#pragma comment(linker, "/defaultlib:version.lib")

#ifndef DLLVER_PLATFORM_WINDOWS
#error ModuleVer.h requires a newer version of the SDK than you have!
#error Please update your SDK files.
#endif

//////////////////
// This class loads a library. Destructor frees for automatic cleanup.
//
class CTRL_EXT_CLASS CLoadLibrary {
private:
	HINSTANCE m_hinst;
public:
	CLoadLibrary(LPCTSTR lpszName) : m_hinst(LoadLibrary(lpszName)) { }
	~CLoadLibrary()		 { FreeLibrary(m_hinst); }
	operator HINSTANCE () { return m_hinst; } 	// cast operator
};

//////////////////
// CModuleVersion version info about a module.
// To use:
//
// CModuleVersion ver
// if (ver.GetFileVersionInfo("_T("mymodule))) {
//		// info is in ver, you can call GetValue to get variable info like
//		CString s = ver.GetValue(_T("CompanyName"));
// }
//
// You can also call the static fn DllGetVersion to get DLLVERSIONINFO.
//
class CTRL_EXT_CLASS CModuleVersion : public VS_FIXEDFILEINFO {
protected:
	BYTE* m_pVersionInfo;	// all version info

	struct TRANSLATION {
		WORD langID;			// language ID
		WORD charset;			// character set (code page)
	} m_translation;

public:
	CModuleVersion();
	virtual ~CModuleVersion();

	BOOL		GetFileVersionInfo(LPCTSTR modulename);
	CString	GetValue(LPCTSTR lpKeyName);
	static BOOL DllGetVersion(LPCTSTR modulename, DLLVERSIONINFO& dvi);
};

#endif
