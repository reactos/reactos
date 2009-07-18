#ifndef _WIN32K_XFORMOBJ_H_
#define _WIN32K_XFORMOBJ_H_

ULONG
NTAPI
XFORMOBJ_iSetXform(
	OUT XFORMOBJ *pxo,
	IN XFORML * pxform);

ULONG
NTAPI
XFORMOBJ_iCombine(
	IN XFORMOBJ *pxo,
	IN XFORMOBJ *pxo1,
	IN XFORMOBJ *pxo2);

ULONG
NTAPI
XFORMOBJ_iCombineXform(
	IN XFORMOBJ *pxo,
	IN XFORMOBJ *pxo1,
	IN XFORML *pxform,
	IN BOOL bLeftMultiply);

ULONG
NTAPI
XFORMOBJ_Inverse(
	OUT XFORMOBJ *pxoDst,
	IN XFORMOBJ *pxoSrc);

#define IntDPtoLP(dc, pp, c) XFORMOBJ_bApplyXform((XFORMOBJ*)&(dc)->mxDeviceToWorld, XF_LTOL, c, pp, pp);
#define IntLPtoDP(dc, pp, c) XFORMOBJ_bApplyXform((XFORMOBJ*)&(dc)->mxWorldToDevice, XF_LTOL, c, pp, pp);
#define CoordDPtoLP(dc, pp) XFORMOBJ_bApplyXform((XFORMOBJ*)&(dc)->mxDeviceToWorld, XF_LTOL, 1, pp, pp);
#define CoordLPtoDP(dc, pp) XFORMOBJ_bApplyXform((XFORMOBJ*)&(dc)->mxWorldToDevice, XF_LTOL, 1, pp, pp);
#define XForm2MatrixS(m, x) XFORMOBJ_iSetXform((XFORMOBJ*)m, (XFORML*)x)
#define MatrixS2XForm(x, m) XFORMOBJ_iGetXform((XFORMOBJ*)m, (XFORML*)x)

#endif /* not _WIN32K_XFORMOBJ_H_ */
