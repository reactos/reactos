/*
 *  @doc INTERNAL
 *
 *  @module _TOKENS.H -- All the tokens and then some |
 *
 *  Authors: <nl>
 *      Original RichEdit 1.0 RTF converter: Anthony Francisco <nl>
 *      Conversion to C++ and RichEdit 2.0:  Murray Sargent
 *      
 *  @devnote
 *      The Text Object Model (TOM) keywords come first followed by picture
 *      and object keywords.  The order within a group can matter, since it
 *      may be used to simplify the input process.  Token values <lt> 256 are
 *      used for target character codes.
 */

#ifndef I_TOKENS_H_
#define I_TOKENS_H_
#pragma INCMSG("--- Beg 'tokens.h'")

typedef WORD    TOKEN;

/*
 *      Keyword --> Token table
 */
typedef struct _keyword
{
    CHAR *  szKeyword;              // The RTF keyword sans '\\'
    TOKEN   token;
} KEYWORD;

enum                                // Control tokens
{
    tokenLowestToken = 256,         // Lowest token for a keyword
    tokenText,                      // A string of characters
    tokenUnknownKeyword,            // A keyword we don't recognize
    tokenError,                     // Error condition token
    tokenUnknown,                   // Unknown token
    tokenEOF,                       // End-of-file token
    tokenStartGroup,                // Start group token
    tokenEndGroup,                  // End group token
    tokenObjectDataValue,           // Data for object 
    tokenPictureDataValue           // Data for picture

};

// @enum RTFKEYWORDS | RichEdit RTF Control Word Tokens

enum RTFKEYWORDS                    // Keyword tokens
{
    tokenRtf = 275,                 // @emem rtf
    tokenCharSetAnsi,               // @emem ansi
    tokenCharSetMacintosh,          // @emem mac
    tokenCharSetPc,                 // @emem pc
    tokenCharSetPs2,                // @emem pca
    tokenAnsiCodePage,              // @emem ansicpg

    tokenDefaultFont,               // @emem deff
    tokenDefaultLanguage,           // @emem deflang
    tokenDefaultTabWidth,           // @emem deftab
    tokenParagraphDefault,          // @emem pard
    tokenCharacterDefault,          // @emem plain


    // Fonts
    tokenFontTable,                 // @emem fonttbl
    tokenFontSelect,                // @emem f
                                    //          Keep next 8 in order
    tokenFontFamilyDefault,         // @emem fnil
    tokenFontFamilyRoman,           // @emem froman
    tokenFontFamilySwiss,           // @emem fswiss
    tokenFontFamilyModern,          // @emem fmodern
    tokenFontFamilyScript,          // @emem fscript
    tokenFontFamilyDecorative,      // @emem fdecor
    tokenFontFamilyTechnical,       // @emem ftech
    tokenFontFamilyBidi,            // @emem fbidi

    tokenCharSet,                   // @emem fcharset
    tokenPitch,                     // @emem fprq
    tokenFontEmbedded,              // @emem fontemb
    tokenRealFontName,              // @emem fname
    tokenFontFile,                  // @emem fontfile
    tokenCodePage,                  // @emem cpg
    tokenFontSize,                  // @emem fs

    // Colors
    tokenColorTable,                // @emem colortbl
    tokenColorBackground,           // @emem highlight (used to be cb)
    tokenColorForeground,           // @emem cf
                                    //          Keep next 3 in order
    tokenColorRed,                  // @emem red
    tokenColorGreen,                // @emem green
    tokenColorBlue,                 // @emem blue


    // Character formatting                     Keep next 15 effects in order
    tokenBold,                      // @emem b
    tokenItalic,                    // @emem i
    tokenUnderline,                 // @emem ul
    tokenStrikeOut,                 // @emem strike
    tokenProtect,                   // @emem protect
    tokenLink,                      // @emem link (check this...)
    tokenSmallCaps,                 // @emem scaps
    tokenCaps,                      // @emem caps
    tokenHiddenText,                // @emem v
    tokenOutline,                   // @emem outl
    tokenShadow,                    // @emem shad
    tokenEmboss,                    // @emem embo
    tokenImprint,                   // @emem impr
    tokenDisabled,                  // @emem disabled
    tokenRevised,                   // @emem revised
    
    tokenDeleted,                   // @emem deleted

    tokenStopUnderline,             // @emem ulnone Keep next 10 in order
    tokenUnderlineWord,             // @emem ulw    This thru uld are standard
    tokenUnderlineDouble,           // @emem uldb    Word underlines
    tokenUnderlineDotted,           // @emem uld        
    tokenUnderlineWave,             // @emem ulwave This down thru uldash are
    tokenUnderlineThick,            // @emem ulth    for FE
    tokenUnderlineHairline,         // @emem ulhair
    tokenUnderlineDashDotDotted,    // @emem uldashdd
    tokenUnderlineDashDotted,       // @emem uldashd    
    tokenUnderlineDash,             // @emem uldash

    tokenDown,                      // @emem dn
    tokenUp,                        // @emem up
                                    //              Keep next 3 in order
    tokenSubscript,                 // @emem sub
    tokenNoSuperSub,                // @emem nosupersub
    tokenSuperscript,               // @emem super

    tokenAnimText,                  // @emem animtext
    tokenExpand,                    // @emem expndtw
    tokenKerning,                   // @emem kerning
    tokenLanguage,                  // @emem lang
    tokenCharStyle,                 // @emem cs

    // Paragraph Formatting
    tokenEndParagraph,              // @emem par
    tokenSoftBreak,                 // @emem line
    tokenIndentFirst,               // @emem fi
    tokenIndentLeft,                // @emem li
    tokenIndentRight,               // @emem ri
                                    //          Keep next 4 in order
    tokenAlignLeft,                 // @emem ql     PFA_LEFT
    tokenAlignRight,                // @emem qr     PFA_RIGHT
    tokenAlignCenter,               // @emem qc     PFA_CENTER
    tokenAlignJustify,              // @emem qj     PFA_JUSTIFY

    tokenSpaceBefore,               // @emem sb
    tokenSpaceAfter,                // @emem sa
    tokenLineSpacing,               // @emem sl
    tokenLineSpacingRule,           // @emem slmult
    tokenStyle,                     // @emem s

    tokenLToRPara,                  // @emem ltrpar
                                    //          keep next 8 in order 
    tokenRToLPara,                  // @emem rtlpar
    tokenKeep,                      // @emem keep
    tokenKeepNext,                  // @emem keepn
    tokenPageBreakBefore,           // @emem pagebb
    tokenNoLineNumber,              // @emem noline
    tokenNoWidCtlPar,               // @emem nowidctlpar
    tokenHyphPar,                   // @emem hyphpar
    tokenSideBySide,                // @emem sbys

    tokenTabPosition,               // @emem tx
    tokenTabBar,                    // @emem tb
                                    //          keep next 5 in order 
    tokenTabLeaderDots,             // @emem tldot
    tokenTabLeaderHyphen,           // @emem tlhyph
    tokenTabLeaderUnderline,        // @emem tlul
    tokenTabLeaderThick,            // @emem tlth
    tokenTabLeaderEqual,            // @emem tleq
                                    //          keep next 4 in order 
    tokenCenterTab,                 // @emem tqc
    tokenFlushRightTab,             // @emem tqr
    tokenDecimalTab,                // @emem tqdec

    tokenParaNum,                   // @emem pn
    tokenParaNumIndent,             // @emem pnindent
    tokenParaNumBullet,             // @emem pnlvlblt
    tokenParaNumText,               // @emem pntext
    tokenParaNumAfter,              // @emem pntxta
    tokenParaNumBefore,             // @emem pntxtb

    tokenOptionalDestination,       // @emem *
    tokenField,                     // @emem field
    tokenFieldResult,               // @emem fldrslt
    tokenFieldInstruction,          // @emem fldinst
    tokenStyleSheet,                // @emem stylesheet
    tokenEndSection,                // @emem sect
    tokenSectionDefault,            // @emem sectd
    tokenDocumentArea,              // @emem info

    // Tables
    tokenInTable,                   // @emem intbl
    tokenCell,                      // @emem cell
    tokenRow,                       // @emem row

    // Special characters
    tokenUnicode,                   // @emem u
    tokenFormulaCharacter,          // |
    tokenIndexSubentry,             // :

    //  BiDi keywords
    tokenRToLChars,                 // @emem rtlch
    tokenLToRChars,                 // @emem ltrch
    tokenDisplayRToL,               // @emem rtlmark
    tokenDisplayLToR,               // @emem ltrmark
    tokenRToLDocument,              // @emem rtldoc
    tokenLToRDocument,              // @emem ltrdoc
    tokenZeroWidthJoiner,           // @emem zwj
    tokenZeroWidthNonJoiner,        // @emem zwnj


    //  T3J keywords
    tokenHorizontalRender,          // @emem horzdoc
    tokenVerticalRender,            // @emem vertdoc
    tokenFollowingPunct,            // @emem fchars
    tokenLeadingPunct,              // @emem lchars
    tokenNoOverflow,                // @emem nooverflow
    tokenNoWordBreak,               // @emem nocwrap
    tokenNoWordWrap,                // @emem nowwrap


    // Pictures                                     Keep next 4 in RECT order
    tokenCropLeft,                  // @emem piccropl
    tokenCropTop,                   // @emem piccropt
    tokenCropBottom,                // @emem piccropb
    tokenCropRight,                 // @emem piccropr
    tokenHeight,                    // @emem pich
    tokenWidth,                     // @emem picw
    tokenScaleX,                    // @emem picscalex
    tokenScaleY,                    // @emem picscaley
    tokenPicture,                   // @emem pict
    tokenDesiredHeight,             // @emem pichgoal
    tokenDesiredWidth,              // @emem picwgoal
                                    //              Keep next 3 in order
    tokenPictureWindowsBitmap,      // @emem wbitmap
    tokenPictureWindowsMetafile,    // @emem wmetafile
    tokenPictureWindowsDIB,         // @emem dibitmap

    tokenBinaryData,                // @emem bin
    tokenPictureQuickDraw,          // @emem macpict
    tokenPictureOS2Metafile,        // @emem pmmetafile
    tokenBitmapBitsPerPixel,        // @emem wbmbitspixel
    tokenBitmapNumPlanes,           // @emem wbmplanes
    tokenBitmapWidthBytes,          // @emem wbmwidthbytes


    // Objects
//  tokenCropLeft,                  // @emem objcropl       (see // Pictures)
//  tokenCropTop,                   // @emem objcropt
//  tokenCropRight,                 // @emem objcropr
//  tokenCropBottom,                // @emem objcropb
//  tokenHeight,                    // @emem objh
//  tokenWidth,                     // @emem objw
//  tokenScaleX,                    // @emem objscalex
//  tokenScaleY,                    // @emem objscaley
                                    //              Keep next 3 in order
    tokenObjectEmbedded,            // @emem objemb
    tokenObjectLink,                // @emem objlink
    tokenObjectAutoLink,            // @emem objautlink

    tokenObjectClass,               // @emem objclass
    tokenObjectData,                // @emem objdata
    tokenObject,                    // @emem object
    tokenObjectMacICEmbedder,       // @emem objicemb
    tokenObjectName,                // @emem objname
    tokenObjectMacPublisher,        // @emem objpub
    tokenObjectSetSize,             // @emem objsetsize
    tokenObjectMacSubscriber,       // @emem objsub
    tokenObjectResult,              // @emem result
    tokenObjectUpdate,              // @emem objupdate

    // Document info and layout
    tokenTimeSecond,                // @emem sec
    tokenTimeMinute,                // @emem min
    tokenTimeHour,                  // @emem hr
    tokenTimeDay,                   // @emem dy
    tokenTimeMonth,                 // @emem mo
    tokenTimeYear,                  // @emem yr
    tokenRevAuthor,                 // @emem revauth

    tokenMarginLeft,                // @emem margl
    tokenMarginRight,               // @emem margr
    tokenSectionMarginLeft,         // @emem marglsxn
    tokenSectionMarginRight         // @emem margrsxn
};

// @enum TOKENINDEX | RTFWrite Indices into rgKeyword[]

enum TOKENINDEX                     // rgKeyword[] indices
{                                   // MUST be in exact 1-to-1 with rgKeyword
    i_animtext,                     //  entries (see tokens.c).  Names consist
    i_ansi,                         //  of i_ followed by RTF control word
    i_ansicpg,                      
    i_b,                            
    i_blue,
    i_bullet,
    i_caps,
    i_cf,
    i_colortbl,
    i_cpg,
    i_cs,
    i_deff,
    i_deflang,
    i_deftab,
    i_deleted,
    i_dibitmap,
    i_disabled,
    i_dn,
    i_embo,
    i_emdash,
    i_emspace,
    i_endash,
    i_enspace,
    i_expndtw,
    i_f,
    i_fcharset,
    i_fdecor,
    i_fi,
    i_field,
    i_fldinst,
    i_fldrslt,
    i_fmodern,
    i_fname,
    i_fnil,
    i_fontemb,
    i_fontfile,
    i_fonttbl,
    i_fprq,
    i_froman,
    i_fs,
    i_fscript,
    i_fswiss,
    i_ftech,
    i_green,
    i_highlight,
    i_hyphpar,
    i_i,
    i_impr,
    i_info,
    i_keep,
    i_keepn,
    i_kerning,
    i_lang,
    i_ldblquote,
    i_li,
    i_lnkd,
    i_line,
    i_lquote,
    i_ltrpar,
    i_mac,
    i_noline,
    i_nosupersub,
    i_nowidctlpar,
    i_objautlink,
    i_objclass,
    i_objcropb,
    i_objcropl,
    i_objcropr,
    i_objcropt,
    i_objdata,
    i_object,
    i_objemb,
    i_objh,
    i_objicemb,
    i_objlink,
    i_objname,
    i_objpub,
    i_objscalex,
    i_objscaley,
    i_objsetsize,
    i_objsub,
    i_objupdate,
    i_objw,
    i_outl,
    i_pagebb,
    i_par,
    i_pard,
    i_pc,
    i_pca,
    i_piccropb,
    i_piccropl,
    i_piccropr,
    i_piccropt,
    i_pich,
    i_pichgoal,
    i_picscalex,
    i_picscaley,
    i_pict,
    i_picw,
    i_picwgoal,
    i_plain,
    i_pn,
    i_pnindent,
    i_pnlvlblt,
    i_pntext,
    i_pntxta,
    i_pntxtb,
    i_protect,
    i_qc,
    i_qj,
    i_ql,
    i_qr,
    i_rdblquote,
    i_red,
    i_result,
    i_ri,
    i_rquote,
    i_rtf,
    i_rtlpar,
    i_s,
    i_sa,
    i_sb,
    i_sbys,
    i_scaps,
    i_shad,
    i_sl,
    i_slmult,
    i_strike,
    i_stylesheet,
    i_sub,
    i_super,
    i_tab,
    i_tb,
    i_tldot,
    i_tleq,
    i_tlhyph,
    i_tlth,
    i_tlul,
    i_tqc,
    i_tqdec,
    i_tqr,
    i_tx,
    i_u,
    i_ul,
    i_uld,
    i_uldb,
    i_ulnone,
    i_ulw,
    i_up,
    i_v,
    i_wbitmap,
    i_wbmbitspixel,
    i_wbmplanes,
    i_wbmwidthbytes,
    i_wmetafile
};

#pragma INCMSG("--- End 'tokens.h'")
#else
#pragma INCMSG("*** Dup 'tokens.h'")
#endif
