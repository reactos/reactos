/****************************************************************************\
*                                                                            *
* penwin.h -    Pen Windows functions, types, and definitions                *
*                                                                            *
*               Version 2.0                                                  *
*                                                                            *
*               Copyright (c) 1992-1993 Microsoft Corp. All rights reserved. *
*                                                                            *
*******************************************************************************
*
* The following symbols control inclusion of various parts of this file;
* (indented identifiers are included by the previous main identifier):
*
* #define:				To prevent inclusion of:
*
* PENVER					Pen Windows version number (0x0200).  To exclude
*							definitions introduced in version 2.0 (or above)
*							#define PENVER 0x0100 before #including <penwin.h>
*
* NOPENALC           Alphabet Code definitions and macros
* NOPENAPPS				Pen Applications: Screen Keyboard
* NOPENCTL				H/BEDIT, IEDIT, and pen-enabled USER controls, including:
*   NOPENBEDIT			 : Boxed Edit Control
*   NOPENIEDIT			 : Ink Edit Control
*   NOPENHEDIT        : (H)Edit control
* NOPENDATA				PenData APIs and definitions
* NOPENDICT				Dictionary support
* NOPENDRIVER			Pen Driver definitions, incl OEM
* NOPENGEST				Gesture macros and Gesture Mapper (1.0)
* NOPENHRC				Handwriting Recognizer APIs and definitions
* NOPENINKPUT			Inking and Input APIs and definitions
* NOPENMISC				Miscellaneous Info and Utility APIs and definitions
* NOPENMSGS				Pen Messages and definitions
* NOPENNLS				National Language Support
* NOPENRC1				Recognition Context APIs and definitions (1.0)
* NOPENRES				Pen resources, including:
*   NOPENBMP			 : Pen-related bitmaps
*   NOPENCURS			 : Pen-related cursors
* NOPENTARGET			Targeting APIs and definitions
* NOPENVIRTEVENT		Virtual Event layer APIs
*
* WINPAD             non-WinPad components, subincludes:
*                     : NOPENAPPS, NOPENDICT, NOPENGEST, NOPENRC1
*
* "FBC" in the comments means that the feature exists only for
* backward compatibility. It should not be used by new applications.
\****************************************************************************/

#ifndef _INC_PENWIN
#define _INC_PENWIN

#include <windows.h>

#ifndef RC_INVOKED
#pragma pack(1)
#endif /* RC_INVOKED */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#ifndef PENVER		/* may be pre-defined; otherwise assume version 2.0 */
	#define PENVER  0x0200
#endif //!PENVER

#ifdef WINPAD
	#define NOPENAPPS
	#define NOPENDICT
	#define NOPENGEST
	#define NOPENRC1
#endif //WINPAD

#ifndef NOPENAPPS	// not available in WINPAD
#ifndef RC_INVOKED
	#include <skbapi.h>
#endif /* !RC_INVOKED */
#endif /*!NOPENAPPS */

// other subsets:

#ifdef NOPENCTL
	#define NOPENBEDIT
	#define NOPENIEDIT
#endif /* NOPENCTL */

#ifdef NOPENRES
	#define NOPENBMP
	#define NOPENCURS
#endif /* NOPENRES */


//////////////////////////////////////////////////////////////////////////////
/****** Definitions 1: for everything including RC compiler *****************/

//---------------------------------------------------------------------------
#ifndef NOPENALC

// Enabled Alphabet:
#define ALC_DEFAULT				0x00000000L	// nothing
#define ALC_LCALPHA				0x00000001L	// a..z
#define ALC_UCALPHA				0x00000002L	// A..Z
#define ALC_NUMERIC				0x00000004L	// 0..9
#define ALC_PUNC					0x00000008L	// !-;`"?()&.,; and backslash
#define ALC_MATH					0x00000010L	// %^*()-+={}<>,/.
#define ALC_MONETARY				0x00000020L	// ,.$ or local
#define ALC_OTHER					0x00000040L	// @#|_~[]
#define ALC_ASCII					0x00000080L	// restrict to 7-bit chars 20..7f
#define ALC_WHITE					0x00000100L	// white space
#define ALC_NONPRINT				0x00000200L	// sp tab ret ctrl glyphs
#define ALC_DBCS					0x00000400L	// allow DBCS variety of SBCS
#define ALC_JIS1					0x00000800L	// kanji JPN, ShiftJIS 1 only
#define ALC_GESTURE				0x00004000L	// gestures
#define ALC_USEBITMAP			0x00008000L	// use rc.rgbfAlc to enable chars
#define ALC_HIRAGANA				0x00010000L	// hiragana JPN
#define ALC_KATAKANA				0x00020000L	// katakana JPN
#define ALC_KANJI					0x00040000L	// kanji JPN, ShiftJIS 1+2+3
#define ALC_OEM					0x0FF80000L	// OEM recognizer-specific
#define ALC_RESERVED				0xF0003000L	// avail for future use
#define ALC_NOPRIORITY			0x00000000L	// for alcPriority == none

#define ALC_ALPHA\
	(ALC_LCALPHA | ALC_UCALPHA)											// 0x00000003L

#define ALC_ALPHANUMERIC\
	(ALC_ALPHA | ALC_NUMERIC)												// 0x00000007L

#define ALC_SYSMINIMUM\
	(ALC_ALPHANUMERIC | ALC_PUNC | ALC_WHITE | ALC_GESTURE) 		// 0x0000410FL	

#define ALC_ALL\
	(ALC_SYSMINIMUM | ALC_MATH | ALC_MONETARY\
	| ALC_OTHER | ALC_NONPRINT)											// 0x0000437FL

#define ALC_KANJISYSMINIMUM\
	(ALC_SYSMINIMUM | ALC_HIRAGANA | ALC_KATAKANA | ALC_JIS1)	// 0x0003490FL

#define ALC_KANJIALL\
	(ALC_ALL | ALC_HIRAGANA | ALC_KATAKANA | ALC_KANJI)			// 0x0007437FL

#endif /*!NOPENALC */


//---------------------------------------------------------------------------
#ifndef NOPENBMP

// Public Bitmaps :
#define OBM_SKBBTNUP				32767
#define OBM_SKBBTNDOWN			32766
#define OBM_SKBBTNDISABLED		32765

#define OBM_ZENBTNUP				32764
#define OBM_ZENBTNDOWN			32763
#define OBM_ZENBTNDISABLED		32762

#define OBM_HANBTNUP				32761
#define OBM_HANBTNDOWN			32760
#define OBM_HANBTNDISABLED		32759

#define OBM_KKCBTNUP				32758
#define OBM_KKCBTNDOWN			32757
#define OBM_KKCBTNDISABLED		32756

#define OBM_SIPBTNUP				32755
#define OBM_SIPBTNDOWN			32754
#define OBM_SIPBTNDISABLED		32753

#define OBM_PTYBTNUP				32752
#define OBM_PTYBTNDOWN			32751
#define OBM_PTYBTNDISABLED		32750
#endif /*!NOPENBMP */


//---------------------------------------------------------------------------
#ifndef NOPENCURS
// Default pen cursor to indicate writing, points northwest
#define IDC_PEN					MAKEINTRESOURCE(32631)

// alternate select cursor: upsidedown standard arrow, points southeast
#define IDC_ALTSELECT			MAKEINTRESOURCE(32501)

#endif /*!NOPENCURS */


//---------------------------------------------------------------------------
#ifndef NOPENBEDIT
// box edit styles:
#define BXS_NONE					0x0000U	// none
#define BXS_RECT					0x0001U	// use rectangle instead of cusp
#define BXS_BOXCROSS				0x0004U	// use cross at box center
#define BXS_MASK					0x0007U	// mask for above

#endif /*!NOPENBEDIT */

//---------------------------------------------------------------------------
#ifndef NOPENIEDIT

// IEdit Pop-up Menu Command Items
#define IEM_UNDO					1			// Undo
#define IEM_CUT					2			// Cut
#define IEM_COPY					3			// Copy
#define IEM_PASTE					4			// Paste
#define IEM_CLEAR					5			// Clear 
#define IEM_SELECTALL			6			// Select All Strokes
#define IEM_ERASE					7			// Use Eraser
#define IEM_PROPERTIES			8			// DoProperties
#define IEM_HELP					9			// Help
#define IEM_LASSO					10			// Use Lasso
#define IEM_RESIZE				11			// Resize
// #define IEM_EDIT		  		12			// Edit... (Formats)
// #define IEM_COLOR				13			// Ink Color
// #define IEM_NIB		  		14			// Ink Nib
// #define IEM_SELECT	  		15			// Select...	
// #define IEM_OPTIONS	  		16			// Options...
//								  		17-99		// Reserved
#define IEM_USER					100		// first menu item# available to app

// IEdit Style Attributes
#define IES_BORDER				0x0001	// ctl has a border
#define IES_HSCROLL				0x0002	// ctl is horizontally scrollable
#define IES_VSCROLL				0x0004	// ctl is vertically scrollable
#define IES_OWNERDRAW			0x0008	// ctl will be drawn by parent window
#ifdef WINPAD
#define IES_READONLY				0x0010	// ctl is read only
#define IES_NTPARENT				0x0020	// 
#endif //WINPAD

#endif /*!NOPENIEDIT */


#ifndef RC_INVOKED	// ... rest of file of no interest to rc compiler
//////////////////////////////////////////////////////////////////////////////

/****** Definitions 2: RC compiler excluded ********************************/

//---------------------------------------------------------------------------
#ifndef NOPENDATA

// PenData API constants:


// ANIMATEINFO callback options:
#define AI_CBSTROKE				0xFFFF 	// Animate callback after every stroke

// ANIMATEINFO options:
#define AI_SKIPUPSTROKES		0x0001	// ignore upstrokes in animation

// CompressPenData() API options:
#define CMPD_COMPRESS			0x0001
#define CMPD_DECOMPRESS			0x0002

// CreatePenDataRegion types:
#define CPDR_BOX					1			// bounding box
#define CPDR_LASSO				2			// lasso

// CreatePenData (CPD) and Pen Hardware (PHW) Flags;
// The order of PHW flags is important:
#define CPD_DEFAULT				0x047F	// CPD_TIME | PHW_ALL
#define CPD_USERBYTE				0x0100	// alloc 8 bits/stroke
#define CPD_USERWORD				0x0200	// alloc 16 bits/stroke
#define CPD_USERDWORD			0x0300	// alloc 32 bits/stroke
#define CPD_TIME					0x0400	// maintain abs time info per stroke

// DrawPenDataEx() flags/options:
#define DPD_HDCPEN				0x0001	// use pen selected in HDC
#define DPD_DRAWSEL				0x0002 	// draw the selection

// ExtractPenDataPoints options (EPDP_xx):
#define EPDP_REMOVE				0x0001	// Remove points from the pendata

// ExtractPenDataStrokes options and modifiers (EPDS_xx):
#define EPDS_SELECT				1			// selected strokes
#define EPDS_STROKEINDEX		2			// index
#define EPDS_USER					3			// user-specific value
#define EPDS_PENTIP				4			// complete pentip
#define EPDS_TIPCOLOR			5			// pentip color
#define EPDS_TIPWIDTH			6			// pentip width
#define EPDS_TIPNIB				7			// pentip nib style
#define EPDS_INKSET				8			// inkset match

#define EPDS_EQ					0x0000	// default: same as
#define EPDS_LT					0x0010	// all strokes less than
#define EPDS_GT					0x0020	// all strokes greater than
#define EPDS_NOT					0x0040	// all strokes not matching
#define EPDS_NE					0x0040	// alias
#define EPDS_GTE					0x0050	// alias for NOT LT
#define EPDS_LTE					0x0060	// alias for NOT GT

#define EPDS_REMOVE				0x8000	// remove matching strokes from source

// GetPenDataAttributes options (GPA_xx):
#define GPA_MAXLEN				1	// length of longest stroke
#define GPA_POINTS				2	// total number of points
#define GPA_PDTS					3	// PDTS_xx bits
#define GPA_RATE					4	// get sampling rate
#define GPA_RECTBOUND			5	// bounding rect of all points
#define GPA_RECTBOUNDINK		6	// ditto, adj for fat ink
#define GPA_SIZE					7	// size of pendata in bytes
#define GPA_STROKES				8	// total number of strokes
#define GPA_TIME					9	// absolute time at creation of pendata
#define GPA_USER					10	// number of user bytes available: 0, 1, 2, 4
#define GPA_VERSION				11	// version number of pendata

// GetStrokeAttributes options (GSA_xx):
#define GSA_PENTIP				1	// get stroke pentip (color, width, nib)
#define GSA_PENTIPCLASS			2	// same as GSA_PENTIP
#define GSA_USER					3	// get stroke user value
#define GSA_USERCLASS			4	// get stroke's class user value
#define GSA_TIME					5	// get time of stroke
#define GSA_SIZE					6	// get size of stroke in points and bytes
#define GSA_SELECT				7	// get selection status of stroke
#define GSA_DOWN					8	// get up/down state of stroke
#define GSA_RECTBOUND			9	// get the bounding rectangle of the stroke

// GetStrokeTableAttributes options (GSA_xx):
#define GSA_PENTIPTABLE			10	// get table-indexed pentip
#define GSA_SIZETABLE			11	// get count of Stroke Class Table entries
#define GSA_USERTABLE			12	// get table-indexed user value


#ifndef IX_END
#define IX_END						0xFFFF	// to or past last available index
#endif //!IX_END

// PenTip:
#define PENTIP_NIBDEFAULT		((BYTE)0)		// default pen tip nib style
#define PENTIP_HEIGHTDEFAULT	((BYTE)0)		// default pen tip nib height
#define PENTIP_OPAQUE			((BYTE)0xFF)	// default opaque ink
#define PENTIP_HILITE			((BYTE)0x80)
#define PENTIP_TRANSPARENT		((BYTE)0)

// General PenData API return values (PDR_xx):
#define PDR_NOHIT					3			// hit test failed
#define PDR_HIT					2			// hit test succeeded
#define PDR_OK						1			// success
#define PDR_CANCEL				0			// callback cancel or impasse

#define PDR_ERROR					(-1)		// parameter or unspecified error
#define PDR_PNDTERR				(-2)		// bad pendata
#define PDR_VERSIONERR			(-3)		// pendata version error
#define PDR_COMPRESSED			(-4)		// pendata is compressed
#define PDR_STRKINDEXERR		(-5)		// stroke index error
#define PDR_PNTINDEXERR			(-6)		// point index error
#define PDR_MEMERR				(-7)		// memory error
#define PDR_INKSETERR			(-8)		// bad inkset
#define PDR_ABORT					(-9)		// pendata has become invalid, e.g.
#define PDR_NA						(-10)		// option not available (pw kernel)

#define PDR_USERDATAERR			(-16)		// user data error
#define PDR_SCALINGERR			(-17)		// scale error
#define PDR_TIMESTAMPERR		(-18)		// timestamp error
#define PDR_OEMDATAERR			(-19)		// OEM data error
#define PDR_SCTERR				(-20)		// SCT error (full)

// PenData Scaling (PDTS):
#define PDTS_LOMETRIC			0			// 0.01mm
#define PDTS_HIMETRIC			1			// 0.001mm
#define PDTS_HIENGLISH			2			// 0.001"
#define PDTS_STANDARDSCALE		2			// PDTS_HIENGLISH	alias
#define PDTS_DISPLAY				3			// display pixel
#define PDTS_ARBITRARY			4			// app-specific scaling
#define PDTS_SCALEMASK			0x000F	// scaling values in low nibble

// CompactPenData() API trim options:
#define PDTT_DEFAULT				0x0000         
#define PDTT_PENINFO				0x0100
#define PDTT_UPPOINTS			0x0200
#define PDTT_OEMDATA				0x0400
#define PDTT_COLLINEAR			0x0800 
#define PDTT_COLINEAR			0x0800 	// alt sp alias
#define PDTT_DECOMPRESS			0x4000	// decompress the data
#define PDTT_COMPRESS			0x8000
#define PDTT_ALL					0x0F00	// PENINFO|UPPOINTS|OEMDATA|COLLINEAR

#define PHW_NONE					0x0000	// no OEMdata
#define PHW_PRESSURE				0x0001	// report pressure in OEMdata if avail
#define PHW_HEIGHT				0x0002	// ditto height
#define PHW_ANGLEXY				0x0004	// ditto xy angle
#define PHW_ANGLEZ				0x0008	// ditto z angle
#define PHW_BARRELROTATION		0x0010	// ditto barrel rotation
#define PHW_OEMSPECIFIC			0x0020	// ditto OEM-specific value
#define PHW_PDK					0x0040	// report per-point PDK_xx in OEM data
#define PHW_ALL					0x007F	// report everything


// compact pen data trim options: matches PDTT_values (see above)
#define PDTS_COMPRESS2NDDERIV	0x0010	// compress using 2nd deriv
#define PDTS_COMPRESSMETHOD	0x00F0	// sum of compress method flags
#define PDTS_NOPENINFO			0x0100	// removes PENINFO struct from header
#define PDTS_NOUPPOINTS			0x0200	// remove up pts
#define PDTS_NOOEMDATA			0x0400	// remove OEM data
#define PDTS_NOCOLLINEAR		0x0800	// remove successive identical pts
#define PDTS_NOCOLINEAR			0x0800	// alt sp alias
#define PDTS_NOTICK				0x1000	// remove timing info (2.0)
#define PDTS_NOUSER				0x2000	// remove user info (2.0)
#define PDTS_NOEMPTYSTROKES	0x4000	// remove empty strokes (2.0)
#define PDTS_COMPRESSED			0x8000	// perform lossless compression


// SetStrokeAttributes options (SSA_xx):
#define SSA_PENTIP				1			// set stroke tip (color, width, nib)
#define SSA_PENTIPCLASS			2			// set stroke's class pentip
#define SSA_USER					3			// set stroke user value
#define SSA_USERCLASS			4			// set stroke's class user value
#define SSA_TIME					5			// set time of stroke
#define SSA_SELECT				6			// set selection status of stroke
#define SSA_DOWN					7			// set up/down state of stroke

// SetStrokeTableAttributes options (SSA_xx):
#define SSA_PENTIPTABLE			8			// set table-indexed pentip
#define SSA_USERTABLE			9			// set table-indexed user value

// PenTip flag bits:
#define TIP_ERASECOLOR			1			// erase specific color pentip.rgb

// TrimPenData() API options:
#define TPD_RECALCSIZE			0x0000	// no trim, used for resize calc
#define TPD_USER					0x0080	// per-stroke user info
#define TPD_TIME					0x0100	// per-stroke timing info
#define TPD_UPPOINTS				0x0200	// x-y data up points
#define TPD_COLLINEAR			0x0400	// colinear and coincident points
#define TPD_COLINEAR				0x0400	// alt sp alias
#define TPD_PENINFO				0x0800	// PenInfo struct and all OEM
#define TPD_PHW					0x1000	// OEM & pdk except stroke tick or user
#define TPD_OEMDATA				0x1000	// ditto
#define TPD_EMPTYSTROKES		0x2000	// strokes with zero points
#define TPD_EVERYTHING			0x3FFF	// everything (incl PHW_xx) except down pts

#endif /*!NOPENDATA */

//---------------------------------------------------------------------------
#ifndef NOPENDICT	// not available in WINPAD

// Dictionary:
#define cbDictPathMax			255
#define DIRQ_QUERY				1
#define DIRQ_DESCRIPTION		2
#define DIRQ_CONFIGURE			3
#define DIRQ_OPEN					4
#define DIRQ_CLOSE				5
#define DIRQ_SETWORDLISTS		6
#define DIRQ_STRING				7
#define DIRQ_SUGGEST				8
#define DIRQ_ADD					9
#define DIRQ_DELETE				10
#define DIRQ_FLUSH				11
#define DIRQ_RCCHANGE			12
#define DIRQ_SYMBOLGRAPH		13
#define DIRQ_INIT					14
#define DIRQ_CLEANUP				15
#define DIRQ_COPYRIGHT			16
#define DIRQ_USER					4096
#endif /*!NOPENDICT */


//---------------------------------------------------------------------------
#ifndef NOPENDRIVER

// Pen driver:
#define BITPENUP					0x8000

// Pen Driver messages:
#define DRV_SetPenDriverEntryPoints    DRV_RESERVED+1
#define DRV_RemovePenDriverEntryPoints DRV_RESERVED+2
#define DRV_SetPenSamplingRate         DRV_RESERVED+3
#define DRV_SetPenSamplingDist         DRV_RESERVED+4
#define DRV_GetName                    DRV_RESERVED+5
#define DRV_GetVersion                 DRV_RESERVED+6
#define DRV_GetPenInfo                 DRV_RESERVED+7
#define DRV_PenPlayStart					DRV_RESERVED+8
#define DRV_PenPlayBack						DRV_RESERVED+9
#define DRV_PenPlayStop						DRV_RESERVED+10
#define DRV_GetCalibration             DRV_RESERVED+11
#define DRV_SetCalibration             DRV_RESERVED+12

#define MAXOEMDATAWORDS			6			// rgwOemData[MAXOEMDATAWORDS]

// Pen Collection Mode termination conditions:
// (note update doc for PCMINFO struct if change these)
#define PCM_PENUP					0x00000001L	// stop on penup
#define PCM_RANGE					0x00000002L	// stop on leaving range
#define PCM_INVERT				0x00000020L	// stop on tap of opposite end
#define PCM_RECTEXCLUDE			0x00002000L	// click in exclude rect
#define PCM_RECTBOUND			0x00004000L	// click outside bounds rect
#define PCM_TIMEOUT				0x00008000L	// no activity for timeout ms
// new for 2.0:
#define PCM_RGNBOUND				0x00010000L   // click outside bounding region
#define PCM_RGNEXCLUDE			0x00020000L   // click in exclude region
#define PCM_DOPOLLING			0x00040000L   // polling mode
#define PCM_TAPNHOLD				0x00080000L   // check for Tap And Hold
//#define PCM_VERMASK				0x70000000L   TODO
#define PCM_ADDDEFAULTS			RC_LDEFAULTFLAGS /* 0x80000000L */

// Pen Device Capabilities:
#define PDC_INTEGRATED			0x00000001L	// display==digitizer
#define PDC_PROXIMITY			0x00000002L	// detect non-contacting pen
#define PDC_RANGE					0x00000004L	// event on out-of-range
#define PDC_INVERT				0x00000008L	// pen opposite end detect
#define PDC_RELATIVE				0x00000010L	// pen driver coords
#define PDC_BARREL1				0x00000020L	// barrel button 1 present
#define PDC_BARREL2				0x00000040L	// ditto 2
#define PDC_BARREL3				0x00000080L	// ditto 3

// Pen Driver Kit states:
#define PDK_NULL					0x0000	// default to no flags set
#define PDK_UP						0x0000	// PDK_NULL alias
#define PDK_DOWN					0x0001	// pentip switch ON due to contact
#define PDK_BARREL1				0x0002	// barrel1 switch depressed
#define PDK_BARREL2				0x0004	// ditto 2
#define PDK_BARREL3				0x0008	// ditto 3
#define PDK_SWITCHES				0x000f	// sum of down + barrels 1,2,3
#define PDK_TRANSITION			0x0010	// set by GetPenHwData
#define PDK_UNUSED10				0x0020
#define PDK_UNUSED20				0x0040
#define PDK_INVERTED				0x0080	// other end of pen used as tip
#define PDK_PENIDMASK			0x0F00	// bits 8..11 physical pen id (0..15)
#define PDK_UNUSED1000			0x1000
#define PDK_INKSTOPPED			0x2000  // Inking stopped
#define PDK_OUTOFRANGE			0x4000	// pen left range (OEM data invalid)
#define PDK_DRIVER				0x8000	// pen (not mouse) event

#define PDK_TIPMASK				0x0001	// mask for testing PDK_DOWN

// OEM-specific values for Pen Driver:
#define PDT_NULL					0
#define PDT_PRESSURE				1			// pressure supported
#define PDT_HEIGHT				2			// height above tablet
#define PDT_ANGLEXY				3			// xy (horiz) angle supported
#define PDT_ANGLEZ				4			// z (vert) angle supported
#define PDT_BARRELROTATION		5			// barrel is rotated
#define PDT_OEMSPECIFIC			16		// max

// Denotes the ID of the current packet
#define PID_CURRENT				(UINT)(-1)

// Recognition and GetPenHwData Returns:
#define REC_OEM					(-1024)	// first recognizer-specific debug val
#define REC_LANGUAGE				(-48)	// unsupported language field
#define REC_GUIDE					(-47)	// invalid GUIDE struct
#define REC_PARAMERROR			(-46)	// bad param
#define REC_INVALIDREF			(-45)	// invalid data ref param
#define REC_RECTEXCLUDE			(-44)	// invalid rect
#define REC_RECTBOUND			(-43)	// invalid rect
#define REC_PCM					(-42)	// invalid lPcm parameter
#define REC_RESULTMODE			(-41)
#define REC_HWND					(-40)	// invalid window to send results to
#define REC_ALC					(-39)	// invalid enabled alphabet
#define REC_ERRORLEVEL			(-38)	// invalid errorlevel
#define REC_CLVERIFY				(-37)	// invalid verification level
#define REC_DICT					(-36)	// invalid dict params
#define REC_HREC					(-35)	// invalid recognition handle
#define REC_BADEVENTREF			(-33)	// invalid wEventRef
#define REC_NOCOLLECTION		(-32)	// collection mode not set
#define REC_DEBUG					(-32) // beginning of debug values
#define REC_POINTEREVENT		(-31)	// tap or tap&hold event
#define REC_BADHPENDATA			(-9)  // invalid hpendata header or locking
#define REC_OOM					(-8)	// out of memory error
#define REC_NOINPUT				(-7)	// no data collected before termination
#define REC_NOTABLET				(-6)	// tablet not physically present
#define REC_BUSY					(-5)	// another task is using recognizer
#define REC_BUFFERTOOSMALL		(-4)	// ret by GetPenHwEventData()
#define REC_ABORT					(-3)	// recog stopped by EndPenCollection()
#define REC_NA						(-2)	// function not available
#define REC_OVERFLOW				(-1)	// data overflow
#define REC_OK						0		// interrim completion
#define REC_TERMBOUND			1		// hit outside bounding rect
#define REC_TERMEX				2		// hit inside exclusion rect
#define REC_TERMPENUP			3		// pen up
#define REC_TERMRANGE			4		// pen left proximity
#define REC_TERMTIMEOUT			5		// no writing for timeout ms
#define REC_DONE					6		// normal completion
#define REC_TERMOEM				512	// first recognizer-specific retval

#endif /*!NOPENDRIVER */


//---------------------------------------------------------------------------
#ifndef NOPENGEST	// not available in WINPAD

// Gesture Mapping aliases:
#define MAP_GESTOGES				(RCRT_GESTURE|RCRT_GESTURETRANSLATED)
#define MAP_GESTOVKEYS			(RCRT_GESTURETOKEYS|RCRT_ALREADYPROCESSED)

#endif /*!NOPENGEST */


//---------------------------------------------------------------------------
#ifndef NOPENHRC

// Handwriting Recognizer:

// GetResultsHRC options:
#define GRH_ALL					0			// get all results
#define GRH_GESTURE				1			// get only gesture results
#define GRH_NONGESTURE			2			// get all but gesture results

// Gesture sets for EnableGestureSetHRC (bit flags):
#define GST_SEL					0x00000001L		// sel & lasso
#define GST_CLIP					0x00000002L		// cut copy paste
#define GST_WHITE					0x00000004L		// sp bs tab ret
#define GST_DEL					0x00000008L		// clear clearword
#define GST_EDIT					0x00000010L		// insert correct undo
#define GST_SYS					0x0000001FL		// all of the above
#define GST_CIRCLELO				0x00000100L		// lowercase circle
#define GST_CIRCLEUP				0x00000200L		// uppercase circle
#define GST_CIRCLE				0x00000300L		// all circle
#define GST_ALL					0x0000031FL		// all of the above

// General HRC API return values (HRCR_xx):
#define HRCR_NORESULTS			4			// No possible results  to be found
#define HRCR_COMPLETE			3			// finished recognition
#define HRCR_GESTURE				2			// recognized gesture
#define HRCR_OK					1			// success
#define HRCR_INCOMPLETE			0			// recognizer is processing input
#define HRCR_ERROR				(-1)		// invalid param or unspecified error
#define HRCR_MEMERR				(-2)		// memory error
#define HRCR_INVALIDGUIDE		(-3)		// invalid GUIDE struct
#define HRCR_INVALIDPNDT		(-4)		// invalid pendata
#define HRCR_UNSUPPORTED		(-5)		// recognizer does not support feature
#define HRCR_CONFLICT			(-6)		// training conflict
#define HRCR_HOOKED				(-8)		// hookasaurus ate the result

// system wordlist for AddWordsHWL:
#define HWL_SYSTEM				((HWL)1)	// magic value means system wordlist

// inkset returns:
#define ISR_ERROR					(-1)		// Memory or other error
#define ISR_BADINKSET			(-2)		// bad source inkset
#define ISR_BADINDEX				(-3)		// bad inkset index

#ifndef IX_END
#define IX_END						0xFFFF	// to or past last available index
#endif //!IX_END

#define MAXHOTSPOT				8			// max number of hotspots possible

// ProcessHRC time constants:
#define PH_MAX						0xFFFFFFFFL	// recognize rest of ink
#define PH_DEFAULT				0xFFFFFFFEL	// reasonable time
#define PH_MIN						0xFFFFFFFDL	// minimum time

// ResultsHookHRC options:
#define RHH_STD					0			// GetResultsHRC
#define RHH_BOX					1			// GetBoxResultsHRC

// SetWordlistCoercionHRC options:
#define SCH_NONE					0			// turn off coercion
#define SCH_REJECT				1			// recog results rejected if no match
#define SCH_ADVISE				2			// macro is hint only
#define SCH_FORCE					3			// some result is forced from macro

// Symbol Context Insert Modes
#define SCIM_INSERT				0			// insert
#define SCIM_OVERWRITE			1			// overwrite

// SetResultsHookHREC options:
#define SRH_HOOKALL				(HREC)1	// hook all recognizers

// SetInternationalHRC options:
#define SSH_RD						1			// to right and down (English)
#define SSH_RU						2			// to right and up
#define SSH_LD						3			// to left and down (Hebrew)
#define SSH_LU						4			// to left and up
#define SSH_DL						5			// down and to the left (Chinese)
#define SSH_DR						6			// down and to the right (Chinese)
#define SSH_UL						7			// up and to the left
#define SSH_UR						8			// up and to the right

#define SIH_ALLANSICHAR			1			// use all ANSI

// special SYV values:
#define SYV_NULL					0x00000000L
#define SYV_UNKNOWN				0x00000001L
#define SYV_EMPTY					0x00000003L
#define SYV_BEGINOR				0x00000010L
#define SYV_ENDOR					0x00000011L
#define SYV_OR						0x00000012L
#define SYV_SOFTNEWLINE			0x00000020L
#define SYV_SPACENULL			0x00010000L	// SyvCharacterToSymbol('\0')

// SYV values for gestures:
#define SYV_SELECTFIRST			0x0002FFC0L   // . means circle in following
#define SYV_LASSO					0x0002FFC1L   // lasso o-tap
#define SYV_SELECTLEFT			0x0002FFC2L   // no glyph
#define SYV_SELECTRIGHT			0x0002FFC3L   // no glyph
#define SYV_SELECTLAST			0x0002FFCFL   // 16 SYVs reserved for selection

#define SYV_CLEARCHAR			0x0002FFD2L   // d.
#define SYV_HELP					0x0002FFD3L   // no glyph
#define SYV_KKCONVERT			0x0002FFD4L   // k.
#define SYV_CLEAR					0x0002FFD5L   // d.
#define SYV_INSERT				0x0002FFD6L   // ^.
#define SYV_CONTEXT				0x0002FFD7L   // m.
#define SYV_EXTENDSELECT		0x0002FFD8L   // no glyph
#define SYV_UNDO					0x0002FFD9L   // u.
#define SYV_COPY					0x0002FFDAL   // c.
#define SYV_CUT					0x0002FFDBL   // x.
#define SYV_PASTE					0x0002FFDCL   // p.
#define SYV_CLEARWORD			0x0002FFDDL   // no glyph
#define SYV_USER					0x0002FFDEL   // reserved
#define SYV_CORRECT				0x0002FFDFL   // check.

#define SYV_BACKSPACE			0x00020008L   // no glyph
#define SYV_TAB					0x00020009L   // t.
#define SYV_RETURN				0x0002000DL   // n.
#define SYV_SPACE					0x00020020L   // s.

// Application specific gestures, Circle a-z and Circle A-Z:
#define SYV_APPGESTUREMASK		0x00020000L
#define SYV_CIRCLEUPA			0x000224B6L		// map into Unicode space
#define SYV_CIRCLEUPZ			0x000224CFL		// 	for circled letters
#define SYV_CIRCLELOA			0x000224D0L
#define SYV_CIRCLELOZ			0x000224E9L

// SYV definitions for shapes:
#define SYV_SHAPELINE			0x00040001L
#define SYV_SHAPEELLIPSE		0x00040002L
#define SYV_SHAPERECT			0x00040003L
#define SYV_SHAPEMIN				SYV_SHAPELINE	// alias
#define SYV_SHAPEMAX				SYV_SHAPERECT	// alias

// SYV classes:
#define SYVHI_SPECIAL			0
#define SYVHI_ANSI				1
#define SYVHI_GESTURE			2
#define SYVHI_KANJI				3
#define SYVHI_SHAPE				4
#define SYVHI_UNICODE			5
#define SYVHI_VKEY				6

// TrainHREC options:
#define TH_QUERY					0			// query the user if conflict
#define TH_FORCE					1			// ditto no query
#define TH_SUGGEST				2			// abandon training if conflict

// Return values for WCR_TRAIN Function
#define TRAIN_NONE				0x0000
#define TRAIN_DEFAULT			0x0001
#define TRAIN_CUSTOM				0x0002
#define TRAIN_BOTH				(TRAIN_DEFAULT | TRAIN_CUSTOM)

// Control values for TRAINSAVE
#define TRAIN_SAVE				0			// save changes that have been made
#define TRAIN_REVERT				1			// discard changes that have been made
#define TRAIN_RESET				2			// use factory settings

// ConfigRecognizer and ConfigHREC options:
#define WCR_RECOGNAME			0			// ConfigRecognizer 1.0
#define WCR_QUERY					1
#define WCR_CONFIGDIALOG		2
#define WCR_DEFAULT				3
#define WCR_RCCHANGE				4
#define WCR_VERSION				5
#define WCR_TRAIN					6
#define WCR_TRAINSAVE			7
#define WCR_TRAINMAX				8
#define WCR_TRAINDIRTY			9
#define WCR_TRAINCUSTOM			10
#define WCR_QUERYLANGUAGE		11
#define WCR_USERCHANGE			12

// ConfigHREC options:
#define WCR_PWVERSION			13			// ver of PenWin recognizer supports
#define WCR_GETALCPRIORITY		14			// get recognizer's ALC priority
#define WCR_SETALCPRIORITY		15			// set recognizer's ALC priority
#define WCR_GETANSISTATE		16			// get ALLANSICHAR state
#define WCR_SETANSISTATE		17			// set ALLANSICHAR if T

#define WCR_PRIVATE				1024

// sub-functions of WCR_USERCHANGE
#define CRUC_NOTIFY				0			// user name change
#define CRUC_REMOVE				1			// user name deleted

// Word List Types:
#define WLT_STRING				0			// one string
#define WLT_STRINGTABLE			1			// array of strings
#define WLT_EMPTY					2			// empty wordlist
#define WLT_WORDLIST				3			// handle to a wordlist

#endif /*!NOPENHRC */

//---------------------------------------------------------------------------
#ifndef NOPENIEDIT

// IEdit Background Options
#define IEB_DEFAULT				0			// default (use COLOR_WINDOW)
#define IEB_BRUSH					1			// paint background with brush
#define IEB_BIT_UL				2			// bitmap, upper-left aligned
#define IEB_BIT_CENTER			3			// bitmap, centered in control
#define IEB_BIT_TILE				4			// bitmap, tiled repeatedly in ctl
#define IEB_BIT_STRETCH			5			// bitmap, stretched to fit ctl
#define IEB_OWNERDRAW			6			// parent window will draw background

// IEdit Drawing Options
#define IEDO_NONE					0x0000	// no drawing
#define IEDO_FAST					0x0001	// ink drawn as fast as possible (def)
#define IEDO_SAVEUPSTROKES		0x0002	// save upstrokes
#define IEDO_RESERVED			0xFFFC	// reserved bits

// IEdit Input Options
#define IEI_MOVE					0x0001	// move ink into ctl
#define IEI_RESIZE				0x0002	// resize ink to fit within ctl
#define IEI_CROP					0x0004	// discard ink outside of ctl
#define IEI_DISCARD				0x0008	// discard all ink if any outside ctl	
#define IEI_RESERVED				0xFFF0	// reserved

// IEdit IE_GETINK options
#define IEGI_ALL					0x0000	// get all ink from control
#define IEGI_SELECTION			0x0001	// get selected ink from control

// IEdit IE_SETMODE/IE_GETMODE (mode) options
#define IEMODE_READY				0			// default inking, moving, sizing mode
#define IEMODE_ERASE				1			// erasing Mode
#define IEMODE_LASSO				2			// lasso selection mode

// IEdit	Notification Bits
#define IEN_NULL					0x0000	// null notification
#define IEN_PDEVENT				0x0001	// notify about pointing device events
#define IEN_PAINT					0x0002	// send painting-related notifications
#define IEN_FOCUS					0x0004	// send focus-related notifications
#define IEN_SCROLL				0x0008	// send scrolling notifications
#define IEN_EDIT					0x0010	// send editing/change notifications
#define IEN_RESERVED				0xFFC0	// reserved

// IEdit Return Values
#define IER_OK						0			// success
#define IER_NO						0			// ctl cannot do request
#define IER_YES					1			// ctl can do request
#define IER_ERROR					(-1)		// unspecified error; operation failed
#define IER_PARAMERR				(-2)		// bogus lParam value, bad handle, etc
#define IER_OWNERDRAW			(-3)		// can't set drawopts in ownerdraw ctl
#define IER_SECURITY				(-4)		// security protection disallows action
#define IER_SELECTION			(-5)		// nothing selected in control
#define IER_SCALE					(-6)		// merge:  incompatible scaling factors
#define IER_MEMERR				(-7)		// memory error
#define IER_NOCOMMAND			(-8)		// tried IE_GETCOMMAND w/no command
#define IER_NOGESTURE			(-9)		// tried IE_GETGESTURE w/no gesture
#define IER_NOPDEVENT			(-10)		// tried IE_GETPDEVENT but no event
#define IER_NOTINPAINT			(-11)		// tried IE_GETPAINTSTRUCT but no paint
#define IER_PENDATA				(-12)		// can't do request with NULL hpd in ctl


// IEdit Recognition Options
#define IEREC_NONE				0x0000	// No recognition
#define IEREC_GESTURE			0x0001	// Gesture recognition
#define IEREC_ALL					(IEREC_GESTURE)
#define IEREC_RESERVED			0xFFFE	// Reserved

// IEdit Security Options
#define IESEC_NOCOPY				0x0001	// copying disallowed
#define IESEC_NOCUT				0x0002	// cutting disallowed
#define IESEC_NOPASTE			0x0004	// pasting disallowed
#define IESEC_NOUNDO				0x0008	// undoing disallowed
#define IESEC_NOINK				0x0010	// inking  disallowed
#define IESEC_NOERASE			0x0020	// erasing disallowed
#define IESEC_NOGET				0x0040	// IE_GETINK message verboten
#define IESEC_NOSET				0x0080	// IE_SETINK message verboten
#define IESEC_RESERVED			0xFF00	// reserved

// IEdit IE_SETFORMAT/IE_GETFORMAT options
#define IESF_ALL					0x0001	// set/get stk fmt of all ink
#define IESF_SELECTION			0x0002	// set/get stk fmt of selected ink
#define IESF_STROKE				0x0004	// set/get stk fmt of specified stroke
//
#define IESF_TIPCOLOR			0x0008	// set color
#define IESF_TIPWIDTH			0x0010	// set width
#define IESF_PENTIP				(IESF_TIPCOLOR|IESF_TIPWIDTH)
//

// IEdit IE_SETINK options
#define IESI_REPLACE				0x0000	// replace ink in control
#define IESI_APPEND				0x0001	// append ink to existing control ink

// Ink Edit Control (IEdit) definitions
// IEdit Notifications
#define IN_PDEVENT		((IEN_PDEVENT<<8)|0)	// pointing device event occurred
#define IN_ERASEBKGND	((IEN_NULL<<8)|1)		// control needs bkgnd erased
#define IN_PREPAINT		((IEN_PAINT<<8)|2)	// before control paints its ink
#define IN_PAINT			((IEN_NULL<<8)|3)		// control needs to be painted
#define IN_POSTPAINT		((IEN_PAINT<<8)|4)	// after control has painted
#define IN_MODECHANGED	((IEN_EDIT<<8)|5)		// mode changed
#define IN_CHANGE			((IEN_EDIT<<8)|6)		// contents changed & painted
#define IN_UPDATE			((IEN_EDIT<<8)|7)		// contents changed & !painted
#define IN_SETFOCUS		((IEN_FOCUS<<8)|8)	// IEdit is getting focus
#define IN_KILLFOCUS		((IEN_FOCUS<<8)|9)	// IEdit is losing focus
#define IN_MEMERR			((IEN_NULL<<8)|10)	// Memory error
#define IN_HSCROLL		((IEN_SCROLL<<8)|11)	// horz scrolled, not painted
#define IN_VSCROLL		((IEN_SCROLL<<8)|12)	// vert scrolled, not painted
#define IN_GESTURE		((IEN_EDIT<<8)|13)	// user has gestured on control
#define IN_COMMAND		((IEN_EDIT<<8)|14)	// command selected from menu
#define IN_CLOSE			((IEN_NULL<<8)|15)	// I-Edit is being closed

#endif /*!NOPENIEDIT */


//---------------------------------------------------------------------------
#ifndef NOPENINKPUT

// PenIn[k]put API constants

// Default Processing
#define LRET_DONE					1L
#define LRET_ABORT				(-1L)
#define LRET_HRC					(-2L)
#define LRET_HPENDATA			(-3L)
#define LRET_PRIVATE				(-4L)

// Inkput:
#define PCMR_OK					0
#define PCMR_ALREADYCOLLECTING (-1)
#define PCMR_INVALIDCOLLECTION (-2)
#define PCMR_EVENTLOCK			(-3)
#define PCMR_INVALID_PACKETID	(-4)
#define PCMR_TERMTIMEOUT		(-5)
#define PCMR_TERMRANGE			(-6)
#define PCMR_TERMPENUP			(-7)
#define PCMR_TERMEX				(-8)
#define PCMR_TERMBOUND			(-9)
#define PCMR_APPTERMINATED		(-10)
#define PCMR_TAP					(-11)	// alias PCMR_TAPNHOLD_LAST
#define PCMR_SELECT				(-12)	// ret because of tap & hold
#define PCMR_OVERFLOW			(-13)
#define PCMR_ERROR				(-14)	// parameter or unspecified error
#define PCMR_DISPLAYERR			(-15)	// inking only
#define PCMR_NA					(-16)	// not available


#define PII_INKCLIPRECT			0x0001
#define PII_INKSTOPRECT			0x0002
#define PII_INKCLIPRGN			0x0004
#define PII_INKSTOPRGN			0x0008
#define PII_INKPENTIP			0x0010
#define PII_SAVEBACKGROUND		0x0020
#define PII_CLIPSTOP				0x0040

#define PIT_RGNBOUND				0x0001
#define PIT_RGNEXCLUDE			0x0002
#define PIT_TIMEOUT				0x0004
#define PIT_TAPNHOLD				0x0008

#endif /*!NOPENINKPUT */


//---------------------------------------------------------------------------
#ifndef NOPENMISC

// Misc RC Definitions:
#define CL_NULL					0
#define CL_MINIMUM				1			// minimum confidence level
#define CL_MAXIMUM				100		// max (require perfect recog)
#define cwRcReservedMax			8			// rc.rgwReserved[cwRcReservedMax]
#define ENUM_MINIMUM				1
#define ENUM_MAXIMUM				4096

#define HKP_SETHOOK				0			// SetRecogHook()
#define HKP_UNHOOK				0xFFFF

#define HWR_RESULTS				0
#define HWR_APPWIDE				1

#define iSycNull					(-1)
#define LPDFNULL					((LPDF)NULL)
#define MAXDICTIONARIES			16			// rc.rglpdf[MAXDICTIONARIES]
#define wPntAll					(UINT)0xFFFF
#define cbRcLanguageMax				44		// rc.lpLanguage[cbRcLanguageMax]
#define cbRcUserMax					32		// rc.lpUser[cbRcUserMax]
#define cbRcrgbfAlcMax				32		// rc.rgbfAlc[cbRcrgbfAlcMax]
#define RC_WDEFAULT					0xffff
#define RC_LDEFAULT					0xffffffffL
#define RC_WDEFAULTFLAGS			0x8000
#define RC_LDEFAULTFLAGS			0x80000000L

// CorrectWriting() API constants:
// LOWORD values: 
#define CWR_STRIPCR				0x0001	// strip carriage ret (\r)
#define CWR_STRIPLF				0x0002	// strip	linefeed (\n)
#define CWR_STRIPTAB				0x0004	// strip tab (\t)
#define CWR_SINGLELINEEDIT		(CWR_STRIPCR|CWR_STRIPLF|CWR_STRIPTAB)	// all of the above
#define CWR_INSERT				0x0008   // use "Insert Text" instead of "Edit Text" in the title
#define CWR_TITLE					0x0010	// interp dwReserved as LPSTR
#define CWR_KKCONVERT			0x0020	// JPN initiate IME

// HIWORD values: keyboard types
#define CWRK_DEFAULT          0   		// default keyboard type
#define CWRK_BASIC            1   		// basic keyboard
#define CWRK_FULL             2   		// full keyboard
#define CWRK_NUMPAD           3   		// numeric keyboard
#define CWRK_ATMPAD           4   		// ATM type keyboard

#ifdef JAPAN
#define GPMI_OK					0L
#define GPMI_INVALIDPMI			0x8000L
#endif // JAPAN

// inkwidth limits
#define INKWIDTH_MINIMUM		0			// 0 invisible, 1..15 pixel widths
#define INKWIDTH_MAXIMUM		15			// max width in pixels

// Get/SetPenMiscInfo:
// PMI_RCCHANGE is for WM_GLOBALRCCHANGE compatability only:
#define PMI_RCCHANGE					0	// invalid for Get/SetPenMiscInfo

#define PMI_BEDIT						1	// boxed edit info
#define PMI_IMECOLOR             2	// input method editor color
#define PMI_CXTABLET					3	// tablet width
#define PMI_CYTABLET					4	// tablet height
#define PMI_COUNTRY					5	// country
#define PMI_PENTIP					6	// pen tip: color, width, nib
#define PMI_ENABLEFLAGS				7	// PWE_xx enablements
#define PMI_TIMEOUT					8	// handwriting timeout
#define PMI_TIMEOUTGEST				9	// gesture timeout
#define PMI_TIMEOUTSEL				10	// select (press&hold) timeout
#define PMI_SYSFLAGS					11	// component load configuration
#define PMI_INDEXFROMRGB			12	// color table index from RGB
#define PMI_RGBFROMINDEX			13	// RGB from color table index
#define PMI_SYSREC					14	// handle to system recognizer
#define PMI_TICKREF					15 // reference absolute time
#define PMI_USER						16 // name of user

#define PMI_SAVE						0x1000	// save setting to file

// Set/GetPenMiscInfo/PMI_ENABLEFLAGS flags:
#define PWE_AUTOWRITE				0x0001	// pen functionality where IBeam
#define PWE_ACTIONHANDLES			0x0002	// action handles in controls
#define PWE_INPUTCURSOR				0x0004	// show cursor while writing
#define PWE_LENS						0x0008	// allow lens popup

// GetPenMiscInfo/PMI_SYSFLAGS flags:
#define PWF_RC1						0x0001	// Windows for Pen 1.0 RC support
#define PWF_PEN						0x0004	// pen drv loaded & hdwe init'd
#define PWF_INKDISPLAY				0x0008	// ink-compatible display drv loaded
#define PWF_RECOGNIZER				0x0010	// system recognizer installed
#define PWF_SKB						0x0020	// screen keyboard available
#define PWF_BEDIT						0x0100	// boxed edit support
#define PWF_HEDIT						0x0200	// free input edit support
#define PWF_IEDIT						0x0400	// ink edit support
#define PWF_ENHANCED					0x1000	// enh features (gest, 1ms timing)
#define PWF_FULL\
	PWF_RC1	|PWF_PEN		|PWF_INKDISPLAY	|PWF_RECOGNIZER	|PWF_SKB|\
	PWF_BEDIT|PWF_HEDIT	|PWF_IEDIT			|PWF_ENHANCED


// RegisterPenApp() API constants:
#define RPA_DEFAULT					0x0001	// == RPA_HEDIT
#define RPA_HEDIT						0x0001	// convert EDIT to HEDIT
#define RPA_KANJIFIXEDBEDIT		0x0002
#define RPA_DBCSPRIORITY			0x0004	// assume DBCS has priority (Japan)

#define PMIR_OK						0L
#define PMIR_INDEX					(-1L)
#define PMIR_VALUE					(-2L)
#define PMIR_INVALIDBOXEDITINFO	(-3L)
#define PMIR_INIERROR				(-4L)
#define PMIR_ERROR					(-5L)
#define PMIR_NA						(-6L)

#ifdef JAPAN
#define SPMI_OK						0L
#define SPMI_INVALIDBOXEDITINFO	1L
#define SPMI_INIERROR				2L
#define SPMI_INVALIDPMI				0x8000L
#endif //JAPAN

#endif /*!NOPENMISC */


//---------------------------------------------------------------------------
#ifndef NOPENRC1	// not available in WINPAD

// RC Options and Flags:
// GetGlobalRC() API return codes:
#define GGRC_OK					0			// no err
#define GGRC_DICTBUFTOOSMALL	1			// lpDefDict buffer too small for path
#define GGRC_PARAMERROR			2			// invalid params: call ignored
#define GGRC_NA					3			// function not available

// RC Direction:
#define RCD_DEFAULT				0			// def none
#define RCD_LR						1			// left to right like English
#define RCD_RL						2			// right to left like Arabic
#define RCD_TB						3			// top to bottom like Japanese
#define RCD_BT						4			// bottom to top like some Chinese

// RC International Preferences:
#define RCIP_ALLANSICHAR		0x0001	// all ANSI chars
#define RCIP_MASK					0x0001

// RC Options:
#define RCO_NOPOINTEREVENT		0x00000001L	// no recog tap, tap/hold
#define RCO_SAVEALLDATA			0x00000002L	// save pen data like upstrokes
#define RCO_SAVEHPENDATA		0x00000004L	// save pen data for app
#define RCO_NOFLASHUNKNOWN		0x00000008L	// no ? cursor on unknown
#define RCO_TABLETCOORD			0x00000010L	// tablet coords used in RC
#define RCO_NOSPACEBREAK		0x00000020L	// no space break recog -> dict
#define RCO_NOHIDECURSOR		0x00000040L	// display cursor during inking
#define RCO_NOHOOK				0x00000080L	// disallow ink hook (passwords)
#define RCO_BOXED					0x00000100L	// valid rc.guide provided
#define RCO_SUGGEST				0x00000200L	// for dict suggest
#define RCO_DISABLEGESMAP		0x00000400L	// disable gesture mapping
#define RCO_NOFLASHCURSOR		0x00000800L	// no cursor feedback
#define RCO_BOXCROSS				0x00001000L	// show + at boxedit center
#define RCO_COLDRECOG			0x00008000L	// result is from cold recog
#define RCO_SAVEBACKGROUND		0x00010000L // Save background from ink
#define RCO_DODEFAULT			0x00020000L // do default gesture processing

// RC Orientation of Tablet:
#define RCOR_NORMAL				1			// tablet not rotated
#define RCOR_RIGHT				2			// rotated 90 deg anticlockwise
#define RCOR_UPSIDEDOWN			3			// rotated 180 deg
#define RCOR_LEFT					4			// rotated 90 deg clockwise

// RC Preferences:
#define RCP_LEFTHAND				0x0001	// left handed input
#define RCP_MAPCHAR				0x0004	// fill in syg.lpsyc (ink) for training

// RCRESULT wResultsType values:
#define RCRT_DEFAULT				0x0000	// normal ret
#define RCRT_UNIDENTIFIED		0x0001 	// result contains unidentified results
#define RCRT_GESTURE				0x0002	// result is a gesture
#define RCRT_NOSYMBOLMATCH		0x0004	// nothing recognized (no ink match)
#define RCRT_PRIVATE				0x4000	// recognizer-specific symbol
#define RCRT_NORECOG				0x8000	// no recog attempted, only data ret
#define RCRT_ALREADYPROCESSED	0x0008	// GestMgr hooked it
#define RCRT_GESTURETRANSLATED 0x0010	// GestMgr translated it to ANSI value
#define RCRT_GESTURETOKEYS		0x0020	// ditto to set of virtual keys

// RC Result Return Mode specification:
#define RRM_STROKE				0			// return results after each stroke
#define RRM_SYMBOL				1			// per symbol (e.g. boxed edits)
#define RRM_WORD					2			// on recog of a word
#define RRM_NEWLINE				3			// on recog of a line break
#define RRM_COMPLETE				16		// on PCM_xx specified completion

// SetGlobalRC() API return code flags:
#define SGRC_OK					0x0000	// no err
#define SGRC_USER					0x0001	// invalid User name
#define SGRC_PARAMERROR			0x0002	// param error: call ignored
#define SGRC_RC					0x0004	// supplied RC has errors
#define SGRC_RECOGNIZER			0x0008	// DefRecog name invalid
#define SGRC_DICTIONARY			0x0010	// lpDefDict path invalid
#define SGRC_INIFILE				0x0020	// error saving to penwin.ini
#define SGRC_NA					0x8000	// function not available

#endif /*!NOPENRC1 */


//---------------------------------------------------------------------------

#ifndef NOPENTARGET

#define TPT_CLOSEST				0x0001   // Assign to the closest target   
#define TPT_INTERSECTINK		0x0002   // target with intersecting ink
#define TPT_TEXTUAL				0x0004   // apply textual heuristics
#define TPT_DEFAULT				(TPT_TEXTUAL | TPT_INTERSECTINK | TPT_CLOSEST)

#endif /*!NOPENTARGET */


//---------------------------------------------------------------------------
#ifndef NOPENVIRTEVENT

// Virtual Event Layer:
#define VWM_MOUSEMOVE			0x0001
#define VWM_MOUSELEFTDOWN		0x0002
#define VWM_MOUSELEFTUP			0x0004
#define VWM_MOUSERIGHTDOWN		0x0008
#define VWM_MOUSERIGHTUP		0x0010
#endif /*!NOPENVIRTEVENT */


#endif /* RC_INVOKED */	// ... all the way back from definitions:2

/****** Messages and Defines ************************************************/

// Windows Messages WM_PENWINFIRST (0x0380) and WM_PENWINLAST (0x038F)
// are defined in windows.h and winmin.h



//---------------------------------------------------------------------------
#ifndef NOPENMSGS

#ifndef NOPENRC1	// not available in WINPAD
#define WM_RCRESULT				(WM_PENWINFIRST+1)	// 0x381
#define WM_HOOKRCRESULT			(WM_PENWINFIRST+2)	// 0x382
#endif /*!NOPENRC1*/

#define WM_PENMISCINFO			(WM_PENWINFIRST+3)	// 0x383
#define WM_GLOBALRCCHANGE		(WM_PENWINFIRST+3)	// alias

#ifndef NOPENAPPS	// not available in WINPAD
#define WM_SKB						(WM_PENWINFIRST+4)	// 0x384
#endif /*!NOPENAPPS */

#define WM_PENCTL					(WM_PENWINFIRST+5)	// 0x385
#define WM_HEDITCTL				(WM_PENWINFIRST+5)	// FBC: alias

// WM_HEDITCTL (WM_PENCTL) wParam options:
#ifndef WINPAD
#define HE_GETRC					3		// FBC: get RC from HEDIT/BEDIT control
#define HE_SETRC					4		// FBC: ditto set
#endif //!WINPAD
#define HE_GETINFLATE			5		// FBC: get inflate rect
#define HE_SETINFLATE			6		// FBC: ditto set
#define HE_GETUNDERLINE		 	7		// get underline mode
#define HE_SETUNDERLINE		 	8		// ditto set
#define HE_GETINKHANDLE		 	9		// get handle to captured ink
#define HE_SETINKMODE			10		// begin HEDIT cold recog mode
#define HE_STOPINKMODE			11		// end cold recog mode
#define HE_GETRCRESULTCODE	 	12		// FBC: result of recog after HN_ENDREC
#define HE_DEFAULTFONT			13		// switch BEDIT to def font
#define HE_CHARPOSITION		 	14		// BEDIT byte offset -> char position
#define HE_CHAROFFSET			15		// BEDIT char position -> byte offset
#define HE_GETBOXLAYOUT		 	20		// get BEDIT layout
#define HE_SETBOXLAYOUT		 	21		// ditto set
#define HE_GETRCRESULT			22		// FBC: get RCRESULT after HN_RCRESULT
#define HE_KKCONVERT			 	30		// JPN start kana-kanji conversion
#define HE_GETKKCONVERT		 	31		// JPN get KK state
#define HE_CANCELKKCONVERT	 	32		// JPN cancel KK conversion
#define HE_FIXKKCONVERT		 	33		// JPN force KK result
#define HE_ENABLEALTLIST		40		// en/disable dropdown recog alt's
#define HE_SHOWALTLIST			41		// show dropdown (assume enabled)
#define HE_HIDEALTLIST			42		// hide dropdown alternatives

//------------------------------
// JPN KanaKanji conversion subfunctions:
#define HEKK_DEFAULT				0		// def
#define HEKK_CONVERT				1		// convert in place
#define HEKK_CANDIDATE			2		// start conversion dialog

// HE_STOPINKMODE (stop cold recog) options:
#define HEP_NORECOG				0		// don't recog ink
#define HEP_RECOG					1		// recog ink
#define HEP_WAITFORTAP			2		// recog after tap in window

// WM_PENCTL notifications:
#define HN_ENDREC					4		// recog complete
#define HN_DELAYEDRECOGFAIL	5		// HE_STOPINKMODE (cold recog) failed
#define HN_RESULT 				20		// HEDIT/BEDIT has received new ink/recognition result
#define HN_RCRESULT           HN_RESULT
#define HN_ENDKKCONVERT			30		// JPN KK conversion complete
#define HN_BEGINDIALOG			40		// Lens/EditText/garbage detection dialog is about 
                                    // to come up on this hedit/bedit
#define HN_ENDDIALOG			   41		// Lens/EditText/garbage detection dialog has
                                    // just been destroyed 

//------------------------------
#ifndef NOPENIEDIT

// Messages common with other controls:
#define IE_GETMODIFY			(EM_GETMODIFY)		// gets the mod'n (dirty) bit
#define IE_SETMODIFY			(EM_SETMODIFY)		// sets the mod'n (dirty) bit
#define IE_CANUNDO			(EM_CANUNDO)		// queries whether can undo
#define IE_UNDO				(EM_UNDO)			// undo
#define IE_EMPTYUNDOBUFFER	(EM_EMPTYUNDOBUFFER)	// clears IEDIT undo buffer

#define IE_MSGFIRST			(WM_USER+150)		// 0x496 == 1174

// IEdit and WinPad common messages:
#define IE_GETINK				(IE_MSGFIRST+0)	// gets ink from the control
#define IE_SETINK				(IE_MSGFIRST+1)	// sets ink into the control
#define IE_GETPENTIP			(IE_MSGFIRST+2)	// gets the cur def ink pentip
#define IE_SETPENTIP			(IE_MSGFIRST+3)	// sets the cur def ink pentip
#define IE_GETERASERTIP		(IE_MSGFIRST+4)	// gets the cur eraser pentip
#define IE_SETERASERTIP		(IE_MSGFIRST+5)	// sets the cur eraser pentip
#define IE_GETBKGND			(IE_MSGFIRST+6)	// gets the bkgnd options
#define IE_SETBKGND			(IE_MSGFIRST+7)	// sets the bkgnd options
#define IE_GETGRIDORIGIN	(IE_MSGFIRST+8)	// gets the bkgnd grid origin
#define IE_SETGRIDORIGIN 	(IE_MSGFIRST+9)	// sets the bkgnd grid origin
#define IE_GETGRIDPEN		(IE_MSGFIRST+10)	// gets the bkgnd grid pen
#define IE_SETGRIDPEN		(IE_MSGFIRST+11)	// sets the bkgnd grid pen
#define IE_GETGRIDSIZE		(IE_MSGFIRST+12)	// gets the bkgnd grid size
#define IE_SETGRIDSIZE		(IE_MSGFIRST+13)	// sets the bkgnd grid size
#define IE_GETMODE			(IE_MSGFIRST+14)	// gets the current pen mode
#define IE_SETMODE			(IE_MSGFIRST+15)	// sets the current pen mode
#define IE_GETINKRECT		(IE_MSGFIRST+16)	// gets the rectbound of the ink

// Winpad-specific messages:
#ifdef WINPAD
#define IE_GETORIGIN 		(IE_MSGFIRST+17)	// gets the control origin
#define IE_SETORIGIN 		(IE_MSGFIRST+18)	// sets the control origin
#define IE_GETSCROLLSTEP 	(IE_MSGFIRST+19)	// gets the scrolling step
#define IE_SETSCROLLSTEP 	(IE_MSGFIRST+20)	// sets the scrolling step
#define IE_GETCHANGEINK		(IE_MSGFIRST+21)	// gets last changed ink
#define IE_SCALEINK 			(IE_MSGFIRST+22)	// scales the control ink
#define IE_GETTAPCONVERT 	(IE_MSGFIRST+23)	// gets the cur turd dection 
#define IE_SETTAPCONVERT 	(IE_MSGFIRST+24)	// sets the cur turd dection
#define IE_GETTAPPOS 		(IE_MSGFIRST+25)	// gets the tap position
#define IE_GETPALETTE 		(IE_MSGFIRST+26)	// gets the palette
#define IE_SETPALETTE 		(IE_MSGFIRST+27)	// gets the palette
#define IE_HHUNUSED1			(IE_MSGFIRST+28)	// Unused
#define IE_HHUNUSED2			(IE_MSGFIRST+29)	// Unused
#define IE_HHUNUSED3			(IE_MSGFIRST+30)	// Unused
#define IE_HHUNUSED4			(IE_MSGFIRST+31)	// Unused
#define IE_HHUNUSED5			(IE_MSGFIRST+32)	// Unused
#define IE_HHUNUSED6			(IE_MSGFIRST+33)	// Unused
#endif //WINPAD

// IEdit-specific messages:
#ifndef WINPAD
#define IE_GETAPPDATA		(IE_MSGFIRST+34)	// gets the user-defined datum
#define IE_SETAPPDATA		(IE_MSGFIRST+35)	// sets the user-defined data
#define IE_GETDRAWOPTS		(IE_MSGFIRST+36)	// gets the ink draw options
#define IE_SETDRAWOPTS		(IE_MSGFIRST+37)	// sets the ink options
#define IE_GETFORMAT			(IE_MSGFIRST+38)	// gets format of stroke(s)
#define IE_SETFORMAT			(IE_MSGFIRST+39)	// sets format of stroke(s)
#define IE_GETINKINPUT		(IE_MSGFIRST+40)	// gets the ink input option
#define IE_SETINKINPUT		(IE_MSGFIRST+41)	// sets the ink input option
#define IE_GETNOTIFY			(IE_MSGFIRST+42)	// gets the notification bits
#define IE_SETNOTIFY			(IE_MSGFIRST+43)	// sets the notification bits
#define IE_GETRECOG			(IE_MSGFIRST+44)	// gets recognition options
#define IE_SETRECOG			(IE_MSGFIRST+45)	// sets recognition options
#define IE_GETSECURITY		(IE_MSGFIRST+46)	// gets the security options
#define IE_SETSECURITY		(IE_MSGFIRST+47)	// sets the security options
#define IE_GETSEL				(IE_MSGFIRST+48)	// gets sel status of a stroke
#define IE_SETSEL				(IE_MSGFIRST+49)	// sets sel status of a stroke
#define IE_DOCOMMAND			(IE_MSGFIRST+50)	// send command to IEdit
#define IE_GETCOMMAND		(IE_MSGFIRST+51)	// gets user command
#define IE_GETCOUNT			(IE_MSGFIRST+52)	// gets count of strks in I-Edit
#define IE_GETGESTURE		(IE_MSGFIRST+53)	// gets details on user gesture
#define IE_GETMENU			(IE_MSGFIRST+54)	// gets handle to pop-up menu
#define IE_GETPAINTDC		(IE_MSGFIRST+55)	// gets the HDC for painting
#define IE_GETPDEVENT		(IE_MSGFIRST+56)	// gets details of last pd event
#define IE_GETSELCOUNT		(IE_MSGFIRST+57)	// gets count of selected strks
#define IE_GETSELITEMS		(IE_MSGFIRST+58)	// gets indices of all sel strks
#define IE_GETSTYLE			(IE_MSGFIRST+59)	// gets IEdit control styles
#endif //!WINPAD

#endif /*!NOPENIEDIT */

//------------------------------
#ifndef NOPENHEDIT

// (H)Edit Control:
// CTLINITHDIT.dwFlags values
#define CIH_NOGDMSG           0x0001  // disable garbage detection message box for this edit
#define CIH_NOACTIONHANDLE    0x0002  // disable action handles for this edit
#define CIH_NOEDITTEXT        0x0004  // disable Lens/Edit/Insert text for this edit
#define CIH_NOFLASHCURSOR     0x0008  // don't flash cursor on tap-n-hold in this (h)edit

#endif /* !NOPENHEDIT */

//------------------------------
#ifndef NOPENBEDIT

// Boxed Edit Control:
// box edit alternative list:
#define HEAL_DEFAULT				-1L     // AltList def value for lParam

// box edit Info:
#define BEI_FACESIZE				32		// max size of font name, = LF_FACESIZE
#define BEIF_BOXCROSS			0x0001

// box edit size:
#define BESC_DEFAULT				0
#define BESC_ROMANFIXED			1
#define BESC_KANJIFIXED			2
#define BESC_USERDEFINED		3

// CTLINITBEDIT.wFlags values
#define CIB_NOGDMSG           0x0001  // disable garbage detection message box for this bedit
#define CIB_NOACTIONHANDLE    0x0002  // disable action handles for this bedit
#define CIB_NOFLASHCURSOR     0x0004  // don't flash cursor on tap-n-hold in this bedit

#define BXD_CELLWIDTH			12
#define BXD_CELLHEIGHT			16
#define BXD_BASEHEIGHT			13
#define BXD_BASEHORZ				0
#define BXD_MIDFROMBASE			0
#define BXD_CUSPHEIGHT			2
#define BXD_ENDCUSPHEIGHT		4

#define BXDK_CELLWIDTH			32
#define BXDK_CELLHEIGHT			32
#define BXDK_BASEHEIGHT			28
#define BXDK_BASEHORZ			0
#define BXDK_MIDFROMBASE		0
#define BXDK_CUSPHEIGHT			28
#define BXDK_ENDCUSPHEIGHT		10

#ifdef JAPAN
// IME colors for bedit
#define COLOR_BE_INPUT			   0 
#define COLOR_BE_INPUT_TEXT		1 
#define COLOR_BE_CONVERT		   2 
#define COLOR_BE_CONVERT_TEXT	   3 
#define COLOR_BE_CONVERTED		   4 
#define COLOR_BE_CONVERTED_TEXT	5 
#define COLOR_BE_UNCONVERT	      6 
#define COLOR_BE_UNCONVERT_TEXT	7 
#define COLOR_BE_CURSOR			   8 
#define COLOR_BE_CURSOR_TEXT	   9 
#define COLOR_BE_PRECONVERT		10
#define COLOR_BE_PRECONVERT_TEXT	11
#define MAXIMECOLORS			      12
#endif

#endif /*!NOPENBEDIT */

#define WM_PENMISC				(WM_PENWINFIRST+6)	// 0x386

// WM_PENMISC message constants:
#define PMSC_BEDITCHANGE		1	// broadcast when BEDIT changes
#define PMSC_PENUICHANGE		2	// JPN broadcast when PENUI changes
#define PMSC_SUBINPCHANGE		3	// JPN broadcast when SUBINPUT changes
#define PMSC_KKCTLENABLE		4	// JPN
#define PMSC_GETPCMINFO			5	// query the window's PCMINFO
#define PMSC_SETPCMINFO			6	// set the window's PCMINFO
#define PMSC_GETINKINGINFO		7	// query the window's INKINGINFO
#define PMSC_SETINKINGINFO		8	// set the window's INKINGINFO
#define PMSC_GETHRC				9	// query the window's HRC
#define PMSC_SETHRC				10	// set the window's HRC
#define PMSC_GETSYMBOLCOUNT	11	// count of symbols in result recd by window
#define PMSC_GETSYMBOLS 		12	// ditto symbols
#define PMSC_SETSYMBOLS 		13	// ditto set symbols
#ifndef WINPAD
#define PMSC_LOADPW				15	// broadcast load state on penwin
#endif //!WINPAD
#define PMSC_INKSTOP				16	// 

// PMSCL_xx lParam values for PMSC_xx:
#define PMSCL_UNLOADED			0L	// penwin just unloaded
#define PMSCL_LOADED				1L	// penwin just loaded
#define PMSCL_UNLOADING			2L	// penwin about to unload



#define WM_CTLINIT				(WM_PENWINFIRST+7)	// 0x387

// WM_CTLINIT message constants:
#define CTLINIT_HEDIT			1
#define CTLINIT_BEDIT			7
#define CTLINIT_IEDIT			9
#define CTLINIT_MAX				10

#define WM_PENEVENT				(WM_PENWINFIRST+8)	// 0x388

// WM_PENEVENT message values for wParam:
#define PE_PENDOWN				1	// pen tip down
#define PE_PENUP					2	// pen tip went from down to up
#define PE_PENMOVE				3	// pen moved without a tip transition
#define PE_TERMINATING			4	// Peninput about to terminate
#define PE_TERMINATED			5	// Peninput terminated
#define PE_BUFFERWARNING		6	// Buffer half full.
#define PE_BEGININPUT			7	// begin default input
#define PE_SETTARGETS			8	// set target data structure (TARGINFO)
#define PE_BEGINDATA				9	// init message to all targets
#define PE_MOREDATA				10	// target gets more data
#define PE_ENDDATA				11	// termination message to all targets
#define PE_GETPCMINFO			12	// get input collection info
#define PE_GETINKINGINFO		13	// get inking info
#define PE_ENDINPUT				14	// Input termination message to window
                                	// 	starting default input 
#define PE_RESULT             15 // sent after ProcessHRC but before GetResultsHRC

#endif /*!NOPENMSGS */


/****** Definitions 3: RC compiler excluded ********************************/

#ifndef RC_INVOKED	// ... rest of file of no interest to rc compiler



//////////////////////////////////////////////////////////////////////////////
/****** Macros **************************************************************/


// misc macros:
//---------------------------------------------------------------------------
#ifndef NOPENDRIVER

#define FPenUpX(x)				((BOOL)(((x) & BITPENUP) != 0))
#define GetWEventRef()			(LOWORD(GetMessageExtraInfo()))
#endif /*!NOPENDRIVER */

//---------------------------------------------------------------------------
#ifndef NOPENALC

// ALC macros:

#define MpAlcB(lprc,i)			((lprc)->rgbfAlc[((i) & 0xff) >> 3])
#define MpIbf(i)					((BYTE)(1 << ((i) & 7)))
#define SetAlcBitAnsi(lprc,i)	do {MpAlcB(lprc,i) |= MpIbf(i);} while (0)
#define ResetAlcBitAnsi(lprc,i) do {MpAlcB(lprc,i) &= ~MpIbf(i);} while (0)
#define IsAlcBitAnsi(lprc, i)	((MpAlcB(lprc,i) & MpIbf(i)) != 0)
#endif /*!NOPENALC */

//---------------------------------------------------------------------------
#ifndef NOPENGEST	// not available in WINPAD

// Gesture Macros:

#define FIsLoAppGesture(syv)	(syv >= SYV_CIRCLELOA && syv <= SYV_CIRCLELOZ)
#define FIsUpAppGesture(syv)	(syv >= SYV_CIRCLEUPA && syv <= SYV_CIRCLEUPZ)
#define FIsAppGesture(syv)		(syv>=SYV_CIRCLEUPA && syv<=SYV_CIRCLELOZ)
#define SyvAppGestureFromLoAnsi(ansi)	((DWORD)(BYTE)ansi- 'a'+SYV_CIRCLELOA)
#define SyvAppGestureFromUpAnsi(ansi)	((DWORD)(BYTE)ansi- 'A'+SYV_CIRCLEUPA)
#define AnsiFromSyvAppGesture(syv)		ChSyvToAnsi( \
	syv-(FIsUpAppGesture(syv)? SYV_CIRCLEUPA-(SYV)'A': SYV_CIRCLELOA-(SYV)'a'))

#define IsGestureToGesture(lprcresult) \
	(((lprcresult)->wResultstype & MAP_GESTOGES) == MAP_GESTOGES)

#define IsGestureToVkeys(lprcresult) \
	(((lprcresult)->wResultstype & MAP_GESTOVKEYS) == MAP_GESTOVKEYS)

#define SetAlreadyProcessed(lprcresult) \
	((lprcresult)->wResultsType = ((lprcresult)->wResultsType \
	& ~RCRT_GESTURETOKEYS) | RCRT_ALREADYPROCESSED)
#endif /*!NOPENGEST */


//---------------------------------------------------------------------------
#ifndef NOPENDATA

// draw 2.0 pendata using internal stroke formats:
#define DrawPenDataFmt(hdc, lprect, hpndt)\
	DrawPenDataEx(hdc, lprect, hpndt, 0, IX_END, 0, IX_END, NULL, NULL, 0)

#endif /*!NOPENDATA */

//---------------------------------------------------------------------------
#ifndef NOPENHRC

// Handwriting Recognizer:

// Intervals:
// difference of two absolute times (at2 > at1 for positive result):
#define dwDiffAT(at1, at2)\
	(1000L*((at2).sec - (at1).sec) - (DWORD)(at1).ms + (DWORD)(at2).ms)

// comparison of two absolute times (TRUE if at1 < at2):
#define FLTAbsTime(at1, at2)\
	((at1).sec < (at2).sec || ((at1).sec == (at2).sec && (at1).ms < (at2).ms))

#define FLTEAbsTime(at1, at2)\
	((at1).sec < (at2).sec || ((at1).sec == (at2).sec && (at1).ms <= (at2).ms))

#define FEQAbsTime(at1, at2)\
	((at1).sec == (at2).sec && (at1).ms == (at2).ms)

// test if abstime is within an interval:
#define FAbsTimeInInterval(at, lpi)\
	(FLTEAbsTime((lpi)->atBegin, at) && FLTEAbsTime(at, (lpi)->atEnd))

// test if interval (lpiT) is within an another interval (lpiS):
#define FIntervalInInterval(lpiT, lpiS)\
	(FLTEAbsTime((lpiS)->atBegin, (lpiT)->atBegin)\
	&& FLTEAbsTime((lpiT)->atEnd, (lpiS)->atEnd))

// test if interval (lpiT) intersects another interval (lpiS):
#define FIntervalXInterval(lpiT, lpiS)\
	(!(FLTAbsTime((lpiT)->atEnd, (lpiS)->atBegin)\
	|| FLTAbsTime((lpiS)->atEnd, (lpiT)->atBegin)))

// duration of an LPINTERVAL in ms:
#define dwDurInterval(lpi)	dwDiffAT((lpi)->atBegin, (lpi)->atEnd)

// fill a pointer to an ABSTIME structure from a count of seconds and ms:
#define MakeAbsTime(lpat, sec, ms) do {\
	(lpat)->sec = sec + ((ms) / 1000);\
	(lpat)->ms = (ms) % 1000;\
	} while (0)


// SYV macros:
#define FIsSpecial(syv)			(HIWORD((syv))==SYVHI_SPECIAL)
#define FIsAnsi(syv)				(HIWORD((syv))==SYVHI_ANSI)
#define FIsGesture(syv)			(HIWORD((syv))==SYVHI_GESTURE)
#define FIsKanji(syv)			(HIWORD((syv))==SYVHI_KANJI)
#define FIsShape(syv)			(HIWORD((syv))==SYVHI_SHAPE)
#define FIsUniCode(syv)			(HIWORD((syv))==SYVHI_UNICODE)
#define FIsVKey(syv)				(HIWORD((syv))==SYVHI_VKEY)

#define ChSyvToAnsi(syv)			((BYTE) (LOBYTE(LOWORD((syv)))))
#define WSyvToKanji(syv)			((WORD) (LOWORD((syv))))
#define SyvCharacterToSymbol(c)	((LONG)(unsigned char)(c) | 0x00010000)
#define SyvKanjiToSymbol(c)		((LONG)(UINT)(c) | 0x00030000)

#define FIsSelectGesture(syv)	\
   ((syv) >= SYVSELECTFIRST && (syv) <= SYVSELECTLAST)

#define FIsStdGesture(syv)		\
   (									\
   FIsSelectGesture(syv)		\
   || (syv)==SYV_CLEAR			\
   || (syv)==SYV_HELP			\
   || (syv)==SYV_EXTENDSELECT	\
   || (syv)==SYV_UNDO			\
   || (syv)==SYV_COPY			\
   || (syv)==SYV_CUT				\
   || (syv)==SYV_PASTE			\
   || (syv)==SYV_CLEARWORD		\
   || (syv)==SYV_KKCONVERT		\
   || (syv)==SYV_USER			\
   || (syv)==SYV_CORRECT		\
   )

#define FIsAnsiGesture(syv)	\
   (									\
   (syv) == SYV_BACKSPACE		\
   || (syv) == SYV_TAB			\
   || (syv) == SYV_RETURN		\
   || (syv) == SYV_SPACE		\
   )

#endif /*!NOPENHRC */      

//---------------------------------------------------------------------------
#ifndef NOPENINKPUT

#define EventRefFromLparam(lParam)        (LOWORD((lParam)))
#define TerminationFromLparam(lParam)     ((int)(LOWORD((lParam))))
#define HpcmFromLparam(lParam)            ((HPCM)HIWORD((lParam)))
#endif   /*!NOPENINKPUT*/

//---------------------------------------------------------------------------
#ifndef NOPENTARGET
#define HwndFromHtrg(htrg)              ((HWND)(htrg))
#define HtrgFromHwnd(hwnd)               ((HTRG)(struct hwnd__ FAR*)(hwnd))
#endif /*!NOPENTARGET*/



//////////////////////////////////////////////////////////////////////////////
/****** Typedefs ************************************************************/


// Simple:
typedef LONG						ALC;		// Enabled Alphabet
typedef int							CL;		// Confidence Level
typedef UINT						HKP;		// Hook Parameter
typedef int							REC;		// recognition result
typedef LONG						SYV;		// Symbol Value
typedef DWORD						HTRG;		// handle to target

DECLARE_HANDLE(HPCM);					   // Handle to Pen Collection Info
DECLARE_HANDLE(HPENDATA);					// handle to ink
DECLARE_HANDLE(HREC);						// handle to recognizer

// Pointer Types:
typedef ALC FAR*					LPALC;			// ptr to ALC
typedef LPVOID						LPOEM;			// alias
typedef SYV FAR*					LPSYV;			// ptr to SYV
typedef HPENDATA FAR*			LPHPENDATA;		// ptr to HPENDATA

// Function Prototypes:
typedef int			(CALLBACK *ENUMPROC)(LPSYV, int, VOID FAR*);
typedef int			(CALLBACK *LPDF)(int, LPVOID, LPVOID, int, DWORD, DWORD);
typedef BOOL		(CALLBACK *RCYIELDPROC)(VOID);



// Structures:

#ifdef WINPAD
typedef struct tagRECTL
	{
   LONG left;
   LONG top;
   LONG right;
   LONG bottom;
	}
	RECTL, *PRECTL, NEAR *NPRECTL, FAR *LPRECTL, const FAR *LPCRECTL;
#endif //WINPAD

typedef struct tagABSTIME		// 2.0 absolute date/time
	{
	DWORD sec;	// number of seconds since 1/1/1970, ret by CRTlib time() fn
	WORD ms;		// additional offset in ms, 0..999 (NB WORD not UINT)
	}
	ABSTIME, FAR *LPABSTIME;

//---------------------------------------------------------------------------
#ifndef NOPENHEDIT

typedef struct tagCTLINITHEDIT // 2.0 init struct for (h)edit
   {
	DWORD cbSize;					// sizeof(CTLINITHEDIT)
   HWND hwnd;						// (h)edit window handle
   int id;							// its id
   DWORD dwFlags;             // CIE_xx
   DWORD dwReserved;				// for future use
   }
   CTLINITHEDIT, FAR *LPCTLINITHEDIT;
#endif /* !NOPENHEDIT */


//---------------------------------------------------------------------------
#ifndef NOPENBEDIT

typedef struct tagBOXLAYOUT	 // 1.0 box edit layout
   {
   int cyCusp;      				 // pixel height of box (BXS_RECT) or cusp
   int cyEndCusp;					 // pixel height of cusps at extreme ends
   UINT style;      				 // BXS_xx style
   DWORD dwReserved1;			 // reserved
   DWORD dwReserved2;			 // reserved
   DWORD dwReserved3;			 // reserved
   }
   BOXLAYOUT, FAR *LPBOXLAYOUT;

typedef struct tagIMECOLORS	// 2.0 IME undetermined string color info.
	{
	int cColors;					// count of colors to be set/get
	LPINT lpnElem;					// address of array of elements
	COLORREF FAR *lprgbIme;		// address of array of RGB values
	}
	IMECOLORS, FAR *LPIMECOLORS;

typedef struct tagCTLINITBEDIT // 2.0 init struct for box edit
   {
	DWORD cbSize;					// sizeof(CTLINITBEDIT)
   HWND hwnd;						// box edit window handle
   int id;							// its id
   WORD wSizeCategory;			// BESC_xx
   WORD wFlags;               // CIB_xx
   WORD wReserved;				// for future use
   }
   CTLINITBEDIT, FAR *LPCTLINITBEDIT;

typedef struct tagBOXEDITINFO	 // 1.1 box edit Size Info
   {
   int cxBox;						 // width of a single box
   int cyBox;						 // ditto height
   int cxBase;						 // in-box x-margin to guideline
   int cyBase;						 // in-box y offset from top to baseline
   int cyMid;						 // 0 or distance from baseline to midline
   BOXLAYOUT boxlayout;			 // embedded BOXLAYOUT structure
   UINT wFlags;					 // BEIF_xx
   BYTE szFaceName[BEI_FACESIZE];	// font face name
   UINT wFontHeight;				 // font height
   UINT rgwReserved[8];			 // for future use
   }
   BOXEDITINFO, FAR *LPBOXEDITINFO;
#endif /*!NOPENBEDIT */


//---------------------------------------------------------------------------
#ifndef NOPENCTL

typedef struct tagRECTOFS		 // 1.0 rectangle offset for nonisometric inflation
   {
   int dLeft;						 // inflation leftwards from left side
   int dTop;						 // ditto upwards from top
   int dRight;						 // ditto rightwards from right
   int dBottom;					 // ditto downwards from bottom
   }
   RECTOFS, FAR *LPRECTOFS;
#endif /*!NOPENCTL */


//---------------------------------------------------------------------------
#ifndef NOPENDATA

typedef struct tagPENDATAHEADER	// 1.0 main pen data header
   {
   UINT wVersion;					 // pen data format version
   UINT cbSizeUsed;        	 // size of pendata mem block in bytes
   UINT cStrokes;          	 // number of strokes (incl up-strokes)
   UINT cPnt;              	 // count of all points
   UINT cPntStrokeMax;			 // length (in points) of longest stroke
   RECT rectBound;				 // bounding rect of all down points
   UINT wPndts;            	 // PDTS_xx bits
   int  nInkWidth;				 // ink width in pixels
   DWORD rgbInk;					 // ink color
   }
   PENDATAHEADER, FAR *LPPENDATAHEADER, FAR *LPPENDATA;

typedef struct tagSTROKEINFO	 // 1.0 stroke header
   {
   UINT cPnt;        			 // count of points in stroke
   UINT cbPnts;    				 // size of stroke in bytes
   UINT wPdk;        			 // state of stroke
   DWORD dwTick;    				 // time at beginning of stroke
   }
   STROKEINFO, FAR *LPSTROKEINFO;

typedef struct tagPENTIP		// 2.0 Pen Tip characteristics
	{
	DWORD		cbSize;				// sizeof(PENTIP)
   BYTE     btype;         	// pen type/nib (calligraphic nib, etc.)
 	BYTE		bwidth;				// width of Nib (typically == nInkWidth)
 	BYTE		bheight;				// height of Nib
	BYTE		bOpacity;			// 0=transparent, 0x80=hilite, 0xFF=opaque
   COLORREF rgb;	         	// pen color
	DWORD		dwFlags;				// TIP_xx flags
	DWORD 	dwReserved;			// for future expansion
	}
	PENTIP, FAR *LPPENTIP;


typedef BOOL (CALLBACK *ANIMATEPROC)(HPENDATA, UINT, UINT, UINT FAR*, LPARAM);

typedef struct tagANIMATEINFO	// 2.0 Animation parameters
	{
	DWORD		cbSize;				// sizeof(ANIMATEINFO)
	UINT		uSpeedPct;			// speed percent to animate at
	UINT		uPeriodCB;			// time between calls to callback in ms
	UINT		fuFlags;				// animation flags
	LPARAM	lParam;				// value to pass to callback
	DWORD		dwReserved;			// reserved
	}
	ANIMATEINFO, FAR *LPANIMATEINFO;
#endif /*!NOPENDATA */


//---------------------------------------------------------------------------
#ifndef NOPENDRIVER

typedef struct tagOEMPENINFO	// 1.0 OEM pen/tablet hdwe info
   {
   UINT wPdt;						// pen data type
   UINT wValueMax;				// largest val ret by device
   UINT wDistinct;				// number of distinct readings possible
   }
   OEMPENINFO, FAR *LPOEMPENINFO;

typedef struct tagPENPACKET	// 1.0 pen packet
   {
   UINT wTabletX;					// x in raw coords
   UINT wTabletY;					// ditto y
   UINT wPDK;						// state bits
   UINT rgwOemData[MAXOEMDATAWORDS];	// OEM-specific data
   }
   PENPACKET, FAR *LPPENPACKET;

typedef struct tagOEM_PENPACKET	// 2.0 
   {
   UINT wTabletX;					// x in raw coords
   UINT wTabletY;					// ditto y
   UINT wPDK;						// state bits
   UINT rgwOemData[MAXOEMDATAWORDS];	// OEM-specific data
   DWORD dwTime;
   }
	OEM_PENPACKET, FAR *LPOEM_PENPACKET;

typedef struct tagPENINFO		// 1.0 pen/tablet hdwe info
   {
   UINT cxRawWidth;       		// max x coord and tablet width in 0.001"
   UINT cyRawHeight;       	// ditto y, height
   UINT wDistinctWidth;   		// number of distinct x values tablet ret
   UINT wDistinctHeight;  		// ditto y
   int nSamplingRate; 			// samples / second
   int nSamplingDist; 			// min distance to move before generating event
   LONG lPdc;        			// Pen Device Capabilities
   int cPens;        			// number of pens supported
   int cbOemData;    			// width of OEM data packet
   OEMPENINFO rgoempeninfo[MAXOEMDATAWORDS];	// supported OEM data types
   UINT rgwReserved[7];     	// for internal use
   UINT fuOEM;						// which OEM data, timing, PDK_xx to report
   }
   PENINFO, FAR *LPPENINFO;

typedef struct tagCALBSTRUCT  // 1.0 pen calibration
   {
   int wOffsetX;
   int wOffsetY;
   int wDistinctWidth;
   int wDistinctHeight;
   }
   CALBSTRUCT, FAR *LPCALBSTRUCT;

typedef BOOL		(CALLBACK *LPFNRAWHOOK)(LPPENPACKET);
#endif /*!NOPENDRIVER */


//---------------------------------------------------------------------------
#ifndef NOPENHRC

// Handwriting Recognizer:
typedef DWORD						HRC;			// Handwriting Recognition Context
typedef DWORD						HRCRESULT;	// HRC result
typedef DWORD						HWL;			// Handwriting wordlist

typedef HRC							FAR *LPHRC;
typedef HRCRESULT					FAR *LPHRCRESULT;
typedef HWL							FAR *LPHWL;

typedef BOOL (CALLBACK *HRCRESULTHOOKPROC)
	(HREC, HRC, UINT, UINT, UINT, LPVOID);

// Inksets:
DECLARE_HANDLE(HINKSET);								// handle to an inkset
typedef HINKSET					FAR* LPHINKSET;	// ptr to HINKSET

typedef struct tagINTERVAL		// 2.0 interval structure for inksets
	{
	ABSTIME atBegin;				// begining of 1-ms granularity interval
	ABSTIME atEnd;					// 1 ms past end of interval
	}
	INTERVAL, FAR *LPINTERVAL;

typedef struct tagBOXRESULTS	// 2.0 
	{
   int indxBox;
   HINKSET hinksetBox;
   SYV rgSyv[1];
	}
	BOXRESULTS, FAR *LPBOXRESULTS;

typedef struct tagGUIDE			 // 1.0 guide structure
   {
   int xOrigin;    				 // left edge of first box (screen coord))
   int yOrigin;					 // ditto top edge
   int cxBox;						 // width of a single box
   int cyBox;						 // ditto height
   int cxBase;						 // in-box x-margin to guideline
   int cyBase;						 // in-box y offset from top to baseline
   int cHorzBox;					 // count of boxed columns
   int cVertBox;					 // ditto rows
   int cyMid;						 // 0 or distance from baseline to midline
   }
   GUIDE, FAR *LPGUIDE;

#endif /*!NOPENHRC */

//---------------------------------------------------------------------------
#ifndef NOPENIEDIT

#ifndef WINPAD
typedef struct tagCTLINITIEDIT		// 2.0 init struct for Ink Edit
	{
	DWORD		cbSize;				// sizeof(CTLINITIEDIT)
	HWND		hwnd;					// IEdit window handle
	int		id;					// its ID
	WORD		ieb;					// IEB_* (background) bits
	WORD		iedo;					// IEDO_* (draw options) bits
	WORD		iei;					// IEI_* (ink input) bits
	WORD		ien;					// IEN_* (notification) bits
	WORD		ierec;				// IEREC_* (recognition) bits
	WORD		ies;					// IES_* (style) bits
	WORD		iesec;				// IESEC_* (security) bits
	HPENDATA	hpndt;				// initial pendata (or NULL if none)
	WORD		pdts;					// initial pendata scale factor (PDTS_*)
	HGDIOBJ	hgdiobj;				// background brush or bitmap handle
	HPEN		hpenGrid;			// pen to use in drawing grid
	POINT		ptOrgGrid;			// grid lines point of origin
	WORD		wVGrid;				// vertical gridline spacing
	WORD		wHGrid;				// horizontal gridline spacing
	DWORD		dwApp;				// application-defined data
	DWORD		dwReserved;			// reserved for future use
	}
	CTLINITIEDIT, FAR *LPCTLINITIEDIT;
#endif //!WINPAD

typedef struct tagPDEVENT		// 2.0 
	{
	DWORD		cbSize;				// sizeof(PDEVENT)
	HWND		hwnd;					// window handle of I-Edit
	UINT		wm;					// WM_* (window message) of event
	WPARAM	wParam;				// wParam of message
	LPARAM	lParam;				// lParam of message
	POINT		pt;					// event pt in I-Edit client co-ords
	BOOL		fPen;					// TRUE if pen (or other inking device)
	LONG		lExInfo;				// GetMessageExtraInfo() return value
	DWORD		dwReserved;			// for future use
	}
	PDEVENT, FAR *LPPDEVENT;

typedef struct tagSTRKFMT		// 2.0 
	{
	DWORD		cbSize;				// sizeof(STRKFMT)
	WORD		iesf;					// stroke format flags and return bits
	UINT		iStrk;				// stroke index if IESF_STROKE
	PENTIP	tip;					// ink tip attributes
	DWORD		dwUser;				// user data for strokes
	DWORD		dwReserved;			// for future use
	}
	STRKFMT, FAR *LPSTRKFMT;
#endif /*!NOPENIEDIT */


//---------------------------------------------------------------------------
#ifndef NOPENINKPUT

typedef struct tagPCMINFO		// 2.0 Pen Collection Mode Information
   {
	DWORD cbSize;					// sizeof(PCMINFO)
   DWORD dwPcm;					// PCM_xxx flags
   RECT  rectBound;				// if finish on pendown outside this rect
   RECT  rectExclude;			// if finish on pendown inside this rect
   HRGN  hrgnBound;				// if finish on pendown outside this region
   HRGN  hrgnExclude;			// if finish on pendown inside this region
   DWORD dwTimeout;				// if finish after timeout, this many ms
   }
	PCMINFO, FAR *LPPCMINFO;

typedef struct tagINKINGINFO	// 2.0 Pen Inking Information
   {
	DWORD    cbSize;				// sizeof(INKINGINFO)
   UINT     wFlags;				// One of the PII_xx flags
	PENTIP	tip;					// Pen type, size and color
   RECT     rectClip;      	// Clipping rect for the ink
   RECT     rectInkStop;   	// Rect in which a pen down stops inking
   HRGN     hrgnClip;      	// Clipping region for the ink
   HRGN     hrgnInkStop;   	// Region in which a pen down stops inking
   } 
	INKINGINFO, FAR *LPINKINGINFO;
#endif /*!NOPENINKPUT */


//---------------------------------------------------------------------------
#ifndef NOPENRC1	// not available in WINPAD

typedef struct tagSYC			// 1.0 Symbol Correspondence for Ink
   {
   UINT wStrokeFirst;			// first stroke, inclusive
   UINT wPntFirst;				// first point in first stroke, inclusive
   UINT wStrokeLast;				// last stroke, inclusive
   UINT wPntLast;					// last point in last stroke, inclusive
   BOOL fLastSyc;					// T: no more SYCs follow for current SYE
   }
   SYC, FAR *LPSYC;
    

typedef struct tagSYE			// 1.0 Symbol Element
   {
   SYV syv;							// symbol value
   LONG lRecogVal;				// for internal use by recognizer
   CL cl;							// confidence level
   int iSyc;						// SYC index
   }
   SYE, FAR *LPSYE;


typedef struct tagSYG			// 1.0 Symbol Graph
   {
   POINT rgpntHotSpots[MAXHOTSPOT]; // hot spots (max 8)
   int cHotSpot;					// number of valid hot spots in rgpntHotSpots
   int nFirstBox;					// row-major index to box of 1st char in result
   LONG lRecogVal;				// reserved for use by recoognizer
   LPSYE lpsye;					// nodes of symbol graph
   int cSye;						// number of SYEs in symbol graph
   LPSYC lpsyc;					// ptr to corresp symbol ink
   int cSyc;						// ditto count
   }
   SYG, FAR *LPSYG;


typedef struct tagRC				// 1.0 Recognition Context (RC)
   {
   HREC hrec;						// handle of recognizer to use
   HWND hwnd;						// window to send results to
   UINT wEventRef;        		// index into ink buffer
   UINT wRcPreferences;			// flags: RCP_xx Preferences
   LONG lRcOptions;				// RCO_xx options
   RCYIELDPROC lpfnYield;		// procedure called during Yield()
   BYTE lpUser[cbRcUserMax];	// current writer
   UINT wCountry;					// country code
   UINT wIntlPreferences;		// flags: RCIP_xx
   char lpLanguage[cbRcLanguageMax];	// language strings
   LPDF rglpdf[MAXDICTIONARIES];			// list of dictionary functions
   UINT wTryDictionary;			// max enumerations to search
   CL clErrorLevel;				// level where recognizer should reject input
   ALC alc;							// enabled alphabet
   ALC alcPriority;				// prioritizes the ALC_ codes
   BYTE rgbfAlc[cbRcrgbfAlcMax];	// bit field for enabled characters
   UINT wResultMode;				// RRM_xx when to send (asap or when complete)
   UINT wTimeOut;					// recognition timeout in ms
   LONG lPcm;						// flags: PCM_xx for ending recognition
   RECT rectBound;				// bounding rect for inking (def:screen coords)
   RECT rectExclude;				// pen down inside this terminates recognition
   GUIDE guide;					// struct: defines guidelines for recognizer
   UINT wRcOrient;				// RCOR_xx orientation of writing wrt tablet
   UINT wRcDirect;				// RCD_xx direction of writing
   int nInkWidth;					// ink width 0 (none) or 1..15 pixels
   COLORREF rgbInk;				// ink color
   DWORD dwAppParam;				// for application use
   DWORD dwDictParam;			// for app use to be passed on to dictionaries
   DWORD dwRecognizer;			// for app use to be passed on to recognizer
   UINT rgwReserved[cwRcReservedMax];	// reserved for future use by Windows
   }
   RC, FAR *LPRC;


typedef struct tagRCRESULT		// 1.0 Recognition Result
   {
   SYG syg;							// symbol graph
   UINT wResultsType;			// see RCRT_xx
   int cSyv;						// count of symbol values
   LPSYV lpsyv;					// NULL-term ptr to recog's best guess
   HANDLE hSyv;					// globally-shared handle to lpsyv mem
   int nBaseLine;					// 0 or baseline of input writing
   int nMidLine;					// ditto midline
   HPENDATA hpendata;			// pen data mem
   RECT rectBoundInk;			// ink data bounds
   POINT pntEnd;					// pt that terminated recog
   LPRC lprc;						// recog context used
   }
   RCRESULT, FAR *LPRCRESULT;

typedef int			(CALLBACK *LPFUNCRESULTS)(LPRCRESULT, REC);

#endif /*!NOPENRC1 */


//---------------------------------------------------------------------------
#ifndef NOPENTARGET

typedef struct tagTARGET		// 2.0 Geometry for a single target.
   {
   DWORD dwFlags;					// individual target flags
   WORD idTarget;					// TARGINFO.rgTarget[] index
   HTRG  htrgTarget;				// DWORD equiv
   RECTL rectBound;				// Bounding rect of the target
   DWORD dwData;					// data collection info per target
   RECTL rectBoundInk;			// Reserved for internal use, must be zero
   RECTL rectBoundLastInk;		// Reserved for internal use, must be zero
   }
   TARGET, FAR *LPTARGET;

typedef struct tagTARGINFO		// 2.0 A set of targets
   {
	DWORD cbSize;					// sizeof(TARGINFO)
   DWORD dwFlags;					// flags
   HTRG htrgOwner;				// DWORD equiv
   WORD cTargets;					// count of targets
   WORD iTargetLast;				// last target, used by TargetPoints API
										// if TPT_TEXTUAL flag is set
   TARGET rgTarget[1];			// variable-length array of targets
   }
   TARGINFO, FAR *LPTARGINFO;

typedef struct tagINPPARAMS	// 2.0 
   {
   DWORD cbSize;              // sizeof(INPPARAMS)
   DWORD dwFlags;
   HPENDATA hpndt;
   TARGET target;					// target structure
   }
	INPPARAMS, FAR *LPINPPARAMS;
#endif /*!NOPENTARGET */


//////////////////////////////////////////////////////////////////////////////
/****** APIs and Prototypes *************************************************/
																										

LRESULT CALLBACK	DefPenWindowProc(HWND, UINT, WPARAM, LPARAM);

//---------------------------------------------------------------------------
#ifndef NOPENAPPS	// not available in WINPAD

// Pen System Applications:
BOOL		WINAPI ShowKeyboard(HWND, UINT, LPPOINT, LPSKBINFO);

#endif /*!NOPENAPPS */

//---------------------------------------------------------------------------
#ifndef NOPENDATA	// these APIs are implemented in PKPD.DLL

// PenData:
HPENDATA	WINAPI AddPointsPenData(HPENDATA, LPPOINT, LPVOID, LPSTROKEINFO);
LPPENDATA WINAPI BeginEnumStrokes(HPENDATA);
HPENDATA	WINAPI CompactPenData(HPENDATA, UINT);
int		WINAPI CompressPenData(HPENDATA, UINT, DWORD);
HPENDATA	WINAPI CreatePenData(LPPENINFO, int, UINT, UINT);
HPENDATA	WINAPI CreatePenDataEx(LPPENINFO, UINT, UINT, UINT);
HRGN		WINAPI CreatePenDataRegion(HPENDATA, UINT);
BOOL		WINAPI DestroyPenData(HPENDATA);
VOID		WINAPI DrawPenData(HDC, LPRECT, HPENDATA);
int		WINAPI DrawPenDataEx(HDC, LPRECT, HPENDATA, UINT, UINT, UINT, UINT,
						ANIMATEPROC, LPANIMATEINFO, UINT);
HPENDATA	 WINAPI DuplicatePenData(HPENDATA, UINT);
LPPENDATA WINAPI EndEnumStrokes(HPENDATA);
int		WINAPI ExtractPenDataPoints(HPENDATA, UINT, UINT, UINT, LPPOINT,
						LPVOID, UINT);
int		WINAPI ExtractPenDataStrokes(HPENDATA, UINT, LPARAM,
						LPHPENDATA, UINT);
int		WINAPI GetPenDataAttributes(HPENDATA, LPVOID, UINT);
BOOL		WINAPI GetPenDataInfo(HPENDATA, LPPENDATAHEADER, LPPENINFO, DWORD);
BOOL		WINAPI GetPenDataStroke(LPPENDATA, UINT, LPPOINT FAR*,
						LPVOID FAR*,	LPSTROKEINFO );
BOOL		WINAPI GetPointsFromPenData(HPENDATA, UINT, UINT, UINT, LPPOINT);
int		WINAPI GetStrokeAttributes(HPENDATA, UINT, LPVOID, UINT);
int		WINAPI GetStrokeTableAttributes(HPENDATA, UINT, LPVOID, UINT);
int		WINAPI HitTestPenData(HPENDATA, LPPOINT, UINT, UINT FAR*, UINT FAR*);
int		WINAPI InsertPenData(HPENDATA, HPENDATA, UINT);
int		WINAPI InsertPenDataPoints(HPENDATA, UINT, UINT, UINT,
						LPPOINT, LPVOID);
int		WINAPI InsertPenDataStroke(HPENDATA, UINT, LPPOINT, LPVOID, 
						LPSTROKEINFO);
BOOL		WINAPI MetricScalePenData(HPENDATA, UINT);
BOOL		WINAPI OffsetPenData(HPENDATA, int, int);
int		WINAPI PenDataFromBuffer(LPHPENDATA, UINT, LPBYTE, int, LPDWORD);
int		WINAPI PenDataToBuffer(HPENDATA, LPBYTE, int, LPDWORD);
BOOL		WINAPI RedisplayPenData(HDC, HPENDATA, LPPOINT, LPPOINT,
						int, DWORD);
int		WINAPI RemovePenDataStrokes(HPENDATA, UINT, UINT);
BOOL		WINAPI ResizePenData(HPENDATA, LPRECT);
int		WINAPI SetStrokeAttributes(HPENDATA, UINT, LPARAM, UINT);
int		WINAPI SetStrokeTableAttributes(HPENDATA, UINT, LPARAM, UINT);
int		WINAPI TrimPenData(HPENDATA, DWORD, DWORD);


#endif /*!NOPENDATA */

//---------------------------------------------------------------------------
#ifndef NOPENDICT	// not available in WINPAD

// Dictionary:
BOOL		WINAPI DictionarySearch(LPRC, LPSYE, int, LPSYV, int);
#endif /*!NOPENDICT */

//---------------------------------------------------------------------------
#ifndef NOPENDRIVER

// Pen Hardware/Driver:
BOOL		WINAPI EndPenCollection(REC);
BOOL		WINAPI GetPenAsyncState(UINT);
REC		WINAPI GetPenHwData(LPPOINT, LPVOID, int, UINT, LPSTROKEINFO);
REC		WINAPI GetPenHwEventData(UINT, UINT, LPPOINT, LPVOID,
						int, LPSTROKEINFO);
BOOL		WINAPI IsPenEvent(UINT, LONG);
BOOL		WINAPI SetPenHook(HKP, LPFNRAWHOOK);
VOID		WINAPI UpdatePenInfo(LPPENINFO);
#endif /*!NOPENDRIVER */

//---------------------------------------------------------------------------
#ifndef NOPENGEST	// not available in WINPAD

// Gesture Management:
BOOL		WINAPI ExecuteGesture(HWND, SYV, LPRCRESULT);
#endif /*!NOPENGEST */

//---------------------------------------------------------------------------
#ifndef NOPENHRC

// Handwriting Recognizer:
int		WINAPI AddPenDataHRC(HRC, HPENDATA);
int		WINAPI AddPenInputHRC(HRC, LPPOINT, LPVOID, UINT, LPSTROKEINFO);
int		WINAPI AddWordsHWL(HWL, LPSTR, UINT);
int		WINAPI ConfigHREC(HREC, UINT, WPARAM, LPARAM);
HRC		WINAPI CreateCompatibleHRC(HRC, HREC);
HWL		WINAPI CreateHWL(HREC, LPVOID, UINT, DWORD);
HINKSET	WINAPI CreateInksetHRCRESULT(HRCRESULT, UINT, UINT);
HPENDATA WINAPI CreatePenDataHRC(HRC);
int		WINAPI DestroyHRC(HRC);
int		WINAPI DestroyHRCRESULT(HRCRESULT);
int		WINAPI DestroyHWL(HWL);
int		WINAPI EnableGestureSetHRC(HRC, SYV, BOOL);
int		WINAPI EnableSystemDictionaryHRC(HRC, BOOL);
int		WINAPI EndPenInputHRC(HRC);
int		WINAPI GetAlphabetHRC(HRC, LPALC, LPBYTE);
int		WINAPI GetAlphabetPriorityHRC(HRC, LPALC, LPBYTE);
int		WINAPI GetAlternateWordsHRCRESULT(HRCRESULT, UINT, UINT,
						LPHRCRESULT, UINT);
int		WINAPI GetBoxMappingHRCRESULT(HRCRESULT, UINT, UINT, UINT FAR*);
int      WINAPI GetBoxResultsHRC(HRC, UINT, UINT, UINT, LPBOXRESULTS, BOOL);
int		WINAPI GetGuideHRC(HRC, LPGUIDE, UINT FAR*);
int		WINAPI GetHotspotsHRCRESULT(HRCRESULT, UINT, LPPOINT, UINT);
HREC		WINAPI GetHRECFromHRC(HRC);
int		WINAPI GetInternationalHRC(HRC, UINT FAR*, LPSTR, UINT FAR*,
						UINT FAR*);
int		WINAPI GetMaxResultsHRC(HRC);
int		WINAPI GetResultsHRC(HRC, UINT, LPHRCRESULT, UINT);
int		WINAPI GetSymbolCountHRCRESULT(HRCRESULT);
int		WINAPI GetSymbolsHRCRESULT(HRCRESULT, UINT, LPSYV, UINT);
int		WINAPI GetWordlistHRC(HRC, LPHWL);
int		WINAPI GetWordlistCoercionHRC(HRC);
int		WINAPI ProcessHRC(HRC, DWORD);
int		WINAPI ReadHWL(HWL, HFILE);
int		WINAPI SetAlphabetHRC(HRC, ALC, LPBYTE);
int		WINAPI SetAlphabetPriorityHRC(HRC, ALC, LPBYTE);
int		WINAPI SetBoxAlphabetHRC(HRC, LPALC, UINT);
int		WINAPI SetGuideHRC(HRC, LPGUIDE, UINT);
int		WINAPI SetInternationalHRC(HRC, UINT, LPCSTR, UINT, UINT);
int		WINAPI SetMaxResultsHRC(HRC, UINT);
int		WINAPI SetResultsHookHREC(HREC, HRCRESULTHOOKPROC);
int		WINAPI SetWordlistCoercionHRC(HRC, UINT);
int		WINAPI SetWordlistHRC(HRC, HWL);
int		WINAPI TrainHREC(HREC, LPSYV, UINT, HPENDATA, UINT);
int		WINAPI UnhookResultsHookHREC(HREC, HRCRESULTHOOKPROC);
int		WINAPI WriteHWL(HWL, HFILE);

// Recognizer Installation:
HREC		WINAPI InstallRecognizer(LPSTR);
VOID		WINAPI UninstallRecognizer(HREC);

// Inksets:
BOOL		WINAPI AddInksetInterval(HINKSET, LPINTERVAL);
HINKSET	WINAPI CreateInkset(UINT);
BOOL		WINAPI DestroyInkset(HINKSET);
int		WINAPI GetInksetInterval(HINKSET, UINT, LPINTERVAL);
int		WINAPI GetInksetIntervalCount(HINKSET);

// Symbol Values:
int		WINAPI CharacterToSymbol(LPSTR, int, LPSYV);
BOOL		WINAPI SymbolToCharacter(LPSYV, int, LPSTR, LPINT);
#endif /*!NOPENHRC */

//---------------------------------------------------------------------------
#ifndef NOPENINKPUT

// Pen Input/Inking:
int		WINAPI DoDefaultPenInput(HWND, UINT);
int		WINAPI GetPenInput(HPCM, LPPOINT, LPVOID, UINT, UINT, LPSTROKEINFO);
int		WINAPI PeekPenInput(HPCM, UINT, LPPOINT, LPVOID, UINT);
int		WINAPI StartInking(HPCM, UINT, LPINKINGINFO);
HPCM		WINAPI StartPenInput(HWND, UINT, LPPCMINFO, LPINT);
int		WINAPI StopInking(HPCM);
int		WINAPI StopPenInput(HPCM, UINT, int);
#endif /*!NOPENINKPUT */

//---------------------------------------------------------------------------
#ifndef NOPENMISC

// Miscellaneous/Utilities:
VOID		WINAPI BoundingRectFromPoints(LPPOINT, int, LPRECT);
BOOL		WINAPI DPtoTP(LPPOINT, int);
UINT		WINAPI GetPenAppFlags(VOID);
LONG		WINAPI GetPenMiscInfo(WPARAM, LPARAM);
UINT		WINAPI GetVersionPenWin(VOID);
UINT		WINAPI IsPenAware(VOID);
VOID		WINAPI RegisterPenApp(UINT, UINT);
LONG		WINAPI SetPenMiscInfo(WPARAM, LPARAM);
BOOL		WINAPI TPtoDP(LPPOINT, int);

#ifndef WINPAD
BOOL		WINAPI CorrectWriting(HWND, LPSTR, UINT, LPVOID, DWORD, DWORD);
#endif //!WINPAD

#endif /*!NOPENMISC */

//---------------------------------------------------------------------------
#ifndef NOPENRC1	// not available in WINPAD

// RC1:
VOID		WINAPI EmulatePen(BOOL);
UINT		WINAPI EnumSymbols(LPSYG, UINT, ENUMPROC, LPVOID);
VOID		WINAPI FirstSymbolFromGraph(LPSYG, LPSYV, int, LPINT);
UINT		WINAPI GetGlobalRC(LPRC, LPSTR, LPSTR, int);
int		WINAPI GetSymbolCount(LPSYG);
int		WINAPI GetSymbolMaxLength(LPSYG);
VOID		WINAPI InitRC(HWND, LPRC);
REC		WINAPI ProcessWriting(HWND, LPRC);    
REC		WINAPI Recognize(LPRC);
REC		WINAPI RecognizeData(LPRC, HPENDATA);
UINT		WINAPI SetGlobalRC(LPRC, LPSTR, LPSTR);
BOOL		WINAPI SetRecogHook(UINT, UINT, HWND);
BOOL		WINAPI TrainContext(LPRCRESULT, LPSYE, int, LPSYC, int);
BOOL		WINAPI TrainInk(LPRC, HPENDATA, LPSYV);

// Custom Recognizer functions - not PenWin APIs (formerly in penwoem.h):
VOID		WINAPI CloseRecognizer(VOID);
UINT		WINAPI ConfigRecognizer(UINT, WPARAM, LPARAM);
BOOL		WINAPI InitRecognizer(LPRC);
REC 		WINAPI RecognizeDataInternal(LPRC, HPENDATA, LPFUNCRESULTS);
REC 		WINAPI RecognizeInternal(LPRC, LPFUNCRESULTS);
BOOL		WINAPI TrainContextInternal(LPRCRESULT, LPSYE, int, LPSYC, int);
BOOL		WINAPI TrainInkInternal(LPRC, HPENDATA, LPSYV);
#endif /*!NOPENRC1 */

//---------------------------------------------------------------------------
#ifndef NOPENTARGET

// Ink Targeting:
int		WINAPI TargetPoints(LPTARGINFO, LPPOINT, DWORD, UINT, LPSTROKEINFO);

#endif /*!NOPENTARGET */


//---------------------------------------------------------------------------
#ifndef NOPENVIRTEVENT

// Virtual Event Layer:
VOID		WINAPI AtomicVirtualEvent(BOOL);
VOID		WINAPI PostVirtualKeyEvent(UINT, BOOL);
VOID		WINAPI PostVirtualMouseEvent(UINT, int, int);
#endif /*!NOPENVIRTEVENT */

//---------------------------------------------------------------------------

#ifdef  JAPAN
// Kanji
BOOL		WINAPI KKConvert(HWND hwndConvert, HWND hwndCaller,
						LPSTR lpBuf, UINT cbBuf, LPPOINT lpPnt);
#endif //  JAPAN


#endif /* RC_INVOKED */	// ... all the way back from definitions:3

/****** End of Header Info *************************************************/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif /* RC_INVOKED */

#endif /* #define _INC_PENWIN */
