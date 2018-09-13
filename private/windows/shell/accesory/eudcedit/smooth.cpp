//
// Copyright (c) 1997-1999 Microsoft Corporation.
//

#include	"stdafx.h"

#include	"vdata.h"
#include	"extfunc.h"
#include	"ttfstruc.h"

/* Smooth sort */
#define		SMOOTH		0x8000
#define		NOSMOOTH	0x4000
#define		BACKWARD	0x2000		/* NOSMOOTH is forward vector */
#define		FOREWARD	0x1000		/* NOSMOOTH is backward vector*/

/* Curve Type */
#define		SPLINE_ATR	0x0001

/* Smoothing anchor Point */
#define		SMOOTHANCHOR	0x0080
	/* Smoothing anchor point is based 90degree Rule */
/* for turning */
#define		TURN_OU		1		/* 凹 */
#define		TURN_TOTSU	2		/* 凸 */

#define		sign(n)		(n < 0 ? 1 : 0)

#define		LONGMIN		5


int  SmoothVector(int  lstHdl,int  tmpLst,int  xinMesh,int yinMesh, int  outMesh,struct  SMOOTHPRM *prm,int  fp);
static int  BindVector(int  inLst,int  outLst);
static int  BindPoint(int  inLst,int  outLst);
static int  DetectNoSmooth(int  lsthdl);
static void  turnsort(struct  VDATA *vp);
static void  generalsmooth(struct  VDATA *vp);
static int  isturn(struct  VDATA *vp);
static int  islong(struct  VDATA *vp);
static void  longline(struct  VDATA *vp);
static int  ZoomPoint(int  lstHdl,int  xinMesh,int yinMesh, int  outMesh,int  uFp);
static int  preproc1(int  inLst,int  outLst);
static int  diveachside(struct  VDATA *vp,int  outLst);
static int  divoneside(struct  VDATA *vp,int  outLst);
static int divVec( int ilst, int olst, int divN);
static int  Smooth1(int  inLst,int  outLst,int  weight1,int  weight2,int  weight3);
static int  Smooth2(int  inLst,int  outLst);
int  searchanchor(int  sn,struct  VDATA *sp,struct  VDATA * *ep,int  lim);
static int  smooth1sub(struct  VDATA *vp,int  np,int  outLst,int  w1,int  w2,int  w3);
static int  smooth2sub(struct  VDATA *vp,int  np,int  outLst);
int  RemoveFp(int  lstHdl,int  outMesh,int  uFp);
static int  BindLinears(int  inLst,int  outLst);
static int  DelZeroVec(int  inLst,int  outLst);
int  toTTFFrame(int  lstH,struct  BBX *bbx);
static int  lightSub(int  inLst,int  outLst);
int  SmoothLight(int  ioLst,int  tmpLst,int  width,int height, int  oWidth,int  ufpVal);
int  ConvMesh(int  lstH,int  inMesh,int  outMesh);
static int
straightCurve( int iLst, int oLst)
{
struct VHEAD	*vhd;
struct VDATA	*vp;
struct vecdata	rel, nxtrel;
	int	np;
	int	sts;
struct vecdata	svd;

	if ( (sts = VDGetHead( iLst, &vhd))!=0)
		goto	RET;
	VDNew(oLst);

	while ( vhd->next!=NIL) {
		/* No point data is not Allowed */
		/* Set Origin */
		vp = vhd->headp;
		if ((sts = VDSetData(oLst, &vp->vd))!=0)
			goto	RET;
		vp=vp->next;
		for ( np = 1; np < vhd->nPoints; np++,vp = vp->next) {
			if ( vp->vd.atr & SPLINE_ATR) {
				rel.x = vp->vd.x - vp->prev->vd.x;
				rel.y = vp->vd.y - vp->prev->vd.y;
				nxtrel.x= vp->next->vd.x - vp->vd.x;
				nxtrel.y= vp->next->vd.y - vp->vd.y;
				if ((long)rel.x*nxtrel.y==(long)rel.y*nxtrel.x){
					vp = vp->next;
					np++;
					svd = vp->vd;
					svd.atr &= ~SPLINE_ATR;
					if ((sts = VDSetData(oLst, &svd))!=0)
						goto	RET;
				}
				else {
					if ((sts = VDSetData(oLst, &vp->vd))!=0)
						goto	RET;
					vp = vp->next;
					np++;
					if ( np >= vhd->nPoints)
						break;
					if ((sts = VDSetData(oLst, &vp->vd))!=0)
						goto	RET;
				}
			}
			else {
				if ((sts = VDSetData(oLst, &vp->vd))!=0)
					goto	RET;
			}
		}
		if(VDClose( oLst))
			goto	RET;
		vhd = vhd->next;
	}
	return 0;
RET:
	return -1;
}
/***********************************************************************
 *	Smoothing Main
 */
/* */	int
/* */	SmoothVector(
/* */		int	lstHdl,
/* */		int	tmpLst,
/* */		int	xinMesh,	/* Input Mesh Size*/
/* */		int 	yinMesh,
/* */		int	outMesh,	/* Output Mesh size */
/* */	struct SMOOTHPRM *prm,
/* */		int	fp)		/* Fixed Point sft-val */
/*
 *	reuturns : 0, -1
 *	Remarks : このモジュールの入力は、輪郭抽出の結果のベクタ列で
 *		　原点を左上とする。
 *		　出力は、出力メッシュに合わせるが、原点は左上で、
 *		　ベースライン、左側ベアリングは調整されない
 ***********************************************************************/
{
	int	sts;

	if ( (sts = BindPoint( lstHdl, tmpLst))<0)
		goto	RET;

	if ( (sts = DetectNoSmooth( tmpLst))<0)
		goto	RET;

	if ( (sts = ZoomPoint( tmpLst, xinMesh, yinMesh, outMesh, fp))<0)
		goto	RET;

	if ( (sts = preproc1( tmpLst, lstHdl))<0)
		goto	RET;
	/* */
	if ( prm->UseConic) {
		if ( sts = divVec( lstHdl, tmpLst, 2))
			goto	RET;
		if ( (sts = Smooth1( tmpLst, lstHdl, 1, 2, 1))<0)
			goto	RET;
		
		if ( (sts = Smooth2(  lstHdl, tmpLst))<0)
			goto	RET;
		
		if ( sts = FitConic(tmpLst,lstHdl, prm->SmoothLevel, fp))
			goto	RET;
	}
	else {
		if ( (sts = Smooth1( lstHdl, tmpLst, 1, 2, 1))<0)
			goto	RET;
		if ( (sts = Smooth2( tmpLst, lstHdl))<0)
			goto	RET;
	}
	if ( (sts = DelZeroVec( lstHdl, tmpLst))<0)
		goto	RET;

	if ( (sts = straightCurve( tmpLst, lstHdl))<0)
		goto	RET;

	if ( (sts = DelZeroVec( lstHdl, tmpLst))<0)
		goto	RET;
	sts = BindLinears( tmpLst, lstHdl);


RET:
	return  sts;
}
/***********************************************************************
 *	Bind Same Direction Unit Vector
 */
/* */	static int
/* */	BindVector(
/* */		int inLst, 	/* Absolute Coordinate */
/* */		int outLst)	/* Relative one */
/*
 *	reuturns : 0, -1
 ***********************************************************************/
{
struct VHEAD	*vhd;
struct VDATA	*vp;
struct vecdata	rel, nxtrel;
	int	np;
	int	sts;

	if ( (sts = VDGetHead( inLst, &vhd))!=0)
		goto	RET;
	VDNew(outLst);

	nxtrel.atr = 1;
	while ( vhd->next!=NIL) {
		/* No point data is not Allowed */
		/* Set Origin */
		vp = vhd->headp;
		if ((sts = VDSetData(outLst, &vp->vd))!=0)
			goto	RET;
		rel.x = rel.y = 0;
		for ( np = 0; np < vhd->nPoints; np++,vp = vp->next) {
			nxtrel.x = vp->next->vd.x - vp->vd.x;
			nxtrel.y = vp->next->vd.y - vp->vd.y;
			if ( rel.x==0 && rel.y ==0)
				rel = nxtrel;
			else  {
				if ( rel.y==0 && nxtrel.y==0)
					rel.x += nxtrel.x;
				else if ( rel.x == 0 && nxtrel.x==0)
					rel.y += nxtrel.y;
				else {
					if ((sts=VDSetData(outLst, &rel))!=0)
						goto	RET;
					rel = nxtrel;
				}
			}
		}
		if ( rel.x!=0 || rel.y!=0) {
			if ((sts=VDSetData(outLst, &rel))!=0)
				goto	RET;
		}
		vhd = vhd->next;
		if ((sts = VDClose(outLst))!=0)
			goto	RET;
	}
RET:
	return( sts);
	
}
/***********************************************************************
 *	Bind Same Direction Unit Vector(Absolute Coodinate Version)
 *		Only for short vector ( dont care SPLINE )
 */
/* */	static int
/* */	BindPoint(
/* */		int inLst, 	/* Absolute Coordinate */
/* */		int outLst)	/* .. */
/*
 *	reuturns : 0, -1
 ***********************************************************************/
{
struct VHEAD	*vhd;
struct VDATA	*vp;
struct vecdata	lastvd;
	int	np;
	int	sts;

	if ( (sts = VDGetHead( inLst, &vhd))!=0)
		goto	RET;
	VDNew(outLst);

	while ( vhd->next!=NIL) {
		/* No point data is not Allowed */
		/* Set Origin */
		vp = vhd->headp;
		if ((sts = VDSetData(outLst, &vp->vd))!=0)
			goto	RET;
		lastvd = vp->vd;
		for ( np = 0; np < vhd->nPoints; np++,vp = vp->next) {
			if (vp->next->vd.x != lastvd.x && vp->next->vd.y != lastvd.y) {
				if ((sts=VDSetData(outLst, &vp->vd))!=0)
					goto	RET;
				lastvd = vp->vd;
			}
		}
		vhd = vhd->next;
		if ((sts = VDClose(outLst))!=0)
			goto	RET;
	}
RET:
	return( sts);
	
}
/***********************************************************************
 *	Detect Corner and edge
 */
/* */	static int
/* */	DetectNoSmooth(
/* */		int lsthdl)	/* Absolute Coordinate */
/*
 *	returns : 0, -1
 ***********************************************************************/
{
struct VHEAD	*vhd;
	int	sts;
struct VDATA	*vp;
	int	np;


	if ( (sts = VDGetHead( lsthdl, &vhd))!=0)
		goto	RET;
	while ( vhd->next != NIL) {
		vp = vhd->headp;
		for ( np = 0; np < vhd->nPoints; np++, vp = vp->next) {
			if (  isturn( vp) )
				turnsort(vp);
			else if (islong (vp))
				longline( vp);
			else	/* based on general rule */
				generalsmooth(vp);
		}
		vhd = vhd->next;
	}
RET:
	return sts;
}
/***********************************************************************
 *	Judge smooth or not at Turning Part
 */
/* */	static void
/* */	turnsort(
/* */		struct VDATA *vp)
/*
 *	returns : none
 ***********************************************************************/
{
struct vecdata	cur, prev1, prev2, next1, next2;

	cur.x = vp->vd.x-vp->prev->vd.x;
	cur.y = vp->vd.y-vp->prev->vd.y;
	prev1.x = vp->prev->vd.x-vp->prev->prev->vd.x;
	prev1.y = vp->prev->vd.y-vp->prev->prev->vd.y;
	prev2.x = vp->prev->prev->vd.x-vp->prev->prev->prev->vd.x;
	prev2.y = vp->prev->prev->vd.y-vp->prev->prev->prev->vd.y;
	next1.x = vp->next->vd.x - vp->vd.x;
	next1.y = vp->next->vd.y - vp->vd.y;
	next2.x = vp->next->next->vd.x - vp->next->vd.x;
	next2.y = vp->next->next->vd.y - vp->next->vd.y;

	if ( cur.x) {
		if ( abs(next1.y) > 1)
			vp->vd.atr |= NOSMOOTH ;
		else if ( sign( cur.x) == sign( next2.x))
			vp->vd.atr |= SMOOTH | BACKWARD;
		else
			vp->vd.atr |= NOSMOOTH;
		if ( abs(prev1.y) > 1)
			vp->prev->vd.atr |= NOSMOOTH ;
		else if (sign(cur.x)==sign(prev2.x))
			vp->prev->vd.atr |= SMOOTH | FOREWARD;
		else
			vp->prev->vd.atr |= NOSMOOTH ;
	}
	else {
		if ( abs(next1.x) > 1)
			vp->vd.atr |= NOSMOOTH ;
		else if ( sign( cur.y) == sign( next2.y))
			vp->vd.atr |= SMOOTH | BACKWARD;
		else
			vp->vd.atr |= NOSMOOTH;
		if ( abs(prev1.x) > 1)
			vp->prev->vd.atr |= NOSMOOTH ;
		else if (sign(cur.y)==sign(prev2.y))
			vp->prev->vd.atr |= SMOOTH | FOREWARD;
		else
			vp->prev->vd.atr |= NOSMOOTH;
	}
	return;
}
/***********************************************************************
 *	Judge Smooth or not with general rule
 */
/* */	static void
/* */	generalsmooth( struct VDATA *vp)
/*
 *	returns : none
 ***********************************************************************/
{
struct vecdata	prev2, prev1, cur, next1, next2, next3;

	cur.x = vp->vd.x- vp->prev->vd.x;
	cur.y = vp->vd.y- vp->prev->vd.y;
	prev1.x = vp->prev->vd.x - vp->prev->prev->vd.x;
	prev1.y = vp->prev->vd.y - vp->prev->prev->vd.y;
	prev2.x = vp->prev->prev->vd.x - vp->prev->prev->prev->vd.x;
	prev2.y = vp->prev->prev->vd.y - vp->prev->prev->prev->vd.y;
	next1.x = vp->next->vd.x - vp->vd.x;
	next1.y = vp->next->vd.y - vp->vd.y;
	next2.x = vp->next->next->vd.x - vp->next->vd.x;
	next2.y = vp->next->next->vd.y - vp->next->vd.y;
	next3.x = vp->next->next->next->vd.x - vp->next->next->vd.x;
	next3.y = vp->next->next->next->vd.y - vp->next->next->vd.y;

	if ( cur.y) {
		if ( abs(next1.x)>1 && abs(cur.y)>1) {
			if ( sign(cur.y) == sign(next1.x)
			   && abs(next2.y)==1
			   && abs(prev1.x)==1
			   && sign(cur.y)==sign(prev2.y)
			   && sign(next1.x)==sign( next3.x))
				;
			else
				vp->vd.atr |= NOSMOOTH;
		}
	}
	else {
		if ( abs(next1.y)>1 && abs(cur.x)>1) {
			if ( sign(cur.x) != sign(next1.y)
			   && abs(next2.x)==1
			   && abs(prev1.y)==1
			   && sign(cur.x)==sign(prev2.x)
			   && sign(next1.y)==sign( next3.y))
				;
			else
				vp->vd.atr |= NOSMOOTH;
		}
	}
	return;
}
/***********************************************************************
 *	Judge turning
 */
/* */	static int
/* */	isturn( struct VDATA	*vp)		/* Absolute */
/*
 *	retuns
 ***********************************************************************/
{
	int	turnSort;
struct vecdata	prev, cur, next;

	prev.x = vp->prev->vd.x - vp->prev->prev->vd.x;
	prev.y = vp->prev->vd.y - vp->prev->prev->vd.y;
	cur.x = vp->vd.x - vp->prev->vd.x;
	cur.y = vp->vd.y - vp->prev->vd.y;
	next.x = vp->next->vd.x - vp->vd.x;
	next.y = vp->next->vd.y - vp->vd.y;

	if ( prev.x !=0) {
		if ( sign(prev.x )!=sign(next.x )) {
			if (sign(next.x ) ==sign(cur.y))
				turnSort = TURN_TOTSU;
			else
				turnSort = TURN_OU;
		}
		else	turnSort = 0;
	}
	else {
		if ( sign(prev.y )!=sign(next.y )) {
			if (sign(next.y ) ==sign(cur.x))
				turnSort = TURN_OU;
			else
				turnSort = TURN_TOTSU;
		}
		else	turnSort = 0;
	}
	return	turnSort;
}

/***********************************************************************
 *	judge Long Line or Not
 */
/* */	static int
/* */	islong( struct VDATA *vp)
/*
 *	returns : 0, 1
 ***********************************************************************/
{
struct vecdata	cur, prev1, prev2, next1, next2;
	int	sts;

	cur.x = vp->vd.x - vp->prev->vd.x;
	cur.y = vp->vd.y - vp->prev->vd.y;
	if ( abs(cur.x)<= LONGMIN && abs(cur.y) <= LONGMIN)
		return( 0);
	prev2.x = vp->prev->prev->vd.x - vp->prev->prev->prev->vd.x;
	prev2.y = vp->prev->prev->vd.y - vp->prev->prev->prev->vd.y;
	prev1.x = vp->prev->vd.x - vp->prev->prev->vd.x;
	prev1.y = vp->prev->vd.y - vp->prev->prev->vd.y;
	next1.x = vp->next->vd.x - vp->vd.x;
	next1.y = vp->next->vd.y - vp->vd.y;
	next2.x = vp->next->next->vd.x - vp->next->vd.x;
	next2.y = vp->next->next->vd.y - vp->next->vd.y;
	if ( cur.x) {
		if ( (sign(cur.x) != sign(next2.x)
			|| abs(next1.y)>1
			|| ( abs(next1.y)==1 && abs(cur.x)/2 > abs(next2.x)))
		 && (sign(cur.x) != sign(prev2.x)
			|| abs(prev1.y)>1
			|| ( abs(prev1.y)==1 && abs(cur.x)/2 > abs(prev2.x))))
			sts = 1;
		/* 94.11.09 */
                else if ( abs(cur.x) > 2 && (abs(next1.y)>2 || abs(prev1.y)>2))
			sts = 1;
		/* */
		else	sts = 0;
	}
	else {
		if ( (sign(cur.y) != sign(next2.y)
			|| abs(next1.x)>1
			|| ( abs(next1.x)==1 && abs(cur.y)/2 > abs(next2.y)))
		 && (sign(cur.y) != sign(prev2.y)
			|| abs(prev1.x)>1
			|| ( abs(prev1.x)==1 && abs(cur.y)/2 > abs(prev2.y))))
			sts = 1;
		/* 94.11.09 */
                else if ( abs(cur.y) > 2 && (abs(next1.x)>2 || abs(prev1.x)>2))
			sts = 1;
		/* */
		else	sts = 0;
	}
	return sts;
}
/***********************************************************************
 *	judge Long Line or Not
 */
/* */	static void
/* */	longline( struct VDATA *vp)
/*
 *	returns : 0, 1
 ***********************************************************************/
{
struct vecdata	cur, prev1, prev2, next1, next2;

	prev2.x = vp->prev->prev->vd.x - vp->prev->prev->prev->vd.x;
	prev2.y = vp->prev->prev->vd.y - vp->prev->prev->prev->vd.y;
	prev1.x = vp->prev->vd.x - vp->prev->prev->vd.x;
	prev1.y = vp->prev->vd.y - vp->prev->prev->vd.y;
	cur.x = vp->vd.x - vp->prev->vd.x;
	cur.y = vp->vd.y - vp->prev->vd.y;
	next1.x = vp->next->vd.x - vp->vd.x;
	next1.y = vp->next->vd.y - vp->vd.y;
	next2.x = vp->next->next->vd.x - vp->next->vd.x;
	next2.y = vp->next->next->vd.y - vp->next->vd.y;

	if ( cur.x) {
		/* 前方 */
		if ( abs(next1.y)!=1 || sign(cur.x)!=sign(next2.x))
			vp->vd.atr |= NOSMOOTH;
		else if ( abs( next2.x) ==1 && sign(cur.x)==sign(next1.y))
			vp->vd.atr |= NOSMOOTH;
		else
			vp->vd.atr |= NOSMOOTH | BACKWARD;
		/* 後方 */
		if ( abs(prev1.y)!=1 || sign(cur.x)!=sign(prev2.x))
			vp->prev->vd.atr |= NOSMOOTH;
		else if ( abs( prev2.x) ==1&& sign(cur.x)!=sign(prev1.y))
			vp->prev->vd.atr |= NOSMOOTH;
		else
			vp->prev->vd.atr |= NOSMOOTH | FOREWARD;
	}
	else {
		/* 前方 */
		if ( abs(next1.x)!=1 || sign(cur.y)!=sign(next2.y))
			vp->vd.atr |= NOSMOOTH;
		else if ( abs( next2.y) ==1 && sign(cur.y)!=sign(next1.x))
			vp->vd.atr |= NOSMOOTH;
		else
			vp->vd.atr |= NOSMOOTH | BACKWARD;
		/* 後方 */
		if ( abs(prev1.x)!=1 || sign(cur.y)!=sign(prev2.y))
			vp->prev->vd.atr |= NOSMOOTH;
		else if ( abs( prev2.y) ==1&& sign(cur.y)==sign(prev1.x))
			vp->prev->vd.atr |= NOSMOOTH;
		else
			vp->prev->vd.atr |= NOSMOOTH | FOREWARD;
	}
}
/***********************************************************************
 *	Zoom for fixed point
 */
/* */	static int
/* */	ZoomPoint(
/* */		int	lstHdl,		/* List Handle to proccess */
/* */		int	xinMesh,	/* INPut Dot Mesh */
/* */		int	yinMesh,
/* */		int	outMesh,	/* output mesh (256) */
/* */		int	uFp)		/* Under Fixed Point Value */
/*
 *	returns : none
 *	remarks : 処理結果の座標系は、(128,128)[=outMesh/2]を原点に移動し、
 *		　UFpを掛けた値である
 ***********************************************************************/
{
struct VHEAD	*vhd;
	int	sts;
struct VDATA	*vp;
	int	np;
	int	center;

	center = outMesh/2*uFp;

	if ( (sts = VDGetHead( lstHdl, &vhd))!=0)
		goto	RET;
	while ( vhd->next != NIL) {
		vp = vhd->headp;
		for ( np = 0; np < vhd->nPoints; np++, vp = vp->next) {
			vp->vd.x =(short)((long)vp->vd.x*outMesh*uFp/xinMesh)-center;
			vp->vd.y =(short)((long)vp->vd.y*outMesh*uFp/yinMesh)-center;
		}
		vhd = vhd->next;
	}
RET:
	return	sts;
}
/***********************************************************************
 *	Pre-Proc1 of smoothing( divide long vector )
 */
/* */	static int
/* */	preproc1( int inLst, int outLst)
/*
 *	returns : 0. -1
 ***********************************************************************/
{
struct VHEAD	*vhd;
	int	sts;
struct VDATA	*vp;
	int	np;
struct vecdata	vd;

	if ( (sts = VDGetHead( inLst, &vhd))!=0)
		goto	RET;
	VDNew( outLst);
	while ( vhd->next != NIL) {
		vp = vhd->headp;
		for ( np = 0; np < vhd->nPoints; ) {
			if ( vp->vd.atr & FOREWARD) {
				if ( vp->next->vd.atr& BACKWARD) {
					if ((sts = diveachside( vp, outLst))<0)
						goto	RET;
					np++;
					vp = vp->next;
				}
				else {
					if ((sts=divoneside(vp, outLst))<0)
						goto	RET;
				}
			}
			else if ((vp->vd.atr & BACKWARD)
				&& (vp->prev->vd.atr & FOREWARD)==0){

				if ( (sts = divoneside( vp, outLst))<0)
					goto	RET;

			}
			else {	/* only copy */
				vd = vp->vd;
				if ( vp->vd.atr & NOSMOOTH)
					vd.atr = SMOOTHANCHOR;
				if ((sts=VDSetData( outLst, &vd))<0)
					goto	RET;

			}
			np++;
			vp = vp->next;
		}

		if ( (sts = VDClose( outLst))<0)
			break;

		vhd = vhd->next;
	}
RET:
	return sts;
}
/***********************************************************************
 *	Divide Vector from each side( for PreProc)
 */
/* */	static int
/* */	diveachside( struct VDATA *vp, int outLst)
/*
 *	returns : 0, -1
 ***********************************************************************/
{
struct vecdata	prev, cur, next1, next2, next3;
struct vecdata	div1, div2;
	int	sts;

	prev.x = vp->prev->vd.x - vp->prev->prev->vd.x;
	prev.y = vp->prev->vd.y - vp->prev->prev->vd.y;
	cur.x = vp->vd.x - vp->prev->vd.x;
	cur.y = vp->vd.y - vp->prev->vd.y;
	next1.x = vp->next->vd.x - vp->vd.x;
	next1.y = vp->next->vd.y - vp->vd.y;
	next2.x = vp->next->next->vd.x - vp->next->vd.x;
	next2.y = vp->next->next->vd.y - vp->next->vd.y;
	next3.x = vp->next->next->next->vd.x - vp->next->next->vd.x;
	next3.y = vp->next->next->next->vd.y - vp->next->next->vd.y;
	sts = 0;
	if ( next1.x) {
		if ( (sts = VDSetData( outLst, &vp->vd))<0)
			goto	RET;
		div1.y = vp->vd.y;
		div2.y = vp->vd.y;
		div1.atr = SMOOTHANCHOR;
		div2.atr = SMOOTHANCHOR;
		if ( abs(prev.x) + abs( next3.x) <= abs(next1.x)) {
			div1.x = vp->vd.x + prev.x;
			if ( (sts = VDSetData( outLst, &div1))<0)
				goto	RET;
			div2.x = vp->next->vd.x - next3.x;
			if ( (sts = VDSetData( outLst, &div2))<0)
				goto	RET;
			if ( (sts = VDSetData( outLst, &vp->next->vd))<0)
				goto	RET;
		}
		else {
			div1.x = (short)((long)next1.x*abs(prev.x)/abs(prev.x + next3.x));
			if ( div1.x != 0) {
				div1.x += vp->vd.x;
				if ( (sts = VDSetData( outLst, &div1))<0)
					goto	RET;
				if ( (sts = VDSetData( outLst, &vp->next->vd))<0)
					goto	RET;
			}
			else {
				div1.x = vp->next->vd.x;
				div1.y = vp->next->vd.y;
				if ( (sts = VDSetData( outLst, &div1))<0)
					goto	RET;
			}
		}
	}
	else {
		div1.x = vp->vd.x;
		div2.x = vp->vd.x;
		div1.atr = SMOOTHANCHOR;
		div2.atr = SMOOTHANCHOR;
		if ( (sts = VDSetData( outLst, &vp->vd))<0)
			goto	RET;
		if ( abs(prev.y) + abs( next3.y) <= abs(next1.y)) {
//			div1.y = vp->vd.y + prev.y;
			div1.y = vp->vd.y + prev.y/2;
			if ( (sts = VDSetData( outLst, &div1))<0)
				goto	RET;
//			div2.y = vp->next->vd.y - next3.y;
			div2.y = vp->next->vd.y - next3.y/2;
			if ( (sts = VDSetData( outLst, &div2))<0)
				goto	RET;
			if ( (sts = VDSetData( outLst, &vp->next->vd))<0)
				goto	RET;
		}
		else {
			div1.y = (short)((long)next1.y*abs(prev.y)/abs(prev.y + next3.y));
			if ( div1.y!=0) {
				div1.y += vp->vd.y;
				if ( (sts = VDSetData( outLst, &div1))<0)
					goto	RET;
				if ( (sts = VDSetData( outLst, &vp->next->vd))<0)
					goto	RET;
			}
			else {
				div1.x = vp->next->vd.x;
				div1.y = vp->next->vd.y;
				if ( (sts = VDSetData( outLst, &div1))<0)
					goto	RET;
			}
		}
	}
RET:
	return( sts);
}
/***********************************************************************
 *
 */
/* */	static int
/* */	divoneside(
/* */	struct VDATA	*vp,
/* */		int	outLst)
/*
 *
 ***********************************************************************/
{
struct vecdata	prev, cur, next1, next2;
struct vecdata	div1;
	int	sts;

	sts = 0;
	div1.atr = SMOOTHANCHOR;
	if ( vp->vd.atr & FOREWARD) {
		if ( (sts = VDSetData( outLst, &vp->vd))<0)
			goto	RET;
		prev.x = vp->prev->vd.x - vp->prev->prev->vd.x;
		prev.y = vp->prev->vd.y - vp->prev->prev->vd.y;
		cur.x = vp->vd.x - vp->prev->vd.x;
		cur.y = vp->vd.y - vp->prev->vd.y;
		next1.x = vp->next->vd.x - vp->vd.x;
		next1.y = vp->next->vd.y - vp->vd.y;

		if ( vp->vd.x == vp->prev->vd.x) {
			if ( abs(prev.x) < abs(next1.x)) {
				div1.y = vp->vd.y;
//				div1.x = vp->vd.x + prev.x;
				div1.x = vp->vd.x + prev.x/2;
				if ( (sts = VDSetData( outLst, &div1))<0)
					goto	RET;
			}
		}
		else {
			if ( abs(prev.y) < abs(next1.y)) {
				div1.x = vp->vd.x;
//				div1.y = vp->vd.y + prev.y;
				div1.y = vp->vd.y + prev.y/2;
				if ( (sts = VDSetData( outLst, &div1))<0)
					goto	RET;
			}
		}
	}
	else {
		cur.x = vp->vd.x - vp->prev->vd.x;
		cur.y = vp->vd.y - vp->prev->vd.y;
		next2.x = vp->next->next->vd.x - vp->next->vd.x;
		next2.y = vp->next->next->vd.y - vp->next->vd.y;
		if ( vp->vd.x == vp->next->vd.x) {
			if ( abs(cur.x) > abs(next2.x)) {
//				div1.x = vp->vd.x - next2.x;
				div1.x = vp->vd.x - next2.x/2;
				div1.y = vp->vd.y;
				if ( (sts = VDSetData( outLst, &div1))<0)
					goto	RET;
			}
		}
		else {
			if ( abs(cur.y) > abs(next2.y)) {
				div1.x = vp->vd.x;
//				div1.y = vp->vd.y - next2.y;
				div1.y = vp->vd.y - next2.y/2;
				if ( (sts = VDSetData( outLst, &div1))<0)
					goto	RET;
			}
		}
		if ( (sts = VDSetData( outLst, &vp->vd))<0)
			goto	RET;
	}
RET:
	return( sts);
}
/***********************************************************************
 *	Smoothing 1
 */
/* */	static int
/* */	Smooth1( int	inLst,
/* */		int	outLst,
/* */		int	weight1,		/* Waight of Smoothing Form.*/
/* */		int	weight2,
/* */		int	weight3	)
/*
 *	returns : 0,  -1
 ***********************************************************************/
{
struct VHEAD	*vhd;
	int	sts;
struct VDATA	*svp, *evp;
	int	sp, ep;

	if ( (sts = VDGetHead( inLst, &vhd))!=0)
		goto	RET;
	VDNew( outLst);
	while ( vhd->next != NIL) {
	    svp = vhd->headp;
	    if ((sp=searchanchor(0,svp,&svp,vhd->nPoints))<vhd->nPoints){
			
		while(sp <vhd->nPoints) {
			ep = searchanchor(sp+1, svp->next,&evp, vhd->nPoints);
			if ((sts=smooth1sub( svp, ep-sp,outLst,weight1, weight2, weight3))<0)
				goto	RET;
			if ( ep >= vhd->nPoints)
				break;
			sp = ep;
			svp = evp;
		}
	    }
	    vhd = vhd->next;
	    if ( (sts = VDClose( outLst)) < 0)
		break;
	}
RET:
	return	sts;
}
/***********************************************************************
 *	Smoothing 2 ( mabiki)
 */
/* */	static int
/* */	Smooth2( int inLst, int outLst )
/*
 *	returns : 0,  -1
 ***********************************************************************/
{
struct VHEAD	*vhd;
	int	sts;
struct VDATA	*svp, *evp;
	int	sp, ep;

	if ( (sts = VDGetHead( inLst, &vhd))!=0)
		goto	RET;
	VDNew( outLst);
	while ( vhd->next != NIL) {
	    svp = vhd->headp;
	    if ((sp=searchanchor(0,svp,&svp,vhd->nPoints))<vhd->nPoints){
			
		while(sp <vhd->nPoints) {
			ep = searchanchor(sp+1, svp->next,&evp, vhd->nPoints);
			if ((sts=smooth2sub( svp, ep-sp,outLst))<0)
				goto	RET;
			if ( ep >= vhd->nPoints)
				break;
			sp = ep;
			svp = evp;
		}
	    }
	    vhd = vhd->next;
	    if ( (sts = VDClose( outLst)) < 0)
		break;
	}
RET:
	return	sts;
}
/***********************************************************************
 *	Search Smoothing Anchor Point
 */
/* */	int
/* */	searchanchor(
/* */		int	sn,
/* */	struct VDATA	*sp,
/* */	struct VDATA	**ep,
/* */		int	lim)
/*
 *
 ***********************************************************************/
{
	while ( lim-- > 0) {
		if (sp->vd.atr & SMOOTHANCHOR) {
			*ep = sp;
			return	sn;
		}
		sp = sp->next;
		sn++;
	}
	return( sn);
}
/***********************************************************************
 *	Smooth1 body
 */
/* */	static int
/* */	smooth1sub(
/* */	struct VDATA	*vp,
/* */		int	np,
/* */		int	outLst,
/* */		int	w1,		/* Waight of Smoothing Form.*/
/* */		int	w2,
/* */		int	w3)
/*
 *	returns : 0, -1
 ***********************************************************************/
{
	int	pcnt,
		sts;
struct vecdata	vd;
	int	base;

	if ( (sts = VDSetData( outLst, &vp->vd))<0)
		goto	RET;
	np--;
	vp = vp->next;
	base = w1 + w2 + w3;
	for ( pcnt = 0; pcnt < np; pcnt++, vp= vp->next) {
		vd.x = (vp->prev->vd.x*w1 + vp->vd.x*w2 + vp->next->vd.x*w3)/base;
		vd.y = (vp->prev->vd.y*w1 + vp->vd.y*w2 + vp->next->vd.y*w3)/base;
		vd.atr = vp->vd.atr;
		if ( (sts = VDSetData( outLst, &vd))<0)
			break;
	}
RET:
	return( sts);
}
/***********************************************************************
 *	Smooth2 body ( mabiki body)
 */
/* */	static int
/* */	smooth2sub(
/* */	struct VDATA	*vp,
/* */		int	np,
/* */		int	outLst)
/*
 *	returns : 0, -1
 ***********************************************************************/
{
	int	pcnt,
		sts;

	if ( (sts = VDSetData( outLst, &vp->vd))<0)
		goto	RET;
	np--;
	vp = vp->next;
	for ( pcnt = 0; pcnt < np; pcnt++, vp= vp->next) {
		if ( np < 3 || pcnt % 2 == 0) {
			if ( (sts = VDSetData( outLst, &vp->vd))<0)
				break;
		}
	}
RET:
	return( sts);
}
/***********************************************************************
 *	Remove Fixed Point
 */
/* */	int
/* */	RemoveFp(
/* */		int	lstHdl,		/* List Handle to proccess */
/* */		int	outMesh,	/* output mesh */
/* */		int	uFp)		/* Under Fixed Point Value */
/*
 *	returns : none
 *	remarks : スムーズィングの処理結果の座標系は、UFpで割って原点を
 *		  (128,128) [=outMesh/2]に移動した値である
 ***********************************************************************/
{
struct VHEAD	*vhd;
	int	sts;
struct VDATA	*vp;
	int	center;
	int	np;

	center = outMesh/2;

	if ( (sts = VDGetHead( lstHdl, &vhd))!=0)
		goto	RET;
	while ( vhd->next != NIL) {
		vp = vhd->headp;
		for ( np = 0; np < vhd->nPoints; np++, vp = vp->next) {
			vp->vd.x = vp->vd.x / uFp+center;
			if ( vp->vd.x < 0)	vp->vd.x = 0;
			else if  ( vp->vd.x >= outMesh)	vp->vd.x = outMesh-1;

			vp->vd.y = vp->vd.y / uFp+center;
			if ( vp->vd.y < 0)	vp->vd.y = 0;
			else if  ( vp->vd.y >= outMesh)	vp->vd.y = outMesh-1;
		}
		vhd = vhd->next;
	}
RET:
	return sts;
}

/***********************************************************************
 *	Bind same direction vector
 */
/* */	static int
/* */	BindLinears( int inLst, int outLst)
/*
 ***********************************************************************/
{
struct VHEAD	*vhd;
	int	sts;
struct VDATA	*vp;
	int	np;
struct vecdata	rel1, rel2;

	VDNew( outLst);
	if ( (sts = VDGetHead( inLst, &vhd))!=0)
		goto	RET;
	while ( vhd->next != NIL) {
		vp = vhd->headp;
		/* Origin */
		if ( (sts = VDSetData( outLst, &vp->vd))<0)
			goto	RET;
		vp = vp->next;
		for ( np = 1; np < vhd->nPoints; np++, vp = vp->next) {
			if ( np+1 >= vhd->nPoints) {
				if ((sts=VDSetData( outLst, &vp->vd))<0)
					goto	RET;
				break;
			}
			else if ( vp->vd.atr&SPLINE_ATR) {

				if ((sts=VDSetData( outLst, &vp->vd))<0)
					goto	RET;
				/* End Point */
				vp = vp->next;
				np++;
				if ( np >= vhd->nPoints)
					break;
				if ( (sts = VDSetData( outLst, &vp->vd))<0)
					goto	RET;
			}
			else {
			    rel1.x = vp->next->vd.x - vp->vd.x;
			    rel1.y = vp->next->vd.y - vp->vd.y;
			    rel2.x = vp->vd.x - vp->prev->vd.x;
			    rel2.y = vp->vd.y - vp->prev->vd.y;
			    if ( (vp->next->vd.atr & SPLINE_ATR )
				|| (long)rel1.x*rel2.y != (long)rel1.y*rel2.x) {
				if ( (sts = VDSetData( outLst, &vp->vd))<0)
					goto	RET;
			    }
			}
		}
		if ( (sts = VDClose( outLst))<0)
			goto	RET;
		vhd = vhd->next;
	}
RET:
	return( sts);
}
/***********************************************************************
 *	Deleate Zero Vector ( about Non Curve Part)
 */
/* */	static int
/* */	DelZeroVec( int inLst, int outLst)
/*
 ***********************************************************************/
{
struct VHEAD	*vhd;
	int	sts;
struct VDATA	*vp;
	int	np;

	VDNew( outLst);
	if ( (sts = VDGetHead( inLst, &vhd))!=0)
		goto	RET;
	while ( vhd->next != NIL) {
		vp = vhd->headp;
		/* Origin */
		if ( (sts = VDSetData( outLst, &vp->vd))<0)
			goto	RET;
		vp = vp->next;
		for ( np = 1; np < vhd->nPoints; np++, vp = vp->next) {
			if ( vp->vd.atr&SPLINE_ATR) {
				/* Control Point */
				if ( (sts = VDSetData( outLst, &vp->vd))<0)
					goto	RET;
				/* End Point */
				vp = vp->next;
				np++;
				if ( np >= vhd->nPoints)
					break;
				if ( (sts = VDSetData( outLst, &vp->vd))<0)
					goto	RET;
			}
			else {
				if ( vp->vd.x != vp->prev->vd.x
					   || vp->vd.y != vp->prev->vd.y) {
					if ((sts=VDSetData( outLst, &vp->vd))<0)
						goto	RET;
				}
			}
		}
		if ( (sts = VDClose( outLst))<0)
			goto	RET;
		vhd = vhd->next;
	}
RET:
	return( sts);
}
/*		X			  |Y
 *	O------------		     	+-|------+
 *	|	|			| |	 |
 *	|	|	---> 		| |	 |
 *	|	|		     -----+-------------X
 *	|-------+			O-|------+
 *	|Y				  |
 */
int
toTTFFrame( int lstH, struct BBX *bbx)
{
struct VHEAD	*vhd;
struct VDATA	*vp;
	int	np;
	int	sts;
	short	mesh;

	if ( (sts = VDGetHead( lstH, &vhd))!=0)
		goto	RET;
	if ( (sts = VDGetNCont( lstH))<=0)
		goto	RET;
	mesh = bbx->xMax - bbx->xMin+1;
	if ( mesh > bbx->yMax - bbx->yMin)
		mesh = bbx->yMax - bbx->yMin+1;
	while ( vhd->next != NIL) {
		vp = vhd->headp;
		np = vhd->nPoints;
		while ( np-->0) {
			vp->vd.x += (short)bbx->xMin;
			vp->vd.y = (mesh - 1) - vp->vd.y + bbx->yMin;
			/* Limit in BBX */
			if ( vp->vd.x > bbx->xMax)
				vp->vd.x = (short)bbx->xMax;
			else if (vp->vd.x < bbx->xMin)
				vp->vd.x = (short)bbx->xMin;
			if ( vp->vd.y > bbx->yMax)
				vp->vd.y = (short)bbx->yMax;
			else if (vp->vd.y < bbx->yMin)
				vp->vd.y = (short)bbx->yMin;
			vp = vp->next;
		}
		vhd = vhd->next;
	}
	sts = 0;
RET:
	return sts;
}
static int
lightSub( int inLst, int outLst)
{
struct VHEAD	*vhd;
	int	sts;
struct VDATA	*svp, *evp;
	int	sp, ep;
	if ( (sts = VDGetHead( inLst, &vhd))!=0)
		goto	RET;
	VDNew( outLst);
	while ( vhd->next != NIL) {
	    svp = vhd->headp;
	    if ((sp=searchanchor(0,svp,&svp,vhd->nPoints))<vhd->nPoints){
			
		while(sp <vhd->nPoints) {
			ep = searchanchor(sp+1, svp->next,&evp, vhd->nPoints);
			if ((sts=smooth1sub( svp, ep-sp,outLst,1, 2, 1))<0)
				goto	RET;
			if ( ep >= vhd->nPoints)
				break;
			sp = ep;
			svp = evp;
		}
	    }
	    vhd = vhd->next;
	    if ( (sts = VDClose( outLst)) < 0)
		break;
	}
RET:
	return	sts;
}
static int
divVec( int ilst, int olst, int divN)
{
struct VHEAD	*vhd;
struct VDATA	*vp;
struct vecdata	rel, vd;
	int	np;
	int	n;

	if ( VDGetHead( ilst, &vhd)!=0)
		goto	RET;
	VDNew( olst);
	while ( vhd->next != NIL) {
		vp = vhd->headp;
		np = vhd->nPoints;
		while ( np-->0) {
			if (VDSetData(olst, &vp->vd))
				goto	RET;
			rel.x = vp->next->vd.x - vp->vd.x;
			rel.y = vp->next->vd.y - vp->vd.y;
			vd.atr = 0;
			for ( n=1; n<divN; n++) {
				vd.x = vp->vd.x+rel.x*n/divN;
				vd.y = vp->vd.y+rel.y*n/divN;
				if (VDSetData(olst, &vd))
					goto	RET;
			}
			vp = vp->next;
		}
		vhd = vhd->next;
		VDClose( olst);
	}
	return 0;
RET:
	return -1;
}
int
SmoothLight( int ioLst, int tmpLst, int width, int height, int oWidth, int ufpVal)
{
	int	sts;

	if ( (sts = BindPoint( ioLst, tmpLst))<0)
		goto	RET;
	if ( (sts = DetectNoSmooth( tmpLst))<0)
		goto	RET;
	if ( (sts = ZoomPoint( tmpLst, width, height, oWidth, ufpVal))<0)
		goto	RET;
	if ( (sts = preproc1( tmpLst, ioLst))<0)
		goto	RET;


	if ( sts = divVec( ioLst, tmpLst, 2))
		goto	RET;
	if ( lightSub( tmpLst, ioLst))
		goto	RET;
	if ( lightSub( ioLst, tmpLst))
		goto	RET;

	if ( RemoveFp( tmpLst, oWidth, ufpVal))
		goto	RET;
	if ( (sts = DelZeroVec( tmpLst, ioLst))<0)
		goto	RET;
	return 0;
RET:
	return -1;
}
int
ConvMesh( int lstH, int inMesh, int outMesh)
{
struct VHEAD	*vhd;
struct VDATA	*vp;
	int	np;
	int	sts;
	if ( inMesh==outMesh)
		return 0;
	if ( (sts = VDGetHead( lstH, &vhd))!=0)
		goto	RET;
	if ( (sts = VDGetNCont( lstH))<=0)
		goto	RET;
	while ( vhd->next != NIL) {
		vp = vhd->headp;
		np = vhd->nPoints;
		while ( np-->0) {
			vp->vd.x = (int)((long)vp->vd.x*outMesh/inMesh);
			vp->vd.y = (int)((long)vp->vd.y*outMesh/inMesh);
			vp = vp->next;
		}
		vhd = vhd->next;
	}
	sts = 0;
RET:
	return sts;
}
/* EOF */
