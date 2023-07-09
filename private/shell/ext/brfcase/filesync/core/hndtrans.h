/*
 * hndtrans.h - Handle translation description.
 */


/* Types
 ********/

/* handles */

DECLARE_HANDLE(HHANDLETRANS);
DECLARE_STANDARD_TYPES(HHANDLETRANS);

DECLARE_HANDLE(HGENERIC);
DECLARE_STANDARD_TYPES(HGENERIC);


/* Prototypes
 *************/

/* hndtrans.c */

extern BOOL CreateHandleTranslator(LONG, PHHANDLETRANS);
extern void DestroyHandleTranslator(HHANDLETRANS);
extern BOOL AddHandleToHandleTranslator(HHANDLETRANS, HGENERIC, HGENERIC);
extern void PrepareForHandleTranslation(HHANDLETRANS);
extern BOOL TranslateHandle(HHANDLETRANS, HGENERIC, PHGENERIC);

#ifdef DEBUG

extern BOOL IsValidHHANDLETRANS(HHANDLETRANS);

#endif

