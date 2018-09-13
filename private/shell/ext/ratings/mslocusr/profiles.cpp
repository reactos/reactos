/*****************************************************************/
/**               Microsoft Windows for Workgroups              **/
/**           Copyright (C) Microsoft Corp., 1991-1992          **/
/*****************************************************************/

/* PROFILES.CPP -- Code for user profile management.
 *
 * History:
 *  01/04/94    gregj   Created
 *	06/28/94	gregj	Use sync engine for desktop, programs reconciliation
 *	09/05/96	gregj	Snarfed from MPR for use by IE4 family logon.
 */

#include "mslocusr.h"
#include "msluglob.h"

#include "resource.h"

#include <npmsg.h>
#include <regentry.h>
#include <buffer.h>
#include <shellapi.h>

HMODULE g_hmodShell = NULL;
typedef int (*PFNSHFILEOPERATIONA)(LPSHFILEOPSTRUCTA lpFileOp);
PFNSHFILEOPERATIONA g_pfnSHFileOperationA = NULL;


HRESULT LoadShellEntrypoint(void)
{
    if (g_pfnSHFileOperationA != NULL)
        return S_OK;

    HRESULT hres;
    ENTERCRITICAL
    {
        if (g_hmodShell == NULL) {
            g_hmodShell = ::LoadLibrary("SHELL32.DLL");
        }
        if (g_hmodShell != NULL) {
            g_pfnSHFileOperationA = (PFNSHFILEOPERATIONA)::GetProcAddress(g_hmodShell, "SHFileOperationA");
        }
        if (g_pfnSHFileOperationA == NULL)
            hres = HRESULT_FROM_WIN32(::GetLastError());
        else
            hres = S_OK;
    }
    LEAVECRITICAL

    return hres;
}


void UnloadShellEntrypoint(void)
{
    ENTERCRITICAL
    {
        if (g_hmodShell != NULL) {
            ::FreeLibrary(g_hmodShell);
            g_hmodShell = NULL;
            g_pfnSHFileOperationA = NULL;
        }
    }
    LEAVECRITICAL
}


const DWORD attrLocalProfile = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY;

extern "C" {
extern LONG __stdcall RegRemapPreDefKey(HKEY hkeyNew, HKEY hkeyPredef);
};

#ifdef DEBUG

extern "C" {
BOOL fNoisyReg = FALSE;
};

#endif

LONG MyRegLoadKey(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszFile)
{
#ifdef DEBUG
	if (fNoisyReg) {
		char buf[300];
		::wsprintf(buf, "MyRegLoadKey(\"%s\", \"%s\")\r\n", lpszSubKey, lpszFile);
		::OutputDebugString(buf);
	}
#endif

	/* Since the registry doesn't support long filenames, get the short
	 * alias for the path.  If that succeeds, we use that path, otherwise
	 * we just use the original one and hope it works.
	 */
	CHAR szShortPath[MAX_PATH+1];
	if (GetShortPathName(lpszFile, szShortPath, sizeof(szShortPath)))
		lpszFile = szShortPath;

	return ::RegLoadKey(hKey, lpszSubKey, lpszFile);
}


#ifdef DEBUG
LONG MyRegUnLoadKey(HKEY hKey, LPCSTR lpszSubKey)
{
	if (fNoisyReg) {
		char buf[300];
		::wsprintf(buf, "MyRegUnLoadKey(\"%s\")\r\n", lpszSubKey);
		::OutputDebugString(buf);
	}
	return ::RegUnLoadKey(hKey, lpszSubKey);
}
#endif


LONG MyRegSaveKey(HKEY hKey, LPCSTR lpszFile, LPSECURITY_ATTRIBUTES lpsa)
{
#ifdef DEBUG
	if (fNoisyReg) {
		char buf[300];
		::wsprintf(buf, "MyRegSaveKey(\"%s\")\r\n", lpszFile);
		::OutputDebugString(buf);
	}
#endif

	/* Since the registry doesn't support long filenames, get the short
	 * alias for the path.  If that succeeds, we use that path, otherwise
	 * we just use the original one and hope it works.
	 *
	 * GetShortPathName only works if the file exists, so we have to
	 * create a dummy copy first.
	 */

	HANDLE hTemp = ::CreateFile(lpszFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
								FILE_ATTRIBUTE_NORMAL, NULL);
	if (hTemp == INVALID_HANDLE_VALUE)
		return ::GetLastError();
	::CloseHandle(hTemp);

	CHAR szShortPath[MAX_PATH+1];
	if (::GetShortPathName(lpszFile, szShortPath, sizeof(szShortPath)))
		lpszFile = szShortPath;

	return ::RegSaveKey(hKey, lpszFile, lpsa);
}

#ifndef DEBUG
#define MyRegUnLoadKey	RegUnLoadKey
#endif

LONG OpenLogonKey(HKEY *phKey)
{
	return ::RegOpenKey(HKEY_LOCAL_MACHINE, szLogonKey, phKey);
}


void AddBackslash(LPSTR lpPath)
{
	LPCSTR lpBackslash = ::strrchrf(lpPath, '\\');
	if (lpBackslash == NULL || *(lpBackslash+1) != '\0')
		::strcatf(lpPath, "\\");
}


void AddBackslash(NLS_STR& nlsPath)
{
	ISTR istrBackslash(nlsPath);
	if (!nlsPath.strrchr(&istrBackslash, '\\') ||
		*nlsPath.QueryPch(++istrBackslash) != '\0')
		nlsPath += '\\';
}


void GetDirFromPath(NLS_STR& nlsTempDir, LPCSTR pszPath)
{
	nlsTempDir = pszPath;

	ISTR istrBackslash(nlsTempDir);
	if (nlsTempDir.strrchr(&istrBackslash, '\\'))
		nlsTempDir.DelSubStr(istrBackslash);
}


BOOL FileExists(LPCSTR pszPath)
{
	DWORD dwAttrs = ::GetFileAttributes(pszPath);

	if (dwAttrs != 0xffffffff && !(dwAttrs & FILE_ATTRIBUTE_DIRECTORY))
		return TRUE;
	else
		return FALSE;
}


BOOL DirExists(LPCSTR pszPath)
{
	if (*pszPath == '\0')
		return FALSE;

	DWORD dwAttrs = ::GetFileAttributes(pszPath);

	if (dwAttrs != 0xffffffff && (dwAttrs & FILE_ATTRIBUTE_DIRECTORY))
		return TRUE;
	else
		return FALSE;
}


/* CreateDirectoryPath attempts to create the specified directory;  if the
 * create attempt fails, it tries to create each element of the path in case
 * any intermediate directories also don't exist.
 */
BOOL CreateDirectoryPath(LPCSTR pszPath)
{
    BOOL fRet = ::CreateDirectory(pszPath, NULL);

    if (fRet || (::GetLastError() != ERROR_PATH_NOT_FOUND))
        return fRet;

    NLS_STR nlsTemp(pszPath);
    if (nlsTemp.QueryError() != ERROR_SUCCESS)
        return FALSE;

    LPSTR pszTemp = nlsTemp.Party();
    LPSTR pszNext = pszTemp;

    /* If it's a drive-based path (which it should be), skip the drive
     * and first backslash -- we don't need to attempt to create the
     * root directory.
     */
    if (::strchrf(pszTemp, ':') != NULL) {
        pszNext = ::strchrf(pszTemp, '\\');
        if (pszNext != NULL)
            pszNext++;
    }

    /* Now walk through the path creating one directory at a time. */

    for (;;) {
        pszNext = ::strchrf(pszNext, '\\');
        if (pszNext != NULL) {
            *pszNext = '\0';
        }
        else {
            break;          /* no more intermediate directories to create */
        }

        /* Create the intermediate directory.  No error checking because we're
         * not extremely performance-critical, and we can get errors if the
         * directory already exists, etc.  With security and other things,
         * the set of benign error codes we'd have to check for could be
         * large.
         */
        fRet = ::CreateDirectory(pszTemp, NULL);

        *pszNext = '\\';
        pszNext++;
        if (!*pszNext)      /* ended with trailing slash? */
            return fRet;    /* return last result */
    }

    /* We should have created all the intermediate directories by now.
     * Create the final path.
     */

    return ::CreateDirectory(pszPath, NULL);
}


UINT SafeCopy(LPCSTR pszSrc, LPCSTR pszDest, DWORD dwAttrs)
{
	NLS_STR nlsTempDir(MAX_PATH);
	NLS_STR nlsTempFile(MAX_PATH);
	if (!nlsTempDir || !nlsTempFile)
		return ERROR_NOT_ENOUGH_MEMORY;

	GetDirFromPath(nlsTempDir, pszDest);

	if (!::GetTempFileName(nlsTempDir.QueryPch(), ::szProfilePrefix, 0,
						   nlsTempFile.Party()))
		return ::GetLastError();

	nlsTempFile.DonePartying();

	if (!::CopyFile(pszSrc, nlsTempFile.QueryPch(), FALSE)) {
		UINT err = ::GetLastError();
		::DeleteFile(nlsTempFile.QueryPch());
		return err;
	}

	::SetFileAttributes(pszDest, FILE_ATTRIBUTE_NORMAL);

	::DeleteFile(pszDest);

	// At this point, the temp file has the same attributes as the original
	// (usually read-only, hidden, system).  Some servers, such as NetWare
	// servers, won't allow us to rename a read-only file.  So we have to
	// take the attributes off, rename the file, then put back whatever the
	// caller wants.
	::SetFileAttributes(nlsTempFile.QueryPch(), FILE_ATTRIBUTE_NORMAL);

	if (!::MoveFile(nlsTempFile.QueryPch(), pszDest))
		return ::GetLastError();

	::SetFileAttributes(pszDest, dwAttrs);

	return ERROR_SUCCESS;
}


#ifdef LOAD_PROFILES

void SetProfileTime(LPCSTR pszLocalPath, LPCSTR pszCentralPath)
{
	HANDLE hFile = ::CreateFile(pszCentralPath,
								GENERIC_READ | GENERIC_WRITE,
								FILE_SHARE_READ, NULL,
								OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		FILETIME ft;

		::GetFileTime(hFile, NULL, NULL, &ft);
		::CloseHandle(hFile);

		DWORD dwAttrs = ::GetFileAttributes(pszLocalPath);
		if (dwAttrs & FILE_ATTRIBUTE_READONLY) {
			::SetFileAttributes(pszLocalPath, dwAttrs & ~FILE_ATTRIBUTE_READONLY);
		}
		hFile = ::CreateFile(pszLocalPath, GENERIC_READ | GENERIC_WRITE,
							 FILE_SHARE_READ, NULL, OPEN_EXISTING,
							 FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE) {
			::SetFileTime(hFile, NULL, NULL, &ft);
			::CloseHandle(hFile);
		}
		if (dwAttrs & FILE_ATTRIBUTE_READONLY) {
			::SetFileAttributes(pszLocalPath, dwAttrs & ~FILE_ATTRIBUTE_READONLY);
		}
	}
}


UINT DefaultReconcile(LPCSTR pszCentralPath, LPCSTR pszLocalPath, DWORD dwFlags)
{
	UINT err;

	if (dwFlags & RP_LOGON) {
		if (dwFlags & RP_INIFILE)
			return SafeCopy(pszCentralPath, pszLocalPath, FILE_ATTRIBUTE_NORMAL);

		HANDLE hFile = ::CreateFile(pszCentralPath, GENERIC_READ, FILE_SHARE_READ,
									NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		FILETIME ftCentral;
		if (hFile != INVALID_HANDLE_VALUE) {
			::GetFileTime(hFile, NULL, NULL, &ftCentral);
			::CloseHandle(hFile);
		}
		else {
			ftCentral.dwLowDateTime = 0;	/* can't open, pretend it's really old */
			ftCentral.dwHighDateTime = 0;
		}

		hFile = ::CreateFile(pszLocalPath, GENERIC_READ, FILE_SHARE_READ,
							 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		FILETIME ftLocal;
		if (hFile != INVALID_HANDLE_VALUE) {
			::GetFileTime(hFile, NULL, NULL, &ftLocal);
			::CloseHandle(hFile);
		}
		else {
			ftLocal.dwLowDateTime = 0;	/* can't open, pretend it's really old */
			ftLocal.dwHighDateTime = 0;
		}

		LPCSTR pszSrc, pszDest;

		/*
		 * Find out which file is newer, and make that the source
		 * for the copy.
		 */

		LONG lCompare = ::CompareFileTime(&ftCentral, &ftLocal);
		if (!lCompare) {
			::dwProfileFlags |= PROF_CENTRALWINS;
			return WN_SUCCESS;		/* timestamps match, no copy to do */
		}
		else if (lCompare > 0) {
			pszSrc = pszCentralPath;
			pszDest = pszLocalPath;
			::dwProfileFlags |= PROF_CENTRALWINS;
		}
		else {
			pszSrc = pszLocalPath;
			pszDest = pszCentralPath;
			::dwProfileFlags &= ~PROF_CENTRALWINS;
		}

		err = SafeCopy(pszSrc, pszDest,
					   pszDest == pszCentralPath ? FILE_ATTRIBUTE_NORMAL
					   : attrLocalProfile);
	}
	else {
		err = SafeCopy(pszLocalPath, pszCentralPath, FILE_ATTRIBUTE_NORMAL);
		if (err == WN_SUCCESS) {	/* copied back successfully */

#ifdef EXTENDED_PROFILES	/* chicago doesn't special-case resident profiles */
			if (dwFlags & PROF_RESIDENT) {
				DeleteProfile(pszLocalPath);	/* delete temp file */
			}
#endif

			SetProfileTime(pszLocalPath, pszCentralPath);
		}
	}

	return err;
}


#endif	/* LOAD_PROFILES */


void GetLocalProfileDirectory(NLS_STR& nlsPath)
{
	::GetWindowsDirectory(nlsPath.Party(), nlsPath.QueryAllocSize());
	nlsPath.DonePartying();

	AddBackslash(nlsPath);

	nlsPath.strcat(::szProfilesDirectory);

	::CreateDirectory(nlsPath.QueryPch(), NULL);
}


HRESULT GiveUserDefaultProfile(LPCSTR lpszPath)
{
	HKEY hkeyDefaultUser;
	LONG err = ::RegOpenKey(HKEY_USERS, ::szDefaultUserName, &hkeyDefaultUser);
	if (err == ERROR_SUCCESS) {
		err = ::MyRegSaveKey(hkeyDefaultUser, lpszPath, NULL);
		::RegCloseKey(hkeyDefaultUser);
	}
	return HRESULT_FROM_WIN32(err);
}


void ComputeLocalProfileName(LPCSTR pszUsername, NLS_STR *pnlsLocalProfile)
{
	GetLocalProfileDirectory(*pnlsLocalProfile);

	UINT cbPath = pnlsLocalProfile->strlen();
	LPSTR lpPath = pnlsLocalProfile->Party();
	LPSTR lpFilename = lpPath + cbPath;

	*(lpFilename++) = '\\';
	::strcpyf(lpFilename, pszUsername);		/* start with whole username */

	LPSTR lpFNStart = lpFilename;

	UINT iFile = 0;
	while (!::CreateDirectory(lpPath, NULL)) {
		if (!DirExists(lpPath))
			break;

		/* Couldn't use whole username, start with 5 bytes of username + numbers. */
		if (iFile == 0) {
			::strncpyf(lpFilename, pszUsername, 5);	/* copy at most 5 bytes of username */
			*(lpFilename+5) = '\0';					/* force null term, just in case */
			lpFilename += ::strlenf(lpFilename);
		}
		else if (iFile >= 4095) {	/* max number expressible in 3 hex digits */
			lpFilename = lpFNStart;	/* start using big numbers with no uname prefix */
			if (iFile < 0)			/* if we run out of numbers, abort */
				break;
		}

		::wsprintf(lpFilename, "%03lx", iFile);

		iFile++;
	}

	pnlsLocalProfile->DonePartying();
}


HRESULT CopyProfile(LPCSTR pszSrcPath, LPCSTR pszDestPath)
{
	UINT err = SafeCopy(pszSrcPath, pszDestPath, attrLocalProfile);

	return HRESULT_FROM_WIN32(err);
}


BOOL UseUserProfiles(void)
{
	HKEY hkeyLogon;

	LONG err = OpenLogonKey(&hkeyLogon);
	if (err == ERROR_SUCCESS) {
		DWORD fUseProfiles = 0;
		DWORD cbData = sizeof(fUseProfiles);
		err = ::RegQueryValueEx(hkeyLogon, (LPSTR)::szUseProfiles, NULL, NULL,
								(LPBYTE)&fUseProfiles, &cbData);
		::RegCloseKey(hkeyLogon);
		return (err == ERROR_SUCCESS) && fUseProfiles;
	}

	return FALSE;
}


void EnableProfiles(void)
{
	HKEY hkeyLogon;

	LONG err = OpenLogonKey(&hkeyLogon);
	if (err == ERROR_SUCCESS) {
		DWORD fUseProfiles = 1;
		::RegSetValueEx(hkeyLogon, (LPSTR)::szUseProfiles, 0, REG_DWORD,
						(LPBYTE)&fUseProfiles, sizeof(fUseProfiles));
		::RegCloseKey(hkeyLogon);
	}
}


struct SYNCSTATE
{
    HKEY hkeyProfile;
    NLS_STR *pnlsProfilePath;
    NLS_STR *pnlsOtherProfilePath;
    HKEY hkeyPrimary;
};


/*
 * PrefixMatch determines whether a given path is equal to or a descendant
 * of a given base path.
 */
BOOL PrefixMatch(LPCSTR pszPath, LPCSTR pszBasePath)
{
	UINT cchBasePath = ::strlenf(pszBasePath);
	if (!::strnicmpf(pszPath, pszBasePath, cchBasePath)) {
		/* make sure that the base path matches the whole last component */
		if ((pszPath[cchBasePath] == '\\' || pszPath[cchBasePath] == '\0'))
			return TRUE;
		/* check to see if the base path is a root path;  if so, match */
		LPCSTR pszBackslash = ::strrchrf(pszBasePath, '\\');
		if (pszBackslash != NULL && *(pszBackslash+1) == '\0')
			return TRUE;
		else
			return FALSE;
	}
	else
		return FALSE;
}


#if 0
void ReportReconcileError(SYNCSTATE *pSyncState, TWINRESULT tr, PRECITEM pri,
						  PRECNODE prnSrc, PRECNODE prnDest, BOOL fSrcCentral)
{
	/* If we're copying the file the "wrong" way, swap our idea of the
	 * source and destination.  For the purposes of other profile code,
	 * source and destination refer to the entire profile copy direction.
	 * For this particular error message, they refer to the direction
	 * that this particular file was being copied.
	 */
	if (prnSrc->rnaction == RNA_COPY_TO_ME) {
		PRECNODE prnTemp = prnSrc;
		prnSrc = prnDest;
		prnDest = prnTemp;
		fSrcCentral = !fSrcCentral;
	}

	/* Set the error status on this key to be the destination of the copy,
	 * which is the copy that's now out of date because of the error and
	 * needs to be guarded from harm next time.
	 */
	pSyncState->uiRecError |= fSrcCentral ? RECERROR_LOCAL : RECERROR_CENTRAL;

	pSyncState->dwFlags |= SYNCSTATE_ERROR;

	if (pSyncState->dwFlags & SYNCSTATE_ERRORMSG)
		return;			/* error already reported */

	pSyncState->dwFlags |= SYNCSTATE_ERRORMSG;

	RegEntry re(::szReconcileRoot, pSyncState->hkeyProfile);
	if (re.GetError() == ERROR_SUCCESS && !re.GetNumber(::szDisplayProfileErrors, TRUE))
		return;		/* user doesn't want to see this error message */

	PCSTR pszFile;
	UINT uiMainMsg;

	switch (tr) {
	case TR_DEST_OPEN_FAILED:
	case TR_DEST_WRITE_FAILED:
		uiMainMsg = IERR_ProfRecWriteDest;
		pszFile = prnDest->pcszFolder;
		break;
	case TR_SRC_OPEN_FAILED:
	case TR_SRC_READ_FAILED:
		uiMainMsg = IERR_ProfRecOpenSrc;
		pszFile = prnSrc->pcszFolder;
		break;
	default:
		uiMainMsg = IERR_ProfRecCopy;
		pszFile = pri->pcszName;
		break;
	}

	if (DisplayGenericError(NULL, uiMainMsg, tr, pszFile, ::szNULL,
							MB_YESNO | MB_ICONEXCLAMATION, IDS_TRMsgBase) == IDNO) {
		re.SetValue(::szDisplayProfileErrors, (ULONG)FALSE);
	}
}


#ifdef DEBUG
char szOutbuf[200];
#endif


/*
 * MyReconcile is a wrapper around ReconcileItem.  It needs to detect merge
 * type operations and transform them into copies in the appropriate direction,
 * and recognize when the sync engine wants to replace a file that the user
 * really wants deleted.
 */
void MyReconcile(PRECITEM pri, SYNCSTATE *pSyncState)
{
	if (pri->riaction == RIA_NOTHING)
		return;

	/* Because we don't have a persistent briefcase, we can't recognize when
	 * the user has deleted an item;  the briefcase will want to replace it
	 * with the other version, which is not what the user wants.  So we use
	 * the direction of the profile's copy, and if the sync engine wants to
	 * copy a file from the "destination" of the profile's copy to the "source"
	 * because the "source" doesn't exist, we recognize that as the source
	 * having been deleted and synchronize manually by deleting the dest.
	 *
	 * prnSrc points to the recnode for the item that's coming from the same
	 * side of the transaction that the more recent profile was on;  prnDest
	 * points to the recnode for the other side.
	 *
	 * The test is complicated because we first have to figure out which of
	 * the two directories (nlsDir1, the local dir; or nlsDir2, the central
	 * dir) is the source and which the destination.  Then we have to figure
	 * out which of the two RECNODEs we got matches which directory.
	 */
	PRECNODE prnSrc;
	PRECNODE prnDest;
	LPCSTR pszSrcBasePath;
	BOOL fSrcCentral;
	if (pSyncState->IsMandatory() || (pSyncState->dwFlags & PROF_CENTRALWINS)) {
		pszSrcBasePath = pSyncState->nlsDir2.QueryPch();
		fSrcCentral = TRUE;
	}
	else {
		pszSrcBasePath = pSyncState->nlsDir1.QueryPch();
		fSrcCentral = FALSE;
	}

	if (PrefixMatch(pri->prnFirst->pcszFolder, pszSrcBasePath)) {
		prnSrc = pri->prnFirst;
		prnDest = prnSrc->prnNext;
	}
	else {
		prnDest = pri->prnFirst;
		prnSrc = prnDest->prnNext;
	}

	/*
	 * If files of the same name exist in both places, the sync engine thinks
     * they need to be merged (since we have no persistent briefcase database,
     * it doesn't know that they were originally the same).  The sync engine
     * sets the file stamp of a copied destination file to the file stamp of
     * the source file after copying.  If the file stamps of two files to be
     * merged are the same, we assume that the files are already up-to-date,
     * and we take no reconciliation action.  If the file stamps of two files
     * to be merged are different, we really just want a copy, so we figure out
     * which one is supposed to be definitive and transform the RECITEM and
     * RECNODEs to indicate a copy instead of a merge.
	 *
	 * The definitive copy is the source for mandatory or logoff cases,
	 * otherwise it's the newer file.
	 */
	if (pri->riaction == RIA_MERGE || pri->riaction == RIA_BROKEN_MERGE) {
		BOOL fCopyFromSrc;
        COMPARISONRESULT cr;

		if (pSyncState->IsMandatory())
			fCopyFromSrc = TRUE;
        else {
	        fCopyFromSrc = ! pSyncState->IsLogon();  

            if (pSyncState->CompareFileStamps(&prnSrc->fsCurrent, &prnDest->fsCurrent, &cr) == TR_SUCCESS) {
                if (cr == CR_EQUAL) {
#ifdef MAXDEBUG
			       ::OutputDebugString("Matching file stamps, no action taken\r\n");  
#endif
                   return;
                }
                else if (cr==CR_FIRST_LARGER)       
			       fCopyFromSrc = TRUE;
            }
        }

#ifdef MAXDEBUG
		if (fCopyFromSrc)
			::OutputDebugString("Broken merge, copying from src\r\n");
		else
			::OutputDebugString("Broken merge, copying from dest\r\n");
#endif

		prnSrc->rnaction = fCopyFromSrc ? RNA_COPY_FROM_ME : RNA_COPY_TO_ME;
		prnDest->rnaction = fCopyFromSrc ? RNA_COPY_TO_ME : RNA_COPY_FROM_ME;
		pri->riaction = RIA_COPY;
	}

	/*
	 * If the preferred source file doesn't exist, the sync engine is trying
	 * to create a file to make the two trees the same, when the user/admin
	 * really wanted to delete it (the sync engine doesn't like deleting
	 * files).  So we detect that case here and delete the "destination"
	 * to make the two trees match that way.
	 *
	 * If the last reconciliation had an error, we don't do the deletion
	 * if the site of the error is the current source (i.e., if we're
	 * about to delete the file we couldn't copy before).  Instead we'll
	 * try the operation that the sync engine wants, since that'll be the
	 * copy that failed before.
	 */
	if (prnSrc->rnstate == RNS_DOES_NOT_EXIST &&
		prnSrc->rnaction == RNA_COPY_TO_ME &&
		!((pSyncState->uiRecError & RECERROR_CENTRAL) && fSrcCentral) &&
		!((pSyncState->uiRecError & RECERROR_LOCAL) && !fSrcCentral)) {
		if (IS_EMPTY_STRING(pri->pcszName)) {
			::RemoveDirectory(prnDest->pcszFolder);
		}
		else {
			NLS_STR nlsTemp(prnDest->pcszFolder);
			AddBackslash(nlsTemp);
			nlsTemp.strcat(pri->pcszName);
			if (!nlsTemp.QueryError()) {
#ifdef MAXDEBUG
				if (pSyncState->IsMandatory())
					::OutputDebugString("Mandatory copy wrong way\r\n");

				wsprintf(::szOutbuf, "Deleting 'destination' file %s\r\n", nlsTemp.QueryPch());
				::OutputDebugString(::szOutbuf);
#endif
				::DeleteFile(nlsTemp.QueryPch());
			}
		}
		return;
	}

#ifdef MAXDEBUG
	::OutputDebugString("Calling ReconcileItem.\r\n");
#endif

	TWINRESULT tr;
	if ((tr=pSyncState->ReconcileItem(pri, NULL, 0, 0, NULL, NULL)) != TR_SUCCESS) {
		ReportReconcileError(pSyncState, tr, pri, prnSrc, prnDest, fSrcCentral);
#ifdef MAXDEBUG
		::wsprintf(::szOutbuf, "Error %d from ReconcileItem.\r\n", tr);
		::OutputDebugString(::szOutbuf);
#endif
	}
	else if (!IS_EMPTY_STRING(pri->pcszName))
		pSyncState->dwFlags |= SYNCSTATE_SOMESUCCESS;
}


/*
 * MakePathAbsolute examines a path to see whether it is absolute or relative.
 * If it is relative, it is prepended with the given base path.
 *
 * If the fMustBeRelative parameter is TRUE, then an error is returned if the
 * path was (a) absolute and (b) not a subdirectory of the old profile directory.
 */
BOOL MakePathAbsolute(NLS_STR& nlsDir, LPCSTR lpszBasePath,
					  NLS_STR& nlsOldProfileDir, BOOL fMustBeRelative)
{
	/* If the path starts with a special keyword, replace it. */

	if (*nlsDir.QueryPch() == '*') {
		return ReplaceCommonPath(nlsDir);
	}

	/* If the path is absolute and is relative to whatever the old profile
	 * directory was, transform it to a relative path.  We will then make
	 * it absolute again, using the new base path.
	 */
	if (PrefixMatch(nlsDir, nlsOldProfileDir)) {
		UINT cchDir = nlsDir.strlen();
		LPSTR lpStart = nlsDir.Party();
		::memmovef(lpStart, lpStart + nlsOldProfileDir.strlen(), cchDir - nlsOldProfileDir.strlen() + 1);
		nlsDir.DonePartying();
	}
	else if (::strchrf(nlsDir.QueryPch(), ':') != NULL || *nlsDir.QueryPch() == '\\')
		return !fMustBeRelative;

	if (*lpszBasePath == '\0') {
		nlsDir = lpszBasePath;
		return TRUE;
	}

	NLS_STR nlsBasePath(lpszBasePath);
	if (nlsBasePath.QueryError())
		return FALSE;
	AddBackslash(nlsBasePath);

	ISTR istrStart(nlsDir);
	nlsDir.InsertStr(nlsBasePath, istrStart);
	return !nlsDir.QueryError();
}
#endif  /**** 0 ****/


/*
 * ReplaceCommonPath takes a relative path beginning with a special keyword
 * and replaces the keyword with the corresponding real path.  Currently the
 * keyword supported is:
 *
 * *windir - replaced with the Windows (user) directory
 */
BOOL ReplaceCommonPath(NLS_STR& nlsDir)
{
	NLS_STR *pnlsTemp;
	ISTR istrStart(nlsDir);
	ISTR istrEnd(nlsDir);

	nlsDir.strchr(&istrEnd, '\\');
	pnlsTemp = nlsDir.QuerySubStr(istrStart, istrEnd);
	if (pnlsTemp == NULL)
		return FALSE;				/* out of memory, can't do anything */

	BOOL fSuccess = TRUE;
	if (!::stricmpf(pnlsTemp->QueryPch(), ::szWindirAlias)) {
		UINT cbBuffer = pnlsTemp->QueryAllocSize();
		LPSTR lpBuffer = pnlsTemp->Party();
		UINT cchWindir = ::GetWindowsDirectory(lpBuffer, cbBuffer);
		if (cchWindir >= cbBuffer)
			*lpBuffer = '\0';
		pnlsTemp->DonePartying();
		if (cchWindir >= cbBuffer) {
			pnlsTemp->realloc(cchWindir+1);
			if (!pnlsTemp->QueryError()) {
				::GetWindowsDirectory(pnlsTemp->Party(), cchWindir+1);
				pnlsTemp->DonePartying();
			}
			else
				fSuccess = FALSE;
		}
		if (fSuccess) {
			nlsDir.ReplSubStr(*pnlsTemp, istrStart, istrEnd);
			fSuccess = !nlsDir.QueryError();
		}
	}
	delete pnlsTemp;
	return fSuccess;
}


/*
 * GetSetRegistryPath goes to the registry key and value specified by
 * the current reconciliations's RegKey and RegValue settings, and
 * retrieves or sets a path there.
 */
void GetSetRegistryPath(HKEY hkeyProfile, RegEntry& re, NLS_STR *pnlsPath, BOOL fSet)
{
	NLS_STR nlsKey;

	re.GetValue(::szReconcileRegKey, &nlsKey);
	if (nlsKey.strlen() > 0) {
		NLS_STR nlsValue;
		re.GetValue(::szReconcileRegValue, &nlsValue);
		RegEntry re2(nlsKey, hkeyProfile);
		if (fSet) {
			re2.SetValue(nlsValue, pnlsPath->QueryPch());
            if (!nlsKey.stricmp("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders")) {
                nlsKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
                RegEntry reShell(nlsKey, hkeyProfile);
                reShell.SetValue(nlsValue, pnlsPath->QueryPch());
            }
        }
		else
			re2.GetValue(nlsValue, pnlsPath);
	}
}


/* CopyFolder calls the shell's copy engine to copy files.  The source is a
 * double-null-terminated list;  the destination is a folder.
 */
void CopyFolder(LPBYTE pbSource, LPCSTR pszDest)
{
    CHAR szDest[MAX_PATH];

    ::strcpyf(szDest, pszDest);
    szDest[::strlenf(szDest) + 1] = '\0';

    SHFILEOPSTRUCT fos;

    fos.hwnd = NULL;
    fos.wFunc = FO_COPY;
    fos.pFrom = (LPCSTR)pbSource;
    fos.pTo = szDest;
    fos.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI;
    fos.fAnyOperationsAborted = FALSE;
    fos.hNameMappings = NULL;
    fos.lpszProgressTitle = NULL;

    g_pfnSHFileOperationA(&fos);
}


/*
 * ReconcileKey performs reconciliation for a particular key in the
 * ProfileReconciliation branch of the registry.  It reads the config
 * parameters for the reconciliation, sets up an appropriate twin in
 * the temporary briefcase, and performs the reconciliation.
 */
BOOL ReconcileKey(HKEY hkeySection, LPCSTR lpszSubKey, SYNCSTATE *pSyncState)
{
#ifdef DEBUG
	DWORD dwStart = ::GetTickCount();
#endif

	BOOL fShouldDelete = FALSE;

	RegEntry re(lpszSubKey, hkeySection);
	if (re.GetError() == ERROR_SUCCESS) {
        BUFFER bufSrcStrings(MAX_PATH);
        NLS_STR nlsSrcPath(MAX_PATH);
        NLS_STR nlsDestPath(MAX_PATH);
        NLS_STR nlsName(MAX_PATH);
        if (bufSrcStrings.QueryPtr() != NULL &&
            nlsSrcPath.QueryError() == ERROR_SUCCESS &&
            nlsDestPath.QueryError() == ERROR_SUCCESS &&
            nlsName.QueryError() == ERROR_SUCCESS) {

            /* Get the source path to copy.  Usually it's in the profile,
             * left over from the profile we cloned.  If not, we take the
             * default local name from the ProfileReconciliation key.  If
             * the path already in the registry is not relative to the cloned
             * profile directory, then it's probably set by system policies
             * or something, and we shouldn't touch it.
             */
            if (pSyncState->pnlsOtherProfilePath != NULL) {
        		GetSetRegistryPath(pSyncState->hkeyProfile, re, &nlsSrcPath, FALSE);
                if (nlsSrcPath.strlen() && 
                    !PrefixMatch(nlsSrcPath.QueryPch(), pSyncState->pnlsOtherProfilePath->QueryPch())) {
                    return FALSE;   /* not profile-relative, nothing to do */
                }
            }
            if (!nlsSrcPath.strlen()) {
				re.GetValue(::szDefaultDir, &nlsSrcPath);
            	if (*nlsSrcPath.QueryPch() == '*') {
            		ReplaceCommonPath(nlsSrcPath);
            	}
            }

            /* Get the set of files to copy.  Like NT and unlike win95, we
             * want to clone the entire contents, not necessarily just the
             * files listed (for example, the desktop -- we want all the
             * files and subfolders, not just links).  So, unless the string
             * is empty, which means don't copy any content, just set the reg
             * path, we change any pattern containing wildcards to *.*.
             */
            re.GetValue(::szReconcileName, &nlsName);
            if (nlsName.strlen()) {
                if (::strchrf(nlsName.QueryPch(), '*') != NULL ||
                    ::strchrf(nlsName.QueryPch(), '?') != NULL) {
                    nlsName = "*.*";
                }
            }

            /* Get the destination path.  This is generated from the new
             * profile directory and the LocalFile entry in the registry.
             *
             * Should always do this, even if we're not going to call the
             * copy engine, because we're going to write this path out to
             * the registry.
             */
            re.GetValue(::szLocalFile, &nlsDestPath);
            ISTR istr(nlsDestPath);
            nlsDestPath.InsertStr(*(pSyncState->pnlsProfilePath), istr);

            /* Always create the destination path, even if we don't copy
             * any files into it because the source directory doesn't exist.
             */
            CreateDirectoryPath(nlsDestPath.QueryPch());

            /* Make sure the source directory exists so we won't get useless
             * error messages from the shell copy engine.
             */
            DWORD dwAttr = GetFileAttributes(nlsSrcPath.QueryPch());
            if (dwAttr != 0xffffffff && (dwAttr & FILE_ATTRIBUTE_DIRECTORY) &&
                nlsName.strlen()) {

                AddBackslash(nlsSrcPath);

                /* Build up the double-null-terminated list of file specs to copy. */

                UINT cbUsed = 0;

		    	LPSTR lpName = nlsName.Party();
			    do {
				    LPSTR lpNext = ::strchrf(lpName, ',');
    				if (lpNext != NULL) {
	    				*(lpNext++) = '\0';
		    		}

                    UINT cbNeeded = nlsSrcPath.strlen() + ::strlenf(lpName) + 1;
                    if (bufSrcStrings.QuerySize() - cbUsed < cbNeeded) {
                        if (!bufSrcStrings.Resize(bufSrcStrings.QuerySize() + MAX_PATH))
                            return FALSE;
                    }
                    LPSTR lpDest = ((LPSTR)bufSrcStrings.QueryPtr()) + cbUsed;
                    ::strcpyf(lpDest, nlsSrcPath.QueryPch());
                    lpDest += nlsSrcPath.strlen();
                    ::strcpyf(lpDest, lpName);
                    cbUsed += cbNeeded;

    				lpName = lpNext;
	    		} while (lpName != NULL);

                *((LPSTR)bufSrcStrings.QueryPtr() + cbUsed) = '\0';    /* double null terminate */
	    		nlsName.DonePartying();

                CopyFolder((LPBYTE)bufSrcStrings.QueryPtr(), nlsDestPath.QueryPch());
            }

    		/*
		     * Set a registry key to point to the new local path to this directory.
	    	 */
    		GetSetRegistryPath(pSyncState->hkeyProfile, re, &nlsDestPath, TRUE);
		}
    }

#ifdef MAXDEBUG
	::wsprintf(::szOutbuf, "ReconcileKey duration %d ms.\r\n", ::GetTickCount() - dwStart);
	::OutputDebugString(::szOutbuf);
#endif

	return fShouldDelete;
}


/*
 * GetMaxSubkeyLength just calls RegQueryInfoKey to get the length of the
 * longest named subkey of the given key.  The return value is the size
 * of buffer needed to hold the longest key name, including the null
 * terminator.
 */
DWORD GetMaxSubkeyLength(HKEY hKey)
{
	DWORD cchClass = 0;
	DWORD cSubKeys;
	DWORD cchMaxSubkey;
	DWORD cchMaxClass;
	DWORD cValues;
	DWORD cchMaxValueName;
	DWORD cbMaxValueData;
	DWORD cbSecurityDescriptor;
	FILETIME ftLastWriteTime;

	RegQueryInfoKey(hKey, NULL, &cchClass, NULL, &cSubKeys, &cchMaxSubkey,
					&cchMaxClass, &cValues, &cchMaxValueName, &cbMaxValueData,
					&cbSecurityDescriptor, &ftLastWriteTime);
	return cchMaxSubkey + 1;
}


/*
 * ReconcileSection walks through the ProfileReconciliation key and performs
 * reconciliation for each subkey.  One-time keys are deleted after they are
 * processed.
 */
void ReconcileSection(HKEY hkeyRoot, SYNCSTATE *pSyncState)
{
	NLS_STR nlsKeyName(GetMaxSubkeyLength(hkeyRoot));
	if (!nlsKeyName.QueryError()) {
		DWORD iKey = 0;

		for (;;) {
			DWORD cchKey = nlsKeyName.QueryAllocSize();

			UINT err = ::RegEnumKey(hkeyRoot, iKey, nlsKeyName.Party(), cchKey);
			if (err != ERROR_SUCCESS)
				break;

			nlsKeyName.DonePartying();
			if (ReconcileKey(hkeyRoot, nlsKeyName, pSyncState)) {
				::RegDeleteKey(hkeyRoot, nlsKeyName.QueryPch());
			}
			else
				iKey++;
		}
	}
}


/*
 * ReconcileFiles is called just after the user's profile and policies are
 * loaded at logon, and just before the profile is unloaded at logoff.  It
 * performs all file type reconciliation for the user's profile, excluding
 * the profile itself, of course.
 *
 * nlsOtherProfilePath is the path to the profile which is being cloned,
 * or an empty string if the default profile is being cloned.
 */
HRESULT ReconcileFiles(HKEY hkeyProfile, NLS_STR& nlsProfilePath,
                    NLS_STR& nlsOtherProfilePath)
{
    HRESULT hres = LoadShellEntrypoint();
    if (FAILED(hres))
        return hres;

    if (nlsOtherProfilePath.strlen())
    {
    	ISTR istrBackslash(nlsOtherProfilePath);
	    if (nlsOtherProfilePath.strrchr(&istrBackslash, '\\')) {
            ++istrBackslash;
		    nlsOtherProfilePath.DelSubStr(istrBackslash);
        }
    }

	RegEntry re(::szReconcileRoot, hkeyProfile);
	if (re.GetError() == ERROR_SUCCESS) {
        SYNCSTATE s;
        s.hkeyProfile = hkeyProfile;
        s.pnlsProfilePath = &nlsProfilePath;
        s.pnlsOtherProfilePath = (nlsOtherProfilePath.strlen() != 0) ? &nlsOtherProfilePath : NULL;
        s.hkeyPrimary = NULL;

		RegEntry rePrimary(::szReconcilePrimary, re.GetKey());
		RegEntry reSecondary(::szReconcileSecondary, re.GetKey());
		if (rePrimary.GetError() == ERROR_SUCCESS) {
			ReconcileSection(rePrimary.GetKey(), &s);

			if (reSecondary.GetError() == ERROR_SUCCESS) {
                s.hkeyPrimary = rePrimary.GetKey();
				ReconcileSection(reSecondary.GetKey(), &s);
			}
		}
	}

	return ERROR_SUCCESS;
}


HRESULT DefaultReconcileKey(HKEY hkeyProfile, NLS_STR& nlsProfilePath,
                            LPCSTR pszKeyName, BOOL fSecondary)
{
    HRESULT hres = LoadShellEntrypoint();
    if (FAILED(hres))
        return hres;

	RegEntry re(::szReconcileRoot, hkeyProfile);
	if (re.GetError() == ERROR_SUCCESS) {
        SYNCSTATE s;
        s.hkeyProfile = hkeyProfile;
        s.pnlsProfilePath = &nlsProfilePath;
        s.pnlsOtherProfilePath = NULL;
        s.hkeyPrimary = NULL;

		RegEntry rePrimary(::szReconcilePrimary, re.GetKey());
		if (rePrimary.GetError() == ERROR_SUCCESS) {
            if (fSecondary) {
        		RegEntry reSecondary(::szReconcileSecondary, re.GetKey());
                s.hkeyPrimary = rePrimary.GetKey();
    			ReconcileKey(reSecondary.GetKey(), pszKeyName, &s);
            }
            else
    			ReconcileKey(rePrimary.GetKey(), pszKeyName, &s);
		}
	}

	return ERROR_SUCCESS;
}


HRESULT DeleteProfileFiles(LPCSTR pszPath)
{
    HRESULT hres = LoadShellEntrypoint();
    if (FAILED(hres))
        return hres;

    SHFILEOPSTRUCT fos;
    TCHAR szFrom[MAX_PATH];

    lstrcpy(szFrom, pszPath);

    /* Before we build the complete source filespec, check to see if the
     * directory exists.  In the case of lesser-used folders such as
     * "Application Data", the default may not have ever been created.
     * In that case, we have no contents to copy.
     */
    DWORD dwAttr = GetFileAttributes(szFrom);
    if (dwAttr == 0xffffffff || !(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
        return S_OK;

    AddBackslash(szFrom);
    lstrcat(szFrom, TEXT("*.*"));
    szFrom[lstrlen(szFrom)+1] = '\0';   /* double null terminate from string */

    fos.hwnd = NULL;
    fos.wFunc = FO_DELETE;
    fos.pFrom = szFrom;
    fos.pTo = NULL;
    fos.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI;
    fos.fAnyOperationsAborted = FALSE;
    fos.hNameMappings = NULL;
    fos.lpszProgressTitle = NULL;

    g_pfnSHFileOperationA(&fos);

    ::RemoveDirectory(pszPath);

	return NOERROR;
}


HRESULT DeleteProfile(LPCSTR pszName)
{
	RegEntry re(::szProfileList, HKEY_LOCAL_MACHINE);

	HRESULT hres;

	if (re.GetError() == ERROR_SUCCESS) {
		{	/* extra scope for next RegEntry */
			RegEntry reUser(pszName, re.GetKey());
			if (reUser.GetError() == ERROR_SUCCESS) {
				NLS_STR nlsPath(MAX_PATH);
				if (nlsPath.QueryError() == ERROR_SUCCESS) {
					reUser.GetValue(::szProfileImagePath, &nlsPath);
					if (reUser.GetError() == ERROR_SUCCESS) {
						hres = DeleteProfileFiles(nlsPath.QueryPch());
					}
					else
						hres = HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);

				}
				else
					hres = HRESULT_FROM_WIN32(nlsPath.QueryError());
			}
			else
				hres = HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);
		}
		if (SUCCEEDED(hres)) {
		    ::RegDeleteKey(re.GetKey(), pszName);
            NLS_STR nlsOEMName(pszName);
			if (nlsOEMName.QueryError() == ERROR_SUCCESS) {
			    nlsOEMName.strupr();
			    nlsOEMName.ToOEM();
    			::DeletePasswordCache(nlsOEMName.QueryPch());
            }
		}
	}
	else
		hres = E_UNEXPECTED;

	return hres;
}
