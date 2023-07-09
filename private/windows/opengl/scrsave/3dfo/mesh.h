/******************************Module*Header*******************************\
* Module Name: mesh.h
*
* Declaration of the mesh routines.
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

extern void newMesh(MESH *mesh, int numFaces, int numPts);
extern void delMesh(MESH *mesh);
extern void revolveSurface(MESH *mesh, POINT3D *curve, int steps);
extern void updateObject(MESH *mesh, BOOL bSmooth);
extern void updateObject2(MESH *mesh, BOOL bSmooth);
extern void MakeList(GLuint listID, MESH *mesh);
