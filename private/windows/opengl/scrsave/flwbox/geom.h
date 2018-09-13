/**********************************Module**********************************\
*
* geom.h
*
* 3D FlowerBox screen saver
* Geometry header file
*
* History:
*  Wed Jul 19 14:50:27 1995	-by-	Drew Bliss [drewb]
*   Created
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#ifndef __GEOM_H__
#define __GEOM_H__

// Geometry of a shape

// A side of a shape
typedef struct _SIDE
{
    // Number of triangle strips in this side
    int nstrips;
    // Number of vertices per strip
    int *strip_size;
    // Indices for each point in the triangle strips
    unsigned int *strip_index;

    // The number of elements for glDrawElements call
    int num_eles;
    // Index buffer for glDrawElements
    GLuint *dBuf;
} SIDE;

typedef struct _GEOMETRY
{
    void (*init)(struct _GEOMETRY *geom);
    
    // Number of sides
    int nsides;
    // Sides
    SIDE sides[MAXSIDES];

    // Data for each vertex in the shape
    PT3 *pts, *npts;
    PT3 *normals;

    // Total number of vertices
    int total_pts;

    // Scaling control
    FLT min_sf, max_sf, sf_inc;

    // Initial scale factor setup control
    FLT init_sf;

} GEOMETRY;

#define GEOM_CUBE       0
#define GEOM_TETRA      1
#define GEOM_PYRAMIDS   2

extern GEOMETRY *geom_table[];

void InitVlen(GEOMETRY *geom, int npts, PT3 *pts);
void UpdatePts(GEOMETRY *geom, FLT sf);
void DrawGeom(GEOMETRY *geom);
extern void DrawWithVArrays (GEOMETRY *geom);

#endif // __GEOM_H__
