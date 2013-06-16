#ifndef _U_CURRENT_H_
#define _U_CURRENT_H_

#if defined(MAPI_MODE_UTIL) || defined(MAPI_MODE_GLAPI) || \
    defined(MAPI_MODE_BRIDGE)

#include "glapi/glapi.h"

/* ugly renames to match glapi.h */
#define mapi_table _glapi_table

#ifdef GLX_USE_TLS
#define u_current_table _glapi_tls_Dispatch
#define u_current_user _glapi_tls_Context
#else
#define u_current_table _glapi_Dispatch
#define u_current_user _glapi_Context
#endif

#define u_current_get_internal _glapi_get_dispatch
#define u_current_get_user_internal _glapi_get_context

#define u_current_table_tsd _gl_DispatchTSD

#else /* MAPI_MODE_UTIL || MAPI_MODE_GLAPI || MAPI_MODE_BRIDGE */

#include "u_compiler.h"

struct mapi_table;

#ifdef GLX_USE_TLS

extern __thread struct mapi_table *u_current_table
    __attribute__((tls_model("initial-exec")));

extern __thread void *u_current_user
    __attribute__((tls_model("initial-exec")));

#else /* GLX_USE_TLS */

extern struct mapi_table *u_current_table;
extern void *u_current_user;

#endif /* GLX_USE_TLS */

#endif /* MAPI_MODE_UTIL || MAPI_MODE_GLAPI || MAPI_MODE_BRIDGE */

void
u_current_init(void);

void
u_current_destroy(void);

void
u_current_set(const struct mapi_table *tbl);

struct mapi_table *
u_current_get_internal(void);

void
u_current_set_user(const void *ptr);

void *
u_current_get_user_internal(void);

static INLINE const struct mapi_table *
u_current_get(void)
{
#ifdef GLX_USE_TLS
   return u_current_table;
#else
   return (likely(u_current_table) ?
         u_current_table : u_current_get_internal());
#endif
}

static INLINE const void *
u_current_get_user(void)
{
#ifdef GLX_USE_TLS
   return u_current_user;
#else
   return likely(u_current_user) ? u_current_user : u_current_get_user_internal();
#endif
}

#endif /* _U_CURRENT_H_ */
