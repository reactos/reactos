/*
 * resstr.h - Common return code to string translation routines description.
 */


/* Prototypes
 *************/

/* resstr.c */

#ifdef DEBUG

extern PCSTR GetINTString(int);
extern PCSTR GetULONGString(ULONG);
extern PCSTR GetBOOLString(BOOL);
extern PCSTR GetClipboardFormatNameString(UINT);
extern PCSTR GetCOMPARISONRESULTString(COMPARISONRESULT);

#ifdef INC_OLE2

extern PCSTR GetHRESULTString(HRESULT);

#endif   /* INC_OLE2 */

#ifdef __SYNCENG_H__

extern PCSTR GetTWINRESULTString(TWINRESULT);
extern PCSTR GetCREATERECLISTPROCMSGString(UINT);
extern PCSTR GetRECSTATUSPROCMSGString(UINT);
extern PCSTR GetRECNODESTATEString(RECNODESTATE);
extern PCSTR GetRECNODEACTIONString(RECNODEACTION);

#endif   /* __SYNCENG_H__ */

#endif   /* DEBUG */

