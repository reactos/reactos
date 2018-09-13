//-------------------------------------------------------------------------//
//  defprop.cpp
//-------------------------------------------------------------------------//
#include "pch.h"
#include "resource.h" //  resource symbols
#include "defprop.h"
#include "propvar.h"
#include "dictbase.h"
#include "imageprop.h"

//-------------------------------------------------------------------------//
static HRESULT LoadItemTags( UINT, BSTR&, BSTR&, BSTR& );

//-------------------------------------------------------------------------//
BEGIN_DEFFOLDER_MAP( def_folder_items )
    
    DEFFOLDER_ENTRY( PFID_FaxProperties, IDS_FOLDER_FAX, \
                     PST_IMAGEFILE, PTFI_ALL, PTIA_READONLY )

    DEFFOLDER_ENTRY( PFID_ImageProperties, IDS_FOLDER_IMAGE, \
                     PST_IMAGEFILE, PTFI_ALL, PTIA_READONLY )

    DEFFOLDER_ENTRY( PFID_Description, IDS_FOLDER_DESCRIPTION, \
                     PST_NOSTG|PST_OLESS|PST_NSS|PST_DOCFILE|PST_IMAGEFILE|PST_MEDIAFILE|PST_UNKNOWNDOC, \
                     PTFI_ALL, PTIA_READWRITE )
    
    DEFFOLDER_ENTRY( PFID_Origin,  IDS_FOLDER_SOURCE,  \
                     PST_OLESS|PST_NSS|PST_DOCFILE|PST_IMAGEFILE|PST_MEDIAFILE|PST_UNKNOWNDOC, PTFI_ALL, PTIA_READWRITE )
    
END_DEFFOLDER_MAP()

//-------------------------------------------------------------------------//
#pragma warning( disable : 310 ) // cast truncates constant value
BEGIN_DEFVALLIST( BoolTrueFalse )
    DEFVALLIST_BOOL_ENTRY( VARIANT_FALSE, TRUE, IDS_BOOLVAL_FALSE )
    DEFVALLIST_BOOL_ENTRY( VARIANT_TRUE,  TRUE, IDS_BOOLVAL_TRUE )
END_DEFVALLIST()

BEGIN_DEFVALLIST( BoolYesNo )
    DEFVALLIST_BOOL_ENTRY( VARIANT_TRUE,  TRUE, IDS_BOOLVAL_NO )
    DEFVALLIST_BOOL_ENTRY( VARIANT_FALSE, TRUE, IDS_BOOLVAL_YES )
END_DEFVALLIST()

//  Do not change the order of any of the existing values in the following table!!!; append new values.
BEGIN_DEFVALLIST( MediaStatusVals )
    DEFVALLIST_STRING_ENTRY( (ULONG)PIDMSI_STATUS_NORMAL, VT_UI4, FALSE, TRUE, IDS_STATUSVAL_NORMAL )
    DEFVALLIST_STRING_ENTRY( (ULONG)PIDMSI_STATUS_NEW,    VT_UI4, FALSE, TRUE, IDS_STATUSVAL_NEW )
    DEFVALLIST_STRING_ENTRY( (ULONG)PIDMSI_STATUS_PRELIM, VT_UI4, FALSE, TRUE, IDS_STATUSVAL_PRELIM )
    DEFVALLIST_STRING_ENTRY( (ULONG)PIDMSI_STATUS_DRAFT,  VT_UI4, FALSE, TRUE, IDS_STATUSVAL_DRAFT )
    DEFVALLIST_STRING_ENTRY( (ULONG)PIDMSI_STATUS_INPROGRESS, VT_UI4, FALSE, TRUE, IDS_STATUSVAL_INPROGRESS )
    DEFVALLIST_STRING_ENTRY( (ULONG)PIDMSI_STATUS_EDIT,   VT_UI4, FALSE, TRUE, IDS_STATUSVAL_EDIT )
    DEFVALLIST_STRING_ENTRY( (ULONG)PIDMSI_STATUS_REVIEW, VT_UI4, FALSE, TRUE, IDS_STATUSVAL_REVIEW )
    DEFVALLIST_STRING_ENTRY( (ULONG)PIDMSI_STATUS_PROOF,  VT_UI4, FALSE, TRUE, IDS_STATUSVAL_PROOF )
    DEFVALLIST_STRING_ENTRY( (ULONG)PIDMSI_STATUS_FINAL,  VT_UI4, FALSE, TRUE, IDS_STATUSVAL_FINAL )
    DEFVALLIST_STRING_ENTRY( (ULONG)PIDMSI_STATUS_OTHER,  VT_UI4, FALSE, TRUE, IDS_STATUSVAL_OTHER )
END_DEFVALLIST()

#pragma warning( default : 310 ) // cast truncates constant value


//-------------------------------------------------------------------------//
BEGIN_DEFPROP_MAP( def_property_items )

    //  Properties in the 'General' folder
    DEFPROP_ENTRY( L"Title", FMTID_SummaryInformation, PIDSI_TITLE, IDS_PIDSI_TITLE, VT_LPSTR, 
        PST_OLESS|PST_DOCFILE | PST_NSS|PST_IMAGEFILE|PST_MEDIAFILE|PST_UNKNOWNDOC | PST_SHELLCOLUMN,
        PFID_Description, PTPI_ALL, PTIA_READWRITE,
        PTPIF_MRU, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"Subject", FMTID_SummaryInformation, PIDSI_SUBJECT, IDS_PIDSI_SUBJECT, VT_LPSTR,
        PST_OLESS|PST_DOCFILE | PST_NSS|PST_IMAGEFILE|PST_MEDIAFILE|PST_UNKNOWNDOC | PST_SHELLCOLUMN,
        PFID_Description, PTPI_ALL, PTIA_READWRITE,
        PTPIF_MRU, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"Category", FMTID_DocSummaryInformation, PIDDSI_CATEGORY, IDS_PIDDSI_CATEGORY, VT_LPSTR, 
        PST_OLESS|PST_DOCFILE | PST_NSS|PST_IMAGEFILE|PST_MEDIAFILE|PST_UNKNOWNDOC | PST_SHELLCOLUMN,
        PFID_Description, PTPI_ALL, PTIA_READWRITE,
        PTPIF_MRU, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"Keywords", FMTID_SummaryInformation, PIDSI_KEYWORDS, IDS_PIDSI_KEYWORDS, VT_LPSTR,
        PST_OLESS|PST_DOCFILE | PST_NSS|PST_IMAGEFILE|PST_MEDIAFILE|PST_UNKNOWNDOC,
        PFID_Description, PTPI_ALL, PTIA_READWRITE,
        PTPIF_BASIC, 
        PTCTLID_MULTILINE_EDIT, NULL )

    DEFPROP_ENTRY( L"Rating", FMTID_MediaFileSummaryInformation, PIDMSI_RATING, IDS_PIDMSI_RATING, VT_LPWSTR,
        PST_NSS|PST_IMAGEFILE|PST_MEDIAFILE,
        PFID_Description, PTPI_ALL, PTIA_READWRITE,
        PTPIF_MRU, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"Template", FMTID_SummaryInformation, PIDSI_TEMPLATE, IDS_PIDSI_TEMPLATE, VT_LPSTR,
        PST_OLESS|PST_DOCFILE,
        PFID_Description, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"PageCount", FMTID_SummaryInformation, PIDSI_PAGECOUNT, IDS_PIDSI_PAGECOUNT, VT_I4,
        PST_OLESS|PST_DOCFILE,
        PFID_Description, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"WordCount", FMTID_SummaryInformation, PIDSI_WORDCOUNT, IDS_PIDSI_WORDCOUNT, VT_I4,
        PST_OLESS|PST_DOCFILE,
        PFID_Description, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"CharCount", FMTID_SummaryInformation, PIDSI_CHARCOUNT, IDS_PIDSI_CHARCOUNT, VT_I4,
        PST_OLESS|PST_DOCFILE,
        PFID_Description, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"ByteCount", FMTID_DocSummaryInformation, PIDDSI_BYTECOUNT, IDS_PIDDSI_BYTECOUNT, VT_I4,
        PST_OLESS|PST_DOCFILE,
        PFID_Description, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"LineCount", FMTID_DocSummaryInformation, PIDDSI_LINECOUNT, IDS_PIDDSI_LINECOUNT, VT_I4,
        PST_OLESS|PST_DOCFILE,
        PFID_Description, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"ParCount", FMTID_DocSummaryInformation, PIDDSI_PARCOUNT, IDS_PIDDSI_PARCOUNT, VT_I4,
        PST_OLESS|PST_DOCFILE,
        PFID_Description, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"SlideCount", FMTID_DocSummaryInformation, PIDDSI_SLIDECOUNT, IDS_PIDDSI_SLIDECOUNT, VT_I4,
        PST_OLESS|PST_DOCFILE,
        PFID_Description, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"NoteCount", FMTID_DocSummaryInformation, PIDDSI_NOTECOUNT, IDS_PIDDSI_NOTECOUNT, VT_I4,
        PST_OLESS|PST_DOCFILE,
        PFID_Description, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"HiddenCount", FMTID_DocSummaryInformation, PIDDSI_HIDDENCOUNT, IDS_PIDDSI_HIDDENCOUNT, VT_I4,
        PST_OLESS|PST_DOCFILE,
        PFID_Description, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"MMClipCount", FMTID_DocSummaryInformation, PIDDSI_MMCLIPCOUNT, IDS_PIDDSI_MMCLIPCOUNT, VT_I4,
        PST_OLESS|PST_DOCFILE,
        PFID_Description, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENUM_ENTRY( L"Scale", FMTID_DocSummaryInformation, PIDDSI_SCALE, IDS_PIDDSI_SCALE, VT_BOOL,
        PST_OLESS|PST_DOCFILE,
        PFID_Description, PTPI_ALL, PTIA_READONLY,
        PTPIF_ENUM|PTPIF_INNATE, 
        PTCTLID_DROPLIST_COMBO, NULL,
        DEFVALLIST_COUNT( BoolYesNo ), BoolYesNo )

#if 0   // Can't deal with these vector types.
    DEFPROP_ENTRY( L"HeadingPair", FMTID_DocSummaryInformation, PIDDSI_HEADINGPAIR, IDS_PIDDSI_HEADINGPAIR, VT_VARIANT|VT_VECTOR,
        PST_OLESS|PST_DOCFILE,
        PFID_Description, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"DocParts", FMTID_DocSummaryInformation, PIDDSI_DOCPARTS, IDS_PIDDSI_DOCPARTS, VT_LPSTR|VT_VECTOR,
        PST_OLESS|PST_DOCFILE,
        PFID_Description, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )
#endif

    DEFPROP_ENTRY( L"LinksUpToDate", FMTID_DocSummaryInformation, PIDDSI_LINKSDIRTY, IDS_PIDDSI_LINKSDIRTY, VT_BOOL,
        PST_OLESS|PST_DOCFILE,
        PFID_Description, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPLIST_COMBO, NULL )

    DEFPROP_ENTRY( L"Comments", FMTID_SummaryInformation, PIDSI_COMMENTS, IDS_PIDSI_COMMENTS, VT_LPSTR,
        PST_OLESS|PST_DOCFILE | PST_NSS|PST_IMAGEFILE|PST_MEDIAFILE|PST_UNKNOWNDOC,
        PFID_Description, PTPI_ALL, PTIA_READWRITE,
        PTPIF_BASIC, 
        PTCTLID_MULTILINE_EDIT, NULL )

    DEFPROP_ENTRY( L"FileType", FMTID_ImageSummaryInformation, PIDISI_FILETYPE, IDS_PIDISG_FILETYPE, VT_LPWSTR,
        PST_NOSTG|PST_NSS|PST_IMAGEFILE,
        PFID_ImageProperties, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"Width", FMTID_ImageSummaryInformation, PIDISI_CX, IDS_PIDISG_CX, VT_UI4,
        PST_NOSTG|PST_NSS|PST_IMAGEFILE,
        PFID_ImageProperties, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"Height", FMTID_ImageSummaryInformation, PIDISI_CY, IDS_PIDISG_CY, VT_UI4,
        PST_NOSTG|PST_NSS|PST_IMAGEFILE,
        PFID_ImageProperties, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"ResolutionX", FMTID_ImageSummaryInformation, PIDISI_RESOLUTIONX, IDS_PIDISG_RESOLUTIONX, VT_UI4,
        PST_NOSTG|PST_NSS|PST_IMAGEFILE,
        PFID_ImageProperties, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"ResolutionY", FMTID_ImageSummaryInformation, PIDISI_RESOLUTIONY, IDS_PIDISG_RESOLUTIONY, VT_UI4,
        PST_NOSTG|PST_NSS|PST_IMAGEFILE,
        PFID_ImageProperties, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )
    
    DEFPROP_ENTRY( L"BitDepth", FMTID_ImageSummaryInformation, PIDISI_BITDEPTH, IDS_PIDISG_BITDEPTH, VT_UI4,
        PST_NOSTG|PST_NSS|PST_IMAGEFILE,
        PFID_ImageProperties, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"Colorspace", FMTID_ImageSummaryInformation, PIDISI_COLORSPACE, IDS_PIDISG_COLORSPACE, VT_LPWSTR,
        PST_NOSTG|PST_NSS|PST_IMAGEFILE,
        PFID_ImageProperties, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"Gamma", FMTID_ImageSummaryInformation, PIDISI_GAMMAVALUE, IDS_PIDISG_GAMMAVALUE, VT_UI4,
        PST_NOSTG|PST_NSS|PST_IMAGEFILE,
        PFID_ImageProperties, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"Compression", FMTID_ImageSummaryInformation, PIDISI_COMPRESSION, IDS_PIDISG_COMPRESSION, VT_LPWSTR,
        PST_NOSTG|PST_NSS|PST_IMAGEFILE,
        PFID_ImageProperties, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    //  Properties in the 'Fax' folder
    DEFPROP_ENTRY( L"FaxTime", FMTID_FaxSummaryInformation, PIDFSI_TIME, IDS_PIDFSI_TIME, VT_FILETIME,
        PST_NOSTG|PST_NSS|PST_IMAGEFILE,
        PFID_FaxProperties, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_CALENDARTIME, NULL )

    DEFPROP_ENTRY( L"FaxSenderName", FMTID_FaxSummaryInformation, PIDFSI_SENDERNAME, IDS_PIDFSI_SENDERNAME, VT_LPWSTR,
        PST_NOSTG|PST_NSS|PST_IMAGEFILE,
        PFID_FaxProperties, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"FaxTSID", FMTID_FaxSummaryInformation, PIDFSI_TSID, IDS_PIDFSI_TSID, VT_LPWSTR,
        PST_NOSTG|PST_NSS|PST_IMAGEFILE,
        PFID_FaxProperties, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"FaxCallerID", FMTID_FaxSummaryInformation, PIDFSI_CALLERID, IDS_PIDFSI_CALLERID, VT_LPWSTR,
        PST_NOSTG|PST_NSS|PST_IMAGEFILE,
        PFID_FaxProperties, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"FaxRecipientName", FMTID_FaxSummaryInformation, PIDFSI_RECIPIENTNAME, IDS_PIDFSI_RECIPIENTNAME, VT_LPWSTR,
        PST_NOSTG|PST_NSS|PST_IMAGEFILE,
        PFID_FaxProperties, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"FaxRecipientNumber", FMTID_FaxSummaryInformation, PIDFSI_RECIPIENTNUMBER, IDS_PIDFSI_RECIPIENTNUMBER, VT_LPWSTR,
        PST_NOSTG|PST_NSS|PST_IMAGEFILE,
        PFID_FaxProperties, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"FaxCSID", FMTID_FaxSummaryInformation, PIDFSI_CSID, IDS_PIDFSI_CSID, VT_LPWSTR,
        PST_NOSTG|PST_NSS|PST_IMAGEFILE,
        PFID_FaxProperties, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"FaxRouting", FMTID_FaxSummaryInformation, PIDFSI_ROUTING, IDS_PIDFSI_ROUTING, VT_LPWSTR,
        PST_NOSTG|PST_NSS|PST_IMAGEFILE,
        PFID_FaxProperties, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    //  Properties in the 'Source' folder
    DEFPROP_ENTRY( L"SequenceNo", FMTID_MediaFileSummaryInformation, PIDMSI_SEQUENCE_NO, IDS_PIDMSI_SEQUENCE_NO, VT_LPWSTR,
        PST_NSS|PST_IMAGEFILE|PST_MEDIAFILE,
        PFID_Origin, PTPI_ALL, PTIA_READWRITE,
        PTPIF_MRU, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"Owner", FMTID_MediaFileSummaryInformation, PIDMSI_OWNER, IDS_PIDMSI_OWNER, VT_LPWSTR,
        PST_NSS|PST_IMAGEFILE|PST_MEDIAFILE,
        PFID_Origin, PTPI_ALL, PTIA_READWRITE,
        PTPIF_MRU, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"Editor", FMTID_MediaFileSummaryInformation, PIDMSI_EDITOR, IDS_PIDMSI_EDITOR, VT_LPWSTR,
        PST_NSS|PST_IMAGEFILE|PST_MEDIAFILE,
        PFID_Origin, PTPI_ALL, PTIA_READWRITE,
        PTPIF_MRU, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"Supplier", FMTID_MediaFileSummaryInformation, PIDMSI_SUPPLIER, IDS_PIDMSI_SUPPLIER, VT_LPWSTR,
        PST_NSS|PST_IMAGEFILE|PST_MEDIAFILE,
        PFID_Origin, PTPI_ALL, PTIA_READWRITE,
        PTPIF_MRU, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"Source", FMTID_MediaFileSummaryInformation, PIDMSI_SOURCE, IDS_PIDMSI_SOURCE, VT_LPWSTR,
        PST_NSS|PST_IMAGEFILE|PST_MEDIAFILE|PST_UNKNOWNDOC,
        PFID_Origin, PTPI_ALL, PTIA_READWRITE,
        PTPIF_MRU, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"Copyright", FMTID_MediaFileSummaryInformation, PIDMSI_COPYRIGHT, IDS_PIDMSI_COPYRIGHT, VT_LPSTR,
        PST_NSS | PST_IMAGEFILE|PST_MEDIAFILE | PST_SHELLCOLUMN,
        PFID_Origin, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"Project", FMTID_MediaFileSummaryInformation, PIDMSI_PROJECT, IDS_PIDMSI_PROJECT, VT_LPWSTR,
        PST_NSS|PST_IMAGEFILE|PST_MEDIAFILE,
        PFID_Origin, PTPI_ALL, PTIA_READWRITE,
        PTPIF_MRU, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENUM_ENTRY( L"Status", FMTID_MediaFileSummaryInformation, PIDMSI_STATUS, IDS_PIDMSI_STATUS, VT_UI4,
        PST_NSS|PST_IMAGEFILE|PST_MEDIAFILE,
        PFID_Origin, PTPI_ALL, PTIA_READWRITE,
        PTPIF_ENUM, 
        PTCTLID_DROPLIST_COMBO, NULL,
        DEFVALLIST_COUNT( MediaStatusVals ), MediaStatusVals )

    DEFPROP_ENTRY( L"Author", FMTID_SummaryInformation, PIDSI_AUTHOR, IDS_PIDSI_AUTHOR, VT_LPSTR,
        PST_OLESS|PST_DOCFILE | PST_NSS|PST_IMAGEFILE|PST_MEDIAFILE|PST_UNKNOWNDOC | PST_SHELLCOLUMN,
        PFID_Origin, PTPI_ALL, PTIA_READWRITE,
        PTPIF_MRU, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"LastAuthor", FMTID_SummaryInformation, PIDSI_LASTAUTHOR, IDS_PIDSI_LASTAUTHOR, VT_LPSTR,
        PST_OLESS|PST_DOCFILE,
        PFID_Origin, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )
        
    DEFPROP_ENTRY( L"RevNumber", FMTID_SummaryInformation, PIDSI_REVNUMBER, IDS_PIDSI_REVNUMBER, VT_LPSTR,
        PST_OLESS|PST_DOCFILE | PST_NSS|PST_IMAGEFILE|PST_MEDIAFILE|PST_UNKNOWNDOC,
        PFID_Origin, PTPI_ALL, PTIA_READWRITE,
        PTPIF_MRU, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"AppName", FMTID_SummaryInformation, PIDSI_APPNAME,      IDS_PIDSI_APPNAME,        VT_LPSTR,
        PST_OLESS|PST_DOCFILE,
        PFID_Origin, PTPI_ALL, PTIA_READWRITE,
        PTPIF_MRU, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"PresentationTarget", FMTID_DocSummaryInformation, PIDDSI_PRESFORMAT,  IDS_PIDDSI_PRESFORMAT,  VT_LPSTR,
        PST_OLESS|PST_DOCFILE,
        PFID_Origin, PTPI_ALL, PTIA_READWRITE,
        PTPIF_MRU, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"Company", FMTID_DocSummaryInformation, PIDDSI_COMPANY,     IDS_PIDDSI_COMPANY,     VT_LPSTR,
        PST_OLESS|PST_DOCFILE | PST_SHELLCOLUMN,
        PFID_Origin, PTPI_ALL, PTIA_READWRITE,
        PTPIF_MRU, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"Manager", FMTID_DocSummaryInformation, PIDDSI_MANAGER,     IDS_PIDDSI_MANAGER,     VT_LPSTR,
        PST_OLESS|PST_DOCFILE,
        PFID_Origin, PTPI_ALL, PTIA_READWRITE,
        PTPIF_MRU, 
        PTCTLID_DROPDOWN_COMBO, NULL )

    DEFPROP_ENTRY( L"CreateDTM", FMTID_SummaryInformation, PIDSI_CREATE_DTM,   IDS_PIDSI_CREATE_DTM,     VT_FILETIME /*(UTC)*/,
        PST_OLESS|PST_DOCFILE,
        PFID_Origin, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC,
        PTCTLID_CALENDARTIME, NULL )

    DEFPROP_ENTRY( L"Production", FMTID_MediaFileSummaryInformation, PIDMSI_PRODUCTION, IDS_PIDMSI_PRODUCTION_DTM, VT_FILETIME /*(UTC)*/,
        PST_NSS|PST_IMAGEFILE|PST_MEDIAFILE,
        PFID_Origin, PTPI_ALL, PTIA_READWRITE,
        PTPIF_BASIC,
        PTCTLID_CALENDARTIME, NULL )

    DEFPROP_ENTRY( L"LastSaveDTM", FMTID_SummaryInformation, PIDSI_LASTSAVE_DTM, IDS_PIDSI_LASTSAVE_DTM,   VT_FILETIME /*(UTC)*/,
        PST_OLESS|PST_DOCFILE,
        PFID_Origin, PTPI_ALL, PTIA_READWRITE,
        PTPIF_BASIC,
        PTCTLID_CALENDARTIME, NULL )

    DEFPROP_ENTRY( L"LastPrinted", FMTID_SummaryInformation, PIDSI_LASTPRINTED,  IDS_PIDSI_LASTPRINTED,    VT_FILETIME /*(UTC)*/,
        PST_OLESS|PST_DOCFILE ,
        PFID_Origin, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_CALENDARTIME, NULL )

    DEFPROP_ENTRY( L"EditTime", FMTID_SummaryInformation, PIDSI_EDITTIME,     IDS_PIDSI_EDITTIME,       VT_FILETIME /*(UTC)*/,
        PST_OLESS|PST_DOCFILE,
        PFID_Origin, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_TIME, NULL )

#if 0
    DEFPROP_ENTRY( L"Security", FMTID_SummaryInformation, PIDSI_DOC_SECURITY, IDS_PIDSI_DOC_SECURITY,   VT_I4,
        PST_OLESS|PST_DOCFILE,
        PFID_Origin, PTPI_ALL, PTIA_READONLY,
        PTPIF_BASIC|PTPIF_INNATE, 
        PTCTLID_DROPDOWN_COMBO, NULL )
#endif
};

//-------------------------------------------------------------------------//
int DefFolderCount()
{
    return DEFFOLDER_ENTRY_COUNT( def_folder_items );
}
//-------------------------------------------------------------------------//
EXTERN_C HRESULT MakeDefFolderItem( long iDefFolder, PROPFOLDERITEM* pItem, LPARAM lParam )
{
    ASSERT( iDefFolder < DEFFOLDER_ENTRY_COUNT( def_folder_items ) );

    if( !pItem ) return E_POINTER ;
    memset( pItem, 0, sizeof(PROPFOLDERITEM) );

    const DEFFOLDERITEM* pDFI = &def_folder_items[iDefFolder];

    pItem->cbStruct   = sizeof(PROPFOLDERITEM);
    pItem->mask       = pDFI->mask ;
    pItem->dwAccess   = pDFI->dwAccess ;
    pItem->iOrder     = iDefFolder + 0xFFFF ;
    pItem->lParam     = lParam ;
    pItem->pfid       = *pDFI->ppfid ;
    
    LoadItemTags( pDFI->nIDStringRes, 
                  pItem->bstrName, 
                  pItem->bstrDisplayName, 
                  pItem->bstrQtip );
       
    return S_OK ;
}

//-------------------------------------------------------------------------//
BOOL IsDefFolderPFID( long iDefFolder, REFPFID refpfid )
{
    ASSERT( iDefFolder < DEFFOLDER_ENTRY_COUNT( def_folder_items ) );
    return IsEqualGUID( *def_folder_items[iDefFolder].ppfid, refpfid );
}

//-------------------------------------------------------------------------//
BOOL IsDefFolderStgType( long iDefFolder, ULONG dwSrcType )
{
    ASSERT( iDefFolder >= 0 && iDefFolder < DEFFOLDER_ENTRY_COUNT( def_folder_items ) );
    return (GETSTGTYPE( dwSrcType ) & GETSTGTYPE( def_folder_items[iDefFolder].dwSrcType )) !=0 ;
}

//-------------------------------------------------------------------------//
BOOL IsDefFolderDocType( long iDefFolder, ULONG dwSrcType )
{
    ASSERT( iDefFolder >= 0 && iDefFolder < DEFFOLDER_ENTRY_COUNT( def_folder_items ) );
    return (GETDOCTYPE( dwSrcType ) & GETDOCTYPE( def_folder_items[iDefFolder].dwSrcType )) !=0 ;
}

//-------------------------------------------------------------------------//
BOOL IsDefFolderSrcType( long iDefFolder, ULONG dwSrcType )
{
    ASSERT( iDefFolder >= 0 && iDefFolder < DEFFOLDER_ENTRY_COUNT( def_folder_items ) );
    return IsDefFolderStgType( iDefFolder, dwSrcType ) &&
           IsDefFolderDocType( iDefFolder, dwSrcType );
}


//-------------------------------------------------------------------------//
//  Maps property identifier to defprop array index for fast indirection.
class CDefPropMap : public TDictionaryBase< CPropertyUID, int >
//-------------------------------------------------------------------------//
{
public:
    CDefPropMap() : m_fInit( FALSE )    {  /*TRACE( TEXT("CDefPropMap::CDefPropMap()\n") );*/ }
    ~CDefPropMap()                      {  /*TRACE( TEXT("CDefPropMap::~CDefPropMap()\n") );*/ }
    
    BOOL Initialize();

protected:
    virtual ULONG HashKey( const CPropertyUID& key ) const  { 
        //TRACE( TEXT("CDefPropMap::HashKey()\n") );
        //TRACE( TEXT("CDefPropMap->this: 0x%08lX (__vfptr: 0x%08lX) \n"), (ULONG)this, ((ULONG*)this)[0] );
        return key.Hash();
    }

private:
    BOOL m_fInit ;
};

static CDefPropMap  _DefPropMap ; // the one and only.

//-------------------------------------------------------------------------//
//  Stuffs default property map map with associations.
BOOL CDefPropMap::Initialize()
{
    if( !m_fInit )
    {
        TRACE( TEXT("CDefPropMap::Initialize()\n") );

        CPropertyUID puid ;
        int          i = -1 ;

        // Initialize.
        for( i=0; i<DEFPROP_ENTRY_COUNT( def_property_items ); i++ )
        {
            puid.Set( *def_property_items[i].pFmtID,
                      def_property_items[i].propID,
                      def_property_items[i].vt );
            (*this)[puid] = i ;
        }
        
        m_fInit = TRUE ;
    }
    return m_fInit ;
}

//-------------------------------------------------------------------------//
long DefPropCount()
{
    return DEFPROP_ENTRY_COUNT( def_property_items );
}

//-------------------------------------------------------------------------//
BOOL IsDefPropStgType( long iDefProp, ULONG dwSrcType )
{
    ASSERT( iDefProp >= 0 && iDefProp < DEFPROP_ENTRY_COUNT( def_property_items ) );
    ULONG dwStgType = GETSTGTYPE( dwSrcType );
    return (dwStgType & GETSTGTYPE( def_property_items[iDefProp].dwSrcType ))!=0 ;
}

//-------------------------------------------------------------------------//
BOOL IsDefPropDocType( long iDefProp, ULONG dwSrcType )
{
    ASSERT( iDefProp >= 0 && iDefProp < DEFPROP_ENTRY_COUNT( def_property_items ) );
    ULONG dwDocType = GETDOCTYPE( dwSrcType );

    return (dwDocType & GETDOCTYPE( def_property_items[iDefProp].dwSrcType ))!=0 ;
}

//-------------------------------------------------------------------------//
BOOL IsDefPropCxtType( long iDefProp, ULONG dwSrcType )
{
    ASSERT( iDefProp >= 0 && iDefProp < DEFPROP_ENTRY_COUNT( def_property_items ) );
    ULONG dwCxtType = GETCXTTYPE( dwSrcType );
    return (dwCxtType & GETCXTTYPE( def_property_items[iDefProp].dwSrcType ))!=0 ;
}

//-------------------------------------------------------------------------//
BOOL IsDefPropSrcType( long iDefProp, ULONG dwSrcType )
{
    return IsDefPropStgType( iDefProp, dwSrcType ) &&
           IsDefPropDocType( iDefProp, dwSrcType );
}

//-------------------------------------------------------------------------//
BOOL DefPropHasFlags( long iDefProp, ULONG dwFlags )
{
    if( iDefProp<0 || iDefProp>= DEFPROP_ENTRY_COUNT( def_property_items ) )
    {
        ASSERT( FALSE );
        return FALSE ;
    }

    return (def_property_items[iDefProp].dwFlags & dwFlags) != 0 ;
}
 
//-------------------------------------------------------------------------//
long FindDefPropertyItem( IN REFFMTID fmtID, IN PROPID propID, IN VARTYPE vt )
{
    CPropertyUID puid ;
    int          i = -1 ;

    //  If we haven't initialize our dictionary, do it now.
    if( !_DefPropMap.Initialize() )
        return i ;

    //  Do a dictionary lookup:
    puid.Set( fmtID, propID, vt );

    if( !_DefPropMap.Lookup( puid, i ) )
        return -1 ;

    return i ;
}

//-------------------------------------------------------------------------//
long FindDefPropertyItemLite( IN REFFMTID fmtID, IN PROPID propID )
{
    for( int i = 0; i < DefPropCount(); i++ )
    {
        if( propID == def_property_items[i].propID &&
            IsEqualGUID( fmtID, *def_property_items[i].pFmtID ) )
            return i ;
    }
    return -1 ;
}

//-------------------------------------------------------------------------//
HRESULT GetDefPropertyItem( 
    IN long iDefProp, 
    OUT DEFPROPERTYITEM* pDefProp )
{
    if( iDefProp<0 || iDefProp>= DEFPROP_ENTRY_COUNT( def_property_items ) )
        return E_INVALIDARG ;

    if( !pDefProp )
        return E_POINTER ;

    *pDefProp = def_property_items[ iDefProp ];
    return S_OK ;
}

//-------------------------------------------------------------------------//
EXTERN_C HRESULT MakeDefPropertyItem(
    long iDefProp,
    PROPERTYITEM* pItem,
    IN LPARAM lParam )
{
    if( iDefProp<0 || iDefProp>= DEFPROP_ENTRY_COUNT( def_property_items ) )
        return E_INVALIDARG ;

    pItem->cbStruct     = sizeof(*pItem);
    pItem->mask         = def_property_items[iDefProp].mask ;
    pItem->dwAccess     = def_property_items[iDefProp].dwAccess ;
    pItem->iOrder       = iDefProp ;
    pItem->puid.fmtid   = *def_property_items[iDefProp].pFmtID ;
    pItem->puid.propid  = def_property_items[iDefProp].propID ;
    pItem->puid.vt      = 
    pItem->val.vt       = def_property_items[iDefProp].vt ;
    pItem->pfid         = *def_property_items[iDefProp].ppfid ;
    pItem->dwFlags      = def_property_items[iDefProp].dwFlags ;
    pItem->ctlID        = def_property_items[iDefProp].ctlID ;
    pItem->lParam       = lParam ;
    pItem->val.vt       = pItem->puid.vt ;

    LoadItemTags( def_property_items[iDefProp].nIDStringRes,
                  pItem->bstrName, 
                  pItem->bstrDisplayName, 
                  pItem->bstrQtip );

    return S_OK ;
}

//-------------------------------------------------------------------------//
EXTERN_C HRESULT MakeDefPropertyItemEx( IN REFFMTID fmtID, IN PROPID propID, IN VARTYPE vt, OUT PROPERTYITEM* pItem, IN LPARAM lParam )
{
    long iDefProp ;

    if( (iDefProp = FindDefPropertyItem( fmtID, propID, vt )) >= 0 )
        return MakeDefPropertyItem( iDefProp, pItem, lParam );

    return E_ABORT ;
}

//-------------------------------------------------------------------------//
HRESULT GetDefPropItemID( IN long iDefProp, OUT const FMTID** pFmtID, OUT PROPID* pPropID, OUT VARTYPE* pVt )
{
    if( !VALID_DEFPROP_ENTRY( iDefProp, def_property_items ) )
        return E_INVALIDARG ;

    if( pFmtID )
        *pFmtID = def_property_items[iDefProp].pFmtID ;
    if( pPropID )
        *pPropID = def_property_items[iDefProp].propID ;
    if( pVt )
        *pVt = def_property_items[iDefProp].vt ;
    return S_OK ;
}


//-------------------------------------------------------------------------//
//  Helper function to load and parse folder and property item display and Qtip tags.
HRESULT LoadItemTags( UINT nIDS, BSTR& bstrName, BSTR& bstrDisplayName, BSTR& bstrQtip )
{
    TCHAR   szBuf[MAX_STRINGRES+1];
    HRESULT hr = E_FAIL ;

    if( LoadString( _Module.GetResourceInstance(), nIDS,
                    szBuf, MAX_STRINGRES )>0 )
    {
        PSTRTOK pTokenList = NULL, pTok ;
        USES_CONVERSION ;

        VERIFY( GetTokens( szBuf, TEXT("|"), &pTokenList ) > 0 );
        pTok = pTokenList ;
        
        //  Programmatic name
        if( pTok != NULL && pTok->cchTok > 0 )
        {
            bstrName = SysAllocString( T2W( (LPTSTR)pTok->pszTok ) );
            pTok = pTok->pNext ;
        }
        //  Display name
        if( pTok != NULL && pTok->cchTok > 0 )
        {
            bstrDisplayName = SysAllocString( T2W( (LPTSTR)pTok->pszTok ) );
            pTok = pTok->pNext ;
        }
        //  Quick tip text
        if( pTok != NULL && pTok->cchTok > 0 )
        {
            bstrQtip = SysAllocString( T2W( (LPTSTR)pTok->pszTok ) );
            pTok = pTok->pNext ;
        }

        FreeTokens( &pTokenList );
        hr = S_OK ;
    }

    return hr ;
}

//-------------------------------------------------------------------------//
HRESULT GetDefPropItemValues( IN long iDefProp, OUT DEFVAL const** paDefVals, OUT ULONG* pcDefVals )
{
    if( !VALID_DEFPROP_ENTRY( iDefProp, def_property_items ) )
        return E_INVALIDARG ;

    if( paDefVals ) *paDefVals = def_property_items[iDefProp].pDefVals ;
    if( pcDefVals ) *pcDefVals = def_property_items[iDefProp].cDefVals ;
    return S_OK ;
}
