/***********************************************************************
//
// ERR.H
//
//		Copyright (c) 1992 - Microsoft Corp.
//		All rights reserved.
//		Microsoft Confidential
//
// Error values for file engine functions.
//
// johnhe - 03-13-92
***********************************************************************/

//**********************************************************************
//	Parsing errors
//**********************************************************************

#define	ERR_UNKNOWN					-1		// Unknown error
#define	ERR_UNKNOWN_SWITCH		-2		// Unknown criteria switch was specified
#define	ERR_NO_END_SEARCHSTR		-3		// No end search string delimiter
#define	ERR_DATE_FORMAT			-4		// Invalid date format
#define	ERR_TIME_FORMAT			-5		// Invalid time format
#define	ERR_DATE_COMBO				-6		// Invalid date combination
#define	ERR_SIZE_FORMAT			-7		// Invalid size format
#define	ERR_SIZE_COMBO				-8		// Invalid size combination
#define	ERR_ATTR_FORMAT			-9		// Invalid attribute format
#define	ERR_ATTR_COMBO				-10	// Invalid attribute combination
#define	ERR_MULT_SRCHSTR			-11	// Multiple search strings specified
#define	ERR_STRLST_LEN				-12	// Search string length is too long
#define	ERR_SRCH_EXPRESSION		-13	// Invalid search expression

#define	ERR_DUP_DESTINATION		-14	// Duplicate destination file specs
#define	ERR_SWITCH_COMBO			-15	// Invalid switch combination
#define	ERR_NO_SOURCE				-16	// No source filespec given
#define	ERR_NOT_ON_NAME			-17	// /NOT was applied to single filename

#define	ERR_FILESPEC_LEN			-18	// More than 1K of filespec strings
#define	ERR_NOT_FILE_LEN			-19	// More than 1K of !filespec strings

#define	ERR_INVALID_SRC_PATH		-20	// Invalid source path (not found)

#define	ERR_SOURCE_ISDEVICE		-21	// Source filespec is reserved device
#define	ERR_DEST_ISDEVICE			-22	// Dest. filespec is reserved device
#define	ERR_NO_DESTINATION		-23	// No destination was specified
#define	ERR_INVALID_PARAMETR		-24	// Extra parameter on cmd line

//**********************************************************************
//	System errors
//**********************************************************************

#define	ERR_NOMEMORY				-25	// Insuffient memory error
#define	ERR_MEM_CORRUPT			-26	// Error returned on MemFree()
#define	ERR_USER_ABORT				-27	// User aborted (CTRL+C)
#define	ERR_NOT_SUPPORTED			-28	// Unsupported callback request
#define	ERR_COLLATE_TABLE			-29	// Error on DOS call get collate table

//**********************************************************************
//	File or disk errors
//**********************************************************************

#define	ERR_BAD_FILESPEC			-50	// Bad file specification
#define	ERR_DIR_CREATE				-51	// Error creating a subdirectory entry
#define	ERR_FILE_READ				-52	// Error reading a file
#define	ERR_INVALID_DRIVE			-53	// Invalid drive specification
#define	ERR_INVALID_DESTINATION	-54	// Invalid destination filespec

//**********************************************************************
//	Copy errors
//**********************************************************************

#define	ERR_CPY_OVER_SELF			-75	// File cannot be copied over itself
#define	ERR_CLEAR_ARCHIVE			-76	// Error clearing file's archive bit
#define	ERR_RDONLY_DESTINATION	-77	// Destination file is readonly
#define	ERR_CYLINDRIC_COPY		-79	// Destination path is child of source

//**********************************************************************
//	Errors accessing a specified list file
//**********************************************************************

#define	ERR_FILELIST				-80	// Unknown error accessing the file list
#define	ERR_BAD_LISTFILE			-81	// File list was not found
#define	ERR_FILELIST_ACCESS		-82	// Sharing error accessing file list

//**********************************************************************
//	Resource load errors
//**********************************************************************

#define	ERR_READING_MSG			-100	// Error reading string resource
#define	ERR_MSG_LOADED				-101	// Strings have already been loaded

//**********************************************************************
//	Misc error values
//**********************************************************************

#define	ERR_DO_HELP					ERR_NO_SOURCE	// Display help
