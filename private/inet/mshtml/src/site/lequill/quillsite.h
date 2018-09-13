
#ifndef _QuillSite_H
#define _QuillSite_H

#pragma once

extern "C"{

#ifndef __ITextLayoutManager_FWD_DEFINED__
#define __ITextLayoutManager_FWD_DEFINED__
typedef interface ITextLayoutManager ITextLayoutManager ; 
#endif

#ifndef __ITextLayoutAccess_FWD_DEFINED__
#define __ITextLayoutAccess_FWD_DEFINED__
typedef interface ITextLayoutAccess ITextLayoutAccess ; 
#endif

#ifndef __QuillLayoutManager_FWD_DEFINED__
#define __QuillLayoutManager_FWD_DEFINED__
#ifdef __cplusplus
typedef class QuillLayoutManager QuillLayoutManager ; 
#else
typedef struct QuillLayoutManager QuillLayoutManager ; 
#endif
#endif

#ifndef __ITextLayoutGroup_FWD_DEFINED__
#define __ITextLayoutGroup_FWD_DEFINED__
typedef interface ITextLayoutGroup ITextLayoutGroup ; 
#endif

#ifndef __ITextLayoutElement_FWD_DEFINED__
#define __ITextLayoutElement_FWD_DEFINED__
typedef interface ITextLayoutElement ITextLayoutElement ; 
#endif

#ifndef __ITextLayoutGroupSite_FWD_DEFINED__
#define __ITextLayoutGroupSite_FWD_DEFINED__
typedef interface ITextLayoutGroupSite ITextLayoutGroupSite ; 
#endif

#ifndef __ITextStory_FWD_DEFINED__
#define __ITextStory_FWD_DEFINED__
typedef interface ITextStory ITextStory ; 
#endif

#ifndef __ITextLayoutSite_FWD_DEFINED__
#define __ITextLayoutSite_FWD_DEFINED__
typedef interface ITextLayoutSite ITextLayoutSite ; 
#endif

#ifndef __ITextInPlacePaint_FWD_DEFINED__
#define __ITextInPlacePaint_FWD_DEFINED__
typedef interface ITextInPlacePaint ITextInPlacePaint ; 
#endif

#ifndef __ITextConstraintPoints_FWD_DEFINED__
#define __ITextConstraintPoints_FWD_DEFINED__
typedef interface ITextConstraintPoints ITextConstraintPoints ; 
#endif

#ifndef __ITextChangeNotification_FWD_DEFINED__
#define __ITextChangeNotification_FWD_DEFINED__
typedef interface ITextChangeNotification ITextChangeNotification ; 
#endif

#ifndef __ILineServicesHost_FWD_DEFINED__
#define __ILineServicesHost_FWD_DEFINED__
typedef interface ILineServicesHost ILineServicesHost ; 
#endif

#include "unknwn.h"
#include "objidl.h"
#include "oaidl.h"
#include "oleidl.h"


#ifndef _ITextPropertyList_H
#define _ITextPropertyList_H

#pragma once

extern "C"{

#ifndef __ITextPropertyList_FWD_DEFINED__
#define __ITextPropertyList_FWD_DEFINED__
typedef interface ITextPropertyList ITextPropertyList ; 
#endif

#include "unknwn.h"
#include "objidl.h"
#include "oaidl.h"
#include "oleidl.h"

/*----------------------------------------------------------------------------
	TextPropertyListDef.h

	Owner: AlexMog
	Copyright (c) 1998-2000 Microsoft Corporation 

	Generic property interface
	
----------------------------------------------------------------------------*/

//////////////////////////////////////////////////////////////////////////
//
//	Definitions for ITextPropertyList
//
//  this file is to be included in .idl before 

#ifndef _TextPropertyListDef_h_
#define _TextPropertyListDef_h_

	// REVIEW alexmog: find a nice way to remove unreadable prefixes
	// Predefined tags
	enum
	{
		// All negative tags are reserved
		tptagNil = -1,
		tptagReserved = 0
	};
	
	// Text Property Types
	enum
	{
		tptReserved = 0, // ignore this property
		tptUnknown = 1,   // Not any known type (handled as array of bytes or by custom hander)
		tptByte	   = 2,	  // Byte
		tptU1	   = 3,   // unsigned byte
		tptI2      = 4,   // Short int
		tptU2      = 5,   // Word
		tptI4      = 6,   // Long int
		tptU4      = 7,   // Double word
		tptFloat   = 8,   // 32-bit float
		tptMaxRgb,        // Types below this can be treated as array of bytes, and their m_cb is valid
		
		tptBit     = 21,  // bit or bit field (m_b is a byte offsed, m_bitMask is a bit mask)
		
		tptCustom  = 1000,
	};

	// Text Property Units
	enum
	{
		tpuUnknown = -1,	// unit unknown (still may be converable to a known one)
        tpuNative = 0,		// no conversion applicable (e.g. enum)
        
        tpuBool = 1,        // boolean - represented in property list as all 0s or all 1s
        
		tpuPoint = 11,
		tpuPica,
		tpuInch,
		tpuCm,
		tpuMm,
		tpuEm,
		tpuEn,
		tpuEx,
		tpuDisplayPixels,	// display pixels (note that when printing printer is the display device)
		tpuTargetPixels,	// target device pixels
		tpuPercent,
		tpuRelative,
		tpuTimesRelative,
		tpuMax,
	};

	// Text Property Flags
	enum
	{
		tpfCustomFetch = 0x01,
		tpfCustomApply = 0x02,
		tpfCustomCompare = 0x04,
	};

	struct TextPropertyDescription
	{
	public:
		short	m_tag;	// for validation. must be equal to position in the table
		
	public:
		short	m_nType;				// property type (tpt)
		short	m_nUnit;				// native unit (use tpuUnknown with custom handlers) (tpu)
		short	m_cb;					// size of value
		short	m_b;					// offset within the structure
		byte	m_mask;					// bit mask for bit fields

		byte	m_iFlags;				
		
	public:
		inline int get_Tag() { return m_tag; }
		inline int get_Type() { return m_nType; }
		inline int get_Unit() { return m_nUnit; }
		inline int get_Cb()   { return m_cb; }
		inline int get_Offset() { return m_b; }
		inline int get_Mask() { return m_mask; }
		inline int get_Flags() { return m_iFlags; }

#if DBG || defined(DEBUG)
		inline int validateTag(int tag)
		{
			return (tag == m_tag);
		}
#endif // DBG
	};

    // define offset() macro
    #ifndef offset
    #define offset(type, field) ((int) (&((type *) 0)->field))
    #endif // offset

	// an interface to access and convert properties
	//
	// REVIEW alexmog: this is a struct so that it doesn't have to inherit form IUnknown
	struct ITextPropertyAccess
	{
	public:
		// get structure's base address, as refered in the dictionary
		virtual STDMETHODIMP_(void *) get_This() = 0;

		// REVIEW alexmog: this should return the dictionary as well
		
		virtual STDMETHODIMP FetchProperty(int tag, int nUnit, const void *pvPropData, void *pvDst, int cbDst) = 0;
		virtual STDMETHODIMP ApplyProperty(int tag, int nUnit, const void *pvPropData, void *pvDst, int cbDst) = 0;
		
		// REVIEW alexmog: custom compare functionality not defined yet
		virtual STDMETHODIMP CompareProperty(int tag, int nUnit, const void *pvSrc, const void *pvDst) = 0;

		// REVIEW alexmog: would be cool to have a base implementation, but where to put the code?
	};

	struct TextPropertyDictionary
	{
	public:
		int m_cProps;
		int m_firstTag;
		struct TextPropertyDescription *m_ptpd;

	public:
		// note: no put methods are available. these tables should be static const
		inline int get_CProps() { return m_cProps; }
		inline int get_FirstTag() { return m_firstTag; }
		inline struct TextPropertyDescription *get_Table() { return m_ptpd; }
		inline struct TextPropertyDescription *get_Description(int tag) 
		{ 
			Assert(tag - m_firstTag >= 0 && tag - m_firstTag < m_cProps);
            TextPropertyDescription *ptpd = &get_Table()[tag - m_firstTag];
			Assert(ptpd->validateTag(tag));
			return ptpd; 
		}
	};

	// standard positions for seek
	enum 
	{
		tplPositionStart = 0,
		tplPositionEnd = -1,
	};

#endif // _TextPropertyListDef_h_







EXTERN_C const IID IID_ITextPropertyList;

  interface  ITextPropertyList : public IUnknown
    {
    public:
        virtual HRESULT  STDMETHODCALLTYPE Init(
                void ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetFullList(
                struct TextPropertyDictionary __RPC_FAR *pDict , struct ITextPropertyAccess __RPC_FAR *pipaSrc ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetDifference(
                struct TextPropertyDictionary __RPC_FAR *pDict , struct ITextPropertyAccess __RPC_FAR *pipaSrc , struct ITextPropertyAccess __RPC_FAR *pipaBase ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE Apply(
                struct TextPropertyDictionary __RPC_FAR *pDictDst , struct ITextPropertyAccess __RPC_FAR *pipaDst ) = 0;
    };

#ifdef DEBUG

#define CheckITextPropertyListMembers(klass)\
	struct _##klass##_ITextPropertyList_Init\
		{\
		_##klass##_ITextPropertyList_Init(HRESULT  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextPropertyList_GetFullList\
		{\
		_##klass##_ITextPropertyList_GetFullList(HRESULT  (##klass##::*pfn)(struct TextPropertyDictionary __RPC_FAR *, struct ITextPropertyAccess __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextPropertyList_GetDifference\
		{\
		_##klass##_ITextPropertyList_GetDifference(HRESULT  (##klass##::*pfn)(struct TextPropertyDictionary __RPC_FAR *, struct ITextPropertyAccess __RPC_FAR *, struct ITextPropertyAccess __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextPropertyList_Apply\
		{\
		_##klass##_ITextPropertyList_Apply(HRESULT  (##klass##::*pfn)(struct TextPropertyDictionary __RPC_FAR *, struct ITextPropertyAccess __RPC_FAR *)){}\
		};\
	void klass::VerifyITextPropertyList(){\
	_##klass##_ITextPropertyList_Init pfn1(Init);\
	_##klass##_ITextPropertyList_GetFullList pfn2(GetFullList);\
	_##klass##_ITextPropertyList_GetDifference pfn3(GetDifference);\
	_##klass##_ITextPropertyList_Apply pfn4(Apply);\
	}\

#else

#define CheckITextPropertyListMembers(klass)

#endif /* DEBUG */

#define idITextPropertyList	0xBAA94BBC

#ifdef DeclareSmartPointer
#ifndef ITextPropertyListSPMethodsDefined
extern "C++" { TEMPLATECLASSB class ITextPropertyListSPMethods : public TEMPLATEBASEB(IUnknownSPMethods) { }; }
#define ITextPropertyListSPMethodsDefined
#endif
DeclareSmartPointer(ITextPropertyList)
#define SPITextPropertyList auto SP_ITextPropertyList
#endif

#define DeclareITextPropertyListMethods()\
	(FNOBJECT) Apply,\
	(FNOBJECT) GetDifference,\
	(FNOBJECT) GetFullList,\
	(FNOBJECT) Init,\

#define DeclareITextPropertyListVtbl()\
	,_Method1\
	,_Method2\
	,_Method3\
	,_Method4\

}



#endif

typedef enum __MIDL_IQuillSiteTypes_0001 { 
qsverRup = 2403 } QSVersion ; 
typedef enum __MIDL_IQuillSiteTypes_0002 { 
qresString = 0 , 
qresTypeLast = qresString + 1 } QResType ; 
typedef enum __MIDL_IQuillSiteTypes_0003 { 
qstrPageBreak = 0 , 
qstrColumnBreak = qstrPageBreak + 1 , 
qresLast = qstrColumnBreak + 1 } QResource ; 
typedef enum __MIDL_IQuillSiteTypes_0004 { 
qbkmodeSolid = 0 , 
qbkmodeTransparent = - 1 } BkgndMode ; 
typedef enum __MIDL_IQuillSiteTypes_0005 { 
qvaTop = 0 , 
qvaMiddle = qvaTop + 1 , 
qvaBottom = qvaMiddle + 1 , 
qvaAbsMiddle = qvaBottom + 1 , 
qvaAbsBottom = qvaAbsMiddle + 1 } VerticalAlign ; 
typedef enum __MIDL_IQuillSiteTypes_0006 { 
qpaintEdit = 0 , 
qpaintEditRequestOffScreen = qpaintEdit + 1 , 
qpaintEditRequestScreenAccess = qpaintEditRequestOffScreen + 1 } EditKind ; 
typedef enum __MIDL_IQuillSiteTypes_0007 { 
qclickSingle = 0 , 
qclickDouble = qclickSingle + 1 , 
qclickTriple = qclickDouble + 1 , 
qclickQuad = qclickTriple + 1 , 
qclickExtend = qclickQuad + 1 } QMouseClick ; 
#define LAYOUTOPTIONS_EXTERNAL 1
#define OBJFLAGS_OWNLINE			0x01
#define OBJFLAGS_HIDDEN			0x02
#define OBJFLAGS_ABSAUTOTOP		0x04
#define OBJFLAGS_ABSAUTOLEFT		0x08
#define OBJFLAGS_PERCENTWIDTH	0x10
#define OBJFLAGS_PERCENTHEIGHT	0x20
#define FETCHFLAGS_OBJECT		0x01
#define FETCHFLAGS_OWNLINE		0x02
#define LINE_FLAG_NONE				0x0000
#define LINE_FLAG_HAS_ABSOLUTE_ELT	0x0001
#define LINE_FLAG_HAS_ALIGNED		0x0002
#define LINE_FLAG_HAS_EMBED_OR_WBR	0x0004
#define LINE_FLAG_HAS_NESTED_RO		0x0008
#define LINE_FLAG_HAS_BREAK_CHAR		0x0010
#define LINE_FLAG_HAS_BACKGROUND		0x0020
#define LINE_FLAG_HAS_A_BR			0x0040
#define LINE_FLAG_HAS_RELATIVE		0x0080
#define LINE_FLAG_HAS_NBSP			0x0100
#define LINE_FLAG_HAS_EATENSPACE		0x0200
#define LINE_FLAG_HAS_NOBLAST		0x0200
#define LINE_FLAG_HAS_CLEARLEFT		0x0400
#define LINE_FLAG_HAS_CLEARRIGHT		0x0800
typedef enum __MIDL_IQuillSiteTypes_0008 { 
propertyGroupChar = 0 , 
propertyGroupPara = propertyGroupChar + 1 } PropertyGroup ; 
typedef enum __MIDL_IQuillSiteTypes_0009 { 
qobjNone = 0 , 
qobjRange = qobjNone + 1 , 
qobjRangePrecise = qobjRange + 1 , 
qobjField = qobjRangePrecise + 1 , 
qobjInlineObject = qobjField + 1 , 
qobjFrame = qobjInlineObject + 1 } QObjects ; 
typedef enum __MIDL_IQuillSiteTypes_0010 { 
QNFLAGS_SELF = 0x1 , 
QNFLAGS_ANCESTORS = 0x2 , 
QNFLAGS_DESCENDENTS = 0x4 , 
QNFLAGS_TREELEVEL = 0x8 , 
QNFLAGS_TARGETMASK = 0xf , 
QNFLAGS_TEXTCHANGE = 0x10 , 
QNFLAGS_TREECHANGE = 0x20 , 
QNFLAGS_LAYOUTCHANGE = 0x40 , 
QNFLAGS_FOR_ACTIVEX = 0x80 , 
QNFLAGS_FOR_LAYOUTS = 0x100 , 
QNFLAGS_FOR_POSITIONED = 0x200 , 
QNFLAGS_FOR_WINDOWED = 0x400 , 
QNFLAGS_FOR_ALLELEMENTS = 0x800 , 
QNFLAGS_CATEGORYMASK = 0xff0 , 
QNFLAGS_SENDUNTILHANDLED = 0x10000 , 
QNFLAGS_LAZYRANGE = 0x20000 , 
QNFLAGS_CLEANCHANGE = 0x40000 , 
QNFLAGS_CALLSUPERLAST = 0x80000 , 
QNFLAGS_DATAISVALID = 0x100000 , 
QNFLAGS_SYNCHRONOUSONLY = 0x200000 , 
QNFLAGS_DONOTBLOCK = 0x400000 , 
QNFLAGS_CLEARFORMATSELF = 0x800000 , 
QNFLAGS_CLEARFORMAT = 0x1000000 , 
QNFLAGS_AUTOONLY = 0x2000000 , 
QNFLAGS_PROPERTYMASK = 0xfff0000 , 
QNFLAGS_FORCE = 0x10000000 , 
QNFLAGS_SENDENDED = 0x20000000 , 
QNFLAGS_DONOTLAYOUT = 0x40000000 , 
QNFLAGS_CONTROLMASK = 0xf0000000 } QNOTIFYFLAGS ; 
#define IDL_JPars_BUG 1


/////////////////////////////////////////////////////////////////////
// Public Quill constants
//

// Maximum value for floating-point point dimensions
#define ptsLarge	84546L	// max number of points that can be converted to long Emus

// Field options
#define	qfEditable			0x40
#define qfUseFieldFont		0x80
#define qfHot				0x100
#define	qfBleedFormatting	0x200

// Find field options
#define	qfindAll	0x00
#define	qfindType	0x01
#define	qfindData	0x02
#define	qfindFlags	0x04

// Field types
#define	qfldWordField		-1
#define	qfldHyperlinkField	-2
#define qfldHIVField		-3
#define qfldAnnotationField -4
#define qfldClientTokenField	-5

// Intra field delimiters
#define qfcUnused                0xFFFD	//0xFFFF	
#define qfcStartOfInline		 0xFEFF
#define qfcStartAnnotation		 0xFEFF
#define qfcEndOfInline           0xFFFF	
#define qfcEndRubyAnnotation     0xFFFE	
#define qfcEndRubyMain           0xFFFF	

// old-files support
#define qfcEndOfInlineAlternate			0x0080
#define qfcEndRubyAnnotationAlternate	0x0081
#define qfcEndRubyMainAlternate			0x0090

// ruby alignments
#define qraDefault		'0'
#define qra010			'1'
#define qra121			'2'
#define qraCenter		'3'
#define qraLeft			'4'
#define qraRight		'5'

// Undo group flags
#define	qgrpFreezeNone		0x0
#define	qgrpFreezeAllViews	0x1

// Constraint flags
#define	qconsAvoidLeft		0x1
#define	qconsAvoidRight		0x2
#define	qconsDropCap		0x4
#define qconsWrapWithin		0x8

// Client coordinate conversions
#define	qcoordsPosition			0x1
#define	qcoordsSize				0x2
#define	qcoordsPixelsToPoints	0x4
#define	qcoordsPointsToPixels	0x8

// Mouse options
#define	qmoAllowDragDrop			0x01
#define	qmoAllowSelection			0x02
#define qmoShowInsCursor			0x04
#define	qmoShowFieldCursor			0x08
#define qmoAllowMarginSelection		0x10
#define qmoAllowAltMarginSelection	0x20
#define	qmoAutoWordSelect			0x40
#define qmoDoubleClickOleActivate   0x80
#define qmoAllowCropImage			0x100
#define qmoNoRestrictToPrintable	0x200

// Text display options
#define qtdoShowTextAsPrinted		0x01
#define qtdoShowAnimations			0x02
#define qtdoShowSpaces				0x04
#define qtdoShowParaMarks			0x08
#define qtdoShowTabs				0x10
#define qtdoShowCondHyphens			0x20
#define qtdoShowSplats				0x40
#define qtdoShowSpelling			0x80
#define qtdoShowBreaks				0x1000
#define qtdoShowAll					(qtdoShowSpaces|qtdoShowParaMarks|qtdoShowTabs|qtdoShowCondHyphens|qtdoShowSplats|qtdoShowBreaks)

#define qtdoSlideUp					0x100
#define qtdoAlignLines				0x200
#define qtdoShowTextBoxesAsEndnotes	0x400	// temp - NIA_DEMO // REVIEW alexmog
#define qtdoShowEndOfCellChar		0x800
#define qtdoConvertBackslashIntoYen	0x2000

// Event notification options
// ************************* WARNING ****************************
// * qevt values and qeno values must be in one-to-one			*
// * correspondance! See CQuillView::FEventEnabled(long qevt).	*
// **************************************************************
#define qenoCellFull				0x0001
#define qenoCellEmpty				0x0002
#define qenoCellOverflow			0x0004
#define qenoCellTextFits			0x0008
#define qenoTypingWS				0x0010
#define qenoTypingPunc				0x0020
#define qenoTypingAlphaUC			0x0040
#define qenoTypingAlphaLC			0x0080
#define qenoTypingAlphaNC			0x0100
#define qenoTypingNumber			0x0200
#define qenoFrameResize				0x0400
#define qenoTip						0x0800
#define qenoFontPropChange			0x1000
#define qenoParaPropChange			0x2000
#define qenoStoryEdit				0x4000
#define qenoImeStart				0x8000
#define	qenoImeEnd					0x10000
#define qenoLast					qenoImeEnd

#define qenoTypingAlpha		(qenoTypingAlphaUC|qenoTypingAlphaLC|qenoTypingAlphaNC)
#define qenoTypingAlphaNum	(qenoTypingAlpha|qenoTypingNumber)
#define qenoTypingBounds    (qenoTypingWS|qenoTypingPunc)
#define qenoTypingAllEvts	(qenoTypingWS|qenoTypingPunc|qenoTypingAlphaNum)
#define qenoCellAllEvts		(qenoCellFull|qenoCellEmpty|qenoCellOverflow|qenoCellTextFits)
#define qenoImeAllEvts		(qenoImeStart|qenoImeEnd)

// Event codes (passed back to client)
// ************************* WARNING ****************************
// * qevt values and qeno values must be in one-to-one			*
// * correspondance! See CQuillView::FEventEnabled(long qevt).	*
// **************************************************************
#define qevtNull					0
#define qevtCellFull				-1
#define qevtCellEmpty				-2
#define qevtCellOverflow			-3
#define qevtCellTextFits			-4
#define qevtTypingWS				-5
#define qevtTypingPunc				-6
#define qevtTypingAlphaUC			-7
#define qevtTypingAlphaLC			-8
#define qevtTypingAlphaNC			-9
#define qevtTypingNumber			-10
#define qevtFrameResize				-11
#define qevtTip						-12
#define qevtFontPropChange			-13
#define qevtParaPropChange			-14
#define qevtStoryEdit				-15
#define qevtImeStart				-16
#define qevtImeEnd					-17
#define qevtLast					(qevtImeEnd)
// Debug codes (passed back to client)
#if defined(DEBUG)
#define qevtDebugOffset					(0x10000000)
#define qevtDebugFormatTextCell			(-(qevtDebugOffset + 0))
#define qevtDebugLayoutEmergency		(-(qevtDebugOffset + 1))
#endif // DEBUG

// Cell Layout States
#define qclsFull		qevtCellFull
#define qclsEmpty		qevtCellEmpty
#define qclsOverflow	qevtCellOverflow
#define qclsTextFits	qevtCellTextFits

// CSC options 
#define	qcscNone				0x000 // CSC (Autospacing) disabled 
#define	qcscSpaceFEAlpha		0x001 // Autospace between FE and alpha text
#define	qcscSpaceFEDigit		0x002 // Autospace between FE and narrow digits
#define	qcscPunctStartLine		0x004 // Remove extra space from leading char if applicable
#define	qcscHangingPunct		0x008 // ** no longer supported, use SetHangingPunct API **
#define	qcscKernByAlgorithm		0x010 // Modwith adjustments

// Distribution options
// These extend the tomAlignXXX range of values
#define qaDistributeLetter		4
#define qaJustifyFE				5
#define qaNewspaper				6
#define qaNewspaperFE			7

// Text Justification options
#define qtjoExpandOnly				0
#define qtjoCompressPunctuation		1
#define qtjoCompressPunctAndKana	2

// List separator extensions to the tom constants
#define qlsParenthesis  (0<<16)
#define qlsDoubleParen  (1<<16)
#define qlsPeriod       (2<<16)
#define qlsPlain        (3<<16)
#define qlsSquare       (4<<16)
#define qlsColon        (5<<16)
#define qlsDoubleSquare (6<<16)
#define qlsDoubleHyphen (7<<16)

// FE Line break levels
#define qlbcOff 			0
#define qlbcLooseFE			1
#define qlbcLooseAll		2  // old files support only
#define qlbcNormalFE		3
#define qlbcNormalAll		4  // old files support only
#define qlbcStrictFE		5
#define qlbcStrictAll		6  // old files support only

#define qlbcLoose			qlbcLooseFE
#define qlbcNormal			qlbcNormalFE
#define qlbcStrict			qlbcStrictFE


// Text Flow options
#define qtfHorizontal				0x01
#define qtfVerticalFE				0x02 
#define qtfBottomToTop				0x04 // Not supported, will act like qtfHorizontal
#define qtfTopToBottom				0x08 // Not supported, will act like qtfHorizontal
#define qtfHorizontalFE				0x10
#define qtfVertical					0x20 // Not supported, will act like qtfHorizontal
#define qtfVerticalFEULeft			0x40 // puts underline on left, otherwise same as qtfVerticalFE
#define qtfHorizontalFEULeft		0x80 // puts underline underneath, otherwise same as qtfHorizontalFE

// Typing options
#define qtpoAllowTyping				0x01
#define qtpoAutoSmartQuotes			0x02
#define qtpoAllowBeforeFootnotes	0x04
#define qtpoAutoWordFormat			0x08

// Draw flags
#define	qdrawNoText					0x01
#define	qdrawNoGraphics				0x02
#define qdrawNoClip					0x04
#define	qdrawNoClipText				0x08
#define qdrawNoSelectionHighlight	0x10
#define qdrawUpsideDown				0x20
#define qdrawPrinterDithersText		0x40
#define qdrawEnhancedMetafile		0x80

// Picture flip flags 
#define qflipNil					0x00
#define qflipHorz					0x01
#define qflipVert					0x02

// Smart quote characters
#define chDOpenQuoteDflt			0x201c
#define chDCloseQuoteDflt			0x201d
#define chSOpenQuoteDflt			0x2018
#define chSCloseQuoteDflt			0x2019

// Common non-document characters
#define qchEmbedding				0xFFFC	//Official Unicode character for embedding.
#define chPageBreak					0x0c	// Ascii FF (formfeed) code
#define chColumnBreak				0x0e
#define chNonBreakHyphen			0x1e	// non-breaking hyphen
#define chNonReqHyphen				0x1f	// optional hyphen

// Tab-leader characters
#define chDot						'.'
#define chDash						'-'
#define chLine						'_'
#define chDoubleLine				'='
#define chBullet					0x2022

// Quill style definitions
#define stiNormal					(-1)			// this is the TOM constant for "normal" style
#define qstyidNormal				0				// Quill's internal ID value for "normal"

// Quill COLORREF values (see the documentation overview: Color Model)
#define qcrClientId			0x80000000L		// Quill Type: client color ID flag
#define qcrDefault			0x40000000L		// Quill Type: Use the default color
#define qcrInvisible		0x20000000L		// Quill Type: Don't draw this -- it is invisible
#define qcrTypeMask			0xFC000000L		// AND this mask on a CR, then check for values above
#define qcrResolvedMask		0XFD000000L		// AND this mask on a CR, if we get 0, we can pass CR to Windows

#define qcrPaletteIndex		0x01000000L		// Windows Type: a Palette Index value
#define qcrPaletteRgb		0x02000000L		// Windows Type: match the RGB value to closest match in palette


// Standard client token ids
// NOTES:
// 1. UPDATE DOCUMENTATION in comment preceding CQSRange::InsertClientToken
//    if any standard types are added.
// 2. Once a qtokID is created here and checked in, CONSIDER IT HARDCODED FOR
//    ALL TIME. Changing a previously checked-in qctk value will cause
//    documents containing tokens with that previous qctk to display the
//    token differently, or not at all.

#define tokIDNull			0
#define qtokIDFirst			-1
#define qtokIDPageNumber	(qtokIDFirst - 0) // Automatic page number       (Default provided by Quill)
#define qtokIDDate			(qtokIDFirst - 1) // Date at token creation time (NO default provided by Quill)
#define qtokIDTime			(qtokIDFirst - 2) // Time at token creation time (NO default provided by Quill)
#define qtokIDPrintDate		(qtokIDFirst - 3) // Date updated at print time  (NO default provided by Quill)
#define qtokIDPrintTime		(qtokIDFirst - 4) // Time updated at print time  (NO default provided by Quill)
#define qtokIDPrevPageNumber	(qtokIDFirst - 5) // Automatic page number       (Default provided by Quill)
#define qtokIDNextPageNumber	(qtokIDFirst - 6) // Automatic page number       (Default provided by Quill)

// QuillStories custom error codes (range is 0x3000 - 0x3fff)
// (descriptive comments for each of these can be found in the help file)

#define	QUILL_E_GENERAL	MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x3000)
#define QUILL_E_NOVISIFONT				MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x3001)

// editing errors (range is 0x3100 - 0x31ff)
#define	QUILL_E_FORMAT					MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x3100)
#define	QUILL_E_NOUNDO					MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x3101)
#define	QUILL_E_SIZE					MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x3102)
#define	QUILL_E_CANTRESIZE				MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x3103)
#define QUILL_E_NOSTORY					MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x3104)
#define QUILL_E_CANTEDIT				MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x3105)
#define QUILL_E_INVALIDILC				MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x3106)

// file errors (range is 0x3200 - 0x32ff)
#define	QUILL_E_INVALIDFILEFORMAT	MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x3200)

// style success (range is 0x3300 - 0x33ff)
#define QUILL_S_STYLEEXISTS				MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x3300)
// style errors (range is 0x3300 - 0x33ff)
#define QUILL_E_STYLEEXISTS				MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x3300)
#define QUILL_E_DELETEDSTYLE			MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x3301)
#define QUILL_E_NOSTYLESHEET			MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x3302)
#define QUILL_E_STANDARDSTYLE			MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x3303)



// ILC constants
#define ilcOneOnly		(0)
#define ilcAll			(-1)

// document navigation
#define quillPage		0x100
#define quillAtom		0x101
#define quillLineLimit	0x102
#define quillUpDownLine 0x103
#define quillCharSet	0x104
#define quillWordProof	0x105			// word for proofing (only blank space is a separator)
#define quillWordProofNoSpaces	0x106	// word for proofing with no trailing or leading spaces or punctuation
#define quillCellOverflow 0x107
#define quillCellLimit	0x108			// cell limits for navigation keystrokes.

// QuillStories command codes for keyboard mapping.
// These must be #defined to be compatible with the resource compiler

#define qcmdNil			 -1
#define qcmdQSFirst		0xdb00
#define qcmdQSLim		0xdc00

#define qcmdCopy		0xdb00 	// copy selection to the clipboard
#define qcmdPaste		0xdb01 	// paste the clipboard over the selection
#define qcmdCut			0xdb02	// cut the selection to the clipboard
#define qcmdBackspace	0xdb03 	// backspace or clear command
#define qcmdDelete		0xdb04 	// forward delete of DEL key
#define qcmdDeleteBackWord	0xdb05	// delete previous word
#define qcmdDeleteWord	0xdb06	// delete next word

#define qcmdInsertPageBreak			0xdb07	// Insert a hard page break
#define qcmdInsertColumnBreak 		0xdb08	// Insert a hard column break
#define qcmdInsertOptionalHyphen 	0xdb09	// Insert an optional hyphen
#define qcmdInsertNonbreakingHyphen	0xdb0a	// Insert a non-breaking hyphen
#define qcmdInsertNonbreakingSpace	0xdb0b	// Insert a non-breaking space
#define qcmdInsertZeroWidthSpace	0xdb0c	// Insert a zero width space
#define qcmdInsertZeroWidthNoBreakSpace	0xdb0d	// Insert a zero width non breaking space


// Support for moving the cursor
#define qcmdKeyCtrl			0x0010 // used for modifying keystroke commands below - don't pass this in by itself
#define qcmdKeyShift		0x0020 // used for modifying keystroke commands below - don't pass this in by itself
#define qcmdKeyCtrlShift	(qcmdKeyCtrl+qcmdKeyShift)

#define qcmdSelectFirst		0xdb20 // FIRST SELECTION MOVE COMMAND
// Base direction keys
#define qcmdKeyBaseFirst	0xdb20	// FIRST POSSIBLE BASE KEY COMMAND
#define qcmdKeyLeft			0xdb20 // cursor left
#define qcmdKeyRight		0xdb21 // cursor right
#define qcmdKeyUp			0xdb22 // cursor up one line
#define qcmdKeyDown			0xdb23 // cursor down
#define qcmdKeyBaseLast		0xdb23	// LAST POSSIBLE BASE KEY COMMAND
// Base logical keys
#define qcmdMoveBeginLine	0xdb24 // move cursor to start of line
#define qcmdMoveEndLine		0xdb25 // move cursor to end of line
#define	qcmdMovePrevScreen	0xdb26 // move cursor to previous screen
#define	qcmdMoveNextScreen	0xdb27 // move cursor to next screen
#define qcmdMovePrevChar	0xdb28 // move cursor to previous character
#define qcmdMoveNextChar	0xdb29 // move cursor to next character
#define qcmdMovePrevLine	0xdb2a // move cursor to previous line
#define qcmdMoveNextLine	0xdb2b // move cursor to next line
#define qcmdMoveBeginCell	0xdb2c // move cursor to start of layout cell
#define qcmdMoveEndCell		0xdb2d // move cursor to end of layout cell

// Control direction keys
#define qcmdKeyCtrlLeft		(qcmdKeyLeft+qcmdKeyCtrl)	// move cursor left with control
#define qcmdKeyCtrlRight	(qcmdKeyRight+qcmdKeyCtrl)	// move cursor right with control
#define qcmdKeyCtrlUp		(qcmdKeyUp+qcmdKeyCtrl)		// move cursor up with control
#define qcmdKeyCtrlDown		(qcmdKeyDown+qcmdKeyCtrl)	// move cursor down with control
// Control logical keys
#define qcmdMoveFirstChar	(qcmdMoveBeginLine+qcmdKeyCtrl)	// move cursor to first character in the story
#define qcmdMoveLastChar	(qcmdMoveEndLine+qcmdKeyCtrl)	// move cursor to last character in the story
#define	qcmdMoveBeginScreen	(qcmdMovePrevScreen+qcmdKeyCtrl)// move cursor to start of screen
#define	qcmdMoveEndScreen	(qcmdMoveNextScreen+qcmdKeyCtrl)// move cursor to end of screen
#define qcmdMovePrevWord	(qcmdMovePrevChar+qcmdKeyCtrl)	// move cursor to previous word
#define qcmdMoveNextWord	(qcmdMoveNextChar+qcmdKeyCtrl)	// move cursor to next word
#define qcmdMovePrevPara	(qcmdMovePrevLine+qcmdKeyCtrl)	// move cursor to previous paragraph
#define qcmdMoveNextPara	(qcmdMoveNextLine+qcmdKeyCtrl)	// move cursor to next paragraph

// Support for extending the selection
#define qcmdExtendFirst		0xdb40 // FIRST SELECTION EXTEND COMMAND
// Shift direction keys
#define qcmdKeyShiftLeft	(qcmdKeyLeft+qcmdKeyShift)	// move cursor left with shift
#define qcmdKeyShiftRight	(qcmdKeyRight+qcmdKeyShift)	// move cursor right with shift
#define qcmdKeyShiftUp		(qcmdKeyUp+qcmdKeyShift)	// move cursor up with shift
#define qcmdKeyShiftDown	(qcmdKeyDown+qcmdKeyShift)	// move cursor down with shift
// Shift logical keys
#define qcmdExtendBeginLine	(qcmdMoveBeginLine+qcmdKeyShift)	// extend selection to start of line
#define qcmdExtendEndLine	(qcmdMoveEndLine+qcmdKeyShift)		// extend selection to end of line
#define	qcmdExtendPrevScreen	(qcmdMovePrevScreen+qcmdKeyShift)	// extend selection to previous screen
#define	qcmdExtendNextScreen	(qcmdMoveNextScreen+qcmdKeyShift)	// extend selection to next screen
#define qcmdExtendPrevChar	(qcmdMovePrevChar+qcmdKeyShift)		// extend selection to previous character
#define qcmdExtendNextChar	(qcmdMoveNextChar+qcmdKeyShift)		// extend selection to next character
#define qcmdExtendPrevLine	(qcmdMovePrevLine+qcmdKeyShift)		// extend selection to previous line
#define qcmdExtendNextLine	(qcmdMoveNextLine+qcmdKeyShift)		// extend selection to next line
#define qcmdExtendBeginCell	(qcmdMoveBeginCell+qcmdKeyShift)	// extend selection to start of layout cell
#define qcmdExtendEndCell	(qcmdMoveEndCell+qcmdKeyShift)		// extend selection to end of layout cell

// Control Shift direction keys
#define qcmdKeyCtrlShiftLeft	(qcmdKeyLeft+qcmdKeyCtrlShift)	// move cursor left with control and shift
#define qcmdKeyCtrlShiftRight	(qcmdKeyRight+qcmdKeyCtrlShift)	// move cursor right with control and shift
#define qcmdKeyCtrlShiftUp		(qcmdKeyUp+qcmdKeyCtrlShift)	// move cursor up with control and shift
#define qcmdKeyCtrlShiftDown	(qcmdKeyDown+qcmdKeyCtrlShift)	// move cursor down with control and shift
// Control Shift logical keys
#define qcmdExtendFirstChar	(qcmdMoveBeginLine+qcmdKeyCtrlShift)	// move cursor to first character in the story
#define qcmdExtendLastChar	(qcmdMoveEndLine+qcmdKeyCtrlShift)		// move cursor to last character in the story
#define	qcmdExtendBeginScreen	(qcmdMovePrevScreen+qcmdKeyCtrlShift)	// move cursor to start of screen
#define	qcmdExtendEndScreen	(qcmdMoveNextScreen+qcmdKeyCtrlShift)	// move cursor to end of screen
#define qcmdExtendPrevWord	(qcmdMovePrevChar+qcmdKeyCtrlShift)		// move cursor to previous word
#define qcmdExtendNextWord	(qcmdMoveNextChar+qcmdKeyCtrlShift)		// move cursor to next word
#define qcmdExtendPrevPara	(qcmdMovePrevLine+qcmdKeyCtrlShift)		// move cursor to previous paragraph
#define qcmdExtendNextPara	(qcmdMoveNextLine+qcmdKeyCtrlShift)		// move cursor to next paragraph

#define qcmdSelectStory		0xdb5f

// Legacy values - these will go away because they don't make sense with rotated text.  Please do not use them anymore.
#define qcmdMoveLeft			qcmdMovePrevChar
#define qcmdMoveRight			qcmdMoveNextChar
#define qcmdMoveUp				qcmdMovePrevLine
#define qcmdMoveDown			qcmdMoveNextLine
#define qcmdMoveLeftWord		qcmdMovePrevWord
#define qcmdMoveRightWord		qcmdMoveNextWord
#define qcmdMoveUpPara			qcmdMovePrevPara
#define qcmdMoveDownPara		qcmdMoveNextPara
#define qcmdMoveTop				qcmdMoveFirstChar
#define qcmdMoveBottom			qcmdMoveLastChar
#define qcmdMoveUpScreen		qcmdMovePrevScreen
#define qcmdMoveDownScreen		qcmdMoveNextScreen
#define qcmdExtendLeft			qcmdExtendPrevChar
#define qcmdExtendRight			qcmdExtendNextChar
#define qcmdExtendUp			qcmdExtendPrevLine
#define qcmdExtendDown			qcmdExtendNextLine
#define qcmdExtendLeftWord		qcmdExtendPrevWord
#define qcmdExtendRightWord		qcmdExtendNextWord
#define qcmdExtendUpPara		qcmdExtendPrevPara
#define qcmdExtendDownPara		qcmdExtendNextPara
#define qcmdExtendTop			qcmdExtendFirstChar
#define qcmdExtendBottom		qcmdExtendLastChar
#define qcmdExtendUpScreen		qcmdExtendPrevScreen
#define qcmdExtendDownScreen	qcmdExtendNextScreen
// End Legacy values

#define qcmdExtendLast		0xdb5f // LAST POSSIBLE EXTEND SELECTION COMMAND
#define qcmdSelectLast		0xdb5f // LAST POSSIBLE SELECTION COMMAND

// Support for keyboard font operations.
#define qcmdFontFirst		0xdb80 // FIRST FONT RELATED COMMAND
#define qcmdFontReset				0xdb80 	// reset the font formatting of selected text
#define qcmdFontToggleBold			0xdb81  // toggle the bold attribute of selected text
#define qcmdFontToggleItalic		0xdb82 	// toggle the italic attribute of selected text
#define qcmdFontToggleUnderline		0xdb83 	// toggle the underline attribute of selected text
#define qcmdFontToggleSubscript		0xdb84 	// toggle the subscript property of selected text
#define qcmdFontToggleSuperscript	0xdb85 	// toggle the superscript property of selected text
#define qcmdFontToggleAllCaps		0xdb86	// toggle the AllCaps property of selected text
#define qcmdFontToggleSmallCaps		0xdb87	// toggle the SmallCaps property of selected text
#define qcmdFontToggleWordUnderline	0xdb88 	// toggle the word-underline attribute of selected text
#define qcmdFontToggleDoubleUnderline 0xdb89 // toggle the double-underline attribute of selected text

#define qcmdFontLast			0xdb9f 	// LAST FONT RELATED COMMAND

// Support for keyboard paragraph operations
#define qcmdParaFirst		0xdba0 // FIRST PARA RELATED COMMAND
#define qcmdParaLeft		0xdba0 // align selected paragraphs left
#define qcmdParaCenter		0xdba1 // align selected paragraphs as centered
#define qcmdParaRight		0xdba2 // align selected paragraphs right
#define qcmdParaJustify		0xdba3 // align selected paragraphs as justified
#define qcmdParaSingleSpace	0xdba4 // single-space selected paragraphs
#define qcmdParaDoubleSpace	0xdba5 // double-space selected paragraphs
#define qcmdPara15Space		0xdba6 // 1.5 space selected paragraphs
#define qcmdParaToggleSpaceBefore 0xdba7 // toggle space-before-paragraph
#define qcmdParaIncreaseIndent 0xdba8			// Increase paragraph indent
#define qcmdParaDecreaseIndent 0xdba9			// Decrease paragraph indent
#define qcmdParaIncreaseHangingIndent 0xdbaa	// Increase hanging indent
#define qcmdParaDecreaseHangingIndent 0xdbab	// Decrease hanging indent
#define qcmdParaReset		0xdbac // Remove paragraph formatting

#define qcmdParaLast		0xdbbf // LAST PARA RELATED COMMAND

// Debugging - troubleshooting support
#define qcmdRedrawView			0xdbc0  // Redraw the view.
// Debugging - troubleshooting support
#define qcmdQuillDebugCentral	0xdbc1  // Put up debug dialog (DEBUG version only)
#define qcmdQuillDebugBreak		0xdbc2  // Break to the debugger (DEBUG version only)

#define qcmdQuillSpellerOn		0xdbc3  // Turns backqround speller on 
#define qcmdQuillSpellerOff		0xdbc4  // Turns backqround speller off
#define	qcmdToggleShowAll		0xdbc5	// Toggle ShowAll characters

// Support for IME operations
#define qcmdImeFirst	0xdbe0
#define qcmdImeAccept	0xdbe0	// Terminate any current IME composition and accept the changes
#define qcmdImeCancel	0xdbe1	// Terminate any current IME composition and ignore the changes

#define qcmdImeLast		0xdbef	// LAST IME RELATED COMMAND

// Quill internal use
#define	qtokenSkipNone		0x00
#define	qtokenSkipEditable	0x01

#define qavoidNone				0x00
#define	qavoidNonEditableTokens	0x01

// Support for additional Underline styles //Future: these might become tomXXX constants
#define	qkulThick			 6	//  Thick underline
#define qkulDashed			 7	//  Dashed line
#define	qkulDashedDot		 9	//  Dot-Dash..  
#define	qkulDashedDotDot	10	//  Dot-Dot-Dash...
#define qkulWavy			11	//  Wavy
#define qkulWavyHeavy		16	//  Thick wavy line
#define qkulDottedHeavy		17	//  Thick dotted line
#define qkulDashedHeavy		18	//	Thick dashed line
#define qkulDashedDotHeavy	19	//  Thick dot-dash...
#define qkulDashedDotDotHeavy 20	// thick dot-dot-dash..
#define qkulDashedLong		21	//	long dashes
#define qkulDashedLongHeavy 22	//	Thick long dashes
#define qkulWavyDouble		23	//  Double wavy underline

// Support for additional tab leader styles //Future: these might become tomXXX constants
#define qtlDoubleLine		4
#define qtlBullet			5

// Special Language IDs
#define lcidNeutral		MAKELCID(LANG_NEUTRAL, SUBLANG_NEUTRAL)

// Script identifiers
#define qsidDefault		0x00000001
#define qsidAsciiLatin	0x00000002
#define qsidLatin		0x00000004
#define qsidGreek		0x00000008
#define qsidCyrillic	0x00000010
#define qsidArmenian	0x00000020
#define qsidHebrew		0x00000040
#define qsidArabic		0x00000080
#define qsidDevanagari	0x00000100
#define qsidBengali		0x00000200
#define qsidGurmukhi	0x00000400
#define qsidGujarati	0x00000800
#define qsidOriya		0x00001000
#define qsidTamil		0x00002000
#define qsidTeluga		0x00004000
#define qsidKannada		0x00008000
#define qsidMalayalam	0x00010000
#define qsidThai		0x00020000
#define qsidLao			0x00040000
#define qsidTibetan		0x00080000
#define qsidGeorgian	0x00100000
#define qsidHangul		0x00200000
#define qsidKana		0x00400000
#define qsidBopomofo	0x00800000
#define qsidHan			0x01000000
#define qsidAll			0x01FFFFFF
#define qsidFEOnly		(qsidHangul|qsidKana|qsidBopomofo|qsidHan)

// Find text flags - in addition to TOM defined flags
#define qMatchWidth		0x10	// Far East: full width and half width characters NOT considered equivalent

// Emphasis marks
#define qemNone				0
#define qemOverSolidCircle	1
#define qemOverComma		2
#define qemOverRing			3

// Baseline Alignment
#define qbaHang		0
#define qbaCenter	1
#define qbaRoman	2
#define qbaBottom	3
#define qbaAuto		4
#define qbaMin		(qbaHang)
#define qbaMax		(qbaAuto)
#define qbaDefault	(qbaAuto)

// Story types - in addition to TOM defined story types
#define quillContinuedRegionStory	12
#define quillContinuedFromStory		13
#define quillContinuedOnStory		14

// Special language id for no proofing
#define qlidNoProof	0x80000000

#ifndef __midl
#undef IQDEBUGPROC
typedef HRESULT (CALLBACK* IQDEBUGPROC)(char*, char*, short, char*, short, BOOL, int *pResult);
#endif







/*----------------------------------------------------------------------------
	TextPropertyTags.hxx

	Owner: AlexMog
	Copyright (c) 1998-2000 Microsoft Corporation 

	Predefined property tags for text properties
	
----------------------------------------------------------------------------*/

#ifndef _TextPropertyTags_h_
#define _TextPropertyTags_h_

enum _enum_TextPropertyTags
{
// character properties
    tptagCharFirstTag = 1,
	tptagBold,
	tptagItalic,
	tptagUnderline,
	tptagOutline,
	tptagShadow,
	tptagStrikethrough,
	tptagSmallCaps,
	tptagCaps,
	tptagVanish,
	tptagProtected,
	tptagEmboss,
	tptagEngrave,
	tptagIss,
	tptagSfx,
	tptagCsk,
	tptagCrFore,
	tptagCrBack,
	tptagCrShadow,
	tptagCrEffectFill,
	tptagFont,						// REVIEW alexmog: handle composite fonts
	tptagFps,
	tptagFpsKern,
	tptagDylPos,
	tptagDxlSpace,
	tptagIsty,
	tptagLcid,
	tptagCharClientLong,
	tptagCtk,
	tptagCtkLong,
	tptagYsr,
	tptagFss,
	tptagFcs,
	tptagPctShadowHorzOffset,
	tptagPctShadowVertOffset,
	tptagQem,
	tptagNoProofing,
	tptagPassword,
	tptagFontName,
    tptagFfc,
// REVIEW alexmog: do something so that different properties could be grouped together
//                 e.g. char props could be added right after other char props, without
//                 affecting para props. It can be done by starting para props from some 
//                 large number, but then prop tables need to be organized differently

	tptagCharFormatReserved_0,
	tptagCharFormatReserved_1,
	tptagCharFormatReserved_2,
	tptagCharFormatReserved_3,
	tptagCharFormatReserved_4,
	tptagCharFormatReserved_5,
	tptagCharFormatReserved_6,
	tptagCharFormatReserved_7,
	tptagCharFormatReserved_8,
	tptagCharFormatReserved_9,


// Paragraph properties
    tptagParaFirstTag,
    tptagHorizontalAlignment,   // see tpvalAlignmentXxx for alignment values
    tptagVerticalAlignment,
    tptagClnDropCap, 
    tptagClnDropCapRaised, 
    tptagCchDropCap, 
    tptagPageBreakBefore, 
    tptagLineSpacing,           // needs special fetch/application. Units is important
    tptagIndentFirst,
    tptagIndentLeft,
    tptagIndentRight, 
    tptagSpaceBefore, 
    tptagSpaceAfter,
    tptagListType,
    tptagListLevel,
    tptagListAlignment, 
    tptagListDistanceToText,
    tptagListTab,
    tptagListFont,
    tptagListFontSize,
    tptagListColor,
    tptagListNumbering,
    tptagParaNumber,
    tptagTabs,
    tptagParaBorders,                   // unused, values undefined
    tptagBorderColor,
    tptagBordefLineStyle,
    tptagBordefLineWidth,
    tptagBordefLineSpacing,
    tptagShadingForeColor,
    tptagShadingBackColor,
    tptagShadingPattern,
    tptagParaClientLong,
    tptagKeepLinesTogether,
    tptagKeepWithFollowing,
    tptagParaStyle,
    tptagSuppressLineNumbering,
    tptagSuppressAutoHyphenation,
    tptagWidowControl,
    tptagOrphanControl,
    tptagCharacterSpaceControl,
    tptagFELineBreakControl,
    tptagFETextJustification,
    tptagFEBaseLineAlignment,
    tptagFENoWesternWordWrap,
    tptagHangingPunctiation,
    tptagHangulWordWrap,
};

enum _enum_TextPropertyValues
{
    tpvalAlignmentNone = 0,
    
    tpvalAlignmentHorz = 0,
    tpvalAlignmentLeft,
    tpvalAlignmentCenter,
    tpvalAlignmentRight,
    tpvalAlignmentJustified,
    tpvalAlignmentHorzLim,
    
    tpvalAlignmentVert = 0x10,
    tpvalAlignmentTop,
    tpvalAlignmentBottom,
    tpvalAlignmentVertCentered,
    tpvalAlignmentVertJustified,
    tpvalAlignmentVertLim,

    tpvalUnderlineNone = 0x20,
    tpvalUnderlineSingle,
    tpvalUnderlineWord,
    tpvalUnderlineDouble,
    tpvalUnderlineDot,
    tpvalUnderlineDash,
    tpvalUnderlineDotDash,
};

#define tpMaskColor 0xFF000000
#define tpMaskColorUndefined 0x01000000
#define tpMaskColorTransparent 0x02000000

#endif // _TextPropertyTags_h_







EXTERN_C const IID IID_ITextLayoutManager;

  interface  ITextLayoutManager : public IUnknown
    {
    public:
        virtual HRESULT  STDMETHODCALLTYPE CreateTextLayoutGroup(
                ITextLayoutGroup __RPC_FAR *__RPC_FAR *ppgrp ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE CheckDLLVersion(
                long verHeader , long __RPC_FAR *pverDll ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetResource(
                long iResource , long iResType , void __RPC_FAR *__RPC_FAR *ppRes ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE SetResource(
                long iResource , long iResType , void __RPC_FAR *pRes , int cbRes ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetHyphenationEngine(
                LPVOID __RPC_FAR *ppHyphEngine ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE SetHyphenationEngine(
                LPVOID pHyphEngine ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE CreateTextPropertyList(
                ITextPropertyList __RPC_FAR *__RPC_FAR *pptpl ) = 0;
    };

#ifdef DEBUG

#define CheckITextLayoutManagerMembers(klass)\
	struct _##klass##_ITextLayoutManager_CreateTextLayoutGroup\
		{\
		_##klass##_ITextLayoutManager_CreateTextLayoutGroup(HRESULT  (##klass##::*pfn)(ITextLayoutGroup __RPC_FAR *__RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutManager_CheckDLLVersion\
		{\
		_##klass##_ITextLayoutManager_CheckDLLVersion(HRESULT  (##klass##::*pfn)(long , long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutManager_GetResource\
		{\
		_##klass##_ITextLayoutManager_GetResource(HRESULT  (##klass##::*pfn)(long , long , void __RPC_FAR *__RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutManager_SetResource\
		{\
		_##klass##_ITextLayoutManager_SetResource(HRESULT  (##klass##::*pfn)(long , long , void __RPC_FAR *, int )){}\
		};\
	struct _##klass##_ITextLayoutManager_GetHyphenationEngine\
		{\
		_##klass##_ITextLayoutManager_GetHyphenationEngine(HRESULT  (##klass##::*pfn)(LPVOID __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutManager_SetHyphenationEngine\
		{\
		_##klass##_ITextLayoutManager_SetHyphenationEngine(HRESULT  (##klass##::*pfn)(LPVOID )){}\
		};\
	struct _##klass##_ITextLayoutManager_CreateTextPropertyList\
		{\
		_##klass##_ITextLayoutManager_CreateTextPropertyList(HRESULT  (##klass##::*pfn)(ITextPropertyList __RPC_FAR *__RPC_FAR *)){}\
		};\
	void klass::VerifyITextLayoutManager(){\
	_##klass##_ITextLayoutManager_CreateTextLayoutGroup pfn1(CreateTextLayoutGroup);\
	_##klass##_ITextLayoutManager_CheckDLLVersion pfn2(CheckDLLVersion);\
	_##klass##_ITextLayoutManager_GetResource pfn3(GetResource);\
	_##klass##_ITextLayoutManager_SetResource pfn4(SetResource);\
	_##klass##_ITextLayoutManager_GetHyphenationEngine pfn5(GetHyphenationEngine);\
	_##klass##_ITextLayoutManager_SetHyphenationEngine pfn6(SetHyphenationEngine);\
	_##klass##_ITextLayoutManager_CreateTextPropertyList pfn7(CreateTextPropertyList);\
	}\

#else

#define CheckITextLayoutManagerMembers(klass)

#endif /* DEBUG */

#define idITextLayoutManager	0x7f67baa0

#ifdef DeclareSmartPointer
#ifndef ITextLayoutManagerSPMethodsDefined
extern "C++" { TEMPLATECLASSB class ITextLayoutManagerSPMethods : public TEMPLATEBASEB(IUnknownSPMethods) { }; }
#define ITextLayoutManagerSPMethodsDefined
#endif
DeclareSmartPointer(ITextLayoutManager)
#define SPITextLayoutManager auto SP_ITextLayoutManager
#endif

#define DeclareITextLayoutManagerMethods()\
	(FNOBJECT) CreateTextPropertyList,\
	(FNOBJECT) SetHyphenationEngine,\
	(FNOBJECT) GetHyphenationEngine,\
	(FNOBJECT) SetResource,\
	(FNOBJECT) GetResource,\
	(FNOBJECT) CheckDLLVersion,\
	(FNOBJECT) CreateTextLayoutGroup,\

#define DeclareITextLayoutManagerVtbl()\
	,_Method1\
	,_Method2\
	,_Method3\
	,_Method4\
	,_Method5\
	,_Method6\
	,_Method7\





EXTERN_C const IID IID_ITextLayoutAccess;

  interface  ITextLayoutAccess : public IUnknown
    {
    public:
        virtual HRESULT  STDMETHODCALLTYPE GetTextLayoutGroup(
                ITextLayoutGroup __RPC_FAR *__RPC_FAR *ppgrp ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetLayoutOptions(
                DWORD __RPC_FAR *pdwOptions ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE SetLayoutOptions(
                DWORD dwOptions ) = 0;
    };

#ifdef DEBUG

#define CheckITextLayoutAccessMembers(klass)\
	struct _##klass##_ITextLayoutAccess_GetTextLayoutGroup\
		{\
		_##klass##_ITextLayoutAccess_GetTextLayoutGroup(HRESULT  (##klass##::*pfn)(ITextLayoutGroup __RPC_FAR *__RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutAccess_GetLayoutOptions\
		{\
		_##klass##_ITextLayoutAccess_GetLayoutOptions(HRESULT  (##klass##::*pfn)(DWORD __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutAccess_SetLayoutOptions\
		{\
		_##klass##_ITextLayoutAccess_SetLayoutOptions(HRESULT  (##klass##::*pfn)(DWORD )){}\
		};\
	void klass::VerifyITextLayoutAccess(){\
	_##klass##_ITextLayoutAccess_GetTextLayoutGroup pfn1(GetTextLayoutGroup);\
	_##klass##_ITextLayoutAccess_GetLayoutOptions pfn2(GetLayoutOptions);\
	_##klass##_ITextLayoutAccess_SetLayoutOptions pfn3(SetLayoutOptions);\
	}\

#else

#define CheckITextLayoutAccessMembers(klass)

#endif /* DEBUG */

#define idITextLayoutAccess	0x5CA01BC0

#ifdef DeclareSmartPointer
#ifndef ITextLayoutAccessSPMethodsDefined
extern "C++" { TEMPLATECLASSB class ITextLayoutAccessSPMethods : public TEMPLATEBASEB(IUnknownSPMethods) { }; }
#define ITextLayoutAccessSPMethodsDefined
#endif
DeclareSmartPointer(ITextLayoutAccess)
#define SPITextLayoutAccess auto SP_ITextLayoutAccess
#endif

#define DeclareITextLayoutAccessMethods()\
	(FNOBJECT) SetLayoutOptions,\
	(FNOBJECT) GetLayoutOptions,\
	(FNOBJECT) GetTextLayoutGroup,\

#define DeclareITextLayoutAccessVtbl()\
	,_Method1\
	,_Method2\
	,_Method3\





EXTERN_C const IID IID_ITextLayoutGroup;

  interface  ITextLayoutGroup : public IUnknown
    {
    public:
        virtual HRESULT  STDMETHODCALLTYPE SetTextLayoutGroupSite(
                ITextLayoutGroupSite __RPC_FAR *pgrpsite ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE CreateTextLayoutElement(
                ITextStory __RPC_FAR *ptstory , ITextLayoutElement __RPC_FAR *__RPC_FAR *pptle ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE SetTargetContext(
                HDC hdc , BOOL fInval ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE SetReadyState(
                int nReadyState ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE Idle(
                unsigned int tickNow ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE SetTextDisplayOptions(
                long tdo ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetTextDisplayOptions(
                long __RPC_FAR *ptdo ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE SetEventNotificationOptions(
                long eno ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetEventNotificationOptions(
                long __RPC_FAR *peno ) = 0;
    };

#ifdef DEBUG

#define CheckITextLayoutGroupMembers(klass)\
	struct _##klass##_ITextLayoutGroup_SetTextLayoutGroupSite\
		{\
		_##klass##_ITextLayoutGroup_SetTextLayoutGroupSite(HRESULT  (##klass##::*pfn)(ITextLayoutGroupSite __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutGroup_CreateTextLayoutElement\
		{\
		_##klass##_ITextLayoutGroup_CreateTextLayoutElement(HRESULT  (##klass##::*pfn)(ITextStory __RPC_FAR *, ITextLayoutElement __RPC_FAR *__RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutGroup_SetTargetContext\
		{\
		_##klass##_ITextLayoutGroup_SetTargetContext(HRESULT  (##klass##::*pfn)(HDC , BOOL )){}\
		};\
	struct _##klass##_ITextLayoutGroup_SetReadyState\
		{\
		_##klass##_ITextLayoutGroup_SetReadyState(HRESULT  (##klass##::*pfn)(int )){}\
		};\
	struct _##klass##_ITextLayoutGroup_Idle\
		{\
		_##klass##_ITextLayoutGroup_Idle(HRESULT  (##klass##::*pfn)(unsigned int )){}\
		};\
	struct _##klass##_ITextLayoutGroup_SetTextDisplayOptions\
		{\
		_##klass##_ITextLayoutGroup_SetTextDisplayOptions(HRESULT  (##klass##::*pfn)(long )){}\
		};\
	struct _##klass##_ITextLayoutGroup_GetTextDisplayOptions\
		{\
		_##klass##_ITextLayoutGroup_GetTextDisplayOptions(HRESULT  (##klass##::*pfn)(long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutGroup_SetEventNotificationOptions\
		{\
		_##klass##_ITextLayoutGroup_SetEventNotificationOptions(HRESULT  (##klass##::*pfn)(long )){}\
		};\
	struct _##klass##_ITextLayoutGroup_GetEventNotificationOptions\
		{\
		_##klass##_ITextLayoutGroup_GetEventNotificationOptions(HRESULT  (##klass##::*pfn)(long __RPC_FAR *)){}\
		};\
	void klass::VerifyITextLayoutGroup(){\
	_##klass##_ITextLayoutGroup_SetTextLayoutGroupSite pfn1(SetTextLayoutGroupSite);\
	_##klass##_ITextLayoutGroup_CreateTextLayoutElement pfn2(CreateTextLayoutElement);\
	_##klass##_ITextLayoutGroup_SetTargetContext pfn3(SetTargetContext);\
	_##klass##_ITextLayoutGroup_SetReadyState pfn4(SetReadyState);\
	_##klass##_ITextLayoutGroup_Idle pfn5(Idle);\
	_##klass##_ITextLayoutGroup_SetTextDisplayOptions pfn6(SetTextDisplayOptions);\
	_##klass##_ITextLayoutGroup_GetTextDisplayOptions pfn7(GetTextDisplayOptions);\
	_##klass##_ITextLayoutGroup_SetEventNotificationOptions pfn8(SetEventNotificationOptions);\
	_##klass##_ITextLayoutGroup_GetEventNotificationOptions pfn9(GetEventNotificationOptions);\
	}\

#else

#define CheckITextLayoutGroupMembers(klass)

#endif /* DEBUG */

#define idITextLayoutGroup	0x7f67baa1

#ifdef DeclareSmartPointer
#ifndef ITextLayoutGroupSPMethodsDefined
extern "C++" { TEMPLATECLASSB class ITextLayoutGroupSPMethods : public TEMPLATEBASEB(IUnknownSPMethods) { }; }
#define ITextLayoutGroupSPMethodsDefined
#endif
DeclareSmartPointer(ITextLayoutGroup)
#define SPITextLayoutGroup auto SP_ITextLayoutGroup
#endif

#define DeclareITextLayoutGroupMethods()\
	(FNOBJECT) GetEventNotificationOptions,\
	(FNOBJECT) SetEventNotificationOptions,\
	(FNOBJECT) GetTextDisplayOptions,\
	(FNOBJECT) SetTextDisplayOptions,\
	(FNOBJECT) Idle,\
	(FNOBJECT) SetReadyState,\
	(FNOBJECT) SetTargetContext,\
	(FNOBJECT) CreateTextLayoutElement,\
	(FNOBJECT) SetTextLayoutGroupSite,\

#define DeclareITextLayoutGroupVtbl()\
	,_Method1\
	,_Method2\
	,_Method3\
	,_Method4\
	,_Method5\
	,_Method6\
	,_Method7\
	,_Method8\
	,_Method9\





EXTERN_C const IID IID_ITextLayoutElement;

  interface  ITextLayoutElement : public IUnknown
    {
    public:
        virtual HRESULT  STDMETHODCALLTYPE Remove(
                void ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetTextStory(
                ITextStory __RPC_FAR *__RPC_FAR *ppstory ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE SetTextLayoutSite(
                ITextLayoutSite __RPC_FAR *psite , DWORD dwSiteCookie ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetTextLayoutSite(
                ITextLayoutSite __RPC_FAR *__RPC_FAR *ppsite , DWORD __RPC_FAR *pdwSiteCookie ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetMinMaxWidth(
                int nUnit , long __RPC_FAR *pdxtMin , long __RPC_FAR *pdxtMax , long __RPC_FAR *pdytHeight , long __RPC_FAR *pdytBaseline , BOOL __RPC_FAR *pfNoContent ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetLayoutHeight(
                int nUnit , long fFullRecalc , long fLazy , long __RPC_FAR *pdxtLongestLine , long __RPC_FAR *pdytHeight , long __RPC_FAR *pdytBaseline , int fComputeExactWidth , BOOL __RPC_FAR *pfNoContent ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE ShiftLines(
                long dxpBody , long dypHeight , long dxpLeft , long dxpRight , long dypTop , long dypBottom ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE HaveLines(
                void ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetMaxCpCalced(
                long __RPC_FAR *pcpMaxCalced ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE Update(
                HDC hdc , RECT __RPC_FAR *prcView , RECT __RPC_FAR *prcInvalid , DWORD dwFlags ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE Render(
                HDC hdc , long X , long Y , long dxpInch , long dypInch , DWORD dwFlags , RECT __RPC_FAR *prcClip , RECT __RPC_FAR *prcWBounds , float degs ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE HandleMessage(
                UINT msg , WPARAM wParam , LPARAM lParam , LRESULT __RPC_FAR *plResultParam ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE HitTestPoint(
                POINT __RPC_FAR *pptLoc , long __RPC_FAR *pcrs , long __RPC_FAR *pobj ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE CpFromPoint(
                POINT __RPC_FAR *pptLoc , DWORD dwFlags , long __RPC_FAR *pcp , long __RPC_FAR *pfEOL ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetCpMetrics(
                long cp , long Type , RECT __RPC_FAR *prcCp , long __RPC_FAR *px , long __RPC_FAR *py ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetLineInfoFromPoint(
                long dxpPos , long dypPos , BOOL fSkipFrame , long xExposedWidth , BOOL fIgnoreRelLines , long __RPC_FAR *pdypLineTop , long __RPC_FAR *pcpFirst ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetLineInfo(
                LONG cp , BOOL fEnd , LONG __RPC_FAR *plBaseline , LONG __RPC_FAR *plXPosition , LONG __RPC_FAR *plLineHeight , LONG __RPC_FAR *plTextHeight , LONG __RPC_FAR *plDescent ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE CpMoveFromCp(
                LONG cpCur , BOOL fEolCur , POINT pt , long lUnit , LONG __RPC_FAR *pCpNew , BOOL __RPC_FAR *pEolNew ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE NotifySelectionChanged(
                long cpStart , long cpEnd , BOOL fSelected ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE IsCommandAvailable(
                long qcmd ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE ExecCommand(
                long qcmd ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE MouseDown(
                HWND hwnd , HDC hdc , POINT ptDown , long qclick ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE MouseMove(
                HWND hwnd , HDC hdc , POINT ptMove ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE MouseUp(
                HWND hwnd , HDC hdc , POINT ptUp ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE Notify(
                ITextChangeNotification __RPC_FAR *pnf , BOOL __RPC_FAR *pfRequestResize ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE NotifyLayoutChanged(
                void ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE NotifyConstraintPointsChanged(
                void ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE RegionFromRange(
                long cpStart , long cpFinish , long cpClipStart , long cpClipFinish , void __RPC_FAR *paryRects , BOOL fRestrictToVisible , BOOL fBlockElement , long xOffset , long yOffset , RECT __RPC_FAR *prcBoundingRect ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE Reset(
                void ) = 0;
    };

#ifdef DEBUG

#define CheckITextLayoutElementMembers(klass)\
	struct _##klass##_ITextLayoutElement_Remove\
		{\
		_##klass##_ITextLayoutElement_Remove(HRESULT  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextLayoutElement_GetTextStory\
		{\
		_##klass##_ITextLayoutElement_GetTextStory(HRESULT  (##klass##::*pfn)(ITextStory __RPC_FAR *__RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutElement_SetTextLayoutSite\
		{\
		_##klass##_ITextLayoutElement_SetTextLayoutSite(HRESULT  (##klass##::*pfn)(ITextLayoutSite __RPC_FAR *, DWORD )){}\
		};\
	struct _##klass##_ITextLayoutElement_GetTextLayoutSite\
		{\
		_##klass##_ITextLayoutElement_GetTextLayoutSite(HRESULT  (##klass##::*pfn)(ITextLayoutSite __RPC_FAR *__RPC_FAR *, DWORD __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutElement_GetMinMaxWidth\
		{\
		_##klass##_ITextLayoutElement_GetMinMaxWidth(HRESULT  (##klass##::*pfn)(int , long __RPC_FAR *, long __RPC_FAR *, long __RPC_FAR *, long __RPC_FAR *, BOOL __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutElement_GetLayoutHeight\
		{\
		_##klass##_ITextLayoutElement_GetLayoutHeight(HRESULT  (##klass##::*pfn)(int , long , long , long __RPC_FAR *, long __RPC_FAR *, long __RPC_FAR *, int , BOOL __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutElement_ShiftLines\
		{\
		_##klass##_ITextLayoutElement_ShiftLines(HRESULT  (##klass##::*pfn)(long , long , long , long , long , long )){}\
		};\
	struct _##klass##_ITextLayoutElement_HaveLines\
		{\
		_##klass##_ITextLayoutElement_HaveLines(HRESULT  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextLayoutElement_GetMaxCpCalced\
		{\
		_##klass##_ITextLayoutElement_GetMaxCpCalced(HRESULT  (##klass##::*pfn)(long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutElement_Update\
		{\
		_##klass##_ITextLayoutElement_Update(HRESULT  (##klass##::*pfn)(HDC , RECT __RPC_FAR *, RECT __RPC_FAR *, DWORD )){}\
		};\
	struct _##klass##_ITextLayoutElement_Render\
		{\
		_##klass##_ITextLayoutElement_Render(HRESULT  (##klass##::*pfn)(HDC , long , long , long , long , DWORD , RECT __RPC_FAR *, RECT __RPC_FAR *, float )){}\
		};\
	struct _##klass##_ITextLayoutElement_HandleMessage\
		{\
		_##klass##_ITextLayoutElement_HandleMessage(HRESULT  (##klass##::*pfn)(UINT , WPARAM , LPARAM , LRESULT __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutElement_HitTestPoint\
		{\
		_##klass##_ITextLayoutElement_HitTestPoint(HRESULT  (##klass##::*pfn)(POINT __RPC_FAR *, long __RPC_FAR *, long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutElement_CpFromPoint\
		{\
		_##klass##_ITextLayoutElement_CpFromPoint(HRESULT  (##klass##::*pfn)(POINT __RPC_FAR *, DWORD , long __RPC_FAR *, long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutElement_GetCpMetrics\
		{\
		_##klass##_ITextLayoutElement_GetCpMetrics(HRESULT  (##klass##::*pfn)(long , long , RECT __RPC_FAR *, long __RPC_FAR *, long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutElement_GetLineInfoFromPoint\
		{\
		_##klass##_ITextLayoutElement_GetLineInfoFromPoint(HRESULT  (##klass##::*pfn)(long , long , BOOL , long , BOOL , long __RPC_FAR *, long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutElement_GetLineInfo\
		{\
		_##klass##_ITextLayoutElement_GetLineInfo(HRESULT  (##klass##::*pfn)(LONG , BOOL , LONG __RPC_FAR *, LONG __RPC_FAR *, LONG __RPC_FAR *, LONG __RPC_FAR *, LONG __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutElement_CpMoveFromCp\
		{\
		_##klass##_ITextLayoutElement_CpMoveFromCp(HRESULT  (##klass##::*pfn)(LONG , BOOL , POINT , long , LONG __RPC_FAR *, BOOL __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutElement_NotifySelectionChanged\
		{\
		_##klass##_ITextLayoutElement_NotifySelectionChanged(HRESULT  (##klass##::*pfn)(long , long , BOOL )){}\
		};\
	struct _##klass##_ITextLayoutElement_IsCommandAvailable\
		{\
		_##klass##_ITextLayoutElement_IsCommandAvailable(HRESULT  (##klass##::*pfn)(long )){}\
		};\
	struct _##klass##_ITextLayoutElement_ExecCommand\
		{\
		_##klass##_ITextLayoutElement_ExecCommand(HRESULT  (##klass##::*pfn)(long )){}\
		};\
	struct _##klass##_ITextLayoutElement_MouseDown\
		{\
		_##klass##_ITextLayoutElement_MouseDown(HRESULT  (##klass##::*pfn)(HWND , HDC , POINT , long )){}\
		};\
	struct _##klass##_ITextLayoutElement_MouseMove\
		{\
		_##klass##_ITextLayoutElement_MouseMove(HRESULT  (##klass##::*pfn)(HWND , HDC , POINT )){}\
		};\
	struct _##klass##_ITextLayoutElement_MouseUp\
		{\
		_##klass##_ITextLayoutElement_MouseUp(HRESULT  (##klass##::*pfn)(HWND , HDC , POINT )){}\
		};\
	struct _##klass##_ITextLayoutElement_Notify\
		{\
		_##klass##_ITextLayoutElement_Notify(HRESULT  (##klass##::*pfn)(ITextChangeNotification __RPC_FAR *, BOOL __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutElement_NotifyLayoutChanged\
		{\
		_##klass##_ITextLayoutElement_NotifyLayoutChanged(HRESULT  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextLayoutElement_NotifyConstraintPointsChanged\
		{\
		_##klass##_ITextLayoutElement_NotifyConstraintPointsChanged(HRESULT  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextLayoutElement_RegionFromRange\
		{\
		_##klass##_ITextLayoutElement_RegionFromRange(HRESULT  (##klass##::*pfn)(long , long , long , long , void __RPC_FAR *, BOOL , BOOL , long , long , RECT __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutElement_Reset\
		{\
		_##klass##_ITextLayoutElement_Reset(HRESULT  (##klass##::*pfn)(void )){}\
		};\
	void klass::VerifyITextLayoutElement(){\
	_##klass##_ITextLayoutElement_Remove pfn1(Remove);\
	_##klass##_ITextLayoutElement_GetTextStory pfn2(GetTextStory);\
	_##klass##_ITextLayoutElement_SetTextLayoutSite pfn3(SetTextLayoutSite);\
	_##klass##_ITextLayoutElement_GetTextLayoutSite pfn4(GetTextLayoutSite);\
	_##klass##_ITextLayoutElement_GetMinMaxWidth pfn5(GetMinMaxWidth);\
	_##klass##_ITextLayoutElement_GetLayoutHeight pfn6(GetLayoutHeight);\
	_##klass##_ITextLayoutElement_ShiftLines pfn7(ShiftLines);\
	_##klass##_ITextLayoutElement_HaveLines pfn8(HaveLines);\
	_##klass##_ITextLayoutElement_GetMaxCpCalced pfn9(GetMaxCpCalced);\
	_##klass##_ITextLayoutElement_Update pfn10(Update);\
	_##klass##_ITextLayoutElement_Render pfn11(Render);\
	_##klass##_ITextLayoutElement_HandleMessage pfn12(HandleMessage);\
	_##klass##_ITextLayoutElement_HitTestPoint pfn13(HitTestPoint);\
	_##klass##_ITextLayoutElement_CpFromPoint pfn14(CpFromPoint);\
	_##klass##_ITextLayoutElement_GetCpMetrics pfn15(GetCpMetrics);\
	_##klass##_ITextLayoutElement_GetLineInfoFromPoint pfn16(GetLineInfoFromPoint);\
	_##klass##_ITextLayoutElement_GetLineInfo pfn17(GetLineInfo);\
	_##klass##_ITextLayoutElement_CpMoveFromCp pfn18(CpMoveFromCp);\
	_##klass##_ITextLayoutElement_NotifySelectionChanged pfn19(NotifySelectionChanged);\
	_##klass##_ITextLayoutElement_IsCommandAvailable pfn20(IsCommandAvailable);\
	_##klass##_ITextLayoutElement_ExecCommand pfn21(ExecCommand);\
	_##klass##_ITextLayoutElement_MouseDown pfn22(MouseDown);\
	_##klass##_ITextLayoutElement_MouseMove pfn23(MouseMove);\
	_##klass##_ITextLayoutElement_MouseUp pfn24(MouseUp);\
	_##klass##_ITextLayoutElement_Notify pfn25(Notify);\
	_##klass##_ITextLayoutElement_NotifyLayoutChanged pfn26(NotifyLayoutChanged);\
	_##klass##_ITextLayoutElement_NotifyConstraintPointsChanged pfn27(NotifyConstraintPointsChanged);\
	_##klass##_ITextLayoutElement_RegionFromRange pfn28(RegionFromRange);\
	_##klass##_ITextLayoutElement_Reset pfn29(Reset);\
	}\

#else

#define CheckITextLayoutElementMembers(klass)

#endif /* DEBUG */

#define idITextLayoutElement	0x7f67baa2

#ifdef DeclareSmartPointer
#ifndef ITextLayoutElementSPMethodsDefined
extern "C++" { TEMPLATECLASSB class ITextLayoutElementSPMethods : public TEMPLATEBASEB(IUnknownSPMethods) { }; }
#define ITextLayoutElementSPMethodsDefined
#endif
DeclareSmartPointer(ITextLayoutElement)
#define SPITextLayoutElement auto SP_ITextLayoutElement
#endif

#define DeclareITextLayoutElementMethods()\
	(FNOBJECT) Reset,\
	(FNOBJECT) RegionFromRange,\
	(FNOBJECT) NotifyConstraintPointsChanged,\
	(FNOBJECT) NotifyLayoutChanged,\
	(FNOBJECT) Notify,\
	(FNOBJECT) MouseUp,\
	(FNOBJECT) MouseMove,\
	(FNOBJECT) MouseDown,\
	(FNOBJECT) ExecCommand,\
	(FNOBJECT) IsCommandAvailable,\
	(FNOBJECT) NotifySelectionChanged,\
	(FNOBJECT) CpMoveFromCp,\
	(FNOBJECT) GetLineInfo,\
	(FNOBJECT) GetLineInfoFromPoint,\
	(FNOBJECT) GetCpMetrics,\
	(FNOBJECT) CpFromPoint,\
	(FNOBJECT) HitTestPoint,\
	(FNOBJECT) HandleMessage,\
	(FNOBJECT) Render,\
	(FNOBJECT) Update,\
	(FNOBJECT) GetMaxCpCalced,\
	(FNOBJECT) HaveLines,\
	(FNOBJECT) ShiftLines,\
	(FNOBJECT) GetLayoutHeight,\
	(FNOBJECT) GetMinMaxWidth,\
	(FNOBJECT) GetTextLayoutSite,\
	(FNOBJECT) SetTextLayoutSite,\
	(FNOBJECT) GetTextStory,\
	(FNOBJECT) Remove,\

#define DeclareITextLayoutElementVtbl()\
	,_Method1\
	,_Method2\
	,_Method3\
	,_Method4\
	,_Method5\
	,_Method6\
	,_Method7\
	,_Method8\
	,_Method9\
	,_Method10\
	,_Method11\
	,_Method12\
	,_Method13\
	,_Method14\
	,_Method15\
	,_Method16\
	,_Method17\
	,_Method18\
	,_Method19\
	,_Method20\
	,_Method21\
	,_Method22\
	,_Method23\
	,_Method24\
	,_Method25\
	,_Method26\
	,_Method27\
	,_Method28\
	,_Method29\





EXTERN_C const IID IID_ITextLayoutGroupSite;

  interface  ITextLayoutGroupSite : public IUnknown
    {
    public:
        virtual HRESULT  STDMETHODCALLTYPE GetDefaultTextProperties(
                ITextPropertyList __RPC_FAR *__RPC_FAR *pptpl , int __RPC_FAR *__RPC_FAR *__RPC_FAR *TBD ) = 0;
        virtual void  STDMETHODCALLTYPE OnError(
                HRESULT hr ) = 0;
        virtual void  STDMETHODCALLTYPE BeginLongOp(
                void ) = 0;
        virtual void  STDMETHODCALLTYPE EndLongOp(
                void ) = 0;
    };

#ifdef DEBUG

#define CheckITextLayoutGroupSiteMembers(klass)\
	struct _##klass##_ITextLayoutGroupSite_GetDefaultTextProperties\
		{\
		_##klass##_ITextLayoutGroupSite_GetDefaultTextProperties(HRESULT  (##klass##::*pfn)(ITextPropertyList __RPC_FAR *__RPC_FAR *, int __RPC_FAR *__RPC_FAR *__RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutGroupSite_OnError\
		{\
		_##klass##_ITextLayoutGroupSite_OnError(void  (##klass##::*pfn)(HRESULT )){}\
		};\
	struct _##klass##_ITextLayoutGroupSite_BeginLongOp\
		{\
		_##klass##_ITextLayoutGroupSite_BeginLongOp(void  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextLayoutGroupSite_EndLongOp\
		{\
		_##klass##_ITextLayoutGroupSite_EndLongOp(void  (##klass##::*pfn)(void )){}\
		};\
	void klass::VerifyITextLayoutGroupSite(){\
	_##klass##_ITextLayoutGroupSite_GetDefaultTextProperties pfn1(GetDefaultTextProperties);\
	_##klass##_ITextLayoutGroupSite_OnError pfn2(OnError);\
	_##klass##_ITextLayoutGroupSite_BeginLongOp pfn3(BeginLongOp);\
	_##klass##_ITextLayoutGroupSite_EndLongOp pfn4(EndLongOp);\
	}\

#else

#define CheckITextLayoutGroupSiteMembers(klass)

#endif /* DEBUG */

#define idITextLayoutGroupSite	0x7f67baa3

#ifdef DeclareSmartPointer
#ifndef ITextLayoutGroupSiteSPMethodsDefined
extern "C++" { TEMPLATECLASSB class ITextLayoutGroupSiteSPMethods : public TEMPLATEBASEB(IUnknownSPMethods) { }; }
#define ITextLayoutGroupSiteSPMethodsDefined
#endif
DeclareSmartPointer(ITextLayoutGroupSite)
#define SPITextLayoutGroupSite auto SP_ITextLayoutGroupSite
#endif

#define DeclareITextLayoutGroupSiteMethods()\
	(FNOBJECT) EndLongOp,\
	(FNOBJECT) BeginLongOp,\
	(FNOBJECT) OnError,\
	(FNOBJECT) GetDefaultTextProperties,\

#define DeclareITextLayoutGroupSiteVtbl()\
	,_Method1\
	,_Method2\
	,_Method3\
	,_Method4\





EXTERN_C const IID IID_ITextStory;

  interface  ITextStory : public IUnknown
    {
    public:
        virtual HRESULT  STDMETHODCALLTYPE GetLength(
                long __RPC_FAR *pcpMac ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetRunLimits(
                long cp , long __RPC_FAR *pcpFirst , long __RPC_FAR *pcpLim ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetPapLimits(
                long cp , long __RPC_FAR *pcpFirst , long __RPC_FAR *pcpLim ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE FetchText(
                long cp , wchar_t __RPC_FAR *pchBuffer , int cchBuffer , int __RPC_FAR *pcchFetch ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE FetchTextProperties(
                long cp , long propertyGroup , ITextPropertyList __RPC_FAR *__RPC_FAR *pptpl ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE IsNestedLayoutAtCp(
                DWORD dwCookieTP , long cp ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetNestedObjectMargins(
                DWORD dwCookieTP , long cp , RECT __RPC_FAR *rcMargins ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetNestedObjectAttributes(
                DWORD dwCookieTP , long cp , long __RPC_FAR *pcwchRun , long __RPC_FAR *palign , long __RPC_FAR *pstylePos , DWORD __RPC_FAR *pdwFlags ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetHtmlDoc(
                IHTMLDocument2 __RPC_FAR *__RPC_FAR *ppDoc ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetCurrentElement(
                IHTMLElement __RPC_FAR *__RPC_FAR *ppElement ) = 0;
    };

#ifdef DEBUG

#define CheckITextStoryMembers(klass)\
	struct _##klass##_ITextStory_GetLength\
		{\
		_##klass##_ITextStory_GetLength(HRESULT  (##klass##::*pfn)(long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextStory_GetRunLimits\
		{\
		_##klass##_ITextStory_GetRunLimits(HRESULT  (##klass##::*pfn)(long , long __RPC_FAR *, long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextStory_GetPapLimits\
		{\
		_##klass##_ITextStory_GetPapLimits(HRESULT  (##klass##::*pfn)(long , long __RPC_FAR *, long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextStory_FetchText\
		{\
		_##klass##_ITextStory_FetchText(HRESULT  (##klass##::*pfn)(long , wchar_t __RPC_FAR *, int , int __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextStory_FetchTextProperties\
		{\
		_##klass##_ITextStory_FetchTextProperties(HRESULT  (##klass##::*pfn)(long , long , ITextPropertyList __RPC_FAR *__RPC_FAR *)){}\
		};\
	struct _##klass##_ITextStory_IsNestedLayoutAtCp\
		{\
		_##klass##_ITextStory_IsNestedLayoutAtCp(HRESULT  (##klass##::*pfn)(DWORD , long )){}\
		};\
	struct _##klass##_ITextStory_GetNestedObjectMargins\
		{\
		_##klass##_ITextStory_GetNestedObjectMargins(HRESULT  (##klass##::*pfn)(DWORD , long , RECT __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextStory_GetNestedObjectAttributes\
		{\
		_##klass##_ITextStory_GetNestedObjectAttributes(HRESULT  (##klass##::*pfn)(DWORD , long , long __RPC_FAR *, long __RPC_FAR *, long __RPC_FAR *, DWORD __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextStory_GetHtmlDoc\
		{\
		_##klass##_ITextStory_GetHtmlDoc(HRESULT  (##klass##::*pfn)(IHTMLDocument2 __RPC_FAR *__RPC_FAR *)){}\
		};\
	struct _##klass##_ITextStory_GetCurrentElement\
		{\
		_##klass##_ITextStory_GetCurrentElement(HRESULT  (##klass##::*pfn)(IHTMLElement __RPC_FAR *__RPC_FAR *)){}\
		};\
	void klass::VerifyITextStory(){\
	_##klass##_ITextStory_GetLength pfn1(GetLength);\
	_##klass##_ITextStory_GetRunLimits pfn2(GetRunLimits);\
	_##klass##_ITextStory_GetPapLimits pfn3(GetPapLimits);\
	_##klass##_ITextStory_FetchText pfn4(FetchText);\
	_##klass##_ITextStory_FetchTextProperties pfn5(FetchTextProperties);\
	_##klass##_ITextStory_IsNestedLayoutAtCp pfn6(IsNestedLayoutAtCp);\
	_##klass##_ITextStory_GetNestedObjectMargins pfn7(GetNestedObjectMargins);\
	_##klass##_ITextStory_GetNestedObjectAttributes pfn8(GetNestedObjectAttributes);\
	_##klass##_ITextStory_GetHtmlDoc pfn9(GetHtmlDoc);\
	_##klass##_ITextStory_GetCurrentElement pfn10(GetCurrentElement);\
	}\

#else

#define CheckITextStoryMembers(klass)

#endif /* DEBUG */

#define idITextStory	0x7f67baa4

#ifdef DeclareSmartPointer
#ifndef ITextStorySPMethodsDefined
extern "C++" { TEMPLATECLASSB class ITextStorySPMethods : public TEMPLATEBASEB(IUnknownSPMethods) { }; }
#define ITextStorySPMethodsDefined
#endif
DeclareSmartPointer(ITextStory)
#define SPITextStory auto SP_ITextStory
#endif

#define DeclareITextStoryMethods()\
	(FNOBJECT) GetCurrentElement,\
	(FNOBJECT) GetHtmlDoc,\
	(FNOBJECT) GetNestedObjectAttributes,\
	(FNOBJECT) GetNestedObjectMargins,\
	(FNOBJECT) IsNestedLayoutAtCp,\
	(FNOBJECT) FetchTextProperties,\
	(FNOBJECT) FetchText,\
	(FNOBJECT) GetPapLimits,\
	(FNOBJECT) GetRunLimits,\
	(FNOBJECT) GetLength,\

#define DeclareITextStoryVtbl()\
	,_Method1\
	,_Method2\
	,_Method3\
	,_Method4\
	,_Method5\
	,_Method6\
	,_Method7\
	,_Method8\
	,_Method9\
	,_Method10\





EXTERN_C const IID IID_ITextLayoutSite;

  interface  ITextLayoutSite : public IUnknown
    {
    public:
        virtual HRESULT  STDMETHODCALLTYPE GetLayoutSize(
                DWORD dwSiteCookie , int nUnit , long __RPC_FAR *pdxpWidth , long __RPC_FAR *pdypHeight ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetPadding(
                DWORD dwSiteCookie , int nUnit , long __RPC_FAR *pdxpLeft , long __RPC_FAR *pdxpRight , long __RPC_FAR *pdypTop , long __RPC_FAR *pdypBottom ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetColumns(
                DWORD dwSiteCookie , int __RPC_FAR *pcColumns ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetConstraintPoints(
                DWORD dwSiteCookie , ITextConstraintPoints __RPC_FAR *__RPC_FAR *ppPoints , DWORD __RPC_FAR *pdwConsCookie ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetDisplayRect(
                DWORD dwSiteCookie , RECT __RPC_FAR *prect ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetScale(
                DWORD dwSiteCookie , long __RPC_FAR *pdxpInch , long __RPC_FAR *pdypInch ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetBkgndMode(
                DWORD dwSiteCookie , long __RPC_FAR *pBkMode , COLORREF __RPC_FAR *pcrBack ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetRotation(
                DWORD dwSiteCookie , float __RPC_FAR *pdegs ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetTextFlow(
                DWORD dwSiteCookie , long __RPC_FAR *pqtf ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetVerticalAlign(
                DWORD dwSiteCookie , long __RPC_FAR *pVA ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetAllowSplitWordsAcrossConstraints(
                DWORD dwSiteCookie , BOOL __RPC_FAR *pfAllow ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetHyphenation(
                DWORD dwSiteCookie , long __RPC_FAR *pfHyphenate , float __RPC_FAR *pptsHyphenationZone ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetScrollPosition(
                long __RPC_FAR *pdxpScroll , long __RPC_FAR *pdypScroll ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetVisible(
                long __RPC_FAR *pfVisible ) = 0;
        virtual void  STDMETHODCALLTYPE OnSelectionChanged(
                DWORD dwSiteCookie , long qselchg ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE SetCrs(
                DWORD dwSiteCookie , long dgc , float degs ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetVisibleRect(
                DWORD dwSiteCookie , RECT __RPC_FAR *prect ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE EnsureRectVisible(
                DWORD dwSiteCookie , RECT __RPC_FAR *prcBound , RECT __RPC_FAR *prcFrame , long scrHorz , long scrVert ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE ScrollForMouseDrag(
                DWORD dwSiteCookie , POINT pt ) = 0;
        virtual BOOL  STDMETHODCALLTYPE HasFocus(
                DWORD dwSiteCookie ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE RequestResize(
                DWORD dwSiteCookie , float ptsNewWidth , float ptsNewHeight ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetHdcRef(
                DWORD dwSiteCookie , HDC __RPC_FAR *phdcRef ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE FreeHdcRef(
                DWORD dwSiteCookie , HDC hdcRef ) = 0;
        virtual BOOL  STDMETHODCALLTYPE UpdateWindow(
                DWORD dwSiteCookie ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE CursorOut(
                DWORD dwSiteCookie , POINT pt , long qcmdPhysical , long qcmdLogical ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE EventNotify(
                DWORD dwSiteCookie , long qevt , long evtLong1 , long evtLong2 ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE CalcNestedSizeAtCp(
                DWORD dwSiteCookie , DWORD dwCookieTP , long cp , long fUnderLayout , long xWidthMax , SIZE __RPC_FAR *psize ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetAbsoluteRenderPosition(
                DWORD dwSiteCookie , DWORD dwCookieTP , long cp , long __RPC_FAR *pdxpLeft , long __RPC_FAR *pdypTop ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE DrawNestedLayoutAtCp(
                DWORD dwSiteCookie , DWORD dwCookieTP , long cp ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE PositionNestedLayout(
                DWORD dwSiteCookie , DWORD dwCookieTP , long cp , RECT __RPC_FAR *prc ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE AppendRectHelper(
                DWORD dwSiteCookie , void __RPC_FAR *pvaryRects , RECT __RPC_FAR *prcBound , RECT __RPC_FAR *prcLine , long xOffset , long yOffset , long cp , long cpClipStart , long cpClipFinish ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE InitDisplayTreeForPositioning(
                DWORD dwSiteCookie , long cpStart ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE PositionLayoutDispNode(
                DWORD dwSiteCookie , long cpObject , long dx , long dy ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE DrawBackgroundAndBorder(
                DWORD dwSiteCookie , RECT __RPC_FAR *prcClip , long cpStart , long cpLim ) = 0;
    };

#ifdef DEBUG

#define CheckITextLayoutSiteMembers(klass)\
	struct _##klass##_ITextLayoutSite_GetLayoutSize\
		{\
		_##klass##_ITextLayoutSite_GetLayoutSize(HRESULT  (##klass##::*pfn)(DWORD , int , long __RPC_FAR *, long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutSite_GetPadding\
		{\
		_##klass##_ITextLayoutSite_GetPadding(HRESULT  (##klass##::*pfn)(DWORD , int , long __RPC_FAR *, long __RPC_FAR *, long __RPC_FAR *, long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutSite_GetColumns\
		{\
		_##klass##_ITextLayoutSite_GetColumns(HRESULT  (##klass##::*pfn)(DWORD , int __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutSite_GetConstraintPoints\
		{\
		_##klass##_ITextLayoutSite_GetConstraintPoints(HRESULT  (##klass##::*pfn)(DWORD , ITextConstraintPoints __RPC_FAR *__RPC_FAR *, DWORD __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutSite_GetDisplayRect\
		{\
		_##klass##_ITextLayoutSite_GetDisplayRect(HRESULT  (##klass##::*pfn)(DWORD , RECT __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutSite_GetScale\
		{\
		_##klass##_ITextLayoutSite_GetScale(HRESULT  (##klass##::*pfn)(DWORD , long __RPC_FAR *, long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutSite_GetBkgndMode\
		{\
		_##klass##_ITextLayoutSite_GetBkgndMode(HRESULT  (##klass##::*pfn)(DWORD , long __RPC_FAR *, COLORREF __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutSite_GetRotation\
		{\
		_##klass##_ITextLayoutSite_GetRotation(HRESULT  (##klass##::*pfn)(DWORD , float __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutSite_GetTextFlow\
		{\
		_##klass##_ITextLayoutSite_GetTextFlow(HRESULT  (##klass##::*pfn)(DWORD , long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutSite_GetVerticalAlign\
		{\
		_##klass##_ITextLayoutSite_GetVerticalAlign(HRESULT  (##klass##::*pfn)(DWORD , long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutSite_GetAllowSplitWordsAcrossConstraints\
		{\
		_##klass##_ITextLayoutSite_GetAllowSplitWordsAcrossConstraints(HRESULT  (##klass##::*pfn)(DWORD , BOOL __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutSite_GetHyphenation\
		{\
		_##klass##_ITextLayoutSite_GetHyphenation(HRESULT  (##klass##::*pfn)(DWORD , long __RPC_FAR *, float __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutSite_GetScrollPosition\
		{\
		_##klass##_ITextLayoutSite_GetScrollPosition(HRESULT  (##klass##::*pfn)(long __RPC_FAR *, long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutSite_GetVisible\
		{\
		_##klass##_ITextLayoutSite_GetVisible(HRESULT  (##klass##::*pfn)(long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutSite_OnSelectionChanged\
		{\
		_##klass##_ITextLayoutSite_OnSelectionChanged(void  (##klass##::*pfn)(DWORD , long )){}\
		};\
	struct _##klass##_ITextLayoutSite_SetCrs\
		{\
		_##klass##_ITextLayoutSite_SetCrs(HRESULT  (##klass##::*pfn)(DWORD , long , float )){}\
		};\
	struct _##klass##_ITextLayoutSite_GetVisibleRect\
		{\
		_##klass##_ITextLayoutSite_GetVisibleRect(HRESULT  (##klass##::*pfn)(DWORD , RECT __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutSite_EnsureRectVisible\
		{\
		_##klass##_ITextLayoutSite_EnsureRectVisible(HRESULT  (##klass##::*pfn)(DWORD , RECT __RPC_FAR *, RECT __RPC_FAR *, long , long )){}\
		};\
	struct _##klass##_ITextLayoutSite_ScrollForMouseDrag\
		{\
		_##klass##_ITextLayoutSite_ScrollForMouseDrag(HRESULT  (##klass##::*pfn)(DWORD , POINT )){}\
		};\
	struct _##klass##_ITextLayoutSite_HasFocus\
		{\
		_##klass##_ITextLayoutSite_HasFocus(BOOL  (##klass##::*pfn)(DWORD )){}\
		};\
	struct _##klass##_ITextLayoutSite_RequestResize\
		{\
		_##klass##_ITextLayoutSite_RequestResize(HRESULT  (##klass##::*pfn)(DWORD , float , float )){}\
		};\
	struct _##klass##_ITextLayoutSite_GetHdcRef\
		{\
		_##klass##_ITextLayoutSite_GetHdcRef(HRESULT  (##klass##::*pfn)(DWORD , HDC __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutSite_FreeHdcRef\
		{\
		_##klass##_ITextLayoutSite_FreeHdcRef(HRESULT  (##klass##::*pfn)(DWORD , HDC )){}\
		};\
	struct _##klass##_ITextLayoutSite_UpdateWindow\
		{\
		_##klass##_ITextLayoutSite_UpdateWindow(BOOL  (##klass##::*pfn)(DWORD )){}\
		};\
	struct _##klass##_ITextLayoutSite_CursorOut\
		{\
		_##klass##_ITextLayoutSite_CursorOut(HRESULT  (##klass##::*pfn)(DWORD , POINT , long , long )){}\
		};\
	struct _##klass##_ITextLayoutSite_EventNotify\
		{\
		_##klass##_ITextLayoutSite_EventNotify(HRESULT  (##klass##::*pfn)(DWORD , long , long , long )){}\
		};\
	struct _##klass##_ITextLayoutSite_CalcNestedSizeAtCp\
		{\
		_##klass##_ITextLayoutSite_CalcNestedSizeAtCp(HRESULT  (##klass##::*pfn)(DWORD , DWORD , long , long , long , SIZE __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutSite_GetAbsoluteRenderPosition\
		{\
		_##klass##_ITextLayoutSite_GetAbsoluteRenderPosition(HRESULT  (##klass##::*pfn)(DWORD , DWORD , long , long __RPC_FAR *, long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutSite_DrawNestedLayoutAtCp\
		{\
		_##klass##_ITextLayoutSite_DrawNestedLayoutAtCp(HRESULT  (##klass##::*pfn)(DWORD , DWORD , long )){}\
		};\
	struct _##klass##_ITextLayoutSite_PositionNestedLayout\
		{\
		_##klass##_ITextLayoutSite_PositionNestedLayout(HRESULT  (##klass##::*pfn)(DWORD , DWORD , long , RECT __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextLayoutSite_AppendRectHelper\
		{\
		_##klass##_ITextLayoutSite_AppendRectHelper(HRESULT  (##klass##::*pfn)(DWORD , void __RPC_FAR *, RECT __RPC_FAR *, RECT __RPC_FAR *, long , long , long , long , long )){}\
		};\
	struct _##klass##_ITextLayoutSite_InitDisplayTreeForPositioning\
		{\
		_##klass##_ITextLayoutSite_InitDisplayTreeForPositioning(HRESULT  (##klass##::*pfn)(DWORD , long )){}\
		};\
	struct _##klass##_ITextLayoutSite_PositionLayoutDispNode\
		{\
		_##klass##_ITextLayoutSite_PositionLayoutDispNode(HRESULT  (##klass##::*pfn)(DWORD , long , long , long )){}\
		};\
	struct _##klass##_ITextLayoutSite_DrawBackgroundAndBorder\
		{\
		_##klass##_ITextLayoutSite_DrawBackgroundAndBorder(HRESULT  (##klass##::*pfn)(DWORD , RECT __RPC_FAR *, long , long )){}\
		};\
	void klass::VerifyITextLayoutSite(){\
	_##klass##_ITextLayoutSite_GetLayoutSize pfn1(GetLayoutSize);\
	_##klass##_ITextLayoutSite_GetPadding pfn2(GetPadding);\
	_##klass##_ITextLayoutSite_GetColumns pfn3(GetColumns);\
	_##klass##_ITextLayoutSite_GetConstraintPoints pfn4(GetConstraintPoints);\
	_##klass##_ITextLayoutSite_GetDisplayRect pfn5(GetDisplayRect);\
	_##klass##_ITextLayoutSite_GetScale pfn6(GetScale);\
	_##klass##_ITextLayoutSite_GetBkgndMode pfn7(GetBkgndMode);\
	_##klass##_ITextLayoutSite_GetRotation pfn8(GetRotation);\
	_##klass##_ITextLayoutSite_GetTextFlow pfn9(GetTextFlow);\
	_##klass##_ITextLayoutSite_GetVerticalAlign pfn10(GetVerticalAlign);\
	_##klass##_ITextLayoutSite_GetAllowSplitWordsAcrossConstraints pfn11(GetAllowSplitWordsAcrossConstraints);\
	_##klass##_ITextLayoutSite_GetHyphenation pfn12(GetHyphenation);\
	_##klass##_ITextLayoutSite_GetScrollPosition pfn13(GetScrollPosition);\
	_##klass##_ITextLayoutSite_GetVisible pfn14(GetVisible);\
	_##klass##_ITextLayoutSite_OnSelectionChanged pfn15(OnSelectionChanged);\
	_##klass##_ITextLayoutSite_SetCrs pfn16(SetCrs);\
	_##klass##_ITextLayoutSite_GetVisibleRect pfn17(GetVisibleRect);\
	_##klass##_ITextLayoutSite_EnsureRectVisible pfn18(EnsureRectVisible);\
	_##klass##_ITextLayoutSite_ScrollForMouseDrag pfn19(ScrollForMouseDrag);\
	_##klass##_ITextLayoutSite_HasFocus pfn20(HasFocus);\
	_##klass##_ITextLayoutSite_RequestResize pfn21(RequestResize);\
	_##klass##_ITextLayoutSite_GetHdcRef pfn22(GetHdcRef);\
	_##klass##_ITextLayoutSite_FreeHdcRef pfn23(FreeHdcRef);\
	_##klass##_ITextLayoutSite_UpdateWindow pfn24(UpdateWindow);\
	_##klass##_ITextLayoutSite_CursorOut pfn25(CursorOut);\
	_##klass##_ITextLayoutSite_EventNotify pfn26(EventNotify);\
	_##klass##_ITextLayoutSite_CalcNestedSizeAtCp pfn27(CalcNestedSizeAtCp);\
	_##klass##_ITextLayoutSite_GetAbsoluteRenderPosition pfn28(GetAbsoluteRenderPosition);\
	_##klass##_ITextLayoutSite_DrawNestedLayoutAtCp pfn29(DrawNestedLayoutAtCp);\
	_##klass##_ITextLayoutSite_PositionNestedLayout pfn30(PositionNestedLayout);\
	_##klass##_ITextLayoutSite_AppendRectHelper pfn31(AppendRectHelper);\
	_##klass##_ITextLayoutSite_InitDisplayTreeForPositioning pfn32(InitDisplayTreeForPositioning);\
	_##klass##_ITextLayoutSite_PositionLayoutDispNode pfn33(PositionLayoutDispNode);\
	_##klass##_ITextLayoutSite_DrawBackgroundAndBorder pfn34(DrawBackgroundAndBorder);\
	}\

#else

#define CheckITextLayoutSiteMembers(klass)

#endif /* DEBUG */

#define idITextLayoutSite	0x7f67baa7

#ifdef DeclareSmartPointer
#ifndef ITextLayoutSiteSPMethodsDefined
extern "C++" { TEMPLATECLASSB class ITextLayoutSiteSPMethods : public TEMPLATEBASEB(IUnknownSPMethods) { }; }
#define ITextLayoutSiteSPMethodsDefined
#endif
DeclareSmartPointer(ITextLayoutSite)
#define SPITextLayoutSite auto SP_ITextLayoutSite
#endif

#define DeclareITextLayoutSiteMethods()\
	(FNOBJECT) DrawBackgroundAndBorder,\
	(FNOBJECT) PositionLayoutDispNode,\
	(FNOBJECT) InitDisplayTreeForPositioning,\
	(FNOBJECT) AppendRectHelper,\
	(FNOBJECT) PositionNestedLayout,\
	(FNOBJECT) DrawNestedLayoutAtCp,\
	(FNOBJECT) GetAbsoluteRenderPosition,\
	(FNOBJECT) CalcNestedSizeAtCp,\
	(FNOBJECT) EventNotify,\
	(FNOBJECT) CursorOut,\
	(FNOBJECT) UpdateWindow,\
	(FNOBJECT) FreeHdcRef,\
	(FNOBJECT) GetHdcRef,\
	(FNOBJECT) RequestResize,\
	(FNOBJECT) HasFocus,\
	(FNOBJECT) ScrollForMouseDrag,\
	(FNOBJECT) EnsureRectVisible,\
	(FNOBJECT) GetVisibleRect,\
	(FNOBJECT) SetCrs,\
	(FNOBJECT) OnSelectionChanged,\
	(FNOBJECT) GetVisible,\
	(FNOBJECT) GetScrollPosition,\
	(FNOBJECT) GetHyphenation,\
	(FNOBJECT) GetAllowSplitWordsAcrossConstraints,\
	(FNOBJECT) GetVerticalAlign,\
	(FNOBJECT) GetTextFlow,\
	(FNOBJECT) GetRotation,\
	(FNOBJECT) GetBkgndMode,\
	(FNOBJECT) GetScale,\
	(FNOBJECT) GetDisplayRect,\
	(FNOBJECT) GetConstraintPoints,\
	(FNOBJECT) GetColumns,\
	(FNOBJECT) GetPadding,\
	(FNOBJECT) GetLayoutSize,\

#define DeclareITextLayoutSiteVtbl()\
	,_Method1\
	,_Method2\
	,_Method3\
	,_Method4\
	,_Method5\
	,_Method6\
	,_Method7\
	,_Method8\
	,_Method9\
	,_Method10\
	,_Method11\
	,_Method12\
	,_Method13\
	,_Method14\
	,_Method15\
	,_Method16\
	,_Method17\
	,_Method18\
	,_Method19\
	,_Method20\
	,_Method21\
	,_Method22\
	,_Method23\
	,_Method24\
	,_Method25\
	,_Method26\
	,_Method27\
	,_Method28\
	,_Method29\
	,_Method30\
	,_Method31\
	,_Method32\
	,_Method33\
	,_Method34\





EXTERN_C const IID IID_ITextInPlacePaint;

  interface  ITextInPlacePaint : public IUnknown
    {
    public:
        virtual HRESULT  STDMETHODCALLTYPE BeginIncrementalPaint(
                DWORD dwSiteCookie , long tc , RECT __RPC_FAR *prcMax , DWORD dwFlags , HDC __RPC_FAR *phdcRef , POINT __RPC_FAR *rgptRotate ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE EndIncrementalPaint(
                DWORD dwSiteCookie , long tc ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE BeginRectPaint(
                DWORD dwSiteCookie , long tc , RECT __RPC_FAR *prc , DWORD dwFlags , long SurfaceKind , HDC __RPC_FAR *phdc , POINT __RPC_FAR *rgptRotate ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE EndRectPaint(
                DWORD dwSiteCookie , long tc , HDC __RPC_FAR *phdc ) = 0;
        virtual BOOL  STDMETHODCALLTYPE IsRectObscured(
                DWORD dwSiteCookie , long tc , RECT __RPC_FAR *prc , POINT __RPC_FAR *rgptRotate ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE InvalidateRect(
                DWORD dwSiteCookie , long tc , RECT __RPC_FAR *prc , DWORD dwFlags , POINT __RPC_FAR *rgptRotate ) = 0;
    };

#ifdef DEBUG

#define CheckITextInPlacePaintMembers(klass)\
	struct _##klass##_ITextInPlacePaint_BeginIncrementalPaint\
		{\
		_##klass##_ITextInPlacePaint_BeginIncrementalPaint(HRESULT  (##klass##::*pfn)(DWORD , long , RECT __RPC_FAR *, DWORD , HDC __RPC_FAR *, POINT __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextInPlacePaint_EndIncrementalPaint\
		{\
		_##klass##_ITextInPlacePaint_EndIncrementalPaint(HRESULT  (##klass##::*pfn)(DWORD , long )){}\
		};\
	struct _##klass##_ITextInPlacePaint_BeginRectPaint\
		{\
		_##klass##_ITextInPlacePaint_BeginRectPaint(HRESULT  (##klass##::*pfn)(DWORD , long , RECT __RPC_FAR *, DWORD , long , HDC __RPC_FAR *, POINT __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextInPlacePaint_EndRectPaint\
		{\
		_##klass##_ITextInPlacePaint_EndRectPaint(HRESULT  (##klass##::*pfn)(DWORD , long , HDC __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextInPlacePaint_IsRectObscured\
		{\
		_##klass##_ITextInPlacePaint_IsRectObscured(BOOL  (##klass##::*pfn)(DWORD , long , RECT __RPC_FAR *, POINT __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextInPlacePaint_InvalidateRect\
		{\
		_##klass##_ITextInPlacePaint_InvalidateRect(HRESULT  (##klass##::*pfn)(DWORD , long , RECT __RPC_FAR *, DWORD , POINT __RPC_FAR *)){}\
		};\
	void klass::VerifyITextInPlacePaint(){\
	_##klass##_ITextInPlacePaint_BeginIncrementalPaint pfn1(BeginIncrementalPaint);\
	_##klass##_ITextInPlacePaint_EndIncrementalPaint pfn2(EndIncrementalPaint);\
	_##klass##_ITextInPlacePaint_BeginRectPaint pfn3(BeginRectPaint);\
	_##klass##_ITextInPlacePaint_EndRectPaint pfn4(EndRectPaint);\
	_##klass##_ITextInPlacePaint_IsRectObscured pfn5(IsRectObscured);\
	_##klass##_ITextInPlacePaint_InvalidateRect pfn6(InvalidateRect);\
	}\

#else

#define CheckITextInPlacePaintMembers(klass)

#endif /* DEBUG */

#define idITextInPlacePaint	0x7f67baa8

#ifdef DeclareSmartPointer
#ifndef ITextInPlacePaintSPMethodsDefined
extern "C++" { TEMPLATECLASSB class ITextInPlacePaintSPMethods : public TEMPLATEBASEB(IUnknownSPMethods) { }; }
#define ITextInPlacePaintSPMethodsDefined
#endif
DeclareSmartPointer(ITextInPlacePaint)
#define SPITextInPlacePaint auto SP_ITextInPlacePaint
#endif

#define DeclareITextInPlacePaintMethods()\
	(FNOBJECT) InvalidateRect,\
	(FNOBJECT) IsRectObscured,\
	(FNOBJECT) EndRectPaint,\
	(FNOBJECT) BeginRectPaint,\
	(FNOBJECT) EndIncrementalPaint,\
	(FNOBJECT) BeginIncrementalPaint,\

#define DeclareITextInPlacePaintVtbl()\
	,_Method1\
	,_Method2\
	,_Method3\
	,_Method4\
	,_Method5\
	,_Method6\





EXTERN_C const IID IID_ITextConstraintPoints;

  interface  ITextConstraintPoints : public IUnknown
    {
    public:
        virtual long  STDMETHODCALLTYPE GetCount(
                DWORD dwConsCookie ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetPolyCount(
                DWORD dwConsCookie , long iPoly , long __RPC_FAR *pCount ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetPoints(
                DWORD dwConsCookie , long iPoly , POINT __RPC_FAR *__RPC_FAR *ppPoints ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetMinimumWrapDistance(
                DWORD dwConsCookie , float __RPC_FAR *pptsLeftAway , float __RPC_FAR *pptsRightAway ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetConstraintFlags(
                DWORD dwConsCookie , DWORD __RPC_FAR *__RPC_FAR *ppconsFlags ) = 0;
    };

#ifdef DEBUG

#define CheckITextConstraintPointsMembers(klass)\
	struct _##klass##_ITextConstraintPoints_GetCount\
		{\
		_##klass##_ITextConstraintPoints_GetCount(long  (##klass##::*pfn)(DWORD )){}\
		};\
	struct _##klass##_ITextConstraintPoints_GetPolyCount\
		{\
		_##klass##_ITextConstraintPoints_GetPolyCount(HRESULT  (##klass##::*pfn)(DWORD , long , long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextConstraintPoints_GetPoints\
		{\
		_##klass##_ITextConstraintPoints_GetPoints(HRESULT  (##klass##::*pfn)(DWORD , long , POINT __RPC_FAR *__RPC_FAR *)){}\
		};\
	struct _##klass##_ITextConstraintPoints_GetMinimumWrapDistance\
		{\
		_##klass##_ITextConstraintPoints_GetMinimumWrapDistance(HRESULT  (##klass##::*pfn)(DWORD , float __RPC_FAR *, float __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextConstraintPoints_GetConstraintFlags\
		{\
		_##klass##_ITextConstraintPoints_GetConstraintFlags(HRESULT  (##klass##::*pfn)(DWORD , DWORD __RPC_FAR *__RPC_FAR *)){}\
		};\
	void klass::VerifyITextConstraintPoints(){\
	_##klass##_ITextConstraintPoints_GetCount pfn1(GetCount);\
	_##klass##_ITextConstraintPoints_GetPolyCount pfn2(GetPolyCount);\
	_##klass##_ITextConstraintPoints_GetPoints pfn3(GetPoints);\
	_##klass##_ITextConstraintPoints_GetMinimumWrapDistance pfn4(GetMinimumWrapDistance);\
	_##klass##_ITextConstraintPoints_GetConstraintFlags pfn5(GetConstraintFlags);\
	}\

#else

#define CheckITextConstraintPointsMembers(klass)

#endif /* DEBUG */

#define idITextConstraintPoints	0x7f67baa9

#ifdef DeclareSmartPointer
#ifndef ITextConstraintPointsSPMethodsDefined
extern "C++" { TEMPLATECLASSB class ITextConstraintPointsSPMethods : public TEMPLATEBASEB(IUnknownSPMethods) { }; }
#define ITextConstraintPointsSPMethodsDefined
#endif
DeclareSmartPointer(ITextConstraintPoints)
#define SPITextConstraintPoints auto SP_ITextConstraintPoints
#endif

#define DeclareITextConstraintPointsMethods()\
	(FNOBJECT) GetConstraintFlags,\
	(FNOBJECT) GetMinimumWrapDistance,\
	(FNOBJECT) GetPoints,\
	(FNOBJECT) GetPolyCount,\
	(FNOBJECT) GetCount,\

#define DeclareITextConstraintPointsVtbl()\
	,_Method1\
	,_Method2\
	,_Method3\
	,_Method4\
	,_Method5\





EXTERN_C const IID IID_ITextChangeNotification;

  interface  ITextChangeNotification : public IUnknown
    {
    public:
        virtual long  STDMETHODCALLTYPE Type(
                void ) = 0;
        virtual BOOL  STDMETHODCALLTYPE IsType(
                long ntype ) = 0;
        virtual long  STDMETHODCALLTYPE AntiType(
                void ) = 0;
        virtual BOOL  STDMETHODCALLTYPE IsTextChange(
                void ) = 0;
        virtual BOOL  STDMETHODCALLTYPE IsTreeChange(
                void ) = 0;
        virtual BOOL  STDMETHODCALLTYPE IsLayoutChange(
                void ) = 0;
        virtual BOOL  STDMETHODCALLTYPE IsForOleSites(
                void ) = 0;
        virtual BOOL  STDMETHODCALLTYPE IsForAllElements(
                void ) = 0;
        virtual BOOL  STDMETHODCALLTYPE IsFlagSet(
                long f ) = 0;
        virtual DWORD  STDMETHODCALLTYPE LayoutFlags(
                void ) = 0;
        virtual void  STDMETHODCALLTYPE Data(
                long __RPC_FAR *pl ) = 0;
        virtual void __RPC_FAR  * STDMETHODCALLTYPE Node(
                void ) = 0;
        virtual void __RPC_FAR  * STDMETHODCALLTYPE Element(
                void ) = 0;
        virtual void __RPC_FAR  * STDMETHODCALLTYPE Handler(
                void ) = 0;
        virtual long  STDMETHODCALLTYPE SI(
                void ) = 0;
        virtual long  STDMETHODCALLTYPE CElements(
                void ) = 0;
        virtual long  STDMETHODCALLTYPE ElementsChanged(
                void ) = 0;
        virtual long  STDMETHODCALLTYPE Cp(
                void ) = 0;
        virtual long  STDMETHODCALLTYPE Cch(
                void ) = 0;
        virtual long  STDMETHODCALLTYPE CchChanged(
                void ) = 0;
        virtual long  STDMETHODCALLTYPE Run(
                void ) = 0;
        virtual long  STDMETHODCALLTYPE NRuns(
                void ) = 0;
        virtual long  STDMETHODCALLTYPE Ich(
                void ) = 0;
        virtual DWORD  STDMETHODCALLTYPE SerialNumber(
                void ) = 0;
        virtual BOOL  STDMETHODCALLTYPE IsReceived(
                DWORD sn ) = 0;
    };

#ifdef DEBUG

#define CheckITextChangeNotificationMembers(klass)\
	struct _##klass##_ITextChangeNotification_Type\
		{\
		_##klass##_ITextChangeNotification_Type(long  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_IsType\
		{\
		_##klass##_ITextChangeNotification_IsType(BOOL  (##klass##::*pfn)(long )){}\
		};\
	struct _##klass##_ITextChangeNotification_AntiType\
		{\
		_##klass##_ITextChangeNotification_AntiType(long  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_IsTextChange\
		{\
		_##klass##_ITextChangeNotification_IsTextChange(BOOL  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_IsTreeChange\
		{\
		_##klass##_ITextChangeNotification_IsTreeChange(BOOL  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_IsLayoutChange\
		{\
		_##klass##_ITextChangeNotification_IsLayoutChange(BOOL  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_IsForOleSites\
		{\
		_##klass##_ITextChangeNotification_IsForOleSites(BOOL  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_IsForAllElements\
		{\
		_##klass##_ITextChangeNotification_IsForAllElements(BOOL  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_IsFlagSet\
		{\
		_##klass##_ITextChangeNotification_IsFlagSet(BOOL  (##klass##::*pfn)(long )){}\
		};\
	struct _##klass##_ITextChangeNotification_LayoutFlags\
		{\
		_##klass##_ITextChangeNotification_LayoutFlags(DWORD  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_Data\
		{\
		_##klass##_ITextChangeNotification_Data(void  (##klass##::*pfn)(long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITextChangeNotification_Node\
		{\
		_##klass##_ITextChangeNotification_Node(void __RPC_FAR  * (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_Element\
		{\
		_##klass##_ITextChangeNotification_Element(void __RPC_FAR  * (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_Handler\
		{\
		_##klass##_ITextChangeNotification_Handler(void __RPC_FAR  * (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_SI\
		{\
		_##klass##_ITextChangeNotification_SI(long  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_CElements\
		{\
		_##klass##_ITextChangeNotification_CElements(long  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_ElementsChanged\
		{\
		_##klass##_ITextChangeNotification_ElementsChanged(long  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_Cp\
		{\
		_##klass##_ITextChangeNotification_Cp(long  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_Cch\
		{\
		_##klass##_ITextChangeNotification_Cch(long  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_CchChanged\
		{\
		_##klass##_ITextChangeNotification_CchChanged(long  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_Run\
		{\
		_##klass##_ITextChangeNotification_Run(long  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_NRuns\
		{\
		_##klass##_ITextChangeNotification_NRuns(long  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_Ich\
		{\
		_##klass##_ITextChangeNotification_Ich(long  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_SerialNumber\
		{\
		_##klass##_ITextChangeNotification_SerialNumber(DWORD  (##klass##::*pfn)(void )){}\
		};\
	struct _##klass##_ITextChangeNotification_IsReceived\
		{\
		_##klass##_ITextChangeNotification_IsReceived(BOOL  (##klass##::*pfn)(DWORD )){}\
		};\
	void klass::VerifyITextChangeNotification(){\
	_##klass##_ITextChangeNotification_Type pfn1(Type);\
	_##klass##_ITextChangeNotification_IsType pfn2(IsType);\
	_##klass##_ITextChangeNotification_AntiType pfn3(AntiType);\
	_##klass##_ITextChangeNotification_IsTextChange pfn4(IsTextChange);\
	_##klass##_ITextChangeNotification_IsTreeChange pfn5(IsTreeChange);\
	_##klass##_ITextChangeNotification_IsLayoutChange pfn6(IsLayoutChange);\
	_##klass##_ITextChangeNotification_IsForOleSites pfn7(IsForOleSites);\
	_##klass##_ITextChangeNotification_IsForAllElements pfn8(IsForAllElements);\
	_##klass##_ITextChangeNotification_IsFlagSet pfn9(IsFlagSet);\
	_##klass##_ITextChangeNotification_LayoutFlags pfn10(LayoutFlags);\
	_##klass##_ITextChangeNotification_Data pfn11(Data);\
	_##klass##_ITextChangeNotification_Node pfn12(Node);\
	_##klass##_ITextChangeNotification_Element pfn13(Element);\
	_##klass##_ITextChangeNotification_Handler pfn14(Handler);\
	_##klass##_ITextChangeNotification_SI pfn15(SI);\
	_##klass##_ITextChangeNotification_CElements pfn16(CElements);\
	_##klass##_ITextChangeNotification_ElementsChanged pfn17(ElementsChanged);\
	_##klass##_ITextChangeNotification_Cp pfn18(Cp);\
	_##klass##_ITextChangeNotification_Cch pfn19(Cch);\
	_##klass##_ITextChangeNotification_CchChanged pfn20(CchChanged);\
	_##klass##_ITextChangeNotification_Run pfn21(Run);\
	_##klass##_ITextChangeNotification_NRuns pfn22(NRuns);\
	_##klass##_ITextChangeNotification_Ich pfn23(Ich);\
	_##klass##_ITextChangeNotification_SerialNumber pfn24(SerialNumber);\
	_##klass##_ITextChangeNotification_IsReceived pfn25(IsReceived);\
	}\

#else

#define CheckITextChangeNotificationMembers(klass)

#endif /* DEBUG */

#define idITextChangeNotification	0xe63507ca

#ifdef DeclareSmartPointer
#ifndef ITextChangeNotificationSPMethodsDefined
extern "C++" { TEMPLATECLASSB class ITextChangeNotificationSPMethods : public TEMPLATEBASEB(IUnknownSPMethods) { }; }
#define ITextChangeNotificationSPMethodsDefined
#endif
DeclareSmartPointer(ITextChangeNotification)
#define SPITextChangeNotification auto SP_ITextChangeNotification
#endif

#define DeclareITextChangeNotificationMethods()\
	(FNOBJECT) IsReceived,\
	(FNOBJECT) SerialNumber,\
	(FNOBJECT) Ich,\
	(FNOBJECT) NRuns,\
	(FNOBJECT) Run,\
	(FNOBJECT) CchChanged,\
	(FNOBJECT) Cch,\
	(FNOBJECT) Cp,\
	(FNOBJECT) ElementsChanged,\
	(FNOBJECT) CElements,\
	(FNOBJECT) SI,\
	(FNOBJECT) Handler,\
	(FNOBJECT) Element,\
	(FNOBJECT) Node,\
	(FNOBJECT) Data,\
	(FNOBJECT) LayoutFlags,\
	(FNOBJECT) IsFlagSet,\
	(FNOBJECT) IsForAllElements,\
	(FNOBJECT) IsForOleSites,\
	(FNOBJECT) IsLayoutChange,\
	(FNOBJECT) IsTreeChange,\
	(FNOBJECT) IsTextChange,\
	(FNOBJECT) AntiType,\
	(FNOBJECT) IsType,\
	(FNOBJECT) Type,\

#define DeclareITextChangeNotificationVtbl()\
	,_Method1\
	,_Method2\
	,_Method3\
	,_Method4\
	,_Method5\
	,_Method6\
	,_Method7\
	,_Method8\
	,_Method9\
	,_Method10\
	,_Method11\
	,_Method12\
	,_Method13\
	,_Method14\
	,_Method15\
	,_Method16\
	,_Method17\
	,_Method18\
	,_Method19\
	,_Method20\
	,_Method21\
	,_Method22\
	,_Method23\
	,_Method24\
	,_Method25\





EXTERN_C const IID IID_ILineServicesHost;

  interface  ILineServicesHost : public IUnknown
    {
    public:
        virtual HRESULT  STDMETHODCALLTYPE GetTreePointer(
                long cpRel , DWORD __RPC_FAR *pdwCookieTP , long __RPC_FAR *pcchOffsetOfCpRelFromTPStart ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE FreeTreePointer(
                DWORD dwCookieTP ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE SetTreePointerCp(
                DWORD dwCookieTP , long cpRel , long __RPC_FAR *pcchOffsetOfCpRelFromTPStart ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetTreePointerCch(
                DWORD dwCookieTP , long __RPC_FAR *pcch ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE MoveToNextTreePos(
                DWORD dwCookieTP ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE IsNestedLayoutAtTreePos(
                DWORD dwCookieTP ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE AdvanceBeforeLine(
                DWORD dwCookieTP , long cpRelOrigLineStart , long __RPC_FAR *pcchSkip ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE AdvanceAfterLine(
                DWORD dwCookieTP , DWORD dwCookieLC , long cpRelOrigLineLim , long __RPC_FAR *pcchSkip ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE HiddenBeforeAlignedObj(
                DWORD dwCookieTP , long cpRelOrigLineLim , long __RPC_FAR *pcchSkip ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetLineContext(
                DWORD __RPC_FAR *pdwCookie ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE FreeLineContext(
                DWORD dwCookie ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE SetContext(
                DWORD dwCookie ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE ClearContext(
                DWORD dwCookie ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE Setup(
                DWORD dwCookie , long dxtMaxWidth , long cpStart , BOOL fMinMaxPass ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE CpRelFromCpLs(
                DWORD dwCookie , long cpLs , long __RPC_FAR *pcpRel ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE CpLsFromCpRel(
                DWORD dwCookie , long cpRel , long __RPC_FAR *pcpLs ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE DiscardLine(
                DWORD dwCookie ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE FetchHostRun(
                DWORD dwCookie , long cpRelLs , LPCWSTR __RPC_FAR *ppwchRun , DWORD __RPC_FAR *pcchRun , BOOL __RPC_FAR *pfHidden , void __RPC_FAR *plsChp , long propertyGroup , ITextPropertyList __RPC_FAR *__RPC_FAR *pptpl , void __RPC_FAR *__RPC_FAR *ppvHostRun , DWORD __RPC_FAR *pdwFlags ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE TerminateLineAfterRun(
                DWORD dwCookie , void __RPC_FAR *pvHostRun ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetLineSummaryInfo(
                DWORD dwCookie , long cpRelLineLim , DWORD __RPC_FAR *pdwFlags ) = 0;
    };

#ifdef DEBUG

#define CheckILineServicesHostMembers(klass)\
	struct _##klass##_ILineServicesHost_GetTreePointer\
		{\
		_##klass##_ILineServicesHost_GetTreePointer(HRESULT  (##klass##::*pfn)(long , DWORD __RPC_FAR *, long __RPC_FAR *)){}\
		};\
	struct _##klass##_ILineServicesHost_FreeTreePointer\
		{\
		_##klass##_ILineServicesHost_FreeTreePointer(HRESULT  (##klass##::*pfn)(DWORD )){}\
		};\
	struct _##klass##_ILineServicesHost_SetTreePointerCp\
		{\
		_##klass##_ILineServicesHost_SetTreePointerCp(HRESULT  (##klass##::*pfn)(DWORD , long , long __RPC_FAR *)){}\
		};\
	struct _##klass##_ILineServicesHost_GetTreePointerCch\
		{\
		_##klass##_ILineServicesHost_GetTreePointerCch(HRESULT  (##klass##::*pfn)(DWORD , long __RPC_FAR *)){}\
		};\
	struct _##klass##_ILineServicesHost_MoveToNextTreePos\
		{\
		_##klass##_ILineServicesHost_MoveToNextTreePos(HRESULT  (##klass##::*pfn)(DWORD )){}\
		};\
	struct _##klass##_ILineServicesHost_IsNestedLayoutAtTreePos\
		{\
		_##klass##_ILineServicesHost_IsNestedLayoutAtTreePos(HRESULT  (##klass##::*pfn)(DWORD )){}\
		};\
	struct _##klass##_ILineServicesHost_AdvanceBeforeLine\
		{\
		_##klass##_ILineServicesHost_AdvanceBeforeLine(HRESULT  (##klass##::*pfn)(DWORD , long , long __RPC_FAR *)){}\
		};\
	struct _##klass##_ILineServicesHost_AdvanceAfterLine\
		{\
		_##klass##_ILineServicesHost_AdvanceAfterLine(HRESULT  (##klass##::*pfn)(DWORD , DWORD , long , long __RPC_FAR *)){}\
		};\
	struct _##klass##_ILineServicesHost_HiddenBeforeAlignedObj\
		{\
		_##klass##_ILineServicesHost_HiddenBeforeAlignedObj(HRESULT  (##klass##::*pfn)(DWORD , long , long __RPC_FAR *)){}\
		};\
	struct _##klass##_ILineServicesHost_GetLineContext\
		{\
		_##klass##_ILineServicesHost_GetLineContext(HRESULT  (##klass##::*pfn)(DWORD __RPC_FAR *)){}\
		};\
	struct _##klass##_ILineServicesHost_FreeLineContext\
		{\
		_##klass##_ILineServicesHost_FreeLineContext(HRESULT  (##klass##::*pfn)(DWORD )){}\
		};\
	struct _##klass##_ILineServicesHost_SetContext\
		{\
		_##klass##_ILineServicesHost_SetContext(HRESULT  (##klass##::*pfn)(DWORD )){}\
		};\
	struct _##klass##_ILineServicesHost_ClearContext\
		{\
		_##klass##_ILineServicesHost_ClearContext(HRESULT  (##klass##::*pfn)(DWORD )){}\
		};\
	struct _##klass##_ILineServicesHost_Setup\
		{\
		_##klass##_ILineServicesHost_Setup(HRESULT  (##klass##::*pfn)(DWORD , long , long , BOOL )){}\
		};\
	struct _##klass##_ILineServicesHost_CpRelFromCpLs\
		{\
		_##klass##_ILineServicesHost_CpRelFromCpLs(HRESULT  (##klass##::*pfn)(DWORD , long , long __RPC_FAR *)){}\
		};\
	struct _##klass##_ILineServicesHost_CpLsFromCpRel\
		{\
		_##klass##_ILineServicesHost_CpLsFromCpRel(HRESULT  (##klass##::*pfn)(DWORD , long , long __RPC_FAR *)){}\
		};\
	struct _##klass##_ILineServicesHost_DiscardLine\
		{\
		_##klass##_ILineServicesHost_DiscardLine(HRESULT  (##klass##::*pfn)(DWORD )){}\
		};\
	struct _##klass##_ILineServicesHost_FetchHostRun\
		{\
		_##klass##_ILineServicesHost_FetchHostRun(HRESULT  (##klass##::*pfn)(DWORD , long , LPCWSTR __RPC_FAR *, DWORD __RPC_FAR *, BOOL __RPC_FAR *, void __RPC_FAR *, long , ITextPropertyList __RPC_FAR *__RPC_FAR *, void __RPC_FAR *__RPC_FAR *, DWORD __RPC_FAR *)){}\
		};\
	struct _##klass##_ILineServicesHost_TerminateLineAfterRun\
		{\
		_##klass##_ILineServicesHost_TerminateLineAfterRun(HRESULT  (##klass##::*pfn)(DWORD , void __RPC_FAR *)){}\
		};\
	struct _##klass##_ILineServicesHost_GetLineSummaryInfo\
		{\
		_##klass##_ILineServicesHost_GetLineSummaryInfo(HRESULT  (##klass##::*pfn)(DWORD , long , DWORD __RPC_FAR *)){}\
		};\
	void klass::VerifyILineServicesHost(){\
	_##klass##_ILineServicesHost_GetTreePointer pfn1(GetTreePointer);\
	_##klass##_ILineServicesHost_FreeTreePointer pfn2(FreeTreePointer);\
	_##klass##_ILineServicesHost_SetTreePointerCp pfn3(SetTreePointerCp);\
	_##klass##_ILineServicesHost_GetTreePointerCch pfn4(GetTreePointerCch);\
	_##klass##_ILineServicesHost_MoveToNextTreePos pfn5(MoveToNextTreePos);\
	_##klass##_ILineServicesHost_IsNestedLayoutAtTreePos pfn6(IsNestedLayoutAtTreePos);\
	_##klass##_ILineServicesHost_AdvanceBeforeLine pfn7(AdvanceBeforeLine);\
	_##klass##_ILineServicesHost_AdvanceAfterLine pfn8(AdvanceAfterLine);\
	_##klass##_ILineServicesHost_HiddenBeforeAlignedObj pfn9(HiddenBeforeAlignedObj);\
	_##klass##_ILineServicesHost_GetLineContext pfn10(GetLineContext);\
	_##klass##_ILineServicesHost_FreeLineContext pfn11(FreeLineContext);\
	_##klass##_ILineServicesHost_SetContext pfn12(SetContext);\
	_##klass##_ILineServicesHost_ClearContext pfn13(ClearContext);\
	_##klass##_ILineServicesHost_Setup pfn14(Setup);\
	_##klass##_ILineServicesHost_CpRelFromCpLs pfn15(CpRelFromCpLs);\
	_##klass##_ILineServicesHost_CpLsFromCpRel pfn16(CpLsFromCpRel);\
	_##klass##_ILineServicesHost_DiscardLine pfn17(DiscardLine);\
	_##klass##_ILineServicesHost_FetchHostRun pfn18(FetchHostRun);\
	_##klass##_ILineServicesHost_TerminateLineAfterRun pfn19(TerminateLineAfterRun);\
	_##klass##_ILineServicesHost_GetLineSummaryInfo pfn20(GetLineSummaryInfo);\
	}\

#else

#define CheckILineServicesHostMembers(klass)

#endif /* DEBUG */

#define idILineServicesHost	0xD315B4FC

#ifdef DeclareSmartPointer
#ifndef ILineServicesHostSPMethodsDefined
extern "C++" { TEMPLATECLASSB class ILineServicesHostSPMethods : public TEMPLATEBASEB(IUnknownSPMethods) { }; }
#define ILineServicesHostSPMethodsDefined
#endif
DeclareSmartPointer(ILineServicesHost)
#define SPILineServicesHost auto SP_ILineServicesHost
#endif

#define DeclareILineServicesHostMethods()\
	(FNOBJECT) GetLineSummaryInfo,\
	(FNOBJECT) TerminateLineAfterRun,\
	(FNOBJECT) FetchHostRun,\
	(FNOBJECT) DiscardLine,\
	(FNOBJECT) CpLsFromCpRel,\
	(FNOBJECT) CpRelFromCpLs,\
	(FNOBJECT) Setup,\
	(FNOBJECT) ClearContext,\
	(FNOBJECT) SetContext,\
	(FNOBJECT) FreeLineContext,\
	(FNOBJECT) GetLineContext,\
	(FNOBJECT) HiddenBeforeAlignedObj,\
	(FNOBJECT) AdvanceAfterLine,\
	(FNOBJECT) AdvanceBeforeLine,\
	(FNOBJECT) IsNestedLayoutAtTreePos,\
	(FNOBJECT) MoveToNextTreePos,\
	(FNOBJECT) GetTreePointerCch,\
	(FNOBJECT) SetTreePointerCp,\
	(FNOBJECT) FreeTreePointer,\
	(FNOBJECT) GetTreePointer,\

#define DeclareILineServicesHostVtbl()\
	,_Method1\
	,_Method2\
	,_Method3\
	,_Method4\
	,_Method5\
	,_Method6\
	,_Method7\
	,_Method8\
	,_Method9\
	,_Method10\
	,_Method11\
	,_Method12\
	,_Method13\
	,_Method14\
	,_Method15\
	,_Method16\
	,_Method17\
	,_Method18\
	,_Method19\
	,_Method20\

}



#endif

