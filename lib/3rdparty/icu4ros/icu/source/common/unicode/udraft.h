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

#ifndef UDRAFT_H
#define UDRAFT_H

#ifdef U_HIDE_DRAFT_API

#    if U_DISABLE_RENAMING
#        define u_fclose u_fclose_DRAFT_API_DO_NOT_USE
#        define u_feof u_feof_DRAFT_API_DO_NOT_USE
#        define u_fflush u_fflush_DRAFT_API_DO_NOT_USE
#        define u_fgetConverter u_fgetConverter_DRAFT_API_DO_NOT_USE
#        define u_fgetc u_fgetc_DRAFT_API_DO_NOT_USE
#        define u_fgetcodepage u_fgetcodepage_DRAFT_API_DO_NOT_USE
#        define u_fgetcx u_fgetcx_DRAFT_API_DO_NOT_USE
#        define u_fgetfile u_fgetfile_DRAFT_API_DO_NOT_USE
#        define u_fgetlocale u_fgetlocale_DRAFT_API_DO_NOT_USE
#        define u_fgets u_fgets_DRAFT_API_DO_NOT_USE
#        define u_file_read u_file_read_DRAFT_API_DO_NOT_USE
#        define u_file_write u_file_write_DRAFT_API_DO_NOT_USE
#        define u_finit u_finit_DRAFT_API_DO_NOT_USE
#        define u_fopen u_fopen_DRAFT_API_DO_NOT_USE
#        define u_fprintf u_fprintf_DRAFT_API_DO_NOT_USE
#        define u_fprintf_u u_fprintf_u_DRAFT_API_DO_NOT_USE
#        define u_fputc u_fputc_DRAFT_API_DO_NOT_USE
#        define u_fputs u_fputs_DRAFT_API_DO_NOT_USE
#        define u_frewind u_frewind_DRAFT_API_DO_NOT_USE
#        define u_fscanf u_fscanf_DRAFT_API_DO_NOT_USE
#        define u_fscanf_u u_fscanf_u_DRAFT_API_DO_NOT_USE
#        define u_fsetcodepage u_fsetcodepage_DRAFT_API_DO_NOT_USE
#        define u_fsetlocale u_fsetlocale_DRAFT_API_DO_NOT_USE
#        define u_fsettransliterator u_fsettransliterator_DRAFT_API_DO_NOT_USE
#        define u_fstropen u_fstropen_DRAFT_API_DO_NOT_USE
#        define u_fungetc u_fungetc_DRAFT_API_DO_NOT_USE
#        define u_snprintf u_snprintf_DRAFT_API_DO_NOT_USE
#        define u_snprintf_u u_snprintf_u_DRAFT_API_DO_NOT_USE
#        define u_sprintf u_sprintf_DRAFT_API_DO_NOT_USE
#        define u_sprintf_u u_sprintf_u_DRAFT_API_DO_NOT_USE
#        define u_sscanf u_sscanf_DRAFT_API_DO_NOT_USE
#        define u_sscanf_u u_sscanf_u_DRAFT_API_DO_NOT_USE
#        define u_vfprintf u_vfprintf_DRAFT_API_DO_NOT_USE
#        define u_vfprintf_u u_vfprintf_u_DRAFT_API_DO_NOT_USE
#        define u_vfscanf u_vfscanf_DRAFT_API_DO_NOT_USE
#        define u_vfscanf_u u_vfscanf_u_DRAFT_API_DO_NOT_USE
#        define u_vsnprintf u_vsnprintf_DRAFT_API_DO_NOT_USE
#        define u_vsnprintf_u u_vsnprintf_u_DRAFT_API_DO_NOT_USE
#        define u_vsprintf u_vsprintf_DRAFT_API_DO_NOT_USE
#        define u_vsprintf_u u_vsprintf_u_DRAFT_API_DO_NOT_USE
#        define u_vsscanf u_vsscanf_DRAFT_API_DO_NOT_USE
#        define u_vsscanf_u u_vsscanf_u_DRAFT_API_DO_NOT_USE
#        define ucal_getTZDataVersion ucal_getTZDataVersion_DRAFT_API_DO_NOT_USE
#        define ucasemap_getBreakIterator ucasemap_getBreakIterator_DRAFT_API_DO_NOT_USE
#        define ucasemap_setBreakIterator ucasemap_setBreakIterator_DRAFT_API_DO_NOT_USE
#        define ucasemap_toTitle ucasemap_toTitle_DRAFT_API_DO_NOT_USE
#        define ucasemap_utf8FoldCase ucasemap_utf8FoldCase_DRAFT_API_DO_NOT_USE
#        define ucasemap_utf8ToTitle ucasemap_utf8ToTitle_DRAFT_API_DO_NOT_USE
#        define udatpg_addPattern udatpg_addPattern_DRAFT_API_DO_NOT_USE
#        define udatpg_clone udatpg_clone_DRAFT_API_DO_NOT_USE
#        define udatpg_close udatpg_close_DRAFT_API_DO_NOT_USE
#        define udatpg_getAppendItemFormat udatpg_getAppendItemFormat_DRAFT_API_DO_NOT_USE
#        define udatpg_getAppendItemName udatpg_getAppendItemName_DRAFT_API_DO_NOT_USE
#        define udatpg_getBaseSkeleton udatpg_getBaseSkeleton_DRAFT_API_DO_NOT_USE
#        define udatpg_getBestPattern udatpg_getBestPattern_DRAFT_API_DO_NOT_USE
#        define udatpg_getDateTimeFormat udatpg_getDateTimeFormat_DRAFT_API_DO_NOT_USE
#        define udatpg_getDecimal udatpg_getDecimal_DRAFT_API_DO_NOT_USE
#        define udatpg_getPatternForSkeleton udatpg_getPatternForSkeleton_DRAFT_API_DO_NOT_USE
#        define udatpg_getSkeleton udatpg_getSkeleton_DRAFT_API_DO_NOT_USE
#        define udatpg_open udatpg_open_DRAFT_API_DO_NOT_USE
#        define udatpg_openBaseSkeletons udatpg_openBaseSkeletons_DRAFT_API_DO_NOT_USE
#        define udatpg_openEmpty udatpg_openEmpty_DRAFT_API_DO_NOT_USE
#        define udatpg_openSkeletons udatpg_openSkeletons_DRAFT_API_DO_NOT_USE
#        define udatpg_replaceFieldTypes udatpg_replaceFieldTypes_DRAFT_API_DO_NOT_USE
#        define udatpg_setAppendItemFormat udatpg_setAppendItemFormat_DRAFT_API_DO_NOT_USE
#        define udatpg_setAppendItemName udatpg_setAppendItemName_DRAFT_API_DO_NOT_USE
#        define udatpg_setDateTimeFormat udatpg_setDateTimeFormat_DRAFT_API_DO_NOT_USE
#        define udatpg_setDecimal udatpg_setDecimal_DRAFT_API_DO_NOT_USE
#        define uloc_getLocaleForLCID uloc_getLocaleForLCID_DRAFT_API_DO_NOT_USE
#        define uset_clone uset_clone_DRAFT_API_DO_NOT_USE
#        define uset_cloneAsThawed uset_cloneAsThawed_DRAFT_API_DO_NOT_USE
#        define uset_freeze uset_freeze_DRAFT_API_DO_NOT_USE
#        define uset_isFrozen uset_isFrozen_DRAFT_API_DO_NOT_USE
#        define uset_span uset_span_DRAFT_API_DO_NOT_USE
#        define uset_spanBack uset_spanBack_DRAFT_API_DO_NOT_USE
#        define uset_spanBackUTF8 uset_spanBackUTF8_DRAFT_API_DO_NOT_USE
#        define uset_spanUTF8 uset_spanUTF8_DRAFT_API_DO_NOT_USE
#    else
#        define u_fclose_3_8 u_fclose_DRAFT_API_DO_NOT_USE
#        define u_feof_3_8 u_feof_DRAFT_API_DO_NOT_USE
#        define u_fflush_3_8 u_fflush_DRAFT_API_DO_NOT_USE
#        define u_fgetConverter_3_8 u_fgetConverter_DRAFT_API_DO_NOT_USE
#        define u_fgetc_3_8 u_fgetc_DRAFT_API_DO_NOT_USE
#        define u_fgetcodepage_3_8 u_fgetcodepage_DRAFT_API_DO_NOT_USE
#        define u_fgetcx_3_8 u_fgetcx_DRAFT_API_DO_NOT_USE
#        define u_fgetfile_3_8 u_fgetfile_DRAFT_API_DO_NOT_USE
#        define u_fgetlocale_3_8 u_fgetlocale_DRAFT_API_DO_NOT_USE
#        define u_fgets_3_8 u_fgets_DRAFT_API_DO_NOT_USE
#        define u_file_read_3_8 u_file_read_DRAFT_API_DO_NOT_USE
#        define u_file_write_3_8 u_file_write_DRAFT_API_DO_NOT_USE
#        define u_finit_3_8 u_finit_DRAFT_API_DO_NOT_USE
#        define u_fopen_3_8 u_fopen_DRAFT_API_DO_NOT_USE
#        define u_fprintf_3_8 u_fprintf_DRAFT_API_DO_NOT_USE
#        define u_fprintf_u_3_8 u_fprintf_u_DRAFT_API_DO_NOT_USE
#        define u_fputc_3_8 u_fputc_DRAFT_API_DO_NOT_USE
#        define u_fputs_3_8 u_fputs_DRAFT_API_DO_NOT_USE
#        define u_frewind_3_8 u_frewind_DRAFT_API_DO_NOT_USE
#        define u_fscanf_3_8 u_fscanf_DRAFT_API_DO_NOT_USE
#        define u_fscanf_u_3_8 u_fscanf_u_DRAFT_API_DO_NOT_USE
#        define u_fsetcodepage_3_8 u_fsetcodepage_DRAFT_API_DO_NOT_USE
#        define u_fsetlocale_3_8 u_fsetlocale_DRAFT_API_DO_NOT_USE
#        define u_fsettransliterator_3_8 u_fsettransliterator_DRAFT_API_DO_NOT_USE
#        define u_fstropen_3_8 u_fstropen_DRAFT_API_DO_NOT_USE
#        define u_fungetc_3_8 u_fungetc_DRAFT_API_DO_NOT_USE
#        define u_snprintf_3_8 u_snprintf_DRAFT_API_DO_NOT_USE
#        define u_snprintf_u_3_8 u_snprintf_u_DRAFT_API_DO_NOT_USE
#        define u_sprintf_3_8 u_sprintf_DRAFT_API_DO_NOT_USE
#        define u_sprintf_u_3_8 u_sprintf_u_DRAFT_API_DO_NOT_USE
#        define u_sscanf_3_8 u_sscanf_DRAFT_API_DO_NOT_USE
#        define u_sscanf_u_3_8 u_sscanf_u_DRAFT_API_DO_NOT_USE
#        define u_vfprintf_3_8 u_vfprintf_DRAFT_API_DO_NOT_USE
#        define u_vfprintf_u_3_8 u_vfprintf_u_DRAFT_API_DO_NOT_USE
#        define u_vfscanf_3_8 u_vfscanf_DRAFT_API_DO_NOT_USE
#        define u_vfscanf_u_3_8 u_vfscanf_u_DRAFT_API_DO_NOT_USE
#        define u_vsnprintf_3_8 u_vsnprintf_DRAFT_API_DO_NOT_USE
#        define u_vsnprintf_u_3_8 u_vsnprintf_u_DRAFT_API_DO_NOT_USE
#        define u_vsprintf_3_8 u_vsprintf_DRAFT_API_DO_NOT_USE
#        define u_vsprintf_u_3_8 u_vsprintf_u_DRAFT_API_DO_NOT_USE
#        define u_vsscanf_3_8 u_vsscanf_DRAFT_API_DO_NOT_USE
#        define u_vsscanf_u_3_8 u_vsscanf_u_DRAFT_API_DO_NOT_USE
#        define ucal_getTZDataVersion_3_8 ucal_getTZDataVersion_DRAFT_API_DO_NOT_USE
#        define ucasemap_getBreakIterator_3_8 ucasemap_getBreakIterator_DRAFT_API_DO_NOT_USE
#        define ucasemap_setBreakIterator_3_8 ucasemap_setBreakIterator_DRAFT_API_DO_NOT_USE
#        define ucasemap_toTitle_3_8 ucasemap_toTitle_DRAFT_API_DO_NOT_USE
#        define ucasemap_utf8FoldCase_3_8 ucasemap_utf8FoldCase_DRAFT_API_DO_NOT_USE
#        define ucasemap_utf8ToTitle_3_8 ucasemap_utf8ToTitle_DRAFT_API_DO_NOT_USE
#        define udatpg_addPattern_3_8 udatpg_addPattern_DRAFT_API_DO_NOT_USE
#        define udatpg_clone_3_8 udatpg_clone_DRAFT_API_DO_NOT_USE
#        define udatpg_close_3_8 udatpg_close_DRAFT_API_DO_NOT_USE
#        define udatpg_getAppendItemFormat_3_8 udatpg_getAppendItemFormat_DRAFT_API_DO_NOT_USE
#        define udatpg_getAppendItemName_3_8 udatpg_getAppendItemName_DRAFT_API_DO_NOT_USE
#        define udatpg_getBaseSkeleton_3_8 udatpg_getBaseSkeleton_DRAFT_API_DO_NOT_USE
#        define udatpg_getBestPattern_3_8 udatpg_getBestPattern_DRAFT_API_DO_NOT_USE
#        define udatpg_getDateTimeFormat_3_8 udatpg_getDateTimeFormat_DRAFT_API_DO_NOT_USE
#        define udatpg_getDecimal_3_8 udatpg_getDecimal_DRAFT_API_DO_NOT_USE
#        define udatpg_getPatternForSkeleton_3_8 udatpg_getPatternForSkeleton_DRAFT_API_DO_NOT_USE
#        define udatpg_getSkeleton_3_8 udatpg_getSkeleton_DRAFT_API_DO_NOT_USE
#        define udatpg_openBaseSkeletons_3_8 udatpg_openBaseSkeletons_DRAFT_API_DO_NOT_USE
#        define udatpg_openEmpty_3_8 udatpg_openEmpty_DRAFT_API_DO_NOT_USE
#        define udatpg_openSkeletons_3_8 udatpg_openSkeletons_DRAFT_API_DO_NOT_USE
#        define udatpg_open_3_8 udatpg_open_DRAFT_API_DO_NOT_USE
#        define udatpg_replaceFieldTypes_3_8 udatpg_replaceFieldTypes_DRAFT_API_DO_NOT_USE
#        define udatpg_setAppendItemFormat_3_8 udatpg_setAppendItemFormat_DRAFT_API_DO_NOT_USE
#        define udatpg_setAppendItemName_3_8 udatpg_setAppendItemName_DRAFT_API_DO_NOT_USE
#        define udatpg_setDateTimeFormat_3_8 udatpg_setDateTimeFormat_DRAFT_API_DO_NOT_USE
#        define udatpg_setDecimal_3_8 udatpg_setDecimal_DRAFT_API_DO_NOT_USE
#        define uloc_getLocaleForLCID_3_8 uloc_getLocaleForLCID_DRAFT_API_DO_NOT_USE
#        define uset_cloneAsThawed_3_8 uset_cloneAsThawed_DRAFT_API_DO_NOT_USE
#        define uset_clone_3_8 uset_clone_DRAFT_API_DO_NOT_USE
#        define uset_freeze_3_8 uset_freeze_DRAFT_API_DO_NOT_USE
#        define uset_isFrozen_3_8 uset_isFrozen_DRAFT_API_DO_NOT_USE
#        define uset_spanBackUTF8_3_8 uset_spanBackUTF8_DRAFT_API_DO_NOT_USE
#        define uset_spanBack_3_8 uset_spanBack_DRAFT_API_DO_NOT_USE
#        define uset_spanUTF8_3_8 uset_spanUTF8_DRAFT_API_DO_NOT_USE
#        define uset_span_3_8 uset_span_DRAFT_API_DO_NOT_USE
#    endif /* U_DISABLE_RENAMING */

#endif /* U_HIDE_DRAFT_API */
#endif /* UDRAFT_H */

