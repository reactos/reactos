/*
 *  exe.c   Get info from a EXEHDR
 *
 *  Modification History:
 *
 *  4/03/89  ToddLa Wrote it
 *  4/09/90  T-JackD modification such that the type of error is reflected...
 *  4/17/90  t-jackd modification such that notification of error can be set...
 */
#include <windows.h>
#include "desk.h"
#include <newexe.h>
#include "exe.h"

static DWORD dwDummy;
#define FOPEN(sz)                CreateFile(sz, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL )
#define FCLOSE(fh)               CloseHandle(fh)
#define FREAD(fh,buf,len)        (ReadFile(fh,buf,len, &dwDummy, NULL) ? dwDummy : HFILE_ERROR)
#define FSEEK(fh,off,i)          SetFilePointer(fh,(DWORD)off, NULL, i)
#define F_SEEK_SET                    FILE_BEGIN

BOOL NEAR PASCAL IsFAPI(int fh, struct new_exe FAR *pne, long off);

/*
 *  Function will return a specific piece of information from a new EXEHDR
 *
 *      szFile      - Path Name a new exe
 *      pBuf        - Buffer to place returned info
 *      nBuf        - Size of buffer in BYTES
 *      fInfo       - What info to get?
 *
 *          GEI_MODNAME         - Get module name
 *          GEI_DESCRIPTION     - Get description
 *          GEI_FLAGS           - Get EXEHDR flags
 *
 *  returns:  LOWORD = ne_magic, HIWORD = ne_exever
 *            0 if error
 */

DWORD FAR PASCAL GetExeInfo(LPTSTR szFile, void FAR *pBuf, int nBuf, UINT fInfo)
{
    HANDLE      fh;
    DWORD       off;
    DWORD       dw;
    BYTE        len;
    struct exe_hdr exehdr;
    struct new_exe newexe;

    fh = FOPEN(szFile);

    if (fh == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    if (FREAD(fh, &exehdr, sizeof(exehdr)) != sizeof(exehdr) ||
        exehdr.e_magic != EMAGIC ||
        exehdr.e_lfanew == 0L)
    {
            goto error;        /* Abort("Not an exe",h); */
    }

    FSEEK(fh, exehdr.e_lfanew, F_SEEK_SET);

    if (FREAD(fh, &newexe, sizeof(newexe)) != sizeof(newexe))
    {
            goto error;      // Read error
    }

    if (newexe.ne_magic == PEMAGIC)
    {
            if (fInfo != GEI_DESCRIPTION &&
                fInfo != GEI_EXPVER)
                    goto error;

            // make the file name the description
            lstrcpy(pBuf, szFile);

            // read the SubsystemVersion

            FSEEK(fh,exehdr.e_lfanew+18*4,F_SEEK_SET);
            FREAD(fh,&dw,4);

            newexe.ne_expver = LOBYTE(LOWORD(dw)) << 8 | LOBYTE(HIWORD(dw));
            goto exit;
    }

    if (newexe.ne_magic != NEMAGIC)
    {
            goto error;      // Invalid NEWEXE
    }

    switch (fInfo)
    {
#ifdef FAPI
        case GEI_FAPI:
            *(BOOL FAR *)pBuf = IsFAPI(fh,(struct new_exe FAR *)&newexe,
                exehdr.e_lfanew);
            break;
#endif

        case GEI_EXEHDR:
            *(struct new_exe FAR *)pBuf = newexe;
            break;

        case GEI_FLAGS:
            *(WORD FAR *)pBuf = newexe.ne_flags;
            break;

        /* module name is the first entry in the medident name table */
        case GEI_MODNAME:
            off = exehdr.e_lfanew + newexe.ne_restab;
            goto readstr;
            break;

        /* module name is the first entry in the non-medident name table */
        case GEI_DESCRIPTION:
            off = newexe.ne_nrestab;
readstr:
            FSEEK(fh, off, F_SEEK_SET);
            FREAD(fh, &len, sizeof(BYTE));

            nBuf--;         // leave room for a \0

            if (len > (BYTE)nBuf)
                len = (BYTE)nBuf;

#ifdef UNICODE
            {
                LPSTR pbTmp;
                pbTmp = LocalAlloc(LMEM_FIXED, len);

                FREAD(fh, pbTmp, len);

                len = MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pbTmp, len,
                        (LPWSTR)pBuf, nBuf / SIZEOF(WCHAR));

                LocalFree(pbTmp);

            }
#else
            FREAD(fh, pBuf, len);
#endif
            ((LPTSTR)pBuf)[len] = 0;
            break;

        case GEI_EXPVER:
            break;

        default:
            goto error;
            break;
    }

exit:
    FCLOSE(fh);
    return MAKELONG(newexe.ne_magic, newexe.ne_expver);

error:
    FCLOSE(fh);
    return 0;
}

#ifdef UNICODE

/*
 * Code taken from kernel32.dll
 */
#define DEFAULT_WAIT_FOR_INPUT_IDLE_TIMEOUT 30000

UINT WinExecN( LPCTSTR lpCmdLine, UINT uCmdShow )
{
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL CreateProcessStatus;
    DWORD ErrorCode;

    ZeroMemory(&StartupInfo,sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = (WORD)uCmdShow;
    CreateProcessStatus = CreateProcess(
                            NULL,
                            (LPTSTR)lpCmdLine,
                            NULL,
                            NULL,
                            FALSE,
                            0,
                            NULL,
                            NULL,
                            &StartupInfo,
                            &ProcessInformation
                            );

    if ( CreateProcessStatus ) {
        //
        // Wait for the started process to go idle. If it doesn't go idle in
        // 10 seconds, return anyway.
        //
        WaitForInputIdle(ProcessInformation.hProcess,
                    DEFAULT_WAIT_FOR_INPUT_IDLE_TIMEOUT);
        CloseHandle(ProcessInformation.hProcess);
        CloseHandle(ProcessInformation.hThread);
        return 33;
        }
    else {
        //
        // If CreateProcess failed, then look at GetLastError to determine
        // appropriate return code.
        //

        ErrorCode = GetLastError();
        switch ( ErrorCode ) {
            case ERROR_FILE_NOT_FOUND:
                return 2;

            case ERROR_PATH_NOT_FOUND:
                return 3;

            case ERROR_BAD_EXE_FORMAT:
                return 11;

            default:
                return 0;
            }
        }
}
#endif

#ifdef FAPI

#pragma error(THIS CODE HAS NOT BEEN PORTED TO NT)


#include <ctype.h>
#define UP_CASE(f)              toupper(f)

int NEAR PASCAL lstrncmpi(LPSTR pch1, LPSTR pch2, int n)
{
    while (*pch1 && --n > 0 && UP_CASE(*pch1) == UP_CASE(*pch2))
         *pch1++,*pch2++;
    return UP_CASE(*pch1) != UP_CASE(*pch2);
}

char    szDOSCALLS[] = "DOSCALLS";
#define lenDOSCALLS 8

/* BOOL NEAR PASCAL IsFAPI(fh,off)
 *
 *  Function will return whether a exe is a FAPI exe
 *
 *      fh      - Open file handle to NEW EXE
 *      off     - Base of module table
 *
 *  returns:  TRUE if FAPI, FALSE otherwise.
 */
BOOL NEAR PASCAL IsFAPI(int fh, struct new_exe FAR * pne, long off)
{
    char    buf[256];
    char FAR  *pch;
    WORD FAR  *pw;
    int     n;
    UINT    i;
    BOOL    f = FALSE;

    /*
     *    look through the imported module table for the name "DOSCALLS" if
     *    found the EXE is a FAPI app.
     *
     *  NOTE! assumes module table will fit in a 256 byte buffer
     */

    // make sure this doesn't point off the end of the buffer we will use

    if (pne->ne_modtab > sizeof(buf))
    return FALSE;

    FSEEK(fh,off,F_SEEK_SET);
    FREAD(fh,buf,sizeof(buf));

    pw = (WORD FAR *)(buf + pne->ne_modtab);

    for (i = 0; i < pne->ne_cmod; i++)
    {
        pch = (LPSTR)buf + pne->ne_imptab + *pw++;

    if (pch > (LPSTR)(buf + sizeof(buf)))    // be sure we don't go off the end
        break;

        n = (int)*pch++;

    if (n == 0)
        break;

    if (n == lenDOSCALLS && !lstrncmpi(szDOSCALLS, pch, lenDOSCALLS))
    {
        f = TRUE;
        break;
    }
    }

    return f;
}

#endif
