//
// Copyright (c) 1997-1999 Microsoft Corporation.
//

#include	"stdafx.h"


#include	"vdata.h"
#include	"extfunc.h"
#define		sign(n)		(n < 0 ? 1 : 0)
#define		fpow(f)		((double)f*f)
#define		lpow(f)		((long)f*f)
#define		SPLINE_ATR	0x0001
#define	SMOOTHANCHOR	0x80
#define	SABUNMIN	(1*16)


int  FitConic(int  inLst,int  outLst,int  level,int  ufp);
static int  SetConic(struct  VDATA *svp,int  nElm,int  outLst,int  level);
static int  CurvTerm(struct  VDATA *svp,int  nElm,struct  VDATA * *term,int  level);
static void  sabun2(struct  VDATA *vp,struct  vecdata *vd);
static int  Fitting(struct  VDATA *sp,struct  VDATA *ep,int  nElm,int  outLst);
static void  onecurve(struct  VDATA *sp,struct  VDATA *midp,struct  VDATA *ep,struct  vecdata *cp);
static int  twocurve(struct  VDATA *sp,struct  VDATA *ep,int  nElm,struct  vecdata *cp,struct  vecdata *mid,struct  vecdata *cp2);
static long  getdist(struct  vecdata *lp1,struct  vecdata *lp2,struct  vecdata *p);
static int  CrsPnt(struct  vecdata *p1,struct  vecdata *p2,struct  vecdata *p3,struct  vecdata *p4,struct  vecdata *crsp);
static long  plen(struct  vecdata *p1,struct  vecdata *p2);
static int  isbind(struct  vecdata *v1,struct  vecdata *v2,struct  vecdata *v3);
static long  GetCurveHeight(struct  VDATA *sp,struct  VDATA *ep,int  nelm,struct  VDATA * *rvp,int  *pos);

static int	underFP;
/***********************************************************************
 *	Fit Conic Spline to Short Vector
 */
/* */	int
/* */	FitConic(
/* */		int	inLst, 
/* */		int	outLst,
/* */		int	level,		/* Smoothing Level 0-SMOOTHLEVELMAX-1 */
/* */		int	ufp)
/*
 *	returns : 0, -1
 ***********************************************************************/
{
struct VHEAD	*vhd;
	int	sts;
struct VDATA	*svp, *evp;
	int	sp, ep;
	int	ncont;

	underFP = ufp;
	if ( (sts = VDGetHead( inLst, &vhd))!=0)
		goto	RET;
	VDNew( outLst);
	ncont=0;
	while ( vhd->next != NIL) {
	    svp = vhd->headp;
	    if ((sp=searchanchor(0,svp,&svp,vhd->nPoints))<vhd->nPoints){
		while(sp<vhd->nPoints) {
			ep = searchanchor(sp+1, svp->next,&evp, vhd->nPoints);

			if ((sts = SetConic( svp, ep-sp, outLst, level))<0)
				goto	RET;

			if ( ep >= vhd->nPoints)
				break;
			sp = ep;
			svp = evp;
		}
	    }
	    ncont++;
	    if ((sts = VDClose( outLst))<0)
		goto	RET;
	    vhd = vhd->next;
	}
RET:
	return( sts);
}
/***********************************************************************
 *	Set Conic Curves to specified category
 */
/* */	static int
/* */	SetConic ( struct VDATA *svp, int nElm, int outLst, int level)
/*
 *	returns : 0, -1
 ***********************************************************************/
{
struct VDATA *vp;
	int	nPart;
	int	sts;

	
	sts = 0;
	while ( nElm > 0) {
		/* Divide non contigus */
		nPart = CurvTerm( svp,  nElm, &vp , level);
		sts = Fitting( svp, vp, nPart, outLst);
		svp = vp;
		nElm -= nPart;
	}
	return	sts;
}
/***********************************************************************
 *	Determine the part which is described with one Conic
 */
/* */	static int
/* */	CurvTerm(
/* */		struct VDATA *svp, 
/* */			int	nElm, 
/* */		struct VDATA **term,
/* */			int	level)
/*
 *	returns: none
 ***********************************************************************/
{
struct vecdata	sub1, sub2;
	int	ecnt;

	if ( nElm < 3) {
		for ( ecnt=0; ecnt < nElm; ecnt++)
			svp = svp->next;
		*term =  svp;
		return( nElm);
	}
	sabun2( svp, &sub1);
	for ( ecnt = 2; ecnt < nElm; ecnt++)	{
		svp = svp->next;
		sabun2( svp, &sub2);
		if ((sign(sub1.x)!=sign(sub2.x)
			&& abs(sub1.x)+abs(sub2.x)> SABUNMIN*(level+1))
		 || (sign(sub1.y)!=sign(sub2.y)
			 &&abs(sub1.y)+abs(sub2.y)> SABUNMIN*(level+1))) {
			*term = svp->next;
			return ecnt;
		}
		sub1 = sub2;
	}
	*term = svp->next->next;
	return	nElm;
}
/***********************************************************************
 *	2nd Sabun
 */
/* */	static void
/* */	sabun2( struct VDATA *vp, struct vecdata *vd)
/*
 *	returns : none
 ***********************************************************************/
{
	vd->x = vp->vd.x - vp->next->vd.x*2 + vp->next->next->vd.x;
	vd->y = vp->vd.y - vp->next->vd.y*2 + vp->next->next->vd.y;
	return;
}
/***********************************************************************
 *	Curve fit and set data
 */
/* */	static int
/* */	Fitting(
/* */		struct VDATA *sp,	/* Conic Start Point */
/* */		struct VDATA *ep,	/* Conic End Point */
/* */		int	nElm,
/* */		int	outLst)
/*
 *	returns : 0, -1
 ***********************************************************************/
{
struct vecdata	vd;
struct vecdata	cp, mid, cp2;
	int	sts;

	if ( nElm==0) 
		return 0;
	else if ( nElm == 1) {
		vd = sp->vd;
		if ((sts = VDSetData( outLst, &vd))<0)
			goto	RET;
	}
	else if ( nElm == 2) {
		onecurve( sp , sp->next, ep,  &cp );
		vd = sp->vd;
		if ((sts = VDSetData( outLst, &vd))<0)
			goto	RET;
		cp.atr = SPLINE_ATR;
		if ((sts = VDSetData( outLst, &cp))<0)
			goto	RET;
	}
	else {
		if ( twocurve( sp, ep, nElm, &cp, &mid, &cp2)) {
			vd = sp->vd;
			if ((sts = VDSetData( outLst, &vd))<0)
				goto	RET;
			cp.atr = SPLINE_ATR;
			if ((sts = VDSetData( outLst, &cp))<0)
				goto	RET;
			mid.atr = 0;
			if ((sts = VDSetData( outLst, &mid))<0)
				goto	RET;
			cp2.atr = SPLINE_ATR;
			if ((sts = VDSetData( outLst, &cp2))<0)
				goto	RET;
		}
		else {
			vd = sp->vd;
			if ((sts = VDSetData( outLst, &vd))<0)
				goto	RET;
			cp.atr = SPLINE_ATR;
			if ((sts = VDSetData( outLst, &cp))<0)
				goto	RET;
		}
	}
RET:
	return	sts;
}
/***********************************************************************
 *	Fit One Curve
 */
/* */	static void
/* */	onecurve( 
/* */	struct VDATA *sp, 
/* */	struct VDATA *midp, 
/* */	struct VDATA *ep, 
/* */	struct vecdata *cp)
/*
 *	returns : none
 ***********************************************************************/
{
struct vecdata	p;

	p.x = (midp->vd.x*4 - sp->vd.x - ep->vd.x)/2;
	p.y = (midp->vd.y*4 - sp->vd.y - ep->vd.y)/2;

	if ( sp->vd.x < ep->vd.x) {
		if ( p.x > ep->vd.x)	p.x = ep->vd.x;
		if ( p.x < sp->vd.x)	p.x = sp->vd.x;
	}
	else {
		if ( p.x < ep->vd.x)	p.x = ep->vd.x;
		if ( p.x > sp->vd.x)	p.x = sp->vd.x;
	}
	if ( sp->vd.y < ep->vd.y) {
		if ( p.y > ep->vd.y)	p.y = ep->vd.y;
		if ( p.y < sp->vd.y)	p.y = sp->vd.y;
	}
	else {
		if ( p.y < ep->vd.y)	p.y = ep->vd.y;
		if ( p.y > sp->vd.y)	p.y = sp->vd.y;
	}
	*cp = p;
}
/**********************************************************************
 *	Fit with a pair of Conic
 */
/* */	static int
/* */	twocurve( 
/* */		struct VDATA *sp, 
/* */		struct VDATA *ep, 
/* */		int	nElm,
/* */		struct vecdata *cp,
/* */		struct vecdata *mid,
/* */		struct vecdata *cp2)
/*
 *	returns : 0, 1( pair)
 **********************************************************************/
{
struct  VDATA	*midvp, *maxvp;
	long	maxlen;
	int	twin;
	long	sclen, eclen;
	int	pos;
	int	cp1pos, cp2pos;

	/* Get Curve Peak */
	maxlen = GetCurveHeight( sp, ep, nElm, &maxvp, &pos);
	/*
	if ( plen( &sp->vd, &ep->vd)/25 > maxlen) {
		onecurve( sp , maxvp, ep,  cp );
		twin = 0;
	}
	else {
	*/
		onecurve( sp , maxvp, ep,  cp );
		sclen = plen( &sp->vd, cp);
		eclen = plen( &ep->vd, cp);
		midvp = maxvp;
		if ( sclen > eclen*4 
		  || sclen*4 < eclen
		  || (long)(maxvp->vd.x - sp->vd.x)*(cp->x-sp->vd.x)<0L
		  || (long)(maxvp->vd.y - sp->vd.y)*(cp->y-sp->vd.y)<0L
		  || ( plen( &sp->vd, &ep->vd)>25L*25*underFP
			&& plen( &sp->vd, &ep->vd)<maxlen*64)) {

			maxlen=GetCurveHeight(sp, midvp, pos, &maxvp, &cp1pos);
			onecurve( sp, maxvp, midvp,  cp );
			maxlen=GetCurveHeight(midvp,ep,nElm-pos,&maxvp,&cp2pos);
			onecurve( midvp , maxvp, ep,  cp2 );
			*mid = midvp->vd;
			twin = 1;
		}
		else  {
			twin = 0;
		}
/*
	}
*/
	return( twin);
}
/**********************************************************************
 *	distance of point from line
 */	
/* */	static long
/* */	getdist(
/* */	struct vecdata	*lp1,
/* */	struct vecdata	*lp2,	/* Line p1, p2 */
/* */	struct vecdata	*p)		/* Point */
/*
 *	return :  Disttance 2nd Value
 **********************************************************************/
{
	long	a, b, c;
	long	height;

	a = - ( lp2->y - lp1->y);
	b = lp2->x - lp1->x;
	c = (long)(-lp1->y) * lp2->x + (long)(lp1->x) * lp2->y;
	if ( a==0 && b==0)
		height = c;
	else {
		height = a*p->x + b*p->y+c;
		if ( labs(height)>46340L)
			height = (long)(fpow(height)/(fpow(a)+fpow(b)));
		else
			height = lpow(height)/(lpow(a)+lpow(b));
	}
	return( height);
}
/**********************************************************************
 *	Cross Point
 */
/* */	static int
/* */	CrsPnt(
/* */	struct	vecdata	*p1,		/* vector point p1 to p2 */
/* */	struct	vecdata	*p2,
/* */	struct	vecdata	*p3,		/* vector point p3 to p4 */
/* */	struct	vecdata	*p4,
/* */	struct	vecdata	*crsp)
/*
 *	returns : 0, -1(no cross)
 **********************************************************************/
{
	int	rel1x, rel1y, rel2x, rel2y;
	long	cmul, cmul2;
	int	sts;

	rel1x = p2->x - p1->x;	
	rel1y = p2->y - p1->y;	
	rel2x = p4->x - p3->x;	
	rel2y = p4->y - p3->y;	
	cmul = (long)rel1y*rel2x - (long)rel2y*rel1x;

	if (cmul == 0L)
		sts = -1;
	else {
		cmul2 = (long)rel2x * (p3->y - p1->y) - 
			(long)rel2y * (p3->x - p1->x);
		crsp->x = (int)(cmul2*rel1x/cmul) + p1->x;
		crsp->y = (int)(cmul2*rel1y/cmul) + p1->y;
		if ( (long)rel1x*(crsp->x-p1->x)<0
		   || (long)rel1y*(crsp->y-p1->y)<0
		   || (long)rel2x*(crsp->x-p4->x)<0
		   || (long)rel2y*(crsp->y-p4->y)<0)
			sts = -1;
		else	sts = 0;
	}

	return	sts;
}
/**********************************************************************
 *	distance of points
 */	
/* */	static long
/* */	plen(
/* */		struct vecdata	*p1,
/* */		struct vecdata	*p2)
/*
 *	return :  Disttance 2nd Value
 **********************************************************************/
{
	return( lpow((long)(p2->x-p1->x)) + lpow((long)(p2->y - p1->y)));
}
/**********************************************************************
 *	bind two conic to one
 */
/* */	static  int
/* */	isbind( 
/* */		struct vecdata	*v1,
/* */		struct vecdata	*v2,
/* */		struct vecdata	*v3)
/*
 *	returns : 0, 1
 **********************************************************************/
{
	int	l12, l23;

	l12  = v1->x - v2->x;
	l23  = v2->x - v3->x;
	if ( sign( l12)!=sign(l23))
		goto	NORET;
	l12  = abs( l12);
	l23  = abs( l23);
	if ( (l12-l23)!=0) {
		if ( l12 / abs(l12 - l23) <10)
			goto	NORET;
	}
	l12  = v1->y - v2->y;
	l23  = v2->y - v3->y;
	if ( sign( l12)!=sign(l23))
		goto	NORET;
	l12  = abs( l12);
	l23  = abs( l23);
	if ( (l12-l23)!=0) {
		if ( l12 / abs(l12 - l23) <10)
			goto	NORET;
	}
	return 1;
NORET:
	return 0;
}
/**********************************************************************
 *	Get Curve liaison Peak height
 */
/* */	static long
/* */	GetCurveHeight( 
/* */	struct VDATA	*sp, 
/* */	struct VDATA	*ep, 
/* */		int	nelm, 
/* */	struct VDATA * * rvp,
/* */		int	*pos)
/*
 *	returns : maxlength( 2nd value)
 **********************************************************************/
{
	int	ecnt;
struct VDATA	*vp, *maxvp;
	long	maxlen, len;
	int	maxpos;

	/* Get Curve Peak */
	vp = sp->next;
	maxvp = vp;
	maxlen = 0;
	maxpos = 0;
	for ( ecnt = 0; ecnt < nelm-1; ecnt++, vp = vp->next) {
		len = getdist( &sp->vd, &ep->vd, &vp->vd);
		if ( len > maxlen) {
			maxlen = len;
			maxvp = vp;
			maxpos = ecnt;
		}
	}
	*rvp = maxvp;
	*pos = maxpos;
	return maxlen;
}
/* EOF */
