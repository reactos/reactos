#include "kjs.h"
#include "js.h"

PKJS kjs_create_interp( VOID *Reserved ) {
  PKJS kjs = ExAllocatePool( NonPagedPool, sizeof(KJS) );
  if( !kjs ) return kjs;
  kjs->interp = js_create_interp(NULL, kjs);
  return kjs;
}

void kjs_destroy_interp( PKJS kjs ) {
  js_destroy_interp( kjs->interp );
  ExFreePool( kjs );
}

void kjs_eval( PKJS kjs, PCHAR commands ) {
  if( !js_eval( kjs->interp, commands ) )
    DbgPrint( "JS Error: %s\n", kjs->vm->error );
}

void kjs_system_register( PKJS kjs, PCHAR name, PVOID context,
			  PKJS_METHOD function ) {
  JSSymbolList *nsym = kjs->ctx->registered_symbols;
  JSSymbol sym = js_vm_intern(kjs->vm, name);

  while( nsym ) {
    if( sym == nsym->symbol ) {
      nsym->registered_function = function;
      nsym->context = context;
      return;
    }
    nsym = nsym->next;
  }

  nsym = js_calloc(kjs->vm, 1, sizeof(JSSymbolList));
  nsym->symbol = sym;
  nsym->registered_function = function;
  nsym->context = context;
  nsym->next = kjs->ctx->registered_symbols;
  kjs->ctx->registered_symbols = nsym;
}

void kjs_system_unregister( PKJS kjs, PVOID context, PKJS_METHOD function ) {
  JSSymbolList *nsym = kjs->ctx->registered_symbols;
  while( nsym ) {
    if( function == nsym->registered_function &&
	context  == nsym->context ) {
      nsym->registered_function = 0;
      nsym->context = 0;
    }
    nsym = nsym->next;
  }
}
