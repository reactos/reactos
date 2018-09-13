#include <wtypes.h>
//#include <oaidl.h>
#include <oleauto.h>
#include <mapi.h>

// Structures and Functions used by BMAPI and VB2C

#define RECIPIENT   	((USHORT)0x0001)
#define FILE        	((USHORT)0x0002)
#define MESSAGE     	((USHORT)0x0004)
#define USESAFEARRAY	((USHORT)0x1000)

#if defined (WIN32)
#ifndef BMAPI_ENTRY                         		 // PFP
  #define BMAPI_ENTRY  ULONG FAR PASCAL    // define BMAPI_ENTRY  PFP
#endif                                      		 // PFP
#else
#ifndef BMAPI_ENTRY                         		 // PFP
  #define BMAPI_ENTRY  ULONG __export FAR PASCAL    // define BMAPI_ENTRY  PFP
#endif                                      		 // PFP
#endif

#ifndef LHANDLE
  #define LHANDLE ULONG
#endif

#ifndef ERR
  #define ERR 	USHORT
#endif


//-----------------------------------------------------------------------
// WIN32 definitions of 32 bit VB interface function support
//-----------------------------------------------------------------------
typedef struct
{
	ULONG	ulReserved;
	BSTR 	bstrSubject;
	BSTR 	bstrNoteText;
	BSTR 	bstrMessageType;
	BSTR 	bstrDate;
	BSTR 	bstrConversationID;
	ULONG 	flFlags;
	ULONG 	nRecipCount;
	ULONG 	nFileCount;
} VB_Message,FAR *lpVB_Message;

typedef VB_Message VB_MESSAGE;
typedef VB_Message FAR *LPVB_MESSAGE;


typedef struct
{
	ULONG 	ulReserved;
	ULONG 	ulRecipClass;
	BSTR 	bstrName;
	BSTR 	bstrAddress;
	ULONG 	ulEIDSize;
	BSTR 	bstrEID;
} VB_Recip,FAR *lpVB_Recip;

typedef VB_Recip VB_RECIPIENT;
typedef VB_Recip FAR *LPVB_RECIPIENT;


typedef struct
{
	ULONG 	ulReserved;
	ULONG 	flFlags;
	ULONG 	nPosition;
	BSTR 	bstrPathName;
	BSTR 	bstrFileName;
	BSTR 	bstrFileType;
} VB_File, FAR *lpVB_File;

/*

// OLEAUT32.DLL loadlib and getprocaddress support

// WINOLEAUTAPI_(void) SysFreeString(BSTR);
typedef VOID (STDAPICALLTYPE *LPFNSYSFREESTRING)
	( BSTR bstr );

// WINOLEAUTAPI_(unsigned int) SysStringByteLen(BSTR bstr);
typedef UINT (STDAPICALLTYPE *LPFNSYSSTRINGBYTELEN)
	(BSTR bstr);

// WINOLEAUTAPI_(BSTR) SysAllocStringByteLen(const char FAR* psz, unsigned int len);
typedef BSTR (STDAPICALLTYPE *LPFNSYSALLOCSTRINGBYTELEN)
	(const char *psz, UINT len );

// WINOLEAUTAPI_(BSTR) SysAllocString(const OLECHAR FAR*);
typedef BSTR (STDAPICALLTYPE *LPFNSYSALLOCSTRING)
	(const OLECHAR * szwString);

// WINOLEAUTAPI_(int)  SysReAllocString(BSTR FAR*, const OLECHAR FAR*);
typedef INT (STDAPICALLTYPE *LPFNSYSREALLOCSTRING)
	(BSTR * lpBstr, const OLECHAR * szwString);

// WINOLEAUTAPI_(unsigned int) SysStringLen(BSTR);
typedef UINT (STDAPICALLTYPE *LPFNSYSSTRINGLEN)
	(BSTR bstr);

// WINOLEAUTAPI SafeArrayAccessData(SAFEARRAY FAR* psa, void HUGEP* FAR* ppvData);
typedef HRESULT (STDAPICALLTYPE *LPFNSAFEARRAYACCESSDATA)
	(struct tagSAFEARRAY *psa, void **ppvData);

// WINOLEAUTAPI SafeArrayUnaccessData(SAFEARRAY FAR* psa);
typedef HRESULT (STDAPICALLTYPE *LPFNSAFEARRAYUNACCESSDATA)
	(struct tagSAFEARRAY *psa);

extern LPFNSYSFREESTRING 			lpfnSysFreeString;
extern LPFNSYSSTRINGBYTELEN 		lpfnSysStringByteLen;
extern LPFNSYSALLOCSTRINGBYTELEN	lpfnSysAllocStringByteLen;
extern LPFNSYSALLOCSTRING			lpfnSysAllocString;
extern LPFNSYSREALLOCSTRING			lpfnSysReAllocString;
extern LPFNSYSSTRINGLEN				lpfnSysStringLen;
extern LPFNSAFEARRAYACCESSDATA		lpfnSafeArrayAccessData;
extern LPFNSAFEARRAYUNACCESSDATA	lpfnSafeArrayUnaccessData;

#undef SysFreeString
#undef SysStringByteLen
#undef SysAllocStringByteLen
#undef SysAllocString
#undef SysReAllocString
#undef SysStringLen
#undef SafeArrayAccessData
#undef SafeArrayUnaccessData
  

#define SysFreeString			(*lpfnSysFreeString)
#define SysStringByteLen 		(*lpfnSysStringByteLen)
#define SysAllocStringByteLen	(*lpfnSysAllocStringByteLen)
#define SysAllocString			(*lpfnSysAllocString)
#define SysReAllocString		(*lpfnSysReAllocString)
#define SysStringLen			(*lpfnSysStringLen)
#define SafeArrayAccessData		(*lpfnSafeArrayAccessData)
#define SafeArrayUnaccessData	(*lpfnSafeArrayUnaccessData)
  
    */



typedef VB_File 			VB_FILE;
typedef VB_File FAR *		LPVB_FILE;

typedef MapiMessage 		VB_MAPI_MESSAGE;
typedef MapiMessage FAR *	LPMAPI_MESSAGE;
typedef LPMAPI_MESSAGE FAR *LPPMAPI_MESSAGE;


typedef MapiRecipDesc 		MAPI_RECIPIENT;
typedef MapiRecipDesc FAR *	LPMAPI_RECIPIENT;
typedef LPMAPI_RECIPIENT FAR *LPPMAPI_RECIPIENT;

typedef MapiFileDesc 		MAPI_FILE;
typedef MapiFileDesc FAR *	LPMAPI_FILE;
typedef LPMAPI_FILE FAR *	LPPMAPI_FILE;

typedef HANDLE FAR *		LPHANDLE;

typedef VB_File 			VB_FILE;
typedef VB_File FAR * 		LPVB_FILE;

typedef MapiMessage 		VB_MAPI_MESSAGE;
typedef MapiMessage FAR *	LPMAPI_MESSAGE;
typedef LPMAPI_MESSAGE FAR *LPPMAPI_MESSAGE;


typedef MapiRecipDesc 		MAPI_RECIPIENT;
typedef MapiRecipDesc FAR *	LPMAPI_RECIPIENT;
typedef LPMAPI_RECIPIENT FAR *LPPMAPI_RECIPIENT;

typedef MapiFileDesc 		MAPI_FILE;
typedef MapiFileDesc FAR *	LPMAPI_FILE;
typedef LPMAPI_FILE FAR *	LPPMAPI_FILE;

typedef HANDLE FAR *		LPHANDLE;
typedef LPHANDLE FAR *		LPPHANDLE;

/*
#if defined WIN32




//-----------------------------------------------------------------------
// WIN32 definitions of 32 bit VB interface functions
//-----------------------------------------------------------------------
BMAPI_ENTRY BMAPISendMail (LHANDLE 			hSession,
                           ULONG 			ulUIParam,
                           LPVB_MESSAGE 	lpM,
                           LPSAFEARRAY *    lppsaRecips,
                           LPSAFEARRAY * 	lppsaFiles,
                           ULONG 			flFlags,
                           ULONG 			ulReserved);

BMAPI_ENTRY BMAPIFindNext(LHANDLE 	hSession,
                          ULONG 	ulUIParam,
                          BSTR * 	bstrType,
                          BSTR * 	bstrSeed,
                          ULONG 	flFlags,
                          ULONG 	ulReserved,
                          BSTR * 	lpbstrId);

BMAPI_ENTRY BMAPIReadMail (LPULONG 	lpulMessage,
                           LPULONG 	nRecips,
                           LPULONG 	nFiles,
                           LHANDLE 	hSession,
                           ULONG 	ulUIParam,
                           BSTR * 	lpbstrID,
                           ULONG 	flFlags,
                           ULONG 	ulReserved);

BMAPI_ENTRY BMAPIGetReadMail(ULONG 			lpMessage,
                             LPVB_MESSAGE 	lpvbMessage,
                             LPSAFEARRAY *  lppsaRecips,
                             LPSAFEARRAY *	lppsaFiles,
                             LPVB_RECIPIENT	lpvbOrig);

BMAPI_ENTRY BMAPISaveMail( LHANDLE 			hSession,
                           ULONG 			ulUIParam,
                           LPVB_MESSAGE 	lpM,
                           LPSAFEARRAY * 	lppsaRecips,
                           LPSAFEARRAY *	lppsaFiles,
                           ULONG 			flFlags,
                           ULONG 			ulReserved,
                           BSTR * 			lpbstrID);

BMAPI_ENTRY BMAPIAddress (LPULONG 			lpulRecip,
                          LHANDLE 			hSession,
                          ULONG 			ulUIParam,
                          BSTR * 			lpbstrCaption,
                          ULONG 			ulEditFields,
                          BSTR * 			lpbstrLabel,
                          LPULONG 			lpulRecipients,
                          LPSAFEARRAY * 	lppsaRecip,		// LPVB_RECIPIENT
                          ULONG 			ulFlags,
                          ULONG 			ulReserved);

BMAPI_ENTRY BMAPIGetAddress (ULONG			ulRecipientData,
                             ULONG 			count,
                             LPSAFEARRAY *	lppsaRecips);

BMAPI_ENTRY BMAPIDetails (LHANDLE 			hSession,
                          ULONG 			ulUIParam,
                          LPVB_RECIPIENT	lpVB,
                          ULONG 			ulFlags,
                          ULONG 			ulReserved);

BMAPI_ENTRY BMAPIResolveName (LHANDLE			hSession,
                              ULONG 			ulUIParam,
                              BSTR  			bstrMapiName,
                              ULONG 			ulFlags,
                              ULONG 			ulReserved,
                              LPVB_RECIPIENT 	lpVB);
*/

typedef ULONG (FAR PASCAL BMAPISENDMAIL)(
    LHANDLE 		hSession,
    ULONG 			ulUIParam,
    LPVB_MESSAGE 	lpM,
    LPSAFEARRAY *    lppsaRecips,
    LPSAFEARRAY * 	lppsaFiles,
    ULONG 			flFlags,
    ULONG 			ulReserved
);
typedef BMAPISENDMAIL FAR *LPBMAPISENDMAIL;
BMAPISENDMAIL BMAPISendMail;


typedef ULONG (FAR PASCAL BMAPIFINDNEXT)(
    LHANDLE hSession,
    ULONG 	ulUIParam,
    BSTR * 	bstrType,
    BSTR * 	bstrSeed,
    ULONG 	flFlags,
    ULONG 	ulReserved,
    BSTR * 	lpbstrId
);
typedef BMAPIFINDNEXT FAR *LPBMAPIFINDNEXT;
BMAPIFINDNEXT BMAPIFindNext;


typedef ULONG (FAR PASCAL BMAPIREADMAIL)(
    LPULONG     lpulMessage,
    LPULONG     nRecips,
    LPULONG     nFiles,
    LHANDLE     hSession,
    ULONG 	    ulUIParam,
    BSTR * 	    lpbstrID,
    ULONG 	    flFlags,
    ULONG 	    ulReserved
);
typedef BMAPIREADMAIL FAR *LPBMAPIREADMAIL;
BMAPIREADMAIL BMAPIReadMail;


typedef ULONG (FAR PASCAL BMAPIGETREADMAIL)(
    ULONG 			lpMessage,
    LPVB_MESSAGE 	lpvbMessage,
    LPSAFEARRAY *  lppsaRecips,
    LPSAFEARRAY *	lppsaFiles,
    LPVB_RECIPIENT	lpvbOrig    
);
typedef BMAPIGETREADMAIL FAR *LPBMAPIGETREADMAIL;
BMAPIGETREADMAIL BMAPIGetReadMail;


typedef ULONG (FAR PASCAL BMAPISAVEMAIL)(
    LHANDLE 			hSession,
    ULONG 			    ulUIParam,
    LPVB_MESSAGE 	    lpM,
    LPSAFEARRAY * 	    lppsaRecips,
    LPSAFEARRAY *	    lppsaFiles,
    ULONG 			    flFlags,
    ULONG 			    ulReserved,
    BSTR * 			    lpbstrID
);
typedef BMAPISAVEMAIL FAR *LPBMAPISAVEMAIL;
BMAPISAVEMAIL BMAPISaveMail;


typedef ULONG (FAR PASCAL BMAPIADDRESS)(
    LPULONG 			lpulRecip,
    LHANDLE 			hSession,
    ULONG 			    ulUIParam,
    BSTR * 			    lpbstrCaption,
    ULONG 			    ulEditFields,
    BSTR * 			    lpbstrLabel,
    LPULONG 			lpulRecipients,
    LPSAFEARRAY * 	    lppsaRecip,		// LPVB_RECIPIENT
    ULONG 			    ulFlags,
    ULONG 			    ulReserved
);
typedef BMAPIADDRESS FAR *LPBMAPIADDRESS;
BMAPIADDRESS BMAPIAddress;


typedef ULONG (FAR PASCAL BMAPIGETADDRESS)(
    ULONG			ulRecipientData,
    ULONG 			count,
    LPSAFEARRAY *	lppsaRecips
);
typedef BMAPIGETADDRESS FAR *LPBMAPIGETADDRESS;
BMAPIGETADDRESS BMAPIGetAddress;


typedef ULONG (FAR PASCAL BMAPIDETAILS)(
    LHANDLE 			hSession,
    ULONG 			ulUIParam,
    LPVB_RECIPIENT	lpVB,
    ULONG 			ulFlags,
    ULONG 			ulReserved
);
typedef BMAPIDETAILS FAR *LPBMAPIDETAILS;
BMAPIDETAILS BMAPIDetails;


typedef ULONG (FAR PASCAL BMAPIRESOLVENAME)(
    LHANDLE			hSession,
    ULONG 			ulUIParam,
    BSTR  			bstrMapiName,
    ULONG 			ulFlags,
    ULONG 			ulReserved,
    LPVB_RECIPIENT 	lpVB
);
typedef BMAPIRESOLVENAME FAR *LPBMAPIRESOLVENAME;
BMAPIRESOLVENAME BMAPIResolveName;
















