#ifndef LSCBK_DEFINED
#define LSCBK_DEFINED

/* LineServices callbacks */

#include "lsdefs.h"
#include "lsdevice.h"
#include "lsksplat.h"
#include "lskjust.h"
#include "lstflow.h"
#include "endres.h"
#include "mwcls.h"
#include "lsact.h"
#include "lspract.h"
#include "brkcond.h"
#include "brkcls.h"
#include "gprop.h"
#include "gmap.h"
#include "lsexpinf.h"
#include "lskalign.h"
#include "plstabs.h"
#include "pheights.h"
#include "plsrun.h"
#include "plscbk.h"
#include "plschp.h"
#include "plspap.h"
#include "plstxm.h"
#include "plshyph.h"
#include "plsstinf.h"
#include "plsulinf.h"
#include "plsems.h"

#define cpFirstAnm (-0x7FFFFFFF)   /* Used for the fetch of the first Autonumber run */


struct lscbk	/* Interfaces to application-specific callbacks */
{
	/* Dynamic memory APIs */
	void* (WINAPI* pfnNewPtr)(POLS, DWORD);
	void  (WINAPI* pfnDisposePtr)(POLS, void*);
	void* (WINAPI* pfnReallocPtr)(POLS, void*, DWORD);


	LSERR (WINAPI* pfnFetchRun)(POLS, LSCP,
							    LPCWSTR*, DWORD*, BOOL*, PLSCHP, PLSRUN*);
	/* FetchRun:
	 *  pols (IN):
	 *  cp (IN):
	 *  &lpwchRun (OUT): run of characters.
	 *  &cchRun (OUT): number of characters in run
	 *  &fHidden (OUT) : hidden run?
	 *  &lsChp (OUT): char properties of run 
	 *  &plsrun (OUT): abstract representation of run properties
	 */

	LSERR (WINAPI* pfnGetAutoNumberInfo)(POLS, LSKALIGN*, PLSCHP, PLSRUN*, WCHAR*, PLSCHP, PLSRUN*, BOOL*, long*, long*);

	/* GetAutoNumberInfo:
	 *  pols (IN):
	 *  &lskalAnm (OUT):
	 *  &lschpAnm (OUT): lschp for Anm
	 *  &plsrunAnm (OUT): plsrun for Anm
	 *  &wchAdd (OUT): character to add (Nil is treated as none)
	 *  &lschpWch (OUT): lschp for added char
	 *  &plsrunWch (OUT): plsrun for added char
	 *  &fWord95Model(OUT):
	 *  &duaSpaceAnm(OUT):	relevant iff fWord95Model
	 *  &duaWidthAnm(OUT):	relevant iff fWord95Model
	 */

	LSERR (WINAPI* pfnGetNumericSeparators)(POLS, PLSRUN, WCHAR*,WCHAR*);
	/* GetNumericSeparators:
	 *  pols (IN):
	 *  plsrun (IN): run pointer as returned from FetchRun
	 *  &wchDecimal (OUT): decimal separator for this run.
	 *  &wchThousands (OUT): thousands separator for this run
	 */

	LSERR (WINAPI* pfnCheckForDigit)(POLS, PLSRUN, WCHAR, BOOL*);
	/* GetNumericSeparators:
	 *  pols (IN):
	 *  plsrun (IN): run pointer as returned from FetchRun
	 *  wch (IN): character to check
	 *  &fIsDigit (OUT): this character is digit
	 */

	LSERR (WINAPI* pfnFetchPap)(POLS, LSCP, PLSPAP);
	/* FetchPap:
	 *  pols (IN):
	 *  cp (IN): an arbitrary cp value inside the paragraph
	 *  &lsPap (OUT): Paragraph properties.
	 */

	LSERR (WINAPI* pfnFetchTabs)(POLS, LSCP, PLSTABS, BOOL*, long*, WCHAR*);
	/* FetchTabs:
	 *  pols (IN):
	 *  cp (IN): an arbitrary cp value inside the paragraph
	 *  &lstabs (OUT): tabs array
	 *  &fHangingTab (OUT): there is hanging tab
	 *  &duaHangingTab (OUT): dua of hanging tab
	 *  &wchHangingTabLeader (OUT): leader of hanging tab
	 */

	LSERR (WINAPI* pfnGetBreakThroughTab)(POLS, long, long, long*);
	/* GetBreakThroughTab:
	 *  pols (IN):
	 *  uaRightMargin (IN): right margin for breaking
	 *  uaTabPos (IN): breakthrough tab position
	 *  uaRightMarginNew (OUT): new right margin
	 */

	LSERR (WINAPI* pfnFGetLastLineJustification)(POLS, LSKJUST, LSKALIGN, ENDRES, BOOL*, LSKALIGN*);
	/* FGetLastLineJustification:
	 *  pols (IN):
	 *  lskj (IN): kind of justification for the paragraph
	 *  lskal (IN): kind of alignment for the paragraph
	 *  endr (IN): result of formatting
	 *  &fJustifyLastLine (OUT): should last line be fully justified
	 *  &lskalLine (OUT): kind of alignment for this line
	 */

	LSERR (WINAPI* pfnCheckParaBoundaries)(POLS, LSCP, LSCP, BOOL*);
	/* CheckParaBoundaries:
	 *  pols (IN):
	 *  cpOld (IN):
	 *  cpNew (IN):
	 *  &fChanged (OUT): "Dangerous" change between paragraph properties.
	 */

	LSERR (WINAPI* pfnGetRunCharWidths)(POLS, PLSRUN, 
									 	LSDEVICE, LPCWSTR,
										DWORD, long, LSTFLOW,
										int*,long*,long*);
	/* GetRunCharWidths:
	 *  pols (IN):
	 *  plsrun (IN):
	 *  lsDeviceID (IN): presentation or reference
	 *  lpwchRun (IN): run of characters
	 *  cwchRun (IN): number of characters in run
	 *  du (IN): available space for characters
	 *  kTFlow (IN): text direction and orientation
	 *  rgDu (OUT): widths of characters
	 *  &duRun (OUT): sum of widths in rgDx[0] to rgDu[limDx-1]
	 *  &limDu (OUT): number of widths fetched
	 */

	LSERR (WINAPI* pfnCheckRunKernability)(POLS, PLSRUN,PLSRUN, BOOL*);
	/* CheckRunKernability:
	 *  pols (IN):
	 *  plsrunLeft (IN): 1st of pair of adjacent runs
	 *  plsrunRight (IN): 2nd of pair of adjacent runs
	 *  &fKernable (OUT) : if TRUE, Line Service may kern between these runs
	 */

	LSERR (WINAPI* pfnGetRunCharKerning)(POLS, PLSRUN,
										 LSDEVICE, LPCWSTR,
										 DWORD, LSTFLOW, int*);
	/* GetRunCharKerning:
	 *  pols (IN):
	 *  plsrun (IN):
	 *  lsDeviceID (IN): presentation or reference
	 *  lpwchRun (IN): run of characters
	 *  cwchRun (IN): number of characters in run
	 *  kTFlow (IN): text direction and orientation
	 *  rgDu (OUT): widths of characters
	 */

	LSERR (WINAPI* pfnGetRunTextMetrics)(POLS, PLSRUN,
										 LSDEVICE, LSTFLOW, PLSTXM);
	/* GetRunTextMetrics:
	 *  pols (IN):
	 *  plsrun (IN):
	 *  deviceID (IN):  presentation, reference, or absolute
	 *  kTFlow (IN): text direction and orientation
	 *  &lsTxMet (OUT): Text metrics
	 */

	LSERR (WINAPI* pfnGetRunUnderlineInfo)(POLS, PLSRUN, PCHEIGHTS, LSTFLOW,
										   PLSULINFO);
	/* GetRunUnderlineInfo:
	 *  pols (IN):
	 *  plsrun (IN):
	 *  heightsPres (IN):
	 *  kTFlow (IN): text direction and orientation
	 *  &lsUlInfo (OUT): Underline information
	 */

	LSERR (WINAPI* pfnGetRunStrikethroughInfo)(POLS, PLSRUN, PCHEIGHTS, LSTFLOW,
											  PLSSTINFO);
	/* GetRunStrikethroughInfo:
	 *  pols (IN):
	 *  plsrun (IN):
	 *  heightsPres (IN):
	 *  kTFlow (IN): text direction and orientation
	 *  &lsStInfo (OUT): Strikethrough information
	 */

	LSERR (WINAPI* pfnGetBorderInfo)(POLS, PLSRUN, LSTFLOW, long*, long*);
	/* GetBorderInfo:
	 *  pols (IN):
	 *  plsrun (IN):
	 *  kTFlow (IN): text direction and orientation
	 *  &durBorder (OUT): Width of the border on the reference device
	 *  &dupBorder (OUT): Width of the border on the presentation device
	 */


	LSERR (WINAPI* pfnReleaseRun)(POLS, PLSRUN);
	/* ReleaseRun:
	 *  pols (IN):
	 *  plsrun (IN): run to be released, from GetRun() or FetchRun()
	 */

	LSERR (WINAPI* pfnHyphenate)(POLS, PCLSHYPH, LSCP, LSCP, PLSHYPH);
	/* Hyphenate:
	 *  pols (IN):
	 *  &lsHyphLast (IN): last hyphenation found. kysr==kysrNil means "none"
	 *  cpBeginWord (IN): 1st cp in word which exceeds column
	 *  cpExceed (IN): 1st which exceeds column, in this word
	 *  &lsHyph (OUT): hyphenation results. kysr==kysrNil means "none"
	 */

	LSERR (WINAPI* pfnGetHyphenInfo)(POLS, PLSRUN, DWORD*, WCHAR*);
	/* GetHyphenInfo:
	 *  pols (IN):
	 *  plsrun (IN):
     *  kysr (OUT)	  Ysr type - see "lskysr.h"
     *  wchYsr (OUT)  Character code of YSR
	*/

	LSERR (WINAPI* pfnDrawUnderline)(POLS, PLSRUN, UINT,
								const POINT*, DWORD, DWORD, LSTFLOW,
								UINT, const RECT*);
	/* DrawUnderline:
	 *  pols (IN):
	 *  plsrun (IN): run to use for the underlining
	 *  kUlbase (IN): underline kind 
	 *  pptStart (IN): starting position (top left)
	 *  dupUL (IN): underline width
	 *  dvpUL (IN) : underline thickness
	 *  kTFlow (IN): text direction and orientation
	 *  kDisp (IN) : display mode - opaque, transparent
	 *  prcClip (IN) : clipping rectangle
	 */

	LSERR (WINAPI* pfnDrawStrikethrough)(POLS, PLSRUN, UINT,
								const POINT*, DWORD, DWORD, LSTFLOW,
								UINT, const RECT*);
	/* DrawStrikethrough:
	 *  pols (IN):
	 *  plsrun (IN): the run for the strikethrough
	 *  kStbase (IN): strikethrough kind 
	 *  pptStart (IN): starting position (top left)
	 *  dupSt (IN): strikethrough width
	 *  dvpSt (IN) : strikethrough thickness
	 *  kTFlow (IN): text direction and orientation
	 *  kDisp (IN) : display mode - opaque, transparent
	 *  prcClip (IN) : clipping rectangle
	 */

	LSERR (WINAPI* pfnDrawBorder)(POLS, PLSRUN, const POINT*, PCHEIGHTS, PCHEIGHTS,
								  PCHEIGHTS, PCHEIGHTS, long, long, LSTFLOW, UINT, const RECT*);

	/* DrawBorder:
	 *  pols (IN):
	 *  plsrun (IN): plsrun of the first bordered run
	 *  pptStart (IN): starting point for the border
	 *  pheightsLineFull (IN): height of the line including SpaceBefore & SpaceAfter
	 *  pheightsLineWithoutAddedSpace (IN): height of the line without SpaceBefore & SpaceAfter
	 *  pheightsSubline (IN): height of subline
	 *  pheightsRuns (IN): height of collected runs to be bordered
	 *  dupBorder (IN): width of one border
	 *  dupRunsInclBorders (IN): width of collected runs
	 *  kTFlow (IN): text direction and orientation
	 *  kDisp (IN) : display mode - opaque, transparent
	 *  prcClip (IN) : clipping rectangle
	 */

	LSERR (WINAPI* pfnDrawUnderlineAsText)(POLS, PLSRUN, const POINT*,
										   long, LSTFLOW, UINT, const RECT*);
	/* DrawUnderlineAsText:
	 *  pols (IN):
	 *  plsrun (IN): run to use for the underlining
	 *  pptStart (IN): starting pen position
	 *  dupLine (IN): length of UL
	 *  kTFlow (IN): text direction and orientation
	 *  kDisp (IN) : display mode - opaque, transparent
	 *  prcClip (IN) : clipping rectangle
	 */

	LSERR (WINAPI* pfnFInterruptUnderline)(POLS, PLSRUN, LSCP, PLSRUN, LSCP,BOOL*);
	/* FInterruptUnderline:
	 *  pols (IN):
	 *  plsrunFirst (IN): run pointer for the previous run
	 *  cpLastFirst (IN): cp of the last character of the previous run
	 *  plsrunSecond (IN): run pointer for the current run
	 *  cpStartSecond (IN): cp of the first character of the current run
	 *  &fInterruptUnderline (OUT): do you want to interrupt drawing of the underline between these runs
	 */

	LSERR (WINAPI* pfnFInterruptShade)(POLS, PLSRUN, PLSRUN, BOOL*);
	/* FInterruptShade:
	 *  pols (IN):
	 *  plsrunFirst (IN): run pointer for the previous run
	 *  plsrunSecond (IN): run pointer for the current run
	 *  &fInterruptShade (OUT): do you want to interrupt shading between these runs
	 */

	LSERR (WINAPI* pfnFInterruptBorder)(POLS, PLSRUN, PLSRUN, BOOL*);
	/* FInterruptBorder:
	 *  pols (IN):
	 *  plsrunFirst (IN): run pointer for the previous run
	 *  plsrunSecond (IN): run pointer for the current run
	 *  &fInterruptBorder (OUT): do you want to interrupt border between these runs
	 */


	LSERR (WINAPI* pfnShadeRectangle)(POLS, PLSRUN, const POINT*, PCHEIGHTS, PCHEIGHTS,
								  PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, long, long, LSTFLOW, UINT, const RECT*);

	/* ShadeRectangle:
	 *  pols (IN):
	 *  plsrun (IN): plsrun of the first shaded run
	 *  pptStart (IN): starting point for the shading rectangle
	 *  pheightsLineWithAddSpace(IN): height of the line including SpaceBefore & SpaceAfter (main baseline, 
	 *						lstflow of main line)
	 *  pheightsLineWithoutAddedSpace (IN): height of the line without SpaceBefore & SpaceAfter
	 *  pheightsSubline (IN): height of subline (local baseline, lstflow of subline)
	 *  pheightsRunsExclTrail (IN): height of collected runs to be shaded excluding 
	 *									trailing spaces area (local baseline, lstflow of subline)
	 *  pheightsRunsInclTrail (IN): height of collected runs to be shaded including
	 *									trailing spaces area (local baseline, lstflow of subline)
	 *  dupRunsExclTrail (IN): width of collected runs excluding trailing spaces area
	 *  dupRunsInclTrail (IN): width of collected runs including trailing spaces area
	 *  kTFlow (IN): text direction and orientation of subline
	 *  kDisp (IN) : display mode - opaque, transparent
	 *  prcClip (IN) : clipping rectangle
	 */

	LSERR (WINAPI* pfnDrawTextRun)(POLS, PLSRUN, BOOL, BOOL, 
								   const POINT*, LPCWSTR, const int*, DWORD, 
								   LSTFLOW, UINT, const POINT*, PCHEIGHTS, long, long, const RECT*);
	/* DrawTextRun:
	 *  pols (IN):
	 *  plsrun (IN):
	 *  fStrikeout (IN) :
	 *  fUnderline (IN) :
	 *  pptText (IN): starting point for the text output
	 *  lpwchRun (IN): run of characters
	 *  rgDupRun (IN): widths of characters
	 *  cwchRun (IN): number of characters in run
	 *  kTFlow (IN): text direction and orientation
	 *  kDisp (IN): display mode - opaque, transparent
	 *  pptRun (IN): starting point of the run
	 *  heightsPres (IN): presentation heights for this run
	 *  dupRun (IN): presentation width for this run
	 *  dupLimUnderline (IN): underlining limit
	 *  pRectClip (IN): clipping rectangle
	 */

    LSERR (WINAPI* pfnDrawSplatLine)(POLS, enum lsksplat, LSCP, const POINT*,
									 PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, long, LSTFLOW,
									 UINT, const RECT*);
	/* DrawSplatLine:
	 *  pols (IN):
	 *  ksplat (IN): See definitions in lsksplat.h
	 *  cpSplat (IN): location of the break character which caused the splat.
	 *  pptSplatLine (IN) : starting position of the splat line
	 *  pheightsLineFull (IN): height of the line including SpaceBefore & SpaceAfter
	 *  pheightsLineWithoutAddedSpace (IN): height of the line without SpaceBefore & SpaceAfter
	 *  pheightsSubline (IN): height of subline
	 *  dup (IN): distance to right margin
	 *  kTFlow (IN): text direction and orientation
	 *  kDisp (IN): display mode - opaque, transparent
	 *  &rcClip (IN) : clipping rectangle
	 */


/* Advanced typography enabling API's */

	/* Glyph enabling */

	LSERR (WINAPI* pfnFInterruptShaping)(POLS, LSTFLOW, PLSRUN, PLSRUN, BOOL*);
	/* FInterruptShaping:
	 *  pols (IN):
	 *  kTFlow (IN): text direction and orientation
	 *  plsrunFirst (IN): run pointer for the previous run
	 *  plsrunSecond (IN): run pointer for the current run
	 *  &fInterruptShaping (OUT): do you want to interrupt character shaping between these runs
	*/

	LSERR (WINAPI* pfnGetGlyphs)(POLS, PLSRUN, LPCWSTR, DWORD, LSTFLOW, PGMAP, PGINDEX*, PGPROP*, DWORD*);
	/* GetGlyphs:
	 *  pols (IN):
	 *  plsrun (IN): run pointer of the first run
	 *  pwch (IN): pointer to the string of character codes
	 *  cwch (IN): number of characters to be shaped
	 *  kTFlow (IN): text direction and orientation
	 *  rgGmap (OUT): parallel to the char codes mapping wch->glyph info
	 *  &rgGindex (OUT): array of output glyph indices
	 *  &rgGprop (OUT): array of output glyph properties
	 *  &cgindex (OUT): number of output glyph indices
	 */

	LSERR (WINAPI* pfnGetGlyphPositions)(POLS, PLSRUN, LSDEVICE, LPWSTR, PCGMAP, DWORD,
											PCGINDEX, PCGPROP, DWORD, LSTFLOW, int*, PGOFFSET);
	/* GetGlyphPositions:
	 *  pols (IN):
	 *  plsrun (IN): run pointer of the first run
	 *  lsDeviceID (IN): presentation or reference
	 *  pwch (IN): pointer to the string of character codes
	 *  pgmap (IN): array of wch->glyph mapping
	 *  cwch (IN): number of characters to be shaped
	 *  rgGindex (IN): array of glyph indices
	 *  rgGprop (IN): array of glyph properties
	 *  cgindex (IN): number glyph indices
	 *  kTFlow (IN): text direction and orientation
	 *  rgDu (OUT): array of widths of glyphs
	 *  rgGoffset (OUT): array of offsets of glyphs
	 */

	LSERR (WINAPI* pfnResetRunContents)(POLS, PLSRUN, LSCP, LSDCP, LSCP, LSDCP);
	/* ResetRunContents:
	 *  pols (IN):
	 *  plsrun (IN): run pointer as returned from FetchRun
	 *  cpFirstOld (IN): cpFirst before shaping
	 *  dcpOld (IN): dcp before shaping
	 *  cpFirstNew (IN): cpFirst after shaping
	 *  dcpNew (IN): dcp after shaping
	 */

	LSERR (WINAPI* pfnDrawGlyphs)(POLS, PLSRUN, BOOL, BOOL, PCGINDEX, const int*, const int*,
						PGOFFSET, PGPROP, PCEXPTYPE, DWORD,
						LSTFLOW, UINT, const POINT*, PCHEIGHTS, long, long, const RECT*);
	/* DrawGlyphs:
	 *  pols (IN):
	 *  plsrun (IN): run pointer of the first run
	 *  fStrikeout (IN) :
	 *  fUnderline (IN) :
	 *  pglyph (IN): array of glyph indices
	 *  rgDu (IN): array of widths of glyphs
	 *  rgDuBeforeJust (IN): array of widths of glyphs before justification
	 *  rgGoffset (IN): array of offsets of glyphs
	 *  rgGprop (IN): array of glyph properties
	 *  rgExpType (IN): array of glyph expansion types
	 *  cglyph (IN): number glyph indices
	 *  kTFlow (IN): text direction and orientation
	 *  kDisp (IN): display mode - opaque, transparent
	 *  pptRun (IN): starting point of the run
	 *  heightsPres (IN): presentation heights for this run
	 *  dupRun (IN): presentation width for this run
	 *  dupLimUnderline (IN): underlining limit
	 *  pRectClip (IN): clipping rectangle
	 */

	/* Glyph justification */

	LSERR (WINAPI* pfnGetGlyphExpansionInfo)(POLS, PLSRUN, LSDEVICE, LPCWSTR, PCGMAP, DWORD, 
							PCGINDEX, PCGPROP, DWORD, LSTFLOW, BOOL, PEXPTYPE, LSEXPINFO*);
	/* GetGlyphExpansionInfo:
	 *  pols (IN):
	 *  plsrun (IN): run pointer of the first run
	 *  lsDeviceID (IN): presentation or reference
	 *  pwch (IN): pointer to the string of character codes
	 *  rggmap (IN): array of wchar->glyph mapping
	 *  cwch (IN): number of characters to be shaped
	 *  rgglyph (IN): array of glyph indices
	 *  rgProp (IN): array of glyph properties
	 *  cglyph (IN): number glyph indices
	 *  kTFlow (IN): text direction and orientation
	 *  fLastTextChunkOnLine (IN): Last text chunk on line?
	 *  rgExpType (OUT): array of glyph expansion types
	 *  rgexpinfo (OUT): array of glyph expansion info
	 */

	LSERR (WINAPI* pfnGetGlyphExpansionInkInfo)(POLS, PLSRUN, LSDEVICE, GINDEX, GPROP, LSTFLOW, DWORD, long*);
	/* GetGlyphExpansionInkInfo:
	 *  pols (IN):
	 *  plsrun (IN): run pointer of the first run
	 *  lsDeviceID (IN): presentation or reference
	 *  gindex (IN): glyph index
	 *  gprop (IN): glyph properties
	 *  kTFlow (IN): text direction and orientation
	 *  cAddInkDiscrete (IN): number of discrete values (minus 1, because maximum is already known)
	 *  rgDu (OUT): array of discrete values
	 */

	/* FarEast realted typograpy issues */

	LSERR (WINAPI* pfnGetEms)(POLS, PLSRUN, LSTFLOW, PLSEMS);
	/* GetEms:
	 *  pols (IN):
	 *  plsrun (IN): run pointer as returned from FetchRun
	 *  kTFlow (IN): text direction and orientation
	 *  &lsems (OUT): different fractions of EM in appropriate pixels
	 */

	LSERR (WINAPI* pfnPunctStartLine)(POLS, PLSRUN, MWCLS, WCHAR, LSACT*);
	/* PunctStartLine:
	 *  pols (IN):
	 *  plsrun (IN): run pointer for the char
	 *  mwcls (IN): mod width class for the char
	 *  wch (IN): char
	 *  &lsact (OUT): action on the first char on the line
	 */

	LSERR (WINAPI* pfnModWidthOnRun)(POLS, PLSRUN, WCHAR, PLSRUN, WCHAR,
									   LSACT*);
	/* ModWidthOnRun:
	 *  pols (IN):
	 *  plsrunFirst (IN): run pointer for the first char
	 *  wchFirst (IN): first char
	 *  plsrunSecond (IN): run pointer for the second char
	 *  wchSecond (IN): second char
	 *  &lsact (OUT): action on the last char in 1st run
	 */

	LSERR (WINAPI* pfnModWidthSpace)(POLS, PLSRUN, PLSRUN, WCHAR, PLSRUN, WCHAR,
									 LSACT*);
	/* ModWidthSpace:
	 *  pols (IN):
	 *  plsrunCur (IN): run pointer for the current run
	 *  plsrunPrev (IN): run pointer for the previous char
	 *  wchPrev (IN): previous char
	 *  plsrunNext (IN): run pointer for the next char
	 *  wchNext (IN): next char
	 *  &lsact (OUT): action on space's width
	 */

	LSERR (WINAPI* pfnCompOnRun)(POLS, PLSRUN, WCHAR, PLSRUN, WCHAR,
								   LSPRACT*);
	/* CompOnRun:
	 *  pols (IN):
	 *  plsrunFirst (IN): run pointer for the first char
	 *  wchFirst (IN): first char
	 *  plsrunSecond (IN): run pointer for the second char
	 *  wchSecond (IN): second char
	 *  &lspract (OUT): prioritized action on the last char in 1st run
	 */

	LSERR (WINAPI* pfnCompWidthSpace)(POLS, PLSRUN, PLSRUN, WCHAR, PLSRUN, WCHAR,
									  LSPRACT*);
	/* CompWidthSpace:
	 *  pols (IN):
	 *  plsrunCur (IN): run pointer for the current run
	 *  plsrunPrev (IN): run pointer for the previous char
	 *  wchPrev (IN): previous char
	 *  plsrunNext (IN): run pointer for the next char
	 *  wchNext (IN): next char
	 *  &lspract (OUT): prioritized action on space's width
	 */


	LSERR (WINAPI* pfnExpOnRun)(POLS, PLSRUN, WCHAR, PLSRUN, WCHAR,
								  LSACT*);
	/* ExpOnRun:
	 *  pols (IN):
	 *  plsrunFirst (IN): run pointer for the first char
	 *  wchFirst (IN): first char
	 *  plsrunSecond (IN): run pointer for the second char
	 *  wchSecond (IN): second char
	 *  &lsact (OUT): action on the last run char from 1st run
	 */

	LSERR (WINAPI* pfnExpWidthSpace)(POLS, PLSRUN, PLSRUN, WCHAR, PLSRUN,
									   WCHAR, LSACT*);
	/* ExpWidthSpace:
	 *  pols (IN):
	 *  plsrunCur (IN): run pointer for the current run
	 *  plsrunPrev (IN): run pointer for the previous char
	 *  wchPrev (IN): previous char
	 *  plsrunNext (IN): run pointer for the next char
	 *  wchNext (IN): next char
	 *  &lsact (OUT): action on space's width
	 */

	LSERR (WINAPI* pfnGetModWidthClasses)(POLS, PLSRUN, const WCHAR*, DWORD, MWCLS*);
	/* GetModWidthClasses:
	 *  pols (IN):
	 *  plsrun (IN): run pointer for the characters
	 *  rgwch (IN): array of characters
	 *  cwch (IN): number of characters in the rgwch array
	 *  rgmwcls(OUT): array of ModWidthClass's for chars from the rgwch array
	 */

	LSERR (WINAPI* pfnGetBreakingClasses)(POLS, PLSRUN, LSCP, WCHAR, BRKCLS*, BRKCLS*);
	/* GetBreakingClasses:
	 *  pols (IN):
	 *  plsrun (IN): run pointer for the char
	 *  cp (IN): cp of the character
	 *  wch (IN): character
	 *  &brkclsFirst (OUT): breaking class for this char as the leading one in a pair
	 *  &brkclsSecond (OUT): breaking class for this char as the following one in a pair
	 */

	LSERR (WINAPI* pfnFTruncateBefore)(POLS, PLSRUN, LSCP, WCHAR, long, PLSRUN, LSCP, WCHAR, long, long, BOOL*);
	/* FTruncateBefore:
	 *  pols (IN):
	 *	plsrunCur (IN): plsrun of the current character 
	 *  cpCur (IN): cp of truncation char
	 *  wchCur (IN): truncation character 
	 *  durCur (IN): width of truncation character
	 *	plsrunPrev (IN): plsrun of the previous character 
	 *  cpPrev (IN): cp of the previous character
	 *  wchPrev (IN): previous character 
	 *  durPrev (IN): width of truncation character
	 *  durCut (IN): width from the RM until the end of the current character
	 *  &fTruncateBefore (OUT): truncation point is before this character
	 * 			(if it exceeds RM)
	 */
	
	LSERR (WINAPI* pfnCanBreakBeforeChar)(POLS, BRKCLS, BRKCOND*);
	/* CanBreakBeforeChar:
	 *  pols (IN):
	 *	brkcls (IN): breaking class for the char as the following one in a pair
	 *  &brktxtBefore (OUT): break condition before the character
	 */

	LSERR (WINAPI* pfnCanBreakAfterChar)(POLS, BRKCLS, BRKCOND*);
	/* CanBreakAfterChar:
	 *  pols (IN):
	 *  brkcls (IN): breaking class for the char as the leading one in a pair
	 *  &brktxtAfter (OUT): break text condition after the character
	 */


	LSERR (WINAPI* pfnFHangingPunct)(POLS, PLSRUN, MWCLS, WCHAR, BOOL*);
	/* FHangingPunct:
	 *  pols (IN):
	 *  plsrun (IN): run pointer for the char
	 *  mwcls (IN): mod width class of this char
	 *  wch (IN): character
	 *  &fHangingPunct (OUT): can be pushed to the right margin?
	 */

	LSERR (WINAPI* pfnGetSnapGrid)(POLS, WCHAR*, PLSRUN*, LSCP*, DWORD, BOOL*, DWORD*);
	/* GetGridInfo:
	 *  pols (IN):
	 *  rgwch (IN): array of characters
	 *  rgplsrun (IN): array of corresponding plsrun's
	 *  rgcp (IN): array of corresponding cp's
	 *  iwch (IN): number of characters
	 *	rgfSnap (OUT): array of fSnap flags for all characters
	 *	pwGridNumber (OUT): number of grid points on the line
	 */

	LSERR (WINAPI* pfnDrawEffects)(POLS, PLSRUN, UINT,
								   const POINT*, LPCWSTR, const int*, const int*, DWORD, 
								   LSTFLOW, UINT, PCHEIGHTS, long, long, const RECT*);
	/* DrawTextRun:
	 *  pols (IN):
	 *  plsrun (IN):
	 *  EffectsFlags (IN): set of client defined special effects bits
	 *  ppt (IN): output location
	 *  lpwchRun (IN): run of characters
	 *  rgDupRun (IN): widths of characters
	 *  rgDupLeftCut (IN): dup cut from the left side of the char
	 *  cwchRun (IN): number of characters in run
	 *  kTFlow (IN): text direction and orientation
	 *  kDisp (IN): display mode - opaque, transparent
	 *  heightsPres (IN): presentation heights for this run
	 *  dupRun (IN): presentation width for this run
	 *  dupLimUnderline (IN): underlining limit
	 *  pRectClip (IN): clipping rectangle
	 */

	LSERR (WINAPI* pfnFCancelHangingPunct)(POLS, LSCP, LSCP, WCHAR, MWCLS, BOOL*);

	/* FCancelHangingPunct:
	 *  pols (IN):
	 *  cpLim (IN): cpLim of the line
	 *  cpLastAdjustable (IN): cp of the last adjustable character on the line
	 *  wch (IN): last character
	 *  mwcls (IN): mod width class of this char
	 *  pfCancelHangingPunct (OUT): cancel hanging punctuation?
	*/

	LSERR (WINAPI* pfnModifyCompAtLastChar)(POLS, LSCP, LSCP, WCHAR, MWCLS, long, long, long*);

	/* ModifyCompAtLastChar:
	 *  pols (IN):
	 *  cpLim (IN): cpLim of the line
	 *  cpLastAdjustable (IN): cp of the last adjustable character on the line
	 *  wch (IN): last character
	 *  mwcls (IN): mod width class of this char
	 *  durCompLastRight (IN): suggested compression on the right side
	 *  durCompLastLeft (IN): suggested compression on the left side
	 *  pdurCahngeComp (OUT): change compression amount on the last char
	*/

	/* Enumeration callbacks */

	LSERR (WINAPI* pfnEnumText)(POLS, PLSRUN, LSCP, LSDCP, LPCWSTR, DWORD, LSTFLOW, BOOL,
											BOOL, const POINT*, PCHEIGHTS, long, BOOL, long*);
	/* EnumText:
	 *  pols (IN):
	 *  plsrun (IN): from DNODE
	 *  cpFirst (IN): from DNODE
	 *  dcp (IN): from DNODE
	 *  rgwch(IN): array of characters
	 *  cwch(IN): number of characters
	 *  lstflow (IN): text flow
	 *  fReverseOrder (IN): enumerate in reverse order
	 *  fGeometryProvided (IN):
	 *  pptStart (IN): starting position, iff fGeometryProvided
	 *  pheightsPres(IN): from DNODE, relevant iff fGeometryProvided
	 *  dupRun(IN): from DNODE, relevant iff fGeometryProvided
	 *  fCharWidthProvided (IN):
	 *  rgdup(IN): array of character widths, iff fCharWidthProvided
	*/

	LSERR (WINAPI* pfnEnumTab)(POLS, PLSRUN, LSCP, LPCWSTR, WCHAR, LSTFLOW, BOOL,
													BOOL, const POINT*, PCHEIGHTS, long);
	/* EnumTab:
	 *  pols (IN):
	 *  plsrun (IN): from DNODE
	 *  cpFirst (IN): from DNODE
	 *  rgwch(IN): Pointer to one Tab character
	 *  wchTabLeader (IN): tab leader
	 *  lstflow (IN): text flow
	 *  fReverseOrder (IN): enumerate in reverse order
	 *  fGeometryProvided (IN):
	 *  pptStart (IN): starting position, iff fGeometryProvided
	 *  pheightsPres(IN): from DNODE, relevant iff fGeometryProvided
	 *  dupRun(IN): from DNODE, relevant iff fGeometryProvided
	*/

	LSERR (WINAPI* pfnEnumPen)(POLS, BOOL, LSTFLOW, BOOL, BOOL, const POINT*, long, long);
	/* EnumPen:
	 *  pols (IN):
	 *  fBorder (IN):
	 *  lstflow (IN): text flow
	 *  fReverseOrder (IN): enumerate in reverse order
	 *  fGeometryProvided (IN):
	 *  pptStart (IN): starting position, iff fGeometryProvided
	 *  dup(IN): from DNODE iff fGeometryProvided
	 *  dvp(IN): from DNODE iff fGeometryProvided
	*/

	/* Objects bundling */

	LSERR (WINAPI* pfnGetObjectHandlerInfo)(POLS, DWORD, void*);
	/* GetObjectHandlerInfo:
	 *  pols (IN):
	 *  idObj (IN): id of the object handler
	 *  pObjectInfo (OUT): initialization information of the specified object
	*/


	/* Debugging APIs */
	void (WINAPI *pfnAssertFailed)(char*, char*, int);

};
typedef struct lscbk LSCBK;

#endif /* !LSCBK_DEFINED */

