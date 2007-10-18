#ifndef CARDLIB_INCLUDED
#define CARDLIB_INCLUDED

#define CARDLIBPROC __stdcall

void CardBlt(HDC hdc, int x, int y, int nCardNum);
void CardLib_SetZoomSpeed(int);

#define CS_EI_NONE	0
#define CS_EI_SUNK	1
#define CS_EI_CIRC	67
#define CS_EI_X		66

#define CS_DEFXOFF			12		//x-offset
#define CS_DEFYOFF			18		//y-offset
#define CS_NO3D				1		//default 3d counts (recommened)
#define CS_DEF3D			10		//(best for decks)

#define CS_DRAG_NONE		0
#define CS_DRAG_TOP			1
#define CS_DRAG_ALL			2
#define CS_DRAG_CALLBACK	3

#define CS_DROP_NONE		0
#define CS_DROP_ALL			1
#define	CS_DROP_CALLBACK	2

#define CS_XJUST_NONE		0
#define CS_XJUST_RIGHT		1
#define CS_XJUST_CENTER		2

#define CS_YJUST_NONE		0
#define CS_YJUST_BOTTOM		1
#define CS_YJUST_CENTER		2

#define CB_STATIC			0		//static text label
#define CB_PUSHBUTTON		1		//normal button
#define CB_ALIGN_CENTER		0		//centered is default
#define CB_ALIGN_LEFT		2
#define CB_ALIGN_RIGHT		4

#define CS_FACE_UP		 0	//all cards face-up
#define CS_FACE_DOWN	 1	//all cards face-down
#define CS_FACE_DOWNUP	 2	//bottom X cards down, top-most face-up
#define CS_FACE_UPDOWN	 3	//bottom X cards up, top-most face-down
#define CS_FACE_ANY		 4	//cards can be any orientation

#define CS_DROPZONE_NODROP	-1

//
//	Define the standard card-back indices
//
#define ecbCROSSHATCH	53
#define ecbWEAVE1		54
#define ecbWEAVE2		55
#define ecbROBOT		56
#define ecbFLOWERS		57
#define ecbVINE1		58
#define ecbVINE2		59
#define ecbFISH1		60
#define ecbFISH2		61
#define ecbSHELLS		62
#define ecbCASTLE		63
#define ecbISLAND		64
#define ecbCARDHAND		65
#define ecbUNUSED		66
#define ecbTHE_X		67
#define ecbTHE_O		68


class CardRegion;
class CardButton;
class CardStack;
class CardWindow;

typedef bool (CARDLIBPROC *pCanDragProc)    (CardRegion &stackobj, int iNumDragging);
typedef bool (CARDLIBPROC *pCanDropProc)    (CardRegion &stackobj, const CardStack &cards);
typedef void (CARDLIBPROC *pClickProc)      (CardRegion &stackobj, int iNumCards);
typedef void (CARDLIBPROC *pAddProc)        (CardRegion &stackobj, const CardStack &cards);
typedef void (CARDLIBPROC *pRemoveProc)     (CardRegion &stackobj, int iNumRemoved);

typedef void (CARDLIBPROC *pResizeWndProc)  (int width, int height);
typedef int  (CARDLIBPROC *pDropZoneProc)   (int dzid, const CardStack &cards);

typedef void (CARDLIBPROC *pButtonProc)		(CardButton &pButton);


#include "card.h"
#include "cardbutton.h"
#include "cardstack.h"
#include "cardregion.h"
#include "cardcount.h"
#include "cardwindow.h"

#ifdef _DEBUG
typedef bool (CARDLIBPROC *pDebugClickProc) (CardRegion &stackobj);
void CardLib_SetStackClickProc(pDebugClickProc proc);
#endif


#endif
