//========================================================================
//
// PSOutputDev.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <signal.h>
#include <math.h>
#include "goo/GooString.h"
#include "goo/GooList.h"
#include "poppler-config.h"
#include "GlobalParams.h"
#include "Object.h"
#include "Error.h"
#include "Function.h"
#include "Gfx.h"
#include "GfxState.h"
#include "GfxFont.h"
#include "UnicodeMap.h"
#include <fofi/FoFiType1C.h>
#include <fofi/FoFiTrueType.h>
#include "Catalog.h"
#include "Page.h"
#include "Stream.h"
#include "Annot.h"
#include "PSOutputDev.h"
#include "UGooString.h"

#ifdef MACOS
// needed for setting type/creator of MacOS files
#include "ICSupport.h"
#endif

//------------------------------------------------------------------------
// PostScript prolog and setup
//------------------------------------------------------------------------

// The '~' escapes mark prolog code that is emitted only in certain
// levels:
//
//   ~[123][sn]
//      ^   ^----- s=psLevel*Sep, n=psLevel*
//      +----- 1=psLevel1*, 2=psLevel2*, 3=psLevel3*

static char *prolog[] = {
  "/xpdf 75 dict def xpdf begin",
  "% PDF special state",
  "/pdfDictSize 15 def",
  "~1sn",
  "/pdfStates 64 array def",
  "  0 1 63 {",
  "    pdfStates exch pdfDictSize dict",
  "    dup /pdfStateIdx 3 index put",
  "    put",
  "  } for",
  "~123sn",
  "/pdfSetup {",
  "  3 1 roll 2 array astore",
  "  /setpagedevice where {",
  "    pop 3 dict begin",
  "      /PageSize exch def",
  "      /ImagingBBox null def",
  "      /Policies 1 dict dup begin /PageSize 3 def end def",
  "      { /Duplex true def } if",
  "    currentdict end setpagedevice",
  "  } {",
  "    pop pop",
  "  } ifelse",
  "} def",
  "~1sn",
  "/pdfOpNames [",
  "  /pdfFill /pdfStroke /pdfLastFill /pdfLastStroke",
  "  /pdfTextMat /pdfFontSize /pdfCharSpacing /pdfTextRender",
  "  /pdfTextRise /pdfWordSpacing /pdfHorizScaling /pdfTextClipPath",
  "] def",
  "~123sn",
  "/pdfStartPage {",
  "~1sn",
  "  pdfStates 0 get begin",
  "~23sn",
  "  pdfDictSize dict begin",
  "~23n",
  "  /pdfFillCS [] def",
  "  /pdfFillXform {} def",
  "  /pdfStrokeCS [] def",
  "  /pdfStrokeXform {} def",
  "~1n",
  "  /pdfFill 0 def",
  "  /pdfStroke 0 def",
  "~1s",
  "  /pdfFill [0 0 0 1] def",
  "  /pdfStroke [0 0 0 1] def",
  "~23sn",
  "  /pdfFill [0] def",
  "  /pdfStroke [0] def",
  "  /pdfFillOP false def",
  "  /pdfStrokeOP false def",
  "~123sn",
  "  /pdfLastFill false def",
  "  /pdfLastStroke false def",
  "  /pdfTextMat [1 0 0 1 0 0] def",
  "  /pdfFontSize 0 def",
  "  /pdfCharSpacing 0 def",
  "  /pdfTextRender 0 def",
  "  /pdfTextRise 0 def",
  "  /pdfWordSpacing 0 def",
  "  /pdfHorizScaling 1 def",
  "  /pdfTextClipPath [] def",
  "} def",
  "/pdfEndPage { end } def",
  "~23s",
  "% separation convention operators",
  "/findcmykcustomcolor where {",
  "  pop",
  "}{",
  "  /findcmykcustomcolor { 5 array astore } def",
  "} ifelse",
  "/setcustomcolor where {",
  "  pop",
  "}{",
  "  /setcustomcolor {",
  "    exch",
  "    [ exch /Separation exch dup 4 get exch /DeviceCMYK exch",
  "      0 4 getinterval cvx",
  "      [ exch /dup load exch { mul exch dup } /forall load",
  "        /pop load dup ] cvx",
  "    ] setcolorspace setcolor",
  "  } def",
  "} ifelse",
  "/customcolorimage where {",
  "  pop",
  "}{",
  "  /customcolorimage {",
  "    gsave",
  "    [ exch /Separation exch dup 4 get exch /DeviceCMYK exch",
  "      0 4 getinterval",
  "      [ exch /dup load exch { mul exch dup } /forall load",
  "        /pop load dup ] cvx",
  "    ] setcolorspace",
  "    10 dict begin",
  "      /ImageType 1 def",
  "      /DataSource exch def",
  "      /ImageMatrix exch def",
  "      /BitsPerComponent exch def",
  "      /Height exch def",
  "      /Width exch def",
  "      /Decode [1 0] def",
  "    currentdict end",
  "    image",
  "    grestore",
  "  } def",
  "} ifelse",
  "~123sn",
  "% PDF color state",
  "~1n",
  "/g { dup /pdfFill exch def setgray",
  "     /pdfLastFill true def /pdfLastStroke false def } def",
  "/G { dup /pdfStroke exch def setgray",
  "     /pdfLastStroke true def /pdfLastFill false def } def",
  "/fCol {",
  "  pdfLastFill not {",
  "    pdfFill setgray",
  "    /pdfLastFill true def /pdfLastStroke false def",
  "  } if",
  "} def",
  "/sCol {",
  "  pdfLastStroke not {",
  "    pdfStroke setgray",
  "    /pdfLastStroke true def /pdfLastFill false def",
  "  } if",
  "} def",
  "~1s",
  "/k { 4 copy 4 array astore /pdfFill exch def setcmykcolor",
  "     /pdfLastFill true def /pdfLastStroke false def } def",
  "/K { 4 copy 4 array astore /pdfStroke exch def setcmykcolor",
  "     /pdfLastStroke true def /pdfLastFill false def } def",
  "/fCol {",
  "  pdfLastFill not {",
  "    pdfFill aload pop setcmykcolor",
  "    /pdfLastFill true def /pdfLastStroke false def",
  "  } if",
  "} def",
  "/sCol {",
  "  pdfLastStroke not {",
  "    pdfStroke aload pop setcmykcolor",
  "    /pdfLastStroke true def /pdfLastFill false def",
  "  } if",
  "} def",
  "~23n",
  "/cs { /pdfFillXform exch def dup /pdfFillCS exch def",
  "      setcolorspace } def",
  "/CS { /pdfStrokeXform exch def dup /pdfStrokeCS exch def",
  "      setcolorspace } def",
  "/sc { pdfLastFill not { pdfFillCS setcolorspace } if",
  "      dup /pdfFill exch def aload pop pdfFillXform setcolor",
  "     /pdfLastFill true def /pdfLastStroke false def } def",
  "/SC { pdfLastStroke not { pdfStrokeCS setcolorspace } if",
  "      dup /pdfStroke exch def aload pop pdfStrokeXform setcolor",
  "     /pdfLastStroke true def /pdfLastFill false def } def",
  "/op { /pdfFillOP exch def",
  "      pdfLastFill { pdfFillOP setoverprint } if } def",
  "/OP { /pdfStrokeOP exch def",
  "      pdfLastStroke { pdfStrokeOP setoverprint } if } def",
  "/fCol {",
  "  pdfLastFill not {",
  "    pdfFillCS setcolorspace",
  "    pdfFill aload pop pdfFillXform setcolor",
  "    pdfFillOP setoverprint",
  "    /pdfLastFill true def /pdfLastStroke false def",
  "  } if",
  "} def",
  "/sCol {",
  "  pdfLastStroke not {",
  "    pdfStrokeCS setcolorspace",
  "    pdfStroke aload pop pdfStrokeXform setcolor",
  "    pdfStrokeOP setoverprint",
  "    /pdfLastStroke true def /pdfLastFill false def",
  "  } if",
  "} def",
  "~23s",
  "/k { 4 copy 4 array astore /pdfFill exch def setcmykcolor",
  "     /pdfLastFill true def /pdfLastStroke false def } def",
  "/K { 4 copy 4 array astore /pdfStroke exch def setcmykcolor",
  "     /pdfLastStroke true def /pdfLastFill false def } def",
  "/ck { 6 copy 6 array astore /pdfFill exch def",
  "      findcmykcustomcolor exch setcustomcolor",
  "      /pdfLastFill true def /pdfLastStroke false def } def",
  "/CK { 6 copy 6 array astore /pdfStroke exch def",
  "      findcmykcustomcolor exch setcustomcolor",
  "      /pdfLastStroke true def /pdfLastFill false def } def",
  "/op { /pdfFillOP exch def",
  "      pdfLastFill { pdfFillOP setoverprint } if } def",
  "/OP { /pdfStrokeOP exch def",
  "      pdfLastStroke { pdfStrokeOP setoverprint } if } def",
  "/fCol {",
  "  pdfLastFill not {",
  "    pdfFill aload length 4 eq {",
  "          setcmykcolor",
  "        }{",
  "          findcmykcustomcolor exch setcustomcolor",
  "        } ifelse",
  "    pdfFillOP setoverprint",
  "    /pdfLastFill true def /pdfLastStroke false def",
  "  } if",
  "} def",
  "/sCol {",
  "  pdfLastStroke not {",
  "    pdfStroke aload length 4 eq {",
  "      setcmykcolor",
  "    }{",
  "      findcmykcustomcolor exch setcustomcolor",
  "    } ifelse",
  "    pdfStrokeOP setoverprint",
  "    /pdfLastStroke true def /pdfLastFill false def",
  "  } if",
  "} def",
  "~123sn",
  "% build a font",
  "/pdfMakeFont {",
  "  4 3 roll findfont",
  "  4 2 roll matrix scale makefont",
  "  dup length dict begin",
  "    { 1 index /FID ne { def } { pop pop } ifelse } forall",
  "    /Encoding exch def",
  "    currentdict",
  "  end",
  "  definefont pop",
  "} def",
  "/pdfMakeFont16 {",
  "  exch findfont",
  "  dup length dict begin",
  "    { 1 index /FID ne { def } { pop pop } ifelse } forall",
  "    /WMode exch def",
  "    currentdict",
  "  end",
  "  definefont pop",
  "} def",
  "~3sn",
  "/pdfMakeFont16L3 {",
  "  1 index /CIDFont resourcestatus {",
  "    pop pop 1 index /CIDFont findresource /CIDFontType known",
  "  } {",
  "    false",
  "  } ifelse",
  "  {",
  "    0 eq { /Identity-H } { /Identity-V } ifelse",
  "    exch 1 array astore composefont pop",
  "  } {",
  "    pdfMakeFont16",
  "  } ifelse",
  "} def",
  "~123sn",
  "% graphics state operators",
  "~1sn",
  "/q {",
  "  gsave",
  "  pdfOpNames length 1 sub -1 0 { pdfOpNames exch get load } for",
  "  pdfStates pdfStateIdx 1 add get begin",
  "  pdfOpNames { exch def } forall",
  "} def",
  "/Q { end grestore } def",
  "~23sn",
  "/q { gsave pdfDictSize dict begin } def",
  "/Q {",
  "  end grestore",
  "  /pdfLastFill where {",
  "    pop",
  "    pdfLastFill {",
  "      pdfFillOP setoverprint",
  "    } {",
  "      pdfStrokeOP setoverprint",
  "    } ifelse",
  "  } if",
  "} def",
  "~123sn",
  "/cm { concat } def",
  "/d { setdash } def",
  "/i { setflat } def",
  "/j { setlinejoin } def",
  "/J { setlinecap } def",
  "/M { setmiterlimit } def",
  "/w { setlinewidth } def",
  "% path segment operators",
  "/m { moveto } def",
  "/l { lineto } def",
  "/c { curveto } def",
  "/re { 4 2 roll moveto 1 index 0 rlineto 0 exch rlineto",
  "      neg 0 rlineto closepath } def",
  "/h { closepath } def",
  "% path painting operators",
  "/S { sCol stroke } def",
  "/Sf { fCol stroke } def",
  "/f { fCol fill } def",
  "/f* { fCol eofill } def",
  "% clipping operators",
  "/W { clip newpath } def",
  "/W* { eoclip newpath } def",
  "% text state operators",
  "/Tc { /pdfCharSpacing exch def } def",
  "/Tf { dup /pdfFontSize exch def",
  "      dup pdfHorizScaling mul exch matrix scale",
  "      pdfTextMat matrix concatmatrix dup 4 0 put dup 5 0 put",
  "      exch findfont exch makefont setfont } def",
  "/Tr { /pdfTextRender exch def } def",
  "/Ts { /pdfTextRise exch def } def",
  "/Tw { /pdfWordSpacing exch def } def",
  "/Tz { /pdfHorizScaling exch def } def",
  "% text positioning operators",
  "/Td { pdfTextMat transform moveto } def",
  "/Tm { /pdfTextMat exch def } def",
  "% text string operators",
  "/cshow where {",
  "  pop",
  "  /cshow2 {",
  "    dup {",
  "      pop pop",
  "      1 string dup 0 3 index put 3 index exec",
  "    } exch cshow",
  "    pop pop",
  "  } def",
  "}{",
  "  /cshow2 {",
  "    currentfont /FontType get 0 eq {",
  "      0 2 2 index length 1 sub {",
  "        2 copy get exch 1 add 2 index exch get",
  "        2 copy exch 256 mul add",
  "        2 string dup 0 6 5 roll put dup 1 5 4 roll put",
  "        3 index exec",
  "      } for",
  "    } {",
  "      dup {",
  "        1 string dup 0 3 index put 3 index exec",
  "      } forall",
  "    } ifelse",
  "    pop pop",
  "  } def",
  "} ifelse",
  "/awcp {", // awidthcharpath
  "  exch {",
  "    false charpath",
  "    5 index 5 index rmoveto",
  "    6 index eq { 7 index 7 index rmoveto } if",
  "  } exch cshow2",
  "  6 {pop} repeat",
  "} def",
  "/Tj {",
  "  fCol",  // because stringwidth has to draw Type 3 chars
  "  1 index stringwidth pdfTextMat idtransform pop",
  "  sub 1 index length dup 0 ne { div } { pop pop 0 } ifelse",
  "  pdfWordSpacing pdfHorizScaling mul 0 pdfTextMat dtransform 32",
  "  4 3 roll pdfCharSpacing pdfHorizScaling mul add 0",
  "  pdfTextMat dtransform",
  "  6 5 roll Tj1",
  "} def",
  "/Tj16 {",
  "  fCol",  // because stringwidth has to draw Type 3 chars
  "  2 index stringwidth pdfTextMat idtransform pop",
  "  sub exch div",
  "  pdfWordSpacing pdfHorizScaling mul 0 pdfTextMat dtransform 32",
  "  4 3 roll pdfCharSpacing pdfHorizScaling mul add 0",
  "  pdfTextMat dtransform",
  "  6 5 roll Tj1",
  "} def",
  "/Tj16V {",
  "  fCol",  // because stringwidth has to draw Type 3 chars
  "  2 index stringwidth pdfTextMat idtransform exch pop",
  "  sub exch div",
  "  0 pdfWordSpacing pdfTextMat dtransform 32",
  "  4 3 roll pdfCharSpacing add 0 exch",
  "  pdfTextMat dtransform",
  "  6 5 roll Tj1",
  "} def",
  "/Tj1 {",
  "  0 pdfTextRise pdfTextMat dtransform rmoveto",
  "  currentpoint 8 2 roll",
  "  pdfTextRender 1 and 0 eq {",
  "    6 copy awidthshow",
  "  } if",
  "  pdfTextRender 3 and dup 1 eq exch 2 eq or {",
  "    7 index 7 index moveto",
  "    6 copy",
  "    currentfont /FontType get 3 eq { fCol } { sCol } ifelse",
  "    false awcp currentpoint stroke moveto",
  "  } if",
  "  pdfTextRender 4 and 0 ne {",
  "    8 6 roll moveto",
  "    false awcp",
  "    /pdfTextClipPath [ pdfTextClipPath aload pop",
  "      {/moveto cvx}",
  "      {/lineto cvx}",
  "      {/curveto cvx}",
  "      {/closepath cvx}",
  "    pathforall ] def",
  "    currentpoint newpath moveto",
  "  } {",
  "    8 {pop} repeat",
  "  } ifelse",
  "  0 pdfTextRise neg pdfTextMat dtransform rmoveto",
  "} def",
  "/TJm { pdfFontSize 0.001 mul mul neg 0",
  "       pdfTextMat dtransform rmoveto } def",
  "/TJmV { pdfFontSize 0.001 mul mul neg 0 exch",
  "        pdfTextMat dtransform rmoveto } def",
  "/Tclip { pdfTextClipPath cvx exec clip newpath",
  "         /pdfTextClipPath [] def } def",
  "~1ns",
  "% Level 1 image operators",
  "~1n",
  "/pdfIm1 {",
  "  /pdfImBuf1 4 index string def",
  "  { currentfile pdfImBuf1 readhexstring pop } image",
  "} def",
  "~1s",
  "/pdfIm1Sep {",
  "  /pdfImBuf1 4 index string def",
  "  /pdfImBuf2 4 index string def",
  "  /pdfImBuf3 4 index string def",
  "  /pdfImBuf4 4 index string def",
  "  { currentfile pdfImBuf1 readhexstring pop }",
  "  { currentfile pdfImBuf2 readhexstring pop }",
  "  { currentfile pdfImBuf3 readhexstring pop }",
  "  { currentfile pdfImBuf4 readhexstring pop }",
  "  true 4 colorimage",
  "} def",
  "~1ns",
  "/pdfImM1 {",
  "  fCol /pdfImBuf1 4 index 7 add 8 idiv string def",
  "  { currentfile pdfImBuf1 readhexstring pop } imagemask",
  "} def",
  "/pdfImM1a {",
  "  { 2 copy get exch 1 add exch } imagemask",
  "  pop pop",
  "} def",
  "~23sn",
  "% Level 2 image operators",
  "/pdfImBuf 100 string def",
  "/pdfIm {",
  "  image",
  "  { currentfile pdfImBuf readline",
  "    not { pop exit } if",
  "    (%-EOD-) eq { exit } if } loop",
  "} def",
  "~23s",
  "/pdfImSep {",
  "  findcmykcustomcolor exch",
  "  dup /Width get /pdfImBuf1 exch string def",
  "  dup /Decode get aload pop 1 index sub /pdfImDecodeRange exch def",
  "  /pdfImDecodeLow exch def",
  "  begin Width Height BitsPerComponent ImageMatrix DataSource end",
  "  /pdfImData exch def",
  "  { pdfImData pdfImBuf1 readstring pop",
  "    0 1 2 index length 1 sub {",
  "      1 index exch 2 copy get",
  "      pdfImDecodeRange mul 255 div pdfImDecodeLow add round cvi",
  "      255 exch sub put",
  "    } for }",
  "  6 5 roll customcolorimage",
  "  { currentfile pdfImBuf readline",
  "    not { pop exit } if",
  "    (%-EOD-) eq { exit } if } loop",
  "} def",
  "~23sn",
  "/pdfImM {",
  "  fCol imagemask",
  "  { currentfile pdfImBuf readline",
  "    not { pop exit } if",
  "    (%-EOD-) eq { exit } if } loop",
  "} def",
  "/pdfImClip {",
  "  gsave",
  "  0 2 4 index length 1 sub {",
  "    dup 4 index exch 2 copy",
  "    get 5 index div put",
  "    1 add 3 index exch 2 copy",
  "    get 3 index div put",
  "  } for",
  "  pop pop rectclip",
  "} def",
  "/pdfImClipEnd { grestore } def",
  "~23n",
  "% shading operators",
  "/colordelta {",
  "  false 0 1 3 index length 1 sub {",
  "    dup 4 index exch get 3 index 3 2 roll get sub abs 0.004 gt {",
  "      pop true",
  "    } if",
  "  } for",
  "  exch pop exch pop",
  "} def",
  "/funcCol { func n array astore } def",
  "/funcSH {",
  "  dup 0 eq {",
  "    true",
  "  } {",
  "    dup 6 eq {",
  "      false",
  "    } {",
  "      4 index 4 index funcCol dup",
  "      6 index 4 index funcCol dup",
  "      3 1 roll colordelta 3 1 roll",
  "      5 index 5 index funcCol dup",
  "      3 1 roll colordelta 3 1 roll",
  "      6 index 8 index funcCol dup",
  "      3 1 roll colordelta 3 1 roll",
  "      colordelta or or or",
  "    } ifelse",
  "  } ifelse",
  "  {",
  "    1 add",
  "    4 index 3 index add 0.5 mul exch 4 index 3 index add 0.5 mul exch",
  "    6 index 6 index 4 index 4 index 4 index funcSH",
  "    2 index 6 index 6 index 4 index 4 index funcSH",
  "    6 index 2 index 4 index 6 index 4 index funcSH",
  "    5 3 roll 3 2 roll funcSH pop pop",
  "  } {",
  "    pop 3 index 2 index add 0.5 mul 3 index  2 index add 0.5 mul",
  "    funcCol sc",
  "    dup 4 index exch mat transform m",
  "    3 index 3 index mat transform l",
  "    1 index 3 index mat transform l",
  "    mat transform l pop pop h f*",
  "  } ifelse",
  "} def",
  "/axialCol {",
  "  dup 0 lt {",
  "    pop t0",
  "  } {",
  "    dup 1 gt {",
  "      pop t1",
  "    } {",
  "      dt mul t0 add",
  "    } ifelse",
  "  } ifelse",
  "  func n array astore",
  "} def",
  "/axialSH {",
  "  dup 0 eq {",
  "    true",
  "  } {",
  "    dup 8 eq {",
  "      false",
  "    } {",
  "      2 index axialCol 2 index axialCol colordelta",
  "    } ifelse",
  "  } ifelse",
  "  {",
  "    1 add 3 1 roll 2 copy add 0.5 mul",
  "    dup 4 3 roll exch 4 index axialSH",
  "    exch 3 2 roll axialSH",
  "  } {",
  "    pop 2 copy add 0.5 mul axialCol sc",
  "    exch dup dx mul x0 add exch dy mul y0 add",
  "    3 2 roll dup dx mul x0 add exch dy mul y0 add",
  "    dx abs dy abs ge {",
  "      2 copy yMin sub dy mul dx div add yMin m",
  "      yMax sub dy mul dx div add yMax l",
  "      2 copy yMax sub dy mul dx div add yMax l",
  "      yMin sub dy mul dx div add yMin l",
  "      h f*",
  "    } {",
  "      exch 2 copy xMin sub dx mul dy div add xMin exch m",
  "      xMax sub dx mul dy div add xMax exch l",
  "      exch 2 copy xMax sub dx mul dy div add xMax exch l",
  "      xMin sub dx mul dy div add xMin exch l",
  "      h f*",
  "    } ifelse",
  "  } ifelse",
  "} def",
  "/radialCol {",
  "  dup t0 lt {",
  "    pop t0",
  "  } {",
  "    dup t1 gt {",
  "      pop t1",
  "    } if",
  "  } ifelse",
  "  func n array astore",
  "} def",
  "/radialSH {",
  "  dup 0 eq {",
  "    true",
  "  } {",
  "    dup 8 eq {",
  "      false",
  "    } {",
  "      2 index dt mul t0 add radialCol",
  "      2 index dt mul t0 add radialCol colordelta",
  "    } ifelse",
  "  } ifelse",
  "  {",
  "    1 add 3 1 roll 2 copy add 0.5 mul",
  "    dup 4 3 roll exch 4 index radialSH",
  "    exch 3 2 roll radialSH",
  "  } {",
  "    pop 2 copy add 0.5 mul dt mul t0 add axialCol sc",
  "    exch dup dx mul x0 add exch dup dy mul y0 add exch dr mul r0 add",
  "    0 360 arc h",
  "    dup dx mul x0 add exch dup dy mul y0 add exch dr mul r0 add",
  "    0 360 arc h f*",
  "  } ifelse",
  "} def",
  "~123sn",
  "end",
  NULL
};

static char *cmapProlog[] = {
  "/CIDInit /ProcSet findresource begin",
  "10 dict begin",
  "  begincmap",
  "  /CMapType 1 def",
  "  /CMapName /Identity-H def",
  "  /CIDSystemInfo 3 dict dup begin",
  "    /Registry (Adobe) def",
  "    /Ordering (Identity) def",
  "    /Supplement 0 def",
  "  end def",
  "  1 begincodespacerange",
  "    <0000> <ffff>",
  "  endcodespacerange",
  "  0 usefont",
  "  1 begincidrange",
  "    <0000> <ffff> 0",
  "  endcidrange",
  "  endcmap",
  "  currentdict CMapName exch /CMap defineresource pop",
  "end",
  "10 dict begin",
  "  begincmap",
  "  /CMapType 1 def",
  "  /CMapName /Identity-V def",
  "  /CIDSystemInfo 3 dict dup begin",
  "    /Registry (Adobe) def",
  "    /Ordering (Identity) def",
  "    /Supplement 0 def",
  "  end def",
  "  /WMode 1 def",
  "  1 begincodespacerange",
  "    <0000> <ffff>",
  "  endcodespacerange",
  "  0 usefont",
  "  1 begincidrange",
  "    <0000> <ffff> 0",
  "  endcidrange",
  "  endcmap",
  "  currentdict CMapName exch /CMap defineresource pop",
  "end",
  "end",
  NULL
};

//------------------------------------------------------------------------
// Fonts
//------------------------------------------------------------------------

struct PSSubstFont {
  char *psName;			// PostScript name
  double mWidth;		// width of 'm' character
};

static char *psFonts[] = {
  "Courier",
  "Courier-Bold",
  "Courier-Oblique",
  "Courier-BoldOblique",
  "Helvetica",
  "Helvetica-Bold",
  "Helvetica-Oblique",
  "Helvetica-BoldOblique",
  "Symbol",
  "Times-Roman",
  "Times-Bold",
  "Times-Italic",
  "Times-BoldItalic",
  "ZapfDingbats",
  NULL
};

static PSSubstFont psSubstFonts[] = {
  {"Helvetica",             0.833},
  {"Helvetica-Oblique",     0.833},
  {"Helvetica-Bold",        0.889},
  {"Helvetica-BoldOblique", 0.889},
  {"Times-Roman",           0.788},
  {"Times-Italic",          0.722},
  {"Times-Bold",            0.833},
  {"Times-BoldItalic",      0.778},
  {"Courier",               0.600},
  {"Courier-Oblique",       0.600},
  {"Courier-Bold",          0.600},
  {"Courier-BoldOblique",   0.600}
};

// Encoding info for substitute 16-bit font
struct PSFont16Enc {
  Ref fontID;
  GooString *enc;
};

//------------------------------------------------------------------------
// process colors
//------------------------------------------------------------------------

#define psProcessCyan     1
#define psProcessMagenta  2
#define psProcessYellow   4
#define psProcessBlack    8
#define psProcessCMYK    15

//------------------------------------------------------------------------
// PSOutCustomColor
//------------------------------------------------------------------------

class PSOutCustomColor {
public:

  PSOutCustomColor(double cA, double mA,
		   double yA, double kA, GooString *nameA);
  ~PSOutCustomColor();

  double c, m, y, k;
  GooString *name;
  PSOutCustomColor *next;
};

PSOutCustomColor::PSOutCustomColor(double cA, double mA,
				   double yA, double kA, GooString *nameA) {
  c = cA;
  m = mA;
  y = yA;
  k = kA;
  name = nameA;
  next = NULL;
}

PSOutCustomColor::~PSOutCustomColor() {
  delete name;
}

//------------------------------------------------------------------------
// DeviceNRecoder
//------------------------------------------------------------------------

class DeviceNRecoder: public FilterStream {
public:

  DeviceNRecoder(Stream *strA, int widthA, int heightA,
		 GfxImageColorMap *colorMapA);
  virtual ~DeviceNRecoder();
  virtual StreamKind getKind() { return strWeird; }
  virtual void reset();
  virtual int getChar()
    { return (bufIdx >= bufSize && !fillBuf()) ? EOF : buf[bufIdx++]; }
  virtual int lookChar()
    { return (bufIdx >= bufSize && !fillBuf()) ? EOF : buf[bufIdx]; }
  virtual GooString *getPSFilter(int psLevel, char *indent) { return NULL; }
  virtual GBool isBinary(GBool last = gTrue) { return gTrue; }
  virtual GBool isEncoder() { return gTrue; }

private:

  GBool fillBuf();

  int width, height;
  GfxImageColorMap *colorMap;
  Function *func;
  ImageStream *imgStr;
  int buf[gfxColorMaxComps];
  int pixelIdx;
  int bufIdx;
  int bufSize;
};

DeviceNRecoder::DeviceNRecoder(Stream *strA, int widthA, int heightA,
			       GfxImageColorMap *colorMapA):
    FilterStream(strA) {
  width = widthA;
  height = heightA;
  colorMap = colorMapA;
  imgStr = NULL;
  pixelIdx = 0;
  bufIdx = gfxColorMaxComps;
  bufSize = ((GfxDeviceNColorSpace *)colorMap->getColorSpace())->
              getAlt()->getNComps();
  func = ((GfxDeviceNColorSpace *)colorMap->getColorSpace())->
           getTintTransformFunc();
}

DeviceNRecoder::~DeviceNRecoder() {
  if (imgStr) {
    delete imgStr;
  }
}

void DeviceNRecoder::reset() {
  imgStr = new ImageStream(str, width, colorMap->getNumPixelComps(),
			   colorMap->getBits());
  imgStr->reset();
}

GBool DeviceNRecoder::fillBuf() {
  Guchar pixBuf[gfxColorMaxComps];
  GfxColor color;
  double x[gfxColorMaxComps], y[gfxColorMaxComps];
  int i;

  if (pixelIdx >= width * height) {
    return gFalse;
  }
  imgStr->getPixel(pixBuf);
  colorMap->getColor(pixBuf, &color);
  for (i = 0;
       i < ((GfxDeviceNColorSpace *)colorMap->getColorSpace())->getNComps();
       ++i) {
    x[i] = colToDbl(color.c[i]);
  }
  func->transform(x, y);
  for (i = 0; i < bufSize; ++i) {
    buf[i] = (int)(y[i] * 255 + 0.5);
  }
  bufIdx = 0;
  ++pixelIdx;
  return gTrue;
}

//------------------------------------------------------------------------
// PSOutputDev
//------------------------------------------------------------------------

extern "C" {
typedef void (*SignalFunc)(int);
}

static void outputToFile(void *stream, char *data, int len) {
  fwrite(data, 1, len, (FILE *)stream);
}

PSOutputDev::PSOutputDev(const char *fileName, XRef *xrefA, Catalog *catalog,
			 int firstPage, int lastPage, PSOutMode modeA,
                         int paperWidthA, int paperHeightA, GBool duplexA,
			 int imgLLXA, int imgLLYA, int imgURXA, int imgURYA,
			 GBool manualCtrlA) {
  FILE *f;
  PSFileType fileTypeA;

  underlayCbk = NULL;
  underlayCbkData = NULL;
  overlayCbk = NULL;
  overlayCbkData = NULL;

  fontIDs = NULL;
  fontFileIDs = NULL;
  fontFileNames = NULL;
  font16Enc = NULL;
  xobjStack = NULL;
  embFontList = NULL;
  customColors = NULL;
  haveTextClip = gFalse;
  t3String = NULL;

  // open file or pipe
  if (!strcmp(fileName, "-")) {
    fileTypeA = psStdout;
    f = stdout;
  } else if (fileName[0] == '|') {
    fileTypeA = psPipe;
#ifdef HAVE_POPEN
#ifndef WIN32
    signal(SIGPIPE, (SignalFunc)SIG_IGN);
#endif
    if (!(f = popen(fileName + 1, "w"))) {
      error(-1, "Couldn't run print command '%s'", fileName);
      ok = gFalse;
      return;
    }
#else
    error(-1, "Print commands are not supported ('%s')", fileName);
    ok = gFalse;
    return;
#endif
  } else {
    fileTypeA = psFile;
    if (!(f = fopen(fileName, "w"))) {
      error(-1, "Couldn't open PostScript file '%s'", fileName);
      ok = gFalse;
      return;
    }
  }

  init(outputToFile, f, fileTypeA,
       xrefA, catalog, firstPage, lastPage, modeA,
       imgLLXA, imgLLYA, imgURXA, imgURYA, manualCtrlA,
       paperWidthA, paperHeightA, duplexA);
}

PSOutputDev::PSOutputDev(PSOutputFunc outputFuncA, void *outputStreamA,
			 XRef *xrefA, Catalog *catalog,
			 int firstPage, int lastPage, PSOutMode modeA,
                         int paperWidthA, int paperHeightA, GBool duplexA,
			 int imgLLXA, int imgLLYA, int imgURXA, int imgURYA,
			 GBool manualCtrlA) {
  underlayCbk = NULL;
  underlayCbkData = NULL;
  overlayCbk = NULL;
  overlayCbkData = NULL;

  fontIDs = NULL;
  fontFileIDs = NULL;
  fontFileNames = NULL;
  font16Enc = NULL;
  xobjStack = NULL;
  embFontList = NULL;
  customColors = NULL;
  haveTextClip = gFalse;
  t3String = NULL;

  init(outputFuncA, outputStreamA, psGeneric,
       xrefA, catalog, firstPage, lastPage, modeA,
       imgLLXA, imgLLYA, imgURXA, imgURYA, manualCtrlA,
       paperWidthA, paperHeightA, duplexA);
}

void PSOutputDev::init(PSOutputFunc outputFuncA, void *outputStreamA,
		       PSFileType fileTypeA, XRef *xrefA, Catalog *catalog,
		       int firstPage, int lastPage, PSOutMode modeA,
		       int imgLLXA, int imgLLYA, int imgURXA, int imgURYA,
		       GBool manualCtrlA, int paperWidthA, int paperHeightA,
                       GBool duplexA) {
  Page *page;
  PDFRectangle *box;

  // initialize
  ok = gTrue;
  outputFunc = outputFuncA;
  outputStream = outputStreamA;
  fileType = fileTypeA;
  xref = xrefA;
  level = globalParams->getPSLevel();
  mode = modeA;
  paperWidth = paperWidthA;
  paperHeight = paperHeightA;
  imgLLX = imgLLXA;
  imgLLY = imgLLYA;
  imgURX = imgURXA;
  imgURY = imgURYA;
  if (paperWidth < 0 || paperHeight < 0) {
    // this check is needed in case the document has zero pages
    if (firstPage > 0 && firstPage <= catalog->getNumPages()) {
      page = catalog->getPage(firstPage);
      paperWidth = (int)ceil(page->getMediaWidth());
      paperHeight = (int)ceil(page->getMediaHeight());
    } else {
      paperWidth = 1;
      paperHeight = 1;
    }
  }
  if (imgLLX == 0 && imgURX == 0 && imgLLY == 0 && imgURY == 0) {
    globalParams->getPSImageableArea(&imgLLX, &imgLLY, &imgURX, &imgURY);
    if (imgURX <= 0 || imgURY <= 0) {
      imgLLX = imgLLY = 0;
      imgURX = paperWidth;
      imgURY = paperHeight;
    }
  }
  manualCtrl = manualCtrlA;
  if (mode == psModeForm) {
    lastPage = firstPage;
  }
  processColors = 0;
  inType3Char = gFalse;

#if OPI_SUPPORT
  // initialize OPI nesting levels
  opi13Nest = 0;
  opi20Nest = 0;
#endif

  tx0 = ty0 = 0;
  xScale0 = yScale0 = 0;
  rotate0 = -1;
  clipLLX0 = clipLLY0 = 0;
  clipURX0 = clipURY0 = -1;

  // initialize fontIDs, fontFileIDs, and fontFileNames lists
  fontIDSize = 64;
  fontIDLen = 0;
  fontIDs = (Ref *)gmallocn(fontIDSize, sizeof(Ref));
  fontFileIDSize = 64;
  fontFileIDLen = 0;
  fontFileIDs = (Ref *)gmallocn(fontFileIDSize, sizeof(Ref));
  fontFileNameSize = 64;
  fontFileNameLen = 0;
  fontFileNames = (GooString **)gmallocn(fontFileNameSize, sizeof(GooString *));
  psFileNames = (GooString **)gmallocn(fontFileNameSize, sizeof(GooString *));
  nextTrueTypeNum = 0;
  font16EncLen = 0;
  font16EncSize = 0;

  xobjStack = new GooList();
  numSaves = 0;
  numTilingPatterns = 0;
  nextFunc = 0;

  // initialize embedded font resource comment list
  embFontList = new GooString();

  if (!manualCtrl) {
    // this check is needed in case the document has zero pages
    if (firstPage > 0 && firstPage <= catalog->getNumPages()) {
      writeHeader(firstPage, lastPage,
		  catalog->getPage(firstPage)->getMediaBox(),
		  catalog->getPage(firstPage)->getCropBox(),
		  catalog->getPage(firstPage)->getRotate());
    } else {
      box = new PDFRectangle(0, 0, 1, 1);
      writeHeader(firstPage, lastPage, box, box, 0);
      delete box;
    }
    if (mode != psModeForm) {
      writePS("%%BeginProlog\n");
    }
    writeXpdfProcset();
    if (mode != psModeForm) {
      writePS("%%EndProlog\n");
      writePS("%%BeginSetup\n");
    }
    writeDocSetup(catalog, firstPage, lastPage, duplexA);
    if (mode != psModeForm) {
      writePS("%%EndSetup\n");
    }
  }

  // initialize sequential page number
  seqPage = 1;
}

PSOutputDev::~PSOutputDev() {
  PSOutCustomColor *cc;
  int i;

  if (ok) {
    if (!manualCtrl) {
      writePS("%%Trailer\n");
      writeTrailer();
      if (mode != psModeForm) {
	writePS("%%EOF\n");
      }
    }
    if (fileType == psFile) {
#ifdef MACOS
      ICS_MapRefNumAndAssign((short)((FILE *)outputStream)->handle);
#endif
      fclose((FILE *)outputStream);
    }
#ifdef HAVE_POPEN
    else if (fileType == psPipe) {
      pclose((FILE *)outputStream);
#ifndef WIN32
      signal(SIGPIPE, (SignalFunc)SIG_DFL);
#endif
    }
#endif
  }
  if (embFontList) {
    delete embFontList;
  }
  if (fontIDs) {
    gfree(fontIDs);
  }
  if (fontFileIDs) {
    gfree(fontFileIDs);
  }
  if (fontFileNames) {
    for (i = 0; i < fontFileNameLen; ++i) {
      delete fontFileNames[i];
    }
    gfree(fontFileNames);
  }
  if (psFileNames) {
    for (i = 0; i < fontFileNameLen; ++i) {
      if (psFileNames[i])
        delete psFileNames[i];
    }
    gfree(psFileNames);
  }
  if (font16Enc) {
    for (i = 0; i < font16EncLen; ++i) {
      delete font16Enc[i].enc;
    }
    gfree(font16Enc);
  }
  if (xobjStack) {
    delete xobjStack;
  }
  while (customColors) {
    cc = customColors;
    customColors = cc->next;
    delete cc;
  }
}

void PSOutputDev::writeHeader(int firstPage, int lastPage,
			      PDFRectangle *mediaBox, PDFRectangle *cropBox,
			      int pageRotate) {
  double x1, y1, x2, y2;

  switch (mode) {
  case psModePS:
    writePS("%!PS-Adobe-3.0\n");
    writePSFmt("%%%%Creator: xpdf/pdftops %s\n", xpdfVersion);
    writePSFmt("%%%%LanguageLevel: %d\n",
	       (level == psLevel1 || level == psLevel1Sep) ? 1 :
	       (level == psLevel2 || level == psLevel2Sep) ? 2 : 3);
    if (level == psLevel1Sep || level == psLevel2Sep || level == psLevel3Sep) {
      writePS("%%DocumentProcessColors: (atend)\n");
      writePS("%%DocumentCustomColors: (atend)\n");
    }
    writePS("%%DocumentSuppliedResources: (atend)\n");
    writePSFmt("%%%%DocumentMedia: plain %d %d 0 () ()\n",
	       paperWidth, paperHeight);
    writePSFmt("%%%%BoundingBox: 0 0 %d %d\n", paperWidth, paperHeight);
    writePSFmt("%%%%Pages: %d\n", lastPage - firstPage + 1);
    writePS("%%EndComments\n");
    writePS("%%BeginDefaults\n");
    writePS("%%PageMedia: plain\n");
    writePS("%%EndDefaults\n");
    break;
  case psModeEPS:
    writePS("%!PS-Adobe-3.0 EPSF-3.0\n");
    writePSFmt("%%%%Creator: xpdf/pdftops %s\n", xpdfVersion);
    writePSFmt("%%%%LanguageLevel: %d\n",
	       (level == psLevel1 || level == psLevel1Sep) ? 1 :
	       (level == psLevel2 || level == psLevel2Sep) ? 2 : 3);
    if (level == psLevel1Sep || level == psLevel2Sep || level == psLevel3Sep) {
      writePS("%%DocumentProcessColors: (atend)\n");
      writePS("%%DocumentCustomColors: (atend)\n");
    }
    epsX1 = cropBox->x1;
    epsY1 = cropBox->y1;
    epsX2 = cropBox->x2;
    epsY2 = cropBox->y2;
    if (pageRotate == 0 || pageRotate == 180) {
      x1 = epsX1;
      y1 = epsY1;
      x2 = epsX2;
      y2 = epsY2;
    } else { // pageRotate == 90 || pageRotate == 270
      x1 = 0;
      y1 = 0;
      x2 = epsY2 - epsY1;
      y2 = epsX2 - epsX1;
    }
    writePSFmt("%%%%BoundingBox: %d %d %d %d\n",
	       (int)floor(x1), (int)floor(y1), (int)ceil(x2), (int)ceil(y2));
    if (floor(x1) != ceil(x1) || floor(y1) != ceil(y1) ||
	floor(x2) != ceil(x2) || floor(y2) != ceil(y2)) {
      writePSFmt("%%%%HiResBoundingBox: %g %g %g %g\n", x1, y1, x2, y2);
    }
    writePS("%%DocumentSuppliedResources: (atend)\n");
    writePS("%%EndComments\n");
    break;
  case psModeForm:
    writePS("%!PS-Adobe-3.0 Resource-Form\n");
    writePSFmt("%%%%Creator: xpdf/pdftops %s\n", xpdfVersion);
    writePSFmt("%%%%LanguageLevel: %d\n",
	       (level == psLevel1 || level == psLevel1Sep) ? 1 :
	       (level == psLevel2 || level == psLevel2Sep) ? 2 : 3);
    if (level == psLevel1Sep || level == psLevel2Sep || level == psLevel3Sep) {
      writePS("%%DocumentProcessColors: (atend)\n");
      writePS("%%DocumentCustomColors: (atend)\n");
    }
    writePS("%%DocumentSuppliedResources: (atend)\n");
    writePS("%%EndComments\n");
    writePS("32 dict dup begin\n");
    writePSFmt("/BBox [%d %d %d %d] def\n",
	       (int)floor(mediaBox->x1), (int)floor(mediaBox->y1),
	       (int)ceil(mediaBox->x2), (int)ceil(mediaBox->y2));
    writePS("/FormType 1 def\n");
    writePS("/Matrix [1 0 0 1 0 0] def\n");
    break;
  }
}

void PSOutputDev::writeXpdfProcset() {
  GBool lev1, lev2, lev3, sep, nonSep;
  char **p;
  char *q;

  writePSFmt("%%%%BeginResource: procset xpdf %s 0\n", xpdfVersion);
  lev1 = lev2 = lev3 = sep = nonSep = gTrue;
  for (p = prolog; *p; ++p) {
    if ((*p)[0] == '~') {
      lev1 = lev2 = lev3 = sep = nonSep = gFalse;
      for (q = *p + 1; *q; ++q) {
	switch (*q) {
	case '1': lev1 = gTrue; break;
	case '2': lev2 = gTrue; break;
	case '3': lev3 = gTrue; break;
	case 's': sep = gTrue; break;
	case 'n': nonSep = gTrue; break;
	}
      }
    } else if ((level == psLevel1 && lev1 && nonSep) ||
	       (level == psLevel1Sep && lev1 && sep) ||
	       (level == psLevel2 && lev2 && nonSep) ||
	       (level == psLevel2Sep && lev2 && sep) ||
	       (level == psLevel3 && lev3 && nonSep) ||
	       (level == psLevel3Sep && lev3 && sep)) {
      writePSFmt("%s\n", *p);
    }
  }
  writePS("%%EndResource\n");

  if (level >= psLevel3) {
    for (p = cmapProlog; *p; ++p) {
      writePSFmt("%s\n", *p);
    }
  }
}

void PSOutputDev::writeDocSetup(Catalog *catalog,
				int firstPage, int lastPage,
                                GBool duplexA) {
  Page *page;
  Dict *resDict;
  Annots *annots;
  Object obj1, obj2;
  int pg, i;

  if (mode == psModeForm) {
    // swap the form and xpdf dicts
    writePS("xpdf end begin dup begin\n");
  } else {
    writePS("xpdf begin\n");
  }
  for (pg = firstPage; pg <= lastPage; ++pg) {
    page = catalog->getPage(pg);
    if ((resDict = page->getResourceDict())) {
      setupResources(resDict);
    }
    annots = new Annots(xref, catalog, page->getAnnots(&obj1));
    obj1.free();
    for (i = 0; i < annots->getNumAnnots(); ++i) {
      if (annots->getAnnot(i)->getAppearance(&obj1)->isStream()) {
	obj1.streamGetDict()->lookup("Resources", &obj2);
	if (obj2.isDict()) {
	  setupResources(obj2.getDict());
	}
	obj2.free();
      }
      obj1.free();
    }
    delete annots;
  }
  if (mode != psModeForm) {
    if (mode != psModeEPS && !manualCtrl) {
      writePSFmt("%d %d %s pdfSetup\n",
		 paperWidth, paperHeight, duplexA ? "true" : "false");
    }
#if OPI_SUPPORT
    if (globalParams->getPSOPI()) {
      writePS("/opiMatrix matrix currentmatrix def\n");
    }
#endif
  }
}

void PSOutputDev::writePageTrailer() {
  if (mode != psModeForm) {
    writePS("pdfEndPage\n");
  }
}

void PSOutputDev::writeTrailer() {
  PSOutCustomColor *cc;

  if (mode == psModeForm) {
    writePS("/Foo exch /Form defineresource pop\n");
  } else {
    writePS("end\n");
    writePS("%%DocumentSuppliedResources:\n");
    writePS(embFontList->getCString());
    if (level == psLevel1Sep || level == psLevel2Sep ||
	level == psLevel3Sep) {
      writePS("%%DocumentProcessColors:");
      if (processColors & psProcessCyan) {
	writePS(" Cyan");
	 }
      if (processColors & psProcessMagenta) {
	writePS(" Magenta");
      }
      if (processColors & psProcessYellow) {
	writePS(" Yellow");
      }
      if (processColors & psProcessBlack) {
	writePS(" Black");
      }
      writePS("\n");
      writePS("%%DocumentCustomColors:");
      for (cc = customColors; cc; cc = cc->next) {
	writePSFmt(" (%s)", cc->name->getCString());
      }
      writePS("\n");
      writePS("%%CMYKCustomColor:\n");
      for (cc = customColors; cc; cc = cc->next) {
	writePSFmt("%%%%+ %g %g %g %g (%s)\n",
		   cc->c, cc->m, cc->y, cc->k, cc->name->getCString());
      }
    }
  }
}

void PSOutputDev::setupResources(Dict *resDict) {
  Object xObjDict, xObjRef, xObj, patDict, patRef, pat, resObj;
  Ref ref0, ref1;
  GBool skip;
  int i, j;

  setupFonts(resDict);
  setupImages(resDict);

  //----- recursively scan XObjects
  resDict->lookup("XObject", &xObjDict);
  if (xObjDict.isDict()) {
    for (i = 0; i < xObjDict.dictGetLength(); ++i) {

      // avoid infinite recursion on XObjects
      skip = gFalse;
      if ((xObjDict.dictGetValNF(i, &xObjRef)->isRef())) {
	ref0 = xObjRef.getRef();
	for (j = 0; j < xobjStack->getLength(); ++j) {
	  ref1 = *(Ref *)xobjStack->get(j);
	  if (ref1.num == ref0.num && ref1.gen == ref0.gen) {
	    skip = gTrue;
	    break;
	  }
	}
	if (!skip) {
	  xobjStack->append(&ref0);
	}
      }
      if (!skip) {

	// process the XObject's resource dictionary
	xObjDict.dictGetVal(i, &xObj);
	if (xObj.isStream()) {
	  xObj.streamGetDict()->lookup("Resources", &resObj);
	  if (resObj.isDict()) {
	    setupResources(resObj.getDict());
	  }
	  resObj.free();
	}
	xObj.free();
      }

      if (xObjRef.isRef() && !skip) {
	xobjStack->del(xobjStack->getLength() - 1);
      }
      xObjRef.free();
    }
  }
  xObjDict.free();

  //----- recursively scan Patterns
  resDict->lookup("Pattern", &patDict);
  if (patDict.isDict()) {
    inType3Char = gTrue;
    for (i = 0; i < patDict.dictGetLength(); ++i) {

      // avoid infinite recursion on Patterns
      skip = gFalse;
      if ((patDict.dictGetValNF(i, &patRef)->isRef())) {
	ref0 = patRef.getRef();
	for (j = 0; j < xobjStack->getLength(); ++j) {
	  ref1 = *(Ref *)xobjStack->get(j);
	  if (ref1.num == ref0.num && ref1.gen == ref0.gen) {
	    skip = gTrue;
	    break;
	  }
	}
	if (!skip) {
	  xobjStack->append(&ref0);
	}
      }
      if (!skip) {

	// process the Pattern's resource dictionary
	patDict.dictGetVal(i, &pat);
	if (pat.isStream()) {
	  pat.streamGetDict()->lookup("Resources", &resObj);
	  if (resObj.isDict()) {
	    setupResources(resObj.getDict());
	  }
	  resObj.free();
	}
	pat.free();
      }

      if (patRef.isRef() && !skip) {
	xobjStack->del(xobjStack->getLength() - 1);
      }
      patRef.free();
    }
    inType3Char = gFalse;
  }
  patDict.free();
}

void PSOutputDev::setupFonts(Dict *resDict) {
  Object obj1, obj2;
  Ref r;
  GfxFontDict *gfxFontDict;
  GfxFont *font;
  int i;

  gfxFontDict = NULL;
  resDict->lookupNF("Font", &obj1);
  if (obj1.isRef()) {
    obj1.fetch(xref, &obj2);
    if (obj2.isDict()) {
      r = obj1.getRef();
      gfxFontDict = new GfxFontDict(xref, &r, obj2.getDict());
    }
    obj2.free();
  } else if (obj1.isDict()) {
    gfxFontDict = new GfxFontDict(xref, NULL, obj1.getDict());
  }
  if (gfxFontDict) {
    for (i = 0; i < gfxFontDict->getNumFonts(); ++i) {
      if ((font = gfxFontDict->getFont(i))) {
	setupFont(font, resDict);
      }
    }
    delete gfxFontDict;
  }
  obj1.free();
}

void PSOutputDev::setupFont(GfxFont *font, Dict *parentResDict) {
  Ref fontFileID;
  GooString *name;
  PSFontParam *fontParam;
  GooString *psName;
  char type3Name[64], buf[16];
  GBool subst;
  UnicodeMap *uMap;
  char *charName;
  double xs, ys;
  int code;
  double w1, w2;
  double *fm;
  int i, j;
  DisplayFontParam *dfp;

  // check if font is already set up
  for (i = 0; i < fontIDLen; ++i) {
    if (fontIDs[i].num == font->getID()->num &&
	fontIDs[i].gen == font->getID()->gen) {
      return;
    }
  }

  // add entry to fontIDs list
  if (fontIDLen >= fontIDSize) {
    fontIDSize += 64;
    fontIDs = (Ref *)greallocn(fontIDs, fontIDSize, sizeof(Ref));
  }
  fontIDs[fontIDLen++] = *font->getID();

  xs = ys = 1;
  subst = gFalse;

  // check for resident 8-bit font
  if (font->getName() &&
      (fontParam = globalParams->getPSFont(font->getName()))) {
    psName = new GooString(fontParam->psFontName->getCString());

  // check for embedded Type 1 font
  } else if (globalParams->getPSEmbedType1() &&
	     font->getType() == fontType1 &&
	     font->getEmbeddedFontID(&fontFileID)) {
    psName = filterPSName(font->getEmbeddedFontName());
    setupEmbeddedType1Font(&fontFileID, psName);

  // check for embedded Type 1C font
  } else if (globalParams->getPSEmbedType1() &&
	     font->getType() == fontType1C &&
	     font->getEmbeddedFontID(&fontFileID)) {
    psName = filterPSName(font->getEmbeddedFontName());
    setupEmbeddedType1CFont(font, &fontFileID, psName);

  // check for external Type 1 font file
  } else if (globalParams->getPSEmbedType1() &&
	     font->getType() == fontType1 &&
	     font->getExtFontFile()) {
    // this assumes that the PS font name matches the PDF font name
    psName = font->getName()->copy();
    setupExternalType1Font(font->getExtFontFile(), psName);

  // check for embedded TrueType font
  } else if (globalParams->getPSEmbedTrueType() &&
	     font->getType() == fontTrueType &&
	     font->getEmbeddedFontID(&fontFileID)) {
    psName = filterPSName(font->getEmbeddedFontName());
    setupEmbeddedTrueTypeFont(font, &fontFileID, psName);

  // check for external TrueType font file
  } else if (globalParams->getPSEmbedTrueType() &&
	     font->getType() == fontTrueType &&
	     font->getExtFontFile()) {
    psName = setupExternalTrueTypeFont(font);

  // check for embedded CID PostScript font
  } else if (globalParams->getPSEmbedCIDPostScript() &&
	     font->getType() == fontCIDType0C &&
	     font->getEmbeddedFontID(&fontFileID)) {
    psName = filterPSName(font->getEmbeddedFontName());
    setupEmbeddedCIDType0Font(font, &fontFileID, psName);

  // check for embedded CID TrueType font
  } else if (globalParams->getPSEmbedCIDTrueType() &&
	     font->getType() == fontCIDType2 &&
	     font->getEmbeddedFontID(&fontFileID)) {
    psName = filterPSName(font->getEmbeddedFontName());
    setupEmbeddedCIDTrueTypeFont(font, &fontFileID, psName, gTrue);

  } else if (font->getType() == fontType3) {
    sprintf(type3Name, "T3_%d_%d",
	    font->getID()->num, font->getID()->gen);
    psName = new GooString(type3Name);
    setupType3Font(font, psName, parentResDict);

  // check for external CID TrueType font file
  } else if (globalParams->getPSEmbedCIDTrueType() &&
	     font->getType() == fontCIDType2 &&
	     font->getExtFontFile()) {
    psName = setupExternalCIDTrueTypeFont(font, font->getExtFontFile());

  // do 8-bit font substitution
  } else if (!font->isCIDFont()) {
    subst = gTrue;
    name = font->getName();
    psName = NULL;
    if (name) {
      for (i = 0; psFonts[i]; ++i) {
	if (name->cmp(psFonts[i]) == 0) {
	  psName = new GooString(psFonts[i]);
	  break;
	}
      }
    }
    if (!psName) {
      if (font->isFixedWidth()) {
	i = 8;
      } else if (font->isSerif()) {
	i = 4;
      } else {
	i = 0;
      }
      if (font->isBold()) {
	i += 2;
      }
      if (font->isItalic()) {
	i += 1;
      }
      psName = new GooString(psSubstFonts[i].psName);
      for (code = 0; code < 256; ++code) {
	if ((charName = ((Gfx8BitFont *)font)->getCharName(code)) &&
	    charName[0] == 'm' && charName[1] == '\0') {
	  break;
	}
      }
      if (code < 256) {
	w1 = ((Gfx8BitFont *)font)->getWidth(code);
      } else {
	w1 = 0;
      }
      w2 = psSubstFonts[i].mWidth;
      xs = w1 / w2;
      if (xs < 0.1) {
	xs = 1;
      }
      if (font->getType() == fontType3) {
	// This is a hack which makes it possible to substitute for some
	// Type 3 fonts.  The problem is that it's impossible to know what
	// the base coordinate system used in the font is without actually
	// rendering the font.
	ys = xs;
	fm = font->getFontMatrix();
	if (fm[0] != 0) {
	  ys *= fm[3] / fm[0];
	}
      } else {
	ys = 1;
      }
    }

  // do 16-bit font substitution
  } else if ((fontParam = globalParams->
	        getPSFont16(font->getName(),
			    ((GfxCIDFont *)font)->getCollection(),
			    font->getWMode()))) {
    subst = gTrue;
    psName = fontParam->psFontName->copy();
    if (font16EncLen >= font16EncSize) {
      font16EncSize += 16;
      font16Enc = (PSFont16Enc *)greallocn(font16Enc,
					   font16EncSize, sizeof(PSFont16Enc));
    }
    font16Enc[font16EncLen].fontID = *font->getID();
    font16Enc[font16EncLen].enc = fontParam->encoding->copy();
    if ((uMap = globalParams->getUnicodeMap(font16Enc[font16EncLen].enc))) {
      uMap->decRefCnt();
      ++font16EncLen;
    } else {
      error(-1, "Couldn't find Unicode map for 16-bit font encoding '%s'",
	    font16Enc[font16EncLen].enc->getCString());
    }

  // try the display font for embedding
  } else if (globalParams->getPSEmbedCIDTrueType() &&
	     ((GfxCIDFont *)font)->getCollection() &&
	     (dfp = globalParams->
	      getDisplayFont(font)) &&
	     dfp->kind == displayFontTT) {
    psName = setupExternalCIDTrueTypeFont(font, dfp->tt.fileName, dfp->tt.faceIndex);

  // give up - can't do anything with this font
  } else {
    error(-1, "Couldn't find a font to substitute for '%s' ('%s' character collection)",
	  font->getName() ? font->getName()->getCString() : "(unnamed)",
	  ((GfxCIDFont *)font)->getCollection()
	    ? ((GfxCIDFont *)font)->getCollection()->getCString()
	    : "(unknown)");
    return;
  }

  // generate PostScript code to set up the font
  if (font->isCIDFont()) {
    if (level == psLevel3 || level == psLevel3Sep) {
      writePSFmt("/F%d_%d /%s %d pdfMakeFont16L3\n",
		 font->getID()->num, font->getID()->gen, psName->getCString(),
		 font->getWMode());
    } else {
      writePSFmt("/F%d_%d /%s %d pdfMakeFont16\n",
		 font->getID()->num, font->getID()->gen, psName->getCString(),
		 font->getWMode());
    }
  } else {
    writePSFmt("/F%d_%d /%s %g %g\n",
	       font->getID()->num, font->getID()->gen, psName->getCString(),
	       xs, ys);
    for (i = 0; i < 256; i += 8) {
      writePSFmt((i == 0) ? "[ " : "  ");
      for (j = 0; j < 8; ++j) {
	if (font->getType() == fontTrueType &&
	    !subst &&
	    !((Gfx8BitFont *)font)->getHasEncoding()) {
	  sprintf(buf, "c%02x", i+j);
	  charName = buf;
	} else {
	  charName = ((Gfx8BitFont *)font)->getCharName(i+j);
	  // this is a kludge for broken PDF files that encode char 32
	  // as .notdef
	  if (i+j == 32 && charName && !strcmp(charName, ".notdef")) {
	    charName = "space";
	  }
	}
	writePS("/");
	writePSName(charName ? charName : (char *)".notdef");
	// the empty name is legal in PDF and PostScript, but PostScript
	// uses a double-slash (//...) for "immediately evaluated names",
	// so we need to add a space character here
	if (charName && !charName[0]) {
	  writePS(" ");
	}
      }
      writePS((i == 256-8) ? (char *)"]\n" : (char *)"\n");
    }
    writePS("pdfMakeFont\n");
  }

  delete psName;
}

void PSOutputDev::setupEmbeddedType1Font(Ref *id, GooString *psName) {
  static char hexChar[17] = "0123456789abcdef";
  Object refObj, strObj, obj1, obj2, obj3;
  Dict *dict;
  int length1, length2, length3;
  int c;
  int start[4];
  GBool binMode;
  int i;

  // check if font is already embedded
  for (i = 0; i < fontFileIDLen; ++i) {
    if (fontFileIDs[i].num == id->num &&
	fontFileIDs[i].gen == id->gen)
      return;
  }

  // add entry to fontFileIDs list
  if (fontFileIDLen >= fontFileIDSize) {
    fontFileIDSize += 64;
    fontFileIDs = (Ref *)greallocn(fontFileIDs, fontFileIDSize, sizeof(Ref));
  }
  fontFileIDs[fontFileIDLen++] = *id;

  // get the font stream and info
  refObj.initRef(id->num, id->gen);
  refObj.fetch(xref, &strObj);
  refObj.free();
  if (!strObj.isStream()) {
    error(-1, "Embedded font file object is not a stream");
    goto err1;
  }
  if (!(dict = strObj.streamGetDict())) {
    error(-1, "Embedded font stream is missing its dictionary");
    goto err1;
  }
  dict->lookup("Length1", &obj1);
  dict->lookup("Length2", &obj2);
  dict->lookup("Length3", &obj3);
  if (!obj1.isInt() || !obj2.isInt() || !obj3.isInt()) {
    error(-1, "Missing length fields in embedded font stream dictionary");
    obj1.free();
    obj2.free();
    obj3.free();
    goto err1;
  }
  length1 = obj1.getInt();
  length2 = obj2.getInt();
  length3 = obj3.getInt();
  obj1.free();
  obj2.free();
  obj3.free();

  // beginning comment
  writePSFmt("%%%%BeginResource: font %s\n", psName->getCString());
  embFontList->append("%%+ font ");
  embFontList->append(psName->getCString());
  embFontList->append("\n");

  // copy ASCII portion of font
  strObj.streamReset();
  for (i = 0; i < length1 && (c = strObj.streamGetChar()) != EOF; ++i) {
    writePSChar(c);
  }

  // figure out if encrypted portion is binary or ASCII
  binMode = gFalse;
  for (i = 0; i < 4; ++i) {
    start[i] = strObj.streamGetChar();
    if (start[i] == EOF) {
      error(-1, "Unexpected end of file in embedded font stream");
      goto err1;
    }
    if (!((start[i] >= '0' && start[i] <= '9') ||
	  (start[i] >= 'A' && start[i] <= 'F') ||
	  (start[i] >= 'a' && start[i] <= 'f')))
      binMode = gTrue;
  }

  // convert binary data to ASCII
  if (binMode) {
    for (i = 0; i < 4; ++i) {
      writePSChar(hexChar[(start[i] >> 4) & 0x0f]);
      writePSChar(hexChar[start[i] & 0x0f]);
    }
#if 0 // this causes trouble for various PostScript printers
    // if Length2 is incorrect (too small), font data gets chopped, so
    // we take a few extra characters from the trailer just in case
    length2 += length3 >= 8 ? 8 : length3;
#endif
    while (i < length2) {
      if ((c = strObj.streamGetChar()) == EOF) {
	break;
      }
      writePSChar(hexChar[(c >> 4) & 0x0f]);
      writePSChar(hexChar[c & 0x0f]);
      if (++i % 32 == 0) {
	writePSChar('\n');
      }
    }
    if (i % 32 > 0) {
      writePSChar('\n');
    }

  // already in ASCII format -- just copy it
  } else {
    for (i = 0; i < 4; ++i) {
      writePSChar(start[i]);
    }
    for (i = 4; i < length2; ++i) {
      if ((c = strObj.streamGetChar()) == EOF) {
	break;
      }
      writePSChar(c);
    }
  }

  // write padding and "cleartomark"
  for (i = 0; i < 8; ++i) {
    writePS("00000000000000000000000000000000"
	    "00000000000000000000000000000000\n");
  }
  writePS("cleartomark\n");

  // ending comment
  writePS("%%EndResource\n");

 err1:
  strObj.streamClose();
  strObj.free();
}

//~ This doesn't handle .pfb files or binary eexec data (which only
//~ happens in pfb files?).
void PSOutputDev::setupExternalType1Font(GooString *fileName, GooString *psName) {
  FILE *fontFile;
  int c;
  int i;

  // check if font is already embedded
  for (i = 0; i < fontFileNameLen; ++i) {
    if (!fontFileNames[i]->cmp(fileName)) {
      return;
    }
  }

  // add entry to fontFileNames list
  if (fontFileNameLen >= fontFileNameSize) {
    fontFileNameSize += 64;
    fontFileNames = (GooString **)greallocn(fontFileNames,
					  fontFileNameSize, sizeof(GooString *));
    psFileNames = (GooString **)greallocn(psFileNames,
				       fontFileNameSize, sizeof(GooString *));
  }
  fontFileNames[fontFileNameLen] = fileName->copy();
  psFileNames[fontFileNameLen] = psName->copy();
  fontFileNameLen++;

  // beginning comment
  writePSFmt("%%%%BeginResource: font %s\n", psName->getCString());
  embFontList->append("%%+ font ");
  embFontList->append(psName->getCString());
  embFontList->append("\n");

  // copy the font file
  if (!(fontFile = fopen(fileName->getCString(), "rb"))) {
    error(-1, "Couldn't open external font file");
    return;
  }
  while ((c = fgetc(fontFile)) != EOF) {
    writePSChar(c);
  }
  fclose(fontFile);

  // ending comment
  writePS("%%EndResource\n");
}

void PSOutputDev::setupEmbeddedType1CFont(GfxFont *font, Ref *id,
					  GooString *psName) {
  char *fontBuf;
  int fontLen;
  FoFiType1C *ffT1C;
  int i;

  // check if font is already embedded
  for (i = 0; i < fontFileIDLen; ++i) {
    if (fontFileIDs[i].num == id->num &&
	fontFileIDs[i].gen == id->gen)
      return;
  }

  // add entry to fontFileIDs list
  if (fontFileIDLen >= fontFileIDSize) {
    fontFileIDSize += 64;
    fontFileIDs = (Ref *)greallocn(fontFileIDs, fontFileIDSize, sizeof(Ref));
  }
  fontFileIDs[fontFileIDLen++] = *id;

  // beginning comment
  writePSFmt("%%%%BeginResource: font %s\n", psName->getCString());
  embFontList->append("%%+ font ");
  embFontList->append(psName->getCString());
  embFontList->append("\n");

  // convert it to a Type 1 font
  fontBuf = font->readEmbFontFile(xref, &fontLen);
  if ((ffT1C = FoFiType1C::make(fontBuf, fontLen))) {
    ffT1C->convertToType1(NULL, gTrue, outputFunc, outputStream);
    delete ffT1C;
  }
  gfree(fontBuf);

  // ending comment
  writePS("%%EndResource\n");
}

void PSOutputDev::setupEmbeddedTrueTypeFont(GfxFont *font, Ref *id,
					    GooString *psName) {
  char unique[32];
  char *fontBuf;
  int fontLen;
  FoFiTrueType *ffTT;
  Gushort *codeToGID;
  int i;

  // check if font is already embedded
  for (i = 0; i < fontFileIDLen; ++i) {
    if (fontFileIDs[i].num == id->num &&
	fontFileIDs[i].gen == id->gen) {
      sprintf(unique, "_%d", nextTrueTypeNum++);
      psName->append(unique);
      break;
    }
  }

  // add entry to fontFileIDs list
  if (i == fontFileIDLen) {
    if (fontFileIDLen >= fontFileIDSize) {
      fontFileIDSize += 64;
      fontFileIDs = (Ref *)greallocn(fontFileIDs, fontFileIDSize, sizeof(Ref));
    }
    fontFileIDs[fontFileIDLen++] = *id;
  }

  // beginning comment
  writePSFmt("%%%%BeginResource: font %s\n", psName->getCString());
  embFontList->append("%%+ font ");
  embFontList->append(psName->getCString());
  embFontList->append("\n");

  // convert it to a Type 42 font
  fontBuf = font->readEmbFontFile(xref, &fontLen);
  if ((ffTT = FoFiTrueType::make(fontBuf, fontLen))) {
    codeToGID = ((Gfx8BitFont *)font)->getCodeToGIDMap(ffTT);
    ffTT->convertToType42(psName->getCString(),
			  ((Gfx8BitFont *)font)->getHasEncoding()
			    ? ((Gfx8BitFont *)font)->getEncoding()
			    : (char **)NULL,
			  codeToGID, outputFunc, outputStream);
    gfree(codeToGID);
    delete ffTT;
  }
  gfree(fontBuf);

  // ending comment
  writePS("%%EndResource\n");
}

GooString *PSOutputDev::setupExternalTrueTypeFont(GfxFont *font) {
  GooString *fileName;
  char *fontBuf;
  int fontLen;
  FoFiTrueType *ffTT;
  Gushort *codeToGID;
  GooString *psName;
  int i;

  // check if font is already embedded
  fileName = font->getExtFontFile();
  for (i = 0; i < fontFileNameLen; ++i) {
    if (!fontFileNames[i]->cmp(fileName)) {
      return psFileNames[i]->copy();
    }
  }

  psName = filterPSName(font->getName());
  // add entry to fontFileNames list
  if (i == fontFileNameLen) {
    if (fontFileNameLen >= fontFileNameSize) {
      fontFileNameSize += 64;
      fontFileNames =
	(GooString **)greallocn(fontFileNames,
			      fontFileNameSize, sizeof(GooString *));
      psFileNames =
	(GooString **)greallocn(psFileNames,
			     fontFileNameSize, sizeof(GooString *));
    }
  }
  fontFileNames[fontFileNameLen] = fileName->copy();
  psFileNames[fontFileNameLen] = psName->copy();
  fontFileNameLen++;

  // beginning comment
  writePSFmt("%%%%BeginResource: font %s\n", psName->getCString());
  embFontList->append("%%+ font ");
  embFontList->append(psName->getCString());
  embFontList->append("\n");

  // convert it to a Type 42 font
  fontBuf = font->readExtFontFile(&fontLen);
  if ((ffTT = FoFiTrueType::make(fontBuf, fontLen))) {
    codeToGID = ((Gfx8BitFont *)font)->getCodeToGIDMap(ffTT);
    ffTT->convertToType42(psName->getCString(),
			  ((Gfx8BitFont *)font)->getHasEncoding()
			    ? ((Gfx8BitFont *)font)->getEncoding()
			    : (char **)NULL,
			  codeToGID, outputFunc, outputStream);
    delete ffTT;
    gfree(codeToGID);
 }
  gfree(fontBuf);

  // ending comment
  writePS("%%EndResource\n");
  return psName;
}

GooString *PSOutputDev::setupExternalCIDTrueTypeFont(GfxFont *font, GooString *fileName, int faceIndex) {
//   char *fontBuf;
//   int fontLen;
  FoFiTrueType *ffTT;
  Gushort *codeToGID;
  GooString *psName;
  int i;
  GooString *myFileName;

  myFileName = fileName->copy();
  if (faceIndex > 0) {
    char tmp[32];
    sprintf(tmp, ",%d", faceIndex);
    myFileName->append(tmp);
  }
  // check if font is already embedded
  for (i = 0; i < fontFileNameLen; ++i) {
    if (!fontFileNames[i]->cmp(myFileName)) {
      delete myFileName;
      return psFileNames[i]->copy();
    }
  }

  psName = filterPSName(font->getName());
  // add entry to fontFileNames list
  if (i == fontFileNameLen) {
    if (fontFileNameLen >= fontFileNameSize) {
      fontFileNameSize += 64;
      fontFileNames =
	(GooString **)grealloc(fontFileNames,
			     fontFileNameSize * sizeof(GooString *));
      psFileNames =
	(GooString **)grealloc(psFileNames,
			     fontFileNameSize * sizeof(GooString *));
    }
  }
  fontFileNames[fontFileNameLen] = myFileName;
  psFileNames[fontFileNameLen] = psName->copy();
  fontFileNameLen++;

  // beginning comment
  writePSFmt("%%%%BeginResource: font %s\n", psName->getCString());
  embFontList->append("%%+ font ");
  embFontList->append(psName->getCString());
  embFontList->append("\n");

  // convert it to a CID type2 font
  if ((ffTT = FoFiTrueType::load(fileName->getCString(), faceIndex))) {
      int n = ((GfxCIDFont *)font)->getCIDToGIDLen();
      if (n) {
	codeToGID = (Gushort *)gmalloc(n * sizeof(Gushort));
	memcpy(codeToGID, ((GfxCIDFont *)font)->getCIDToGID(), n * sizeof(Gushort));
      } else {
	codeToGID = ((GfxCIDFont *)font)->getCodeToGIDMap(ffTT, &n);
      }
      if (globalParams->getPSLevel() >= psLevel3) {
	// Level 3: use a CID font
	ffTT->convertToCIDType2(psName->getCString(),
				codeToGID, n, gTrue,
				outputFunc, outputStream);
      } else {
	// otherwise: use a non-CID composite font
	ffTT->convertToType0(psName->getCString(),
			     codeToGID, n, gTrue,
			     outputFunc, outputStream);
      }
      gfree(codeToGID);
      delete ffTT;
  }

  // ending comment
  writePS("%%EndResource\n");
  return psName;
}

void PSOutputDev::setupEmbeddedCIDType0Font(GfxFont *font, Ref *id,
					    GooString *psName) {
  char *fontBuf;
  int fontLen;
  FoFiType1C *ffT1C;
  int i;

  // check if font is already embedded
  for (i = 0; i < fontFileIDLen; ++i) {
    if (fontFileIDs[i].num == id->num &&
	fontFileIDs[i].gen == id->gen)
      return;
  }

  // add entry to fontFileIDs list
  if (fontFileIDLen >= fontFileIDSize) {
    fontFileIDSize += 64;
    fontFileIDs = (Ref *)greallocn(fontFileIDs, fontFileIDSize, sizeof(Ref));
  }
  fontFileIDs[fontFileIDLen++] = *id;

  // beginning comment
  writePSFmt("%%%%BeginResource: font %s\n", psName->getCString());
  embFontList->append("%%+ font ");
  embFontList->append(psName->getCString());
  embFontList->append("\n");

  // convert it to a Type 0 font
  fontBuf = font->readEmbFontFile(xref, &fontLen);
  if ((ffT1C = FoFiType1C::make(fontBuf, fontLen))) {
    if (globalParams->getPSLevel() >= psLevel3) {
      // Level 3: use a CID font
      ffT1C->convertToCIDType0(psName->getCString(), outputFunc, outputStream);
    } else {
      // otherwise: use a non-CID composite font
      ffT1C->convertToType0(psName->getCString(), outputFunc, outputStream);
    }
    delete ffT1C;
  }
  gfree(fontBuf);

  // ending comment
  writePS("%%EndResource\n");
}

void PSOutputDev::setupEmbeddedCIDTrueTypeFont(GfxFont *font, Ref *id,
					       GooString *psName,
					       GBool needVerticalMetrics) {
  char unique[32];
  char *fontBuf;
  int fontLen;
  FoFiTrueType *ffTT;
  int i;

  // check if font is already embedded
  for (i = 0; i < fontFileIDLen; ++i) {
    if (fontFileIDs[i].num == id->num &&
	fontFileIDs[i].gen == id->gen) {
      sprintf(unique, "_%d", nextTrueTypeNum++);
      psName->append(unique);
      break;
    }
  }

  // add entry to fontFileIDs list
  if (fontFileIDLen >= fontFileIDSize) {
    fontFileIDSize += 64;
    fontFileIDs = (Ref *)greallocn(fontFileIDs, fontFileIDSize, sizeof(Ref));
  }
  fontFileIDs[fontFileIDLen++] = *id;

  // beginning comment
  writePSFmt("%%%%BeginResource: font %s\n", psName->getCString());
  embFontList->append("%%+ font ");
  embFontList->append(psName->getCString());
  embFontList->append("\n");

  // convert it to a Type 0 font
  fontBuf = font->readEmbFontFile(xref, &fontLen);
  if ((ffTT = FoFiTrueType::make(fontBuf, fontLen))) {
    if (globalParams->getPSLevel() >= psLevel3) {
      // Level 3: use a CID font
      ffTT->convertToCIDType2(psName->getCString(),
			      ((GfxCIDFont *)font)->getCIDToGID(),
			      ((GfxCIDFont *)font)->getCIDToGIDLen(),
			      needVerticalMetrics,
			      outputFunc, outputStream);
    } else {
      // otherwise: use a non-CID composite font
      ffTT->convertToType0(psName->getCString(),
			   ((GfxCIDFont *)font)->getCIDToGID(),
			   ((GfxCIDFont *)font)->getCIDToGIDLen(),
			   needVerticalMetrics,
			   outputFunc, outputStream);
    }
    delete ffTT;
  }
  gfree(fontBuf);

  // ending comment
  writePS("%%EndResource\n");
}

void PSOutputDev::setupType3Font(GfxFont *font, GooString *psName,
				 Dict *parentResDict) {
  Dict *resDict;
  Dict *charProcs;
  Object charProc;
  Gfx *gfx;
  PDFRectangle box;
  double *m;
  char buf[256];
  int i;

  // set up resources used by font
  if ((resDict = ((Gfx8BitFont *)font)->getResources())) {
    inType3Char = gTrue;
    setupResources(resDict);
    inType3Char = gFalse;
  } else {
    resDict = parentResDict;
  }

  // beginning comment
  writePSFmt("%%%%BeginResource: font %s\n", psName->getCString());
  embFontList->append("%%+ font ");
  embFontList->append(psName->getCString());
  embFontList->append("\n");

  // font dictionary
  writePS("8 dict begin\n");
  writePS("/FontType 3 def\n");
  m = font->getFontMatrix();
  writePSFmt("/FontMatrix [%g %g %g %g %g %g] def\n",
	     m[0], m[1], m[2], m[3], m[4], m[5]);
  m = font->getFontBBox();
  writePSFmt("/FontBBox [%g %g %g %g] def\n",
	     m[0], m[1], m[2], m[3]);
  writePS("/Encoding 256 array def\n");
  writePS("  0 1 255 { Encoding exch /.notdef put } for\n");
  writePS("/BuildGlyph {\n");
  writePS("  exch /CharProcs get exch\n");
  writePS("  2 copy known not { pop /.notdef } if\n");
  writePS("  get exec\n");
  writePS("} bind def\n");
  writePS("/BuildChar {\n");
  writePS("  1 index /Encoding get exch get\n");
  writePS("  1 index /BuildGlyph get exec\n");
  writePS("} bind def\n");
  if ((charProcs = ((Gfx8BitFont *)font)->getCharProcs())) {
    writePSFmt("/CharProcs %d dict def\n", charProcs->getLength());
    writePS("CharProcs begin\n");
    box.x1 = m[0];
    box.y1 = m[1];
    box.x2 = m[2];
    box.y2 = m[3];
    gfx = new Gfx(xref, this, resDict, &box, NULL);
    inType3Char = gTrue;
    t3Cacheable = gFalse;
    for (i = 0; i < charProcs->getLength(); ++i) {
      writePS("/");
      char *aux = charProcs->getKey(i)->getCStringCopy();
      writePSName(aux);
      delete[] aux;
      writePS(" {\n");
      gfx->display(charProcs->getVal(i, &charProc));
      charProc.free();
      if (t3String) {
	if (t3Cacheable) {
	  sprintf(buf, "%g %g %g %g %g %g setcachedevice\n",
		  t3WX, t3WY, t3LLX, t3LLY, t3URX, t3URY);
	} else {
	  sprintf(buf, "%g %g setcharwidth\n", t3WX, t3WY);
	}
	(*outputFunc)(outputStream, buf, strlen(buf));
	(*outputFunc)(outputStream, t3String->getCString(),
		      t3String->getLength());
	delete t3String;
	t3String = NULL;
      }
      (*outputFunc)(outputStream, "Q\n", 2);
      writePS("} def\n");
    }
    inType3Char = gFalse;
    delete gfx;
    writePS("end\n");
  }
  writePS("currentdict end\n");
  writePSFmt("/%s exch definefont pop\n", psName->getCString());

  // ending comment
  writePS("%%EndResource\n");
}

void PSOutputDev::setupImages(Dict *resDict) {
  Object xObjDict, xObj, xObjRef, subtypeObj;
  int i;

  if (!(mode == psModeForm || inType3Char)) {
    return;
  }

  //----- recursively scan XObjects
  resDict->lookup("XObject", &xObjDict);
  if (xObjDict.isDict()) {
    for (i = 0; i < xObjDict.dictGetLength(); ++i) {
      xObjDict.dictGetValNF(i, &xObjRef);
      xObjDict.dictGetVal(i, &xObj);
      if (xObj.isStream()) {
	xObj.streamGetDict()->lookup("Subtype", &subtypeObj);
	if (subtypeObj.isName("Image")) {
	  if (xObjRef.isRef()) {
	    setupImage(xObjRef.getRef(), xObj.getStream());
	  } else {
	    error(-1, "Image in resource dict is not an indirect reference");
	  }
	}
	subtypeObj.free();
      }
      xObj.free();
      xObjRef.free();
    }
  }
  xObjDict.free();
}

void PSOutputDev::setupImage(Ref id, Stream *str) {
  GBool useASCIIHex;
  int c;
  int size, line, col, i;

  // construct an encoder stream
  useASCIIHex = level == psLevel1 || level == psLevel1Sep ||
                globalParams->getPSASCIIHex();
  if (useASCIIHex) {
    str = new ASCIIHexEncoder(str);
  } else {
    str = new ASCII85Encoder(str);
  }

  // compute image data size
  str->reset();
  col = size = 0;
  do {
    do {
      c = str->getChar();
    } while (c == '\n' || c == '\r');
    if (c == (useASCIIHex ? '>' : '~') || c == EOF) {
      break;
    }
    if (c == 'z') {
      ++col;
    } else {
      ++col;
      for (i = 1; i <= (useASCIIHex ? 1 : 4); ++i) {
	do {
	  c = str->getChar();
	} while (c == '\n' || c == '\r');
	if (c == (useASCIIHex ? '>' : '~') || c == EOF) {
	  break;
	}
	++col;
      }
    }
    if (col > 225) {
      ++size;
      col = 0;
    }
  } while (c != (useASCIIHex ? '>' : '~') && c != EOF);
  ++size;
  writePSFmt("%d array dup /ImData_%d_%d exch def\n", size, id.num, id.gen);
  str->close();

  // write the data into the array
  str->reset();
  line = col = 0;
  writePS((char *)(useASCIIHex ? "dup 0 <" : "dup 0 <~"));
  do {
    do {
      c = str->getChar();
    } while (c == '\n' || c == '\r');
    if (c == (useASCIIHex ? '>' : '~') || c == EOF) {
      break;
    }
    if (c == 'z') {
      writePSChar(c);
      ++col;
    } else {
      writePSChar(c);
      ++col;
      for (i = 1; i <= (useASCIIHex ? 1 : 4); ++i) {
	do {
	  c = str->getChar();
	} while (c == '\n' || c == '\r');
	if (c == (useASCIIHex ? '>' : '~') || c == EOF) {
	  break;
	}
	writePSChar(c);
	++col;
      }
    }
    // each line is: "dup nnnnn <~...data...~> put<eol>"
    // so max data length = 255 - 20 = 235
    // chunks are 1 or 4 bytes each, so we have to stop at 232
    // but make it 225 just to be safe
    if (col > 225) {
      writePS((char *)(useASCIIHex ? "> put\n" : "~> put\n"));
      ++line;
      writePSFmt((char *)(useASCIIHex ? "dup %d <" : "dup %d <~"), line);
      col = 0;
    }
  } while (c != (useASCIIHex ? '>' : '~') && c != EOF);
  writePS((char *)(useASCIIHex ? "> put\n" : "~> put\n"));
  writePS("pop\n");
  str->close();

  delete str;
}

void PSOutputDev::startPage(int pageNum, GfxState *state) {
  int x1, y1, x2, y2, width, height;
  int imgWidth, imgHeight, imgWidth2, imgHeight2;
  GBool landscape;


  if (mode == psModePS) {
    writePSFmt("%%%%Page: %d %d\n", pageNum, seqPage);
    writePS("%%BeginPageSetup\n");
  }

  // underlays
  if (underlayCbk) {
    (*underlayCbk)(this, underlayCbkData);
  }
  if (overlayCbk) {
    saveState(NULL);
  }

  switch (mode) {

  case psModePS:
    // rotate, translate, and scale page
    imgWidth = imgURX - imgLLX;
    imgHeight = imgURY - imgLLY;
    x1 = (int)floor(state->getX1());
    y1 = (int)floor(state->getY1());
    x2 = (int)ceil(state->getX2());
    y2 = (int)ceil(state->getY2());
    width = x2 - x1;
    height = y2 - y1;
    tx = ty = 0;
    // rotation and portrait/landscape mode
    if (rotate0 >= 0) {
      rotate = (360 - rotate0) % 360;
      landscape = gFalse;
    } else {
      rotate = (360 - state->getRotate()) % 360;
      if (rotate == 0 || rotate == 180) {
    if (width > height && width > imgWidth) {
	  rotate += 90;
	  landscape = gTrue;
	} else {
	  landscape = gFalse;
	}
      } else { // rotate == 90 || rotate == 270
	if (height > width && height > imgWidth) {
	  rotate = 270 - rotate;
	  landscape = gTrue;
	} else {
	  landscape = gFalse;
	}
      }
    }
      writePSFmt("%%%%PageOrientation: %s\n",
	       landscape ? "Landscape" : "Portrait");
      writePS("pdfStartPage\n");
    if (rotate == 0) {
      imgWidth2 = imgWidth;
      imgHeight2 = imgHeight;
    } else if (rotate == 90) {
      writePS("90 rotate\n");
      ty = -imgWidth;
      imgWidth2 = imgHeight;
      imgHeight2 = imgWidth;
    } else if (rotate == 180) {
      writePS("180 rotate\n");
      imgWidth2 = imgWidth;
      imgHeight2 = imgHeight;
      tx = -imgWidth;
      ty = -imgHeight;
    } else { // rotate == 270
      writePS("270 rotate\n");
      tx = -imgHeight;
      imgWidth2 = imgHeight;
      imgHeight2 = imgWidth;
    }
    // shrink or expand
    if (xScale0 > 0 && yScale0 > 0) {
      xScale = xScale0;
      yScale = yScale0;
    } else if ((globalParams->getPSShrinkLarger() &&
	 (width > imgWidth2 || height > imgHeight2)) ||
	(globalParams->getPSExpandSmaller() &&
	 (width < imgWidth2 && height < imgHeight2))) {
      xScale = (double)imgWidth2 / (double)width;
      yScale = (double)imgHeight2 / (double)height;
      if (yScale < xScale) {
	xScale = yScale;
      } else {
	yScale = xScale;
      }
    } else {
      xScale = yScale = 1;
    }
    // deal with odd bounding boxes or clipping
    if (clipLLX0 < clipURX0 && clipLLY0 < clipURY0) {
      tx -= xScale * clipLLX0;
      ty -= yScale * clipLLY0;
    } else {
    tx -= xScale * x1;
    ty -= yScale * y1;
    }
    // center
    if (globalParams->getPSCenter()) {
      if (clipLLX0 < clipURX0 && clipLLY0 < clipURY0) {
	tx += (imgWidth2 - xScale * (clipURX0 - clipLLX0)) / 2;
	ty += (imgHeight2 - yScale * (clipURY0 - clipLLY0)) / 2;
      } else {
      tx += (imgWidth2 - xScale * width) / 2;
      ty += (imgHeight2 - yScale * height) / 2;
    }
    }
    tx += rotate == 0 ? imgLLX + tx0 : imgLLY + ty0;
    ty += rotate == 0 ? imgLLY + ty0 : -(imgLLX + tx0);
    if (tx != 0 || ty != 0) {
      writePSFmt("%g %g translate\n", tx, ty);
    }
    if (xScale != 1 || yScale != 1) {
      writePSFmt("%0.4f %0.4f scale\n", xScale, yScale);
    }
    if (clipLLX0 < clipURX0 && clipLLY0 < clipURY0) {
      writePSFmt("%g %g %g %g re W\n",
		 clipLLX0, clipLLY0, clipURX0 - clipLLX0, clipURY0 - clipLLY0);
    } else {
      writePSFmt("%d %d %d %d re W\n", x1, y1, x2 - x1, y2 - y1);
    }

    writePS("%%EndPageSetup\n");
    ++seqPage;
    break;

  case psModeEPS:
    writePS("pdfStartPage\n");
    tx = ty = 0;
    rotate = (360 - state->getRotate()) % 360;
    if (rotate == 0) {
    } else if (rotate == 90) {
      writePS("90 rotate\n");
      tx = -epsX1;
      ty = -epsY2;
    } else if (rotate == 180) {
      writePS("180 rotate\n");
      tx = -(epsX1 + epsX2);
      ty = -(epsY1 + epsY2);
    } else { // rotate == 270
      writePS("270 rotate\n");
      tx = -epsX2;
      ty = -epsY1;
    }
    if (tx != 0 || ty != 0) {
      writePSFmt("%g %g translate\n", tx, ty);
    }
    xScale = yScale = 1;
    break;

  case psModeForm:
    writePS("/PaintProc {\n");
    writePS("begin xpdf begin\n");
    writePS("pdfStartPage\n");
    tx = ty = 0;
    xScale = yScale = 1;
    rotate = 0;
    break;
  }
}

void PSOutputDev::endPage() {
  if (overlayCbk) {
    restoreState(NULL);
    (*overlayCbk)(this, overlayCbkData);
  }


  if (mode == psModeForm) {
    writePS("pdfEndPage\n");
    writePS("end end\n");
    writePS("} def\n");
    writePS("end end\n");
  } else {
    if (!manualCtrl) {
      writePS("showpage\n");
    }
      writePS("%%PageTrailer\n");
      writePageTrailer();
    }
}

void PSOutputDev::saveState(GfxState *state) {
  writePS("q\n");
  ++numSaves;
}

void PSOutputDev::restoreState(GfxState *state) {
  writePS("Q\n");
  --numSaves;
}

void PSOutputDev::updateCTM(GfxState *state, double m11, double m12,
			    double m21, double m22, double m31, double m32) {
  writePSFmt("[%g %g %g %g %g %g] cm\n", m11, m12, m21, m22, m31, m32);
}

void PSOutputDev::updateLineDash(GfxState *state) {
  double *dash;
  double start;
  int length, i;

  state->getLineDash(&dash, &length, &start);
  writePS("[");
  for (i = 0; i < length; ++i) {
    writePSFmt("%g%s",
	       dash[i] == 0 ? 1 : dash[i],
	       (i == length-1) ? "" : " ");
  }
  writePSFmt("] %g d\n", start);
}

void PSOutputDev::updateFlatness(GfxState *state) {
  writePSFmt("%d i\n", state->getFlatness());
}

void PSOutputDev::updateLineJoin(GfxState *state) {
  writePSFmt("%d j\n", state->getLineJoin());
}

void PSOutputDev::updateLineCap(GfxState *state) {
  writePSFmt("%d J\n", state->getLineCap());
}

void PSOutputDev::updateMiterLimit(GfxState *state) {
  writePSFmt("%g M\n", state->getMiterLimit());
}

void PSOutputDev::updateLineWidth(GfxState *state) {
  writePSFmt("%g w\n", state->getLineWidth());
}

void PSOutputDev::updateFillColorSpace(GfxState *state) {
  switch (level) {
  case psLevel1:
  case psLevel1Sep:
    break;
  case psLevel2:
  case psLevel3:
    if (state->getFillColorSpace()->getMode() != csPattern) {
      dumpColorSpaceL2(state->getFillColorSpace(), gTrue, gFalse);
      writePS(" cs\n");
    }
    break;
  case psLevel2Sep:
  case psLevel3Sep:
    break;
  }
}

void PSOutputDev::updateStrokeColorSpace(GfxState *state) {
  switch (level) {
  case psLevel1:
  case psLevel1Sep:
    break;
  case psLevel2:
  case psLevel3:
    if (state->getStrokeColorSpace()->getMode() != csPattern) {
      dumpColorSpaceL2(state->getStrokeColorSpace(), gTrue, gFalse);
      writePS(" CS\n");
    }
    break;
  case psLevel2Sep:
  case psLevel3Sep:
    break;
  }
}

void PSOutputDev::updateFillColor(GfxState *state) {
  GfxColor color;
  GfxColor *colorPtr;
  GfxGray gray;
  GfxCMYK cmyk;
  GfxSeparationColorSpace *sepCS;
  double c, m, y, k;
  int i;

  switch (level) {
  case psLevel1:
    state->getFillGray(&gray);
    writePSFmt("%g g\n", colToDbl(gray));
    break;
  case psLevel1Sep:
    state->getFillCMYK(&cmyk);
    c = colToDbl(cmyk.c);
    m = colToDbl(cmyk.m);
    y = colToDbl(cmyk.y);
    k = colToDbl(cmyk.k);
    writePSFmt("%g %g %g %g k\n", c, m, y, k);
    addProcessColor(c, m, y, k);
    break;
  case psLevel2:
  case psLevel3:
    if (state->getFillColorSpace()->getMode() != csPattern) {
      colorPtr = state->getFillColor();
      writePS("[");
      for (i = 0; i < state->getFillColorSpace()->getNComps(); ++i) {
	if (i > 0) {
	  writePS(" ");
      }
	writePSFmt("%g", colToDbl(colorPtr->c[i]));
      }
      writePS("] sc\n");
    }
    break;
  case psLevel2Sep:
  case psLevel3Sep:
    if (state->getFillColorSpace()->getMode() == csSeparation) {
      sepCS = (GfxSeparationColorSpace *)state->getFillColorSpace();
      color.c[0] = gfxColorComp1;
      sepCS->getCMYK(&color, &cmyk);
      writePSFmt("%g %g %g %g %g (%s) ck\n",
		 colToDbl(state->getFillColor()->c[0]),
		 colToDbl(cmyk.c), colToDbl(cmyk.m),
		 colToDbl(cmyk.y), colToDbl(cmyk.k),
		 sepCS->getName()->getCString());
      addCustomColor(sepCS);
    } else {
      state->getFillCMYK(&cmyk);
      c = colToDbl(cmyk.c);
      m = colToDbl(cmyk.m);
      y = colToDbl(cmyk.y);
      k = colToDbl(cmyk.k);
      writePSFmt("%g %g %g %g k\n", c, m, y, k);
      addProcessColor(c, m, y, k);
    }
    break;
  }
  t3Cacheable = gFalse;
}

void PSOutputDev::updateStrokeColor(GfxState *state) {
  GfxColor color;
  GfxColor *colorPtr;
  GfxGray gray;
  GfxCMYK cmyk;
  GfxSeparationColorSpace *sepCS;
  double c, m, y, k;
  int i;

  switch (level) {
  case psLevel1:
    state->getStrokeGray(&gray);
    writePSFmt("%g G\n", colToDbl(gray));
    break;
  case psLevel1Sep:
    state->getStrokeCMYK(&cmyk);
    c = colToDbl(cmyk.c);
    m = colToDbl(cmyk.m);
    y = colToDbl(cmyk.y);
    k = colToDbl(cmyk.k);
    writePSFmt("%g %g %g %g K\n", c, m, y, k);
    addProcessColor(c, m, y, k);
    break;
  case psLevel2:
  case psLevel3:
    if (state->getStrokeColorSpace()->getMode() != csPattern) {
      colorPtr = state->getStrokeColor();
      writePS("[");
      for (i = 0; i < state->getStrokeColorSpace()->getNComps(); ++i) {
	if (i > 0) {
	  writePS(" ");
      }
	writePSFmt("%g", colToDbl(colorPtr->c[i]));
      }
      writePS("] SC\n");
    }
    break;
  case psLevel2Sep:
  case psLevel3Sep:
    if (state->getStrokeColorSpace()->getMode() == csSeparation) {
      sepCS = (GfxSeparationColorSpace *)state->getStrokeColorSpace();
      color.c[0] = gfxColorComp1;
      sepCS->getCMYK(&color, &cmyk);
      writePSFmt("%g %g %g %g %g (%s) CK\n",
		 colToDbl(state->getStrokeColor()->c[0]),
		 colToDbl(cmyk.c), colToDbl(cmyk.m),
		 colToDbl(cmyk.y), colToDbl(cmyk.k),
		 sepCS->getName()->getCString());
      addCustomColor(sepCS);
    } else {
      state->getStrokeCMYK(&cmyk);
      c = colToDbl(cmyk.c);
      m = colToDbl(cmyk.m);
      y = colToDbl(cmyk.y);
      k = colToDbl(cmyk.k);
      writePSFmt("%g %g %g %g K\n", c, m, y, k);
      addProcessColor(c, m, y, k);
    }
    break;
  }
  t3Cacheable = gFalse;
}

void PSOutputDev::addProcessColor(double c, double m, double y, double k) {
  if (c > 0) {
    processColors |= psProcessCyan;
  }
  if (m > 0) {
    processColors |= psProcessMagenta;
  }
  if (y > 0) {
    processColors |= psProcessYellow;
  }
  if (k > 0) {
    processColors |= psProcessBlack;
  }
}

void PSOutputDev::addCustomColor(GfxSeparationColorSpace *sepCS) {
  PSOutCustomColor *cc;
  GfxColor color;
  GfxCMYK cmyk;

  for (cc = customColors; cc; cc = cc->next) {
    if (!cc->name->cmp(sepCS->getName())) {
      return;
    }
  }
  color.c[0] = gfxColorComp1;
  sepCS->getCMYK(&color, &cmyk);
  cc = new PSOutCustomColor(colToDbl(cmyk.c), colToDbl(cmyk.m),
			    colToDbl(cmyk.y), colToDbl(cmyk.k),
			    sepCS->getName()->copy());
  cc->next = customColors;
  customColors = cc;
}

void PSOutputDev::updateFillOverprint(GfxState *state) {
  if (level >= psLevel2) {
    writePSFmt("%s op\n", state->getFillOverprint() ? "true" : "false");
  }
}

void PSOutputDev::updateStrokeOverprint(GfxState *state) {
  if (level >= psLevel2) {
    writePSFmt("%s OP\n", state->getStrokeOverprint() ? "true" : "false");
  }
}

void PSOutputDev::updateFont(GfxState *state) {
  if (state->getFont()) {
    writePSFmt("/F%d_%d %g Tf\n",
	       state->getFont()->getID()->num, state->getFont()->getID()->gen,
	       fabs(state->getFontSize()) < 0.00001 ? 0.00001
	                                            : state->getFontSize());
  }
}

void PSOutputDev::updateTextMat(GfxState *state) {
  double *mat;

  mat = state->getTextMat();
  if (fabs(mat[0] * mat[3] - mat[1] * mat[2]) < 0.00001) {
    // avoid a singular (or close-to-singular) matrix
    writePSFmt("[0.00001 0 0 0.00001 %g %g] Tm\n", mat[4], mat[5]);
  } else {
  writePSFmt("[%g %g %g %g %g %g] Tm\n",
	     mat[0], mat[1], mat[2], mat[3], mat[4], mat[5]);
  }
}

void PSOutputDev::updateCharSpace(GfxState *state) {
  writePSFmt("%g Tc\n", state->getCharSpace());
}

void PSOutputDev::updateRender(GfxState *state) {
  int rm;

  rm = state->getRender();
  writePSFmt("%d Tr\n", rm);
  rm &= 3;
  if (rm != 0 && rm != 3) {
    t3Cacheable = gFalse;
  }
}

void PSOutputDev::updateRise(GfxState *state) {
  writePSFmt("%g Ts\n", state->getRise());
}

void PSOutputDev::updateWordSpace(GfxState *state) {
  writePSFmt("%g Tw\n", state->getWordSpace());
}

void PSOutputDev::updateHorizScaling(GfxState *state) {
  double h;

  h = state->getHorizScaling();
  if (fabs(h) < 0.01) {
    h = 0.01;
  }
  writePSFmt("%g Tz\n", h);
}

void PSOutputDev::updateTextPos(GfxState *state) {
  writePSFmt("%g %g Td\n", state->getLineX(), state->getLineY());
}

void PSOutputDev::updateTextShift(GfxState *state, double shift) {
  if (state->getFont()->getWMode()) {
    writePSFmt("%g TJmV\n", shift);
  } else {
    writePSFmt("%g TJm\n", shift);
  }
}

void PSOutputDev::stroke(GfxState *state) {
  doPath(state->getPath());
  if (t3String) {
    // if we're construct a cacheable Type 3 glyph, we need to do
    // everything in the fill color
    writePS("Sf\n");
  } else {
    writePS("S\n");
  }
}

void PSOutputDev::fill(GfxState *state) {
  doPath(state->getPath());
  writePS("f\n");
}

void PSOutputDev::eoFill(GfxState *state) {
  doPath(state->getPath());
  writePS("f*\n");
}

void PSOutputDev::tilingPatternFill(GfxState *state, Object *str,
				    int paintType, Dict *resDict,
				    double *mat, double *bbox,
				    int x0, int y0, int x1, int y1,
				    double xStep, double yStep) {
  PDFRectangle box;
  Gfx *gfx;

  // define a Type 3 font
  writePS("8 dict begin\n");
  writePS("/FontType 3 def\n");
  writePS("/FontMatrix [1 0 0 1 0 0] def\n");
  writePSFmt("/FontBBox [%g %g %g %g] def\n",
	     bbox[0], bbox[1], bbox[2], bbox[3]);
  writePS("/Encoding 256 array def\n");
  writePS("  0 1 255 { Encoding exch /.notdef put } for\n");
  writePS("  Encoding 120 /x put\n");
  writePS("/BuildGlyph {\n");
  writePS("  exch /CharProcs get exch\n");
  writePS("  2 copy known not { pop /.notdef } if\n");
  writePS("  get exec\n");
  writePS("} bind def\n");
  writePS("/BuildChar {\n");
  writePS("  1 index /Encoding get exch get\n");
  writePS("  1 index /BuildGlyph get exec\n");
  writePS("} bind def\n");
  writePS("/CharProcs 1 dict def\n");
  writePS("CharProcs begin\n");
  box.x1 = bbox[0];
  box.y1 = bbox[1];
  box.x2 = bbox[2];
  box.y2 = bbox[3];
  gfx = new Gfx(xref, this, resDict, &box, NULL);
  writePS("/x {\n");
  if (paintType == 2) {
    writePSFmt("%g 0 %g %g %g %g setcachedevice\n",
	       xStep, bbox[0], bbox[1], bbox[2], bbox[3]);
  } else {
    writePSFmt("%g 0 setcharwidth\n", xStep);
  }
  inType3Char = gTrue;
  ++numTilingPatterns;
  gfx->display(str);
  --numTilingPatterns;
  inType3Char = gFalse;
  writePS("} def\n");
  delete gfx;
  writePS("end\n");
  writePS("currentdict end\n");
  writePSFmt("/xpdfTile%d exch definefont pop\n", numTilingPatterns);

  // draw the tiles
  writePSFmt("/xpdfTile%d findfont setfont\n", numTilingPatterns);
  writePSFmt("gsave [%g %g %g %g %g %g] concat\n",
	     mat[0], mat[1], mat[2], mat[3], mat[4], mat[5]);
  writePSFmt("%d 1 %d { %g exch %g mul m %d 1 %d { pop (x) show } for } for\n",
	     y0, y1 - 1, x0 * xStep, yStep, x0, x1 - 1);
  writePS("grestore\n");
}

void PSOutputDev::functionShadedFill(GfxState *state,
				     GfxFunctionShading *shading) {
  double x0, y0, x1, y1;
  double *mat;
  int i;

  shading->getDomain(&x0, &y0, &x1, &y1);
  mat = shading->getMatrix();
  writePSFmt("/mat [%g %g %g %g %g %g] def\n",
	     mat[0], mat[1], mat[2], mat[3], mat[4], mat[5]);
  writePSFmt("/n %d def\n", shading->getColorSpace()->getNComps());
  if (shading->getNFuncs() == 1) {
    writePS("/func ");
    cvtFunction(shading->getFunc(0));
    writePS("def\n");
  } else {
    writePS("/func {\n");
    for (i = 0; i < shading->getNFuncs(); ++i) {
      if (i < shading->getNFuncs() - 1) {
	writePS("2 copy\n");
      }
      cvtFunction(shading->getFunc(i));
      writePS("exec\n");
      if (i < shading->getNFuncs() - 1) {
	writePS("3 1 roll\n");
      }
    }
    writePS("} def\n");
  }
  writePSFmt("%g %g %g %g 0 funcSH\n", x0, y0, x1, y1);
}

void PSOutputDev::axialShadedFill(GfxState *state, GfxAxialShading *shading) {
  double xMin, yMin, xMax, yMax;
  double x0, y0, x1, y1, dx, dy, mul;
  double tMin, tMax, t, t0, t1;
  int i;

  // get the clip region bbox
  state->getUserClipBBox(&xMin, &yMin, &xMax, &yMax);

  // compute min and max t values, based on the four corners of the
  // clip region bbox
  shading->getCoords(&x0, &y0, &x1, &y1);
  dx = x1 - x0;
  dy = y1 - y0;
  mul = 1 / (dx * dx + dy * dy);
  tMin = tMax = ((xMin - x0) * dx + (yMin - y0) * dy) * mul;
  t = ((xMin - x0) * dx + (yMax - y0) * dy) * mul;
  if (t < tMin) {
    tMin = t;
  } else if (t > tMax) {
    tMax = t;
  }
  t = ((xMax - x0) * dx + (yMin - y0) * dy) * mul;
  if (t < tMin) {
    tMin = t;
  } else if (t > tMax) {
    tMax = t;
  }
  t = ((xMax - x0) * dx + (yMax - y0) * dy) * mul;
  if (t < tMin) {
    tMin = t;
  } else if (t > tMax) {
    tMax = t;
  }
  if (tMin < 0 && !shading->getExtend0()) {
    tMin = 0;
  }
  if (tMax > 1 && !shading->getExtend1()) {
    tMax = 1;
  }

  // get the function domain
  t0 = shading->getDomain0();
  t1 = shading->getDomain1();

  // generate the PS code
  writePSFmt("/t0 %g def\n", t0);
  writePSFmt("/t1 %g def\n", t1);
  writePSFmt("/dt %g def\n", t1 - t0);
  writePSFmt("/x0 %g def\n", x0);
  writePSFmt("/y0 %g def\n", y0);
  writePSFmt("/dx %g def\n", x1 - x0);
  writePSFmt("/x1 %g def\n", x1);
  writePSFmt("/y1 %g def\n", y1);
  writePSFmt("/dy %g def\n", y1 - y0);
  writePSFmt("/xMin %g def\n", xMin);
  writePSFmt("/yMin %g def\n", yMin);
  writePSFmt("/xMax %g def\n", xMax);
  writePSFmt("/yMax %g def\n", yMax);
  writePSFmt("/n %d def\n", shading->getColorSpace()->getNComps());
  if (shading->getNFuncs() == 1) {
    writePS("/func ");
    cvtFunction(shading->getFunc(0));
    writePS("def\n");
  } else {
    writePS("/func {\n");
    for (i = 0; i < shading->getNFuncs(); ++i) {
      if (i < shading->getNFuncs() - 1) {
	writePS("dup\n");
      }
      cvtFunction(shading->getFunc(i));
      writePS("exec\n");
      if (i < shading->getNFuncs() - 1) {
	writePS("exch\n");
      }
    }
    writePS("} def\n");
  }
  writePSFmt("%g %g 0 axialSH\n", tMin, tMax);
}

void PSOutputDev::radialShadedFill(GfxState *state,
				   GfxRadialShading *shading) {
  double x0, y0, r0, x1, y1, r1, t0, t1, sMin, sMax;
  double xMin, yMin, xMax, yMax;
  double d0, d1;
  int i;

  // get the shading info
  shading->getCoords(&x0, &y0, &r0, &x1, &y1, &r1);
  t0 = shading->getDomain0();
  t1 = shading->getDomain1();

  // compute the (possibly extended) s range
  sMin = 0;
  sMax = 1;
  if (shading->getExtend0()) {
    if (r0 < r1) {
      // extend the smaller end
      sMin = -r0 / (r1 - r0);
    } else {
      // extend the larger end
      state->getUserClipBBox(&xMin, &yMin, &xMax, &yMax);
      d0 = (x0 - xMin) * (x0 - xMin);
      d1 = (x0 - xMax) * (x0 - xMax);
      sMin = d0 > d1 ? d0 : d1;
      d0 = (y0 - yMin) * (y0 - yMin);
      d1 = (y0 - yMax) * (y0 - yMax);
      sMin += d0 > d1 ? d0 : d1;
      sMin = (sqrt(sMin) - r0) / (r1 - r0);
      if (sMin > 0) {
	sMin = 0;
      } else if (sMin < -20) {
	// sanity check
	sMin = -20;
      }
    }
  }
  if (shading->getExtend1()) {
    if (r1 < r0) {
      // extend the smaller end
      sMax = -r0 / (r1 - r0);
    } else if (r1 > r0) {
      // extend the larger end
      state->getUserClipBBox(&xMin, &yMin, &xMax, &yMax);
      d0 = (x1 - xMin) * (x1 - xMin);
      d1 = (x1 - xMax) * (x1 - xMax);
      sMax = d0 > d1 ? d0 : d1;
      d0 = (y1 - yMin) * (y1 - yMin);
      d1 = (y1 - yMax) * (y1 - yMax);
      sMax += d0 > d1 ? d0 : d1;
      sMax = (sqrt(sMax) - r0) / (r1 - r0);
      if (sMax < 1) {
	sMax = 1;
      } else if (sMax > 20) {
	// sanity check
	sMax = 20;
      }
    }
  }

  // generate the PS code
  writePSFmt("/x0 %g def\n", x0);
  writePSFmt("/x1 %g def\n", x1);
  writePSFmt("/dx %g def\n", x1 - x0);
  writePSFmt("/y0 %g def\n", y0);
  writePSFmt("/y1 %g def\n", y1);
  writePSFmt("/dy %g def\n", y1 - y0);
  writePSFmt("/r0 %g def\n", r0);
  writePSFmt("/r1 %g def\n", r1);
  writePSFmt("/dr %g def\n", r1 - r0);
  writePSFmt("/t0 %g def\n", t0);
  writePSFmt("/t1 %g def\n", t1);
  writePSFmt("/dt %g def\n", t1 - t0);
  writePSFmt("/n %d def\n", shading->getColorSpace()->getNComps());
  if (shading->getNFuncs() == 1) {
    writePS("/func ");
    cvtFunction(shading->getFunc(0));
    writePS("def\n");
  } else {
    writePS("/func {\n");
    for (i = 0; i < shading->getNFuncs(); ++i) {
      if (i < shading->getNFuncs() - 1) {
	writePS("dup\n");
      }
      cvtFunction(shading->getFunc(i));
      writePS("exec\n");
      if (i < shading->getNFuncs() - 1) {
	writePS("exch\n");
      }
    }
    writePS("} def\n");
  }
  writePSFmt("%g %g 0 radialSH\n", sMin, sMax);
}

void PSOutputDev::clip(GfxState *state) {
  doPath(state->getPath());
  writePS("W\n");
}

void PSOutputDev::eoClip(GfxState *state) {
  doPath(state->getPath());
  writePS("W*\n");
}

void PSOutputDev::doPath(GfxPath *path) {
  GfxSubpath *subpath;
  double x0, y0, x1, y1, x2, y2, x3, y3, x4, y4;
  int n, m, i, j;

  n = path->getNumSubpaths();

  if (n == 1 && path->getSubpath(0)->getNumPoints() == 5) {
    subpath = path->getSubpath(0);
    x0 = subpath->getX(0);
    y0 = subpath->getY(0);
    x4 = subpath->getX(4);
    y4 = subpath->getY(4);
    if (x4 == x0 && y4 == y0) {
      x1 = subpath->getX(1);
      y1 = subpath->getY(1);
      x2 = subpath->getX(2);
      y2 = subpath->getY(2);
      x3 = subpath->getX(3);
      y3 = subpath->getY(3);
      if (x0 == x1 && x2 == x3 && y0 == y3 && y1 == y2) {
	writePSFmt("%g %g %g %g re\n",
		   x0 < x2 ? x0 : x2, y0 < y1 ? y0 : y1,
		   fabs(x2 - x0), fabs(y1 - y0));
	return;
      } else if (x0 == x3 && x1 == x2 && y0 == y1 && y2 == y3) {
	writePSFmt("%g %g %g %g re\n",
		   x0 < x1 ? x0 : x1, y0 < y2 ? y0 : y2,
		   fabs(x1 - x0), fabs(y2 - y0));
	return;
      }
    }
  }

  for (i = 0; i < n; ++i) {
    subpath = path->getSubpath(i);
    m = subpath->getNumPoints();
    writePSFmt("%g %g m\n", subpath->getX(0), subpath->getY(0));
    j = 1;
    while (j < m) {
      if (subpath->getCurve(j)) {
	writePSFmt("%g %g %g %g %g %g c\n", subpath->getX(j), subpath->getY(j),
		   subpath->getX(j+1), subpath->getY(j+1),
		   subpath->getX(j+2), subpath->getY(j+2));
	j += 3;
      } else {
	writePSFmt("%g %g l\n", subpath->getX(j), subpath->getY(j));
	++j;
      }
    }
    if (subpath->isClosed()) {
      writePS("h\n");
    }
  }
}

void PSOutputDev::drawString(GfxState *state, GooString *s) {
  GfxFont *font;
  int wMode;
  GooString *s2;
  double dx, dy, dx2, dy2, originX, originY;
  char *p;
  UnicodeMap *uMap;
  CharCode code;
  Unicode u[8];
  char buf[8];
  int len, nChars, uLen, n, m, i, j;

  // check for invisible text -- this is used by Acrobat Capture
  if (state->getRender() == 3) {
    return;
  }

  // ignore empty strings
  if (s->getLength() == 0) {
    return;
  }

  // get the font
  if (!(font = state->getFont())) {
    return;
  }
  wMode = font->getWMode();

  // check for a subtitute 16-bit font
  uMap = NULL;
  if (font->isCIDFont()) {
    for (i = 0; i < font16EncLen; ++i) {
      if (font->getID()->num == font16Enc[i].fontID.num &&
	  font->getID()->gen == font16Enc[i].fontID.gen) {
	uMap = globalParams->getUnicodeMap(font16Enc[i].enc);
	break;
      }
    }
  }

  // compute width of chars in string, ignoring char spacing and word
  // spacing -- the Tj operator will adjust for the metrics of the
  // font that's actually used
  dx = dy = 0;
  nChars = 0;
  p = s->getCString();
  len = s->getLength();
  if (font->isCIDFont()) {
    s2 = new GooString();
  } else {
    s2 = s;
  }
  while (len > 0) {
    n = font->getNextChar(p, len, &code,
			  u, (int)(sizeof(u) / sizeof(Unicode)), &uLen,
			  &dx2, &dy2, &originX, &originY);
    if (font->isCIDFont()) {
      if (uMap) {
	for (i = 0; i < uLen; ++i) {
	  m = uMap->mapUnicode(u[i], buf, (int)sizeof(buf));
	  for (j = 0; j < m; ++j) {
	    s2->append(buf[j]);
	  }
	}
	//~ this really needs to get the number of chars in the target
	//~ encoding - which may be more than the number of Unicode
	//~ chars
	nChars += uLen;
      } else {
	s2->append((char)((code >> 8) & 0xff));
	s2->append((char)(code & 0xff));
	++nChars;
      }
    }
    dx += dx2;
    dy += dy2;
    p += n;
    len -= n;
  }
  dx *= state->getFontSize() * state->getHorizScaling();
  dy *= state->getFontSize();
  if (uMap) {
    uMap->decRefCnt();
  }

  if (s2->getLength() > 0) {
    writePSString(s2);
    if (font->isCIDFont()) {
      if (wMode) {
	writePSFmt(" %d %g Tj16V\n", nChars, dy);
      } else {
	writePSFmt(" %d %g Tj16\n", nChars, dx);
      }
    } else {
      writePSFmt(" %g Tj\n", dx);
    }
  }
  if (font->isCIDFont()) {
    delete s2;
  }

  if (state->getRender() & 4) {
    haveTextClip = gTrue;
  }
}

void PSOutputDev::endTextObject(GfxState *state) {
  if (haveTextClip) {
    writePS("Tclip\n");
    haveTextClip = gFalse;
  }
}

void PSOutputDev::drawImageMask(GfxState *state, Object *ref, Stream *str,
				int width, int height, GBool invert,
				GBool inlineImg) {
  int len;

  len = height * ((width + 7) / 8);
  if (level == psLevel1 || level == psLevel1Sep) {
    doImageL1(ref, NULL, invert, inlineImg, str, width, height, len);
  } else {
    doImageL2(ref, NULL, invert, inlineImg, str, width, height, len,
	      NULL, NULL, 0, 0, gFalse);
  }
}

void PSOutputDev::drawImage(GfxState *state, Object *ref, Stream *str,
			    int width, int height, GfxImageColorMap *colorMap,
			    int *maskColors, GBool inlineImg) {
  int len;

  len = height * ((width * colorMap->getNumPixelComps() *
		   colorMap->getBits() + 7) / 8);
  switch (level) {
  case psLevel1:
    doImageL1(ref, colorMap, gFalse, inlineImg, str, width, height, len);
    break;
  case psLevel1Sep:
    //~ handle indexed, separation, ... color spaces
    doImageL1Sep(colorMap, gFalse, inlineImg, str, width, height, len);
    break;
  case psLevel2:
  case psLevel2Sep:
  case psLevel3:
  case psLevel3Sep:
    doImageL2(ref, colorMap, gFalse, inlineImg, str,
	      width, height, len, maskColors, NULL, 0, 0, gFalse);
    break;
  }
  t3Cacheable = gFalse;
}

void PSOutputDev::drawMaskedImage(GfxState *state, Object *ref, Stream *str,
				  int width, int height,
				  GfxImageColorMap *colorMap,
				  Stream *maskStr,
				  int maskWidth, int maskHeight,
				  GBool maskInvert) {
  int len;

  len = height * ((width * colorMap->getNumPixelComps() *
		   colorMap->getBits() + 7) / 8);
  switch (level) {
  case psLevel1:
    doImageL1(ref, colorMap, gFalse, gFalse, str, width, height, len);
    break;
  case psLevel1Sep:
    //~ handle indexed, separation, ... color spaces
    doImageL1Sep(colorMap, gFalse, gFalse, str, width, height, len);
    break;
  case psLevel2:
  case psLevel2Sep:
  case psLevel3:
  case psLevel3Sep:
    doImageL2(ref, colorMap, gFalse, gFalse, str, width, height, len,
	      NULL, maskStr, maskWidth, maskHeight, maskInvert);
    break;
  }
  t3Cacheable = gFalse;
}

void PSOutputDev::doImageL1(Object *ref, GfxImageColorMap *colorMap,
			    GBool invert, GBool inlineImg,
			    Stream *str, int width, int height, int len) {
  ImageStream *imgStr;
  Guchar pixBuf[gfxColorMaxComps];
  GfxGray gray;
  int col, x, y, c, i;

  if (inType3Char && !colorMap) {
    if (inlineImg) {
      // create an array
      str = new FixedLengthEncoder(str, len);
      str = new ASCIIHexEncoder(str);
      str->reset();
      col = 0;
      writePS("[<");
      do {
	do {
	  c = str->getChar();
	} while (c == '\n' || c == '\r');
	if (c == '>' || c == EOF) {
	  break;
	}
	writePSChar(c);
	++col;
	// each line is: "<...data...><eol>"
	// so max data length = 255 - 4 = 251
	// but make it 240 just to be safe
	// chunks are 2 bytes each, so we need to stop on an even col number
	if (col == 240) {
	  writePS(">\n<");
	  col = 0;
	}
      } while (c != '>' && c != EOF);
      writePS(">]\n");
      writePS("0\n");
      str->close();
      delete str;
    } else {
      // set up to use the array already created by setupImages()
      writePSFmt("ImData_%d_%d 0\n", ref->getRefNum(), ref->getRefGen());
    }
  }

  // image/imagemask command
  if (inType3Char && !colorMap) {
    writePSFmt("%d %d %s [%d 0 0 %d 0 %d] pdfImM1a\n",
	       width, height, invert ? "true" : "false",
	       width, -height, height);
  } else if (colorMap) {
    writePSFmt("%d %d 8 [%d 0 0 %d 0 %d] pdfIm1\n",
	       width, height,
	       width, -height, height);
  } else {
    writePSFmt("%d %d %s [%d 0 0 %d 0 %d] pdfImM1\n",
	       width, height, invert ? "true" : "false",
	       width, -height, height);
  }

  // image data
  if (!(inType3Char && !colorMap)) {

    if (colorMap) {

      // set up to process the data stream
      imgStr = new ImageStream(str, width, colorMap->getNumPixelComps(),
			       colorMap->getBits());
      imgStr->reset();

      // process the data stream
      i = 0;
      for (y = 0; y < height; ++y) {

	// write the line
	for (x = 0; x < width; ++x) {
	  imgStr->getPixel(pixBuf);
	  colorMap->getGray(pixBuf, &gray);
	  writePSFmt("%02x", colToByte(gray));
	  if (++i == 32) {
	    writePSChar('\n');
	    i = 0;
	  }
	}
      }
      if (i != 0) {
	writePSChar('\n');
      }
      delete imgStr;

    // imagemask
    } else {
      str->reset();
      i = 0;
      for (y = 0; y < height; ++y) {
	for (x = 0; x < width; x += 8) {
	  writePSFmt("%02x", str->getChar() & 0xff);
	  if (++i == 32) {
	    writePSChar('\n');
	    i = 0;
	  }
	}
      }
      if (i != 0) {
	writePSChar('\n');
      }
      str->close();
    }
  }
}

void PSOutputDev::doImageL1Sep(GfxImageColorMap *colorMap,
			       GBool invert, GBool inlineImg,
			       Stream *str, int width, int height, int len) {
  ImageStream *imgStr;
  Guchar *lineBuf;
  Guchar pixBuf[gfxColorMaxComps];
  GfxCMYK cmyk;
  int x, y, i, comp;

  // width, height, matrix, bits per component
  writePSFmt("%d %d 8 [%d 0 0 %d 0 %d] pdfIm1Sep\n",
	     width, height,
	     width, -height, height);

  // allocate a line buffer
  lineBuf = (Guchar *)gmalloc(4 * width);

  // set up to process the data stream
  imgStr = new ImageStream(str, width, colorMap->getNumPixelComps(),
			   colorMap->getBits());
  imgStr->reset();

  // process the data stream
  i = 0;
  for (y = 0; y < height; ++y) {

    // read the line
    for (x = 0; x < width; ++x) {
      imgStr->getPixel(pixBuf);
      colorMap->getCMYK(pixBuf, &cmyk);
      lineBuf[4*x+0] = colToByte(cmyk.c);
      lineBuf[4*x+1] = colToByte(cmyk.m);
      lineBuf[4*x+2] = colToByte(cmyk.y);
      lineBuf[4*x+3] = colToByte(cmyk.k);
      addProcessColor(colToDbl(cmyk.c), colToDbl(cmyk.m),
		      colToDbl(cmyk.y), colToDbl(cmyk.k));
    }

    // write one line of each color component
    for (comp = 0; comp < 4; ++comp) {
      for (x = 0; x < width; ++x) {
	writePSFmt("%02x", lineBuf[4*x + comp]);
	if (++i == 32) {
	  writePSChar('\n');
	  i = 0;
	}
      }
    }
  }

  if (i != 0) {
    writePSChar('\n');
  }

  delete imgStr;
  gfree(lineBuf);
}

void PSOutputDev::doImageL2(Object *ref, GfxImageColorMap *colorMap,
			    GBool invert, GBool inlineImg,
			    Stream *str, int width, int height, int len,
			    int *maskColors, Stream *maskStr,
			    int maskWidth, int maskHeight, GBool maskInvert) {
  ImageStream *imgStr;
  Guchar *line, *pix;
  GooString *s;
  int n, numComps;
  GBool useRLE, useASCII, useASCIIHex, useCompressed;
  GfxSeparationColorSpace *sepCS;
  GfxColor color;
  GfxCMYK cmyk;
  int c;
  int col, i, x, x0, y, y0, maskXor;

  // color key masking
  if (maskColors && colorMap && !inlineImg) {
    // can't read the stream twice for inline images -- but masking
    // isn't allowed with inline images anyway
    writePS("[\n");
    numComps = colorMap->getNumPixelComps();
    imgStr = new ImageStream(str, width, numComps, colorMap->getBits());
    imgStr->reset();
    for (y = 0, y0 = 0; y < height; ++y) {
      if (!(line = imgStr->getLine())) {
	break;
      }
      for (x = 0, x0 = 0, pix = line; x < width; ++x, pix += numComps) {
	for (i = 0; i < numComps; ++i) {
	  if (pix[i] < maskColors[2*i] ||
	      pix[i] > maskColors[2*i+1]) {
	    break;
	  }
	}
	if (i == numComps) {
	  if (y0 < y) {
	    writePSFmt("0 %d %d %d\n", height - y, width, y - y0);
	  }
	  if (x0 < x) {
	    writePSFmt("%d %d %d 1\n", x0, height - y - 1, x - x0);
	  }
	  x0 = x + 1;
	  y0 = y + 1;
	}
      }
      if (x0 > 0 && x0 < width) {
	writePSFmt("%d %d %d 1\n", x0, height - y - 1, width - x0);
      }
    }
    if (y0 < height) {
      writePSFmt("0 0 %d %d\n", width, height - y0);
    }
    delete imgStr;
    str->close();
    writePSFmt("] %d %d pdfImClip\n", width, height);

  // explicit masking
  } else if (maskStr) {
    writePS("[\n");
    imgStr = new ImageStream(maskStr, maskWidth, 1, 1);
    imgStr->reset();
    maskXor = maskInvert ? 1 : 0;
    for (y = 0, y0 = 0; y < maskHeight; ++y) {
      if (!(line = imgStr->getLine())) {
	break;
      }
      for (x = 0, x0 = 0, pix = line; x < maskWidth; ++x, ++pix) {
	if (*pix ^ maskXor) {
	  if (y0 < y) {
	    writePSFmt("0 %d %d %d\n", maskHeight - y, maskWidth, y - y0);
	  }
	  if (x0 < x) {
	    writePSFmt("%d %d %d 1\n", x0, maskHeight - y - 1, x - x0);
	  }
	  x0 = x + 1;
	  y0 = y + 1;
	}
      }
      if (x0 > 0 && x0 < maskWidth) {
	writePSFmt("%d %d %d 1\n", x0, maskHeight - y - 1, maskWidth - x0);
      }
    }
    if (y0 < maskHeight) {
      writePSFmt("0 0 %d %d\n", maskWidth, maskHeight - y0);
    }
    delete imgStr;
    maskStr->close();
    writePSFmt("] %d %d pdfImClip\n", maskWidth, maskHeight);
  }

  // color space
  if (colorMap) {
    dumpColorSpaceL2(colorMap->getColorSpace(), gFalse, gTrue);
    writePS(" setcolorspace\n");
  }

  useASCIIHex = globalParams->getPSASCIIHex();

  // set up the image data
  if (mode == psModeForm || inType3Char) {
    if (inlineImg) {
      // create an array
      str = new FixedLengthEncoder(str, len);
      if (useASCIIHex) {
	str = new ASCIIHexEncoder(str);
      } else {
	str = new ASCII85Encoder(str);
      }
      str->reset();
      col = 0;
      writePS((char *)(useASCIIHex ? "[<" : "[<~"));
      do {
	do {
	  c = str->getChar();
	} while (c == '\n' || c == '\r');
	if (c == (useASCIIHex ? '>' : '~') || c == EOF) {
	  break;
	}
	if (c == 'z') {
	  writePSChar(c);
	  ++col;
	} else {
	  writePSChar(c);
	  ++col;
	  for (i = 1; i <= (useASCIIHex ? 1 : 4); ++i) {
	    do {
	      c = str->getChar();
	    } while (c == '\n' || c == '\r');
	    if (c == (useASCIIHex ? '>' : '~') || c == EOF) {
	      break;
	    }
	    writePSChar(c);
	    ++col;
	  }
	}
	// each line is: "<~...data...~><eol>"
	// so max data length = 255 - 6 = 249
	// chunks are 1 or 5 bytes each, so we have to stop at 245
	// but make it 240 just to be safe
	if (col > 240) {
	  writePS((char *)(useASCIIHex ? ">\n<" : "~>\n<~"));
	  col = 0;
	}
      } while (c != (useASCIIHex ? '>' : '~') && c != EOF);
      writePS((char *)(useASCIIHex ? ">]\n" : "~>]\n"));
      writePS("0\n");
      str->close();
      delete str;
    } else {
      // set up to use the array already created by setupImages()
      writePSFmt("ImData_%d_%d 0\n", ref->getRefNum(), ref->getRefGen());
    }
  }

  // image dictionary
  writePS("<<\n  /ImageType 1\n");

  // width, height, matrix, bits per component
  writePSFmt("  /Width %d\n", width);
  writePSFmt("  /Height %d\n", height);
  writePSFmt("  /ImageMatrix [%d 0 0 %d 0 %d]\n", width, -height, height);
  if (colorMap && colorMap->getColorSpace()->getMode() == csDeviceN) {
    writePSFmt("  /BitsPerComponent 8\n");
  } else {
    writePSFmt("  /BitsPerComponent %d\n",
	       colorMap ? colorMap->getBits() : 1);
  }

  // decode 
  if (colorMap) {
    writePS("  /Decode [");
    if ((level == psLevel2Sep || level == psLevel3Sep) &&
	colorMap->getColorSpace()->getMode() == csSeparation) {
      // this matches up with the code in the pdfImSep operator
      n = (1 << colorMap->getBits()) - 1;
      writePSFmt("%g %g", colorMap->getDecodeLow(0) * n,
		 colorMap->getDecodeHigh(0) * n);
    } else if (colorMap->getColorSpace()->getMode() == csDeviceN) {
      numComps = ((GfxDeviceNColorSpace *)colorMap->getColorSpace())->
	           getAlt()->getNComps();
      for (i = 0; i < numComps; ++i) {
	if (i > 0) {
	  writePS(" ");
	}
	writePS("0 1");
      }
    } else {
      numComps = colorMap->getNumPixelComps();
      for (i = 0; i < numComps; ++i) {
	if (i > 0) {
	  writePS(" ");
	}
	writePSFmt("%g %g", colorMap->getDecodeLow(i),
		   colorMap->getDecodeHigh(i));
      }
    }
    writePS("]\n");
  } else {
    writePSFmt("  /Decode [%d %d]\n", invert ? 1 : 0, invert ? 0 : 1);
  }

  if (mode == psModeForm || inType3Char) {

    // data source
    writePS("  /DataSource { 2 copy get exch 1 add exch }\n");

    // end of image dictionary
    writePSFmt(">>\n%s\n", colorMap ? "image" : "imagemask");

    // get rid of the array and index
    writePS("pop pop\n");

  } else {

    // data source
    writePS("  /DataSource currentfile\n");
    s = str->getPSFilter(level < psLevel2 ? 1 : level < psLevel3 ? 2 : 3,
			 "    ");
    if ((colorMap && colorMap->getColorSpace()->getMode() == csDeviceN) ||
	inlineImg || !s) {
      useRLE = gTrue;
      useASCII = gTrue;
      useCompressed = gFalse;
    } else {
      useRLE = gFalse;
      useASCII = str->isBinary();
      useCompressed = gTrue;
    }
    if (useASCII) {
      writePSFmt("    /ASCII%sDecode filter\n",
		 useASCIIHex ? "Hex" : "85");
    }
    if (useRLE) {
      writePS("    /RunLengthDecode filter\n");
    }
    if (useCompressed) {
      writePS(s->getCString());
    }
    if (s) {
      delete s;
    }

    // cut off inline image streams at appropriate length
    if (inlineImg) {
      str = new FixedLengthEncoder(str, len);
    } else if (useCompressed) {
      str = str->getBaseStream();
    }

    // recode DeviceN data
    if (colorMap && colorMap->getColorSpace()->getMode() == csDeviceN) {
      str = new DeviceNRecoder(str, width, height, colorMap);
    }

    // add RunLengthEncode and ASCIIHex/85 encode filters
    if (useRLE) {
      str = new RunLengthEncoder(str);
    }
    if (useASCII) {
      if (useASCIIHex) {
	str = new ASCIIHexEncoder(str);
      } else {
	str = new ASCII85Encoder(str);
      }
    }

    // end of image dictionary
    writePS(">>\n");
#if OPI_SUPPORT
    if (opi13Nest) {
      if (inlineImg) {
	// this can't happen -- OPI dictionaries are in XObjects
	error(-1, "Internal: OPI in inline image");
	n = 0;
      } else {
	// need to read the stream to count characters -- the length
	// is data-dependent (because of ASCII and RLE filters)
	str->reset();
	n = 0;
	while ((c = str->getChar()) != EOF) {
	  ++n;
	}
	str->close();
      }
      // +6/7 for "pdfIm\n" / "pdfImM\n"
      // +8 for newline + trailer
      n += colorMap ? 14 : 15;
      writePSFmt("%%%%BeginData: %d Hex Bytes\n", n);
    }
#endif
    if ((level == psLevel2Sep || level == psLevel3Sep) && colorMap &&
	colorMap->getColorSpace()->getMode() == csSeparation) {
      color.c[0] = gfxColorComp1;
      sepCS = (GfxSeparationColorSpace *)colorMap->getColorSpace();
      sepCS->getCMYK(&color, &cmyk);
      writePSFmt("%g %g %g %g (%s) pdfImSep\n",
		 colToDbl(cmyk.c), colToDbl(cmyk.m),
		 colToDbl(cmyk.y), colToDbl(cmyk.k),
		 sepCS->getName()->getCString());
    } else {
      writePSFmt("%s\n", colorMap ? "pdfIm" : "pdfImM");
    }

    // copy the stream data
    str->reset();
    while ((c = str->getChar()) != EOF) {
      writePSChar(c);
    }
    str->close();

    // add newline and trailer to the end
    writePSChar('\n');
    writePS("%-EOD-\n");
#if OPI_SUPPORT
    if (opi13Nest) {
      writePS("%%EndData\n");
    }
#endif

    // delete encoders
    if (useRLE || useASCII || inlineImg) {
      delete str;
    }
  }

  if ((maskColors && colorMap && !inlineImg) || maskStr) {
    writePS("pdfImClipEnd\n");
  }
}

void PSOutputDev::dumpColorSpaceL2(GfxColorSpace *colorSpace,
				   GBool genXform, GBool updateColors) {
  GfxCalGrayColorSpace *calGrayCS;
  GfxCalRGBColorSpace *calRGBCS;
  GfxLabColorSpace *labCS;
  GfxIndexedColorSpace *indexedCS;
  GfxSeparationColorSpace *separationCS;
  GfxDeviceNColorSpace *deviceNCS;
  GfxColorSpace *baseCS;
  Guchar *lookup, *p;
  double x[gfxColorMaxComps], y[gfxColorMaxComps];
  GfxColor color;
  GfxCMYK cmyk;
  Function *func;
  int n, numComps, numAltComps;
  int byte;
  int i, j, k;

  switch (colorSpace->getMode()) {

  case csDeviceGray:
    writePS("/DeviceGray");
    if (genXform) {
      writePS(" {}");
    }
    if (updateColors) {
    processColors |= psProcessBlack;
    }
    break;

  case csCalGray:
    calGrayCS = (GfxCalGrayColorSpace *)colorSpace;
    writePS("[/CIEBasedA <<\n");
    writePSFmt(" /DecodeA {%g exp} bind\n", calGrayCS->getGamma());
    writePSFmt(" /MatrixA [%g %g %g]\n",
	       calGrayCS->getWhiteX(), calGrayCS->getWhiteY(),
	       calGrayCS->getWhiteZ());
    writePSFmt(" /WhitePoint [%g %g %g]\n",
	       calGrayCS->getWhiteX(), calGrayCS->getWhiteY(),
	       calGrayCS->getWhiteZ());
    writePSFmt(" /BlackPoint [%g %g %g]\n",
	       calGrayCS->getBlackX(), calGrayCS->getBlackY(),
	       calGrayCS->getBlackZ());
    writePS(">>]");
    if (genXform) {
      writePS(" {}");
    }
    if (updateColors) {
    processColors |= psProcessBlack;
    }
    break;

  case csDeviceRGB:
    writePS("/DeviceRGB");
    if (genXform) {
      writePS(" {}");
    }
    if (updateColors) {
    processColors |= psProcessCMYK;
    }
    break;

  case csCalRGB:
    calRGBCS = (GfxCalRGBColorSpace *)colorSpace;
    writePS("[/CIEBasedABC <<\n");
    writePSFmt(" /DecodeABC [{%g exp} bind {%g exp} bind {%g exp} bind]\n",
	       calRGBCS->getGammaR(), calRGBCS->getGammaG(),
	       calRGBCS->getGammaB());
    writePSFmt(" /MatrixABC [%g %g %g %g %g %g %g %g %g]\n",
	       calRGBCS->getMatrix()[0], calRGBCS->getMatrix()[1],
	       calRGBCS->getMatrix()[2], calRGBCS->getMatrix()[3],
	       calRGBCS->getMatrix()[4], calRGBCS->getMatrix()[5],
	       calRGBCS->getMatrix()[6], calRGBCS->getMatrix()[7],
	       calRGBCS->getMatrix()[8]);
    writePSFmt(" /WhitePoint [%g %g %g]\n",
	       calRGBCS->getWhiteX(), calRGBCS->getWhiteY(),
	       calRGBCS->getWhiteZ());
    writePSFmt(" /BlackPoint [%g %g %g]\n",
	       calRGBCS->getBlackX(), calRGBCS->getBlackY(),
	       calRGBCS->getBlackZ());
    writePS(">>]");
    if (genXform) {
      writePS(" {}");
    }
    if (updateColors) {
    processColors |= psProcessCMYK;
    }
    break;

  case csDeviceCMYK:
    writePS("/DeviceCMYK");
    if (genXform) {
      writePS(" {}");
    }
    if (updateColors) {
    processColors |= psProcessCMYK;
    }
    break;

  case csLab:
    labCS = (GfxLabColorSpace *)colorSpace;
    writePS("[/CIEBasedABC <<\n");
    writePSFmt(" /RangeABC [0 100 %g %g %g %g]\n",
	       labCS->getAMin(), labCS->getAMax(),
	       labCS->getBMin(), labCS->getBMax());
    writePS(" /DecodeABC [{16 add 116 div} bind {500 div} bind {200 div} bind]\n");
    writePS(" /MatrixABC [1 1 1 1 0 0 0 0 -1]\n");
    writePS(" /DecodeLMN\n");
    writePS("   [{dup 6 29 div ge {dup dup mul mul}\n");
    writePSFmt("     {4 29 div sub 108 841 div mul } ifelse %g mul} bind\n",
	       labCS->getWhiteX());
    writePS("    {dup 6 29 div ge {dup dup mul mul}\n");
    writePSFmt("     {4 29 div sub 108 841 div mul } ifelse %g mul} bind\n",
	       labCS->getWhiteY());
    writePS("    {dup 6 29 div ge {dup dup mul mul}\n");
    writePSFmt("     {4 29 div sub 108 841 div mul } ifelse %g mul} bind]\n",
	       labCS->getWhiteZ());
    writePSFmt(" /WhitePoint [%g %g %g]\n",
	       labCS->getWhiteX(), labCS->getWhiteY(), labCS->getWhiteZ());
    writePSFmt(" /BlackPoint [%g %g %g]\n",
	       labCS->getBlackX(), labCS->getBlackY(), labCS->getBlackZ());
    writePS(">>]");
    if (genXform) {
      writePS(" {}");
    }
    if (updateColors) {
    processColors |= psProcessCMYK;
    }
    break;

  case csICCBased:
    // there is no transform function to the alternate color space, so
    // we can use it directly
    dumpColorSpaceL2(((GfxICCBasedColorSpace *)colorSpace)->getAlt(),
		     genXform, updateColors);
    break;

  case csIndexed:
    indexedCS = (GfxIndexedColorSpace *)colorSpace;
    baseCS = indexedCS->getBase();
    writePS("[/Indexed ");
    dumpColorSpaceL2(baseCS, gFalse, gFalse);
    n = indexedCS->getIndexHigh();
    numComps = baseCS->getNComps();
    lookup = indexedCS->getLookup();
    writePSFmt(" %d <\n", n);
    if (baseCS->getMode() == csDeviceN) {
      func = ((GfxDeviceNColorSpace *)baseCS)->getTintTransformFunc();
      numAltComps = ((GfxDeviceNColorSpace *)baseCS)->getAlt()->getNComps();
      p = lookup;
      for (i = 0; i <= n; i += 8) {
	writePS("  ");
	for (j = i; j < i+8 && j <= n; ++j) {
	  for (k = 0; k < numComps; ++k) {
	    x[k] = *p++ / 255.0;
	  }
	  func->transform(x, y);
	  for (k = 0; k < numAltComps; ++k) {
	    byte = (int)(y[k] * 255 + 0.5);
	    if (byte < 0) {
	      byte = 0;
	    } else if (byte > 255) {
	      byte = 255;
	    }
	    writePSFmt("%02x", byte);
	  }
	  if (updateColors) {
	    color.c[0] = dblToCol(j);
	  indexedCS->getCMYK(&color, &cmyk);
	    addProcessColor(colToDbl(cmyk.c), colToDbl(cmyk.m),
			    colToDbl(cmyk.y), colToDbl(cmyk.k));
	  }
	}
	writePS("\n");
      }
    } else {
      for (i = 0; i <= n; i += 8) {
	writePS("  ");
	for (j = i; j < i+8 && j <= n; ++j) {
	  for (k = 0; k < numComps; ++k) {
	    writePSFmt("%02x", lookup[j * numComps + k]);
	  }
	  if (updateColors) {
	    color.c[0] = dblToCol(j);
	  indexedCS->getCMYK(&color, &cmyk);
	    addProcessColor(colToDbl(cmyk.c), colToDbl(cmyk.m),
			    colToDbl(cmyk.y), colToDbl(cmyk.k));
	  }
	}
	writePS("\n");
      }
    }
    writePS(">]");
    if (genXform) {
      writePS(" {}");
    }
    break;

  case csSeparation:
    separationCS = (GfxSeparationColorSpace *)colorSpace;
    writePS("[/Separation /");
    writePSName(separationCS->getName()->getCString());
      writePS("  ");
    dumpColorSpaceL2(separationCS->getAlt(), gFalse, gFalse);
      writePS("\n");
    cvtFunction(separationCS->getFunc());
    writePS("]");
    if (genXform) {
      writePS(" {}");
    }
    if (updateColors) {
    addCustomColor(separationCS);
    }
    break;

  case csDeviceN:
    // DeviceN color spaces are a Level 3 PostScript feature.
    deviceNCS = (GfxDeviceNColorSpace *)colorSpace;
    dumpColorSpaceL2(deviceNCS->getAlt(), gFalse, updateColors);
    if (genXform) {
      writePS(" ");
      cvtFunction(deviceNCS->getTintTransformFunc());
    }
    break;

  case csPattern:
    //~ unimplemented
    break;
  }
}

#if OPI_SUPPORT
void PSOutputDev::opiBegin(GfxState *state, Dict *opiDict) {
  Object dict;

  if (globalParams->getPSOPI()) {
    opiDict->lookup("2.0", &dict);
    if (dict.isDict()) {
      opiBegin20(state, dict.getDict());
      dict.free();
    } else {
      dict.free();
      opiDict->lookup("1.3", &dict);
      if (dict.isDict()) {
	opiBegin13(state, dict.getDict());
      }
      dict.free();
    }
  }
}

void PSOutputDev::opiBegin20(GfxState *state, Dict *dict) {
  Object obj1, obj2, obj3, obj4;
  double width, height, left, right, top, bottom;
  int w, h;
  int i;

  writePS("%%BeginOPI: 2.0\n");
  writePS("%%Distilled\n");

  dict->lookup("F", &obj1);
  if (getFileSpec(&obj1, &obj2)) {
    writePSFmt("%%%%ImageFileName: %s\n",
	       obj2.getString()->getCString());
    obj2.free();
  }
  obj1.free();

  dict->lookup("MainImage", &obj1);
  if (obj1.isString()) {
    writePSFmt("%%%%MainImage: %s\n", obj1.getString()->getCString());
  }
  obj1.free();

  //~ ignoring 'Tags' entry
  //~ need to use writePSString() and deal with >255-char lines

  dict->lookup("Size", &obj1);
  if (obj1.isArray() && obj1.arrayGetLength() == 2) {
    obj1.arrayGet(0, &obj2);
    width = obj2.getNum();
    obj2.free();
    obj1.arrayGet(1, &obj2);
    height = obj2.getNum();
    obj2.free();
    writePSFmt("%%%%ImageDimensions: %g %g\n", width, height);
  }
  obj1.free();

  dict->lookup("CropRect", &obj1);
  if (obj1.isArray() && obj1.arrayGetLength() == 4) {
    obj1.arrayGet(0, &obj2);
    left = obj2.getNum();
    obj2.free();
    obj1.arrayGet(1, &obj2);
    top = obj2.getNum();
    obj2.free();
    obj1.arrayGet(2, &obj2);
    right = obj2.getNum();
    obj2.free();
    obj1.arrayGet(3, &obj2);
    bottom = obj2.getNum();
    obj2.free();
    writePSFmt("%%%%ImageCropRect: %g %g %g %g\n", left, top, right, bottom);
  }
  obj1.free();

  dict->lookup("Overprint", &obj1);
  if (obj1.isBool()) {
    writePSFmt("%%%%ImageOverprint: %s\n", obj1.getBool() ? "true" : "false");
  }
  obj1.free();

  dict->lookup("Inks", &obj1);
  if (obj1.isName()) {
    writePSFmt("%%%%ImageInks: %s\n", obj1.getName());
  } else if (obj1.isArray() && obj1.arrayGetLength() >= 1) {
    obj1.arrayGet(0, &obj2);
    if (obj2.isName()) {
      writePSFmt("%%%%ImageInks: %s %d",
		 obj2.getName(), (obj1.arrayGetLength() - 1) / 2);
      for (i = 1; i+1 < obj1.arrayGetLength(); i += 2) {
	obj1.arrayGet(i, &obj3);
	obj1.arrayGet(i+1, &obj4);
	if (obj3.isString() && obj4.isNum()) {
	  writePS(" ");
	  writePSString(obj3.getString());
	  writePSFmt(" %g", obj4.getNum());
	}
	obj3.free();
	obj4.free();
      }
      writePS("\n");
    }
    obj2.free();
  }
  obj1.free();

  writePS("gsave\n");

  writePS("%%BeginIncludedImage\n");

  dict->lookup("IncludedImageDimensions", &obj1);
  if (obj1.isArray() && obj1.arrayGetLength() == 2) {
    obj1.arrayGet(0, &obj2);
    w = obj2.getInt();
    obj2.free();
    obj1.arrayGet(1, &obj2);
    h = obj2.getInt();
    obj2.free();
    writePSFmt("%%%%IncludedImageDimensions: %d %d\n", w, h);
  }
  obj1.free();

  dict->lookup("IncludedImageQuality", &obj1);
  if (obj1.isNum()) {
    writePSFmt("%%%%IncludedImageQuality: %g\n", obj1.getNum());
  }
  obj1.free();

  ++opi20Nest;
}

void PSOutputDev::opiBegin13(GfxState *state, Dict *dict) {
  Object obj1, obj2;
  int left, right, top, bottom, samples, bits, width, height;
  double c, m, y, k;
  double llx, lly, ulx, uly, urx, ury, lrx, lry;
  double tllx, tlly, tulx, tuly, turx, tury, tlrx, tlry;
  double horiz, vert;
  int i, j;

  writePS("save\n");
  writePS("/opiMatrix2 matrix currentmatrix def\n");
  writePS("opiMatrix setmatrix\n");

  dict->lookup("F", &obj1);
  if (getFileSpec(&obj1, &obj2)) {
    writePSFmt("%%ALDImageFileName: %s\n",
	       obj2.getString()->getCString());
    obj2.free();
  }
  obj1.free();

  dict->lookup("CropRect", &obj1);
  if (obj1.isArray() && obj1.arrayGetLength() == 4) {
    obj1.arrayGet(0, &obj2);
    left = obj2.getInt();
    obj2.free();
    obj1.arrayGet(1, &obj2);
    top = obj2.getInt();
    obj2.free();
    obj1.arrayGet(2, &obj2);
    right = obj2.getInt();
    obj2.free();
    obj1.arrayGet(3, &obj2);
    bottom = obj2.getInt();
    obj2.free();
    writePSFmt("%%ALDImageCropRect: %d %d %d %d\n", left, top, right, bottom);
  }
  obj1.free();

  dict->lookup("Color", &obj1);
  if (obj1.isArray() && obj1.arrayGetLength() == 5) {
    obj1.arrayGet(0, &obj2);
    c = obj2.getNum();
    obj2.free();
    obj1.arrayGet(1, &obj2);
    m = obj2.getNum();
    obj2.free();
    obj1.arrayGet(2, &obj2);
    y = obj2.getNum();
    obj2.free();
    obj1.arrayGet(3, &obj2);
    k = obj2.getNum();
    obj2.free();
    obj1.arrayGet(4, &obj2);
    if (obj2.isString()) {
      writePSFmt("%%ALDImageColor: %g %g %g %g ", c, m, y, k);
      writePSString(obj2.getString());
      writePS("\n");
    }
    obj2.free();
  }
  obj1.free();

  dict->lookup("ColorType", &obj1);
  if (obj1.isName()) {
    writePSFmt("%%ALDImageColorType: %s\n", obj1.getName());
  }
  obj1.free();

  //~ ignores 'Comments' entry
  //~ need to handle multiple lines

  dict->lookup("CropFixed", &obj1);
  if (obj1.isArray()) {
    obj1.arrayGet(0, &obj2);
    ulx = obj2.getNum();
    obj2.free();
    obj1.arrayGet(1, &obj2);
    uly = obj2.getNum();
    obj2.free();
    obj1.arrayGet(2, &obj2);
    lrx = obj2.getNum();
    obj2.free();
    obj1.arrayGet(3, &obj2);
    lry = obj2.getNum();
    obj2.free();
    writePSFmt("%%ALDImageCropFixed: %g %g %g %g\n", ulx, uly, lrx, lry);
  }
  obj1.free();

  dict->lookup("GrayMap", &obj1);
  if (obj1.isArray()) {
    writePS("%ALDImageGrayMap:");
    for (i = 0; i < obj1.arrayGetLength(); i += 16) {
      if (i > 0) {
	writePS("\n%%+");
      }
      for (j = 0; j < 16 && i+j < obj1.arrayGetLength(); ++j) {
	obj1.arrayGet(i+j, &obj2);
	writePSFmt(" %d", obj2.getInt());
	obj2.free();
      }
    }
    writePS("\n");
  }
  obj1.free();

  dict->lookup("ID", &obj1);
  if (obj1.isString()) {
    writePSFmt("%%ALDImageID: %s\n", obj1.getString()->getCString());
  }
  obj1.free();

  dict->lookup("ImageType", &obj1);
  if (obj1.isArray() && obj1.arrayGetLength() == 2) {
    obj1.arrayGet(0, &obj2);
    samples = obj2.getInt();
    obj2.free();
    obj1.arrayGet(1, &obj2);
    bits = obj2.getInt();
    obj2.free();
    writePSFmt("%%ALDImageType: %d %d\n", samples, bits);
  }
  obj1.free();

  dict->lookup("Overprint", &obj1);
  if (obj1.isBool()) {
    writePSFmt("%%ALDImageOverprint: %s\n", obj1.getBool() ? "true" : "false");
  }
  obj1.free();

  dict->lookup("Position", &obj1);
  if (obj1.isArray() && obj1.arrayGetLength() == 8) {
    obj1.arrayGet(0, &obj2);
    llx = obj2.getNum();
    obj2.free();
    obj1.arrayGet(1, &obj2);
    lly = obj2.getNum();
    obj2.free();
    obj1.arrayGet(2, &obj2);
    ulx = obj2.getNum();
    obj2.free();
    obj1.arrayGet(3, &obj2);
    uly = obj2.getNum();
    obj2.free();
    obj1.arrayGet(4, &obj2);
    urx = obj2.getNum();
    obj2.free();
    obj1.arrayGet(5, &obj2);
    ury = obj2.getNum();
    obj2.free();
    obj1.arrayGet(6, &obj2);
    lrx = obj2.getNum();
    obj2.free();
    obj1.arrayGet(7, &obj2);
    lry = obj2.getNum();
    obj2.free();
    opiTransform(state, llx, lly, &tllx, &tlly);
    opiTransform(state, ulx, uly, &tulx, &tuly);
    opiTransform(state, urx, ury, &turx, &tury);
    opiTransform(state, lrx, lry, &tlrx, &tlry);
    writePSFmt("%%ALDImagePosition: %g %g %g %g %g %g %g %g\n",
	       tllx, tlly, tulx, tuly, turx, tury, tlrx, tlry);
    obj2.free();
  }
  obj1.free();

  dict->lookup("Resolution", &obj1);
  if (obj1.isArray() && obj1.arrayGetLength() == 2) {
    obj1.arrayGet(0, &obj2);
    horiz = obj2.getNum();
    obj2.free();
    obj1.arrayGet(1, &obj2);
    vert = obj2.getNum();
    obj2.free();
    writePSFmt("%%ALDImageResoution: %g %g\n", horiz, vert);
    obj2.free();
  }
  obj1.free();

  dict->lookup("Size", &obj1);
  if (obj1.isArray() && obj1.arrayGetLength() == 2) {
    obj1.arrayGet(0, &obj2);
    width = obj2.getInt();
    obj2.free();
    obj1.arrayGet(1, &obj2);
    height = obj2.getInt();
    obj2.free();
    writePSFmt("%%ALDImageDimensions: %d %d\n", width, height);
  }
  obj1.free();

  //~ ignoring 'Tags' entry
  //~ need to use writePSString() and deal with >255-char lines

  dict->lookup("Tint", &obj1);
  if (obj1.isNum()) {
    writePSFmt("%%ALDImageTint: %g\n", obj1.getNum());
  }
  obj1.free();

  dict->lookup("Transparency", &obj1);
  if (obj1.isBool()) {
    writePSFmt("%%ALDImageTransparency: %s\n", obj1.getBool() ? "true" : "false");
  }
  obj1.free();

  writePS("%%BeginObject: image\n");
  writePS("opiMatrix2 setmatrix\n");
  ++opi13Nest;
}

// Convert PDF user space coordinates to PostScript default user space
// coordinates.  This has to account for both the PDF CTM and the
// PSOutputDev page-fitting transform.
void PSOutputDev::opiTransform(GfxState *state, double x0, double y0,
			       double *x1, double *y1) {
  double t;

  state->transform(x0, y0, x1, y1);
  *x1 += tx;
  *y1 += ty;
  if (rotate == 90) {
    t = *x1;
    *x1 = -*y1;
    *y1 = t;
  } else if (rotate == 180) {
    *x1 = -*x1;
    *y1 = -*y1;
  } else if (rotate == 270) {
    t = *x1;
    *x1 = *y1;
    *y1 = -t;
  }
  *x1 *= xScale;
  *y1 *= yScale;
}

void PSOutputDev::opiEnd(GfxState *state, Dict *opiDict) {
  Object dict;

  if (globalParams->getPSOPI()) {
    opiDict->lookup("2.0", &dict);
    if (dict.isDict()) {
      writePS("%%EndIncludedImage\n");
      writePS("%%EndOPI\n");
      writePS("grestore\n");
      --opi20Nest;
      dict.free();
    } else {
      dict.free();
      opiDict->lookup("1.3", &dict);
      if (dict.isDict()) {
	writePS("%%EndObject\n");
	writePS("restore\n");
	--opi13Nest;
      }
      dict.free();
    }
  }
}

GBool PSOutputDev::getFileSpec(Object *fileSpec, Object *fileName) {
  if (fileSpec->isString()) {
    fileSpec->copy(fileName);
    return gTrue;
  }
  if (fileSpec->isDict()) {
    fileSpec->dictLookup("DOS", fileName);
    if (fileName->isString()) {
      return gTrue;
    }
    fileName->free();
    fileSpec->dictLookup("Mac", fileName);
    if (fileName->isString()) {
      return gTrue;
    }
    fileName->free();
    fileSpec->dictLookup("Unix", fileName);
    if (fileName->isString()) {
      return gTrue;
    }
    fileName->free();
    fileSpec->dictLookup("F", fileName);
    if (fileName->isString()) {
      return gTrue;
    }
    fileName->free();
  }
  return gFalse;
}
#endif // OPI_SUPPORT

void PSOutputDev::type3D0(GfxState *state, double wx, double wy) {
  writePSFmt("%g %g setcharwidth\n", wx, wy);
  writePS("q\n");
}

void PSOutputDev::type3D1(GfxState *state, double wx, double wy,
			  double llx, double lly, double urx, double ury) {
  t3WX = wx;
  t3WY = wy;
  t3LLX = llx;
  t3LLY = lly;
  t3URX = urx;
  t3URY = ury;
  t3String = new GooString();
  writePS("q\n");
  t3Cacheable = gTrue;
}

void PSOutputDev::psXObject(Stream *psStream, Stream *level1Stream) {
  Stream *str;
  int c;

  if ((level == psLevel1 || level == psLevel1Sep) && level1Stream) {
    str = level1Stream;
  } else {
    str = psStream;
  }
  str->reset();
  while ((c = str->getChar()) != EOF) {
    writePSChar(c);
  }
  str->close();
}

//~ can nextFunc be reset to 0 -- maybe at the start of each page?
//~   or maybe at the start of each color space / pattern?
void PSOutputDev::cvtFunction(Function *func) {
  SampledFunction *func0;
  ExponentialFunction *func2;
  StitchingFunction *func3;
  PostScriptFunction *func4;
  int thisFunc, m, n, nSamples, i, j, k;

  switch (func->getType()) {

  case -1:			// identity
    writePS("{}\n");
    break;

  case 0:			// sampled
    func0 = (SampledFunction *)func;
    thisFunc = nextFunc++;
    m = func0->getInputSize();
    n = func0->getOutputSize();
    nSamples = n;
    for (i = 0; i < m; ++i) {
      nSamples *= func0->getSampleSize(i);
    }
    writePSFmt("/xpdfSamples%d [\n", thisFunc);
    for (i = 0; i < nSamples; ++i) {
      writePSFmt("%g\n", func0->getSamples()[i]);
    }
    writePS("] def\n");
    writePSFmt("{ %d array %d array %d 2 roll\n", 2*m, m, m+2);
    // [e01] [efrac] x0 x1 ... xm-1
    for (i = m-1; i >= 0; --i) {
      // [e01] [efrac] x0 x1 ... xi
      writePSFmt("%g sub %g mul %g add\n",
	      func0->getDomainMin(i),
	      (func0->getEncodeMax(i) - func0->getEncodeMin(i)) /
	        (func0->getDomainMax(i) - func0->getDomainMin(i)),
	      func0->getEncodeMin(i));
      // [e01] [efrac] x0 x1 ... xi-1 xi'
      writePSFmt("dup 0 lt { pop 0 } { dup %d gt { pop %d } if } ifelse\n",
		 func0->getSampleSize(i) - 1, func0->getSampleSize(i) - 1);
      // [e01] [efrac] x0 x1 ... xi-1 xi'
      writePS("dup floor cvi exch dup ceiling cvi exch 2 index sub\n");
      // [e01] [efrac] x0 x1 ... xi-1 floor(xi') ceiling(xi') xi'-floor(xi')
      writePSFmt("%d index %d 3 2 roll put\n", i+3, i);
      // [e01] [efrac] x0 x1 ... xi-1 floor(xi') ceiling(xi')
      writePSFmt("%d index %d 3 2 roll put\n", i+3, 2*i+1);
      // [e01] [efrac] x0 x1 ... xi-1 floor(xi')
      writePSFmt("%d index %d 3 2 roll put\n", i+2, 2*i);
      // [e01] [efrac] x0 x1 ... xi-1
    }
    // [e01] [efrac]
    for (i = 0; i < n; ++i) {
      // [e01] [efrac] y(0) ... y(i-1)
      for (j = 0; j < (1<<m); ++j) {
	// [e01] [efrac] y(0) ... y(i-1) s(0) s(1) ... s(j-1)
	writePSFmt("xpdfSamples%d\n", thisFunc);
	k = m - 1;
	writePSFmt("%d index %d get\n", i+j+2, 2 * k + ((j >> k) & 1));
	for (k = m - 2; k >= 0; --k) {
	  writePSFmt("%d mul %d index %d get add\n",
		     func0->getSampleSize(k),
		     i + j + 3,
		     2 * k + ((j >> k) & 1));
	}
	if (n > 1) {
	  writePSFmt("%d mul %d add ", n, i);
	}
	writePS("get\n");
      }
      // [e01] [efrac] y(0) ... y(i-1) s(0) s(1) ... s(2^m-1)
      for (j = 0; j < m; ++j) {
	// [e01] [efrac] y(0) ... y(i-1) s(0) s(1) ... s(2^(m-j)-1)
	for (k = 0; k < (1 << (m - j)); k += 2) {
	  // [e01] [efrac] y(0) ... y(i-1) <k/2 s' values> <2^(m-j)-k s values>
	  writePSFmt("%d index %d get dup\n", i + k/2 + (1 << (m-j)) - k, j);
	  writePS("3 2 roll mul exch 1 exch sub 3 2 roll mul add\n");
	  writePSFmt("%d 1 roll\n", k/2 + (1 << m-j) - k - 1);
	}
	// [e01] [efrac] s'(0) s'(1) ... s(2^(m-j-1)-1)
      }
      // [e01] [efrac] y(0) ... y(i-1) s
      writePSFmt("%g mul %g add\n",
		 func0->getDecodeMax(i) - func0->getDecodeMin(i),
		 func0->getDecodeMin(i));
      writePSFmt("dup %g lt { pop %g } { dup %g gt { pop %g } if } ifelse\n",
		 func0->getRangeMin(i), func0->getRangeMin(i),
		 func0->getRangeMax(i), func0->getRangeMax(i));
      // [e01] [efrac] y(0) ... y(i-1) y(i)
    }
    // [e01] [efrac] y(0) ... y(n-1)
    writePSFmt("%d %d roll pop pop }\n", n+2, n);
    break;

  case 2:			// exponential
    func2 = (ExponentialFunction *)func;
    n = func2->getOutputSize();
    writePSFmt("{ dup %g lt { pop %g } { dup %g gt { pop %g } if } ifelse\n",
	       func2->getDomainMin(0), func2->getDomainMin(0),
	       func2->getDomainMax(0), func2->getDomainMax(0));
    // x
    for (i = 0; i < n; ++i) {
      // x y(0) .. y(i-1)
      writePSFmt("%d index %g exp %g mul %g add\n",
		 i, func2->getE(), func2->getC1()[i] - func2->getC0()[i],
		 func2->getC0()[i]);
      if (func2->getHasRange()) {
	writePSFmt("dup %g lt { pop %g } { dup %g gt { pop %g } if } ifelse\n",
		   func2->getRangeMin(i), func2->getRangeMin(i),
		   func2->getRangeMax(i), func2->getRangeMax(i));
      }
    }
    // x y(0) .. y(n-1)
    writePSFmt("%d %d roll pop }\n", n+1, n);
    break;

  case 3:			// stitching
    func3 = (StitchingFunction *)func;
    thisFunc = nextFunc++;
    for (i = 0; i < func3->getNumFuncs(); ++i) {
      cvtFunction(func3->getFunc(i));
      writePSFmt("/xpdfFunc%d_%d exch def\n", thisFunc, i);
    }
    writePSFmt("{ dup %g lt { pop %g } { dup %g gt { pop %g } if } ifelse\n",
	       func3->getDomainMin(0), func3->getDomainMin(0),
	       func3->getDomainMax(0), func3->getDomainMax(0));
    for (i = 0; i < func3->getNumFuncs() - 1; ++i) {
      writePSFmt("dup %g lt { %g sub %g mul %g add xpdfFunc%d_%d } {\n",
		 func3->getBounds()[i+1],
		 func3->getBounds()[i],
		 (func3->getEncode()[2*i+1] - func3->getEncode()[2*i]) /
		   (func3->getBounds()[i+1] - func3->getBounds()[i]),
		 func3->getEncode()[2*i],
		 thisFunc, i);
    }
    writePSFmt("%g sub %g mul %g add xpdfFunc%d_%d\n",
	       func3->getBounds()[i],
	       (func3->getEncode()[2*i+1] - func3->getEncode()[2*i]) /
	         (func3->getBounds()[i+1] - func3->getBounds()[i]),
	       func3->getEncode()[2*i],
	       thisFunc, i);
    for (i = 0; i < func3->getNumFuncs() - 1; ++i) {
      writePS("} ifelse\n");
    }
    writePS("}\n");
    break;

  case 4:			// PostScript
    func4 = (PostScriptFunction *)func;
    writePS(func4->getCodeString()->getCString());
    writePS("\n");
    break;
  }
}

void PSOutputDev::writePSChar(char c) {
  if (t3String) {
    t3String->append(c);
  } else {
    (*outputFunc)(outputStream, &c, 1);
  }
}

void PSOutputDev::writePS(char *s) {
  if (t3String) {
    t3String->append(s);
  } else {
    (*outputFunc)(outputStream, s, strlen(s));
  }
}

void PSOutputDev::writePSFmt(const char *fmt, ...) {
  va_list args;
  char buf[512];

  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  va_end(args);
  if (t3String) {
    t3String->append(buf);
  } else {
    (*outputFunc)(outputStream, buf, strlen(buf));
  }
}

void PSOutputDev::writePSString(GooString *s) {
  Guchar *p;
  int n, line;
  char buf[8];

  writePSChar('(');
  line = 1;
  for (p = (Guchar *)s->getCString(), n = s->getLength(); n; ++p, --n) {
    if (line >= 64) {
      writePSChar('\\');
      writePSChar('\n');
      line = 0;
    }
    if (*p == '(' || *p == ')' || *p == '\\') {
      writePSChar('\\');
      writePSChar((char)*p);
      line += 2;
    } else if (*p < 0x20 || *p >= 0x80) {
      sprintf(buf, "\\%03o", *p);
      writePS(buf);
      line += 4;
    } else {
      writePSChar((char)*p);
      ++line;
    }
  }
  writePSChar(')');
}

void PSOutputDev::writePSName(char *s) {
  char *p;
  char c;

  p = s;
  while ((c = *p++)) {
    if (c <= (char)0x20 || c >= (char)0x7f ||
	c == '(' || c == ')' || c == '<' || c == '>' ||
	c == '[' || c == ']' || c == '{' || c == '}' ||
	c == '/' || c == '%') {
      writePSFmt("#%02x", c & 0xff);
    } else {
      writePSChar(c);
    }
  }
}

GooString *PSOutputDev::filterPSName(GooString *name) {
  GooString *name2;
  char buf[8];
  int i;
  char c;

  name2 = new GooString();

  // ghostscript chokes on names that begin with out-of-limits
  // numbers, e.g., 1e4foo is handled correctly (as a name), but
  // 1e999foo generates a limitcheck error
  c = name->getChar(0);
  if (c >= '0' && c <= '9') {
    name2->append('f');
  }

  for (i = 0; i < name->getLength(); ++i) {
    c = name->getChar(i);
    if (c <= (char)0x20 || c >= (char)0x7f ||
	c == '(' || c == ')' || c == '<' || c == '>' ||
	c == '[' || c == ']' || c == '{' || c == '}' ||
	c == '/' || c == '%') {
      sprintf(buf, "#%02x", c & 0xff);
      name2->append(buf);
    } else {
      name2->append(c);
    }
  }
  return name2;
}
