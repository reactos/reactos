/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2010  VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <precomp.h>

#include "git_sha1.h"

/**
 * Override the context's GL version if the environment variable
 * MESA_GL_VERSION_OVERRIDE is set. Valid values of MESA_GL_VERSION_OVERRIDE
 * are point-separated version numbers, such as "3.0".
 */
static void
override_version(struct gl_context *ctx, GLuint *major, GLuint *minor)
{
   const char *env_var = "MESA_GL_VERSION_OVERRIDE";
   const char *version;
   int n;

   version = getenv(env_var);
   if (!version) {
      return;
   }

   n = sscanf(version, "%u.%u", major, minor);
   if (n != 2) {
      fprintf(stderr, "error: invalid value for %s: %s\n", env_var, version);
      return;
   }
}

/**
 * Examine enabled GL extensions to determine GL version.
 * Return major and minor version numbers.
 */
static void
compute_version(struct gl_context *ctx)
{
    /* report openGL 2.0 */
    ctx->VersionMajor = 2;
    ctx->VersionMinor = 0;

    override_version(ctx, &ctx->VersionMajor, &ctx->VersionMinor);

    ctx->VersionString = (char *) malloc(20);
    if (ctx->VersionString) {
        _mesa_snprintf(ctx->VersionString, 20,
            "%u.%u Mesa " MESA_VERSION_STRING
#ifdef MESA_GIT_SHA1
            " (" MESA_GIT_SHA1 ")"
#endif
         ,
         ctx->VersionMajor, ctx->VersionMinor);
    }
}

/**
 * Set the context's VersionMajor, VersionMinor, VersionString fields.
 * This should only be called once as part of context initialization
 * or to perform version check for GLX_ARB_create_context_profile.
 */
void
_mesa_compute_version(struct gl_context *ctx)
{
   if (ctx->VersionMajor)
      return;

   compute_version(ctx);
}
