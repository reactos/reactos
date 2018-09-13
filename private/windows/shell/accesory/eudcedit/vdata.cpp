//
// Copyright (c) 1997-1999 Microsoft Corporation.
//

#include	"stdafx.h"


#define		LISTDATAMAX	4

#define		NIL	(void *)0

struct vecdata	{
	short	x, y, atr;
	};

struct VDATA	{
	struct VDATA	*next, *prev;
	struct vecdata	vd;
	};

struct VHEAD	{
	struct VHEAD	*next, *prev;
	struct VDATA	*headp;
	int		nPoints;
	};
struct VCNTL	{
	struct VHEAD	*rootHead;
	struct VHEAD	*currentHead;
	int		nCont;
	struct VDATA	*cvp;
	int	mendp;
	void	*memroot;
	void	*cmem;
	};


int  VDInit(void);
void  VDTerm(void);
void  VDNew(int  lsthdl);
int  VDClose(int  lsthdl);
int  VDSetData(int  lsthdl,struct  vecdata *pnt);
int  VDGetData(int  lsthdl,int  contN,int  pn,struct  vecdata *pnt);
int  VDGetHead(int  lsthdl,struct  VHEAD * *vhd);
static void  *getmem(int  lsthdl,int  siz);
int  VDGetNCont(int  lstHdl);
int  VDReverseList(int  lstHdl);
int  VDCopy(int  srcH,int  dstH);

struct VCNTL	VCntlTbl[LISTDATAMAX];
#define		ALLOCMEMUNIT	2048

static int	init=0;
/***********************************************************************
 *	initialize data
 */
/* */	int
/* */	VDInit()
/*
 *	returns : 0, -1( out of memory)
 ***********************************************************************/
{
	int	lsthdl;
	void	*mem;

	if ( init)
		return( 0);
	for( lsthdl = 0; lsthdl < LISTDATAMAX; lsthdl++) {
		/* Allocate First memory */
		mem = (void *)malloc( ALLOCMEMUNIT);
		if ( mem==NIL)	
			return( -1);
		*((void **)mem) = NIL;
		VCntlTbl[lsthdl].memroot = mem;
		VDNew(lsthdl);
	}

	init = 1;
	return( 0);
}
/***********************************************************************
 *	Terminate
 */
/* */	void
/* */	VDTerm()
/*
 *	returns : none
 ***********************************************************************/
{
	void	*mem, *nextmem;
	int	lsthdl;
	if ( init) {
		for( lsthdl = 0; lsthdl < LISTDATAMAX; lsthdl++) {
			mem = VCntlTbl[lsthdl].memroot;
			do {
				nextmem = *((void * *)mem);
				free( mem);
				mem = nextmem;
			} while ( mem!=NIL);
		}
		init = 0;
	}
	return;
}
/***********************************************************************
 *	New Data
 */
/* */	void
/* */	VDNew(int lsthdl)
/*
 *	returns : none
 ***********************************************************************/
{
struct VCNTL	*vc;

	vc = VCntlTbl+lsthdl;
	vc->cmem = vc->memroot;
	vc->mendp  =  sizeof( void *);
	vc->currentHead = vc->rootHead = (struct VHEAD *)((char *)(vc->cmem)+vc->mendp);
	vc->mendp += sizeof( struct VHEAD);

	vc->currentHead->prev = (struct VHEAD *)NIL;
	vc->currentHead->next = (struct VHEAD *)NIL;
	vc->currentHead->headp = (struct VDATA *)NIL;
	vc->currentHead->nPoints = 0;
	vc->cvp = (struct VDATA *)NIL;
	vc->nCont = 0;

}
/***********************************************************************
 *	Close Contour
 */
/* */	int
/* */	VDClose(int lsthdl)
/*
 *	returns : none
 ***********************************************************************/
{
struct VHEAD	*vh;
struct VCNTL	*vc;

	vc = VCntlTbl+lsthdl;

	vc->cvp->next = vc->currentHead->headp;
	vc->currentHead->headp->prev = vc->cvp;
	vh = (struct VHEAD *)getmem( lsthdl, sizeof(struct VHEAD));
	if ( vh == NIL)	return( -1);
	vc->currentHead->next = vh;
	vh->prev = vc->currentHead;
	vh->next = (struct VHEAD *)NIL;
	vh->headp = (struct VDATA *)NIL;
	vh->nPoints = 0;
	vc->currentHead = vh;
	vc->cvp = (struct VDATA *)NIL;
	vc->nCont++;

	return (0);
}
/***********************************************************************
 *	Set Data
 */
/* */	int
/* */	VDSetData ( 
/* */		int lsthdl,
/* */	struct vecdata	*pnt)
/*
 *	return : 0, -1 ( no memory)
 ***********************************************************************/
{
	void	*mem;
struct VCNTL	*vc;

	vc = VCntlTbl+lsthdl;

	mem = getmem( lsthdl,sizeof(  struct VDATA));
	if ( mem == NIL) {
		return -1;
	}
	if ( vc->cvp== NIL) {	/* Contour First Point*/
		vc->cvp = vc->currentHead->headp = (struct VDATA *)mem;
		vc->cvp->vd = *pnt;
		vc->cvp->next = vc->cvp->prev= (struct VDATA *)NIL;
	}
	else {
		vc->cvp->next = (struct VDATA *)mem;
		vc->cvp->next->prev = vc->cvp;
		vc->cvp = vc->cvp->next;
		vc->cvp->vd = *pnt;
		vc->cvp->next =(struct VDATA *) NIL;
	}
	vc->currentHead->nPoints++;
	return  0;
}
/***********************************************************************
 *	Get Data
 */
/* */	int
/* */	VDGetData( 
/* */	int	lsthdl, 
/* */	int	contN, 
/* */	int	pn, 
/* */	struct  vecdata *pnt)
/*
 *	returns : 0, -1 ( Illeagal Coontour Number)
 ***********************************************************************/
{
struct VHEAD	*vhd;
struct VDATA	*cvd;


	if ( lsthdl <0 ||lsthdl >= LISTDATAMAX) 
		return -1;
	if ((vhd = VCntlTbl[lsthdl].rootHead)==NIL)
		return -1;
	while ( contN-->0)
		vhd = vhd->next;
	cvd = vhd->headp;
	while ( pn-->0)
		cvd = cvd->next;

	*pnt = cvd->vd;

	return	0;
}
/***********************************************************************
 *	Get Data Head
 */
/* */	int
/* */	VDGetHead( 
/* */	int	lsthdl, 
/* */	struct VHEAD	**vhd)
/*
 *	returns : 0, -1
 ***********************************************************************/
{
	if ( lsthdl >= 0 && lsthdl < LISTDATAMAX) {
		*vhd = VCntlTbl[lsthdl].rootHead;
		return( 0);
	}
	else
		return( -1);
}
/***********************************************************************
 *	Get Memory
 */
/* */	static void *
/* */	getmem(	int lsthdl, int	siz)
/*
 *	returns : 0, -1
 ***********************************************************************/
{
	void	*mem;
struct VCNTL	*vc;

	vc = VCntlTbl+lsthdl;

	if ( vc->mendp + siz >= ALLOCMEMUNIT) {
		mem = *((void **)vc->cmem);
		if ( mem == NIL ) {
			mem = (void *)malloc(ALLOCMEMUNIT);
			if ( mem == NIL)
				return( NIL);
			*((void * *)mem) = NIL;
			*((void * *)vc->cmem) = mem; /* */
			vc->cmem = mem;
		}
		else
			vc->cmem  = mem;
		vc->mendp = sizeof(void *); 
	}
	mem = (void *)((char *)(vc->cmem) + vc->mendp);
	vc->mendp += siz;
	return(mem );
}
/***********************************************************************
 *	Get Number of COntours
 */
/* */	int
/* */	VDGetNCont( int lstHdl)
/*
 *	returns : Number of Contour
 ***********************************************************************/
{
	if ( lstHdl >= 0 && lstHdl < LISTDATAMAX)
		return VCntlTbl[lstHdl].nCont;
	else
		return( -1);
}
/***********************************************************************
 *	Reverse List
 */
/* */	int
/* */	VDReverseList(  int lstHdl)
/*
 *	returns : 0, -1 ( handle No )
 ***********************************************************************/
{
	int	cont;
	int	np;
struct VHEAD	*vh;
struct VDATA	*vp, *nvp;

	if ( lstHdl < 0 || lstHdl >= LISTDATAMAX)
		return -1;
	vh = VCntlTbl[lstHdl].rootHead;
	for ( cont = 0; cont < VCntlTbl[lstHdl].nCont; cont++ ) {
		vp = vh ->headp;
		np = vh->nPoints;
		while ( np-->0) {
			nvp = vp->next;
			vp->next = vp->prev;
			vp->prev = nvp;
			vp = nvp;
		}
		vh = vh->next;
	}
	return 0;
}
/***********************************************************************
 *	Copy Data
 */
/* */	int
/* */	VDCopy( int srcH, int dstH)
/*
 *	returns : 0, -1(Invalid Handle)
 ***********************************************************************/
{
	int	cont;
	int	np;
struct VHEAD	*vh;
struct VDATA	*vp;

	if ( srcH < 0 || srcH >= LISTDATAMAX
	  || dstH < 0 || dstH >= LISTDATAMAX)
		return -1;

	VDNew( dstH);

	vh = VCntlTbl[srcH].rootHead;
	for ( cont = 0; cont < VCntlTbl[srcH].nCont; cont++ ) {
		vp = vh ->headp;
		np = vh->nPoints;
		while ( np-->0) {
			if ( VDSetData( dstH, &vp->vd))
				return -1;
			vp = vp->next;
		}
		vh = vh->next;
		VDClose( dstH);
	}
	return 0;
}
/* EOF */
