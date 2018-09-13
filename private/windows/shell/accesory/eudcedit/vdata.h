//
// Copyright (c) 1997-1999 Microsoft Corporation.
//

#define		LISTDATAMAX	4
#define		NIL		((void *)0)
struct vecdata	{
	short	x, y, atr, dummy;
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
int  VDGetHead(int  lsthdl,struct  VHEAD * *vhd);
int  VDGetNCont(int  lsthdl);
int  VDReverseList(int  lsthdl);
int  VDCopy(int  srcH, int dstH);

/* EOF */
