#include	"mktyplib.h"

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#ifdef WIN16
#include	<toolhelp.h>
#endif //WIN16
#ifndef MAC
#include	<commdlg.h>
#endif //!MAC
#ifdef	MAC
#include	<memory.h>
#include	<quickdra.h>	// stuff for initialization
#include	<fonts.h>
#include	<windows.h>
#include	<menus.h>
#include	<dialogs.h>
#include	<script.h>      // needed for ParseTable()
#endif //MAC
#ifndef WIN32
#include	<ole2.h>
#include	"dispatch.h"
#endif //!WIN32
#include	"fileinfo.h"

#include	"errors.h"
#include	"intlstr.h"	// szBanner, szUsage, rgszErr structure defs

// external data
extern TYPLIB typlib;

// external routines
extern VOID FAR ParseOdlFile(CHAR * szInputFile);	// from PARSER.C
extern VOID FAR OutputHFile(CHAR * szHFile);		// from HOUT.C
extern VOID FAR OutputTyplib(CHAR * szTypeLibFile);	// from TYPOUT.C
extern VOID FAR CleanupImportedTypeLibs(void);		// from TYPOUT.C

#ifdef DEBUG
//UNDONE: (oleprog #557) can't use dimalloc because dstrmgr memory leaks
//#define USE_DIMALLOC		//catches memory leaks, etc
#endif //DEBUG

#ifdef WIN16
#ifdef DEBUG			// only disable alerts in the debug version
#define NODEBUGALERTS		// activate to disable debug windows alerts
#endif //DEBUG
#endif //WIN16

#ifdef	USE_DIMALLOC
extern BOOL GetDebIMalloc(IMalloc FAR* FAR* ppMalloc);	// from dimalloc.cxx
#endif

// local defines
#ifdef WIN16
#define	CB_MAX_PATHNAME 127
#else //WIN16
#define	CB_MAX_PATHNAME 255
#endif //WIN16

#define	ARGS_OK		0		// FParseCl() return values
#define	GIVE_USAGE	1

#ifdef WIN16
CHAR * szBatchStart = "@echo off\n";
CHAR * szBatchEnd = "if errorlevel 1 goto goterror\n"
		    "echo 0 >%s\n"
		    ":goterror\n";
#endif	//WIN16

// public variables used for error reporting
DWORD	lnCur=0;				// current line #
WORD	colCur=0;		// current column number
CHAR	szFileCur[CB_MAX_PATHNAME+1];	// current file name
DWORD	lnLast;			// current line #
WORD	colLast;		// current column number
SCODE	scodeErrCur;		// SCODE of current output error

CHAR *	szExpected = "";	// initially empty string


FILE * hFileInput = NULL;	// file handle for reading
FILE * hHFile = NULL;		// file handle for .H file output
FILE * hFileOutput = NULL;	// file handle for redirected output

// forward decls and public data for DBCS support
char g_rgchLeadBytes[256];
static VOID InitLeadByteTable();

// prototypes
#ifndef NO_MPW
VOID main (int argc, CHAR *argv[]);
#else	//NO_MPW
VOID main (VOID);
extern VOID FAR DumpTypeLib(CHAR * szTypeLibFile);	// from TLVIWER.CPP
#endif	//NO_MPW
#ifdef MAC
VOID MacMessageBox(CHAR * szOutput);
#endif
SHORT NEAR FParseCl(int argc, CHAR * argv[]);
VOID NEAR DoPreProcess(VOID);
CHAR * NEAR CloneNameAddExt(CHAR * szInputFile, CHAR * szExt);
VOID NEAR ErrorExit(VOID);
VOID NEAR DisplaySuccess (CHAR * szTypeLibFile);
VOID NEAR ParseErrorLnCol(ERR err, DWORD lnCur, WORD colCur);


// local data
#ifndef	MAC
HCURSOR hcrsWait;
#endif //!MAC

// data filled in from the command line
CHAR *	szInputFile = NULL;		// filename we're inputting from
CHAR *	szTypeLibFile = NULL;		// filename we're outputting to
CHAR *	szHFile = NULL;			// filename for .H file output
CHAR *	szOutputFile = NULL;		// filename for redirected output
BOOL	fHFile = FALSE;			// TRUE ==> output a .H file
BOOL	fNologo = FALSE;		// TRUE ==> don't print the header
BOOL	fGiveUsage = FALSE;		// TRUE ==> give usage screen
SYSKIND SysKind = SYS_DEFAULT;		// kind of typlib to generate
int     iAlignMax = ALIGN_MAX_DEFAULT;  // alignment max
int     iAlignDef = ALIGN_MAX_DEFAULT;  // standard alignment for the platform
BOOL	fSuppressWarnings = FALSE;	// TRUE ==> suppress warnings
#if defined(WIN16) || (defined (MAC) && !defined(_PPCMAC))
DWORD	f2DefaultCC = f2CDECL;		// default calling convention for
					// win16 & 68k Mac
#else
DWORD	f2DefaultCC = f2STDCALL;	// default calling convention for
					// win32 & ppc mac
#endif

#if	FV_CPP
BOOL	fCPP = TRUE;			// default to using the pre-processor
CHAR *  szCppExe = "cl.exe";		// default pre-processor EXE
#ifndef WIN32
HTASK	hTaskCpp;			// task notification data
#endif
CHAR *  szCppOpts = "-C -E -D__MKTYPLIB__";	// default pre-processor opts
CHAR *  szTempFile = NULL;
#ifdef WIN16
#define CB_CPPDEFS  128		// bounded by max command line length on win16
#else //WIN16
#define CB_CPPDEFS  512		// *not* bounded by max command line length.
				// 512 is big enough for all practical purposes.
#endif //WIN16
CHAR szCppDefs[CB_CPPDEFS];		// pre-processor defines/includes

#endif	//FV_CPP

BOOL	fOLEInitialized = FALSE;        // TRUE ==> OLE has been initialized
#ifdef MAC
BOOL	fAppletInitialized = FALSE;     // TRUE ==> OLE has been initialized
#endif //MAC

#ifdef	DEBUG
BOOL	fDebug = FALSE;                 // TRUE ==> dump debug info
#endif	//DEBUG


#ifndef WIN16
#define OLEINITIALIZE OleInitialize
#define OLEUNINITIALIZE OleUninitialize
#else //!WIN16
#define OLEINITIALIZE CoInitialize	// don't need all of OleInitialize
#define OLEUNINITIALIZE CoUninitialize
#endif //!WIN16

#if 0	// doesn't seem to be necessary under WIN16, WIN32 or MAC
VOID FAR pascal WinMain(HANDLE hInstanceCur, HANDLE hInstancePrev, LPSTR lpCmdLine, int nCmdShow)
{
   main(0, NULL);	// doesn't return
}
#endif //0

#ifdef WIN16
int FAR pascal WEP(int x)
{
   x = x;		// fix retail warning
   return 1;     	// success
}
#endif //WIN16

//#define KEEPTEMP		// activate to retain temp files

#ifdef	KEEPTEMP
  #define MyRemove(x)	0	// pretend no error
#else	//KEEPTEMP
  #define MyRemove remove
#endif	//KEEPTEMP

#ifdef NODEBUGALERTS
    WINDEBUGINFO Olddebuginfo;
#endif


// MkTypLib entry point
VOID main
(
#ifndef NO_MPW
    int argc,        /* Number of strings in array argv          */
    CHAR *argv[]     /* Array of command-line argument strings   */
#endif	// NO_MPW
)
{

#ifdef USE_DIMALLOC
    IMalloc FAR *pmalloc;
#endif //USE_DIMALLOC
#ifdef NO_MPW
#define MAX_ARGS 21
    int argc;        /* Number of strings in array argv          */
    CHAR *argv[MAX_ARGS];  /* Array of command-line argument strings   */
    FILE * hFileArgs;
#endif	// NO_MPW

    int fArgErr;
    HRESULT res;
#ifndef MAC
    OPENFILENAME ofn;
#endif

#ifdef NODEBUGALERTS
    WINDEBUGINFO debuginfo;

    GetWinDebugInfo(&debuginfo, WDI_OPTIONS);
    Olddebuginfo = debuginfo;		// save for restoration
    debuginfo.dwOptions |= DBO_SILENT;
    SetWinDebugInfo(&debuginfo);
#endif	//NODEBUGALERTS

    // init key fields in the main 'typlib' structure before we use them
    typlib.pEntry = NULL;	// no entries seen so far
    typlib.pImpLib = NULL;	// no imported libraries initially

#ifdef MAC
#ifdef NO_MPW
    // Do mysterious MAC init stuff
    MaxApplZone();
#endif //NO_MPW

    InitGraf((Ptr) &qd.thePort);
#ifdef NO_MPW
    InitFonts();
    InitWindows();
    InitMenus();
    InitDialogs(nil);
    InitCursor();
#endif //NO_MPW

    PPCInit();		// required by OleInitialize

    // init the OLE Applet
    if ((res = InitOleManager(0)) != NOERROR)
	ParseError(ERR_OM);		// UNDONE: correct error?
    fAppletInitialized = TRUE;

#ifdef NO_MPW
    // If a file exists called "MKTYPLIB.ARG", load up argc, argv[] to satisfy
    // our command line parser.
    if (hFileArgs = fopen("mktyplib.arg", "r"))
	{
	    argc = 1;
	    while (argc < MAX_ARGS)
		{
		argv[argc] = malloc(50);
		if (fscanf(hFileArgs, " %s ", argv[argc]) == EOF)
		    break;
		argc++;
		}
	    fclose(hFileArgs);

	}
   else
	{
	    // activate to output to file instead of using lame MAC MessageBox's
	    // szOutputFile  = "m.log";		// redirected output
	    szInputFile   = "m.odl";		// input file
	    fHFile = TRUE;			// want a .H file

	    fArgErr = FALSE;			// no arg error
	    goto ArgsParsed;
	}
#endif //NO_MPW
#endif //MAC

    InitLeadByteTable();

    fArgErr = FParseCl(argc, argv);	// parse the command line

#ifdef MAC
#ifdef NO_MPW
ArgsParsed:
#endif //NO_MPW
#endif //MAC

    if (szOutputFile)
	{
#ifdef WIN16
            // perform in-place conversion to OEM char set
            AnsiToOem(szOutputFile, szOutputFile);

            // (don't bother converting back - this string is not used again)
#endif // WIN16

	    hFileOutput = fopen(szOutputFile, "w");
	    // if problem opening output file, then just revert to normal
	    // MessageBox output.
	    // CONSIDER: give an error, too?
	}

    if (!fNologo)
	{
	    DisplayLine(szBanner);		// display the copyright banner
	    // add a blank line in some cases to make it look better
	    if (hFileOutput)
		fputs("\n", hFileOutput);
#ifndef WIN16
#ifndef NO_MPW
	    else
		printf("\n");
#endif	//NO_MPW
#endif //!WIN16
	}

    if (fArgErr || fGiveUsage)
	{
GiveUsage:
	    DisplayLine(szUsage);
	    ErrorExit();	// clean up and exit(1)
	}

#ifndef	MAC
    // use common dialog to get input filename if user didn't specify one
    if (szInputFile == NULL)
	{
	    szInputFile = malloc(CB_MAX_PATHNAME+1);
            
	    memset(&ofn, 0, sizeof(OPENFILENAME));
	    ofn.lStructSize = sizeof(OPENFILENAME);
//	    ofn.hwndOwner = g_hwndMain;
	    ofn.hwndOwner = NULL;
	    ofn.lpstrFile = szInputFile;
	    ofn.nMaxFile = CB_MAX_PATHNAME+1;
	    *szInputFile = '\0';
	    ofn.lpstrFilter = "Object Description Lang.\0*.odl\0\0";
	    ofn.nFilterIndex = 1; 
	    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	    // if anything went wrong -- just give the usage message
	    if (GetOpenFileName(&ofn) == 0)
		goto GiveUsage;
	}
#endif	//!MAC

    Assert(szInputFile);

// now compute filenames based off the input filename

    if (szTypeLibFile == NULL)	// if output file not specified
    	{   // use input filename with ".tlb" extension
	    szTypeLibFile = CloneNameAddExt(szInputFile, ".tlb");
    	}

    if (fHFile && szHFile == NULL)	// if header filename not specified
    	{   // use input filename with ".h" extension
	    szHFile = CloneNameAddExt(szInputFile, ".h");
    	}


    // If output file ends up with the same name as the input file, then
    // the user is screwed. Just give the usage message.
    if (!strcmp(szInputFile, szTypeLibFile))
	goto GiveUsage;

    // If .h file ends up with the same name as either the input file or output
    // file, then the user is screwed. Just give the usage message.
    if (szHFile && (!strcmp(szInputFile, szHFile) || !strcmp(szTypeLibFile, szHFile)))
	goto GiveUsage;

#ifdef USE_DIMALLOC

    // Use the dimalloc implementation, since the default implementation
    // doesn't work yet in mac ole.
    if (!GetDebIMalloc(&pmalloc))
      ParseError(ERR_OM);		// UNDONE: correct error?

    res = OLEINITIALIZE(pmalloc);
    pmalloc->lpVtbl->Release(pmalloc);
#else
    // must init OLE
    res = OLEINITIALIZE(NULL);
#endif

    if (FAILED(res))
	ParseError(ERR_OM);		// UNDONE: correct error?
    fOLEInitialized = TRUE;

#ifndef MAC
     hcrsWait = LoadCursor(NULL, (LPSTR)IDC_WAIT);
     SetCursor(hcrsWait);		// turn on the hourglass cursor
     // UNDONE: this doesn't always stay on in WIN16, nor does it seem to
     // UNDONE: have any affect in WIN32.
#endif //!MAC

#if FV_CPP
    if (fCPP)			// if we're to pre-process input file
	DoPreProcess();
#endif	//FV_CPP

    strcpy(szFileCur, szInputFile);	// init current file name (for
					// error reporting)

#if FV_CPP
    ParseOdlFile(fCPP ? szTempFile : szInputFile);	// parse the input file
#else
    ParseOdlFile(szInputFile);		// parse the input file
#endif

#if FV_CPP
    if (szTempFile)
	{
	    SideAssert(MyRemove(szTempFile) == 0);	// delete tmp file created above
	    szTempFile = NULL;
	}
#endif	//FV_CPP

    if (fHFile)				// output .H file if desired
	OutputHFile(szHFile);

    // Now emit the type library
    OutputTyplib(szTypeLibFile);

#ifdef NO_MPW
    // Now dump the type library
    DumpTypeLib(szTypeLibFile);
#endif

    CleanupImportedTypeLibs();		// release any imported typelibs

    OLEUNINITIALIZE();			// terminate OLE

#ifdef MAC
    UninitOleManager();			// clean up applet
#endif //MAC

    DisplaySuccess(szTypeLibFile);	// holy *&*%&^%, it worked!!!

    if (hFileOutput)			// close redirected output file
	fclose(hFileOutput);

#ifdef NODEBUGALERTS
    SetWinDebugInfo(&Olddebuginfo);
#endif	//NODEBUGALERTS
    exit(0);

}

// helper to print the "success" message -- moved out of line to reduce
// stack usage of main routine.
VOID NEAR DisplaySuccess
(
    CHAR * szTypeLibFile
)
{
    CHAR szBuf[255];

    sprintf (szBuf, szFmtSuccess, szTypeLibFile);
    DisplayLine(szBuf);			// then display the message
}


VOID FAR ParseErrorTokLast
(
    ERR err
)
{
    ParseErrorLnCol(err, lnLast, colLast);
}

// Cleans up, and reports an error.  Doesn't return unless this is a warning.
VOID FAR ParseError
(
    ERR err
)
{
    ParseErrorLnCol(err, lnCur, colCur);
}

// Cleans up, and reports an error.  Doesn't return unless this is a warning.
// Reports an error.  If this is a warning, returns.
// If this is an error, cleans up and exits
VOID NEAR ParseErrorLnCol
(
    ERR err,
    DWORD lnCur,
    WORD colCur
)
{
    CHAR szError[255];
    BOOL fWarning;

    fWarning = ((err >= WARN_FIRST && err < GENERAL_ERR_LAST) || 
                (err >= PWARN_FIRST && err < OERR_FIRST));

    if (fWarning && fSuppressWarnings)
	return;

    // first figure out the error text
    if (err < OERR_FIRST)
	{	// parser/lexer error or warning
	    Assert(err);
	    sprintf (szError,
		     (fWarning ? szFmtWarnFileLineCol : szFmtErrFileLineCol),
		     szFileCur, lnCur, lnCur, colCur, rgszErr[err-1], szExpected);
	}
    else
	{
	    // output errors shoudn't come through here.
	    Assert(err >= ERR_FIRST && err < GENERAL_ERR_LAST);
	    sprintf (szError,
		     (fWarning ? szFmtWarnGeneral : szFmtErrGeneral),
		     rgszErr[err-1]);
	}

    // display error/warning
    DisplayLine(szError);

    // If not a warning, then clean up and exit(1)
    if (!fWarning)
        ErrorExit();
}

// Cleans up, and reports an error with an insertion string.  Doesn't return.
VOID FAR ItemError
(
    CHAR * szErrFormat,
    LPSTR lpszItem,
    ERR err
)
{
    CHAR szItem[255];
    CHAR szBuf[255];

    Assert(lpszItem);
    _fstrcpy(szItem, lpszItem); 	// copy item name near

    // format the error
    sprintf (szBuf, szErrFormat, szFileCur, szItem, rgszErr[err-1], scodeErrCur);

    // then display the error
    DisplayLine(szBuf);

    // then clean up and exit(1)
    ErrorExit();
}


VOID NEAR ErrorExit()
{

    // first clean up whatever mess we've left behind
    if (hFileInput)			// in error during reading
	fclose(hFileInput);		//   then close the input file

#if FV_CPP
    if (szTempFile)			// if error during pre-processing
	{				// or parsing, delete tmp file
	    MyRemove(szTempFile);
	}
#endif	//FV_CPP

    if (hHFile)				// if error during write of .H file
	{
	    fclose(hHFile);		//  then close the .H file
	    MyRemove(szHFile);		//  and delete partially-generated file
	}

    if (hFileOutput)			// close redirected output file
	fclose(hFileOutput);

    CleanupImportedTypeLibs();		// release any imported typelibs

    if (fOLEInitialized)		// terminate OLE
	OLEUNINITIALIZE();

#ifdef MAC
    if (fAppletInitialized)		// terminate OLE
       UninitOleManager();		// clean up applet
#endif //MAC

#ifdef NODEBUGALERTS
    SetWinDebugInfo(&Olddebuginfo);
#endif	//NODEBUGALERTS
    exit(1);			// exit with error
}


// creates a new pathame string that is a dup of the input pathname, with the
// extension (if any) replaced by the given extension.
CHAR * NEAR CloneNameAddExt
(
    CHAR * szInputFile,
    CHAR * szExt
)
{
    CHAR * szFile;
    CHAR * pExt;
    CHAR * pch;

    // CONSIDER:
    // assumes the extension is no more than 3 bytes incl '.' (ie non-DBCS)
    // (This is true for the moment - can it ever change???)

    // alloc string with enough space for ".", extension, and null
    szFile = malloc(strlen(szInputFile)+1+3+1);
    strcpy(szFile, szInputFile); 	// start with input file name

    // find "." (if present) that occurs after last \ or /.
    pExt = NULL;
    for (pch = szFile; *pch; NextChar(pch))
	{
	    switch (*pch)
		{
#ifdef	MAC
		case ':':
		case ' ':       // start search over at a space
#else	//MAC
		case '\\':
		case '/':
#endif	//MAC
		    pExt = NULL;
		    break;
		case '.':
		    pExt = pch;
		    break;
		default:
		    ;
		}
	}
    if (pExt == NULL)	// if no extension after last '\', then
	pExt = pch;	// append an extension to the name.

    strcpy (pExt, szExt);	// replace extension (if present) with
				// desired extension

    return szFile;
}

#if FV_CPP

#ifdef WIN32
// created this from the WIN32 docs, using info on WinExec and CreateProcess
// WARNING: the example given in the WIN32 docs on CreateProcess is bogus.
VOID NEAR DoPreProcess
(
)
{
    char szBuffer[512];
    BOOL fSuccess;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    DWORD  dw;
    DWORD  dwExitCode;
    HANDLE hProcess;
    HANDLE hOutput;

    memset (&si, 0, sizeof(si));
    memset (&pi, 0, sizeof(pi));
    memset (&sa, 0, sizeof(sa));

    szTempFile = strdup(tempnam(".", "~mki"));	// for pre-processed output

    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    hOutput = CreateFile(szTempFile, GENERIC_WRITE, 0, &sa,
			 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    if (hOutput == INVALID_HANDLE_VALUE)
	ParseError(ERR_CPP);

    sprintf(szBuffer, "%s %s%s %s",
		   szCppExe, szCppOpts, szCppDefs, szInputFile);

    // init si structure
    si.cb = sizeof(si);
    si.lpTitle = "MkTypLib: C pre-processor";
    //si.dwX = 100;		// just guessing...
    //si.dwY = 100;
    //si.dwXSize = 1000;
    //si.dwYSize = 1000;
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hOutput;
    //si.hStdError = hOutputError;	// CONSIDER: show this to the user?
    //si.wShowWindow = SW_SHOWMINIMIZED;
    si.wShowWindow = SW_HIDE;

    // Setting the Inherit Handles flag to TRUE so that it works when
    // run under nmake with our output redirected. 
    fSuccess = CreateProcess(NULL, szBuffer, NULL, NULL, TRUE,
			     // this gives us a window'ed CL
			     //DETACHED_PROCESS,
			     0,

			     NULL, NULL,
			     &si, &pi);

    if (!fSuccess) {
        CloseHandle(hOutput);
	ParseError(ERR_CPP);
    }

    // if we were successful, now wait for it to be done
    hProcess = pi.hProcess;

    // wait for the process to complete (120 second timeoout)
    dw = WaitForSingleObject(pi.hProcess, 120000L);

#ifndef STATUS_SUCCESS
	#define STATUS_SUCCESS 0
#endif

    if (dw == STATUS_SUCCESS)
	fSuccess = GetExitCodeProcess(pi.hProcess, &dwExitCode);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(hOutput);

    // report any error
    if (dw != STATUS_SUCCESS || !fSuccess || dwExitCode != 0) 
	ParseError(ERR_CPP);
}

#else  //WIN32

// pre-process input file, creating a tmp file
// In order to get the return code of the C pre-processor, I'm
// spawing a batch file that invokes CL, and creates a signal file
// if it is successful.
VOID NEAR DoPreProcess
(
)
{
    char szBuffer[255];

    HANDLE hInstCpp;
    TASKENTRY taskentry;
    FILE * hFile;
    CHAR * szTempBatch;
    CHAR * szTempSig;
    CHAR * szTempRoot;
    ERR err = ERR_CPP;		// assume error
    int cbTempFilenames;
    char * szComSpec;

    // figure out the names of the temp files
    // (note: uses OEM char set)
    szTempRoot = tempnam(".", "~mkt");		// get base name for temp files
    hFile = fopen(szTempRoot, "w");		// create the file now, to
    if (hFile == NULL)				// reserve this set of names
	ParseError(ERR_CPP);
    fclose(hFile);

    cbTempFilenames = strlen(szTempRoot)+1+3+1;	// base name + ext + null
    szTempBatch = malloc(cbTempFilenames);	// for .BAT file
    strcpy(szTempBatch, szTempRoot);
    strcat(szTempBatch, ".bat");

    szTempSig = malloc(cbTempFilenames);	// for .SIG file
    strcpy(szTempSig, szTempRoot);
    strcat(szTempSig, ".sig");

    szTempFile = malloc(cbTempFilenames);	// for pre-processed oupput 
    strcpy(szTempFile, szTempRoot);
    strcat(szTempFile, ".inp");

    // CONSIDER: Check for existence of any of these files, if any exist, then
    // CONSIDER: try a different base name for the files.

    // open the temp .BAT file
    hFile = fopen(szTempBatch, "w");
    if (hFile == NULL)
	goto cleanup2;

    // all errors after this point should go to 'cleanup'

    if (fputs(szBatchStart, hFile) < 0)	// write the first part
	goto cleanup;

    sprintf(szBuffer, "%s %s%s %s>",
		   szCppExe, szCppOpts, szCppDefs, szInputFile);

    // convert this string to the OEM char set
    AnsiToOem(szBuffer, szBuffer);

    // append szTempFile
    strcat(szBuffer, szTempFile);
    strcat(szBuffer, "\n");
  
    if (fputs(szBuffer, hFile) < 0)		// write the CPP command
	goto cleanup;
  
    sprintf(szBuffer, szBatchEnd, szTempSig);
    if (fputs(szBuffer, hFile) < 0)	// write the error check code
	goto cleanup;

    fclose(hFile);
    hFile = NULL;		// file no longer open

    szComSpec = getenv("COMSPEC");
    if (szComSpec == NULL)
	szComSpec = "command.com";

    sprintf(szBuffer, "%s /c %s", szComSpec, szTempBatch);
    hInstCpp = WinExec(szBuffer, SW_SHOWMINIMIZED);   // shell the pre-processor
    if (hInstCpp < 32)		// if error spawning pre-processor
	goto cleanup;

    Yield();			// give it a chance to start

    // find task associated with this instance.  In extreme cases it may have
    // finished even before we're executing this code.
    taskentry.dwSize = sizeof(TASKENTRY);
    if (TaskFirst(&taskentry) == 0) {
	goto taskdone;
    }

    while (taskentry.hInst != hInstCpp) {
        if (TaskNext(&taskentry) == 0) {
	    goto taskdone;
	}
    }

    hTaskCpp = taskentry.hTask;

    while (IsTask(hTaskCpp))
	{
	    SideAssert(TaskFindHandle(&taskentry, hTaskCpp) != 0);
	    if (taskentry.hInst != hInstCpp)
		{
		    // different hInst associated with this htask,
		    // so the app must have terminated
		    break;
		}

	    Yield();		// wait until it's done

	}

taskdone:

    // If signal file doesn't exist, then there was a problem pre-processing
    // the input file.  If it exists, then it worked.
    if (!MyRemove(szTempSig))
	err = ERR_NONE;		// it worked!

cleanup:
    if (hFile)					// close tmp batch file if
	fclose(hFile);				// error during write
    SideAssert(MyRemove(szTempBatch) == 0);	// delete tmp batch file

cleanup2:
    SideAssert(MyRemove(szTempRoot) == 0);	// delete placeholder file

    if (err != ERR_NONE)			// report any error
	ParseError(err);
}

#endif // WIN32
#endif // FV_CPP


// ******************************************************
// FParseCl() -  Commandline argument parser
//
// Purpose:
//	Parse the command line
//
// Entry:
//	None
//
// Exit:
//	Returns ARGS_OK or GIVE_USAGE
// ******************************************************
SHORT NEAR FParseCl
(
int	argc,
CHAR	*argv[]
)
{
    CHAR *arg;			// pointer to current argument
    SHORT  i;			// argument counter
    BOOL fAlignSpecified = FALSE; // TRUE iff the /align switch was used

#if FV_CPP
    *szCppDefs = '\0';		// no definitions initially
#endif //FV_CPP

    for (i = 1; i < argc; i++)
	{	// while more args to get
	    arg = argv[i];		// get next argument

	    if (*arg == '/' || *arg == '-')	// a switch
		{
GotArg:
		    arg++;		// skip switch

		    if (!stricmp(arg,"?") || !(stricmp(arg, "help")))
			fGiveUsage = TRUE; // give help

		    else if (!stricmp(arg,"nologo"))
			fNologo = TRUE;  // no banner

		    else if (!stricmp(arg,"win16"))
                        {
			    SysKind = SYS_WIN16;   // make Win16 type library
			    f2DefaultCC = f2CDECL;
                            if (!fAlignSpecified)
                                iAlignMax = 1;
                            iAlignDef = 1;
                        }

		    else if (!stricmp(arg,"win32"))
                        {
			    SysKind = SYS_WIN32;   // make Win32 type library
			    f2DefaultCC = f2STDCALL;
                            if (!fAlignSpecified)
                                iAlignMax = 4;
                            iAlignDef = 4;
                        }

		    else if (!stricmp(arg,"mac"))
                        {
			    SysKind = SYS_MAC;     // make Mac type library
			    f2DefaultCC = f2CDECL;
                            if (!fAlignSpecified)
                                iAlignMax = 2;
                            iAlignDef = 2;
                        }

		    else if (!stricmp(arg,"mips"))
                        {
                            SysKind = SYS_WIN32;
			    f2DefaultCC = f2STDCALL;
                            if (!fAlignSpecified)
                                iAlignMax = 8;
                            iAlignDef = 8;
                        }
		    else if (!stricmp(arg,"alpha"))
                        {
                            SysKind = SYS_WIN32;
			    f2DefaultCC = f2STDCALL;
                            if (!fAlignSpecified)
                                iAlignMax = 8;
                            iAlignDef = 8;
                        }

		    else if (!stricmp(arg,"ppc32"))
                        {
                            SysKind = SYS_WIN32;
			    f2DefaultCC = f2STDCALL;
                            if (!fAlignSpecified)
                                iAlignMax = 8;
                            iAlignDef = 8;
                        }

		    else if (!stricmp(arg,"ppc"))
                        {
                            SysKind = SYS_MAC;
			    f2DefaultCC = f2STDCALL;
                            if (!fAlignSpecified)
                                iAlignMax = 8;
                            iAlignDef = 8;
                        }

                    else if (!stricmp(arg,"align"))     // set alignment val
                        {   // next arg is integer val
                            if (++i == argc)    // no more args!
                                return GIVE_USAGE;
                            iAlignMax = atoi(argv[i]);
                            if (iAlignMax < 1)
                                return GIVE_USAGE;
                            fAlignSpecified = TRUE;
                        }
		    else if (!stricmp(arg,"w0"))
			fSuppressWarnings = TRUE;  // no warnings
#ifdef	DEBUG
		    else if (!stricmp(arg,"debug"))
			fDebug = TRUE;  // dump debug info
#endif //DEBUG
		    else if (!stricmp(arg,"tlb"))
			{   // next arg should be filename -- get it
			    if (++i == argc)	// error if no more args
				return GIVE_USAGE;
			    szTypeLibFile = strdup(argv[i]); // save filename
			}
		    else if (!stricmp(arg,"h"))
			{   // next arg should be filename -- get it
			    fHFile = TRUE;	// want a .H file
			    if (++i == argc)	// we're done if no more args
				break;
			    arg = argv[i];	// get next argument

			    // if this is another arg, then just process it
			    if (*arg == '/' || *arg == '-')
				goto GotArg;
			    szHFile = strdup(arg);	// save .H filename
			}
		    else if (!stricmp(arg,"o"))
			{   // next arg should be filename -- get it
			    if (++i == argc)	// error if no more args
				return GIVE_USAGE;
			    szOutputFile = strdup(argv[i]);	// save filename
			}
#if FV_CPP
		    else if (*arg == 'D' || *arg == 'I')
		        {	// append this /D or /I switch to szCppDefs

			    strcat(szCppDefs, " ");	// append a space
			    strcat(szCppDefs, argv[i]); // save /D, /I switch

			    if (*(arg+1) == '\0')
				{ // /D or /I specified by themselves, then
				  // next arg is what's important -- append
				  // that, too
				    if (++i == argc)	// error if no more args
					return GIVE_USAGE;
				    strcat(szCppDefs, " ");	// add a space
				    strcat(szCppDefs, argv[i]); // add CPP defs
				}
			    // I don't feel like adding to ton of code to keep
			    // us from overflowing the buffer.  Instead, we've
			    // got a really big buffer, so if we overwrite it,
			    // give an error & hope we don't crash on exit.
			    if (strlen(szCppDefs) >= sizeof(szCppDefs)-1) {
				return GIVE_USAGE;
			    }
		        }
		    else if (!stricmp(arg,"cpp_cmd"))
			{   // next arg should be CPP pathname -- get it
			    if (++i == argc)	// error if no more args
				return GIVE_USAGE;
			    szCppExe = strdup(argv[i]); // save pathname
			}
		    else if (!stricmp(arg,"cpp_opt"))
		        {  // next arg should be CPP options (in quotes)
			    if (++i == argc)	// error if no more args
				return GIVE_USAGE;
			    szCppOpts = strdup(argv[i]); // get CPP options
		        }
		    else if (!stricmp(arg,"nocpp"))
			fCPP = FALSE;	// don't use C pre-processor
#endif //FV_CPP
		    else	// invalid switch
			return GIVE_USAGE;
		}
	    else
		{	//  not a switch -- should be a filename
		    if (szInputFile)		// only 1 filename allowed
			return GIVE_USAGE;
		    szInputFile = strdup(arg);	// store input filename
		}
	} // end for

#ifndef MAC
    // doesn't matter if filename not given -- common dialog will take care
    // of it later.
    return ARGS_OK;
#else	//!MAC
    if (szInputFile)		// if filename specified, then success
	return ARGS_OK;

    return GIVE_USAGE;		// no file name given -- error
#endif	//!MAC

}


// display a line to the user.  Assumes line doesn't end with a newline.
VOID FAR DisplayLine(CHAR * szOutput)
{
    if (hFileOutput)
	{
#ifndef MAC
            // convert szOutput in-place to OEM char set
            AnsiToOem(szOutput, szOutput);
#endif // !MAC

	    fputs(szOutput, hFileOutput);
	    fputs("\n", hFileOutput);

#ifndef MAC
            // convert back to ANSI in case the caller reuses this string
            OemToAnsi(szOutput, szOutput);
#endif // !MAC

	}
    else
	{
#ifdef	NO_MPW
	    MacMessageBox(szOutput);
#else	//NO_MPW
#ifndef WIN16

#ifndef MAC
            // convert szOutput in-place to OEM char set
            AnsiToOem(szOutput, szOutput);
#endif // !MAC

            printf("%s\n", szOutput);

#ifndef MAC
            // convert back to ANSI in case the caller reuses this string
            OemToAnsi(szOutput, szOutput);
#endif // !MAC

#else	//!WIN16
	    // poor man's output under Windows
	    MessageBox(NULL, szOutput, szAppTitle, MB_OK);
#endif	//!WIN16
#endif	//NO_MPW
	}
}


#ifdef	DEBUG

// not used in non-dimalloc versions, but it simplifies the link process
//#ifdef USE_DIMALLOC
/***
*DebAssertShow - called when assertion fails
*Purpose:
*   This function is called when an assertion fails.  It prints
*   out the appropriate information and exits.
*
*Entry:
*   szFileName - filename where assertion failed
*   uLine - line number where assertion failed
*   szComment - reason assertion failed
*
*Exit:
*   None.
*
***********************************************************************/

void DebAssertShow(LPSTR lpszFileName, UINT uLine, LPSTR szComment)
{
    CHAR szFileName[255];
    _fstrcpy(szFileName, lpszFileName);		// copy near

    AssertFail(szFileName, (WORD)uLine);
}

//#endif // USE_DIMALLOC



VOID AssertFail
(
    CHAR * szFile,
    WORD  lineNo
)
{
    CHAR szAssert[256];
#ifndef MAC
    int id;
#endif //!MAC

#ifdef MAC
    *szAssert = sprintf(szAssert+1, "Typelib assertion: File %s, line %d", szFile, lineNo);
#else	//MAC
    sprintf(szAssert, "Assertion failed.  File %s, line %d.", szFile, lineNo);

#endif	//MAC

    if (hFileOutput)
	{
	    fputs(szAssert, hFileOutput);
	    fputs("\n", hFileOutput);
	    ErrorExit();	// clean up and exit(1)
	}
    else
	{
#ifdef	MAC
            // Can't use the the Ole2 internal assertion mechanism anymore,
	    // because if we're using the retail OLE, it doesn't assert.
	    //  So we just break into macsbug ourselves.
	    //FnAssert(lpstrExpr, lpstrMsg, lpstrFileName, iLine);
	    DebugStr((const unsigned char FAR*)szAssert);
	    // ErrorExit();	// don't clean up and exit(1)
#else
	    id = MessageBox(NULL, szAssert, "MkTypLib Assertion.  OK to continue, CANCEL to quit.", MB_OKCANCEL);
	    if (id == IDCANCEL)
	        ErrorExit();	// clean up and exit(1)
#endif	//MAC
	}
}

#if 0
// output a string, debug version only.  Assumes string ends with a newline.
VOID DebugOut
(
    CHAR * szOut
)
{

#ifndef MAC
    int id;
#endif //!MAC

    if (hFileOutput)
	{
	    fputs(szOut, hFileOutput);
	    fputs("\n", hFileOutput);
	}
    else
	{
#ifdef	MAC
	    MacMessageBox(szOut);
#else	//MAC
	    id = MessageBox(NULL, szOut, "MkTypLib debug output.  OK to continue, CANCEL to quit.", MB_OKCANCEL);
	    if (id == IDCANCEL)
		ErrorExit();		// clean up and exit(1)
#endif	//MAC
	}
}
#endif //0

#endif	//DEBUG

#ifdef	MAC
#ifdef NO_MPW
// UNDONE: eventually rip this & and it's resource from the MPW mktyplib build
VOID MacMessageBox
(
    CHAR * szOutput
)
{
    BYTE szBufTmp[256];

    // convert to pascal-style string
    *szBufTmp = (BYTE)(min(strlen(szOutput), 255));
    strncpy(szBufTmp+1, szOutput, *szBufTmp);

    // put up the alert
    ParamText((ConstStr255Param)szBufTmp, "", "", "");
    Alert(128, NULL);
}
#endif //NO_MPW
#endif	//MAC


// initialize g_rgchLeadBytes
//
// use the Windows API IsDBCSLeadByte() or the Mac API ParseTable()
//
static VOID InitLeadByteTable()
{
#if MAC

    // as in Silver, the font must be switched first

    // preserve the old font in the current grafPort
    short fontSave = qd.thePort->txFont;

    // then set the grafPort font to 1 (Application Default) because
    // ParseTable uses qd.thePort->txFont to determine the currect script,
    // which in turn determines the lead byte table.  We want the application
    // default table.
    TextFont(1);

    // UNDONE: is it necessary to init this to 0?
    memset(g_rgchLeadBytes, 0, 256);

    ParseTable(g_rgchLeadBytes);

    // restore the old font
    TextFont(fontSave);
   
#else  // !MAC

    int c;

    memset(g_rgchLeadBytes, 0, 128);

    // start at 128 since there aren't any lead bytes before that
    for(c = 128; c < 256; c++) {
        g_rgchLeadBytes[c] = (char)IsDBCSLeadByte((char)c);
    }

#endif // !MAC
}

// perform AnsiNext on all platforms
extern char * XStrInc(char * pch)
{
    return IsLeadByte(*pch) ? pch + 2 : pch + 1;
}

