/* Utilities - Win32 utilities (Windows NT and Windows '95)
   Copyright (C) 1994, 1995, 1996 the Free Software Foundation.

   Written 1996 by Juan Grigera<grigera@isis.unlp.edu.ar>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  
*/

#include <config.h>
#include <windows.h>
#include "util_win32.h"
#include "trace_nt.h"

/* int win32_GetPlatform ()
	Checks in which OS Midnight Commander is running.

  Returns:
	OS_WinNT    - Windows NT 3.x
	OS_Win95    - Windows 4.x

  Note: GetVersionEx (Win32API) is called only once.
*/
int win32_GetPlatform ()
{
    static int platform = 0;

    return (platform ? platform : (platform = win32_GetVersionEx()) );
}

/* int win32_GetVersionEx () 
   intended for use by win32_GetPlatform only 
*/
int win32_GetVersionEx ()
{
    OSVERSIONINFO ovi;

    ovi.dwOSVersionInfoSize = sizeof(ovi);
    win32APICALL( GetVersionEx(&ovi) );

    return ovi.dwPlatformId;
}

/* int win32_GetEXEType (const char* filename)
        Determines whether filename (an Executable) is
        a Console application (CUI) or a Graphical application(GUI).

        filename - Name of executable file to check

  Returns: 	EXE_win16	- Windows 3.x archive or OS/2
		EXE_win32CUI    - NT or Chicago Console API, also OS/2
		EXE_win32GUI    - NT or Chicago GUI API
		EXE_otherCUI	- DOS COM, MZ, ZM, Phar Lap
		EXE_Unknown	- Unknown
		EXE_Error	- Couldn't read file/EXE image

  TODO:   better management of OS/2 images
	  EXE_CompressedArchive can be easily implemented
  Notes:  This function parses the executable header (the only ugly way
          to do it). If header is not found or not understood,
          0 is returned.

          Information on NE, LE, LX and MZ taken from Ralf Brown's interrupt
          list, under INT 21-function 4B ("EXEC" - LOAD AND/OR EXECUTE PROGRAM),
	  Tables 0806 - 836.

	  Parsing of PE header (Win32 signature, "Portable Executable")
          taken from MSKBase article Number: Q90493.
*/

/* ---- Executable Signatures ---- */

/* Alternative DOS signagure */
#define IMAGE_DOS_SIGNATURE_ALTERNATIVE		0x4D5A      	/* ZM */

/* Phar Lap .EXP files */
#define IMAGE_OLDPHARLAP_SIGNATURE 		0x504D		/* MP */
#define IMAGE_NEWPHARLAP_286_SIGNATURE  	0x3250		/* P2 */
#define IMAGE_NEWPHARLAP_386_SIGNATURE 		0x3350		/* P3 */

/* New Executables */
#define	IMAGE_LX_SIGNATURE			0x584C		/* LX */
#define	IMAGE_PE_SIGNATURE			0x4550		/* PE */


int win32_GetEXEType (const char* a_szFileName)
{
/* FIXME: MinGW cannot compile this code */
#ifndef __MINGW32__
    HANDLE hImage;
    DWORD  dwDumm;
    DWORD  SectionOffset;
    DWORD  CoffHeaderOffset;
    WORD   wSignature;
/*    DWORD  MoreDosHeader[16]; */

    IMAGE_DOS_HEADER      image_dos_header;
    IMAGE_FILE_HEADER     image_file_header;
    IMAGE_OPTIONAL_HEADER image_optional_header;
/*    IMAGE_SECTION_HEADER  image_section_header; */

    /*  Open the EXE file - Use Native API for SHARE compatibility */
    hImage = CreateFile(a_szFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hImage == INVALID_HANDLE_VALUE)  {
        win32Trace (("win32_GetEXEType: Could not open file %s. API Error %d.", a_szFileName, GetLastError()));
	return EXE_Error;
    }

    /* Read the MZ (DOS) image header.  */
    win32APICALL( ReadFile (hImage, (LPVOID)&image_dos_header, sizeof(IMAGE_DOS_HEADER), &dwDumm, NULL) );

    switch (image_dos_header.e_magic) {
	case IMAGE_DOS_SIGNATURE:			/* MZ or ZM  */
	case IMAGE_DOS_SIGNATURE_ALTERNATIVE:
		break;

 	case IMAGE_OLDPHARLAP_SIGNATURE:		/* MP, P2, P3: Phar Lap executables */
	case IMAGE_NEWPHARLAP_286_SIGNATURE:
	case IMAGE_NEWPHARLAP_386_SIGNATURE:
		return EXE_otherCUI;

	default:
		return EXE_otherCUI;			/* Probably .COM? */
    }

    /*  Read more MS-DOS header.       */
/*    win32APICALL( ReadFile (hImage, MoreDosHeader, sizeof(MoreDosHeader)); */

    /*  Get new executable header */
    CoffHeaderOffset = SetFilePointer(hImage, image_dos_header.e_lfanew, NULL, FILE_BEGIN);
/*     + sizeof(ULONG); */
    win32APICALL( ReadFile (hImage, (LPVOID) &wSignature, sizeof(WORD), &dwDumm, NULL) );

    switch (wSignature) {
	case IMAGE_PE_SIGNATURE:		/* PE - Portable Executable  */
	    break;
	case IMAGE_OS2_SIGNATURE:		/* NE - New Executable OS/2 and Windows 3.x */
	case IMAGE_OS2_SIGNATURE_LE:		/* LE - Linear Execuable (Windows 3.x)      */
	case IMAGE_LX_SIGNATURE:		/* LX - Linear Execuable (OS/2)		    */
	    return EXE_win16;
	default:
	    return EXE_Unknown;			/* unknown New Executable or bad pointer    */

    }

    /* Continue parsing PE (COFF-like) */
    SectionOffset = CoffHeaderOffset + IMAGE_SIZEOF_FILE_HEADER + IMAGE_SIZEOF_NT_OPTIONAL_HEADER;

    win32APICALL( ReadFile(hImage, (LPVOID) &image_file_header, IMAGE_SIZEOF_FILE_HEADER, &dwDumm, NULL) );

    /*  Read optional header.   */
    win32APICALL( ReadFile(hImage, (LPVOID) &image_optional_header, IMAGE_SIZEOF_NT_OPTIONAL_HEADER, &dwDumm, NULL) );

    switch (image_optional_header.Subsystem) {
	case IMAGE_SUBSYSTEM_WINDOWS_GUI:
	    return EXE_win32GUI;

	case IMAGE_SUBSYSTEM_WINDOWS_CUI:
	case IMAGE_SUBSYSTEM_OS2_CUI:
	case IMAGE_SUBSYSTEM_POSIX_CUI:
	    return EXE_win32CUI;

	case IMAGE_SUBSYSTEM_UNKNOWN:
	case IMAGE_SUBSYSTEM_NATIVE:
	    return EXE_Unknown;			/* FIXME: what is "NATIVE??" */
	default:
            win32Trace(("Unknown type %u.\n", image_optional_header.Subsystem));
	    return EXE_Unknown;			
    }
#else
    return EXE_Unknown;
#endif /* !__MINGW32__ */
}


