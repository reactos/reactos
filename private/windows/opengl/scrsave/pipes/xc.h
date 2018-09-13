#ifndef __xc_h__
#define __xc_h__

#include "sscommon.h"


// useful for xc-coords
enum {
    RIGHT = 0,
    TOP,
    LEFT,
    BOTTOM
};

// Cross_section (xc) class

class XC {
public:
    GLfloat     xLeft, xRight;  // bounding box
    GLfloat     yTop, yBottom;
    int         numPts;
    POINT2D     *pts;        // CW points around the xc, from +x

    XC( int numPts );
    XC( const XC& xc );
    XC( XC *xc );
    ~XC();
    void        Scale( float scale );
    float       MaxExtent();
    float       MinTurnRadius( int relDir );
    void        CalcArcACValues90( int dir, float r, float *acPts );
    void        CalcArcACValuesByDistance(  float *acPts );
    void        ConvertPtsZ( POINT3D *pts, float z );
protected:
    void        CalcBoundingBox();
};

// Specific xc's derived from base xc class

class ELLIPTICAL_XC : public XC {
public:
    ELLIPTICAL_XC( float r1, float r2 );
    ~ELLIPTICAL_XC();
private:
    void SetControlPoints( float r1, float r2 );
};

class RANDOM4ARC_XC : public XC {
public:
    RANDOM4ARC_XC( float r );
    ~RANDOM4ARC_XC();
private:
    void SetControlPoints( float radius );
};

#endif __xc_h__
