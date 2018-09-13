#ifndef I_CSIMUTIL_HXX_
#define I_CSIMUTIL_HXX_
#pragma INCMSG("--- Beg 'csimutil.hxx'")

#define LINE_SIZE_WIDTH 3
#define CORNER_SIZE 3

#define SHAPE_TYPE_RECT 0
#define SHAPE_TYPE_CIRCLE 1
#define SHAPE_TYPE_POLY 2
#define SELECT_MODE 3

#define DELIMS _T(", ;")

MtExtern(CPointAry)
MtExtern(CPointAry_pv)
DECLARE_CDataAry(CPointAry, POINT, Mt(CPointAry), Mt(CPointAry_pv))

typedef struct
{
    LONG lx, ly, lradius;
} SCircleCoords;

typedef struct
{
    HRGN hPoly;
} SPolyCoords;

union CoordinateUnion
{
    RECT Rect;
    SCircleCoords Circle;
    SPolyCoords Polygon;
};


HRESULT NextNum(LONG *plNum, TCHAR **ppch);

BOOL PointInCircle(POINT pt, LONG lx, LONG ly, LONG lradius);
BOOL Contains(POINT pt, union CoordinateUnion coords, UINT nShapeType);

#pragma INCMSG("--- End 'csimutil.hxx'")
#else
#pragma INCMSG("*** Dup 'csimutil.hxx'")
#endif
