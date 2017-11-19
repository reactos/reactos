/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/apistubs.c
 * PURPOSE:              OpenGL32 lib, glXXX functions
 */

#include "opengl32.h"



#ifndef __i386__

#define USE_GL_FUNC(name, proto_args, call_args, offset, stack)         \
void GLAPIENTRY gl##name proto_args                                     \
{                                                                       \
    const GLDISPATCHTABLE * Dispatch = IntGetCurrentDispatchTable();    \
    if (!Dispatch)                                                      \
        return;                                                         \
    Dispatch->name call_args ;                                          \
}

#define USE_GL_FUNC_RET(name, ret_type, proto_args, call_args, offset, stack)   \
ret_type GLAPIENTRY gl##name proto_args                                         \
{                                                                               \
    const GLDISPATCHTABLE * Dispatch = IntGetCurrentDispatchTable();            \
    if (!Dispatch)                                                              \
        return 0;                                                               \
    return Dispatch->name call_args ;                                           \
}

#include "glfuncs.h"

#endif //__i386__

/* Unknown debug function */
GLint GLAPIENTRY glDebugEntry(GLint unknown1, GLint unknown2)
{
    return 0;
}
