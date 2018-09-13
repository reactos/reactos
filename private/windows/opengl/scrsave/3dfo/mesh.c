/******************************Module*Header*******************************\
* Module Name: mesh.c
*
* Routines to create a mesh representation of a 3D object and to turn it
* into an OpenGL description.
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/
#include <stdlib.h>
#include <windows.h>
#include <GL\gl.h>
#include <string.h>
#include <math.h>
#include "ss3dfo.h"
#include "mesh.h"

#define ZERO_EPS    0.00000001

/******************************Public*Routine******************************\
* newMesh
*
* Allocate memory for the mesh structure to accomodate the specified number
* of points and faces.
*
\**************************************************************************/

void newMesh(MESH *mesh, int numFaces, int numPts)
{
    mesh->numFaces = 0;
    mesh->numPoints = 0;

    if (numPts) {
        mesh->pts = SaverAlloc((LONG)numPts * (LONG)sizeof(POINT3D));
        mesh->norms = SaverAlloc((LONG)numPts * (LONG)sizeof(POINT3D));
    }
    mesh->faces = SaverAlloc((LONG)numFaces * (LONG)sizeof(MFACE));
}

/******************************Public*Routine******************************\
* delMesh
*
* Delete the allocated portions of the MESH structure.
*
\**************************************************************************/

void delMesh(MESH *mesh)
{    
    SaverFree(mesh->pts);
    SaverFree(mesh->norms);
    SaverFree(mesh->faces);
}

/******************************Public*Routine******************************\
* iPtInList
*
* Add a vertex and its normal to the mesh.  If the vertex already exists,
* add in the normal to the existing normal (we to accumulate the average
* normal at each vertex).  Normalization of the normals is the
* responsibility of the caller.
*
\**************************************************************************/

static int iPtInList(MESH *mesh, POINT3D *p, POINT3D *norm, int start)
{
    int i;
    POINT3D *pts = mesh->pts + start;

    for (i = start; i < mesh->numPoints; i++, pts++)
    {
    // If the vertices are within ZERO_EPS of each other, then its the same
    // vertex.

        if ( fabs(pts->x - p->x) < ZERO_EPS &&
             fabs(pts->y - p->y) < ZERO_EPS &&
             fabs(pts->z - p->z) < ZERO_EPS )
        {
            mesh->norms[i].x += norm->x;
            mesh->norms[i].y += norm->y;
            mesh->norms[i].z += norm->z;
            return i;
        }
    }
    
    mesh->pts[i] = *p;
    mesh->norms[i] = *norm;
    mesh->numPoints++;
    return i;
}


/******************************Public*Routine******************************\
* revolveSurface
*
* Takes the set of points in curve and fills the mesh structure with a
* surface of revolution.  The surface consists of quads made up of the
* points in curve rotated about the y-axis.  The number of increments
* in the revolution is determined by the steps parameter.
*
\**************************************************************************/

#define MAXPREC 40

void revolveSurface(MESH *mesh, POINT3D *curve, int steps)
{
    int i;
    int j;
    int facecount = 0;
    double rotation = 0.0;
    double rotInc;
    double cosVal;
    double sinVal;
    int stepsSqr;
    POINT3D norm;
    POINT3D a[MAXPREC + 1];
    POINT3D b[MAXPREC + 1];
    
    if (steps > MAXPREC)
        steps = MAXPREC;
    rotInc = (2.0 * PI) / (double)(steps - 1);
    stepsSqr = steps * steps;
    newMesh(mesh, stepsSqr, 4 * stepsSqr);

    for (j = 0; j < steps; j++, rotation += rotInc) {
        cosVal = cos(rotation);
        sinVal = sin(rotation);
        for (i = 0; i < steps; i++) {
            a[i].x = (float) (curve[i].x * cosVal + curve[i].z * sinVal);
            a[i].y = (float) (curve[i].y);
            a[i].z = (float) (curve[i].z * cosVal - curve[i].x * sinVal);
        }

        cosVal = cos(rotation + rotInc);
        sinVal = sin(rotation + rotInc);
        for (i = 0; i < steps; i++) {
            b[i].x = (float) (curve[i].x * cosVal + curve[i].z * sinVal);
            b[i].y = (float) (curve[i].y);
            b[i].z = (float) (curve[i].z * cosVal - curve[i].x * sinVal);
        }

        for (i = 0; i < (steps - 1); i++) {
            ss_calcNorm(&norm, &b[i + 1], &b[i], &a[i]);
            if ((norm.x * norm.x) + (norm.y * norm.y) + (norm.z * norm.z) < 0.9)
                ss_calcNorm(&norm, &a[i], &a[i+1], &b[i + 1]);
            mesh->faces[facecount].material = j & 7;
            mesh->faces[facecount].norm = norm;
            mesh->faces[facecount].p[0] = iPtInList(mesh, &b[i], &norm, 0);
            mesh->faces[facecount].p[1] = iPtInList(mesh, &a[i], &norm, 0);
            mesh->faces[facecount].p[2] = iPtInList(mesh, &b[i + 1], &norm, 0);
            mesh->faces[facecount].p[3] = iPtInList(mesh, &a[i + 1], &norm, 0); 
            mesh->numFaces++;
            facecount++;
        }
    }

    ss_normalizeNorms(mesh->norms, mesh->numPoints);
}


/******************************Public*Routine******************************\
* updateObject
*
* Takes the mesh structure and converts the data into OpenGL immediate
* mode commands.
*
\**************************************************************************/

void updateObject(MESH *mesh, BOOL bSmooth)
{
    int i;
    int a, b;
    int aOffs, bOffs, cOffs, dOffs;
    MFACE *faces;
    POINT3D *pp;
    POINT3D *pn;
    int lastC, lastD;

    pp = mesh->pts;
    pn = mesh->norms;

    glBegin(GL_QUAD_STRIP);
    for (i = 0, faces = mesh->faces, lastC = faces->p[0], lastD = faces->p[1];
         i < mesh->numFaces; i++, faces++) {

        a = faces->p[0];
        b = faces->p[1];

        if (!bSmooth) {
            if ((a != lastC) || (b != lastD)) {
                glNormal3fv((GLfloat *)&(faces - 1)->norm);

                glVertex3fv((GLfloat *)((char *)pp + 
                            (lastC << 3) + (lastC << 2)));
                glVertex3fv((GLfloat *)((char *)pp + 
                            (lastD << 3) + (lastD << 2)));
                glEnd();
                glBegin(GL_QUAD_STRIP);
            }

            glNormal3fv((GLfloat *)&faces->norm);
            glVertex3fv((GLfloat *)((char *)pp + (a << 3) + (a << 2)));
            glVertex3fv((GLfloat *)((char *)pp + (b << 3) + (b << 2)));
        } else {
            if ((a != lastC) || (b != lastD)) {
                cOffs = (lastC << 3) + (lastC << 2);
                dOffs = (lastD << 3) + (lastD << 2);

                glNormal3fv((GLfloat *)((char *)pn + cOffs));
                glVertex3fv((GLfloat *)((char *)pp + cOffs));
                glNormal3fv((GLfloat *)((char *)pn + dOffs));
                glVertex3fv((GLfloat *)((char *)pp + dOffs));
                glEnd();
                glBegin(GL_QUAD_STRIP);
            }

            aOffs = (a << 3) + (a << 2);
            bOffs = (b << 3) + (b << 2);

            glNormal3fv((GLfloat *)((char *)pn + aOffs));
            glVertex3fv((GLfloat *)((char *)pp + aOffs));
            glNormal3fv((GLfloat *)((char *)pn + bOffs));
            glVertex3fv((GLfloat *)((char *)pp + bOffs));
        }

        lastC = faces->p[2];
        lastD = faces->p[3];
    }

    if (!bSmooth) {
        glNormal3fv((GLfloat *)&(faces - 1)->norm);
        glVertex3fv((GLfloat *)((char *)pp + (lastC << 3) + (lastC << 2)));
        glVertex3fv((GLfloat *)((char *)pp + (lastD << 3) + (lastD << 2)));
    } else {
        cOffs = (lastC << 3) + (lastC << 2);
        dOffs = (lastD << 3) + (lastD << 2);

        glNormal3fv((GLfloat *)((char *)pn + cOffs));
        glVertex3fv((GLfloat *)((char *)pp + cOffs));
        glNormal3fv((GLfloat *)((char *)pn + dOffs));
        glVertex3fv((GLfloat *)((char *)pp + dOffs));
    }

    glEnd();
}


/******************************Public*Routine******************************\
* updateObject
*
* This is a special case that handles a mesh structure that represents
* a strip that is a 1 high loop.
*
* Takes the mesh structure and converts the data into OpenGL immediate
* mode commands.
*
\**************************************************************************/

void updateObject2(MESH *mesh, BOOL bSmooth)
{
    int i;
    int a, b;
    int aOffs, bOffs, cOffs, dOffs;
    MFACE *faces;
    POINT3D *pp;
    POINT3D *pn;
    int lastC, lastD;

    pp = mesh->pts;
    pn = mesh->norms;

    glBegin(GL_QUAD_STRIP);
    for (i = 0, faces = mesh->faces, lastC = faces->p[0], lastD = faces->p[1];
         i < mesh->numFaces; i++, faces++) {

        a = faces->p[0];
        b = faces->p[1];

        if (!bSmooth) {
            glNormal3fv((GLfloat *)&faces->norm);
            glVertex3fv((GLfloat *)((char *)pp + (a << 3) + (a << 2)));
            glVertex3fv((GLfloat *)((char *)pp + (b << 3) + (b << 2)));
        } else {
            aOffs = (a << 3) + (a << 2);
            bOffs = (b << 3) + (b << 2);

            glNormal3fv((GLfloat *)((char *)pn + aOffs));
            glVertex3fv((GLfloat *)((char *)pp + aOffs));
            glNormal3fv((GLfloat *)((char *)pn + bOffs));
            glVertex3fv((GLfloat *)((char *)pp + bOffs));
        }

        lastC = faces->p[2];
        lastD = faces->p[3];
    }

    if (!bSmooth) {
        glNormal3fv((GLfloat *)&(mesh->faces)->norm);
        glVertex3fv((GLfloat *)((char *)pp + (lastC << 3) + (lastC << 2)));
        glVertex3fv((GLfloat *)((char *)pp + (lastD << 3) + (lastD << 2)));
    } else {
        cOffs = (lastC << 3) + (lastC << 2);
        dOffs = (lastD << 3) + (lastD << 2);

        glNormal3fv((GLfloat *)((char *)pn + cOffs));
        glVertex3fv((GLfloat *)((char *)pp + cOffs));
        glNormal3fv((GLfloat *)((char *)pn + dOffs));
        glVertex3fv((GLfloat *)((char *)pp + dOffs));
    }

    glEnd();
}


/******************************Public*Routine******************************\
* MakeList
*
* Takes the mesh structure and converts the data into OpenGL display
* list.
*
\**************************************************************************/

void MakeList(GLuint listID, MESH *mesh)
{
    int i;
    int a, b;
    int aOffs, bOffs, cOffs, dOffs;
    MFACE *faces;
    BOOL bSmooth;
    POINT3D *pp;
    POINT3D *pn;
    GLint shadeModel;
    int lastC, lastD;

    glGetIntegerv(GL_SHADE_MODEL, &shadeModel);

    bSmooth = (shadeModel == GL_SMOOTH);

    glNewList(listID, GL_COMPILE);

    pp = mesh->pts;
    pn = mesh->norms;

    glBegin(GL_QUAD_STRIP);
    for (i = 0, faces = mesh->faces, lastC = faces->p[0], lastD = faces->p[1];
         i < mesh->numFaces; i++, faces++) {

        a = faces->p[0];
        b = faces->p[1];

        if (!bSmooth) {

            if ((a != lastC) || (b != lastD)) {
                glNormal3fv((GLfloat *)&((faces - 1)->norm));

                glVertex3fv((GLfloat *)((char *)pp + 
                            (lastC << 3) + (lastC << 2)));
                glVertex3fv((GLfloat *)((char *)pp + 
                            (lastD << 3) + (lastD << 2)));
                glEnd();
                glBegin(GL_QUAD_STRIP);
            }

            glNormal3fv((GLfloat *)&faces->norm);
            glVertex3fv((GLfloat *)((char *)pp + (a << 3) + (a << 2)));
            glVertex3fv((GLfloat *)((char *)pp + (b << 3) + (b << 2)));
        } else {
            if ((a != lastC) || (b != lastD)) {
                cOffs = (lastC << 3) + (lastC << 2);
                dOffs = (lastD << 3) + (lastD << 2);

                glNormal3fv((GLfloat *)((char *)pn + cOffs));
                glVertex3fv((GLfloat *)((char *)pp + cOffs));
                glNormal3fv((GLfloat *)((char *)pn + dOffs));
                glVertex3fv((GLfloat *)((char *)pp + dOffs));
                glEnd();
                glBegin(GL_QUAD_STRIP);
            }

            aOffs = (a << 3) + (a << 2);
            bOffs = (b << 3) + (b << 2);

            glNormal3fv((GLfloat *)((char *)pn + aOffs));
            glVertex3fv((GLfloat *)((char *)pp + aOffs));
            glNormal3fv((GLfloat *)((char *)pn + bOffs));
            glVertex3fv((GLfloat *)((char *)pp + bOffs));
        }

        lastC = faces->p[2];
        lastD = faces->p[3];
    }

    if (!bSmooth) {
        glNormal3fv((GLfloat *)&((faces - 1)->norm));
        glVertex3fv((GLfloat *)((char *)pp + (lastC << 3) + (lastC << 2)));
        glVertex3fv((GLfloat *)((char *)pp + (lastD << 3) + (lastD << 2)));
    } else {
        cOffs = (lastC << 3) + (lastC << 2);
        dOffs = (lastD << 3) + (lastD << 2);

        glNormal3fv((GLfloat *)((char *)pn + cOffs));
        glVertex3fv((GLfloat *)((char *)pp + cOffs));
        glNormal3fv((GLfloat *)((char *)pn + dOffs));
        glVertex3fv((GLfloat *)((char *)pp + dOffs));
    }

    glEnd();

    glEndList();
}
