//
// Copyright (c) 1997-1999 Microsoft Corporation.
//

#include	"stdafx.h"


#include	"vdata.h"

#define		UNDERFP	0
#define		NMAX	129

int	MkPoly(	int inlst, int outLst);
static int DDACon(struct vecdata *s,struct vecdata *cp,struct vecdata *e,int lstHdl);
static int  fsqrt(int  n);
/***********************************************************************
 *	Make Poly line
 */
/* */	int
/* */	MkPoly(
/* */		int	inLst,
/* */		int	outLst)
/*
 *	returns : 0, -1
 ***********************************************************************/
{
struct VHEAD	*vhd;
	int	pcnt,
		sts;
struct VDATA	*vp;

	if ( (sts = VDGetHead( inLst, &vhd))!=0)
		goto	RET;
	VDNew( outLst);
	while ( vhd->next != NIL) {
		vp = vhd->headp;
		pcnt = vhd->nPoints;
		while ( pcnt >0) {
			if ( vp->next->vd.atr &1) {
				sts = DDACon( &vp->vd, &vp->next->vd,
					&vp->next->next->vd, outLst);
				if ( sts)
					goto	RET;
				vp = vp->next;
				pcnt--;
				if ( pcnt>0) {
					vp = vp->next;
					pcnt--;
				}
			}
			else {
				if ( (sts = VDSetData( outLst, &vp->vd))<0)
					goto	RET;
				vp = vp->next;
				pcnt--;
			}
		}
		vhd = vhd->next;
		if ( (sts = VDClose( outLst))<0)
			goto	RET;
	}
RET:
	return( sts);
}


static int	lowsqr[NMAX] = {
			   1,   2,   2,   3,   3,
			   4,   4,   4,   4,   5,
			   5,   5,   5,   6,   6,
			   6,   6,   6,   6,   7,

			   7,   7,   7,   7,   7,
			   8,   8,   8,   8,   8,
			   8,   8,   8,   9,   9,
			   9,   9,   9,   9,   9,

			   9,  10,  10,  10,  10,
			  10,  10,  10,  10,  10,
			  10,  11,  11,  11,  11,
			  11,  11,  11,  11,  11,

			  11,  12,  12,  12,  12,
			  12,  12,  12,  12,  12,
			  12,  12,  12,  13,  13,
			  13,  13,  13,  13,  13,

			  13,  13,  13,  13,  13,
			  14,  14,  14,  14,  14,
			  14,  14,  14,  14,  14,
			  14,  14,  14,  14,  15,

			  15,  15,  15,  15,  15,
			  15,  15,  15,  15,  15,
			  15,  15,  15,  16,  16,
			  16,  16,  16,  16,  16,

			  16,  16,  16,  16,  16,
			  16,  16,  16,  16,
			   };

static int	sqrtbl[NMAX] = {
			    1,    4,    9,   16,   25,
			   36,   49,   64,   81,  100,
			  121,  144,  169,  196,  225,
			  256,  289,  324,  361,  400,
			  441,  484,  529,  576,  625,
			  676,  729,  784,  841,  900,
			  961, 1024, 1089, 1156, 1225,
			 1296, 1369, 1444, 1521, 1600,
			 1681, 1764, 1849, 1936, 2025,
			 2116, 2209, 2304, 2401, 2500,
			 2601, 2704, 2809, 2916, 3025,
			 3136, 3249, 3364, 3481, 3600,
			 3721, 3844, 3969, 4096, 4225,
			 4356, 4489, 4624, 4761, 4900,
			 5041, 5181, 5329, 5476, 5625, 
			 5776, 5929, 6084, 6241, 6400,
			 6561, 6724, 6889, 7056, 7225,
			 7396, 7569, 7744, 7921, 8100,
			 8281, 8464, 8649, 8836, 9025,
			 9216, 9409, 9604, 9801, 10000,
			 10201, 10404, 10609, 10816, 11025,
			 11236, 11449, 11664, 11881, 12100,
			 12321, 12544, 12769, 12996, 13225,
			 13456, 13689, 13924, 14161, 14400,
			 14641, 14884, 15129, 15376, 15625,
			 15876, 16129, 16384, 16641 
			 };

/************************************************************
 *	DDA Poly line generate
 */
/* */	static int
/* */	DDACon(
/* */		struct vecdata	*s,
/* */		struct vecdata	*cp,
/* */		struct vecdata	*e,
/* */			int	lstHdl)
/*
 *	returns :  0, -1
 ************************************************************/
{
	int	n2xmax, n2ymax, n2max;
	int	i, n;
	long	f1x,		/* for f1　 */
		fx,		/* for F(i) */
		px,		/* for G(i) */
		g1x, g2x;
	long	f1y,		/* for f1　 */
		fy,		/* for F(i) */
		py,		/* for G(i) */
		g1y, g2y;
	int	sts, num;
	long	relx, rely; 	/* 相対座標 */
	int	n2, n2hlf;
struct vecdata	pntdata;

	/* Set Start Point */
	if ( (sts= VDSetData( lstHdl, s))<0)
		goto	RET;

	/* 分割数 Nを求める */
	n2xmax = (e->x - cp->x) - ( cp->x - s->x);

	if ( n2xmax < 0)	n2xmax = -n2xmax;
	else if ( n2xmax == 0 && cp->x==s->x)
		goto	TERM_SET;

	n2ymax = (e->y - cp->y) - ( cp->y - s->y);
	if ( n2ymax < 0)	n2ymax = -n2ymax;
	else if ( n2ymax == 0 && cp->y==s->y)
		goto	TERM_SET;

	if ( n2xmax > n2ymax)		n2max = (n2xmax*2) >> UNDERFP;
	else				n2max = (n2ymax*2) >> UNDERFP;
	n = fsqrt( n2max); 

	/* 分割点の座標を求める */ 
	if ( n > 1) {
		n2 = n*n;
		n2hlf = (n*n)/2;	 /* Expect Optimize */

		px = (long)s->x*n2;
		py = (long)s->y*n2;

		g2x = (e->x - cp->x) - (cp->x - s->x);
		g2y = (e->y - cp->y) - (cp->y - s->y);

		g1x = (long)n*(cp->x - s->x) *2;
		g1y = (long)n*(cp->y - s->y) *2;

		f1x = g2x*2;
		f1y = g2y*2;

		fx = g1x + g2x;
		fy = g1y + g2y;

		px += fx;
		py += fy;

		/* 最初の点を求める : 始点からの相対座標より計算する */
		relx = fx;
		rely = fy;
		if (relx >= 0)	relx += n2hlf;
		else		relx -= n2hlf;
		if (rely >= 0)	rely += n2hlf;
		else		rely -= n2hlf;

		pntdata.x = (int)(relx / n2 + s->x);
		pntdata.y = (int)(rely / n2 + s->y);
		pntdata.atr = 0;
		if ( (sts= VDSetData( lstHdl, &pntdata ))<0)
			goto	RET;
		if (n > 2) {
			num = n - 1;
			for ( i = 2; i < num; i++) {
				fx += f1x;
				fy += f1y;

				px +=  fx;
				py +=  fy;

				/* 最初と最後の点以外の点を絶対座標で計算する */
				pntdata.x = (int)((px + n2hlf) /n2);
				pntdata.y = (int)((py + n2hlf) /n2);
				if ( (sts= VDSetData(lstHdl, &pntdata ))<0)
					goto	RET;
			}
			fx += f1x;
			fy += f1y;

			px +=  fx;
			py +=  fy;

			relx = px - (long)e->x*n2;
			rely = py - (long)e->y*n2;
			if (relx >= 0)	relx += n2hlf;
			else		relx -= n2hlf;
			if (rely >= 0)	rely += n2hlf;
			else		rely -= n2hlf;
	
			/* 最後の点を求める : 終点からの相対座標より計算する */
			pntdata.x = (int)(relx / n2 + e->x);
			pntdata.y = (int)(rely / n2 + e->y);
			if ( (sts= VDSetData(lstHdl, &pntdata))<0)
				goto	RET;
		}
	}
TERM_SET:

	/* 曲線の終点は設定しない */
RET:
	return( sts);
}
/************************************************************
 *	Fast SQRT (Nの平方根を求める)
 */
/* */	static int
/* */	fsqrt( int n)
/*
 *	returns : 
 ************************************************************/
{
	int	i;

	if ( n < NMAX*2) {
		i = lowsqr[ n/2];
	}
	else {
		for ( i=0; i<NMAX; i++ )
			if ( sqrtbl[i] > n)	break;
		i++;
	}
	return( i );
}
/* EOF */
