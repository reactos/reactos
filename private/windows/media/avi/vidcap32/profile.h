/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1995  Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/
/****************************************************************************
 *
 *   profile.h: Registry access 
 *
 *   Vidcap32 Source code
 *
 ***************************************************************************/

/*
 * utility functions to read and write values to the profile,
 * using mmtools.ini for Win16 or current user\software\microsoft\mm tools
 * in the registry for Win32
 */

/*
 * read a BOOL flag from the profile, or return default if
 * not found.
 */
BOOL mmGetProfileFlag(LPTSTR appname, LPTSTR valuename, BOOL bDefault);

/*
 * write a boolean value to the registry, if it is not the
 * same as the default or the value already there
 */
VOID mmWriteProfileFlag(LPTSTR appname, LPTSTR valuename, BOOL bValue, BOOL bDefault);

/*
 * read a UINT from the profile, or return default if
 * not found.
 */
UINT mmGetProfileInt(LPTSTR appname, LPTSTR valuename, UINT uDefault);

/*
 * write a UINT to the profile, if it is not the
 * same as the default or the value already there
 */
VOID mmWriteProfileInt(LPTSTR appname, LPTSTR valuename, UINT uValue, UINT uDefault);

/*
 * read a string from the profile into pResult.
 * result is number of bytes written into pResult
 */
DWORD
mmGetProfileString(
    LPTSTR appname,
    LPTSTR valuename,
    LPTSTR pDefault,
    LPTSTR pResult,
    int cbResult
);


/*
 * write a string to the profile
 */
VOID mmWriteProfileString(LPTSTR appname, LPTSTR valuename, LPTSTR pData);


/*
 * read binary values from the profile into pResult.
 * result is number of bytes written into pResult
 */
DWORD
mmGetProfileBinary(
    LPTSTR appname,
    LPTSTR valuename,
    LPVOID pDefault,  
    LPVOID pResult,   // if NULL, return the required buffer size
    int cbSize);

/*
 * write binary data to the profile
 */
VOID
mmWriteProfileBinary(LPTSTR appname, LPTSTR valuename, LPVOID pData, int cbData);
		   





