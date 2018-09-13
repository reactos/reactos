/******************************Module*Header*******************************\
* Module Name: genwin.c
*
* The Windows Logo style of the 3D Flying Objects screen saver.
*
* Animated 3D model of the Microsoft (R) Windows NT (TM) flag logo.
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

#define WIN_TOP_BORDER      (float)0.1
#define WIN_RIGHT_BORDER    WIN_TOP_BORDER
#define WIN_CROSSBAR        (0.6522f * WIN_TOP_BORDER)
#define WIN_NUMPIECES       7
#define WIN_NUMCOLUMNS      6
#define WIN_GAP             (WIN_TOP_BORDER / 8.0f)
#define WIN_GAP_X           (2.0f * WIN_GAP)
#define WIN_HEIGHT          ((WIN_GAP * 6.0f) + \
                                (WIN_NUMPIECES * WIN_TOP_BORDER))
#define WIN_WIDTH           (0.7024f * WIN_HEIGHT)
#define WIN_THICKNESS       WIN_CROSSBAR

#define WIN_TOTALWIDTH      (WIN_TOP_BORDER * 1.1f * (float)WIN_NUMCOLUMNS + \
                             WIN_WIDTH)

#define BLOCK_TOP            0x0001
#define BLOCK_BOTTOM         0x0002
#define BLOCK_LEFT           0x0004
#define BLOCK_RIGHT          0x0008
#define BLOCK_FRONT          0x0010
#define BLOCK_BACK           0x0020
#define BLOCK_ALL            0x003f
#define DELTA_BLEND          0x2000
#define NO_BLEND             0x1000

#define CUBE_FACES          6
#define CUBE_POINTS         8

#define MAX_FRAMES 20

#define MAXPREC 15
#define S_IPREC 3

static int Frames = 10;
static MESH winMesh[MAX_FRAMES];
static MESH winStreamer[MAX_FRAMES];
static float sinAngle = 0.0f;
static float xTrans = 0.2f;
static int curMatl = 0;
static int iPrec = 10;

static RGBA matlBrightSpecular = {1.0f, 1.0f, 1.0f, 1.0f};
static RGBA matlDimSpecular = {0.5f, 0.5f, 0.5f, 1.0f};
static RGBA matlNoSpecular = {0.0f, 0.0f, 0.0f, 0.0f};
static GLfloat light0Pos[] = {20.0f, -10.0f, 20.0f, 0.0f};

static RGBA light1Ambient  = {0.0f, 0.0f, 0.0f, 0.0f};
static RGBA light1Diffuse  = {0.4f, 0.4f, 0.4f, 1.0f};
static RGBA light1Specular = {0.0f, 0.0f, 0.0f, 0.0f};
static GLfloat light1Pos[] = {-20.0f, 5.0f, 0.0f, 0.0f};

static RGBA winColors[] = {{0.3f, 0.3f, 0.3f, 1.0f},
                           {0.94f, 0.37f, 0.13f, 1.0f},    // red
                           {0.22f, 0.42f, 0.78f, 1.0f},    // blue
                           {0.35f, 0.71f, 0.35f, 1.0f},    // green
                           {0.95f, 0.82f, 0.12f, 1.0f}};   // yellow

static int iPtInList(MESH *mesh, int start, 
                     POINT3D *p, POINT3D *norm, BOOL blend)
{
    int i;
    POINT3D *pts = mesh->pts + start;

    if (blend) {
        for (i = start; i < mesh->numPoints; i++, pts++) {
            if ((pts->x == p->x) && (pts->y == p->y) && (pts->z == p->z)) {
                mesh->norms[i].x += norm->x;
                mesh->norms[i].y += norm->y;
                mesh->norms[i].z += norm->z;
                return i;
            }
        }
    } else {
        i = mesh->numPoints;
    }
    
    mesh->pts[i] = *p;
    mesh->norms[i] = *norm;
    mesh->numPoints++;
    return i;
}

float getZPos(float x)
{
    float xAbs = x - xTrans;
    float angle = (float) (sinAngle + ((2.0 * PI) * (xAbs / WIN_TOTALWIDTH)));

    xAbs += (WIN_TOTALWIDTH / 2.0f);
    xAbs = WIN_TOTALWIDTH - xAbs;

    return (float)((sin((double)angle) / 4.0) * 
                   sqrt((double)(xAbs / WIN_TOTALWIDTH )));
}

void AddFace(MESH *mesh, int startBlend, POINT3D *pos, float w, float h)
{
#define FACE_VERTEX(i) \
    iPtInList(mesh, startBlend, pts + i, &mesh->faces[faceCount].norm, TRUE)

    int faceCount = mesh->numFaces;     
    int numPts = mesh->numPoints;
    POINT3D *pts = mesh->pts + numPts;
    float zLeft = getZPos(pos->x);
    float zRight = getZPos(pos->x + w);


    pts->x = (float)pos->x; 
    pts->y = (float)pos->y;   
    pts->z = zLeft; 
    pts++;

    pts->x = (float)pos->x;
    pts->y = (float)(pos->y + h);  
    pts->z = zLeft;  
    pts++;

    pts->x = (float)(pos->x + w);  
    pts->y = (float)(pos->y + h);  
    pts->z = zRight;  
    pts++;

    pts->x = (float)(pos->x + w);
    pts->y = (float)pos->y;  
    pts->z = zRight;

    pts -= 3;

    mesh->faces[faceCount].material = curMatl;
    ss_calcNorm(&mesh->faces[faceCount].norm, pts + 2, pts + 1, pts);
    mesh->faces[faceCount].p[3] = FACE_VERTEX(0);
    mesh->faces[faceCount].p[2] = FACE_VERTEX(1);
    mesh->faces[faceCount].p[1] = FACE_VERTEX(2);
    mesh->faces[faceCount].p[0] = FACE_VERTEX(3);
    mesh->numFaces++;
}

#define BLOCK_VERTEX(face, i)\
{\
    if (flags & DELTA_BLEND) {\
        mesh->faces[faceCount].p[face] = \
            iPtInList(mesh, blendStart, &pts[i], &norms[((i & 0x2) >> 1)],\
                      bBlend);\
    } else\
        mesh->faces[faceCount].p[face] = \
            iPtInList(mesh, blendStart, &pts[i],\
                      &mesh->faces[faceCount].norm, bBlend);\
}

#define DELTA_FACT  (float)10.0

void AddBlock(MESH *mesh, int blendStart, POINT3D *pos, 
              float w, float h, float d, ULONG flags)
{

    POINT3D pts[8];
    POINT3D ptsL[8];
    POINT3D ptsR[8];
    POINT3D norms[2];
    POINT3D posPrev;
    float zLeft = getZPos(pos->x);
    float zRight = getZPos(pos->x + w);
    int faceCount = mesh->numFaces;     
    BOOL bBlend = ((flags & NO_BLEND) == 0);

    flags |= DELTA_BLEND;

    pts[0].x = (float)pos->x; 
    pts[0].y = (float)(pos->y + h);   
    pts[0].z = zLeft;

    pts[1].x = (float)pos->x;
    pts[1].y = (float)(pos->y + h);  
    pts[1].z = zLeft + d;  

    pts[2].x = (float)(pos->x + w);  
    pts[2].y = (float)(pos->y + h);  
    pts[2].z = zRight + d;  

    pts[3].x = (float)(pos->x + w);
    pts[3].y = (float)(pos->y + h);
    pts[3].z = zRight;

    pts[4].x = (float)pos->x;
    pts[4].y = (float)pos->y;
    pts[4].z = zLeft;

    pts[5].x = (float)pos->x;
    pts[5].y = (float)pos->y;
    pts[5].z = zLeft + d;  

    pts[6].x = (float)(pos->x + w);  
    pts[6].y = (float)pos->y;
    pts[6].z = zRight + d;  

    pts[7].x = (float)(pos->x + w);  
    pts[7].y = (float)pos->y;
    pts[7].z = zRight;

    if (flags & DELTA_BLEND) {
	float prevW = w;
        posPrev = *pos;

        w /= DELTA_FACT;
        zRight = getZPos(pos->x + w);

        ptsL[0].x = (float)pos->x; 
        ptsL[0].y = (float)(pos->y + h);   
        ptsL[0].z = zLeft;

        ptsL[1].x = (float)pos->x;
        ptsL[1].y = (float)(pos->y + h);  
        ptsL[1].z = zLeft + d;  

        ptsL[2].x = (float)(pos->x + w);  
        ptsL[2].y = (float)(pos->y + h);  
        ptsL[2].z = zRight + d;  

        ptsL[3].x = (float)(pos->x + w);
        ptsL[3].y = (float)(pos->y + h);
        ptsL[3].z = zRight;

        ptsL[4].x = (float)pos->x;
        ptsL[4].y = (float)pos->y;
        ptsL[4].z = zLeft;

        ptsL[5].x = (float)pos->x;
        ptsL[5].y = (float)pos->y;
        ptsL[5].z = zLeft + d;  

        ptsL[6].x = (float)(pos->x + w);  
        ptsL[6].y = (float)pos->y;
        ptsL[6].z = zRight + d;  

        ptsL[7].x = (float)(pos->x + w);  
        ptsL[7].y = (float)pos->y;
        ptsL[7].z = zRight;

        pos->x += (prevW - w);
        zLeft = getZPos(pos->x);
        zRight = getZPos(pos->x + w);

        ptsR[0].x = (float)pos->x; 
        ptsR[0].y = (float)(pos->y + h);   
        ptsR[0].z = zLeft;

        ptsR[1].x = (float)pos->x;
        ptsR[1].y = (float)(pos->y + h);  
        ptsR[1].z = zLeft + d;  

        ptsR[2].x = (float)(pos->x + w);  
        ptsR[2].y = (float)(pos->y + h);  
        ptsR[2].z = zRight + d;  

        ptsR[3].x = (float)(pos->x + w);
        ptsR[3].y = (float)(pos->y + h);
        ptsR[3].z = zRight;

        ptsR[4].x = (float)pos->x;
        ptsR[4].y = (float)pos->y;
        ptsR[4].z = zLeft;

        ptsR[5].x = (float)pos->x;
        ptsR[5].y = (float)pos->y;
        ptsR[5].z = zLeft + d;  

        ptsR[6].x = (float)(pos->x + w);  
        ptsR[6].y = (float)pos->y;
        ptsR[6].z = zRight + d;  

        ptsR[7].x = (float)(pos->x + w);  
        ptsR[7].y = (float)pos->y;
        ptsR[7].z = zRight;

        *pos = posPrev;

    }

    if (flags & BLOCK_TOP) {
        mesh->faces[faceCount].material = curMatl;
        ss_calcNorm(&mesh->faces[faceCount].norm, &pts[0], &pts[1], &pts[2]);
        if (flags & DELTA_BLEND) {
            ss_calcNorm(&norms[0], &ptsL[0], &ptsL[1], &ptsL[2]);
            ss_calcNorm(&norms[1], &ptsR[0], &ptsR[1], &ptsR[2]);
        }
        BLOCK_VERTEX(0, 0);
        BLOCK_VERTEX(1, 1);
        BLOCK_VERTEX(2, 2);
        BLOCK_VERTEX(3, 3);
        faceCount++;
        mesh->numFaces++;
    }

    if (flags & BLOCK_BOTTOM) {
        mesh->faces[faceCount].material = curMatl;
        ss_calcNorm(&mesh->faces[faceCount].norm, &pts[4], &pts[7], &pts[6]);
        if (flags & DELTA_BLEND) {
            ss_calcNorm(&norms[0], &ptsL[4], &ptsL[7], &ptsL[6]);
            ss_calcNorm(&norms[1], &ptsR[4], &ptsR[7], &ptsR[6]);
        }
        BLOCK_VERTEX(0, 4);
        BLOCK_VERTEX(1, 7);
        BLOCK_VERTEX(2, 6);
        BLOCK_VERTEX(3, 5);
        faceCount++;
        mesh->numFaces++;
    }

    if (flags & BLOCK_LEFT) {
        mesh->faces[faceCount].material = curMatl;
        ss_calcNorm(&mesh->faces[faceCount].norm, &pts[1], &pts[0], &pts[4]);
        if (flags & DELTA_BLEND) {
            ss_calcNorm(&norms[0], &ptsL[1], &ptsL[0], &ptsL[4]);
            ss_calcNorm(&norms[1], &ptsR[1], &ptsR[0], &ptsR[4]);
        }
        BLOCK_VERTEX(0, 1);
        BLOCK_VERTEX(1, 0);
        BLOCK_VERTEX(2, 4);
        BLOCK_VERTEX(3, 5);
        faceCount++;
        mesh->numFaces++;
    }

    if (flags & BLOCK_RIGHT) {
        mesh->faces[faceCount].material = curMatl;
        ss_calcNorm(&mesh->faces[faceCount].norm, &pts[3], &pts[2], &pts[6]);
        if (flags & DELTA_BLEND) {
            ss_calcNorm(&norms[0], &ptsL[3], &ptsL[2], &ptsL[6]);
            ss_calcNorm(&norms[1], &ptsR[3], &ptsR[2], &ptsR[6]);
        }
        BLOCK_VERTEX(0, 3);
        BLOCK_VERTEX(1, 2);
        BLOCK_VERTEX(2, 6);
        BLOCK_VERTEX(3, 7);
        faceCount++;
        mesh->numFaces++;
    }

    if (flags & BLOCK_FRONT) {
        mesh->faces[faceCount].material = curMatl;
        ss_calcNorm(&mesh->faces[faceCount].norm, &pts[0], &pts[3], &pts[7]);
        if (flags & DELTA_BLEND) {
            ss_calcNorm(&norms[0], &ptsL[0], &ptsL[3], &ptsL[7]);
            ss_calcNorm(&norms[1], &ptsR[0], &ptsR[3], &ptsR[7]);
        }
        BLOCK_VERTEX(0, 0);
        BLOCK_VERTEX(1, 3);
        BLOCK_VERTEX(2, 7);
        BLOCK_VERTEX(3, 4);
        faceCount++;
        mesh->numFaces++;
    }

    if (flags & BLOCK_BACK) {
        mesh->faces[faceCount].material = curMatl;
        ss_calcNorm(&mesh->faces[faceCount].norm, &pts[1], &pts[5], &pts[6]);
        if (flags & DELTA_BLEND) {
            ss_calcNorm(&norms[0], &ptsL[1], &ptsL[5], &ptsL[6]);
            ss_calcNorm(&norms[1], &ptsR[1], &ptsR[5], &ptsR[6]);
        }
        BLOCK_VERTEX(0, 1);
        BLOCK_VERTEX(1, 5);
        BLOCK_VERTEX(2, 6);
        BLOCK_VERTEX(3, 2);
        mesh->numFaces++;
    }

}

void genWin(MESH *winMesh, MESH *winStreamer)
{
    POINT3D pos, posCenter;
    float w, h, d;
    float wMax, hMax;
    float xpos;
    int i, j, prec;
    int startBlend;

    newMesh(winMesh, CUBE_FACES * iPrec * 20, CUBE_POINTS * iPrec * 20);

    //
    // create window frame
    //

    w = (WIN_WIDTH - WIN_TOP_BORDER) / (float)iPrec;
    h = (float)WIN_TOP_BORDER;
    d = (float)WIN_THICKNESS;

    // draw top and bottom portions

    pos.y = 0.0f;
    pos.z = 0.0f;

    for (i = 0, pos.x = xTrans, startBlend = winMesh->numPoints;
         i < iPrec; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_TOP);
    for (i = 0, pos.x = xTrans, startBlend = winMesh->numPoints;
         i < iPrec; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_BOTTOM);
    for (i = 0, pos.x = xTrans, startBlend = winMesh->numPoints; 
         i < iPrec; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_FRONT);
    for (i = 0, pos.x = xTrans, startBlend = winMesh->numPoints;
         i < iPrec; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_BACK);
    pos.x = xTrans;
    AddBlock(winMesh, 0, &pos, w, h, d, BLOCK_LEFT | NO_BLEND);

    pos.y = WIN_HEIGHT - WIN_TOP_BORDER;
    pos.z = 0.0f;

    for (i = 0, pos.x = xTrans, startBlend = winMesh->numPoints;
         i < iPrec; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_TOP);
    for (i = 0, pos.x = xTrans, startBlend = winMesh->numPoints;
         i < iPrec; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_BOTTOM);
    for (i = 0, pos.x = xTrans, startBlend = winMesh->numPoints;
         i < iPrec; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_FRONT);
    for (i = 0, pos.x = xTrans, startBlend = winMesh->numPoints;
         i < iPrec; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_BACK);
    pos.x = xTrans;
    AddBlock(winMesh, 0, &pos, w, h, d, BLOCK_LEFT | NO_BLEND);
    
    // draw middle horizontal portions

    prec = (iPrec / 2);
    w = (WIN_WIDTH - WIN_TOP_BORDER - WIN_CROSSBAR) / 2.0f;
    w /= (float)prec;
    h = WIN_CROSSBAR;
    pos.y = (WIN_HEIGHT - WIN_CROSSBAR) / 2.0f;
    pos.z = 0.0f;

    for (i = 0, pos.x = xTrans, startBlend = winMesh->numPoints;
         i < prec; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_TOP);
    for (i = 0, pos.x = xTrans, startBlend = winMesh->numPoints;
         i < prec; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_BOTTOM);
    for (i = 0, pos.x = xTrans, startBlend = winMesh->numPoints;
         i < prec; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_FRONT);
    for (i = 0, pos.x = xTrans, startBlend = winMesh->numPoints;
         i < prec; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_BACK);

    xpos = pos.x + WIN_CROSSBAR;

    for (i = 0, pos.x = xpos, startBlend = winMesh->numPoints;
         i < prec; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_TOP);
    for (i = 0, pos.x = xpos, startBlend = winMesh->numPoints;
         i < prec; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_BOTTOM);
    for (i = 0, pos.x = xpos, startBlend = winMesh->numPoints;
         i < prec; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_FRONT);
    for (i = 0, pos.x = xpos, startBlend = winMesh->numPoints;
         i < prec; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_BACK);

    pos.x = xTrans;
    AddBlock(winMesh, 0, &pos, w, h, d, BLOCK_LEFT | NO_BLEND);


    // Draw thick right-hand edge of frame

    pos.x = xpos = xTrans + WIN_WIDTH - WIN_RIGHT_BORDER;
    pos.y = 0.0f;
    pos.z = 0.0f;
    w = WIN_RIGHT_BORDER / (float)S_IPREC;
    h = WIN_HEIGHT;

    AddBlock(winMesh, winMesh->numPoints, &pos, w, h, d, BLOCK_LEFT);

    for (i = 0, pos.x = xpos, startBlend = winMesh->numPoints;
         i < S_IPREC; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_FRONT);
    for (i = 0, pos.x = xpos, startBlend = winMesh->numPoints;
         i < S_IPREC; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_BACK);
    for (i = 0, pos.x = xpos, startBlend = winMesh->numPoints;
         i < S_IPREC; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_TOP);
    pos.y = WIN_HEIGHT;
    for (i = 0, pos.x = xpos, startBlend = winMesh->numPoints;
         i < S_IPREC; i++, pos.x += w)
        AddBlock(winMesh, startBlend, &pos, w, h, d, BLOCK_BOTTOM);

    pos.y = 0.0f;
    pos.x = xTrans + WIN_WIDTH - w;
    AddBlock(winMesh, winMesh->numPoints, &pos, w, h, d, BLOCK_RIGHT);

    // draw middle-vertical portion of frame

    pos.x = xTrans + (WIN_WIDTH - WIN_RIGHT_BORDER) / 2.0f - (WIN_CROSSBAR / 2.0f);
    pos.y = WIN_TOP_BORDER;
    pos.z = 0.0f;
    w = WIN_CROSSBAR;
    h = WIN_HEIGHT - 2.0f * WIN_TOP_BORDER;
    AddBlock(winMesh, 0, &pos, w, h, d, BLOCK_ALL | NO_BLEND);

    //
    // add the panels
    //

    w = (WIN_WIDTH - WIN_RIGHT_BORDER - WIN_CROSSBAR) / 2.0f;
    h = (WIN_HEIGHT - 2.0f * WIN_TOP_BORDER - WIN_CROSSBAR) / 2.0f;

    w /= (float)(iPrec / 2);

    curMatl = 2;
    pos.x = xTrans;
    pos.y = WIN_TOP_BORDER;
    for (i = 0, startBlend = winMesh->numPoints; i < iPrec / 2; i++) {
        AddFace(winMesh, startBlend, &pos, w, h);
        pos.x += w;
    }
    curMatl = 4;
    pos.x += WIN_CROSSBAR;
    for (i = 0, startBlend = winMesh->numPoints; i < iPrec / 2; i++) {
        AddFace(winMesh, startBlend, &pos, w, h);
        pos.x += w;
    }

    curMatl = 1;
    pos.x = xTrans;
    pos.y = WIN_TOP_BORDER + h + WIN_CROSSBAR;
    for (i = 0, startBlend = winMesh->numPoints; i < iPrec / 2; i++) {
        AddFace(winMesh, startBlend, &pos, w, h);
        pos.x += w;
    }
    curMatl = 3;
    pos.x += WIN_CROSSBAR;
    for (i = 0, startBlend = winMesh->numPoints; i < iPrec / 2; i++) {
        AddFace(winMesh, startBlend, &pos, w, h);
        pos.x += w;
    }

    ss_normalizeNorms(winMesh->norms, winMesh->numPoints);

    newMesh(winStreamer, CUBE_FACES * WIN_NUMPIECES * WIN_NUMCOLUMNS,
            CUBE_POINTS * WIN_NUMPIECES * WIN_NUMCOLUMNS);

    h = hMax = WIN_TOP_BORDER;
    w = wMax = WIN_TOP_BORDER * 1.1f;

    posCenter.x = pos.x = xTrans - wMax - WIN_GAP_X;
    posCenter.y = pos.y = 0.0f;

    for (i = 0; i < WIN_NUMCOLUMNS; i++) {
        for (j = 0; j < WIN_NUMPIECES; j++) {
            if (((j % 3) == 0) || (i == 0))
                curMatl = 0;
            else if (j < 3)
                curMatl = 2;
            else
                curMatl = 1;
            AddBlock(winStreamer, 0, &pos, w, h, d, BLOCK_ALL);
            pos.y += (hMax + WIN_GAP);
        }

        posCenter.x -= (wMax + WIN_GAP_X);
        posCenter.y = 0.0f;

        h = h * 0.8f;
        w = w * 0.8f;

        pos.x = posCenter.x;
        pos.y = posCenter.y;

        pos.x += (wMax - w) / 2.0f;
        pos.y += (hMax - h) / 2.0f;
    }    
    ss_normalizeNorms(winStreamer->norms, winStreamer->numPoints);
}

void initWinScene()
{
    int i;
    float angleDelta;

    iPrec = (int)(fTesselFact * 10.5);
    if (iPrec < 5)
        iPrec = 5;
    if (iPrec > MAXPREC)
        iPrec = MAXPREC;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -0.75, 1.25, 0.0, 3.0);
    glTranslatef(0.0f, 0.0f, -1.5f);
    glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);

    glLightfv(GL_LIGHT1, GL_AMBIENT, (GLfloat *) &light1Ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, (GLfloat *) &light1Diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, (GLfloat *) &light1Specular);
    glLightfv(GL_LIGHT1, GL_POSITION, light1Pos);
    glEnable(GL_LIGHT1);

    glMatrixMode(GL_MODELVIEW);

    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);

    Frames = (int)((float)(MAX_FRAMES / 2) * fTesselFact);

    if (Frames < 5)
        Frames = 5;
    if (Frames > MAX_FRAMES)
        Frames = MAX_FRAMES;

    angleDelta = (float) ((2.0 * PI) / Frames);
    sinAngle = 0.0f;

    for (i = 0; i < Frames; i++) {
        genWin(&winMesh[i], &winStreamer[i]);
        sinAngle += angleDelta;
    }
}

void delWinScene()
{
    int i;

    for (i = 0; i < Frames; i++) {
        delMesh(&winMesh[i]);
        delMesh(&winStreamer[i]);
    }
}

void updateWinScene(int flags)
{
    MESH *mesh;
    MFACE *faces;
    int i;
    static double mxrot = 23.0;
    static double myrot = 23.0;
    static double mzrot = 5.7;
    static double mxrotInc = 0.0;
    static double myrotInc = 3.0;
    static double mzrotInc = 0.0;
    static int h = 0;
    static int frameNum = 0;
    POINT3D *pp;
    POINT3D *pn;
    int lastC, lastD;
    int aOffs, bOffs, cOffs, dOffs;
    int a, b;

    if (bColorCycle) {
        ss_HsvToRgb((float)h, 1.0f, 1.0f, &winColors[0] );

        h++;
        h %= 360;
    }

    glLoadIdentity();
    glRotatef((GLfloat) mxrot, 1.0f, 0.0f, 0.0f);
    glRotatef((GLfloat) myrot, 0.0f, 1.0f, 0.0f);
    glRotatef((GLfloat) mzrot, 0.0f, 0.0f, 1.0f);

    curMatl = 0;
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, (GLfloat *) &winColors[0]);
    glMaterialfv(GL_FRONT, GL_SPECULAR, (GLfloat *) &matlBrightSpecular);
    glMaterialf(GL_FRONT, GL_SHININESS, 60.0f);

    mesh = &winMesh[frameNum];

    glBegin(GL_QUAD_STRIP);

    pp = mesh->pts;
    pn = mesh->norms;

    for (i = 0, faces = mesh->faces, lastC = faces->p[0], lastD = faces->p[1];
         i < mesh->numFaces; i++, faces++) {

        a = faces->p[0];
        b = faces->p[1];

	if (!bSmoothShading) {
            if ((a != lastC) || (b != lastD)) {
                glNormal3fv((GLfloat *)&(faces - 1)->norm);

                glVertex3fv((GLfloat *)((char *)pp + 
                            (lastC << 3) + (lastC << 2)));
                glVertex3fv((GLfloat *)((char *)pp + 
                            (lastD << 3) + (lastD << 2)));
                glEnd();
                glBegin(GL_QUAD_STRIP);
            }

            if (faces->material != curMatl) {
                curMatl = faces->material;
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,
                             (GLfloat *) &matlNoSpecular);
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, 
                             (GLfloat *) &winColors[curMatl]);
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

            if (faces->material != curMatl) {
                curMatl = faces->material;
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,
                             (GLfloat *) &matlNoSpecular);
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, 
                             (GLfloat *) &winColors[curMatl]);
            }

            glNormal3fv((GLfloat *)((char *)pn + aOffs));
            glVertex3fv((GLfloat *)((char *)pp + aOffs));
            glNormal3fv((GLfloat *)((char *)pn + bOffs));
            glVertex3fv((GLfloat *)((char *)pp + bOffs));
        }

        lastC = faces->p[3];
        lastD = faces->p[2];
    }

    if (!bSmoothShading) {
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

    glMaterialfv(GL_FRONT, GL_SPECULAR, (GLfloat *) &matlDimSpecular);

    glBegin(GL_QUADS);

    mesh = &winStreamer[frameNum];

    for (i = 0, faces = mesh->faces; 
         i < mesh->numFaces; i++, faces++) {
        int a, b, c, d;

        a = faces->p[0];
        b = faces->p[1];
        c = faces->p[2];
        d = faces->p[3];

        if (faces->material != curMatl) {
            curMatl = faces->material;
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, 
                         (GLfloat *)&winColors[curMatl]);
        }

        glNormal3fv((GLfloat *)&faces->norm);
        glVertex3fv((GLfloat *)(mesh->pts + a));
        glVertex3fv((GLfloat *)(mesh->pts + b));
        glVertex3fv((GLfloat *)(mesh->pts + c));
        glVertex3fv((GLfloat *)(mesh->pts + d));
    }

    glEnd();

    mxrot += mxrotInc;
    myrot += myrotInc;
    mzrot += mzrotInc;

    if ((myrot < -45.0) || (myrot > 45.0))
        myrotInc = -myrotInc;

    frameNum++;
    if (frameNum >= Frames)
        frameNum = 0;
}
