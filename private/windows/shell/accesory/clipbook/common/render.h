
/******************************************************************************

                        R E N D E R   H E A D E R

    Name:       render.h
    Date:       1/20/94
    Creator:    John Fu

    Description:
        This is the header file for render.c

******************************************************************************/



HANDLE RenderFormat(
    FORMATHEADER    *pfmthdr,
    register HANDLE fh);


HANDLE RenderFormatDibToBitmap(
    FORMATHEADER    *pfmthdr,
    register HANDLE fh,
    HPALETTE        hPalette);
