/******************************Module*Header*******************************\
* Module Name: genexpld.c
*
* The Explode style of the 3D Flying Objects screen saver.
*
* Simulation of a sphere that occasionally explodes.
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <windows.h>
#include <GL\gl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ss3dfo.h"

#define RADIUS         	0.3
#define STEPS    	30
#define MAXPREC		20

static MATRIX *faceMat;
static float *xstep;
static float *ystep;
static float *zstep;
static float *xrot;
static float *yrot;
static float *zrot;
static MESH explodeMesh;
static int iPrec = 10;

static BOOL bOpenGL11;

// Data type accepted by glInterleavedArrays
typedef struct _POINT_N3F_V3F {
    POINT3D normal;
    POINT3D vertex;
} POINT_N3F_V3F;

static POINT_N3F_V3F *pN3V3;

static GLfloat matl1Diffuse[] = {1.0f, 0.8f, 0.0f, 1.0f};
static GLfloat matl2Diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
static GLfloat matlSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};
static GLfloat light0Pos[] = {100.0f, 100.0f, 100.0f, 0.0f};

void genExplode()
{
    int i;
    POINT3D circle[MAXPREC+1];
    double angle;
    double step = -PI / (float)(iPrec - 1);
    double start = PI / 2.0;
    
    for (i = 0, angle = start; i < iPrec; i++, angle += step) {
        circle[i].x = (float) (RADIUS * cos(angle));
        circle[i].y = (float) (RADIUS * sin(angle));
        circle[i].z = 0.0f;
    }

    revolveSurface(&explodeMesh, circle, iPrec);

    for (i = 0; i < explodeMesh.numFaces; i++) {
        ss_matrixIdent(&faceMat[i]);
        xstep[i] = (float)(((float)(rand() & 0x3) * PI) / ((float)STEPS + 1.0));
        ystep[i] = (float)(((float)(rand() & 0x3) * PI) / ((float)STEPS + 1.0));
        zstep[i] = (float)(((float)(rand() & 0x3) * PI) / ((float)STEPS + 1.0));
        xrot[i] = 0.0f;
        yrot[i] = 0.0f;
        zrot[i] = 0.0f;
    }
}

void initExplodeScene()
{
    iPrec = (int)(fTesselFact * 10.5);
    if (iPrec < 5)
        iPrec = 5;
    if (iPrec > MAXPREC)
        iPrec = MAXPREC;

    faceMat = (MATRIX *)SaverAlloc((iPrec * iPrec) * 
    				 (4 * 4 * sizeof(float)));
    xstep = SaverAlloc(iPrec * iPrec * sizeof(float));
    ystep = SaverAlloc(iPrec * iPrec * sizeof(float));
    zstep = SaverAlloc(iPrec * iPrec * sizeof(float));
    xrot = SaverAlloc(iPrec * iPrec * sizeof(float));
    yrot = SaverAlloc(iPrec * iPrec * sizeof(float));
    zrot = SaverAlloc(iPrec * iPrec * sizeof(float));
    
    genExplode();

    // Find out the OpenGL version that we are running on.
    bOpenGL11 = ss_fOnGL11();

    // Setup the data arrays.
    pN3V3 = SaverAlloc(explodeMesh.numFaces * 4 * sizeof(POINT_N3F_V3F));

    // If we are running on OpenGL 1.1, use the new vertex array functions.
    if (bOpenGL11) {
        glInterleavedArrays(GL_N3F_V3F, 0, pN3V3);
    }

    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, matl1Diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, matlSpecular);
    glMaterialf(GL_FRONT, GL_SHININESS, 100.0f);

    glMaterialfv(GL_BACK, GL_AMBIENT_AND_DIFFUSE, matl2Diffuse);
    glMaterialfv(GL_BACK, GL_SPECULAR, matlSpecular);
    glMaterialf(GL_BACK, GL_SHININESS, 60.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-0.33, 0.33, -0.33, 0.33, 0.3, 3.0);

    glTranslatef(0.0f, 0.0f, -1.5f);
}


void delExplodeScene()
{
    delMesh(&explodeMesh);
    
    SaverFree(faceMat);
    SaverFree(xstep);
    SaverFree(ystep);
    SaverFree(zstep);
    SaverFree(xrot);
    SaverFree(yrot);
    SaverFree(zrot);
    SaverFree(pN3V3);
}

void updateExplodeScene(int flags)
{
    static double mxrot = 0.0;
    static double myrot = 0.0;
    static double mzrot = 0.0;
    static double mxrotInc = 0.0;
    static double myrotInc = 0.1;
    static double mzrotInc = 0.0;
    static float maxR;
    static float r = 0.0f;
    static float rotZ = 0.0f;
    static int count = 0;
    static int direction = 1;
    static int restCount = 0;
    static float lightSpin = 0.0f;
    static float spinDelta = 5.0f;
    static int h = 0;
    static RGBA color;
    int i;
    MFACE *faces;
    POINT_N3F_V3F *pn3v3;


    if( gbBounce ) {
        // floating window bounced off an edge
        if (mxrotInc) {
            mxrotInc = 0.0;
            myrotInc = 0.1;
        } else if (myrotInc) {
            myrotInc = 0.0;
            mzrotInc = 0.1;
        } else if (mzrotInc) {
            mzrotInc = 0.0;
            mxrotInc = 0.1;
        }
        gbBounce = FALSE;
    }

    mxrot += mxrotInc;
    myrot += myrotInc;
    mzrot += mzrotInc;

    if (bColorCycle || h == 0) {
        ss_HsvToRgb((float)h, 1.0f, 1.0f, &color);

        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, (GLfloat *) &color);

        h++;
        h %= 360;
    }

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glRotatef(-lightSpin, 0.0f, 1.0f, 0.0f);
    glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);
    lightSpin += spinDelta;
    if ((lightSpin > 90.0) || (lightSpin < 0.0))
        spinDelta = -spinDelta;
    glPopMatrix();

    if (!bOpenGL11) {
        glBegin(GL_QUADS);
    }

    for(
        i = 0, faces = explodeMesh.faces, pn3v3 = pN3V3;
        i < explodeMesh.numFaces;
	i++, faces++, pn3v3 += 4
       ) {
        int a, b, c, d;
        int j;
        POINT3D vector;
        
        ss_matrixIdent(&faceMat[i]);
        ss_matrixRotate(&faceMat[i], xrot[i], yrot[i], zrot[i]);

        if (restCount)
            ;
        else {
            xrot[i] += (xstep[i]);
            yrot[i] += (ystep[i]);
            zrot[i] += (zstep[i]);
        } 

        a = faces->p[0];
        b = faces->p[1];
        c = faces->p[3];
        d = faces->p[2];
        
        memcpy(&pn3v3[0].vertex, (explodeMesh.pts + a), sizeof(POINT3D));
        memcpy(&pn3v3[1].vertex, (explodeMesh.pts + b), sizeof(POINT3D));
        memcpy(&pn3v3[2].vertex, (explodeMesh.pts + c), sizeof(POINT3D));
        memcpy(&pn3v3[3].vertex, (explodeMesh.pts + d), sizeof(POINT3D));

        vector.x = pn3v3[0].vertex.x;
        vector.y = pn3v3[0].vertex.y;
        vector.z = pn3v3[0].vertex.z;

        for (j = 0; j < 4; j++) {
            pn3v3[j].vertex.x -= vector.x;
            pn3v3[j].vertex.y -= vector.y;
            pn3v3[j].vertex.z -= vector.z;
            ss_xformPoint((POINT3D *)&pn3v3[j].vertex, (POINT3D *)&pn3v3[j].vertex, &faceMat[i]);
            pn3v3[j].vertex.x += vector.x + (vector.x * r);
            pn3v3[j].vertex.y += vector.y + (vector.y * r);
            pn3v3[j].vertex.z += vector.z + (vector.z * r);
        }
        if (bSmoothShading) {
            memcpy(&pn3v3[0].normal, (explodeMesh.norms + a), sizeof(POINT3D));
            memcpy(&pn3v3[1].normal, (explodeMesh.norms + b), sizeof(POINT3D));
            memcpy(&pn3v3[2].normal, (explodeMesh.norms + c), sizeof(POINT3D));
            memcpy(&pn3v3[3].normal, (explodeMesh.norms + d), sizeof(POINT3D));
           
            for (j = 0; j < 4; j++)
                ss_xformNorm((POINT3D *)&pn3v3[j].normal, (POINT3D *)&pn3v3[j].normal, &faceMat[i]);
        } else {            
            memcpy(&pn3v3[0].normal, &faces->norm, sizeof(POINT3D));
            ss_xformNorm((POINT3D *)&pn3v3[0].normal, (POINT3D *)&pn3v3[0].normal, &faceMat[i]);
            memcpy(&pn3v3[1].normal, &pn3v3[0].normal, sizeof(POINT3D));
            memcpy(&pn3v3[2].normal, &pn3v3[0].normal, sizeof(POINT3D));
            memcpy(&pn3v3[3].normal, &pn3v3[0].normal, sizeof(POINT3D));
        }

        if (!bOpenGL11) {
            if (bSmoothShading) {
                glNormal3fv((GLfloat *)&pn3v3[0].normal);
                glVertex3fv((GLfloat *)&pn3v3[0].vertex);
                glNormal3fv((GLfloat *)&pn3v3[1].normal);
                glVertex3fv((GLfloat *)&pn3v3[1].vertex);
                glNormal3fv((GLfloat *)&pn3v3[2].normal);
                glVertex3fv((GLfloat *)&pn3v3[2].vertex);
                glNormal3fv((GLfloat *)&pn3v3[3].normal);
                glVertex3fv((GLfloat *)&pn3v3[3].vertex);
            } else {
                glNormal3fv((GLfloat *)&pn3v3[0].normal);
                glVertex3fv((GLfloat *)&pn3v3[0].vertex);
                glVertex3fv((GLfloat *)&pn3v3[1].vertex);
                glVertex3fv((GLfloat *)&pn3v3[2].vertex);
                glVertex3fv((GLfloat *)&pn3v3[3].vertex);
            }
        }
    }

    if (bOpenGL11) {
        glDrawArrays(GL_QUADS, 0, explodeMesh.numFaces * 4);
    } else {
        glEnd();
    }

    if (restCount) {
        restCount--;
        goto resting;
    }

    if (direction) {
        maxR = r;
        r += (float) (0.3 * pow((double)(STEPS - count) / (double)STEPS, 4.0));
    } else {
        r -= (float) (maxR / (double)(STEPS));
    }

    count++;
    if (count > STEPS) {
        direction ^= 1;
        count = 0;

        if (direction == 1) {
            restCount = 10;
            r = 0.0f;

            for (i = 0; i < explodeMesh.numFaces; i++) {
                ss_matrixIdent(&faceMat[i]);
                xstep[i] = (float) (((float)(rand() & 0x3) * PI) / ((float)STEPS + 1.0));
                ystep[i] = (float) (((float)(rand() & 0x3) * PI) / ((float)STEPS + 1.0));
                zstep[i] = (float) (((float)(rand() & 0x3) * PI) / ((float)STEPS + 1.0));
                
                xrot[i] = 0.0f;
                yrot[i] = 0.0f;
                zrot[i] = 0.0f;
            }
        }
    }

resting:
    ;
}
