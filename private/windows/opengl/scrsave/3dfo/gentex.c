/******************************Module*Header*******************************\
* Module Name: gentex.c
*
* The Textured Flag style of the 3D Flying Objects screen saver.
*
* Texture maps .BMP files onto a simulation of a flag waving in the breeze.
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <stdlib.h>
#include <windows.h>
#include <GL\gl.h>
#include <GL\glu.h>
#include <string.h>
#include <math.h>
#include "ss3dfo.h"

static float winTotalwidth = (float)0.75;
static float winTotalheight = (float)0.75 * (float)0.75;

#define MAX_FRAMES 20

// IPREC is the number of faces in the mesh that models the flag.

#define IPREC   15

static int Frames = 10;
static MESH winMesh[MAX_FRAMES];
static float sinAngle = (float)0.0;
static float xTrans = (float)0.0;
static int curMatl = 0;

// Material properties.

static RGBA matlBrightSpecular = {1.0f, 1.0f, 1.0f, 1.0f};
static RGBA matlDimSpecular    = {0.5f, 0.5f, 0.5f, 1.0f};
static RGBA matlNoSpecular     = {0.0f, 0.0f, 0.0f, 0.0f};

// Lighting properties.

static GLfloat light0Pos[] = {20.0f, 5.0f, 20.0f, 0.0f};
static GLfloat light1Pos[] = {-20.0f, 5.0f, 0.0f, 0.0f};
static RGBA light1Ambient  = {0.0f, 0.0f, 0.0f, 0.0f};
static RGBA light1Diffuse  = {0.4f, 0.4f, 0.4f, 1.0f};
static RGBA light1Specular = {0.0f, 0.0f, 0.0f, 0.0f};

static RGBA flagColors[] = {{1.0f, 1.0f, 1.0f, 1.0f},
                            {0.94f, 0.37f, 0.13f, 1.0f},    // red
                           };

// Default texture resource

static TEX_RES gTexRes = { TEX_BMP, IDB_DEFTEX };

static TEXTURE gTex = {0}; // One global texture

/******************************Public*Routine******************************\
* iPtInList
*
* Add a vertex and its normal to the mesh.  If the vertex already exists,
* add in the normal to the existing normal (we to accumulate the average
* normal at each vertex).  Normalization of the normals is the
* responsibility of the caller.
*
\**************************************************************************/

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

/******************************Public*Routine******************************\
* getZpos
*
* Get the z-position (depth) of the "wavy" flag component at the given x.
*
* The function used to model the wave is:
*
*        1/2
*   z = x    * sin((2*PI*x + sinAngle) / 4)
*
* The shape of the wave varies from frame to frame by changing the
* phase, sinAngle.
*
\**************************************************************************/

float getZpos(float x)
{
    float xAbs = x - xTrans;
    float angle = sinAngle + ((float) (2.0 * PI) * (xAbs / winTotalwidth));

    xAbs = winTotalwidth - xAbs;
//    xAbs += (winTotalwidth / 2.0);

    return (float)((sin((double)angle) / 4.0) *
                   sqrt((double)(xAbs / winTotalwidth )));
}

/******************************Public*Routine******************************\
* genTex
*
* Generate a mesh representing a frame of the flag.  The phase, sinAngle,
* is a global variable.
*
\**************************************************************************/

void genTex(MESH *winMesh)
{
    POINT3D pos;
    POINT3D pts[4];
    float w, h;
    int i;

    newMesh(winMesh, IPREC * IPREC, IPREC * IPREC);

// Width and height of each face

    w = (winTotalwidth) / (float)(IPREC + 1);
    h = winTotalheight;

// Generate the mesh data.  At equally spaced intervals along the x-axis,
// we compute the z-position of the flag surface.

    pos.y = (float) 0.0;
    pos.z = (float) 0.0;

    for (i = 0, pos.x = xTrans; i < IPREC; i++, pos.x += w) {
        int faceCount = winMesh->numFaces;

        pts[0].x = (float)pos.x; 
        pts[0].y = (float)(pos.y);   
        pts[0].z = getZpos(pos.x);

        pts[1].x = (float)pos.x;
        pts[1].y = (float)(pos.y + h);  
        pts[1].z = getZpos(pos.x);

        pts[2].x = (float)(pos.x + w);  
        pts[2].y = (float)(pos.y);  
        pts[2].z = getZpos(pos.x + w);

        pts[3].x = (float)(pos.x + w);
        pts[3].y = (float)(pos.y + h);
        pts[3].z = getZpos(pos.x + w);

    // Compute the face normal.

        ss_calcNorm(&winMesh->faces[faceCount].norm, pts + 2, pts + 1, pts);

    // Add the face to the mesh.

        winMesh->faces[faceCount].material = 0;
        winMesh->faces[faceCount].p[0] = iPtInList(winMesh, 0, pts,
            &winMesh->faces[faceCount].norm, TRUE);
        winMesh->faces[faceCount].p[1] = iPtInList(winMesh, 0, pts + 1,
            &winMesh->faces[faceCount].norm, TRUE);
        winMesh->faces[faceCount].p[2] = iPtInList(winMesh, 0, pts + 2,
            &winMesh->faces[faceCount].norm, TRUE);
        winMesh->faces[faceCount].p[3] = iPtInList(winMesh, 0, pts + 3,
            &winMesh->faces[faceCount].norm, TRUE);

        winMesh->numFaces++;
    }

// Normalize the vertex normals in the mesh.

    ss_normalizeNorms(winMesh->norms, winMesh->numPoints);
}

/******************************Public*Routine******************************\
* initTexScene
*
* Initialize the screen saver.
*
* This function is exported to the main module in ss3dfo.c.
*
\**************************************************************************/

void initTexScene()
{
    int i;
    float angleDelta;
    float aspectRatio;

    // Initialize the transform.

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-0.25, 1.0, -0.25, 1.0, 0.0, 3.0);
    glTranslatef(0.0f, 0.0f, -1.5f);

    // Initialize and turn on lighting.

    glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);

    glLightfv(GL_LIGHT1, GL_AMBIENT, (GLfloat *) &light1Ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, (GLfloat *) &light1Diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, (GLfloat *) &light1Specular);
    glLightfv(GL_LIGHT1, GL_POSITION, light1Pos);
    glEnable(GL_LIGHT1);
    glDisable(GL_DEPTH_TEST);

    // Leave OpenGL in a state ready to accept the model view transform (we
    // are going to have the flag vary its orientation from frame to frame).

    glMatrixMode(GL_MODELVIEW);

    // Define orientation of polygon faces.

    glFrontFace(GL_CW);
    //    glEnable(GL_CULL_FACE);

    Frames = (int)((float)(MAX_FRAMES / 2) * fTesselFact);

    // Load user texture - if that fails load default texture resource

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if( ss_LoadTextureFile( &gTexFile, &gTex ) ||
        ss_LoadTextureResource( &gTexRes, &gTex) )
    {
        glEnable(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        ss_SetTexture( &gTex );

    // Correct aspect ratio of flag to match image.
    //
    // The 1.4 is a correction factor to account for the length of the
    // curve that models the surface ripple of the waving flag.  This
    // factor is the length of the curve at zero phase.  It would be
    // more accurate to determine the length of the curve at each phase,
    // but this is a sufficient approximation for our purposes.

        aspectRatio = ((float) gTex.height / (float) gTex.width)
                      * (float) 1.4;

        if (aspectRatio < (float) 1.0) {
            winTotalwidth  = (float)0.75;
            winTotalheight = winTotalwidth * aspectRatio;
        } else {
            winTotalheight = (float) 0.75;
            winTotalwidth  = winTotalheight / aspectRatio;
        };
    }

    if (Frames < 5)
        Frames = 5;
    if (Frames > MAX_FRAMES)
        Frames = MAX_FRAMES;

    // Generate the geometry data (stored in the array of mesh structures),
    // for each frame of the animation.  The shape of the flag is varied by
    // changing the global variable sinAngle.

    angleDelta = (float)(2.0 * PI) / (float)Frames;
    sinAngle = (float) 0.0;

    for (i = 0; i < Frames; i++) {
        genTex(&winMesh[i]);
        sinAngle += angleDelta;
    }
}

/******************************Public*Routine******************************\
* delTexScene
*
* Cleanup the data associated with this screen saver.
*
* This function is exported to the main module in ss3dfo.c.
*
\**************************************************************************/

void delTexScene()
{
    int i;

    for (i = 0; i < Frames; i++)
        delMesh(&winMesh[i]);

    // Delete the texture
    ss_DeleteTexture( &gTex );
}

/******************************Public*Routine******************************\
* updateTexScene
*
* Generate a scene by taking one of the meshes and rendering it with
* OpenGL.
*
* This function is exported to the main module in ss3dfo.c.
*
\**************************************************************************/

void updateTexScene(int flags)
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
    GLfloat s = (GLfloat) 0.0;
    GLfloat ds;

// In addition to having the flag wave (an effect acheived by switching
// meshes from frame to frame), the flag changes its orientation from
// frame to frame.  This is done by applying a model view transform.

    glLoadIdentity();
    glRotatef((float)mxrot, 1.0f, 0.0f, 0.0f);
    glRotatef((float)myrot, 0.0f, 1.0f, 0.0f);
    glRotatef((float)mzrot, 0.0f, 0.0f, 1.0f);
    
// Divide the texture into IPREC slices.  ds is the texture coordinate
// delta we apply as we move along the x-axis.

    ds = (GLfloat)1.0 / (GLfloat)IPREC;

// Setup the material property of the flag.  The material property, light
// properties, and polygon orientation will interact with the texture.

    curMatl = 0;
//    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, &flagColors[0]);
//    glMaterialfv(GL_FRONT, GL_SPECULAR, &matlBrightSpecular);
//    glMaterialf(GL_FRONT, GL_SHININESS, 60.0);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, (GLfloat *) &flagColors[0]);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (GLfloat *) &matlBrightSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, (float) 60.0);

// Pick the mesh for the current frame.

    mesh = &winMesh[frameNum];

// Take the geometry data is the mesh and convert it to a single OpenGL
// quad strip.  If smooth shading is required, use the vertex normals stored
// in the mesh.  Otherwise, use the face normals.
//
// As we define each vertex, we also define a corresponding vertex and
// texture coordinate.

    glBegin(GL_QUAD_STRIP);

    pp = mesh->pts;
    pn = mesh->norms;

    for (i = 0, faces = mesh->faces, lastC = faces->p[0], lastD = faces->p[1];
         i < mesh->numFaces; i++, faces++) {

        a = faces->p[0];
        b = faces->p[1];

        if (!bSmoothShading) {
            // Since flag is a single quad strip, this isn't needed.
            // But lets keep it in case we ever change to a more
            // complex model (ie., one that uses more than one quad
            // strip).
            #if 0
            if ((a != lastC) || (b != lastD)) {
                glNormal3fv((GLfloat *)&(faces - 1)->norm);

                glTexCoord2f(s, (float) 0.0);
                glVertex3fv((GLfloat *)((char *)pp + 
                            (lastC << 3) + (lastC << 2)));
                glTexCoord2f(s, (float) 1.0);
                glVertex3fv((GLfloat *)((char *)pp + 
                            (lastD << 3) + (lastD << 2)));
                s += ds;
                glEnd();
                glBegin(GL_QUAD_STRIP);
            }
            #endif

            if (faces->material != curMatl) {
                curMatl = faces->material;
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,
                             (GLfloat *) &matlNoSpecular);
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, 
                             (GLfloat *) &flagColors[curMatl]);
            }

            glNormal3fv((GLfloat *)&faces->norm);
            glTexCoord2f(s, (float) 0.0);
            glVertex3fv((GLfloat *)((char *)pp + (a << 3) + (a << 2)));
            glTexCoord2f(s, (float) 1.0);
            glVertex3fv((GLfloat *)((char *)pp + (b << 3) + (b << 2)));
            s += ds;
        } else {
            // Since flag is a single quad strip, this isn't needed.
            // But lets keep it in case we ever change to a more
            // complex model (ie., one that uses more than one quad
            // strip).
            #if 0
            if ((a != lastC) || (b != lastD)) {
                cOffs = (lastC << 3) + (lastC << 2);
                dOffs = (lastD << 3) + (lastD << 2);

                glTexCoord2f(s, (float) 0.0);
                glNormal3fv((GLfloat *)((char *)pn + cOffs));
                glVertex3fv((GLfloat *)((char *)pp + cOffs));
                glTexCoord2f(s, (float) 1.0);
                glNormal3fv((GLfloat *)((char *)pn + dOffs));
                glVertex3fv((GLfloat *)((char *)pp + dOffs));
                s += ds;
                glEnd();
                glBegin(GL_QUAD_STRIP);
            }
            #endif

            aOffs = (a << 3) + (a << 2);
            bOffs = (b << 3) + (b << 2);

            if (faces->material != curMatl) {
                curMatl = faces->material;
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,
                             (GLfloat *) &matlNoSpecular);
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, 
                             (GLfloat *) &flagColors[curMatl]);
            }

            glTexCoord2f(s, (float) 0.0);
            glNormal3fv((GLfloat *)((char *)pn + aOffs));
            glVertex3fv((GLfloat *)((char *)pp + aOffs));
            glTexCoord2f(s, (float) 1.0);
            glNormal3fv((GLfloat *)((char *)pn + bOffs));
            glVertex3fv((GLfloat *)((char *)pp + bOffs));
            s += ds;
        }

        lastC = faces->p[2];
        lastD = faces->p[3];
    }

    if (!bSmoothShading) {
        glNormal3fv((GLfloat *)&(faces - 1)->norm);
        glTexCoord2f(s, (float) 0.0);
        glVertex3fv((GLfloat *)((char *)pp + (lastC << 3) + (lastC << 2)));
        glTexCoord2f(s, (float) 1.0);
        glVertex3fv((GLfloat *)((char *)pp + (lastD << 3) + (lastD << 2)));
    } else {
        cOffs = (lastC << 3) + (lastC << 2);
        dOffs = (lastD << 3) + (lastD << 2);

        glTexCoord2f(s, (float) 0.0);
        glNormal3fv((GLfloat *)((char *)pn + cOffs));
        glVertex3fv((GLfloat *)((char *)pp + cOffs));
        glTexCoord2f(s, (float) 1.0);
        glNormal3fv((GLfloat *)((char *)pn + dOffs));
        glVertex3fv((GLfloat *)((char *)pp + dOffs));
    }

    glEnd();

// Transfer the image to the floating OpenGL window.

// Determine the flag orientation for the next frame.
// What we are doing is an oscillating rotation about the y-axis
// (mxrotInc and mzrotInc are currently 0).

    mxrot += mxrotInc;
    myrot += myrotInc;
    mzrot += mzrotInc;

    if ((myrot < -65.0) || (myrot > 25.0))
        myrotInc = -myrotInc;

    frameNum++;
    if (frameNum >= Frames)
        frameNum = 0;
}
