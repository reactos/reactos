/*
 * ispriv.h - url.dll APIs.
 */


#ifndef __ISPRIV_H__
#define __ISPRIV_H__


#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */


/* Constants
 ************/

/* Define API decoration for direct import of DLL functions. */

#ifdef _INTSHCUT_
#define INTSHCUTPRIVAPI
#else
#define INTSHCUTPRIVAPI             DECLSPEC_IMPORT
#endif


/* Prototypes
 *************/

/******************************************************************************

@doc INTERNAL

@func HRESULT | AddMIMEFileTypesPS | Adds the MIME-enabled File Types property
sheet to a set of property sheets to a list of property sheets.

@parm LPFNADDPROPSHEETPAGE | pfnAddPage | Callback function to be called to add
the property sheet.

@parm LPARAM | lparam | Data to be passed to callback function.

@rdesc Returns one of the following return codes on success:

@flag S_OK | Pages added.

otherwise returns one of the following return codes on error:

@flag E_ABORT | pfnAddPage callback failed.

@flag E_OUTOFMEMORY | There is not enough memory to complete the operation.

******************************************************************************/

INTSHCUTPRIVAPI HRESULT WINAPI AddMIMEFileTypesPS(LPFNADDPROPSHEETPAGE pfnAddPage,
                                                  LPARAM lparam);


#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */


#endif   /* ! __ISPRIV_H__ */

