#ifndef __WIN32K_COORD_H
#define __WIN32K_COORD_H

VOID
FASTCALL
IntDPtoLP ( PDC dc, LPPOINT Points, INT Count );

VOID
FASTCALL
CoordDPtoLP ( PDC Dc, LPPOINT Point );

int
FASTCALL
IntGetGraphicsMode ( PDC dc );

VOID
FASTCALL
CoordLPtoDP ( PDC Dc, LPPOINT Point );

VOID
FASTCALL
IntLPtoDP ( PDC dc, LPPOINT Points, INT Count );

int STDCALL IntGdiSetMapMode(PDC, int);

BOOL
FASTCALL
IntGdiModifyWorldTransform(PDC pDc,
                           CONST LPXFORM lpXForm,
                           DWORD Mode);

#endif
