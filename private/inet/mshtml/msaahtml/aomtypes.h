//================================================================================
//		File:	AOMTYPES.H
//		Date: 	5/30/97
//		Desc:	contains definition of CAccElement class.
//				CAccElement is the abstract base class for all 
//				accessible 	elements.
//
//		Author: Arunj
//
//================================================================================

#ifndef __AOMTYPES__
#define __AOMTYPES__



//------------------------------------------------
//	Defines for the various Accessible Object
//	Model items supported.
//------------------------------------------------

#define	AOMITEM_NOTSUPPORTED			0x00000000

#define	AOMITEM_ITEM_MASK				0x0000FFFF
#define	AOMITEM_ANCHOR					0x00000001
#define	AOMITEM_AREA					0x00000002
#define	AOMITEM_BUTTON					0x00004003
#define	AOMITEM_CHECKBOX				0x00004004
#define	AOMITEM_EDITFIELD				0x00004005
#define	AOMITEM_FRAME					0x00000006
#define	AOMITEM_IMAGE					0x00000007
#define	AOMITEM_MAP						0x00000009
#define AOMITEM_MARQUEE					0x0000000A
#define	AOMITEM_PLUGIN					0x0000000B
#define	AOMITEM_RADIOBUTTON				0x0000400C
#define	AOMITEM_SELECTLIST				0x0000400D
#define	AOMITEM_SOUND					0x0000000E
#define	AOMITEM_TABLE					0x0000000F
#define	AOMITEM_TABLECELL				0x00000010
#define	AOMITEM_TEXT					0x00008011
#define	AOMITEM_IMAGEBUTTON				0x00004012
#define AOMITEM_DOCUMENT				0x00000013
#define AOMITEM_WINDOW					0x00000014
#define AOMITEM_DIV						0x00000015


#define	AOMITEM_CONTROL_MASK			0x00004000
#define	AOMITEM_AE_MASK					0x00008000

#define	AOMITEM_IGNORETEXTRANGEOF		0x00000FFE

#define	AOMITEM_BUTTON_MASK				0x00F00000
#define	AOMITEM_INPUTBUTTON				0x00100000
#define	AOMITEM_BUTTONBUTTON			0x00200000

#define AOMITEM_MAYHAVECHILDREN_MASK	0x80000000
#define AOMITEM_MAYHAVECHILDREN			0x80000000
	
#define DUMMY_SOURCEINDEX				65536


//================================================================================
//	Macros
//================================================================================

#define	IsAOMTypeAE(x)					( ((CAccElement*) x)->GetAOMType() & AOMITEM_AE_MASK )
#define	IsAOMTypeAO(x)					( !(IsAOMTypeAE(x)) )
#define	IsAOMTypeControl(x)				( ((CAccElement*) x)->GetAOMType() & AOMITEM_CONTROL_MASK )



#endif  // __AOMTYPES__