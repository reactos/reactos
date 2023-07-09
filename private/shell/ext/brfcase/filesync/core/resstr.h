/*
 * resstr.h - Common return code to string translation routines description.
 */


/* Prototypes
 *************/

/* resstr.c */

#ifdef DEBUG

extern LPCTSTR GetINTString(int);
extern LPCTSTR GetULONGString(ULONG);
extern LPCTSTR GetBOOLString(BOOL);
extern LPCTSTR GetCOMPARISONRESULTString(COMPARISONRESULT);

#ifdef INC_OLE2

extern LPCTSTR GetHRESULTString(HRESULT);

#endif   /* INC_OLE2 */

#ifdef __SYNCENG_H__

extern LPCTSTR GetTWINRESULTString(TWINRESULT);
extern LPCTSTR GetCREATERECLISTPROCMSGString(UINT);
extern LPCTSTR GetRECSTATUSPROCMSGString(UINT);
extern LPCTSTR GetRECNODESTATEString(RECNODESTATE);
extern LPCTSTR GetRECNODEACTIONString(RECNODEACTION);

#endif   /* __SYNCENG_H__ */

#endif   /* DEBUG */

