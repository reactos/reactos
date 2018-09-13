/* stream.c - OLE stream I/O routines.
 *
 * Created by Microsoft Corporation.
 */

#include "packager.h"


static LPSTR *glplpstr;
static STREAMOP gsop;



/* SetFile() - Set the file to be written.
 */
VOID
SetFile(
    STREAMOP sop,
    INT fh,
    LPSTR *lplpstr
    )
{
    switch (gsop = sop)
    {
        case SOP_FILE:
            glpStream->fh = fh;
            break;

        case SOP_MEMORY:
            gcbObject = 0L;
            glplpstr = lplpstr;
            break;
    }
}



/* ReadStream() - Read bytes from memory, from a file, or just count them.
 */
DWORD
ReadStream(
    LPAPPSTREAM lpStream,
    LPSTR lpstr,
    DWORD cb
    )
{
    switch (gsop)
    {
        case SOP_FILE:
            return _lread(lpStream->fh, lpstr, cb);
            break;

        case SOP_MEMORY:
            gcbObject += cb;

            if (glplpstr)
                MemRead(glplpstr, lpstr, cb);

            break;
    }

    return cb;
}



/* PosStream() - Reset the position of the file pointer.
 *
 * Note:  This is never used; luckily, or it would mess up the count.
 */
DWORD
PosStream(
    LPAPPSTREAM lpStream,
    LONG pos,
    INT iorigin)
{
    return _llseek(lpStream->fh, pos, iorigin);
}



/* WriteStream() - Write bytes to memory, to a file, or just count them.
 */
DWORD
WriteStream(
    LPAPPSTREAM lpStream,
    LPSTR lpstr,
    DWORD cb
    )
{
    switch (gsop)
    {
        case SOP_FILE:
            return _lwrite(lpStream->fh, lpstr, cb);

        case SOP_MEMORY:
            gcbObject += cb;

            if (glplpstr)
                MemWrite(glplpstr, lpstr, cb);

            break;
    }

    return cb;
}



/********************* Memory read/write functions ********************/
/* MemRead() - Read bytes from the memory (stream).
 */
DWORD
MemRead(
    LPSTR *lplpStream,
    LPSTR lpItem,
    DWORD dwSize
    )
{
    DWORD cb;
    CHAR *hpDest = lpItem;
    CHAR *hpSrc = *lplpStream;

    for (cb = dwSize; cb; cb--)
        *hpDest++ = *hpSrc++;

    *lplpStream = hpSrc;

    return dwSize;
}



/* MemWrite() - Write bytes to the memory (stream).
 */
DWORD
MemWrite(
    LPSTR *lplpStream,
    LPSTR lpItem,
    DWORD dwSize
    )
{
    DWORD cb;
    CHAR *hpDest = *lplpStream;
    CHAR *hpSrc = lpItem;

    for (cb = dwSize; cb; cb--)
        *hpDest++ = *hpSrc++;

    *lplpStream = hpDest;

    return dwSize;
}


