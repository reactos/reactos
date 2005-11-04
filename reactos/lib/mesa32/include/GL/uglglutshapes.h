/* uglglutshapes.h - Public header GLUT Shapes */

/* Copyright (c) Mark J. Kilgard, 1994, 1995, 1996, 1998. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

#ifndef GLUTSHAPES_H
#define GLUTSHAPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <GL/gl.h>  

void glutWireSphere (GLdouble radius, GLint slices, GLint stacks);
void glutSolidSphere (GLdouble radius, GLint slices, GLint stacks);
void glutWireCone (GLdouble base, GLdouble height,
		   GLint slices, GLint stacks);
void glutSolidCone (GLdouble base, GLdouble height,
		    GLint slices, GLint stacks);
void glutWireCube (GLdouble size);
void glutSolidCube (GLdouble size);
void glutWireTorus (GLdouble innerRadius, GLdouble outerRadius,
		    GLint sides, GLint rings);
void glutSolidTorus (GLdouble innerRadius, GLdouble outerRadius,
		     GLint sides, GLint rings);
void glutWireDodecahedron (void);
void glutSolidDodecahedron (void);
void glutWireOctahedron (void);
void glutSolidOctahedron (void);
void glutWireTetrahedron (void);
void glutSolidTetrahedron (void);
void glutWireIcosahedron (void);
void glutSolidIcosahedron (void);
void glutWireTeapot (GLdouble size);
void glutSolidTeapot (GLdouble size);

#ifdef __cplusplus
}
#endif

#endif
