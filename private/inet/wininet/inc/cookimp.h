#ifndef _COOKIMP_H_
#define _COOKIMP_H_

#define NS_NAVI3        	0x00030000
#define NS_NAVI4        	0x00040000
#define NS_NAVI5        	0x00050000  //  NS_NAVI5 is a guess

BOOL FindNetscapeCookieFile( IN DWORD dwNSVer, OUT LPTSTR szFilename, /* in-out */ LPDWORD lpnBufSize);

//  functions to identify active NS version

BOOL GetActiveNetscapeVersion( LPDWORD lpVersion);
BOOL GetExecuteableFromExtension( IN LPCTSTR szExtension, OUT LPTSTR szFilepath, 
                            LPDWORD pcFilenameSize, OUT LPTSTR* pFilenameSubstring);


//  writes version of Netscape to registry for future reference

BOOL SetNetscapeImportVersion( IN DWORD dwNSVersion);
BOOL GetNetscapeImportVersion( OUT DWORD* pNSVersion);

//  dumps the contents of a file out to memory

BOOL ReadFileToBuffer( IN LPCTSTR szFilename, LPBYTE* ppBuf, LPDWORD lpcbBufSize);


/*
Current behavior for cookie importing:
on first entry:
	Check if netscape is default browser, identify version and save in registry.

on every entry:
	check if version is saved in registry, destructively merge
*/


/*
Something of a justification for behavior:
  There are a couple ways to determine the version of cookie file to be imported from.  For all
intents ane purposes, what is being determined is whether the version to import from is less than
or greater than/equal to four.
  The version of the the executeable associated with htm files is what we use.  If the executeable
associated with htm files is not netscape, then presumeably the user isn't using netscape and we
don't want the cookies anyhow.
  An alternative was to use the last installed version of netscape installed which is indicated
in the CurrentVersion\\AppPaths reg key.  This key gets ripped away though, if the user uninstalls
one version and uses an older version.  Also, we can't expect the user to be using the last
installed version of netscape.

  Once IE is installed and ran, it may be associated with htm files while we still want to import
cookies from the once active version of netscape.  Because of this the version of netscape found
to be used during DllInstall(true,HKLM is) saved in the registry.  This works on 
uninstallation/reinstallation since we can always expect DllInstall(true,HKLM) to be ran again
before reentry into any DllInstall(true,HKCU) where the cookies are imported for each user.
*/

#endif

