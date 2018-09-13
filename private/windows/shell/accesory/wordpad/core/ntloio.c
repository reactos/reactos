//
// %%File:      NTLOIO.C
//
// %%Unit:      CORE/Common Conversions Code
//
// %%Author:    SMueller
//
// Copyright (C) 1993, Microsoft Corp.
//
// This file contains NT (Win32) specific low-level I/O routines.
//
// We provide wrappers for standard Win32 APIs.
//
// The routines here should work in exes and dlls.  Ideally without
// ifdefs.
//
// ToDo:
// - Open needs to concern itself with binary mode
//

#include "conv.h"
DeclareFileName

#include "ntloio.h"


//
// Local functions
//

//
// Exported APIs
//

#if defined(USEFUNCS)
/*   F   I N I T   L O I O   N T   */
/*-------------------------------------------------------------------------
    Owner: SMueller

	Initialize the LoIO package.  Essential to call this routine before
	doing any other LoIO stuff.
-------------------------------------------------------------------------*/
GLOBALBOOL _FInitLoIO_NT(VOID)
{
	// nothing currently comes to mind
	return(fTrue);
}
#endif // USEFUNCS


#if defined(USEFUNCS)
/*  F   U N I N I T   L O I O   N T   */
/*-------------------------------------------------------------------------
    Owner: SMueller

	Uninitialize the LoIO package.  Good form to call this routine
	when done LoIO stuff.
-------------------------------------------------------------------------*/
GLOBALBOOL _FUninitLoIO_NT(VOID)
{
	// nothing currently comes to mind
	return(fTrue);
}
#endif // USEFUNCS


/*   F H   O P E N   F S   N T   */
/*-------------------------------------------------------------------------
    Owner: SMueller
 
	Opens a file and returns a file handle to it, creating it
	if it doesn't already exist and oflags specifies that we should.
	If file couldn't be opened, return FI_ERROR.
	Note that oflags is specified using convio canonical flags, as
	opposed to any Windows OF_* or Win32 FILE_* values.

	Consider: using FILE_FLAG_DELETE_ON_CLOSE to support our auto-delete
	functionality
-------------------------------------------------------------------------*/
GLOBALFH _FhOpenFs_NT(CHAR* szFileSpec, OFLAGS oflags)
{
	FH		fh;
	BOOL	fCreate;
	BOOL	fTruncate;
	BOOL	fFailExists;
	BOOL	fBinary;
	BOOL	fAppend;
	DWORD	permission;
	DWORD	createmode;
	DWORD	attributes;
	DWORD	sharemode;

	// extract useful info from oflags
	// we don't do much error checking since it's been done higher up
	fCreate = oflags & FI_CREATE;
	fTruncate = oflags & FI_TRUNCATE;
	fFailExists = oflags & FI_FAILEXISTS;
	fAppend = oflags & FI_APPEND;

	// The actual mapping encoded below
	//    fCreate  &&  fTruncate  && fFailExists    ->    CREATE_NEW
	//    fCreate  &&  fTruncate                    ->    CREATE_ALWAYS
	//    fCreate  &&                fFailExists    ->    CREATE_NEW
	//    fCreate                                   ->    OPEN_ALWAYS
	//                 fTruncate  && fFailExists    ->    failure
	//                 fTruncate                    ->    TRUNCATE_EXISTING
	//                               fFailExists    ->    failure
	//                   <none>                     ->    OPEN_EXISTING
	//
	if (fCreate && fFailExists)
		createmode = CREATE_NEW;
	else if (fCreate && fTruncate)
		createmode = CREATE_ALWAYS;
	else if (fCreate)
		createmode = OPEN_ALWAYS;
	else if (fFailExists)
		return (FH)FI_ERROR;
	else if (fTruncate)
		createmode = TRUNCATE_EXISTING;
	else // none
		createmode = OPEN_EXISTING;

	// hints to file system
	attributes = FILE_FLAG_SEQUENTIAL_SCAN;
	if (oflags & FI_TEMP)
		attributes |= FILE_ATTRIBUTE_TEMPORARY;

	// the only thing we potentially care about file type is whether
	// it's text or binary.
	if (oflags & FI_RTF || oflags & FI_TEXT)
	 	fBinary = fFalse;
	else if (oflags & FI_BINARY)
	 	fBinary = fTrue;
	else
		AssertSz(fFalse, "_FhOpenFs_NT: bogus logical file type");

	// mask out values we no longer care about
	oflags &= FI_READWRITE;

	// REVIEW smueller(jimw): Why not use a switch statement here?
	//  (since OFLAGS is short, an int type)?
	// extract the main mode and map to Windows value
	sharemode = 0;
	if (oflags == FI_READ)
		{
	 	permission = GENERIC_READ;
		sharemode = FILE_SHARE_READ;
		}
	else if (oflags == FI_WRITE)
	 	permission = GENERIC_WRITE;
	else if (oflags == FI_READWRITE)
	 	permission = GENERIC_READ | GENERIC_WRITE;
	else
		AssertSz(fFalse, "_FhOpenFs_NT: bogus open mode");

	fh = CreateFile(szFileSpec, permission, sharemode, (LPSECURITY_ATTRIBUTES)0,
	                createmode, attributes, (HANDLE)NULL);

	// if open succeeded, and caller wants, position file pointer at end
	if (fh == INVALID_HANDLE_VALUE)
		{
		Debug(DWORD err = GetLastError());
		return (FH)FI_ERROR;
		}

	if (fAppend)
		{
		SetFilePointer(fh, 0, NULL, FILE_END);
		}

	return fh;
}


#if defined(USEFUNCS)
/*   F   C L O S E   F H   N T   */
/*-------------------------------------------------------------------------
    Owner: SMueller
 
	Close a file handle.  Return success/failure.
	review: check for the existence of a return code.
-------------------------------------------------------------------------*/
GLOBALBOOL _FCloseFh_NT(FH fh, OFLAGS oflags)
{
	return CloseHandle(fh);
}
#endif // USEFUNCS


/*   C B   R E A D   F H   N T   */
/*-------------------------------------------------------------------------
    Owner: SMueller
 
	Read cb bytes from file fh into buffer at pb.  Return count of
	bytes actually read, or FI_ERROR.
-------------------------------------------------------------------------*/
GLOBALLONG _CbReadFh_NT(FH fh, VOID *pb, LONG cb)
{
	LONG cbr;
	return ReadFile(fh, pb, cb, &cbr, NULL) ? cbr : FI_ERROR;
}


/*   C B   W R I T E   F H   N T   */
/*-------------------------------------------------------------------------
	Owner: SMueller

	Write cb bytes from buffer at pb to file fh.  Return count of
	bytes actually written, or FI_ERROR.
-------------------------------------------------------------------------*/
GLOBALLONG _CbWriteFh_NT(FH fh, VOID *pb, LONG cb)
{
	LONG cbw;
	return WriteFile(fh, pb, cb, &cbw, NULL) ? cbw : FI_ERROR;
}


#if defined(USEFUNCS)
/*   F C   S E E K   F H   N T   */
/*-------------------------------------------------------------------------
	Owner: SMueller

	Seek from location so, fc bytes away on file fh.  Return new
	location or FI_ERROR.
-------------------------------------------------------------------------*/
GLOBALFC _FcSeekFh_NT(FH fh, FC fc, SHORT so)
{
	return(SetFilePointer(fh, fc, NULL, so));
}
#endif // USEFUNCS


#if defined(USEFUNCS)
/*   F C   C U R R   F H   N T   */
/*-------------------------------------------------------------------------
	Owner: SMueller

	Return current file position or FI_ERROR.
-------------------------------------------------------------------------*/
GLOBALFC _FcCurrFh_NT(FH fh)
{
	// find out where we are by moving nowhere from here
	return(SetFilePointer(fh, 0, NULL, FILE_CURRENT));
}
#endif // USEFUNCS


#if defined(USEFUNCS)
/*   F C   M A X   F H   N T   */
/*-------------------------------------------------------------------------
	Owner: SMueller

	Return maximum file position (i.e. size of file, i.e. offset of EOF)
	or FI_ERROR.
-------------------------------------------------------------------------*/
GLOBALFC _FcMaxFh_NT(FH fh)
{
	return(GetFileSize(fh, NULL));
}
#endif // USEFUNCS


/*   F C   S E T   M A X   F H   N T   */
/*-------------------------------------------------------------------------
	Owner: SMueller

	Set end of file to current position.  Return new file size or FI_ERROR.
-------------------------------------------------------------------------*/
GLOBALFC _FcSetMaxFh_NT(FH fh)
{
	FC fc;

	fc = SetFilePointer(fh, 0, NULL, FILE_CURRENT);  // get current position
	return (SetEndOfFile(fh) ? fc : FI_ERROR);
}


#if defined(USEFUNCS)
/*   F   D E L E T E   S Z   N T   */
/*-------------------------------------------------------------------------
	Owner: SMueller

	Delete an existing file.  Return success/failure.
-------------------------------------------------------------------------*/
GLOBALBOOL _FDeleteSz_NT(CHAR *szFileSpec)
{
	return DeleteFile(szFileSpec);
}
#endif // USEFUNCS


#if defined(USEFUNCS)
/*   F   R E N A M E   S Z   S Z   N T   */
/*-------------------------------------------------------------------------
	Owner: SMueller

	Rename an existing file.  Supports rename across directories.
	Return success/failure.
-------------------------------------------------------------------------*/
GLOBALBOOL _FRenameSzSz_NT(CHAR *szFileSpec, CHAR *szNewSpec)
{
	return MoveFile(szFileSpec, szNewSpec);
}
#endif // USEFUNCS


/*   F   G E T   C O N V E R T E R   D I R   N T   */
/*-------------------------------------------------------------------------
    Owner: SMueller
 
    Gets the FileSpec for the directory where the currently executing
	converter file lives.  Directory will always contain a trailing
	backslash.
-------------------------------------------------------------------------*/
GLOBALBOOL _FGetConverterDir_NT(CHAR ***phszDirectory)
{
	UINT lRet;
	CHAR *psz;
	INT cbsz;

	*phszDirectory = (CHAR**)HAllocAbort(MAXPATH + 1);
	psz = **phszDirectory;
	lRet = GetModuleFileName(hInstance, psz, MAXPATH);

	if (lRet == 0 || lRet >= MAXPATH)
		{
		FreeH(*phszDirectory);
		return fFalse;
		}
	FTruncateFileSpec(psz);

	// ensure there's a trailing backslash
	cbsz = CchSz(psz);
	if (psz[cbsz - 1] != '\\')
		{
		psz[cbsz] = '\\';
		psz[cbsz + 1] = '\0';
		}

	return fTrue;
}


/*   F   G E T   T E M P   D I R   N T   */
/*-------------------------------------------------------------------------
    Owner: SMueller
 
    Gets the FileSpec for the directory where temp files are to be stored.
	Directory will always contain a trailing backslash.
-------------------------------------------------------------------------*/
GLOBALBOOL _FGetTempDir_NT(CHAR ***phszDirectory)
{
	UINT lRet;
	CHAR *psz;
	INT cbsz;
    UINT nTest;
    char rgchTest[MAXPATH + 1];

	*phszDirectory = (CHAR**)HAllocAbort(MAXPATH + 1);
	psz = **phszDirectory;
	lRet = GetTempPath(MAXPATH, psz);

	if (lRet == 0 || lRet > MAXPATH)
		{
		FreeH(*phszDirectory);
		return fFalse;
		}

	// ensure there's a trailing backslash
	cbsz = CchSz(psz);
	if (psz[cbsz - 1] != '\\')
		{
		psz[cbsz] = '\\';
		psz[cbsz + 1] = '\0';
		}

    // Copied from the conv96 project -- MikeW
    
    // if we don't have a valid temp directory (because of fouled up %TEMP%
    // and %TMP%) ...
    if ((nTest = GetTempFileName(psz, "tst", 0, rgchTest)) == 0)
        {
        // ... use preferences (Windows) directory, which is probably not
        // fouled up, and writable.
        FreeH(*phszDirectory);
        return _FGetPrefsDir_NT(phszDirectory);
        }
    else
        {
        // clean up after GetTempFileName, which actually does create the temp
        // file, but at least does a reasonably good job of deciding quickly
        // that a directory doesn't exist or isn't writable
        DeleteFile(rgchTest);
        }

    return fTrue;
}


/*   F   G E T   P R E F S   D I R   N T   */
/*-------------------------------------------------------------------------
    Owner: SMueller
 
    Gets the FileSpec for the directory where preferences files are to
    be stored.  Directory will always contain a trailing backslash.
-------------------------------------------------------------------------*/
GLOBALBOOL _FGetPrefsDir_NT(CHAR ***phszDirectory)
{
	UINT lRet;
	CHAR *psz;
	INT cbsz;

	*phszDirectory = (CHAR**)HAllocAbort(MAXPATH + 1);
	psz = **phszDirectory;
	lRet = GetWindowsDirectory(psz, MAXPATH);

	if (lRet == 0 || lRet > MAXPATH)
		{
		FreeH(*phszDirectory);
		return fFalse;
		}

	// ensure there's a trailing backslash
	cbsz = CchSz(psz);
	if (psz[cbsz - 1] != '\\')
		{
		psz[cbsz] = '\\';
		psz[cbsz + 1] = '\0';
		}

	return fTrue;
}

