/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WKERNEL.C
 *  WOW32 16-bit Kernel API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wkernel.c);


ULONG FASTCALL WK32WritePrivateProfileString(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ pszSection;
    PSZ pszKey;
    PSZ pszValue;
    PSZ pszFilename;
    register PWRITEPRIVATEPROFILESTRING16 parg16;
    BOOL fIsWinIni;
    CHAR szLowercase[MAX_PATH];

    GETARGPTR(pFrame, sizeof(WRITEPRIVATEPROFILESTRING16), parg16);
    GETPSZPTR(parg16->f1, pszSection);
    GETPSZPTR(parg16->f2, pszKey);
    GETPSZPTR(parg16->f3, pszValue);
    GETPSZPTR(parg16->f4, pszFilename);

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    strcpy(szLowercase, pszFilename);
    WOW32_strlwr(szLowercase);

    fIsWinIni = IS_WIN_INI(szLowercase);

    // Trying to install or change default printer to fax printer?
    if (fIsWinIni &&
        pszSection &&
        pszKey &&
        pszValue &&
        !WOW32_stricmp(pszSection, szDevices) &&
        IsFaxPrinterWriteProfileString(pszSection, pszKey, pszValue)) {

        ul = TRUE;
        goto Done;
    }

    ul = GETBOOL16( WritePrivateProfileString(
             pszSection,
             pszKey,
             pszValue,
             pszFilename
             ));

    if( ul != 0 &&
        fIsWinIni &&
        IS_EMBEDDING_SECTION( pszSection ) &&
        pszKey != NULL &&
        pszValue != NULL ) {

        UpdateClassesRootSubKey( pszKey, pszValue);
    }

Done:
    FREEPSZPTR(pszSection);
    FREEPSZPTR(pszKey);
    FREEPSZPTR(pszValue);
    FREEPSZPTR(pszFilename);
    FREEARGPTR(parg16);

    return ul;
}


ULONG FASTCALL WK32WriteProfileString(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ   pszSection;
    PSZ   pszKey;
    PSZ   pszValue;
    register PWRITEPROFILESTRING16 parg16;

    GETARGPTR(pFrame, sizeof(WRITEPROFILESTRING16), parg16);
    GETPSZPTR(parg16->f1, pszSection);
    GETPSZPTR(parg16->f2, pszKey);
    GETPSZPTR(parg16->f3, pszValue);

    // For WinFax Lite install hack. Bug #126489  See wow32fax.c
    if(!gbWinFaxHack) { 

        // Trying to install or change default printer to fax printer?
        if (pszSection &&
            pszKey &&
            pszValue &&
            !WOW32_stricmp(pszSection, szDevices) &&
            IsFaxPrinterWriteProfileString(pszSection, pszKey, pszValue)) {
    
            ul = TRUE;
            goto Done;
        }
 
    } else {
        IsFaxPrinterWriteProfileString(pszSection, pszKey, pszValue);
    }

    ul = GETBOOL16( WriteProfileString(
             pszSection,
             pszKey,
             pszValue
             ));

    if( ( ul != 0 ) &&
        IS_EMBEDDING_SECTION( pszSection ) &&
        ( pszKey != NULL ) &&
        ( pszValue != NULL ) ) {
        UpdateClassesRootSubKey( pszKey, pszValue);
    }

Done:
    FREEPSZPTR(pszSection);
    FREEPSZPTR(pszKey);
    FREEPSZPTR(pszValue);
    FREEARGPTR(parg16);
    return ul;
}


ULONG FASTCALL WK32GetProfileString(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ pszSection;
    PSZ pszKey;
    PSZ pszDefault;
    PSZ pszReturnBuffer;
    UINT cchMax;
#ifdef FE_SB
    PSZ pszTmp = NULL;
#endif // FE_SB
    register PGETPROFILESTRING16 parg16;

    GETARGPTR(pFrame, sizeof(GETPROFILESTRING16), parg16);
    GETPSZPTR(parg16->f1, pszSection);
    GETPSZPTR(parg16->f2, pszKey);
    GETPSZPTR(parg16->f3, pszDefault);
    ALLOCVDMPTR(parg16->f4, parg16->f5, pszReturnBuffer);
    cchMax = INT32(parg16->f5);
#ifdef FE_SB
    //
    // For those applications that expect sLongDate contains
    // windows 3.1J picture format string.
    //
    if (GetSystemDefaultLangID() == 0x411 &&
            !lstrcmpi(pszSection, "intl") && !lstrcmpi(pszKey, "sLongDate")) {

        pszTmp = pszKey;
        pszKey = "sLongDate16";
    }
#endif // FE_SB

    if (IS_EMBEDDING_SECTION( pszSection ) &&
        !WasSectionRecentlyUpdated() ) {
        if( pszKey == NULL ) {
            UpdateEmbeddingAllKeys();
        } else {
            UpdateEmbeddingKey( pszKey );
        }
        SetLastTimeUpdated();

    } else if (pszSection &&
               pszKey &&
               !WOW32_stricmp(pszSection, szDevices) &&
               IsFaxPrinterSupportedDevice(pszKey)) {

        ul = GETINT16(GetFaxPrinterProfileString(pszSection, pszKey, pszDefault, pszReturnBuffer, cchMax));
        goto FlushAndReturn;
    }

    ul = GETINT16(GetProfileString(
             pszSection,
             pszKey,
             pszDefault,
             pszReturnBuffer,
             cchMax));


    //
    // Win3.1/Win95 compatibility:  Zap the first trailing blank in pszDefault
    // with null, but only if the default string was returned.  To detect
    // the default string being returned we need to ignore trailing blanks.
    //
    // This code is duplicated in thunks for GetProfileString and
    // GetPrivateProfileString, update both if you make changes.
    //

    if ( pszDefault && pszKey )  {

        int  n, nLenDef;

        //
        // Is the default the same as the returned string up to any NULLs?
        //
        nLenDef = 0;
        n=0;
        while (
            (pszDefault[nLenDef] == pszReturnBuffer[n]) &&
            pszReturnBuffer[n]
            ) {
            
            n++;
            nLenDef++;
        }
        
        //
        // Did we get out of the loop because we're at the end of the returned string?
        //
        if ( '\0' != pszReturnBuffer[n] ) {
            //
            // No.  The strings are materially different - fall out.
            //
        }
        else {
            //
            // Ok, the strings are identical to the end of the returned string.
            // Is the default string spaces out to the end?
            //
            while ( ' ' == pszDefault[nLenDef] ) {
                nLenDef++;
            }
            
            //
            // The last thing was not a space.  If it was a NULL, then the app
            // passed in a string with trailing NULLs as default.  (Otherwise
            // the two strings are materially different and we do nothing.)
            //
            if ( '\0' == pszDefault[nLenDef] ) {
                
                char szBuf[4];  // Some random, small number of chars to get.
                                // If the string is long, we'll get only 3 chars
                                // (and NULL), but so what? - we only need to know if
                                // we got a default last time...
                
                //
                // The returned string is the same as the default string
                // without trailing blanks, but this might be coincidence,
                // so see if a call with empty pszDefault returns anything.
                // If it does, we don't zap because the default isn't
                // being used.
                //

                if (0 == GetProfileString(pszSection, pszKey, "", szBuf, sizeof(szBuf))) {

                    //
                    // Zap first trailing blank in pszDefault with null.
                    //

                    pszDefault[ul] = 0;
                    FLUSHVDMPTR(parg16->f3 + ul, 1, pszDefault + ul);
                }
            }
        }
    }


FlushAndReturn:
#ifdef FE_SB
    if ( pszTmp )
        pszKey = pszTmp;

    if (CURRENTPTD()->dwWOWCompatFlags2 & WOWCF_USEUPPER) { // for WinWrite
        if (pszSection && !lstrcmpi(pszSection, "windows") &&
             pszKey && !lstrcmpi(pszKey, "device")) {
            CharUpper(pszReturnBuffer);
        }
        if (pszSection && !lstrcmpi(pszSection, "devices") && pszKey) {
            CharUpper(pszReturnBuffer);
        }
    }
#endif // FE_SB
    FLUSHVDMPTR(parg16->f4, (ul + (pszSection && pszKey) ? 1 : 2), pszReturnBuffer);
    FREEPSZPTR(pszSection);
    FREEPSZPTR(pszKey);
    FREEPSZPTR(pszDefault);
    FREEVDMPTR(pszReturnBuffer);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32GetPrivateProfileString(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ pszSection;
    PSZ pszKey;
    PSZ pszDefault;
    PSZ pszReturnBuffer;
    PSZ pszFilename;
    UINT cchMax;
#ifdef FE_SB
    PSZ pszTmp = NULL;
#endif // FE_SB
    register PGETPRIVATEPROFILESTRING16 parg16;
    CHAR szLowercase[MAX_PATH];

    GETARGPTR(pFrame, sizeof(GETPRIVATEPROFILESTRING16), parg16);
    GETPSZPTR(parg16->f1, pszSection);
    GETPSZPTR(parg16->f2, pszKey);
    GETPSZPTR(parg16->f3, pszDefault);
    ALLOCVDMPTR(parg16->f4, parg16->f5, pszReturnBuffer);
    GETPSZPTR(parg16->f6, pszFilename);

    // PC3270 (Personal communications): while installing this app it calls
    // GetPrivateProfileString (sectionname, NULL, defaultbuffer, returnbuffer,
    // cch = 0, filename). On win31 this call returns relevant data in return
    // buffer and corresponding size as return value. On NT, since the
    // buffersize(cch) is '0' no data is copied into the return buffer and
    // return value is zero which makes this app abort installation.
    //
    // So restricted compatibility:
    //   if above is the case set
    //      cch = 64k - offset of returnbuffer;
    //
    // A safer 'cch' would be
    //      cch = GlobalSize(selector of returnbuffer) -
    //                                (offset of returnbuffer);
    //                                                           - nanduri

    if (!(cchMax = INT32(parg16->f5))) {
        if (pszKey == (PSZ)NULL) {
            if (pszReturnBuffer != (PSZ)NULL) {
                 cchMax = 0xffff - (LOW16(parg16->f4));
            }
        }
    }

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    strcpy(szLowercase, pszFilename);
    WOW32_strlwr(szLowercase);

    if (IS_WIN_INI( szLowercase )) {
#ifdef FE_SB
        //
        // For those applications that expect sLongDate contains
        // windows 3.1J picture format string.
        //
        if (GetSystemDefaultLangID() == 0x411 &&
            lstrcmpi( pszSection, "intl") == 0 &&
            lstrcmpi( pszKey, "sLongDate") == 0) {
            pszTmp = pszKey;
            pszKey = "sLongDate16";
        }
#endif // FE_SB
        if (IS_EMBEDDING_SECTION( pszSection ) &&
            !WasSectionRecentlyUpdated() ) {
            if( pszKey == NULL ) {
                UpdateEmbeddingAllKeys();
            } else {
                UpdateEmbeddingKey( pszKey );
            }
            SetLastTimeUpdated();

        } else if (pszSection &&
                   pszKey &&
                   !WOW32_stricmp(pszSection, szDevices) &&
                   IsFaxPrinterSupportedDevice(pszKey)) {

            ul = GETINT16(GetFaxPrinterProfileString(pszSection, pszKey, pszDefault, pszReturnBuffer, cchMax));
            goto FlushAndReturn;
        }
    }

    ul = GETUINT16(GetPrivateProfileString(
        pszSection,
        pszKey,
        pszDefault,
        pszReturnBuffer,
        cchMax,
        pszFilename));

    
    // start comaptibility hacks
    
    
    //
    // Win3.1/Win95 compatibility:  Zap the first trailing blank in pszDefault
    // with null, but only if the default string was returned.  To detect
    // the default string being returned we need to ignore trailing blanks.
    //
    // This code is duplicated in thunks for GetProfileString and
    // GetPrivateProfileString, update both if you make changes.
    //

    if ( pszDefault && pszKey )  {

        int  n, nLenDef;

        //
        // Is the default the same as the returned string up to any NULLs?
        //
        nLenDef = 0;
        n=0;
        while (
            (pszDefault[nLenDef] == pszReturnBuffer[n]) &&
            pszReturnBuffer[n]
            ) {
            
            n++;
            nLenDef++;
        }
        
        //
        // Did we get out of the loop because we're at the end of the returned string?
        //
        if ( '\0' != pszReturnBuffer[n] ) {
            //
            // No.  The strings are materially different - fall out.
            //
        }
        else {
            //
            // Ok, the strings are identical to the end of the returned string.
            // Is the default string spaces out to the end?
            //
            while ( ' ' == pszDefault[nLenDef] ) {
                nLenDef++;
            }
            
            //
            // The last thing was not a space.  If it was a NULL, then the app
            // passed in a string with trailing NULLs as default.  (Otherwise
            // the two strings are materially different and we do nothing.)
            //
            if ( '\0' == pszDefault[nLenDef] ) {
                
                char szBuf[4];  // Some random, small number of chars to get.
                                // If the string is long, we'll get only 3 chars
                                // (and NULL), but so what? - we only need to know if
                                // we got a default last time...
                
                //
                // The returned string is the same as the default string
                // without trailing blanks, but this might be coincidence,
                // so see if a call with empty pszDefault returns anything.
                // If it does, we don't zap because the default isn't
                // being used.
                //
                if (0 == GetProfileString(pszSection, pszKey, "", szBuf, sizeof(szBuf))) {

                    //
                    // Zap first trailing blank in pszDefault with null.
                    //

                    pszDefault[ul] = 0;
                    FLUSHVDMPTR(parg16->f3 + ul, 1, pszDefault + ul);
                }
            }
        }
    }

    
    if( CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_SAYITSNOTTHERE ) {

        // CrossTalk 2.2 gets hung in a loop while trying to match a printer in
        // their xtalk.ini file with a printer name in the PrintDlg listbox.  
        // There is a bug in their code for handling this that gets exposed by
        // the fact that NT PrintDlg listboxes do not include the port name as
        // Win3.1 & Win'95 do.  We avoid the buggy code altogether with this 
        // hack by telling them that the preferred printer isn't stored in 
        // xtalk.ini. See bug #43168  a-craigj

        if(WOW32_strstr(szLowercase, "xtalk.ini")) {
            if(!WOW32_stricmp(pszSection, "Printer")) {
                if(!WOW32_stricmp(pszKey, "Device")) {
                    strcpy(pszReturnBuffer, pszDefault);
                    ul = strlen(pszReturnBuffer);
                }
            }
        }
    }


FlushAndReturn:
#ifdef FE_SB
    if ( pszTmp )
        pszKey = pszTmp;
#endif // FE_SB
    if (ul) {
        FLUSHVDMPTR(parg16->f4, (ul + (pszSection && pszKey) ? 1 : 2), pszReturnBuffer);
        LOGDEBUG(8,("GetPrivateProfileString returns '%s'\n", pszReturnBuffer));
    }

#ifdef DEBUG

    //
    // Check for bad return on retrieving entire section by walking
    // the section making sure it's full of null-terminated strings
    // with an extra null at the end.  Also ensure that this all fits
    // within the buffer.
    //

    if (!pszKey) {
        PSZ psz;

        //
        // We don't want to complain if the poorly-formed buffer was the one
        // passed in as pszDefault by the caller.
        //

        // Although the api docs clearly state that pszDefault should never
        // be null but win3.1 is nice enough to still deal with this. Delphi is
        // passing pszDefault as NULL and this following code causes an
        // assertion in WOW. So added the pszDefault check first.
        //
        // sudeepb 11-Sep-1995


        if (!pszDefault || WOW32_strcmp(pszReturnBuffer, pszDefault)) {

            psz = pszReturnBuffer;

            while (psz < (pszReturnBuffer + ul + 2) && *psz) {
                psz += strlen(psz) + 1;
            }

            WOW32ASSERTMSGF(
                psz < (pszReturnBuffer + ul + 2),
                ("GetPrivateProfileString of entire section returns poorly formed buffer.\n"
                 "pszReturnBuffer = %p, return value = %d\n",
                 pszReturnBuffer,
                 ul
                 ));
        }
    }

#endif // DEBUG

    FREEPSZPTR(pszSection);
    FREEPSZPTR(pszKey);
    FREEPSZPTR(pszDefault);
    FREEVDMPTR(pszReturnBuffer);
    FREEPSZPTR(pszFilename);
    FREEARGPTR(parg16);
    RETURN(ul);
}




ULONG FASTCALL WK32GetProfileInt(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2;
    register PGETPROFILEINT16 parg16;

    GETARGPTR(pFrame, sizeof(GETPROFILEINT16), parg16);
    GETPSZPTR(parg16->f1, psz1);
    GETPSZPTR(parg16->f2, psz2);

    ul = GETWORD16(GetProfileInt(
    psz1,
    psz2,
    INT32(parg16->f3)
    ));

    //
    // In HKEY_CURRENT_USER\Control Panel\Desktop\WindowMetrics, there
    // are a bunch of values that define the screen appearance. You can
    // watch these values get updated when you go into the display control
    // panel applet and change the "appearance scheme", or any of the
    // individual elements. The win95 shell is different than win31 in that it
    // sticks "twips" values in there instead of pixels. These are calculated
    // with the following formula:
    //
    //  twips = - pixels * 72 * 20 / cyPixelsPerInch
    //
    //  pixels = -twips * cyPixelsPerInch / (72*20)
    //
    // So if the value is negative, it is in twips, otherwise it in pixels.
    // The idea is that these values are device independent. NT is
    // different than win95 in that we provide an Ini file mapping to this
    // section of the registry where win95 does not. Now, when the Lotus
    // Freelance Graphics 2.1 tutorial runs, it mucks around with the look
    // of the screen, and it changes the border width of window frames by
    // using SystemParametersInfo(). When it tries to restore it, it uses
    // GetProfileInt("Windows", "BorderWidth", <default>), which on win31
    // returns pixels, on win95 returns the default (no ini mapping), and
    // on NT returns TWIPS. Since this negative number is interpreted as
    // a huge UINT, then the window frames become huge. What this code
    // below will do is translate the number back to pixels.   [neilsa]
    //

    if ((CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_PIXELMETRICS) &&
        !WOW32_stricmp(psz1, "Windows") &&
        !WOW32_stricmp(psz2, "BorderWidth") &&
        ((INT)ul < 0)) {

        HDC hDC = CreateDC("DISPLAY", NULL, NULL, NULL);
        ul = (ULONG) (-(INT)ul * GetDeviceCaps(hDC, LOGPIXELSY)/(72*20));
        DeleteDC(hDC);

    }

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}



ULONG FASTCALL WK32GetPrivateProfileInt(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2;
    PSZ psz4;
    register PGETPRIVATEPROFILEINT16 parg16;

    GETARGPTR(pFrame, sizeof(GETPRIVATEPROFILEINT16), parg16);
    GETPSZPTR(parg16->f1, psz1);
    GETPSZPTR(parg16->f2, psz2);
    GETPSZPTR(parg16->f4, psz4);

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    ul = GETWORD16(GetPrivateProfileInt(
    psz1,
    psz2,
    INT32(parg16->f3),
    psz4
    ));

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEPSZPTR(psz4);
    FREEARGPTR(parg16);
    RETURN(ul);
}



ULONG FASTCALL WK32GetModuleFileName(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz2;
    register PGETMODULEFILENAME16 parg16;
    HANDLE hT;

    GETARGPTR(pFrame, sizeof(GETMODULEFILENAME16), parg16);
    ALLOCVDMPTR(parg16->f2, parg16->f3, psz2);
    //
    // ShellExecute DDE returns (HINST)33 when DDE is used
    // to satisfy a request.  This looks like a task alias
    // to ISTASKALIAS but it's not.
    //

    if ( ISTASKALIAS(parg16->f1) && 33 != parg16->f1) {
        ul = GetHtaskAliasProcessName(parg16->f1,psz2,INT32(parg16->f3));
    } else {
        hT = (parg16->f1 && (33 != parg16->f1))
                 ? (HMODULE32(parg16->f1))
                 : GetModuleHandle(NULL) ;
        ul = GETINT16(GetModuleFileName(hT, psz2, INT32(parg16->f3)));
    }

    FLUSHVDMPTR(parg16->f2, strlen(psz2)+1, psz2);
    FREEVDMPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32WOWFreeResource(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWOWFREERESOURCE16 parg16;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);

    ul = GETBOOL16(FreeResource(
                       HCURSOR32(parg16->f1)
                       ));

    FREEARGPTR(parg16);
    RETURN(ul);
}



ULONG FASTCALL WK32GetDriveType(PVDMFRAME pFrame)
{
    ULONG ul;
    CHAR    RootPathName[] = "?:\\";
    register PGETDRIVETYPE16 parg16;

    GETARGPTR(pFrame, sizeof(GETDRIVETYPE16), parg16);

    // Form Root path
    RootPathName[0] = (CHAR)('A'+ parg16->f1);

    ul = GetDriveType (RootPathName);
    // bugbug  - temporariy fixed, should be removed when base changes
    // its return value for non-exist drives
    // Windows 3.0 sdk manaul said this api should return 1
    // if the drive doesn't exist. Windows 3.1 sdk manual said
    // this api should return 0 if it failed. Windows 3.1 winfile.exe
    // expects 0 for noexisting drives. The NT WIN32 API uses
    // 3.0 convention. Therefore, we reset the value to zero
    // if it is 1.
    if (ul <= 1)
        ul = 0;

    // DRIVE_CDROM and DRIVE_RAMDISK are not supported under Win 3.1
    if ( ul == DRIVE_CDROM ) {
        ul = DRIVE_REMOTE;
    }
    if ( ul == DRIVE_RAMDISK ) {
        ul = DRIVE_FIXED;
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}

/* WK32TermsrvGetWindowsDir - Front end to TermsrvGetWindowDirectory.
 *
 *
 * Entry - pszPath   - Pointer to return buffer for path (ascii)
 *         usPathLen - Size of path buffer (bytes)
 *
 * Exit
 *     SUCCESS
 *       True
 *
 *     FAILURE
 *       False
 *
 */
ULONG FASTCALL WK32TermsrvGetWindowsDir(PVDMFRAME pFrame)
{
    PTERMSRVGETWINDIR16 parg16;
    PSTR    psz;
    NTSTATUS Status = 0;
    USHORT   usPathLen;

    //
    // Get arguments.
    //
    GETARGPTR(pFrame, sizeof(TERMSRVGETWINDIR16), parg16);
    psz = SEGPTR(FETCHWORD(parg16->pszPathSegment),
                 FETCHWORD(parg16->pszPathOffset));
    usPathLen = FETCHWORD(parg16->usPathLen);
    FREEARGPTR(parg16);

    Status = GetWindowsDirectoryA(psz, usPathLen);

    // If this is a long path name then get the short one
    // Otherwise it will return the same path

    GetShortPathNameA(psz, psz, (DWORD)Status);

    // Get the real size.
    Status = lstrlen(psz);

    return(NT_SUCCESS(Status));
}

