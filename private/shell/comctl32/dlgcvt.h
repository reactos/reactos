#ifndef _INC_COMCTL32_DLGCVT_H
#define _INC_COMCTL32_DLGCVT_H



#ifndef _INC_WINDOWSX
#   include <windowsx.h>
#endif
#ifndef __CCSTOCK_H__
#   include <ccstock.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif 

HRESULT CvtDlgToDlgEx(LPDLGTEMPLATE pTemplate, LPDLGTEMPLATEEX *ppTemplateExOut, int iCharset);

#ifdef __cplusplus
} // extern "C"
#endif



#endif // _INC_COMCTL32_DLGCVT_H
