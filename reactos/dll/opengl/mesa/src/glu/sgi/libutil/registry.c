/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */

#include "gluos.h"
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const GLubyte versionString[] = "1.3";
static const GLubyte extensionString[] =
    "GLU_EXT_nurbs_tessellator "
    "GLU_EXT_object_space_tess "
    ;

const GLubyte * GLAPIENTRY
gluGetString(GLenum name)
{

    if (name == GLU_VERSION) {
	return versionString;
    } else if (name == GLU_EXTENSIONS) {
	return extensionString;
    }
    return NULL;
}

/* extName is an extension name.
 * extString is a string of extensions separated by blank(s). There may or 
 * may not be leading or trailing blank(s) in extString.
 * This works in cases of extensions being prefixes of another like
 * GL_EXT_texture and GL_EXT_texture3D.
 * Returns GL_TRUE if extName is found otherwise it returns GL_FALSE.
 */
GLboolean GLAPIENTRY
gluCheckExtension(const GLubyte *extName, const GLubyte *extString)
{
  GLboolean flag = GL_FALSE;
  char *word;
  char *lookHere;
  char *deleteThis;

  if (extString == NULL) return GL_FALSE;

  deleteThis = lookHere = (char *)malloc(strlen((const char *)extString)+1); 
  if (lookHere == NULL)
     return GL_FALSE;
  /* strtok() will modify string, so copy it somewhere */
  strcpy(lookHere,(const char *)extString);

  while ((word= strtok(lookHere," ")) != NULL) {
     if (strcmp(word,(const char *)extName) == 0) {
        flag = GL_TRUE;
	break;
     }  
     lookHere = NULL;		/* get next token */
  }
  free((void *)deleteThis);
  return flag;
} /* gluCheckExtension() */



/*** registry.c ***/
