#ifndef KJS_H
#define KJS_H

#include "jsint.h"
#include "js.h"
#include "kjs_structs.h"

typedef struct system_ctx_st SystemCtx;

typedef struct _KJS {
  JSInterpPtr interp;
  JSVirtualMachine *vm;
  SystemCtx *ctx;
} KJS, *PKJS;

extern PKJS kjs_create_interp( VOID *Reserved );
extern VOID kjs_destroy_interp( PKJS kjs );
extern VOID kjs_eval( PKJS js_interp, PCHAR commands );
extern VOID kjs_system_register( PKJS js_interp, PCHAR method, PVOID context,
				 PKJS_METHOD function );
extern VOID kjs_system_unregister( PKJS js_interp, PVOID context,
				   PKJS_METHOD function );

#endif/*KJS_H*/
