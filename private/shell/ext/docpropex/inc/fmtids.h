//-------------------------------------------------------------------------//
//
//  FMTIDs.h
//
//-------------------------------------------------------------------------//

#ifndef __FMTIDS_H__
#define __FMTIDS_H__

//-------------------------------------------------------------------------//
EXTERN_C const FMTID FMTID_ImageSummaryInformation,
                     FMTID_AudioSummaryInformation,
                     FMTID_VideoSummaryInformation,
                     FMTID_MediaFileSummaryInformation ;
//-------------------------------------------------------------------------//

#ifndef  PIDISI_FILETYPE

//  FMTID_ImageSummaryInformation - Property IDs
#define PIDISI_FILETYPE                 0x00000002L  // VT_LPWSTR 
#define PIDISI_CX                       0x00000003L  // VT_UI4 
#define PIDISI_CY                       0x00000004L  // VT_UI4 
#define PIDISI_RESOLUTIONX              0x00000005L  // VT_UI4
#define PIDISI_RESOLUTIONY              0x00000006L  // VT_UI4
#define PIDISI_BITDEPTH                 0x00000007L  // VT_UI4
#define PIDISI_COLORSPACE               0x00000008L  // VT_LPWSTR 
#define PIDISI_COMPRESSION              0x00000009L  // VT_LPWSTR 
#define PIDISI_TRANSPARENCY             0x0000000AL  // VT_UI4 
#define PIDISI_GAMMAVALUE               0x0000000BL  // VT_UI4 

//  FMTID_MediaFileSummaryInformation - Property IDs
#define PIDMSI_EDITOR                   0x00000002L  // VT_LPWSTR
#define PIDMSI_SUPPLIER                 0x00000003L  // VT_LPWSTR
#define PIDMSI_SOURCE                   0x00000004L  // VT_LPWSTR
#define PIDMSI_SEQUENCE_NO              0x00000005L  // VT_LPWSTR
#define PIDMSI_PROJECT                  0x00000006L  // VT_LPWSTR
#define PIDMSI_STATUS                   0x00000007L  // VT_UI4
#define PIDMSI_OWNER                    0x00000008L  // VT_LPWSTR
#define PIDMSI_RATING                   0x00000009L  // VT_LPWSTR
#define PIDMSI_PRODUCTION_DTM           0x0000000AL  // VT_LPWSTR
#define PIDMSI_COPYRIGHT                0x0000000BL  // VT_LPWSTR

//  PIDMSI_STATUS value set definition
enum PIDMSI_STATUS_VALUE
{
    PIDMSI_STATUS_NORMAL  = 0,
    PIDMSI_STATUS_NEW,
    PIDMSI_STATUS_PRELIM,
    PIDMSI_STATUS_DRAFT,
    PIDMSI_STATUS_INPROGRESS,
    PIDMSI_STATUS_EDIT,
    PIDMSI_STATUS_REVIEW,
    PIDMSI_STATUS_PROOF,
    PIDMSI_STATUS_FINAL,
    PIDMSI_STATUS_OTHER   = 0x7FFF
} ;

#endif // PIDISI_FILETYPE
#endif __FMTIDS_H__