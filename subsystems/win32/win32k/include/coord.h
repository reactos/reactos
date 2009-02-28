#ifndef __WIN32K_COORD_H
#define __WIN32K_COORD_H

#include <include/dc.h>

#define IntDPtoLP(dc, pp, c) XFORMOBJ_bApplyXform((XFORMOBJ*)&(dc)->DcLevel.mxDeviceToWorld, XF_LTOL, c, pp, pp);
#define IntLPtoDP(dc, pp, c) XFORMOBJ_bApplyXform((XFORMOBJ*)&(dc)->DcLevel.mxWorldToDevice, XF_LTOL, c, pp, pp);
#define CoordDPtoLP(dc, pp) XFORMOBJ_bApplyXform((XFORMOBJ*)&(dc)->DcLevel.mxDeviceToWorld, XF_LTOL, 1, pp, pp);
#define CoordLPtoDP(dc, pp) XFORMOBJ_bApplyXform((XFORMOBJ*)&(dc)->DcLevel.mxWorldToDevice, XF_LTOL, 1, pp, pp);
#define XForm2MatrixS(m, x) XFORMOBJ_iSetXform((XFORMOBJ*)m, (XFORML*)x)
#define MatrixS2XForm(x, m) XFORMOBJ_iGetXform((XFORMOBJ*)m, (XFORML*)x)

int
FASTCALL
IntGetGraphicsMode ( PDC dc );

int APIENTRY IntGdiSetMapMode(PDC, int);

BOOL
FASTCALL
IntGdiModifyWorldTransform(PDC pDc,
                           CONST LPXFORM lpXForm,
                           DWORD Mode);

VOID FASTCALL IntMirrorWindowOrg(PDC);
void FASTCALL IntFixIsotropicMapping(PDC dc);

#endif
