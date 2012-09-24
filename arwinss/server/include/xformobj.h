#pragma once

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
