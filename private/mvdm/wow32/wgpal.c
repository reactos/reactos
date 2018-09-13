/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WGPAL.C
 *  WOW32 16-bit GDI API support
 *
 *  History:
 *  07-Mar-1991 Jeff Parsons (jeffpar)
 *  Created.
 *
 *  09-Apr-1991 NigelT
 *  Various defines are used here to remove calls to Win32
 *  features which don't work yet.
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wgpal.c);


ULONG FASTCALL WG32GetSystemPaletteEntries(PVDMFRAME pFrame)
{
    ULONG ul = 0L;
    PPALETTEENTRY ppal;
    register PGETSYSTEMPALETTEENTRIES16 parg16;

    GETARGPTR(pFrame, sizeof(GETSYSTEMPALETTEENTRIES16), parg16);

    GETVDMPTR(parg16->f4, parg16->f3 * sizeof(PALETTEENTRY), ppal);

    if( ppal ) {

        ul = GETWORD16(GetSystemPaletteEntries(HDC32(parg16->f1),
                                               WORD32(parg16->f2),
                                               WORD32(parg16->f3),
                                               ppal));

        // if we fail but are on a rgb device, fill in the default 256 entries.
        // WIN31 just calls Escape(hdc,GETCOLORTABLE) which on NT just calls
        // GetSysteemPaletteEntries().

        if (!ul && (GetDeviceCaps(HDC32(parg16->f1),BITSPIXEL) > 8))
        {
            if (parg16->f4 == 0)
            {
                ul = 256;
            }
            else
            {
                int j;
                int i = WORD32(parg16->f2);
                int c = WORD32(parg16->f3);

                if ((c + i) > 256)
                    c = 256 - i;

                if (c > 0)
                {
                    BYTE abGreenRed[8] = {0x0,0x25,0x48,0x6d,0x92,0xb6,0xdb,0xff};
                    BYTE abBlue[4]     = {0x0,0x55,0xaa,0xff};

                    // green mask 00000111
                    // red mask   00111000
                    // blue mask  11000000
                    // could certainly do this faster with a table and mem copy
                    // but I don't really care about performance here.  Apps
                    // shouldn't be doing this.  That is why it is in the wow
                    // layer.

                    for (j = 0; j < c; ++j,++i)
                    {
                        ppal[j].peGreen = abGreenRed[i & 0x07];
                        ppal[j].peRed   = abGreenRed[(i >> 3) & 0x07];
                        ppal[j].peBlue  = abBlue[(i >> 6) & 0x03];
                        ppal[j].peFlags = 0;
                    }

                    ul = c;
                }
            }
        }

        FREEVDMPTR(ppal);
    }

    FREEARGPTR(parg16);

    RETURN(ul);
}
