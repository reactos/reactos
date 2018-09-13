#ifndef __IMAGEPROP_H__
#define __IMAGEPROP_H__

#define MAX_METADATA_TEXT   256

#ifdef _X86_
#include "imageflt\\image.h"
//-------------------------------------------------------------------------//
typedef struct tagIFLPROPERTIES
{
    ULONG                   cbStruct ;      // structure size.
    ULONG                   mask ;          // structure member mask.

    IFLTYPE                 type ;          // image type, IFLT_NONE
    IFLCLASS                imageclass ;    // image class, IFLCL_NONE

    LONG                    cx ;            // 0L
    LONG                    cy ;            // 0L
    LONG                    linecount;      // raster line count, 0L
    LONG                    bpp ;           // bits per pixel, 0L
    LONG                    bpc ;           // bits per channel,.0L
    LONG                    dpmX ;          // horizontal dots per meter, 0L
    LONG                    dpmY ;          // vertical dots per meter, 0L
    LONG                    gamma ;         // Gamma correction
    LONG                    imagecount ;    // # of contained images, 0L
    LONG                    Reserved ;

    IFLTILEFORMAT           tilefmt ;       // IFLTF_NONE
    IFLSEQUENCE             lineseq ;       // IFLSEQ_TOPDOWN ;
    IFLCOMPRESSION          compression ;   // IFLCOMP_NONE ;
    SYSTEMTIME              datetime ;      //  (zeroed).
    IFLBMPVERSION           bmpver ;        // 0L ;

    ULONG                   textmask ;
    WCHAR                   szTitle[MAX_METADATA_TEXT] ;        // SI::PIDSI_TITLE
    WCHAR                   szAuthor[MAX_METADATA_TEXT] ;       // SI::PIDSI_AUTHOR
    WCHAR                   szDescription[MAX_METADATA_TEXT] ;
    WCHAR                   szComments[MAX_METADATA_TEXT] ;     // SI::PIDSI_COMMENTS
    WCHAR                   szSoftware[MAX_METADATA_TEXT] ;     // SI::PIDSI_APPNAME
    WCHAR                   szSource[MAX_METADATA_TEXT] ;       // MSI::PIDMSI_SOURCE
    WCHAR                   szCopyright[MAX_METADATA_TEXT] ;    // 
    WCHAR                   szDisclaimer[MAX_METADATA_TEXT] ;
    WCHAR                   szWarning[MAX_METADATA_TEXT] ;

    ULONG                   faxmask ;       // fax property bits
    FILETIME                ftFaxTime ;
    WCHAR                   szFaxRecipName[MAX_METADATA_TEXT] ;
    WCHAR                   szFaxRecipNumber[MAX_METADATA_TEXT] ;
    WCHAR                   szFaxSenderName[MAX_METADATA_TEXT] ;
    WCHAR                   szFaxRouting[MAX_METADATA_TEXT] ;
    WCHAR                   szFaxCallerID[MAX_METADATA_TEXT] ;
    WCHAR                   szFaxTSID[MAX_METADATA_TEXT] ;
    WCHAR                   szFaxCSID[MAX_METADATA_TEXT] ;

} IFLPROPERTIES, *PIFLPROPERTIES, *LPIFLPROPERTIES ;

//  Image property mask bits
#define IPF_TYPE            0x00000001
#define IPF_CLASS           0x00000002
#define IPF_CX              0x00000004
#define IPF_CY              0x00000008
#define IPF_BPP             0x00000010
#define IPF_BPC             0x00000020
#define IPF_LINECOUNT       0x00000040
#define IPF_DPMX            0x00000080
#define IPF_DPMY            0x00000100
#define IPF_GAMMA           0x00000200
#define IPF_IMAGECOUNT      0x00000400
#define IPF_TILEFMT         0x00000800
#define IPF_LINESEQ         0x00001000
#define IPF_COMPRESSION     0x00002000
#define IPF_DATETIME        0x00004000
#define IPF_BMPVER          0x00008000
#define IPF_FAX             0x00010000
#define IPF_TEXT            0x00020000

#define IPF_IMAGE   (IPF_TYPE|IPF_CLASS|IPF_CX|IPF_CY|IPF_BPP|IPF_BPC| \
                     IPF_LINECOUNT|IPF_DPMX|IPF_DPMY|IPF_GAMMA| \
                     IPF_IMAGECOUNT|IPF_TILEFMT|IPF_LINESEQ| \
                     IPF_COMPRESSION|IPF_DATETIME|IPF_BMPVER)

//  Fax property mask bits
#define IFPF_FAXTIME          0x0001
#define IFPF_RECIPIENTNAME    0x0002
#define IFPF_RECIPIENTNUMBER  0x0004
#define IFPF_SENDERNAME       0x0008
#define IFPF_CALLERID         0x0010
#define IFPF_TSID             0x0020
#define IFPF_CSID             0x0040
#define IFPF_ROUTING          0x0080

//  Text property mask bits
#define ITPF_TITLE            0x0001
#define ITPF_AUTHOR           0x0002
#define ITPF_DESCRIPTION      0x0004
#define ITPF_COMMENTS         0x0008
#define ITPF_SOFTWARE         0x0010
#define ITPF_COPYRIGHT        0x0020

//-------------------------------------------------------------------------//
EXTERN_C BOOL    InitIflProperties( IFLPROPERTIES* pIP ) ;
EXTERN_C BOOL    ClearIflProperties( IFLPROPERTIES* pIP ) ;
EXTERN_C HRESULT AcquireImageProperties( const WCHAR* pwszFileName, IFLPROPERTIES* pIP, ULONG* pcProps ) ;
EXTERN_C BOOL    HasImageProperties( IFLPROPERTIES* pIP ) ;
EXTERN_C BOOL    HasFaxProperties( IFLPROPERTIES* pIP ) ;
EXTERN_C HRESULT IflErrorToHResult( IFLERROR iflError, BOOL bExtended ) ;
//-------------------------------------------------------------------------//

#endif _X86_

//  FaxSummmaryInformation property set defininition
extern const GUID FMTID_FaxSummaryInformation ;


#endif __IMAGEPROP_H__