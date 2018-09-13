/**********************************Module**********************************\
*
* ssflwbox.h
*
* 3D FlowerBox screen saver
* Base header file
*
* History:
*  Wed Jul 19 14:50:27 1995	-by-	Drew Bliss [drewb]
*   Created
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#ifndef __SSFLWBOX_H__
#define __SSFLWBOX_H__

#ifndef PI
#define PI 3.14159265358979323846
#endif

// Minimum and maximum number of side subdivisions
#define MINSUBDIV 2
#define MAXSUBDIV 10

// Maximum values allowed
#define MAXSIDES 8
#define MAXSPTS ((MAXSUBDIV+1)*(MAXSUBDIV+1))
#define MAXPTS (MAXSIDES*MAXSPTS)
#define MAXSFACES (MAXSUBDIV*MAXSUBDIV)
#define MAXFACES (MAXSIDES*MAXSFACES)
#define MAXFPTS 4

// Number of colors used in checkerboarding
#define NCCOLS 2

// Allow floating point type configurability
typedef GLfloat FLT;
typedef struct
{
    FLT x, y, z;
} PT3;

// Configurable options
typedef struct _CONFIG
{
    BOOL smooth_colors;
    BOOL triangle_colors;
    BOOL cycle_colors;
    BOOL spin;
    BOOL bloom;
    int subdiv;
    int color_pick;
    int image_size;
    int geom;
    int two_sided;
} CONFIG;

extern CONFIG config;

extern GLfloat checker_cols[MAXSIDES][NCCOLS][4];
extern GLfloat side_cols[MAXSIDES][4];
extern GLfloat solid_cols[4];

#if defined(assert)
#undef assert
#endif
#if DBG
#define dprintf(args) dprintf_out args
void dprintf_out(char *fmt, ...);
#define assert(e) if (!(e)) assert_failed(__FILE__, __LINE__, #e); else 0
void assert_failed(char *file, int line, char *msg);
#else
#define dprintf(args)
#define assert(e)
#endif

#define DIMA(a) (sizeof(a)/sizeof(a[0]))

/******************************Public*Routine******************************\
*
* Basic vector math macros
*
* History:
*  Wed Jul 19 14:49:49 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#define V3Sub(a, b, r) \
    ((r)->x = (a)->x-(b)->x, (r)->y = (a)->y-(b)->y, (r)->z = (a)->z-(b)->z)
#define V3Add(a, b, r) \
    ((r)->x = (a)->x+(b)->x, (r)->y = (a)->y+(b)->y, (r)->z = (a)->z+(b)->z)
#define V3Cross(a, b, r) \
    ((r)->x = (a)->y*(b)->z-(b)->y*(a)->z,\
     (r)->y = (a)->z*(b)->x-(b)->z*(a)->x,\
     (r)->z = (a)->x*(b)->y-(b)->x*(a)->y)
extern FLT V3Len(PT3 *v);

#endif // __SSFLWBOX_H__
