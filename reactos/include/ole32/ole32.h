#ifndef ___OLE32_H
#define ___OLE32_H

#include <windows.h>
#include "wtypes.h"
#include "objbase.h"
#include "obj_base.h"
#include "obj_misc.h"

#include "obj_storage.h"
#include "obj_moniker.h"
#include "obj_clientserver.h"
#include "obj_dataobject.h"
#include "obj_dragdrop.h"
#include "obj_inplace.h"
#include "obj_oleobj.h"
#include "obj_oleview.h"
#include "obj_cache.h"
#include "obj_marshal.h"

#include "rpc.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*
 *  OLE version conversion declarations
 */

typedef struct _OLESTREAM* LPOLESTREAM;
typedef struct _OLESTREAMVTBL {
	DWORD	CALLBACK (*Get)(LPOLESTREAM,LPSTR,DWORD);
	DWORD	CALLBACK (*Put)(LPOLESTREAM,LPSTR,DWORD);
} OLESTREAMVTBL;
typedef OLESTREAMVTBL*	LPOLESTREAMVTBL;
typedef struct _OLESTREAM {
	LPOLESTREAMVTBL	lpstbl;
} OLESTREAM;

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif  /* __OLE32_H */
