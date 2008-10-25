/*
*******************************************************************************
*   Copyright (C) 2004-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*
*   file name:  
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   Created by: genheaders.pl, a perl script written by Ram Viswanadha
*
*  Contains data for commenting out APIs.
*  Gets included by umachine.h
*
*  THIS FILE IS MACHINE-GENERATED, DON'T PLAY WITH IT IF YOU DON'T KNOW WHAT
*  YOU ARE DOING, OTHERWISE VERY BAD THINGS WILL HAPPEN!
*/

#ifndef UINTRNAL_H
#define UINTRNAL_H

#ifdef U_HIDE_INTERNAL_API

#    if U_DISABLE_RENAMING
#        define RegexPatternDump RegexPatternDump_INTERNAL_API_DO_NOT_USE
#        define pl_addFontRun pl_addFontRun_INTERNAL_API_DO_NOT_USE
#        define pl_addLocaleRun pl_addLocaleRun_INTERNAL_API_DO_NOT_USE
#        define pl_addValueRun pl_addValueRun_INTERNAL_API_DO_NOT_USE
#        define pl_close pl_close_INTERNAL_API_DO_NOT_USE
#        define pl_closeFontRuns pl_closeFontRuns_INTERNAL_API_DO_NOT_USE
#        define pl_closeLine pl_closeLine_INTERNAL_API_DO_NOT_USE
#        define pl_closeLocaleRuns pl_closeLocaleRuns_INTERNAL_API_DO_NOT_USE
#        define pl_closeValueRuns pl_closeValueRuns_INTERNAL_API_DO_NOT_USE
#        define pl_countLineRuns pl_countLineRuns_INTERNAL_API_DO_NOT_USE
#        define pl_create pl_create_INTERNAL_API_DO_NOT_USE
#        define pl_getAscent pl_getAscent_INTERNAL_API_DO_NOT_USE
#        define pl_getDescent pl_getDescent_INTERNAL_API_DO_NOT_USE
#        define pl_getFontRunCount pl_getFontRunCount_INTERNAL_API_DO_NOT_USE
#        define pl_getFontRunFont pl_getFontRunFont_INTERNAL_API_DO_NOT_USE
#        define pl_getFontRunLastLimit pl_getFontRunLastLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getFontRunLimit pl_getFontRunLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getLeading pl_getLeading_INTERNAL_API_DO_NOT_USE
#        define pl_getLineAscent pl_getLineAscent_INTERNAL_API_DO_NOT_USE
#        define pl_getLineDescent pl_getLineDescent_INTERNAL_API_DO_NOT_USE
#        define pl_getLineLeading pl_getLineLeading_INTERNAL_API_DO_NOT_USE
#        define pl_getLineVisualRun pl_getLineVisualRun_INTERNAL_API_DO_NOT_USE
#        define pl_getLineWidth pl_getLineWidth_INTERNAL_API_DO_NOT_USE
#        define pl_getLocaleRunCount pl_getLocaleRunCount_INTERNAL_API_DO_NOT_USE
#        define pl_getLocaleRunLastLimit pl_getLocaleRunLastLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getLocaleRunLimit pl_getLocaleRunLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getLocaleRunLocale pl_getLocaleRunLocale_INTERNAL_API_DO_NOT_USE
#        define pl_getParagraphLevel pl_getParagraphLevel_INTERNAL_API_DO_NOT_USE
#        define pl_getTextDirection pl_getTextDirection_INTERNAL_API_DO_NOT_USE
#        define pl_getValueRunCount pl_getValueRunCount_INTERNAL_API_DO_NOT_USE
#        define pl_getValueRunLastLimit pl_getValueRunLastLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getValueRunLimit pl_getValueRunLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getValueRunValue pl_getValueRunValue_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunAscent pl_getVisualRunAscent_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunDescent pl_getVisualRunDescent_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunDirection pl_getVisualRunDirection_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunFont pl_getVisualRunFont_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunGlyphCount pl_getVisualRunGlyphCount_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunGlyphToCharMap pl_getVisualRunGlyphToCharMap_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunGlyphs pl_getVisualRunGlyphs_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunLeading pl_getVisualRunLeading_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunPositions pl_getVisualRunPositions_INTERNAL_API_DO_NOT_USE
#        define pl_isComplex pl_isComplex_INTERNAL_API_DO_NOT_USE
#        define pl_line pl_line_INTERNAL_API_DO_NOT_USE
#        define pl_nextLine pl_nextLine_INTERNAL_API_DO_NOT_USE
#        define pl_openEmptyFontRuns pl_openEmptyFontRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openEmptyLocaleRuns pl_openEmptyLocaleRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openEmptyValueRuns pl_openEmptyValueRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openFontRuns pl_openFontRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openLocaleRuns pl_openLocaleRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openValueRuns pl_openValueRuns_INTERNAL_API_DO_NOT_USE
#        define pl_paragraph pl_paragraph_INTERNAL_API_DO_NOT_USE
#        define pl_reflow pl_reflow_INTERNAL_API_DO_NOT_USE
#        define pl_resetFontRuns pl_resetFontRuns_INTERNAL_API_DO_NOT_USE
#        define pl_resetLocaleRuns pl_resetLocaleRuns_INTERNAL_API_DO_NOT_USE
#        define pl_resetValueRuns pl_resetValueRuns_INTERNAL_API_DO_NOT_USE
#        define pl_visualRun pl_visualRun_INTERNAL_API_DO_NOT_USE
#        define ucol_collatorToIdentifier ucol_collatorToIdentifier_INTERNAL_API_DO_NOT_USE
#        define ucol_equals ucol_equals_INTERNAL_API_DO_NOT_USE
#        define ucol_forgetUCA ucol_forgetUCA_INTERNAL_API_DO_NOT_USE
#        define ucol_getAttributeOrDefault ucol_getAttributeOrDefault_INTERNAL_API_DO_NOT_USE
#        define ucol_getUnsafeSet ucol_getUnsafeSet_INTERNAL_API_DO_NOT_USE
#        define ucol_identifierToShortString ucol_identifierToShortString_INTERNAL_API_DO_NOT_USE
#        define ucol_openFromIdentifier ucol_openFromIdentifier_INTERNAL_API_DO_NOT_USE
#        define ucol_prepareShortStringOpen ucol_prepareShortStringOpen_INTERNAL_API_DO_NOT_USE
#        define ucol_shortStringToIdentifier ucol_shortStringToIdentifier_INTERNAL_API_DO_NOT_USE
#        define uprv_getDefaultCodepage uprv_getDefaultCodepage_INTERNAL_API_DO_NOT_USE
#        define uprv_getDefaultLocaleID uprv_getDefaultLocaleID_INTERNAL_API_DO_NOT_USE
#        define ures_openFillIn ures_openFillIn_INTERNAL_API_DO_NOT_USE
#        define utf8_appendCharSafeBody utf8_appendCharSafeBody_INTERNAL_API_DO_NOT_USE
#        define utf8_back1SafeBody utf8_back1SafeBody_INTERNAL_API_DO_NOT_USE
#        define utf8_countTrailBytes utf8_countTrailBytes_INTERNAL_API_DO_NOT_USE
#        define utf8_nextCharSafeBody utf8_nextCharSafeBody_INTERNAL_API_DO_NOT_USE
#        define utf8_prevCharSafeBody utf8_prevCharSafeBody_INTERNAL_API_DO_NOT_USE
#    else
#        define RegexPatternDump_3_8 RegexPatternDump_INTERNAL_API_DO_NOT_USE
#        define pl_addFontRun_3_8 pl_addFontRun_INTERNAL_API_DO_NOT_USE
#        define pl_addLocaleRun_3_8 pl_addLocaleRun_INTERNAL_API_DO_NOT_USE
#        define pl_addValueRun_3_8 pl_addValueRun_INTERNAL_API_DO_NOT_USE
#        define pl_closeFontRuns_3_8 pl_closeFontRuns_INTERNAL_API_DO_NOT_USE
#        define pl_closeLine_3_8 pl_closeLine_INTERNAL_API_DO_NOT_USE
#        define pl_closeLocaleRuns_3_8 pl_closeLocaleRuns_INTERNAL_API_DO_NOT_USE
#        define pl_closeValueRuns_3_8 pl_closeValueRuns_INTERNAL_API_DO_NOT_USE
#        define pl_close_3_8 pl_close_INTERNAL_API_DO_NOT_USE
#        define pl_countLineRuns_3_8 pl_countLineRuns_INTERNAL_API_DO_NOT_USE
#        define pl_create_3_8 pl_create_INTERNAL_API_DO_NOT_USE
#        define pl_getAscent_3_8 pl_getAscent_INTERNAL_API_DO_NOT_USE
#        define pl_getDescent_3_8 pl_getDescent_INTERNAL_API_DO_NOT_USE
#        define pl_getFontRunCount_3_8 pl_getFontRunCount_INTERNAL_API_DO_NOT_USE
#        define pl_getFontRunFont_3_8 pl_getFontRunFont_INTERNAL_API_DO_NOT_USE
#        define pl_getFontRunLastLimit_3_8 pl_getFontRunLastLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getFontRunLimit_3_8 pl_getFontRunLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getLeading_3_8 pl_getLeading_INTERNAL_API_DO_NOT_USE
#        define pl_getLineAscent_3_8 pl_getLineAscent_INTERNAL_API_DO_NOT_USE
#        define pl_getLineDescent_3_8 pl_getLineDescent_INTERNAL_API_DO_NOT_USE
#        define pl_getLineLeading_3_8 pl_getLineLeading_INTERNAL_API_DO_NOT_USE
#        define pl_getLineVisualRun_3_8 pl_getLineVisualRun_INTERNAL_API_DO_NOT_USE
#        define pl_getLineWidth_3_8 pl_getLineWidth_INTERNAL_API_DO_NOT_USE
#        define pl_getLocaleRunCount_3_8 pl_getLocaleRunCount_INTERNAL_API_DO_NOT_USE
#        define pl_getLocaleRunLastLimit_3_8 pl_getLocaleRunLastLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getLocaleRunLimit_3_8 pl_getLocaleRunLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getLocaleRunLocale_3_8 pl_getLocaleRunLocale_INTERNAL_API_DO_NOT_USE
#        define pl_getParagraphLevel_3_8 pl_getParagraphLevel_INTERNAL_API_DO_NOT_USE
#        define pl_getTextDirection_3_8 pl_getTextDirection_INTERNAL_API_DO_NOT_USE
#        define pl_getValueRunCount_3_8 pl_getValueRunCount_INTERNAL_API_DO_NOT_USE
#        define pl_getValueRunLastLimit_3_8 pl_getValueRunLastLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getValueRunLimit_3_8 pl_getValueRunLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getValueRunValue_3_8 pl_getValueRunValue_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunAscent_3_8 pl_getVisualRunAscent_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunDescent_3_8 pl_getVisualRunDescent_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunDirection_3_8 pl_getVisualRunDirection_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunFont_3_8 pl_getVisualRunFont_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunGlyphCount_3_8 pl_getVisualRunGlyphCount_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunGlyphToCharMap_3_8 pl_getVisualRunGlyphToCharMap_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunGlyphs_3_8 pl_getVisualRunGlyphs_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunLeading_3_8 pl_getVisualRunLeading_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunPositions_3_8 pl_getVisualRunPositions_INTERNAL_API_DO_NOT_USE
#        define pl_isComplex_3_8 pl_isComplex_INTERNAL_API_DO_NOT_USE
#        define pl_line_3_8 pl_line_INTERNAL_API_DO_NOT_USE
#        define pl_nextLine_3_8 pl_nextLine_INTERNAL_API_DO_NOT_USE
#        define pl_openEmptyFontRuns_3_8 pl_openEmptyFontRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openEmptyLocaleRuns_3_8 pl_openEmptyLocaleRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openEmptyValueRuns_3_8 pl_openEmptyValueRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openFontRuns_3_8 pl_openFontRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openLocaleRuns_3_8 pl_openLocaleRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openValueRuns_3_8 pl_openValueRuns_INTERNAL_API_DO_NOT_USE
#        define pl_paragraph_3_8 pl_paragraph_INTERNAL_API_DO_NOT_USE
#        define pl_reflow_3_8 pl_reflow_INTERNAL_API_DO_NOT_USE
#        define pl_resetFontRuns_3_8 pl_resetFontRuns_INTERNAL_API_DO_NOT_USE
#        define pl_resetLocaleRuns_3_8 pl_resetLocaleRuns_INTERNAL_API_DO_NOT_USE
#        define pl_resetValueRuns_3_8 pl_resetValueRuns_INTERNAL_API_DO_NOT_USE
#        define pl_visualRun_3_8 pl_visualRun_INTERNAL_API_DO_NOT_USE
#        define ucol_collatorToIdentifier_3_8 ucol_collatorToIdentifier_INTERNAL_API_DO_NOT_USE
#        define ucol_equals_3_8 ucol_equals_INTERNAL_API_DO_NOT_USE
#        define ucol_forgetUCA_3_8 ucol_forgetUCA_INTERNAL_API_DO_NOT_USE
#        define ucol_getAttributeOrDefault_3_8 ucol_getAttributeOrDefault_INTERNAL_API_DO_NOT_USE
#        define ucol_getUnsafeSet_3_8 ucol_getUnsafeSet_INTERNAL_API_DO_NOT_USE
#        define ucol_identifierToShortString_3_8 ucol_identifierToShortString_INTERNAL_API_DO_NOT_USE
#        define ucol_openFromIdentifier_3_8 ucol_openFromIdentifier_INTERNAL_API_DO_NOT_USE
#        define ucol_prepareShortStringOpen_3_8 ucol_prepareShortStringOpen_INTERNAL_API_DO_NOT_USE
#        define ucol_shortStringToIdentifier_3_8 ucol_shortStringToIdentifier_INTERNAL_API_DO_NOT_USE
#        define uprv_getDefaultCodepage_3_8 uprv_getDefaultCodepage_INTERNAL_API_DO_NOT_USE
#        define uprv_getDefaultLocaleID_3_8 uprv_getDefaultLocaleID_INTERNAL_API_DO_NOT_USE
#        define ures_openFillIn_3_8 ures_openFillIn_INTERNAL_API_DO_NOT_USE
#        define utf8_appendCharSafeBody_3_8 utf8_appendCharSafeBody_INTERNAL_API_DO_NOT_USE
#        define utf8_back1SafeBody_3_8 utf8_back1SafeBody_INTERNAL_API_DO_NOT_USE
#        define utf8_countTrailBytes_3_8 utf8_countTrailBytes_INTERNAL_API_DO_NOT_USE
#        define utf8_nextCharSafeBody_3_8 utf8_nextCharSafeBody_INTERNAL_API_DO_NOT_USE
#        define utf8_prevCharSafeBody_3_8 utf8_prevCharSafeBody_INTERNAL_API_DO_NOT_USE
#    endif /* U_DISABLE_RENAMING */

#endif /* U_HIDE_INTERNAL_API */
#endif /* UINTRNAL_H */

