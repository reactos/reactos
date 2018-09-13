/* handle.h */

/*****************************/
/* Definitions               */
/*****************************/

#define		MAX_HANDLES		100

/*****************************/
/* Function Definitions      */
/*****************************/

cspContext_t *checkHandle (HCRYPTPROV handle);
BOOL setHandle(cspContext_t *context, HCRYPTPROV handle);
HCRYPTPROV getNextHandle ();