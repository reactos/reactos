#ifndef KJS_STRUCTS_H
#define KJS_STRUCTS_H

struct _KJS;
struct js_node_st;

typedef int (*PKJS_METHOD)( void *context,
			    struct js_node_st *js_node, struct js_node_st *result );

typedef struct _JSSymbolList {
  JSSymbol symbol;
  PKJS_METHOD registered_function;
  struct _JSSymbolList *next;
  PVOID context;
} JSSymbolList;

struct system_ctx_st
{
  JSSymbol s_print;
  JSSymbol s_mread;
  JSSymbol s_mwrite;
  JSSymbol s_reg;
  JSSymbol s_regdir;

  JSSymbolList *registered_symbols;

  JSSymbol s_bits;
  JSSymbol s_canonicalHost;
  JSSymbol s_canonicalHostCPU;
  JSSymbol s_canonicalHostVendor;
  JSSymbol s_canonicalHostOS;
  JSSymbol s_errno;
  JSSymbol s_lineBreakSequence;
  JSSymbol s_stderr;
  JSSymbol s_stdin;
  JSSymbol s_stdout;

  /* System file handles. */
  JSNode pstderr;
  JSNode pstdin;
  JSNode pstdout;
};

#endif/*KJS_STRUCTS_H*/
