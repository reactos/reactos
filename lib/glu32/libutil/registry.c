/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
**
** $Date$ $Revision: 1.1 $
** $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libutil/registry.c,v 1.1 2004/02/02 16:39:16 navaraf Exp $
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
