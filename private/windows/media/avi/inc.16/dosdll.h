//*********************************************************************
//
//	DOSDLL.H
//
//		Copyright (c) 1992 - Microsoft Corp.
//		All rights reserved.
//		Microsoft Confidential
//
//	This header file contains the typedefs and #defines needed when
//	calling into and calling back from a DOS type DLL.
//
// PROGRAMMER CAVEAT:
//		Remember that all pointers in the DLL must be far because
//		the C runtime libraries which use near pointers always assume
//		that DS == SS which is not the case with a DLL.
//*********************************************************************

#define			CMD_YIELD		0		// Standard callback cmd to call Yield()
#define			SIGNAL_ABORT	1		// Signal to abort DLL process

//*********************************************************************
//	Windows memory allocation functions used in the implementation
// of GetMemory() and FreeMemory().
//*********************************************************************

#ifndef	GMEM_FIXED
	#define	GMEM_FIXED		0

	unsigned		_far _pascal GlobalAlloc( unsigned Flags, unsigned long Bytes );
	unsigned		_far _pascal GlobalFree( unsigned );
	void *		_far _pascal GlobalLock( unsigned );
	unsigned		_far _pascal GlobalUnlock( unsigned );
	unsigned long _far _pascal GlobalHandle( unsigned );

#endif

//#define	DllYield()	((void)(*CallBackAddr)( NULL, CMD_YIELD, 0, 0, 0, 0 ))

//*********************************************************************
// TO_DLL		- Declaration for a DLL entry function.
// TO_DLL_PTR	- Declaration for a pointer to a DLL entry function.
//*********************************************************************

// #define	TO_DLL unsigned long _far _cdecl
typedef	unsigned	long _far _cdecl TO_DLL( unsigned, unsigned, ... );

typedef	unsigned long (_far _cdecl *TO_DLL_PTR)();

//*********************************************************************
// FROM_DLL		- Declaration for a callback dispatcher function.
// FROM_DLL_PTR- Declaration for a pointer to a callback dispatcher funct.
//	FROM_DLL_ARGS- Argument prototype for callback dispatcher function.
//*********************************************************************

#define	FROM_DLL	unsigned long _loadds _far _pascal
#define  FROM_DLL_ARGS	CB_FUNC_PTR,unsigned,unsigned,unsigned long,unsigned, unsigned
typedef	unsigned long (_loadds _far _pascal *FROM_DLL_PTR)();

//*********************************************************************
//	CB_FUNC		- Declaration for a callback function.
// CB_FUNC_PTR	- Declaration for a pointer to a callback function.
//	CB_FUNC_ARGS- Argument prototype for callback function.
//*********************************************************************

#define	CB_FUNC	unsigned long _far
#define	CB_FUNC_ARGS	unsigned,unsigned,unsigned long
typedef	unsigned long (_far *CB_FUNC_PTR)();

//*********************************************************************

#define	PTR_TYPE					FPTR_TYPE
#define	SZPTR_TYPE				FSZPTR_TYPE
#define	CODE_PTR_TYPE			DWORD_TYPE

//*********************************************************************

#define	WORD_TYPE				00
#define	DWORD_TYPE				01U
#define	NPTR_TYPE				02U
#define	FPTR_TYPE				03U
#define	NSZPTR_TYPE				04U
#define	FSZPTR_TYPE				05U
#define	VARI_TYPE				07U

#define	TRANS_NONE				00
#define	TRANS_SRC				(01U << 14)
#define	TRANS_DEST				(02U << 14)
#define	TRANS_BOTH				(03U << 14)

#define	LEN_SHIFT				12

//*********************************************************************
//	DllCall()	- Entry function for call DLL.
//	CallBack()	- Function in control program which dispatches callbacks
//*********************************************************************

// TO_DLL DllCall( unsigned Cmd, unsigned ArgCount, unsigned Descriptor, ... );

// FROM_DLL CallBack( unsigned long (_far *Func)(CB_FUNC_ARGS),
//						 unsigned Cmd, unsigned uParam, unsigned long lParam,
//						 unsigned Descriptor, unsigned Size );

//*********************************************************************
//	The following #defines are used to simulate a function call from the
//	control program to the DLL. There is only one entry point into the
//	DLL so a function number is used to specify the function being called.
//	A complete and detailed description of how the transport descriptors
//	should be specified is in ENTRY.ASM.
//
// PROGRAMMER CAVEAT:
//		Remember that these are #defines and not function prototypes
//		so all references to types must be done as casts....
// 	Also remember that any pointer to data must be FAR because
//		DS is always set to the DLL  own heap on entry and SS != DS.
//*********************************************************************

#define	DLL_SIGNAL				0				// DllSignal()
#define	DLL_SET_CALLBACK		1				// SetCallBackAddr()
#define	DLL_COPY					2				// DllCopyMain()
#define	DLL_DELETE				3				// DllDeleteMain()
#define	DLL_DIR					4				// DllDirMain()

#define	DLL_GET_DISK_FREE		5				// DllGetDiskFree()
#define	DLL_LOAD_MSGS			6				// DllLoadMsgs()
#define	DLL_LOAD_STRING		7				// DllLoadString()

#define	DLL_EXT_OPEN_FILE		8				// DllDosExtOpen()
#define	DLL_OPEN_FILE			9				// DllDosOpen()
#define	DLL_READ_FILE			10				// DllDosRead()
#define	DLL_WRITE_FILE			11				// DllDosWrite()
#define	DLL_CLOSE_FILE			12				// DllDosClose()
#define	DLL_SEEK_FILE			13				// DllDosSeek()

#define	DLL_GET_MEDIA_ID		14				// DllDosGetMediaId()

#define	DLL_GET_CWD_ID			15				// DllGetCwd()
#define	DLL_SET_CWD_ID			16				// DllSetCwd()
#define	DLL_GET_DRV_ID			17				// DllGetDrive()
#define	DLL_SET_DRV_ID			18				// DllSetDrive()

#define	DLL_MKDIR				19				// DllMakeDir()
#define	DLL_RMDIR				20				// DllRemoveDir()
#define	DLL_MKDIR_TREE			21				// DllCreateDirTree()

#define	DLL_RENAME				22				// DllRenameFiles()
#define	DLL_MOVE					23				// DllRenameFiles()

#define	DLL_FIND					24				// DllFindFiles()
#define	DLL_TOUCH				25				// DllTouchFiles()
#define	DLL_ATTRIB				26				// DllAttribFiles()
#define	DLL_SET_CNTRY_INF		27				// DllSetCntryInfo()
#define	DLL_RELEASE				28				// DllReleaseInstance()

//*********************************************************************
//	DLL function which is called by the user of the DLL to set a global
//	callback address where all callback will be routed thru. The
//	callback function must follow the criteria exampled in CB_ENTRY.ASM.
//	This function will initialize the SignalValue and the DOS version
//	number.
//
//	void DllSetCallBackAddr( long (far cdecl *FuncPtr)() )
//
//	ARGUMENTS:
//		FunctPtr		- Pointer to callback entry function (TO_DLL_PTR)
//	RETURNS:
//		int			- OK
//
//*********************************************************************

#define DllSetCallBackAddr( CallBackAddr )\
				((void)(*DllEntry)( DLL_SET_CALLBACK, 1,\
				DWORD_TYPE + TRANS_NONE,\
				CallBackAddr ))

//*********************************************************************
//	DLL function to set a signal value which will cause the currently
//	executing function to abort and return an error code. If the
//	SignalValue is < 0 it will be returned returned unchanged as the
//	error code. If the value > 0 it will be considered a user abort
//	and ERR_USER_ABORT will be returned.
//
//	void DllSignal( int Signal )
//
//	ARGUMENTS:
//		Signal		- Signal value.
//	RETURNS:
//		void
//
//*********************************************************************

#define DllSignal( x )\
				((void)(*DllEntry)( DLL_SIGNAL, 1,\
				WORD_TYPE + TRANS_NONE,\
				(int)x ))

//*********************************************************************
//	Main entry point for the copy/move engine. Accepts a command line and
//	copies the files meeting the specified criteria.
//
//	int DllCopyFiles( char *szCmdLine, char *szEnvStr, CPY_CALLBACK CpyCallBack )
//	int DllMoveFiles( char *szCmdLine, char *szEnvStr, CPY_CALLBACK CpyCallBack )
//
//	ARGUMENTS:
//		szCmdLine	- Ptr to command line string, less command name
//		szEnvStr		- Ptr to optional enviroment cmd string or NULL
//		CpyCallBack	- Ptr to copy callback function.
//	RETURNS:
//		int			- OK if all files copies successfull else error code
//						  which is < 0 if a parse error and > 0 if a DOS
//						  or C runtime error.
//	
//	szCmdLine is ptr to commandline string.
//
//		"srcfiles [dest] [SrchCriteria] [/E][/M][/N][/P][/R][/S][/U][/V][/W]
//
//  	source       Specifies the file or files to be copied.
//  	             and may be substituted with /F:filename
//  	             to use filespecs from a text file.
//
//  	destination  Specifies the directory and/or filename
//  	             for the new file(s).
//
//  	SrchCriteria Any extended search criteria supported by
//  	             by the findfile engine.
//
//		/C   Confirm on overwrite of existing file.
//
//		/D   Prompt for next disk when current on is full
//
//  	/E   Copies any subdirectories, even if empty.
//
//  	/M   Turns the source files archive attribute bit off after
//  	     copying the file.
//
//  	/N   Adds new files to destination directory. "CAN" be used
//  	     with /S or /U switches.
//
//		/O   Replace existing files regardless of date not compable
//         with /N or /U.
//
//  	/P   Prompts for confirmation before copying each file.
//
//  	/R   Overwrites read-only files as well as reqular files.
//
//  	/S   Copies files from specified directory and it's
//  	     subdirectories.
//
//  	/U   Replaces (updates) only files that are older than
//  	     source files (May be used with /A)
//
//  	/V   Verifies that new files are written correctly.
//
//  	/W   Prompts you to press a key before copying. (Not implemented)
//
//  	/X   Emulate XCOPY's ablity to read as many files as possible
//  	     before writing them to the destination.
//
//	CpyCallBack is a ptr to a callback function which supports these
//	these callback functions.
//
//	int far CPY_CALLBACK)( int Func, unsigned long ulArg0, void far *pArg1,
//								  void far *pArg2, void far *pArg3 );
//
//		CB_CPY_FLGS		0x0001		// Passing back parsed copy flags
//		CB_CPY_ENVERR	0x0002		// Passing back non-fatal error
//		CB_CPY_SWITCH	0x0003		//	Passing back unrecongized switch
//		CB_CPY_ERR_STR	0x0004		// Passing back error error string
//		CB_CPY_FOUND	0x0005		// File was found and ready to copy
//		CB_CPY_FWRITE	0x0006		// Destination is about to be written
//		CB_CPY_QISDIR	0x0007		// Query user if dest is file or dir
//
//	Option bits which may be passed by by CB_CPY_FLGS are:
//
//		CPY_CONFIRM			0x0001 /C Confirm before overwrite existing file
//		CPY_EMPTY			0x0002 /E Copy empty subdirectories
//		CPY_MODIFY			0x0004 /M Set the archive bit on source
//		CPY_NEW				0x0008 /N Copy if file !exist on destination
//		CPY_EXISTING		0x0010 /O Copy over existing files only.
//		CPY_PROMPT			0x0020 /P Prompt before copying file
//		CPY_RDONLY			0x0040 /R Overwrite readonly files
//		CPY_UPDATE			0x0080 /U Copy only files new than destin
//		CPY_VERIFY			0x0100 /V Turn DOS verify to ON
//		CPY_WAIT				0x0200 /W Prompt before first file
//		CPY_XCOPY			0x0400 /X Use buffered copy.
//    CPY_FULL          0x1000 /D Prompt for next disk when current is full
//		CPY_HELP				0x0800 /? Display help
//
//*********************************************************************

#define DllCopyFiles( szCmdLine, szEnv, CB_CpyCallBack )\
				((int)(*DllEntry)( DLL_COPY, 3,\
				FSZPTR_TYPE + TRANS_SRC,\
				FSZPTR_TYPE + TRANS_SRC,\
				DWORD_TYPE + TRANS_NONE,\
				(char far *)szCmdLine,\
				(char far *)szEnv,\
				(int (far pascal *)())CB_CpyCallBack ))

//*********************************************************************

#define DllMoveFiles( szCmdLine, szEnv, CB_CpyCallBack )\
				((int)(*DllEntry)( DLL_MOVE, 3,\
				FSZPTR_TYPE + TRANS_SRC,\
				FSZPTR_TYPE + TRANS_SRC,\
				DWORD_TYPE + TRANS_NONE,\
				(char far *)szCmdLine,\
				(char far *)szEnv,\
				(int (far pascal *)())CB_CpyCallBack ))

//*********************************************************************
//	Main entry point for the file delete engine. Accepts a command line
//	and deletes the files meeting the specified criteria.
//
//	int DllDelFiles( char *szCmdLine, char *szEnvStr, DEL_CALLBACK DelCallBack )
//
//	ARGUMENTS:
//		szCmdLine	- Ptr to command line string, less command name
//		szEnvStr		- Ptr to optional enviroment cmd string or NULL
//		DelCallBack	- Ptr to delete callback function.
//	RETURNS:
//		int			- OK if all files deleted successfull else error code
//						  which is < 0 if a parse error and > 0 if a DOS
//						  or C runtime error.
//	
//	szCmdLine is ptr to commandline string.
//
//		"srcfiles [SrchCriteria] [/E] [/P] [/R] [/S] [/U]"
//
//  	source       Specifies the file or files to be deleted
//  	             and may be substituted with /F:filename
//  	             to use filespecs from a text file.
//
//  	SrchCriteria Any extended search criteria supported by
//  	             by the findfile engine.
//
//		/E   Delete empty subdirectories
//  	/P   Prompts for confirmation before copying each file
//		/R   Delete readonly files which match search criteria
//		/S   Delete files in specified path and all its subdirectories
//		/U   Alias for /A*/R/E
//
//
//	DelCallBack is a ptr to a callback function which supports these
//	these callback functions.
//
//	int far DEL_CALLBACK)( int Func, unsigned long ulArg0, void far *pArg1,
//								  void far *pArg2 )
//
//		CB_DEL_FLGS			0x0001 	// Passing back parsed delete flags
//		CB_DEL_ENVERR		0x0002	// Passing back non-fatal error
//		CB_DEL_SWITCH		0x0003	//	Passing back unrecongized switch
//		CB_DEL_ERR_STR		0x0004 	// Passing back error error string
//		CB_DEL_FOUND		0x0005 	// File was found and ready to delete
//		CB_DEL_QDELALL		0x0006	// Query user if should delete *.*
//		CB_DEL_QDELALL		0x0006	// Query user if should delete *.*
//
//*********************************************************************

#define DllDeleteFiles( szCmdLine, szEnv, CB_DelCallBack )\
				((int)(*DllEntry)( DLL_DELETE, 3,\
				FSZPTR_TYPE + TRANS_SRC,\
				FSZPTR_TYPE + TRANS_SRC,\
				DWORD_TYPE + TRANS_NONE,\
				(char far *)szCmdLine,\
				(char far *)szEnv,\
 				(int (far pascal *)())CB_DelCallBack ))

//*********************************************************************
//	Main entry point for the file dir engine. Accepts a command line
//	and does an application callback the files meeting the specified
//	criteria.
//
//	int DllDirFiles( char *szCmdLine, char *szEnvStr, DIR_CALLBACK DirCallBack )
//
//	ARGUMENTS:
//		szCmdLine	- Ptr to command line string, less command name
//		szEnvStr		- Ptr to optional enviroment cmd string or NULL
//		DirCallBack	- Ptr to dir callback function.
//	RETURNS:
//		int			- OK if all files directoried successfull else error code
//						  which is < 0 if a parse error and > 0 if a DOS
//						  or C runtime error.
//
//	szCmdLine is ptr to commandline string.
//
//		"srcfiles [SrchCriteria] [/B] [/L] [/O] [/P] [/S] [/V] [/W] [/?]
//
//  	source       Specifies the file or files to be directoried
//  	             and may be substituted with /F:filename
//  	             to use filespecs from a text file.
//
//  	SrchCriteria Any extended search criteria supported by
//  	             by the findfile engine.
//
//		Switches:								#defined value in DirFlgs
//
//		/B Display a bare listing			DIR_BARE			0x0001
//		/L	Display in lower case			DIR_LCASE		0x0002
//		/O Display in sorted order			DIR_ORDERED		0x0004
//		/P Paged output						DIR_PAGED		0x0008
//		/S Recurse subdirectories			DIR_SUBDIRS		0X0010
//		/V Display verbose information	DIR_VERBOSE		0x0020
//		/W Display wide listing				DIR_WIDE			0x0040
//		/? Display help						DIR_HELP			0x0080
//
//
//	DelCallBack is a ptr to a callback function which supports these
//	these callback functions.
//
//	int far DIR_CALLBACK)( int Func, unsigned long ulArg0, void far *pArg1,
//								  void far *pArg2 )
//
//		CB_DIR_FLGS			0x0001 	// Passing back parsed dir flags
//		CB_DIR_ENVERR		0x0002	// Passing back non-fatal error
//		CB_DIR_SWITCH		0x0003	// Passing a non-search switch
//		CB_DIR_ERR_STR		0x0004 	// Passing back an error string
//		CB_DIR_FOUND		0x0005 	// File was found and ready to delete
//		CB_DIR_ENTER		0x0006	// A search is starting on a new directory
//		CB_DIR_LEAVE		0x0007	// No more files on current directory
//		CB_DIR_NEWSEARCH  0x0008	// Starting a new srch with diff. filespec
//		CB_DIR_ENDPATH		0x0009	// End of current search path
//		CB_QUERY_ACCESS	0x000a	// Query if access date is required.
//
//*********************************************************************

#define DllDirFiles( szCmdLine, szEnv, CB_DirCallBack )\
				((int)(*DllEntry)( DLL_DIR, 3,\
				FSZPTR_TYPE + TRANS_SRC,\
				FSZPTR_TYPE + TRANS_SRC,\
				DWORD_TYPE + TRANS_NONE,\
				(char far *)szCmdLine,\
				(char far *)szEnv,\
				(int (pascal far *)())CB_DirCallBack))

//**********************************************************************
// Returns disk free information for the specified drive.
//
//	int DllGetDiskFree( int cDrvLetter, struct _diskfree_t *DrvInfo )
//
//	ARGUMENTS:
//		DrvLetter	- Drive letter to get disk free information about
//		pDrvInfo		- Ptr to a drive information structure to fill in
//	RETURNS:
//		int			- OK if no errors else errno from C runtime
//
//**********************************************************************

#define DllGetDiskFree( DriveLetter, DiskFreeStruc )\
		      ((int)(*DllEntry)( DLL_GET_DISK_FREE, 2,\
		      WORD_TYPE + TRANS_NONE,\
		      PTR_TYPE + TRANS_DEST,\
		      sizeof( struct diskfree_t ),\
		      DriveLetter,\
		      (struct diskfree_t _far *)DiskFreeStruc ))

//*********************************************************************
//	Main entry point for the file rename engine. Accepts a command line
//	and renames the files meeting the specified criteria.
//
//	int DllRenameFiles( char *szCmdLine, char *szEnvStr, REN_CALLBACK RenCallBack )
//
//	ARGUMENTS:
//		szCmdLine	- Ptr to command line string, less command name
//		szEnvStr		- Ptr to optional enviroment cmd string or NULL
//		DelCallBack	- Ptr to rename callback function.
//	RETURNS:
//		int			- OK if all files renamed successfull else error code
//						  which is < 0 if a parse error and > 0 if a DOS
//						  or C runtime error.
//	
//	szCmdLine is ptr to commandline string.
//
//		"srcfiles [SrchCriteria] [/P] [/S]"
//
//  	source       Specifies the file or files to be renamed
//  	             and may be substituted with /F:filename
//  	             to use filespecs from a text file.
//
//  	SrchCriteria Any extended search criteria supported by
//  	             by the findfile engine.
//
//  	/P   Prompts for confirmation before copying each file
//		/S   Rename files in specified path and all its subdirectories
//
//
//	DelCallBack is a ptr to a callback function which supports these
//	these callback functions.
//
//	int far REN_CALLBACK)( int Func, unsigned long ulArg0, void far *pArg1,
//								  void far *pArg2 )
//
//		CB_REN_FLGS			0x0001 	// Passing back parsed rename flags
//		CB_REN_ENVERR		0x0002	// Passing back non-fatal error
//		CB_REN_SWITCH		0x0003	//	Passing back unrecongized switch
//		CB_REN_ERR_STR		0x0004 	// Passing back error error string
//		CB_REN_FOUND		0x0005 	// File was found and ready to rename
//
//*********************************************************************

#define DllRenameFiles( szCmdLine, szEnv, CB_DelCallBack )\
				((int)(*DllEntry)( DLL_RENAME, 3,\
				FSZPTR_TYPE + TRANS_SRC,\
				FSZPTR_TYPE + TRANS_SRC,\
				DWORD_TYPE + TRANS_NONE,\
				(char far *)szCmdLine,\
				(char far *)szEnv,\
 				(int (far pascal *)())CB_DelCallBack ))

//*********************************************************************
//	Loads a group of messages from the resource file in the specified
//	file into memory for latter retrieval by LoadStr(). In addition to
//	the requested messages the function will also load all error messages
//	in the ranges of 0-0xff and 0xff00 - 0xffff (-256 thru +255) on
//	the first call to the function. The resource table is built in
//	StrTable and then sorted, and then StrBuf is allocated and the
//	resource strings are read into the buffer.
//
//	NOTE:
//		Currently this function may only be called once.
//
//	int LoadMsgs( char *szFile, unsigned uStart, unsigned uEnd )
//
//	ARGUMENTS:
//		szFile		- Ptr to .EXE file containing the messages to load
//		uStart		- Starting message number to load into memory
//		uEnd			- Ending message number to be loaded into memory.
//	RETURNS:
//		int			- OK if all messages (including normal preloaded messages)
//						  are successfully loaded, else an error code.
//	
//*********************************************************************

#define DllLoadMsgs( szFile, uStart, uEnd )\
				((int)(*DllEntry)( DLL_LOAD_MSGS, 3,\
				FSZPTR_TYPE + TRANS_SRC,\
				WORD_TYPE + TRANS_NONE,\
				WORD_TYPE + TRANS_NONE,\
				(char far *)szFile,\
				(unsigned)uStart,\
				(unsigned)uEnd ))

//*********************************************************************
//	Windows emulatation function for accessing a string resource. Copies
//	the specified string resource into a caller supplied buffer and
//	appends a terminating zero to it. Because string groups are aligned
//	standard boundaries there is normally a lot of trailing zeros which
//	we strip off after reading in a group of strings.
//
//	NOTE:
//		Currently this functions requires that the string resource was
//		previously loaded into memory by LoadMsgs() which should be called
//		once at the begining of the program to preload all required
//		messages.
//	
//	int DllLoadString( unsigned hInst, unsigned idResource, char far *szBuf,
//							 int iBufLen )
//
//	ARGUMENTS:
//		hInst			- Instance of the calling program (should be zero)
//		idResource	- The string ID as specified in the .RC file
//		szBuf			- Buffer to copy the string to
//		iBufLen		- Max characters to copy into the specified buffer
//	RETURNS:
//		int			- The number of bytes copied. This number will be
//						  0 if the specified resource cannot be located in
//						  StrTable[].
//
//*********************************************************************

#define DllLoadString( hInst, idResource, szBuf, iBufLen )\
				((int)(*DllEntry)( DLL_LOAD_STRING, 4,\
				WORD_TYPE + TRANS_NONE,\
				WORD_TYPE + TRANS_NONE,\
				FSZPTR_TYPE + TRANS_SRC,\
				WORD_TYPE + TRANS_NONE,\
				(unsigned)hInst,\
				(unsigned)idResource,\
				(char far *)szBuf,\
				(int)iBufLen ))
				

//*********************************************************************
//	Extended file open function used DOS function 6ch to open a file
//	and return a file handle.
//	
//	unsigned DllDosExtOpen(  char *szFile, unsigned uMode, unsigned uAttribs,
//									 unsigned uCreat, unsigned *uFhandle )
//	
//	ARGUMENTS:
//		szFile	- Ptr to buffer containing a fully qualified filespec string
//		uMode		- Open mode for file access and sharing. (BX)
//		uAttribs	- Attributes for file if being created. (CX)
//		uCreate	- Create action flags. (DX)
//		uFhandle- Ptr to unsigned file handle
//	RETURNS:
//		unsigned - OK if no errors and open file handle stored in *uFhandle
//					  ELSE DOS error code and errno set to C runtime erro code
//	
//*********************************************************************

#define DllDosExtOpen( szFile, uMode, uAttribs, uCreate, pHandle )\
				((int)(*DllEntry)( DLL_EXT_OPEN_FILE, 5,\
		      FSZPTR_TYPE + TRANS_SRC,\
				WORD_TYPE + TRANS_NONE,\
				WORD_TYPE + TRANS_NONE,\
				WORD_TYPE + TRANS_NONE,\
		      FPTR_TYPE + TRANS_DEST,\
		      sizeof( int ),\
		      (char far *)szFile,\
				(unsigned)uMode,\
				(unsigned)uAttribs,\
				(unsigned)uCreate,\
		      (int far *)pHandle ))

//*********************************************************************
//	See C_RUNTIME _dos_open() for complete description.
//*********************************************************************

// unsigned DosOpenFile( char far *szFileSpec, unsigned uMode, int *pHandle );
#define DllDosOpen( szFile, uMode, pHandle )\
		      ((int)(*DllEntry)( DLL_OPEN_FILE, 3,\
		      FSZPTR_TYPE + TRANS_SRC,\
				WORD_TYPE + TRANS_NONE,\
		      FPTR_TYPE + TRANS_DEST,\
		      sizeof( int ),\
		      (char far *)szFile,\
				(unsigned)uMode,\
		      (int far *)pHandle ))

//*********************************************************************
//	See C_RUNTIME _dos_read() for complete description.
//*********************************************************************

// unsigned DosReadFile( int fHandle, char far *Buf, unsigned Bytes,
//		      unsigned *puRead );
#define DllDosRead( fHandle, pBuf, Bytes, pRead )\
		      ((int)(*DllEntry)( DLL_READ_FILE, 4,\
				WORD_TYPE + TRANS_NONE,\
		      FPTR_TYPE + TRANS_DEST,\
		      Bytes,\
				WORD_TYPE + TRANS_NONE,\
		      FPTR_TYPE + TRANS_DEST,\
		      sizeof( int ),\
		      (int)fHandle,\
				(void far *)pBuf,\
				(unsigned)Bytes,\
				(unsigned far *)pRead ))

//*********************************************************************
//	See C_RUNTIME _dos_write() for complete description.
//*********************************************************************

// unsigned DosWriteFile( int fHandle, char far *Buf, unsigned Bytes,
//		      unsigned *puWrite );
#define DllDosWrite( fHandle, pBuf, Bytes, pWritten )\
		      ((int)(*DllEntry)( DLL_WRITE_FILE, 4,\
				WORD_TYPE + TRANS_NONE,\
		      FPTR_TYPE + TRANS_SRC,\
		      Bytes,\
				WORD_TYPE + TRANS_NONE,\
		      FPTR_TYPE + TRANS_DEST,\
		      sizeof( int ),\
		      (int)fHandle,\
				(void far *)pBuf,\
				(unsigned)Bytes,\
				(unsigned far *)pWritten ))

//*********************************************************************
//	See C_RUNTIME _dos_close() for complete description.
//*********************************************************************

// unsigned DosCloseFile( int fHandle );
#define	DllDosClose( fHandle )\
		      ((int)(*DllEntry)( DLL_CLOSE_FILE, 1,\
				WORD_TYPE + TRANS_NONE,\
				(int)fHandle ))


//*********************************************************************
//	Seeks to a new position in an open file using DOS function 0x42.
//	
//	unsigned _dos_seek( int fHandle, long lOffset, int iOrgin, long *plCurPos );
//	
//	ARGUMENTS:
//		fHandle	- Open DOS file handle
//		lOffset	- Offset to seek to in the file
//		iOrigin	- Origin to seek from can be:
//					  SEEK_SET From begining of file
//					  SEEK_CUR From current position if the file
//					  SEEK_END From the end of the file
//		plCurPos - Ptr to dword value where absolute position in the file will
//					  be stored after the seek
//	RETURNS:
//		unsigned - OK if no errors and open file handle stored in *uFhandle
//					  ELSE DOS error code and errno set to C runtime erro code
//	
//*********************************************************************

// DllDosSeek( int fHandle, long 0L, int SEEK_SET, long *lPos )
#define	DllDosSeek( fHandle, lPos, Type, lpNewPos )\
		      ((int)(*DllEntry)( DLL_SEEK_FILE, 4,\
				WORD_TYPE + TRANS_NONE,\
				DWORD_TYPE + TRANS_NONE,\
				WORD_TYPE + TRANS_NONE,\
		      FPTR_TYPE + TRANS_DEST,\
				sizeof( long ),\
				(int)fHandle,\
				(long)lPos,\
				(int)Type,\
				(long far *)lpNewPos ))
			
//**********************************************************************
// Fills in a media ID information structure passed by the caller.
//
//	NOTE: The _dos_getmedia_id call may return a volume id which does
//			not match that found with a _dos_findfirst() so we do the
//			_dos_findfirst() to be compatible with DOS 5.0 DIR cmd.
//
//	int GetMediaId( int cDrvLetter, struct MEDIA_ID_INF *pMediaInf )
//
//	ARGUMENTS:
//		cDrvLetter	- Drive letter to get media information about
//		pMediaInf	- Ptr to a media information structure to fill in
//	RETURNS:
//		int			- OK if no errors else errno from C runtime
//
//**********************************************************************

// DllGetMediaId( char DrvLetter, struct MEDIA_ID_INF *pMediaInf )
#define	DllGetMediaId( DrvLetter, pMediaInf )\
		      ((int)(*DllEntry)( DLL_GET_MEDIA_ID, 2,\
				WORD_TYPE + TRANS_NONE,\
		      FPTR_TYPE + TRANS_DEST,\
				sizeof( struct MEDIA_ID_INF ),\
				(char)DrvLetter,\
				(struct MEDIA_ID_INF far *)pMediaInf ))

//**********************************************************************
//	Fills in a user supplied buffer with the current directory path string
//	for a specified drive. The path does not contain the drive letter or
//	root directory specifier, ie: "dos\user\bin". To get the current
//	directory on the current drive call the function with drive
//	specified as 0.
//
//	int DllGetdCwd( int iDrive, char *szBuf )
//
//	ARGUMENTS:
//		iDrive	- Drive specifier (0=default,1=A:,2=B:,3=C:,...)
//		szBuf		- Ptr to buffer to accept path string which should be 256
//					  bytes in length.
//	RETURNS:
//		int		- OK if specified drive C runtime error code
//
//**********************************************************************

#define	DllGetdCwd( iDrive, szBuf )\
		      ((int)(*DllEntry)( DLL_GET_CWD_ID, 2,\
				WORD_TYPE + TRANS_NONE,\
		      FSZPTR_TYPE + TRANS_DEST,\
				(int)iDrive,\
				(char far *)szBuf ))


//**********************************************************************
//	Sets the working directory to that specified by a path string passed
//	by the caller. The path string may include a drive specifier and the
//	path may be relative to the current directory on the drive affected.
//
//	int DllSetCwd( char *szBuf )
//
//	ARGUMENTS:
//		szBuf	- Ptr to string which specifies the directory to change to.
//	RETURNS:
//		int		- OK if specified drive C runtime error code
//
//**********************************************************************

#define	DllSetCwd( szBuf )\
		      ((int)(*DllEntry)( DLL_SET_CWD_ID, 1,\
		      FSZPTR_TYPE + TRANS_SRC,\
				(char far *)szBuf ))

//**********************************************************************
//	Gets the current drive using DOS function 0x19. The value obtained is
//	the based 1 drive (A:=1, B:=2, C:=3, ...)
//
//	void DllGetDrive( unsigned *pDrive )
//
//	ARGUMENTS:
//		pDrive	- Pointer to unsigned value where the drive number will
//					  be stored.
//	RETURNS:
//		void
//
//**********************************************************************

#define	DllGetDrive( pDrive )\
		      ((void)(*DllEntry)( DLL_GET_DRV_ID, 1,\
		      FPTR_TYPE + TRANS_DEST,\
				sizeof( unsigned ),\
				(unsigned far *)pDrive ))

//**********************************************************************
//	Sets the current drive using DOS function 0x0e. The drive is specified
//	using base 1 so that A:=1, B:=2, C:=3, etc.
//
//	void DllSetDrive( unsigned uDrive, unsigned *pNumDrvs )
//
//	ARGUMENTS:
//		uDrive	- Drive number to set as current drive.
//		pNumDrvs	- Pointer to unsigned value where the total number of drives
//				     in the system will be store. (This is the value of
//					  lastdrive= in the config.sys).
//	RETURNS:
//		void		- No return value is passed. Use DllGetDrive() to determine
//					  if the call was successful.
//
//**********************************************************************

#define	DllSetDrive( uDrive, pNumDrvs )\
		      ((void)(*DllEntry)( DLL_SET_DRV_ID, 2,\
				WORD_TYPE + TRANS_NONE,\
		      FPTR_TYPE + TRANS_DEST,\
				sizeof( unsigned ),\
				(unsigned)(uDrive),\
				(unsigned far *)pNumDrvs ))

//**********************************************************************
//	Creates a new directory with the specified name. The string specifying
//	the name may be a fully qualified path or relative to the current
//	drive and directory.
//
//	int DllMakeDir( char *szDir )
//
//	ARGUMENTS:
//		szDir		- Ptr to path directory name string
//	RETURNS:
//		int		- OK in successfull else C runtime error code of
//					  EACCESS if directory already exists or conflicting
//					  file name, or ENOENT if the path is invalid
//
//**********************************************************************

#define	DllMakeDir( szDir )\
		      ((int)(*DllEntry)( DLL_MKDIR, 1,\
		      FSZPTR_TYPE + TRANS_SRC,\
				(char far *)szDir ))


//**********************************************************************
//	Deletes the directory with the specified name. The string specifying
//	the name may be a fully qualified path or relative to the current
//	drive and directory.
//
//	int DllRemoveDir( char *szDir )
//
//	ARGUMENTS:
//		szDir		- Ptr to directory name string
//	RETURNS:
//		int		- OK in successfull else C runtime error code of
//					  EACCESS if name given is not a directory or the
//					  directory is not empty or is the current or
//					  root directory, or ENOENT if the path is invalid.
//
//**********************************************************************

#define	DllRemoveDir( szDir )\
		      ((int)(*DllEntry)( DLL_RMDIR, 1,\
		      FSZPTR_TYPE + TRANS_SRC,\
				(char far *)szDir ))


//**********************************************************************
//	Creates a complete directory path from a caller supplied path string.
//	Any or all of the directories in the specified path may already
//	exist when the function is called. The path string may be drive or
//	UNC based and may include a trailing backslash.
//
//	int DllCreateDirTree( char *szPath )
//
//	ARGUMENTS:
//		szPath		- Fully qualified path string.
//	RETURNS:
//		int			- OK if successful else EACCES or ENOENT
//
//**********************************************************************

#define	DllCreateDirTree( szDir )\
		      ((int)(*DllEntry)( DLL_MKDIR_TREE, 1,\
		      FSZPTR_TYPE + TRANS_SRC,\
				(char far *)szDir ))



//**********************************************************************
//	Main entry point for the find/grep engine. Accepts a command line and
//	emulates the DOS FIND command.
//
//
//	int FindFiles( char *szCmdLine, FIND_CALLBACK FindCallBack )
//
//	szCmdLine is ptr to commandline string.
//
//		"srcfiles [SrchCriteria] [/V] [/C] [/N] [/I]
//
//  	source       Specifies the file or files to be finds
//  	             and may be substituted with /F:filename
//  	             to use filespecs from a text file.
//
//  	SrchCriteria Any extended search criteria supported by
//  	             by the findfile engine.
//
//		/V   Displays all lines NOT containing the specified string.
//		/C   Displays only the count of lines containing the string.
//		/N   Displays line numbers with the displayed lines.
//		/I   Ignores the case of characters when searching for the string.
//
//	FindCallBack is a ptr to a callback function which supports these
//	these callback functions.
//
//	long (far pascal *FIND_CALLBACK)( int Func, unsigned uArg0,
// 								 	 		 void far *pArg1, void far *pArg2,
//										 	 	 void far *pArg3 );
//
//		CB_FIND_FLGS		0x0001	// Passing back parsed FIND flags
//		CB_FIND_ENVERR		0x0002	// Passing back non-fatal error
//		CB_FIND_SWITCH		0x0003	//	Passing back unrecongized switch
//		CB_FIND_ERR_STR	0x0004	// Passing back error error string
//		CB_FIND_FOUND		0x0005	// File matching search criteria found
//		CB_FIND_MATCH		0x0006	// Passing back matching line from file
//		CB_FIND_COUNT		0x0007	// Passing back count of matching lines
//
//***********************************************************************

#define DllFindFiles( szCmdLine, szEnv, CB_FindCallBack )\
				((int)(*DllEntry)( DLL_FIND, 3,\
				FSZPTR_TYPE + TRANS_SRC,\
				FSZPTR_TYPE + TRANS_SRC,\
				DWORD_TYPE + TRANS_NONE,\
				(char far *)szCmdLine,\
				(char far *)szEnv,\
				(int (pascal far *)())CB_FindCallBack))


//**********************************************************************
//	File touch engine entry function. Allows setting the time/date
//	stamp on files.
//
//	int TouchFiles( char *szCmdLine, TOUCH_CALLBACK TouchCallBack )
//
//	szCmdLine is ptr to commandline string.
//
//		"srcfiles [SrchCriteria] [/TDM:mm-dd-yy[:hh:mm:ss]]
//                             [/TTA:hh:mm:ss]
//                             [/TDA:mm-dd-yy]
//
//  	source       Specifies the file or files to be touches
//  	             and may be substituted with /F:filename
//  	             to use filespecs from a text file.
//
//  	SrchCriteria Any extended search criteria supported by
//  	             by the findfile engine.
//
//    /TDM: Set last write date and optional time to specified value.
//    /TTM: Set last write time to specified value.
//    /TDA: Set last access date and optional time to specified value.
//    /TTA:	Set last access time to specified value.
//
//	TouchCallBack is a ptr to a callback function which supports these
//	these callback functions.
//
//	long (far pascal *TOUCH_CALLBACK)( int Func, unsigned uArg0,
//                                    void far *pArg1, void far *pArg2,
//                                    void far *pArg3 );
//
//    CB_TOUCH_FLGS     0x0001   // Passing back parsed TOUCH flags
//    CB_TOUCH_ENVERR   0x0002   // Passing back non-fatal error
//    CB_TOUCH_SWITCH   0x0003   //	Passing back unrecongized switch
//    CB_TOUCH_ERR_STR	0x0004   // Passing back error error string
//    CB_TOUCH_FOUND    0x0005   // File matching search criteria found
//
//***********************************************************************


#define DllTouchFiles( szCmdLine, szEnv, CB_TouchCallBack )\
				((int)(*DllEntry)( DLL_TOUCH, 3,\
				FSZPTR_TYPE + TRANS_SRC,\
				FSZPTR_TYPE + TRANS_SRC,\
				DWORD_TYPE + TRANS_NONE,\
				(char far *)szCmdLine,\
				(char far *)szEnv,\
				(int (pascal far *)())CB_TouchCallBack))


//**********************************************************************
//	File Attrib engine entry function. Allows setting access attributes
// on files
//
// int AttribFiles( char *szCmdLine, char *szEnvStr,
//                  ATTRIB_CALLBACK CB_AttrMain )
//
//	szCmdLine is ptr to commandline string.
//
//		"srcfiles [SrchCriteria] [{+|-}A] [{+|-}H] [{+|-}R] [{+|-}S]
//
//  	source       Specifies the file or files to be attribs
//  	             and may be substituted with /F:filename
//  	             to use filespecs from a text file.
//
//  	SrchCriteria Any extended search criteria supported by
//  	             by the findfile engine.
//
//    +   Sets an attribute.
//    -   Clears an attribute.
//    R   Read-only file attribute.
//    A   Archive file attribute.
//    S   System file attribute.
//    H   Hidden file attribute.
//    /S  Processes files in all directories in the specified path.
//		
//
//	AttribCallBack is a ptr to a callback function which supports these
//	these callback functions.
//
//	long (far pascal *ATTRIB_CALLBACK)( int Func, unsigned uArg0,
//                                    void far *pArg1, void far *pArg2,
//                                    void far *pArg3 );
//
//    CB_ATTRIB_FLGS     0x0001   // Passing back parsed ATTRIB flags
//    CB_ATTRIB_ENVERR   0x0002   // Passing back non-fatal error
//    CB_ATTRIB_SWITCH   0x0003   // Passing back unrecongized switch
//    CB_ATTRIB_ERR_STR	 0x0004   // Passing back error error string
//    CB_ATTRIB_FOUND    0x0005   // File matching search criteria found
//
//***********************************************************************

#define DllAttribFiles( szCmdLine, szEnv, CB_AttribCallBack )\
				((int)(*DllEntry)( DLL_ATTRIB, 3,\
				FSZPTR_TYPE + TRANS_SRC,\
				FSZPTR_TYPE + TRANS_SRC,\
				DWORD_TYPE + TRANS_NONE,\
				(char far *)szCmdLine,\
				(char far *)szEnv,\
				(int (pascal far *)())CB_AttribCallBack))

//**********************************************************************
//	Sets up the country specific information for the .DLL. The country
//	information is passed in a a buffer containg:
//
//	Offset
//	0			Case map	table 
//	256		Collate table
//	512		File name char table
//	768		Extended country information structure
//	808		END
//
//	int DllSetCntryInfo( char far *pBuf )
//
//	ARGUMENTS:
//		pBuf		- Ptr to buffer described above
//	RETURNS:
//		void
//
//**********************************************************************

#define	DllSetCntryInfo( pBuf )\
		      ((void)(*DllEntry)( DLL_SET_CNTRY_INF, 1,\
		      FPTR_TYPE + TRANS_SRC,\
				(unsigned)(808),\
				(char far *)pBuf ))


//**********************************************************************
//	Frees the instance data for the current instance of the DLL. Should
//	be the last call a Windows App makes to the DLL.
//
//	int ReleaseDataSeg( void )
//
//	ARGUMENTS:
//		NONE
//	RETURNS:
//		int		- OK if successful else ERR_MEM_CORRUPT
//
//**********************************************************************

#define	DllReleaseInstance( )\
		      ((int)(*DllEntry)( DLL_RELEASE, 0 ))
