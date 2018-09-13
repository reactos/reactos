/* (C) Microsoft Corp., 1991. All rights reserved. */

/* mapcopy.c - duplicates the MIDIMAP.CFG file because of poor design
 * !!!!!!!!!!!! THIS IS NEEDLESS GRATUITIOUS CODE !!!!!!!!!!!!!!!!!!!
 * The MIDIMAP and applet were badly implemented and because of design
 * flaws needed this gratuitous file copy to back up the configuration
 * file.
 */

#include<windows.h>
#include <mmsystem.h>
#if defined(WIN32)
#include <port1632.h>
#endif
#include "hack.h"
#include"midimap.h"
#include "extern.h"
#include "midi.h"

#define MAXBUF  (10 * 1024)     // size of default copy buffer
#define FOPEN(sz)           (_lopen(sz,OF_READ|OF_SHARE_DENY_NONE))
#define FCREATE(sz)         (_lcreat(sz,0))
#define FCLOSE(fh)          (_lclose(fh))
#define FREAD(fh,buf,len)   (_lread(fh,buf,len))
#define FWRITE(fh,buf,len)  (_lwrite(fh,buf,len))
#define FALLOC(n)           (LPVOID)GlobalLock(GlobalAlloc(GMEM_MOVEABLE, (DWORD)(n)))
#define FFREE(n)            (GlobalUnlock((HGLOBAL)HIWORD((DWORD)(n))),GlobalFree((HGLOBAL)HIWORD((DWORD)(n))))
#define SLASH(c)            ((c) == '/' || (c) == '\\')


static int FileCopy(LPSTR szSrc, LPSTR szDst);

#if defined(WIN16)

static BOOL FileCopy(LPSTR,LPSTR);
static UINT GetFileAttributes(LPSTR);
static DWORD DosDiskFree ( BYTE bDrive );

static int DosCWDrive (VOID);

/*
 * Get DOS file attributes
 */
static UINT GetFileAttributes(LPSTR lpszPath)
{
    _asm {
        push    ds                  ; Preserve DS
        lds     dx,lpszPath         ; DS:DI = lpszPath
        mov     ax,4300h            ; Get File Attributes
        int     21h
        jc      GFAErr
        mov     ax,cx               ; AX = attribute
        jmp     GFAExit             ; Return attribute
GFAErr: mov     ah,80h              ; Return negative error code
GFAExit:
        pop     ds                  ; Restore DS
    }
}

int FAR PASCAL DosDelete(LPSTR szFile)
{
    _asm {
        push    ds                  ; Preserve DS
        lds     dx,szFile
        mov     ah,41h
        int     21h
        jc      dexit
        xor     ax,ax
dexit:
        pop     ds                  ; Restore DS
    }
}

static DWORD DosDiskFree ( BYTE bDrive )
{
    WORD wSectors,wBytes,wClusters;

    _asm {
         mov    dl,bDrive
         mov    ah,36h
         int    21h
         mov    wSectors,ax
         mov    wClusters,bx
         mov    wBytes,cx
    }

    if (wSectors == 0xffff)
        return -1;
    else
        return ((DWORD)wClusters*(DWORD)wSectors*(DWORD)wBytes);
}

static int DosCWDrive ()
{
    _asm {
        mov     ah,19h              ; Get Current Drive
        int     21h
        sub     ah,ah               ; Zero out AH
    }
}
#endif //WIN16

/*
 * Create a duplicate map file for editing
 */

BOOL FAR PASCAL DupMapCfg(LPSTR lpstrCfgPath, LPSTR lpstrBakPath)
{
    extern BOOL fReadOnly;
    UINT uAttr;
    OFSTRUCT of;
    int err;
    char szFunc[50],szMessage[256];

    if (OpenFile(lpstrCfgPath,&of,OF_EXIST|OF_SHARE_DENY_NONE) == HFILE_ERROR)
    {
        // We need to let the mapper create a new one.
        return FALSE;
    }

    uAttr = GetFileAttributes(lpstrCfgPath);
    if (uAttr != (UINT)(-1))
    {
        if (uAttr & FILE_ATTRIBUTE_READONLY)
        {

                LoadString(hLibInst, IDS_FCERR_WARN, szFunc, sizeof(szFunc));
                LoadString(hLibInst, IDS_FCERR_READONLY, szMessage, sizeof(szMessage));
                MessageBox(NULL, szMessage, szFunc,
                        MB_ICONEXCLAMATION | MB_OK);

                fReadOnly = TRUE;  // Stay Read Only for the remainder
                return FALSE;
        }
    }
    else
            return FALSE;

    err = FileCopy(lpstrCfgPath,lpstrBakPath);
    if (err != IDS_FCERR_SUCCESS)
    {
            LoadString(hLibInst, IDS_FCERR_ERROR, szFunc, sizeof(szFunc));
            LoadString(hLibInst, err, szMessage, sizeof(szMessage));
            MessageBox(NULL, szMessage, szFunc, MB_ICONHAND | MB_TASKMODAL
                    | MB_OK);

            fReadOnly = TRUE; // Read Only if there is some kind of disk problem
            return FALSE;
    }
    return TRUE;
}

/*
 * Copy the duplicate map file over the original map file
 */
BOOL FAR PASCAL UpdateMapCfg(LPSTR lpstrCfgPath, LPSTR lpstrBakPath)
{
    int err;
    char szFunc[50],szMessage[256];

    if ((err = FileCopy(lpstrBakPath,lpstrCfgPath)) != IDS_FCERR_SUCCESS)
    {
            LoadString(hLibInst, IDS_FCERR_ERROR, szFunc, sizeof(szFunc));
            LoadString(hLibInst, err, szMessage, sizeof(szMessage));
            MessageBox(NULL, szMessage, szFunc, MB_ICONHAND | MB_TASKMODAL
                    | MB_OK);
    }

    return err;
}


#if defined(WIN16)

/* So who was too lazy to write a comment? */
static int FileCopy(LPSTR szSrc,LPSTR szDst)
{
    HFILE fhSrc,fhDst;
    WORD size;
    BOOL fComplete = TRUE;
    OFSTRUCT of;
    LPSTR lpbBuf;
    DWORD dwSrcSize,dwDestFree;
    BYTE bDrive;
    int err;

    if (OpenFile(szSrc,&of,OF_EXIST|OF_SHARE_DENY_NONE) == HFILE_ERROR)
    {
        err = IDS_FCERR_NOSRC;
        goto exit; //ERROR:File Not Found
    }

    fhSrc = FOPEN(szSrc);

    if (fhSrc == HFILE_ERROR)
    {
        err = IDS_FCERR_NOSRC;
        return FALSE; //ERROR:File inaccessible
    }

    dwSrcSize = _llseek(fhSrc,0,2); //file size
    _llseek(fhSrc,0,0);

    if (*(szDst+1) == ':')
    {
            bDrive = (BYTE)(*(szDst) - 'A' + 1);
    }
    else
            bDrive = (BYTE)(DosCWDrive() + 1);


    if ((LONG)(dwDestFree = DosDiskFree(bDrive)) != -1)
    {
            if (dwSrcSize > dwDestFree)
            {
                    err = IDS_FCERR_DISKFULL;
                    goto errclose1;
            }
    }
/*      This failed on a wierd 386 machine for no good reason, so removed.
        MM Bug 6277.
    else
    {
            err = IDS_FCERR_DISK;
            goto errclose1;
    }
*/

    lpbBuf = FALLOC(MAXBUF);

    if (lpbBuf == NULL)
    {
        err = IDS_FCERR_LOMEM;
        goto errclose1; //ERROR:Low Memory
    }

    fhDst = FCREATE(szDst);

    if (fhDst == HFILE_ERROR)
    {
        err = IDS_FCERR_NODEST;
        goto errfree; //ERROR:Couldn't create Destination
    }

    while (size = FREAD(fhSrc,lpbBuf,MAXBUF))
    {
        if (FWRITE(fhDst,lpbBuf,size) != size)
        {
            err = IDS_FCERR_WRITE;
            goto errclose; //ERROR:Write Error
        }
    }
    err = IDS_FCERR_SUCCESS;

errclose:   // Close the Destination File
    FCLOSE(fhDst);
errfree:    // Free the buffer
    FFREE(lpbBuf);
errclose1:  // Close the Source File
    FCLOSE(fhSrc);
exit:
    return err;
}

#else

/* Copy the file szSrc to the file szDst, overwriting it if it already exists */
static int FileCopy(LPSTR szSrc, LPSTR szDst)
{
  if (CopyFile(szSrc, szDst, FALSE)) {
     return IDS_FCERR_SUCCESS;
  } else {
      switch (GetLastError()) {
          // ??? This is throwing away valuable error information!
          default:
              return IDS_FCERR_NOSRC;
      }
  }

} /* FileCopy */
#endif //WIN16
